#include "PrimaryGeneratorAction.hh"
#include "RadicalConstants.hh"

#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4ParticleDefinition.hh"
#include "G4Event.hh"
#include "G4GenericMessenger.hh"
#include "G4SystemOfUnits.hh"
#include "Randomize.hh"

PrimaryGeneratorAction::PrimaryGeneratorAction()
{
  fGun = new G4ParticleGun(1);
  auto e = G4ParticleTable::GetParticleTable()->FindParticle("e-");
  fGun->SetParticleDefinition(e);
  fGun->SetParticleEnergy(50. * GeV);
  fGun->SetParticleMomentumDirection(G4ThreeVector(0, 0, 1));
  // Start just inside the upstream world wall, aimed along +z.
  fGun->SetParticlePosition(G4ThreeVector(0, 0, -radical::kWorldZ/2 + 1.*mm));

  fMessenger = new G4GenericMessenger(this, "/radical/gun/",
                                      "Beam spot controls.");
  auto& cmd = fMessenger->DeclareProperty("spotSize", fSpotSize,
      "Transverse Gaussian beam-spot sigma (default mm). 0 = pencil beam.");
  cmd.SetParameterName("sigma", true);
  cmd.SetDefaultValue("3.0");
}

PrimaryGeneratorAction::~PrimaryGeneratorAction()
{
  delete fMessenger;
  delete fGun;
}

void PrimaryGeneratorAction::GeneratePrimaries(G4Event* event)
{
  G4double x = 0., y = 0.;
  if (fSpotSize > 0.) {
    x = G4RandGauss::shoot(0., fSpotSize) * mm;
    y = G4RandGauss::shoot(0., fSpotSize) * mm;
  }
  fGun->SetParticlePosition(
      G4ThreeVector(x, y, -radical::kWorldZ/2 + 1.*mm));
  fGun->GeneratePrimaryVertex(event);
}
