# ArSonComp — interface gallery

All images below are **actual screenshots captured by the Windows JUCE editor smoke test**, not AI illustrations or visual mockups. The latest interface-changing source is M7; the subsequent v0.2.0-rc.1 installer work did not change the GUI.

**Provenance:** [Windows run #41267899861](https://github.com/masarray/arson-comp/actions/runs/41267899861), which includes the test executable and screenshot bundle.

## Full editor — 612 × 580 px

![ArSonComp full-size editor with transfer graph, meters, and hardware controls](images/arsoncomp-full.png)

The black-glass area displays the transfer curve and real input/output/gain-reduction telemetry, with primary compression controls immediately beneath the screen.

## Minimum — 580 × 540 px

![Responsive ArSonComp minimum editor](images/arsoncomp-minimum.png)

Responsive layout retains essential controls and a readable full graph without enlarging the window unnecessarily.

## Sidechain

![ArSonComp sidechain page](images/arsoncomp-sidechain.png)

The tab exposes detector HP/LP filters, Peak/RMS selection, external sidechain and Listen controls. Switching tabs does not change active audio parameters.

## MINI

![ArSonComp MINI mode](images/arsoncomp-mini.png)

A shorter control-oriented view, with the same native control bindings.

## Preset browser

![ArSonComp factory preset browser](images/arsoncomp-presets.png)

46 original factory starting points with categorization, search, descriptions, and favorites. These are not imported proprietary presets.

---

**Screenshot update rule:** when the GUI changes, refresh these files from the matching successful JUCE Windows editor run, rather than inserting a concept image. [Go back to README](../README.md).