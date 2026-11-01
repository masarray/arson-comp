# Release status — ArSonComp v0.2.0-rc.1

**Public release:** [v0.2.0-rc.1](https://github.com/masarray/arson-comp/releases/tag/v0.2.0-rc.1)  
**Release commit:** `0a039ef6d340e26aa01a62df3670f74da1f03a12`  
**Installer CI:** [Run #41273600039](https://github.com/masarray/arson-comp/actions/runs/41273600039)  
**Most recent GUI screenshots:** [M7 Windows run #41267899861](https://github.com/masarray/arson-comp/actions/runs/41267899861)

## Automated checks completed

- Windows x64 VST3 and Standalone compiled, JUCE/editor and DSP tests passed, and the Inno Setup EXE was produced.
- Ubuntu x86_64 compiled, executed tests under a virtual display, and produced DEB and portable archives.
- macOS arm64 + x86_64 universal build and tests completed, ad-hoc signing and signature verification passed, and universal PKG/VST3 archive were produced.
- Public prerelease assets and SHA-256 checksums were published.

## Evidence still required before a production-stable label

- Verify with pluginval and two independent real VST3 hosts on target operating systems.
- Measure physical-driver callback timing, xruns and sustained performance at 48 kHz / 128, 64 and 32 samples; validate a 60-minute session.
- Record actual Maono PS22 ASIO behavior with bypass/on A/B/C/D isolation, rather than infer it from CPU utilization.
- Validate project restore, external sidechain routing, automation and editor open/close in installed hosts.
- Conduct loudness-matched audio quality comparisons; no verified FabFilter sonic equivalence.
- Review JUCE/ASIO SDK redistribution terms; Apple Developer ID and notarization remain needed for frictionless macOS installation.

**Public availability and successful CI are not the same as production-stable audio certification.** This version is intentionally labelled a *Release Candidate* until physical and host-level gates are complete.

[Installation guide](INSTALL-PUBLIC.md) · [Windows physical test](WINDOWS-QUICKSTART.md) · [Validation archive](VALIDATION.md).