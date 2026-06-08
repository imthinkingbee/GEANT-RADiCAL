#include "CaloSD.hh"

#include "G4Step.hh"
#include "G4HCofThisEvent.hh"
#include "G4SDManager.hh"
#include "G4TouchableHistory.hh"
#include "G4ios.hh"

G4ThreadLocal G4Allocator<CaloHit>* CaloHitAllocator = nullptr;

CaloSD::CaloSD(const G4String& name, const G4String& hitsCollectionName, G4int nLayers)
  : G4VSensitiveDetector(name), fNLayers(nLayers)
{
  collectionName.insert(hitsCollectionName);
}

void CaloSD::Initialize(G4HCofThisEvent* hce)
{
  fHitsCollection =
      new CaloHitsCollection(SensitiveDetectorName, collectionName[0]);

  if (fHCID < 0)
    fHCID = G4SDManager::GetSDMpointer()->GetCollectionID(fHitsCollection);
  hce->AddHitsCollection(fHCID, fHitsCollection);

  // Pre-create one hit per layer so indexing by copy number is direct.
  for (G4int i = 0; i < fNLayers; ++i)
    fHitsCollection->insert(new CaloHit(i));
}

G4bool CaloSD::ProcessHits(G4Step* step, G4TouchableHistory*)
{
  G4double edep = step->GetTotalEnergyDeposit();
  if (edep <= 0.) return false;

  // The LYSO plate copy number is the layer index 0..28.
  G4int layer = step->GetPreStepPoint()->GetTouchable()->GetCopyNumber(0);
  if (layer < 0 || layer >= fNLayers) return false;

  (*fHitsCollection)[layer]->AddEdep(edep);
  return true;
}
