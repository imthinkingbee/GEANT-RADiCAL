//============================================================================
// DetectorConstruction.hh
//============================================================================
#ifndef DetectorConstruction_hh
#define DetectorConstruction_hh 1

#include "G4VUserDetectorConstruction.hh"
#include "globals.hh"
#include <vector>

class G4VPhysicalVolume;
class G4LogicalVolume;
class G4VSolid;
class DetectorMessenger;

class DetectorConstruction : public G4VUserDetectorConstruction
{
  public:
    DetectorConstruction();
    ~DetectorConstruction() override;

    G4VPhysicalVolume* Construct() override;
    void ConstructSDandField() override;

    // messenger hooks
    void SetBuildBeamline(G4bool b)  { fBuildBeamline = b; }
    void SetBuildPbGlass(G4bool b)   { fBuildPbGlass = b; }
    void SetBuildMCP(G4bool b)       { fBuildMCP = b; }
    void SetBuildTriggers(G4bool b)  { fBuildTriggers = b; }
    void SetLysoYieldScale(G4double s) { fLysoYieldScale = s; }
    void SetWlsCenterZ(G4double z)   { fWlsCenterZ = z; fWlsCenterZSet = true; }

    // geometry queries
    G4double GetLysoLayerZ(G4int i) const { return fLysoZ.at(i); }

  private:
    void DefineMaterials();
    G4VSolid* MakeHoledPlate(const G4String& name, G4double thickness) const;
    void BuildModule(G4LogicalVolume* world);
    void BuildCapillaries(G4LogicalVolume* world);
    void BuildBeamline(G4LogicalVolume* world);
    void ApplyOpticalSurfaces();

    DetectorMessenger* fMessenger = nullptr;

    G4LogicalVolume* fWorldLV = nullptr;
    G4LogicalVolume* fLysoLV  = nullptr;   // sensitive
    G4LogicalVolume* fPbGlassLV = nullptr; // sensitive (leakage)
    std::vector<G4LogicalVolume*> fSiPMLVs; // 8 SiPM logical volumes
    std::vector<G4LogicalVolume*> fTyvekLVs;

    std::vector<G4double> fLysoZ;          // z of each LYSO layer centre

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
