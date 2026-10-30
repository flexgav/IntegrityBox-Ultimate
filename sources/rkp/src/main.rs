//! IntegrityBox's device issuer. Only the pinned Device RKP feature is registered.
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

fn all_methods_refused(codes: &[&str]) -> bool {
    !codes.is_empty() && codes.iter().all(|code| matches!(*code, "unsupported_device" | "ta_rejected"))
}

fn retry_method(code: &str) -> bool {
    // A local publication error must not issue another key; an unknown server
    // submission must not trigger another method in the same operation either.
    !matches!(code, "internal_error" | "submission_unknown" | "acknowledgement_required")
}

async fn run(args: &clap::ArgMatches, ctx: &Context) -> CommandResult {
    let feature = &duck_rkp_device::FEATURE;
    let Some(("provision", provision)) = args.subcommand() else { return feature.run(args, ctx).await; };
    let auto = provision.get_raw("protocol").and_then(|mut values| values.next()).and_then(|s| s.to_str()) == Some("auto");
    if !auto || !provision.get_flag("acknowledge_limitations") { return feature.run(args, ctx).await; }
    let status_args = cli().get_matches_from(["ib-rkp", "rkp-device", "status"]);
    let status = feature.run(status_args.subcommand().unwrap().1, ctx).await?;
    let eligible = methods(&status.data);
    if eligible.is_empty() { return feature.run(args, ctx).await; }
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
                output.data["attempts"] = Value::Array(attempts);
                return Ok(output);
            }
            Err(mut failure) => {
                attempts.push(json!({"method":method,"error":{"code":failure.code,"message":failure.message}}));
                if !retry_method(failure.code) {
                    failure.details = Some(json!({"attempts":attempts}));
                    return Err(failure);
                }
                failures.push(failure.code);
            }
        }
    }
    Err(CommandFailure {
        command: "rkp-device.provision", code: "all_methods_failed",
        message: "All eligible device RKP methods failed".into(),
        details: Some(json!({"attempts":attempts,"device_refused":all_methods_refused(&failures)})),
    })
}

#[tokio::main]
async fn main() {
    // Runtime ownership is explicit. Do not discover another module's data or honor
    // its test sysroot, profile, signing materials, endpoints or workbench modes.
    let root = std::env::var("MODPATH")
        .unwrap_or_else(|_| "/data/adb/modules/playintegrityfix".into());
    let data = std::env::var("IB_RKP_DATA")
        .unwrap_or_else(|_| "/data/adb/Box-Brain/RKP/engine".into());
    let ctx = Context {
        paths: AppPaths::for_root(Path::new(&root), Path::new(&data)),
        sysroot: Sysroot::new("/"),
    };
    let matches = cli().get_matches();
    let (_, args) = matches.subcommand().expect("subcommand required");
    match run(args, &ctx).await {
        Ok(mut result) => {
            // Provisioning artifacts stay private files, never UI/job JSON.
            if let Some(data) = result.data.as_object_mut() {
                data.remove("keybox_xml");
                data.remove("challenge_hex");
            }
            envelope::emit(&envelope::success(&result));
        }
        Err(error) => {
            envelope::emit(&envelope::failure(&error));
            std::process::exit(1);
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn only_prerequisite_eligible_adapters_are_queued_in_order() {
        assert_eq!(methods(&json!({"prerequisites_met":true,"v2_available":true,"v1_available":true,"v1_native_available":true})), ["v2","v1","v1-native"]);
        assert_eq!(methods(&json!({"prerequisites_met":true,"v1_available":true})), ["v1"]);
        assert!(methods(&json!({"prerequisites_met":false,"v2_available":true})).is_empty());
    }
    #[test]
    fn transport_or_network_errors_do_not_blacklist_firmware() {
        assert!(all_methods_refused(&["ta_rejected","unsupported_device"]));
        assert!(!all_methods_refused(&["ta_rejected","challenge_unavailable"]));
        assert!(!all_methods_refused(&["ta_transport_error"]));
        assert!(!all_methods_refused(&[]));
    }
    #[test]
    fn local_publication_and_unknown_submission_never_repeat_issuance() {
        assert!(!retry_method("internal_error"));
        assert!(!retry_method("submission_unknown"));
        assert!(retry_method("ta_rejected"));
    }
    #[test]
    fn worker_and_acknowledgement_contracts_remain_registered() {
        cli().debug_assert();
        let args=cli().get_matches_from(["ib-rkp","rkp-device","provision","--protocol","auto","--acknowledge-limitations"]);
        assert!(args.subcommand().unwrap().1.subcommand().unwrap().1.get_flag("acknowledge_limitations"));
        assert!(cli().try_get_matches_from(["ib-rkp","rkp","provision"]).is_err());
    }
}
