/***********************************************************************\
 * Polaron Formation Process Implementation
 * Physics: mass enhancement + energy conservation (Δ - ℏω)
 ***********************************************************************/
#include "G4CMPPolaronFormation.hh"
#include "G4CMPPolaronInfo.hh"
#include "G4CMPDriftElectron.hh"
#include "G4CMPDriftHole.hh"
#include "G4LatticeLogical.hh"
#include "G4Track.hh"
#include "G4Step.hh"
#include "G4StepPoint.hh"
#include "G4ParticleDefinition.hh"
#include "G4PhysicalConstants.hh"
#include "G4SystemOfUnits.hh"
#include "G4LatticePhysical.hh"
#include <iostream>
#include "G4LatticeManager.hh"
#include "G4CMPSecondaryUtils.hh"
#include "G4RandomDirection.hh"
#include "G4CMPConfigManager.hh"
#include "G4CMPDriftTrackInfo.hh"
#include "G4CMPTrackUtils.hh"

// ══════════════════════════════════════════════════════════════════════
// STATIC MEMBER INITIALIZATION
// ══════════════════════════════════════════════════════════════════════

G4double G4CMPPolaronFormation::ALPHA_ELECTRON = 1.2;
G4double G4CMPPolaronFormation::ALPHA_HOLE = 1.2;
G4double G4CMPPolaronFormation::FORMATION_ENERGY = 0.75;
G4double G4CMPPolaronFormation::HW_LO = 0.060;

// ══════════════════════════════════════════════════════════════════════
// CONSTRUCTOR AND DESTRUCTOR
// ══════════════════════════════════════════════════════════════════════

G4CMPPolaronFormation::G4CMPPolaronFormation(const G4String& aName)
  : G4VDiscreteProcess(aName, fUserDefined),
    fVerboseLevel(0),
    fPolaronsFormed(0),
    fLattice(nullptr) {

  SetProcessSubType(323);

  if (fVerboseLevel > 0) {
    G4cout << "\n=== G4CMPPolaronFormation Initialized ===" << G4endl;
    G4cout << "  Formation Energy (Δ): " << FORMATION_ENERGY << " eV" << G4endl;
    G4cout << "  LO Phonon (ℏω): " << HW_LO << " eV" << G4endl;
    G4cout << "  Mass Enhancement (e-): (1 + " << ALPHA_ELECTRON << "/6) = "
           << GetMassEnhancementFactor(ALPHA_ELECTRON) << G4endl;
    G4cout << "  Mass Enhancement (h+): (1 + " << ALPHA_HOLE << "/6) = "
           << GetMassEnhancementFactor(ALPHA_HOLE) << G4endl;
    G4cout << "=======================================\n" << G4endl;
  }
}

G4CMPPolaronFormation::~G4CMPPolaronFormation() {
  if (fVerboseLevel > 0 && fPolaronsFormed > 0) {
    G4cout << "G4CMPPolaronFormation: Total polarons formed = "
           << fPolaronsFormed << G4endl;
  }
}

// ══════════════════════════════════════════════════════════════════════
// APPLICABILITY
// ══════════════════════════════════════════════════════════════════════

G4bool G4CMPPolaronFormation::IsApplicable(const G4ParticleDefinition& aPD) const {
  return (aPD == *G4CMPDriftElectron::Definition() ||
          aPD == *G4CMPDriftHole::Definition());
}

// ══════════════════════════════════════════════════════════════════════
// MEAN FREE PATH (force immediate conversion)
// ══════════════════════════════════════════════════════════════════════
G4double G4CMPPolaronFormation::GetMeanFreePath(
    const G4Track& aTrack,
    G4double previousStepSize,
    G4ForceCondition* condition) {

  static std::set<G4int> polaronFormed;
  G4int trackID = aTrack.GetTrackID();

  if (polaronFormed.count(trackID)) {
    *condition = NotForced;
    return DBL_MAX;
  }

  polaronFormed.insert(trackID);
  *condition = Forced;
  return 0.;
}

// ══════════════════════════════════════════════════════════════════════
// MAIN PROCESS: Form polaron (mass + energy + momentum update)
// ══════════════════════════════════════════════════════════════════════

G4VParticleChange* G4CMPPolaronFormation::PostStepDoIt(const G4Track& aTrack,
                                                       const G4Step& aStep) {
  aParticleChange.Initialize(aTrack);

  if (!G4CMPConfigManager::GetPolaronActive()) {
    return &aParticleChange;
  }

  G4StepPoint* postStepPoint = aStep.GetPostStepPoint();
  if (postStepPoint->GetStepStatus() == fGeomBoundary) {
    return &aParticleChange;
  }

  G4bool isElectron = (aTrack.GetParticleDefinition() ==
                       G4CMPDriftElectron::Definition());

  G4double initialKinE = aTrack.GetKineticEnergy();

  G4cout << "\n[PolaronFormation] " << (isElectron ? "Electron" : "Hole")
         << " at E_k = " << initialKinE/eV << " eV" << G4endl;

  // ========================================================================
  // ENERGY CONSERVATION: E_final = E_initial + Δ - ℏω
  // ========================================================================
  G4double bindingEnergy = FORMATION_ENERGY * eV;
  G4double phononEnergy = HW_LO * eV;
  G4double E_final = initialKinE + bindingEnergy - phononEnergy;

  if (E_final <= 0.) {
    aParticleChange.ProposeTrackStatus(fStopAndKill);
    aParticleChange.ProposeLocalEnergyDeposit(initialKinE);
    return &aParticleChange;
  }

  G4cout << "  E_initial: " << initialKinE/eV << " eV" << G4endl;
  G4cout << "  Δ (binding): " << bindingEnergy/eV << " eV" << G4endl;
  G4cout << "  ℏω (phonon): " << phononEnergy/eV << " eV" << G4endl;
  G4cout << "  E_final: " << E_final/eV << " eV" << G4endl;
  G4cout << "  ΔE: " << (E_final - initialKinE)/eV << " eV" << G4endl;

  // ========================================================================
  // GET LATTICE AND MOMENTUM
  // ========================================================================
  auto trackInfo = G4CMP::GetTrackInfo<G4CMPDriftTrackInfo>(aTrack);
  G4LatticePhysical* lattice = nullptr;
  if (trackInfo) {
    lattice = trackInfo->Lattice();
  }

  if (!lattice) {
    G4VPhysicalVolume* volume = aTrack.GetVolume();
    if (volume) {
      lattice = G4LatticeManager::GetLatticeManager()->GetLattice(volume);
    }
  }

  G4ThreeVector momentum = aTrack.GetMomentum();
  G4double oldMass = aTrack.GetDefinition()->GetPDGMass();
  G4double newMass = oldMass * GetMassEnhancementFactor(isElectron ? ALPHA_ELECTRON : ALPHA_HOLE);

  // ========================================================================
  // MOMENTUM: Use new mass and new energy to get final momentum
  // Direction preserved
  // ========================================================================
  G4ThreeVector pFinal = momentum.unit() * std::sqrt(2.0 * newMass * E_final);

  G4cout << "  m_bare: " << oldMass/electron_mass_c2 << " m_e" << G4endl;
  G4cout << "  m_polaron: " << newMass/electron_mass_c2 << " m_e" << G4endl;
  G4cout << "  p_mag before: " << momentum.mag()/eV << " eV/c" << G4endl;
  G4cout << "  p_mag after: " << pFinal.mag()/eV << " eV/c" << G4endl;

  // ========================================================================
  // UPDATE PARTICLE CHANGE
  // ========================================================================
  aParticleChange.ProposeEnergy(E_final);
  aParticleChange.ProposeMomentum(pFinal);

  // Mark as polaron
  G4double alpha = isElectron ? ALPHA_ELECTRON : ALPHA_HOLE;
  G4CMPPolaronInfo* polaronInfo = new G4CMPPolaronInfo(alpha);
  aTrack.SetUserInformation(polaronInfo);

  // Emit phonons
  EmitPolaronPhonons(aTrack, FORMATION_ENERGY);

  fPolaronsFormed++;

  G4cout << "[PolaronFormation] END\n" << G4endl;

  return &aParticleChange;
}

// ══════════════════════════════════════════════════════════════════════
// HELPER METHODS
// ══════════════════════════════════════════════════════════════════════

G4double G4CMPPolaronFormation::GetMassEnhancementFactor(G4double alpha) const {
  return 1.0 + alpha / 6.0;
}

void G4CMPPolaronFormation::UpdateCarrierMass(const G4Track& aTrack,
                                              G4bool isElectron) const {
  G4VPhysicalVolume* volume = aTrack.GetVolume();
  if (!volume) return;

  G4LatticePhysical* lattice = G4LatticeManager::GetLatticeManager()->GetLattice(volume);
  if (!lattice) {
    if (fVerboseLevel > 0) {
      G4cerr << "WARNING: No lattice found!" << G4endl;
    }
    return;
  }

  G4double alpha = isElectron ? ALPHA_ELECTRON : ALPHA_HOLE;
  G4double mass_factor = GetMassEnhancementFactor(alpha);

  if (isElectron) {
    G4double m_bare = lattice->GetElectronMass();
    G4double m_enhanced = m_bare * mass_factor;

    G4cout << "\n=== POLARON MASS CHANGE (ELECTRON) ===" << G4endl;
    G4cout << "  Bare mass:     " << m_bare / electron_mass_c2 << " m₀" << G4endl;
    G4cout << "  Enhanced mass: " << m_enhanced / electron_mass_c2 << " m₀" << G4endl;
    G4cout << "  Factor:        " << mass_factor << G4endl;
    G4cout << "=====================================\n" << G4endl;
  } else {
    G4double m_bare = lattice->GetHoleMass();
    G4double m_enhanced = m_bare * mass_factor;

    G4cout << "\n=== POLARON MASS CHANGE (HOLE) ===" << G4endl;
    G4cout << "  Bare mass:     " << m_bare / electron_mass_c2 << " m₀" << G4endl;
    G4cout << "  Enhanced mass: " << m_enhanced / electron_mass_c2 << " m₀" << G4endl;
    G4cout << "  Factor:        " << mass_factor << G4endl;
    G4cout << "==================================\n" << G4endl;
  }
}

// ══════════════════════════════════════════════════════════════════════
// RUNTIME CONFIGURATION
// ══════════════════════════════════════════════════════════════════════

void G4CMPPolaronFormation::SetPolaronFormationActive(G4bool active) {
  G4CMPConfigManager::SetPolaronActive(active);
}

G4bool G4CMPPolaronFormation::GetPolaronFormationActive() const {
  return G4CMPConfigManager::GetPolaronActive();
}

void G4CMPPolaronFormation::SetFormationEnergy(G4double energy) {
  G4CMPConfigManager::SetPolaronFormationEnergy(energy);
}

G4double G4CMPPolaronFormation::GetFormationEnergy() const {
  return G4CMPConfigManager::GetPolaronFormationEnergy();
}

void G4CMPPolaronFormation::SetPhononEfficiency(G4double eff) {
  G4CMPConfigManager::SetPolaronPhononEfficiency(eff);
}

G4double G4CMPPolaronFormation::GetPhononEfficiency() const {
  return G4CMPConfigManager::GetPolaronPhononEfficiency();
}

void G4CMPPolaronFormation::EmitPolaronPhonons(const G4Track& aTrack,
                                                G4double formation_energy) {
  G4double efficiency = G4CMPConfigManager::GetPolaronPhononEfficiency();
  G4double effectiveEnergy = formation_energy * efficiency;

  G4int num_phonons = (G4int)(effectiveEnergy / HW_LO + 0.5);

  G4cout << "\n=== PHONON EMISSION ===" << G4endl;
  G4cout << "  Formation energy: " << formation_energy << " eV" << G4endl;
  G4cout << "  Phonon efficiency: " << efficiency * 100 << "%" << G4endl;
  G4cout << "  Effective energy: " << effectiveEnergy << " eV" << G4endl;
  G4cout << "  Number of LO phonons: " << num_phonons << G4endl;

  if (num_phonons <= 0) {
    G4cout << "=======================\n" << G4endl;
    return;
  }

  G4LatticePhysical* lattice = G4LatticeManager::GetLatticeManager()->
                               GetLattice(aTrack.GetTouchable()->GetVolume());
  if (!lattice) {
    G4cout << "  [ERROR: No lattice]" << G4endl;
    return;
  }

  for (int i = 0; i < num_phonons; i++) {
    G4ThreeVector direction = G4RandomDirection();
    G4double phonon_energy = HW_LO * eV;

    G4double rand = G4UniformRand();
    G4double LDOS = lattice->GetLDOS();
    G4double STDOS = lattice->GetSTDOS();
    G4double FTDOS = lattice->GetFTDOS();
    G4double total_DOS = LDOS + STDOS + FTDOS;

    G4int mode = 0;
    if (total_DOS > 0) {
      G4double p_long = LDOS / total_DOS;
      G4double p_st = STDOS / total_DOS;

      if (rand < p_long) {
        mode = 0;
      } else if (rand < p_long + p_st) {
        mode = 1;
      } else {
        mode = 2;
      }
    }

    G4Track* phonon = G4CMP::CreatePhonon(aTrack,
                                          mode,
                                          direction,
                                          phonon_energy,
                                          aTrack.GetGlobalTime(),
                                          aTrack.GetPosition());

    if (phonon) {
      aParticleChange.AddSecondary(phonon);

      G4String mode_name[] = {"Long", "TransSlow", "TransFast"};
      G4cout << "  Phonon " << i+1 << ": E = " << HW_LO << " eV, "
             << "mode = " << mode_name[mode] << G4endl;
    }
  }

  G4cout << "  [" << num_phonons << " LO phonons emitted]" << G4endl;
  G4cout << "=======================\n" << G4endl;
}
