/***********************************************************************\
 * This software is licensed under the terms of the GNU General Public *
 * License version 3 or later. See G4CMP/LICENSE for the full license. *
\***********************************************************************/

#include "ChargeActionInitialization.hh"
#include "ChargePrimaryGeneratorAction.hh"
#include "G4CMPStackingAction.hh"
#include "ChargeRunAction.hh"
#include "ChargeEventAction.hh"
#include "ChargeStepping.hh"



void ChargeActionInitialization::Build() const {
  SetUserAction(new ChargePrimaryGeneratorAction);
  SetUserAction(new G4CMPStackingAction);
   MyRunAction *runAction = new MyRunAction();
     SetUserAction(runAction);

     MyEventAction *eventAction = new MyEventAction(runAction);
         SetUserAction(eventAction);

         MySteppingAction *steppingAction = new MySteppingAction(eventAction);
         SetUserAction(steppingAction);
} 
