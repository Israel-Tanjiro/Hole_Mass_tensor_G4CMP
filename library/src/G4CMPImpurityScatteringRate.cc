/***********************************************************************\
 * This software is licensed under the terms of the GNU General Public *
 * License version 3 or later. See G4CMP/LICENSE for the full license. *
\***********************************************************************/

/// \file library/src/G4CMPImpurityScatteringRate.cc
/// \brief Implementation of impurity (Coulomb) scattering rate

#include "G4CMPImpurityScatteringRate.hh"
#include "G4CMPUtils.hh"
#include "G4PhysicalConstants.hh"
#include "G4SystemOfUnits.hh"
#include "G4Track.hh"
#include <cmath>

// ══════════════════════════════════════════════════════════════════════════
// STATIC SAPPHIRE PARAMETERS
// ══════════════════════════════════════════════════════════════════════════

const G4double G4CMPImpurityScatteringRate::fN_impurity_Sapphire = 1e11 / (cm*cm*cm);  // 1e11 cm⁻³ = 1e17 m⁻³ (high-purity)
const G4double G4CMPImpurityScatteringRate::fEpsilon_r_Sapphire = 9.30;

// ══════════════════════════════════════════════════════════════════════════
// CONSTRUCTOR AND DESTRUCTOR
// ══════════════════════════════════════════════════════════════════════════

G4CMPImpurityScatteringRate::G4CMPImpurityScatteringRate(
  G4double m_eff_rel, G4double impurity_conc_cm3, G4double epsilon_r)
  : G4CMPVScatteringRate("ImpurityScattering"),
    fN_impurity(impurity_conc_cm3 * cm3),
    fEpsilon_r(epsilon_r),
    fVerboseLevel(0) {

  fMass_eff = m_eff_rel * electron_mass_c2 / c_squared;

  G4double m_rel = m_eff_rel;
  fE_T = 0.75 * eV * m_rel / epsilon_r;

  ComputePrefactor();

  if (fVerboseLevel > 0) {
    G4cout << "G4CMPImpurityScatteringRate constructed (parameterized)" << G4endl;
  }
}

// NOTE: A separate no-argument constructor is intentionally not defined here.
// The header only declares G4CMPImpurityScatteringRate(const G4String& aName
// = "ImpurityScattering"), which already serves as the default constructor
// (callable with zero arguments via its default argument). A distinct
// G4CMPImpurityScatteringRate() definition would not match any declaration
// and fails to compile ("does not match any declaration in class").

G4CMPImpurityScatteringRate::G4CMPImpurityScatteringRate(const G4String& aName)
  : G4CMPVScatteringRate(aName),
    fVerboseLevel(0) {

  InitializeSapphire();
  ComputePrefactor();

  if (fVerboseLevel > 0) {
    G4cout << "G4CMPImpurityScatteringRate constructed (Sapphire, hardcoded)" << G4endl;
  }
}

G4CMPImpurityScatteringRate::~G4CMPImpurityScatteringRate() {
}

// ══════════════════════════════════════════════════════════════════════════
// INITIALIZATION
// ══════════════════════════════════════════════════════════════════════════

void G4CMPImpurityScatteringRate::InitializeSapphire() {
  fN_impurity = fN_impurity_Sapphire;
  fEpsilon_r = fEpsilon_r_Sapphire;

  fMass_eff = 0.30 * electron_mass_c2 / c_squared;

  G4double m_rel = 0.30;
  fE_T = 0.75 * eV * m_rel / fEpsilon_r;
}

void G4CMPImpurityScatteringRate::ComputePrefactor() {
  // Brooks-Herring formula for ionized Coulomb impurities (sapphire-appropriate)
  // γ_imp(E) = N_i Z² e⁴ F(b) / (16π√2 (εs ε₀)² √m* · E^(3/2))
  // F(b) = ln(1+b) − b/(1+b)  is the Coulomb logarithm
  // This is a dimensionless prefactor; actual rate computed in RateAtEnergy()
  fPrefactor = (fN_impurity * electron_charge * electron_charge * electron_charge * electron_charge) /
               (16.0 * pi * std::sqrt(2.0) * fEpsilon_r * fEpsilon_r *
                epsilon0 * epsilon0 * std::sqrt(fMass_eff));
}

// ══════════════════════════════════════════════════════════════════════════
// SCATTERING RATE CALCULATIONS
// ══════════════════════════════════════════════════════════════════════════

G4double G4CMPImpurityScatteringRate::Rate(const G4Track& aTrack) const {
  G4double Ekin = aTrack.GetKineticEnergy();
  return RateAtEnergy(Ekin);
}

G4double G4CMPImpurityScatteringRate::RateAtEnergy(G4double Ekin_J) const {
  if (Ekin_J <= 0.) return 0.;

  // Brooks-Herring formula for ionized impurities (Shan et al., Heinz et al. 2003)
  // γ_imp(E) = [N_i Z² e⁴ F(b)] / [16π√2 (εs ε₀)² √m* · E^(3/2)]
  //
  // where:
  //   F(b) = ln(1+b) − b/(1+b)    (Coulomb logarithm)
  //   b = 8 m* E / (ħ κ_TF)²       (screening parameter)
  //   κ_TF² = n e² / (εs ε₀ E_F)   (Thomas-Fermi wavevector)
  //   E_F = ħ²(3π²n)^(2/3)/(2m*)   (Fermi energy)

  // Carrier gas density for screening (photo-excited electrons at threshold, typical)
  static const G4double N_CARRIER = 1e20 / (m*m*m);  // 1e20 m⁻³

  // Compute Fermi energy
  G4double E_F = (hbar_Planck * hbar_Planck / (2.0 * fMass_eff)) *
                 std::pow(3.0 * pi * pi * N_CARRIER, 2.0/3.0);

  if (E_F <= 0.) return 0.;

  // Thomas-Fermi screening: κ_TF² = n e² / (εs ε₀ E_F)
  G4double kap_sq = (N_CARRIER * electron_charge * electron_charge) /
                    (fEpsilon_r * epsilon0 * E_F);

  if (kap_sq <= 0.) return 0.;

  // Screening parameter: b = 8 m* E / (ħ κ_TF)²
  G4double b = std::max(8.0 * fMass_eff * Ekin_J / (hbar_Planck * hbar_Planck * kap_sq), 1e-10);

  // Coulomb logarithm: F(b) = ln(1+b) − b/(1+b)
  G4double F_b = std::max(std::log(1.0 + b) - b / (1.0 + b), 1e-10);

  // Energy^(-3/2) dependence (characteristic of Brooks-Herring)
  G4double E_power = std::pow(Ekin_J, -1.5);

  return fPrefactor * F_b * E_power;
}

// NOTE: SetVerboseLevel is already defined inline in the header
// (virtual void SetVerboseLevel(G4int vb) { fVerboseLevel = vb; }).
// A second out-of-line definition here is a redefinition error.
