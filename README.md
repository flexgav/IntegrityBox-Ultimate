<div align="center">
  <img src="ibu.png" alt="IntegrityBox Ultimate" style="border-radius: 16px; margin-bottom: 20px; box-shadow: 0 10px 30px rgba(0,0,0,0.5); width: 100%; max-width: 800px;">
  <h1>🛡️ IntegrityBox Ultimate</h1>
  <p><b>Advanced Android Environment & Attestation Manager</b></p>
  
  <a href="https://github.com/flexgav/IntegrityBox-Ultimate/releases" target="_blank">
    <img src="https://raw.githubusercontent.com/flexgav/IntegrityBox-Ultimate/main/assets/download.png" alt="Download Button" width="150">
  </a>
</div>

<br>

## 🚀 Overview
**IntegrityBox Ultimate** is a comprehensive, modular toolkit designed for advanced Android environment management, system identity alignment, and deep diagnostics. Built with a focus on performance, security, and a beautiful Material You WebUI, it provides a robust framework for managing hardware-backed attestation components and evading strict system detections.

---

## 📋 Requirements
To utilize the full potential of IntegrityBox Ultimate, ensure you have the following modules installed alongside it:
1. **Tricky Store** or **TEE Simulator** *(Required for hardware attestation)*.
2. **Zygisk Next** or **ReZygisk** *(Optional if using Zygiskless Mode)*.

---

## 🛠️ Installation & Setup
IntegrityBox Ultimate is designed to be "flash and forget", but requires a brief initial setup via its built-in WebUI.
1. **Install** the required modules listed above and reboot.
2. **Flash** IntegrityBox Ultimate via your Root Manager (KernelSU, APatch, or Magisk).
3. **Reboot** your device.
4. **Open the WebUI:** Go to your Root Manager's module list and tap the action/settings button on the IntegrityBox Ultimate module.
5. **Explore:** Use the dashboard to check your current status, set up AutoPilot, or apply necessary patches.

---

## 📚 Built-in Knowledge Base
We have eliminated the need for a massive, hard-to-read README file. **IntegrityBox Ultimate features a comprehensive, interactive Knowledge Base directly inside the WebUI.**

* **💡 Module Guides:** Tap the blinking lightbulb icon at the top of any module page to learn exactly what that specific tool does and how it affects your system.
* **ℹ️ Feature Details:** Tap the `( i )` buttons next to individual sections for in-depth technical explanations of toggles, switches, and colored status badges.
* **🤖 AI Assistant:** An offline, built-in AI Assistant is available in the WebUI to answer common questions and troubleshoot issues interactively.

---

## 💡 Quick Troubleshooting
* **Failing Play Integrity?** Ensure your SELinux is set to `Enforcing`. Permissive mode will cause immediate failures. Check if your hardware keybox is valid or banned.
* **"Device not certified" in Play Store?** If you pass DEVICE/STRONG integrity but still see this error, use the **Certification Fix** module in the WebUI to perform a deep data wipe of Play Services (requires re-login).
* **Banking apps crashing?** Use the **Applist Detector** and **Hide Suspicious Files** modules to mask your root environment (TWRP, Magisk folders) from aggressive anti-cheat scans.

---

## 🤝 Credits & Acknowledgements
This project uses concepts and code from the following amazing open-source work:
- @ez-me for ezme-nodebug.
- @osm0sis for PlayIntegrityFork.
- **LSPosed Team** for Shamiko's late start service script.
- **MeowDump** for the original Integrity-Box foundation.
- **You**, for using this module.
- ☝️ **GOD**, for everything 🙌.