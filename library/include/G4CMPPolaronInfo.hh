/***********************************************************************\
 * This software is licensed under the terms of the GNU General Public *
 * License version 3 or later. See G4CMP/LICENSE for the full license. *
\***********************************************************************/

/// \file library/include/G4CMPPolaronInfo.hh
/// \brief Utility class to mark carriers as polarons and store polaron data

#ifndef G4CMPPolaronInfo_h
#define G4CMPPolaronInfo_h 1

#include "G4VUserTrackInformation.hh"

/// \class G4CMPPolaronInfo
/// \brief Stores polaron formation information on G4Track
///
/// When a carrier forms a polaron, this object is attached to the track
/// via SetUserInformation(). Downstream processes (scattering, etc.) can
/// check if a carrier is a polaron by querying this information.

class G4CMPPolaronInfo : public G4VUserTrackInformation {
public:
  /// Constructor: mark carrier as polaron with given Fröhlich coupling
  G4CMPPolaronInfo(G4double alphaCoupling = 0.0);

  virtual ~G4CMPPolaronInfo() {}

  /// Check if this carrier is a polaron
  G4bool IsPolaron() const { return fIsPolaron; }

  /// Get Fröhlich coupling constant for this polaron
  G4double GetAlphaCoupling() const { return fAlphaCoupling; }

  /// Set polaron flag
  void SetIsPolaron(G4bool flag) { fIsPolaron = flag; }

  /// Set Fröhlich coupling constant
  void SetAlphaCoupling(G4double alpha) { fAlphaCoupling = alpha; }

  /// Print information
  virtual void Print() const;

private:
  G4bool fIsPolaron;           ///< true if carrier formed a polaron
  G4double fAlphaCoupling;     ///< Fröhlich coupling constant α
};

#endif // G4CMPPolaronInfo_h
