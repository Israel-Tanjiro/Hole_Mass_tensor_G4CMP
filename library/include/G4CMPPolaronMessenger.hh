/***********************************************************************\
 * This software is licensed under the terms of the GNU General Public *
 * License version 3 or later. See G4CMP/LICENSE for the full license. *
\***********************************************************************/

/// \file library/include/G4CMPPolaronMessenger.hh
/// \brief Macro command definitions for G4CMPPolaronFormation process
///
/// Follows G4CMP pattern: G4CMPConfigMessenger <-> G4CMPConfigManager
/// This messenger: G4CMPPolaronMessenger <-> G4CMPPolaronFormation

#ifndef G4CMPPolaronMessenger_h
#define G4CMPPolaronMessenger_h 1

#include "G4UImessenger.hh"

class G4CMPPolaronFormation;
class G4UIcommand;
class G4UIcmdWithABool;
class G4UIcmdWithADouble;
class G4UIcmdWithADoubleAndUnit;
class G4UIcmdWithoutParameter;

/// \class G4CMPPolaronMessenger
/// \brief User interface commands for polaron formation process
///
/// Provides macro-level control of polaron formation physics:
///   /process/polaron/active           - Enable/disable formation
///   /process/polaron/formationEnergy  - Set Stokes shift (eV)
///   /process/polaron/phononEfficiency - Set phonon fraction (0-1)
///   /process/polaron/verbose          - Toggle debug output

class G4CMPPolaronMessenger : public G4UImessenger {
public:
  explicit G4CMPPolaronMessenger(G4CMPPolaronFormation* process);
  virtual ~G4CMPPolaronMessenger();

  virtual void SetNewValue(G4UIcommand* command, G4String newValues);

private:
  G4CMPPolaronFormation* fProcess;

  // Commands (matched to G4CMP style)
  G4UIcmdWithABool* fActiveCmd;
  G4UIcmdWithADoubleAndUnit* fFormationEnergyCmd;
  G4UIcmdWithADouble* fPhononEfficiencyCmd;
  G4UIcmdWithoutParameter* fVerboseCmd;
};

#endif  // G4CMPPolaronMessenger_h
