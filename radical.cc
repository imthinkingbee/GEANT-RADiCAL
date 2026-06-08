//============================================================================
// radical.cc - main program for the RADiCAL module simulation.
//
//   ./radical              -> interactive session with visualization
//   ./radical run.mac      -> batch mode, executes the macro
//============================================================================
#include "DetectorConstruction.hh"
#include "PhysicsList.hh"
#include "ActionInitialization.hh"

#include "G4RunManagerFactory.hh"
#include "G4UImanager.hh"
#include "G4UIExecutive.hh"
#include "G4VisExecutive.hh"
#include "G4SystemOfUnits.hh"
#include "Randomize.hh"

#include <ctime>

int main(int argc, char** argv)
{
  // Detect batch vs interactive.
  G4UIExecutive* ui = nullptr;
  if (argc == 1) ui = new G4UIExecutive(argc, argv);

  // Seed RNG from time (override in macro with /random/setSeeds for reproducibility).
  CLHEP::HepRandom::setTheEngine(new CLHEP::RanecuEngine);
  CLHEP::HepRandom::setTheSeed((long)time(nullptr));

  auto* runManager =
      G4RunManagerFactory::CreateRunManager(G4RunManagerType::Default);

  runManager->SetUserInitialization(new DetectorConstruction());
  runManager->SetUserInitialization(new PhysicsList());
  runManager->SetUserInitialization(new ActionInitialization());

  // Visualization.
  auto* visManager = new G4VisExecutive();
  visManager->Initialize();

  auto* uiManager = G4UImanager::GetUIpointer();

  if (!ui) {
    // batch
    G4String command = "/control/execute ";
    G4String fileName = argv[1];
    uiManager->ApplyCommand(command + fileName);
  } else {
    // interactive
    uiManager->ApplyCommand("/control/execute init_vis.mac");
    ui->SessionStart();
    delete ui;
  }

  delete visManager;
  delete runManager;
  return 0;
}
