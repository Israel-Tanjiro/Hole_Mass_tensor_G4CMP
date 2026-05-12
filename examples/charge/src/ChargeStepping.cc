// ChargeStepping.cc
// Fills three ntuples defined in ChargeRunAction:
//   0  "Initial" — first step of each carrier
//   1  "Steps"   — every bulk step (for p_mag vs direction analysis)
//   2  "Hits"    — carrier absorbed at electrode boundary

#include "ChargeStepping.hh"
#include "G4RunManager.hh"
#include "G4VProcess.hh"
#include "G4SystemOfUnits.hh"

MySteppingAction::MySteppingAction(MyEventAction* eventAction)
    : fEventAction(eventAction)
{}

MySteppingAction::~MySteppingAction()
{}

void MySteppingAction::UserSteppingAction(const G4Step* step)
{
    // ── Basic track / step info ───────────────────────────────────────────────
    G4Track*              track    = step->GetTrack();
    G4int                 trackID  = track->GetTrackID();
    G4int                 stepNo   = track->GetCurrentStepNumber();
    G4String              partName = track->GetDefinition()->GetParticleName();
    G4int                 eventID  = G4RunManager::GetRunManager()
                                         ->GetCurrentEvent()->GetEventID();

    const G4StepPoint* pre  = step->GetPreStepPoint();
    const G4StepPoint* post = step->GetPostStepPoint();

    // ── Pre-step state (position, momentum, kinetic energy) ──────────────────
    G4ThreeVector prePos = pre->GetPosition();
    G4double x_cm  = prePos.x() / CLHEP::cm;
    G4double y_cm  = prePos.y() / CLHEP::cm;
    G4double z_cm  = prePos.z() / CLHEP::cm;

    G4ThreeVector preMom = pre->GetMomentum();
    G4double px_eV = preMom.x() / CLHEP::eV;
    G4double py_eV = preMom.y() / CLHEP::eV;
    G4double pz_eV = preMom.z() / CLHEP::eV;

    G4double Ekin_eV = pre->GetKineticEnergy() / CLHEP::eV;

    // Process that produced this step (post-step process)
    G4String procName = "unknown";
    if (post->GetProcessDefinedStep())
        procName = post->GetProcessDefinedStep()->GetProcessName();

    G4AnalysisManager* man = G4AnalysisManager::Instance();

    // ── Ntuple 0: first step — initial carrier state ──────────────────────────
    if (stepNo == 1) {
        man->FillNtupleIColumn(0, 0, trackID);
        man->FillNtupleIColumn(0, 1, eventID);
        man->FillNtupleSColumn(0, 2, partName);
        man->FillNtupleDColumn(0, 3, x_cm);
        man->FillNtupleDColumn(0, 4, y_cm);
        man->FillNtupleDColumn(0, 5, z_cm);
        man->FillNtupleDColumn(0, 6, px_eV);
        man->FillNtupleDColumn(0, 7, py_eV);
        man->FillNtupleDColumn(0, 8, pz_eV);
        man->FillNtupleDColumn(0, 9, Ekin_eV);
        man->AddNtupleRow(0);
    }

    // ── Ntuple 1: per-step trajectory (bulk steps only) ──────────────────────
    // Skip boundary steps to keep file size manageable; the "Hits" ntuple
    // captures the final boundary crossing separately.
    G4bool isBoundary = (post->GetStepStatus() == fGeomBoundary);
    if (!isBoundary) {
        man->FillNtupleIColumn(1, 0, trackID);
        man->FillNtupleIColumn(1, 1, eventID);
        man->FillNtupleSColumn(1, 2, partName);
        man->FillNtupleIColumn(1, 3, stepNo);
        man->FillNtupleDColumn(1, 4, x_cm);
        man->FillNtupleDColumn(1, 5, y_cm);
        man->FillNtupleDColumn(1, 6, z_cm);
        man->FillNtupleDColumn(1, 7, px_eV);
        man->FillNtupleDColumn(1, 8, py_eV);
        man->FillNtupleDColumn(1, 9, pz_eV);
        man->FillNtupleDColumn(1, 10, Ekin_eV);
        man->FillNtupleSColumn(1, 11, procName);
        man->AddNtupleRow(1);
    }

    // ── Ntuple 2: hit — carrier absorbed at electrode boundary ───────────────
    // Condition: track is killed AND it crossed a geometry boundary.
    // G4CMP carriers end with fStopAndKill at the electrode surface.
    // We do NOT require NonIonizingEnergyDeposit > 0 because G4CMP carriers
    // typically deposit zero energy in the standard Geant4 sense.
    G4bool isKilled  = (track->GetTrackStatus() == fStopAndKill);
    if (isKilled && isBoundary) {
        G4ThreeVector postPos = post->GetPosition();
        G4double xf_cm  = postPos.x() / CLHEP::cm;
        G4double yf_cm  = postPos.y() / CLHEP::cm;
        G4double zf_cm  = postPos.z() / CLHEP::cm;

        G4double time_ns = post->GetGlobalTime() / CLHEP::ns;

        G4String volumeName = "unknown";
        if (post->GetTouchableHandle()->GetVolume())
            volumeName = post->GetTouchableHandle()->GetVolume()->GetName();

        man->FillNtupleIColumn(2, 0, trackID);
        man->FillNtupleIColumn(2, 1, eventID);
        man->FillNtupleSColumn(2, 2, partName);
        man->FillNtupleDColumn(2, 3, xf_cm);
        man->FillNtupleDColumn(2, 4, yf_cm);
        man->FillNtupleDColumn(2, 5, zf_cm);
        man->FillNtupleDColumn(2, 6, px_eV);   // pre-step momentum of the last step
        man->FillNtupleDColumn(2, 7, py_eV);
        man->FillNtupleDColumn(2, 8, pz_eV);
        man->FillNtupleDColumn(2, 9, Ekin_eV);
        man->FillNtupleDColumn(2, 10, time_ns);
        man->FillNtupleSColumn(2, 11, volumeName);
        man->FillNtupleSColumn(2, 12, procName);
        man->AddNtupleRow(2);
    }
}
