/***********************************************************************\
 * This software is licensed under the terms of the GNU General Public *
 * License version 3 or later. See G4CMP/LICENSE for the full license. *
\***********************************************************************/

/// \file library/src/G4CMPPolaronScatteringRate.cc
/// \brief Implementation of polaron scattering rate model
///
/// Integrates three universal scattering mechanisms (acoustic, optical, impurity)
/// using Mathiessen's rule: Γ_total = Γ_ac + Γ_opt + Γ_imp
/// T = 300K (fixed for Phase 1), Sapphire material

#include "G4CMPPolaronScatteringRate.hh"
#include "G4CMPAcousticScatteringRate.hh"
#include "G4CMPOpticalScatteringRate.hh"
#include "G4CMPImpurityScatteringRate.hh"
#include "G4CMPDriftElectronPolaron.hh"
#include "G4CMPDriftHolePolaron.hh"
#include "G4CMPUtils.hh"
#include "G4PhysicalConstants.hh"
#include "G4SystemOfUnits.hh"
#include "G4Track.hh"
#include <cmath>
#include "Randomize.hh"

// ============================================================================
// STATIC CONSTANTS
// ============================================================================

// Temperature (fixed for Phase 1 - can be parameterized later)
const G4double G4CMPPolaronScatteringRate::fTemperature = 1.0 * kelvin;

// Debye temperature for Sapphire (Al₂O₃)
const G4double G4CMPPolaronScatteringRate::fDebyeTemperature_Sapphire = 1050.0 * kelvin;

// ============================================================================
// CONSTRUCTOR AND DESTRUCTOR
// ============================================================================

G4CMPPolaronScatteringRate::G4CMPPolaronScatteringRate(const G4String& aName)
  : G4CMPVScatteringRate(aName),
    fAcousticRate(0),
    fOpticalRate(0),
    fImpurityRate(0),
    fVerboseLevel(0) {

  // =========================================================================
  // CREATE INSTANCES OF THREE UNIVERSAL SCATTERING RATE CLASSES
  // =========================================================================
  // Each class supports both Option A (hardcoded Sapphire) and
  // Option B (parameterized materials). Using Option A for Phase 1.

  fAcousticRate = new G4CMPAcousticScatteringRate();
  fOpticalRate = new G4CMPOpticalScatteringRate();
  fImpurityRate = new G4CMPImpurityScatteringRate();

  if (fVerboseLevel > 0) {
    G4cout << "G4CMPPolaronScatteringRate constructed (Phase 1: Sapphire)" << G4endl;
    G4cout << "  Temperature: " << fTemperature / kelvin << " K" << G4endl;
    G4cout << "  Debye Temp: " << fDebyeTemperature_Sapphire / kelvin << " K" << G4endl;
    G4cout << "  Three mechanisms:" << G4endl;
    G4cout << "    - Acoustic (Bardeen-Shockley)" << G4endl;
    G4cout << "    - Optical (Fröhlich, 6 LO modes)" << G4endl;
    G4cout << "    - Impurity (Conwell-Weisskopf)" << G4endl;
  }
}

G4CMPPolaronScatteringRate::~G4CMPPolaronScatteringRate() {
  if (fAcousticRate) delete fAcousticRate;
  if (fOpticalRate) delete fOpticalRate;
  if (fImpurityRate) delete fImpurityRate;
}

// ============================================================================
// RATE CALCULATIONS
// ============================================================================

G4double G4CMPPolaronScatteringRate::AcousticRate(const G4Track& aTrack) const {
  /// Acoustic phonon scattering rate (Bardeen-Shockley)
  /// Γ_ac(E,T) = [√2·m*^(3/2)·E₁²·k_B·T / (π·ħ⁴·ρ·v_LA²)] · E^(-1/2)

  if (!fAcousticRate) return 0.;
  return fAcousticRate->Rate(aTrack);
}

G4double G4CMPPolaronScatteringRate::OpticalRate(const G4Track& aTrack) const {
  /// Optical phonon scattering rate (Fröhlich, Callen)
  /// Γ_opt(E,T) = Σ_s [π·α_F,s·ω_s·F(u)·(1+n_s)]
  /// where u = E/(ħ·ω_LO)

  if (!fOpticalRate) return 0.;
  return fOpticalRate->Rate(aTrack);
}

G4double G4CMPPolaronScatteringRate::ImpurityRate(const G4Track& aTrack) const {
  /// Impurity (Coulomb) scattering rate (Conwell-Weisskopf)
  /// Γ_imp(E) = [4√2·N_I·ħ²·√E] / [m*^(3/2)·(E + E_T)]
  /// where E_T = (3/4)·eV·(m*/m_e)/ε_r

  if (!fImpurityRate) return 0.;
  return fImpurityRate->Rate(aTrack);
}

G4double G4CMPPolaronScatteringRate::Rate(const G4Track& aTrack) const {
  /// Total scattering rate: Γ_total = Γ_acoustic + Γ_optical + Γ_impurity
  /// Mathiessen's rule (assumes independent scattering mechanisms)

  G4double Γ_ac = AcousticRate(aTrack);
  G4double Γ_opt = OpticalRate(aTrack);
  G4double Γ_imp = ImpurityRate(aTrack);
  G4double Γ_total = Γ_ac + Γ_opt + Γ_imp;

  if (fVerboseLevel > 1) {
    G4double Ekin = aTrack.GetKineticEnergy();
    G4cout << "Rate(" << Ekin / eV << " eV):"
           << " Γ_ac=" << Γ_ac / (1e12 * hertz) << "THz"
           << " Γ_opt=" << Γ_opt / (1e12 * hertz) << "THz"
           << " Γ_imp=" << Γ_imp / (1e12 * hertz) << "THz"
           << " Γ_total=" << Γ_total / (1e12 * hertz) << "THz" << G4endl;
  }

  return Γ_total;
}

// ============================================================================
// TRANSPORT PROPERTIES
// ============================================================================

G4double G4CMPPolaronScatteringRate::MeanFreePath(const G4Track& aTrack) const {
  /// Mean free path: λ = v_polaron / Γ_total

  G4double Γ_total = Rate(aTrack);

  if (Γ_total <= 0.) return 1e10 * meter;  // Large mean free path if no scattering

  // Get polaron velocity (rough estimates for now)
  // Can be refined with proper polaron velocity calculation later
  G4double v_polaron = 1e7 * m / s;  // ~100 km/s for electron polaron

  return v_polaron / Γ_total;
}

G4double G4CMPPolaronScatteringRate::RelaxationTime(const G4Track& aTrack) const {
  /// Relaxation time: τ = 1 / Γ_total

  G4double Γ_total = Rate(aTrack);

  if (Γ_total <= 0.) return 1e-6 * second;  // Large time if no scattering

  return 1.0 / Γ_total;
}

// ============================================================================
// SCATTERING MECHANISM SELECTION
// ============================================================================

G4int G4CMPPolaronScatteringRate::ChooseScatteringMechanism(const G4Track& aTrack) const {
  /// Sample which mechanism triggers: 0=acoustic, 1=optical, 2=impurity
  /// Weighted by relative rates using rejection method
 return 2; 
 //  G4double Γ_ac = AcousticRate(aTrack);
 //  G4double Γ_opt = OpticalRate(aTrack);
 //  G4double Γ_imp = ImpurityRate(aTrack);
 //  G4double Γ_total = Γ_ac + Γ_opt + Γ_imp;

 //  if (Γ_total <= 0.) return 0;  // Default to acoustic if no scattering

 //  G4double rand = G4UniformRand() * Γ_total;

 //  if (rand < Γ_ac) {
 //    return 0;  // Acoustic
 //  } else if (rand < (Γ_ac + Γ_opt)) {
 //    return 1;  // Optical
 //  } else {
 //    return 2;  // Impurity
 // }



}

G4String G4CMPPolaronScatteringRate::GetDominantMechanism(const G4Track& aTrack) const {
  /// Return name of dominant mechanism at this energy and temperature

  G4double Γ_ac = AcousticRate(aTrack);
  G4double Γ_opt = OpticalRate(aTrack);
  G4double Γ_imp = ImpurityRate(aTrack);

  G4double Γ_max = std::max({Γ_ac, Γ_opt, Γ_imp});

  if (Γ_max <= 0.) return "None";

  if (Γ_ac == Γ_max) {
    return "Acoustic";
  } else if (Γ_opt == Γ_max) {
    return "Optical";
  } else {
    return "Impurity";
  }
}

// ============================================================================
// MECHANISM-SPECIFIC HANDLERS
// ============================================================================

void G4CMPPolaronScatteringRate::HandleAcousticScattering(G4Track& aTrack) const {
  /// Handle acoustic scattering event
  /// - Inelastic: loses ~1 meV per scattering
  /// - Small-angle deflection (mostly forward)

  G4double Ekin_initial = aTrack.GetKineticEnergy();
  G4double E_loss = 0.001 * eV;  // Acoustic phonon energy (~1 meV)

  if (Ekin_initial <= E_loss) {
    // Polaron cannot lose more energy than it has
    aTrack.SetKineticEnergy(0.);
  } else {
    aTrack.SetKineticEnergy(Ekin_initial - E_loss);
  }

  // Small-angle deflection (cos θ = 2u - 1, u uniform)
  G4double u = G4UniformRand();
  G4double cos_theta = 2.0 * u - 1.0;
  ScatterDirection(aTrack, cos_theta);

  if (fVerboseLevel > 2) {
    G4cout << "  AcousticScattering: ΔE=" << E_loss / eV << " eV"
           << " cosθ=" << cos_theta << G4endl;
  }
}

void G4CMPPolaronScatteringRate::HandleOpticalScattering(G4Track& aTrack) const {
  /// Handle optical scattering event
  /// - Inelastic: loses ħω_opt (59.7 meV dominant mode)
  /// - Large-angle deflection (mostly isotropic)

  G4double Ekin_initial = aTrack.GetKineticEnergy();
  G4double E_loss = 0.0597 * eV;  // Dominant Eu-LO2 mode energy (~59.7 meV)

  if (Ekin_initial <= E_loss) {
    // Polaron cannot lose more energy than it has
    aTrack.SetKineticEnergy(0.);
  } else {
    aTrack.SetKineticEnergy(Ekin_initial - E_loss);
  }

  // Large-angle deflection (isotropic: cos θ = 2u - 1, u uniform)
  G4double u = G4UniformRand();
  G4double cos_theta = 2.0 * u - 1.0;
  ScatterDirection(aTrack, cos_theta);

  if (fVerboseLevel > 2) {
    G4cout << "  OpticalScattering: ΔE=" << E_loss / eV << " eV"
           << " cosθ=" << cos_theta << G4endl;
  }
}

void G4CMPPolaronScatteringRate::HandleImpurityScattering(G4Track& aTrack) const {
  /// Handle impurity scattering event
  /// - Elastic: no energy loss
  /// - Small-angle deflection (mostly forward, cos θ peaked at +1)

  // Elastic: kinetic energy unchanged
  // Coulomb deflection: forward-peaked (cos θ = 1 + 2u·ln(u), u uniform)

  G4double u = G4UniformRand();
  G4double ln_u = (u > 0.) ? std::log(u) : 0.;
  G4double cos_theta = 1.0 + 2.0 * u * ln_u;

  // Clamp to [-1, +1]
  if (cos_theta < -1.0) cos_theta = -1.0;
  if (cos_theta > +1.0) cos_theta = +1.0;

  ScatterDirection(aTrack, cos_theta);

  if (fVerboseLevel > 2) {
    G4cout << "  ImpurityScattering: Elastic"
           << " cosθ=" << cos_theta << G4endl;
  }
}

void G4CMPPolaronScatteringRate::ScatterDirection(
  G4Track& aTrack, G4double cos_theta) const {
  /// Apply scattering angle to track momentum
  /// cos_theta: cosine of scattering angle

  // const G4double pi = 3.14159265359;
  G4double theta = std::acos(std::min(1.0, std::max(-1.0, cos_theta)));
  G4double phi = 2.0 * pi * G4UniformRand();

  // Rotate the track's momentum direction
  G4ThreeVector old_dir = aTrack.GetMomentumDirection();

  // Create perpendicular vectors for rotation
  G4ThreeVector u = old_dir.unit();
  G4ThreeVector v = G4ThreeVector(
    -u.y(), u.x(), 0.0
  );
  if (v.mag() < 0.01) {
    v = G4ThreeVector(0.0, -u.z(), u.y());
  }
  v = v.unit();

  G4ThreeVector w = u.cross(v).unit();

  // New direction: rotate by (theta, phi)
  G4double sin_theta = std::sin(theta);
  G4ThreeVector new_dir = cos_theta * u + sin_theta * (std::cos(phi) * v + std::sin(phi) * w);
  new_dir = new_dir.unit();

  aTrack.SetMomentumDirection(new_dir);

  if (fVerboseLevel > 3) {
    G4cout << "  ScatterDirection: θ=" << theta / degree << "°"
           << " φ=" << phi / degree << "°" << G4endl;
  }
}

// ============================================================================
// HELPER METHODS
// ============================================================================

G4bool G4CMPPolaronScatteringRate::IsPolaronElectron(const G4Track& aTrack) const {
  return G4CMP::IsElectronPolaron(aTrack.GetParticleDefinition());
}

G4bool G4CMPPolaronScatteringRate::IsPolaronHole(const G4Track& aTrack) const {
  return G4CMP::IsHolePolaron(aTrack.GetParticleDefinition());
}

G4double G4CMPPolaronScatteringRate::GetPolaronMass(const G4Track& aTrack) const {
  /// Return polaron effective mass (in units of electron mass)

  if (IsPolaronElectron(aTrack)) {
    return 0.3;  // Electron polaron
  } else if (IsPolaronHole(aTrack)) {
    return 3.6;  // Hole polaron
  } else {
    return 1.0;  // Default (shouldn't happen)
  }
}

// ============================================================================
// DIAGNOSTICS
// ============================================================================

void G4CMPPolaronScatteringRate::SetVerboseLevel(G4int vb) {
  fVerboseLevel = vb;

  if (fAcousticRate) fAcousticRate->SetVerboseLevel(vb);
  if (fOpticalRate) fOpticalRate->SetVerboseLevel(vb);
  if (fImpurityRate) fImpurityRate->SetVerboseLevel(vb);

  if (fVerboseLevel > 0) {
    G4cout << "G4CMPPolaronScatteringRate::SetVerboseLevel(" << vb << ")" << G4endl;
  }
}
