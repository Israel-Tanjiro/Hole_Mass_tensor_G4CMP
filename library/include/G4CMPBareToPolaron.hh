/***********************************************************************\
 * This software is licensed under the terms of the GNU General Public *
 * License version 3 or later. See G4CMP/LICENSE for the full license. *
\***********************************************************************/

/// \file library/include/G4CMPBareToPolaron.hh
/// \brief Process to detect and convert bare carriers to polarons

// In G4CMPBareToPolaron.hh, after includes but before class definition:
#ifndef G4CMPBareToPolaron_h
#define G4CMPBareToPolaron_h

#include "G4VDiscreteProcess.hh"

class G4ParticleDefinition;

class G4CMPBareToPolaron : public G4VDiscreteProcess  {
public:
  G4CMPBareToPolaron(const G4String& aName = "BareToPolaron");
  virtual ~G4CMPBareToPolaron();
  
  virtual G4bool IsApplicable(const G4ParticleDefinition&) override;
  
  // Get mean free path (discrete process interface)
  virtual G4double GetMeanFreePath(const G4Track& aTrack,
                                    G4double previousStepSize,
                                    G4ForceCondition* condition) override;

  // Main process: convert bare to polaron
  virtual G4VParticleChange* PostStepDoIt(const G4Track&, const G4Step&) override;
  
private:
  G4double fThresholdEnergy;
  G4ParticleDefinition* fElectronPolaron;
  G4ParticleDefinition* fHolePolaron;
  G4int fPolaronsFormed;
  G4int verboseLevel;

  G4ParticleDefinition* GetElectronPolaronDefinition();
  G4ParticleDefinition* GetHolePolaronDefinition();
  G4bool IsBareElectron(const G4ParticleDefinition* pd) const;
  G4bool IsBareHole(const G4ParticleDefinition* pd) const;
};

#endif