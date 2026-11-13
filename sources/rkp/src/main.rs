// IntegrityBox's device issuer. Only the pinned Device RKP feature is registered.
use duck_core::{AppPaths, CommandFailure, CommandResult, Context, Feature, Sysroot, envelope};
use serde_json::{Value, json};
use std::{ffi::OsString, path::Path};

fn cli() -> clap::Command {
    clap::Command::new("ib-rkp")
        .version(env!("CARGO_PKG_VERSION"))
        .subcommand_required(true)
        .subcommand(duck_rkp_device::FEATURE.command())
}

fn methods(status: &Value) -> Vec<&'static str> {
    if status["prerequisites_met"].as_bool() != Some(true) { return Vec::new(); }
    [("v2", "v2_available"), ("v1", "v1_available"), ("v1-native", "v1_native_available")]
        .into_iter().filter_map(|(method, flag)| (status[flag].as_bool() == Some(true)).then_some(method)).collect()
}

// The policy owns the servicemanager/VINTF probe. Its absence excludes only AIDL;
// unknown (including a declared lazy service) still permits an actual AIDL call.
fn adapt_status(status: &mut Value, rpc: &str) {
    let rpc = match rpc { "present" => "present", "absent" => "absent", _ => "unknown" };
    let original_aidl = status["v1_available"].as_bool() == Some(true);
    if rpc == "absent" { status["v1_available"] = json!(false); }
    let availability: Vec<Value> = [
        ("v2", "v2_available"), ("v1", "v1_available"), ("v1-native", "v1_native_available")
    ].into_iter().map(|(method, flag)| {
        let available = status[flag].as_bool() == Some(true);
        let reason = if available { "" } else if method == "v1" && original_aidl && rpc == "absent" {
            "rkp_hal_missing"
        } else { "method_components_missing" };
        json!({"method":method,"available":available,"reason":reason})
    }).collect();
    status["system_rpc"] = json!(rpc);
    status["method_availability"] = json!(availability);
    if status["prerequisites_met"].as_bool() == Some(true) && methods(status).is_empty() {
        status["prerequisites_met"] = json!(false);
        status["blockers"] = json!(["no_rkp_methods"]);
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
enum FailureKind { Device, DeviceTemporary, Network, Server, Local, Uncertain }
impl FailureKind {
    fn name(self) -> &'static str {
        match self {
            Self::Device => "device_incompatible",
            Self::DeviceTemporary => "device_temporary",
            Self::Network => "network",
            Self::Server => "server",
            Self::Local => "local",
            Self::Uncertain => "uncertain",
        }
    }
}

// The pinned upstream loses the numeric status when converting its typed errors
// to CommandFailure. Normalize only its exact, code-bound error formats here.
// Unknown text stays uncertain; UI and shell never interpret error messages.
fn native_status(code: &str, message: &str) -> Option<i32> {
    let prefix = match code {
        "ta_rejected" => "KeyMint rejected the CSR request with status ",
        "ta_transport_error" => "KeyMint transport failed: send_cmd returned ",
        _ => return None,
    };
    message.strip_prefix(prefix)?.parse().ok()
}
fn failure_kind(method: &str, code: &str, message: &str) -> FailureKind {
    use FailureKind::*;
    match code {
        "unsupported_device" => Device,
        "ta_rejected" => match native_status(code, message) {
            // Keymaster: resource pressure, cancellation, contention, hardware
            // communication failure and rate/operation limits are retryable.
            Some(-31 | -41 | -46 | -47 | -48 | -49 | -54 | -56) => DeviceTemporary,
            Some(-1000) => Uncertain,
            Some(status) if status < 0 => Device,
            Some(_) => Uncertain,
            None => Uncertain,
        },
        "ta_transport_error" if method != "v1" => {
            match native_status(code, message) {
                // Native transport errno values are distinct from Keymaster errors.
                Some(-11 | -16 | -110 | -31 | -41 | -46 | -47 | -48 | -49 | -54 | -56) => DeviceTemporary,
                Some(-1 | -2 | -6 | -19 | -22 | -38 | -95) => Device,
                Some(_) => Uncertain,
                None => Uncertain,
            }
        }
        "ta_timeout" => DeviceTemporary,
        "ta_transport_error" => Uncertain,
        "challenge_unavailable" | "network_error" => Network,
        "rkp_client_error" | "rkp_server_error" | "device_not_registered" => Server,
        "internal_error" | "acknowledgement_required" => Local,
        _ => Uncertain,
    }
}
fn retry_method(code: &str) -> bool {
    !matches!(code, "internal_error" | "submission_unknown" | "acknowledgement_required")
}
fn final_kind(failures: &[FailureKind]) -> FailureKind {
    use FailureKind::*;
    if !failures.is_empty() && failures.iter().all(|kind| *kind == Device) { return Device; }
    // Any server/network/uncertain result prevents a hardware-incompatibility verdict.
    for kind in [Network, Server, Uncertain, Local, DeviceTemporary] {
        if failures.contains(&kind) { return kind; }
    }
    Uncertain
}
fn rpc_state() -> String {
    std::env::var("IB_RKP_SYSTEM_RPC").unwrap_or_else(|_| "unknown".into())
}

async fn run_with_feature(feature: &dyn Feature, args: &clap::ArgMatches, ctx: &Context, rpc: &str) -> CommandResult {
    if matches!(args.subcommand(), Some(("status", _))) {
        let mut result = feature.run(args, ctx).await?;
        adapt_status(&mut result.data, rpc);
        return Ok(result);
    }
    let Some(("provision", provision)) = args.subcommand() else { return feature.run(args, ctx).await; };
    let auto = provision.get_raw("protocol").and_then(|mut values| values.next()).and_then(|s| s.to_str()) == Some("auto");
    if !auto || !provision.get_flag("acknowledge_limitations") { return feature.run(args, ctx).await; }
    let status_args = cli().get_matches_from(["ib-rkp", "rkp-device", "status"]);
    let mut status = feature.run(status_args.subcommand().unwrap().1, ctx).await?;
    adapt_status(&mut status.data, rpc);
    let eligible = methods(&status.data);
    if eligible.is_empty() {
        return Err(CommandFailure {
            command:"rkp-device.provision",code:"unsupported_device",
            message:"No device RKP method meets its prerequisites".into(),
            details:Some(json!({"failure_kind":"device_incompatible","device_incompatible":true,
                "methods_checked":true,"attempts":[],"method_availability":status.data["method_availability"]})),
        });
    }
    let mut attempts = Vec::new();
    let mut failures = Vec::new();
    for method in eligible {
        let mut argv: Vec<OsString> = ["ib-rkp", "rkp-device", "provision", "--protocol", method, "--acknowledge-limitations"].into_iter().map(OsString::from).collect();
        for (id, flag) in [("fingerprint", "--fingerprint"), ("server_url", "--server-url"), ("output", "--output")] {
            if let Some(value) = provision.get_raw(id).and_then(|mut values| values.next()) {
                argv.extend([OsString::from(flag), value.to_owned()]);
            }
        }
        let selected = cli().get_matches_from(argv);
        match feature.run(selected.subcommand().unwrap().1, ctx).await {
            Ok(mut output) => {
                attempts.push(json!({"method":method,"error":null}));
                output.data["attempts"] = json!(attempts);
                output.data["method_availability"] = status.data["method_availability"].clone();
                return Ok(output);
            }
            Err(mut failure) => {
                let kind = failure_kind(method, failure.code, &failure.message);
                attempts.push(json!({"method":method,"error":{"code":failure.code,
                    "kind":kind.name(),"native_status":native_status(failure.code,&failure.message)}}));
                if !retry_method(failure.code) {
                    failure.details = Some(json!({"attempts":attempts,"failure_kind":kind.name(),
                        "device_incompatible":false,"methods_checked":false,
                        "method_availability":status.data["method_availability"]}));
                    return Err(failure);
                }
                failures.push(kind);
            }
        }
    }
    let kind = final_kind(&failures);
    Err(CommandFailure {
        command:"rkp-device.provision",code:"all_methods_failed",
        message:"All eligible device RKP methods failed".into(),
        details:Some(json!({"attempts":attempts,"failure_kind":kind.name(),
            "device_incompatible":kind==FailureKind::Device,"methods_checked":true,
            "method_availability":status.data["method_availability"]})),
    })
}

async fn run(args: &clap::ArgMatches, ctx: &Context) -> CommandResult {
    run_with_feature(&duck_rkp_device::FEATURE, args, ctx, &rpc_state()).await
}

#[tokio::main]
async fn main() {
    let root = std::env::var("MODPATH").unwrap_or_else(|_| "/data/adb/modules/playintegrityfix".into());
    let data = std::env::var("IB_RKP_DATA").unwrap_or_else(|_| "/data/adb/Box-Brain/RKP/engine".into());
    let ctx = Context {
        paths: AppPaths::for_root(Path::new(&root), Path::new(&data)),
        sysroot: Sysroot::new("/"),
    };
    let matches = cli().get_matches();
    let (_, args) = matches.subcommand().expect("subcommand required");
    match run(args, &ctx).await {
        Ok(mut result) => {
            if let Some(data) = result.data.as_object_mut() {
                data.remove("keybox_xml"); data.remove("challenge_hex");
            }
            envelope::emit(&envelope::success(&result));
        }
        Err(error) => { envelope::emit(&envelope::failure(&error)); std::process::exit(1); }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::{collections::VecDeque, sync::Mutex};

    struct Issuer {
        results: Mutex<VecDeque<CommandResult>>,
        calls: Mutex<Vec<String>>,
    }
    impl Feature for Issuer {
        fn info(&self) -> duck_core::FeatureInfo { duck_rkp_device::FEATURE.info() }
        fn command(&self) -> clap::Command { duck_rkp_device::FEATURE.command() }
        fn run<'a>(&'a self, args: &'a clap::ArgMatches, _: &'a Context) -> duck_core::BoxFuture<'a, CommandResult> {
            Box::pin(async move {
                if let Some(("status", _)) = args.subcommand() {
                    return Ok(duck_core::CommandOutput { command:"rkp-device.status", data:capabilities() });
                }
                let (_, provision) = args.subcommand().unwrap();
                let protocol = provision.get_raw("protocol").unwrap().next().unwrap().to_str().unwrap();
                self.calls.lock().unwrap().push(protocol.to_owned());
                self.results.lock().unwrap().pop_front().expect("unexpected issuance")
            })
        }
    }
    fn refusal(code: &'static str, message: &str) -> CommandResult {
        Err(CommandFailure { command:"rkp-device.provision",code,message:message.into(),details:None })
    }
    async fn exercise(results: Vec<CommandResult>, rpc: &str) -> (CommandResult, Vec<String>) {
        let issuer = Issuer { results:Mutex::new(results.into()),calls:Mutex::new(Vec::new()) };
        let ctx = Context { paths:AppPaths::for_root(Path::new("."),Path::new(".")),sysroot:Sysroot::new("/") };
        let selected = cli().get_matches_from(["ib-rkp","rkp-device","provision","--protocol","auto","--acknowledge-limitations"]);
        let result = run_with_feature(&issuer, selected.subcommand().unwrap().1, &ctx, rpc).await;
        (result, issuer.calls.into_inner().unwrap())
    }
    #[tokio::test]
    async fn native_fallback_can_succeed_without_system_rpc() {
        let (result,calls) = exercise(vec![
            refusal("ta_rejected","KeyMint rejected the CSR request with status -18"),
            Ok(duck_core::CommandOutput { command:"rkp-device.provision",data:json!({"csr_backend":"native"}) }),
        ],"absent").await;
        let output = result.unwrap();
        assert_eq!(calls, ["v2","v1-native"]);
        assert_eq!(output.data["attempts"][0]["error"]["kind"], "device_incompatible");
        assert_eq!(output.data["attempts"][1]["error"], Value::Null);
    }
    #[tokio::test]
    async fn every_eligible_method_is_tried_before_device_verdict() {
        let (result,calls) = exercise(vec![
            refusal("ta_rejected","KeyMint rejected the CSR request with status -18"),
            refusal("unsupported_device","AIDL cannot issue on this device"),
            refusal("ta_transport_error","KeyMint transport failed: send_cmd returned -1"),
        ],"present").await;
        assert_eq!(calls, ["v2","v1","v1-native"]);
        let details = result.unwrap_err().details.unwrap();
        assert_eq!(details["device_incompatible"], true);
        assert_eq!(details["methods_checked"], true);
    }
    #[tokio::test]
    async fn server_failure_and_unknown_submission_never_establish_incompatibility() {
        let (result,calls) = exercise(vec![
            refusal("ta_rejected","KeyMint rejected the CSR request with status -18"),
            refusal("rkp_server_error","unavailable"),
            refusal("ta_transport_error","KeyMint transport failed: send_cmd returned -1"),
        ],"present").await;
        assert_eq!(calls.len(),3);
        let details = result.unwrap_err().details.unwrap();
        assert_eq!(details["failure_kind"], "server");
        assert_eq!(details["device_incompatible"], false);
        let (result,calls) = exercise(vec![refusal("submission_unknown","unconfirmed submission")],"present").await;
        assert_eq!(calls, ["v2"]);
        let details = result.unwrap_err().details.unwrap();
        assert_eq!(details["methods_checked"], false);
        assert_eq!(details["device_incompatible"], false);
    }
    fn capabilities() -> Value {
        json!({"prerequisites_met":true,"v2_available":true,"v1_available":true,"v1_native_available":true})
    }
    #[test]
    fn absent_rpc_excludes_only_aidl_and_preserves_native_methods() {
        let mut status=capabilities(); adapt_status(&mut status,"absent");
        assert_eq!(methods(&status),["v2","v1-native"]);
        assert_eq!(status["method_availability"][1]["reason"],"rkp_hal_missing");
    }
    #[test]
    fn unknown_lazy_rpc_keeps_aidl_eligible() {
        let mut status=capabilities();adapt_status(&mut status,"unknown");
        assert_eq!(methods(&status),["v2","v1","v1-native"]);
    }
    #[test]
    fn no_adapter_is_an_explicit_prerequisite_blocker() {
        let mut status=json!({"prerequisites_met":true,"v1_available":true});
        adapt_status(&mut status,"absent");
        assert_eq!(status["prerequisites_met"],false);assert!(methods(&status).is_empty());
    }
    #[test]
    fn busy_device_and_network_never_prove_incompatibility() {
        for code in [-31,-41,-46,-47,-48,-49,-54,-56] {
            assert_eq!(failure_kind("v2","ta_rejected",&format!("KeyMint rejected the CSR request with status {code}")),FailureKind::DeviceTemporary);
        }
        for code in [-11,-16,-110] {
            assert_eq!(failure_kind("v2","ta_transport_error",&format!("KeyMint transport failed: send_cmd returned {code}")),FailureKind::DeviceTemporary);
        }
        assert_eq!(failure_kind("v2","ta_rejected","KeyMint rejected the CSR request with status -1000"),FailureKind::Uncertain);
        assert_eq!(failure_kind("v2","ta_transport_error","KeyMint transport failed: send_cmd returned -999"),FailureKind::Uncertain);
        assert_eq!(final_kind(&[FailureKind::Device,FailureKind::Network]),FailureKind::Network);
        assert_eq!(final_kind(&[FailureKind::Device,FailureKind::Uncertain]),FailureKind::Uncertain);
        assert_ne!(final_kind(&[]),FailureKind::Device);
    }
    #[test]
    fn legacy_local_vendor_failures_are_separate_from_server_errors() {
        let v2=failure_kind("v2","ta_rejected","KeyMint rejected the CSR request with status -18");
        let v1=failure_kind("v1-native","ta_transport_error","KeyMint transport failed: send_cmd returned -1");
        assert_eq!(final_kind(&[v2,v1]),FailureKind::Device);
        assert_eq!(failure_kind("v2","challenge_unavailable","offline"),FailureKind::Network);
        assert_eq!(failure_kind("v2","rkp_server_error","503"),FailureKind::Server);
    }
    #[test]
    fn unknown_vendor_error_formats_are_not_hardware_verdicts() {
        assert_eq!(failure_kind("v2","ta_rejected","unrecognized format"),FailureKind::Uncertain);
        assert_eq!(failure_kind("v1","ta_transport_error","bridge timeout"),FailureKind::Uncertain);
    }
    #[test]
    fn local_publication_and_unknown_submissions_never_repeat_issuance() {
        for code in ["internal_error","submission_unknown","acknowledgement_required"] { assert!(!retry_method(code)); }
        assert!(retry_method("ta_rejected"));cli().debug_assert();
    }
}
