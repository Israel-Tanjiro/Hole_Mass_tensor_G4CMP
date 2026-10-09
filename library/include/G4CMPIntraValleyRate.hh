/***********************************************************************\
 * This software is licensed under the terms of the GNU General Public *
 * License version 3 or later. See G4CMP/LICENSE for the full license. *
\***********************************************************************/

/// \file library/include/G4CMPIntraValleyRate.hh
/// \brief Compute intravalley scattering rate for single-valley materials
///
/// For single-valley materials like Sapphire (Al₂O₃):
/// - Bare carriers: Basic intravalley scattering (if defined)
/// - Polaron carriers: Three mechanisms via Mathiessen's rule
///   Γ_total = Γ_acoustic + Γ_optical + Γ_impurity
///
/// Detects polaron state via G4CMPPolaronInfo user track information.

#ifndef G4CMPIntraValleyRate_hh
#define G4CMPIntraValleyRate_hh 1

#include "G4CMPVScatteringRate.hh"
#include "G4Track.hh"

// Forward declarations
class G4CMPAcousticScatteringRate;
class G4CMPOpticalScatteringRate;
class G4CMPImpurityScatteringRate;

class G4CMPIntraValleyRate : public G4CMPVScatteringRate {
public:
  G4CMPIntraValleyRate(const G4String& name = "IntraValley");
  virtual ~G4CMPIntraValleyRate();

  // Main interface
  virtual G4double Rate(const G4Track& aTrack) const;
  virtual G4double Threshold(G4double Eabove=0.) const;

  // Load material parameters from lattice
  virtual void LoadDataForTrack(const G4Track* track,
                                const G4bool overrideMomentumReset=false);

  // Component rates (for diagnostics)
  G4double AcousticRate(const G4Track& aTrack) const;
  G4double OpticalRate(const G4Track& aTrack) const;
  G4double ImpurityRate(const G4Track& aTrack) const;

  // Polaron detection
  G4bool IsPolaronTrack(const G4Track& aTrack) const;
  G4bool IsElectron(const G4ParticleDefinition* particle) const;
  G4bool IsHole(const G4ParticleDefinition* particle) const;

  // Set verbosity
  virtual void SetVerboseLevel(G4int vb);

protected:
  G4CMPAcousticScatteringRate* fAcousticRate;
  G4CMPOpticalScatteringRate*  fOpticalRate;
  G4CMPImpurityScatteringRate* fImpurityRate;

  G4int fVerboseLevel;

  // Material parameters (loaded from lattice)
  G4double fDensity;
  G4double fTemperature;
  G4double fKT;  // k_B * T
};

#endif  /* G4CMPIntraValleyRate_hh */
