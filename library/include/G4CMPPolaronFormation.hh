/***********************************************************************\
 * This software is licensed under the terms of the GNU General Public *
 * License version 3 or later. See G4CMP/LICENSE for the full license. *
\***********************************************************************/

/// \file library/include/G4CMPPolaronFormation.hh
/// \brief Process for large polaron formation upon carrier creation in sapphire

#ifndef G4CMPPolaronFormation_h
#define G4CMPPolaronFormation_h 1

#include "G4VDiscreteProcess.hh"
#include "G4ParticleChange.hh"

class G4Track;
class G4Step;
class G4LatticePhysical;

/// \class G4CMPPolaronFormation
/// \brief Process that converts electrons/holes to large polarons upon creation

class G4CMPPolaronFormation : public G4VDiscreteProcess {
public:
  explicit G4CMPPolaronFormation(const G4String& processName = "PolaronFormation");
  virtual ~G4CMPPolaronFormation();

  // Process interface
  virtual G4bool IsApplicable(const G4ParticleDefinition& aPD) const;

  virtual G4double GetMeanFreePath(const G4Track& aTrack,
                                    G4double previousStepSize,
                                    G4ForceCondition* condition);

  virtual G4VParticleChange* PostStepDoIt(const G4Track& aTrack,
                                          const G4Step& aStep);

  // Runtime configuration (for macro commands)
  void SetPolaronFormationActive(G4bool active);
  G4bool GetPolaronFormationActive() const;
  void SetFormationEnergy(G4double energy);
  G4double GetFormationEnergy() const;
  void SetPhononEfficiency(G4double eff);
  G4double GetPhononEfficiency() const;

protected:
  // Physics constants
  static G4double ALPHA_ELECTRON;        // Fröhlich coupling
  static G4double ALPHA_HOLE;            // Fröhlich coupling
  static G4double FORMATION_ENERGY;      // eV - Stokes shift
  static G4double HW_LO;                 // eV - LO phonon frequency

  // Mass tensor update
  void UpdateCarrierMass(const G4Track& aTrack, G4bool isElectron) const;

  G4double GetMassEnhancementFactor(G4double alphaCoupling) const;

  // Phonon emission
  void EmitPolaronPhonons(const G4Track& aTrack, G4double formationEnergy);

private:
  G4int fVerboseLevel;
  G4int fPolaronsFormed;
  G4LatticePhysical* fLattice;

  G4ParticleChange aParticleChange;
};

#endif // G4CMPPolaronFormation_h
