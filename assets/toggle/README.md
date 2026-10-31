# ArSonComp ON/OFF toggle assets

Two state images are authored as transparent SVG source files. Corresponding
high-resolution RGBA PNG exports are supplied with the design deliverables.

- ON: green case, metal actuator on the left
- OFF: red case, metal actuator on the right
- Source canvas: 640 × 246; recommended PNG export: 768 × 295
- Use images at the same view rectangle for a no-layout-shift state change.

Export for JUCE using: `cairosvg arson_toggle_on.svg -o arson_toggle_on.png -W 768 -H 295`
(and analogously for OFF). No changes to DSP or plugin UI wiring are included.
