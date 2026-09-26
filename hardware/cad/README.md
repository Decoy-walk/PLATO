# CAD (Fusion 360 exports)

Fusion 360's native cloud file (`.f3d`) isn't meaningful to version in git — it's
an opaque binary tied to Autodesk's own cloud/version history, not diffable
here. Instead, export a neutral format from Fusion 360 at each milestone and
commit that:

| Format | Use for |
|--------|---------|
| `.step` / `.stp` | Full solid model (hinges, sensor mounts, enclosure) — preferred for anything meant to be re-opened/edited in other CAD tools |
| `.stl` | 3D-printable mesh of the current print-ready state |
| `.dxf` | 2D profiles (e.g. laser-cut hinge plates) if any part of the block is flat-stock |

## Naming convention

```
plato-block_v<major>.<minor>_<yyyy-mm-dd>.step
```

e.g. `plato-block_v0.3_2026-08-27.step`. Bump `major` on a structural change
(hinge count, layout), `minor` on a dimensional tweak (wall thickness, mount
hole size).

## Keeping CAD and firmware in sync

The physical hinge count/layout and `firmware/PLATO_XIAO_SAMD21/config.h` /
`FlexMuxManager::kChannelCount` need to agree. When exporting a new revision
that changes hinge count or numbering order, note it in the commit message
and update `kChannelCount` (and the mux wiring notes in the top-level
`README.md`) in the same PR so the two never drift apart silently.

## Wiring diagrams

Two DXF diagrams cover the same `firmware/Plato_002_fsr_led_buzzer_vibration/`
circuit (XIAO SAMD21, FSR402 voltage divider, LED, buzzer, vibration motor
module), for two different physical breadboards. Both are generated with
[ezdxf](https://ezdxf.mozman.at/) as plain vector geometry on named layers —
open either via **Insert > Insert DXF** into a Fusion 360 sketch (or any
other DXF-reading tool). Neither is a to-scale footprint layout; for the
exact row-by-row wiring instructions, see `firmware/Plato_001_fsr_led/README.md`
and `firmware/Plato_002_fsr_led_buzzer_vibration/README.md`.

- **`plato_002_wiring_diagram.dxf`** — matches the **no-rail mini
  breadboard** the firmware READMEs document: 3V3/GND are net labels only,
  and every use of them is its own point-to-point jumper back to the chip
  (no shared rail exists on that board). Layers: `WIRES`, `COMPONENTS`,
  `RAILS`, `LABELS`.
- **`plato_002_wiring_diagram_shared_rail.dxf`** — redrawn for a
  **standard breadboard with real +/- power rails**: the chip only needs
  ONE jumper to the +rail and ONE to the -rail; every component taps the
  rail directly instead of getting its own dedicated wire back to the
  chip. Simpler to build if you're not constrained to the small no-rail
  board. Layers: `WIRES`, `COMPONENTS`, `RAIL_PLUS`, `RAIL_MINUS`, `LABELS`.

Pick whichever matches the physical breadboard actually on hand — they're
electrically equivalent, just wired differently.

## Adding a new export

From your local machine (this repo isn't checked out where Fusion 360 runs):

```bash
git add hardware/cad/plato-block_vX.Y_YYYY-MM-DD.step
git commit -m "Add CAD export vX.Y: <what changed>"
git push
```
