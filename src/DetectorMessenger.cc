#include "DetectorMessenger.hh"
#include "DetectorConstruction.hh"

#include "G4UIdirectory.hh"
#include "G4UIcmdWithABool.hh"
#include "G4UIcmdWithADouble.hh"
#include "G4UIcmdWithADoubleAndUnit.hh"
#include "G4UIcmdWithAString.hh"

DetectorMessenger::DetectorMessenger(DetectorConstruction* det)
  : fDet(det)
{
  fDir = new G4UIdirectory("/radical/det/");
  fDir->SetGuidance("RADiCAL detector construction controls.");

  fGeomCmd = new G4UIcmdWithAString("/radical/det/geometry", this);
  fGeomCmd->SetGuidance("Geometry mode: single | array3x3 | hex.");
  fGeomCmd->SetParameterName("mode", false);
  fGeomCmd->SetCandidates("single array3x3 hex");
  fGeomCmd->AvailableForStates(G4State_PreInit);

  fBeamlineCmd = new G4UIcmdWithABool("/radical/det/buildBeamline", this);
  fBeamlineCmd->SetGuidance("Build the full beam-test beamline (triggers/MCP/Pb-glass).");
  fBeamlineCmd->SetParameterName("flag", true);
  fBeamlineCmd->SetDefaultValue(true);
  fBeamlineCmd->AvailableForStates(G4State_PreInit);

  fPbGlassCmd = new G4UIcmdWithABool("/radical/det/buildPbGlass", this);
  fPbGlassCmd->SetGuidance("Build the downstream Pb-glass backing calorimeter.");
  fPbGlassCmd->SetParameterName("flag", true);
  fPbGlassCmd->AvailableForStates(G4State_PreInit);

  fMcpCmd = new G4UIcmdWithABool("/radical/det/buildMCP", this);
  fMcpCmd->SetGuidance("Build the upstream MCP reference-timing tube.");
  fMcpCmd->SetParameterName("flag", true);
  fMcpCmd->AvailableForStates(G4State_PreInit);

  fTrigCmd = new G4UIcmdWithABool("/radical/det/buildTriggers", this);
  fTrigCmd->SetGuidance("Build the A1/A2 plastic trigger counters.");
  fTrigCmd->SetParameterName("flag", true);
  fTrigCmd->AvailableForStates(G4State_PreInit);

  fYieldCmd = new G4UIcmdWithADouble("/radical/det/lysoYieldScale", this);
  fYieldCmd->SetGuidance("Scale the LYSO scintillation light yield (1.0 = nominal 30000/MeV).");
  fYieldCmd->SetParameterName("scale", true);
  fYieldCmd->SetDefaultValue(1.0);
  fYieldCmd->AvailableForStates(G4State_PreInit);

  fWlsZCmd = new G4UIcmdWithADoubleAndUnit("/radical/det/wlsCenterZ", this);
  fWlsZCmd->SetGuidance("Override the WLS filament centre z (module-centred coords).");
  fWlsZCmd->SetParameterName("z", false);
  fWlsZCmd->SetUnitCategory("Length");
  fWlsZCmd->AvailableForStates(G4State_PreInit);
}

DetectorMessenger::~DetectorMessenger()
{
  delete fGeomCmd;
  delete fBeamlineCmd;
  delete fPbGlassCmd;
  delete fMcpCmd;
  delete fTrigCmd;
  delete fYieldCmd;
  delete fWlsZCmd;
  delete fDir;
}

void DetectorMessenger::SetNewValue(G4UIcommand* cmd, G4String val)
{
  if (cmd == fGeomCmd) fDet->SetGeometry(val);
  else if (cmd == fBeamlineCmd) fDet->SetBuildBeamline(fBeamlineCmd->GetNewBoolValue(val));
  else if (cmd == fPbGlassCmd) fDet->SetBuildPbGlass(fPbGlassCmd->GetNewBoolValue(val));
  else if (cmd == fMcpCmd) fDet->SetBuildMCP(fMcpCmd->GetNewBoolValue(val));
  else if (cmd == fTrigCmd) fDet->SetBuildTriggers(fTrigCmd->GetNewBoolValue(val));
  else if (cmd == fYieldCmd) fDet->SetLysoYieldScale(fYieldCmd->GetNewDoubleValue(val));
  else if (cmd == fWlsZCmd) fDet->SetWlsCenterZ(fWlsZCmd->GetNewDoubleValue(val));
}
