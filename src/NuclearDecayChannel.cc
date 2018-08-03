//
// ********************************************************************
// * License and Disclaimer                                           *
// *                                                                  *
// * The  Geant4 software  is  copyright of the Copyright Holders  of *
// * the Geant4 Collaboration.  It is provided  under  the terms  and *
// * conditions of the Geant4 Software License,  included in the file *
// * LICENSE and available at  http://cern.ch/geant4/license .  These *
// * include a list of copyright holders.                             *
// *                                                                  *
// * Neither the authors of this software system, nor their employing *
// * institutes,nor the agencies providing financial support for this *
// * work  make  any representation or  warranty, express or implied, *
// * regarding  this  software system or assume any liability for its *
// * use.  Please see the license in the file  LICENSE  and URL above *
// * for the full disclaimer and the limitation of liability.         *
// *                                                                  *
// * This  code  implementation is the result of  the  scientific and *
// * technical work of the GEANT4 collaboration.                      *
// * By using,  copying,  modifying or  distributing the software (or *
// * any work based  on the software)  you  agree  to acknowledge its *
// * use  in  resulting  scientific  publications,  and indicate your *
// * acceptance of all terms of the Geant4 Software license.          *
// ********************************************************************
//
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//
// MODULES:             NuclearDecayChannel.cc
//
// Version:             0.b.4
// Date:                14/04/00
// Author:              F Lei & P R Truscott
// Organisation:        DERA UK
// Customer:            ESA/ESTEC, NOORDWIJK
// Contract:            12115/96/JG/NL Work Order No. 3
//
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//
// CHANGE HISTORY
// --------------
//
// 29 February 2000, P R Truscott, DERA UK
// 0.b.3 release.
//
// 18 October 2002, F Lei
//            modified link metheds in DecayIt() to G4PhotoEvaporation() in order to
//            use the new Internal Coversion feature.      
// 13 April 2000, F Lei, DERA UK
//            Changes made are:
//            1) Use PhotonEvaporation instead of DiscreteGammaDeexcitation
//            2) verbose control
//
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
///////////////////////////////////////////////////////////////////////////////
//
#include "G4NuclearLevelManager.hh"
#include "G4NuclearLevelStore.hh"
#include "NuclearDecayChannel.hh"
#include "G4DynamicParticle.hh"
#include "G4DecayProducts.hh"    //LR
#include "G4DecayTable.hh"
#include "G4PhysicsLogVector.hh"
#include "G4ParticleChangeForRadDecay.hh"
#include "G4IonTable.hh"

#include "G4BetaFermiFunction.hh"
#include "G4PhotonEvaporation.hh"
#include "G4AtomicTransitionManager.hh"
#include "G4AtomicShell.hh"
#include "G4AtomicDeexcitation.hh"

using namespace CLHEP;

const G4double G4NuclearDecayChannel:: pTolerance = 0.001;
const G4double G4NuclearDecayChannel:: levelTolerance = 2.0*keV;
//const G4bool G4NuclearDecayChannel:: FermiOn = true;

///////////////////////////////////////////////////////////////////////////////
//
//
// Constructor for one decay product (the nucleus).
//
G4NuclearDecayChannel::G4NuclearDecayChannel
                      (const G4RadioactiveDecayMode &theMode,
                       G4int Verbose,
                       const G4ParticleDefinition *theParentNucleus,
                       G4double theBR,
                       G4double theQtransition,
                       G4int A,
                       G4int Z,
                       G4double theDaughterExcitation) :
  G4GeneralPhaseSpaceDecay(Verbose), decayMode(theMode)
{
#ifdef G4VERBOSE
  if (GetVerboseLevel()>1)
    {G4cout <<"G4G4NuclearDecayChannel constructor for " <<G4int(theMode) <<G4endl;}
#endif
  SetParent(theParentNucleus);
  FillParent();
  parent_mass = theParentNucleus->GetPDGMass();
  SetBR (theBR);
  SetNumberOfDaughters (1);
  FillDaughterNucleus (0, A, Z, theDaughterExcitation);
  Qtransition = theQtransition;
}
///////////////////////////////////////////////////////////////////////////////
//
//
// Constructor for a daughter nucleus and one other particle.
//
G4NuclearDecayChannel::G4NuclearDecayChannel
                      (const G4RadioactiveDecayMode &theMode,
                       G4int Verbose,
                       const G4ParticleDefinition *theParentNucleus,
                       G4double theBR,
                       G4double theQtransition,
                       G4int A,
                       G4int Z,
                       G4double theDaughterExcitation,
                       const G4String theDaughterName1,
											 AngularDistribution theAngularDistribution) :
			G4GeneralPhaseSpaceDecay(theAngularDistribution,Verbose), decayMode(theMode)
{
#ifdef G4VERBOSE
  if (GetVerboseLevel()>1)
    {G4cout <<"G4NuclearDecayChannel constructor for " <<G4int(theMode) <<G4endl;}
#endif
  SetParent (theParentNucleus);
  FillParent();
  parent_mass = theParentNucleus->GetPDGMass();
  SetBR (theBR);
  SetNumberOfDaughters (2);
  SetDaughter(0, theDaughterName1);
  FillDaughterNucleus (1, A, Z, theDaughterExcitation);
  Qtransition = theQtransition;
}
///////////////////////////////////////////////////////////////////////////////
//
//
// Constructor for a daughter nucleus and two other particles.
//
G4NuclearDecayChannel::G4NuclearDecayChannel
                      (const G4RadioactiveDecayMode &theMode,
                       G4int Verbose,
                       const G4ParticleDefinition *theParentNucleus,
                       G4double theBR,
                       G4double theFFN,
		       G4bool betaS, 
		       CLHEP::RandGeneral* randBeta,
                       G4double theQtransition,
                       G4int A,
                       G4int Z,
                       G4double theDaughterExcitation,
                       const G4String theDaughterName1,
                       const G4String theDaughterName2) :
			G4GeneralPhaseSpaceDecay(Verbose), decayMode(theMode)
  //,BetaSimple(betaS),
  //			RandomEnergy(randBeta),  Qtransition(theQtransition),FermiFN(theFFN)
{
#ifdef G4VERBOSE
  if (GetVerboseLevel()>1)
    {G4cout <<"G4NuclearDecayChannel constructor for " <<G4int(theMode) <<G4endl;}
#endif
  SetParent (theParentNucleus);
  FillParent();
  parent_mass = theParentNucleus->GetPDGMass();
  SetBR (theBR);
  SetNumberOfDaughters (3);
  SetDaughter(0, theDaughterName1);
  SetDaughter(2, theDaughterName2);
  FillDaughterNucleus(1, A, Z, theDaughterExcitation);
  BetaSimple = betaS;
  RandomEnergy = randBeta;
  Qtransition = theQtransition;
  FermiFN = theFFN;
}

////////////////////////////////////////////////////////////////////////////////
//
//
//
//
#include "G4HadTmpUtil.hh"

void G4NuclearDecayChannel::FillDaughterNucleus (G4int index, G4int A, G4int Z,
  G4double theDaughterExcitation)
{
  //
  //
  // Determine if the proposed daughter nucleus has a sensible A, Z and excitation
  // energy.
  //
  if (A<1 || Z<0 || theDaughterExcitation <0.0)
  {
    G4cerr <<"Error in G4NuclearDecayChannel::FillDaughterNucleus";
    G4cerr <<"Inappropriate values of daughter A, Z or excitation" <<G4endl;
    G4cerr <<"A = " <<A <<" and Z = " <<Z;
    G4cerr <<" Ex = " <<theDaughterExcitation*MeV  <<"MeV" <<G4endl;
    G4Exception(__FILE__, G4inttostring(__LINE__), FatalException, "G4G4NuclearDecayChannel::FillDaughterNucleus");
  }
  //
  //
  // Save A and Z to local variables.  Find the GROUND STATE of the daughter
  // nucleus and save this, as an ion, in the array of daughters.
  //
  daughterA = A;
  daughterZ = Z;
  if (Z == 1 && A == 1) {
    daughterNucleus = G4Proton::Definition();
  } else if (Z == 0 && A == 1) {
    daughterNucleus = G4Neutron::Definition();
  } else {
    G4IonTable *theIonTable =
      (G4IonTable*)(G4ParticleTable::GetParticleTable()->GetIonTable());
    daughterNucleus = theIonTable->GetIon(daughterZ, daughterA, theDaughterExcitation*MeV);
  }
  daughterExcitation = theDaughterExcitation;
  SetDaughter(index, daughterNucleus);
}

///////////////////////////////////////////////////////////////////////////////
//
//
//
//

// Heavily modified for gamma decay. Give the daugther nucleus the
// momentum of the parent. This assumes that the user has set up the
// decay properties of the daughter elsewhere.
G4DecayProducts *G4NuclearDecayChannel::DecayIt (G4double theParentMass)
{
  //
  //
  // Load-up the details of the parent and daughter particles if they have not
  // been defined properly.
  //
  if (parent == 0) FillParent();
  if (daughters == 0) FillDaughters();
  //
  //
  // We want to ensure that the difference between the total
  // parent and daughter masses equals the energy liberated by the transition.
  //
  theParentMass = 0.0;
  for( G4int index=0; index < numberOfDaughters; index++)
    {theParentMass += daughters[index]->GetPDGMass();}
  theParentMass += Qtransition  ;
  // bug fix for beta+ decay (flei 25/09/01)
  if (decayMode == 2) theParentMass -= 2*0.511 * MeV;
  //
#ifdef G4VERBOSE
  if (GetVerboseLevel()>1) {
    G4cout << "G4NuclearDecayChannel::DecayIt "<< G4endl;
    G4cout << "the decay mass = " << theParentMass << G4endl;
  }
#endif

  SetParentMass (theParentMass);
  
  //
  //
  // Define a product vector.
  //
  G4DecayProducts *products = 0;
  //
  //
  // Depending upon the number of daughters, select the appropriate decay
  // kinematics scheme.
  //
  switch (numberOfDaughters)
    {
    case 0:
      G4cerr << "G4NuclearDecayChannel::DecayIt ";
      G4cerr << " daughters not defined " <<G4endl;
      break;
    case 1:
      products =  OneBodyDecayIt();
      break;
    case 2:
      products =  TwoBodyDecayIt();
      break;
    case 3:
      products =  BetaDecayIt();
      break;
    default:
      G4cerr <<"Error in G4G4NuclearDecayChannel::DecayIt" <<G4endl;
      G4cerr <<"Number of daughters in decay = " <<numberOfDaughters <<G4endl;
      G4Exception(__FILE__, G4inttostring(__LINE__), FatalException,  "G4NuclearDecayChannel::DecayIt");
    }
  if (products == 0) {
    G4cerr << "G4NuclearDecayChannel::DecayIt ";
    G4cerr << *parent_name << " can not decay " << G4endl;
    DumpInfo();
  }

  return products;
}

////////////////////////////////////////////////////////////////////////////////
//

G4DecayProducts *G4NuclearDecayChannel::BetaDecayIt()

{
  if (GetVerboseLevel()>1) G4cout << "G4Decay::BetaDecayIt()"<<G4endl;

  //daughters'mass
  G4double daughtermass[3];
  G4double sumofdaughtermass = 0.0;
  G4double pmass = GetParentMass();
  for (G4int index=0; index<3; index++)
    {
     daughtermass[index] = daughters[index]->GetPDGMass();
     sumofdaughtermass += daughtermass[index];
    }

  //create parent G4DynamicParticle at rest
  G4ParticleMomentum dummy;
  G4DynamicParticle * parentparticle = new G4DynamicParticle( parent, dummy, 0.0);

  //create G4Decayproducts
  G4DecayProducts *products = new G4DecayProducts(*parentparticle);
  delete parentparticle;

  G4double Q = pmass - sumofdaughtermass;  

  // 09/11/2004 flei
  // All Beta decays are now treated with the improved 3 body decay algorithm. No more slow/fast modes
  /*
  if (BetaSimple == true) {

    // Use the histogramed distribution to generate the beta energy
    G4double daughtermomentum[2];
    G4double daughterenergy[2];
    daughterenergy[0] = RandomEnergy->shoot() * Q;
    daughtermomentum[0] = std::sqrt(daughterenergy[0]*daughterenergy[0] +
			       2.0*daughterenergy[0] * daughtermass[0]);
    // the recoil neuleus is asummed to have a maximum energy of Q/daughterA/1000.
    daughterenergy[1] = G4UniformRand() * Q/(1000.*daughterA);
    G4double recoilmomentumsquared = daughterenergy[1]*daughterenergy[1] +
                               2.0*daughterenergy[1] * daughtermass[1];
    if (recoilmomentumsquared < 0.0) recoilmomentumsquared = 0.0;
    daughtermomentum[1] = std::sqrt(recoilmomentumsquared);
    //
    //create daughter G4DynamicParticle
    G4double costheta, sintheta, phi, sinphi, cosphi;
    //    G4double costhetan, sinthetan, phin, sinphin, cosphin;
    costheta = 2.*G4UniformRand()-1.0;
    sintheta = std::sqrt((1.0-costheta)*(1.0+costheta));
    phi  = twopi*G4UniformRand()*rad;
    sinphi = std::sin(phi);
    cosphi = std::cos(phi);
    G4ParticleMomentum direction0(sintheta*cosphi,sintheta*sinphi,costheta);
    G4DynamicParticle * daughterparticle
      = new G4DynamicParticle( daughters[0], direction0*daughtermomentum[0]);
    products->PushProducts(daughterparticle);
    // The two products are independent in directions
    costheta = 2.*G4UniformRand()-1.0;
    sintheta = std::sqrt((1.0-costheta)*(1.0+costheta));
    phi  = twopi*G4UniformRand()*rad;
    sinphi = std::sin(phi);
    cosphi = std::cos(phi);
    G4ParticleMomentum direction1(sintheta*cosphi,sintheta*sinphi,costheta);
    daughterparticle
      = new G4DynamicParticle( daughters[1], direction1*daughtermomentum[1]);
    products->PushProducts(daughterparticle);

    // the neutrino is igored in this case

  } else {
  */
    /* original slow method  
    //calculate daughter momentum
    //  Generate two
    G4double rd1, rd2;
    G4double daughtermomentum[3];
    G4double daughterenergy[3];
    G4double momentummax=0.0, momentumsum = 0.0;
    G4double fermif;
    G4BetaFermiFunction* aBetaFermiFunction;
    if (decayMode == 1) {
      // beta-decay 
      aBetaFermiFunction = new G4BetaFermiFunction (daughterA, daughterZ);
    } else {
      // beta+decay
      aBetaFermiFunction = new G4BetaFermiFunction (daughterA, -daughterZ);
    }
    if (GetVerboseLevel()>1) {
      G4cout<< " Q =  " <<Q<<G4endl;
      G4cout<< " daughterA =  " <<daughterA<<G4endl;
      G4cout<< " daughterZ =  " <<daughterZ<<G4endl;
      G4cout<< " decayMode = " <<static_cast<G4int>(decayMode) << G4endl;
      G4cout<< " FermiFN =  " <<FermiFN<<G4endl;
    }
    do
      {
	rd1 = G4UniformRand();
	rd2 = G4UniformRand();
	
	momentummax = 0.0;
	momentumsum = 0.0;
	
	// daughter 0
	
	//     energy = rd2*(pmass - sumofdaughtermass);
	daughtermomentum[0] = std::sqrt(rd2) * std::sqrt((Q + 2.0*daughtermass[0])*Q);
	daughterenergy[0] = std::sqrt(daughtermomentum[0]*daughtermomentum[0] +
				 daughtermass[0] * daughtermass[0]) - daughtermass[0];
	if ( daughtermomentum[0] >momentummax )momentummax =  daughtermomentum[0];
	momentumsum  +=  daughtermomentum[0];
	
	// daughter 2
	//     energy = (1.-rd1)*(pmass - sumofdaughtermass);
	daughtermomentum[2] = std::sqrt(rd1)*std::sqrt((Q + 2.0*daughtermass[2])*Q);
	daughterenergy[2] = std::sqrt(daughtermomentum[2]*daughtermomentum[2] +
				 daughtermass[2] * daughtermass[2]) - daughtermass[2];
	if ( daughtermomentum[2] >momentummax )momentummax =  daughtermomentum[2];
	momentumsum  +=  daughtermomentum[2];
	
	// daughter 1
	
	daughterenergy[1] = Q - daughterenergy[0] - daughterenergy[2];
	if (daughterenergy[1] > 0.0) {
	  daughtermomentum[1] = std::sqrt(daughterenergy[1]*daughterenergy[1] +
				     2.0*daughterenergy[1] * daughtermass[1]);
	  if ( daughtermomentum[1] >momentummax ) momentummax =
						    daughtermomentum[1];
	  momentumsum +=  daughtermomentum[1];
	} else {
	  momentummax = momentumsum = Q;
	}
	// beta particles is sampled with no coulomb effects applied above. Now
	// apply the Fermi function using rejection method.
	daughterenergy[0] = daughterenergy[0]*MeV/0.511;
	fermif = aBetaFermiFunction->GetFF(daughterenergy[0])/FermiFN;
	// fermif: normalised Fermi factor
	if (G4UniformRand() > fermif) momentummax = momentumsum =  Q;
	// rejection method
      } while (momentummax >  momentumsum - momentummax );
    delete aBetaFermiFunction;
    
    // output message
    if (GetVerboseLevel()>1) {
      G4cout <<"     daughter 0:" <<daughtermomentum[0]/GeV <<"[GeV/c]" <<G4endl;
      G4cout <<"     daughter 1:" <<daughtermomentum[1]/GeV <<"[GeV/c]" <<G4endl;
      G4cout <<"     daughter 2:" <<daughtermomentum[2]/GeV <<"[GeV/c]" <<G4endl;
      G4cout <<"   momentum sum:" <<momentumsum/GeV <<"[GeV/c]" <<G4endl;
    }
    */
    // faster method as suggested by Dirk Kruecker of FZ-Julich
    G4double daughtermomentum[3];
    G4double daughterenergy[3];
    // Use the histogramed distribution to generate the beta energy
    daughterenergy[0] = RandomEnergy->shoot() * Q;
    daughtermomentum[0] = std::sqrt(daughterenergy[0]*daughterenergy[0] +
			       2.0*daughterenergy[0] * daughtermass[0]);
	 
    //neutrino energy distribution is flat within the kinematical limits
    G4double rd = 2*G4UniformRand()-1;
    // limits
    G4double Mme=pmass-daughtermass[0];
    G4double K=0.5-daughtermass[1]*daughtermass[1]/(2*Mme*Mme-4*pmass*daughterenergy[0]);
	  
    daughterenergy[2]=K*(Mme-daughterenergy[0]+rd*daughtermomentum[0]);
    daughtermomentum[2] = daughterenergy[2] ; 
	  
    // the recoil neuleus
    daughterenergy[1] = Q-daughterenergy[0]-daughterenergy[2];
    G4double recoilmomentumsquared = daughterenergy[1]*daughterenergy[1] +
                               2.0*daughterenergy[1] * daughtermass[1];
    if (recoilmomentumsquared < 0.0) recoilmomentumsquared = 0.0;
    daughtermomentum[1] = std::sqrt(recoilmomentumsquared);
  
    // output message
    if (GetVerboseLevel()>1) {
      G4cout <<"     daughter 0:" <<daughtermomentum[0]/GeV <<"[GeV/c]" <<G4endl;
      G4cout <<"     daughter 1:" <<daughtermomentum[1]/GeV <<"[GeV/c]" <<G4endl;
      G4cout <<"     daughter 2:" <<daughtermomentum[2]/GeV <<"[GeV/c]" <<G4endl;
    }
    //create daughter G4DynamicParticle
    G4double costheta, sintheta, phi, sinphi, cosphi;
    G4double costhetan, sinthetan, phin, sinphin, cosphin;
    costheta = 2.*G4UniformRand()-1.0;
    sintheta = std::sqrt((1.0-costheta)*(1.0+costheta));
    phi  = twopi*G4UniformRand()*rad;
    sinphi = std::sin(phi);
    cosphi = std::cos(phi);
    G4ParticleMomentum direction0(sintheta*cosphi,sintheta*sinphi,costheta);
    G4DynamicParticle * daughterparticle
      = new G4DynamicParticle( daughters[0], direction0*daughtermomentum[0]);
    products->PushProducts(daughterparticle);
    
    costhetan = (daughtermomentum[1]*daughtermomentum[1]-
		 daughtermomentum[2]*daughtermomentum[2]-
		 daughtermomentum[0]*daughtermomentum[0])/
      (2.0*daughtermomentum[2]*daughtermomentum[0]);
    // added the following test to avoid rounding erros. A problem
    // reported bye Ben Morgan of Uni.Warwick
    if (costhetan > 1.) costhetan = 1.;
    if (costhetan < -1.) costhetan = -1.;
    sinthetan = std::sqrt((1.0-costhetan)*(1.0+costhetan));
    phin  = twopi*G4UniformRand()*rad;
    sinphin = std::sin(phin);
    cosphin = std::cos(phin);
    G4ParticleMomentum direction2;
    direction2.setX( sinthetan*cosphin*costheta*cosphi - 
		     sinthetan*sinphin*sinphi + costhetan*sintheta*cosphi);
    direction2.setY( sinthetan*cosphin*costheta*sinphi +
		     sinthetan*sinphin*cosphi + costhetan*sintheta*sinphi);
    direction2.setZ( -sinthetan*cosphin*sintheta +
		     costhetan*costheta);
    daughterparticle = new G4DynamicParticle
      ( daughters[2], direction2*(daughtermomentum[2]/direction2.mag()));
    products->PushProducts(daughterparticle);
    
    daughterparticle =
      new G4DynamicParticle (daughters[1],
			     (direction0*daughtermomentum[0] +
			      direction2*(daughtermomentum[2]/direction2.mag()))*(-1.0));
    products->PushProducts(daughterparticle);
    // }
  //   delete daughterparticle;
  
  if (GetVerboseLevel()>1) {
    G4cout << "G4NuclearDecayChannel::BetaDecayIt ";
    G4cout << "  create decay products in rest frame " <<G4endl;
    products->DumpInfo();
  }
  return products;
}
