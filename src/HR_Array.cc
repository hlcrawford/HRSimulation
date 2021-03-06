#include "HR_Array.hh"

HR_Array::HR_Array()
{
  this->InitData();
}

void HR_Array::InitData()
{

  G4String path = "./";

  if( path.find( "./", 0 ) != string::npos ) {
    G4int position = path.find( "./", 0 );
    if( position == 0 )
      path.erase( position, 2 );
  }
  iniPath = path;

  cryostatStatus        = false;

  // Slot 0 Position (starting point for Euler angle rotations)    
  cryostatPos0.setX(0.);
  cryostatPos0.setY(0.);
  cryostatPos0.setZ(406.); 

  // Slot 1 Position (starting point for Euler angle rotations)    
  //  cryostatPos0.setX(204.741*mm);  // old geometry spec
  //  cryostatPos0.setY( 60.337*mm);  // old geometry spec
  //  cryostatPos0.setZ(345.364*mm);  // old geometry spec

  // approximate polygonal feed-through space with cylinder
  G4double extend = 93.*mm;

  cryostatPos0.setMag(cryostatPos0.mag() - extend);

  cryostatZplanes[0] = 0.;
  cryostatZplanes[1] = extend;
  cryostatZplanes[2] = 145.*mm + extend;
  cryostatZplanes[3] = 155.*mm + extend;
  cryostatZplanes[4] = 217.*mm + extend;
  cryostatZplanes[5] = 217.*mm + extend;
  cryostatZplanes[6] = 232.*mm + extend;

  cryostatRinner[0]  = 0.;
  cryostatRinner[1]  = 0.;
  cryostatRinner[2]  = 0.;
  cryostatRinner[3]  = 0.;
  cryostatRinner[4]  = 0.;
  cryostatRinner[5]  = 0.;
  cryostatRinner[6]  = 0.;

  cryostatRouter[0]  = 100.*mm;
  cryostatRouter[1]  = 130.*mm;
  cryostatRouter[2]  = 130.*mm;
  cryostatRouter[3]  = 140.*mm;
  cryostatRouter[4]  = 140.*mm;
  cryostatRouter[5]  = 165.*mm;
  cryostatRouter[6]  = 165.*mm;

  readOut            = false;

  matCryst           = NULL;
  matWalls           = NULL;
  matBackWalls       = NULL;
  matHole            = NULL;
  matCryo            = NULL;

  matCrystName       = "Germanium";

  matWallsName       = "Aluminium";

  matBackWallsName   = "BackWallMaterial";
  //  matBackWallsName   = "Aluminium";

  matHoleName        = "Vacuum";

  matCryoName        = "Aluminium";

  nEuler             = 0;
  eulerFile          = iniPath + "aeuler";

  nPgons             = 0;
  solidFile          = iniPath + "asolid";

  nWalls             = 0;
  wallsFile          = iniPath + "awalls";
  
  nClAng             = 0;
  clustFile          = iniPath + "aclust";
  
  nDets              = 0;
  iCMin              = 0;
  iGMin              = 0;
  
  maxSec             = 1;
  maxSli             = 1;
  
  arrayRmin          = 0.;
  arrayRmax          = 0.;

  thetaShift         = 0.;
  phiShift           = 0.;
  thetaPrisma        = 0.;
  
  posShift           = G4ThreeVector();
  
  useCylinder        = true;

  usePassive         = true;

  drawReadOut        = false;

  makeCapsule        = false;
  
  maxSolids          = 0;

  totSegments        = 0;
  
  stepFactor         = 1;
  stepHasChanged     = false;
  
  myMessenger        = new HR_Array_Messenger(this);

} 

HR_Array::~HR_Array()
{
  clust.clear();
  pgons.clear();
  euler.clear();
  walls.clear();
  
  nSegments.clear();
  tSegments.clear();

  pgSegLl.clear();
  pgSegLu.clear();
  pgSegRl.clear();
  pgSegRu.clear();

  segVolume.clear();
  segCenter.clear();
  
  delete  myMessenger;
}


void HR_Array::Placement()
{

  // Sensitive Detector
  G4RunManager* runManager = G4RunManager::GetRunManager();
  DetectorConstruction* theDetector  = (DetectorConstruction*) runManager->GetUserDetectorConstruction();

  ReadSolidFile();
  ReadClustFile();
  ReadWallsFile();
  ReadEulerFile();
  // ReadSliceFile();
  
  if( FindMaterials() )
    return;

  ConstructGeCrystals();

  if(nWalls)
    ConstructTheWalls();

  ConstructTheCapsules();

  G4int depth;
  if(makeCapsule)
    depth = 2;
  else
    depth = 1;

  theDetector->GetGammaSD()->SetDepth(depth);

  ConstructTheClusters();
  PlaceTheClusters();

  if( readOut ) {
    // delete old structures
    nSegments.clear();
    
    pgSegLl.clear();
    pgSegLu.clear();
    pgSegRl.clear();
    pgSegRu.clear();
    
    segVolume.clear();
    segCenter.clear();

    nSegments.resize(nPgons);
    tSegments.resize(nPgons);
    totSegments = 0;

    for(G4int ii = 0; ii < nPgons; ii++){
      G4int nn = CalculateSegments(ii);
      nSegments[ii] = nn;
      tSegments[ii] = totSegments;
      totSegments  += nn;
    }
    //ConstructSegments();
  }

}

G4int HR_Array::FindMaterials()
{
  // search the material by its name
  G4Material* ptMaterial = G4Material::GetMaterial(matCrystName);
  if (ptMaterial) {
    matCryst = ptMaterial;
    G4cout << "\n----> The crystals material is " << matCryst->GetName() << G4endl;
  }
  else {
    G4cout << " Could not find the material " << matCrystName << G4endl;
    G4cout << " Could not build the array! " << G4endl;
    return 1;
  }  

  // search the material by its name
  ptMaterial = G4Material::GetMaterial(matWallsName);
  if (ptMaterial) {
    matWalls = ptMaterial;
    G4cout << "\n----> The wall material is " << matWalls->GetName() << G4endl;
  }
  else {
    G4cout << " Could not find the material " << matWallsName << G4endl;
    G4cout << " Could not build the walls! " << G4endl;
  }
    
  // search the material by its name
  ptMaterial = G4Material::GetMaterial(matBackWallsName);
  if (ptMaterial) {
    matBackWalls = ptMaterial;
    G4cout << "\n----> The back wall material is " << matBackWalls->GetName() << G4endl;
  }
  else {
    G4cout << " Could not find the material " << matBackWallsName << G4endl;
    G4cout << " Could not build the walls behind the crystals! " << G4endl;
  }
    
  ptMaterial = G4Material::GetMaterial(matHoleName);
  if (ptMaterial) {
    matHole = ptMaterial;
    G4cout << "\n----> The hole material is " << matHole->GetName() << G4endl;
  }
  else {
    G4cout << " Could not find the material " << matHoleName << G4endl;
    G4cout << " Could not build the capsules! " << G4endl;
  }

  ptMaterial = G4Material::GetMaterial(matCryoName);
  if (ptMaterial) {
    matCryo = ptMaterial;
    G4cout << "\n----> The cryostat material is " << matCryo->GetName() << G4endl;
  }
  else {
    G4cout << " Could not find the material " << matCryoName << G4endl;
    G4cout << " Could not build the cryostats! " << G4endl;
  }
  
  return 0;  
  
}

/////////////////////////////////////////////////////////////
///////////////// methods to read the files
/////////////////////////////////////////////////////////////

void HR_Array:: ReadSolidFile()
{
  FILE      *fp;
  char      line[256];
  G4int     lline, i1, i2, i3, nvdots, opgon;
  float     x, y, z, X, Y, Z;
  
  nPgons =  0;
  nDets  =  0;
  nClus  =  0;
  
  if( (fp = fopen(solidFile, "r")) == NULL) {
    G4cout << "\nError opening data file " << solidFile << G4endl;
    exit(-1);
  }
  G4cout << "\nReading description of crystals from file " << solidFile << " ..." << G4endl;

  pgons.clear();
  
  nvdots   =  0;
  opgon    = -1;
  maxPgons = -1;
  CpolyhPoints *pPg = NULL;

  while(fgets(line, 255, fp) != NULL) {
    lline = strlen(line);
    if(lline < 2) continue;
    if(line[0] == '#') continue;
    if(sscanf(line,"%d %d %d %f %f %f %f %f %f", &i1, &i2, &i3, &x, &y, &z, &X, &Y, &Z) != 9) {
      nPgons++;
      break;
    }
    if(opgon != i1) {
      nPgons++;
      opgon = i1;
      pgons.push_back( CpolyhPoints() );
      pPg = &pgons.back();
      pPg->whichGe  = i1;
      if( i1 > maxPgons )
        maxPgons = i1;
      pPg->npoints  = 2*i2;
      pPg->tubX       = -1.*mm;
      pPg->tubY       = -1.*mm;
      pPg->tubZ       = -1.*mm;
      pPg->tubr       = -1.*mm;
      pPg->tubR       = -1.*mm;
      pPg->tubL       = -1.*mm;
      pPg->capSpace   = -1.*mm;
      pPg->capThick   = -1.*mm;
      pPg->passThick1 = -1.*mm;
      pPg->passThick2 = -1.*mm;
      pPg->colx       =  0.;
      pPg->coly       =  0.;
      pPg->colz       =  0.;
      pPg->vertex.resize(pPg->npoints);
      pPg->cylinderMakesSense = true;
      pPg->makeCapsule        = true;
      pPg->isPlanar           = false;
      pPg->segSize_x  = -1.*mm;
      pPg->segSize_y  = -1.*mm;
      pPg->maxSize_x  = -1000.*m;
      pPg->maxSize_y  = -1000.*m;
      pPg->minSize_x  =  1000.*m;
      pPg->minSize_y  =  1000.*m;
      pPg->guardThick[0] = -1.*mm;
      pPg->guardThick[1] = -1.*mm;
      pPg->guardThick[2] = -1.*mm;
      pPg->guardThick[3] = -1.*mm;
      pPg->nSeg_x     =  1;
      pPg->nSeg_y     =  1;
    }
    if(i2==0 && i3==0) {
      pPg->tubr = ((G4double)x) * mm;
      pPg->tubR = ((G4double)y) * mm;
      pPg->tubL = ((G4double)z) * mm;
      pPg->tubX = ((G4double)X) * mm;
      pPg->tubY = ((G4double)Y) * mm;
      pPg->tubZ = ((G4double)Z) * mm;
    }
    else if(i2==0 && i3==1) {
      pPg->thick      = ((G4double)x) * mm;
      pPg->passThick1 = ((G4double)y) * mm;
      pPg->passThick2 = ((G4double)z) * mm;
      pPg->capSpace   = ((G4double)X) * mm;
      pPg->capThick   = ((G4double)Y) * mm;
    }
    else if(i2==0 && i3==2) {
      pPg->colx     = ((G4double)x);
      pPg->coly     = ((G4double)y);
      pPg->colz     = ((G4double)z);
    }
    else if(i2==0 && i3==3) {   // planar
      pPg->cylinderMakesSense = false;
      pPg->makeCapsule        = false;
      pPg->isPlanar           = true;
      pPg->nSeg_x     =     (G4int)x;
      pPg->nSeg_y     =     (G4int)y;
      pPg->guardThick[0] = ((G4double)z) * mm;
      pPg->guardThick[1] = ((G4double)X) * mm;
      pPg->guardThick[2] = ((G4double)Y) * mm;
      pPg->guardThick[3] = ((G4double)Z) * mm;
    }
    else {
      pPg->vertex[i3   ] = G4Point3D( ((G4double)x), ((G4double)y),  ((G4double)z) ) * mm;
      pPg->vertex[i3+i2] = G4Point3D( ((G4double)X), ((G4double)Y),  ((G4double)Z) ) * mm;
      nvdots += 2;
    }
  }

  fclose(fp);
  G4cout << nPgons << " polyhedra for a total of " << nvdots << " vertex points read." << G4endl;
  
  G4int npt, npg, nn;
  G4double tolerance = 0.5*mm;

  for(npg = 0; npg < nPgons; npg++) {
    pPg = &pgons[npg];
    npt =  pPg->npoints;
    if(!npt) continue;

    // Calculates z of the two faces of the original polyhedron
    pPg->centerFace1 = G4Point3D();
    pPg->centerFace2 = G4Point3D();
    for (nn=0; nn< npt/2; nn++) {
      pPg->centerFace1 += pPg->vertex[nn];
      pPg->centerFace2 += pPg->vertex[nn+npt/2];
    }
    pPg->centerFace1 /= npt/2;
    pPg->centerFace2 /= npt/2;

    pPg->zFace1 = pPg->centerFace1.z();
    pPg->zFace2 = pPg->centerFace2.z();
    pPg->zCenter = 0.5 * (pPg->zFace1 + pPg->zFace2);

    // Calculates the minimum radius of a cylinder surrounding the polyhedron
    G4double minR2 = 0;
    G4double theR2;

    for (nn=0; nn<npt; nn++) {
      theR2 = pow( pPg->vertex[nn].x(), 2.) + pow(pPg->vertex[nn].y(), 2.);
      if (theR2 > minR2) minR2 = theR2;
    }
    pPg->minR = sqrt(minR2) + 2.*tolerance; // Safety margin!
    
    // Planar detectors
    if( pPg->isPlanar ) {
      // Number of segments
      if( pPg->nSeg_x <= 0 ) {
        G4cout << " Warning! Invalid segment number for solid " 
	       << pPg->whichGe << ", set to 1 instead." << G4endl;
	pPg->nSeg_x = 1;       
      }
      if( pPg->nSeg_y <= 0 ) {
        G4cout << " Warning! Invalid segment number for solid " 
	       << pPg->whichGe << ", set to 1 instead." << G4endl;
	pPg->nSeg_y = 1;       
      }
    
      // Max and min coordinates
      for( nn=0; nn<pPg->npoints; nn++ ) {
        if( pPg->vertex[nn].x() > pPg->maxSize_x ) pPg->maxSize_x = pPg->vertex[nn].x();
        if( pPg->vertex[nn].y() > pPg->maxSize_y ) pPg->maxSize_y = pPg->vertex[nn].y();
        if( pPg->vertex[nn].x() < pPg->minSize_x ) pPg->minSize_x = pPg->vertex[nn].x();
        if( pPg->vertex[nn].y() < pPg->minSize_y ) pPg->minSize_y = pPg->vertex[nn].y();
      }
      
      // segment size
      pPg->segSize_x = ( pPg->maxSize_x - pPg->minSize_x ) / pPg->nSeg_x;
      pPg->segSize_y = ( pPg->maxSize_y - pPg->minSize_y ) / pPg->nSeg_y;
    }
  }
  
}

void HR_Array:: ReadWallsFile()
{
  FILE      *fp;
  char      line[256];
  G4int     lline, i1, i2, i3, i4, i5, i6, nvdots, opgon;
  float     x, y, z, X, Y, Z;
  CpolyhPoints *pPg = NULL;

  nWalls = 0;
  nWlTot = 0;
  if(!wallsFile) return;

  if( (fp = fopen(wallsFile, "r")) == NULL) {
    G4cout << "\nError opening data file " << wallsFile << G4endl;
    G4cout << "No walls included." << G4endl;
    return;
  }

  G4cout << "\nReading description of walls from file " << wallsFile << " ..." << G4endl;

  walls.clear();
  
  nvdots = 0;
  opgon  = -1;
#ifdef GRETA
#ifdef GRETA_DEBUG
  // writes out a modified copy of the original file
  G4bool isOpen = true;
  FILE *fp1;
  if( (fp1=fopen("awallsG","w"))==NULL )
    isOpen = false;
#endif
#endif    

  while(fgets(line, 255, fp) != NULL) {
    lline = strlen(line);
    if(lline < 1) continue;
    if(line[0] == '#') continue;
//    if(sscanf(line,"%d %d %d %d %d %d %lf %lf %lf %lf %lf %lf", 
    if(sscanf(line,"%d %d %d %d %d %d %f %f %f %f %f %f", 
         &i1, &i2, &i3, &i4, &i5, &i6, &x, &y, &z, &X, &Y, &Z) != 12)
      break;
    if(opgon != i3) {
      nWalls++;
      opgon = i3;
      walls.push_back( CpolyhPoints() );
      pPg = &walls.back();
      pPg->whichGe   = i1;
      pPg->whichCrystal = i2;
      pPg->whichWall = i3;
      pPg->npoints   = 2*i5;
      pPg->vertex.resize(pPg->npoints);
    }
    pPg->vertex[i6   ] = G4Point3D( ((G4double)x), ((G4double)y),  ((G4double)z) ) * mm;
    pPg->vertex[i6+i5] = G4Point3D( ((G4double)X), ((G4double)Y),  ((G4double)Z) ) * mm;
    nvdots += 2;
    
#ifdef GRETA
#ifdef GRETA_DEBUG
    if( isOpen ) {
      CclusterAngles *pCa = NULL;
      CeulerAngles   *pEa = NULL;
      for( G4int iii=0; iii<nClAng; iii++ ) {
        pCa = &clust[iii];
        if( pCa->whichClus == pPg->whichGe ) break;
      }
      for( G4int jjj=0; jjj<pCa->nsolids; jjj++ ) {
        pEa = &(pCa->solids[jjj]);
        if( pEa->numPhys == pPg->whichCrystal ) break;
      }
//      fprintf( fp1, "%4.1d %4.1d %4.1d %4.1d %4.1d %4.1d %12.6lf %12.6lf %12.6lf %12.6lf %12.6lf %12.6lf\n",
      fprintf( fp1, "%4.1d %4.1d %4.1d %4.1d %4.1d %4.1d %12.6f %12.6f %12.6f %12.6f %12.6f %12.6f\n",
        pPg->whichGe, pPg->whichCrystal, pPg->whichWall, i4, pPg->npoints/2, i6,
	(*(pEa->pTransf) * G4Point3D( x * mm, y * mm,  z * mm )).x()/mm,
	(*(pEa->pTransf) * G4Point3D( x * mm, y * mm,  z * mm )).y()/mm,
	(*(pEa->pTransf) * G4Point3D( x * mm, y * mm,  z * mm )).z()/mm,
	(*(pEa->pTransf) * G4Point3D( X * mm, Y * mm,  Z * mm )).x()/mm,
	(*(pEa->pTransf) * G4Point3D( X * mm, Y * mm,  Z * mm )).y()/mm,
	(*(pEa->pTransf) * G4Point3D( X * mm, Y * mm,  Z * mm )).z()/mm ); 
//	  ((pEa->rotMat) * G4Point3D( ((G4double)x), ((G4double)y),  ((G4double)z) )).x(),
//	  ((pEa->rotMat) * G4Point3D( ((G4double)x), ((G4double)y),  ((G4double)z) )).y(),
//	  ((pEa->rotMat) * G4Point3D( ((G4double)x), ((G4double)y),  ((G4double)z) )).z(),
//	  ((pEa->rotMat) * G4Point3D( ((G4double)X), ((G4double)Y),  ((G4double)Z) )).x(),
//	  ((pEa->rotMat) * G4Point3D( ((G4double)X), ((G4double)Y),  ((G4double)Z) )).y(),
//	  ((pEa->rotMat) * G4Point3D( ((G4double)X), ((G4double)Y),  ((G4double)Z) )).z() ); 
    }
#endif
#endif    
  }

  fclose(fp);
#ifdef GRETA
#ifdef GRETA_DEBUG
  fclose(fp1);
#endif
#endif    
  G4cout << nWalls << " polyhedra for a total of " << nvdots << " vertex points read." << G4endl;
}

void HR_Array:: ReadClustFile()
{
  FILE      *fp;
  char      line[256];
  G4int     lline, i1, i2, i3, nsolids, oclust;
  float  ps, th, ph;
  float  x, y, z;
  
  nClAng    =  0;
  maxSolids = 0;
  
  if( (fp = fopen(clustFile, "r")) == NULL) {
    G4cout << "\nError opening data file " << clustFile << G4endl;
    exit(-1);
  }
  
  G4RotationMatrix rm;

  G4cout << "\nReading description of clusters from file " << clustFile << " ..." << G4endl;

  clust.clear();
  
  nsolids =  0;
  oclust  = -1;
  CclusterAngles *pPg = NULL;
  CeulerAngles   *pEa = NULL;

  while(fgets(line, 255, fp) != NULL) {
    lline = strlen(line);
    if(lline < 2) continue;
    if(line[0] == '#') continue;
    if(sscanf(line,"%d %d %d %f %f %f %f %f %f", &i1, &i2, &i3, &ps, &th, &ph, &x, &y, &z) != 9) {
      nClAng++;
      break;
    }
    if(oclust != i1) {
      nClAng++;
      oclust = i1;
      clust.push_back( CclusterAngles() );
      pPg = &clust.back();
      pPg->whichClus = i1;
      pPg->nsolids   = 0;
      pPg->solids.clear();
      pPg->nwalls   = 0;
      pPg->pAssV = new G4AssemblyVolume();
    }
    pPg->solids.push_back( CeulerAngles() );
    pEa = &pPg->solids.back();
    pEa->whichGe = i2;
    pEa->numPhys = i3;
    
    pEa->ps = ((G4double)ps) * deg;
    pEa->th = ((G4double)th) * deg;
    pEa->ph = ((G4double)ph) * deg;
    
    pEa->rotMat.set(0,0,0);
    pEa->rotMat.rotateZ(((G4double)ps) * deg);
    pEa->rotMat.rotateY(((G4double)th) * deg);
    pEa->rotMat.rotateZ(((G4double)ph) * deg);
    
    pEa->trasl = G4ThreeVector( ((G4double)x), ((G4double)y), ((G4double)z) ) * mm;

    pEa->pTransf = new G4Transform3D( pEa->rotMat, pEa->trasl );
    
    pPg->nsolids++;        
    nsolids++;    
  }

  fclose(fp);
  
  for( G4int ii=0; ii<nClAng; ii++ ) {
    pPg = &clust[ii];
    if( pPg->nsolids > maxSolids )
      maxSolids = pPg->nsolids;
  } 
  
  G4cout << " Read " << nClAng << " cluster description for a total of " << nsolids << " individual solids." << G4endl;
}

void HR_Array:: ReadEulerFile()
{
  FILE      *fp;
  char      line[256];
  G4int     lline, i1, i2;
  float     ps, th, ph, x, y, z;
  
  if( (fp = fopen(eulerFile, "r")) == NULL) {
    G4cout << "\nError opening data file " << eulerFile << G4endl;
    exit(-1);
  }

  euler.clear();

  G4cout << "\nReading Euler angles from file " << eulerFile << " ..." << G4endl;
  nEuler = 0;
  
  G4RotationMatrix  rm;
  CeulerAngles     *pEa = NULL;

  while(fgets(line, 255, fp) != NULL) {
    lline = strlen(line);
    if(lline < 2) continue;
    if(line[0] == '#') continue;
//    if(sscanf(line,"%d %d %lf %lf %lf %lf %lf %lf", &i1, &i2, &ps, &th, &ph, &x, &y, &z) != 8)
    if(sscanf(line,"%d %d %f %f %f %f %f %f", &i1, &i2, &ps, &th, &ph, &x, &y, &z) != 8)
      break;
    euler.push_back( CeulerAngles() );
    pEa = &euler[nEuler];
    
    
    pEa->numPhys = i1;
    pEa->whichGe = i2;

    pEa->rotMat.set( 0, 0, 0 );
    pEa->rotMat.rotateZ(((G4double)ps)*deg);
    pEa->rotMat.rotateY(((G4double)th)*deg);
    pEa->rotMat.rotateZ(((G4double)ph)*deg);
    
    pEa->ps      = ((G4double)ps)*deg;
    pEa->th      = ((G4double)th)*deg;
    pEa->ph      = ((G4double)ph)*deg;
    
    pEa->trasl   = G4ThreeVector( ((G4double)x), ((G4double)y), ((G4double)z) ) * mm;
    
    pEa->pTransf = new G4Transform3D( pEa->rotMat, pEa->trasl );
    
    nEuler++;
  }

  fclose(fp);
  G4cout << nEuler << " Euler angles read." << G4endl;
}

///////////////////////////////////////////////////////////////////////////////////////
///////////////// methods to construct and place the actual volumes
/////////////////////////////////////////////////////////////////////////////////////

void HR_Array::ConstructGeCrystals()
{
    
  G4RunManager* runManager = G4RunManager::GetRunManager();

  DetectorConstruction* theDetector  = (DetectorConstruction*) runManager->GetUserDetectorConstruction();
  
  if( !matCryst ) {
    G4cout << G4endl << "----> Missing material, cannot build the crystals!" << G4endl;
    return;
  }
  
  char sName[50];
  
  // identity matrix
  G4RotationMatrix rm;
  rm.set(0,0,0);

  G4int ngen, nPg, nGe, nPh = 0, nPt;

  // data to construct the cylinders and the passive parts
  // they are generated in their final position to avoid problems in placement
  G4double *InnRadGe;
  G4double *OutRadGe;
  G4double *zSliceGe;
  
  G4cout << G4endl << "Generating crystals ... " << G4endl;
  ngen = 0;
  for(nPg = 0; nPg < nPgons; nPg++) {
    CpolyhPoints *pPg = &pgons[nPg];
    nGe = pPg->whichGe;
    if(nGe < 0) {
      G4cout << "DetectorConstruction::ConstructGeCrystals : crystal " << nPg
             << " skipped because nGe ( " << nGe << " ) out of range " << G4endl;
      continue;
    }
    nPt = pPg->npoints;
    if(nPt < 6) {
      G4cout << "DetectorConstruction::ConstructGeCrystals : crystal " << nPg
             << " skipped because of too few ( " << nPt << " ) points " << G4endl;
      continue;
    }

    sprintf(sName, "gePoly%2.2d", nGe);
    pPg->pPoly  = new CConvexPolyhedron(G4String(sName), pPg->vertex);
    
    pPg->pDetVA  = new G4VisAttributes( G4Color(pPg->colx, pPg->coly, pPg->colz) );
    // pPg->pDetVA->SetForceWireframe(true); /* ADDED */
    
    G4double zFace1 = pPg->zFace1;
    
    pPg->pDetL1 = NULL;
    pPg->pDetL2 = NULL;

    if ( !useCylinder || !pPg->cylinderMakesSense ) {
      sprintf(sName, "geDetPolyL%2.2d", nGe);
      pPg->pDetL  = new G4LogicalVolume( pPg->pPoly, matCryst, G4String(sName), 0, 0, 0 ); 
      
      // Passive area behind the detector (placed later as daughter of the original crystal)
      if( usePassive ) {
        if( pPg->passThick1 > 0. ) {
          zSliceGe = new G4double[2];
          zSliceGe[0] = pPg->zCenter+pPg->tubL/2.-pPg->passThick1;
          zSliceGe[1] = pPg->zCenter+pPg->tubL/2.;

          InnRadGe = new G4double[2];
          InnRadGe[0] = 0.;
          InnRadGe[1] = 0.;

          OutRadGe = new G4double[2];
          OutRadGe[0] = pPg->minR;
          OutRadGe[1] = pPg->minR;

          sprintf(sName, "geTubsB%2.2d", nGe);
          pPg->pTubs1 = new G4Polycone(G4String(sName), 0.*deg, 360.*deg, 2, zSliceGe, InnRadGe, OutRadGe );
          sprintf(sName, "gePassB%2.2d", nGe);
          pPg->pCaps1 = new G4IntersectionSolid(G4String(sName), pPg->pPoly, pPg->pTubs1, G4Transform3D( rm, G4ThreeVector() ) );
          sprintf(sName, "gePassBL%2.2d", nGe);
          pPg->pDetL1 = new G4LogicalVolume( pPg->pCaps1, matCryst, G4String(sName), 0, 0, 0 );
          pPg->pDetL1->SetVisAttributes( pPg->pDetVA );
	}
        else
          pPg->pDetL1 = NULL;
      }            
    }

    if( usePassive ) {
      if( pPg->pDetL1 ) {
        pPg->pDetP1 = NULL;
        sprintf(sName, "gePassBP%3.3d", nPh);
        pPg->pDetP1 =  new G4PVPlacement( 0, G4ThreeVector(), pPg->pDetL1, G4String(sName), pPg->pDetL, false, 0);
      } 
    }
    
    pPg->pDetL->SetVisAttributes( pPg->pDetVA );
    pPg->pDetL->SetSensitiveDetector( theDetector->GetGammaSD() );    //LR
    ngen++;
  }
  G4cout << "Number of generated crystals is " << ngen << G4endl;
}

void HR_Array::ConstructTheCapsules()
{
  G4int    nPg, nSid, nGe;
  G4bool   movePlane;
  G4double dist1 = 0.;
  G4double dist2 = 0.;
  char     sName[128];
  
  CpolyhPoints* pPg = NULL;
  CpolyhPoints* pPv = NULL;
  CpolyhPoints* pPc = NULL;
  
  G4RotationMatrix rm;
  rm.set(0, 0, 0);
  
  if( !matWalls || !matBackWalls || !matHole ) {
    G4cout << G4endl << "----> Missing materials, cannot build the capsules!" << G4endl;
    return;
  }

  G4cout << G4endl << "Generating the capsules ... " << G4endl;

  capsI.clear();
  capsO.clear();
  
  capsI.resize( nPgons );
  capsO.resize( nPgons );
  
//  dist2 += dist1;
  
  for( nPg=0; nPg<nPgons; nPg++ ) {
    pPg = &pgons[nPg];
    
    nGe = pPg->whichGe;
    
    if( pPg->isPlanar ) {  // planar: no capsule, guardring
   //   dist1 = pPg->guardThick;
   //   dist2 = dist1;
      
      if( makeCapsule ) {
	pPv = &capsO[nPg];  
	sprintf(sName, "plaPoly%2.2d", nGe);
	pPv->pPoly  = new CConvexPolyhedron(G4String(sName), pPg->vertex);
  //      for( nSid=0; nSid<pPg->pPoly->GetnPlanes()-2; nSid++ )
	for( nSid=2; nSid<pPg->pPoly->GetnPlanes(); nSid++ )
	  movePlane = pPv->pPoly->MovePlane( nSid, pPg->guardThick[nSid%4] );
	pPv->whichGe = nGe;
	sprintf(sName, "plaPolyL%2.2d", nGe);
	pPv->pDetL = new G4LogicalVolume( pPv->pPoly, matCryst, G4String(sName), 0, 0, 0 ); 
	pPv->pDetVA  = new G4VisAttributes( G4Color(pPg->colx, pPg->coly, pPg->colz) );
	pPv->pDetVA->SetForceWireframe(true);
	pPv->pDetL->SetVisAttributes( pPv->pDetVA );

	pPc = &capsI[nPg];  
	sprintf(sName, "plbPoly%2.2d", nGe);
	pPc->pPoly  = new CConvexPolyhedron(G4String(sName), pPg->vertex);
//	for( nSid=0; nSid<pPg->pPoly->GetnPlanes()-2; nSid++ )
	for( nSid=2; nSid<pPg->pPoly->GetnPlanes(); nSid++ )
	  movePlane = pPc->pPoly->MovePlane( nSid, pPg->guardThick[nSid%4] );
	pPc->whichGe = nGe;
	sprintf(sName, "plbPolyL%2.2d", nGe);
	pPc->pDetL = new G4LogicalVolume( pPc->pPoly, matCryst, G4String(sName), 0, 0, 0 );
	pPc->pDetVA  = new G4VisAttributes( G4Color(pPg->colx, pPg->coly, pPg->colz) );
	pPc->pDetVA->SetForceWireframe(true);
	pPc->pDetL->SetVisAttributes( pPc->pDetVA );

        new G4PVPlacement( 0, G4ThreeVector(), pPc->pDetL, G4String(sName), pPv->pDetL, false, 0);
	new G4PVPlacement( 0, G4ThreeVector(), pPg->pDetL, G4String(sName), pPc->pDetL, false, 0);
      }
      else {
	pPv = &capsO[nPg];  
	sprintf(sName, "plaPoly%2.2d", nGe);
	pPv->pPoly  = new CConvexPolyhedron(G4String(sName), pPg->vertex);
  //      for( nSid=0; nSid<pPg->pPoly->GetnPlanes()-2; nSid++ )
	for( nSid=2; nSid<pPg->pPoly->GetnPlanes(); nSid++ )
	  movePlane = pPv->pPoly->MovePlane( nSid, pPg->guardThick[nSid%4] );
	pPv->whichGe = nGe;
	sprintf(sName, "plaPolyL%2.2d", nGe);
	pPv->pDetL = new G4LogicalVolume( pPv->pPoly, matCryst, G4String(sName), 0, 0, 0 ); 
	pPv->pDetVA  = new G4VisAttributes( G4Color(pPg->colx, pPg->coly, pPg->colz) );
	pPv->pDetVA->SetForceWireframe(true);
	pPv->pDetL->SetVisAttributes( pPv->pDetVA );
	new G4PVPlacement( 0, G4ThreeVector(), pPg->pDetL, G4String(sName), pPv->pDetL, false, 0);
      }
    }
    else if( !pPg->makeCapsule ) {  // no capsule
      if( makeCapsule ) {
	pPv = &capsO[nPg];  
	sprintf(sName, "cryPoly%2.2d", nGe);
	pPv->pPoly  = new CConvexPolyhedron(G4String(sName), pPg->vertex);
	sprintf(sName, "cryCoax%2.2d", nGe);
        pPv->pCoax  = new G4Polycone( *(pPg->pCoax) );
	sprintf(sName, "cryCaps%2.2d", nGe);
        pPv->pCaps = new G4IntersectionSolid(G4String(sName), pPv->pPoly, pPv->pCoax, G4Transform3D( rm, G4ThreeVector() ) );	
	pPv->whichGe = nGe;
	sprintf(sName, "cryPolyL%2.2d", nGe);
	pPv->pDetL = new G4LogicalVolume( pPv->pCaps, matHole, G4String(sName), 0, 0, 0 ); 
	pPv->pDetVA  = new G4VisAttributes( G4Color(pPg->colx, pPg->coly, pPg->colz) );
	pPv->pDetVA->SetForceWireframe(true);
	pPv->pDetVA->SetVisibility(false);
	pPv->pDetVA->SetDaughtersInvisible(false);
	pPv->pDetL->SetVisAttributes( pPv->pDetVA );

	pPc = &capsI[nPg];  
	sprintf(sName, "crzPoly%2.2d", nGe);
	pPc->pPoly  = new CConvexPolyhedron(G4String(sName), pPg->vertex);
	sprintf(sName, "crzCoax%2.2d", nGe);
        pPc->pCoax  = new G4Polycone( *(pPg->pCoax) );
	sprintf(sName, "crzCaps%2.2d", nGe);
        pPc->pCaps = new G4IntersectionSolid(G4String(sName), pPc->pPoly, pPc->pCoax, G4Transform3D( rm, G4ThreeVector() ) );	
	pPc->whichGe = nGe;
	sprintf(sName, "crzPolyL%2.2d", nGe);
	pPc->pDetL = new G4LogicalVolume( pPc->pCaps, matHole, G4String(sName), 0, 0, 0 ); 
	pPc->pDetVA  = new G4VisAttributes( G4Color(pPg->colx, pPg->coly, pPg->colz) );
	pPc->pDetVA->SetForceWireframe(true);
	pPc->pDetVA->SetVisibility(false);
	pPc->pDetVA->SetDaughtersInvisible(false);
	pPc->pDetL->SetVisAttributes( pPc->pDetVA );

	new G4PVPlacement( 0, G4ThreeVector(), pPg->pDetL, G4String(sName), pPc->pDetL, false, 0);
        new G4PVPlacement( 0, G4ThreeVector(), pPc->pDetL, G4String(sName), pPv->pDetL, false, 0);
      }
      else {
	pPv = &capsO[nPg];  
	sprintf(sName, "cryPoly%2.2d", nGe);
        pPv->pPoly  = new CConvexPolyhedron(G4String(sName), pPg->vertex);
	sprintf(sName, "cryCoax%2.2d", nGe);
        pPv->pCoax  = new G4Polycone( *(pPg->pCoax) );
	sprintf(sName, "cryCaps%2.2d", nGe);
        pPv->pCaps = new G4IntersectionSolid(G4String(sName), pPv->pPoly, pPv->pCoax, G4Transform3D( rm, G4ThreeVector() ) );	
	pPv->whichGe = nGe;
	sprintf(sName, "cryPolyL%2.2d", nGe);
	pPv->pDetL = new G4LogicalVolume( pPv->pCaps, matHole, G4String(sName), 0, 0, 0 ); 
	pPv->pDetVA  = new G4VisAttributes( G4Color(pPg->colx, pPg->coly, pPg->colz) );
	pPv->pDetVA->SetForceWireframe(true);
	pPv->pDetVA->SetVisibility(false);
	pPv->pDetVA->SetDaughtersInvisible(false);
	pPv->pDetL->SetVisAttributes( pPv->pDetVA );
	new G4PVPlacement( 0, G4ThreeVector(), pPg->pDetL, G4String(sName), pPv->pDetL, false, 0);
      }
    }
    else {
      dist1 = pPg->capSpace;
      dist2 = dist1 + pPg->capThick;
      if( makeCapsule ) {

	pPv = &capsI[nPg];  
	sprintf(sName, "vaPoly%2.2d", nGe);
	pPv->pPoly  = new CConvexPolyhedron(G4String(sName), pPg->vertex);
	for( nSid=0; nSid<pPg->pPoly->GetnPlanes(); nSid++ )
	  movePlane = pPv->pPoly->MovePlane( nSid, dist1 );
	pPv->whichGe = nGe;
	sprintf(sName, "geVacPolyL%2.2d", nGe);
	pPv->pDetL = new G4LogicalVolume( pPv->pPoly, matHole, G4String(sName), 0, 0, 0 ); 
	//LR	pPv->pDetVA  = new G4VisAttributes( G4Color(pPg->colx, pPg->coly, pPg->colz) );
	pPv->pDetVA  = new G4VisAttributes( G4Color(0, 0, 0) );
	pPv->pDetVA->SetForceWireframe(true);
	pPv->pDetVA->SetVisibility(false);
	pPv->pDetL->SetVisAttributes( pPv->pDetVA );

	pPc = &capsO[nPg];  
	sprintf(sName, "caPoly%2.2d", nGe);
	pPc->pPoly  = new CConvexPolyhedron(G4String(sName), pPg->vertex);
	for( nSid=0; nSid<pPg->pPoly->GetnPlanes(); nSid++ )
	  movePlane = pPc->pPoly->MovePlane( nSid, dist2 );
	pPc->whichGe = nGe;
	sprintf(sName, "geCapPolyL%2.2d", nGe);
	pPc->pDetL = new G4LogicalVolume( pPc->pPoly, matWalls, G4String(sName), 0, 0, 0 );
	//LR pPc->pDetVA  = new G4VisAttributes( G4Color(pPg->colx, pPg->coly, pPg->colz) );
	pPc->pDetVA  = new G4VisAttributes( G4Color(0, 0, 0.5) );
	pPc->pDetVA->SetForceWireframe(true);
	pPc->pDetL->SetVisAttributes( pPc->pDetVA );

	new G4PVPlacement( 0, G4ThreeVector(), pPg->pDetL, G4String(sName), pPv->pDetL, false, 0);
	new G4PVPlacement( 0, G4ThreeVector(), pPv->pDetL, G4String(sName), pPc->pDetL, false, 0);
      }
      else {
	pPv = &capsO[nPg];  
	sprintf(sName, "cryPoly%2.2d", nGe);
	pPv->pPoly  = new CConvexPolyhedron(G4String(sName), pPg->vertex);
	for( nSid=0; nSid<pPg->pPoly->GetnPlanes(); nSid++ )
	  movePlane = pPv->pPoly->MovePlane( nSid, dist1 );
	pPv->whichGe = nGe;
	sprintf(sName, "cryPolyL%2.2d", nGe);
	pPv->pDetL = new G4LogicalVolume( pPv->pPoly, matHole, G4String(sName), 0, 0, 0 ); 
	pPv->pDetVA  = new G4VisAttributes( G4Color(pPg->colx, pPg->coly, pPg->colz) );
	pPv->pDetVA->SetForceWireframe(true);
	pPv->pDetVA->SetVisibility(false);
	pPv->pDetVA->SetDaughtersInvisible(false);
	pPv->pDetL->SetVisAttributes( pPv->pDetVA );
	new G4PVPlacement( 0, G4ThreeVector(), pPg->pDetL, G4String(sName), pPv->pDetL, false, 0);
      }
    }
  }
  G4cout << "Number of generated capsules is " << nPgons << G4endl;
}

void HR_Array::ConstructTheWalls()
{
  char sName[50];
  G4int ngen, nPg, nGe, nPt;
  
  if( !matWalls || !matBackWalls ) {
    G4cout << G4endl << "----> Missing material, cannot build the walls!" << G4endl;
    return;
  }
  
  G4cout << G4endl << "Generating walls ... " << G4endl;

  ngen = 0;
  for(nPg = 0; nPg < nWalls; nPg++) {
    CpolyhPoints *pPg = &walls[nPg];
    nGe = pPg->whichGe;
    if(nGe < 0 || nGe >= nPgons) continue;
    nPt = pPg->npoints;
    if(nPt >=6) {
      sprintf(sName, "wlPoly%2.2d", nGe);
      pPg->pPoly  = new CConvexPolyhedron(G4String(sName), pPg->vertex );

      sprintf(sName, "wlDetL%2.2d", nGe);
      // Use a different material for the back walls                        //LR
      if(nPg < 18)                                                          //LR
	pPg->pDetL  = new G4LogicalVolume( pPg->pPoly, matWalls, G4String(sName), 0, 0, 0 );
      else                                                                  //LR
	pPg->pDetL  = new G4LogicalVolume( pPg->pPoly, matBackWalls, G4String(sName), 0, 0, 0 );                                                            //LR

      //LR      pPg->pDetVA = new G4VisAttributes( G4Colour(0.5, 0.5, 0.5) );
      pPg->pDetVA = new G4VisAttributes( G4Colour(1, 1, 1) );
      pPg->pDetVA->SetForceWireframe(true);
      pPg->pDetL->SetVisAttributes( pPg->pDetVA );

      ngen++;
    }
  }
  G4cout << "Number of generated walls is " << ngen << G4endl;
}

void HR_Array::ConstructTheClusters()
{
  
  G4int            nCa, nPg, nSo;
  CclusterAngles*  pCa;
  CpolyhPoints*    pPg;
  CeulerAngles*    pEa;
  G4RotationMatrix rm;
  G4ThreeVector    rotatedPos;
  G4Transform3D    transf;

  G4cout << G4endl << "Building the clusters ..." << G4endl;
    
  for( nCa=0; nCa<nClAng; nCa++ ) {
    G4cout << " Cluster #" << nCa << G4endl;
    pCa = &clust[nCa];
    for( nSo=0; nSo<pCa->nsolids; nSo++ ) {
      pEa = &pCa->solids[nSo];
      
      rm = pEa->rotMat;

      rotatedPos = pEa->trasl;
      
      // germanium detectors
      for( nPg=0; nPg<nPgons; nPg++ ) {
//        if( makeCapsule && pgons[nPg].makeCapsule )
//          pPg = &capsO[nPg];
//        else
//          pPg = &pgons[nPg];  
        pPg = &capsO[nPg];  
        
        if( pPg->whichGe != pEa->whichGe )
          continue;
        if( !pPg->pDetL )
          continue;
          
        if( pCa->pAssV ) {
          transf = G4Transform3D( *(pEa->pTransf) );
          pCa->pAssV->AddPlacedVolume( pPg->pDetL, transf );
        }
        
        printf( "  Solid %4d      %9.2f %9.2f %9.2f %9.2f %9.2f %9.2f\n", 
          pPg->whichGe, pEa->ps/deg, pEa->th/deg, pEa->ph/deg, rotatedPos.x()/cm, rotatedPos.y()/cm, rotatedPos.z()/cm );      
       
      }
    }
    // the walls
#ifdef GRETA
    for( nSo=0; nSo<pCa->nsolids; nSo++ ) {
      pEa = &pCa->solids[nSo];
      
      rm = pEa->rotMat;

      rotatedPos = pEa->trasl;
      
      for(nPg=0; nPg<nWalls; nPg++) {
        pPg = &walls[nPg];

        if( pPg->whichGe != pCa->whichClus )
          continue;
//        if( pPg->whichCrystal != nSo )
        if( pPg->whichCrystal != pEa->numPhys )
          continue;
        if( !pPg->pDetL )
          continue;
        if( pCa->pAssV ) {
          transf = G4Transform3D( *(pEa->pTransf) );
          pCa->pAssV->AddPlacedVolume( pPg->pDetL, transf );
        }
        printf( "   Wall %4d      %9.2f %9.2f %9.2f %9.2f %9.2f %9.2f\n", 
          pPg->whichWall, pEa->ps/deg, pEa->th/deg, pEa->ph/deg, rotatedPos.x()/cm, rotatedPos.y()/cm, rotatedPos.z()/cm );      
      }
    }
#else
    rm.set(0, 0, 0);
    G4double ps = 0.;
    G4double th = 0.;
    G4double ph = 0.;
    rotatedPos = G4ThreeVector();

    // walls
    for( nPg=0; nPg<nWalls; nPg++ ) {
      pPg = &walls[nPg];

      if( pPg->whichGe != pCa->whichClus )
        continue;
      if( !pPg->pDetL )
        continue;

      if( pCa->pAssV ) {
        transf = G4Transform3D( rm, rotatedPos );
        pCa->pAssV->AddPlacedVolume( pPg->pDetL, transf );
        pCa->nwalls++;
      }  

      printf( "  Wall  %4d      %9.2f %9.2f %9.2f %9.2f %9.2f %9.2f\n", pPg->whichWall,
       ps/deg, th/deg, ph/deg, rotatedPos.x()/cm, rotatedPos.y()/cm, rotatedPos.z()/cm );
    }
#endif    
  }
}

void HR_Array::PlaceTheClusters()
{
  G4int    nGe, nCl, nEa, nCa, nPg, nSol, nPt, indexP;
  G4int    ii, jj;
  
  CclusterAngles* pCa = NULL;
  CpolyhPoints*   pPg = NULL;
  CeulerAngles*   pEa = NULL;
  CeulerAngles*   pEc = NULL;

  G4RotationMatrix rm;
  G4RotationMatrix rm1;
  G4RotationMatrix radd;
  G4RotationMatrix rmP;    // PRISMA rotation
  G4ThreeVector rotatedPos;
  G4ThreeVector rotatedPos1;
  G4ThreeVector rotatedPos2;
  G4Transform3D transf;

  G4int  iClTot = 0;
  G4int  iClMin = -1;
  G4int  iClMax = -1;  
  
  nDets  = 0;
  nWlTot = 0;
  nClus  = 0;

  G4cout << G4endl << "Placing clusters ... " << G4endl;
  
  G4RunManager* runManager = G4RunManager::GetRunManager();
  DetectorConstruction* theDetector  = (DetectorConstruction*) runManager->GetUserDetectorConstruction();
  
  G4int  *iCl = new G4int[nClAng];
  memset(iCl, 0, nClAng*sizeof(G4int));

  arrayRmin =  1.e10;
  arrayRmax = -1.e10;
  
  crystType.clear();
  crystType.resize(nEuler);
  
  planarLUT.clear();
  planarLUT.resize(nEuler);
  
  rmP.set(0, 0, 0 );
  if(thetaPrisma != 0.) 
    rmP.rotateX( thetaPrisma );

  // For the cryostats
  G4Polycone* Cryostat = new G4Polycone("Cryostat", 0., 360.*deg, 7, cryostatZplanes, cryostatRinner, cryostatRouter);
  G4LogicalVolume* logicCryostat = new G4LogicalVolume(Cryostat, matCryo, "Cryostat_log", 0, 0, 0 );
  
  for(nEa = 0; nEa < nEuler; nEa++) {
    pEc = &euler[nEa];
    nCl = pEc->whichGe;
    if(nCl < 0) continue;
    
    nGe = pEc->numPhys;
    
    for(nCa = 0; nCa < nClAng; nCa++) {
      pCa = &clust[nCa];
      if(pCa->whichClus != nCl) continue;
      if(!pCa->pAssV) continue;
      
      rm = pEc->rotMat;

      rotatedPos = pEc->trasl + posShift;
      
      if( (thetaShift*phiShift!=0.) || (thetaShift+phiShift!=0.) ) {
        radd.set(0, 0, 0 );
        radd.rotateY( thetaShift );
        radd.rotateZ( phiShift );
        rotatedPos = radd( rotatedPos );
        rm = radd * rm;
      }
      
      if(thetaPrisma != 0.) {
        rotatedPos = rmP( rotatedPos );
	rm = rmP * rm;
      }

      indexP = 1000 * nGe + maxSolids * nGe;
      
      transf = G4Transform3D( rm, rotatedPos );
      pCa->pAssV->MakeImprint(theDetector->HallLog(), transf, indexP-1);

      // Place a Cryostat                                                   //LR
      if(cryostatStatus){
	cryostatPos = cryostatPos0;
	cryostatPos.rotateZ(pEc->ps);
        cryostatPos.rotateY(pEc->th);
	cryostatPos.rotateZ(pEc->ph);
	cryostatRot = G4RotationMatrix::IDENTITY;
	cryostatRot.rotateY( cryostatPos.getTheta() );
	cryostatRot.rotateZ( cryostatPos.getPhi() );

	new G4PVPlacement(G4Transform3D(cryostatRot,cryostatPos), logicCryostat, "Cryostat", theDetector->HallLog(), false, 0 );
      }

      // Since the solids are defined centered in the origin, need to recalculate
      // the size of the equivalent shell with the crystals placed
      // For this we can neglect the additional rotation radd!
      for( nSol=0; nSol<pCa->nsolids; nSol++ ) {
        pEa = &pCa->solids[nSol];
        
        rm1 = pEa->rotMat;
        
        rotatedPos1 = pEa->trasl;
        
        for( nPg=0; nPg<nPgons; nPg++ ) {
          pPg= & pgons[nPg];
          if( pPg->whichGe != pEa->whichGe )
            continue;
            
          for( nPt=0; nPt<pPg->npoints; nPt++ ) {
            rotatedPos2 = G4ThreeVector( pPg->vertex[nPt] );  
            rotatedPos2 = rm( rm1( rotatedPos2 ) + rotatedPos1 ) + rotatedPos;
            arrayRmin = min(arrayRmin, rotatedPos2.mag());
            arrayRmax = max(arrayRmax, rotatedPos2.mag());
          }
          // should consider also the centres of the faces!!!
          rotatedPos2 = G4ThreeVector( pPg->centerFace1 );
          rotatedPos2 = rm( rm1( rotatedPos2 ) + rotatedPos1 ) + rotatedPos;
          arrayRmin = min(arrayRmin, rotatedPos2.mag());
          arrayRmax = max(arrayRmax, rotatedPos2.mag());
          rotatedPos2 = G4ThreeVector( pPg->centerFace2 );
          rotatedPos2 = rm( rm1( rotatedPos2 ) + rotatedPos1 ) + rotatedPos;
          arrayRmin = min(arrayRmin, rotatedPos2.mag());
          arrayRmax = max(arrayRmax, rotatedPos2.mag());
        }
      }
      

      nDets  += pCa->nsolids;
      nWlTot += pCa->nwalls;
      nClus++;
      
      if(iClMin < 0 || nGe < iClMin) iClMin = nGe;
      if(iClMax < 0 || nGe > iClMax) iClMax = nGe;
      
      printf("%4d %4d %4d %8d %9.2f %9.2f %9.2f %9.2f %9.2f %9.2f\n",
             iClTot, nGe, nCl, indexP, pEc->ps/deg, pEc->th/deg, pEc->ph/deg,
                pEc->trasl.x()/cm, pEc->trasl.y()/cm, pEc->trasl.z()/cm );
      iCl[nCl]++;
      iClTot++;
    }
  }
  nClus = iClMax - iClMin + 1;
  
  // store the crystal type for pulse shape calculations
  crystType.resize((1+iClMax)*maxSolids);
  planarLUT.resize((1+iClMax)*maxSolids);
  for( ii=0; ii<((G4int)crystType.size()); ii++ )
    crystType[ii] = -1;
  for( ii=0; ii<((G4int)planarLUT.size()); ii++ )
    planarLUT[ii] = 0;  // by default, coaxial  
  
  for(nEa = 0; nEa < nEuler; nEa++) {
    pEc = &euler[nEa];
    nCl = pEc->whichGe;
    if(nCl < 0) continue;
    
    nGe = pEc->numPhys;
    
    for(nCa = 0; nCa < nClAng; nCa++) {
      pCa = &clust[nCa];
      if(pCa->whichClus != nCl) continue;
      if(!pCa->pAssV) continue;
      for( ii=0; ii<((G4int)pCa->solids.size()); ii++ ) {
        pEa= &pCa->solids[ii];
        crystType[nGe*maxSolids+ii] = pEa->whichGe;
	
	for( jj=0; jj<nPgons; jj++ ) {
	  pPg = &pgons[jj];
	  if( pPg->whichGe != pEa->whichGe ) continue;
	  if( !pPg->isPlanar ) continue;
	  planarLUT[nGe*maxSolids+ii] = 1;
	}
        //G4cout << nGe*maxSolids+ii << " " << pEa->whichGe << G4endl;
      }
    }
  }
  
  iCMin = iClMin;
  iCMax = iClMax;
  iGMin = maxSolids*iClMin;
  iGMax = maxSolids*(iClMax+1)-1;
  
  //G4cout << " MaxSolids is " << maxSolids << G4endl;
  
  G4cout << "Number of placed clusters is " << iClTot << "  [ ";
  for(nPg = 0; nPg < nClAng; nPg++) G4cout << iCl[nPg] << " ";
  G4cout << "]" << G4endl;
  G4cout << "Cluster  Index ranging from " << iClMin << " to " << iClMax << G4endl;
  G4cout << "Detector Index ranging from " << maxSolids*iClMin << " to " << maxSolids*(iClMax+1)-1 << G4endl << G4endl;
  G4cout << "Number of placed walls is " << nWlTot << G4endl << G4endl;
  delete [] iCl;
  
  G4cout << "The equivalent shell extends from " << arrayRmin/cm << " cm to " 
         << arrayRmax/cm << " cm" << G4endl << G4endl;

}

void HR_Array::ShowStatus()
{
  G4cout << G4endl;
  G4int prec = G4cout.precision(3);
  G4cout.setf(ios::fixed);
  G4cout << " Array composed of " << std::setw(3) << nDets  << " detectors" << G4endl;
  G4cout << "       arranged in " << std::setw(3) << nClus  << " clusters"  << G4endl;
  G4cout << " Array composed of " << std::setw(3) << nWlTot << " walls"  << G4endl;
  G4cout << " Description of detectors                read from " << solidFile << G4endl;
  G4cout << " Description of dead materials (walls)   read from " << wallsFile << G4endl;
  G4cout << " Description of clusters                 read from " << clustFile << G4endl;
  G4cout << " Euler angles for clusters               read from " << eulerFile << G4endl;
  
  if( readOut ) {
    G4cout << " Slicing planes for read out of segments read from " << sliceFile << G4endl;
    G4cout << " Generated " << nSeg << " sub-segments " << G4endl;
  }
  if( makeCapsule )
    G4cout << " The capsules have been generated with the proper thickness and spacing." << G4endl;
  else
    G4cout << " The capsules have not been generated." << G4endl;
  if( useCylinder )
    G4cout << " The intersection with a cylinder has been considered in generating the crystals." << G4endl;
  else
    G4cout << " The intersection with a cylinder has not been considered in generating the crystals." << G4endl;  
  if( usePassive )
    G4cout << " The passivated zones have been considered." << G4endl;
  else
    G4cout << " The passivated zones have not been considered." << G4endl;         
       
  G4cout << " The detectors   material is " << matCrystName << G4endl;
  G4cout << " The walls       material is " << matWallsName << G4endl;
  if( thetaShift || phiShift )
    G4cout << " The array is rotated by theta, phi = " << thetaShift/deg << ", " 
                                                       << phiShift/deg   << " degrees" << G4endl;
  if( thetaPrisma != 0. )
    G4cout << " PRISMA rotation is theta = " << thetaPrisma/deg << " degrees" << G4endl;
  if( posShift.mag2() )
    G4cout << " The array is shifted by " << posShift/mm << " mm" << G4endl;
  
  G4cout.unsetf(ios::fixed);
  G4cout.precision(prec);
}

void HR_Array::WriteHeader(std::ofstream &outFileLMD, G4double unit)
{
  char line[128];
  FILE *fp1, *fp2, *fp3, *fp4, *fp5; 

  outFileLMD << "GRETINA" << G4endl;
  outFileLMD << "SUMMARY "    << arrayRmin/unit << "  "  << arrayRmax/unit  << "  "
                              << nDets        << "  "  << nPgons        << " ";
                             
  if( readOut ) {
    for( G4int ii=0; ii<nPgons; ii++ )
      outFileLMD << pgons[ii].nslice << " " << pgons[ii].npoints/2 << " ";
  }
  else {
    for( G4int i=0; i<nPgons; i++ )
      outFileLMD << "1 1 ";
  }
  outFileLMD << G4endl; 
  
//  sprintf(line, "TRANSFORMATION %7.3lf %7.3lf %7.3lf %7.3lf %7.3lf\n",
  sprintf(line, "TRANSFORMATION %7.3f %7.3f %7.3f %7.3f %7.3f %7.3f\n",
       posShift.x()/unit, posShift.y()/unit, posShift.z()/unit, thetaShift/deg, phiShift/deg, thetaPrisma/deg);
  outFileLMD << G4String(line);	                            

  if( usePassive )
    outFileLMD << "PASSIVE 1" << G4endl;
  else
    outFileLMD << "PASSIVE 0" << G4endl;

  if( makeCapsule )
    outFileLMD << "CAPSULES 1" << G4endl;
  else
    outFileLMD << "CAPSULES 0" << G4endl;
    
  //  G4RunManager * runManager = G4RunManager::GetRunManager();          // LR
  //  RunAction* theRun    = (RunAction*) runManager->GetUserRunAction(); // LR
  
  //  G4int verboseLevel = theRun->GetVerbose();                          // LR
  //       Maybe fix the messenger someday.                               // LR
  G4int verboseLevel = 1;                                                 // LR

  if( verboseLevel > 0 ) {
        
    outFileLMD << "SOLID " << solidFile << G4endl;
    if( (fp1 = fopen(solidFile, "r")) == NULL) {
      outFileLMD << "ENDSOLID" << G4endl;
    }
    else {
      while( fgets(line,128,fp1) )
        outFileLMD << line;
      outFileLMD << "ENDSOLID" << G4endl;
      fclose(fp1);
    }  

    outFileLMD << "CLUSTER " << clustFile << G4endl;
    if( (fp3 = fopen(clustFile, "r")) == NULL) {
      outFileLMD << "ENDCLUSTER" << G4endl;
    }
    else {
      while( fgets(line,128,fp3) )
        outFileLMD << line;
      outFileLMD << "ENDCLUSTER" << G4endl;
      fclose(fp3);
    }    

    outFileLMD << "WALLS " << wallsFile << G4endl;
    if( (fp2 = fopen(wallsFile, "r")) == NULL) {
      outFileLMD << "ENDWALLS" << G4endl;
    }
    else {
      while( fgets(line,128,fp2) )
        outFileLMD << line;
      outFileLMD << "ENDWALLS" << G4endl;
      fclose(fp2);
    }    

    outFileLMD << "EULER " << eulerFile << G4endl;
    if( (fp4 = fopen(eulerFile, "r")) == NULL) {
      outFileLMD << "ENDEULER" << G4endl;
    }
    else {
      while( fgets(line,128,fp4) )
        outFileLMD << line;
      outFileLMD << "ENDEULER" << G4endl;
      fclose(fp4);
    }

    if( readOut ) {
      outFileLMD << "SLICES " << sliceFile << G4endl;
      if( (fp5 = fopen(sliceFile, "r")) == NULL) {
        outFileLMD << "ENDSLICES" << G4endl;
      }
      else {
        while( fgets(line,128,fp5) )
          outFileLMD << line;
        outFileLMD << "ENDSLICES" << G4endl;
        fclose(fp5);
      }  
    }
    // writes out LUT for crystal types
    G4int prec = outFileLMD.precision(3);
    outFileLMD.setf(ios::fixed);
    outFileLMD << "CRYSTAL_LUT" << G4endl;
    for( G4int ii=0; ii<((G4int)crystType.size()); ii++ )
      outFileLMD << std::setw(4) << ii << std::setw(3) << crystType[ii] << G4endl;
    outFileLMD << "ENDCRYSTAL_LUT" << G4endl;
    outFileLMD << "PLANAR_LUT" << G4endl;
    for( G4int ii=0; ii<((G4int)planarLUT.size()); ii++ )
      outFileLMD << std::setw(4) << ii << std::setw(3) << planarLUT[ii] << G4endl;
    outFileLMD << "ENDPLANAR_LUT" << G4endl;
    outFileLMD.unsetf(ios::fixed);
    outFileLMD.precision(prec);
  }   

  if( verboseLevel > 1 )
    WritePositions( outFileLMD, unit );
    
  if( verboseLevel > 2 )
    WriteCrystalTransformations( outFileLMD, unit );  
}


void HR_Array::WritePositions(std::ofstream &outFileLMD, G4double unit)
{ 
  char  line[128];
  G4int nPh;
  
  CeulerAngles* pEa = NULL;

  outFileLMD << "POSITION_CLUSTERS" << G4endl;
  
  for( G4int ne=0; ne<nEuler; ne++ ) {
    pEa = &euler[ne];
    
    nPh = pEa->numPhys;
    // Rotation matrix and
    // positions of the clusters (recalculated here)
    G4RotationMatrix frameRot, radd, rmP;
    G4ThreeVector rotatedPos = pEa->trasl + posShift;
    
    frameRot = pEa->rotMat;
    if( (thetaShift*phiShift!=0.) || (thetaShift+phiShift!=0.) ) {
      radd.set(0, 0, 0 );
      radd.rotateY( thetaShift );
      radd.rotateZ( phiShift );
      frameRot = radd * frameRot;
      rotatedPos = radd(rotatedPos);
    }
    
    if(thetaPrisma!=0.) {
      rmP.set(0, 0, 0);
      rmP.rotateX( thetaPrisma );
      frameRot   = rmP * frameRot;
      rotatedPos = rmP(rotatedPos);
    }
      
    //G4ThreeVector rotatedPos = euler[ne].trasl;

    sprintf( line, " %3d  0    %10.5f %10.5f %10.5f\n", nPh, 
                          rotatedPos.x()/unit, rotatedPos.y()/unit, rotatedPos.z()/unit );
    outFileLMD << line;
      
    sprintf( line, "      1    %10.5f %10.5f %10.5f\n",  
                           frameRot.xx(), frameRot.xy(), frameRot.xz() );
    outFileLMD << line;
    sprintf( line, "      2    %10.5f %10.5f %10.5f\n", 
                          frameRot.yx(), frameRot.yy(), frameRot.yz() );
    outFileLMD << line;
    sprintf( line, "      3    %10.5f %10.5f %10.5f\n", 
                          frameRot.zx(), frameRot.zy(), frameRot.zz() );
    outFileLMD << line;

  }
  outFileLMD << "ENDPOSITION_CLUSTERS" << G4endl;
  
  outFileLMD << "POSITION_CRYSTALS" << G4endl;
  WriteCrystalPositions( outFileLMD );
  outFileLMD << "ENDPOSITION_CRYSTALS" << G4endl;
  
  

  if( readOut ) {
    outFileLMD << "POSITION_SEGMENTS" << G4endl;
    WriteSegmentPositions( outFileLMD );
    outFileLMD << "ENDPOSITION_SEGMENTS" << G4endl;
  }
}

void HR_Array::WriteCrystalPositions(std::ofstream &outFileLMD, G4double unit)
{
  G4int nGe, nPh, nCl;
  G4int iPg, iCa, ne, nSol;
  
  CeulerAngles   *peA;
  CeulerAngles   *peA1;
  CclusterAngles *pcA;
  CpolyhPoints   *ppG;  
  char line[128]; 
  
  G4RotationMatrix rm, radd, rm1, rmP, frameRot;
  G4ThreeVector    trasl, trasl1;
  G4ThreeVector    rotatedPos;
  
  for(ne = 0; ne < nEuler; ne++) {

    peA = &euler[ne];
    nCl = peA->whichGe;
    if(nCl < 0) continue;
    nPh = peA->numPhys * maxSolids;

    rm = peA->rotMat;
    trasl = peA->trasl + posShift;

    if( (thetaShift*phiShift!=0.) || (thetaShift+phiShift!=0.) ) {
      radd.set(0, 0, 0 );
      radd.rotateY( thetaShift );
      radd.rotateZ( phiShift );
      trasl = radd( trasl );
      rm = radd * rm;
    }
    
    if(thetaPrisma!=0.) {
      rmP.set(0, 0, 0);
      rmP.rotateX( thetaPrisma );
      rm    = rmP * rm;
      trasl = rmP(trasl);
    }
    
    for( iCa = 0; iCa<nClAng; iCa++ ) {
      pcA = &clust[iCa];
      if( pcA->whichClus != nCl ) continue;
      
      for( nSol = 0; nSol<pcA->nsolids; nSol++ ) {
        peA1 = &pcA->solids[nSol];
        nGe  = peA1->whichGe;
        if( nGe < 0 ) continue;

        rm1 = peA1->rotMat;
        
        frameRot = rm * rm1;
        
        trasl1 = peA1->trasl;
        
        for(iPg = 0; iPg < nPgons; iPg++) {
          ppG = &pgons[iPg];
          if(ppG->whichGe != nGe) continue; // looks for the right solid
            rotatedPos  = rm( trasl1 ) + trasl;
            

            sprintf( line, " %3d  0    %10.5f %10.5f %10.5f\n", 
                     nPh, rotatedPos.x()/unit, rotatedPos.y()/unit, rotatedPos.z()/unit );
            outFileLMD << line;
            sprintf( line, "      1    %10.5f %10.5f %10.5f\n", 
                     frameRot.xx(), frameRot.xy(), frameRot.xz() );
            outFileLMD << line;
            sprintf( line, "      2    %10.5f %10.5f %10.5f\n", 
                     frameRot.yx(), frameRot.yy(), frameRot.yz() );
            outFileLMD << line;
            sprintf( line, "      3    %10.5f %10.5f %10.5f\n", 
                     frameRot.zx(), frameRot.zy(), frameRot.zz() );
            outFileLMD << line;
            nPh++;
        }  
      }
    } 
  }
}

void HR_Array::WriteCrystalAngles( G4String file )
{
  G4int nGe, nPh, nCl;
  G4int iPg, iCa, ne, nSol;
  
  G4double theta, phi;
  
  CeulerAngles   *peA;
  CeulerAngles   *peA1;
  CclusterAngles *pcA;
  CpolyhPoints   *ppG;  
  char line[128]; 
  
  G4RotationMatrix rm, radd, rm1, rmP, frameRot;
  G4ThreeVector    trasl, trasl1;
  G4ThreeVector    rotatedPos;
  
  std::ofstream outFileLMD;
  
  outFileLMD.open(file);
  if( !outFileLMD.is_open() ) {
    G4cout << " --> Could not open " << file << " output file, aborting ..." << G4endl;
    return;
  }  
  
  G4cout << " --> Writing out crystal angles to " << file << " file..." << G4endl;
  for(ne = 0; ne < nEuler; ne++) {

    peA = &euler[ne];
    nCl = peA->whichGe;
    if(nCl < 0) continue;
    nPh = peA->numPhys * maxSolids;

    rm = peA->rotMat;
    trasl = peA->trasl + posShift;

    if( (thetaShift*phiShift!=0.) || (thetaShift+phiShift!=0.) ) {
      radd.set(0, 0, 0 );
      radd.rotateY( thetaShift );
      radd.rotateZ( phiShift );
      trasl = radd( trasl );
      rm = radd * rm;
    }

    if(thetaPrisma!=0.) {
      rmP.set(0, 0, 0);
      rmP.rotateX( thetaPrisma );
      rm    = rmP * rm;
      trasl = rmP(trasl);
    }
    
    for( iCa = 0; iCa<nClAng; iCa++ ) {
      pcA = &clust[iCa];
      if( pcA->whichClus != nCl ) continue;
      
      for( nSol = 0; nSol<pcA->nsolids; nSol++ ) {
        peA1 = &pcA->solids[nSol];
        nGe  = peA1->whichGe;
        if( nGe < 0 ) continue;

        rm1 = peA1->rotMat;
        
        frameRot = rm * rm1;
        
        trasl1 = peA1->trasl;
        
        for(iPg = 0; iPg < nPgons; iPg++) {
          ppG = &pgons[iPg];
          if(ppG->whichGe != nGe) continue; // looks for the right solid
            rotatedPos  = rm( trasl1 ) + trasl;
	    
	    theta = rotatedPos.theta();
	    phi   = rotatedPos.phi();
	    if( phi < 0. ) phi += 360.*deg;
	    
	    sprintf( line, "  Riv#%4.1d    Theta= %9.4f        Phi= %9.4f\n", 
	                                                 nPh, theta/deg, phi/deg );
	    //sprintf( line, " %3d %9d %9.4f %9.4f\n", 0, nPh, theta/deg, phi/deg );
            outFileLMD << line;
            G4cout << line;
            nPh++;
        }  
      }
    } 
  }
  outFileLMD.close();
  G4cout << " --> Crystal angles successfully written out to " << file << " file." << G4endl;
}


void HR_Array::WriteSegmentPositions(std::ofstream &outFileLMD, G4double unit)
{
  G4int nGe, nPh, nCl;
  G4int iPg, iCa, ne, nSol, sector, slice;
  G4double  step = 1.*mm / ((G4double)stepFactor);   // integration step for CalculateVolumeAndCenter()

  if( (segVolume.size() == 0) || stepHasChanged ) {
    G4cout << "Calculating volume and center of segments (with step = " << step/mm << "mm ) ..." << G4endl;
    segVolume.resize(totSegments);
    segCenter.resize(totSegments);
    for(iPg = 0; iPg < nPgons; iPg++) {
      CalculateVolumeAndCenter(iPg, tSegments[iPg], nSegments[iPg], step);
    }
    stepHasChanged = false;
  }
  
  CeulerAngles   *peA;
  CeulerAngles   *peA1;
  CclusterAngles *pcA;
  CpolyhPoints   *ppG;  
  G4int indexS;         // index of segment in segCenter
  char line[128]; 
  
  G4RotationMatrix rm, rm1, radd, rmP;
  G4ThreeVector    trasl, trasl1;
  G4Point3D        centreSeg, rotatedPos;
  G4Transform3D    clusterToWorld, crystalToCluster, crystalToWorld;
  
  for(ne = 0; ne < nEuler; ne++) {

    peA = &euler[ne];
    nCl = peA->whichGe;
    if(nCl < 0) continue;
    nPh = peA->numPhys * maxSolids;

    trasl = peA->trasl + posShift;
    rm = peA->rotMat;

    if( (thetaShift*phiShift!=0.) || (thetaShift+phiShift!=0.) ) {
      radd.set(0, 0, 0 );
      radd.rotateY( thetaShift );
      radd.rotateZ( phiShift );
      trasl = radd( trasl );
      rm = radd * rm;
    }

    if(thetaPrisma!=0.) {
      rmP.set(0, 0, 0);
      rmP.rotateX( thetaPrisma );
      rm    = rmP * rm;
      trasl = rmP(trasl);
    }

    clusterToWorld = G4Transform3D( rm, trasl );
    
    for( iCa = 0; iCa<nClAng; iCa++ ) {
      pcA = &clust[iCa];
      if( pcA->whichClus != nCl ) continue;
      
      for( nSol = 0; nSol<pcA->nsolids; nSol++ ) {
        peA1 = &pcA->solids[nSol];
        nGe  = peA1->whichGe;
        if( nGe < 0 ) continue;
        
        rm1 = peA1->rotMat;

        trasl1 = peA1->trasl;

        crystalToCluster = G4Transform3D( rm1, trasl1 );
        
        crystalToWorld = clusterToWorld * crystalToCluster;
        
        for(iPg = 0; iPg < nPgons; iPg++) {
          ppG = &pgons[iPg];
          if(ppG->whichGe != nGe) continue; // looks for the right solid
          indexS = tSegments[iPg];
          
          for( slice=0; slice<ppG->nslice; slice++ ) {
            for( sector=0; sector<ppG->npoints/2; sector++, indexS++ ) {
            
              centreSeg  = G4Point3D(segCenter[indexS]);
              
              //rotatedPos  = rm( rm1( centreSeg ) + trasl1 ) + trasl; // old style!
              // could be: rotatedPos = clusterToWorld * (crystalToCluster*centreSeg)
              rotatedPos = crystalToWorld * centreSeg;

              sprintf( line, " %3d %2d %2d %10.5f %10.5f %10.5f  %10.5f\n", 
                       nPh, slice, sector, rotatedPos.x()/unit, rotatedPos.y()/unit, rotatedPos.z()/unit, segVolume[indexS]/unit*unit*unit );
              outFileLMD << line;
	    }
          }
          
          nPh++;
        }  
      }
    } 
  }
}

void HR_Array::WriteCrystalTransformations(std::ofstream &outFileLMD, G4double unit)
{
  G4int nGe, nPh, nCl;
  G4int iPg, iCa, ne, nSol;
  G4int ii=0;
  
  CeulerAngles   *peA;
  CeulerAngles   *peA1;
  CclusterAngles *pcA;
  CpolyhPoints   *ppG;  
  char line[128]; 
  
  G4RotationMatrix rm, rm1, radd, rmP;
  G4ThreeVector    trasl, trasl1;
  G4Point3D        centreSeg, rotatedPos;
  G4Transform3D    clusterToWorld, crystalToCluster, crystalToWorld;
  
  outFileLMD << "TRANSFORMATION_CRYSTALS" << G4endl;
  
  for(ne = 0; ne < nEuler; ne++) {

    peA = &euler[ne];
    nCl = peA->whichGe;
    if(nCl < 0) continue;
    nPh = peA->numPhys * maxSolids;

    rm = peA->rotMat;
    trasl = peA->trasl + posShift;

    if( (thetaShift*phiShift!=0.) || (thetaShift+phiShift!=0.) ) {
      radd.set(0, 0, 0 );
      radd.rotateY( thetaShift );
      radd.rotateZ( phiShift );
      trasl = radd( trasl );
      rm = radd * rm;
    }

    if(thetaPrisma!=0.) {
      rmP.set(0, 0, 0);
      rmP.rotateX( thetaPrisma );
      rm    = rmP * rm;
      trasl = rmP(trasl);
    }
      

    clusterToWorld = G4Transform3D( rm, trasl );
    
    for( iCa = 0; iCa<nClAng; iCa++ ) {
      pcA = &clust[iCa];
      if( pcA->whichClus != nCl ) continue;
      
      for( nSol = 0; nSol<pcA->nsolids; nSol++ ) {
        peA1 = &pcA->solids[nSol];
        nGe  = peA1->whichGe;
        if( nGe < 0 ) continue;
        
        rm1 = peA1->rotMat;

        trasl1 = peA1->trasl;

        crystalToCluster = G4Transform3D( rm1, trasl1 );
        
        crystalToWorld = clusterToWorld * crystalToCluster;
        
        for(iPg = 0; iPg < nPgons; iPg++) {
          ppG = &pgons[iPg];
          if(ppG->whichGe != nGe) continue; // looks for the right solid
          
          ii = 0;
	  sprintf( line, " %3d %2d %2d %10.5f %10.5f %10.5f %10.5f\n", 
            	   nPh, crystType[nPh], ii, crystalToWorld[ii][0], crystalToWorld[ii][1], crystalToWorld[ii][2], crystalToWorld[ii][3]/unit );
          outFileLMD << line;
	  for( ii=1; ii<3; ii++ ) {
            sprintf( line, "        %2d %10.5f %10.5f %10.5f %10.5f\n", 
            	     ii, crystalToWorld[ii][0], crystalToWorld[ii][1], crystalToWorld[ii][2], crystalToWorld[ii][3]/unit );
            outFileLMD << line;
	  
	  }
          nPh++;
        }  
      }
    } 
  }
  outFileLMD << "ENDTRANSFORMATION_CRYSTALS" << G4endl;
}

void HR_Array::WriteSegmentAngles( G4String name, G4int format )
{
  if( format < 0  ){
    G4cout << " Illegal format value!!!" << G4endl;
    return;
  }
  if( format > 2  ){
    G4cout << " Illegal format value!!!" << G4endl;
    return;
  }
  if( !readOut ) {
    G4cout << " Segments have not been defined, aborting ..." << G4endl;
    return;
  }
  
  G4int nGe, nPh, nCl;
  G4int iPg, iCa, ne, nSol, sector, slice;
  G4double  step = 1.*mm / ((G4double)stepFactor);   // integration step for CalculateVolumeAndCenter()

  if( (segVolume.size() == 0) || stepHasChanged ) {
    G4cout << "Calculating volume and center of segments (with step = " << step/mm << "mm ) ..." << G4endl;
    segVolume.resize(totSegments);
    segCenter.resize(totSegments);
    for(iPg = 0; iPg < nPgons; iPg++) {
      CalculateVolumeAndCenter(iPg, tSegments[iPg], nSegments[iPg], step);
    }
    stepHasChanged = false;
  }
  
  CeulerAngles   *peA;
  CeulerAngles   *peA1;
  CclusterAngles *pcA;
  CpolyhPoints   *ppG;  
  G4int indexS;         // index of segment in segCenter
  
  G4RotationMatrix rm, rm1, radd, rmP;
  G4ThreeVector    trasl, trasl1;
  G4Point3D        centreSeg, rotatedPos;
  G4Transform3D    clusterToWorld, crystalToCluster, crystalToWorld;
  
  FILE *agatanumber;
  if( (agatanumber = fopen(name, "w")) == NULL ) {
    G4cout << "\nCould not open " << name
           << ", will not write angles for the segments!" << G4endl;
    return;
  }
  
  switch( format ) {
    case 0:
      G4cout << " Writing out segment angles in GSORT format to " << name << " ..." << G4endl;
      break;
    case 1:
      G4cout << " Writing out segment positions in POLAR COORDINATES format to " << name << " ..." << G4endl;
      break;
    case 2:
      G4cout << " Writing out segment positions in CARTESIAN COORDINATES format to " << name << " ..." << G4endl;
      break;
  }
  
  for(ne = 0; ne < nEuler; ne++) {

    peA = &euler[ne];
    nCl = peA->whichGe;
    if(nCl < 0) continue;
    nPh = peA->numPhys * maxSolids;

    rm = peA->rotMat;
    trasl = peA->trasl + posShift;

    if( (thetaShift*phiShift!=0.) || (thetaShift+phiShift!=0.) ) {
      radd.set(0, 0, 0 );
      radd.rotateY( thetaShift );
      radd.rotateZ( phiShift );
      trasl = radd( trasl );
      rm = radd * rm;
    }

    if(thetaPrisma!=0.) {
      rmP.set(0, 0, 0);
      rmP.rotateX( thetaPrisma );
      rm    = rmP * rm;
      trasl = rmP(trasl);
    }
      

    clusterToWorld = G4Transform3D( rm, trasl );
    
    for( iCa = 0; iCa<nClAng; iCa++ ) {
      pcA = &clust[iCa];
      if( pcA->whichClus != nCl ) continue;
      
      for( nSol = 0; nSol<pcA->nsolids; nSol++ ) {
        peA1 = &pcA->solids[nSol];
        nGe  = peA1->whichGe;
        if( nGe < 0 ) continue;
        
        rm1 = peA1->rotMat;

        trasl1 = peA1->trasl;

        crystalToCluster = G4Transform3D( rm1, trasl1 );
        
        crystalToWorld = clusterToWorld * crystalToCluster;
        
        for(iPg = 0; iPg < nPgons; iPg++) {
          ppG = &pgons[iPg];
          if(ppG->whichGe != nGe) continue; // looks for the right solid
          indexS = tSegments[iPg];
          
          for( slice=0; slice<ppG->nslice; slice++ ) {
            for( sector=0; sector<ppG->npoints/2; sector++, indexS++ ) {
            
              centreSeg  = G4Point3D(segCenter[indexS]);
              
              //rotatedPos  = rm( rm1( centreSeg ) + trasl1 ) + trasl; // old style!
              // could be: rotatedPos = clusterToWorld * (crystalToCluster*centreSeg)
              rotatedPos = crystalToWorld * centreSeg;

	      switch( format ) {
	        case 0:
//		  fprintf(agatanumber," %3d %9d %9.4lf %9.4lf\n",
		  fprintf(agatanumber,"  Riv#%4.1d %9.1d    Theta= %9.4f        Phi= %9.4f\n",
		      nPh, slice*10+sector, rotatedPos.theta()/deg,
		      ((rotatedPos.phi()>0.) ? (rotatedPos.phi()/deg) : (rotatedPos.phi()/deg + 360.)));
	          break;
	        case 1:
//		  fprintf(agatanumber," %3d %9d %9.4lf %9.4lf %9.4lf\n",
		  fprintf(agatanumber," %3d %9d %9.4f %9.4f %9.4f\n",
		      nPh, slice*10+sector, rotatedPos.mag()/mm, rotatedPos.theta()/deg,
		      ((rotatedPos.phi()>0.) ? (rotatedPos.phi()/deg) : (rotatedPos.phi()/deg + 360.)));
	          break;
	        case 2:
//		  fprintf(agatanumber," %3d %9d %9.4lf %9.4lf %9.4lf\n",
		  fprintf(agatanumber," %3d %9d %9.4f %9.4f %9.4f\n",
		      nPh, slice*10+sector, rotatedPos.x()/mm, rotatedPos.y()/mm, rotatedPos.z()/mm);
	          break;
	      }
	    }
          }
          
          nPh++;
        }  
      }
    } 
  }
  fclose(agatanumber);
  switch( format ) {
    case 0:
      G4cout << " Segment angles successfully written out to " << name << G4endl;
      break;
    case 1:
      G4cout << " Segment positions successfully written out to " << name << G4endl;
      break;
    case 2:
      G4cout << " Segment positions successfully written out to " << name << G4endl;
      break;
  }
}


// plane vv = (a,b,c,d)  pV=(a,b,c)
// line A-->B = AB
// intercept pX = (pV cross (pA cross pB) - d * AB ) / pV dot AB
G4Point3D  HR_Array::XPlaneLine(const G4Plane3D &vv, const G4Point3D &pA, const G4Point3D &pB)
{
  G4Point3D  AB;
  G4Normal3D pV;
  G4Point3D  AxB, VAB;
  G4double   xp;

  pV = vv.normal();

  AB = pB - pA;

  xp = pV.dot(AB);
  if(!xp) return G4Point3D();

  AxB = pA.cross(pB);
  VAB = pV.cross(AxB);

  G4Point3D pX;
  pX = (VAB - vv.d()*AB)/xp;

  return pX;
}

G4int HR_Array::CheckOverlap(G4int iPg, G4int start, G4int nsegs)
{
  CpolyhPoints ** ppsegs = new CpolyhPoints * [4*nsegs];  // collect here the pointers to all segs of this shape

  G4int nstot = 0;
  for(G4int n = 0; n < nsegs; n++) {
    ppsegs[nstot++] = &pgSegLl[start + n];
    ppsegs[nstot++] = &pgSegLu[start + n];
    ppsegs[nstot++] = &pgSegRl[start + n];
    ppsegs[nstot++] = &pgSegRu[start + n];
  }

  CpolyhPoints *ppS1;
  CpolyhPoints *ppS2;
  G4int i1, np1, i2;
  G4Point3D pt1;
  EInside inside;

  G4int nproblems = 0;
  for(i1 = 0; i1 < nstot; i1++) {
    ppS1 = ppsegs[i1];
    for(np1 = 0; np1 < ppS1->npoints; np1++) {
      pt1 = ppS1->pPoly->GetPoints(np1);
      inside =  pgons[iPg].pPoly->Inside( G4ThreeVector(pt1) );
      if(inside == kOutside) {
        printf("Warning: crystal %d : point %3d of segment %3d(%d) is outside its crystal\n", iPg, np1, i1/4, i1%4);
        nproblems++;
      }
      for(i2 = 0; i2 < nstot; i2++) {
        if(i2 == i1) continue;        // no check with itself
        ppS2 = ppsegs[i2];
        inside = ppS2->pPoly->Inside( G4ThreeVector(pt1) );
        if(inside == kInside) {
          printf("Warning: crystal %d : point %3d of segmentL %3d(%d) is inside segment %3d(%d)\n", iPg, np1, i1/4, i1%4, i2/4, i2%4);
          nproblems++;
        }
      }
    }
  }
  delete [] ppsegs;
  return nproblems;
}

G4int HR_Array::GetSegmentNumber( G4int offset, G4int nGe, G4ThreeVector position )
{
  if( !readOut )
    return 0;
  //  else if( planarLUT[nGe%1000] )
  //    return GetPlanSegmentNumber( nGe, position );
  else    
    return GetCoaxSegmentNumber( nGe, position );

  return 0;
}

G4int HR_Array::GetCoaxSegmentNumber( G4int nGe, G4ThreeVector position )
{
  EInside inside;

  CeulerAngles   *pEa = NULL;
  CclusterAngles *pCa = NULL;
  CpolyhPoints   *ppgerm = NULL;
  CpolyhPoints   *ppseg  = NULL;

  G4int detNum   = nGe%1000;
  G4int cluNum   = nGe/1000;
  G4int subIndex = detNum%maxSolids;
  
  //  G4cout << " nGe, det, clu, ind " << nGe << " " << detNum<< " " << cluNum << " " << subIndex << G4endl;
  
  G4int slice=0, sector=0, ss, nCa, nPg, indexS, whichGe;
  
  
  G4int whichClus = -100;
  
  for( nCa=0; nCa<(G4int)(euler.size()); nCa++ ) {
    pEa = &euler[nCa];
    //    G4cout << nCa << " " << pEa->numPhys << " " << pEa->whichGe << G4endl;
    if( pEa->numPhys == cluNum ) {
      whichClus = pEa->whichGe;
      break;
    }   
  }
  
  if( whichClus < 0 ) {
    G4cout << " Warning! Could not find any detector containing this point: " << position/cm
                                                                            << " cm" << G4endl;
    return -1;  
  }

  for( nCa=0; nCa<nClAng; nCa++ ) {
    pCa = &clust[nCa];
    if( pCa->whichClus != whichClus ) continue;

    whichGe = pCa->solids[subIndex].whichGe;
    //    G4cout << " whichGe " << whichGe << G4endl;

    for( nPg=0; nPg<nPgons; nPg++ ) {
      ppgerm = &pgons[nPg];
      if( ppgerm->whichGe != whichGe ) continue;
      //      G4cout << " ppgerm->whichGe " << nGe << " " << whichGe << " " << ppgerm->whichGe << " " << position/mm << G4endl;
      indexS = tSegments[nPg];
      for( slice=0; slice<ppgerm->nslice; slice++ ) {
        for( sector=0; sector<ppgerm->npoints/2; sector++, indexS++ ) { 
          // the four parts composing the segment
          for( ss = 0; ss < 4; ss++ ) {
            switch (ss) {
            case 0:
              ppseg = &pgSegLl[indexS];                   // the lower segment at the left
              break;
            case 1:
              ppseg = &pgSegLu[indexS];                   // the upper segment at the left
              break;
            case 2:
              ppseg = &pgSegRl[indexS];                   // the lower segment at the right
              break;
            case 3:
              ppseg = &pgSegRu[indexS];                   // the upper segment at the right
              break;
            }
            inside = ppseg->pPoly->Inside( position );
            if( inside != kOutside ){
	      //	      G4cout << "slice = " << slice << "   sector = " << sector << G4endl;
              return 10*slice+sector;
	    }
          }
        }
      }

      G4cout << " Warning! Could not find any segment containing this point: " << position/cm
                                                                               << " cm" << G4endl;
      return -1;  
    }
  }
  G4cout << " Warning! Could not find any detector containing this point: " << position/cm
                                                                            << " cm" << G4endl;
  return -1;  
}

void HR_Array::CalculateVolumeAndCenter(G4int iPg, G4int start, G4int nsegs, G4double step)
{
  G4int ns, ss, npts, nn;
  G4double xMin, yMin, zMin;
  G4double xMax, yMax, zMax;
  std::vector<CpolyhPoints *> ppsegs;  // collect here the pointers the four sub-segments
  ppsegs.resize(4);
  for(ns = 0; ns < nsegs; ns++) {
    // gather the composing segments
    ppsegs[0] = &pgSegLl[start + ns];
    ppsegs[1] = &pgSegLu[start + ns];
    ppsegs[2] = &pgSegRl[start + ns];
    ppsegs[3] = &pgSegRu[start + ns];

    // define a bounding box enclosing all the sub-segments
    // (an own one is more convenient here than a G4BoundingBox object)
    xMin = xMax = ppsegs[0]->pPoly->GetPoints(0).x();
    yMin = yMax = ppsegs[0]->pPoly->GetPoints(0).y();
    zMin = zMax = ppsegs[0]->pPoly->GetPoints(0).z();
    for(ss = 0; ss < 4; ss++) {
      G4Point3D pts;
      npts = ppsegs[ss]->pPoly->GetnPoints();
      for(nn = 0; nn < npts; nn++) {
        pts  = ppsegs[ss]->pPoly->GetPoints(nn);
        xMin = min(xMin, pts.x());
        xMax = max(xMax, pts.x());
        yMin = min(yMin, pts.y());
        yMax = max(yMax, pts.y());
        zMin = min(zMin, pts.z());
        zMax = max(zMax, pts.z());
      }
    }
    // some rounding of limits
    xMin = floor(xMin);
    yMin = floor(yMin);
    zMin = floor(zMin);
    xMax = ceil(xMax);
    yMax = ceil(yMax);
    zMax = ceil(zMax);
    
    G4double rr2, rc2 = 0., Rc2 = 0., zch = 0.; 
    G4bool useCylinder = this->useCylinder && pgons[iPg].cylinderMakesSense;
    if(useCylinder) {
      rc2 = pow(pgons[iPg].tubr, 2.);
      Rc2 = pow(pgons[iPg].tubR, 2.);
      zch = pgons[iPg].zFace1 + pgons[iPg].thick;
    }

    G4Point3D theCenter(0., 0., 0.);
    G4Point3D myPoint;
    G4double  theVolume;
    G4int nseen = 0;
    G4int ntot  = 0;
    G4int ssold = -1;
    G4double xx, yy, zz;
    G4double dd1 = step;
    G4double dd2 = dd1/2.;
    EInside inside;
    G4ThreeVector pnt;

    for(zz = zMin+dd2; zz < zMax; zz += dd1) {
      for(yy = yMin+dd2; yy < yMax; yy += dd1) {
        for(xx = xMin+dd2; xx < xMax; xx += dd1) {
          myPoint.set(xx, yy, zz);
          ntot++;
          // Intersection with the cylinder: we check directly
          // (however, we neglect the passive areas)
          if( useCylinder ) {
            rr2 = myPoint.x() * myPoint.x() + myPoint.y() * myPoint.y();
            if(rr2 > Rc2) continue;                                 // outside cyl
            if( ( myPoint.z() > zch ) && ( rr2 < rc2 ) ) continue;  // coax hole
          }

          // check if inside one of the sub-segments composing this segment
          pnt = G4ThreeVector(myPoint);
          if(ssold >= 0) {                                          // start with the old one
            inside = ppsegs[ssold]->pPoly->Inside(pnt);
            if(inside == kInside) {
              nseen++;
              theCenter += myPoint;
              continue;                                             // no need to check the others
            }
          }
          for(ss = 0; ss < 4; ss++) {                               // look into the (other) segments 
            if(ss == ssold) continue;
            inside = ppsegs[ss]->pPoly->Inside(pnt);
            if(inside == kInside) {
              nseen++;
              theCenter += myPoint;
              ssold = ss;
              break;                                                // no need to check the others
            }
          }
          ssold = -1;                                               // outside all of them
        }
      }
    }

    theCenter = theCenter/nseen;
    theVolume = nseen*pow(dd1,3.);
    printf( " Crystal# %2d  Segment# %3d : %6d shot %6d seen --> Volume = %6.3f cm3   Center = (%7.3f %7.3f %7.3f) cm\n",
              iPg, ns, ntot, nseen, theVolume/cm3, theCenter.x()/cm, theCenter.y()/cm, theCenter.z()/cm);
    segVolume[start + ns] = theVolume;
    segCenter[start + ns] = theCenter;
  }
  ppsegs.clear();
}


///////////////////////////////////////////////////////////////////
/// methods for the messenger
////////////////////////////////////////////////////////////////////
void HR_Array::SetSolidFile(G4String nome)
{
  if( nome(0) == '/' )
    solidFile = nome;
  else {
    if( nome.find( "./", 0 ) != string::npos ) {
      G4int position = nome.find( "./", 0 );
      if( position == 0 )
        nome.erase( position, 2 );
    }  
    solidFile = iniPath + nome;
  }  
     
  G4cout << " ----> The solid vertexes are read from "
             << solidFile << G4endl;
}
  
void HR_Array::SetAngleFile(G4String nome)
{
  if( nome(0) == '/' )
    eulerFile = nome;
  else {
    if( nome.find( "./", 0 ) != string::npos ) {
      G4int position = nome.find( "./", 0 );
      if( position == 0 )
        nome.erase( position, 2 );
    }  
    eulerFile = iniPath + nome;
  }  

  G4cout << " ----> The Euler angles are read from "
             << eulerFile << G4endl;
}
    
void HR_Array::SetWallsFile(G4String nome)
{
  if( nome(0) == '/' )
    wallsFile = nome;
  else {
    if( nome.find( "./", 0 ) != string::npos ) {
      G4int position = nome.find( "./", 0 );
      if( position == 0 )
        nome.erase( position, 2 );
    }  
    wallsFile = iniPath + nome;
  }  
  
  G4cout << " ----> The walls vertexes are read from "
             << wallsFile << G4endl;
}
    
void HR_Array::SetClustFile(G4String nome)
{
  if( nome(0) == '/' )
    clustFile = nome;
  else {
    if( nome.find( "./", 0 ) != string::npos ) {
      G4int position = nome.find( "./", 0 );
      if( position == 0 )
        nome.erase( position, 2 );
    }  
    clustFile = iniPath + nome;
  }  
  
  G4cout << " ----> The cluster descriptions are read from "
             << clustFile << G4endl;
}

void HR_Array::SetSliceFile(G4String nome)
{
  if( nome(0) == '/' )
    sliceFile = nome;
  else {
    if( nome.find( "./", 0 ) != string::npos ) {
      G4int position = nome.find( "./", 0 );
      if( position == 0 )
        nome.erase( position, 2 );
    }  
    sliceFile = iniPath + nome;
  }  
  
  G4cout << " ----> The slice planes are read from "
             << sliceFile << G4endl;
}
  
void HR_Array::SetDetMate(G4String materialName)
{
  // search the material by its name
  G4Material* ptMaterial = G4Material::GetMaterial(materialName);
  if (ptMaterial) {
    matCrystName = materialName;
    G4cout << "\n ----> The detector material is "
          << materialName << G4endl;
  }
  else {
    G4cout << " Material not found! " << G4endl;
    G4cout << "Keeping previously set detector material ("
           << matCryst->GetName() << ")" << G4endl;
  }
}

void HR_Array::SetWallsMate(G4String materialName)
{
  // search the material by its name
  G4Material* ptMaterial = G4Material::GetMaterial(materialName);
  if (ptMaterial) {
    matWallsName = materialName;
    G4cout << "\n ----> The walls material is "
          << materialName << G4endl;
  }
  else {
    G4cout << " Material not found! " << G4endl;
    G4cout << " ----> Keeping previously set walls material ("
           << matWalls->GetName() << ")" << G4endl;
  }
}

void HR_Array::SetBackWallsMate(G4String materialName)
{
  // search the material by its name
  G4Material* ptMaterial = G4Material::GetMaterial(materialName);
  if (ptMaterial) {
    matBackWallsName = materialName;
    G4cout << "\n ----> The back walls material is "
          << materialName << G4endl;
  }
  else {
    G4cout << " Material not found! " << G4endl;
    G4cout << " ----> Keeping previously set walls material ("
           << matBackWalls->GetName() << ")" << G4endl;
  }
}

void HR_Array::SetThetaShift( G4double angle )
{
  thetaShift = angle;
  G4cout << " ----> The positions are rotated by an angle theta = " << thetaShift/deg << " degrees" << G4endl;
}

void HR_Array::SetThetaPrisma( G4double angle )
{
  thetaPrisma = angle;
  G4cout << " ----> PRISMA is rotated by an angle theta = " << thetaPrisma/deg << " degrees" << G4endl;
}

void HR_Array::SetPhiShift( G4double angle )
{
  phiShift = angle;
  G4cout << " ----> The positions are rotated by an angle phi = " << phiShift/deg << " degrees" << G4endl;
}

void HR_Array::SetUseCylinder( G4bool value )
{
  useCylinder = value;
  if( useCylinder ) {
    if( drawReadOut ) {
      G4cout << " Cannot use cylinder when drawing the Read Out geometry! " << G4endl;
      useCylinder = false;
      G4cout << " ----> The intersection between a cylinder and a polyhedron will not be performed." << G4endl;
    }
    else
      G4cout << " ----> The intersection between a cylinder and a polyhedron will be performed." << G4endl;
  }
  else  
    G4cout << " ----> The intersection between a cylinder and a polyhedron will not be performed." << G4endl;
}

void HR_Array::SetUsePassive( G4bool value )
{
  usePassive = value;
  if( usePassive ) {
    if( drawReadOut ) {
      G4cout << " Cannot use passive germanium parts when drawing the Read Out geometry! " << G4endl;
      usePassive = false;
      G4cout << " ----> Passive germanium parts will not be placed." << G4endl;
    }
    else
      G4cout << " ----> Passive germanium parts will be placed." << G4endl;
  }
  else  
    G4cout << " ----> Passive germanium parts will not be placed." << G4endl;
}

void HR_Array::SetDrawReadOut( G4bool value )
{
  drawReadOut = value;
  if( drawReadOut ) {
    usePassive  = false;
    useCylinder = false;
    G4cout << " ----> The read out geometry will be built as actual geometry." << G4endl;
    G4cout << " ----> The use of the cylinder and of the passive germanium parts will be disabled." << G4endl;
  }  
  else  
    G4cout << " ----> The read out geometry will be built." << G4endl;
}

void HR_Array::SetMakeCapsules( G4bool value )
{
  makeCapsule = value;
  if( makeCapsule )
    G4cout << " ----> The capsules will be generated." << G4endl;
  else  
    G4cout << " ----> The capsules will not be generated." << G4endl;
}

void HR_Array::SetStep( G4int factor )
{
  if( factor > 0 ) {
    stepFactor = factor;
    G4cout << " ----> Integration step has been set to " << 1./((G4double)stepFactor) << " mm" << G4endl;
    stepHasChanged = true;
  }
  else {
    G4cout << " ----> Could not change step, keeping step " << 1./((G4double)stepFactor) << " mm" << G4endl;
    stepHasChanged = false;
  }
}

void HR_Array::SetPosShift( G4ThreeVector shift )
{
  posShift = shift * mm;

  G4int prec = G4cout.precision(4);
  G4cout.setf(ios::fixed);
  G4cout  << " --> Array will be shifted to ("
          << std::setw(8) << posShift.x()/mm
          << std::setw(8) << posShift.y()/mm
          << std::setw(8) << posShift.z()/mm << " ) mm" << G4endl;
  G4cout.unsetf(ios::fixed);
  G4cout.precision(prec);
}
  
///////////////////
// The Messenger
///////////////////

#include "G4UIdirectory.hh"
#include "G4UIcmdWithAString.hh"
#include "G4UIcmdWithoutParameter.hh"
#include "G4UIcmdWithADouble.hh"
#include "G4UIcmdWith3Vector.hh"
#include "G4UIcmdWithAnInteger.hh"
#include "G4UIcmdWithABool.hh"

HR_Array_Messenger::HR_Array_Messenger(HR_Array* pTarget)
:myTarget(pTarget)
{ 
  const char *aLine;
  G4String commandName;
  G4String directoryName;

  directoryName = "/HR/detector/";

  commandName = directoryName + "solidFile";
  aLine = commandName.c_str();
  SetSolidCmd = new G4UIcmdWithAString(aLine, this);
  SetSolidCmd->SetGuidance("Select file with the detector vertexes.");
  SetSolidCmd->SetGuidance("Required parameters: 1 string.");
  SetSolidCmd->AvailableForStates(G4State_PreInit,G4State_Idle);
    
  commandName = directoryName + "clustFile";
  aLine = commandName.c_str();
  SetClustCmd = new G4UIcmdWithAString(aLine, this);
  SetClustCmd->SetGuidance("Select file with the cluster description.");
  SetClustCmd->SetGuidance("Required parameters: 1 string.");
  SetClustCmd->AvailableForStates(G4State_PreInit,G4State_Idle);
    
  commandName = directoryName + "angleFile";
  aLine = commandName.c_str();
  SetAngleCmd = new G4UIcmdWithAString(aLine, this);
  SetAngleCmd->SetGuidance("Select file with the detector angles.");
  SetAngleCmd->SetGuidance("Required parameters: 1 string.");
  SetAngleCmd->AvailableForStates(G4State_PreInit,G4State_Idle);
    
  commandName = directoryName + "wallsFile";
  aLine = commandName.c_str();
  SetWallsCmd = new G4UIcmdWithAString(aLine, this);
  SetWallsCmd->SetGuidance("Select file with the walls vertexes.");
  SetWallsCmd->SetGuidance("Required parameters: 1 string.");
  SetWallsCmd->AvailableForStates(G4State_PreInit,G4State_Idle);

  commandName = directoryName + "detectorMaterial";
  aLine = commandName.c_str();
  DetMatCmd = new G4UIcmdWithAString(aLine, this);
  DetMatCmd->SetGuidance("Select Material of the detector.");
  DetMatCmd->SetGuidance("Required parameters: 1 string.");
  DetMatCmd->SetParameterName("choice",false);
  DetMatCmd->AvailableForStates(G4State_PreInit,G4State_Idle);

  commandName = directoryName + "wallsMaterial";
  aLine = commandName.c_str();
  WalMatCmd = new G4UIcmdWithAString(aLine, this);
  WalMatCmd->SetGuidance("Select Material of the walls.");
  WalMatCmd->SetGuidance("Required parameters: 1 string.");
  WalMatCmd->SetParameterName("choice",false);
  WalMatCmd->AvailableForStates(G4State_PreInit,G4State_Idle);

  commandName = directoryName + "backWallsMaterial";
  aLine = commandName.c_str();
  BackWalMatCmd = new G4UIcmdWithAString(aLine, this);
  BackWalMatCmd->SetGuidance("Select Material of the back walls.");
  BackWalMatCmd->SetGuidance("Required parameters: 1 string.");
  BackWalMatCmd->SetParameterName("choice",false);
  BackWalMatCmd->AvailableForStates(G4State_PreInit,G4State_Idle);
  
  commandName = directoryName + "rotateArray";
  aLine = commandName.c_str();
  RotateArrayCmd = new G4UIcmdWithAString(aLine, this);
  RotateArrayCmd->SetGuidance("Select rotation of the array.");
  RotateArrayCmd->SetGuidance("Required parameters: 2 double (rotation angles in degrees).");
  RotateArrayCmd->SetParameterName("choice",false);
  RotateArrayCmd->AvailableForStates(G4State_PreInit,G4State_Idle);

  commandName = directoryName + "rotatePrisma";
  aLine = commandName.c_str();
  RotatePrismaCmd = new G4UIcmdWithADouble(aLine, this);
  RotatePrismaCmd->SetGuidance("Select rotation of PRISMA.");
  RotatePrismaCmd->SetGuidance("Required parameters: 1 double (rotation angle in degrees).");
  RotatePrismaCmd->SetParameterName("choice",false);
  RotatePrismaCmd->AvailableForStates(G4State_PreInit,G4State_Idle);

  commandName = directoryName + "traslateArray";
  aLine = commandName.c_str();
  TraslateArrayCmd = new G4UIcmdWith3Vector(aLine, this);
  TraslateArrayCmd->SetGuidance("Select traslation of the array.");
  TraslateArrayCmd->SetGuidance("Required parameters: 3 double (x, y, z components in mm).");
  TraslateArrayCmd->AvailableForStates(G4State_PreInit,G4State_Idle);

  commandName = directoryName + "writeSegments";
  aLine = commandName.c_str();
  WriteAnglesCmd = new G4UIcmdWithAString(aLine, this);
  WriteAnglesCmd->SetGuidance("Writes out segment positions in several formats.");
  WriteAnglesCmd->SetGuidance("Required parameters: 1 string (name of the file where angles are written out), 1 integer.");
  WriteAnglesCmd->SetGuidance("  --> 0: writes out segment angles in GSORT format.");
  WriteAnglesCmd->SetGuidance("  --> 1: writes out segment positions in polar coordinates.");
  WriteAnglesCmd->SetGuidance("  --> 2: writes out segment positions in cartesian coordinates.");
  WriteAnglesCmd->SetParameterName("choice",false);
  WriteAnglesCmd->AvailableForStates(G4State_PreInit,G4State_Idle);

  commandName = directoryName + "writeAngles";
  aLine = commandName.c_str();
  WriteCryAnglesCmd = new G4UIcmdWithAString(aLine, this);
  WriteCryAnglesCmd->SetGuidance("Writes out crystal angles in GSORT format.");
  WriteCryAnglesCmd->SetGuidance("Required parameters: 1 string (name of the file where angles are written out)");
  WriteCryAnglesCmd->SetParameterName("choice",false);
  WriteCryAnglesCmd->AvailableForStates(G4State_PreInit,G4State_Idle);

  commandName = directoryName + "enablePassive";
  aLine = commandName.c_str();
  EnablePassiveCmd = new G4UIcmdWithABool(aLine, this);
  EnablePassiveCmd->SetGuidance("Generate passive germanium parts");
  EnablePassiveCmd->SetGuidance("Required parameters: none.");
  EnablePassiveCmd->SetParameterName("usePassive",true);
  EnablePassiveCmd->SetDefaultValue(true);
  EnablePassiveCmd->AvailableForStates(G4State_PreInit,G4State_Idle);

  commandName = directoryName + "disablePassive";
  aLine = commandName.c_str();
  DisablePassiveCmd = new G4UIcmdWithABool(aLine, this);
  DisablePassiveCmd->SetGuidance("Do not generate passive germanium parts");
  DisablePassiveCmd->SetGuidance("Required parameters: none.");
  DisablePassiveCmd->SetParameterName("usePassive",true);
  DisablePassiveCmd->SetDefaultValue(false);
  DisablePassiveCmd->AvailableForStates(G4State_PreInit,G4State_Idle);
  
  commandName = directoryName + "enableCapsules";
  aLine = commandName.c_str();
  EnableCapsulesCmd = new G4UIcmdWithABool(aLine, this);
  EnableCapsulesCmd->SetGuidance("Generate passive capsules.");
  EnableCapsulesCmd->SetGuidance("Required parameters: none.");
  EnableCapsulesCmd->SetParameterName("makeCapsule",true);
  EnableCapsulesCmd->SetDefaultValue(true);
  EnableCapsulesCmd->AvailableForStates(G4State_PreInit,G4State_Idle);

  commandName = directoryName + "disableCapsules";
  aLine = commandName.c_str();
  DisableCapsulesCmd = new G4UIcmdWithABool(aLine, this);
  DisableCapsulesCmd->SetGuidance("Do not generate passive capsules.");
  DisableCapsulesCmd->SetGuidance("Required parameters: none.");
  DisableCapsulesCmd->SetParameterName("makeCapsule",true);
  DisableCapsulesCmd->SetDefaultValue(false);
  DisableCapsulesCmd->AvailableForStates(G4State_PreInit,G4State_Idle);
  
  commandName = directoryName + "enableCyl";
  aLine = commandName.c_str();
  EnableCylCmd = new G4UIcmdWithABool(aLine, this);
  EnableCylCmd->SetGuidance("Consider the intersection between a cylinder and a polyhedron.");
  EnableCylCmd->SetGuidance("Required parameters: none.");
  EnableCylCmd->SetParameterName("useCylinder",true);
  EnableCylCmd->SetDefaultValue(true);
  EnableCylCmd->AvailableForStates(G4State_PreInit,G4State_Idle);

  commandName = directoryName + "enableCryostats";
  aLine = commandName.c_str();
  cryostatCmd = new G4UIcmdWithoutParameter(aLine, this);
  cryostatCmd->SetGuidance("Include cryostats");
  cryostatCmd->SetGuidance("Required parameters: none.");
  cryostatCmd->AvailableForStates(G4State_PreInit,G4State_Idle);

  commandName = directoryName + "drawReadOut";
  aLine = commandName.c_str();
  DrawReadOutCmd = new G4UIcmdWithABool(aLine, this);
  DrawReadOutCmd->SetGuidance("Generates the read out geometry as the actual geometry.");
  DrawReadOutCmd->SetGuidance("For geometry testing purposes only, DO NOT RUN!");
  DrawReadOutCmd->SetGuidance("Required parameters: none.");
  DrawReadOutCmd->SetParameterName("drawReadOut",true);
  DrawReadOutCmd->SetDefaultValue(true);
  DrawReadOutCmd->AvailableForStates(G4State_PreInit,G4State_Idle);

  commandName = directoryName + "dontdrawReadOut";
  aLine = commandName.c_str();
  DontDrawReadOutCmd = new G4UIcmdWithABool(aLine, this);
  DontDrawReadOutCmd->SetGuidance("Disables the read out geometry as the actual geometry.");
  DontDrawReadOutCmd->SetGuidance("Required parameters: none.");
  DontDrawReadOutCmd->SetParameterName("drawReadOut",true);
  DontDrawReadOutCmd->SetDefaultValue(false);
  DontDrawReadOutCmd->AvailableForStates(G4State_PreInit,G4State_Idle);

  commandName = directoryName + "step";
  aLine = commandName.c_str();
  SetStepCmd = new G4UIcmdWithAnInteger(aLine, this);  
  SetStepCmd->SetGuidance("Define size of the integration step to calculate the segment position.");
  SetStepCmd->SetGuidance("Default step value: 1 mm.");
  SetStepCmd->SetGuidance("Required parameters: 1 integer (step will be divided by the input value).");
  SetStepCmd->AvailableForStates(G4State_PreInit,G4State_Idle);

}

HR_Array_Messenger::~HR_Array_Messenger()
{
  delete DetMatCmd;
  delete WalMatCmd;
  delete BackWalMatCmd;
  delete RotateArrayCmd;
  delete RotatePrismaCmd;
  delete TraslateArrayCmd;
  delete WriteAnglesCmd;
  delete WriteCryAnglesCmd;
  delete SetSolidCmd;
  delete SetAngleCmd;
  delete SetWallsCmd;
  delete SetClustCmd;
  delete SetSliceCmd;
  delete EnableCylCmd;
  delete DisableCylCmd;
  delete EnablePassiveCmd;
  delete DisablePassiveCmd;
  delete EnableCapsulesCmd;
  delete DisableCapsulesCmd;
  delete DrawReadOutCmd;
  delete DontDrawReadOutCmd;
  delete SetStepCmd;
}

void HR_Array_Messenger::SetNewValue(G4UIcommand* command,G4String newValue)
{ 

  if( command == SetSolidCmd ) {
    myTarget->SetSolidFile(newValue);
  } 
  if( command == SetAngleCmd ) {
    myTarget->SetAngleFile(newValue);
  } 
  if( command == SetWallsCmd ) {
    myTarget->SetWallsFile(newValue);
  } 
#ifdef ANTIC
  if( command == SetAnticCmd ) {
    myTarget->SetAnticFile(newValue);
  } 
#endif  
  if( command == SetClustCmd ) {
    myTarget->SetClustFile(newValue);
  } 
  if( command == DetMatCmd ) {
    myTarget->SetDetMate(newValue);
  }
  if( command == WalMatCmd ) {
    myTarget->SetWallsMate(newValue);
  }
  if( command == BackWalMatCmd ) {
    myTarget->SetBackWallsMate(newValue);
  }
  if( command == RotateArrayCmd ) {
    float e1, e2;
//    sscanf( newValue, "%lf %lf", &e1, &e2);
    sscanf( newValue, "%f %f", &e1, &e2);
    myTarget->SetThetaShift( ((G4double)e1)*deg );
    myTarget->SetPhiShift  ( ((G4double)e2)*deg );
  }
  if( command == RotatePrismaCmd ) {
    myTarget->SetThetaPrisma(RotatePrismaCmd->GetNewDoubleValue(newValue)*deg);
  }
  if( command == TraslateArrayCmd ) {
    myTarget->SetPosShift(TraslateArrayCmd->GetNew3VectorValue(newValue));
  }
  if( command == EnableCylCmd ) {
    myTarget->SetUseCylinder( EnableCylCmd->GetNewBoolValue(newValue) );
  }
  if( command == DisableCylCmd ) {
    myTarget->SetUseCylinder( DisableCylCmd->GetNewBoolValue(newValue) );
  }
  if( command == cryostatCmd ) {
    myTarget->SetCryostats();
  }
  if( command == EnablePassiveCmd ) {
    myTarget->SetUsePassive( EnablePassiveCmd->GetNewBoolValue(newValue) );
  }
  if( command == DisablePassiveCmd ) {
    myTarget->SetUsePassive( DisablePassiveCmd->GetNewBoolValue(newValue) );
  }
  if( command == WriteAnglesCmd ) {
    G4int length = newValue.length();
    G4int position = 0;
    G4int format;
    G4String name;
    G4String formato;
    if( newValue.find(" ", position) != string::npos ) {
      position = newValue.find(" ", position);
      name = newValue.substr(0, position);
      formato = newValue.substr(position, length);
      sscanf( formato.c_str(), "%d", &format);
      myTarget->WriteSegmentAngles(G4String(name), format);
    }  
  }
  if( command == WriteCryAnglesCmd ) {
    myTarget->WriteCrystalAngles(newValue);
  }

  if( command == DrawReadOutCmd ) {
    myTarget->SetDrawReadOut( DrawReadOutCmd->GetNewBoolValue(newValue) );
  }
  if( command == DontDrawReadOutCmd ) {
    myTarget->SetDrawReadOut( DontDrawReadOutCmd->GetNewBoolValue(newValue) );
  }
  if( command == EnableCapsulesCmd ) {
    myTarget->SetMakeCapsules( EnableCapsulesCmd->GetNewBoolValue(newValue) );
  }
  if( command == DisableCapsulesCmd ) {
    myTarget->SetMakeCapsules( DisableCapsulesCmd->GetNewBoolValue(newValue) );
  }

  if( command == SetStepCmd ) {
    myTarget->SetStep( SetStepCmd->GetNewIntValue(newValue) );
  }
}
