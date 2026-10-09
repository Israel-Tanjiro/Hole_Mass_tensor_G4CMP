/***********************************************************************\
 * This software is licensed under the terms of the GNU General Public *
 * License version 3 or later. See G4CMP/LICENSE for the full license. *
\***********************************************************************/

/// \file library/src/G4CMPIntraValleyScattering.cc
/// \brief Implementation of discrete process for intravalley scattering

#include "G4CMPIntraValleyScattering.hh"
#include "G4CMPPolaronInfo.hh"
#include "G4CMPDriftElectron.hh"
#include "G4CMPDriftHole.hh"
#include "G4CMPDriftTrackInfo.hh"
#include "G4CMPTrackUtils.hh"
#include "G4CMPUtils.hh"
#include "G4ParticleChangeForLoss.hh"
#include "G4PhysicalConstants.hh"
#include "G4SystemOfUnits.hh"
#include "G4Track.hh"
#include "G4Step.hh"
#include "G4StepPoint.hh"
#include "G4LatticeManager.hh"
#include "G4LatticePhysical.hh"
#include "Randomize.hh"
#include <iomanip>

// ============================================================================
// CONSTRUCTOR AND DESTRUCTOR
// ============================================================================

G4CMPIntraValleyScattering::G4CMPIntraValleyScattering(const G4String& processName)
  : G4CMPVDriftProcess(processName, static_cast<G4CMPProcessSubType>(331)),
    fRateModel(nullptr) {

  fRateModel = new G4CMPIntraValleyRate(processName);
}

G4CMPIntraValleyScattering::~G4CMPIntraValleyScattering() {
  if (fRateModel) delete fRateModel;
}

// ============================================================================
// APPLICABILITY
// ============================================================================

G4bool G4CMPIntraValleyScattering::IsApplicable(const G4ParticleDefinition& aParticle) {
  // Apply to electrons and holes (both bare and polarons)
  return (G4CMP::IsElectron(&aParticle) || G4CMP::IsHole(&aParticle));
}

// ============================================================================
// MEAN FREE PATH CALCULATION
// ============================================================================

G4double G4CMPIntraValleyScattering::GetMeanFreePath(const G4Track& aTrack,
                                                      G4double,
                                                      G4ForceCondition* condition) {
  static G4int call_count = 0;
  ++call_count;

  G4cout << "\n█████ [GetMeanFreePath Call #" << call_count << "] █████" << G4endl;
  G4cout << "  Particle: " << aTrack.GetParticleDefinition()->GetParticleName() << G4endl;
  G4cout << "  E_kin: " << aTrack.GetKineticEnergy() / eV << " eV" << G4endl;
  G4cout << "  Step#: " << aTrack.GetCurrentStepNumber() << G4endl;

  // ========================================================================
  // CRITICAL: Only scattering happens for POLARONS
  // If this is NOT a polaron, don't limit the step
  // ========================================================================

  G4bool isPolaron = fRateModel->IsPolaronTrack(aTrack);
  G4cout << "  Is Polaron? " << (isPolaron ? "YES ✓" : "NO ✗") << G4endl;

  if (!isPolaron) {
    G4cout << "  → SKIP: Not a polaron, returning DBL_MAX" << G4endl;
    G4cout << "█████ [GetMeanFreePath END] █████\n" << G4endl;
    *condition = NotForced;
    return DBL_MAX;  // Not a polaron - no IntraValley scattering
  }

  // Load material data
  G4cout << "  → Loading data for track..." << G4endl;
  const_cast<G4CMPIntraValleyRate*>(fRateModel)->LoadDataForTrack(&aTrack);

  // Get scattering rate (will be non-zero only for polarons)
  // Γ_total is in G4 internal rate units (1/ns); divide by "hertz" (=1e-9)
  // to convert to Hz for display.
  G4double Γ_total = fRateModel->Rate(aTrack);
  G4cout << "  Γ_total: " << Γ_total / hertz << " Hz" << G4endl;

  if (Γ_total <= 0.) {
    G4cout << "  → Rate <= 0! Returning DBL_MAX" << G4endl;
    G4cout << "█████ [GetMeanFreePath END] █████\n" << G4endl;
    *condition = Forced;
    return DBL_MAX;  // No scattering
  }

  // Get carrier velocity
  // For now, use simple estimate; can be refined with proper band structure
  G4double v_polaron = 1.1e4 * m / s;  // ~11 km/s (order of magnitude)
  G4cout << "  v_polaron: " << v_polaron / (m/s) << " m/s" << G4endl;

  // Mean free path: λ = v / Γ
  // v_polaron is already in G4 internal units (mm/ns), and Γ_total is already
  // in G4 internal rate units (1/ns), since Γ_total = Σ(hw/hbar_Planck) with
  // hw and hbar_Planck both in internal MeV*ns units. The ratio v/Γ is
  // therefore directly in mm (G4's internal length unit) -- no extra "hertz"
  // factor is needed. Multiplying by hertz (=1e-9) here inflated lambda by
  // 1e9x, which the previous hardcoded "lambda=0.00001*m" override was
  // apparently papering over.
  G4double lambda = v_polaron / Γ_total;
  G4cout << "  λ (MFP): " << lambda / micrometer << " µm" << G4endl;
  G4cout << "  Condition: NotForced" << G4endl;
  G4cout << "█████ [GetMeanFreePath END - SCATTERING WILL HAPPEN] █████\n" << G4endl;

  *condition = NotForced;
  return lambda;
}

// ============================================================================
// SCATTERING EVENT
// ============================================================================

G4VParticleChange* G4CMPIntraValleyScattering::PostStepDoIt(const G4Track& aTrack,
                                                             const G4Step& aStep) {
  // Initialize inherited aParticleChange member variable (like Luke does)
  InitializeParticleChange(GetValleyIndex(aTrack), aTrack);
  G4StepPoint* postStepPoint = aStep.GetPostStepPoint();

  // Don't do anything at a volume boundary
  if (postStepPoint->GetStepStatus() == fGeomBoundary) {
    return &aParticleChange;
  }

  // ========================================================================
  // POLARON CHECK: Only process polaron-marked carriers!
  // ========================================================================
  G4bool isPolaron = fRateModel->IsPolaronTrack(aTrack);
  if (!isPolaron) {
    return &aParticleChange;  // Not a polaron - return unchanged
  }

  // ========================================================================
  // COLLECT ANCILLARY INFORMATION (like Luke does)
  // ========================================================================
  const G4String& trkName = aTrack.GetDefinition()->GetParticleName();
  auto trackInfo = G4CMP::GetTrackInfo<G4CMPDriftTrackInfo>(aTrack);
  const G4LatticePhysical* lat = trackInfo->Lattice();

  G4int iValley = GetValleyIndex(aTrack);  // Doesn't change valley for IntraValley
  G4double Etrk = GetKineticEnergy(aTrack);

  if (Etrk <= 0.) {
    aParticleChange.ProposeTrackStatus(fStopAndKill);
    ClearNumberOfInteractionLengthLeft();
    return &aParticleChange;
  }

  // ========================================================================
  // CHECK: Has polaron thermalized to ground state?
  // If below LO threshold, optical scattering is frozen and no energy loss
  // is possible. Thermalization complete - kill track.
  // (When E-field + Luke are enabled later, new tracks will have energy input)
  // ========================================================================
  G4double threshold_meV = 91.1;  // LO emission threshold
  G4double Etrk_meV = Etrk / eV * 1000.0;
  if (Etrk_meV < threshold_meV) {
    // Polaron has reached ground state - no more energy loss possible
    if (GetVerboseLevel() > 0) {
      G4cout << "IntraValley: " << trkName << " thermalized to ground state at "
             << Etrk/eV << " eV (below " << threshold_meV << " meV threshold). "
             << "Terminating track." << G4endl;
    }
    aParticleChange.ProposeTrackStatus(fStopAndKill);
    ClearNumberOfInteractionLengthLeft();
    return &aParticleChange;
  }

  // Get momentum in local frame (like Luke)
  G4ThreeVector ptrk = GetLocalDirection(aStep.GetPostStepPoint()->GetMomentum());
  G4ThreeVector ktrk(0.);
  G4double mass = 0.;

  if (IsElectron()) {
    ktrk = lat->MapPtoK(iValley, ptrk);
    // Transform to spherical frame (like Luke)
    ktrk = lat->EllipsoidalToSphericalTranformation(iValley, ktrk);
    mass = lat->GetElectronMass();
  } else if (IsHole()) {
    G4ThreeVector p_local = GetLocalMomentum(aTrack);
    ktrk = lat->MapPtoK_hole(p_local);
    ktrk = lat->HoleEllipsoidalToSphericalTransformation(ktrk);
    mass = lat->GetHoleConductivityMass();
  } else {
    G4Exception("G4CMPIntraValleyScattering::PostStepDoIt", "IntraValley001",
                EventMustBeAborted, "Unknown charge carrier");
    return &aParticleChange;
  }

  G4double kmag = ktrk.mag();
  G4ThreeVector kdir = ktrk.unit();

  if (GetVerboseLevel() > 1) {
    G4cout << trkName << " IntraValley: E=" << Etrk/eV << " eV, kmag=" << kmag << G4endl;
  }

  // ========================================================================
  // INTRAVALLEY SCATTERING PHYSICS (simplified vs Luke)
  // ========================================================================

  // Reload rate model with current track data
  const_cast<G4CMPIntraValleyRate*>(fRateModel)->LoadDataForTrack(&aTrack);

  // Choose scattering mechanism based on rates
  G4double Γ_opt = fRateModel->OpticalRate(aTrack);
  G4double Γ_imp = fRateModel->ImpurityRate(aTrack);
  G4double Γ_total = Γ_opt + Γ_imp;

  G4double E_loss = 0.;
  if (Γ_total > 0.) {
    G4double rand = G4UniformRand() * Γ_total;
    if (rand < Γ_opt) {
      E_loss = 0.0597 * eV;  // Optical: 59.7 meV
    } else {
      E_loss = 0.;  // Impurity: elastic
    }
  }

  // Calculate final energy
  G4double E_final = Etrk - E_loss;
  if (E_final <= 0.) {
    aParticleChange.ProposeTrackStatus(fStopAndKill);
    aParticleChange.ProposeLocalEnergyDeposit(Etrk);
    ClearNumberOfInteractionLengthLeft();
    return &aParticleChange;
  }

  // Sample scattering angle (isotropic for optical, forward-peaked for impurity)
  G4double u = G4UniformRand();
  G4double cos_theta = 2.0 * u - 1.0;  // Default: isotropic

  if (E_loss == 0.) {  // Impurity scattering: forward-peaked
    G4double ln_u = (u > 0.) ? std::log(u) : 0.;
    cos_theta = 1.0 + 2.0 * u * ln_u;
    cos_theta = std::min(1.0, std::max(-1.0, cos_theta));
  }

  G4double theta_scatter = std::acos(cos_theta);  // Convert to angle
  G4double phi = 2.0 * pi * G4UniformRand();

  // Rotate wavevector by scattering angle (like Luke)
  G4ThreeVector ktrk_scattered = ktrk;
  ktrk_scattered.rotate(kdir.orthogonal(), theta_scatter);
  ktrk_scattered.rotate(kdir, phi);

  // Transform back from spherical to ellipsoidal (like Luke)
  G4ThreeVector k_recoil = ktrk_scattered;

  G4ThreeVector precoil;
  if (IsHole()) {
    k_recoil = lat->HoleSphericalToEllipsoidalTransformation(k_recoil);
    precoil = lat->MapKtoP_hole(k_recoil);
    // Adjust momentum to match desired final energy (like Luke's MapEkintoP)
    precoil = lat->MapEkintoP_hole(precoil, E_final);
  } else {  // Electron
    k_recoil = lat->SphericalToEllipsoidalTranformation(iValley, k_recoil);
    precoil = lat->MapKtoP(iValley, k_recoil);
    // Adjust momentum to match desired final energy (like Luke's MapEkintoP)
    precoil = lat->MapEkintoP(iValley, precoil, E_final);
  }

  // Rotate to global direction and call FillParticleChange (like Luke)
  RotateToGlobalDirection(precoil);
  FillParticleChange(iValley, E_final, precoil);

  if (GetVerboseLevel() > 1) {
    G4cout << "IntraValley scatter: " << E_loss/eV*1000 << " meV loss, "
           << "E_final=" << E_final/eV << " eV" << G4endl;
  }

  aParticleChange.ProposeTrackStatus(fAlive);
  ClearNumberOfInteractionLengthLeft();

  return &aParticleChange;
}
