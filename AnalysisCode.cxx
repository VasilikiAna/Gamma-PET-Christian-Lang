/*
* CHLGPETAnalysis.cxx
*
2011 by Christian Lang
*
*
based on ExternalAnalysisPipelineExample.cxx
*
Copyright (C) 1998-2010 by Andreas Zoglauer.
*
*
* All rights reserved.
*
*
* This code implementation is the intellectual property of
* Andreas Zoglauer and Christian Lang.
*
* By copying, distributing or modifying the Program (or any work
* based on the Program) you indicate your acceptance of this statement,
* and all its terms.
*
*/
// Standard
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <csignal>
#include <math.h>
#include <stdlib.h>
using namespace std;

// ROOT
#include <TROOT.h>
#include <TEnv.h>
#include <TSystem.h>
#include <TApplication.h>
#include <TStyle.h>
#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>
#include <TLine.h>

// MEGAlib
#include "MGlobal.h"
#include "MStreams.h"
#include "MDGeometryQuest.h"
#include "MDDetector.h"
#include "MFileEventsSim.h"
#include "MDVolumeSequence.h"
#include "MSimEvent.h"
#include "MSimHT.h"
#include "MGeometryRevan.h"
#include "MRERawEvent.h"
#include "MREHit.h"
#include "MRawEventAnalyzer.h"
#include "MERCSRChiSquare.h"
#include "MImagerExternallyManaged.h"
#include "MBPData.h"
#include "MImage.h"
#include "MVector.h"
//#include "MTime.h"


float genergycutmin = 1200.0, genergycutmax = 1350.0;
/******************************************************************************/
class ExternalAnalysisPipelineExample
{
public:

  /// Default constructor
  ExternalAnalysisPipelineExample();
  /// Default destructor
  ~ExternalAnalysisPipelineExample();


  /// Parse the command line
  bool ParseCommandLine(int argc, char** argv);

  /// Initialize the run
  bool Initialize();

  /// Analyze what eveer needs to be analyzed...
  bool Analyze();

  /// Interrupt the analysis
  void Interrupt() { m_Interrupt = true; }

private:
  ///
  MRERawEvent* Convert(MSimEvent*);

private:
  /// True, if the analysis needs to be interrupted
  bool m_Interrupt;

  /// The geometry file name
  MString m_GeometryFileName;

  /// The simulation file name
  MString m_SimulationFileName;

  /// A standard geometry
  MDGeometryQuest* m_Geometry;

  /// A special geometry for revan
  MGeometryRevan* m_RevanGeometry;


  /// The event reconstructor
  MRawEventAnalyzer* m_RawEventAnalyzer;
  MRawEventAnalyzer* m_RawEventAnalyzerOben;
  MRawEventAnalyzer* m_RawEventAnalyzerUnten;
  MRawEventAnalyzer* m_RawEventAnalyzerLinks;
  MRawEventAnalyzer* m_RawEventAnalyzerRechts;

  
  /// The image reconstructor
  MImagerExternallyManaged* m_Imager;
  MEventSelector S;

  TH1D* m_HistEnergyObenDSSSD;
  TH1D* m_HistEnergyUntenDSSSD;
  TH1D* m_HistEnergyUntenAbsorber;
  TH1D* m_HistEnergySumCC;
  TH1D* m_HistEnergyObenCC;
  TH1D* m_HistEnergyISDSSSDOben;
  TH1D* m_HistEnergyISDSSSDRechts;
  TH1D* m_HistEnergyBeforeLinks;
  TH1D* m_HistEnergyEmi;
  TH1D* m_HistEnergyBefore;  //i added this here
  TH2D* m_HistAnglevsEnergy;

  TH1D* m_HistTime;
  TH1D* m_HistDiametral;
  TH2D* m_HistLines;
  TH2D* m_HistHits;



};
/******************************************************************************/
/******************************************************************************
* Default constructor
*/
ExternalAnalysisPipelineExample::ExternalAnalysisPipelineExample() : m_Interrupt(false)
{
gStyle->SetPalette(1, 0);
}


/******************************************************************************
* Default destructor
*/
ExternalAnalysisPipelineExample::~ExternalAnalysisPipelineExample()
{
// Intentionally left blanck
}


/******************************************************************************
* Parse the command line
*/

bool ExternalAnalysisPipelineExample::ParseCommandLine(int argc, char** argv)
{
  ostringstream Usage;
  Usage<<endl;
  Usage<<" Usage: ExternalAnalysisPipelineExample <options>"<<endl;
  Usage<<"General options:"<<endl;
  Usage<<"-g:geometry file name"<<endl;
  Usage<<"-s:simulation file name"<<endl;
  Usage<<"-h:print this help"<<endl;
  Usage<<endl;

  string Option;

  // Check for help
  for (int i = 1; i < argc; i++) {
   Option = argv[i];
   if (Option == "-h" || Option == "--help" || Option == "?" || Option == "-?") {
    cout<<Usage.str()<<endl;
    return false;
   }
  }


  // Now parse the command line options:
  for (int i = 1; i < argc; i++) {
   Option = argv[i];
   // First check if each option has sufficient arguments:
   // Single argument
   if (Option == "-s" || Option == "-g") {
    if (!((argc > i+1) && 
            (argv[i+1][0] != '-' || isalpha(argv[i+1][1]) == 0))){
     cout<<"Error: Option "<<argv[i][1]<<" needs a second argument!"<<endl;
     cout<<Usage.str()<<endl;
     return false;
    }
   }

    // Multiple arguments template
    /*
    else if (Option == "-??") {
   if (!((argc > i+2) &&
    (argv[i+1][0] != ’-’ || isalpha(argv[i+1][1]) == 0) &&
    (argv[i+2][0] != ’-’ || isalpha(argv[i+2][1]) == 0))){
    cout<<"Error: Option "<<argv[i][1]<<" needs two arguments!"<<endl;
    cout<<Usage.str()<<endl;
    return false;
    }
    }
    */

   // Then fulfill the options:
   if (Option == "-s") {
    m_SimulationFileName = argv[++i];
    cout<<"Accepting simulation file name: "<<m_SimulationFileName<<endl;
    } else if (Option == "-g") {
    m_GeometryFileName = argv[++i];
    cout<<"Accepting geometry file name: "<<m_GeometryFileName<<endl;
    } else {
    cout<<"Error: Unknown option \""<<Option<<"\"!"<<endl;
    cout<<Usage.str()<<endl;
    return false;
   }
  }
 return true;
}
/******************************************************************************

 Do whatever analysis is necessary
*
  I N I T I A L I Z E
*
*
*/

bool ExternalAnalysisPipelineExample::Initialize()
{
  // Load geometry:
  m_Geometry = new MDGeometryQuest();

  if (m_Geometry->ScanSetupFile(m_GeometryFileName) == true) {
   cout<<"Geometry "<<m_Geometry->GetName()<<" loaded!"<<endl;
   m_Geometry->ActivateNoising(true);
   m_Geometry->SetGlobalFailureRate(0.0);
   } else {
   cout<<"Loading of geometry "<<m_Geometry->GetName()<<" failed!!"<<endl;
   return false;
  }

  // Load geometry:
  m_RevanGeometry = new MGeometryRevan();

  if (m_RevanGeometry->ScanSetupFile(m_GeometryFileName) == true) {

   cout<<"Geometry "<<m_RevanGeometry->GetName()<<" loaded!"<<endl;
   m_RevanGeometry->ActivateNoising(true);
   m_RevanGeometry->SetGlobalFailureRate(0.0);
   } else {
   cout<<"Loading of geometry "<<m_RevanGeometry->GetName()<<" failed!!"<<endl;
   return false;
  }

  

  // Initialize the raw event analyzer:

  m_RawEventAnalyzer = new MRawEventAnalyzer();
  m_RawEventAnalyzer->SetGeometry(m_RevanGeometry);
  //m_RawEventAnalyzer->SetCoincidenceAlgorithm(MRawEventAnalyzer::c_CoincidenceAlgoWindow);
  //m_RawEventAnalyzer->SetCoincidenceWindow(1E-9);
  m_RawEventAnalyzer->SetClusteringAlgorithm(MRawEventAnalyzer::c_ClusteringAlgoDistance);
  m_RawEventAnalyzer->SetStandardClusterizerMinDistanceD1(0.039);
  m_RawEventAnalyzer->SetStandardClusterizerMinDistanceD2(0.625);
  m_RawEventAnalyzer->SetStandardClusterizerMinDistanceD3(0.625);
  m_RawEventAnalyzer->SetStandardClusterizerMinDistanceD4(0.625);
  m_RawEventAnalyzer->SetStandardClusterizerMinDistanceD5(0.625);
  m_RawEventAnalyzer->SetStandardClusterizerMinDistanceD6(0.625);
  m_RawEventAnalyzer->SetStandardClusterizerCenterIsReference(true);

  m_RawEventAnalyzer->SetTrackingAlgorithm(MRawEventAnalyzer::c_TrackingAlgoRank);
  m_RawEventAnalyzer->SetDoTracking(false);
  m_RawEventAnalyzer->SetRejectPurelyAmbiguousTrackSequences(true);
  m_RawEventAnalyzer->SetMaxComptonJump(1);
  m_RawEventAnalyzer->SetNTrackSequencesToKeep(1);
  m_RawEventAnalyzer->SetSearchComptonTracks(true);
  m_RawEventAnalyzer->SetSearchPairTracks(false);

  //Chi-Square via angles
  //m_RawEventAnalyzer->SetCSRAlgorithm(MRawEventAnalyzer::c_CSRAlgoFoM);
  //Chi-Square via energies
  m_RawEventAnalyzer->SetCSRAlgorithm(MRawEventAnalyzer::c_CSRAlgoFoME);
  //m_RawEventAnalyzer->SetCSRAlgorithm(MRawEventAnalyzer::c_CSRAlgoFoMToF);
  //Chi-Square via angles + time
  //m_RawEventAnalyzer->SetCSRAlgorithm(MRawEventAnalyzer::c_CSRAlgoFoMToFAndE);
  //Chi-Square via Bayesian
  //m_RawEventAnalyzer->SetCSRAlgorithm(MRawEventAnalyzer::c_CSRAlgoBayesian);
  //m_RawEventAnalyzer->SetClassicUndecidedHandling(MERCSRChiSquare::c_UndecidedLargerEnergyDeposit);
  //m_RawEventAnalyzer->SetClassicUndecidedHandling(MERCSRChiSquare::c_UndecidedAssumestartD1);
  //m_RawEventAnalyzer->SetClassicUndecidedHandling(MERCSRChiSquare::c_UndecidedLargerKleinNishina);
  m_RawEventAnalyzer->SetClassicUndecidedHandling(MERCSRChiSquare::c_UndecidedIgnore);
  m_RawEventAnalyzer->SetAssumeD1First(true);
  m_RawEventAnalyzer->SetGuaranteeStartD1(true);
  m_RawEventAnalyzer->SetUseComptelTypeEvents(true);
  m_RawEventAnalyzer->SetRejectOneDetectorTypeOnlyEvents(true);
  m_RawEventAnalyzer->SetCSRMaxNHits(6);

  m_RawEventAnalyzer->SetDecayAlgorithm(MRawEventAnalyzer::c_DecayAlgoNone);
  m_RawEventAnalyzer->SetRejectAllBadEvents(true);
  //m_RawEventAnalyzer->SetCSRThresholdMin(0.5);
  //m_RawEventAnalyzer->SetCSRThresholdMax(0.9);
  m_RawEventAnalyzer->SetTotalEnergyMin(genergycutmin);
  m_RawEventAnalyzer->SetTotalEnergyMax(genergycutmax);
  if (m_RawEventAnalyzer->PreAnalysis() == false) return false;

  // Some histograms....
  m_HistEnergyEmi = new TH1D("HistEmi", "HistEmi", 500, 0, 2500);
  m_HistEnergyObenDSSSD = new TH1D("HistObenDSSSD", "HistObenDSSSD", 500, 0, 2500);
  m_HistEnergyUntenDSSSD = new TH1D("HistUntenDSSSD","HistUntenDSSSD",500,0,2500);
  m_HistEnergyUntenAbsorber = new TH1D("HistUntenAbsorber","HistUntenAbsorber",500,0,2000);
  m_HistEnergySumCC = new TH1D("HistSumCC", "HistSumCC", 500, 0, 2500);
  m_HistEnergyObenCC = new TH1D("HistObenCC", "HistObenCC", 500, 0, 2500);
  m_HistEnergyISDSSSDRechts = new TH1D("HistISDSSSDRechts", "HistISDSSSDRechts", 500, 0, 2500);
  m_HistEnergyISDSSSDOben = new TH1D("HistISDSSSDOben", "HistISDSSSDOben", 500, 0, 2500);
  m_HistEnergyBeforeLinks = new TH1D("HistBeforeLinks", "HistBeforeLinks", 200, 0, 2000);
  m_HistEnergyBefore = new TH1D("HistEnergyBefore", "HistEnergyBefore", 200, 0 , 2000);

  m_Imager = new MImagerExternallyManaged(MProjection::c_Cartesian3D);
  m_Imager->SetGeometry(m_Geometry);

  m_Imager->SetViewport(
   -0.51,
   0.51,
   100,
    -0.51,
    0.51,
    100,
   -0.51,
   0.51,
     100);

  // Set the draw modes
  m_Imager->SetPalette(0);
  // Palette: 0: Rainbow, 1: BW, 2: WB, 3: Blue, 4: Red/Blue, 5: Rainbow2, 6: Red, 7: Red/White, 8: Red2, 9:Red3,
  //10:rainbow3
  //m_Imager->SetPalette(m_Data->GetImagePalette());
  //m_Imager->SetSourceCatalog(m_Data->GetImageSourceCatalog());


  //if (Animate == true) {
  // m_Imager->SetAnimationMode(m_Data->GetAnimationMode());
  m_Imager->SetAnimationFrameTime(5);
  m_Imager->SetAnimationFileName("test.gif");
  //} else {
  m_Imager->SetAnimationMode(MImager::c_AnimateBackprojections);
  // }

  // Maths:
  m_Imager->SetApproximatedMaths(false);
  // Set the response type
  m_Imager->SetResponseGaussian(10, 30, 3, 2.5, false);
  m_Imager->SetMemoryManagment(60000, 60000, 2, 1);

  // A new event selector:
  //MEventSelector S;
  S.SetGeometry(m_Geometry);
  S.SetComptonAngle(0,180);
  S.UseComptons(true);
  S.UseTrackedComptons(false);
  S.UseNotTrackedComptons(true);
  S.UsePairs(false);
  S.UsePhotos(false);

  m_Imager->SetEventSelector(S);
  m_Imager->SetDeconvolutionAlgorithmClassicEM();
  //m_Imager->SetStopCriterionByIterations(90);
  m_Imager->SetStopCriterionByIterations(3);
  m_Imager->Initialize();

  return true;
}



/******************************************************************************
* Do whatever analysis is necessary
*/
MRERawEvent* ExternalAnalysisPipelineExample::Convert(MSimEvent* SE)
{

  MRERawEvent* RE = new MRERawEvent();


  RE->SetEventTime(SE->GetTime());
  RE->SetEventId(SE->GetID());
  
  

  // Create raw event hit out of each SimHT and add it to the raw event (RE)
  for (unsigned int h = 0; h < SE->GetNHTs(); ++h) {
   MREHit* REHit = new MREHit();
   REHit->SetDetector(SE->GetHTAt(h)->GetDetectorType());
   REHit->SetPosition(SE->GetHTAt(h)->GetPosition());
   cout<<"SE->GetHTAt(h)->GetNHTs(): " << SE->GetNHTs() << endl;
   cout<<"SE->GetHTAt(h)->GetPosition(): " << SE->GetHTAt(h)->GetPosition() << endl;
   cout<<"SE->GetHTAt(h)->GetPositionX(): " << REHit->GetPositionX() << endl;
   cout<<"SE->GetHTAt(h)->GetPositionY(): " << REHit->GetPositionY() << endl;
   cout<<"SE->GetHTAt(h)->GetPositionZ(): " << REHit->GetPositionZ() << endl;
   cout<<"SE->GetHTAt(h)->GetTime(): " << SE->GetTime() << endl;
   cout<<"SE->GetHTAt(h)->GetDetectorType(): " << SE->GetHTAt(h)->GetDetectorType() << endl;


   REHit->SetEnergy(SE->GetHTAt(h)->GetEnergy());
   REHit->SetTime(SE->GetHTAt(h)->GetTime() + (SE->GetTime()).GetAsDouble());  //REHit->SetTime(SE->GetHTAt(h)->GetTime() + SE->GetTime()); this is how it was in Christan's thesis
   REHit->RetrieveResolutions(m_Geometry);
   REHit->Noise(m_Geometry); // <- for sims only!!
   RE->AddRESE(REHit);
   }
  return RE;
}




/******************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
A N A L Y Z E
//////////////////////////////////////////////////////////////////////////////////////////////////////////
*///////////////////////////////////////////////////////////////////////////////////////////////////

bool ExternalAnalysisPipelineExample::Analyze()
{
  if (m_Interrupt == true) return false;

  // Initialize:
  if (Initialize() == false) return false;

  MFileEventsSim* SimReader = new MFileEventsSim(m_Geometry);
  if (SimReader->Open(m_SimulationFileName) == false) {
   cout<<"Unable to open sim file!"<<endl;
   return false;
   }

  cout << "Opened file " << SimReader->GetFileName() << " created with MEGAlib version: " << SimReader->GetMEGAlibVersion()<< endl;
  SimReader->ShowProgress();


  TCanvas* CanvasBackprojection = new TCanvas();
  TCanvas* CanvasIterated= new TCanvas();

  vector<MPhysicalEvent*> Events;
  vector<MBPData*> ResponseSlices;
  vector<MImage*> Images;


  int whileloopcounter = 0, RechtsLinksCounter = 0, ObenUntenCounter = 0, FlagRechts = 0, FlagLinks = 0, FlagOben =
  0, FlagUnten = 0, RechtsCounter = 0, UntenCounter = 0, LinksCounter = 0, LinksCounterNeg =0, LinksCounterCalori = 0, LinksCounterAbsorber = 0, RechtsCounterCalori = 0, UntenCounterCalori = 0, ObenCounterCalori = 0,
  LinksScatterer = 0, LinksScattererOnly = 0, RechtsScatterer = 0, UntenScatterer =0, ObenScatterer = 0, ObenCounter =0, SimCounter = 0;

  int LOR =0, Flag1275Rechts = 0, Flag1275Links = 0, Flag1275Oben = 0, Flag1275Unten = 0;

  
  double Time = 0.0, TimeTmp = 0.0, AnglePhi = 0.0,
  XPositionRechts = 0.0, YPositionRechts = 0.0, ZPositionRechts = 0.0,
  XPositionLinks = 0.0, YPositionLinks = 0.0, ZPositionLinks = 0.0,
  XPositionOben = 0.0, YPositionOben = 0.0, ZPositionOben = 0.0,
  XPositionUnten = 0.0, YPositionUnten = 0.0, ZPositionUnten = 0.0,
  XPositionRechts2 = 0.0, YPositionRechts2 = 0.0, ZPositionRechts2 = 0.0,
  XPositionLinks2 = 0.0, YPositionLinks2 = 0.0, ZPositionLinks2 = 0.0,
  XPositionOben2 = 0.0, YPositionOben2 = 0.0, ZPositionOben2 = 0.0,
  XPositionUnten2 = 0.0, YPositionUnten2 = 0.0, ZPositionUnten2 = 0.0,
  HitEnergyRechts = 0.0, HitEnergyRechts2 = 0.0,
  VMulti = 0.0, VNormL = 0.0, VNormR= 0.0 , Diametral = 0.0;
 
  
 
  double ScatterMin = 3.4, ScatterMax = 3.6, AbsorberMin = 6.5, AbsorberMax = 9.5; // I added these numbers
 
 // double ScatterMin = 3.1,ScatterMax = 4.0, AbsorberMin = 6.0, AbsorberMax = 12.0;  //this is according to Christian's thesis
  /*
    double ScatterMin = 4.5,
   ScatterMax = 8.0,
   AbsorberMin = 10.0,
   AbsorberMax = 16.0;
   */
  unsigned int ReturnCode;
  unsigned int ReturnCodeOben;
  unsigned int ReturnCodeUnten;
  unsigned int ReturnCodeRechts;
  unsigned int ReturnCodeLinks;

  MSimEvent* SimEvent = 0;
  MRERawEvent* RawEvent = 0;
  MRERawEvent* BestRawEvent = 0;
  MRawEventList* AllRawEvents = 0;


//*****************************************************************************************************
// Loop over the events
//
//******************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////
//
//While Loop
//////////////////////////////////////////////////////////////////////////////////////////////////////
 while ((SimEvent = SimReader->GetNextEvent(false)) != 0) {

   FlagRechts = 0, FlagLinks = 0, FlagOben = 0, FlagUnten = 0,
   Flag1275Rechts = 0, Flag1275Links = 0, Flag1275Oben = 0, Flag1275Unten = 0,

   XPositionRechts = 0.0, YPositionRechts = 0.0, ZPositionRechts = 0.0,
   XPositionLinks = 0.0, YPositionLinks = 0.0, ZPositionLinks = 0.0,
   XPositionOben = 0.0, YPositionOben = 0.0, ZPositionOben = 0.0,
   XPositionUnten = 0.0, YPositionUnten = 0.0, ZPositionUnten = 0.0,
   XPositionRechts2 = 0.0, YPositionRechts2 = 0.0, ZPositionRechts2 = 0.0,
   XPositionLinks2 = 0.0, YPositionLinks2 = 0.0, ZPositionLinks2 = 0.0,
   XPositionOben2 = 0.0, YPositionOben2 = 0.0, ZPositionOben2 = 0.0,
   XPositionUnten2 = 0.0, YPositionUnten2 = 0.0, ZPositionUnten2 = 0.0,
   HitEnergyRechts = 0.0, HitEnergyRechts2 = 0.0,
   AnglePhi = 0.0, VMulti = 0.0, VNormL = 0.0, VNormR= 0.0 , Diametral = 0.0;

  
   if (SimEvent->GetID() == 1) {
    Time = Time + TimeTmp;
   }
   
  
  TimeTmp = (SimEvent->GetTime()).GetAsDouble();    //TimeTmp = SimEvent->GetTime();  this is how it was in the Christian's thesis
  
  whileloopcounter = whileloopcounter + 1;
   // Convert to MRERawEvent
  // RawEvent = Convert(SimEvent);


//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************
//
//
//Event Sorter
//
//
//Assign events from cubic Compton camera setup (4 modules)
//
//to their respective camera module
//
//
//by Christian Lang 2011
//
//*****************************************************************************************
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  MBPData* Data = 0; 

  MRERawEvent*RE = new MRERawEvent();
  MRERawEvent*RELinks = new MRERawEvent();
  MRERawEvent*RERechts = new MRERawEvent();
  MRERawEvent*REOben = new MRERawEvent();
  MRERawEvent*REUnten = new MRERawEvent();
 

  RE->SetEventTime(SimEvent->GetTime());
  RE->SetEventId(SimEvent->GetID());
  REOben->SetEventTime(SimEvent->GetTime());
  REOben->SetEventId(SimEvent->GetID());
  REUnten->SetEventTime(SimEvent->GetTime());
  REUnten->SetEventId(SimEvent->GetID());
  RELinks->SetEventTime(SimEvent->GetTime());
  RELinks->SetEventId(SimEvent->GetID());
  RERechts->SetEventTime(SimEvent->GetTime());
  RERechts->SetEventId(SimEvent->GetID());

  //m_HistTime->Fill(SimEvent->GetTime());



////////////////////////
// Create raw event hit out of each SimHT and add it to the raw event (RE)
///////////////////////////
  for (unsigned int h = 0; h < SimEvent->GetNHTs(); ++h) {

   MREHit* REHit = new MREHit();
   REHit->SetDetector(SimEvent->GetHTAt(h)->GetDetectorType());
   REHit->SetPosition(SimEvent->GetHTAt(h)->GetPosition());
   REHit->SetEnergy(SimEvent->GetHTAt(h)->GetEnergy());
   REHit->SetTime(SimEvent->GetHTAt(h)->GetTime() + (SimEvent->GetTime()).GetAsDouble());
   //REHit->SetTime(SimEvent->GetHTAt(h)->GetTime() + SimEvent->GetTime());
   REHit->RetrieveResolutions(m_Geometry);
   REHit->Noise(m_Geometry); // <- for sims only

   m_HistEnergyEmi->Fill(REHit->GetEnergy());

   //////////flag selection of hits due to Compton camera arrangement and threshold
  
   // if (REHit->GetEnergy < 10) continue;

   if (REHit->GetPositionX() > ScatterMin && REHit->GetPositionX() < ScatterMax && FlagRechts == 1 && REHit->GetEnergy() > 0.0) {
    //cout << " Treffer RECHTS " << endl;
    FlagRechts = 2;
    XPositionRechts2 = REHit->GetPositionX();
    YPositionRechts2 = REHit->GetPositionY();
    ZPositionRechts2 = REHit->GetPositionZ();
    HitEnergyRechts2 = SimEvent->GetHTAt(h)->GetEnergy();
   }
   if (REHit->GetPositionX() > ScatterMin && REHit->GetPositionX() < ScatterMax && FlagRechts == 0 && REHit->GetEnergy() > 0.0) {
    //cout << " Treffer RECHTS " << endl;
   FlagRechts = 1;
   XPositionRechts = REHit->GetPositionX();
   YPositionRechts = REHit->GetPositionY();
   ZPositionRechts = REHit->GetPositionZ();
   HitEnergyRechts = SimEvent->GetHTAt(h)->GetEnergy();
   }
   if (REHit->GetPositionX() < (ScatterMin * -1) && REHit->GetPositionX() > (ScatterMax * -1) && FlagLinks == 1 &&REHit->GetEnergy() > 0.0) {
    //cout << " Treffer LINKS " << endl;
    FlagLinks = 2;
    XPositionLinks2 = REHit->GetPositionX();
    YPositionLinks2 = REHit->GetPositionY();
    ZPositionLinks2 = REHit->GetPositionZ();
    }
   if (REHit->GetPositionX() < (ScatterMin * -1) && REHit->GetPositionX() > (ScatterMax * -1) && FlagLinks == 0 &&REHit->GetEnergy() > 0.0) {
   // cout << " Treffer LINKS " << endl;
    FlagLinks = 1;
    XPositionLinks = REHit->GetPositionX();
    YPositionLinks = REHit->GetPositionY();
    ZPositionLinks = REHit->GetPositionZ();
   }




   if (REHit->GetPositionY() > ScatterMin && REHit->GetPositionY() < ScatterMax && FlagOben == 1 && REHit->GetEnergy()> 0.0) {
    //cout << " Treffer OBEN " << endl;
    FlagOben = 2;
    XPositionOben2 = REHit->GetPositionX();
    YPositionOben2 = REHit->GetPositionY();
    ZPositionOben2 = REHit->GetPositionZ();
   }
   if (REHit->GetPositionY() > ScatterMin && REHit->GetPositionY() < ScatterMax && FlagOben == 0 && REHit->GetEnergy() > 0.0) {
   //cout << " Treffer OBEN " << endl;
    FlagOben = 1;
    XPositionOben = REHit->GetPositionX();
    YPositionOben = REHit->GetPositionY();
    ZPositionOben = REHit->GetPositionZ();
   }
   if (REHit->GetPositionY() < (ScatterMin * -1) && REHit->GetPositionY() > (ScatterMax * -1) && FlagUnten == 1 &&REHit->GetEnergy() > 0.0) {
    //cout << " Treffer UNTEN " << endl;
    FlagUnten = 2;
    XPositionUnten2 = REHit->GetPositionX();
    YPositionUnten2 = REHit->GetPositionY();
    ZPositionUnten2 = REHit->GetPositionZ();
   }
   if (REHit->GetPositionY() < (ScatterMin * -1) && REHit->GetPositionY() > (ScatterMax * -1) && FlagUnten == 0 &&REHit->GetEnergy() > 0.0) {
    //cout << " Treffer UNTEN " << endl;
    FlagUnten = 1;
    XPositionUnten = REHit->GetPositionX();
    YPositionUnten = REHit->GetPositionY();
    ZPositionUnten = REHit->GetPositionZ();
    }
//////////geometrical selection of hits due to Compton camera arrangement and threshold


   if (REHit->GetPositionX() <= (ScatterMin * -1) && REHit->GetEnergy()>0.0) {
    RELinks->AddRESE(REHit);
    LinksCounter = LinksCounter + 1;
   }
   
   if (REHit->GetPositionX() >= -3.0 && REHit->GetPositionX()<0.0 && REHit->GetEnergy()>0.0) {
   // RELinks->AddRESE(REHit);
    LinksCounterNeg = LinksCounterNeg + 1;		// i added this here
   }
   
   
   
   if (REHit->GetPositionX() >= ScatterMin && REHit->GetEnergy() > 0.0){
    RERechts->AddRESE(REHit);
    RechtsCounter = RechtsCounter + 1;
   }
   if (REHit->GetPositionY() <= (ScatterMin * -1) && REHit->GetEnergy()>0.0){
    REUnten->AddRESE(REHit);
    UntenCounter = UntenCounter + 1;
   }
   if (REHit->GetPositionY() >= ScatterMin && REHit->GetEnergy() > 0.0){
    REOben->AddRESE(REHit);
    ObenCounter = ObenCounter + 1;
    }
   if (REHit->GetPositionX() <= -AbsorberMin) {
    LinksCounterCalori = LinksCounterCalori + 1;
   }
   
   if (REHit->GetPositionX() <= -AbsorberMin && REHit->GetPositionX()>=-AbsorberMax) {  //i added this here
    LinksCounterAbsorber = LinksCounterAbsorber + 1;
   }
   
   
   
   if (REHit->GetPositionX() >= AbsorberMin) {   //I added this here
	RechtsCounterCalori = RechtsCounterCalori + 1;
   }
   
    if (REHit->GetPositionY() <= -AbsorberMin) {    //I added this here
    UntenCounterCalori = UntenCounterCalori + 1;
   }
   
    if (REHit->GetPositionY() >= AbsorberMin) {    //I added this here
    ObenCounterCalori = ObenCounterCalori + 1;
   }
   
   
    if (REHit->GetPositionX() <= (ScatterMin* -1) && REHit->GetPositionX() >= (ScatterMax*-1)) {    //I added this here
    LinksScatterer = LinksScatterer + 1;
   }

	 if (REHit->GetPositionX() <= (ScatterMin* -1) && REHit->GetPositionX() >= (ScatterMax*-1) && REHit->GetPositionY()>=-2.5 && REHit->GetPositionY()<=2.5) {    //I added this here
    LinksScattererOnly = LinksScattererOnly + 1;
   }


	if (REHit->GetPositionX() >= ScatterMin && REHit->GetPositionX() <= ScatterMax) {    //I added this here
    RechtsScatterer = RechtsScatterer + 1;
   }
   
   if (REHit->GetPositionY() >= ScatterMin && REHit->GetPositionY() <= ScatterMax) {    //I added this here
    ObenScatterer = ObenScatterer + 1;
   }
   
   if (REHit->GetPositionY() <= -ScatterMin && REHit->GetPositionY() >= -ScatterMax) {    //I added this here
    UntenScatterer = UntenScatterer + 1;
   }

   if (REHit->GetPositionY() >= ScatterMin && REHit->GetPositionY() <= ScatterMax) {
     m_HistEnergyObenDSSSD->Fill(REHit->GetEnergy());
   }
   
   
   if (REHit->GetPositionY() <= -ScatterMin && REHit->GetPositionY() >= -ScatterMax) {    //I added this here
     m_HistEnergyUntenDSSSD->Fill(REHit->GetEnergy());
   }
   
   
   if (REHit->GetPositionY() <= -AbsorberMin && REHit->GetPositionY() >= -AbsorberMax) {    //I added this here
     m_HistEnergyUntenAbsorber->Fill(REHit->GetEnergy());
   }
   
   

// cout << " SE->GetHTAt(h)->GetNHTs(): " << SimEvent->GetNHTs() << endl;
//cout << " SE->GetHTAt(h)->GetPosition(): " << SimEvent->GetHTAt(h)->GetPosition() << endl;
  RE->AddRESE(REHit);  

}   //here ends the For Loop


//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************
// E N D E
//Event Selector
//*****************************************************************************************
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



  RawEvent = 0;
 //RawEvent = RE;

 // Histogram of all 4 Compton camera modules
  m_HistEnergySumCC->Fill(REOben->GetEnergy() + REUnten->GetEnergy() + RERechts->GetEnergy() + RELinks->GetEnergy());

 // Histogram of one Compton camera modules (Oben)
  if (REOben->GetEnergy() > 0.0) m_HistEnergyObenCC->Fill(REOben->GetEnergy());
    
  Flag1275Rechts = 0;
  Flag1275Links = 0;
  Flag1275Oben = 0;
  Flag1275Unten = 0;
  // 511 keV Rechts AND Links


  if (RELinks->GetEnergy() > 480.0 && RELinks->GetEnergy() < 540.0 && RERechts->GetEnergy() > 480.0 && RERechts->GetEnergy() < 540.0)
    LOR = LOR + 1;
  if (REUnten->GetEnergy() > 480.0 && REUnten->GetEnergy() < 540.0 && REOben->GetEnergy() > 480.0 && REOben->GetEnergy() < 540.0)
    LOR = LOR + 1;
  if (RELinks != 0 && LinksCounter > 1 && FlagOben > 0 && FlagUnten > 0 && RELinks->GetEnergy() > genergycutmin &&RELinks->GetEnergy() < genergycutmax) {
   RawEvent = RELinks;
   Flag1275Links = 1;
   cout << "Treffer OBEN<->UNTEN " << endl;
   }
   if (REUnten != 0 && UntenCounter > 1 && FlagRechts > 0 && FlagLinks > 0 && REUnten->GetEnergy() > genergycutmin&& REUnten->GetEnergy() < genergycutmax) {
    RawEvent = REUnten;
    Flag1275Unten = 1;
   cout << "Treffer RECHTS<->LINKS " << endl;
   }
  if (RERechts != 0 && RechtsCounter > 1 && FlagOben > 0 && FlagUnten > 0 && RERechts->GetEnergy() > genergycutmin&& RERechts->GetEnergy() < genergycutmax) {
    RawEvent = RERechts;
    Flag1275Rechts = 1;
   cout << "Treffer OBEN<->UNTEN " << endl;
   }
  if (REOben != 0 && ObenCounter > 1 && FlagRechts > 0 && FlagLinks > 0 && REOben->GetEnergy() > genergycutmin &&REOben->GetEnergy() < genergycutmax) {
    RawEvent = REOben;
    Flag1275Oben = 1;
  cout << "Treffer RECHTS<->LINKS " << endl;
   }
  delete SimEvent;

  if (RawEvent ==0) {cout<<"No Raw Event"<<endl;}
  if (RawEvent == 0) continue;
  
  
  m_HistEnergyBefore->Fill(RawEvent->GetEnergy()); //I add here//
  
//if (whileloopcounter > 1500) break;
//if (RE->GetEnergy() < 2200.0) continue;


//////////////////////////////////////////////////
// Reconstruct
////////////////////////////////////////////////

  m_RawEventAnalyzer->AddRawEvent(RawEvent);
  ReturnCode = m_RawEventAnalyzer->AnalyzeEvent();
  if (ReturnCode == MRawEventAnalyzer::c_AnalysisSucess) {
   cout << "success"<<endl;
   BestRawEvent = 0;
   if (m_RawEventAnalyzer->GetOptimumEvent() != 0) {
     BestRawEvent = m_RawEventAnalyzer->GetOptimumEvent();
     cout << " GetOptimumEvent: " << BestRawEvent << endl;
     cout << " Event type: " << BestRawEvent->GetEventTypeAsString() <<endl;
     cout << "ToString: " << BestRawEvent->ToString()<<endl;
    } else if (m_RawEventAnalyzer->GetBestTryEvent() != 0) {
   BestRawEvent = m_RawEventAnalyzer->GetBestTryEvent();
     cout << " GetBestTryEvent: " << BestRawEvent << endl;
    }
   if (BestRawEvent != 0 ) {
	   
       MPhysicalEvent* Phys = BestRawEvent->GetPhysicalEvent();

       cout << "Physical event type: " << Phys->GetType() <<endl;
       /*
       cout << "GetPosition: " << Phys->GetPosition() <<endl;
       if (dynamic_cast<MComptonEvent*>(Phys) != 0) {
       MComptonEvent* C = dynamic_cast<MComptonEvent*>(Phys);
       cout << "
       First Compton interactions point :" << C->C1() << endl;
       }
       */

   
       Events.push_back(Phys);

/////////////////////////////////
// Image
///////////////////////////////////

       if (FlagRechts > 0 && FlagLinks > 0 && (Flag1275Oben == 1 || Flag1275Unten == 1 )) 
       {
          MBPData* Data = m_Imager->CalculateResponseSliceLine(Phys, XPositionRechts,YPositionRechts, ZPositionRechts,
          XPositionLinks,YPositionLinks, ZPositionLinks);
          RechtsLinksCounter = RechtsLinksCounter + 1;
          if (Data=0){cout<<"Data is zero"<<endl;}
          
          if (Data!=0){   //i made modifications here - added this extra 'if' here
          
          cout << "Data is not zero: event oben or unten" << endl;
          if (RERechts->GetEnergy() > 480 && RERechts->GetEnergy() < 540 && RELinks->GetEnergy() > 480 &&
             RELinks->GetEnergy() < 540) m_HistEnergyISDSSSDRechts->Fill(RERechts->GetEnergy());
             
		 }
       }
       
       
       if (FlagOben > 0 && FlagUnten > 0 && (Flag1275Rechts == 1 || Flag1275Links == 1 )) 
       {
          MBPData* Data = m_Imager->CalculateResponseSliceLine(Phys, XPositionOben,YPositionOben, ZPositionOben, XPositionUnten,
          YPositionUnten, ZPositionUnten);
          ObenUntenCounter = ObenUntenCounter + 1;
          
          if (Data=0){cout<<"Data is also zero"<<endl;}
          
          if (Data != 0) // i made modifications here too - added this extra 'if' here
          {
          cout << "Data is not zero: event rechts or links"<<endl;
          m_HistEnergyISDSSSDOben->Fill(REOben->GetEnergy());
          
	      }
       }
       
       
       
       if (Data != 0) 
       {
          ResponseSlices.push_back(Data);
        //cout<< " Data->BPDataType(): " << Data->BPDataType() << endl;


          if ((ResponseSlices.size() % 5) == 0 && ResponseSlices.size() > 1 && ResponseSlices.size() < 40000 ) 
          {
             for (unsigned int i = 0; i < Images.size(); ++i) delete Images[i];
             m_Imager->ResetStopCriterion();
             cout<<"Content: "<<ResponseSlices.size()<<endl;
             Images = m_Imager->Deconvolve(ResponseSlices);
             Images.front()->Display(CanvasBackprojection);
             Images.back()->Display(CanvasIterated);
          }
       }   //Here ends the if(Data!=0) // may be that this parenthesis is wrong, but it is probably correct!


       if ((ResponseSlices.size() % 10) == 0 && ResponseSlices.size() > 0 && ResponseSlices.size() < 4000 ) {
         cout << "IS# : " << ResponseSlices.size() << endl;
         cout << "Eff(LOR): (only true if T11): " << LOR / (Time * 120000 * 10) << endl;
         cout << "Eff(IS): " << ResponseSlices.size() / (Time * 120000 * 10) << endl;
       }

    } else {  // (if BestRawEvent=0)
  
    cout << "No good event found..." << endl;

   } //here ends the ('if BestRawEvent!=0)

  } // here ends the ('if(ReturnCode==MRawEventAnalyzer::c_AnalysisSuccess)

 }   //here ends the While Loop


/////////////////////////////////////////
// E N D E
//While Loop
///////////////////////////////////////
//if (ResponseSlices.size() > 1000) break;


 Time = Time + TimeTmp;
 SimReader->ShowProgress(true);
/////////////////////////
// Finalize:
///////////////////////////



 cout<<"Events.size(): " << Events.size() << endl;
 cout<<"IS#: " << ResponseSlices.size() << endl;
 cout<<"LOR# " << LOR << endl;
 cout<<"WhileLoop counter: " << whileloopcounter << endl;
 cout<<"Links counter: " << LinksCounter << endl;
 cout<<"Rechts counter: " << RechtsCounter << endl;
 cout<<"Oben counter: " << ObenCounter << endl;
 cout<<"Unten counter: " << UntenCounter << endl;
 cout<<"RechtsLinks counter: " << RechtsLinksCounter << endl;
 cout<<"ObenUnten counter: " << ObenUntenCounter << endl;
 cout<<"Observation time: " << Time << endl;
 cout<<"Eff(IS): " << ResponseSlices.size() / (Time * 120000 * 10) << endl;
 cout<<"Eff(LOR): (only true if T11): " << LOR / (Time * 120000 * 10) << endl;
 cout<<"RawEvent"<< RawEvent<<endl;
 cout<<"Links Calori" << LinksCounterCalori<< endl;
 cout<<"Links Absorber" << LinksCounterAbsorber <<endl;
 cout<<"Rechts Absorber" << RechtsCounterCalori << endl;
 cout<<"Unten Absorber" << UntenCounterCalori << endl;
 cout<<"Oben Absorber" << ObenCounterCalori << endl;
 cout<<"Links Scatterer" << LinksScatterer << endl;
 cout<<"Links Scatterer" << LinksScattererOnly << endl;
 cout<<"Unten Scatterer" << UntenScatterer << endl;
 cout<<"Oben Scatterer" << ObenScatterer << endl;
 cout <<"Rechts Scattere" << RechtsScatterer << endl;
 cout<<"Links Counter Neg"<< LinksCounterNeg << endl;

 TCanvas* CanvasEnergyEmi = new TCanvas();
 CanvasEnergyEmi->cd();
 m_HistEnergyEmi->Draw();
 CanvasEnergyEmi->Update();

 TCanvas* CanvasEnergyObenDSSSD = new TCanvas();
 CanvasEnergyObenDSSSD->cd();
 m_HistEnergyObenDSSSD->Draw();
 CanvasEnergyObenDSSSD->Update();
 
 TCanvas* CanvasEnergyUntenDSSSD = new TCanvas();
 CanvasEnergyUntenDSSSD->cd();
 m_HistEnergyUntenDSSSD->Draw();
 CanvasEnergyUntenDSSSD->Update();
 
 TCanvas* CanvasEnergyUntenAbsorber = new TCanvas();
 CanvasEnergyUntenAbsorber->cd();
 m_HistEnergyUntenAbsorber->Draw();
 CanvasEnergyUntenAbsorber->Update();

 TCanvas* CanvasEnergySumCC = new TCanvas();
 CanvasEnergySumCC->cd();
 m_HistEnergySumCC->Draw();
 CanvasEnergySumCC->Update();
 
 TCanvas* CanvasEnergyObenCC = new TCanvas();
 CanvasEnergyObenCC->cd();
 m_HistEnergyObenCC->Draw();
 CanvasEnergyObenCC->Update();

 TCanvas* CanvasEnergyISDSSSDRechts = new TCanvas();
 CanvasEnergyISDSSSDRechts->cd();
 m_HistEnergyISDSSSDRechts->Draw();
 CanvasEnergyISDSSSDRechts->Update();

 TCanvas* CanvasEnergyISDSSSDOben = new TCanvas();
 CanvasEnergyISDSSSDOben->cd();
 m_HistEnergyISDSSSDOben->Draw();
 CanvasEnergyISDSSSDOben->Update();

 TCanvas* CanvasEnergyBeforeLinks = new TCanvas();
 CanvasEnergyBeforeLinks->cd();
 m_HistEnergyBeforeLinks->Draw();
 CanvasEnergyBeforeLinks->Update();
 
 TCanvas* CanvasEnergyBefore = new TCanvas();
 CanvasEnergyBefore->cd();
 m_HistEnergyBefore->Draw();
 CanvasEnergyBefore->Update();
 
 

 for (unsigned int i = 0; i < Images.size(); ++i) delete Images[i];
  m_Imager->ResetStopCriterion();
  Images = m_Imager->Deconvolve(ResponseSlices);
  Images.front()->Display(CanvasBackprojection);
  Images.back()->Display(CanvasIterated);





return true;
}  // here ends the bool Analyze function


/******************************************************************************/

ExternalAnalysisPipelineExample* g_Prg = 0;
int g_NInterruptCatches = 1;

/******************************************************************************/


/******************************************************************************
* Called when an interrupt signal is flagged
* All catched signals lead to a well defined exit of the program
*/

void CatchSignal(int a)
{
if (g_Prg != 0 && g_NInterruptCatches-- > 0) {
cout<<"Catched signal Ctrl-C (ID="<<a<<"):"<<endl;
g_Prg->Interrupt();
} else {
abort();
}
}


/******************************************************************************
* Main program
*/
int main(int argc, char** argv)
{
//void (*handler)(int);
//handler = CatchSignal;
//(void) signal(SIGINT, CatchSignal);
// Initialize global MEGALIB variables, especially mgui, etc.
MGlobal::Initialize();

TApplication ExternalAnalysisPipelineExampleApp("ExternalAnalysisPipelineExampleApp", 0, 0);

g_Prg = new ExternalAnalysisPipelineExample();

if (g_Prg->ParseCommandLine(argc, argv) == false) {
cerr<<"Error during parsing of command line!"<<endl;
return -1;
}

if (g_Prg->Analyze() == false) {
cerr<<"Error during analysis!"<<endl;
return -2;
}

ExternalAnalysisPipelineExampleApp.Run();
cout<<"Program exited normally!"<<endl;
return 0;
} // end of main program





