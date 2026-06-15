//============================================================================
// DetectorConstruction.hh
//
// Supports three geometry modes (set PreInit via /radical/det/geometry):
//   single    - the baseline 14x14 module (4 corner caps + 1 empty centre)
//   array3x3  - 3x3 array of the enhanced 18x18 modules (Fig. 28, 9 caps each)
//   hex       - a single hexagonal module (7 caps: 6 ring + 1 centre)
//
// Each module's internals (plate stack, Tyvek wrap, capillaries, SiPMs) are
// built inside one air ENVELOPE logical volume, which is then placed once
// (single/hex) or 9 times (array). The envelope's placement copy number is the
// MODULE index; the LYSO plate copy number is the LAYER index. So a hit's
// module = touchable depth 1, layer = touchable depth 0.
//============================================================================
#ifndef DetectorConstruction_hh
#define DetectorConstruction_hh 1

#include "G4VUserDetectorConstruction.hh"
#include "G4ThreeVector.hh"
#include "G4TwoVector.hh"
#include "globals.hh"
#include <vector>

class G4VPhysicalVolume;
class G4LogicalVolume;
class G4VSolid;
class DetectorMessenger;

enum class GeoMode { Single, Array3x3, Hex };

class DetectorConstruction : public G4VUserDetectorConstruction
{
  public:
    DetectorConstruction();
    ~DetectorConstruction() override;

    G4VPhysicalVolume* Construct() override;
    void ConstructSDandField() override;

    // messenger hooks
    void SetGeometry(const G4String& name);
    void SetBuildBeamline(G4bool b)  { fBuildBeamline = b; }
    void SetBuildPbGlass(G4bool b)   { fBuildPbGlass = b; }
    void SetBuildMCP(G4bool b)       { fBuildMCP = b; }
    void SetBuildTriggers(G4bool b)  { fBuildTriggers = b; }
    void SetLysoYieldScale(G4double s) { fLysoYieldScale = s; }
    void SetWlsCenterZ(G4double z)   { fWlsCenterZ = z; fWlsCenterZSet = true; }

    G4int GetNModules() const { return fNModules; }

  private:
    // One capillary hole in a tile (local transverse position).
    struct Hole {
      G4TwoVector pos;
      G4double    dia;
      G4bool      instrumented;   // true -> quartz cap + WLS + 2 SiPMs; false -> air
    };
    // Full description of one module type.
    struct ModuleSpec {
      G4bool   hexagonal = false;
      G4double tileXY    = 0.;     // box full width, or hex flat-to-flat
      G4double lysoThick = 0.;
      G4double wThick    = 0.;
      G4int    nLyso     = 0;
      G4int    nW        = 0;
      std::vector<Hole> holes;
      G4double stackLength() const {
        return nLyso*lysoThick + nW*wThick + (nLyso+nW-1)*0.1; // 0.1 mm Tyvek
      }
    };

    void DefineMaterials();
    ModuleSpec MakeSpec() const;                 // build the spec for fMode
    G4VSolid* MakeHoledPlate(const G4String& name, const ModuleSpec& s,
                             G4double thickness) const;
    G4VSolid* MakeShell(const G4String& name, const ModuleSpec& s,
                        G4double inset, G4double thick, G4double length) const;
    // Builds one module envelope LV (all internals); returns it for placement.
    G4LogicalVolume* BuildModuleEnvelope(const ModuleSpec& s);
    void BuildBeamline(G4LogicalVolume* world);
    void ApplyOpticalSurfaces();

    DetectorMessenger* fMessenger = nullptr;
    GeoMode fMode = GeoMode::Single;

    G4LogicalVolume* fWorldLV   = nullptr;
    G4LogicalVolume* fLysoLV    = nullptr;   // sensitive (per-layer)
    G4LogicalVolume* fPbGlassLV = nullptr;
    G4LogicalVolume* fSiPMLV    = nullptr;
    std::vector<G4LogicalVolume*> fTyvekLVs;

    std::vector<G4double> fLysoZ;            // z of each LYSO layer centre
    G4double fEnvLength = 0.;                // module envelope full length (z)
    G4int    fNModules  = 1;

    // build flags
    G4bool   fBuildBeamline = true;
    G4bool   fBuildPbGlass  = true;
    G4bool   fBuildMCP      = true;
    G4bool   fBuildTriggers = true;
    G4double fLysoYieldScale = 1.0;
    G4bool   fWlsCenterZSet  = false;
    G4double fWlsCenterZ     = 0.;
};

#endif
