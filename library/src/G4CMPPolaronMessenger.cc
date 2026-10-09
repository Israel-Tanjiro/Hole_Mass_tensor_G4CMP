/***********************************************************************\
 * This software is licensed under the terms of the GNU General Public *
 * License version 3 or later. See G4CMP/LICENSE for the full license. *
\***********************************************************************/

/// \file library/src/G4CMPPolaronMessenger.cc
/// \brief Macro command definitions for G4CMPPolaronFormation process
///
/// Follows G4CMP pattern exactly (see G4CMPConfigMessenger.cc)
/// All parameters configurable at runtime via macro commands

#include "G4CMPPolaronMessenger.hh"
#include "G4CMPPolaronFormation.hh"
#include "G4UIcmdWithABool.hh"
#include "G4UIcmdWithADouble.hh"
#include "G4UIcmdWithADoubleAndUnit.hh"
#include "G4UIcmdWithoutParameter.hh"
#include "G4SystemOfUnits.hh"


// Constructor: Create all commands in /process/polaron/ directory

G4CMPPolaronMessenger::G4CMPPolaronMessenger(G4CMPPolaronFormation* process)
    : G4UImessenger("/g4cmp/polaron/",
                    "Polaron formation physics for large polarons in wide-gap semiconductors"),
      fProcess(process),
      fActiveCmd(0),
      fFormationEnergyCmd(0),
      fPhononEfficiencyCmd(0),
      fVerboseCmd(0) {

  // Command: /process/polaron/active
  fActiveCmd = new G4UIcmdWithABool("active", this);
  fActiveCmd->SetGuidance("Enable/disable polaron formation process");
  fActiveCmd->SetGuidance("true  : Polaron formation ON (default)");
  fActiveCmd->SetGuidance("false : Polaron formation OFF (bare carriers)");
  fActiveCmd->SetParameterName("active", false);
  fActiveCmd->SetDefaultValue(true);
  fActiveCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  // Command: /process/polaron/formationEnergy
  fFormationEnergyCmd = new G4UIcmdWithADoubleAndUnit("formationEnergy", this);
  fFormationEnergyCmd->SetGuidance("Set polaron formation energy (Stokes shift)");
  fFormationEnergyCmd->SetGuidance("Physics range: 0.5 - 1.0 eV");
  fFormationEnergyCmd->SetGuidance("Default: 0.75 eV (literature-validated)");
  fFormationEnergyCmd->SetGuidance("Source: French et al., Shan et al., Storchak et al.");
  fFormationEnergyCmd->SetParameterName("energy", false);
  fFormationEnergyCmd->SetUnitCategory("Energy");
  fFormationEnergyCmd->SetDefaultValue(0.75);
  fFormationEnergyCmd->SetDefaultUnit("eV");
  fFormationEnergyCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  // Command: /process/polaron/phononEfficiency
  fPhononEfficiencyCmd = new G4UIcmdWithADouble("phononEfficiency", this);
  fPhononEfficiencyCmd->SetGuidance("Set fraction of formation energy that becomes phonons");
  fPhononEfficiencyCmd->SetGuidance("Range: 0.0 - 1.0");
  fPhononEfficiencyCmd->SetGuidance("Default: 0.75 (75% phonons, 25% heat)");
  fPhononEfficiencyCmd->SetGuidance("Remaining energy (1-efficiency) dissipates as heat");
  fPhononEfficiencyCmd->SetParameterName("efficiency", false);
  fPhononEfficiencyCmd->SetDefaultValue(0.75);
  fPhononEfficiencyCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  // Command: /process/polaron/verbose
  fVerboseCmd = new G4UIcmdWithoutParameter("verbose", this);
  fVerboseCmd->SetGuidance("Toggle verbose output for polaron formation");
  fVerboseCmd->SetGuidance("Prints formation details for each carrier created");
  fVerboseCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
}


// Destructor: Clean up all command objects

G4CMPPolaronMessenger::~G4CMPPolaronMessenger() {
  delete fActiveCmd;           fActiveCmd = 0;
  delete fFormationEnergyCmd;  fFormationEnergyCmd = 0;
  delete fPhononEfficiencyCmd; fPhononEfficiencyCmd = 0;
  delete fVerboseCmd;          fVerboseCmd = 0;
}


// SetNewValue: Process macro commands

void G4CMPPolaronMessenger::SetNewValue(G4UIcommand* command, G4String newValues) {
  if (!fProcess) return;

  // /process/polaron/active true|false
  if (command == fActiveCmd) {
    G4bool active = fActiveCmd->GetNewBoolValue(newValues);
    fProcess->SetPolaronFormationActive(active);
    G4cout << "Polaron formation: " << (active ? "ON" : "OFF") << G4endl;
  }

  // /process/polaron/formationEnergy <value> [unit]
  if (command == fFormationEnergyCmd) {
    G4double energy = fFormationEnergyCmd->GetNewDoubleValue(newValues);
    fProcess->SetFormationEnergy(energy);
    G4cout << "Formation energy set to " << energy / eV << " eV" << G4endl;
  }

  // /process/polaron/phononEfficiency <value>
  if (command == fPhononEfficiencyCmd) {
    G4double efficiency = fPhononEfficiencyCmd->GetNewDoubleValue(newValues);

    // Sanity check
    if (efficiency < 0.0 || efficiency > 1.0) {
      G4cerr << "WARNING: Efficiency must be between 0 and 1, ignoring value " << efficiency << G4endl;
      return;
    }

    fProcess->SetPhononEfficiency(efficiency);
    G4cout << "Phonon efficiency set to " << efficiency * 100 << "% (phonons)"
           << " and " << (1.0 - efficiency) * 100 << "% (heat)" << G4endl;
  }

  // /process/polaron/verbose (toggle)
  if (command == fVerboseCmd) {
    G4int newLevel = (fProcess->GetVerboseLevel() == 0) ? 1 : 0;
    fProcess->SetVerboseLevel(newLevel);
    G4cout << "Verbose output: " << (newLevel > 0 ? "ON" : "OFF") << G4endl;
  }
}
