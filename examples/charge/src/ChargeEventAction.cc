// ChargeEventAction.cc
// Keeps the current event so that trajectory information remains accessible
// to stepping action and analysis manager throughout the event.

#include "ChargeEventAction.hh"

MyEventAction::MyEventAction(MyRunAction*)
{}

MyEventAction::~MyEventAction()
{}

void MyEventAction::BeginOfEventAction(const G4Event*)
{}

void MyEventAction::EndOfEventAction(const G4Event*)
{
    // Keep the event so trajectory data (hits collections, etc.) persists
    // until the analysis manager has written everything to the ROOT file.
    G4EventManager::GetEventManager()->KeepTheCurrentEvent();
}
