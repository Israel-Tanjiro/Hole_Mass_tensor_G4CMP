/***********************************************************************\
 * This software is licensed under the terms of the GNU General Public *
 * License version 3 or later. See G4CMP/LICENSE for the full license. *
\***********************************************************************/

/// \file library/include/G4CMPPolaronScatteringKinematics.hh
/// \brief Kinematics helper class for polaron scattering events
///
/// Handles:
/// - Sampling scattering angles
/// - Energy loss calculations
/// - Direction rotation for inelastic and elastic scattering
/// - Momentum conservation checks

#ifndef G4CMPPolaronScatteringKinematics_hh
#define G4CMPPolaronScatteringKinematics_hh

#include "G4ThreeVector.hh"

class G4Track;
class G4ParticleChange;

class G4CMPPolaronScatteringKinematics {
public:
  G4CMPPolaronScatteringKinematics();
  virtual ~G4CMPPolaronScatteringKinematics() {}

  // ──────────────────────────────────────────────────────────────────
  // SCATTERING ANGLE SAMPLING
  // ──────────────────────────────────────────────────────────────────

  /// Sample isotropic scattering angle (for optical phonons)
  /// Returns cos(θ) uniformly in [-1, +1]
  G4double SampleIsotropicAngle() const;

  /// Sample forward-peaked angle (for acoustic and impurity)
  /// Returns cos(θ) with preference for forward scattering
  /// Uses cos(θ) = 2u - 1 for acoustic, custom peak for impurity
  G4double SampleForwardPeakedAngle(G4bool isImpurity = false) const;

  // ──────────────────────────────────────────────────────────────────
  // ENERGY LOSS HANDLING
  // ──────────────────────────────────────────────────────────────────

  /// Calculate energy loss for acoustic scattering
  /// Returns ~1 meV (inelastic)
  G4double GetAcousticEnergyLoss() const;

  /// Calculate energy loss for optical scattering
  /// Returns ħω_LO (dominant mode ~59.7 meV)
  G4double GetOpticalEnergyLoss() const;

  /// Calculate energy loss for impurity scattering
  /// Returns 0 (elastic)
  G4double GetImpurityEnergyLoss() const { return 0.; }

  // ──────────────────────────────────────────────────────────────────
  // DIRECTION ROTATION
  // ──────────────────────────────────────────────────────────────────

  /// Rotate momentum direction by scattering angle
  /// Applies scattering in random azimuthal plane
  G4ThreeVector RotateDirection(const G4ThreeVector& oldDir,
                                G4double cosTheta);

  /// Build orthonormal basis from initial direction
  /// Creates perpendicular vectors for rotation
  void BuildBasis(const G4ThreeVector& direction,
                  G4ThreeVector& u, G4ThreeVector& v, G4ThreeVector& w) const;

  // ──────────────────────────────────────────────────────────────────
  // CONSERVATION CHECKS
  // ──────────────────────────────────────────────────────────────────

  /// Verify energy conservation
  /// Checks: E_initial = E_final + ΔE_loss
  G4bool CheckEnergyConservation(G4double E_initial, G4double E_final,
                                 G4double dE_loss, G4double tolerance = 0.01) const;

  /// Verify momentum direction is unit vector
  G4bool CheckMomentumDirection(const G4ThreeVector& dir,
                                G4double tolerance = 1e-6) const;

  // ──────────────────────────────────────────────────────────────────
  // DIAGNOSTICS
  // ──────────────────────────────────────────────────────────────────

  void SetVerboseLevel(G4int vb) { fVerboseLevel = vb; }

private:
  G4int fVerboseLevel;

  // ──────────────────────────────────────────────────────────────────
  // CONSTANTS
  // ──────────────────────────────────────────────────────────────────

  // Energy loss values (can be made configurable in Phase 2)
  static constexpr G4double fAcousticEnergyLoss = 1.0;    // meV
  static constexpr G4double fOpticalEnergyLoss = 59.7;    // meV (Eu-LO2)
};

#endif  // G4CMPPolaronScatteringKinematics_hh
