# Release qualification checklist — ArSonComp

**v0.2.0 Stable has been published following explicit owner confirmation of physical testing.** The previously tested RC installers were promoted byte-for-byte with SHA-256 verification. This is a living compatibility/certification checklist: unchecked items remain genuinely unverified, not claims that Stable publication failed.

## Owner release decision
- [x] Owner explicitly reports product physically tested and approves the v0.2.0 Stable label (2026-11-01)
- [x] Stable release published from the tested RC installer bytes with matching SHA-256 checksums
- [ ] Independent hardware test report specifying host, driver, buffer, measured xruns, and test duration archived

## Build, package and artifact integrity
- [x] Windows x64 VST3 + Standalone builds and CTest
- [x] Linux x86_64 build, smoke, DEB and portable packaging
- [x] macOS universal arm64/x86_64, ad-hoc signing verification, PKG/VST3 ZIP
- [x] Official GitHub Release includes SHA-256 checksums
- [x] Actual JUCE screenshot-based public landing, Github Pages deployment
- [ ] Independently run actual installer/uninstaller on clean machines/VMs for each OS, validate user/all-user permissions, DAW visibility, reinstall/upgrade, data preservation and removal
- [ ] Review JUCE/Steinberg SDK licenses and redistribution rights for public binaries

## Host/audio interoperability
- [ ] Pluginval at agreed strictness and log archived
- [ ] At least two independent DAW hosts on Windows, Linux and macOS as supported, VST3 scan and editor open/close
- [ ] Mono/stereo routing, external sidechain, host automation, bypass, and project state save/reopen
- [ ] Host crash/restart resilience, malformed state/preset inputs, repeated plugin instantiation/removal
- [ ] Audible gain matching, dry/wet blend and matched-level comparisons across models and instruments

## Hardware/realtime
- [ ] Physical interface (Maono PS22 on Windows if available) at 48kHz buffers 128, 64, 32; record exact driver and version
- [ ] A direct / B host no plugin / C host bypass / D plugin active comparison under **identical gain/routing**
- [ ] Host-reported dropouts/xruns and callback timing recorded for 1 and 8 instances (CI benchmark is not a replacement)
- [ ] 60-minute active session with GUI opening/closing, CPU and memory trend, xrun count
- [ ] Explicit PASS/FAIL/NOT TESTED evidence bundle for each OS/host/interface combination

## Public trust
- [x] Readable installation instructions and product screenshots from verifiable CI
- [x] Public issue template, security policy and changelog
- [ ] macOS Developer ID signing and Apple notarization for frictionless Gatekeeper experience
- [ ] Domain ownership / Cloudflare Pages primary-site configuration if replacing GitHub Pages

Public Stable v0.2.0 is already released by owner decision. Complete the remaining unchecked evidence for broader interoperability and independent QA; never overwrite previously published binaries. See [Windows physical test](WINDOWS-QUICKSTART.md), [public release](https://github.com/masarray/arson-comp/releases/tag/v0.2.0), and [release status](RELEASE-STATUS.md).
