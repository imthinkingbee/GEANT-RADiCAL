#include "CaloSD.hh"

#include "G4Step.hh"
#include "G4HCofThisEvent.hh"
#include "G4SDManager.hh"
#include "G4TouchableHistory.hh"
#include "G4ios.hh"

G4ThreadLocal G4Allocator<CaloHit>* CaloHitAllocator = nullptr;

CaloSD::CaloSD(const G4String& name, const G4String& hitsCollectionName,
               G4int nLayers, G4int nModules)
  : G4VSensitiveDetector(name), fNLayers(nLayers), fNModules(nModules)
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

  // One hit per (module, layer); index = module*nLayers + layer.
  for (G4int m = 0; m < fNModules; ++m)
    for (G4int l = 0; l < fNLayers; ++l)
      fHitsCollection->insert(new CaloHit(m, l));
}

G4bool CaloSD::ProcessHits(G4Step* step, G4TouchableHistory*)
{
  G4double edep = step->GetTotalEnergyDeposit();
  if (edep <= 0.) return false;

  auto touch = step->GetPreStepPoint()->GetTouchable();
  G4int layer  = touch->GetCopyNumber(0);   // LYSO plate
  G4int module = touch->GetCopyNumber(1);   // module envelope
  if (layer < 0 || layer >= fNLayers || module < 0 || module >= fNModules)
    return false;

  (*fHitsCollection)[module*fNLayers + layer]->AddEdep(edep);
  return true;
}
