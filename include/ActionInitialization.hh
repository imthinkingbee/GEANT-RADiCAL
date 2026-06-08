//============================================================================
// ActionInitialization.hh
//============================================================================
#ifndef ActionInitialization_hh
#define ActionInitialization_hh 1

#include "G4VUserActionInitialization.hh"

class ActionInitialization : public G4VUserActionInitialization
{
  public:
    ActionInitialization() = default;
    ~ActionInitialization() override = default;

    void Build() const override;
    void BuildForMaster() const override;
};

#endif
