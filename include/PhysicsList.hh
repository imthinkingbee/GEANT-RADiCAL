//============================================================================
// PhysicsList.hh
//
// FTFP_BERT (full hadronic + EM) with the high-accuracy EM option4 and the
// G4OpticalPhysics constructor (scintillation, Cerenkov, WLS, boundary,
// absorption, Rayleigh). This gives the "full calorimetry" stack requested:
// electromagnetic, hadronic and optical with photostatistics.
//============================================================================
#ifndef PhysicsList_hh
#define PhysicsList_hh 1

#include "FTFP_BERT.hh"

class PhysicsList : public FTFP_BERT
{
  public:
    explicit PhysicsList(G4int verbose = 0);
    ~PhysicsList() override = default;
};

#endif
