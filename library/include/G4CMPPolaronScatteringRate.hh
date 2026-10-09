/***********************************************************************\
 * This software is licensed under the terms of the GNU General Public *
 * License version 3 or later. See G4CMP/LICENSE for the full license. *
\***********************************************************************/

/// \file library/include/G4CMPPolaronScatteringRate.hh
/// \brief Master polaron scattering rate model integrating three mechanisms
///
/// Combines acoustic, optical, and impurity scattering using Mathiessen's rule.
/// This class is the main interface for polaron scattering in G4CMPPolaronScattering.
/// References: Heinz et al. PRL 90, 247401 (2003); Jacoboni & Reggiani 1983

#ifndef G4CMPPolaronScatteringRate_hh
#define G4CMPPolaronScatteringRate_hh

#include "G4CMPVScatteringRate.hh"

class G4CMPAcousticScatteringRate;
class G4CMPOpticalScatteringRate;
class G4CMPImpurityScatteringRate;

class G4CMPPolaronScatteringRate : public G4CMPVScatteringRate {
public:
  /// Constructor (creates default Sapphire instances)
  G4CMPPolaronScatteringRate(const G4String& aName = "PolaronScattering");

  virtual ~G4CMPPolaronScatteringRate();

  // ──────────────────────────────────────────────────────────────────
  // TOTAL RATE (Mathiessen's rule: Γ_total = Γ_ac + Γ_opt + Γ_imp)
  // ──────────────────────────────────────────────────────────────────

  /// Total scattering rate from all mechanisms
  virtual G4double Rate(const G4Track& aTrack) const;

  // ──────────────────────────────────────────────────────────────────
  // INDIVIDUAL MECHANISM RATES
  // ──────────────────────────────────────────────────────────────────

  /// Acoustic phonon scattering rate (Bardeen-Shockley)
  /// Temperature-dependent: Γ_ac ∝ T · E^(-1/2)
  G4double AcousticRate(const G4Track& aTrack) const;

  /// Optical phonon scattering rate (Fröhlich, 6 LO modes)
  /// Thermally activated above ~500 K: Γ_opt ∝ exp(-ħω/k_B·T) · F(u)
  G4double OpticalRate(const G4Track& aTrack) const;

  /// Impurity (Coulomb) scattering rate (Conwell-Weisskopf)
  /// Temperature-independent: Γ_imp ∝ √E / (E + E_T)
  G4double ImpurityRate(const G4Track& aTrack) const;

  // ──────────────────────────────────────────────────────────────────
  // TRANSPORT PROPERTIES
  // ──────────────────────────────────────────────────────────────────

  /// Mean free path λ = v / Γ_total
  G4double MeanFreePath(const G4Track& aTrack) const;

  /// Relaxation time τ = 1 / Γ_total
  G4double RelaxationTime(const G4Track& aTrack) const;

  // ──────────────────────────────────────────────────────────────────
  // MECHANISM SELECTION
  // ──────────────────────────────────────────────────────────────────

  /// Choose scattering mechanism: 0=acoustic, 1=optical, 2=impurity
  /// Weighted by rates using rejection method
  G4int ChooseScatteringMechanism(const G4Track& aTrack) const;

  /// Get name of dominant mechanism at this energy
  G4String GetDominantMechanism(const G4Track& aTrack) const;

  // ──────────────────────────────────────────────────────────────────
  // SCATTERING EVENT HANDLERS
  // ──────────────────────────────────────────────────────────────────

  /// Handle acoustic scattering event
  /// - Inelastic: loses ~0.001 eV per event
  /// - Small-angle deflection (mostly forward)
  void HandleAcousticScattering(G4Track& aTrack) const;

  /// Handle optical scattering event
  /// - Inelastic: loses ħω_opt (mode-dependent, ~0.0597 eV dominant)
  /// - Large-angle deflection (mostly isotropic)
  void HandleOpticalScattering(G4Track& aTrack) const;

  /// Handle impurity scattering event
  /// - Elastic: no energy loss (Coulomb deflection only)
  /// - Small-angle deflection (forward-peaked)
  void HandleImpurityScattering(G4Track& aTrack) const;

  /// Apply scattering angle to track direction
  /// cos_theta: cosine of scattering angle
  void ScatterDirection(G4Track& aTrack, G4double cos_theta) const;

  // ──────────────────────────────────────────────────────────────────
  // DIAGNOSTICS & CONFIGURATION
  // ──────────────────────────────────────────────────────────────────

  /// Set verbose level for debugging
  virtual void SetVerboseLevel(G4int vb);

  /// No threshold for polaron scattering
  virtual G4double Threshold(G4double) const { return 0.; }

  // ──────────────────────────────────────────────────────────────────
  // HELPER METHODS
  // ──────────────────────────────────────────────────────────────────

  /// Check if track is electron polaron
  G4bool IsPolaronElectron(const G4Track& aTrack) const;

  /// Check if track is hole polaron
  G4bool IsPolaronHole(const G4Track& aTrack) const;

  /// Get effective mass of polaron (m* / m_e)
  G4double GetPolaronMass(const G4Track& aTrack) const;

  // ──────────────────────────────────────────────────────────────────
  // ACCESSORS TO RATE CLASSES (for advanced users)
  // ──────────────────────────────────────────────────────────────────

  G4CMPAcousticScatteringRate* GetAcousticRate() { return fAcousticRate; }
  G4CMPOpticalScatteringRate* GetOpticalRate() { return fOpticalRate; }
  G4CMPImpurityScatteringRate* GetImpurityRate() { return fImpurityRate; }

private:
  // ──────────────────────────────────────────────────────────────────
  // MATERIAL PARAMETERS (SAPPHIRE, PHASE 1)
  // ──────────────────────────────────────────────────────────────────

  static const G4double fTemperature;              // Fixed 300K (Phase 1)
  static const G4double fDebyeTemperature_Sapphire;

  // ──────────────────────────────────────────────────────────────────
  // SCATTERING RATE INSTANCES
  // ──────────────────────────────────────────────────────────────────

  G4CMPAcousticScatteringRate* fAcousticRate;   ///< Bardeen-Shockley
  G4CMPOpticalScatteringRate*  fOpticalRate;    ///< Fröhlich (6 modes)
  G4CMPImpurityScatteringRate* fImpurityRate;   ///< Conwell-Weisskopf

  G4int fVerboseLevel;
};

#endif  // G4CMPPolaronScatteringRate_hh
