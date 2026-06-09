//============================================================================
// RadicalConstants.hh
//
// Single source of truth for the RADiCAL module geometry and a few simulation
// switches. All numbers are taken from:
//   Perez-Lara et al., NIM A 1068 (2024) 169737 (Section 2, Figs. 1, 2)
// unless noted otherwise. Edit here to change the detector.
//============================================================================
#ifndef RadicalConstants_hh
#define RadicalConstants_hh 1

#include "G4SystemOfUnits.hh"
#include "globals.hh"

namespace radical
{
  //--------------------------------------------------------------------------
  // Module tile cross-section (Fig. 2): 14.0 x 14.0 mm^2
  //--------------------------------------------------------------------------
  constexpr G4double kTileXY = 14.0 * mm;

  //--------------------------------------------------------------------------
  // Longitudinal sampling structure (Section 2):
  //   29 LYSO:Ce plates, 1.5 mm thick   (active scintillator)
  //   28 W plates,       2.5 mm thick   (absorber)
  //   Tyvek reflector between EVERY successive plate interface.
  //
  // Paper uses 0.008" (= 0.2032 mm) Tyvek; per user request this build uses
  // 0.1 mm Tyvek between every plate interface (both faces of every LYSO).
  //--------------------------------------------------------------------------
  constexpr G4int    kNLyso      = 29;
  constexpr G4int    kNTungsten  = 28;
  constexpr G4double kLysoThick  = 1.5 * mm;
  constexpr G4double kWThick     = 2.5 * mm;
  constexpr G4double kTyvekThick = 0.1 * mm;   // user-requested value

  // Number of Tyvek sheets = interfaces between adjacent plates.
  // Stack pattern: LYSO (T W T) x28 ... ending on LYSO  => 56 Tyvek sheets.
  constexpr G4int    kNTyvek = kNLyso + kNTungsten - 1; // 56

  // Total active-stack length along the beam (z).
  constexpr G4double kStackLength =
      kNLyso * kLysoThick + kNTungsten * kWThick + kNTyvek * kTyvekThick;
  // = 43.5 + 70.0 + 5.6 = 119.1 mm

  //--------------------------------------------------------------------------
  // Capillary holes (Fig. 2)
  //   4 corner through-holes  phi 1.3 mm at (+/-3.5, +/-3.5) mm  -> capillaries
  //   1 central through-hole  phi 0.9 mm at (0,0)                -> unused (air)
  //--------------------------------------------------------------------------
  constexpr G4double kCornerHoleDia  = 1.3 * mm;
  constexpr G4double kCentralHoleDia = 0.9 * mm;
  constexpr G4double kHoleOffset     = 3.5 * mm; // corner hole offset from center

  //--------------------------------------------------------------------------
  // Quartz WLS capillary (Section 2):
  //   outer diameter 1150 um, inner (core) diameter 950 um, length 183 mm
  //   DSB1 WLS filament: 900 um diameter, 15 mm long, at shower max
  //   remainder of core: fused quartz rod
  //--------------------------------------------------------------------------
  constexpr G4double kCapOuterDia    = 1.150 * mm;
  constexpr G4double kCapCoreDia     = 0.950 * mm;
  constexpr G4double kCapLength      = 183.0 * mm;
  constexpr G4double kWlsFilamentDia = 0.900 * mm;
  constexpr G4double kWlsFilamentLen = 15.0  * mm;

  // Longitudinal position of the WLS filament centre, measured from the FRONT
  // (upstream, -z) face of the active stack. The paper places the 15 mm
  // filament at EM shower max, which for 25-150 GeV sits around LYSO layers
  // ~10-13. We centre it on LYSO tile index 11 by default (see Detector
  // Construction, can be overridden via /radical/det/wlsCenterZ).
  // ESTIMATE: WLS filament centred on LYSO layer 11. Basis: PAPER states the
  // filament sits at shower max (layers ~8-13 over 25-150 GeV); 11 is a
  // mid-range choice. Override per energy with /radical/det/wlsCenterZ.
  constexpr G4int    kWlsCenterLayer = 11;

  //--------------------------------------------------------------------------
  // SiPM (Hamamatsu HDR2) - one at each capillary end (4 capillaries x 2 ends
  // = 8 channels). Modelled as a thin silicon tile coupled to the capillary
  // end face. Channel id = corner*2 + end (end 0 = upstream/-z, 1 = downstream).
  //--------------------------------------------------------------------------
  // ESTIMATE: SiPM active size 1.3 x 1.3 mm, 0.3 mm thick. Basis: sized to fully
  // cover the 1.15 mm capillary end face; exact HDR2 die size not modelled.
  constexpr G4double kSiPMSize  = 1.3 * mm;   // active square side
  constexpr G4double kSiPMThick = 0.3 * mm;
  constexpr G4int    kNChannels = 8;

  //--------------------------------------------------------------------------
  // Overall module envelope (Section 2): 14 x 14 x 135 mm^3.
  // The active stack (119.1 mm) is centred inside; the remainder is the POM
  // end structure / air. Capillaries (183 mm) protrude beyond both ends to the
  // SiPM readout cards.
  //--------------------------------------------------------------------------
  constexpr G4double kModuleLength = 135.0 * mm;

  //--------------------------------------------------------------------------
  // Optical-photon tracking caps (SPEED + MEMORY).
  // Optical photons exceeding either cap are killed in SteppingAction. They
  // remove the long-lived strays that keep bouncing in the Tyvek/quartz without
  // being detected, which dominate CPU time and inflate the peak live-photon
  // count (the latter is what triggers out-of-memory kills on small-RAM
  // machines). The timing observables use the leading edge (first / k-th p.e.),
  // which arrives within a few ns, so these caps barely affect timing; they DO
  // trim the late tail of the detected-p.e. SUM. Set kMaxOpticalTime <= 0 to
  // disable the time cap.
  constexpr G4double kMaxOpticalTime  = 50.0 * ns;
  constexpr G4int    kMaxOpticalSteps = 2000;

  //--------------------------------------------------------------------------
  // Beam-test elements (Section 4, Fig. 11c). All optional via messenger.
  // Order along +z beam: A1 trigger, A2 trigger, MCP, MODULE(0), Pb-glass.
  //--------------------------------------------------------------------------
  // A1: 1x1 cm^2, A2: 2x2 cm^2 plastic scintillator trigger counters.
  constexpr G4double kA1XY = 10.0 * mm;
  constexpr G4double kA2XY = 20.0 * mm;
  constexpr G4double kTrigThick = 5.0 * mm;
  constexpr G4double kA1Z = -400.0 * mm;
  constexpr G4double kA2Z = -350.0 * mm;

  // MCP reference timing tube (Hamamatsu R3809U-50), simplified as a glass
  // window + body just upstream of the module.
  constexpr G4double kMcpZ        = -110.0 * mm;
  constexpr G4double kMcpBodyDia  = 45.0 * mm;
  constexpr G4double kMcpBodyLen  = 30.0 * mm;
  constexpr G4double kMcpWinThick = 2.0  * mm;

  // Pb-glass backing calorimeter: 2x2 array, total 4x4x40 cm^3.
  constexpr G4double kPbBlockXY  = 20.0 * mm;   // each block 2x2 cm
  constexpr G4double kPbBlockLen = 400.0 * mm;  // 40 cm long
  constexpr G4double kPbFrontZ   = 150.0 * mm;  // front face z

  //--------------------------------------------------------------------------
  // World
  //--------------------------------------------------------------------------
  constexpr G4double kWorldXY = 0.6 * m;
  constexpr G4double kWorldZ  = 1.5 * m;
}

#endif
