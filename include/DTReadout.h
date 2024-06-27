#include "TH1D.h"
#include "TCanvas.h"
#include "TColor.h"

#include <../include/CAENDigitizerType.h>
#include "CAENDigitizer.h"
//#include "DTFunc.h"

long get_time();
int DeltaT(TH1D *hist[2]);

void CalcRate(uint64_t &PrevRateTime);
void InitReadoutConfig();
void InitHisto();
void FillHisto(CAEN_DGTZ_EventInfo_t EventInfo);
void DrawHisto(TCanvas *c1);

CAEN_DGTZ_ErrorCode ReadoutLoop(int handle);
CAEN_DGTZ_ErrorCode DataAcquisition(int handle);