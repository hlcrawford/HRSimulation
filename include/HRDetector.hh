#ifndef HRDetector_H
#define HRDetector_H 1

#include "G4Material.hh"
#include "Materials.hh"
#include "G4Tubs.hh"
#include "G4Box.hh"
#include "G4Cons.hh"
#include "G4Torus.hh"
#include "G4LogicalVolume.hh"
#include "G4VPhysicalVolume.hh"
#include "G4ThreeVector.hh"
#include "G4PVPlacement.hh"
#include "G4PVReplica.hh"
#include "G4PVDivision.hh"

#include "TrackerGammaSD.hh"

#include "G4VisAttributes.hh"
#include "G4Colour.hh"
#include "G4SubtractionSolid.hh"
#include "G4IntersectionSolid.hh"
#include "G4UnionSolid.hh"
#include "G4AssemblyVolume.hh"

#include "G4RotationMatrix.hh"
#include "G4Transform3D.hh"
#include "Randomize.hh"
#include "globals.hh"
#include <iostream>
#include <iomanip>

using namespace std;

class HRDetector 
{
public:

  G4LogicalVolume *expHall_log;
  Materials* materials;
  
  HRDetector(G4LogicalVolume*, Materials*, G4String);
  ~HRDetector();

  G4VPhysicalVolume *Construct();

  void setX(G4double x);
  void setY(G4double y);
  void setZ(G4double z);

  void MakeSensitive(TrackerGammaSD*);

  private:

  G4String orientation;

  // Logical volumes

  G4LogicalVolume* detector_log;
  
  // Physical volumes
 
  G4VPhysicalVolume* detector_phys;
  
  // Materials
  G4Material* HpGe;
  G4Material* Al;
  G4Material* Cu;

  // dimensions
  G4double xSize, ySize, zSize;

  G4AssemblyVolume* assemblyHR;
  
  // position
  G4RotationMatrix Rot0;
  G4RotationMatrix HRRot;
  G4ThreeVector Pos0;
  G4ThreeVector HRPos;
  G4ThreeVector DetShift;
  G4ThreeVector DetPos;

  G4Box *detectorBox;

};

#endif

