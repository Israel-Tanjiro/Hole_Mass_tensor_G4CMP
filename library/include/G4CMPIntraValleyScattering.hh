/***********************************************************************\
 * This software is licensed under the terms of the GNU General Public *
 * License version 3 or later. See G4CMP/LICENSE for the full license. *
\***********************************************************************/

/// \file library/include/G4CMPIntraValleyScattering.hh
/// \brief Discrete process for intravalley scattering in single-valley materials
///
/// Applies to polaron carriers marked with G4CMPPolaronInfo.
/// For single-valley materials like Sapphire, handles:
/// - Acoustic phonon scattering
/// - Optical phonon scattering (Fröhlich coupling)
/// - Impurity scattering

#ifndef G4CMPIntraValleyScattering_hh
#define G4CMPIntraValleyScattering_hh 1

#include "G4CMPVDriftProcess.hh"
#include "G4CMPIntraValleyRate.hh"

// NOTE: base class changed from G4CMPVProcess to G4CMPVDriftProcess.
// The implementation (PostStepDoIt) is modeled on the "Luke" acoustic
// scattering process and relies on G4CMPVDriftProcess functionality:
// InitializeParticleChange(), GetValleyIndex(), the inherited
// aParticleChange member, GetLocalDirection()/GetLocalMomentum(),
// IsElectron()/IsHole(), RotateToGlobalDirection(), FillParticleChange(),
// GetVerboseLevel(), and ClearNumberOfInteractionLengthLeft(). The
// constructor also initializes G4CMPVDriftProcess directly in its
// base-initializer list. None of these are members of G4CMPVProcess, so
// the previous base class would not compile against this implementation.
class G4CMPIntraValleyScattering : public G4CMPVDriftProcess {
public:
  G4CMPIntraValleyScattering(const G4String& processName = "G4CMPIntraValleyScattering");
  virtual ~G4CMPIntraValleyScattering();

  // Process interface
  virtual G4bool IsApplicable(const G4ParticleDefinition& aParticle);
  virtual G4double GetMeanFreePath(const G4Track& aTrack, G4double,
                                    G4ForceCondition* condition);
  virtual G4VParticleChange* PostStepDoIt(const G4Track& aTrack,
                                           const G4Step& aStep);

  // Rate model accessor
  virtual G4CMPVScatteringRate* GetRateModel() { return fRateModel; }
  virtual const G4CMPVScatteringRate* GetRateModel() const { return fRateModel; }

private:
  G4CMPIntraValleyRate* fRateModel;
};

#endif  /* G4CMPIntraValleyScattering_hh */
