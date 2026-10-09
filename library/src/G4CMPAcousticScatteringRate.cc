/***********************************************************************\
 * This software is licensed under the terms of the GNU General Public *
 * License version 3 or later. See G4CMP/LICENSE for the full license. *
\***********************************************************************/

/// \file library/src/G4CMPAcousticScatteringRate.cc
/// \brief Implementation of acoustic phonon scattering rate

#include "G4CMPAcousticScatteringRate.hh"
#include "G4CMPUtils.hh"
#include "G4PhysicalConstants.hh"
#include "G4SystemOfUnits.hh"
#include "G4Track.hh"
#include <cmath>

// ══════════════════════════════════════════════════════════════════════════
// STATIC SAPPHIRE PARAMETERS
// ══════════════════════════════════════════════════════════════════════════

const G4double G4CMPAcousticScatteringRate::fE1_Deform_Electron_Sapphire = 19.0 * eV;
const G4double G4CMPAcousticScatteringRate::fE1_Deform_Hole_Sapphire = 4.0 * eV;
const G4double G4CMPAcousticScatteringRate::fDensity_Sapphire = 3987.0 * kg / m3;
const G4double G4CMPAcousticScatteringRate::fSoundVelocity_Sapphire = 11220.0 * m / second;

// ══════════════════════════════════════════════════════════════════════════
// CONSTRUCTOR AND DESTRUCTOR
// ══════════════════════════════════════════════════════════════════════════

G4CMPAcousticScatteringRate::G4CMPAcousticScatteringRate(
  G4double m_eff_rel, G4double E1_eV, G4double density, G4double sound_vel)
  : G4CMPVScatteringRate("AcousticScattering"),
    fVerboseLevel(0) {

  fMass_eff = m_eff_rel * electron_mass_c2 / c_squared;
  fE_deform = E1_eV * eV;
  fDensity = density * kg / m3;
  fSoundVel = sound_vel * m / second;

  ComputePrefactor();

  if (fVerboseLevel > 0) {
    G4cout << "G4CMPAcousticScatteringRate constructed (parameterized)" << G4endl;
  }
}

// NOTE: A separate no-argument constructor is intentionally not defined here.
// The header only declares G4CMPAcousticScatteringRate(const G4String& aName
// = "AcousticScattering"), which already serves as the default constructor
// (callable with zero arguments via its default argument). A distinct
// G4CMPAcousticScatteringRate() definition would not match any declaration
// and fails to compile ("does not match any declaration in class").

G4CMPAcousticScatteringRate::G4CMPAcousticScatteringRate(const G4String& aName)
  : G4CMPVScatteringRate(aName),
    fVerboseLevel(0) {

  InitializeSapphire();
  ComputePrefactor();

  if (fVerboseLevel > 0) {
    G4cout << "G4CMPAcousticScatteringRate constructed (Sapphire, hardcoded)" << G4endl;
  }
}

G4CMPAcousticScatteringRate::~G4CMPAcousticScatteringRate() {
}

// ══════════════════════════════════════════════════════════════════════════
// INITIALIZATION
// ══════════════════════════════════════════════════════════════════════════

void G4CMPAcousticScatteringRate::InitializeSapphire() {
  fMass_eff = 0.30 * electron_mass_c2 / c_squared;
  fE_deform = fE1_Deform_Electron_Sapphire;
  fDensity = fDensity_Sapphire;
  fSoundVel = fSoundVelocity_Sapphire;
}

void G4CMPAcousticScatteringRate::ComputePrefactor() {
  G4double sqrt_2 = std::sqrt(2.0);
  G4double m_factor = std::pow(fMass_eff, 1.5);
  // Use E1 in eV units for proper unit cancellation
  G4double E1_eV_sq = (fE_deform / eV) * (fE_deform / eV);
  // Use ħ in eV·s units: hbar_Planck/eV gives ħ in eV·s
  G4double hbar_eVs = hbar_Planck / eV;  // Convert to eV·s
  G4double h4_eVs = hbar_eVs * hbar_eVs * hbar_eVs * hbar_eVs;
  G4double v_sq = fSoundVel * fSoundVel;

  fPrefactor = (sqrt_2 * m_factor * E1_eV_sq) / (pi * h4_eVs * fDensity * v_sq);
}

// ══════════════════════════════════════════════════════════════════════════
// SCATTERING RATE CALCULATIONS
// ══════════════════════════════════════════════════════════════════════════

G4double G4CMPAcousticScatteringRate::Rate(const G4Track& aTrack) const {
  G4double Ekin = aTrack.GetKineticEnergy();
  G4double T = 1.0 * kelvin;  // Cryogenic: low-T polaron dynamics, emission dominates
  return RateAtEnergy(Ekin, T);
}

G4double G4CMPAcousticScatteringRate::RateAtEnergy(G4double Ekin_J, G4double T) const {
  if (Ekin_J <= 0. || T <= 0.) return 0.;

  G4double sqrt_E = std::sqrt(Ekin_J);
  return fPrefactor * k_Boltzmann * T / sqrt_E;  // ← E^(-1/2), not E^(+1/2)!
}

G4double G4CMPAcousticScatteringRate::AverageRate(G4double T) const {
  if (T <= 0.) return 0.;

  G4double T_factor = std::pow(T / (300.0 * kelvin), 1.5);
  return fPrefactor * k_Boltzmann * T * T_factor;
}

// NOTE: SetVerboseLevel is already defined inline in the header
// (virtual void SetVerboseLevel(G4int vb) { fVerboseLevel = vb; }).
// A second out-of-line definition here is a redefinition error.
