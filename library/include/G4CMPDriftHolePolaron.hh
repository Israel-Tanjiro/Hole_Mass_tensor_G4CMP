/***********************************************************************\
 * This software is licensed under the terms of the GNU General Public *
 * License version 3 or later. See G4CMP/LICENSE for the full license. *
\***********************************************************************/
#ifndef G4CMPDriftHolePolaron_h
#define G4CMPDriftHolePolaron_h 1

#include "globals.hh"
#include "G4ios.hh"
#include "G4ParticleDefinition.hh"

class G4CMPDriftHolePolaron : public G4ParticleDefinition {
public:
  static G4CMPDriftHolePolaron* Definition();
  static G4CMPDriftHolePolaron* G4CMPDriftHolePolaronDefinition();

private:
  static G4CMPDriftHolePolaron* theInstance;
  G4CMPDriftHolePolaron() {;}
};

#endif  /* G4CMPDriftHolePolaron_h */
 
