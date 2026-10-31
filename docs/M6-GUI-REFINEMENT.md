# M6 — proportional controls and coherent typography

Engineering outcome: preserve ArSonComp's compact 612×500 window (minimum 580×480, MINI 320 tall), black-glass LCD, premium knob assets, 46 factory presets and native Windows ASIO. No audio DSP, parameter schema, buffering or worker changes.

## Root cause and corrective geometry

The image resources are **340×57 track** and **58×124 thumb**. The earlier nine-slice rendering stretched a 30-pixel plate through a narrow control slot and separately forced the thumb to 18×37. This broke the relative artwork proportions and made the narrow sliders appear flattened.

`FaderGeometry::calculate()` now chooses **one scale** bounded by (control width−8)/340 and (control height−4)/124, uses it on both complete images, and places the red progress recess behind the thumb. The track and thumb preserve their true aspect ratios regardless of 580/612/704/1000 width. Original transparent PNG assets still decode once in LookAndFeel on GUI initialization. No new cache allocations or DSP operations. Source dimensions are asserted in regression tests.

## Alignment and UX

The main seven-knob row has centrally aligned 10.4-point captions, 7+ pixels of actual empty space above the metal knobs, and 58-pixel knob drag targets. Slider labels have a separate 4+ pixel vertical gap; the fader control box is 34 px high, rather than a 40px box with a stretched 30px plate. Dynamics and Sidechain both use the same geometry and balanced baselines. 'Mini', 'Full', 'File', 'Dynamics', and 'Sidechain' use title case; A/B remains unchanged. Consistent 11-point header buttons, 10.4-point captions and 9.5-point overline typography, with a native crisp wordmark legible on dark metal.

The functional black LCD graph, meter, preset browser, hover readout and sidechain device controls retain their positions/behavior. The input of the real-time compressor is independent of these paint-only modifications.

## Automated validation

JUCE smoke checks (1) exact image resource dimensions, (2) proportional artwork ratios at 70–150px slider widths and 34/40px heights, (3) meter/thumb red movement, (4) 7 main knobs/captions are separated, (5) all 4 Dynamics fader captions separated, (6) both Sidechain fader captions separated, (7) title-case controls, and (8) prior full/MINI resize, hit target, preset and ASIO tests. Keep the existing Linux engine safety and Windows VST3/Standalone checks.

## Physical acceptance still needed

Screenshot review at 612×500 / 580×480 / MINI, Windows 100%/125%/150% DPI and mouse hit-testing. PS22 crackle investigations and musical-compressor preset loudness remain independent release gates. Reference: FabFilter Pro-C 3 uses a separate Compact and resizable workflow; JUCE Graphics supports explicit aspect-preserving rendering. This patch does not copy protected graphics or proprietary DSP.
