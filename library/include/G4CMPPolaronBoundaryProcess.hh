#ifndef G4CMPPolaronBoundaryProcess_h
#define G4CMPPolaronBoundaryProcess_h

#include "G4VDiscreteProcess.hh"

class G4CMPPolaronBoundaryProcess : public G4VDiscreteProcess {
public:
  G4CMPPolaronBoundaryProcess(const G4String& name = "PolaronBoundary");
  virtual ~G4CMPPolaronBoundaryProcess();

  virtual G4double GetMeanFreePath(const G4Track& track,
                                   G4double previousStepSize,
                                   G4ForceCondition* condition) override;

  virtual G4VParticleChange* PostStepDoIt(const G4Track& track,
                                          const G4Step& step) override;

private:
  G4ParticleChange aParticleChange;
};

#endif
