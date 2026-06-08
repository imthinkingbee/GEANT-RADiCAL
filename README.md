# RADiCAL ultra-compact sampling calorimeter — GEANT4 simulation

A full-stack GEANT4 (v11.x) simulation of the **RADiCAL** module described in:

> C. Perez-Lara *et al.*, *"Study of time resolution measurements and prospects
> for energy resolution of an ultra-compact sampling calorimeter (RADiCAL)
> module at EM shower maximum over the energy range 25 GeV–150 GeV"*,
> Nucl. Instrum. Methods Phys. Res. A **1068** (2024) 169737.

It models the full beam-test arrangement (Fig. 11c) with full electromagnetic,
hadronic **and** optical physics, scintillation photostatistics, wavelength
shifting, and a simple SiPM/electronics readout model.

---

## What is modelled

### The RADiCAL module (Sec. 2, Figs. 1–2)
- Tile cross-section **14 × 14 mm²**, overall module **14 × 14 × 135 mm³**.
- **29 LYSO:Ce** plates, **1.5 mm** thick (active scintillator).
- **28 tungsten** plates, **2.5 mm** thick (absorber).
- Interleaved as `LYSO (Tyvek W Tyvek) ×28 … LYSO`, beginning and ending on LYSO.
- **0.1 mm Tyvek** reflector between **every** plate interface (56 sheets total),
  **plus a 0.1 mm Tyvek wrap around the outer 14×14 lateral faces**, all modelled
  as a diffuse ~95 % reflector via `G4LogicalSkinSurface`.
  *(0.1 mm per your request. The paper uses 0.008″ = 0.203 mm Tyvek — change
  `kTyvekThick` in `include/RadicalConstants.hh` to revert.)*
- **5 capillary through-holes** (Fig. 2):
  - 4 corner holes **φ1.3 mm** at **(±3.5, ±3.5) mm** → quartz WLS capillaries.
  - 1 central hole **φ0.9 mm** → left empty (air), as in the reported test.
- **Quartz capillaries**: OD 1.15 mm, core ID 0.95 mm, length 183 mm, each with a
  **DSB1 WLS filament** (φ0.9 mm × 15 mm) embedded at EM shower max; remainder is
  fused quartz. (Wall + quartz-rod fill are both modelled as fused silica.)
- **8 SiPM channels** (Hamamatsu HDR2): one silicon photodetector tile at each of
  the 4 capillaries' two ends. Channel id = `corner*2 + end` (end 0 = upstream/−z,
  1 = downstream/+z).

### The beam line (Sec. 4, Fig. 11c) — all toggle-able
- **A1 / A2** plastic trigger counters (1×1 cm² / 2×2 cm²), upstream.
- **MCP** reference-timing tube (Hamamatsu R3809U-50), simplified to a glass body
  + quartz window just upstream of the module.
- **Pb-glass backing calorimeter**: 2×2 array of 2×2×40 cm lead-glass blocks
  (4×4×40 cm total), downstream, scored for leakage cuts.

### Physics
- `FTFP_BERT` (hadronic + EM) with **`G4EmStandardPhysics_option4`** (high accuracy).
- **`G4OpticalPhysics`**: scintillation, Cerenkov, bulk absorption, Rayleigh,
  boundary processes and **WLS**.
- LYSO scintillation: 30000 γ/MeV, λ_peak 420 nm, τ = 40 ns, Birks 0.0076 mm/MeV.
- DSB1 WLS: absorbs 425 nm, emits 495 nm, **τ = 3.5 ns** (paper value).
- SiPM detection via the optical-boundary `Detection` status with a wavelength-
  dependent PDE (~40 % peak). Photostatistics arise naturally from Poisson
  scintillation × PDE.

---

## Build

GEANT4 11.x (built with Qt/OpenGL for visualization) and CMake ≥ 3.16 are
required. The easiest local install is via conda-forge:

```bash
mamba create -n geant4 -c conda-forge geant4 cmake   # ~1-2 GB incl. data
conda activate geant4
```

Then, from the repo root:

```bash
./configure.sh        # activates the geant4 env, configures + builds into ./build
# (override env name with GEANT4_ENV=myenv ./configure.sh ; "./configure.sh clean" to wipe)
```

`configure.sh` passes the EXPAT flags needed on macOS (the system SDK ships an
older expat that CMake otherwise rejects). Manual equivalent:

```bash
cd GEANT-RADiCAL && mkdir build && cd build
cmake -DGeant4_DIR=$(geant4-config --prefix)/lib/cmake/Geant4 \
      -DCMAKE_PREFIX_PATH=$(geant4-config --prefix) \
      -DEXPAT_INCLUDE_DIR=$(geant4-config --prefix)/include \
      -DEXPAT_LIBRARY=$(geant4-config --prefix)/lib/libexpat.dylib ..
make -j
```

## Run

```bash
# Interactive with visualization:
./radical

# Batch (single energy, writes radical_50GeV.root):
./radical run.mac

# Energy scan 25–150 GeV (one ROOT file per energy):
./radical scan_energy.mac

# Full resolution scan (5, 25, 50 … 400 GeV), one ROOT file per energy:
./radical scan_resolution.mac
```

## Resolution analysis

`analysis/analyze_resolution.py` turns the per-energy ROOT files into energy- and
timing-resolution-vs-energy plots and fits.

```bash
pip install -r analysis/requirements.txt        # uproot numpy scipy matplotlib
cd build && ./radical scan_resolution.mac        # produces radical_<E>GeV.root
python ../analysis/analyze_resolution.py --indir . --outdir plots
```

It applies the paper's selection (radial cut `hypot(beamX,beamY) < 2 mm`, plus a
Pb-glass leakage cut), then:

- **Energy resolution** from the sampled LYSO energy (`eTotLyso_MeV` by default;
  `--energy-var eSM3_MeV` for the shower-max estimator), fit to
  σ_E/E = a/√E ⊕ b/E ⊕ c.
- **Timing resolution** from the MCP-independent estimator
  q = (⟨t_downstream⟩ − ⟨t_upstream⟩)/2 using the per-channel leading-edge
  stamps, fit to σ_t = a/√E ⊕ b (paper: a = 256 ps·√GeV, b = 17.5 ps).

Outputs: `plots/energy_resolution.png`, `plots/timing_resolution.png`,
`plots/resolution_summary.csv`. **Absolute** timing/p.e. numbers depend on the
estimated optical inputs (see `ESTIMATES.md`) — read the *trends* until you tune
the light-yield × WLS-QY × PDE product to bench data.

### Useful UI commands
```
/radical/det/buildBeamline true|false   # whole beam line (pre-init)
/radical/det/buildPbGlass  true|false
/radical/det/buildMCP      true|false
/radical/det/buildTriggers true|false
/radical/det/lysoYieldScale 0.01         # scale LYSO light yield (SPEED, pre-init)
/radical/det/wlsCenterZ 0. mm            # override WLS filament z (pre-init)
/radical/gun/spotSize 3.0 mm             # transverse Gaussian beam sigma (0 = pencil)
/gun/particle e- ; /gun/energy 50 GeV
/run/beamOn 100
```

> ⚠️ **Performance.** A 150 GeV EM shower produces ~10⁷–10⁸ optical photons per
> event with the nominal light yield, so full-optics runs are CPU-heavy and meant
> for batch/cluster use. For quick geometry/shower checks, throttle the optical
> load with `/radical/det/lysoYieldScale 0.01` (energy scoring is unaffected).

---

## Output

A ROOT ntuple `radical` (per-event), columns include:

| column | meaning |
|---|---|
| `beamE_MeV` | primary kinetic energy |
| `eTotLyso_MeV` | total energy in all 29 LYSO layers |
| `eSM3_MeV`, `eSM5_MeV` | energy in 3 / 5 LYSO tiles around shower max |
| `showerMaxLayer` | LYSO layer index with max deposit |
| `eLayer0..28_MeV` | per-layer LYSO energy (→ reproduce Fig. 7 / 9) |
| `pe_ch0..7` | detected photoelectrons per SiPM channel |
| `tThr_ch0..7_ns` | leading-edge time (time of the 5th p.e.) per channel |
| `tFirst_ch0..7_ns` | first-photon arrival time per channel |
| `peSumUp/Dn/Sum` | summed p.e. (upstream / downstream / all) |
| `ePbGlass_MeV` | energy in the Pb-glass backing (leakage cut) |
| `eA1/eA2/eMcp_MeV` | energy in trigger counters / MCP |
| `beamX_mm`, `beamY_mm` | beam impact position (for the radial selection cut) |

To mirror the paper's event selection, cut on small `ePbGlass_MeV` (reject
leakage / non-EM and beam particles that miss the module) and on
`hypot(beamX_mm, beamY_mm) < 2 mm` (the wire-chamber radial cut). The default
beam spot is a 3 mm Gaussian (`/radical/gun/spotSize`); the 14 mm module does
not cover the whole spot, so some halo events miss it — exactly the population
those cuts remove.

The timing observables let you reconstruct the paper's `(Δt_DW ± Δt_UP)/2`
estimators offline. The per-layer energies reproduce the longitudinal shower
profile and the shower-max sampling used in the energy analysis.

---

## Assumptions & estimated inputs

The paper fixes the **geometry** and a few optical constants (emission/absorption
peaks, DSB1 τ = 3.5 ns). **No measured optical data exists for these components
yet**, so every other optical/scintillation number is an engineering estimate.
Each is tagged `ESTIMATE:` in the source (paper-derived values are tagged
`PAPER:`), and **all of them, with their basis, are catalogued in
[`ESTIMATES.md`](ESTIMATES.md)**. To list them:

```bash
grep -rn "ESTIMATE:" src include
```

Estimated inputs include: LYSO light yield / refractive index / decay / Birks;
DSB1 refractive index, WLS absorption-length spectrum and quantum yield; Tyvek
reflectivity (0.95); SiPM PDE (~40 % peak); and the timing threshold
(`kTimingThresholdPE`). Geometry-driven results (shower profile, sampling energy,
leakage) are robust to these; detected-p.e. counts and **absolute** timing
resolution scale with the light-yield × WLS-QY × PDE product and should be treated
as relative until tuned to bench data — see `ESTIMATES.md`.

Other modelling choices worth knowing:
- Capillary wall and quartz-rod fill are a single fused-silica body (the 25 µm
  core/filament gap and any cladding are neglected).
- The MCP is a simplified glass+quartz stand-in (right material budget, not an
  internal MCP structure); it is only a timing reference / upstream radiator.
- The module's outer 14×14 lateral faces are wrapped in a 0.1 mm **Tyvek shell**
  with the same diffuse ~95 % reflective surface as the inter-layer sheets (open
  at the z ends where capillaries exit). The structural POM housing itself is not
  placed; a POM material is defined if you wish to add a jacket.
- The "electronics" is a leading-edge discriminator on the detected-p.e. stream
  (time of the *k*-th p.e.). Swap in a fuller SiPM pulse/amplifier model as needed.

To match the paper's 0.008″ Tyvek instead of the requested 0.1 mm, edit
`kTyvekThick` in `include/RadicalConstants.hh`.

---

## File layout

```
CMakeLists.txt
radical.cc                     # main
include/ , src/
  RadicalConstants.hh          # all geometry numbers (edit here)
  RadicalMaterials.*           # materials + optical property tables
  DetectorConstruction.*       # geometry, optical surfaces, SDs
  DetectorMessenger.*          # /radical/det/ commands
  PhysicsList.*                # FTFP_BERT + EM option4 + optical
  PrimaryGeneratorAction.*     # electron beam
  CaloSD.* / CaloHit.hh        # per-layer LYSO energy scoring
  EventAction.* / RunAction.*  # event accumulation + ntuple
  SteppingAction.*             # SiPM photon detection + aux scoring
  NtupleDef.hh                 # fixed ntuple column layout
macros/
  init_vis.mac vis.mac run.mac scan_energy.mac
```
