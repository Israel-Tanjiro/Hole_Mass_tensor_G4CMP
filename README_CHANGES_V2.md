# G4CMP Polaron Transport — Version 2: Polaron Particle Types (Renamed Electrons/Holes)

## Overview

This branch introduces **polaron particle types** into G4CMP by defining
`G4CMPDriftElectronPolaron` and `G4CMPDriftHolePolaron` as new entries in the Geant4
particle table — dressed-carrier versions of the standard drift electron and hole, carrying
the polaron effective mass from birth.

The physics approach here is: **treat polarons as renamed electrons and holes**. The
existing G4CMP drift and transport machinery works unchanged; we simply register new
particle types that have the correct polaron mass, so G4CMP's standard drift processes
(electric field transport, valley dynamics) automatically use the right mass when propagating
them. A separate creation process (not yet in this branch) converts bare carriers into these
polaron types at the appropriate energy threshold.

---

## New Files (relative to V1 / `hole-mass-tensor-impl`)

| File | Description |
|------|-------------|
| `library/src/G4CMPDriftElectronPolaron.cc` | Defines the electron polaron particle: mass = `0.3 × m_e`, charge = `−e`, stable. Registered in G4ParticleTable under name `"G4CMPDriftElectronPolaron"`. |
| `library/src/G4CMPDriftHolePolaron.cc` | Defines the hole polaron particle: mass = `0.3 × m_e`, charge = `+e`, stable. |
| `library/include/G4CMPDriftElectronPolaron.hh` | Header — singleton pattern via `Definition()` / `G4CMPDriftElectronPolaronDefinition()`. |
| `library/include/G4CMPDriftHolePolaron.hh` | Header. |

---

## Physics Approach

### What "renamed electrons/holes" means

The standard `G4CMPDriftElectron` has the bare electron mass; the standard `G4CMPDriftHole`
uses the hole effective-mass tensor from V1. In this version we define new particle types that
look structurally identical to drift electrons/holes but carry the **polaron effective mass**
(`m*_polaron = 0.3 m_e` for sapphire, from Fröhlich coupling) hard-coded at registration
time, so G4's kinematics and all drift processes automatically use the right inertia.

```cpp
// From G4CMPDriftElectronPolaron.cc:
anInstance = new G4ParticleDefinition(
    "G4CMPDriftElectronPolaron",
    0.3 * electron_mass_c2,   // polaron mass  ← key difference vs bare electron
    0.0 * MeV,                // width
    -1. * eplus,              // charge
    ...
);
```

### What this branch does NOT include (see V3)

- No conversion process (`G4CMPBareToPolaron`) — that is added in V3.
- No phonon emission at conversion.
- No intravalley scattering rate models (optical/acoustic/impurity).
- Standard G4CMP drift processes handle transport once the polaron type exists.

---

## Comparison with V1

V1 (`hole-mass-tensor-impl`) used `G4CMPPolaronFormation` to attach polaron physics to
**existing** `G4CMPDriftElectron`/`G4CMPDriftHole` tracks at runtime via a `G4CMPPolaronInfo`
tag and mass-enhancement factor `m* → m*(1 + α/6)`. That approach modifies the carrier
in-place without creating a new G4 particle type.

This version (V2) instead **defines a proper new particle** with the polaron mass set from
the start, which is cleaner for G4's particle table, track-info management, and secondary
spawning (no stale track-info state from the parent carrier is inherited).

---

## Known Limitations
- Polaron creation (bare → polaron conversion with phonon emission) is not yet implemented
  in this branch → see V3.
- Effective mass (0.3 m_e) is a single scalar; anisotropic polaron mass not yet modeled.

*Work with Claude (Anthropic) — 2026.*
