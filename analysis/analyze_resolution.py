#!/usr/bin/env python3
"""
Energy and timing resolution of the RADiCAL module vs. beam energy.

Reads the per-energy ROOT files produced by macros/scan_resolution.mac
(radical_<E>GeV.root), applies the beam-test-style selection, and computes:

  * Energy resolution   sigma_E / E   from the sampled LYSO energy, fit to
        sigma_E/E = a/sqrt(E) (+) b/E (+) c          (paper Eq. 1 form)
  * Timing resolution   sigma_t       from the MCP-independent estimator
        q = ( <t_downstream> - <t_upstream> ) / 2 ,  fit to
        sigma_t = a/sqrt(E) (+) b                     (paper Eq. 2 form)
    where (+) is a quadrature sum.

Selection (mirrors the paper):
  * radial cut on beam impact:  hypot(beamX, beamY) < --rcut  (default 2 mm)
  * leakage cut:                ePbGlass < --leak-frac * beamE (default 0.2)

Timing uses the per-channel leading-edge stamp tThr_ch* (time of the k-th
detected p.e., k set by kTimingThresholdPE in the C++). Channels are
ch = corner*2 + end, end 0 = upstream (-z), end 1 = downstream (+z), so
upstream = even channels {0,2,4,6}, downstream = odd channels {1,3,5,7}.

Usage:
  python analyze_resolution.py --indir ../build --outdir ../build/plots
"""
import argparse
import glob
import os
import re
import sys

import numpy as np

try:
    import uproot
except ImportError:
    sys.exit("Need uproot:  pip install uproot")
from scipy.optimize import curve_fit
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt


# ---------------------------------------------------------------------------
def gauss(x, A, mu, sigma):
    return A * np.exp(-0.5 * ((x - mu) / sigma) ** 2)


def fit_gaussian(values, nbins=60):
    """Robust Gaussian core fit. Returns (mu, sigma, n_used) or None."""
    values = np.asarray(values, float)
    values = values[np.isfinite(values)]
    if values.size < 30:
        return None
    med = np.median(values)
    mad = np.median(np.abs(values - med)) * 1.4826
    if mad <= 0:
        mad = values.std()
        if mad <= 0:
            return None
    sel = values[(values > med - 3 * mad) & (values < med + 3 * mad)]
    mu, sigma = sel.mean(), sel.std()
    for _ in range(3):
        if sel.size < 30:
            break
        hist, edges = np.histogram(sel, bins=nbins)
        centers = 0.5 * (edges[:-1] + edges[1:])
        try:
            popt, _ = curve_fit(gauss, centers, hist,
                                p0=[hist.max(), sel.mean(), max(sel.std(), 1e-9)],
                                maxfev=20000)
            mu, sigma = popt[1], abs(popt[2])
        except Exception:
            mu, sigma = sel.mean(), sel.std()
            break
        sel = values[(values > mu - 2.5 * sigma) & (values < mu + 2.5 * sigma)]
    return mu, sigma, sel.size


def eres_model(E, a, b, c):
    return np.sqrt((a / np.sqrt(E)) ** 2 + (b / E) ** 2 + c ** 2)


def tres_model(E, a, b):
    return np.sqrt((a / np.sqrt(E)) ** 2 + b ** 2)


# ---------------------------------------------------------------------------
def load_files(indir):
    files = {}
    for path in glob.glob(os.path.join(indir, "radical_*GeV.root")):
        m = re.search(r"radical_(\d+)GeV", os.path.basename(path))
        if m:
            files[int(m.group(1))] = path
    return dict(sorted(files.items()))


def per_energy(path, rcut, leak_frac, energy_var):
    """Return (Emean, Eres, dEres, sigma_t_ps, dsigma_t_ps, n_sel)."""
    t = uproot.open(path)["radical"]
    need = [energy_var, "beamE_MeV", "beamX_mm", "beamY_mm", "ePbGlass_MeV"]
    need += [f"tThr_ch{c}_ns" for c in range(8)]
    a = t.arrays(need, library="np")

    beamE = a["beamE_MeV"]
    r = np.hypot(a["beamX_mm"], a["beamY_mm"])
    sel = (r < rcut) & (a["ePbGlass_MeV"] < leak_frac * beamE)
    n_sel = int(sel.sum())
    if n_sel < 30:
        return None

    # ---- energy resolution ----
    e_est = a[energy_var][sel]
    eg = fit_gaussian(e_est)
    if eg is None:
        return None
    emu, esig, en = eg
    eres = esig / emu if emu > 0 else np.nan
    deres = eres / np.sqrt(2 * en) if en > 0 else np.nan

    # ---- timing resolution: q = (<t_dn> - <t_up>)/2 ----
    up = np.vstack([a[f"tThr_ch{c}_ns"][sel] for c in (0, 2, 4, 6)]).astype(float)
    dn = np.vstack([a[f"tThr_ch{c}_ns"][sel] for c in (1, 3, 5, 7)]).astype(float)
    up[up < 0] = np.nan          # -1 => below p.e. threshold
    dn[dn < 0] = np.nan
    import warnings
    with warnings.catch_warnings():
        warnings.simplefilter("ignore", category=RuntimeWarning)
        up_mean = np.nanmean(up, axis=0)   # NaN where a side has no valid channel
        dn_mean = np.nanmean(dn, axis=0)
    q = 0.5 * (dn_mean - up_mean)               # ns
    q = q[np.isfinite(q)]
    tg = fit_gaussian(q)
    if tg is None:
        sigma_t = dsigma_t = np.nan
    else:
        _, qsig, qn = tg
        sigma_t = qsig * 1000.0                   # ns -> ps
        dsigma_t = sigma_t / np.sqrt(2 * qn)
    return emu, eres, deres, sigma_t, dsigma_t, n_sel


# ---------------------------------------------------------------------------
def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--indir", default=".", help="dir with radical_<E>GeV.root")
    ap.add_argument("--outdir", default="plots", help="output dir for plots/CSV")
    ap.add_argument("--rcut", type=float, default=2.0, help="radial cut [mm]")
    ap.add_argument("--leak-frac", type=float, default=0.2,
                    help="reject events with ePbGlass > leak_frac*beamE")
    ap.add_argument("--energy-var", default="eTotLyso_MeV",
                    choices=["eTotLyso_MeV", "eSM3_MeV", "eSM5_MeV"],
                    help="LYSO energy estimator for the energy resolution")
    args = ap.parse_args()

    files = load_files(args.indir)
    if not files:
        sys.exit(f"No radical_<E>GeV.root files in {args.indir!r}")
    os.makedirs(args.outdir, exist_ok=True)

    E, Emean, Eres, dEres, St, dSt = [], [], [], [], [], []
    print(f"{'E[GeV]':>7} {'Nsel':>6} {'<E_dep>[MeV]':>13} "
          f"{'sigE/E[%]':>10} {'sigma_t[ps]':>12}")
    for e, path in files.items():
        res = per_energy(path, args.rcut, args.leak_frac, args.energy_var)
        if res is None:
            print(f"{e:>7} {'--':>6}  (too few events after cuts, skipped)")
            continue
        emu, eres, deres, st, dst, n = res
        E.append(e); Emean.append(emu)
        Eres.append(eres); dEres.append(deres)
        St.append(st); dSt.append(dst)
        print(f"{e:>7} {n:>6} {emu:>13.2f} {100*eres:>10.2f} "
              f"{st:>12.1f}")

    E = np.array(E, float)
    Emean = np.array(Emean); Eres = np.array(Eres); dEres = np.array(dEres)
    St = np.array(St); dSt = np.array(dSt)

    # ---- fits ----
    eres_fit = None
    if E.size >= 3:
        try:
            p0 = [0.1, 0.1, 0.01]
            w = np.where(dEres > 0, dEres, np.nan)
            eres_fit, _ = curve_fit(eres_model, E, Eres, p0=p0,
                                    sigma=w, absolute_sigma=False, maxfev=20000)
        except Exception as ex:
            print("energy-resolution fit failed:", ex)

    tmask = np.isfinite(St)
    tres_fit = None
    if tmask.sum() >= 2:
        try:
            ws = np.where(dSt[tmask] > 0, dSt[tmask], np.nan)
            tres_fit, _ = curve_fit(tres_model, E[tmask], St[tmask],
                                    p0=[250., 20.], sigma=ws,
                                    absolute_sigma=False, maxfev=20000)
        except Exception as ex:
            print("timing-resolution fit failed:", ex)

    # ---- energy resolution plot ----
    fig, ax = plt.subplots(figsize=(7, 5))
    ax.errorbar(E, 100 * Eres, yerr=100 * dEres, fmt="o", color="C0",
                label="simulation", capsize=3)
    if eres_fit is not None:
        a, b, c = eres_fit
        xs = np.linspace(E.min(), E.max(), 300)
        ax.plot(xs, 100 * eres_model(xs, a, b, c), "C0-",
                label=(rf"$\sigma_E/E=\frac{{{100*a:.1f}\%}}{{\sqrt{{E}}}}"
                       rf"\oplus\frac{{{100*b:.1f}\%}}{{E}}\oplus{100*c:.1f}\%$"))
    ax.set_xlabel("Beam energy  E  [GeV]")
    ax.set_ylabel(r"Energy resolution  $\sigma_E/E$  [%]")
    ax.set_title(f"RADiCAL energy resolution ({args.energy_var})")
    ax.grid(alpha=0.3); ax.legend()
    fig.tight_layout()
    f1 = os.path.join(args.outdir, "energy_resolution.png")
    fig.savefig(f1, dpi=150); plt.close(fig)

    # ---- timing resolution plot ----
    fig, ax = plt.subplots(figsize=(7, 5))
    ax.errorbar(E[tmask], St[tmask], yerr=dSt[tmask], fmt="s", color="C3",
                label="simulation", capsize=3)
    if tres_fit is not None:
        a, b = tres_fit
        xs = np.linspace(E[tmask].min(), E[tmask].max(), 300)
        ax.plot(xs, tres_model(xs, a, b), "C3-",
                label=(rf"$\sigma_t=\frac{{{a:.0f}}}{{\sqrt{{E}}}}\,$ps"
                       rf"$\oplus{b:.1f}\,$ps"))
    ax.set_xlabel("Beam energy  E  [GeV]")
    ax.set_ylabel(r"Timing resolution  $\sigma_t$  [ps]")
    ax.set_title("RADiCAL timing resolution  (DW$-$UP)/2")
    ax.grid(alpha=0.3); ax.legend()
    fig.tight_layout()
    f2 = os.path.join(args.outdir, "timing_resolution.png")
    fig.savefig(f2, dpi=150); plt.close(fig)

    # ---- CSV summary ----
    csv = os.path.join(args.outdir, "resolution_summary.csv")
    with open(csv, "w") as fh:
        fh.write("E_GeV,Edep_mean_MeV,sigmaE_over_E,err,sigma_t_ps,err\n")
        for i in range(E.size):
            fh.write(f"{E[i]:.0f},{Emean[i]:.4f},{Eres[i]:.6f},{dEres[i]:.6f},"
                     f"{St[i]:.4f},{dSt[i]:.4f}\n")

    print("\nWrote:")
    print(" ", f1)
    print(" ", f2)
    print(" ", csv)
    if eres_fit is not None:
        a, b, c = eres_fit
        print(f"\nEnergy resolution fit ({args.energy_var}):")
        print(f"  sigma_E/E = {100*a:.2f}%/sqrt(E) (+) {100*b:.2f}%/E (+) {100*c:.2f}%")
    if tres_fit is not None:
        a, b = tres_fit
        print(f"Timing resolution fit:")
        print(f"  sigma_t = {a:.1f} ps*sqrt(GeV)/sqrt(E) (+) {b:.1f} ps")
        print("  (paper: a = 256 ps*sqrt(GeV), b = 17.5 ps)")


if __name__ == "__main__":
    main()
