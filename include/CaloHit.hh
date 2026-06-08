//============================================================================
// CaloHit.hh - one hit per LYSO layer accumulating deposited energy.
//============================================================================
#ifndef CaloHit_hh
#define CaloHit_hh 1

#include "G4VHit.hh"
#include "G4THitsCollection.hh"
#include "G4Allocator.hh"
#include "globals.hh"

class CaloHit : public G4VHit
{
  public:
    CaloHit() = default;
    explicit CaloHit(G4int layer) : fLayer(layer) {}
    ~CaloHit() override = default;

    inline void* operator new(size_t);
    inline void  operator delete(void*);

    void AddEdep(G4double e) { fEdep += e; }
    G4int    GetLayer() const { return fLayer; }
    G4double GetEdep()  const { return fEdep; }
    void SetLayer(G4int l) { fLayer = l; }

  private:
    G4int    fLayer = -1;
    G4double fEdep  = 0.;
};

using CaloHitsCollection = G4THitsCollection<CaloHit>;

extern G4ThreadLocal G4Allocator<CaloHit>* CaloHitAllocator;

inline void* CaloHit::operator new(size_t)
{
  if (!CaloHitAllocator) CaloHitAllocator = new G4Allocator<CaloHit>;
  return (void*)CaloHitAllocator->MallocSingle();
}

inline void CaloHit::operator delete(void* hit)
{
  CaloHitAllocator->FreeSingle((CaloHit*)hit);
}

#endif
