# Release status — ArSonComp v0.2.0 Stable

**Current public release:** [v0.2.0 Stable](https://github.com/masarray/arson-comp/releases/tag/v0.2.0)  
**Source tag / verified binary commit:** `0a039ef6d340e26aa01a62df3670f74da1f03a12`  
**Cross-platform build and installer CI:** [Run #41273600039](https://github.com/masarray/arson-comp/actions/runs/41273600039)  
**Owner-authorized byte-identical RC → Stable promotion:** [Run #41286912535](https://github.com/masarray/arson-comp/actions/runs/41286912535)  
**GUI screenshot provenance:** [M7 Windows run #41267899861](https://github.com/masarray/arson-comp/actions/runs/41267899861)

## Release status: Stable by owner acceptance

The product owner explicitly confirmed that ArSonComp had been physically tested and directed publication as a Stable release. Stable packaging reuses the **same five installer binaries** as v0.2.0-rc.1; all package hashes match the earlier manifest. The release tag points to the original code commit that produced these binaries; latest website/documentation continues on `main`.

This records the owner's release decision. It does **not** imply that we independently observed hardware xrun measurements or certified every third-party DAW.

## Verified automated distribution

- Windows 10/11 x64: VST3/Standalone and Inno Setup.
- Ubuntu/Debian x86_64: DEB, plus portable Linux x86_64.
- macOS Intel and Apple Silicon: universal application/VST3, ad-hoc signed PKG and VST3 ZIP (PKG **not Apple notarized**).
- Smoke tests and automated build/package CI passed on all three platforms; release carries a SHA-256 manifest.

## Known limitations and follow-up

- macOS Gatekeeper may warn because the installer is unsigned/not notarized; do not disable Gatekeeper globally.
- Third-party pluginval, detailed DAW interoperability, documented physical driver/xrun logs, and licensing/redistribution review remain follow-up evidence items, not claims of completed independent certification.
- Commercial compressor sonic parity is not claimed; default zero **algorithmic sample** latency does not mean zero hardware/driver round-trip time.
- [Ongoing compatibility and release QA](https://github.com/masarray/arson-comp/issues/18) remains open for evidence-based improvements.

[Install guide](INSTALL-PUBLIC.md) · [QA checklist](RELEASE-CHECKLIST.md) · [Windows physical test procedure](WINDOWS-QUICKSTART.md).
