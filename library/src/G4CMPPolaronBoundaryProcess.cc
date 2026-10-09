#include "G4CMPPolaronBoundaryProcess.hh"
#include "G4CMPUtils.hh"
#include "G4Step.hh"
#include "G4StepPoint.hh"
#include "G4SystemOfUnits.hh"
#include "G4Track.hh"

G4CMPPolaronBoundaryProcess::G4CMPPolaronBoundaryProcess(const G4String& name)
  : G4VDiscreteProcess(name, fUserDefined) {
  if (verboseLevel)
    G4cout << GetProcessName() << " created." << G4endl;
}

G4CMPPolaronBoundaryProcess::~G4CMPPolaronBoundaryProcess() {}

G4double G4CMPPolaronBoundaryProcess::GetMeanFreePath(const G4Track&,
                                                      G4double,
                                                      G4ForceCondition* condition) {
  *condition = Forced;
  return DBL_MAX;
}

G4VParticleChange* G4CMPPolaronBoundaryProcess::PostStepDoIt(const G4Track& aTrack,
                                                             const G4Step& aStep) {
  aParticleChange.Initialize(aTrack);

  if (!G4CMP::IsPolaron(aTrack)) return &aParticleChange;

  G4StepPoint* postPoint = aStep.GetPostStepPoint();
  if (postPoint->GetStepStatus() != fGeomBoundary) return &aParticleChange;

  G4double ekin = aTrack.GetKineticEnergy();
  if (ekin > 0.) {
    aParticleChange.ProposeNonIonizingEnergyDeposit(ekin);
    if (verboseLevel > 0)
      G4cout << GetProcessName() << " absorbed polaron, deposited " << ekin/eV << " eV" << G4endl;
  }
  aParticleChange.ProposeTrackStatus(fStopAndKill);
  aParticleChange.ProposeEnergy(0.);

  ClearNumberOfInteractionLengthLeft();
  return &aParticleChange;
}
