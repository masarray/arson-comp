# ArSonComp Factory Bank v2 — Original presets with calibrated starting makeup

46 original purpose-built compressor starting points in 10 categories. Every factory program now includes a positive **makeup gain** intended to make the effect audible rather than sounding merely quieter than bypass.

## Clean
- **Neutral Start** — A balanced starting point. Adjust Threshold for the desired gain reduction.
- **Transparent Control** — Low-ratio, wide-knee compression for natural level regulation.
- **Peak Catch** — Fast peak containment with limited reduction; not a true-peak limiter.
- **Parallel Control** — Parallel compression that retains part of the unprocessed dynamics.

## Vocals
- **Natural Vocal** — Gentle vocal leveling that preserves breath and consonants.
- **Forward Lead** — Assertive lead vocal control without an excessively fast attack.
- **Broadcast Voice** — Consistent spoken-word loudness with program-dependent release.
- **Intimate Vocal** — Soft program leveling for quiet intimate performances.
- **Rock Lead** — Firm vocal dynamics for dense guitar-driven arrangements.
- **Backing Blend** — Gentle glue for stacked backing vocals and harmonies.

## Drums
- **Kick Weight** — Maintain kick impact while smoothing excessive body resonance.
- **Snare Snap** — Retain snare crack with a medium attack and lively release.
- **Parallel Smash** — Dense parallel drum energy, dry/wet blended for punch.
- **Room Pump** — Pronounced room-compressor movement for deliberate effect.
- **Drum Bus Glue** — Moderate drum-bus cohesion with transient headroom.
- **Percussion Tame** — Control uneven percussion peaks without flattening expression.

## Bass
- **Steady Bass** — Control low-note variation and sustain with smooth recovery.
- **Bass Attack** — Preserve front-edge impact with medium attack and release.
- **Synth Bass Control** — Dense synth bass level containment with a smooth knee.
- **Slap Peak Tame** — Control slap and pop spikes while retaining some articulation.

## Guitar
- **Acoustic Gentle** — Smooth acoustic strumming dynamics without pumping.
- **Electric Rhythm** — Subtle rhythm guitar leveling for consistent mixing.
- **Lead Sustain** — Extend lead-line sustain with more assertive compression.
- **Clean Picking** — Keep picked notes even and articulate.

## Piano
- **Piano Natural** — Very gentle dynamics for exposed acoustic piano.
- **Pop Piano** — Even pop piano levels while keeping chord attacks lively.
- **Electric Keys** — Controlled sustain for modern electric keyboards.
- **Percussive Keys** — Let piano transients speak through moderate attack.

## Mixbus
- **Gentle Glue** — 2:1 stereo bus glue with conservative reduction.
- **Forward Mix** — Tighter mix cohesion without crushing the transients.
- **Parallel Mix** — Blend denser bus energy underneath the original.
- **Smooth Program** — Smooth overall dynamic movement for changing material.

## Mastering
- **Gentle Program** — Subtle final program control; start with only 1–2 dB reduction.
- **Dynamic Preserve** — Protect program transients and limit gain reduction.
- **Density Lift** — Moderate parallel density; not a brickwall limiter.
- **Slow Program** — Slow RMS-based envelope for wide program changes.

## Special
- **Rhythmic Pump** — Audible rhythmic compression for electronic sources.
- **Hard Clamp** — Aggressive peak reduction; reduce input if distortion appears.
- **Slow Breathing** — Slow rise and recovery for dramatic ambience.
- **NY Parallel** — Dense parallel compression for energetic drums.

## Live & Broadcast
- **Live Speech** — Low-latency microphone dynamics. No added plugin samples.
- **Live Vocal** — Performance vocal control with conservative artifacts.
- **Podcast Voice** — Natural spoken-word consistency; monitor for pumping.
- **Streaming Bus** — Gentle stereo bus level management for online streaming.
- **Stage Instrument** — Transient-preserving compression for a stage instrument.
- **Radio Presenter** — Firm broadcast speech dynamics with smooth release.

This bank draws inspiration from the **workflow** of modern professional compressor browsers (categorized presets, search, favorites and descriptions). It is not an import or reproduction of any FabFilter factory .ffp files, DSP algorithms, settings or branding. The styles and settings target ArSonComp's independent Clean, Feedback and Smooth compressor engine.

IMPORTANT: presets are starting points. Threshold and gain reduction depend on incoming source level. All presets leave external SC, listen, and bypass OFF to prevent unexpected silent/misrouted live audio. Preset loading preserves the current live routing/safety switches. No new DSP latency or audio-thread allocations.

Reference: https://www.fabfilter.com/help/pro-c/presets/loadingpresets

## Preset loudness / makeup calibration (October 2026)

Every factory program receives a deterministic, fixed **starting Makeup gain**, calculated when the static factory bank is initialized (never on the audio thread): estimate static reduction for a reference **−18 dBFS peak** using the exact knee/range/ratio transfer law, weight it by the preset's Mix, then add a modest intentional audible lift. Vocal and live programs allow up to 6 dB, general instruments/buses up to 5.8 dB, mastering up to 3.2 dB, and deliberately aggressive special effects up to 8 dB. Neutral Start now contains real gentle 2.5:1 / −24 dB compression, instead of factory defaults with almost no reference-level gain reduction.

This is **not** automatic loudness measurement, adaptive normalization, peak limiting, or a guarantee that louder means better. Incoming material with hot peaks can clip later in the DAW or audio interface when makeup is enabled. Reduce Makeup or Input to maintain at least ~3–6 dB of useful monitoring headroom; compare bypass at matched loudness before making critical mix/master decisions. Vocal, karaoke and broadcast presets favor an audible immediately useful lift, whereas mastering deliberately remains conservative. Source level and playing style still affect actual gain reduction.

Automated smoke checks require all 46 programs to have positive makeup, meaningful static compression at a −12 dBFS peak reference and finite audio below 0 dBFS for a modest nominal test input; these are *bounds*, not a guarantee of no clipping on arbitrary signals. No DSP/latency/host parameter ID or preset schema change. Future calibrated reference WAV and perceptual ABX testing remain pending.
