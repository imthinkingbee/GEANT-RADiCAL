//============================================================================
// EventAction.hh
//============================================================================
#ifndef EventAction_hh
#define EventAction_hh 1

#include "G4UserEventAction.hh"
#include "RadicalConstants.hh"
#include "globals.hh"

#include <array>
#include <vector>

class EventAction : public G4UserEventAction
{
  public:
    EventAction() = default;
    ~EventAction() override = default;

    void BeginOfEventAction(const G4Event* event) override;
    void EndOfEventAction(const G4Event* event) override;

    // called from SteppingAction
    void AddSiPMDetection(G4int channel, G4double time);
    void AddPbGlassEdep(G4double e) { fPbGlassEdep += e; }
    void AddA1Edep(G4double e)      { fA1Edep += e; }
    void AddA2Edep(G4double e)      { fA2Edep += e; }
    void AddMcpEdep(G4double e)     { fMcpEdep += e; }

  private:
    G4int fLysoHCID = -1;

    std::array<std::vector<G4double>, radical::kNChannels> fPhotonTimes;
    G4double fPbGlassEdep = 0.;
    G4double fA1Edep = 0.;
    G4double fA2Edep = 0.;
    G4double fMcpEdep = 0.;
};

#endif
