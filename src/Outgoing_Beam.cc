#include "Outgoing_Beam.hh"
#include "GammaDecayChannel.hh"
#include "G4DecayTable.hh"
#include "G4Decay.hh"
#include "G4ProcessManager.hh"

G4Decay Outgoing_Beam::decay;

Outgoing_Beam::Outgoing_Beam()
{
  DZ=0;
  DA=0;
  Ex=1.0*MeV;
  lvlSchemeFileName = "";
  xsectFileName = "";
  Nlevels = 1;
  TarEx=0.*keV;
  TarA = 1;
  TarZ = 1;
  TFrac=0.;
  tau=0.0*ns;

  targetExcitation=false;
  reacted=false;
  source=false;
  sigma_a=0.;
  sigma_b=0.;
  theta_max=180.*deg;
  theta_min=0.;
  theta_bin=0.;
  twopi=8.*atan(1.);
  NQ=1;SQ=0;
  SetUpChargeStates();
  beamIn = NULL;
}

Outgoing_Beam::~Outgoing_Beam()
{
  ;
}

void Outgoing_Beam::defaultIncomingIon(Incoming_Beam *bi)
{
  if (bi == NULL) {
    return;
  }
  beamIn = bi;
}

void Outgoing_Beam::setDecayProperties()
{
  if (beamIn == NULL) {
    G4cerr << "Can not set decay properties as incoming beam has not been set" << G4endl;
    exit(EXIT_FAILURE);
  }

  Zin = beamIn->getZ();
  Ain = beamIn->getA();

  // Load the particle table
  beam     = G4ParticleTable::GetParticleTable()->GetIon(Zin,Ain,0.);
  ion      = G4ParticleTable::GetParticleTable()->GetIon(Zin+DZ,Ain+DA,Ex);
  ionGS    = G4ParticleTable::GetParticleTable()->GetIon(Zin+DZ,Ain+DA,0.);
  ionGS->SetPDGStable(true);

  if(TarA == 1 && TarZ ==1)
    tarIn = G4ParticleTable::GetParticleTable()->FindParticle("proton");
  else if (TarA == 2 && TarZ ==1)
    tarIn = G4ParticleTable::GetParticleTable()->FindParticle("deuteron");
  else if (TarA == 3 && TarZ ==1)
    tarIn = G4ParticleTable::GetParticleTable()->FindParticle("triton");
  else if (TarA == 3 && TarZ ==2)
    tarIn = G4ParticleTable::GetParticleTable()->FindParticle("He3");
  else if (TarA == 1 && TarZ ==0)
    tarIn = G4ParticleTable::GetParticleTable()->FindParticle("neutron");
  else
    tarIn = G4ParticleTable::GetParticleTable()->GetIon(TarZ,    TarA,    0.);

  if(DA == 0){
    tarOut   = tarIn;
    tarOutGS = tarIn;
  } else if(TarA-DA == 1 && TarZ-DZ ==1){
    tarOut = G4ParticleTable::GetParticleTable()->FindParticle("proton");
    tarOutGS = tarOut;
  } else if(TarA-DA == 2 && TarZ-DZ ==1){
    tarOut = G4ParticleTable::GetParticleTable()->FindParticle("deuteron");
    tarOutGS = tarOut;
  } else if(TarA-DA == 3 && TarZ-DZ ==1){
    tarOut = G4ParticleTable::GetParticleTable()->FindParticle("triton");
    tarOutGS = tarOut;
  } else if(TarA-DA == 3 && TarZ-DZ ==2) {
    tarOut = G4ParticleTable::GetParticleTable()->FindParticle("He3");
    tarOutGS = tarOut;
  } else if(TarA-DA == 1 && TarZ-DZ ==0) {
    tarOut = G4ParticleTable::GetParticleTable()->FindParticle("neutron");
    tarOutGS = tarOut;
  } else {
    tarOut = G4ParticleTable::GetParticleTable()->GetIon(TarZ-DZ, TarA-DA, Ex);
    tarOutGS = G4ParticleTable::GetParticleTable()->GetIon(TarZ-DZ, TarA-DA, 0.);
  }
  tarOutGS->SetPDGStable(true);


  if (ion == NULL) {
    G4cerr << "Error: no outgoing ion in particle table "
	   << Zin + DZ << " " << Ain+DA << G4endl;
    exit(EXIT_FAILURE);
  }
  if (ionGS == NULL) {
    G4cerr << "Error: no outgoing ion ground state in particle table "
	   << Zin + DZ << " " << Ain+DA << G4endl;
    exit(EXIT_FAILURE);
  }
  if (tarIn == NULL) {
    if(targetExcitation){
      G4cerr << "Error: no target nucleus in particle table "
	     << TarZ << " " << TarA << G4endl;
      exit(EXIT_FAILURE);
    } else {
      G4cerr << "Warning: no target nucleus in particle table "
	     << TarZ << " " << TarA << G4endl;
    }
  }
  if (tarOut == NULL) {
    if(targetExcitation){
      G4cerr << "Error: no target-like product in particle table "
	     << TarZ-DZ << " " << TarA-DA << G4endl;
      exit(EXIT_FAILURE);
    } else {
      G4cerr << "Warning: no target-like product in particle table "
	     << TarZ-DZ << " " << TarA-DA << G4endl;
    }
  }
  if (tarOutGS == NULL) {
    if(targetExcitation){
      G4cerr << "Error: no target-like ground-state product in particle table "
	     << TarZ-DZ << " " << TarA-DA << G4endl;
      exit(EXIT_FAILURE);
    } else {
      G4cerr << "Warning: no target-like ground-state product in particle table "
	     << TarZ-DZ << " " << TarA-DA << G4endl;
    }
  }

  m1 = beam->GetPDGMass();
  m2 = tarIn->GetPDGMass();
  if(targetExcitation){  
    m3 = ionGS->GetPDGMass();
    m4 = tarOut->GetPDGMass();
  } else {
    m3 = ion->GetPDGMass();
    if (tarOutGS == NULL)
      m4 = 0.;
    else
      m4 = tarOutGS->GetPDGMass();
  }

  G4DecayTable *DecTab = NULL;
  GammaDecayChannel *GamDec = NULL;
  G4ProcessManager *pm = NULL;
  G4ParticleDefinition* product;
  if(targetExcitation)
    product = tarOut;
  else
    product = ion;
  
  if (lvlSchemeFileName == ""){

    levelEnergy[0] = Ex/keV;
    relPop[0] = 1.0;

    G4cout << "Constructing decay properties for Z = " << product->GetAtomicNumber()
	   << " A = " << product->GetAtomicMass() << " with excitation energy " << Ex/keV << " keV" << G4endl;
    G4cout << "Direct gamma decay to the ground state." << G4endl;

    product->SetPDGStable(false);
    product->SetPDGLifeTime(tau);

    DecTab = product->GetDecayTable(); 
    if (DecTab == NULL) {
      DecTab = new G4DecayTable();
      product->SetDecayTable(DecTab);
    }
    GamDec = new GammaDecayChannel(-1,product,1,Ex,0.,theAngularDistribution);
    DecTab->Insert(GamDec);
    //    DecTab->DumpInfo();

    // make sure that the ion has the decay process in its manager
    pm = product->GetProcessManager();
    if (pm == NULL) {
      G4cerr << "Could not find process manager for reaction product." << G4endl;
      exit(EXIT_FAILURE);
    }
    pm->AddProcess(&decay,1,-1,4);
    // pm->DumpInfo();

  } else { // Set up intermediate states and decay properties
    G4cout << "Reading level scheme description from " 
	   << lvlSchemeFileName << G4endl;

    G4int nBranch;
    G4double meanLife, BR, Exf, a0, a2, a4;
    G4ParticleDefinition* intermediateIon;

    openLvlSchemeFile();

    Nlevels = 0;
    while(lvlSchemeFile >> levelEnergy[Nlevels] >> nBranch >> meanLife >> relPop[Nlevels]){
      G4cout << "Constructing decay properties for Z=" << Zin + DZ
	     << " A=" << Ain + DA 
	     << " with excitation " << levelEnergy[Nlevels] 
	     << " keV, mean lifetime " << meanLife
	     << " ps, relative population " << relPop[Nlevels]
	     << G4endl;
      if(Nlevels>0) relPop[Nlevels] += relPop[Nlevels-1];
      for(G4int j = 0; j < nBranch; j++){
	lvlSchemeFile >> BR >> Exf >> a0 >> a2 >> a4;

	intermediateIon = 
	  G4ParticleTable::GetParticleTable()->GetIon(product->GetAtomicNumber(),
						      product->GetAtomicMass(),
						      levelEnergy[Nlevels]*keV);
	if (intermediateIon == NULL) {
	  G4cerr << "Could not find intermediate ion in particle table."
		 << product->GetAtomicNumber() << " " << product->GetAtomicMass() << G4endl;
	  exit(EXIT_FAILURE);
	}
	intermediateIon->SetPDGStable(false);
	intermediateIon->SetPDGLifeTime(meanLife*picosecond);

	DecTab = intermediateIon->GetDecayTable(); 
	if (DecTab == NULL) {
	  DecTab = new G4DecayTable();
	  intermediateIon->SetDecayTable(DecTab);
	}

	theAngularDistribution.SetCoeffs(a0,a2,a4);
	theAngularDistribution.Report();

	GamDec = new GammaDecayChannel(-1,intermediateIon,BR,(levelEnergy[Nlevels]-Exf)*keV,Exf*keV,theAngularDistribution);
	DecTab->Insert(GamDec);

	// make sure that the ion has the decay process in its manager
	pm = intermediateIon->GetProcessManager();
	if (pm == NULL) {
	  G4cerr << "Could not find process manager for reaction product." << G4endl;
	  exit(EXIT_FAILURE);
	}
	pm->AddProcess(&decay,1,-1,4);

      }
      cout << endl;
      DecTab->DumpInfo();
      //      pm->DumpInfo();

      Nlevels++;
    }

    closeLvlSchemeFile();

    // Normalize relative population parameters
    for(G4int j = 0; j<Nlevels; j++){
      relPop[j] /= relPop[Nlevels-1];
      //      G4cout << "Level energy = " << levelEnergy[j] 
      //             << " keV, relPop[" << j << "] = " << relPop[j] 
      //	     << G4endl;
    }
  }
}

//---------------------------------------------------------
void Outgoing_Beam::ScanInitialConditions(const G4Track & aTrack)
{
 
  dirIn=aTrack.GetMomentumDirection();
  posIn=aTrack.GetPosition();
  pIn=aTrack.GetMomentum();
  KEIn=aTrack.GetDynamicParticle()->GetKineticEnergy();
  Ain=aTrack.GetDynamicParticle()->GetDefinition()->GetAtomicMass();
  Zin=aTrack.GetDynamicParticle()->GetDefinition()->GetAtomicNumber();
  //  m1 = aTrack.GetDynamicParticle()->GetDefinition()->GetPDGMass();
  tauIn=aTrack.GetProperTime();
  ReactionFlag=-1;
  if(aTrack.GetVolume()->GetLogicalVolume()->GetName()=="target_log")
    ReactionFlag=0;
  ThresholdFlag = 1;
  if( KEIn < Ex/2/m2*(m1+m2+m3+m4) )
    ThresholdFlag = 0;
  ET = KEIn + m1 + m2;                                      // Total E
  p1 = sqrt( (KEIn + m1)*(KEIn + m1) - m1*m1 );          // Incoming p
  // Lab-frame scattering angle limit
  sin2theta3_max = 
    ( (ET*ET - p1*p1 + m3*m3 - m4*m4)*(ET*ET - p1*p1 + m3*m3 - m4*m4)
      -4*m3*m3*((m1 + m2)*(m1 + m2) + 2*m2*KEIn) )/( 4*m3*m3*p1*p1 );

}

//---------------------------------------------------------
G4ThreeVector Outgoing_Beam::ReactionPosition()
{
 
  posOut=posIn;
  posOut.setZ(posIn.getZ()+eps);
  //  G4cout<<"Reaction took place at "<<G4BestUnit(posOut.getZ(),"Length")<<G4endl;

  return posOut;

}
//---------------------------------------------------------
G4DynamicParticle* Outgoing_Beam::ReactionProduct()
{

  G4int lvl;
  G4double rnd = G4UniformRand();
  for(lvl=0; lvl < Nlevels; lvl++){
    //    G4cout << std::fixed << std::setprecision(4) 
    //	   << "rnd = " << rnd << ", lvl = " << lvl << G4endl;
    if(rnd < relPop[lvl]) break;
  }
  //  G4cout << "Chose lvl = " << lvl << ", E = " << levelEnergy[lvl] << " keV" << G4endl;

  G4int Zout, Aout;
  if(targetExcitation){
    Zout = TarZ - DZ;
    Aout = TarA - DA;
  } else {
    Zout = Zin + DZ;
    Aout = Ain + DA;
  }

  G4ParticleDefinition* product = G4ParticleTable::GetParticleTable()->GetIon(Zout,Aout,levelEnergy[lvl]*keV);

  G4DynamicParticle* aReactionProduct = new G4DynamicParticle(product,GetOutgoingMomentum());

  return aReactionProduct;
}
//---------------------------------------------------------
//G4DynamicParticle* Outgoing_Beam::ProjectileGS()
//{
//  G4DynamicParticle* aReactionProduct =new G4DynamicParticle(ionGS,GetOutgoingMomentum());

  //  aReactionProduct->SetProperTime(tauIn);
  // G4cout<<" Proper time set to "<<aReactionProduct->GetProperTime()<<G4endl;

//  return aReactionProduct;
//}
//---------------------------------------------------------
// G4DynamicParticle* Outgoing_Beam::TargetExcitation() // No longer needed
// {
//   particleTable = G4ParticleTable::GetParticleTable();
//   G4DynamicParticle* aReactionProduct =new G4DynamicParticle(particleTable->FindParticle("gamma"),TargetAngularDistribution(),TarEx);

//   return aReactionProduct;
// }

//---------------------------------------------------------
G4ThreeVector Outgoing_Beam::TargetAngularDistribution() // No longer needed
{
    //TB allow a coulex angular distribution for target excitation (coulex on gold)
    G4ThreeVector direction = G4RandomDirection();
    direction.setTheta(theTargetAngularDistribution.GetRandomAngle());
    //G4cout<<direction.getTheta()<<G4endl;
    return direction;
}
//---------------------------------------------------------
G4ThreeVector Outgoing_Beam::GetOutgoingMomentum()
{
  static const G4ThreeVector ez(0.,0.,1.);

  G4ThreeVector ax,ppOut;
  G4double      theta3;

  // Lab-frame scattering angle ================================================

  theta3=GetDTheta();
  // If theta3 is beyond the limit dictated by the kinematics
  // or if an angle cut is specified ...
  while( sin(theta3)*sin(theta3) > sin2theta3_max
	 || (theta3 < theta_min)
	 || (theta3 > theta_max) ){
    theta3=GetDTheta();
  }

  // Relativistic kinematics ===================================================
  //       Baldin et al, Kinematics of Nuclear Reactions, Pergamon (1961)

  G4double E3Lab = 1/(ET*ET - p1*p1*cos(theta3)*cos(theta3))// Beam-like product
    *( ET*( m2*(KEIn + m1) + (m1*m1+m2*m2+m3*m3-m4*m4)/2. )
       + p1*cos(theta3)*sqrt( ( m2*(KEIn+m1) + (m1*m1+m2*m2-m3*m3-m4*m4)/2. )
			     *( m2*(KEIn+m1) + (m1*m1+m2*m2-m3*m3-m4*m4)/2. ) 
			     - m3*m3*m4*m4 
			     - p1*p1*m3*m3*sin(theta3)*sin(theta3) ) );

  G4double p3Lab = sqrt( E3Lab*E3Lab - m3*m3 );

  G4double p4Lab = sqrt( (ET-E3Lab)*(ET-E3Lab) - m4*m4);  // Target-like product

  G4double theta4 = asin( p3Lab/p4Lab * sin(theta3) );

  G4double pLab = p3Lab;       // Construct pLab vector of the Beam-like product
  G4double theta = theta3;
  if( targetExcitation ){    // Construct pLab vector of the Target-like product
    pLab = p4Lab;
    theta = theta4;
  }

  // Set the magnitude of the outgoing momentum ================================

  ppOut = pIn;
  if( ppOut.mag() > 0)
      ppOut.setMag( pLab );

  // Set the direction of the outgoing momentum ================================

  ax=pIn.cross(ez);
  if (pIn.mag2() == 0.) {
    return ppOut;
  }
  ax.rotate(pIn,G4UniformRand()*twopi);
  
  if (ax.mag2() == 0.) {
    return ppOut;
  }
  ppOut.rotate(ax,theta);

  //  G4cout << std::setprecision(3) << std::setw(10)
  //	 << "ppOut.theta() = " << ppOut.theta()/deg 
  //	 << "  ata = " << -asin(ppOut.getY()/ppOut.mag())/mrad
  //	 << "  bta = " << -asin(ppOut.getX()/ppOut.mag())/mrad
  //	 << G4endl;

  return ppOut;
}

//---------------------------------------------------------
G4double Outgoing_Beam::GetDTheta()
{

  G4double theta;
  if(xsectFileName == ""){
    G4double theta_a,theta_b;
    theta_a = CLHEP::RandGauss::shoot(0,sigma_a);
    theta_b = CLHEP::RandGauss::shoot(0,sigma_b);
    theta = sqrt(theta_a*theta_a+theta_b*theta_b);
  } else {
    CLHEP::RandGeneral randTheta( Xsect, Nxsect );
    theta = theta_min + randTheta.fire()*(theta_max-theta_min);
  }

  return theta;
}
//---------------------------------------------------------
void Outgoing_Beam::setXsectFile(G4String fileName)
{
 
  char line[1000];

  xsectFileName = fileName;
  std::ifstream xsectFile;
  xsectFile.open(xsectFileName);

  xsectFile >> theta_min >> theta_max >> theta_bin;
  xsectFile.getline(line,1000);  // Advance to next line.

  theta_min *= deg;
  theta_max *= deg;
  theta_bin *= deg;

  G4double x;
  Nxsect = 0;
  while(xsectFile >> x){
    xsectFile.getline(line,1000);  // Advance to next line.
    Xsect[Nxsect] = x*sin( theta_min + (Nxsect+0.5)*theta_bin );
    Nxsect++;
  }

  xsectFile.close();

}
//---------------------------------------------------------
void Outgoing_Beam::Report()
{
  setDecayProperties();

  G4cout<<"----> Delta A for the outgoing beam set to  "<<DA<< G4endl;
  G4cout<<"----> Delta Z for the outgoing beam set to  "<<DZ<< G4endl;
  G4cout<<"----> Target A (for kinematics calculations) is set to  "<<TarA<< G4endl;
  G4cout<<"----> Target Z (for kinematics calculations) is set to  "<<TarZ<< G4endl;
  G4cout<<"----> Excitation energy of the outgoing beam set to "<<
    G4BestUnit(Ex,"Energy")<<G4endl;
  G4cout<<"----> Target excitation energy set to "<< G4BestUnit(TarEx,"Energy")<<G4endl;  
  G4cout<<"----> Fraction of target excitations set to "<<TFrac<<G4endl;
  G4cout<<"----> Mass of the incoming beam is " <<beam->GetPDGMass()<<" MeV"<<G4endl;
  G4cout<<"----> Mass of the target is " <<tarIn->GetPDGMass()<<" MeV"<<G4endl;
  G4cout<<"----> Mass of the outgoing beam-like reaction product is " <<ion->GetPDGMass()<<" MeV"<<G4endl;
  if(tarOut != NULL)
    G4cout<<"----> Mass of the outgoing target-like reaction product is " <<tarOut->GetPDGMass()<<" MeV"<<G4endl;
  G4cout<<"----> Lifetime of the excited state for the outgoing beam set to "<<
    G4BestUnit(tau,"Time")<<G4endl; 
  G4cout<<"----> Sigma for ata distribution set to "<<sigma_a<<G4endl;
  G4cout<<"----> Sigma for bta distribution set to "<<sigma_b<<G4endl;
  G4cout<<"----> Number of charge states "<<NQ<<G4endl;
  vector<Charge_State*>::iterator itPos = Q.begin();
  for(; itPos < Q.end(); itPos++) 
    {
      G4cout<<"----> Charge state "     <<(*itPos)->GetCharge()<<G4endl;
      G4cout<<"   => UnReactedFraction "<<(*itPos)->GetUnReactedFraction()<<G4endl;
      G4cout<<"   =>   ReactedFraction "<<(*itPos)->GetReactedFraction()<<G4endl;
      if((*itPos)->GetUseSetKEu())
	{
	  G4cout<<"   =>              KE/A "<<(*itPos)->GetSetKEu()/MeV<<" MeV"<<G4endl; 
	  //	 G4cout<<"   =>                KE "<<(*itPos)->GetSetKE()/MeV<<" MeV"<<G4endl; LR (not initialized if KEu is used)
	}
      else
	G4cout<<"   =>                KE "<<(*itPos)->GetSetKE()/MeV<<" MeV"<<G4endl;
    }
  CalcQUR();
  CalcQR();
}


//---------------------------------------------------------
void Outgoing_Beam::setDA(G4int da)
{

  DA=da;
 G4cout<<"----> Delta A for the outgoing beam set to  "<<DA<< G4endl; 
}
//---------------------------------------------------------
void Outgoing_Beam::setDZ(G4int dz)
{

  DZ=dz;
  G4cout<<"----> Delta Z for the outgoing beam set to  "<<DZ<< G4endl;
  
}
//---------------------------------------------------------
void Outgoing_Beam::setEx(G4double ex)
{

  Ex=ex;
  
G4cout<<"----> Excitation energy of the outgoing beam set to "<<
 G4BestUnit(Ex,"Energy")<<G4endl;

}
//---------------------------------------------------------
void Outgoing_Beam::setTarEx(G4double ex)
{

  TarEx=ex;
  
G4cout<<"----> Target excitation energy set to "<< G4BestUnit(TarEx,"Energy")<<G4endl;

}
//---------------------------------------------------------
void Outgoing_Beam::setTarA(G4int a)
{

  TarA = a;
  
  G4cout<<"----> Target A set to "<< TarA <<G4endl;

}
//---------------------------------------------------------
void Outgoing_Beam::setTarZ(G4int z)
{

  TarZ = z;
  
  G4cout<<"----> Target Z set to "<< TarZ <<G4endl;

}
//---------------------------------------------------------
void Outgoing_Beam::setTFrac(G4double ex)
{
  TFrac=ex;
  if(TFrac>1) TFrac=1.;
  if(TFrac<0) TFrac=0.;

  
G4cout<<"----> Fraction of target excitations set to "<<TFrac<<G4endl;

}
//-----------------------------------------------------------------
void Outgoing_Beam::settau(G4double t)
{

  tau=t;

G4cout<<"----> Lifetime of the excited state for the outgoing beam set to "<<
 G4BestUnit(tau,"Time")<<G4endl; 
}
//---------------------------------------------------------
void Outgoing_Beam::SetUpChargeStates()
{

 vector<Charge_State*>::iterator itPos = Q.begin();
  // clear all elements from the array
  for(; itPos < Q.end(); itPos++)
    delete *itPos;    // free the element from memory
   // finally, clear all elements from the array
  Q.clear();

  for(G4int i=0;i<NQ;i++) 
    Q.push_back(new Charge_State);


}
//---------------------------------------------------------
void Outgoing_Beam::SetQCharge(G4int q)
{

  if(SQ>=0&&SQ<NQ)
    {
      vector<Charge_State*>::iterator itPos = Q.begin();

      for(G4int i=0;i<SQ;i++) itPos++;
      (*itPos)->SetCharge(q);
    }
  else
    G4cout<<" Number of defined charge states ="<<NQ<<" is too small"<<G4endl;
}

//---------------------------------------------------------
void Outgoing_Beam::SetQUnReactedFraction(G4double f)
{

  if(SQ>=0&&SQ<NQ)
    {
      vector<Charge_State*>::iterator itPos = Q.begin();

      for(G4int i=0;i<SQ;i++) itPos++;
      (*itPos)->SetUnReactedFraction(f);
      CalcQUR();
    }
  else
    G4cout<<" Number of defined charge states ="<<NQ<<" is too small"<<G4endl;
}
//---------------------------------------------------------
void Outgoing_Beam::SetQReactedFraction(G4double f)
{

  if(SQ>=0&&SQ<NQ)
    {
      vector<Charge_State*>::iterator itPos = Q.begin();

      for(G4int i=0;i<SQ;i++) itPos++;
      (*itPos)->SetReactedFraction(f);
      CalcQR();
    }
  else
    G4cout<<" Number of defined charge states ="<<NQ<<" is too small"<<G4endl;
}
//---------------------------------------------------------
void Outgoing_Beam::SetQKEu(G4double e)
{
  G4int A;

  A=beamIn->getA()+DA;
  if(SQ>=0&&SQ<NQ)
    {
      vector<Charge_State*>::iterator itPos = Q.begin();

      for(G4int i=0;i<SQ;i++) itPos++;
      (*itPos)->SetKEu(e,A);
      CalcQR();
      CalcQUR();
    }
  else
    G4cout<<" Number of defined charge states ="<<NQ<<" is too small"<<G4endl;
}
//---------------------------------------------------------
void Outgoing_Beam::SetQKE(G4double e)
{

  if(SQ>=0&&SQ<NQ)
    {
      vector<Charge_State*>::iterator itPos = Q.begin();

      for(G4int i=0;i<SQ;i++) itPos++;
      (*itPos)->SetKE(e);
      CalcQR();
      CalcQUR();
    }
  else
    G4cout<<" Number of defined charge states ="<<NQ<<" is too small"<<G4endl;
}
//---------------------------------------------------------
void Outgoing_Beam::CalcQR()
{
  G4double sum;
  vector<Charge_State*>::iterator itPos = Q.begin();

  sum=0.;
  for(; itPos < Q.end(); itPos++)
    sum+=(*itPos)->GetReactedFraction();
  //  G4cout<<" Sum = "<<sum<<G4endl;
  itPos = Q.begin();
  G4int ind=0,i,max,n=0;
  for(; itPos < Q.end(); itPos++)
    { 
  
      max=(G4int)(1000.*(*itPos)->GetReactedFraction()/sum);
      //      G4cout<<" Sum = "<<sum<<" Max = "<<max<<" n "<<n<<G4endl;
      for(i=0;i<max;i++) 
	{
	  QR[ind]=(*itPos)->GetSetKE();
	  QRI[ind]=n;
	  ind++;
	}
      n++;
    }

}
//---------------------------------------------------------
void Outgoing_Beam::CalcQUR()
{
  G4double sum;
  vector<Charge_State*>::iterator itPos = Q.begin();

  sum=0.;
  for(; itPos < Q.end(); itPos++)
    sum+=(*itPos)->GetUnReactedFraction();

  itPos = Q.begin();
  G4int ind=0,i,max,n=0;
  for(; itPos < Q.end(); itPos++)
    { 
      max=(G4int)(1000.*(*itPos)->GetUnReactedFraction()/sum);
      //      G4cout<<" Sum = "<<sum<<" Max = "<<max<<" n "<<n<<" ind "<<ind<<G4endl;
      for(i=0;i<max;i++) 
	{
	  QUR[ind]=(*itPos)->GetSetKE();
	  QURI[ind]=n;
	  ind++;
	}
      n++;
    }
}
//---------------------------------------------------------
G4double Outgoing_Beam::GetURsetKE()
{
  G4int i;

  i=(G4int)(1000.*G4UniformRand());
  Index=QURI[i];
  return QUR[i];
}
//---------------------------------------------------------
G4double Outgoing_Beam::GetRsetKE()
{
  G4int i;

  i=(G4int)(1000.*G4UniformRand());
  Index=QRI[i];
  return QR[i];
}
//----------------------------------------------------------
void Outgoing_Beam::SetCoeff(int index,double a) {
  theAngularDistribution.SetCoeff(index,a);
  return;
}
//----------------------------------------------------------
void Outgoing_Beam::SetTargetCoeff(int index, double a)
{
  theTargetAngularDistribution.SetCoeff(index, a);
  return;
}
//----------------------------------------------------------
void Outgoing_Beam::openLvlSchemeFile()
{
  if(!lvlSchemeFile.is_open()) lvlSchemeFile.open(lvlSchemeFileName.c_str());
  
  if (lvlSchemeFile == NULL) G4cout << "lvlSchemeFile ERROR" << G4endl;
}
//----------------------------------------------------------
void Outgoing_Beam::closeLvlSchemeFile()
{
  lvlSchemeFile.close();

  return;
}
