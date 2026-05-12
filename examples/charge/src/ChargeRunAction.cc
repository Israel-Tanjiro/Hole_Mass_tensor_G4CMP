// ChargeRunAction.cc
// Defines three ntuples:
//   0  "Initial" — first step of each carrier (for convention/direction checks)
//   1  "Steps"   — per-step trajectory data   (for p_mag vs direction analysis)
//   2  "Hits"    — carrier absorbed at electrode (for spread / arrival plots)

#include "ChargeRunAction.hh"

MyRunAction::MyRunAction()
{
    G4AnalysisManager* man = G4AnalysisManager::Instance();
    man->SetVerboseLevel(0);

    // ── Ntuple 0: first step of each carrier ─────────────────────────────────
    man->CreateNtuple("Initial", "First step of each carrier");
    man->CreateNtupleIColumn("TrackID");        // 0
    man->CreateNtupleIColumn("EventID");        // 1
    man->CreateNtupleSColumn("Particle");       // 2
    man->CreateNtupleDColumn("x0_cm");          // 3
    man->CreateNtupleDColumn("y0_cm");          // 4
    man->CreateNtupleDColumn("z0_cm");          // 5
    man->CreateNtupleDColumn("px0_eV");         // 6  momentum x-component
    man->CreateNtupleDColumn("py0_eV");         // 7
    man->CreateNtupleDColumn("pz0_eV");         // 8
    man->CreateNtupleDColumn("Ekin0_eV");       // 9  kinetic energy
    man->FinishNtuple(0);

    // ── Ntuple 1: per-step trajectory ────────────────────────────────────────
    // Bulk steps only (not boundary steps) to keep file size manageable.
    man->CreateNtuple("Steps", "Per-step trajectory data");
    man->CreateNtupleIColumn("TrackID");        // 0
    man->CreateNtupleIColumn("EventID");        // 1
    man->CreateNtupleSColumn("Particle");       // 2
    man->CreateNtupleIColumn("StepNo");         // 3
    man->CreateNtupleDColumn("x_cm");           // 4  pre-step position
    man->CreateNtupleDColumn("y_cm");           // 5
    man->CreateNtupleDColumn("z_cm");           // 6
    man->CreateNtupleDColumn("px_eV");          // 7  pre-step momentum
    man->CreateNtupleDColumn("py_eV");          // 8
    man->CreateNtupleDColumn("pz_eV");          // 9
    man->CreateNtupleDColumn("Ekin_eV");        // 10 pre-step kinetic energy
    man->CreateNtupleSColumn("Process");        // 11 process that caused this step
    man->FinishNtuple(1);

    // ── Ntuple 2: hits — carrier absorbed at electrode ───────────────────────
    man->CreateNtuple("Hits", "Carrier absorbed at electrode boundary");
    man->CreateNtupleIColumn("TrackID");        // 0
    man->CreateNtupleIColumn("EventID");        // 1
    man->CreateNtupleSColumn("Particle");       // 2
    man->CreateNtupleDColumn("xf_cm");          // 3  final position
    man->CreateNtupleDColumn("yf_cm");          // 4
    man->CreateNtupleDColumn("zf_cm");          // 5
    man->CreateNtupleDColumn("pxf_eV");         // 6  final momentum (pre-step of last step)
    man->CreateNtupleDColumn("pyf_eV");         // 7
    man->CreateNtupleDColumn("pzf_eV");         // 8
    man->CreateNtupleDColumn("Ekinf_eV");       // 9  kinetic energy at last step
    man->CreateNtupleDColumn("time_ns");        // 10 global time at absorption
    man->CreateNtupleSColumn("Volume");         // 11 absorbing volume name
    man->CreateNtupleSColumn("Process");        // 12 terminating process name
    man->FinishNtuple(2);
}

MyRunAction::~MyRunAction()
{}

void MyRunAction::BeginOfRunAction(const G4Run* run)
{
    G4AnalysisManager* man = G4AnalysisManager::Instance();
    std::ostringstream fname;
    fname << "charge_out_run" << run->GetRunID() << ".root";
    man->OpenFile(fname.str());
}

void MyRunAction::EndOfRunAction(const G4Run*)
{
    G4AnalysisManager* man = G4AnalysisManager::Instance();
    man->Write();
    man->CloseFile();
}
