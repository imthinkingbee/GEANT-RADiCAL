//============================================================================
// NtupleDef.hh - fixed column layout for the output ntuple.
// RunAction creates the columns in EXACTLY this order; EventAction fills by the
// same indices. Keep the two in sync via these constants only.
//============================================================================
#ifndef NtupleDef_hh
#define NtupleDef_hh 1

#include "globals.hh"
#include "RadicalConstants.hh"

namespace nt
{
  enum Col {
    kEventID = 0,   // I
    kBeamE,          // D  primary kinetic energy [MeV]
    kETotLyso,       // D  total energy in all LYSO layers [MeV]
    kESM3,           // D  energy in 3 LYSO tiles around shower max [MeV]
    kESM5,           // D  energy in 5 LYSO tiles around shower max [MeV]
    kShowerMaxLayer, // I  index of LYSO layer with max edep
    kELayerBase      // D  29 per-layer energies follow: kELayerBase .. +28
  };

  constexpr G4int kPEBase     = kELayerBase + radical::kNLyso;          // 8 I
  constexpr G4int kTThrBase   = kPEBase     + radical::kNChannels;      // 8 D
  constexpr G4int kTFirstBase = kTThrBase   + radical::kNChannels;      // 8 D
  constexpr G4int kPESumUp    = kTFirstBase + radical::kNChannels;      // D
  constexpr G4int kPESumDn    = kPESumUp + 1;                           // D
  constexpr G4int kPESum      = kPESumDn + 1;                           // D
  constexpr G4int kEPbGlass   = kPESum   + 1;                           // D
  constexpr G4int kEA1        = kEPbGlass + 1;                          // D
  constexpr G4int kEA2        = kEA1 + 1;                               // D
  constexpr G4int kEMcp       = kEA2 + 1;                               // D
  constexpr G4int kBeamX      = kEMcp + 1;                              // D  beam impact x [mm]
  constexpr G4int kBeamY      = kBeamX + 1;                             // D  beam impact y [mm]
  constexpr G4int kNColumns   = kBeamY + 1;

  // ESTIMATE: leading-edge timing threshold = 5 detected photoelectrons. Basis:
  // arbitrary stand-in for the paper's fixed-threshold high-gain discriminator;
  // tune to your electronics.
  constexpr G4int kTimingThresholdPE = 5;
}

#endif
