#include "RunAction.hh"
#include "NtupleDef.hh"
#include "RadicalConstants.hh"

#include "G4Run.hh"
#include "G4AnalysisManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4ios.hh"

#include <string>

using namespace radical;

RunAction::RunAction()
{
  auto ana = G4AnalysisManager::Instance();
  ana->SetDefaultFileType("root");
  ana->SetNtupleMerging(true);   // merge worker ntuples in MT mode
  ana->SetVerboseLevel(1);
  ana->SetFileName("radical");   // -> radical.root (override with /analysis/setFileName)

  // ----- create the ntuple (column order MUST match NtupleDef.hh) -----
  ana->CreateNtuple("radical", "RADiCAL module per-event data");
  ana->CreateNtupleIColumn("eventID");          // kEventID
  ana->CreateNtupleDColumn("beamE_MeV");        // kBeamE
  ana->CreateNtupleDColumn("eTotLyso_MeV");     // kETotLyso
  ana->CreateNtupleDColumn("eSM3_MeV");         // kESM3
  ana->CreateNtupleDColumn("eSM5_MeV");         // kESM5
  ana->CreateNtupleIColumn("showerMaxLayer");   // kShowerMaxLayer
  for (G4int i = 0; i < kNLyso; ++i)
    ana->CreateNtupleDColumn("eLayer" + std::to_string(i) + "_MeV");
  for (G4int c = 0; c < kNChannels; ++c)
    ana->CreateNtupleIColumn("pe_ch" + std::to_string(c));
  for (G4int c = 0; c < kNChannels; ++c)
    ana->CreateNtupleDColumn("tThr_ch" + std::to_string(c) + "_ns");
  for (G4int c = 0; c < kNChannels; ++c)
    ana->CreateNtupleDColumn("tFirst_ch" + std::to_string(c) + "_ns");
  ana->CreateNtupleDColumn("peSumUp");          // kPESumUp
  ana->CreateNtupleDColumn("peSumDn");          // kPESumDn
  ana->CreateNtupleDColumn("peSum");            // kPESum
  ana->CreateNtupleDColumn("ePbGlass_MeV");     // kEPbGlass
  ana->CreateNtupleDColumn("eA1_MeV");          // kEA1
  ana->CreateNtupleDColumn("eA2_MeV");          // kEA2
  ana->CreateNtupleDColumn("eMcp_MeV");         // kEMcp
  ana->CreateNtupleDColumn("beamX_mm");         // kBeamX
  ana->CreateNtupleDColumn("beamY_mm");         // kBeamY
  for (G4int m = 0; m < kMaxModules; ++m)       // kEModBase: per-module energy
    ana->CreateNtupleDColumn("eMod" + std::to_string(m) + "_MeV");
  for (G4int m = 0; m < kMaxModules; ++m)       // kPeModBase: per-module p.e.
    ana->CreateNtupleIColumn("peMod" + std::to_string(m));
  ana->FinishNtuple();
}

RunAction::~RunAction() = default;

void RunAction::BeginOfRunAction(const G4Run*)
{
  G4AnalysisManager::Instance()->OpenFile();
}

void RunAction::EndOfRunAction(const G4Run* run)
{
  auto ana = G4AnalysisManager::Instance();
  ana->Write();
  ana->CloseFile();

  if (IsMaster())
    G4cout << "[RADiCAL] Run finished: " << run->GetNumberOfEvent()
           << " events. Output written to " << ana->GetFileName() << G4endl;
}
