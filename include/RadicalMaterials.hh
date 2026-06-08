//============================================================================
// RadicalMaterials.hh
//
// Builds every material used in the simulation and attaches the optical
// property tables (RINDEX, scintillation, WLS, absorption). Centralising this
// keeps DetectorConstruction focused on geometry.
//
// NOTE on optical data: the paper specifies only a few numbers (LYSO/DSB1
// emission/absorption peaks, DSB1 tau = 3.5 ns). NONE of the remaining optical
// constants are measured for this device. Every estimated value is tagged
// "ESTIMATE:" with its basis (grep for it), and all are catalogued in
// ESTIMATES.md. Values taken straight from the paper are tagged "PAPER:".
// Replace the estimates with measured component data when you have it.
//============================================================================
#ifndef RadicalMaterials_hh
#define RadicalMaterials_hh 1

#include "globals.hh"

class G4Material;

class RadicalMaterials
{
  public:
    static RadicalMaterials* Instance();

    void Build();

    G4Material* Air()       const { return fAir; }
    G4Material* Vacuum()    const { return fVacuum; }
    G4Material* LYSO()      const { return fLYSO; }
    G4Material* Tungsten()  const { return fW; }
    G4Material* Tyvek()     const { return fTyvek; }
    G4Material* Quartz()    const { return fQuartz; }
    G4Material* DSB1()      const { return fDSB1; }
    G4Material* Silicon()   const { return fSilicon; }
    G4Material* POM()       const { return fPOM; }
    G4Material* PlasticScint() const { return fPlastic; }
    G4Material* LeadGlass() const { return fLeadGlass; }

  private:
    RadicalMaterials() = default;
    void BuildBaseMaterials();
    void BuildLYSO();
    void BuildQuartz();
    void BuildDSB1();
    void BuildTyvekSurfaceMaterial();
    void BuildPlasticScint();
    void BuildLeadGlass();

    static RadicalMaterials* fInstance;

    G4Material* fAir        = nullptr;
    G4Material* fVacuum     = nullptr;
    G4Material* fLYSO       = nullptr;
    G4Material* fW          = nullptr;
    G4Material* fTyvek      = nullptr;
    G4Material* fQuartz     = nullptr;
    G4Material* fDSB1       = nullptr;
    G4Material* fSilicon    = nullptr;
    G4Material* fPOM        = nullptr;
    G4Material* fPlastic    = nullptr;
    G4Material* fLeadGlass  = nullptr;
};

#endif
