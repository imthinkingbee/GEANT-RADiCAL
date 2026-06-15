# Geometry renderings

GEANT4 renderings of the three detector geometries (beamline hidden for clarity),
each showing a 3D view and a beam's-eye cross-section. Produced with the headless
`TOOLSSG_OFFSCREEN` driver (`zb_png` software rasteriser), combined into one PDF
per geometry.

| File | Geometry |
|---|---|
| `geometry_single.{pdf,png}` | Baseline 14×14 module: 29 LYSO (cyan) / 28 W (grey) plates, Tyvek, 4 corner capillaries + 1 empty central hole |
| `geometry_enhanced.{pdf,png}` | **Single** enhanced 18×18 module with a **3×3 grid of 9 capillaries** (Fig. 28) |
| `geometry_hex.{pdf,png}` | Hexagonal module: 7 capillaries (6 ring + 1 centre) |
| `geometry_array3x3.{pdf,png}` | 3×3 **array of 9** enhanced modules (paper Section 7 containment study) |

In the cross-sections the green squares are the SiPM faces at the capillary ends
(viewed down the beam axis). The cross-section hole patterns reproduce the
Future-Work slide layouts.

Regenerate: render each mode with `/vis/open TSG_OFFSCREEN`, `/vis/drawVolume`,
`/vis/tsg/export zb_png <file>.png` (see the `r_<geo>_<view>.mac` recipe in the
build dir), then combine the PNGs into a PDF.
