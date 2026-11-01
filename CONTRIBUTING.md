# Contributing to ArSonComp

We welcome focused fixes, reproducible issues, compatibility improvements and evidence-based audio engineering contributions.

## Before opening an issue

Check existing [issues](https://github.com/masarray/arson-comp/issues) and [pull requests](https://github.com/masarray/arson-comp/pulls). Audio bug reports should include host and version, OS, VST3/Standalone, driver, interface, sample rate, buffer length, routing, settings, reproducible steps and actual xrun/audio results. Attach non-sensitive audio recordings or host logs when possible. Visual reports should include actual application screenshots at default/minimum DPI.

## Engineering expectations

- Keep the C++17 DSP in `engine/` independent of JUCE; `plugin/` adapts host IO, parameters and GUI.
- Do not allocate, lock, wait, open files or touch GUI from the real-time processing callback.
- Preserve existing APVTS parameter IDs, host recall and zero additional algorithmic sample latency in the default path.
- Changes to compression quality require deterministic numerical tests, loudness-matched listening, and reproducible benchmarks rather than undocumented similarity claims.
- UI work must verify actual JUCE render screenshots; favor compact, legible, responsive, professional interfaces, not design mockups in place of real software.
- Do not assert glitch-free hardware/ASIO behavior from synthetic CI engine tests.

## Validation

For an initial engine-only build:

```sh
cmake -S . -B build -DARSON_BUILD_PLUGIN=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
```

GUI changes require VST3/Standalone smoke and screenshot review. Keep pull requests cohesive, explain user-facing benefits and identify remaining manual checks.

[Architecture](docs/ARD.md) · [Release gates](docs/RELEASE-STATUS.md) · [Engineering contract](AGENTS.md) · [AGPL-3.0](LICENSE).