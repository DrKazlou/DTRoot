#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <../include/CAENDigitizerType.h>
#include "CAENDigitizer.h"
//#include "DTconfig.h"

#include "TH1D.h"
#include "TH2D.h"
#include "TCanvas.h"
#include "TGLabel.h"
#include "TGStatusBar.h"
#include "TGWindow.h"
#include "TGFrame.h"

#include "TFile.h"
#include "TTree.h"



#define MAX_CH 2  /* max. number of channels */
#define MAX_SET 2           /* max. number of independent settings */
#define MAX_GROUPS  8          /* max. number of groups */

#define MAX_GW  1000        /* max. number of generic write commads */

#define VME_INTERRUPT_LEVEL      1
#define VME_INTERRUPT_STATUS_ID  0xAAAA
#define INTERRUPT_TIMEOUT        200  // ms

#define CFGRELOAD_CORRTABLES_BIT (0)
#define CFGRELOAD_DESMODE_BIT (1)

#define NPOINTS 2
#define NACQS   50

#define b_width 4 // 4 ns bin width
#define DEFAULT_CONFIG_FILE  "Config.txt"  /* local directory */


typedef enum {
	OFF_BINARY=	0x00000001,			// Bit 0: 1 = BINARY, 0 =ASCII
	OFF_HEADER= 0x00000002,			// Bit 1: 1 = include header, 0 = just samples data
} OUTFILE_FLAGS;

typedef struct{
	float cal[MAX_SET];
	float offset[MAX_SET];
}DAC_Calibration_data;

typedef struct {
    //int LinkType;
	CAEN_DGTZ_ConnectionType LinkType;
    int LinkNum;
    int ConetNode;
    uint32_t BaseAddress;
	int windx, windy;
	
    int Nch;
    int Nbit;
    float Ts;
    int NumEvents;
    uint32_t RecordLength;
    int PostTrigger;
    int InterruptNumEvents;
    int TestPattern;
    CAEN_DGTZ_EnaDis_t DesMode;
    //int TriggerEdge;
    CAEN_DGTZ_IOLevel_t FPIOtype;
    CAEN_DGTZ_TriggerMode_t ExtTriggerMode;
    uint16_t EnableMask;
    char GnuPlotPath[1000];
    CAEN_DGTZ_TriggerMode_t ChannelTriggerMode[MAX_SET];
	CAEN_DGTZ_TriggerPolarity_t PulsePolarity[MAX_SET];
	//CAEN_DGTZ_PulsePolarity_t PulsePolarity[MAX_SET];
    uint32_t DCoffset[MAX_SET];
    int32_t  DCoffsetGrpCh[MAX_SET][MAX_SET];
    uint32_t Threshold[MAX_SET];
	uint32_t cal_thr[MAX_SET]; // calibrated threshold
	uint32_t thr[MAX_SET]; // threshold from gui
	int Version_used[MAX_SET];
	uint8_t GroupTrgEnableMask[MAX_SET];
    uint32_t MaxGroupNumber;
	
	uint32_t FTDCoffset[MAX_SET];
	uint32_t FTThreshold[MAX_SET];
	CAEN_DGTZ_TriggerMode_t	FastTriggerMode;
	//uint32_t	 FastTriggerEnabled;
	CAEN_DGTZ_EnaDis_t	 FastTriggerEnabled;
    int GWn;
    uint32_t GWaddr[MAX_GW];
    uint32_t GWdata[MAX_GW];
	uint32_t GWmask[MAX_GW];
	OUTFILE_FLAGS OutFileFlags;
	uint16_t DecimationFactor;
    int useCorrections;
    int UseManualTables;
    char TablesFilenames[MAX_X742_GROUP_SIZE][1000];
    CAEN_DGTZ_DRS4Frequency_t DRS4Frequency;
    int StartupCalibration;
	DAC_Calibration_data DAC_Calib;
    char ipAddress[25];
} WaveDumpConfig_t;


typedef struct
{	
	bool fPrint;
	bool fTimer;
	int timer_val;
	bool fStoreTraces;
	
	int loop;
	uint32_t Nb;
	int Nev;
	//int TrgCnt[MAX_CH];
	uint64_t StartTime;
	double DrawTime;
	
	TGLabel  *TLabel;
	TGStatusBar *StatusBar;
	TCanvas *can;
	//const TGWindow *main;
	TGMainFrame *main;
	TFile *ff; // to store traces
	TTree *tree;
	
} ReadoutConfig_t;

typedef struct{
	
	int xbins, xmin, xmax;
	int ybins, ymin, ymax;
	
}hist_settings_t;

typedef struct
{	
	int WF_XMIN, WF_XMAX, WF_YMIN, WF_YMAX;
	int ALBound, ARBound, ILBound, IRBound;
		
	
	char h2Style[10];
	bool fDraw[MAX_CH]; // channel draw flag
	int NPad;	
	
	int cAmpl, cInt, cdT, cPSD_ampl, cPSD_int, cQsl, cIA;		
	bool fBL, fTrace, fAmpl, fInt, fdT, fPSD_ampl, fPSD_int, fQsl, fIA;  // flags for every time of histograms
	
	int CH_2D; // channel to draw th2d
	
	TH1D *dt;	
	TH1D *trace[MAX_CH], *ampl[MAX_CH], *integral[MAX_CH];
	TH2D *int_ampl, *psd_ampl, *psd_int, *qs_ql; 
		
	hist_settings_t ampl_set, integral_set, dt_set, psd_ampl_set;
	
	int BL_CUT, PSD_BIN;
	
	int evt, evt_out, ch_out;
	uint32_t ext_out, tst_out;
	uint64_t time_out;
	std::vector <double> vec_bl[2], vec[2];
	std::vector<std::vector<double>> v_out;
	int EC, TTT;
		
} Histograms_t;


CAEN_DGTZ_ErrorCode GetMoreBoardInfo(int handle, CAEN_DGTZ_BoardInfo_t BoardInfo, WaveDumpConfig_t *WDcfg);

CAEN_DGTZ_ErrorCode Set_calibrated_DCO(int handle, int ch, WaveDumpConfig_t *WDcfg, CAEN_DGTZ_BoardInfo_t BoardInfo);

 CAEN_DGTZ_ErrorCode CheckBoardFailureStatus(int handle);
 
 CAEN_DGTZ_ErrorCode QuitMain(int handle, char* buffer);
 
 void calibrate(int handle,  CAEN_DGTZ_BoardInfo_t BoardInfo);
 


/* ###########################################################################
*  Functions
*  ########################################################################### */

int ParseConfigFile(FILE *f_ini, WaveDumpConfig_t *WDcfg, Histograms_t *Histo);

CAEN_DGTZ_ErrorCode WriteRegisterBitmask(int32_t handle, uint32_t address, uint32_t data, uint32_t mask);

CAEN_DGTZ_ErrorCode  DoProgramDigitizer(int handle, WaveDumpConfig_t* WDcfg, CAEN_DGTZ_BoardInfo_t BoardInfo);

CAEN_DGTZ_ErrorCode ProgramDigitizerWithRelativeThreshold(int handle, WaveDumpConfig_t* WDcfg, CAEN_DGTZ_BoardInfo_t BoardInfo);


CAEN_DGTZ_ErrorCode ProgramDigitizer(int handle, WaveDumpConfig_t WDcfg, CAEN_DGTZ_BoardInfo_t BoardInfo);


CAEN_DGTZ_ErrorCode InitialiseDigitizer(int handle, WaveDumpConfig_t &WDcfg, ReadoutConfig_t Rcfg);