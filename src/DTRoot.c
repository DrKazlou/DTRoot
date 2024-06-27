

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include "CAENDigitizer.h"

#include "DTFrame.h"
#include "DTFunc.h"
#include "DTReadout.h"
#include "DTRoot.h"
#include "DTOpt.h"

#include "TROOT.h"
#include "TApplication.h"
#include "TCanvas.h"
#include "TH1D.h"
#include "TFile.h"
#include "TTree.h"
#include "TBranch.h"
#include "TROOT.h"
#include "TStyle.h"

#define CAEN_USE_DIGITIZERS
#define IGNORE_DPP_DEPRECATED


using namespace std;

	
	
	WaveDumpConfig_t WDcfg;
	Histograms_t Histo;
	ReadoutConfig_t Rcfg;    	
	int handle;	
	  
	CAEN_DGTZ_ErrorCode ret;

	CAEN_DGTZ_UINT16_EVENT_t *Event16 = NULL;




//---- Main program ------------------------------------------------------------

int main(int argc, char **argv)
{
	
   TApplication theApp("App", &argc, argv);

   if (gROOT->IsBatch()) {
      fprintf(stderr, "%s: cannot run in batch mode\n", argv[0]);
      return 1;
   }
   
	memset(&Histo, 0, sizeof(Histo));	
	memset(&WDcfg, 0, sizeof(WDcfg));
	memset(&Rcfg, 0, sizeof(Rcfg));
	
	
	//Configuration file routine
	FILE *f_ini = fopen(DEFAULT_CONFIG_FILE, "r");
	if (f_ini == NULL) {
		printf("Config file not found!\n");
		exit(0);
	}
	ParseConfigFile(f_ini, &WDcfg, &Histo);
	fclose(f_ini);
	
	printf("Window_size %i x %i \n",WDcfg.windx, WDcfg.windy);
	printf("Config's abtained successful TraceLength %i  Polarity %i \n",WDcfg.RecordLength, WDcfg.PulsePolarity[0]);
	//Configuration file routine
	
	handle = 0;
	InitReadoutConfig();
	
	
	new MainFrame(gClient->GetRoot(), WDcfg.windx, WDcfg.windy);
	
	ret = DataAcquisition(handle);
	
	theApp.Run();

   return 0;
}
//#endif
