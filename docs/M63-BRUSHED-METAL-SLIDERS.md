# M6.3 approved flat brushed-metal photographic fader

Two distinct user-approved transparent PNGs are bundled in the VST3 and
Standalone binary:

- assets/sliders/arsoncomp_flat_track_480.png — 480×66 cropped transparent
  stationary horizontally brushed metal ruler, with dark recessed slot.
- assets/sliders/arsoncomp_silver_thumb_96.png — 96×96 circular satin brushed
  silver thumb, moves independently.

These are cropped/downsized from the approved rendered assets, not new vector
approximations. Thumb and ruler retain original aspect ratio at all slider
widths, including the smaller Sidechain pair. Both assets decode once on the
JUCE editor creation thread; the audio thread is unaffected. The 612×580
taller LCD and MINI mode remain unchanged.

Release gates: Windows compile/CTest/screenshot artifacts, Linux sanitizer
engine tests. Final visual acceptance and physical audio tests are separate.
