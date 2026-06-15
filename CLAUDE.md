# CLAUDE.md — RADiCAL simulation developer reference

Dense reference for modifying this GEANT4 simulation **without re-reading the
paper or sweeping every source file**. Read this first; open specific files only
as needed. Keep this file updated when the design changes.

## What this is
GEANT4 **v11.3.2** simulation of the **RADiCAL** ultra-compact sampling
calorimeter and its CERN H2 beam test, from:
Perez-Lara et al., *NIM A* **1068** (2024) 169737 (open access; PDF at
`~/Downloads/1-s2.0-S0168900224006636-main.pdf`).
Repo: https://github.com/imthinkingbee/GEANT-RADiCAL (branch `main`).

## Environment / build / run (this machine)
- GEANT4 + CMake live in a **conda env named `geant4`** (conda-forge).
  Activate: `source /opt/homebrew/Caskroom/miniforge/base/etc/profile.d/conda.sh && conda activate geant4`
- Build: `./configure.sh` (from repo root) — activates env, runs CMake+make into `build/`.
  - macOS gotcha it handles: the system SDK ships expat 2.5.0 but GEANT4 needs
    ≥2.8.1, so CMake must be pointed at the conda expat via
    `-DEXPAT_INCLUDE_DIR`/`-DEXPAT_LIBRARY`. Plain `make` in `build/` is fine after.
- Run: `cd build && ./radical run.mac` (macros are copied into `build/` by CMake).
  Interactive (vis): `./radical` with no args.
- GitHub: `gh` CLI is installed and authed as **imthinkingbee**; `git push` works
  (gh credential helper configured via `gh auth setup-git`). Repo git identity is
  set locally (login + noreply email).
- No `timeout` on macOS. Don't chain foreground `sleep`; run long jobs with
  `run_in_background: true`. Full-optics runs auto-background quickly.

## Macros (`macros/`, copied to `build/`)
- `init_vis.mac`/`vis.mac` — interactive visualization.
- `run.mac` — single 50 GeV run, full physics+optics, 200 evts.
- `scan_energy.mac` — 25–150 GeV.
- `scan_resolution.mac` — **5, 25, 50 … 200 GeV** (9 pts), 2000 evts each; input
  to the analysis script.

## Code map (`include/` + `src/`, `radical.cc` is main)
- `RadicalConstants.hh` — **ALL geometry numbers + a few switches. Edit here to
  change the detector.** namespace `radical`.
- `RadicalMaterials.{hh,cc}` — singleton; builds materials + optical property
  tables (RINDEX, scintillation, WLS, surfaces' inputs).
- `DetectorConstruction.{hh,cc}` — geometry, optical surfaces, SD assignment.
  `BuildModule` (stack+wrap), `BuildCapillaries` (4 caps+WLS+8 SiPM),
  `BuildBeamline` (A1/A2/MCP/Pb-glass), `ApplyOpticalSurfaces`, `MakeHoledPlate`.
- `DetectorMessenger.{hh,cc}` — `/radical/det/...` commands.
- `PhysicsList.{hh,cc}` — `FTFP_BERT` + `G4EmStandardPhysics_option4` +
  `G4OpticalPhysics`; optical params via `G4OpticalParameters`.
- `PrimaryGeneratorAction.{hh,cc}` — e- gun; Gaussian beam spot (`fSpotSize`,
  default 3 mm) via `/radical/gun/spotSize`.
- `CaloSD.{hh,cc}` + `CaloHit.hh` — per-LYSO-layer energy scoring (one hit/layer,
  indexed by plate copy number).
- `SteppingAction.{hh,cc}` — (a) SiPM photon detection via OpBoundary
  `Detection` status; (b) edep scoring in Pb-glass/A1/A2/MCP by volume name.
- `EventAction.{hh,cc}` — reads CaloSD hits, computes shower-max window, p.e.,
  timing stamps, fills ntuple.
- `RunAction.{hh,cc}` — creates/writes the ntuple (column order!).
- `NtupleDef.hh` — **fixed ntuple column layout** (see invariant below).
- `ActionInitialization.{hh,cc}` — wires actions (EventAction shared with
  Stepping/Run on workers).

## Geometry modes (`/radical/det/geometry <mode>`, PreInit)
Three modes, selected at PreInit; default `single`. Each module's internals
(plate stack + Tyvek wrap + capillaries + SiPMs) live in ONE air **envelope** LV,
placed once (single/hex) or 9× (array). **Module index = envelope placement copy
number (touchable depth 1); layer = LYSO copy number (depth 0).** This is how
`CaloSD` and SiPM detection tag the module. `MakeSpec()` defines each mode's tile
shape, plate thickness and hole list; `BuildModuleEnvelope()` builds it.
- `single` — baseline 14×14 module: 4 corner caps + 1 empty central hole (Fig. 2).
- `enhanced` — **single** 18×18 module with a **3×3 grid of 9 capillaries** (Fig.
  28), LYSO 3.0 mm (`kEnhLysoThick`, ESTIMATE = 2×). 1 module. (This is the
  "9-capillary / 3×3-capillary" design — NOT an array of modules.)
- `array3x3` — 3×3 ARRAY of 9 of the enhanced modules above, for transverse
  containment (paper Section 7). Pitch = tile+2·Tyvek+gap. 9 modules → copy 0..8
  (row-major, centre = 4). `enhanced` and `array3x3` share the same module spec;
  the only difference is 1 vs 9 envelope placements (see Construct()).
- `hex` — hexagonal module (`G4Polyhedra`, flat-to-flat 14 mm), 7 caps (6 ring at
  `kHexRingR` + 1 centre), baseline stack.
- New-geometry constants live in `RadicalConstants.hh` (`kEnh*`, `kHex*`,
  `kArray*`, `kMaxModules=9`). Scan macros: `scan_array3x3.mac`, `scan_hex.mac`
  (optics off, energy/containment).
- Ntuple gained `eMod0..8_MeV` + `peMod0..8` (per-module energy / p.e.). For
  `single` only index 0 is used (`eMod0==eTotLyso`). `eTotLyso`/`eLayer`/`eSM*`
  use the **struck** module (max-energy); for array that's the central one.
- Verified: single 50 GeV ≈ 6.7 GeV (unchanged); array central containment ≈ 76 %
  with symmetric neighbour leakage; hex ≈ 5.2 GeV (smaller area). No overlaps.

## Geometry (baseline single module; all numbers in `RadicalConstants.hh`)
- Module 14×14 mm² cross-section; active stack 119.1 mm (centred at z=0, beam +z).
- 29 LYSO (1.5 mm) interleaved 28 W (2.5 mm); pattern `LYSO (T W T)…LYSO`.
- **Tyvek 0.1 mm between EVERY interface (56 sheets) + a 0.1 mm Tyvek outer wrap**
  on the 4 lateral faces (open z-ends). `kTyvekThick`. (Paper uses 0.203 mm; 0.1
  is the user's choice.)
- 5 holes (Fig. 2): 4 corner φ1.3 mm at **(±3.5, ±3.5) mm** → capillaries;
  1 central φ0.9 mm at (0,0) → **empty (air)**.
- Capillary: fused-silica cyl OD 1.15 mm, L 183 mm (wall+rod = one quartz body),
  with DSB1 WLS filament φ0.9×15 mm at shower max (`kWlsCenterLayer=11`, override
  `/radical/det/wlsCenterZ`). Caps centred at z=0 (local z == world z).
- 8 SiPM (Si tiles, 1.3 mm sq) at the 4 cap ends.
- Beamline along +z: A1(z=-400) A2(-350) MCP(-110) MODULE(0) Pb-glass(front +150).
  Toggle via `/radical/det/buildBeamline|buildMCP|buildTriggers|buildPbGlass`.

## Conventions / invariants (DON'T break these)
- **Ntuple column order is fixed.** `NtupleDef.hh` defines indices; `RunAction`
  creates columns in that exact order; `EventAction` fills by the same indices.
  To add a column: append in all three (enum, RunAction Create*, EventAction
  Fill*). Column ids increment across I and D columns together.
- **LYSO plate copy number == layer index 0..28** (CaloSD relies on it).
- **SiPM copy number == channel id = corner*2 + end**, end 0 = upstream(−z),
  1 = downstream(+z). So upstream = even {0,2,4,6}, downstream = odd {1,3,5,7}.
- LYSO energy → CaloSD (hits). All other scored volumes (PbGlass/A1/A2/MCP*) →
  SteppingAction by **volume name** (`G4String` compare). Keep names in sync.
- Optical-photon detection happens in SteppingAction when OpBoundary status ==
  `Detection` at a volume named `SiPM`; time = post-step global time.
- Reflective surfaces are `G4LogicalSkinSurface` applied to every LV in
  `fTyvekLVs` (inter-layer LV + wrap LV) and to the SiPM LV. New reflective
  Tyvek pieces must be pushed to `fTyvekLVs` before `ApplyOpticalSurfaces`.

## Estimates vs. measured (IMPORTANT for physics edits)
- The paper fixes geometry + a few optical numbers only. **No measured optical
  data exists yet.** Every assumed value is tagged `ESTIMATE:` (with basis) in
  source; paper values tagged `PAPER:`. Catalogue: `ESTIMATES.md`.
  - `grep -rn "ESTIMATE:" src include` (≈22 hits).
- Biggest lever on detected light/timing: LYSO yield × DSB1 WLS quantum yield
  (`WLSMEANNUMBERPHOTONS`, currently optimistic 1.0) × SiPM PDE.
- Geometry/EM results (shower profile, sampling energy, leakage) are robust;
  absolute p.e. and absolute σ_t are NOT until tuned to bench data.

## Output + analysis
- ROOT ntuple `radical` per event. Key cols: `beamE_MeV`, `eTotLyso_MeV`,
  `eSM3_MeV`/`eSM5_MeV`, `showerMaxLayer`, `eLayer0..28_MeV`, `pe_ch0..7`,
  `tThr_ch0..7_ns` (time of kth p.e., k=`kTimingThresholdPE`=5),
  `tFirst_ch0..7_ns`, `peSumUp/Dn/Sum`, `ePbGlass_MeV`, `eA1/eA2/eMcp_MeV`,
  `beamX_mm`, `beamY_mm`.
- `analysis/analyze_resolution.py --indir build --outdir build/plots`:
  energy resolution (σ_E/E from `eTotLyso`, fit a/√E⊕b/E⊕c) and timing
  (σ_t from q=(⟨t_dn⟩−⟨t_up⟩)/2, fit a/√E⊕b). Applies cuts: radial<2 mm and
  ePbGlass<0.2·beamE. Deps in `analysis/requirements.txt` (uproot/scipy/mpl).
  Branch names include unit suffix, e.g. `tThr_ch0_ns` (not `tThr_ch0`).
- Validated numbers (sanity): energy-only run gave σ_E/E ≈ 11%/√E ⊕ 0.9%.

## Known gotchas (learned the hard way)
- **Central-hole channeling**: a pencil beam at exactly (0,0) goes down the empty
  φ0.9 mm central hole and deposits ~0 in the module. Always use a finite beam
  spot (default 3 mm). Events that miss the 14 mm module (large `ePbGlass`,
  large radius) are real beam halo — cut them offline (the analysis does).
- **`/process/optical/processActivation Cerenkov|Scintillation false` must be
  issued BEFORE `/run/initialize`** (PreInit), else "Illegal application state".
  This is the fast path for energy-only runs (no optical photons).
- **Performance + MEMORY (this machine has only 8 GB RAM)**: full optics is very
  heavy (10⁷–10⁸ photons/evt; 18 cm capillary TIR + 95% Tyvek = many steps/photon).
  Each MT worker holds one event's photon stack, so peak RAM ≈ per-event × threads.
  On 8 GB, too many threads → **OS OOM-kills the job during the first event**
  ("killed after one event"). Mitigations, in order of impact:
  1. Energy resolution → run **optics off** (`scan_energy_fast.mac`; fast, tiny RAM,
     can use all 8 threads). Optics off via `/process/optical/processActivation
     Cerenkov|Scintillation false` (PreInit).
  2. Full optics → **few threads** (run.mac defaults to 2) and/or
     `/radical/det/lysoYieldScale <small>` (fewer photons; changes p.e. scale —
     not for absolute light/timing).
  3. Optical-photon caps in `SteppingAction` kill strays older than
     `kMaxOpticalTime` (50 ns) or beyond `kMaxOpticalSteps` (2000) — see
     `RadicalConstants.hh`. Cuts CPU tail + caps peak memory; trims late p.e. sum
     but not the leading-edge timing. Tune/disable there.
  - Split scans committed: `scan_energy_fast.mac` (optics off) +
    `scan_timing.mac` (optics on, fewer events). `scan_resolution.mac` = full both.
- **MT thread-safety**: LYSO yield rescaling is done once in `Construct()` (master),
  NOT per-worker in `ConstructSDandField()`.
- Geometry uses `checkOverlaps=true` on all placements; all currently pass.
  Boolean-subtracted plates + capillaries-in-world rely on the corner-hole
  (r0.65) > capillary (r0.575) clearance.

## Common edits — where to touch
- Change a dimension/count → `RadicalConstants.hh` (and `kWlsCenterLayer`).
- Add/adjust an optical property → `RadicalMaterials.cc` (tag `ESTIMATE:`).
- Add a scored volume → place it (`DetectorConstruction`), add an accumulator in
  `EventAction`, score it in `SteppingAction` by name (or a new SD), add ntuple
  column in all three of NtupleDef/RunAction/EventAction.
- Change beam → `PrimaryGeneratorAction` / `/gun/` + `/radical/gun/spotSize`.
- Change physics lists → `PhysicsList.cc`.
