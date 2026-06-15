#include "EventAction.hh"
#include "CaloHit.hh"
#include "NtupleDef.hh"

#include "G4Event.hh"
#include "G4HCofThisEvent.hh"
#include "G4SDManager.hh"
#include "G4PrimaryVertex.hh"
#include "G4PrimaryParticle.hh"
#include "G4AnalysisManager.hh"
#include "G4SystemOfUnits.hh"

#include <algorithm>
#include <numeric>

using namespace radical;

void EventAction::BeginOfEventAction(const G4Event*)
{
  for (auto& v : fPhotonTimes) v.clear();
  fPeMod.fill(0);
  fPbGlassEdep = fA1Edep = fA2Edep = fMcpEdep = 0.;
}

void EventAction::AddSiPMDetection(G4int channel, G4int module, G4double time)
{
  if (module >= 0 && module < kMaxModules) fPeMod[module]++;
  // per-channel timing arrays track module 0 only (single-mode channels)
  if (module == 0 && channel >= 0 && channel < kNChannels)
    fPhotonTimes[channel].push_back(time);
}

void EventAction::EndOfEventAction(const G4Event* event)
{
  // ----- read LYSO energy from the calorimeter hits collection -----
  if (fLysoHCID < 0)
    fLysoHCID = G4SDManager::GetSDMpointer()->GetCollectionID("LYSO_HC");

  // per-(module,layer) energy, per-module totals, grand total
  std::array<G4double, kMaxModules*kNLyso> cell{}; cell.fill(0.);
  std::array<G4double, kMaxModules> eMod{}; eMod.fill(0.);
  G4double eTot = 0.;

  auto hce = event->GetHCofThisEvent();
  if (hce) {
    auto hc = static_cast<CaloHitsCollection*>(hce->GetHC(fLysoHCID));
    if (hc) {
      for (size_t i = 0; i < hc->entries(); ++i) {
        auto* hit = (*hc)[i];
        G4int m = hit->GetModule(), l = hit->GetLayer();
        if (m >= 0 && m < kMaxModules && l >= 0 && l < kNLyso) {
          cell[m*kNLyso + l] += hit->GetEdep();
          eMod[m] += hit->GetEdep();
          eTot   += hit->GetEdep();
        }
      }
    }
  }

  // struck module = the one with the most energy; its per-layer profile is used
  // for the shower-max observables and the eLayer columns.
  G4int struck = 0;
  for (G4int m = 1; m < kMaxModules; ++m) if (eMod[m] > eMod[struck]) struck = m;

  std::array<G4double, kNLyso> layerE{};
  for (G4int l = 0; l < kNLyso; ++l) layerE[l] = cell[struck*kNLyso + l];

  // shower-max layer (argmax) and energy summed over +/-1, +/-2 tiles
  G4int smLayer = 0;
  for (G4int i = 1; i < kNLyso; ++i)
    if (layerE[i] > layerE[smLayer]) smLayer = i;

  auto sumWindow = [&](G4int half) {
    G4double s = 0.;
    for (G4int i = smLayer - half; i <= smLayer + half; ++i)
      if (i >= 0 && i < kNLyso) s += layerE[i];
    return s;
  };
  G4double eSM3 = sumWindow(1);
  G4double eSM5 = sumWindow(2);

  // ----- SiPM photostatistics / timing -----
  std::array<G4int, kNChannels> pe{};
  std::array<G4double, kNChannels> tThr{};
  std::array<G4double, kNChannels> tFirst{};
  for (G4int c = 0; c < kNChannels; ++c) {
    auto& v = fPhotonTimes[c];
    pe[c] = (G4int)v.size();
    if (!v.empty()) {
      std::sort(v.begin(), v.end());
      tFirst[c] = v.front();
      tThr[c] = (pe[c] >= nt::kTimingThresholdPE)
                    ? v[nt::kTimingThresholdPE - 1]
                    : -1.;
    } else {
      tFirst[c] = -1.;
      tThr[c] = -1.;
    }
  }

  G4double peUp = 0., peDn = 0.;
  for (G4int c = 0; c < 4; ++c) { peUp += pe[c*2 + 0]; peDn += pe[c*2 + 1]; }
  G4double peSum = peUp + peDn;

  // ----- primary beam energy and impact position -----
  G4double beamE = 0., beamX = 0., beamY = 0.;
  if (auto* pv = event->GetPrimaryVertex()) {
    beamX = pv->GetX0();
    beamY = pv->GetY0();
    if (pv->GetPrimary()) beamE = pv->GetPrimary()->GetKineticEnergy();
  }

  // ----- fill ntuple -----
  auto ana = G4AnalysisManager::Instance();
  ana->FillNtupleIColumn(nt::kEventID, event->GetEventID());
  ana->FillNtupleDColumn(nt::kBeamE, beamE);
  ana->FillNtupleDColumn(nt::kETotLyso, eTot);
  ana->FillNtupleDColumn(nt::kESM3, eSM3);
  ana->FillNtupleDColumn(nt::kESM5, eSM5);
  ana->FillNtupleIColumn(nt::kShowerMaxLayer, smLayer);
  for (G4int i = 0; i < kNLyso; ++i)
    ana->FillNtupleDColumn(nt::kELayerBase + i, layerE[i]);
  for (G4int c = 0; c < kNChannels; ++c) {
    ana->FillNtupleIColumn(nt::kPEBase + c, pe[c]);
    ana->FillNtupleDColumn(nt::kTThrBase + c, tThr[c]);
    ana->FillNtupleDColumn(nt::kTFirstBase + c, tFirst[c]);
  }
  ana->FillNtupleDColumn(nt::kPESumUp, peUp);
  ana->FillNtupleDColumn(nt::kPESumDn, peDn);
  ana->FillNtupleDColumn(nt::kPESum, peSum);
  ana->FillNtupleDColumn(nt::kEPbGlass, fPbGlassEdep);
  ana->FillNtupleDColumn(nt::kEA1, fA1Edep);
  ana->FillNtupleDColumn(nt::kEA2, fA2Edep);
  ana->FillNtupleDColumn(nt::kEMcp, fMcpEdep);
  ana->FillNtupleDColumn(nt::kBeamX, beamX);
  ana->FillNtupleDColumn(nt::kBeamY, beamY);
  for (G4int m = 0; m < kMaxModules; ++m) {
    ana->FillNtupleDColumn(nt::kEModBase + m, eMod[m]);
    ana->FillNtupleIColumn(nt::kPeModBase + m, fPeMod[m]);
  }
  ana->AddNtupleRow();
}
