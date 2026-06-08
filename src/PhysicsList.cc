#include "PhysicsList.hh"

#include "G4EmStandardPhysics_option4.hh"
#include "G4OpticalPhysics.hh"
#include "G4OpticalParameters.hh"
#include "G4SystemOfUnits.hh"

PhysicsList::PhysicsList(G4int verbose)
  : FTFP_BERT(verbose)
{
  // High-accuracy EM physics (better for low-energy e+/e-, important for
  // optical-photon production and shower shape at shower max).
  ReplacePhysics(new G4EmStandardPhysics_option4());

  // Optical physics: scintillation, Cerenkov, boundary, absorption, WLS, etc.
  RegisterPhysics(new G4OpticalPhysics());

  // Configure optical processes (G4 v11 uses the G4OpticalParameters singleton).
  auto op = G4OpticalParameters::Instance();
  op->SetScintTrackSecondariesFirst(true);
  op->SetCerenkovTrackSecondariesFirst(true);
  op->SetCerenkovMaxPhotonsPerStep(100);
  op->SetCerenkovMaxBetaChange(10.0);
  // Scintillation uses per-material yield with Poisson statistics by default,
  // giving realistic photostatistics. Keep ON.
  op->SetScintByParticleType(false);
}
