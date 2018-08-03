#include "HRDetector_Messenger.hh"

HRDetector_Messenger::HRDetector_Messenger(HRDetector* SD)
:HRDet(SD)
{ 
 
  HRDir = new G4UIdirectory("/HR/");
  HRDir->SetGuidance("HR control.");

  XCmd = new G4UIcmdWithADoubleAndUnit("/HR/setX",this);
  XCmd->SetGuidance("Set the x position of the detector (crystal center)");
  XCmd->SetParameterName("choice",false);
  XCmd->AvailableForStates(G4State_PreInit,G4State_Idle);

  YCmd = new G4UIcmdWithADoubleAndUnit("/HR/setY",this);
  YCmd->SetGuidance("Set the y position of the detector (crystal center)");
  YCmd->SetParameterName("choice",false);
  YCmd->AvailableForStates(G4State_PreInit,G4State_Idle);

  ZCmd = new G4UIcmdWithADoubleAndUnit("/HR/setZ",this);
  ZCmd->SetGuidance("Set the z position of the detector (crystal center)");
  ZCmd->SetParameterName("choice",false);
  ZCmd->AvailableForStates(G4State_PreInit,G4State_Idle);

}



HRDetector_Messenger::~HRDetector_Messenger()
{
  delete HRDir;
  delete XCmd;
  delete YCmd;
  delete ZCmd;
}


void HRDetector_Messenger::SetNewValue(G4UIcommand* command,G4String newValue)
{ 
  if( command == XCmd )
    {HRDet->setX(XCmd->GetNewDoubleValue(newValue));}
  if( command == YCmd )
    {HRDet->setY(YCmd->GetNewDoubleValue(newValue));}
  if( command == ZCmd )
    {HRDet->setZ(ZCmd->GetNewDoubleValue(newValue));}
}

