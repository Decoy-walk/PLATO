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

The physical hinge count/layout and `firmware/PLATO_XIAO_C3/config.h` /
`FlexMuxManager::kChannelCount` need to agree. When exporting a new revision
that changes hinge count or numbering order, note it in the commit message
and update `kChannelCount` (and the mux wiring notes in the top-level
`README.md`) in the same PR so the two never drift apart silently.

## Adding a new export

From your local machine (this repo isn't checked out where Fusion 360 runs):

```bash
git add hardware/cad/plato-block_vX.Y_YYYY-MM-DD.step
git commit -m "Add CAD export vX.Y: <what changed>"
git push
```
