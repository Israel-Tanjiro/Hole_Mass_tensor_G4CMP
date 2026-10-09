#include "G4CMPScatteringRateInterface.hh"
#include "G4CMPDriftElectron.hh"
#include "G4CMPDriftHole.hh"
#include "G4LatticeLogical.hh"
#include "G4CMPTrackUtils.hh"
#include "G4PhysicalConstants.hh"
#include "G4SystemOfUnits.hh"
#include <cmath>

// ══════════════════════════════════════════════════════════════════════
// ACOUSTIC SCATTERING RATE IMPLEMENTATION
// ══════════════════════════════════════════════════════════════════════

G4CMPAcousticScatteringRate::G4CMPAcousticScatteringRate() {}

G4CMPAcousticScatteringRate::~G4CMPAcousticScatteringRate() {}

G4double G4CMPAcousticScatteringRate::GetScatteringRate(
    G4double energy_eV,
    G4double temperature_K,
    const G4ParticleDefinition* particle,
    G4bool isPolaron) const {

  // Get effective mass (enhanced if polaron)
  G4double m_star = GetEffectiveMass(particle, isPolaron);

  // Get deformation potential (material dependent)
  G4double E_ac = (particle == G4CMPDriftElectron::Definition()) ?
                   DEFORM_POTENTIAL_E : DEFORM_POTENTIAL_H;

  // Energy in Joules
  G4double E_joules = energy_eV * eV;

  // Temperature factor
  G4double kT = k_Boltzmann * temperature_K;

  // Formula: γ_ac = (√2·m*^(3/2)·E_ac^2·k_B·T) / (π·ℏ^4·ρ·v_s^2) · √E
  G4double numerator = std::sqrt(2.0) * std::pow(m_star, 1.5) *
                       E_ac * E_ac * kT * std::sqrt(E_joules);

  G4double denominator = pi * std::pow(hbar_Planck, 4) * (DENSITY * kg/m3) *
                        std::pow(SOUND_VELOCITY * m/s, 2);

  G4double rate = numerator / denominator;

  return rate;
}

G4double G4CMPAcousticScatteringRate::GetEffectiveMass(
    const G4ParticleDefinition* particle,
    G4bool isPolaron) const {

  G4bool isElectron = (particle == G4CMPDriftElectron::Definition());

  G4double m_bare = isElectron ? 0.3 * electron_mass_c2 : 3.6 * electron_mass_c2;

  // If polaron, apply enhancement factor (1 + α/6)
  if (isPolaron) {
    G4double alpha = isElectron ? 1.2 : 1.2;
    G4double enhancement = 1.0 + alpha / 6.0;
    return m_bare * enhancement;
  }

  return m_bare;
}

// ══════════════════════════════════════════════════════════════════════
// OPTICAL SCATTERING RATE IMPLEMENTATION
// ══════════════════════════════════════════════════════════════════════

G4CMPOpticalScatteringRate::G4CMPOpticalScatteringRate() {}

G4CMPOpticalScatteringRate::~G4CMPOpticalScatteringRate() {}

G4double G4CMPOpticalScatteringRate::CallenFunction(G4double u) const {
  if (u <= 1.0) return 0.0;  // Below threshold, no scattering

  // F(u) = (2/√u)·ln(√u + √(u-1))
  G4double sqrt_u = std::sqrt(u);
  G4double sqrt_u_minus_1 = std::sqrt(u - 1.0);
  G4double log_arg = sqrt_u + sqrt_u_minus_1;

  return (2.0 / sqrt_u) * std::log(log_arg);
}

G4double G4CMPOpticalScatteringRate::GetScatteringRate(
    G4double energy_eV,
    G4double temperature_K,
    const G4ParticleDefinition* particle,
    G4bool isPolaron) const {

  G4double total_rate = 0.0;

  // Sum over all LO phonon modes
  for (int i = 0; i < N_LO_MODES; ++i) {
    G4double hw_lo = HW_LO[i];  // eV
    G4double alpha_F = COUPLING[i];

    // Threshold: only scatter if E > ℏω_LO
    if (energy_eV < hw_lo) continue;

    // Reduced energy parameter
    G4double u = energy_eV / hw_lo;

    // Bose population of LO phonon
    G4double exp_arg = (hw_lo * eV) / (k_Boltzmann * temperature_K);
    G4double n_B = 1.0 / (std::exp(std::min(exp_arg, 100.0)) - 1.0);

    // Callen function (form factor)
    G4double F_u = CallenFunction(u);

    // Scattering rate for this mode
    // γ_opt = π·α_F·ω_LO·F(u)·(1+n_B)
    G4double rate_mode = pi * alpha_F * (hw_lo * eV) * F_u * (1.0 + n_B);

    total_rate += rate_mode;
  }

  return total_rate;
}

// ══════════════════════════════════════════════════════════════════════
// IMPURITY SCATTERING RATE IMPLEMENTATION
// ══════════════════════════════════════════════════════════════════════

G4CMPImpurityScatteringRate::G4CMPImpurityScatteringRate(
    G4double impurity_concentration)
  : fImpurityConc(impurity_concentration) {}

G4CMPImpurityScatteringRate::~G4CMPImpurityScatteringRate() {}

G4double G4CMPImpurityScatteringRate::ConwellWeiszkopfFunction(
    G4double E_eV,
    G4double T_K) const {

  // Screening length parameter
  // u = 24·π·ε_0·κ·k_B·T / (e^2·N_imp)
  G4double numerator = 24 * pi * epsilon0 * DIELECTRIC_CONSTANT *
                       k_Boltzmann * T_K;
  G4double denominator = e_squared * fImpurityConc;
  G4double u = numerator / denominator;

  // Conwell-Weisskopf screening function (Coulomb scattering)
  // For simplicity: H(u) ≈ ln(1 + u) / u
  G4double H_u = (u > 0) ? std::log(1.0 + u) / u : 1.0;

  return H_u;
}

G4double G4CMPImpurityScatteringRate::GetScatteringRate(
    G4double energy_eV,
    G4double temperature_K,
    const G4ParticleDefinition* particle,
    G4bool isPolaron) const {

  if (energy_eV <= 0.0) return 0.0;
  if (fImpurityConc <= 0.0) return 0.0;

  // Get effective mass (enhanced if polaron)
  G4bool isElectron = (particle == G4CMPDriftElectron::Definition());
  G4double m_bare = isElectron ? 0.3 * electron_mass_c2 : 3.6 * electron_mass_c2;

  G4double m_star = m_bare;
  if (isPolaron) {
    G4double alpha = isElectron ? 1.2 : 1.2;
    G4double enhancement = 1.0 + alpha / 6.0;
    m_star = m_bare * enhancement;
  }

  // Energy in Joules
  G4double E_joules = energy_eV * eV;

  // Screening function
  G4double H_u = ConwellWeiszkopfFunction(energy_eV, temperature_K);

  // Formula: γ_imp = (m*^(1/2)·k_B·T·N_imp·e^4) / (16·π^2·ε_0^2·κ^2·ℏ^3·E) · H(u)
  G4double numerator = std::sqrt(m_star) * k_Boltzmann * temperature_K *
                       fImpurityConc * e_squared * e_squared * H_u;

  G4double denominator = 16.0 * pi * pi * epsilon0 * epsilon0 *
                        DIELECTRIC_CONSTANT * DIELECTRIC_CONSTANT *
                        std::pow(hbar_Planck, 3) * E_joules;

  G4double rate = numerator / denominator;

  return rate;
}

