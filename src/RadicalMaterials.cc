#include "RadicalMaterials.hh"

#include "G4Material.hh"
#include "G4NistManager.hh"
#include "G4MaterialPropertiesTable.hh"
#include "G4Element.hh"
#include "G4SystemOfUnits.hh"
#include "G4PhysicalConstants.hh"

#include <vector>

RadicalMaterials* RadicalMaterials::fInstance = nullptr;

RadicalMaterials* RadicalMaterials::Instance()
{
  if (!fInstance) fInstance = new RadicalMaterials();
  return fInstance;
}

//----------------------------------------------------------------------------
// Helper: convert wavelength (nm) -> photon energy (eV value, with unit).
//----------------------------------------------------------------------------
namespace {
  inline G4double nmToEnergy(G4double lambda_nm)
  {
    // E[eV] = hc/lambda = 1239.841984 eV*nm / lambda[nm]
    return (1239.841984 / lambda_nm) * eV;
  }
}

void RadicalMaterials::Build()
{
  BuildBaseMaterials();
  BuildLYSO();
  BuildQuartz();
  BuildDSB1();
  BuildTyvekSurfaceMaterial();
  BuildPlasticScint();
  BuildLeadGlass();
}

//----------------------------------------------------------------------------
void RadicalMaterials::BuildBaseMaterials()
{
  auto nist = G4NistManager::Instance();

  fAir     = nist->FindOrBuildMaterial("G4_AIR");
  fW       = nist->FindOrBuildMaterial("G4_W");
  fSilicon = nist->FindOrBuildMaterial("G4_Si");
  fPOM     = nist->FindOrBuildMaterial("G4_POLYOXYMETHYLENE"); // POM (Delrin)
  // Tyvek is high-density polyethylene fabric.
  fTyvek   = nist->FindOrBuildMaterial("G4_POLYETHYLENE");

  // Galactic vacuum for the world.
  fVacuum = nist->FindOrBuildMaterial("G4_Galactic");

  // Air gets a refractive index so optical photons can travel in it.
  std::vector<G4double> e   = { nmToEnergy(700.), nmToEnergy(300.) };
  std::vector<G4double> ria = { 1.000293, 1.000293 };
  auto mptAir = new G4MaterialPropertiesTable();
  mptAir->AddProperty("RINDEX", e, ria);
  fAir->SetMaterialPropertiesTable(mptAir);
}

//----------------------------------------------------------------------------
// LYSO:Ce  (Lu(1.8)Y(0.2)SiO5:Ce), density 7.1 g/cm3.
//----------------------------------------------------------------------------
void RadicalMaterials::BuildLYSO()
{
  auto nist = G4NistManager::Instance();
  G4Element* Lu = nist->FindOrBuildElement("Lu");
  G4Element* Y  = nist->FindOrBuildElement("Y");
  G4Element* Si = nist->FindOrBuildElement("Si");
  G4Element* O  = nist->FindOrBuildElement("O");

  // Lu(1.8) Y(0.2) Si O(5)  -> use atom counts (Ce dopant trace, ignored in mix)
  fLYSO = new G4Material("LYSO", 7.10 * g/cm3, 4);
  // Number of atoms per formula unit (scaled x10 to keep integers).
  fLYSO->AddElement(Lu, 18);
  fLYSO->AddElement(Y,   2);
  fLYSO->AddElement(Si, 10);
  fLYSO->AddElement(O,  50);

  // ----- optical properties -----
  // Photon-energy grid spanning the LYSO emission band (peak ~420 nm).
  std::vector<G4double> ePhot = {
      nmToEnergy(650.), nmToEnergy(550.), nmToEnergy(500.),
      nmToEnergy(460.), nmToEnergy(430.), nmToEnergy(420.),
      nmToEnergy(410.), nmToEnergy(390.), nmToEnergy(360.) };

  // ESTIMATE: LYSO refractive index 1.82 (flat). Basis: Saint-Gobain/Crystal
  // Photonics LYSO datasheets quote n ~ 1.81-1.82 near 420 nm. Dispersion neglected.
  std::vector<G4double> rindex(ePhot.size(), 1.82);
  // ESTIMATE: bulk optical attenuation length 1.0 m (flat). Basis: order-of-
  // magnitude for good-quality LYSO; not measured for these plates.
  std::vector<G4double> absl  (ePhot.size(), 1.0 * m);

  // ESTIMATE: emission spectrum shape (relative). Basis: typical LYSO:Ce
  // emission peaked at 420 nm (PAPER value), Gaussian-ish wing shape assumed.
  std::vector<G4double> emit = {
      0.00, 0.02, 0.10, 0.55, 0.95, 1.00, 0.85, 0.30, 0.02 };

  auto mpt = new G4MaterialPropertiesTable();
  mpt->AddProperty("RINDEX",    ePhot, rindex);
  mpt->AddProperty("ABSLENGTH", ePhot, absl);
  mpt->AddProperty("SCINTILLATIONCOMPONENT1", ePhot, emit);

  // ESTIMATE: light yield 30000 photons/MeV. Basis: LYSO:Ce literature range
  // ~25000-33000 /MeV; the paper does not state it. Rescale at runtime with
  // /radical/det/lysoYieldScale.
  mpt->AddConstProperty("SCINTILLATIONYIELD", 30000. / MeV);
  mpt->AddConstProperty("RESOLUTIONSCALE", 1.0);              // ESTIMATE: Poisson (=1.0)
  // ESTIMATE: scintillation decay 40 ns (single component). Basis: LYSO:Ce
  // literature ~36-44 ns; paper does not state it.
  mpt->AddConstProperty("SCINTILLATIONTIMECONSTANT1", 40. * ns);
  mpt->AddConstProperty("SCINTILLATIONYIELD1", 1.0);
  fLYSO->SetMaterialPropertiesTable(mpt);

  // ESTIMATE: Birks constant 0.0076 mm/MeV. Basis: common value for dense
  // inorganic scintillators; not measured here.
  fLYSO->GetIonisation()->SetBirksConstant(0.0076 * mm / MeV);
}

//----------------------------------------------------------------------------
// Fused silica / quartz (SiO2) - capillary wall and quartz-rod fill.
//----------------------------------------------------------------------------
void RadicalMaterials::BuildQuartz()
{
  auto nist = G4NistManager::Instance();
  fQuartz = nist->FindOrBuildMaterial("G4_SILICON_DIOXIDE");

  std::vector<G4double> e = {
      nmToEnergy(650.), nmToEnergy(500.), nmToEnergy(420.), nmToEnergy(350.) };
  // Fused-silica refractive index from the well-known Sellmeier dispersion
  // (literature, effectively exact - not a free estimate).
  std::vector<G4double> rindex = { 1.456, 1.462, 1.468, 1.476 };
  // ESTIMATE: attenuation length 10 m (flat). Basis: high-purity fused silica
  // is effectively transparent over the 18 cm capillary; exact value irrelevant.
  std::vector<G4double> absl   (e.size(), 10.0 * m);

  auto mpt = new G4MaterialPropertiesTable();
  mpt->AddProperty("RINDEX",    e, rindex);
  mpt->AddProperty("ABSLENGTH", e, absl);
  fQuartz->SetMaterialPropertiesTable(mpt);
}

//----------------------------------------------------------------------------
// DSB1 organic plastic wavelength shifter.
//   Absorbs strongly at 425 nm, emission peak 495 nm, decay tau = 3.5 ns.
//   (paper, Section 2). Polystyrene-like base.
//----------------------------------------------------------------------------
void RadicalMaterials::BuildDSB1()
{
  auto nist = G4NistManager::Instance();
  // ESTIMATE: polystyrene base + density. Basis: DSB1 is an organic plastic WLS
  // (Eljen/Notre Dame); exact host polymer/density not given in the paper.
  G4Material* base = nist->FindOrBuildMaterial("G4_POLYSTYRENE");
  fDSB1 = new G4Material("DSB1", base->GetDensity(), 1);
  fDSB1->AddMaterial(base, 1.0);

  // Photon energy grid spanning absorption (425 nm) and emission (495 nm).
  std::vector<G4double> e = {
      nmToEnergy(600.), nmToEnergy(550.), nmToEnergy(510.),
      nmToEnergy(495.), nmToEnergy(470.), nmToEnergy(450.),
      nmToEnergy(425.), nmToEnergy(400.), nmToEnergy(360.) };

  // ESTIMATE: refractive index 1.59 (flat). Basis: typical polystyrene-based
  // plastic scintillator/WLS.
  std::vector<G4double> rindex(e.size(), 1.59);

  // ESTIMATE: bulk (non-WLS) absorption length 2 m. Basis: assume re-emitted
  // light is weakly self-absorbed over the short filament.
  std::vector<G4double> absl(e.size(), 2.0 * m);

  // ESTIMATE: WLS absorption-length spectrum. Basis: PAPER says DSB1 "absorbs
  // strongly at 425 nm". Only the peak is known; the full curve below (short
  // near 425 nm, long for the 495 nm re-emission band) is assumed.
  //                 600   550   510   495    470    450    425     400     360
  std::vector<G4double> wlsAbs = {
      10.*m, 5.*m, 1.*m, 50.*cm, 5.*cm, 1.*cm, 0.2*mm, 0.5*mm, 1.*mm };

  // ESTIMATE: WLS emission spectrum shape. Basis: PAPER gives emission peak
  // 495 nm; band shape below is assumed.
  std::vector<G4double> wlsEmit = {
      0.02, 0.20, 0.85, 1.00, 0.70, 0.30, 0.05, 0.00, 0.00 };

  auto mpt = new G4MaterialPropertiesTable();
  mpt->AddProperty("RINDEX",        e, rindex);
  mpt->AddProperty("ABSLENGTH",     e, absl);
  mpt->AddProperty("WLSABSLENGTH",  e, wlsAbs);
  mpt->AddProperty("WLSCOMPONENT",  e, wlsEmit);
  mpt->AddConstProperty("WLSTIMECONSTANT", 3.5 * ns);     // PAPER: fast tau = 3.5 ns
  // ESTIMATE: WLS quantum yield = 1.0 photon out per photon absorbed. Basis:
  // optimistic upper bound; real yield is < 1. Strongly affects detected light.
  mpt->AddConstProperty("WLSMEANNUMBERPHOTONS", 1.0);

  fDSB1->SetMaterialPropertiesTable(mpt);
}

//----------------------------------------------------------------------------
// Tyvek: bulk material is polyethylene (set above). The reflective behaviour
// is implemented as an optical SURFACE in DetectorConstruction. Here we just
// make sure it has no transparent RINDEX so photons interact at its surface.
//----------------------------------------------------------------------------
void RadicalMaterials::BuildTyvekSurfaceMaterial()
{
  // Intentionally no MPT on Tyvek bulk: the G4LogicalSkinSurface handles
  // the (diffuse, ~95%) reflection. Nothing to do here, kept for clarity.
}

//----------------------------------------------------------------------------
// Plastic scintillator trigger counters (A1, A2). Simple, no scintillation
// optics needed (used only for triggering/material budget); give RINDEX so
// optical photons (if any) behave.
//----------------------------------------------------------------------------
void RadicalMaterials::BuildPlasticScint()
{
  auto nist = G4NistManager::Instance();
  fPlastic = nist->FindOrBuildMaterial("G4_PLASTIC_SC_VINYLTOLUENE");
}

//----------------------------------------------------------------------------
// Lead glass for the backing calorimeter (Cherenkov radiator). Use NIST
// G4_GLASS_LEAD with a refractive index so Cherenkov light could be produced.
//----------------------------------------------------------------------------
void RadicalMaterials::BuildLeadGlass()
{
  auto nist = G4NistManager::Instance();
  fLeadGlass = nist->FindOrBuildMaterial("G4_GLASS_LEAD");

  std::vector<G4double> e = { nmToEnergy(650.), nmToEnergy(400.) };
  // ESTIMATE: lead-glass refractive index 1.62-1.67. Basis: typical SF-type
  // lead glass. Only used if Cherenkov readout of the backing calo is studied;
  // the leakage cut uses deposited energy, which is insensitive to this.
  std::vector<G4double> rindex = { 1.62, 1.67 };
  // ESTIMATE: lead-glass attenuation length 50 cm. Basis: assumed; the leakage
  // cut uses deposited energy, which is insensitive to this.
  std::vector<G4double> absl(e.size(), 50.0 * cm);
  auto mpt = new G4MaterialPropertiesTable();
  mpt->AddProperty("RINDEX",    e, rindex);
  mpt->AddProperty("ABSLENGTH", e, absl);
  fLeadGlass->SetMaterialPropertiesTable(mpt);
}
