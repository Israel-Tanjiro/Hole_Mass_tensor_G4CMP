# G4CMP Polaron Transport — Version 3: Full Scattering Suite

## Overview

This branch builds on V2 (polaron particle types) and adds the complete polaron transport
physics: a bare-carrier → polaron conversion process with momentum conservation and phonon
emission, plus three independent intravalley scattering mechanisms (optical, acoustic,
impurity) combined via Matthiessen's rule, along with critical unit-convention fixes that
make the scattering processes physically active.

---

## New Files (relative to V2 / `v2-polaron-particle-types`)

### Bare Carrier → Polaron Conversion

| File | Description |
|------|-------------|
| `library/src/G4CMPBareToPolaron.cc` | Discrete process on `G4CMPDriftElectron` / `G4CMPDriftHole`. When the carrier's Ekin drops to the lowest LO phonon threshold (48 meV in sapphire), kills the bare carrier and spawns a `G4CMPDriftElectronPolaron` or `G4CMPDriftHolePolaron` secondary. Kill-and-replace pattern preserves momentum: `E'_k = p²/(2 m_polaron)` where `p = √(2 m_bare Ekin)`. |
| `library/include/G4CMPBareToPolaron.hh` | Header |

### Scattering Rate Models — Matthiessen's Rule
`Γ_total = Γ_acoustic + Γ_optical + Γ_impurity`

| File | Mechanism | Key physics |
|------|-----------|-------------|
| `library/src/G4CMPOpticalScatteringRate.cc` | Optical (Fröhlich/Callen) | 6-mode LO phonon coupling for sapphire. Per mode: `Γ = π α_F ω F(u)(1 + n_B)` where `F(u) = (2/√u) ln(√u + √(u−1))` is the Callen function, `n_B` the Bose–Einstein population, `u = Ekin/ħω`. |
| `library/src/G4CMPAcousticScatteringRate.cc` | Acoustic (deformation potential) | Brooks–Herring acoustic scattering. |
| `library/src/G4CMPImpurityScatteringRate.cc` | Impurity (Coulomb, elastic) | Brooks–Herring ionized-impurity: `Γ ∝ N_i F(b) E^(−3/2)`. `ΔE = 0` (elastic). |
| `library/src/G4CMPIntraValleyRate.cc` | Aggregator | Sums all three; gated on `IsPolaronTrack()`. |
| `library/src/G4CMPIntraValleyScattering.cc` | Geant4 process | Computes MFP from `G4CMPIntraValleyRate`; derives from `G4CMPVDriftProcess`. |

### Scattering Event
| File | Description |
|------|-------------|
| `library/src/G4CMPPolaronScattering.cc` | Master discrete process: calls `G4CMPPolaronScatteringRate`, selects mechanism, applies energy loss and angular deflection. MFP = `v_polaron / Γ_total` (both in G4 internal units). |
| `library/src/G4CMPPolaronScatteringRate.cc` | Wraps rate models; provides `ChooseScatteringMechanism()`. |
| `library/src/G4CMPPolaronScatteringKinematics.cc` | Samples scattering angles: isotropic (optical), forward-peaked (acoustic/impurity). |

### Other Transport
| File | Description |
|------|-------------|
| `library/src/G4CMPPolaronFormation.cc` | Alternative formation process (mass-enhancement approach, `m* → m*(1+α/6)`). |
| `library/src/G4CMPPolaronBoundaryProcess.cc` | Polaron behavior at crystal boundaries. |
| `library/src/G4CMPPolaronHoppingTransport.cc` | Hopping transport (small-polaron regime). |

---

## Sapphire (Al₂O₃) LO Mode Parameters — Heinz et al. PRL 90, 247401 (2003)

| Mode     | ħω (meV) | Fröhlich α |
|----------|----------|-----------|
| Eu-LO1   | 48.0     | 0.0261    |
| Eu-LO2   | 59.7     | 0.5203    |
| A2u-LO1  | 63.3     | 0.0038    |
| Eu-LO3   | 78.1     | 0.0053    |
| A2u-LO2  | 109.0    | 0.3526    |
| Eu-LO4   | 112.0    | 0.4521    |

Dominant contribution at low energy (near threshold): Eu-LO2 (59.7 meV, α = 0.5203).
Dominant at high energy: A2u-LO2 + Eu-LO4 (109/112 meV modes, large α, F(u) still finite).

---

## Critical Unit-Convention Fixes

All rates are returned in **G4 internal units of 1/ns** — because they are built from
`ħω / ħ_Planck` where both quantities are in G4's internal MeV·ns units. The correct
conversion rules, and bugs that were fixed:

```
Γ_Hz  = Γ_total / hertz       (hertz = 1/second = 1e-9 in G4 internal units)
λ_mm  = v_polaron / Γ_total   (v in mm/ns, Γ in 1/ns → λ in mm, NO hertz factor)
τ_ps  = (1/Γ_total) / picosecond
```

### Bugs fixed in this version

| File | Bug | Impact | Fix |
|------|-----|--------|-----|
| `G4CMPPolaronScattering.cc` | `lambda = v / (Γ * hertz)` — multiplied by hertz (=1e−9) instead of using Γ directly | MFP inflated 10⁹×: 3.5 nm → 3.5 m at 1 MeV. Scattering **never triggered** in any real geometry. | `lambda = v_polaron / Γ_total` |
| `G4CMPIntraValleyScattering.cc` | `Γ * hertz` → λ inflated 10¹⁸× | Same effect on intravalley process | `v / Γ_total` directly |
| `G4CMPIntraValleyScattering.cc` | Hardcoded `return lambda = 0.00001*m` bypassing all physics | MFP always 10 μm regardless of rate | Removed; correct `lambda` returned |
| `G4CMPIntraValleyRate.cc` | `Γ * hertz` in diagnostic prints | Wrong Hz values displayed (10¹⁸× too small) | Changed to `Γ / hertz` |
| `G4CMPOpticalScatteringRate.cc` | Duplicate no-arg constructor conflicting with `(const G4String& name="...")` declaration | Compile error | Removed duplicate |
| `G4CMPOpticalScatteringRate.cc` | `CallenF()`/`BosePopulation()` defined with `const` but declared `static` | Compile error | Removed `const` |
| `G4CMPAcousticScatteringRate.cc` | Duplicate constructor + `SetVerboseLevel` redefinition | Compile error | Removed both |
| `G4CMPImpurityScatteringRate.cc` | Same | Compile error | Same fix |
| `G4CMPIntraValleyScattering.hh` | Base class declared as `G4CMPVProcess` but `.cc` uses `G4CMPVDriftProcess`-only members (`InitializeParticleChange`, `GetValleyIndex`, `aParticleChange`, etc.) | Compile error | Changed to `G4CMPVDriftProcess` |

### Verified rate values (independent Python calculation)

| Ekin | Dominant mechanism | Γ_total | λ (v = 11,000 m/s) | τ |
|------|-------------------|---------|---------------------|---|
| 1 MeV | Optical (6-mode Fröhlich, u~10⁷) | 3153 /ns | **3.5 nm** | 0.32 ps |
| 0.01 eV | Impurity (below 48 meV LO threshold) | 0.012 /ns | **0.92 mm** | 84 ns |

---

## Debug Instrumentation

Enable with `process->SetVerboseLevel(1)` on the `G4CMPPolaronScattering` process.
Prints once per distinct Ekin encountered:

```
=== G4CMPPolaronScattering::GetMeanFreePath DEBUG ===
  Particle              = G4CMPDriftHolePolaron
  Ekin                  = 0.01 eV
  Gamma_total (raw)     = 0.0119 /ns
  Gamma_total / hertz   = 1.19e+07 Hz
  v_polaron             = 0.011 mm/ns (11000 m/s)
  lambda = v/Gamma      = 0.923 mm  (9.23e-04 m)
======================================================
```

---

## Known Limitations / Open Items

- `v_polaron` in `G4CMPPolaronScattering` is hardcoded to `1.1×10⁴ m/s`. Physically it
  should be `v = sqrt(2 Ekin / m_eff)`, making λ energy-dependent even at fixed Γ.
- `E_loss` per mechanism is fixed (optical: 59.7 meV, acoustic: 1 meV). Should be sampled
  from the phonon energy distribution.
- Acoustic scattering contributes negligibly at tested energies; review against model
  validity range.

---

## Version History Summary

| Branch | Physics |
|--------|---------|
| `hole-mass-tensor-impl` (V1) | Hole mass tensor. `G4CMPPolaronFormation` marks existing electrons/holes as polaron-like with mass enhancement `m*(1+α/6)` + phonon emission, no new particle types. |
| `v2-polaron-particle-types` (V2) | Adds `G4CMPDriftElectronPolaron` / `G4CMPDriftHolePolaron` as distinct G4 particle types with polaron mass (0.3 m_e). Standard drift transport. No scattering processes. |
| `v3-full-polaron-scattering` (V3, this branch) | `G4CMPBareToPolaron` converts bare carriers to V2 polaron types with momentum conservation + phonon emission. Full intravalley scattering (optical/acoustic/impurity) + all unit/compile fixes. |

*Work with Claude (Anthropic) — June–October 2026.*
