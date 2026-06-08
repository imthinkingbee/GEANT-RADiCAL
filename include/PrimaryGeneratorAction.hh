//============================================================================
// PrimaryGeneratorAction.hh - electron beam along +z, default 50 GeV.
// Controllable from macros via the standard /gun/ commands.
//============================================================================
#ifndef PrimaryGeneratorAction_hh
#define PrimaryGeneratorAction_hh 1

#include "G4VUserPrimaryGeneratorAction.hh"
#include "globals.hh"

class G4ParticleGun;
class G4Event;
class G4GenericMessenger;

class PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction
{
  public:
    PrimaryGeneratorAction();
    ~PrimaryGeneratorAction() override;

    void GeneratePrimaries(G4Event* event) override;
    G4ParticleGun* GetParticleGun() { return fGun; }

  private:
    G4ParticleGun* fGun = nullptr;
    G4GenericMessenger* fMessenger = nullptr;

    // Transverse Gaussian beam spot (sigma). Default 3 mm: a realistic H2 spot,
    // and large enough that the pencil beam does not thread the central capillary
    // hole. Set to 0 for a true pencil beam. Controlled by /radical/gun/spotSize.
    G4double fSpotSize = 3.0;   // mm (G4GenericMessenger applies the unit)
};

#endif
