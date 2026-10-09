/***********************************************************************\
 * This software is licensed under the terms of the GNU General Public *
 * License version 3 or later. See G4CMP/LICENSE for the full license. *
\***********************************************************************/

/// \file library/src/G4CMPOpticalScatteringRate.cc
/// \brief Implementation of optical phonon scattering rate

#include "G4CMPOpticalScatteringRate.hh"
#include "G4CMPUtils.hh"
#include "G4PhysicalConstants.hh"
#include "G4SystemOfUnits.hh"
#include "G4Track.hh"
#include <cmath>
#include <algorithm>

// ══════════════════════════════════════════════════════════════════════════
// STATIC SAPPHIRE LO MODE DATA (Heinz et al. PRL 90, 247401 2003)
// ══════════════════════════════════════════════════════════════════════════

const G4int G4CMPOpticalScatteringRate::fNModes_Sapphire = 6;

const G4double G4CMPOpticalScatteringRate::fHW_LO_Sapphire[] = {
  48.0e-3,    // Eu-LO1   48 meV
  59.7e-3,    // Eu-LO2   60 meV (dominant)
  63.3e-3,    // A2u-LO1  63 meV
  78.1e-3,    // Eu-LO3   78 meV
  109.0e-3,   // A2u-LO2  109 meV
  112.0e-3    // Eu-LO4   112 meV
};

const G4double G4CMPOpticalScatteringRate::fAlpha_F_Sapphire[] = {
  0.0261,     // Eu-LO1
  0.5203,     // Eu-LO2 (dominant)
  0.0038,     // A2u-LO1
  0.0053,     // Eu-LO3
  0.3526,     // A2u-LO2
  0.4521      // Eu-LO4
};

// ══════════════════════════════════════════════════════════════════════════
// CONSTRUCTOR AND DESTRUCTOR
// ══════════════════════════════════════════════════════════════════════════

G4CMPOpticalScatteringRate::G4CMPOpticalScatteringRate(
  std::vector<G4double> hw_LO_eV,
  std::vector<G4double> alpha_F_vals,
  G4double m_eff_rel)
  : G4CMPVScatteringRate("OpticalScattering"),
    fNModes(hw_LO_eV.size()),
    fVerboseLevel(0) {

  for (size_t i = 0; i < hw_LO_eV.size(); ++i) {
    fHW_LO.push_back(hw_LO_eV[i] * eV);
    fAlpha_F.push_back(alpha_F_vals[i]);
  }

  fMass_eff = m_eff_rel * electron_mass_c2 / c_squared;

  if (fVerboseLevel > 0) {
    G4cout << "G4CMPOpticalScatteringRate constructed (parameterized)" << G4endl;
  }
}

// NOTE: A separate no-argument constructor is intentionally not defined here.
// The header only declares G4CMPOpticalScatteringRate(const G4String& aName
// = "OpticalScattering"), which already serves as the default constructor
// (callable with zero arguments via its default argument). A distinct
// G4CMPOpticalScatteringRate() definition would not match any declaration
// and fails to compile ("does not match any declaration in class").

G4CMPOpticalScatteringRate::G4CMPOpticalScatteringRate(const G4String& aName)
  : G4CMPVScatteringRate(aName),
    fNModes(fNModes_Sapphire),
    fVerboseLevel(0) {

  InitializeSapphire();

  if (fVerboseLevel > 0) {
    G4cout << "G4CMPOpticalScatteringRate constructed (Sapphire, hardcoded)" << G4endl;
  }
}

G4CMPOpticalScatteringRate::~G4CMPOpticalScatteringRate() {
}

// ══════════════════════════════════════════════════════════════════════════
// INITIALIZATION
// ══════════════════════════════════════════════════════════════════════════

void G4CMPOpticalScatteringRate::InitializeSapphire() {
  fHW_LO.clear();
  fAlpha_F.clear();
  fMass_eff = 0.30 * electron_mass_c2 / c_squared;

  for (G4int i = 0; i < fNModes; ++i) {
    fHW_LO.push_back(fHW_LO_Sapphire[i] * eV);
    fAlpha_F.push_back(fAlpha_F_Sapphire[i]);
  }
}

// ══════════════════════════════════════════════════════════════════════════
// SCATTERING RATE CALCULATIONS
// ══════════════════════════════════════════════════════════════════════════

G4double G4CMPOpticalScatteringRate::Rate(const G4Track& aTrack) const {
  G4double Ekin = aTrack.GetKineticEnergy();
  if (Ekin <= 0.) return 0.;

  G4double T = 1.0 * kelvin;  // Cryogenic: low-T polaron dynamics, emission dominates

  G4double rate_total = 0.;
  for (G4int i = 0; i < fNModes; ++i) {
    rate_total += ModeRate(i, Ekin, T);
  }

  return rate_total;
}

G4double G4CMPOpticalScatteringRate::ModeRate(
  G4int mode_index, G4double Ekin_J, G4double T) const {

  if (mode_index < 0 || mode_index >= fNModes) return 0.;
  if (Ekin_J < fHW_LO[mode_index] || T <= 0.) return 0.;

  G4double hw = fHW_LO[mode_index];
  G4double alpha_F = fAlpha_F[mode_index];
  G4double omega = hw / hbar_Planck;

  G4double u = Ekin_J / hw;
  if (u <= 1.0) return 0.;

  G4double F_val = CallenF(u);
  G4double n_B = BosePopulation(hw, T);

  return pi * alpha_F * omega * F_val * (1.0 + n_B);
}

// NOTE: declared "static" in the header, so no "const" qualifier here
// (and none is needed -- this is a pure function of its arguments).
G4double G4CMPOpticalScatteringRate::CallenF(G4double u) {
  if (u <= 1.0) return 0.;

  G4double sqrt_u = std::sqrt(u);
  G4double sqrt_um1 = std::sqrt(u - 1.0);
  G4double ln_arg = sqrt_u + sqrt_um1;

  if (ln_arg <= 0.) return 0.;
  return (2.0 / sqrt_u) * std::log(ln_arg);
}

// NOTE: declared "static" in the header, so no "const" qualifier here.
G4double G4CMPOpticalScatteringRate::BosePopulation(G4double hw, G4double T) {
  if (T <= 0.) return 0.;

  G4double exponent = hw / (k_Boltzmann * T);

  if (exponent > 100.) return 0.;

  return 1.0 / (std::exp(exponent) - 1.0);
}

// ══════════════════════════════════════════════════════════════════════════
// ACCESSORS
// ══════════════════════════════════════════════════════════════════════════

G4double G4CMPOpticalScatteringRate::GetModeFrequency(G4int mode_index) const {
  if (mode_index < 0 || mode_index >= fNModes) return 0.;
  return fHW_LO[mode_index];
}

G4double G4CMPOpticalScatteringRate::GetModeCoupling(G4int mode_index) const {
  if (mode_index < 0 || mode_index >= fNModes) return 0.;
  return fAlpha_F[mode_index];
}

G4double G4CMPOpticalScatteringRate::Threshold(G4double) const {
  if (fHW_LO.empty()) return 0.;
  return *std::min_element(fHW_LO.begin(), fHW_LO.end());
}

// NOTE: SetVerboseLevel is already defined inline in the header
// (virtual void SetVerboseLevel(G4int vb) { fVerboseLevel = vb; }).
// A second out-of-line definition here is a redefinition error.
