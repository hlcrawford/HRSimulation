#include "HRDetector.hh"

HRDetector::HRDetector(G4LogicalVolume* experimentalHall_log,
				 Materials* mat, G4String orient)
{
  orientation = orient;

  materials=mat;
  expHall_log=experimentalHall_log;

  HpGe = materials->FindMaterial("HpGe");
  Al = materials->FindMaterial("Al");
  Cu = materials->FindMaterial("Cu");

  xSize = 4.*cm; ySize = 4.*cm;
  zSize = 7.5*mm;
  
  Pos0.setX(0);
  Pos0.setY(0);
  Pos0.setZ(0);

  Rot0=G4RotationMatrix::IDENTITY;

  DetShift.setX(0);
  DetShift.setY(0);
  DetShift.setZ(0);
  DetPos = Pos0 + DetShift;

  // Final HR placement
  HRPos.setX(  0.0*mm );
  HRPos.setY(  200.0*mm ); 
  HRPos.setZ(  0.0*mm );
  
  HRRot=G4RotationMatrix::IDENTITY;
  HRRot.rotateX(90.*deg);
}

HRDetector::~HRDetector()
{
}

G4VPhysicalVolume* HRDetector::Construct()
{

  G4cout << "Constructing the HR detector.";
  
  // Detector
  detectorBox = new G4Box("detector", xSize, ySize, zSize);
  detector_log = new G4LogicalVolume(detectorBox, HpGe, "HR_log", 0, 0, 0);

  assemblyHR = new G4AssemblyVolume();
  assemblyHR->AddPlacedVolume(detector_log, DetPos, &Rot0);
  assemblyHR->MakeImprint(expHall_log, HRPos, &HRRot, 31131);
  
  //Visualization Attributes

  // Crystal
  G4Colour dgreen (0.0,0.75, 0.0, 1.0); 
  G4VisAttributes* Vis_1 = new G4VisAttributes(dgreen);
  Vis_1->SetVisibility(true);
  Vis_1->SetForceSolid(true);

  detector_log->SetVisAttributes(Vis_1);
  
  G4cout << "... Done!" << G4endl;
  
  return detector_phys;
}
//---------------------------------------------------------------------
void HRDetector::MakeSensitive(TrackerGammaSD* TrackerGamma)
{
  detector_log->SetSensitiveDetector(TrackerGamma);
}
//---------------------------------------------------------------------
void HRDetector::setX(G4double x)
{
  DetPos.setX(x);
}
//---------------------------------------------------------------------
void HRDetector::setY(G4double y)
{
  DetPos.setY(y);
}
//---------------------------------------------------------------------
void HRDetector::setZ(G4double z)
{
  DetPos.setZ(z);
}
//---------------------------------------------------------------------
