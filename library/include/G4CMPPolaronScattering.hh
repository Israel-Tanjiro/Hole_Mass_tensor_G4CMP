/***********************************************************************\
 * This software is licensed under the terms of the GNU General Public *
 * License version 3 or later. See G4CMP/LICENSE for the full license. *
\***********************************************************************/

/// \file library/include/G4CMPPolaronScattering.hh
/// \brief Discrete process for polaron scattering
///
/// Integrates three scattering mechanisms (acoustic, optical, impurity)
/// using Mathiessen's rule: Γ_total = Γ_ac + Γ_opt + Γ_imp

#ifndef G4CMPPolaronScattering_hh
#define G4CMPPolaronScattering_hh

#include "G4VDiscreteProcess.hh"

class G4CMPPolaronScatteringRate;
class G4CMPPolaronScatteringKinematics;

class G4CMPPolaronScattering : public G4VDiscreteProcess {
public:
  G4CMPPolaronScattering(const G4String& processName = "PolaronScattering");
  virtual ~G4CMPPolaronScattering();

  // Required virtual methods
  virtual G4bool IsApplicable(const G4ParticleDefinition& aParticle);

  virtual G4double GetMeanFreePath(const G4Track& track,
                                    G4double previousStepSize,
                                    G4ForceCondition* condition);

  virtual G4VParticleChange* PostStepDoIt(const G4Track& aTrack, const G4Step& aStep);

  virtual void SetVerboseLevel(G4int vb);

private:
  G4CMPPolaronScatteringRate* fScatteringRate;
  G4CMPPolaronScatteringKinematics* fKinematics;
  G4int fVerboseLevel;
};

#endif  // G4CMPPolaronScattering_hh
