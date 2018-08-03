#ifndef DetectorConstruction_H
#define DetectorConstruction_H 1

#include "G4VUserDetectorConstruction.hh"
#include "Materials.hh"
#include "Experimental_Hall.hh"
#include "Experimental_Hall_Messenger.hh"
#include "Background_Sphere.hh"
#include "Background_Sphere_Messenger.hh"
#include "HRDetector.hh"
#include "HRDetector_Messenger.hh"
#include "Target.hh"
#include "Target_Messenger.hh"

#include "Gretina_NSCL_Shell.hh"
#include "Greta_Shell.hh"
#include "HR_Array.hh"
#include "G4LogicalVolume.hh"
#include "G4VPhysicalVolume.hh"
#include "TrackerIonSD.hh"
#include "TrackerIonSD_Messenger.hh"
#include "TrackerGammaSD.hh"
#include "TrackerGammaSD_Messenger.hh"
#include "G4SDManager.hh"

class HR_Array;
class HR_Array_Messenger;
class DetectorConstruction_Messenger;

class DetectorConstruction : public G4VUserDetectorConstruction
{
public:

  DetectorConstruction();
  ~DetectorConstruction();

  G4VPhysicalVolume* Construct();
  HR_Array* GetArray(){ return the_Array;}
  TrackerGammaSD* GetGammaSD(){ return TrackerGamma;}
  G4Box* GetTarget(){return aTarget->GetTarget();}
  G4VPhysicalVolume* GetTargetPlacement(){return aTarget->GetTargetPlacement();}
  G4double GetTargetThickness(){return aTarget->GetTargetThickness();}
  void setTargetReactionDepth(G4double depth){ aTarget->setTargetReactionDepth(depth);};
  G4ThreeVector* GetTargetPos(){return aTarget->GetPos();}

  G4LogicalVolume*   HallLog(){return ExpHall_log;}
  G4VPhysicalVolume* HallPhys(){return ExpHall_phys;}

  G4double GetBGSphereRmin(){return BackgroundSphere->GetRmin();}
  G4double GetBGSphereRmax(){return BackgroundSphere->GetRmax();}

  void DefineMaterials();

  void SetTargetStatus(G4bool stat){targetStatus = stat;}
  void SetGretinaStatus(G4bool stat){gretinaStatus = stat;}

  void Placement();

private:
  DetectorConstruction_Messenger *myMessenger;

  Materials* materials;

  G4bool targetStatus;
  G4bool gretinaStatus;
  Target* aTarget;

  Background_Sphere* BackgroundSphere;

  HR_Array*   the_Array;

  // Logical volumes
  G4LogicalVolume* ExpHall_log;

  // Physical volumes
  G4VPhysicalVolume* ExpHall_phys;

  Experimental_Hall_Messenger* ExperimentalHallMessenger;
  Target_Messenger*    TargetMessenger;
  Background_Sphere_Messenger* BackgroundSphereMessenger;
  TrackerGammaSD* TrackerGamma;
  TrackerGammaSD_Messenger* TrackerGammaSDMessenger;
  TrackerIonSD* TrackerIon;
  TrackerIonSD_Messenger* TrackerIonSDMessenger;

  HR_Array_Messenger*  the_Array_Messenger;
};


#include "G4UImessenger.hh"
#include "G4UIcmdWithoutParameter.hh"
#include "G4UIcmdWithAString.hh"

class DetectorConstruction_Messenger: public G4UImessenger
{
public:
  DetectorConstruction_Messenger(DetectorConstruction*);
  ~DetectorConstruction_Messenger();
  
private:
  DetectorConstruction*        myTarget;
  
private:
  G4UIcmdWithoutParameter*     UpdateCmd;
  G4UIcmdWithoutParameter*     TargetCmd;
  G4UIcmdWithoutParameter*     NoGretCmd;
  
public:
  void SetNewValue(G4UIcommand*, G4String);
};

#endif
