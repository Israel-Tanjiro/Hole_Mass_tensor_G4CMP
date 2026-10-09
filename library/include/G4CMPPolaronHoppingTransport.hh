/***********************************************************************\
 * This software is licensed under the terms of the GNU General Public *
 * License version 3 or later. See G4CMP/LICENSE for the full license. *
\***********************************************************************/

/// \file library/include/G4CMPPolaronHoppingTransport.hh
/// \brief Small polaron hopping transport via quantum diffusion
///
/// Based on: Storchak et al., Phys. Rev. B 56, 55 (1997)
/// "Quantum transport of electronic polarons in sapphire"
///
/// Implements temperature-dependent quantum diffusion with:
/// - Two-phonon mechanism at high temperature (T > Θ_D/10)
/// - One-phonon mechanism at low temperature (T < Θ_D/50)
/// - Bottleneck effect where slower process dominates
///
/// Physics: Quantum tunneling between lattice sites with:
/// D = D_scale × (normalized energy factors)
/// where D_scale = ℏ × a² / m*  [m²/s]

#ifndef G4CMPPolaronHoppingTransport_hh
#define G4CMPPolaronHoppingTransport_hh 1

#include "G4VDiscreteProcess.hh"

class G4CMPPolaronHoppingTransport : public G4VDiscreteProcess {
public:
  explicit G4CMPPolaronHoppingTransport(const G4String& processName =
                                         "PolaronHoppingTransport");
  virtual ~G4CMPPolaronHoppingTransport();

  // Process applicability (charge carriers)
  virtual G4bool IsApplicable(const G4ParticleDefinition& pd);

  // Mean free path from diffusion coefficient
  virtual G4double GetMeanFreePath(const G4Track& aTrack, G4double,
                                    G4ForceCondition* condition);

  // Transport step
  virtual G4VParticleChange* PostStepDoIt(const G4Track& aTrack,
                                           const G4Step& aStep);

  // Configuration
  void SetDebyeTemperature(G4double theta_D) { fTheta_D = theta_D; }
  void SetDielectricConstant(G4double eps_r) { fEpsilon_r = eps_r; }
  void SetEffectiveMass(G4double m_star_rel);
  void SetGroupVelocity(G4double v) { fGroupVelocity = v; }

  G4double GetDebyeTemperature() const { return fTheta_D; }
  G4double GetGroupVelocity() const { return fGroupVelocity; }

  // Get transport properties for validation
  G4double DiffusionTwoPhonon(G4double Omega, G4double T) const;
  G4double DiffusionOnePhonon(G4double Omega, G4double T) const;
  G4double DiffusionTotal(G4double Omega, G4double T) const;
  G4double MeanFreePathEquivalent(G4double D, G4double v) const;

private:
  // Physical parameters for sapphire (Al₂O₃)
  G4double fTheta_D;           // Debye temperature (K)
  G4double fTheta_D_J;         // Debye temperature in Joules
  G4double fEpsilon_r;         // Dielectric constant
  G4double fMass_eff_rel;      // Effective mass (relative to m_e)
  G4double fMass_eff;          // Absolute effective mass (kg)
  G4double fGroupVelocity;     // Polaron group velocity (m/s)
  G4double fE_T;               // Screening energy (J)
  G4double fE_T_eV;            // Screening energy (eV)

  // Phonon parameters (quantum diffusion) - in Joules
  G4double fOmega_0;           // Reference bandwidth (J)
  G4double fOmega_2ph;         // Two-phonon width (J)
  G4double fOmega_1ph;         // One-phonon width (J)
  G4double fC_1ph;             // One-phonon constant (dimensionless)

  // Diffusion scale normalization
  G4double fD_scale;           // D_scale = ℏ × a² / m*  [m²/s]

  // Particle change object
  G4ParticleChange aParticleChange;

  // Energy level shift between sites
  G4double EnergyShift(const G4Track& aTrack) const;

  // Diffusion coefficient calculation
  G4double ComputeOmega(const G4Track& aTrack) const;

  // Initialize sapphire parameters
  void InitializeSapphire();
  void ComputePrefactors();

  // Verbose output
  void ReportTransportProperties(const G4Track& aTrack, G4double D,
                                  G4double lambda) const;
};

#endif
