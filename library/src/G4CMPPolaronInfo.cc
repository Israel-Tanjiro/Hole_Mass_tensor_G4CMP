/***********************************************************************\
 * This software is licensed under the terms of the GNU General Public *
 * License version 3 or later. See G4CMP/LICENSE for the full license. *
\***********************************************************************/

/// \file library/src/G4CMPPolaronInfo.cc
/// \brief Implementation of G4CMPPolaronInfo utility class

#include "G4CMPPolaronInfo.hh"
#include "G4SystemOfUnits.hh"
#include <iostream>

// ══════════════════════════════════════════════════════════════════════
// CONSTRUCTOR
// ══════════════════════════════════════════════════════════════════════

G4CMPPolaronInfo::G4CMPPolaronInfo(G4double alphaCoupling)
  : G4VUserTrackInformation(),
    fIsPolaron(true),
    fAlphaCoupling(alphaCoupling) {
  // Track user information is now attached to the carrier
}

// ══════════════════════════════════════════════════════════════════════
// PRINT
// ══════════════════════════════════════════════════════════════════════

void G4CMPPolaronInfo::Print() const {
  G4cout << "\n=== G4CMPPolaronInfo ===" << G4endl;
  G4cout << "  IsPolaron: " << (fIsPolaron ? "YES" : "NO") << G4endl;
  G4cout << "  Alpha coupling: " << fAlphaCoupling << G4endl;
  G4cout << "========================\n" << G4endl;
}
