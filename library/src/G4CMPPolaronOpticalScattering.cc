/***********************************************************************\
 * This software is licensed under the terms of the GNU General Public *
 * License version 3 or later. See G4CMP/LICENSE for the full license. *
\***********************************************************************/

/// \file library/src/G4CMPPolaronOpticalScattering.cc
/// \brief Implementation of polaron optical phonon scattering process

#include "G4CMPPolaronOpticalScattering.hh"
#include "G4CMPPolaronInfo.hh"
#include "G4CMPScatteringRateInterface.hh"
#include "G4CMPDriftElectron.hh"
#include "G4CMPDriftHole.hh"
#include "G4CMPUtils.hh"
#include "G4RandomDirection.hh"
#include "G4PhysicalConstants.hh"
#include "G4SystemOfUnits.hh"
#include "G4Track.hh"
#include "G4Step.hh"
#include "G4StepPoint.hh"
#include <cmath>

// ══════════════════════════════════════════════════════════════════════
// STATIC MEMBER INITIALIZATION
// ══════════════════════════════════════════════════════════════════════

std::set<G4int> G4CMPPolaronOpticalScattering::scatteredTracks;

// ══════════════════════════════════════════════════════════════════════
// CONSTRUCTOR AND DESTRUCTOR
// ══════════════════════════════════════════════════════════════════════

G4CMPPolaronOpticalScattering::G4CMPPolaronOpticalScattering(const G4String& processName)
  : G4VDiscreteProcess(processName, fUserDefined),
    fAlphaThreshold(1.0),  // Default: activate only for strong coupling (α ≥ 1.0)
    fScatteringRate(nullptr),
    verboseLevel(0) {

  SetProcessSubType(324);  // Unique process ID for polaron optical scattering

  // Create scattering rate calculator (uses sapphire parameters by default)
  fScatteringRate = new G4CMPOpticalScatteringRate();

  if (verboseLevel > 0) {
    G4cout << "\n=== G4CMPPolaronOpticalScattering Created ===" << G4endl;
    G4cout << "  Alpha threshold: " << fAlphaThreshold << G4endl;
    G4cout << "  Process SubType: " << GetProcessSubType() << G4endl;
    G4cout << "============================================\n" << G4endl;
  }
}

G4CMPPolaronOpticalScattering::~G4CMPPolaronOpticalScattering() {
  if (fScatteringRate) {
    delete fScatteringRate;
    fScatteringRate = nullptr;
  }
}

// ══════════════════════════════════════════════════════════════════════
// APPLICABILITY
// ══════════════════════════════════════════════════════════════════════

G4bool G4CMPPolaronOpticalScattering::IsApplicable(const G4ParticleDefinition& aPD) {
  // Only applies to electrons and holes (will check for polaron marker in GetMeanFreePath)
  return (aPD == *G4CMPDriftElectron::Definition() ||
          aPD == *G4CMPDriftHole::Definition());
}

// ══════════════════════════════════════════════════════════════════════
// MEAN FREE PATH CALCULATION
// ══════════════════════════════════════════════════════════════════════

G4double G4CMPPolaronOpticalScattering::GetMeanFreePath(
    const G4Track& aTrack,
    G4double previousStepSize,
    G4ForceCondition* condition) {

  *condition = NotForced;

  // Only applies to polarons
  if (!IsPolaronTrack(aTrack)) {
    return DBL_MAX;  // Not a polaron, skip this process
  }

  // Check alpha threshold
  G4double alpha = GetAlphaCoupling(aTrack);
  if (alpha < fAlphaThreshold) {
    return DBL_MAX;  // Coupling too weak, skip
  }

  // Calculate scattering rate
  G4double rate = CalculateScatteringRate(aTrack);
  if (rate <= 0.) {
    return DBL_MAX;  // No scattering at this energy
  }

  // Convert rate [1/time] to mean free path
  G4double velocity = aTrack.GetVelocity();
  if (velocity <= 0.) {
    return DBL_MAX;
  }

  G4double meanFreePath = velocity / rate;

  if (verboseLevel > 1) {
    G4cout << "G4CMPPolaronOpticalScattering::GetMeanFreePath" << G4endl;
    G4cout << "  Polaron detected (α = " << alpha << ")" << G4endl;
    G4cout << "  Scattering rate: " << rate / hertz << " Hz" << G4endl;
    G4cout << "  Mean free path: " << meanFreePath / micrometer << " μm" << G4endl;
  }

  return meanFreePath;
}

// ══════════════════════════════════════════════════════════════════════
// POST-STEP SCATTERING
// ══════════════════════════════════════════════════════════════════════

G4VParticleChange* G4CMPPolaronOpticalScattering::PostStepDoIt(const G4Track& aTrack,
                                                               const G4Step& aStep) {
  aParticleChange.Initialize(aTrack);

  // Verify polaron status (should be guaranteed by GetMeanFreePath)
  if (!IsPolaronTrack(aTrack)) {
    return &aParticleChange;
  }

  // Don't process at volume boundaries
  G4StepPoint* postStepPoint = aStep.GetPostStepPoint();
  if (postStepPoint->GetStepStatus() == fGeomBoundary) {
    return &aParticleChange;
  }

  // Get track info
  G4int trackID = aTrack.GetTrackID();
  G4double kinE = aTrack.GetKineticEnergy();
  G4double alpha = GetAlphaCoupling(aTrack);

  if (verboseLevel > 0) {
    G4cout << "\n[PolaronOpticalScattering::PostStepDoIt]" << G4endl;
    G4cout << "  Track ID: " << trackID << G4endl;
    G4cout << "  Kinetic energy: " << kinE / eV << " eV" << G4endl;
    G4cout << "  Alpha coupling: " << alpha << G4endl;
  }

  // Randomize direction (isotropic scattering in space)
  G4ThreeVector newDirection = G4RandomDirection();
  aParticleChange.ProposeMomentumDirection(newDirection);

  if (verboseLevel > 0) {
    G4cout << "  New direction: (" << newDirection.x() << ", "
           << newDirection.y() << ", " << newDirection.z() << ")" << G4endl;
    G4cout << "[PolaronOpticalScattering::PostStepDoIt] END\n" << G4endl;
  }

  return &aParticleChange;
}

// ══════════════════════════════════════════════════════════════════════
// HELPER METHODS
// ══════════════════════════════════════════════════════════════════════

G4bool G4CMPPolaronOpticalScattering::IsPolaronTrack(const G4Track& aTrack) const {
  // Check if track has G4CMPPolaronInfo attached
  const G4VUserTrackInformation* userInfo = aTrack.GetUserInformation();
  if (!userInfo) return false;

  const G4CMPPolaronInfo* polaronInfo = dynamic_cast<const G4CMPPolaronInfo*>(userInfo);
  if (!polaronInfo) return false;

  return polaronInfo->IsPolaron();
}

G4double G4CMPPolaronOpticalScattering::GetAlphaCoupling(const G4Track& aTrack) const {
  const G4VUserTrackInformation* userInfo = aTrack.GetUserInformation();
  if (!userInfo) return 0.;

  const G4CMPPolaronInfo* polaronInfo = dynamic_cast<const G4CMPPolaronInfo*>(userInfo);
  if (!polaronInfo) return 0.;

  return polaronInfo->GetAlphaCoupling();
}

G4double G4CMPPolaronOpticalScattering::CalculateScatteringRate(const G4Track& aTrack) const {
  if (!fScatteringRate) return 0.;

  // Use the polaron optical scattering rate calculator
  G4double energyEV = aTrack.GetKineticEnergy() / eV;
  G4double temperature = 300.0;  // Room temperature (K)
  const G4ParticleDefinition* particle = aTrack.GetParticleDefinition();

  return fScatteringRate->GetScatteringRate(energyEV, temperature, particle, true);
}
