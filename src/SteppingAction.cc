#include "SteppingAction.hh"
#include "EventAction.hh"

#include "G4Step.hh"
#include "G4Track.hh"
#include "G4OpticalPhoton.hh"
#include "G4OpBoundaryProcess.hh"
#include "G4ProcessManager.hh"
#include "G4ProcessVector.hh"
#include "G4VPhysicalVolume.hh"
#include "G4TouchableHandle.hh"
#include "G4SystemOfUnits.hh"

void SteppingAction::UserSteppingAction(const G4Step* step)
{
  auto track = step->GetTrack();

  // ---------- optical photon: look for detection at a SiPM ----------
  if (track->GetDefinition() == G4OpticalPhoton::OpticalPhotonDefinition()) {

    // cache the boundary process pointer (thread-local action object)
    if (!fBoundary) {
      auto pm = track->GetDefinition()->GetProcessManager();
      auto pv = pm->GetProcessList();
      for (G4int i = 0; i < (G4int)pv->size(); ++i) {
        if ((*pv)[i]->GetProcessName() == "OpBoundary") {
          fBoundary = dynamic_cast<G4OpBoundaryProcess*>((*pv)[i]);
          break;
        }
      }
    }

    auto post = step->GetPostStepPoint();
    if (fBoundary && post->GetStepStatus() == fGeomBoundary) {
      if (fBoundary->GetStatus() == Detection) {
        auto vol = post->GetPhysicalVolume();
        if (vol && vol->GetName() == "SiPM") {
          G4int ch = post->GetTouchableHandle()->GetCopyNumber(0);
          fEvent->AddSiPMDetection(ch, post->GetGlobalTime());
        }
      }
    }
    return;
  }

  // ---------- charged/neutral: score energy in auxiliary volumes ----------
  G4double edep = step->GetTotalEnergyDeposit();
  if (edep <= 0.) return;

  auto vol = step->GetPreStepPoint()->GetTouchableHandle()->GetVolume();
  if (!vol) return;
  const G4String& name = vol->GetName();

  if (name == "PbGlass")      fEvent->AddPbGlassEdep(edep);
  else if (name == "A1")      fEvent->AddA1Edep(edep);
  else if (name == "A2")      fEvent->AddA2Edep(edep);
  else if (name == "MCPbody" || name == "MCPwin") fEvent->AddMcpEdep(edep);
  // LYSO energy is handled by CaloSD.
}
