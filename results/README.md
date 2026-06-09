# Results

Plots produced by `analysis/analyze_resolution.py` from the `scan_*` macros.
Regenerate with: `python ../analysis/analyze_resolution.py --indir <dir> --outdir <dir>`.

## Energy resolution — `energy_resolution.pdf` / `.png`
RADiCAL module energy resolution vs. beam energy, electrons 5–200 GeV
(`scan_energy_fast.mac`, 2000 events/energy, optics off — energy resolution
comes from deposited LYSO energy, not detected light). Beam-test selection
applied: radial cut `hypot(beamX,beamY) < 2 mm` and Pb-glass leakage cut.

Fit (`eTotLyso`, all 29 LYSO layers):

    sigma_E/E = 11.75% / sqrt(E)  (+)  0.0% / E  (+)  0.58%

The b/E (noise) term is ~0 because optics are off (no electronic/photon noise);
this is the intrinsic sampling resolution. Close to the RADiCAL design goal of
10%/sqrt(E) (+) 0.7%. See `resolution_summary.csv` for the per-energy values.

## Timing resolution — (to be added)
Requires the optics-on scan (`scan_timing.mac` / reduced demo). The committed
plot will note that absolute sigma_t depends on the (currently estimated) optical
inputs — see `../ESTIMATES.md` — and on light yield; treat the trend, not the
absolute value, as meaningful until tuned to bench data.

## Caveat
Geometry/EM-driven results (energy resolution, shower profile, leakage) are
robust. Absolute detected-light and timing numbers depend on the documented
optical estimates in `../ESTIMATES.md`.
