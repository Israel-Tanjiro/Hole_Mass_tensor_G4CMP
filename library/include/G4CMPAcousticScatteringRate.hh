/***********************************************************************\
 * This software is licensed under the terms of the GNU General Public *
 * License version 3 or later. See G4CMP/LICENSE for the full license. *
\***********************************************************************/

/// \file library/include/G4CMPAcousticScatteringRate.hh
/// \brief Acoustic phonon scattering rate model (Bardeen-Shockley)
///
/// Implements acoustic phonon scattering for electrons, holes, and polarons
/// Formula: γ_ac(E,T) = β·T·√E where β = √2·m*^(3/2)·E₁²/(π·ħ⁴·ρ·v_LA²)
/// References: Bardeen & Shockley 1950; Jacoboni & Reggiani 1983; Heinz et al. 2003

#ifndef G4CMPAcousticScatteringRate_hh
#define G4CMPAcousticScatteringRate_hh

#include "G4CMPVScatteringRate.hh"

class G4CMPAcousticScatteringRate : public G4CMPVScatteringRate {
public:
  /// Constructor with all parameters (for Option B: parameterized materials)
  G4CMPAcousticScatteringRate(
    G4double m_eff_rel,           ///< Effective mass in units of m_e (e.g., 0.30)
    G4double E1_eV,               ///< Deformation potential (eV)
    G4double density,             ///< Mass density (kg/m³)
    G4double sound_vel            ///< Sound velocity (m/s)
  );

  /// Constructor with material name (for Option A: hardcoded Sapphire)
  G4CMPAcousticScatteringRate(const G4String& aName = "AcousticScattering");

  virtual ~G4CMPAcousticScatteringRate();

  /// Calculate scattering rate (energy and temperature dependent)
  /// γ_ac(E,T) = β·T·√E where β = √2·m*^(3/2)·E₁²/(π·ħ⁴·ρ·v_LA²)
  virtual G4double Rate(const G4Track& aTrack) const;

  /// Rate at given energy and temperature
  G4double RateAtEnergy(G4double Ekin_J, G4double T) const;

  /// Temperature-averaged rate (rough estimate)
  G4double AverageRate(G4double T) const;

  /// No threshold for acoustic scattering
  virtual G4double Threshold(G4double) const { return 0.; }

  /// Set verbose level
  virtual void SetVerboseLevel(G4int vb) { fVerboseLevel = vb; }

private:
  // ── Sapphire material parameters (hardcoded for Option A) ───────────────
  static const G4double fE1_Deform_Electron_Sapphire;    // eV
  static const G4double fE1_Deform_Hole_Sapphire;        // eV
  static const G4double fDensity_Sapphire;               // kg/m³
  static const G4double fSoundVelocity_Sapphire;         // m/s

  // ── Member variables ──────────────────────────────────────────────────
  G4double fMass_eff;           // Effective mass (kg) = m_eff_relative * m_e
  G4double fE_deform;           // Deformation potential (J)
  G4double fDensity;            // Mass density (kg/m³)
  G4double fSoundVel;           // Sound velocity (m/s)
  G4double fPrefactor;          // Pre-computed β factor

  G4int fVerboseLevel;

  // ── Helper methods ────────────────────────────────────────────────────
  void ComputePrefactor();      // Calculate pre-computed terms
  void InitializeSapphire();    // Set hardcoded Sapphire parameters
};

#endif  // G4CMPAcousticScatteringRate_hh
