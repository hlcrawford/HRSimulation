#ifndef HRDetector_Messenger_h
#define HRDetector_Messenger_h 1

#include "HRDetector.hh"
#include "globals.hh"
#include "G4UImessenger.hh"
#include "G4UIdirectory.hh"
#include "G4UIcmdWithAString.hh"
#include "G4UIcmdWithADoubleAndUnit.hh"
#include "G4UIcmdWithADouble.hh"
#include "G4UIcmdWithoutParameter.hh"

class HRDetector_Messenger: public G4UImessenger
{
  public:
    HRDetector_Messenger(HRDetector*);
   ~HRDetector_Messenger();
    
    void SetNewValue(G4UIcommand*, G4String);
    
  private:
    HRDetector* HRDet;
   
    G4UIdirectory*             HRDir;  

    G4UIcmdWithADoubleAndUnit* XCmd;
    G4UIcmdWithADoubleAndUnit* YCmd;
    G4UIcmdWithADoubleAndUnit* ZCmd;
};


#endif

