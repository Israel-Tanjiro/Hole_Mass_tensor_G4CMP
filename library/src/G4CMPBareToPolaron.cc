/***********************************************************************\
 * This software is licensed under the terms of the GNU General Public *
 * License version 3 or later. See G4CMP/LICENSE for the full license. *
\***********************************************************************/

/// \file library/src/G4CMPBareToPolaron.cc
/// \brief Process to detect bare carriers reaching threshold and replace
///        them with a polaron secondary (kill-and-replace pattern).
///
/// \note KILL-AND-REPLACE VERSION
///   Instead of converting the track in place via SetDefinition (which
///   leaves the G4CMPDriftTrackInfo's valley index pointing at the old
///   electron/hole valley), this version:
///     1. Computes the polaron's final energy/momentum exactly as before.
///     2. Kills the bare carrier track (fStopAndKill).
///     3. Spawns a brand-new G4Track carrying the polaron particle
///        definition as a secondary.
///   The new track goes through the normal secondary pipeline
///   (G4CMPStackingAction::ClassifyNewTrack -> AttachTrackInfo), so it
///   gets a *fresh* G4CMPDriftTrackInfo with ValleyIndex() == -1 and the
///   correct lattice pointer, with no stale state inherited from the
///   bare carrier.

#include "G4CMPBareToPolaron.hh"
#include "G4CMPDriftElectron.hh"
#include "G4CMPDriftHole.hh"
#include "G4CMPDriftElectronPolaron.hh"
#include "G4CMPDriftHolePolaron.hh"
#include "G4CMPDriftTrackInfo.hh"
#include "G4CMPTrackUtils.hh"
#include "G4CMPUtils.hh"
#include "G4DynamicParticle.hh"
#include "G4ParticleChange.hh"
#include "G4ParticleDefinition.hh"
#include "G4PhysicalConstants.hh"
#include "G4Step.hh"
#include "G4StepPoint.hh"
#include "G4SystemOfUnits.hh"
#include "G4Track.hh"
#include "G4VParticleChange.hh"
#include "G4Exception.hh"
#include "G4LatticePhysical.hh"
#include "G4CMPSecondaryUtils.hh"
#include "G4RandomDirection.hh"
#include "Randomize.hh"
#include "G4RunManager.hh"
#include "G4Event.hh"
#include <iostream>
#include <fstream>
#include <cmath>
#include <vector>

// ─── Phonon emission helper (file-local) ─────────────────────────────
//
// Mirrors G4CMPPolaronFormation::EmitPolaronPhonons(): the binding energy
// Delta released during polaron formation is carried away by a number of
// LO phonons, num_phonons = round((Delta * efficiency) / hbar*omega).
// This helper only *builds* the phonon tracks (does not call AddSecondary)
// so that the caller can call SetNumberOfSecondaries() with the correct
// total count (1 polaron + N phonons) before adding any secondaries, as
// required by G4ParticleChange.
//
// NOTE (energy bookkeeping caveat): E_final for the polaron secondary is
// computed elsewhere as Ekin + Delta - hbar*omega (i.e. it already gains
// +Delta). Emitting ~Delta worth of phonons here ADDS that energy again
// rather than balancing it -- this mirrors the user-selected behavior
// ("tie phonon count to binding energy Delta", matching the reference
// EmitPolaronPhonons(aTrack, FORMATION_ENERGY) call), but it means this
// process is not strictly energy-conserving as currently combined. Flagged
// here for future revisiting.
namespace {

// Fraction of the binding energy Delta that is actually carried off as LO
// phonons. The reference implementation reads this from
// G4CMPConfigManager::GetPolaronPhononEfficiency(), but that getter does not
// exist in this build's G4CMPConfigManager -- so it's hardcoded here instead
// (1.0 == 100%, matching the reference's default behavior). Adjust this
// single constant if a different efficiency is desired.
constexpr G4double kPolaronPhononEfficiency = 1.0;

std::vector<G4Track*> EmitBareToPolaronPhonons(const G4Track& aTrack,
                                                G4double formationEnergy,
                                                G4double phononEnergy,
                                                const G4LatticePhysical* lat) {
  std::vector<G4Track*> phonons;

  if (!lat || phononEnergy <= 0.) return phonons;

  G4double efficiency = kPolaronPhononEfficiency;
  G4double effectiveEnergy = formationEnergy * efficiency;

  G4int numPhonons = (G4int)(effectiveEnergy / phononEnergy + 0.5);

  G4cout << "[BareToPolaron]   -> EmitPolaronPhonons: Delta="
         << formationEnergy/eV << " eV, efficiency=" << efficiency
         << ", effectiveEnergy=" << effectiveEnergy/eV << " eV"
         << ", hbar*omega=" << phononEnergy/eV << " eV"
         << " -> numPhonons=" << numPhonons << G4endl;

  if (numPhonons <= 0) return phonons;

  phonons.reserve(numPhonons);

  for (G4int i = 0; i < numPhonons; i++) {
    G4ThreeVector direction = G4RandomDirection();

    G4double rnd   = G4UniformRand();
    G4double LDOS  = lat->GetLDOS();
    G4double STDOS = lat->GetSTDOS();
    G4double FTDOS = lat->GetFTDOS();
    G4double totalDOS = LDOS + STDOS + FTDOS;

    G4int mode = 0;  // default: longitudinal
    if (totalDOS > 0) {
      G4double pLong = LDOS  / totalDOS;
      G4double pST   = STDOS / totalDOS;
      if (rnd < pLong) mode = 0;             // Longitudinal
      else if (rnd < pLong + pST) mode = 1;  // Slow transverse
      else mode = 2;                          // Fast transverse
    }

    G4Track* phonon = G4CMP::CreatePhonon(aTrack, mode, direction, phononEnergy,
                                           aTrack.GetGlobalTime(),
                                           aTrack.GetPosition());
    if (phonon) phonons.push_back(phonon);
  }

  G4cout << "[BareToPolaron]   -> EmitPolaronPhonons: built " << phonons.size()
         << " phonon secondaries" << G4endl;

  return phonons;
}

}  // namespace

// ─── Constructor and destructor ──────────────────────────────────────

G4CMPBareToPolaron::G4CMPBareToPolaron(const G4String& aName)
  : G4VDiscreteProcess(aName, fUserDefined),
    fThresholdEnergy(0.048 * eV),  // Lowest LO mode in Sapphire
    fElectronPolaron(nullptr),
    fHolePolaron(nullptr),
    fPolaronsFormed(0),
    verboseLevel(0) {

  SetProcessSubType(901);  // Custom subtype for polaron formation

  if (verboseLevel > 0) {
    G4cout << GetProcessName() << " constructed with threshold = "
           << fThresholdEnergy/eV << " eV" << G4endl;
  }
}

G4CMPBareToPolaron::~G4CMPBareToPolaron() {
  if (verboseLevel > 0 && fPolaronsFormed > 0) {
    G4cout << GetProcessName() << " created " << fPolaronsFormed
           << " polarons" << G4endl;
  }
}

// ─── Check applicability ────────────────────────────────────────────

G4bool G4CMPBareToPolaron::IsApplicable(const G4ParticleDefinition& aPD) {
  // Only bare carriers, NOT polarons
  if (G4CMP::IsElectronPolaron(aPD) || G4CMP::IsHolePolaron(aPD)) {
    return false;  // REJECT polarons - don't process them
  }

  return (aPD == *G4CMPDriftElectron::Definition() ||
          aPD == *G4CMPDriftHole::Definition());
}

// ─── Main process: kill bare carrier, spawn polaron secondary ───────

G4VParticleChange* G4CMPBareToPolaron::PostStepDoIt(const G4Track& aTrack,
                                                    const G4Step& aStep) {
  G4ParticleChange* pChange = new G4ParticleChange();
  pChange->Initialize(aTrack);

  G4cout << "[BareToPolaron] PostStepDoIt ENTERED: particle="
         << aTrack.GetParticleDefinition()->GetParticleName()
         << " trackID=" << aTrack.GetTrackID()
         << " Ekin=" << aTrack.GetKineticEnergy()/eV << " eV"
         << " stepStatus=" << aStep.GetPostStepPoint()->GetStepStatus()
         << G4endl;

  // Don't do anything at boundary
  G4StepPoint* postStepPoint = aStep.GetPostStepPoint();
  if (postStepPoint->GetStepStatus() == fGeomBoundary ||
      postStepPoint->GetStepStatus() == fWorldBoundary) {
    G4cout << "[BareToPolaron]   -> SKIPPED (boundary step)" << G4endl;
    return pChange;
  }

  // Conversion happens unconditionally on the bare carrier's first step
  // (GetMeanFreePath always forces this process). fThresholdEnergy is no
  // longer used as a gate here -- kept as a constructor parameter only
  // for any logging/diagnostics that still reference it.
  const G4double Ekin = aTrack.GetKineticEnergy();

  // ========================================================================
  // DETERMINE POLARON TYPE
  // ========================================================================
  G4ParticleDefinition* polaronDef = nullptr;
  G4bool isElectron = false;

  if (IsBareElectron(aTrack.GetParticleDefinition())) {
    polaronDef = GetElectronPolaronDefinition();
    isElectron = true;
    G4cout << "[BareToPolaron]   -> Bare Electron -> Electron Polaron, E_k = "
           << Ekin/eV << " eV" << G4endl;
  } else if (IsBareHole(aTrack.GetParticleDefinition())) {
    polaronDef = GetHolePolaronDefinition();
    isElectron = false;
    G4cout << "[BareToPolaron]   -> Bare Hole -> Hole Polaron, E_k = "
           << Ekin/eV << " eV" << G4endl;
  } else {
    G4cout << "[BareToPolaron]   -> SKIPPED (not a bare electron or hole)"
           << G4endl;
    return pChange;
  }

  if (!polaronDef) {
    G4cout << "[BareToPolaron]   -> ERROR: polaronDef is null" << G4endl;
    G4Exception("G4CMPBareToPolaron::PostStepDoIt", "BareToPolaron001",
                JustWarning, "Failed to get polaron definition");
    return pChange;
  }

  // ========================================================================
  // GET LATTICE AND TRACK INFO (from the bare carrier, before it's killed)
  // ========================================================================
  auto trackInfo = G4CMP::GetTrackInfo<G4CMPDriftTrackInfo>(aTrack);
  if (!trackInfo) {
    G4cout << "[BareToPolaron]   -> ERROR: no G4CMPDriftTrackInfo on track"
           << G4endl;
    G4Exception("G4CMPBareToPolaron::PostStepDoIt", "BareToPolaron002",
                JustWarning, "Failed to get track info");
    return pChange;
  }

  const G4LatticePhysical* lat = trackInfo->Lattice();
  if (!lat) {
    G4cout << "[BareToPolaron]   -> ERROR: trackInfo->Lattice() is null"
           << G4endl;
    G4Exception("G4CMPBareToPolaron::PostStepDoIt", "BareToPolaron003",
                JustWarning, "Failed to get lattice");
    return pChange;
  }

  G4cout << "[BareToPolaron]   -> trackInfo OK, valley=" << trackInfo->ValleyIndex()
         << " lattice=" << (void*)lat << G4endl;

  // ========================================================================
  // ENERGY BUDGET: E_final = E_initial + Delta - hbar*omega
  // (unchanged from the previous version -- still needs the sign
  //  convention double-checked against the source physics model)
  // ========================================================================
  G4double bindingEnergy = 0.75 * eV;     // polaron binding energy
  G4double phononEnergy  = 0.03478 * eV;  // Maximum acoustic phonon energy
                                           // (only acoustic phonons are
                                           // simulated -- was 0.0597 eV
                                           // Eu-LO2 optical mode)
  G4double E_final = Ekin + bindingEnergy - phononEnergy;

  G4cout << "[BareToPolaron]   -> E_final = " << E_final/eV << " eV"
         << " (Ekin=" << Ekin/eV << " + Delta=" << bindingEnergy/eV
         << " - hw=" << phononEnergy/eV << ")" << G4endl;

  if (E_final <= 0.) {
    G4cout << "[BareToPolaron]   -> E_final <= 0, killing track with no polaron"
           << G4endl;
    // Not enough energy left to keep the carrier alive at all
    pChange->ProposeTrackStatus(fStopAndKill);
    pChange->ProposeLocalEnergyDeposit(Ekin);
    ClearNumberOfInteractionLengthLeft();
    return pChange;
  }

  // ========================================================================
  // MOMENTUM HANDLING (direction preserved, magnitude from E_final+newMass)
  // ========================================================================
  G4ThreeVector momentum = aTrack.GetMomentum();

  // ------------------------------------------------------------------------
  // POLARON MASS: m_polaron = m_bare * (1 + alpha/6), alpha = 1.2
  // (same formula as G4CMPStackingAction::SetPolaronMass).
  //
  // NOTE (2026-06-13): newMass previously came from
  // polaronDef->GetPDGMass(), the STATIC PDG mass baked into the polaron
  // particle definitions (G4CMPDriftElectronPolaron / G4CMPDriftHolePolaron),
  // which is 0.3*m_e for the electron polaron and 3.6*m_e for the hole
  // polaron. That static value disagreed with whatever
  // G4CMPStackingAction::SetPolaronMass assigned afterward as the track's
  // dynamic mass (especially for electrons: 0.3 vs 1.3, now vs the new
  // m_bare*1.2 enhancement), so the polaron's "kinematic mass" (p^2/2Ekin,
  // fixed at creation via pfinal below) did not match its assigned
  // transport mass.
  //
  // Computing newMass here directly from the lattice's isotropic bare-
  // carrier mass with the same enhancement factor used by SetPolaronMass
  // keeps both masses self-consistent, independent of whatever static mass
  // is hardcoded in the polaron particle definitions.
  // ------------------------------------------------------------------------
  const G4double alpha           = 1.2;
  const G4double massEnhancement = 1.0 + alpha / 6.0;  // = 1.2
  G4double m_bare  = isElectron ? lat->GetElectronMass() : lat->GetHoleMass();
  G4double newMass = m_bare * massEnhancement * c_squared;  // -> G4 [M]=[E] units

  G4ThreeVector pfinal;

  // Both carriers: isotropic p = sqrt(2*m*E), direction preserved.
  //
  // NOTE (2026-06-13): the electron branch previously used
  // lat->MapEkintoP(iValley, kdir, E_final), i.e. the lattice's anisotropic
  // valley effective mass, while the polaron's PDG mass (newMass, used by
  // G4DynamicParticle to compute Ekin(secondary)) was the fixed scalar
  // 1.3*m_e from SetPolaronMass(). Those two masses disagreed, so
  // Ekin(secondary) != E_final for electrons (see validation plots).
  //
  // For now (per user request) both electron and hole polarons are made
  // isotropic, using the same self-consistent formula as the hole branch:
  // this guarantees Ekin(secondary) == E_final for electrons too, exactly
  // as it already is for holes. A future revision should instead have the
  // polaron inherit the anisotropic mass from the parent electron/hole
  // valley (see header note), rather than using a fixed isotropic mass for
  // either carrier.
  G4double p_mag = std::sqrt(2.0 * newMass * E_final);
  pfinal = p_mag * momentum.unit();

  if (verboseLevel > 1) {
    G4double oldMass = aTrack.GetDefinition()->GetPDGMass();
    G4cout << "  Old mass: " << oldMass/electron_mass_c2 << " m_e"
           << " | New mass: " << newMass/electron_mass_c2 << " m_e" << G4endl;
    G4cout << "  Initial E_k: " << Ekin/eV << " eV"
           << " | Binding E: " << bindingEnergy/eV << " eV"
           << " | Phonon E: " << phononEnergy/eV << " eV" << G4endl;
    G4cout << "  Final E_k: " << E_final/eV << " eV"
           << " | p_final: " << pfinal.mag()/eV << " eV/c" << G4endl;
  }

  // ========================================================================
  // PHONON EMISSION: Delta (binding energy) is carried away by LO phonons,
  // num_phonons = round((Delta * efficiency) / hbar*omega) -- see helper
  // EmitBareToPolaronPhonons() above for the energy-bookkeeping caveat.
  // ========================================================================
  std::vector<G4Track*> phonons =
      EmitBareToPolaronPhonons(aTrack, bindingEnergy, phononEnergy, lat);

  // ========================================================================
  // KILL THE BARE CARRIER
  // ========================================================================
  pChange->ProposeTrackStatus(fStopAndKill);
  pChange->ProposeLocalEnergyDeposit(0.);  // energy is carried by the secondary
  pChange->SetNumberOfSecondaries(1 + (G4int)phonons.size());

  // ========================================================================
  // SPAWN THE POLARON AS A NEW SECONDARY TRACK
  // ========================================================================
  // G4DynamicParticle(definition, momentum-vector) sets the momentum
  // directly; total/kinetic energy are derived from p and the polaron mass.
  //
  // NOTE (2026-06-13): the constructor above initializes the dynamic
  // particle's internal mass from polaronDef->GetPDGMass() (the static
  // 0.3*m_e / 3.6*m_e baked into the particle definitions), and computes
  // Ekin(secondary) = p^2/(2*PDGMass) from THAT mass -- not from our
  // newMass (m_bare*1.2) used to build pfinal above. So Ekin(secondary)
  // and the "kinematic mass" p^2/(2*Ekin) still reflected the old static
  // PDG mass even after pfinal was fixed.
  //
  // Fix: explicitly set the dynamic particle's mass to newMass, then
  // re-apply SetMomentum(pfinal) so Ekin is recomputed from p using
  // newMass. This makes Ekin(secondary) == E_final and kinematic mass
  // == newMass == m_bare*1.2, for both carriers.
  G4DynamicParticle* polaronDP = new G4DynamicParticle(polaronDef, pfinal);
  polaronDP->SetMass(newMass);
  polaronDP->SetMomentum(pfinal);  // recompute Ekin using newMass

  G4Track* polaronTrack = new G4Track(polaronDP,
                                       aTrack.GetGlobalTime(),
                                       aTrack.GetPosition());
  polaronTrack->SetTouchableHandle(aTrack.GetTouchableHandle());

  // Give the polaron a fresh, correct track info right away:
  //   - same lattice as the parent carrier
  //   - ValleyIndex() == -1 (polarons are valley-less)
  // This is belt-and-suspenders alongside G4CMPStackingAction's own
  // polaron dispatch in ClassifyNewTrack -- harmless if that also runs.
  G4CMP::AttachTrackInfo(*polaronTrack, new G4CMPDriftTrackInfo(lat, -1));

  pChange->AddSecondary(polaronTrack);

  G4cout << "[BareToPolaron]   -> SECONDARY CREATED: "
         << polaronDP->GetDefinition()->GetParticleName()
         << " trackID(parent)=" << aTrack.GetTrackID()
         << " pfinal=" << pfinal.mag()/eV << " eV/c"
         << " Ekin(secondary)=" << polaronDP->GetKineticEnergy()/eV << " eV"
         << " pos=" << aTrack.GetPosition()/mm << " mm"
         << G4endl;
  G4cout << "[BareToPolaron]   -> KILLING parent track " << aTrack.GetTrackID()
         << G4endl;

  // ========================================================================
  // ADD THE PHONON SECONDARIES (built above, before SetNumberOfSecondaries)
  // ========================================================================
  for (G4Track* phonon : phonons) {
    pChange->AddSecondary(phonon);
  }
  if (!phonons.empty()) {
    G4cout << "[BareToPolaron]   -> ADDED " << phonons.size()
           << " phonon secondaries (LO, hw=" << phononEnergy/eV << " eV each)"
           << G4endl;
  }

  // ========================================================================
  // RECORD THIS FORMATION EVENT TO A CSV FILE (for validation plots)
  // G4cmp is built as its own library target and does not link against
  // Geant4's analysis module, so G4AnalysisManager isn't available here.
  // Instead, append a plain CSV row -- one per bare-carrier -> polaron
  // conversion -- which ROOT can read directly via TTree::ReadFile(), or
  // which can be loaded with pandas/matplotlib for the same plots.
  // ========================================================================
  {
    G4int eventID = G4RunManager::GetRunManager()->GetCurrentEvent()
                        ? G4RunManager::GetRunManager()->GetCurrentEvent()->GetEventID()
                        : -1;
    G4ThreeVector pos = aTrack.GetPosition();

    const char* csvName = "polaron_formation.csv";

    // Write the header only once: if the file doesn't exist yet or is empty.
    std::ifstream checkFile(csvName);
    G4bool needHeader = !checkFile.good() || checkFile.peek() == std::ifstream::traits_type::eof();
    checkFile.close();

    std::ofstream csvFile(csvName, std::ios::app);
    if (csvFile.is_open()) {
      if (needHeader) {
        csvFile << "EventID,ParentTrackID,Particle,x_cm,y_cm,z_cm,time_ns,"
                   "Ekin0_eV,Delta_eV,hbarOmega_eV,Efinal_eV,EkinPolaron_eV,"
                   "pFinal_eV,NumPhonons"
                << std::endl;
      }
      csvFile << eventID << ","
              << aTrack.GetTrackID() << ","
              << polaronDP->GetDefinition()->GetParticleName() << ","
              << pos.x()/CLHEP::cm << ","
              << pos.y()/CLHEP::cm << ","
              << pos.z()/CLHEP::cm << ","
              << aTrack.GetGlobalTime()/CLHEP::ns << ","
              << Ekin/eV << ","
              << bindingEnergy/eV << ","
              << phononEnergy/eV << ","
              << E_final/eV << ","
              << polaronDP->GetKineticEnergy()/eV << ","
              << pfinal.mag()/eV << ","
              << (G4int)phonons.size()
              << std::endl;
      csvFile.close();
    }
  }

  fPolaronsFormed++;

  return pChange;
}

// ─── Get polaron definitions ────────────────────────────────────────

G4ParticleDefinition* G4CMPBareToPolaron::GetElectronPolaronDefinition() {
  if (!fElectronPolaron) {
    fElectronPolaron = G4CMPDriftElectronPolaron::Definition();
  }
  return fElectronPolaron;
}

G4ParticleDefinition* G4CMPBareToPolaron::GetHolePolaronDefinition() {
  if (!fHolePolaron) {
    fHolePolaron = G4CMPDriftHolePolaron::Definition();
  }
  return fHolePolaron;
}

// ─── Carrier type checks ────────────────────────────────────────────

G4bool G4CMPBareToPolaron::IsBareElectron(const G4ParticleDefinition* pd) const {
  return (pd == G4CMPDriftElectron::Definition());
}

G4bool G4CMPBareToPolaron::IsBareHole(const G4ParticleDefinition* pd) const {
  return (pd == G4CMPDriftHole::Definition());
}

// ─── Get mean free path (for G4VDiscreteProcess) ───────────────────

G4double G4CMPBareToPolaron::GetMeanFreePath(
    const G4Track& aTrack, G4double,
    G4ForceCondition* condition) {

  // REJECT polarons - they don't form polarons again
  if (G4CMP::IsElectronPolaron(aTrack.GetParticleDefinition()) ||
      G4CMP::IsHolePolaron(aTrack.GetParticleDefinition())) {
    *condition = NotForced;
    return DBL_MAX;
  }

  // Bare carrier -> force immediate conversion on its very first step,
  // regardless of its current kinetic energy. PostStepDoIt kills this
  // track (fStopAndKill) and spawns a polaron secondary, so this track
  // can never take a second step and never re-trigger this process.
  *condition = Forced;
  return 0.;
}
