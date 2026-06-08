//============================================================================
// SteppingAction.hh
//  - detects optical photons absorbed at the SiPM (boundary "Detection")
//  - accumulates energy deposited in the non-LYSO scoring volumes
//============================================================================
#ifndef SteppingAction_hh
#define SteppingAction_hh 1

#include "G4UserSteppingAction.hh"
#include "globals.hh"

class EventAction;
class G4OpBoundaryProcess;

class SteppingAction : public G4UserSteppingAction
{
  public:
    explicit SteppingAction(EventAction* ev) : fEvent(ev) {}
    ~SteppingAction() override = default;

    void UserSteppingAction(const G4Step* step) override;

  private:
    EventAction* fEvent = nullptr;
    G4OpBoundaryProcess* fBoundary = nullptr;
};

#endif
