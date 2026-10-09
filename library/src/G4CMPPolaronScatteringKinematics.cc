/***********************************************************************\
 * This software is licensed under the terms of the GNU General Public *
 * License version 3 or later. See G4CMP/LICENSE for the full license. *
\***********************************************************************/

/// \file library/src/G4CMPPolaronScatteringKinematics.cc
/// \brief Implementation of G4CMPPolaronScatteringKinematics

#include "G4CMPPolaronScatteringKinematics.hh"
#include "G4SystemOfUnits.hh"
#include "G4PhysicalConstants.hh"
#include "Randomize.hh"
#include <cmath>
#include <algorithm>

// ============================================================================
// CONSTRUCTOR
// ============================================================================

G4CMPPolaronScatteringKinematics::G4CMPPolaronScatteringKinematics()
  : fVerboseLevel(0) {
}

// ============================================================================
// SCATTERING ANGLE SAMPLING
// ============================================================================

G4double G4CMPPolaronScatteringKinematics::SampleIsotropicAngle() const {
  G4double u = G4UniformRand();
  G4double cosTheta = 2.0 * u - 1.0;
  return cosTheta;
}

G4double G4CMPPolaronScatteringKinematics::SampleForwardPeakedAngle(
  G4bool isImpurity) const {
  G4double u = G4UniformRand();
  G4double cosTheta = 2.0 * u - 1.0;
  return cosTheta;
}

// ============================================================================
// ENERGY LOSS HANDLING
// ============================================================================

G4double G4CMPPolaronScatteringKinematics::GetAcousticEnergyLoss() const {
  return 0.001 * eV;  // ~1 meV
}

G4double G4CMPPolaronScatteringKinematics::GetOpticalEnergyLoss() const {
  return 0.0597 * eV;  // ~59.7 meV
}

// ============================================================================
// DIRECTION ROTATION
// ============================================================================

G4ThreeVector G4CMPPolaronScatteringKinematics::RotateDirection(
  const G4ThreeVector& oldDir,
  G4double cosTheta) {

  cosTheta = std::min(1.0, std::max(-1.0, cosTheta));
  G4double sinTheta = std::sqrt(1.0 - cosTheta * cosTheta);

  G4double phi = 2.0 * pi * G4UniformRand();
  G4double cosPhi = std::cos(phi);
  G4double sinPhi = std::sin(phi);

  G4ThreeVector u, v, w;
  BuildBasis(oldDir, u, v, w);

  G4ThreeVector newDir = cosTheta * u + sinTheta * (cosPhi * v + sinPhi * w);
  return newDir.unit();
}

void G4CMPPolaronScatteringKinematics::BuildBasis(
  const G4ThreeVector& direction,
  G4ThreeVector& u,
  G4ThreeVector& v,
  G4ThreeVector& w) const {

  u = direction.unit();

  if (std::abs(u.x()) < 0.9) {
    v = G4ThreeVector(1.0, 0.0, 0.0);
  } else if (std::abs(u.y()) < 0.9) {
    v = G4ThreeVector(0.0, 1.0, 0.0);
  } else {
    v = G4ThreeVector(0.0, 0.0, 1.0);
  }

  v = v - (v.dot(u)) * u;
  v = v.unit();

  w = u.cross(v);
  w = w.unit();
}

// ============================================================================
// CONSERVATION CHECKS
// ============================================================================

G4bool G4CMPPolaronScatteringKinematics::CheckEnergyConservation(
  G4double E_initial,
  G4double E_final,
  G4double dE_loss,
  G4double tolerance) const {

  G4double expected_Efinal = E_initial - dE_loss;
  G4double delta = std::abs(E_final - expected_Efinal);
  G4double relative_error = (E_initial > 0.) ? delta / E_initial : 0.;

  G4bool is_conserved = (relative_error < tolerance);

  if (fVerboseLevel > 1 || !is_conserved) {
    G4cout << "CheckEnergyConservation:"
           << " E_initial=" << E_initial / eV << " eV"
           << " ΔE_loss=" << dE_loss / eV << " eV"
           << " E_final=" << E_final / eV << " eV (expected " << expected_Efinal / eV << ")"
           << " error=" << relative_error * 100.0 << "%" << G4endl;
  }

  return is_conserved;
}

G4bool G4CMPPolaronScatteringKinematics::CheckMomentumDirection(
  const G4ThreeVector& dir,
  G4double tolerance) const {

  G4double mag = dir.mag();
  G4double delta = std::abs(mag - 1.0);

  G4bool is_unit = (delta < tolerance);

  if (fVerboseLevel > 2 || !is_unit) {
    G4cout << "CheckMomentumDirection: |dir|=" << mag
           << " (expected 1.0), error=" << delta << G4endl;
  }

  return is_unit;
}
