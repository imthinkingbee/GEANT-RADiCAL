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
#include "G4Polyhedra.hh"
#include "G4SubtractionSolid.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4ThreeVector.hh"
#include "G4TwoVector.hh"

#include "G4OpticalSurface.hh"
#include "G4LogicalSkinSurface.hh"

#include "G4VisAttributes.hh"
#include "G4Colour.hh"
#include "G4SystemOfUnits.hh"
#include "G4PhysicalConstants.hh"
#include "G4ios.hh"

#include <string>
#include <vector>
#include <cmath>

using namespace radical;

namespace {
  // Capillary protrusion beyond the active stack at EACH end (to the SiPMs).
  constexpr G4double kCapProtrusion = 32.0 * mm;   // single: 119.1 + 64 ~ 183 mm
}

DetectorConstruction::DetectorConstruction()
{
  fMessenger = new DetectorMessenger(this);
}

DetectorConstruction::~DetectorConstruction()
{
  delete fMessenger;
}

void DetectorConstruction::SetGeometry(const G4String& name)
{
  if      (name == "single")    fMode = GeoMode::Single;
  else if (name == "enhanced")  fMode = GeoMode::Enhanced;
  else if (name == "enhanced5") fMode = GeoMode::Enhanced5;
  else if (name == "array3x3")  fMode = GeoMode::Array3x3;
  else if (name == "hex")       fMode = GeoMode::Hex;
  else G4cout << "[RADiCAL] unknown geometry '" << name
              << "', keeping current." << G4endl;
}

void DetectorConstruction::DefineMaterials()
{
  RadicalMaterials::Instance()->Build();
}

//----------------------------------------------------------------------------
// Build the ModuleSpec (tile shape, plates, hole layout) for the active mode.
//----------------------------------------------------------------------------
DetectorConstruction::ModuleSpec DetectorConstruction::MakeSpec() const
{
  ModuleSpec s;
  s.lysoThick = kLysoThick; s.wThick = kWThick;
  s.nLyso = kNLyso; s.nW = kNTungsten;

  if (fMode == GeoMode::Single) {
    s.hexagonal = false; s.tileXY = kTileXY;
    // 4 corner capillaries + 1 empty central hole (Fig. 2).
    const G4double o = kHoleOffset;
    s.holes.push_back({{ o,  o}, kCornerHoleDia, true});
    s.holes.push_back({{ o, -o}, kCornerHoleDia, true});
    s.holes.push_back({{-o,  o}, kCornerHoleDia, true});
    s.holes.push_back({{-o, -o}, kCornerHoleDia, true});
    s.holes.push_back({{ 0,  0}, kCentralHoleDia, false}); // empty (air)
  }
  else if (fMode == GeoMode::Enhanced || fMode == GeoMode::Array3x3) {
    // Enhanced 18x18 module (Fig. 28): one module with a 3x3 grid of 9
    // capillaries (corners + edge-midpoints + centre), thicker LYSO. The
    // Array3x3 mode tiles 9 of these; Enhanced is a single such module.
    s.hexagonal = false; s.tileXY = kEnhTileXY; s.lysoThick = kEnhLysoThick;
    for (int iy = 1; iy >= -1; --iy)
      for (int ix = -1; ix <= 1; ++ix)
        s.holes.push_back({{ix*kEnhHoleStep, iy*kEnhHoleStep}, kEnhHoleDia, true});
  }
  else if (fMode == GeoMode::Enhanced5) {
    // CONTROL: identical to 'enhanced' (18x18, 3 mm LYSO) but with the original
    // 5-capillary layout (4 corners + centre) instead of 9. Isolates the effect
    // of capillary COUNT alone on the energy resolution.
    s.hexagonal = false; s.tileXY = kEnhTileXY; s.lysoThick = kEnhLysoThick;
    const G4double o = kEnhHoleStep;
    s.holes.push_back({{ o,  o}, kEnhHoleDia, true});
    s.holes.push_back({{ o, -o}, kEnhHoleDia, true});
    s.holes.push_back({{-o,  o}, kEnhHoleDia, true});
    s.holes.push_back({{-o, -o}, kEnhHoleDia, true});
    s.holes.push_back({{ 0,  0}, kEnhHoleDia, true});
  }
  else { // Hex
    s.hexagonal = true; s.tileXY = kHexFlatToFlat;
    s.holes.push_back({{0, 0}, kHexHoleDia, true});           // centre
    for (int k = 0; k < kHexNRing; ++k) {                     // 6 ring holes
      G4double a = k * (CLHEP::twopi / kHexNRing);
      s.holes.push_back({{kHexRingR*std::cos(a), kHexRingR*std::sin(a)},
                         kHexHoleDia, true});
    }
  }
  return s;
}

//----------------------------------------------------------------------------
// Plate (box or hexagonal prism) of the given thickness with all holes removed.
//----------------------------------------------------------------------------
G4VSolid* DetectorConstruction::MakeHoledPlate(const G4String& name,
                                               const ModuleSpec& s,
                                               G4double thickness) const
{
  G4VSolid* solid = nullptr;
  if (s.hexagonal) {
    const G4double apothem = s.tileXY / 2.;     // flat-to-flat / 2
    G4double zP[2] = {-thickness/2, thickness/2};
    G4double ri[2] = {0., 0.};
    G4double ro[2] = {apothem, apothem};        // tangent distance to flats
    solid = new G4Polyhedra(name + "_hex", 0., CLHEP::twopi, 6, 2, zP, ri, ro);
  } else {
    solid = new G4Box(name + "_box", s.tileXY/2, s.tileXY/2, thickness/2);
  }

  int i = 0;
  for (const auto& h : s.holes) {
    auto hole = new G4Tubs(name + "_h" + std::to_string(i++), 0., h.dia/2,
                           thickness, 0., CLHEP::twopi);  // longer => clean cut
    solid = new G4SubtractionSolid(
        name + "_s" + std::to_string(i), solid, hole, nullptr,
        G4ThreeVector(h.pos.x(), h.pos.y(), 0.));
  }
  return solid;
}

//----------------------------------------------------------------------------
// A lateral reflective shell (box or hex), open at both z ends.
//   innerHalf = half-width / apothem of the inner (tile) surface.
//----------------------------------------------------------------------------
G4VSolid* DetectorConstruction::MakeShell(const G4String& name,
                                          const ModuleSpec& s,
                                          G4double innerHalf, G4double thick,
                                          G4double length) const
{
  if (s.hexagonal) {
    G4double zP[2] = {-length/2, length/2};
    G4double riO[2] = {0., 0.}, roO[2] = {innerHalf+thick, innerHalf+thick};
    auto outer = new G4Polyhedra(name+"_o", 0., CLHEP::twopi, 6, 2, zP, riO, roO);
    G4double zI[2] = {-length/2-1.*mm, length/2+1.*mm};
    G4double riI[2] = {0., 0.}, roI[2] = {innerHalf, innerHalf};
    auto inner = new G4Polyhedra(name+"_i", 0., CLHEP::twopi, 6, 2, zI, riI, roI);
    return new G4SubtractionSolid(name, outer, inner);
  }
  auto outer = new G4Box(name+"_o", innerHalf+thick, innerHalf+thick, length/2);
  auto inner = new G4Box(name+"_i", innerHalf, innerHalf, length/2+1.*mm);
  return new G4SubtractionSolid(name, outer, inner);
}

//----------------------------------------------------------------------------
// Build one module envelope LV with all internals. Returns the envelope LV.
//----------------------------------------------------------------------------
G4LogicalVolume* DetectorConstruction::BuildModuleEnvelope(const ModuleSpec& s)
{
  auto mat = RadicalMaterials::Instance();

  const G4double stackLen = s.stackLength();
  const G4double capLen   = stackLen + 2*kCapProtrusion;
  const G4double envHalfL = capLen/2 + kSiPMThick + 1.*mm;
  const G4double innerHalf = s.tileXY/2;                 // tile half-width / apothem
  const G4double envHalfXY = innerHalf + kTyvekThick + 0.1*mm;
  fEnvLength = 2*envHalfL;

  // --- envelope (air) ---
  G4VSolid* envS = nullptr;
  if (s.hexagonal) {
    G4double zP[2] = {-envHalfL, envHalfL};
    G4double ri[2] = {0., 0.}, ro[2] = {envHalfXY, envHalfXY};
    envS = new G4Polyhedra("Env", 0., CLHEP::twopi, 6, 2, zP, ri, ro);
  } else {
    envS = new G4Box("Env", envHalfXY, envHalfXY, envHalfL);
  }
  auto envLV = new G4LogicalVolume(envS, mat->Air(), "Env");
  envLV->SetVisAttributes(G4VisAttributes::GetInvisible());

  // --- plate logical volumes (one each, placed many times) ---
  fLysoLV = new G4LogicalVolume(MakeHoledPlate("LYSO", s, s.lysoThick),
                                mat->LYSO(), "LYSO");
  auto wLV = new G4LogicalVolume(MakeHoledPlate("Tungsten", s, s.wThick),
                                 mat->Tungsten(), "Tungsten");
  auto tyvekLV = new G4LogicalVolume(MakeHoledPlate("Tyvek", s, kTyvekThick),
                                     mat->Tyvek(), "Tyvek");
  fTyvekLVs.clear();
  fTyvekLVs.push_back(tyvekLV);

  auto lysoVis = new G4VisAttributes(G4Colour(0.2, 0.8, 1.0, 0.6));
  lysoVis->SetForceSolid(true); fLysoLV->SetVisAttributes(lysoVis);
  auto wVis = new G4VisAttributes(G4Colour(0.4, 0.4, 0.4, 0.7));
  wVis->SetForceSolid(true); wLV->SetVisAttributes(wVis);
  auto tyvekVis = new G4VisAttributes(G4Colour(1, 1, 1, 0.3));
  tyvekLV->SetVisAttributes(tyvekVis);

  // --- stack: LYSO (T W T) ... LYSO, Tyvek between every interface ---
  G4double z = -stackLen/2;
  fLysoZ.clear();
  for (G4int i = 0; i < s.nLyso; ++i) {
    G4double zc = z + s.lysoThick/2;
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,zc), fLysoLV, "LYSO",
                      envLV, false, i, true);
    fLysoZ.push_back(zc);
    z += s.lysoThick;
    if (i < s.nW) {
      new G4PVPlacement(nullptr, G4ThreeVector(0,0,z+kTyvekThick/2), tyvekLV,
                        "Tyvek", envLV, false, 2*i, true);
      z += kTyvekThick;
      new G4PVPlacement(nullptr, G4ThreeVector(0,0,z+s.wThick/2), wLV,
                        "Tungsten", envLV, false, i, true);
      z += s.wThick;
      new G4PVPlacement(nullptr, G4ThreeVector(0,0,z+kTyvekThick/2), tyvekLV,
                        "Tyvek", envLV, false, 2*i+1, true);
      z += kTyvekThick;
    }
  }
  if (!fWlsCenterZSet) fWlsCenterZ = fLysoZ.at(kWlsCenterLayer);

  // --- outer Tyvek lateral wrap (open z-ends) ---
  auto wrapLV = new G4LogicalVolume(
      MakeShell("TyvekWrap", s, innerHalf, kTyvekThick, stackLen),
      mat->Tyvek(), "TyvekWrap");
  wrapLV->SetVisAttributes(tyvekVis);
  new G4PVPlacement(nullptr, {}, wrapLV, "TyvekWrap", envLV, false, 0, true);
  fTyvekLVs.push_back(wrapLV);

  // --- capillaries + WLS + SiPMs (instrumented holes only) ---
  auto capS = new G4Tubs("Capillary", 0., kCapOuterDia/2, capLen/2, 0., CLHEP::twopi);
  auto capLV = new G4LogicalVolume(capS, mat->Quartz(), "Capillary");
  capLV->SetVisAttributes(new G4VisAttributes(G4Colour(0.8,0.9,1.0,0.4)));

  auto wlsS = new G4Tubs("WLSfilament", 0., kWlsFilamentDia/2,
                         kWlsFilamentLen/2, 0., CLHEP::twopi);
  auto wlsLV = new G4LogicalVolume(wlsS, mat->DSB1(), "WLSfilament");
  auto wlsVis = new G4VisAttributes(G4Colour(1.0,0.6,0.0,0.9));
  wlsVis->SetForceSolid(true); wlsLV->SetVisAttributes(wlsVis);
  new G4PVPlacement(nullptr, G4ThreeVector(0,0,fWlsCenterZ), wlsLV,
                    "WLSfilament", capLV, false, 0, true);

  auto sipmS = new G4Box("SiPM", kSiPMSize/2, kSiPMSize/2, kSiPMThick/2);
  fSiPMLV = new G4LogicalVolume(sipmS, mat->Silicon(), "SiPM");
  auto sipmVis = new G4VisAttributes(G4Colour(0.1,0.6,0.1,1.0));
  sipmVis->SetForceSolid(true); fSiPMLV->SetVisAttributes(sipmVis);

  const G4double zSiPM = capLen/2 + kSiPMThick/2;
  G4int cap = 0;
  for (const auto& h : s.holes) {
    if (!h.instrumented) continue;
    G4ThreeVector p(h.pos.x(), h.pos.y(), 0.);
    new G4PVPlacement(nullptr, p, capLV, "Capillary", envLV, false, cap, true);
    // channel = cap*2 + end (end 0 = upstream/-z, 1 = downstream/+z)
    new G4PVPlacement(nullptr, G4ThreeVector(p.x(),p.y(),-zSiPM), fSiPMLV,
                      "SiPM", envLV, false, cap*2+0, true);
    new G4PVPlacement(nullptr, G4ThreeVector(p.x(),p.y(),+zSiPM), fSiPMLV,
                      "SiPM", envLV, false, cap*2+1, true);
    ++cap;
  }

  G4cout << "[RADiCAL] module: " << (s.hexagonal ? "hex" : "box")
         << " tile=" << s.tileXY/mm << " mm, LYSO=" << s.lysoThick/mm
         << " mm, stack=" << stackLen/mm << " mm, caps=" << cap
         << ", WLS z=" << fWlsCenterZ/mm << " mm." << G4endl;
  return envLV;
}

//----------------------------------------------------------------------------
G4VPhysicalVolume* DetectorConstruction::Construct()
{
  DefineMaterials();
  auto mat = RadicalMaterials::Instance();

  if (fLysoYieldScale != 1.0) {
    auto mpt = mat->LYSO()->GetMaterialPropertiesTable();
    if (mpt && mpt->ConstPropertyExists("SCINTILLATIONYIELD")) {
      G4double y0 = mpt->GetConstProperty("SCINTILLATIONYIELD");
      mpt->AddConstProperty("SCINTILLATIONYIELD", y0*fLysoYieldScale, true);
      G4cout << "[RADiCAL] LYSO yield scaled by " << fLysoYieldScale << G4endl;
    }
  }

  auto worldS = new G4Box("World", kWorldXY/2, kWorldXY/2, kWorldZ/2);
  fWorldLV = new G4LogicalVolume(worldS, mat->Air(), "World");
  fWorldLV->SetVisAttributes(G4VisAttributes::GetInvisible());
  auto worldPV = new G4PVPlacement(nullptr, {}, fWorldLV, "World",
                                   nullptr, false, 0, true);

  ModuleSpec spec = MakeSpec();
  auto envLV = BuildModuleEnvelope(spec);

  // Place the module envelope(s); copy number == module index.
  if (fMode == GeoMode::Array3x3) {
    const G4double pitch = spec.tileXY + 2*kTyvekThick + kArrayGap;
    fNModules = kArraySide * kArraySide;
    G4int m = 0;
    for (int iy = 1; iy >= -1; --iy)
      for (int ix = -1; ix <= 1; ++ix)
        new G4PVPlacement(nullptr, G4ThreeVector(ix*pitch, iy*pitch, 0.),
                          envLV, "Env", fWorldLV, false, m++, true);
    G4cout << "[RADiCAL] 3x3 array, pitch " << pitch/mm << " mm, "
           << fNModules << " modules." << G4endl;
  } else {
    fNModules = 1;
    new G4PVPlacement(nullptr, {}, envLV, "Env", fWorldLV, false, 0, true);
  }

  if (fBuildBeamline) BuildBeamline(fWorldLV);
  ApplyOpticalSurfaces();
  return worldPV;
}

//----------------------------------------------------------------------------
// Beam-test elements. MCP / Pb-glass are positioned just outside the module
// envelope so they never overlap it for any geometry mode.
//----------------------------------------------------------------------------
void DetectorConstruction::BuildBeamline(G4LogicalVolume* world)
{
  auto mat = RadicalMaterials::Instance();
  const G4double zFront = fEnvLength/2;   // |z| of the module envelope end

  if (fBuildTriggers) {
    auto a1LV = new G4LogicalVolume(
        new G4Box("A1", kA1XY/2, kA1XY/2, kTrigThick/2), mat->PlasticScint(), "A1");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,kA1Z), a1LV, "A1", world, false, 0, true);
    auto a2LV = new G4LogicalVolume(
        new G4Box("A2", kA2XY/2, kA2XY/2, kTrigThick/2), mat->PlasticScint(), "A2");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,kA2Z), a2LV, "A2", world, false, 0, true);
    auto v = new G4VisAttributes(G4Colour(0.9,0.9,0.2,0.4));
    a1LV->SetVisAttributes(v); a2LV->SetVisAttributes(v);
  }

  if (fBuildMCP) {
    const G4double mcpZ = -(zFront + 5.*mm + kMcpBodyLen/2);
    auto bodyLV = new G4LogicalVolume(
        new G4Tubs("MCPbody", 0., kMcpBodyDia/2, kMcpBodyLen/2, 0., CLHEP::twopi),
        G4NistManager::Instance()->FindOrBuildMaterial("G4_GLASS_PLATE"), "MCPbody");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,mcpZ), bodyLV, "MCPbody", world, false, 0, true);
    auto winLV = new G4LogicalVolume(
        new G4Tubs("MCPwin", 0., kMcpBodyDia/2, kMcpWinThick/2, 0., CLHEP::twopi),
        mat->Quartz(), "MCPwin");
    new G4PVPlacement(nullptr, G4ThreeVector(0,0,mcpZ - kMcpBodyLen/2 - kMcpWinThick/2),
                      winLV, "MCPwin", world, false, 0, true);
    auto v = new G4VisAttributes(G4Colour(0.5,0.5,0.8,0.3));
    bodyLV->SetVisAttributes(v); winLV->SetVisAttributes(v);
  }

  if (fBuildPbGlass) {
    fPbGlassLV = new G4LogicalVolume(
        new G4Box("PbGlass", kPbBlockXY/2, kPbBlockXY/2, kPbBlockLen/2),
        mat->LeadGlass(), "PbGlass");
    fPbGlassLV->SetVisAttributes(new G4VisAttributes(G4Colour(0.6,0.3,0.6,0.4)));
    const G4double zc = (zFront + 10.*mm) + kPbBlockLen/2;
    const G4double off = kPbBlockXY/2;
    int copy = 0;
    for (int ix=-1; ix<=1; ix+=2)
      for (int iy=-1; iy<=1; iy+=2)
        new G4PVPlacement(nullptr, G4ThreeVector(ix*off, iy*off, zc),
                          fPbGlassLV, "PbGlass", world, false, copy++, true);
  }
}

//----------------------------------------------------------------------------
void DetectorConstruction::ApplyOpticalSurfaces()
{
  std::vector<G4double> e = {
      (1239.841984/700.)*eV, (1239.841984/500.)*eV,
      (1239.841984/420.)*eV, (1239.841984/350.)*eV };

  auto tyvekOS = new G4OpticalSurface("TyvekSurface");
  tyvekOS->SetType(dielectric_metal); tyvekOS->SetFinish(ground);
  tyvekOS->SetModel(unified);
  { std::vector<G4double> r = {0.95,0.95,0.94,0.90}, eff = {0,0,0,0};
    auto m = new G4MaterialPropertiesTable();
    m->AddProperty("REFLECTIVITY", e, r); m->AddProperty("EFFICIENCY", e, eff);
    tyvekOS->SetMaterialPropertiesTable(m); }
  for (auto* lv : fTyvekLVs) new G4LogicalSkinSurface("TyvekSkin", lv, tyvekOS);

  auto sipmOS = new G4OpticalSurface("SiPMSurface");
  sipmOS->SetType(dielectric_metal); sipmOS->SetFinish(polished);
  sipmOS->SetModel(unified);
  { std::vector<G4double> r = {0,0,0,0}, eff = {0.20,0.40,0.38,0.20};
    auto m = new G4MaterialPropertiesTable();
    m->AddProperty("REFLECTIVITY", e, r); m->AddProperty("EFFICIENCY", e, eff);
    sipmOS->SetMaterialPropertiesTable(m); }
  if (fSiPMLV) new G4LogicalSkinSurface("SiPMSkin", fSiPMLV, sipmOS);
}

//----------------------------------------------------------------------------
void DetectorConstruction::ConstructSDandField()
{
  auto sdman = G4SDManager::GetSDMpointer();
  auto caloSD = new CaloSD("LYSO_SD", "LYSO_HC", kNLyso, fNModules);
  sdman->AddNewDetector(caloSD);
  SetSensitiveDetector(fLysoLV, caloSD);
}
