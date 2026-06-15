//============================================================================
// CaloSD.hh - sensitive detector for the LYSO layers.
// Creates one CaloHit per LYSO layer (indexed by the plate copy number) and
// accumulates the energy deposited in that layer per event.
//============================================================================
#ifndef CaloSD_hh
#define CaloSD_hh 1

#include "G4VSensitiveDetector.hh"
#include "CaloHit.hh"

class G4Step;
class G4HCofThisEvent;

class CaloSD : public G4VSensitiveDetector
{
  public:
    CaloSD(const G4String& name, const G4String& hitsCollectionName,
           G4int nLayers, G4int nModules);
    ~CaloSD() override = default;

    void   Initialize(G4HCofThisEvent* hce) override;
    G4bool ProcessHits(G4Step* step, G4TouchableHistory* hist) override;

  private:
    CaloHitsCollection* fHitsCollection = nullptr;
    G4int fNLayers;
    G4int fNModules;
    G4int fHCID = -1;
};

#endif
