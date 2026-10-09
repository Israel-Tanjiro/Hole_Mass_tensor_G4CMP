/***********************************************************************\
 * This software is licensed under the terms of the GNU General Public *
 * License version 3 or later. See G4CMP/LICENSE for the full license. *
\***********************************************************************/

/// \file library/src/G4CMPPolaronScattering.cc
/// \brief Implementation of polaron scattering discrete process

#include "G4CMPPolaronScattering.hh"
#include "G4CMPPolaronScatteringRate.hh"
#include "G4CMPPolaronScatteringKinematics.hh"
#include "G4CMPDriftElectronPolaron.hh"
#include "G4CMPDriftHolePolaron.hh"
#include "G4ParticleChangeForLoss.hh"
#include "G4PhysicalConstants.hh"
#include "G4SystemOfUnits.hh"
#include "G4Track.hh"
#include "G4Step.hh"
#include "Randomize.hh"
#include "G4CMPUtils.hh"
#include <cmath>

// ============================================================================
// CONSTRUCTOR AND DESTRUCTOR
// ============================================================================

G4CMPPolaronScattering::G4CMPPolaronScattering(const G4String& processName)
  : G4VDiscreteProcess(processName, fUserDefined),
    fScatteringRate(0),
    fKinematics(0),
    fVerboseLevel(0) {

  fScatteringRate = new G4CMPPolaronScatteringRate();
  fKinematics = new G4CMPPolaronScatteringKinematics();

  SetProcessSubType(330);  // Custom subtype for polaron scattering
}

G4CMPPolaronScattering::~G4CMPPolaronScattering() {
  if (fScatteringRate) delete fScatteringRate;
  if (fKinematics) delete fKinematics;
}

// ============================================================================
// APPLICABILITY
// ============================================================================

G4bool G4CMPPolaronScattering::IsApplicable(const G4ParticleDefinition& aParticle) {
  return (G4CMP::IsElectronPolaron(&aParticle) || G4CMP::IsHolePolaron(&aParticle));
}

// ============================================================================
// MEAN FREE PATH
// ============================================================================

G4double G4CMPPolaronScattering::GetMeanFreePath(const G4Track& track,
                                                  G4double,
                                                  G4ForceCondition* condition) {

  // Γ_total is returned by fScatteringRate->Rate() in INTERNAL units,
  // i.e. already "per ns" (it's built from hw/hbar_Planck, and
  // hbar_Planck is in MeV*ns -- see G4CMPOpticalScatteringRate etc.).
  G4double Γ_total = fScatteringRate->Rate(track);  // 1/ns (internal units)

  if (Γ_total <= 0.) {
    *condition = Forced;
    return DBL_MAX;
  }

  // ------------------------------------------------------------------
  // Polaron group velocity. NOTE: currently a fixed placeholder
  // (1.1e4 m/s) independent of the track's kinetic energy. Both
  // v_polaron and Γ_total below are expressed in CONSISTENT internal
  // units (mm/ns and 1/ns respectively), so the MFP formula needs no
  // extra "hertz" factor.
  // ------------------------------------------------------------------
  G4double v_polaron = 1.1e4 * m / second;  // mm/ns (internal units) == 11000 m/s

  // ------------------------------------------------------------------
  // λ = v / Γ   (mm/ns) / (1/ns) = mm.  Both quantities are already in
  // internal units, so no hertz conversion belongs here at all.
  //
  // PREVIOUS (buggy) version:
  //   lambda = v_polaron / (Γ_total * hertz);
  // Since hertz = 1e-9, that divides by a denominator ~1e9x too small,
  // i.e. INFLATES lambda by ~1e9x -- effectively switching this process
  // off (MFP grows from nm/mm scale to m/km scale).
  // ------------------------------------------------------------------
  G4double lambda = v_polaron / Γ_total;  // mm (internal units)

  // ------------------------------------------------------------------
  // DEBUG: dump everything needed to sanity-check units. Printed once
  // per distinct kinetic energy seen (so both the 1 MeV and 0.01 eV
  // test cases each get one clear block, without flooding the log
  // every step). Enable with fVerboseLevel > 0 (SetVerboseLevel()).
  // ------------------------------------------------------------------
  if (fVerboseLevel > 0) {
    static G4double lastEkin = -1.;
    G4double Ekin = track.GetKineticEnergy();
    if (lastEkin < 0. || std::abs(Ekin - lastEkin) > 1e-9 * eV) {
      lastEkin = Ekin;

      G4double Gamma_Hz       = Γ_total / hertz;            // correct: 1/ns -> Hz
      G4double lambda_old_buggy = v_polaron / (Γ_total * hertz); // old formula, for comparison

      G4cout << "\n=== G4CMPPolaronScattering::GetMeanFreePath DEBUG ==="
             << "\n  Particle              = " << track.GetParticleDefinition()->GetParticleName()
             << "\n  Ekin                  = " << Ekin / eV << " eV"
             << "\n  Gamma_total (raw)     = " << Γ_total   << " /ns"
             << "\n  Gamma_total / hertz   = " << Gamma_Hz  << " Hz"
             << "\n  v_polaron             = " << v_polaron << " mm/ns ("
             << v_polaron / (m/second) << " m/s)"
             << "\n  lambda = v/Gamma      = " << lambda << " mm  (" << lambda / m << " m)"
             << "\n  lambda old (v/(Γ·Hz)) = " << lambda_old_buggy << " mm  ("
             << lambda_old_buggy / m << " m)  <-- previous buggy result"
             << "\n======================================================\n"
             << G4endl;
    }
  }

  *condition = NotForced;
  return lambda;
}

// ============================================================================
// SCATTERING EVENT
// ============================================================================
G4VParticleChange* G4CMPPolaronScattering::PostStepDoIt(const G4Track& aTrack,
                                                         const G4Step& aStep) {
  // DEBUG: Check interaction length status
  G4double nIL = GetNumberOfInteractionLengthLeft();

  static int call_count = 0;
  if (++call_count % 5 == 0) {
    G4cout << "PostStepDoIt call #" << call_count
           << " nIL=" << nIL << G4endl;
  }

  if (nIL > 0.) {
    if (fVerboseLevel > 1) {
      G4cout << "  -> Skipping (nIL > 0)" << G4endl;
    }
    G4ParticleChangeForLoss* pc = new G4ParticleChangeForLoss();
    pc->InitializeForPostStep(aTrack);
    return pc;
  }

  if (fVerboseLevel > 1) {
    G4cout << "  -> Scattering triggered (nIL <= 0)" << G4endl;
  }

  // ============================================================================
  // CHECK: Has the scattering interaction length been reached?
  // ============================================================================

  if (GetNumberOfInteractionLengthLeft() > 0.) {
    G4ParticleChangeForLoss* pc = new G4ParticleChangeForLoss();
    pc->InitializeForPostStep(aTrack);
    if (fVerboseLevel > 2) {
      G4cout << "PolaronScattering: Interaction length not reached yet" << G4endl;
    }
    return pc;
  }

  // ============================================================================
  // SCATTERING HAS TRIGGERED - Apply physics
  // ============================================================================

  G4ParticleChangeForLoss* aParticleChange = new G4ParticleChangeForLoss();
  aParticleChange->InitializeForPostStep(aTrack);

  G4double Ekin_initial = aTrack.GetKineticEnergy();

  if (Ekin_initial <= 0.) {
    aParticleChange->ProposeTrackStatus(fStopAndKill);
    ClearNumberOfInteractionLengthLeft();
    return aParticleChange;
  }

  // Choose scattering mechanism
  G4int mechanism = fScatteringRate->ChooseScatteringMechanism(aTrack);
  G4String mech_name = fScatteringRate->GetDominantMechanism(aTrack);

  if (mechanism == 0) mech_name = "Acoustic";
  else if (mechanism == 1) mech_name = "Optical";
  else if (mechanism == 2) mech_name = "Impurity";

  // NOTE: fScatteringRate->Rate() returns the raw internal rate in 1/ns,
  // NOT Hz. Convert via "/ hertz" to report an actual Hz value (previous
  // version printed the raw 1/ns number but labeled it "Hz").
  G4double Gamma_raw = fScatteringRate->Rate(aTrack);   // 1/ns
  G4double Gamma_Hz  = Gamma_raw / hertz;               // Hz

  if (fVerboseLevel > 0) {
    G4cout << ">>> PolaronScattering_" << mech_name
           << " E=" << Ekin_initial / eV << " eV"
           << " Gamma=" << Gamma_raw << " /ns"
           << " (" << Gamma_Hz << " Hz)" << G4endl;
  }

  G4double E_loss = 0.;
  G4double E_final = Ekin_initial;

  // Apply energy loss based on mechanism
  if (mechanism == 0) {  // Acoustic
    E_loss = 0.001 * eV;  // ~1 meV
  } else if (mechanism == 1) {  // Optical
    E_loss = 0.0597 * eV;  // ~59.7 meV
  } else {  // Impurity
    E_loss = 0.;  // Elastic
  }

  // Check if polaron has enough energy
  if (Ekin_initial <= E_loss) {
    aParticleChange->ProposeTrackStatus(fStopAndKill);
    aParticleChange->ProposeLocalEnergyDeposit(Ekin_initial);
    ClearNumberOfInteractionLengthLeft();
    return aParticleChange;
  }

  // Update energy
  E_final = Ekin_initial - E_loss;
  aParticleChange->ProposeLocalEnergyDeposit(E_loss);

  // Update direction with scattering angle
  G4double cosTheta = fKinematics->SampleIsotropicAngle();
  if (mechanism == 0 || mechanism == 2) {
    cosTheta = fKinematics->SampleForwardPeakedAngle(mechanism == 2);
  }
  G4ThreeVector newDir = fKinematics->RotateDirection(aTrack.GetMomentumDirection(), cosTheta);
  aParticleChange->ProposeMomentumDirection(newDir);

  // Check if polaron is still alive
  if (E_final <= 0.) {
    aParticleChange->ProposeTrackStatus(fStopAndKill);
  } else {
    aParticleChange->ProposeTrackStatus(fAlive);
  }

  if (fVerboseLevel > 1) {
    G4String mech_name2 = fScatteringRate->GetDominantMechanism(aTrack);
    G4cout << "PolaronScattering: E_in=" << Ekin_initial / eV << " eV"
           << " dE=" << E_loss / eV << " eV"
           << " Mechanism=" << mech_name2 << G4endl;
  }

  // ============================================================================
  // CRITICAL: Reset interaction length counter after scattering
  // ============================================================================

  ClearNumberOfInteractionLengthLeft();

  return aParticleChange;
}

// ============================================================================
// DIAGNOSTICS
// ============================================================================

void G4CMPPolaronScattering::SetVerboseLevel(G4int vb) {
  fVerboseLevel = vb;
  if (fScatteringRate) fScatteringRate->SetVerboseLevel(vb);
  if (fKinematics) fKinematics->SetVerboseLevel(vb);
}
