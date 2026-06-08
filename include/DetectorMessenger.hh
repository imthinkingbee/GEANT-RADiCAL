//============================================================================
// DetectorMessenger.hh - /radical/det/... UI commands.
//============================================================================
#ifndef DetectorMessenger_hh
#define DetectorMessenger_hh 1

#include "G4UImessenger.hh"
#include "globals.hh"

class DetectorConstruction;
class G4UIdirectory;
class G4UIcmdWithABool;
class G4UIcmdWithADouble;
class G4UIcmdWithADoubleAndUnit;

class DetectorMessenger : public G4UImessenger
{
  public:
    explicit DetectorMessenger(DetectorConstruction* det);
    ~DetectorMessenger() override;

    void SetNewValue(G4UIcommand* cmd, G4String val) override;

  private:
    DetectorConstruction* fDet;
    G4UIdirectory*        fDir;
    G4UIcmdWithABool*     fBeamlineCmd;
    G4UIcmdWithABool*     fPbGlassCmd;
    G4UIcmdWithABool*     fMcpCmd;
    G4UIcmdWithABool*     fTrigCmd;
    G4UIcmdWithADouble*   fYieldCmd;
    G4UIcmdWithADoubleAndUnit* fWlsZCmd;
};

#endif
