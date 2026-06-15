# Results

Plots produced by `analysis/analyze_resolution.py` from the `scan_*` macros.
Regenerate with: `python ../analysis/analyze_resolution.py --indir <dir> --outdir <dir>`.

## Energy resolution by geometry

All optics off (energy from deposited LYSO energy), beam-test selection applied
(radial cut `hypot(beamX,beamY) < 2 mm`, Pb-glass leakage cut). Fit form:
`sigma_E/E = a/sqrt(E) (+) b/E (+) c`. The b/E (noise) term is ~0 because optics
are off — these are intrinsic sampling resolutions.

| Geometry | files | fit | events |
|---|---|---|---|
| **single** 14×14 module | `energy_resolution.{pdf,png}`, `resolution_summary.csv` | **11.75%/√E ⊕ 0.58%** | 2000/E, 9 pts (5–200 GeV) |
| **array3x3** (enhanced 18×18 ×9) | `energy_resolution_array3x3.{pdf,png}`, `resolution_summary_array3x3.csv` | **8.0%/√E ⊕ 0.53%** | 800/E, 5 pts (quick) |
| **hex** (7 caps) | `energy_resolution_hex.{pdf,png}`, `resolution_summary_hex.csv` | **12.2%/√E ⊕ 0.24%** | 800/E, 5 pts (quick) |

- **single** (9-point scan): close to the RADiCAL design goal 10%/√E ⊕ 0.7%.
- **array3x3**: `eTotLyso` sums all 9 modules, so this is the *containment-corrected*
  resolution. The 3×3 array drops the stochastic term from ~11.8% to ~8.0%/√E —
  the transverse-containment benefit the array is built for (Section 7). Per-event
  the central module holds ~76% of the shower; the rest is recovered from the
  neighbours (`eMod0..8`).
- **hex**: slightly worse stochastic term than the square module, consistent with
  its smaller cross-sectional area (more lateral leakage).

The array/hex curves are quick scans (5 points, 800 events) for a first look;
rerun `scan_array3x3.mac` / `scan_hex.mac` (2000/E, 9 points) for publication
statistics — ideally on a cluster.

## Timing resolution — (to be added)
Requires the optics-on scan (`scan_timing.mac` / reduced demo). The committed
plot will note that absolute sigma_t depends on the (currently estimated) optical
inputs — see `../ESTIMATES.md` — and on light yield; treat the trend, not the
absolute value, as meaningful until tuned to bench data.

## Caveat
Geometry/EM-driven results (energy resolution, shower profile, leakage) are
robust. Absolute detected-light and timing numbers depend on the documented
optical estimates in `../ESTIMATES.md`.
