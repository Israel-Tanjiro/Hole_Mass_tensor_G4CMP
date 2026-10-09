/***********************************************************************\
 * This software is licensed under the terms of the GNU General Public *
 * License version 3 or later. See G4CMP/LICENSE for the full license. *
\***********************************************************************/
#ifndef G4CMPDriftElectronPolaron_h
#define G4CMPDriftElectronPolaron_h 1

#include "globals.hh"
#include "G4ios.hh"
#include "G4ParticleDefinition.hh"

class G4CMPDriftElectronPolaron : public G4ParticleDefinition {
public:
  static G4CMPDriftElectronPolaron* Definition();
  static G4CMPDriftElectronPolaron* G4CMPDriftElectronPolaronDefinition();

private:
  static G4CMPDriftElectronPolaron* theInstance;
  G4CMPDriftElectronPolaron() {;}
};

#endif  /* G4CMPDriftElectronPolaron_h */
