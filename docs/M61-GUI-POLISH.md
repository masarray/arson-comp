# M6.1 — Dark Brushed Thumb and Uncropped Rotary Knobs

## Scope
This is a GUI-only refinement of the M6 baseline (commit 8878167).
No compressor DSP, parameter, automation, ASIO, preset, or audio scheduling changes.

## Changes
- New bundled, transparent 58×124 PNG thumb, an original dark graphite/black
  brushed-metal cap matching the tone and soft metal bevel of the rotary knob.
  A small coral center pin stays tactile without an oversized silver ridge.
- Remove the prior bright-silver thumb-back and overpainted ridge. Draw the
  image once with its source aspect ratio using the existing FaderGeometry.
- Fix a measurable clipping condition: the complete rotary arc extended
  beyond a 58px slider rectangle because the halo offset and stroke width
  were not included when fitting its diameter. Expand the primary component
  height to 66px within the existing compact panel, then enforce clearance
  with a shared KnobGeometry contract used by renderer and test suite.
- Preserve 612×500 default and 580×480 minimum; retain full / MINI /
  SIDECHAIN support. All assets decode once on the JUCE message thread.

## Acceptance
Actual Windows Standalone/VST3 smoke screenshots in Full, minimum, MINI and
SIDECHAIN; check the dark cap at slider extremes and the full coral arcs above
all seven rotaries. Linux DSP engine tests unchanged. A passing CI is not proof
of stable hardware audio; PS22 crackle investigation remains separate.
