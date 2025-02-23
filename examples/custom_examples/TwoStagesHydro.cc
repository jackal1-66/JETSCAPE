/*******************************************************************************
 * Copyright (c) The JETSCAPE Collaboration, 2018
 *
 * Modular, task-based framework for simulating all aspects of heavy-ion collisions
 * 
 * For the list of contributors see AUTHORS.
 *
 * Report issues at https://github.com/JETSCAPE/JETSCAPE/issues
 *
 * or via email to bugs.jetscape@gmail.com
 *
 * Distributed under the GNU General Public License 3.0 (GPLv3 or later).
 * See COPYING for details.
 ******************************************************************************/
// ------------------------------------------------------------
// JetScape Framework hydro from file Test Program
// (use either shared library (need to add paths; see setup.csh)
// (or create static library and link in)
// -------------------------------------------------------------

#include <iostream>
#include <time.h>

// JetScape Framework includes ...
#include "JetScape.h"
#include "JetEnergyLoss.h"
#include "JetEnergyLossManager.h"
#include "JetScapeWriterStream.h"
#ifdef USE_HEPMC
#include "JetScapeWriterHepMC.h"
#endif

// User modules derived from jetscape framework clasess
#include "AdSCFT.h"
#include "Matter.h"
#include "LBT.h"
#include "Martini.h"
#include "Brick.h"
#include "MusicWrapper.h"
#include "CausalLiquefier.h"
#include "iSpectraSamplerWrapper.h"
#include "TrentoInitial.h"
#include "NullPreDynamics.h"
#include "PGun.h"
#include "PythiaGun.h"
#include "PartonPrinter.h"
#include "HadronizationManager.h"
#include "Hadronization.h"
#include "ColoredHadronization.h"
#include "ColorlessHadronization.h"

#include <chrono>
#include <thread>

using namespace std;

using namespace Jetscape;

// Forward declaration
void Show();

// -------------------------------------

int main(int argc, char** argv)
{
  clock_t t; t = clock();
  time_t start, end; time(&start);
  
  cout<<endl;
    
  // DEBUG=true by default and REMARK=false
  // can be also set also via XML file (at least partially)
  JetScapeLogger::Instance()->SetInfo(true);
  JetScapeLogger::Instance()->SetDebug(true);
  JetScapeLogger::Instance()->SetRemark(false);
  //SetVerboseLevel (9 a lot of additional debug output ...)
  //If you want to suppress it: use SetVerboseLevel(0) or max  SetVerboseLevel(9) or 10
  JetScapeLogger::Instance()->SetVerboseLevel(0);
   
  Show();

  auto jetscape = make_shared<JetScape>();
  const char* masterXMLName = "../config/jetscape_master.xml";
  const char* userXMLName = "../config/jetscape_user.xml";

  jetscape->SetXMLMasterFileName(masterXMLName);
  jetscape->SetXMLUserFileName(userXMLName);

  jetscape->SetNumberOfEvents(1);
  jetscape->SetReuseHydro (true);
  jetscape->SetNReuseHydro (5);
  // jetscape->SetNumberOfEvents(2);
  //jetscape->SetReuseHydro (false);
  //jetscape->SetNReuseHydro (0);

  // Initial conditions and hydro
  auto trento = make_shared<TrentoInitial>();
  auto null_predynamics = make_shared<NullPreDynamics> ();
  //auto pGun= make_shared<PGun> ();
  auto pythiaGun= make_shared<PythiaGun> ();
  auto hydro1 = make_shared<MpiMusic> ();
  auto myliquefier = make_shared<CausalLiquefier> ();
  hydro1->SetId("MUSIC_1");
  //hydro1->add_a_liqueifier(myliquefier);

  jetscape->Add(trento);
  jetscape->Add(null_predynamics);
  jetscape->Add(pythiaGun);
  //jetscape->Add(pGun);

  // add the first hydro
  jetscape->Add(hydro1);

  // Energy loss
  auto jlossmanager = make_shared<JetEnergyLossManager> ();
  auto jloss = make_shared<JetEnergyLoss> ();
  jloss->add_a_liquefier(myliquefier);


  auto matter = make_shared<Matter> ();
  auto lbt = make_shared<LBT> ();
  //auto martini = make_shared<Martini> ();
  // auto adscft = make_shared<AdSCFT> ();

  // Note: if you use Matter, it MUST come first (to set virtuality)
  jloss->Add(matter);
  jloss->Add(lbt);  // go to 3rd party and ./get_lbtTab before adding this module
  //jloss->Add(martini);
  // jloss->Add(adscft);  
  jlossmanager->Add(jloss);  
  jetscape->Add(jlossmanager);
  

  // add the second hydro
  auto hydro2 = make_shared<MpiMusic> ();
  hydro2->add_a_liquefier(myliquefier);
  hydro2->SetId("MUSIC_2");
  jetscape->Add(hydro2);

  // surface sampler
  auto iSS = make_shared<iSpectraSamplerWrapper> ();
  jetscape->Add(iSS);

  // Hadronization
  // This helper module currently needs to be added for hadronization.
  auto printer = make_shared<PartonPrinter> ();
  jetscape->Add(printer);
  auto hadroMgr = make_shared<HadronizationManager> ();
  auto hadro = make_shared<Hadronization> ();
  //auto hadroModule = make_shared<ColoredHadronization> ();
  //hadro->Add(hadroModule);
  auto colorless = make_shared<ColorlessHadronization> ();
  hadro->Add(colorless);
  hadroMgr->Add(hadro);
  jetscape->Add(hadroMgr);

  // Output
  auto writer= make_shared<JetScapeWriterAscii> ("test_out.dat");
  // same as JetScapeWriterAscii but gzipped
  // auto writer= make_shared<JetScapeWriterAsciiGZ> ("test_out.dat.gz");
  // HEPMC3
#ifdef USE_HEPMC
  // auto writer= make_shared<JetScapeWriterHepMC> ("test_out.hepmc");
#endif
  jetscape->Add(writer);

  // Intialize all modules tasks
  jetscape->Init();

  // Run JetScape with all task/modules as specified ...
  jetscape->Exec();

  // "dummy" so far ...
  // Most thinkgs done in write and clear ...
  jetscape->Finish();
  
  INFO_NICE<<"Finished!";
  cout<<endl;

  // wait for 5s
  //std::this_thread::sleep_for(std::chrono::milliseconds(500000));

  t = clock() - t;
  time(&end);
  printf ("CPU time: %f seconds.\n",((float)t)/CLOCKS_PER_SEC);
  printf ("Real time: %f seconds.\n",difftime(end,start));
  //printf ("Real time: %f seconds.\n",(start-end));
  return 0;
}

// -------------------------------------

void Show()
{
  INFO_NICE<<"-----------------------------------------------";
  INFO_NICE<<"| MUSIC Test JetScape Framework ... |";
  INFO_NICE<<"-----------------------------------------------";
  INFO_NICE;
}
