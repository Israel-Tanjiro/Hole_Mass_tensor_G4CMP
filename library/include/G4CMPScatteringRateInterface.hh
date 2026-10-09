#ifndef G4CMPScatteringRateInterface_h
#define G4CMPScatteringRateInterface_h 1

#include "G4Track.hh"
#include "G4ParticleDefinition.hh"

/***********************************************************************\
 * Universal Scattering Rate Interface
 *
 * Provides consistent energy-dependent scattering rate calculations
 * for both bare carriers (electrons/holes) and large polarons.
 *
 * All rates are in SI units: 1/seconds
 * Energy input is in eV
 ***********************************************************************/

class G4CMPScatteringRateInterface {
public:
  virtual ~G4CMPScatteringRateInterface() = default;

  // Core method: compute scattering rate at given energy
  virtual G4double GetScatteringRate(G4double energy_eV,
                                     G4double temperature_K,
                                     const G4ParticleDefinition* particle,
                                     G4bool isPolaron = false) const = 0;

  // Convenience: compute mean free time (inverse of rate)
  G4double GetMeanFreeTime(G4double energy_eV,
                          G4double temperature_K,
                          const G4ParticleDefinition* particle,
                          G4bool isPolaron = false) const {
    G4double rate = GetScatteringRate(energy_eV, temperature_K, particle, isPolaron);
    return (rate > 0) ? 1.0 / rate : 1e99;
  }

  // Get scattering mechanism name for logging
  virtual const char* GetName() const = 0;
};

/***********************************************************************\
 * Acoustic Phonon Scattering Rate (Bardeen-Shockley)
 *
 * Formula (field-driven, energy-dependent):
 * γ_ac = (√2·m*^(3/2)·E_ac^2·k_B·T) / (π·ℏ^4·ρ·v_LA^2) · √E
 *
 * Physical interpretation:
 * - Scales as (m*)^(3/2): heavier carriers scatter more
 * - Scales as √E: higher energy carriers scatter more
 * - Temperature-dependent: more phonons available at high T
 * - Dominates at low T (<200K for sapphire)
 ***********************************************************************/

class G4CMPAcousticScatteringRate : public G4CMPScatteringRateInterface {
public:
  G4CMPAcousticScatteringRate();
  ~G4CMPAcousticScatteringRate();

  G4double GetScatteringRate(G4double energy_eV,
                             G4double temperature_K,
                             const G4ParticleDefinition* particle,
                             G4bool isPolaron = false) const override;

  const char* GetName() const override { return "AcousticScattering"; }

  // Material constants (sapphire)
  static constexpr G4double DEFORM_POTENTIAL_E = 8.0;  // eV (electrons)
  static constexpr G4double DEFORM_POTENTIAL_H = 12.0; // eV (holes)
  static constexpr G4double DENSITY = 3980.0;          // kg/m^3
  static constexpr G4double SOUND_VELOCITY = 11400.0;  // m/s (LA phonon)

private:
  G4double GetEffectiveMass(const G4ParticleDefinition* particle,
                            G4bool isPolaron) const;
};

/***********************************************************************\
 * Optical (LO-Phonon) Scattering Rate (Fröhlich)
 *
 * Formula (field-driven, energy-dependent):
 * γ_opt = π·α_F·ω_LO·F(u)·(1+n_B)
 *
 * where:
 * - α_F: Fröhlich coupling constant
 * - ω_LO: LO phonon frequency (60 meV for sapphire)
 * - F(u) = (2/√u)·ln(√u + √(u-1)) [Callen function]
 * - u = E / ℏω_LO (reduced energy)
 * - n_B = 1/(exp(ℏω_LO/k_B·T) - 1) [Bose population]
 * - Only active when E > ℏω_LO (threshold behavior)
 *
 * Physical interpretation:
 * - Becomes dominant at high T (>200K for sapphire)
 * - Sharp threshold at E = ℏω_LO
 * - Form factor F(u) rises smoothly above threshold
 ***********************************************************************/

class G4CMPOpticalScatteringRate : public G4CMPScatteringRateInterface {
public:
  G4CMPOpticalScatteringRate();
  ~G4CMPOpticalScatteringRate();

  G4double GetScatteringRate(G4double energy_eV,
                             G4double temperature_K,
                             const G4ParticleDefinition* particle,
                             G4bool isPolaron = false) const override;

  const char* GetName() const override { return "OpticalScattering"; }

  // Material constants (sapphire - 6 LO modes)
  static constexpr G4int N_LO_MODES = 6;
  static constexpr G4double HW_LO[N_LO_MODES] = {
    0.0480, 0.0502, 0.0535, 0.0546, 0.0563, 0.112  // eV
  };
  static constexpr G4double COUPLING[N_LO_MODES] = {
    0.15, 0.12, 0.18, 0.14, 0.16, 0.22  // Fröhlich coupling α_F
  };

private:
  G4double CallenFunction(G4double u) const;  // F(u) for form factor
};

/***********************************************************************\
 * Impurity Scattering Rate (Coulomb)
 *
 * Formula (Ionized impurity scattering):
 * γ_imp = (m*^(1/2)·k_B·T·N_imp·e^4) / (16·π^2·ε_0^2·κ^2·ℏ^3·E) · H(u)
 *
 * where:
 * - N_imp: impurity concentration
 * - κ: dielectric constant (10.2 for sapphire)
 * - H(u): Conwell-Weisskopf screening function
 * - Scales as 1/E: dominates at low energy
 *
 * Physical interpretation:
 * - Weakly temperature-dependent
 * - Energy-dependent screening effects
 * - Can dominate at low T if impurity concentration is high
 ***********************************************************************/

class G4CMPImpurityScatteringRate : public G4CMPScatteringRateInterface {
public:
  G4CMPImpurityScatteringRate(G4double impurity_concentration = 1e15);
  ~G4CMPImpurityScatteringRate();

  G4double GetScatteringRate(G4double energy_eV,
                             G4double temperature_K,
                             const G4ParticleDefinition* particle,
                             G4bool isPolaron = false) const override;

  const char* GetName() const override { return "ImpurityScattering"; }

  void SetImpurityConcentration(G4double N_imp) { fImpurityConc = N_imp; }
  G4double GetImpurityConcentration() const { return fImpurityConc; }

  // Material constants (sapphire)
  static constexpr G4double DIELECTRIC_CONSTANT = 10.2;

private:
  G4double ConwellWeiszkopfFunction(G4double E_eV, G4double T_K) const;

  G4double fImpurityConc;  // m^-3
};

#endif
