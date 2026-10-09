/***********************************************************************\
 * This software is licensed under the terms of the GNU General Public *
 * License version 3 or later. See G4CMP/LICENSE for the full license. *
\***********************************************************************/

/// \file library/include/G4CMPPolaronOpticalScattering.hh
/// \brief Optical phonon scattering process for large polarons in sapphire

#ifndef G4CMPPolaronOpticalScattering_h
#define G4CMPPolaronOpticalScattering_h 1

#include "G4VDiscreteProcess.hh"
#include "G4ParticleChange.hh"
#include "G4CMPScatteringRateInterface.hh"
#include <set>

class G4CMPPolaronInfo;

/// \class G4CMPPolaronOpticalScattering
/// \brief Process for polaron optical phonon scattering
///
/// After a carrier forms a polaron (detected via G4CMPPolaronInfo on track),
/// this process applies mode-specific optical phonon scattering using the
/// Fröhlich coupling constants and phonon mode energies.
///
/// **Activation:**
/// - Only applies to carriers marked as polarons
/// - Only if α_Fröhlich ≥ α_threshold (default 1.0)
/// - Currently optimized for Al₂O₃ (sapphire)
///
/// **Physics:**
/// - Uses G4CMPOpticalScatteringRate for rate calculations
/// - Energy-dependent form factor F(u) based on Callen's theory
/// - Temperature-dependent Bose thermal population
/// - Multiple LO modes with distinct coupling strengths

class G4CMPPolaronOpticalScattering : public G4VDiscreteProcess {
public:
  explicit G4CMPPolaronOpticalScattering(const G4String& processName = "PolaronOpticalScattering");
  virtual ~G4CMPPolaronOpticalScattering();

  // Process interface
  virtual G4bool IsApplicable(const G4ParticleDefinition& pd);

  virtual G4double GetMeanFreePath(const G4Track& aTrack,
                                    G4double previousStepSize,
                                    G4ForceCondition* condition);

  virtual G4VParticleChange* PostStepDoIt(const G4Track& aTrack,
                                          const G4Step& aStep);

  // Configuration
  void SetAlphaThreshold(G4double alpha) { fAlphaThreshold = alpha; }
  G4double GetAlphaThreshold() const { return fAlphaThreshold; }

  void SetVerboseLevel(G4int level) { verboseLevel = level; }
  G4int GetVerboseLevel() const { return verboseLevel; }

protected:
  /// Check if this track represents a polaron
  G4bool IsPolaronTrack(const G4Track& aTrack) const;

  /// Get Fröhlich coupling constant from track polaron info
  G4double GetAlphaCoupling(const G4Track& aTrack) const;

  /// Calculate scattering rate for this carrier
  G4double CalculateScatteringRate(const G4Track& aTrack) const;

private:
  // Threshold for polaron activation (α_min ≥ this value)
  G4double fAlphaThreshold;

  // Scattering rate calculator
  G4CMPOpticalScatteringRate* fScatteringRate;

  // Track singleton: ensure single scattering per mean free path
  static std::set<G4int> scatteredTracks;

  G4ParticleChange aParticleChange;
  G4int verboseLevel;
};

#endif // G4CMPPolaronOpticalScattering_h
