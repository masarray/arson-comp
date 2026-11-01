# Changelog

This file summarizes meaningful **user-facing** milestones. It does not imply that the current public release is production-stable.

## [Unreleased] — documentation and web readiness

- Publish a responsive, accessible installer download website for Windows, Linux and macOS on GitHub Pages, with Cloudflare Pages-ready static output.
- Use five verified screenshots from the actual Windows JUCE editor.
- Add a professional product README, contribution templates and clear Release Candidate notices.
- Improve onboarding, privacy/support transparency, security reporting and website integrity checks. These updates do **not** rebuild or change the published v0.2.0-rc.1 audio binaries.

## [0.2.0-rc.1] — 2026-11-01

First public installable **Release Candidate**; [release assets and SHA-256 manifest](https://github.com/masarray/arson-comp/releases/tag/v0.2.0-rc.1).

- Windows x64 VST3 + Standalone via Inno Setup.
- Linux x86_64 DEB installer plus per-user portable archive.
- macOS universal VST3/Standalone PKG with ad-hoc bundle signing (installer **not notarized**).
- Original C++17/JUCE audio compressor with Clean, Feedback and Smooth modes; 46 factory preset starting points.
- Compact full/MINI/Sidechain UI with readable live transfer graph, meters, actual silver brushed-metal slider imagery and matte graphite chassis.
- Automated three-platform build and smoke tests. **Physical audio-interface compatibility, xrun stability, pluginval, installer signing and licensing checks remain unverified release gates.**

## Historical engineering milestones (before first public release)

- **M1–M4:** Compressor DSP, parameter/host integration, realtime safety, standalone ASIO path and audio workflow.
- **M5–M6:** Factory preset calibration and compact GUI refinements.
- **M6.1–M6.3:** Knob clearance and user-approved independent PNG track/thumb slider control.
- **M7:** GUI material polish, smaller silver fader caps, and Windows physical-test kit.

The authoritative long-form history is in [engineering validation](docs/VALIDATION.md) and [release status](docs/RELEASE-STATUS.md).

[0.2.0-rc.1]: https://github.com/masarray/arson-comp/releases/tag/v0.2.0-rc.1
