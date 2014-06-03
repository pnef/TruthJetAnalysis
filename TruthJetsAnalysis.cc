#include <math.h>
#include <vector>
#include <string>
#include <sstream>
#include <set>


#include "TFile.h"
#include "TTree.h"
#include "TClonesArray.h"
#include "TParticle.h"
#include "TDatabasePDG.h"
#include "TMath.h"
#include "TVector3.h"

#include "TruthJetsAnalysis.h"
#include "TruthJetsTools.h"

#include "myFastJetBase.h"
#include "fastjet/ClusterSequence.hh"
#include "fastjet/PseudoJet.hh"  
#include "fastjet/tools/Filter.hh"
#include "fastjet/Selector.hh"
#include "fastjet/tools/Subtractor.hh"
#include "fastjet/ClusterSequenceArea.hh"
#include "fastjet/tools/JetMedianBackgroundEstimator.hh"

#include "Pythia8/Pythia.h"

using namespace std;


// Constructor 
TruthJetsAnalysis::TruthJetsAnalysis(){
    if(fDebug) cout << "TruthJetsAnalysis::TruthJetsAnalysis Start " << endl;
    ftest = 0;
    fDebug = false;
    fOutName = "test.root";
    tool = new TruthJetsTools();

    if(fDebug) cout << "TruthJetsAnalysis::TruthJetsAnalysis End " << endl;
}

// Destructor 
TruthJetsAnalysis::~TruthJetsAnalysis(){
    delete tool;
}

// Begin method
void TruthJetsAnalysis::Begin(){
   // Declare TTree
   tF = new TFile(fOutName.c_str(), "RECREATE");
   tT = new TTree("EventTree", "Event Tree for TruthJets");
    
   DeclareBranches();
   ResetBranches();
   

   return;
}

// End
void TruthJetsAnalysis::End(){
    
    tT->Write();
    tF->Close();
    return;
}

// GetParticles from pythia
bool TruthJetsAnalysis::GetPythiaParticles(Pythia8::Pythia* pythia8, event_type etype, int nPU){

    int n_runs = 1;
    if(etype == pileup){ n_runs = nPU;}

    for(int irun=0; irun<n_runs; ++irun){
        // next event 
        if (!pythia8->next()) return false;

        for (unsigned int ip=0; ip<pythia8->event.size(); ++ip){

            int origin = -1;
            if      (etype == hardscatter) origin = 0;
            else if (etype == pileup     ) origin = irun +1;

            fastjet::PseudoJet p(pythia8->event[ip].px(), pythia8->event[ip].py(), pythia8->event[ip].pz(),pythia8->event[ip].e() );
            p.reset_PtYPhiM(p.pt(), p.rapidity(), p.phi(), 0.); // massless particles 

            // tracks: charged particles with pt>0.5 GeV, |eta|<2.4 
            if(pythia8->event[ip].isFinal()    && 
               fabs(pythia8->event[ip].id())  !=11  && 
               fabs(pythia8->event[ip].id())  !=12  && 
               fabs(pythia8->event[ip].id())  !=13  && 
               fabs(pythia8->event[ip].id())  !=14  && 
               fabs(pythia8->event[ip].id())  !=16  && 
               pythia8->event[ip].isCharged()  && 
               pythia8->event[ip].pT() > 0.5   && 
               fabs(pythia8->event[ip].eta())<2.4){

                if      (origin == 0 ) {HStracks.push_back(p); p.set_user_info(new GhostUserInfo(false, false, true));}
                else if (origin >= 1 ) {PUtracks.push_back(p); p.set_user_info(new GhostUserInfo(false, true,  true));}

            }

            if(pythia8->event[ip].isFinal()         && 
               fabs(pythia8->event[ip].id())  !=11  && 
               fabs(pythia8->event[ip].id())  !=12  && 
               fabs(pythia8->event[ip].id())  !=13  && 
               fabs(pythia8->event[ip].id())  !=14  && 
               fabs(pythia8->event[ip].id())  !=16  && 
               pythia8->event[ip].pT()        > 0.5  ) {

               particlesForJets.push_back(p);

               if(origin == 0){
               particlesForTruthJets.push_back(p);
               }

            }

        }
    } // end irun loop

    return true;
}

// Analyze
void TruthJetsAnalysis::AnalyzeEvent(int ievt, Pythia8::Pythia* pythia8, Pythia8::Pythia* pythia8PU, int nPU){
    if(fDebug) cout << "TruthJetsAnalysis::AnalyzeEvent Begin " << endl;

    // -------------------------
    if(fDebug) cout << "TruthJetsAnalysis::AnalyzeEvent Event Number " << ievt << endl;
    
    // reset branches 
    ResetBranches();
    

    // new event-----------------------
    fTEventNumber = ievt;
    fTNPV         = nPU;
    particlesForJets.clear();
    particlesForTruthJets.clear();
    PUtracks.clear();
    HStracks.clear();
    


    // get new event and return if failed
    bool ok(true);
    ok = GetPythiaParticles(pythia8,   hardscatter);
    ok = GetPythiaParticles(pythia8PU, pileup,     nPU);
    if (!ok) return;
    
    /// ------------------ Caluculate Rho ------------------------------
    fastjet::JetMedianBackgroundEstimator bge(fastjet::SelectorAbsRapMax(1.5), fastjet::JetDefinition(fastjet::kt_algorithm,0.4), fastjet::AreaDefinition(fastjet::active_area_explicit_ghosts));
    bge.set_particles(particlesForJets);
    fastjet::Subtractor theSubtractor(&bge);


    // small R jets: ATLAS Style ------------------------------------------
    fastjet::AreaDefinition        area_def(fastjet::active_area_explicit_ghosts, fastjet::GhostedAreaSpec(5.));
    fastjet::ClusterSequenceArea   cs(particlesForJets, fastjet::JetDefinition(fastjet::antikt_algorithm, 0.4), area_def);

    fastjet::Selector sel_jets_small    = fastjet::SelectorPtMin(20) * fastjet::SelectorAbsRapMax(2.5);

    vector<fastjet::PseudoJet> myJets_beforeSubtraction = fastjet::sorted_by_pt(cs.inclusive_jets()); //was 10
    vector<fastjet::PseudoJet> myJets                   = sel_jets_small(theSubtractor(myJets_beforeSubtraction));

    if(fDebug){ cout << ">>>>> Small R jets <<<<< " << endl;}
    for (unsigned int ij = 0; ij < myJets.size(); ij++){
        if(fTNJetsSmallRFilled == MaxNJetSmallR) {if(fDebug) cout << "Warning: More than " << MaxNJetSmallR << " small R jets" << endl; continue;}
        float truthmatchpt = -1;

        fTJsmallPt          [fTNJetsSmallRFilled] = myJets[ij].pt();
        fTJsmallEta         [fTNJetsSmallRFilled] = myJets[ij].eta();
        fTJsmallPhi         [fTNJetsSmallRFilled] = myJets[ij].phi();
        fTJsmallM           [fTNJetsSmallRFilled] = myJets[ij].m();

        fTNJetsSmallRFilled++;
    }


    // Fill
    tT->Fill();

    if(fDebug) cout << "TruthJetsAnalysis::AnalyzeEvent End " << endl;
    return;
}



// declate branches
void TruthJetsAnalysis::DeclareBranches(){
   
   // Event Properties 
   tT->Branch("EventNumber",               &fTEventNumber,            "EventNumber/I");
   tT->Branch("NPV",                       &fTNPV,                    "NPV/I");

   // smallR jets
   tT->Branch("NJetsFilledSmallR",         &fTNJetsSmallRFilled,       "NJetsFilledSmallR/I");
   tT->Branch("JsmallPt",                  &fTJsmallPt,                "JsmallPt[NJetsFilledSmallR]/F");
   tT->Branch("JsmallEta",                 &fTJsmallEta,               "JsmallEta[NJetsFilledSmallR]/F");
   tT->Branch("JsmallPhi",                 &fTJsmallPhi,               "JsmallPhi[NJetsFilledSmallR]/F");
   tT->Branch("JsmallM",                   &fTJsmallM,                 "JsmallM[NJetsFilledSmallR]/F");
   
   tT->GetListOfBranches()->ls();
    
   return;
}


// resets vars
void TruthJetsAnalysis::ResetBranches(){
      // reset branches 
      fTEventNumber                 = -999;
      fTNPV                         = -999;
      fTNJetsSmallRFilled           = 0;

      for (int iP=0; iP < MaxNJetSmallR; ++iP){
          fTJsmallPt       [iP]= -999;
          fTJsmallPhi      [iP]= -999;
          fTJsmallEta      [iP]= -999;
          fTJsmallM        [iP]= -999;
      }

}

