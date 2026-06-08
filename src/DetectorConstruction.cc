#include "DetectorConstruction.hh"
#include "DetectorMessenger.hh"
#include "RadicalConstants.hh"
#include "RadicalMaterials.hh"
#include "CaloSD.hh"

#include "G4RunManager.hh"
#include "G4SDManager.hh"
#include "G4NistManager.hh"
#include "G4Material.hh"
#include "G4MaterialPropertiesTable.hh"

#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4SubtractionSolid.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4ThreeVector.hh"

#include "G4OpticalSurface.hh"
#include "G4LogicalSkinSurface.hh"

#include "G4VisAttributes.hh"
#include "G4Colour.hh"
#include "G4SystemOfUnits.hh"
#include "G4PhysicalConstants.hh"
#include "G4ios.hh"

#include <string>
#include <vector>

using namespace radical;

namespace {
  // The four corner capillary positions (x,y), Fig. 2.
  const G4ThreeVector kCorner[4] = {
      G4ThreeVector( kHoleOffset,  kHoleOffset, 0),
      G4ThreeVector( kHoleOffset, -kHoleOffset, 0),
      G4ThreeVector(-kHoleOffset,  kHoleOffset, 0),
      G4ThreeVector(-kHoleOffset, -kHoleOffset, 0) };

  inline G4double nmToE(G4double nm) { return (1239.841984 / nm) * eV; }
}

DetectorConstruction::DetectorConstruction()
{
  fMessenger = new DetectorMessenger(this);
}

DetectorConstruction::~DetectorConstruction()
{
  delete fMessenger;
}

void DetectorConstruction::DefineMaterials()
{
  RadicalMaterials::Instance()->Build();
}

//----------------------------------------------------------------------------
// A 14x14 plate of given thickness with the 5 capillary through-holes removed.
//----------------------------------------------------------------------------
G4VSolid* DetectorConstruction::MakeHoledPlate(const G4String& name,
                                               G4double thickness) const
{
  auto plate = new G4Box(name + "_box", kTileXY/2, kTileXY/2, thickness/2);

  const G4double over = thickness;          // make holes longer than the plate
  auto cornerHole = new G4Tubs(name + "_ch", 0., kCornerHoleDia/2,
                               (thickness + over)/2, 0., CLHEP::twopi);
  auto centralHole = new G4Tubs(name + "_cc", 0., kCentralHoleDia/2,
                                (thickness + over)/2, 0., CLHEP::twopi);

  G4VSolid* solid = plate;
  for (int i = 0; i < 4; ++i) {
    solid = new G4SubtractionSolid(
        name + "_s" + std::to_string(i), solid, cornerHole, nullptr,
        G4ThreeVector(kCorner[i].x(), kCorner[i].y(), 0));
  }
  solid = new G4SubtractionSolid(name, solid, centralHole, nullptr,
                                 G4ThreeVector(0, 0, 0));
  return solid;
}

//----------------------------------------------------------------------------
G4VPhysicalVolume* DetectorConstruction::Construct()
{
  DefineMaterials();
  auto mat = RadicalMaterials::Instance();

  // Apply runtime LYSO light-yield scaling once (geometry is built on master).
  if (fLysoYieldScale != 1.0) {
    auto mpt = mat->LYSO()->GetMaterialPropertiesTable();
    if (mpt && mpt->ConstPropertyExists("SCINTILLATIONYIELD")) {
      G4double y0 = mpt->GetConstProperty("SCINTILLATIONYIELD");
      mpt->AddConstProperty("SCINTILLATIONYIELD", y0 * fLysoYieldScale, true);
      G4cout << "[RADiCAL] LYSO scintillation yield scaled by "
             << fLysoYieldScale << " -> " << y0*fLysoYieldScale*MeV
             << " /MeV" << G4endl;
    }
  }

  // ----- World -----
  auto worldS = new G4Box("World", kWorldXY/2, kWorldXY/2, kWorldZ/2);
  fWorldLV = new G4LogicalVolume(worldS, mat->Air(), "World");
  fWorldLV->SetVisAttributes(G4VisAttributes::GetInvisible());
  auto worldPV = new G4PVPlacement(nullptr, {}, fWorldLV, "World",
                                   nullptr, false, 0, true);

  BuildModule(fWorldLV);
  BuildCapillaries(fWorldLV);
  if (fBuildBeamline) BuildBeamline(fWorldLV);

  ApplyOpticalSurfaces();

  return worldPV;
}

//----------------------------------------------------------------------------
void DetectorConstruction::BuildModule(G4LogicalVolume* world)
{
  auto mat = RadicalMaterials::Instance();

  // Single LYSO / W / Tyvek logical volumes, placed many times.
  fLysoLV = new G4LogicalVolume(MakeHoledPlate("LYSO", kLysoThick),
                                mat->LYSO(), "LYSO");
  auto wLV = new G4LogicalVolume(MakeHoledPlate("Tungsten", kWThick),
                                 mat->Tungsten(), "Tungsten");
  auto tyvekLV = new G4LogicalVolume(MakeHoledPlate("Tyvek", kTyvekThick),
                                     mat->Tyvek(), "Tyvek");
  fTyvekLVs.push_back(tyvekLV);

  auto lysoVis = new G4VisAttributes(G4Colour(0.2, 0.8, 1.0, 0.6));
  lysoVis->SetForceSolid(true);
  fLysoLV->SetVisAttributes(lysoVis);
  auto wVis = new G4VisAttributes(G4Colour(0.4, 0.4, 0.4, 0.7));
  wVis->SetForceSolid(true);
  wLV->SetVisAttributes(wVis);
  auto tyvekVis = new G4VisAttributes(G4Colour(1.0, 1.0, 1.0, 0.3));
  tyvekLV->SetVisAttributes(tyvekVis);

  // Stack: LYSO (T W T) repeated, ending on LYSO. Tyvek between every interface.
  G4double z = -kStackLength/2;
  G4int lysoCopy = 0, wCopy = 0;
  fLysoZ.clear();

  for (G4int i = 0; i < kNLyso; ++i) {
    G4double zc = z + kLysoThick/2;
    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, zc), fLysoLV, "LYSO",
                      world, false, lysoCopy++, true);
    fLysoZ.push_back(zc);
    z += kLysoThick;

    if (i < kNTungsten) {
      // Tyvek - W - Tyvek
      G4double zt1 = z + kTyvekThick/2;
      new G4PVPlacement(nullptr, G4ThreeVector(0, 0, zt1), tyvekLV, "Tyvek",
                        world, false, 2*i, true);
      z += kTyvekThick;

      G4double zw = z + kWThick/2;
      new G4PVPlacement(nullptr, G4ThreeVector(0, 0, zw), wLV, "Tungsten",
                        world, false, wCopy++, true);
      z += kWThick;

      G4double zt2 = z + kTyvekThick/2;
      new G4PVPlacement(nullptr, G4ThreeVector(0, 0, zt2), tyvekLV, "Tyvek",
                        world, false, 2*i + 1, true);
      z += kTyvekThick;
    }
  }

  // ----- Outer reflective Tyvek wrap around the 4 lateral faces -----
  // The paper assembles the tiles with reflective Tyvek; here we wrap the
  // module's outer 14x14 perimeter in a 0.1 mm Tyvek shell (open at both z ends
  // where the capillaries exit) carrying the same diffuse ~95% reflective
  // surface as the inter-layer sheets. This reduces lateral light loss.
  {
    G4double outer = kTileXY + 2*kTyvekThick;
    auto wrapOuterS = new G4Box("WrapOuter", outer/2, outer/2, kStackLength/2);
    auto wrapInnerS = new G4Box("WrapInner", kTileXY/2, kTileXY/2,
                                kStackLength/2 + 1.*mm); // longer => open ends
    auto wrapS = new G4SubtractionSolid("TyvekWrap", wrapOuterS, wrapInnerS);
    auto wrapLV = new G4LogicalVolume(wrapS, mat->Tyvek(), "TyvekWrap");
    wrapLV->SetVisAttributes(tyvekVis);
    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, 0), wrapLV, "TyvekWrap",
                      world, false, 0, true);
    fTyvekLVs.push_back(wrapLV); // gets the reflective skin in ApplyOpticalSurfaces
  }

  // Default WLS filament centre = centre of LYSO layer kWlsCenterLayer.
  if (!fWlsCenterZSet)
    fWlsCenterZ = fLysoZ.at(kWlsCenterLayer);

  G4cout << "[RADiCAL] Active stack length = " << kStackLength/mm << " mm, "
         << kNLyso << " LYSO + " << kNTungsten << " W + " << kNTyvek
         << " Tyvek (" << kTyvekThick/mm << " mm each)." << G4endl;
  G4cout << "[RADiCAL] WLS filament centre z = " << fWlsCenterZ/mm
         << " mm (LYSO layer " << kWlsCenterLayer << ")." << G4endl;
}

//----------------------------------------------------------------------------
void DetectorConstruction::BuildCapillaries(G4LogicalVolume* world)
{
  auto mat = RadicalMaterials::Instance();

  // Quartz capillary cylinder (wall + fused quartz rod treated as one quartz
  // body) running the full 183 mm, with a DSB1 WLS filament embedded at shower
  // max. The central phi-0.9 hole is left empty (air), as in the beam test.
  auto capS = new G4Tubs("Capillary", 0., kCapOuterDia/2, kCapLength/2,
                         0., CLHEP::twopi);
  auto capLV = new G4LogicalVolume(capS, mat->Quartz(), "Capillary");
  auto capVis = new G4VisAttributes(G4Colour(0.8, 0.9, 1.0, 0.4));
  capLV->SetVisAttributes(capVis);

  // DSB1 WLS filament inside the capillary core, centred at shower max.
  auto wlsS = new G4Tubs("WLSfilament", 0., kWlsFilamentDia/2,
                         kWlsFilamentLen/2, 0., CLHEP::twopi);
  auto wlsLV = new G4LogicalVolume(wlsS, mat->DSB1(), "WLSfilament");
  auto wlsVis = new G4VisAttributes(G4Colour(1.0, 0.6, 0.0, 0.9));
  wlsVis->SetForceSolid(true);
  wlsLV->SetVisAttributes(wlsVis);
  // Capillary is centred at world z=0, so local z == world z.
  new G4PVPlacement(nullptr, G4ThreeVector(0, 0, fWlsCenterZ), wlsLV,
                    "WLSfilament", capLV, false, 0, true);

  // SiPM tile, placed at both capillary ends.
  auto sipmS = new G4Box("SiPM", kSiPMSize/2, kSiPMSize/2, kSiPMThick/2);
  fSiPMLVs.clear();
  auto sipmLV = new G4LogicalVolume(sipmS, mat->Silicon(), "SiPM");
  auto sipmVis = new G4VisAttributes(G4Colour(0.1, 0.6, 0.1, 1.0));
  sipmVis->SetForceSolid(true);
  sipmLV->SetVisAttributes(sipmVis);
  fSiPMLVs.push_back(sipmLV);

  const G4double zEnd = kCapLength/2;                 // capillary end |z|
  const G4double zSiPM = zEnd + kSiPMThick/2;         // SiPM centre |z|

  for (int c = 0; c < 4; ++c) {
    G4ThreeVector p(kCorner[c].x(), kCorner[c].y(), 0.);
    new G4PVPlacement(nullptr, p, capLV, "Capillary", world, false, c, true);

    // channel id = corner*2 + end (end 0 = upstream/-z, 1 = downstream/+z)
    new G4PVPlacement(nullptr, G4ThreeVector(p.x(), p.y(), -zSiPM), sipmLV,
                      "SiPM", world, false, c*2 + 0, true);
    new G4PVPlacement(nullptr, G4ThreeVector(p.x(), p.y(), +zSiPM), sipmLV,
                      "SiPM", world, false, c*2 + 1, true);
  }
}

//----------------------------------------------------------------------------
// Beam-test elements (Fig. 11c). All in air. Order along +z:
//   A1 trigger, A2 trigger, MCP, MODULE (origin), Pb-glass backing.
//----------------------------------------------------------------------------
void DetectorConstruction::BuildBeamline(G4LogicalVolume* world)
{
  auto mat = RadicalMaterials::Instance();

  if (fBuildTriggers) {
    auto a1S = new G4Box("A1", kA1XY/2, kA1XY/2, kTrigThick/2);
    auto a1LV = new G4LogicalVolume(a1S, mat->PlasticScint(), "A1");
    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, kA1Z), a1LV, "A1",
                      world, false, 0, true);

    auto a2S = new G4Box("A2", kA2XY/2, kA2XY/2, kTrigThick/2);
    auto a2LV = new G4LogicalVolume(a2S, mat->PlasticScint(), "A2");
    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, kA2Z), a2LV, "A2",
                      world, false, 0, true);

    auto trigVis = new G4VisAttributes(G4Colour(0.9, 0.9, 0.2, 0.4));
    a1LV->SetVisAttributes(trigVis);
    a2LV->SetVisAttributes(trigVis);
  }

  if (fBuildMCP) {
    // Simplified MCP tube: a glass body with a quartz entrance window.
    auto bodyS = new G4Tubs("MCPbody", 0., kMcpBodyDia/2, kMcpBodyLen/2,
                            0., CLHEP::twopi);
    auto bodyLV = new G4LogicalVolume(
        bodyS, G4NistManager::Instance()->FindOrBuildMaterial("G4_GLASS_PLATE"),
        "MCPbody");
    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, kMcpZ), bodyLV, "MCPbody",
                      world, false, 0, true);

    auto winS = new G4Tubs("MCPwin", 0., kMcpBodyDia/2, kMcpWinThick/2,
                           0., CLHEP::twopi);
    auto winLV = new G4LogicalVolume(winS, mat->Quartz(), "MCPwin");
    new G4PVPlacement(nullptr,
                      G4ThreeVector(0, 0, kMcpZ - kMcpBodyLen/2 - kMcpWinThick/2),
                      winLV, "MCPwin", world, false, 0, true);

    auto mcpVis = new G4VisAttributes(G4Colour(0.5, 0.5, 0.8, 0.3));
    bodyLV->SetVisAttributes(mcpVis);
    winLV->SetVisAttributes(mcpVis);
  }

  if (fBuildPbGlass) {
    // 2x2 array of 2x2x40 cm lead-glass blocks -> 4x4x40 cm backing calo.
    auto pbS = new G4Box("PbGlass", kPbBlockXY/2, kPbBlockXY/2, kPbBlockLen/2);
    fPbGlassLV = new G4LogicalVolume(pbS, mat->LeadGlass(), "PbGlass");
    auto pbVis = new G4VisAttributes(G4Colour(0.6, 0.3, 0.6, 0.4));
    fPbGlassLV->SetVisAttributes(pbVis);

    const G4double zc = kPbFrontZ + kPbBlockLen/2;
    const G4double off = kPbBlockXY/2;
    int copy = 0;
    for (int ix = -1; ix <= 1; ix += 2)
      for (int iy = -1; iy <= 1; iy += 2)
        new G4PVPlacement(nullptr,
                          G4ThreeVector(ix*off, iy*off, zc), fPbGlassLV,
                          "PbGlass", world, false, copy++, true);
  }
}

//----------------------------------------------------------------------------
void DetectorConstruction::ApplyOpticalSurfaces()
{
  // --- Tyvek: diffuse reflector ---
  std::vector<G4double> e = { nmToE(700.), nmToE(500.), nmToE(420.), nmToE(350.) };

  auto tyvekOS = new G4OpticalSurface("TyvekSurface");
  tyvekOS->SetType(dielectric_metal);
  tyvekOS->SetFinish(ground);          // ground + dielectric_metal => Lambertian
  tyvekOS->SetModel(unified);
  {
    // ESTIMATE: Tyvek diffuse reflectivity ~0.90-0.95 (falling in the UV).
    // Basis: published Tyvek 1025-type reflectance is ~94-97% in the blue/green;
    // values here are typical, not measured for this build.
    std::vector<G4double> refl = { 0.95, 0.95, 0.94, 0.90 };
    std::vector<G4double> eff  = { 0.0, 0.0, 0.0, 0.0 };
    auto mpt = new G4MaterialPropertiesTable();
    mpt->AddProperty("REFLECTIVITY", e, refl);
    mpt->AddProperty("EFFICIENCY",   e, eff);
    tyvekOS->SetMaterialPropertiesTable(mpt);
  }
  for (auto* lv : fTyvekLVs)
    new G4LogicalSkinSurface("TyvekSkin", lv, tyvekOS);

  // --- SiPM: photon detector with PDE ---
  auto sipmOS = new G4OpticalSurface("SiPMSurface");
  sipmOS->SetType(dielectric_metal);
  sipmOS->SetFinish(polished);
  sipmOS->SetModel(unified);
  {
    // ESTIMATE: SiPM photon-detection efficiency, ~40% peak near 450-500 nm,
    // rolling off in red/UV. Basis: typical Hamamatsu SiPM PDE curves; the exact
    // HDR2 device curve is not used. EFFICIENCY here = per-photon detection prob.
    //                         700nm  500nm  420nm  350nm
    std::vector<G4double> refl = { 0.0, 0.0, 0.0, 0.0 };   // absorb all arriving
    std::vector<G4double> eff  = { 0.20, 0.40, 0.38, 0.20 };
    auto mpt = new G4MaterialPropertiesTable();
    mpt->AddProperty("REFLECTIVITY", e, refl);
    mpt->AddProperty("EFFICIENCY",   e, eff);
    sipmOS->SetMaterialPropertiesTable(mpt);
  }
  for (auto* lv : fSiPMLVs)
    new G4LogicalSkinSurface("SiPMSkin", lv, sipmOS);
}

//----------------------------------------------------------------------------
void DetectorConstruction::ConstructSDandField()
{
  auto sdman = G4SDManager::GetSDMpointer();

  // LYSO calorimeter (per-layer energy).
  auto caloSD = new CaloSD("LYSO_SD", "LYSO_HC", kNLyso);
  sdman->AddNewDetector(caloSD);
  SetSensitiveDetector(fLysoLV, caloSD);

  // SiPM photon detection is handled in SteppingAction via the optical
  // boundary "Detection" status, so no SD object is needed there.
}
