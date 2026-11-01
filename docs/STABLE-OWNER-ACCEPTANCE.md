# v0.2.0 Stable — owner acceptance and provenance

- **Owner instruction (2026-11-01):** ArSonComp has already been physically tested; release Stable publicly.
- **Publication:** [v0.2.0 Stable](https://github.com/masarray/arson-comp/releases/tag/v0.2.0), not draft and not prerelease.
- **Source commit:** `0a039ef6d340e26aa01a62df3670f74da1f03a12` (matches RC and distributed binary source).
- **Build/installer CI:** [Run #41273600039](https://github.com/masarray/arson-comp/actions/runs/41273600039) on Windows, Linux and macOS.
- **Stable promotion:** [Run #41286912535](https://github.com/masarray/arson-comp/actions/runs/41286912535) reused **exact five RC installers**; original SHA-256 manifest checked before publication.
- **Public website:** [ArSonComp](https://masarray.github.io/arson-comp/), now pointing to Stable files.

This is an **owner acceptance record**, not an independent hardware certification. Detailed physical test configuration and measured xruns were not provided in the publication instruction. macOS Apple notarization, broad third-party DAW testing, and SDK redistribution review remain documented follow-ups. Do not overstate compatibility.

This release does not overwrite the Release Candidate. Website/docs changes on `main` do not alter the source tag or the five published installer binaries.
