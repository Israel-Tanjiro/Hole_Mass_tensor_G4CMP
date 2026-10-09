/***********************************************************************\
 * This software is licensed under the terms of the GNU General Public *
 * License version 3 or later. See G4CMP/LICENSE for the full license. *
\***********************************************************************/

/// \file library/src/G4CMPIntraValleyRate.cc
/// \brief Implementation of intravalley scattering rate for polarons (low-T, no E-field)
///
/// At low temperature with NO electric field:
/// - Acoustic scattering handled by Luke process (when E-field applied)
/// - Here: only Optical (Fröhlich) + Impurity (Conwell-Weisskopf)
/// Γ_total = Γ_optical + Γ_impurity (Mathiessen's rule)

#include "G4CMPIntraValleyRate.hh"
#include "G4CMPAcousticScatteringRate.hh"
#include "G4CMPOpticalScatteringRate.hh"
#include "G4CMPImpurityScatteringRate.hh"
#include "G4CMPPolaronInfo.hh"
#include "G4CMPDriftElectron.hh"
#include "G4CMPDriftHole.hh"
#include "G4CMPUtils.hh"
#include "G4PhysicalConstants.hh"
#include "G4SystemOfUnits.hh"
#include "G4Track.hh"
#include <algorithm>
#include <iostream>
#include <iomanip>

// ============================================================================
// CONSTRUCTOR AND DESTRUCTOR
// ============================================================================

G4CMPIntraValleyRate::G4CMPIntraValleyRate(const G4String& name)
  : G4CMPVScatteringRate(name),
    fAcousticRate(nullptr),
    fOpticalRate(nullptr),
    fImpurityRate(nullptr),
    fVerboseLevel(0) {

  // Create scattering rate objects (acoustic handled by Luke process with E-field)
  // For low-T, no E-field polaron thermalization: only optical + impurity
  fOpticalRate = new G4CMPOpticalScatteringRate("OpticalIntraValley");
  fImpurityRate = new G4CMPImpurityScatteringRate("ImpurityIntraValley");
  // fAcousticRate = nullptr;  // Luke scattering handles acoustic when E-field present

  if (fVerboseLevel > 0) {
    G4cout << "G4CMPIntraValleyRate constructed (low-T polaron mode):" << G4endl;
    G4cout << "  - Optical (Fröhlich) — primary at low-T" << G4endl;
    G4cout << "  - Impurity (Conwell-Weisskopf) — secondary" << G4endl;
    G4cout << "  - Acoustic (handled by Luke) — disabled, no E-field" << G4endl;
  }
}

G4CMPIntraValleyRate::~G4CMPIntraValleyRate() {
  if (fAcousticRate) delete fAcousticRate;
  if (fOpticalRate) delete fOpticalRate;
  if (fImpurityRate) delete fImpurityRate;
}

// ============================================================================
// POLARON DETECTION
// ============================================================================

G4bool G4CMPIntraValleyRate::IsPolaronTrack(const G4Track& aTrack) const {
  const G4VUserTrackInformation* userInfo = aTrack.GetUserInformation();
  if (!userInfo) return false;

  const G4CMPPolaronInfo* polaronInfo =
    dynamic_cast<const G4CMPPolaronInfo*>(userInfo);

  return (polaronInfo && polaronInfo->IsPolaron());
}

// ============================================================================
// LOAD MATERIAL DATA
// ============================================================================

void G4CMPIntraValleyRate::LoadDataForTrack(const G4Track* track,
                                             const G4bool /*overrideMomentumReset*/) {
  if (!track) return;

  // Load data in optical and impurity rate objects only
  // Acoustic is handled by Luke process (not used in low-T, no E-field mode)
  if (fOpticalRate)
    const_cast<G4CMPOpticalScatteringRate*>(fOpticalRate)->LoadDataForTrack(track);
  if (fImpurityRate)
    const_cast<G4CMPImpurityScatteringRate*>(fImpurityRate)->LoadDataForTrack(track);
}

// ============================================================================
// RATE CALCULATIONS
// ============================================================================

G4double G4CMPIntraValleyRate::AcousticRate(const G4Track& aTrack) const {
  if (!fAcousticRate) return 0.;
  return fAcousticRate->Rate(aTrack);
}

G4double G4CMPIntraValleyRate::OpticalRate(const G4Track& aTrack) const {
  if (!fOpticalRate) return 0.;
  return fOpticalRate->Rate(aTrack);
}

G4double G4CMPIntraValleyRate::ImpurityRate(const G4Track& aTrack) const {
  if (!fImpurityRate) return 0.;
  return fImpurityRate->Rate(aTrack);
}

G4double G4CMPIntraValleyRate::Rate(const G4Track& aTrack) const {
  /// Total scattering rate for low-T polaron thermalization (NO E-field):
  /// Γ_total = Γ_optical + Γ_impurity  (Mathiessen's rule)
  ///
  /// Acoustic handled separately by Luke process when E-field applied.
  /// Only applies to polaron carriers (marked with G4CMPPolaronInfo).

  // Check if this is a polaron track
  if (!IsPolaronTrack(aTrack)) {
    return 0.;  // Not a polaron
  }

  // Compute optical and impurity mechanisms ONLY (acoustic → Luke with E-field)
  G4double Γ_opt = OpticalRate(aTrack);
  G4double Γ_imp = ImpurityRate(aTrack);
  G4double Γ_total = Γ_opt + Γ_imp;

  // ════════════════════════════════════════════════════════════════════════════
  // ENHANCED DEBUG: Track polaron energy evolution
  // ════════════════════════════════════════════════════════════════════════════
  static G4int call_count = 0;
  ++call_count;

  G4double Ekin = aTrack.GetKineticEnergy();
  G4double Ekin_meV = Ekin / eV * 1000.0;  // Convert to meV for clarity
  G4double threshold_meV = 91.1;  // LO emission threshold in meV
  G4bool above_threshold = (Ekin_meV > threshold_meV);

  G4cout << "\n╔════════════════════════════════════════════════════════════════╗" << G4endl;
  G4cout << "║ POLARON ENERGY EVOLUTION [Call #" << call_count << "]" << G4endl;
  G4cout << "╠════════════════════════════════════════════════════════════════╣" << G4endl;
  G4cout << "║ Kinetic Energy: " << std::setw(8) << std::setprecision(4) << Ekin_meV
         << " meV (" << Ekin / eV << " eV)" << G4endl;
  G4cout << "║ LO Threshold:   " << std::setw(8) << threshold_meV
         << " meV  |  Status: ";
  if (above_threshold) {
    G4cout << "✓ ABOVE threshold (optical active)" << G4endl;
  } else {
    G4cout << "✗ BELOW threshold (optical FROZEN)" << G4endl;
  }
  // Γ_opt, Γ_imp, Γ_total are in G4 internal rate units (1/ns).
  // Convert to Hz by dividing by "hertz" (=1e-9), i.e. Γ_Hz = Γ / hertz
  // (equivalently Γ * second). The previous code multiplied by hertz,
  // which is the inverse conversion and gave results 1e18x too small.
  G4double Γ_opt_Hz = Γ_opt / hertz;
  G4double Γ_imp_Hz = Γ_imp / hertz;
  G4double Γ_total_Hz = Γ_total / hertz;

  G4cout << "╠════════════════════════════════════════════════════════════════╣" << G4endl;
  G4cout << "║ Scattering Rates (at T=1K):" << G4endl;
  G4cout << "║   Γ_optical  = " << std::setw(12) << std::scientific << std::setprecision(3)
         << Γ_opt_Hz << " Hz  (" << Γ_opt_Hz/1e3 << " kHz)" << G4endl;
  G4cout << "║   Γ_impurity = " << std::setw(12) << std::scientific << std::setprecision(3)
         << Γ_imp_Hz << " Hz  (" << Γ_imp_Hz/1e3 << " kHz)" << G4endl;
  G4cout << "║   ─────────────────────────────────" << G4endl;
  G4cout << "║   Γ_total   = " << std::setw(12) << std::scientific << std::setprecision(3)
         << Γ_total_Hz << " Hz  (" << Γ_total_Hz/1e3 << " kHz)" << G4endl;
  G4cout << "╠════════════════════════════════════════════════════════════════╣" << G4endl;

  // Identify dominant mechanism
  G4double maxRate = std::max(Γ_opt, Γ_imp);
  G4String dominant;
  if (Γ_opt == maxRate && Γ_opt > 0) {
    dominant = "OPTICAL (phonon emission)";
  } else if (Γ_imp == maxRate && Γ_imp > 0) {
    dominant = "IMPURITY (elastic)";
  } else {
    dominant = "NONE (polaron trapped!)";
  }

  G4cout << "║ Dominant:      " << dominant << G4endl;
  // Mean free path: λ = v/Γ_total directly gives mm (G4 internal length unit)
  // -- no hertz factor needed (see GetMeanFreePath for derivation).
  G4cout << "║ Mean free path: " << std::setw(10) << (1.1e4 * m/s) / Γ_total / micrometer
         << " µm" << G4endl;
  // Scattering time: τ = 1/Γ_total directly gives ns (G4 internal time unit);
  // divide by picosecond to display in ps.
  G4cout << "║ Scattering time: " << std::setw(10) << (1.0 / Γ_total) / picosecond
         << " ps" << G4endl;
  G4cout << "╚════════════════════════════════════════════════════════════════╝\n" << G4endl;

  return Γ_total;
}

// ============================================================================
// THRESHOLD IDENTIFICATION
// ============================================================================

G4double G4CMPIntraValleyRate::Threshold(G4double Eabove) const {
  if (!fOpticalRate) return 0.;
  return fOpticalRate->Threshold(Eabove);
}

// ============================================================================
// DIAGNOSTICS
// ============================================================================

void G4CMPIntraValleyRate::SetVerboseLevel(G4int vb) {
  fVerboseLevel = vb;

  // Acoustic not used in low-T mode (handled by Luke with E-field)
  if (fOpticalRate) fOpticalRate->SetVerboseLevel(vb);
  if (fImpurityRate) fImpurityRate->SetVerboseLevel(vb);
}
