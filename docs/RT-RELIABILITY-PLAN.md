# ArSonComp — RT Reliability and Pro-C Quality Strategy (2026-10-31)

Status: investigation milestone, not proof that physical audio dropouts are fixed.

## Audit findings from the current source
- The hot loop updates 19 parameter ramps every sample and performs log10(detector) and exp(gain-reduction) per channel and sample. These are CPU-cost candidates, not confirmed causes of stutter.
- APVTS parameters are snapped once per block; changed control coefficients are coalesced; the SPSC GUI telemetry queue is bounded and lossy; GUI repaint is message-thread only. Do not discard these existing good practices.
- Engine-level tests already guard process-time allocations. They do not prove no leaks in VST3, editor lifecycle, multiple hosts or long running sessions.
- Earlier GitHub CI averaged around half of one core percent for a stereo instance. Mean throughput alone says nothing about missed host deadlines, worst-case spikes or Windows audio drivers.
- Audio compressor envelope state is causal and serial. Do NOT move live compression to a worker that the realtime callback must wait on: it can introduce queues, unpredictable scheduling, additional latency and new failure modes.

## Reproducible performance measurement
1. Use tests/RealtimeStress.cpp to report mean, P99, P99.9 and max of a simulated block-processing chain (32/64/128 frames; 1/8/24 instances; Peak, RMS+SC and automation). CI results are comparative engine-only wall-clock timing and include OS jitter, not an actual xrun test.
2. On user's Windows hardware, capture standalone and at least two VST3 hosts, including ArSonMiX if available. Document audio device, ASIO/WASAPI driver, sample rate 48/96kHz, buffer 32/64/128, instances 1/8/16, GUI open/closed, OS/DPC latency and actual host xrun counters.
3. Use ETW/WPR/WPA or another sampler to attribute CPU and stalls. Never insert filesystem writes, locks or printf into the process callback.
4. For memory, repeat editor open/close and state reload, then run a 60-minute session with stable instance count. Monitor private bytes, retained allocations and heap diagnostics. Do not label a memory leak without reproducible retained growth.
5. Test candidate optimizations one at a time against a reference waveform: gain and detector conversions, inactive-parameter ramps, peak-only detector path, GUI painting. Preserve state, transient and denormal behavior, automation smoothing and sample partition invariance. Avoid fast-math and unproven LUT shortcuts.

## DSP musical-quality track, separate from realtime optimization
- FabFilter Pro-C 3 documents 14 program-dependent compression styles, character modes, complex sidechain EQ, and optional lookahead/oversampling. Its DSP implementation is proprietary; copying behavior cannot be inferred from the GUI.
- Compare independently designed models using loudness-matched WAVs: vocal, drums, bass and mix bus. Measure compression envelope, overshoot, release recovery, THD/harmonic sidebands, aliasing, pumping, transient crest and dry/wet phase.
- Program-dependent envelopes and true feedback topology need reliable tests before marketing claims. A separate optional lookahead/oversampling quality mode may add latency; preserve the zero-added-sample live path as default.
- No style label counts as an independent algorithm without distinct transient/envelope output and listening validation.

## Engineering acceptance (targets, not achieved results)
- No allocations, deallocations, mutex waits, worker waits, filesystem access or GUI calls from the realtime processing path.
- On the target machine, document P99.9 and maximum audio callback processing times relative to block deadline; target single-instance <=20% of deadline and retain margin for all other mixer plugins. Do not confuse CI benchmark with hardware proof.
- Zero observed xrun during documented 60-minute real-device test at the declared stable buffer, not an unqualified zero-latency promise.
- pluginval, two host projects, state/preset reload, mono/stereo external SC, 32/64/128 frames, DPI and sustained multi-instance lifecycle.
- Match reference recordings by measured loudness and verify audible improvement, not just benchmark improvements.

References: https://steinbergmedia.github.io/vst3_dev_portal/pages/FAQ/Processing.html
https://www.fabfilter.com/help/pro-c/using/styleandcharacter
https://www.fabfilter.com/help/pro-c/using/oversampling
https://www.fabfilter.com/help/pro-c/using/overview
