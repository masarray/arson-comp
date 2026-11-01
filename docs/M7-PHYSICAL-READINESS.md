# M7 — Premium flat brushed metal + hardware field-test gate

## Integrated changes
- Full editor 612×580, minimum 580×540, MINI remains unchanged.
- Approved photographic horizontal ruler track and circular silver thumb retained.
- Thumb visually reduced from 30px to 23.8px at its normal 34px control height
  (about **21% smaller**), while keeping the same draggable JUCE Slider bounds.
- Caption and slider component both lowered by 3px with their 5px separation.
- Old diagonal/spotlight chassis replaced by matte #4B4E50 graphite and
  reproducible horizontal micro-scratches, cached on resize only.
- Fast message-thread paint, bounded meter updates, zero DSP changes.

## Industry reference, not copying
- FabFilter Pro-C 3: large metering with compact controls.
  https://www.fabfilter.com/help/pro-c/using/displays
- TDR Kotelnikov GE: logical sections, readable metering.
  https://docs.tokyodawn.ru/kotelnikov-ge-manual/
- Steinberg VST3 API: isolate realtime audio from GUI allocations and I/O.
  https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical%2BDocumentation/API%2BDocumentation/Index.html

## Evidence and physical acceptance
Windows VST3+Standalone + plugin CTest + four real JUCE snapshots; Linux
sanitizer engine-safety. Source smoke tests include horizontally directional
flat chassis, 20–25px photo thumb, caption clearance, MINI and sidechain.

Package QA/Collect-ArSonComp-Hardware.ps1 with an up-to-date START-HERE.md.
A successful CI **does not** prove Maono PS22 stability. The actual gate is
two VST3 hosts, ASIO/WASAPI driver and 128/64/32 buffers, A/B/C/D crackle
isolation, host XRUN counts, project restore, 60-minute session, and
loudness-matched listening. Test helper only collects metadata and writes a
manual form; it does not silently claim measurements.

## CI budget
One integrated PR and one Windows+Linux CI. Retry only a failed job after
targeted fixes. No lookahead, oversampling or DSP architecture changes.
