# ArSonComp M7 — Windows x64 physical test quickstart

These steps are a **physical verification procedure** for publicly released v0.2.0 Stable. The owner reports having tested the product; the template remains useful for collecting detailed host/driver compatibility evidence.
The exact commit is in `BUILD-INFO.txt`; `SHA256SUMS.txt` lists binary hashes.
The compressor adds zero **algorithmic sample** latency, not zero hardware
round-trip/host/driver latency. Do not assume Maono PS22 stability from CI.

## Install

Standalone: launch `Standalone/ArSonComp.exe`. In Audio Settings choose
the real interface and its **native ASIO driver** if available. If no ASIO
driver appears, verify the vendor installer/driver first; WASAPI can be used
for comparison. Start with a headphone-safe volume.

VST3: copy the **entire** `Plugin/ArSonComp.vst3` bundle to
`C:\Program Files\Common Files\VST3\`, then rescan plugins.
Test in ArSonMiX and one independent DAW that supports VST3.

## GUI and operation

Full size is **612×580**, minimum **580×540**, MINI height **320**.
Confirm the tall black-glass dynamics display, transfer curve, IN/GR/OUT
meters, seven rotary knobs, search/preset browser, FILE/Save/Open, A/B and
Bypass. The bottom tabs switch **Dynamics / Sidechain** without changing audio.

The six horizontal controls (Range, Hold, Mix, Link, HP, LP) retain the
user-approved flat silver brushed-metal track and round silver thumb.
M7 makes the visible thumb smaller without reducing its mouse drag area;
caption and fader are slightly lower. The body is matte brushed graphite,
with **flat horizontal grain** rather than wave-like sheen.

Hover/drag any control to see its exact value in the LCD bottom readout.
Double-click any slider/knob to restore the specified factory default.
Test MINI/FULL, minimum/default width, Sidechain and preset browser for
readability and control collision.

## Safe physical listening procedure

1. Start at **48 kHz, 128 samples**, one plugin instance. Keep LISTEN SC
   disabled unless you are intentionally auditioning the detector.
2. Select Clean mode, approximately **2:1–4:1**, set Threshold for
   **3–6 dB gain reduction** on a steady voice. Adjust Makeup conservatively
   to avoid downstream clipping. Compare Active/Bypass at similar loudness.
3. Repeat with vocals, bass, snare/drum transients, speech and a stereo mix.
   Compare Clean / Feedback / Smooth at matched loudness, checking attack,
   recovery, pumping, sibilance, stereo coherence and unwanted distortion.
4. Only after stable 128-sample operation, test **64 then 32** sample buffers.
   Log audio glitches and the **actual host xrun count/callback spikes**,
   not only Task Manager CPU. Repeat with 1 then 8 active instances.

## Isolate crackles: same sample rate, driver, gain and routing

A: Audio interface direct monitoring, no application processing.
B: Host routing with no ArSonComp inserted.
C: Plugin inserted and **host-bypassed** (immediate bit-transparent bypass).
D: Plugin active with fixed parameters.

If A/B/C crackles, investigate driver, host routing, DPC, USB and interface
before blaming compressor DSP. If only D crackles, record the digital input
and processed output, current parameters, buffer, host xrun log and automation.
The plugin's own Active/Bypass control is smoothed, unlike immediate host bypass.

## Reliability and report

Save/reopen a real DAW project and verify state, factory/user preset, A/B,
all automation, MINI/FULL and external mono/stereo sidechain routing.
Disable SC LISTEN again after checking. Run a **60-minute live audio test**,
open/close the editor, observe private-memory growth, actual xrun counters
and any UI stalls. No physical result is declared until recorded.

Optional: right-click `QA/Collect-ArSonComp-Hardware.ps1` and run with
PowerShell (or run directly from a PowerShell terminal). It writes a
**read-only device inventory and a manual checklist** into `QA/Reports/`.
It does NOT measure xruns, audio timing or hardware latency.

The included `Screenshots/` are actual JUCE CI renders;
`benchmark.txt` and `realtime-stress.txt` are machine-dependent synthetic
engine timings, not real DAW latency or glitch measurements.

ASIO SDK distribution licensing must be reviewed before any public
installer. The 46 factory presets, audio DSP, parameter IDs and latency are
unchanged by M7.
