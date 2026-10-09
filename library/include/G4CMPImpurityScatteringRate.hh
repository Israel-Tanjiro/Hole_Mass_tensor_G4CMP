/***********************************************************************\
 * This software is licensed under the terms of the GNU General Public *
 * License version 3 or later. See G4CMP/LICENSE for the full license. *
\***********************************************************************/

/// \file library/include/G4CMPImpurityScatteringRate.hh
/// \brief Impurity (Coulomb) scattering rate model (Conwell-Weisskopf)
///
/// Implements elastic impurity scattering for electrons, holes, and polarons
/// Pure Coulomb deflection (no energy loss), temperature-independent
/// Formula: γ_imp(E) = [4√2·N_I·ℏ²·√E] / [m*^(3/2)·(E + E_T)]
/// where E_T = (3/4)·eV·(m*/m_e)/ε_r (Debye-Hückel screening energy)
/// References: Conwell & Weisskopf 1950; Jacoboni & Reggiani 1983

#ifndef G4CMPImpurityScatteringRate_hh
#define G4CMPImpurityScatteringRate_hh

#include "G4CMPVScatteringRate.hh"

class G4CMPImpurityScatteringRate : public G4CMPVScatteringRate {
public:
  /// Constructor with all parameters (for Option B: parameterized materials)
  G4CMPImpurityScatteringRate(
    G4double m_eff_eV,           ///< Effective mass in units of m_e (e.g., 0.30)
    G4double impurity_conc_cm3,  ///< Impurity concentration (cm⁻³)
    G4double epsilon_r           ///< Relative permittivity
  );

  /// Constructor with material name (for Option A: hardcoded Sapphire)
  G4CMPImpurityScatteringRate(const G4String& aName = "ImpurityScattering");

  virtual ~G4CMPImpurityScatteringRate();

  /// Calculate scattering rate (energy-dependent, temperature-independent)
  /// γ_imp(E) = [4√2·N_I·ℏ²·√E] / [m*^(3/2)·(E + E_T)]
  virtual G4double Rate(const G4Track& aTrack) const;

  /// Rate at given energy
  G4double RateAtEnergy(G4double Ekin_J) const;

  /// Debye-Hückel screening energy
  /// E_T = (3/4)·eV·(m*/m_e) / ε_r
  G4double ScreeningEnergy() const { return fE_T; }

  /// Get effective mass
  G4double GetEffectiveMass() const { return fMass_eff; }

  /// Get impurity concentration
  G4double GetImpurityConcentration() const { return fN_impurity; }

  /// Get relative permittivity
  G4double GetPermittivity() const { return fEpsilon_r; }

  /// No threshold for impurity scattering
  virtual G4double Threshold(G4double) const { return 0.; }

  /// Set verbose level
  virtual void SetVerboseLevel(G4int vb) { fVerboseLevel = vb; }

private:
  // ── Sapphire material parameters (hardcoded for Option A) ───────────────
  static const G4double fN_impurity_Sapphire;    // cm⁻³
  static const G4double fEpsilon_r_Sapphire;

  // ── Member variables ──────────────────────────────────────────────────
  G4double fMass_eff;           // Effective mass (kg) = m_eff_relative * m_e
  G4double fN_impurity;         // Impurity concentration (m⁻³)
  G4double fEpsilon_r;          // Relative permittivity (dimensionless)
  G4double fE_T;                // Screening energy (J)
  G4double fPrefactor;          // Pre-computed 4√2·N_I·ℏ² / m*^(3/2)

  G4int fVerboseLevel;

  // ── Helper methods ────────────────────────────────────────────────────
  void ComputePrefactor();      // Calculate pre-computed terms
  void InitializeSapphire();    // Set hardcoded Sapphire parameters
};

#endif  // G4CMPImpurityScatteringRate_hh
