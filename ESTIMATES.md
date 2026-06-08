# Estimated inputs — what is measured vs. assumed

**None of the optical/scintillation constants in this simulation are measured for
the actual RADiCAL components.** The paper (NIM A 1068 (2024) 169737) gives the
full mechanical geometry and only a handful of optical numbers. Everything else
is an engineering estimate from literature/datasheet typicals, chosen to be
physically reasonable so the simulation runs and produces sensible light levels —
**not** to reproduce the measured light yield or absolute timing.

Every estimate is tagged `ESTIMATE:` in the source (with its basis); values taken
straight from the paper are tagged `PAPER:`. To list them all:

```bash
grep -rn "ESTIMATE:" src include
grep -rn "PAPER:"    src include
```

---

## Taken directly from the paper (not estimated)

| Quantity | Value | Where |
|---|---|---|
| All mechanical geometry | see `RadicalConstants.hh` | Sec. 2, Fig. 2 |
| LYSO emission peak | 420 nm | used as scint peak |
| DSB1 absorption peak | 425 nm | WLS absorption |
| DSB1 emission peak | 495 nm | WLS emission |
| DSB1 WLS decay time | 3.5 ns | `WLSTIMECONSTANT` |

Note: the paper says **0.008″ (0.203 mm) Tyvek**; this build uses **0.1 mm** at
your request (`kTyvekThick`).

---

## Estimated values (replace with measurements when available)

### LYSO:Ce — `src/RadicalMaterials.cc::BuildLYSO`
| Parameter | Value used | Basis of estimate |
|---|---|---|
| Refractive index | 1.82 (flat, no dispersion) | Saint-Gobain / Crystal Photonics LYSO datasheets (~1.81–1.82 @ 420 nm) |
| Light yield | 30 000 γ/MeV | LYSO:Ce literature range ~25 000–33 000; paper silent |
| Decay time | 40 ns (single component) | LYSO:Ce literature ~36–44 ns; paper silent |
| Bulk attenuation length | 1.0 m (flat) | order-of-magnitude for good LYSO |
| Emission band shape | assumed, peak 420 nm | peak is PAPER; wings assumed |
| Birks constant | 0.0076 mm/MeV | typical dense inorganic scintillator |
| Resolution scale | 1.0 (Poisson) | standard default |

### DSB1 WLS — `BuildDSB1`
| Parameter | Value used | Basis of estimate |
|---|---|---|
| Host polymer / density | polystyrene | organic plastic WLS; host not specified |
| Refractive index | 1.59 (flat) | typical polystyrene-based plastic |
| WLS absorption-length spectrum | strong (0.2 mm) @ 425 nm → long elsewhere | only the 425 nm peak is PAPER; curve assumed |
| WLS emission band shape | assumed, peak 495 nm | peak is PAPER; band assumed |
| WLS quantum yield | 1.0 (photon out per photon in) | optimistic upper bound; real < 1 — **strongly affects detected light** |
| Bulk absorption length | 2.0 m | assumed low self-absorption |

### Quartz / fused silica — `BuildQuartz`
| Parameter | Value used | Basis |
|---|---|---|
| Refractive index | Sellmeier (≈1.46–1.48) | well-known dispersion — effectively exact |
| Attenuation length | 10 m (flat) | transparent over 18 cm; value immaterial |

### Tyvek reflector — `DetectorConstruction.cc::ApplyOpticalSurfaces`
| Parameter | Value used | Basis |
|---|---|---|
| Diffuse reflectivity | 0.95 (blue) → 0.90 (UV) | published Tyvek reflectance ~94–97% in blue/green |
| Finish / model | ground + dielectric_metal (Lambertian) | standard diffuse-reflector recipe |

### SiPM (Hamamatsu HDR2) — `ApplyOpticalSurfaces`
| Parameter | Value used | Basis |
|---|---|---|
| PDE spectrum | ~40% peak (450–500 nm), rolling off | typical Hamamatsu SiPM PDE; exact HDR2 curve not used |
| Active size | 1.3 × 1.3 mm, 0.3 mm thick | sized to cover the 1.15 mm capillary end (geometry assumption) |

### Lead-glass backing — `BuildLeadGlass`
| Parameter | Value used | Basis |
|---|---|---|
| Refractive index | 1.62–1.67 | typical SF-type lead glass; only matters if Cherenkov readout studied |
| Attenuation length | 50 cm | assumed; leakage cut uses deposited energy (insensitive) |

### Readout / electronics — `NtupleDef.hh`, `EventAction.cc`
| Parameter | Value used | Basis |
|---|---|---|
| Timing model | leading-edge: time of the k-th detected p.e. | simple discriminator stand-in for the paper's fixed-threshold high-gain timing |
| Timing threshold k | 5 p.e. (`kTimingThresholdPE`) | arbitrary; tune to your discriminator threshold |
| MCP | glass body + quartz window only | material/timing-reference stand-in, no internal MCP structure |

---

## What these estimates do and don't affect

- **Robust (geometry/EM-driven), insensitive to the optical estimates:**
  longitudinal shower profile, shower-max layer, sampling fraction, per-layer and
  shower-max deposited energy, Pb-glass leakage. These reproduce the paper's
  Fig. 7 / Fig. 9 behaviour.
- **Sensitive to the optical estimates (treat as relative, not absolute):**
  detected photoelectron counts, and therefore the absolute timing resolution.
  The light-yield × WLS-QY × PDE product sets the p.e. scale; until measured,
  use these for *trends* (energy dependence, up/downstream behaviour), and tune
  the product to your bench data before quoting absolute σ_t.
