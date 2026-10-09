/***********************************************************************\
 * This software is licensed under the terms of the GNU General Public *
 * License version 3 or later. See G4CMP/LICENSE for the full license. *
\***********************************************************************/

/// \file library/include/G4CMPOpticalScatteringRate.hh
/// \brief Optical phonon scattering rate model (Fröhlich 1954)
///
/// Implements optical (LO) phonon scattering for electrons, holes, and polarons
/// Supports multiple LO modes with Fröhlich coupling constants
/// Formula (Callen form): γ_opt(E,T) = π·α_F·ω_s·F(u)
/// where u = E/(ℏ·ω_s), F(u) = (1/√u)·ln[(1+√(1-1/u))/(1-√(1-1/u))]
/// References: Fröhlich 1954; Heinz et al. PRL 90, 247401 (2003)

#ifndef G4CMPOpticalScatteringRate_hh
#define G4CMPOpticalScatteringRate_hh

#include "G4CMPVScatteringRate.hh"
#include <vector>

class G4CMPOpticalScatteringRate : public G4CMPVScatteringRate {
public:
  /// Constructor with mode data (for Option B: parameterized materials)
  G4CMPOpticalScatteringRate(
    std::vector<G4double> hw_LO_eV,      ///< LO mode frequencies (eV)
    std::vector<G4double> alpha_F_vals,  ///< Fröhlich coupling constants
    G4double m_eff_eV = 0.30             ///< Effective mass (m_e units)
  );

  /// Constructor with material name (for Option A: hardcoded Sapphire)
  G4CMPOpticalScatteringRate(const G4String& aName = "OpticalScattering");

  virtual ~G4CMPOpticalScatteringRate();

  /// Calculate total scattering rate (sum over all LO modes)
  /// γ_opt(E,T) = Σ_s [γ_opt,s(E,T)]
  virtual G4double Rate(const G4Track& aTrack) const;

  /// Rate for single LO mode (Callen form)
  /// γ_opt,s(E,T) = π·α_F,s·ω_s·F(u) · (1 + n_s) + (n_s)  [including Bose factor]
  G4double ModeRate(G4int mode_index, G4double Ekin_J, G4double T) const;

  /// Callen shape function F(u)
  /// F(u) = (1/√u)·ln[(1+√(1-1/u))/(1-√(1-1/u))]
  static G4double CallenF(G4double u);

  /// Bose-Einstein thermal population
  /// n(ω,T) = 1/(exp(ℏω/k_B·T) - 1)
  static G4double BosePopulation(G4double hw, G4double T);

  /// Get number of LO modes
  G4int GetNModes() const { return fNModes; }

  /// Get LO frequency of mode
  G4double GetModeFrequency(G4int mode_index) const;

  /// Get Fröhlich coupling constant of mode
  G4double GetModeCoupling(G4int mode_index) const;

  /// Energy threshold (LO phonon frequency of lowest mode)
  virtual G4double Threshold(G4double) const;

  /// Set verbose level
  virtual void SetVerboseLevel(G4int vb) { fVerboseLevel = vb; }

private:
  // ── Sapphire LO mode data (hardcoded for Option A) ────────────────────
  static const G4int fNModes_Sapphire;
  static const G4double fHW_LO_Sapphire[];       // ℏω (eV)
  static const G4double fAlpha_F_Sapphire[];     // Fröhlich couplings

  // ── Member variables ──────────────────────────────────────────────────
  G4int fNModes;                                 // Number of LO modes
  std::vector<G4double> fHW_LO;                  // LO frequencies (J)
  std::vector<G4double> fAlpha_F;                // Fröhlich coupling constants

  G4double fMass_eff;                            // Effective mass (kg)
  G4int fVerboseLevel;

  // ── Helper methods ────────────────────────────────────────────────────
  void InitializeSapphire();                     // Set hardcoded Sapphire parameters
};

#endif  // G4CMPOpticalScatteringRate_hh
