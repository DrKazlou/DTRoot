#include <stdio.h>
#include <stdlib.h>
#include <../include/CAENDigitizerType.h>
#include "CAENDigitizer.h"
//#include "DTconfig.h"

#include "DTFunc.h"

#include "TH1D.h"
#include "TROOT.h"
#include "TGMsgBox.h"


#define CAEN_USE_DIGITIZERS
#define IGNORE_DPP_DEPRECATED

extern CAEN_DGTZ_UINT16_EVENT_t *Event16;
extern char * buffer;
//extern Histograms_t Histo;

static CAEN_DGTZ_IRQMode_t INTERRUPT_MODE = CAEN_DGTZ_IRQ_MODE_ROAK;

static double linear_interp(double x0, double y0, double x1, double y1, double x) {
    if (x1 - x0 == 0) {
        fprintf(stderr, "Cannot interpolate values with same x.\n");
        return HUGE_VAL;
    }
    else {
        const double m = (y1 - y0) / (x1 - x0);
        const double q = y1 - m * x1;
        return m * x + q;
    }
}

//part from DTconfig.c

int dc_file[MAX_CH];
float dc_8file[8];
int thr_file[MAX_CH] = { 0 };
/*! \fn      void SetDefaultConfiguration(WaveDumpConfig_t *WDcfg) 
*   \brief   Fill the Configuration Structure with Default Values
*            
*   \param   WDcfg:   Pointer to the WaveDumpConfig data structure
*/
static void SetDefaultConfiguration(WaveDumpConfig_t *WDcfg) {
    int i, j;

    WDcfg->RecordLength = (1024*16);
	WDcfg->PostTrigger = 50;
	WDcfg->NumEvents = 1; // Max. number of events for a block transfer (0 to 1023)
	WDcfg->EnableMask = 0xFFFF;
	WDcfg->GWn = 0;
    WDcfg->ExtTriggerMode = CAEN_DGTZ_TRGMODE_ACQ_ONLY;
    WDcfg->InterruptNumEvents = 0;
    WDcfg->TestPattern = 0;
	WDcfg->DecimationFactor = 1;
    //WDcfg->DesMode = 0;
	WDcfg->DesMode = CAEN_DGTZ_DISABLE;
	//WDcfg->FastTriggerMode = 0; 
	WDcfg->FastTriggerMode = CAEN_DGTZ_TRGMODE_DISABLED; 
    //WDcfg->FastTriggerEnabled = 0; 
	WDcfg->FastTriggerEnabled = CAEN_DGTZ_DISABLE; 
	//WDcfg->FPIOtype = 0;
	WDcfg->FPIOtype = CAEN_DGTZ_IOLevel_NIM;
	
	WDcfg->Nbit = 12; // DT5720
	WDcfg->Nch = 2; // DT5720

//	strcpy(WDcfg->GnuPlotPath, GNUPLOT_DEFAULT_PATH);
	for(i=0; i<MAX_SET; i++) {
		WDcfg->PulsePolarity[i] = CAEN_DGTZ_TriggerOnRisingEdge;  //CAEN_DGTZ_PulsePolarityPositive; 
		WDcfg->Version_used[i] = 0;
		WDcfg->DCoffset[i] = 0;
		WDcfg->Threshold[i] = 0;
        WDcfg->ChannelTriggerMode[i] = CAEN_DGTZ_TRGMODE_DISABLED;
		WDcfg->GroupTrgEnableMask[i] = 0;
		for(j=0; j<MAX_SET; j++) WDcfg->DCoffsetGrpCh[i][j] = -1;
		WDcfg->FTThreshold[i] = 0;
		WDcfg->FTDCoffset[i] =0;
    }
    WDcfg->useCorrections = -1;
    WDcfg->UseManualTables = -1;
    for(i=0; i<MAX_X742_GROUP_SIZE; i++)
        sprintf(WDcfg->TablesFilenames[i], "Tables_gr%d", i);
    WDcfg->DRS4Frequency = CAEN_DGTZ_DRS4_5GHz;
    WDcfg->StartupCalibration = 1;
}

/*! \fn      int ParseConfigFile(FILE *f_ini, WaveDumpConfig_t *WDcfg) 
*   \brief   Read the configuration file and set the WaveDump paremeters
*            
*   \param   f_ini        Pointer to the config file
*   \param   WDcfg:   Pointer to the WaveDumpConfig data structure
*   \return  0 = Success; negative numbers are error codes; positive numbers
*               decodes the changes which need to perform internal parameters
*               recalculations.
*/
int ParseConfigFile(FILE *f_ini, WaveDumpConfig_t *WDcfg, Histograms_t *Histo) 
{
	char str[1000], str1[1000], *pread = NULL;
	int i, ch=-1, val, Off=0, tr = -1;
    int ret = 0;

    // Save previous values (for compare)
    int PrevDesMode = WDcfg->DesMode;
    int PrevUseCorrections = WDcfg->useCorrections;
	int PrevUseManualTables = WDcfg->UseManualTables;
    size_t TabBuf[sizeof(WDcfg->TablesFilenames)];
    // Copy the filenames to watch for changes
    memcpy(TabBuf, WDcfg->TablesFilenames, sizeof(WDcfg->TablesFilenames));      

	/* Default settings */
	SetDefaultConfiguration(WDcfg);

	/* read config file and assign parameters */
	while(!feof(f_ini)) {
		int read;
        char *res = NULL;
        // read a word from the file
        read = fscanf(f_ini, "%s", str);
        if( !read || (read == EOF) || !strlen(str))
			continue;
        // skip comments
        if(str[0] == '#') {
            res = fgets(str, 1000, f_ini);
			continue;
        }
  		
		// Program window size
		if (strstr(str, "WINDOW_SIZE")!=NULL) {
			read = fscanf(f_ini, "%d", &WDcfg->windx);
			read = fscanf(f_ini, "%d", &WDcfg->windy);
			continue;
		}
	
		// Histograms settings 
		if (strstr(str, "HISTO_WF_Y")!=NULL) {
			read = fscanf(f_ini, "%d", &Histo->WF_YMIN);
			read = fscanf(f_ini, "%d", &Histo->WF_YMAX);
			continue;
		}
		
		if (strstr(str, "HISTO_AMPL_SET")!=NULL) {
			read = fscanf(f_ini, "%d", &Histo->ampl_set.xbins);
			read = fscanf(f_ini, "%d", &Histo->ampl_set.xmin);
			read = fscanf(f_ini, "%d", &Histo->ampl_set.xmax);
			continue;
		}
		
		if (strstr(str, "HISTO_INTEGRAL_SET")!=NULL) {
			read = fscanf(f_ini, "%d", &Histo->integral_set.xbins);
			read = fscanf(f_ini, "%d", &Histo->integral_set.xmin);
			read = fscanf(f_ini, "%d", &Histo->integral_set.xmax);
			continue;
		}
		
		if (strstr(str, "HISTO_DT_SET")!=NULL) {
			read = fscanf(f_ini, "%d", &Histo->dt_set.xbins);
			read = fscanf(f_ini, "%d", &Histo->dt_set.xmin);
			read = fscanf(f_ini, "%d", &Histo->dt_set.xmax);
			continue;
		}
		
		if (strstr(str, "HISTO_PSD_AMPL_SET")!=NULL) {
			read = fscanf(f_ini, "%d", &Histo->psd_ampl_set.xbins);
			read = fscanf(f_ini, "%d", &Histo->psd_ampl_set.xmin);
			read = fscanf(f_ini, "%d", &Histo->psd_ampl_set.xmax);
			read = fscanf(f_ini, "%d", &Histo->psd_ampl_set.ybins);
			read = fscanf(f_ini, "%d", &Histo->psd_ampl_set.ymin);
			read = fscanf(f_ini, "%d", &Histo->psd_ampl_set.ymax);
			continue;
		}
		
		
		// Histograms settings 
		
        // Section (COMMON or individual channel)
		if (str[0] == '[') {
            if (strstr(str, "COMMON")) {
                ch = -1;
               continue; 
            }
            if (strstr(str, "TR")) {
				sscanf(str+1, "TR%d", &val);
				 if (val < 0 || val >= MAX_SET) {
                    printf("%s: Invalid channel number\n", str);
                } else {
                    tr = val;
                }
            } else {
                sscanf(str+1, "%d", &val);
                if (val < 0 || val >= MAX_SET) {
                    printf("%s: Invalid channel number\n", str);
                } else {
                    ch = val;
                }
            }
            continue;
		}
 
        // OPEN: read the details of physical path to the digitizer
		if (strstr(str, "OPEN") != NULL) {
			int ip = 0;
			read = fscanf(f_ini, "%s", str1);
			if (strcmp(str1, "USB") == 0)
				WDcfg->LinkType = CAEN_DGTZ_USB;
			else
				if (strcmp(str1, "PCI") == 0)
					WDcfg->LinkType = CAEN_DGTZ_OpticalLink;
				else if (strcmp(str1, "USB_A4818") == 0)
					WDcfg->LinkType = CAEN_DGTZ_USB_A4818;
				else if (strcmp(str1, "USB_A4818_V2718") == 0)
					WDcfg->LinkType = CAEN_DGTZ_USB_A4818_V2718;
				else if (strcmp(str1, "USB_A4818_V3718") == 0)
					WDcfg->LinkType = CAEN_DGTZ_USB_A4818_V3718;
				else if (strcmp(str1, "USB_A4818_V4718") == 0)
					WDcfg->LinkType = CAEN_DGTZ_USB_A4818_V4718;
				else if (strcmp(str1, "USB_V4718") == 0)
					WDcfg->LinkType = CAEN_DGTZ_USB_V4718;
				else if (strcmp(str1, "ETH_V4718") == 0) {
					WDcfg->LinkType = CAEN_DGTZ_ETH_V4718;
					ip = 1;
				}
				else {
					printf("%s %s: Invalid connection type\n", str, str1);
					return -1;
				}
			if (ip == 1) {
				read = fscanf(f_ini, "%s", (char*)&WDcfg->ipAddress);
			}
			else
				read = fscanf(f_ini, "%d", &WDcfg->LinkNum);
			if (WDcfg->LinkType == CAEN_DGTZ_USB)
				WDcfg->ConetNode = 0;
			else
				read = fscanf(f_ini, "%d", &WDcfg->ConetNode);
			read = fscanf(f_ini, "%x", &WDcfg->BaseAddress);
			continue;
		}


        // Acquisition Record Length (number of samples)
		if (strstr(str, "RECORD_LENGTH")!=NULL) {
			read = fscanf(f_ini, "%d", &WDcfg->RecordLength);
			continue;
		}


        // External Trigger (DISABLED, ACQUISITION_ONLY, ACQUISITION_AND_TRGOUT)
		if (strstr(str, "EXTERNAL_TRIGGER")!=NULL) {
			read = fscanf(f_ini, "%s", str1);
			if (strcmp(str1, "DISABLED")==0)
                WDcfg->ExtTriggerMode = CAEN_DGTZ_TRGMODE_DISABLED;
			else if (strcmp(str1, "ACQUISITION_ONLY")==0)
                WDcfg->ExtTriggerMode = CAEN_DGTZ_TRGMODE_ACQ_ONLY;
			else if (strcmp(str1, "ACQUISITION_AND_TRGOUT")==0)
                WDcfg->ExtTriggerMode = CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT;
            else
                printf("%s: Invalid Parameter\n", str);
            continue;
		}

        // Max. number of events for a block transfer (0 to 1023)
		if (strstr(str, "MAX_NUM_EVENTS_BLT")!=NULL) {
			read = fscanf(f_ini, "%d", &WDcfg->NumEvents);
			continue;
		}

		// Post Trigger (percent of the acquisition window)
		if (strstr(str, "POST_TRIGGER")!=NULL) {
			read = fscanf(f_ini, "%d", &WDcfg->PostTrigger);
			continue;
		}

	
	 ///Input polarity	
		if (strstr(str, "PULSE_POLARITY")!=NULL) {
			//CAEN_DGTZ_PulsePolarity_t pp= CAEN_DGTZ_PulsePolarityPositive;
			CAEN_DGTZ_TriggerPolarity_t pp  = CAEN_DGTZ_TriggerOnRisingEdge;
			read = fscanf(f_ini, "%s", str1);
			if (strcmp(str1, "POSITIVE") == 0)
				//pp = CAEN_DGTZ_PulsePolarityPositive;
				pp = CAEN_DGTZ_TriggerOnRisingEdge;
			else if (strcmp(str1, "NEGATIVE") == 0) 
				//pp = CAEN_DGTZ_PulsePolarityNegative;
				pp = CAEN_DGTZ_TriggerOnFallingEdge;
			else
				printf("%s: Invalid Parameter\n", str);

				for (i = 0; i<MAX_SET; i++)///polarity setting (old trigger edge)is the same for all channels
					WDcfg->PulsePolarity[i] = pp;				
			
			continue;
		}
 
		//DC offset (percent of the dynamic range, -50 to 50)
		if (!strcmp(str, "DC_OFFSET")) 
		{

			int dc;
			read = fscanf(f_ini, "%d", &dc);
			if (tr != -1) {
				// 				WDcfg->FTDCoffset[tr] = dc;
				WDcfg->FTDCoffset[tr * 2] = (uint32_t)dc;
				WDcfg->FTDCoffset[tr * 2 + 1] = (uint32_t)dc;
				continue;
			}
			
			val = (int)((dc + 50) * 65535 / 100);
			if (ch == -1)
				for (i = 0; i < MAX_SET; i++)
				{
					WDcfg->DCoffset[i] = val;
					WDcfg->Version_used[i] = 0;
					dc_file[i] = dc;
				}
			else
			{
				WDcfg->DCoffset[ch] = val;
				WDcfg->Version_used[ch] = 0;
				dc_file[ch] = dc;
			}

			continue;
		}


		if (!strcmp(str, "BASELINE_LEVEL")) 
		{

			int dc;
			read = fscanf(f_ini, "%d", &dc);
			if (tr != -1) {
				// 				WDcfg->FTDCoffset[tr] = dc;
				WDcfg->FTDCoffset[tr * 2] = (uint32_t)dc;
				WDcfg->FTDCoffset[tr * 2 + 1] = (uint32_t)dc;
				continue;
			}

			if (ch == -1)
			{
				for (i = 0; i < MAX_SET; i++)
				{
					WDcfg->Version_used[i] = 1;
					dc_file[i] = dc;
					if (WDcfg->PulsePolarity[i] == CAEN_DGTZ_TriggerOnRisingEdge)//CAEN_DGTZ_PulsePolarityPositive)					
						WDcfg->DCoffset[i] = (uint32_t)((float)(fabs(dc - 100))*(655.35));
					
					else if (WDcfg->PulsePolarity[i] == CAEN_DGTZ_TriggerOnFallingEdge)//CAEN_DGTZ_PulsePolarityNegative)					
						WDcfg->DCoffset[i] = (uint32_t)((float)(dc)*(655.35));					
				}
			}
			else 
			{
				WDcfg->Version_used[ch] = 1;
				dc_file[ch] = dc;
				if (WDcfg->PulsePolarity[ch] == CAEN_DGTZ_TriggerOnRisingEdge)//CAEN_DGTZ_PulsePolarityPositive)
				{
					WDcfg->DCoffset[ch] = (uint32_t)((float)(fabs(dc - 100))*(655.35));
					//printf("ch %d positive, offset %d\n",ch, WDcfg->DCoffset[ch]);
				}
					
				
				else if (WDcfg->PulsePolarity[ch] == CAEN_DGTZ_TriggerOnFallingEdge)//CAEN_DGTZ_PulsePolarityNegative)
				{
					WDcfg->DCoffset[ch] = (uint32_t)((float)(dc)*(655.35));
					//printf("ch %d negative, offset %d\n",ch, WDcfg->DCoffset[ch]);
				}
				
				
			   }
			
				continue;
			}
		/*
		if (strstr(str, "GRP_CH_DC_OFFSET")!=NULL) ///xx742
		{
            float dc[8];
			read = fscanf(f_ini, "%f,%f,%f,%f,%f,%f,%f,%f", &dc[0], &dc[1], &dc[2], &dc[3], &dc[4], &dc[5], &dc[6], &dc[7]);
			int j = 0;
			for( j=0;j<8;j++) dc_8file[j] = dc[j];

            for(i=0; i<8; i++) //MAX_SET
			{				
				val = (int)((dc[i]+50) * 65535 / 100); ///DC offset (percent of the dynamic range, -50 to 50)
				WDcfg->DCoffsetGrpCh[ch][i]=val;
            }
			continue;
		}
		*/

		// Threshold
		if (strstr(str, "TRIGGER_THRESHOLD")!=NULL) {
			read = fscanf(f_ini, "%d", &val);
			if (tr != -1) {
//				WDcfg->FTThreshold[tr] = val;
 				WDcfg->FTThreshold[tr*2] = val;
 				WDcfg->FTThreshold[tr*2+1] = val;

				continue;
			}
            if (ch == -1)
				for (i = 0; i < MAX_SET; i++)
				{
					WDcfg->Threshold[i] = val;
					thr_file[i] = val;
				}
			else
			{
				WDcfg->Threshold[ch] = val;
				thr_file[ch] = val;
			}
			continue;
		}

		// Group Trigger Enable Mask (hex 8 bit)
		if (strstr(str, "GROUP_TRG_ENABLE_MASK")!=NULL) {
			read = fscanf(f_ini, "%x", &val);
            if (ch == -1)
                for(i=0; i<MAX_SET; i++)
                    WDcfg->GroupTrgEnableMask[i] = val & 0xFF;
            else
                 WDcfg->GroupTrgEnableMask[ch] = val & 0xFF;
			continue;
		}

        // Channel Auto trigger (DISABLED, ACQUISITION_ONLY, ACQUISITION_AND_TRGOUT)
		if (strstr(str, "CHANNEL_TRIGGER")!=NULL) {
            CAEN_DGTZ_TriggerMode_t tm;
			read = fscanf(f_ini, "%s", str1);
			if (strcmp(str1, "DISABLED") == 0)
				tm = CAEN_DGTZ_TRGMODE_DISABLED;
			else if (strcmp(str1, "ACQUISITION_ONLY") == 0)
				tm = CAEN_DGTZ_TRGMODE_ACQ_ONLY;
			else if (strcmp(str1, "ACQUISITION_AND_TRGOUT") == 0)
				tm = CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT;
			else if (strcmp(str1, "TRGOUT_ONLY") == 0)
				tm = CAEN_DGTZ_TRGMODE_EXTOUT_ONLY;
            else {
                printf("%s: Invalid Parameter\n", str);
                continue;
            }
            if (ch == -1)
                for(i=0; i<MAX_SET; i++)
                    WDcfg->ChannelTriggerMode[i] = tm;
			else
				WDcfg->ChannelTriggerMode[ch] = tm; 
			
		    continue;
		}

        // Front Panel LEMO I/O level (NIM, TTL)
		if (strstr(str, "FPIO_LEVEL")!=NULL) {
			read = fscanf(f_ini, "%s", str1);
			if (strcmp(str1, "TTL")==0)
				WDcfg->FPIOtype = CAEN_DGTZ_IOLevel_TTL; //1;
			else if (strcmp(str1, "NIM")!=0)
				printf("%s: invalid option\n", str);
			continue;
		}

        // Channel Enable (or Group enable for the V1740) (YES/NO)
        if (strstr(str, "ENABLE_INPUT")!=NULL) {
			read = fscanf(f_ini, "%s", str1);
            if (strcmp(str1, "YES")==0) {
                if (ch == -1)
                    WDcfg->EnableMask = 0xFF;
				else
				{
					WDcfg->EnableMask |= (1 << ch);
				}
			    continue;
            } else if (strcmp(str1, "NO")==0) {
                if (ch == -1)
                    WDcfg->EnableMask = 0x00;
                else
                    WDcfg->EnableMask &= ~(1 << ch);
			    continue;
            } else {
                printf("%s: invalid option\n", str);
            }
			continue;
		}

        // Skip startup calibration
        if (strstr(str, "SKIP_STARTUP_CALIBRATION") != NULL) {
            read = fscanf(f_ini, "%s", str1);
            if (strcmp(str1, "YES") == 0)
                WDcfg->StartupCalibration = 0;
            else
                WDcfg->StartupCalibration = 1;
            continue;
        }

        printf("%s: invalid setting\n", str);
	}
	return ret;
}

//part from DTconfig.c


CAEN_DGTZ_ErrorCode GetMoreBoardInfo(int handle, CAEN_DGTZ_BoardInfo_t BoardInfo, WaveDumpConfig_t *WDcfg)
{
    CAEN_DGTZ_ErrorCode ret = CAEN_DGTZ_Success;
    switch(BoardInfo.FamilyCode) {
        CAEN_DGTZ_DRS4Frequency_t freq;

    case CAEN_DGTZ_XX724_FAMILY_CODE:
    case CAEN_DGTZ_XX781_FAMILY_CODE:
    case CAEN_DGTZ_XX782_FAMILY_CODE:
    case CAEN_DGTZ_XX780_FAMILY_CODE:
        WDcfg->Nbit = 14; WDcfg->Ts = 10.0; break;
    case CAEN_DGTZ_XX720_FAMILY_CODE: WDcfg->Nbit = 12; WDcfg->Ts = 4.0;  break;
    case CAEN_DGTZ_XX721_FAMILY_CODE: WDcfg->Nbit =  8; WDcfg->Ts = 2.0;  break;
    case CAEN_DGTZ_XX731_FAMILY_CODE: WDcfg->Nbit =  8; WDcfg->Ts = 2.0;  break;
    case CAEN_DGTZ_XX751_FAMILY_CODE: WDcfg->Nbit = 10; WDcfg->Ts = 1.0;  break;
    case CAEN_DGTZ_XX761_FAMILY_CODE: WDcfg->Nbit = 10; WDcfg->Ts = 0.25;  break;
    case CAEN_DGTZ_XX740_FAMILY_CODE: WDcfg->Nbit = 12; WDcfg->Ts = 16.0; break;
    case CAEN_DGTZ_XX725_FAMILY_CODE: WDcfg->Nbit = 14; WDcfg->Ts = 4.0; break;
    case CAEN_DGTZ_XX730_FAMILY_CODE: WDcfg->Nbit = 14; WDcfg->Ts = 2.0; break;
    case CAEN_DGTZ_XX742_FAMILY_CODE: 
        WDcfg->Nbit = 12; 
        if ((ret = CAEN_DGTZ_GetDRS4SamplingFrequency(handle, &freq)) != CAEN_DGTZ_Success) return CAEN_DGTZ_CommError;
        switch (freq) {
        case CAEN_DGTZ_DRS4_1GHz:
            WDcfg->Ts = 1.0;
            break;
        case CAEN_DGTZ_DRS4_2_5GHz:
            WDcfg->Ts = (float)0.4;
            break;
        case CAEN_DGTZ_DRS4_5GHz:
            WDcfg->Ts = (float)0.2;
            break;
		case CAEN_DGTZ_DRS4_750MHz:
            WDcfg->Ts = (float)(1.0 / 750.0 * 1000.0);
            break;
        }
        switch(BoardInfo.FormFactor) {
        case CAEN_DGTZ_VME64_FORM_FACTOR:
        case CAEN_DGTZ_VME64X_FORM_FACTOR:
            WDcfg->MaxGroupNumber = 4;
            break;
        case CAEN_DGTZ_DESKTOP_FORM_FACTOR:
        case CAEN_DGTZ_NIM_FORM_FACTOR:
        default:
            WDcfg->MaxGroupNumber = 2;
            break;
        }
        break;
    default: return  ret;//-1; 
    }
    if (((BoardInfo.FamilyCode == CAEN_DGTZ_XX751_FAMILY_CODE) ||
        (BoardInfo.FamilyCode == CAEN_DGTZ_XX731_FAMILY_CODE) ) && WDcfg->DesMode)
        WDcfg->Ts /= 2;

    switch(BoardInfo.FamilyCode) {
    case CAEN_DGTZ_XX724_FAMILY_CODE:
    case CAEN_DGTZ_XX781_FAMILY_CODE:
    case CAEN_DGTZ_XX782_FAMILY_CODE:
    case CAEN_DGTZ_XX780_FAMILY_CODE:
    case CAEN_DGTZ_XX720_FAMILY_CODE:
    case CAEN_DGTZ_XX721_FAMILY_CODE:
    case CAEN_DGTZ_XX751_FAMILY_CODE:
    case CAEN_DGTZ_XX761_FAMILY_CODE:
    case CAEN_DGTZ_XX731_FAMILY_CODE:
        switch(BoardInfo.FormFactor) {
        case CAEN_DGTZ_VME64_FORM_FACTOR:
        case CAEN_DGTZ_VME64X_FORM_FACTOR:
            WDcfg->Nch = 8;
            break;
        case CAEN_DGTZ_DESKTOP_FORM_FACTOR:
        case CAEN_DGTZ_NIM_FORM_FACTOR:
            WDcfg->Nch = 4;
            break;
        }
        break;
    case CAEN_DGTZ_XX725_FAMILY_CODE:
    case CAEN_DGTZ_XX730_FAMILY_CODE:
        switch(BoardInfo.FormFactor) {
        case CAEN_DGTZ_VME64_FORM_FACTOR:
        case CAEN_DGTZ_VME64X_FORM_FACTOR:
            WDcfg->Nch = 16;
            break;
        case CAEN_DGTZ_DESKTOP_FORM_FACTOR:
        case CAEN_DGTZ_NIM_FORM_FACTOR:
            WDcfg->Nch = 8;
            break;
        }
        break;
    case CAEN_DGTZ_XX740_FAMILY_CODE:
        switch( BoardInfo.FormFactor) {
        case CAEN_DGTZ_VME64_FORM_FACTOR:
        case CAEN_DGTZ_VME64X_FORM_FACTOR:
            WDcfg->Nch = 64;
            break;
        case CAEN_DGTZ_DESKTOP_FORM_FACTOR:
        case CAEN_DGTZ_NIM_FORM_FACTOR:
            WDcfg->Nch = 32;
            break;
        }
        break;
    case CAEN_DGTZ_XX742_FAMILY_CODE:
        switch( BoardInfo.FormFactor) {
        case CAEN_DGTZ_VME64_FORM_FACTOR:
        case CAEN_DGTZ_VME64X_FORM_FACTOR:
            WDcfg->Nch = 36;
            break;
        case CAEN_DGTZ_DESKTOP_FORM_FACTOR:
        case CAEN_DGTZ_NIM_FORM_FACTOR:
            WDcfg->Nch = 16;
            break;
        }
        break;
    default:
        return ret;//-1;
    }
    return ret;//0;
}


CAEN_DGTZ_ErrorCode  WriteRegisterBitmask(int32_t handle, uint32_t address, uint32_t data, uint32_t mask) {
    CAEN_DGTZ_ErrorCode ret = CAEN_DGTZ_Success;
    uint32_t d32 = 0xFFFFFFFF;

    ret = CAEN_DGTZ_ReadRegister(handle, address, &d32);
    if(ret != CAEN_DGTZ_Success)
        return ret;

    data &= mask;
    d32 &= ~mask;
    d32 |= data;
    ret = CAEN_DGTZ_WriteRegister(handle, address, d32);
    return ret;
}

 CAEN_DGTZ_ErrorCode CheckBoardFailureStatus(int handle) {

	CAEN_DGTZ_ErrorCode ret = CAEN_DGTZ_Success;
	uint32_t status = 0;
	ret = CAEN_DGTZ_ReadRegister(handle, 0x8104, &status);
	if (ret != 0) {
		printf("Error: Unable to read board failure status.\n");
		return ret;
	}

	//usleep(200*1000);

	//read twice (first read clears the previous status)
	ret = CAEN_DGTZ_ReadRegister(handle, 0x8104, &status);
	if (ret != 0) {
		printf("Error: Unable to read board failure status.\n");
		return ret;
	}

	if(!(status & (1 << 7))) {
		printf("Board error detected: PLL not locked.\n");
		return ret;
	}

	return ret;
}

CAEN_DGTZ_ErrorCode Set_calibrated_DCO(int handle, int ch, WaveDumpConfig_t *WDcfg, CAEN_DGTZ_BoardInfo_t BoardInfo) {
	CAEN_DGTZ_ErrorCode ret = CAEN_DGTZ_Success;
	if (WDcfg->Version_used[ch] == 0) //old DC_OFFSET config, skip calibration
		return ret;
	if (WDcfg->PulsePolarity[ch] == CAEN_DGTZ_TriggerOnRisingEdge) { //CAEN_DGTZ_PulsePolarityPositive
		WDcfg->DCoffset[ch] = (uint32_t)((float)(fabs((((float)dc_file[ch] - WDcfg->DAC_Calib.offset[ch]) / WDcfg->DAC_Calib.cal[ch]) - 100.))*(655.35));
		if (WDcfg->DCoffset[ch] > 65535) WDcfg->DCoffset[ch] = 65535;
		if (WDcfg->DCoffset[ch] < 0) WDcfg->DCoffset[ch] = 0;
	}
	else if (WDcfg->PulsePolarity[ch] == CAEN_DGTZ_TriggerOnFallingEdge) { //CAEN_DGTZ_PulsePolarityNegative
		WDcfg->DCoffset[ch] = (uint32_t)((float)(fabs(((fabs(dc_file[ch] - 100.) - WDcfg->DAC_Calib.offset[ch]) / WDcfg->DAC_Calib.cal[ch]) - 100.))*(655.35));
		if (WDcfg->DCoffset[ch] < 0) WDcfg->DCoffset[ch] = 0;
		if (WDcfg->DCoffset[ch] > 65535) WDcfg->DCoffset[ch] = 65535;
	}

	ret = CAEN_DGTZ_SetChannelDCOffset(handle, (uint32_t)ch, WDcfg->DCoffset[ch]);
		if (ret)
			printf("Error setting channel %d offset\n", ch);
	/*
	if (BoardInfo.FamilyCode == CAEN_DGTZ_XX740_FAMILY_CODE) {
		ret = CAEN_DGTZ_SetGroupDCOffset(handle, (uint32_t)ch, WDcfg->DCoffset[ch]);
		if (ret)
			printf("Error setting group %d offset\n", ch);
	}
	else {
		ret = CAEN_DGTZ_SetChannelDCOffset(handle, (uint32_t)ch, WDcfg->DCoffset[ch]);
		if (ret)
			printf("Error setting channel %d offset\n", ch);
	}
	*/

	return ret;
}

CAEN_DGTZ_ErrorCode  Get_current_baseline(int handle, WaveDumpConfig_t* WDcfg, char* buffer, char* EventPtr, CAEN_DGTZ_BoardInfo_t BoardInfo, double *baselines) {
    CAEN_DGTZ_ErrorCode ret = CAEN_DGTZ_Success;
    uint32_t BufferSize = 0;
    int32_t i = 0, ch;
    uint32_t max_sample = 0x1 << WDcfg->Nbit;

    CAEN_DGTZ_UINT16_EVENT_t* Event16 = (CAEN_DGTZ_UINT16_EVENT_t*)EventPtr;
    CAEN_DGTZ_UINT8_EVENT_t* Event8 = (CAEN_DGTZ_UINT8_EVENT_t*)EventPtr;

    int32_t* histo = (int32_t*)malloc(max_sample * sizeof(*histo));
    if (histo == NULL) {
        fprintf(stderr, "Can't allocate histogram.\n");
        //return ERR_MALLOC;
		printf("ERR_MALLOC \n");
    }

    if ((ret = CAEN_DGTZ_ClearData(handle)) != CAEN_DGTZ_Success) {
        fprintf(stderr, "Can't clear data.\n");
        if (histo != NULL)
        free(histo);
    }

    if ((ret = CAEN_DGTZ_SWStartAcquisition(handle)) != CAEN_DGTZ_Success) {
        fprintf(stderr, "Can't start acquisition.\n");
        if (histo != NULL)
        free(histo);
    }

    for (i = 0; i < 100 && BufferSize == 0; i++) {
        if ((ret = CAEN_DGTZ_SendSWtrigger(handle)) != CAEN_DGTZ_Success) {
            fprintf(stderr, "Can't send SW trigger.\n");
            if (histo != NULL)
			free(histo);
        }
        if ((ret = CAEN_DGTZ_ReadData(handle, CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT, buffer, &BufferSize)) != CAEN_DGTZ_Success) {
            fprintf(stderr, "Can't read data.\n");
            if (histo != NULL)
			free(histo);
        }
    }

    if ((ret = CAEN_DGTZ_SWStopAcquisition(handle)) != CAEN_DGTZ_Success) {
        fprintf(stderr, "Can't stop acquisition.\n");
        if (histo != NULL)
        free(histo);
    }

    if (BufferSize == 0) {
        fprintf(stderr, "Can't get SW trigger events.\n");
        if (histo != NULL)
        free(histo);
    }
	//else
	//	printf(" \n Get_current_baseline BuffersSize %i \n", BufferSize);

    if ((ret = CAEN_DGTZ_DecodeEvent(handle, buffer, (void**)&EventPtr)) != CAEN_DGTZ_Success) {
        fprintf(stderr, "Can't decode events\n");
        if (histo != NULL)
        free(histo);
    }

    memset(baselines, 0, BoardInfo.Channels * sizeof(*baselines));
    for (ch = 0; ch < (int32_t)BoardInfo.Channels; ch++) {
        if (WDcfg->EnableMask & (1 << ch)) {
            int32_t event_ch = (BoardInfo.FamilyCode == CAEN_DGTZ_XX740_FAMILY_CODE) ? (ch * 8) : ch; //for x740 boards shift to channel 0 of next group
            uint32_t size = (WDcfg->Nbit == 8) ? Event8->ChSize[event_ch] : Event16->ChSize[event_ch];
            uint32_t s;
            uint32_t maxs = 0;

            memset(histo, 0, max_sample * sizeof(*histo));
            for (s = 0; s < size; s++) {
                uint16_t value = (WDcfg->Nbit == 8) ? Event8->DataChannel[event_ch][s] : Event16->DataChannel[event_ch][i];
                if (value < max_sample) {
                    histo[value]++;
                    if (histo[value] > histo[maxs])
                        maxs = value;
                }
            }

            if (WDcfg->PulsePolarity[ch] == CAEN_DGTZ_TriggerOnRisingEdge)//CAEN_DGTZ_PulsePolarityPositive)
                baselines[ch] = maxs * 100.0 / max_sample;
            else if (WDcfg->PulsePolarity[ch] == CAEN_DGTZ_TriggerOnFallingEdge)//CAEN_DGTZ_PulsePolarityNegative)
                baselines[ch] = 100.0 * (1.0 - (double)maxs / max_sample);
        }
    }

//QuitFunction:

   if (histo != NULL)
       free(histo);

    return ret;
}

/*! \fn      int32_t Set_relative_Threshold(int handle, WaveDumpConfig_t *WDcfg, CAEN_DGTZ_BoardInfo_t BoardInfo)
*   \brief   Finds the correct DCOffset for the BASELINE_LEVEL given in configuration file for each channel and
*            sets the threshold relative to it. To find the baseline, for each channel, two DCOffset values are
*            tried, and in the end the value given by the line passing from them is used. This function ignores
*            the DCOffset calibration previously loaded.
*
*   \param   handle   Digitizer handle
*   \param   WDcfg:   Pointer to the WaveDumpConfig_t data structure
*   \param   BoardInfo: structure with the board info
*/

CAEN_DGTZ_ErrorCode QuitThreshold(int handle, char* buffer, char* evtbuff){
	
CAEN_DGTZ_ErrorCode ret = CAEN_DGTZ_Success;
	
	if (evtbuff != NULL)
        ret = CAEN_DGTZ_FreeEvent(handle, (void**)&evtbuff);
	
    if (buffer != NULL)
        ret = CAEN_DGTZ_FreeReadoutBuffer(&buffer);
	
return ret;
}

CAEN_DGTZ_ErrorCode Set_relative_Threshold(int handle, WaveDumpConfig_t* WDcfg, CAEN_DGTZ_BoardInfo_t BoardInfo) {
    int32_t ch;
	CAEN_DGTZ_ErrorCode ret = CAEN_DGTZ_Success;
    uint32_t  AllocatedSize;
    
    char* buffer = NULL;
    char* evtbuff = NULL;
    uint32_t exdco[MAX_SET];
    double baselines[MAX_SET];
    double dcocalib[MAX_SET][2];

    //preliminary check: if baseline shift is not enabled for any channel quit
    int should_start = 0;
    for (ch = 0; ch < (int32_t)BoardInfo.Channels; ch++) {
        if (WDcfg->EnableMask & (1 << ch) && WDcfg->Version_used[ch] == 1) {
            should_start = 1;
            break;
        }
    }
    if (!should_start)
        return CAEN_DGTZ_Success;

    // Memory allocation
    ret = CAEN_DGTZ_MallocReadoutBuffer(handle, &buffer, &AllocatedSize);
    if (ret) {
        printf("ERR_MALLOC \n");
        QuitThreshold(handle, buffer, evtbuff);
    }
    ret = CAEN_DGTZ_AllocateEvent(handle, (void**)&evtbuff);
    if (ret != CAEN_DGTZ_Success) {
        printf("ERR_MALLOC \n");
        QuitThreshold(handle, buffer, evtbuff);
    }
    
    for (ch = 0; ch < (int32_t)BoardInfo.Channels; ch++) {
        if (WDcfg->EnableMask & (1 << ch) && WDcfg->Version_used[ch] == 1) {
            // Assume the uncalibrated DCO is not far from the correct one
            if (WDcfg->PulsePolarity[ch] == CAEN_DGTZ_TriggerOnRisingEdge)//CAEN_DGTZ_PulsePolarityPositive)
                exdco[ch] = (uint32_t)((100.0 - dc_file[ch]) * 655.35);
            else if (WDcfg->PulsePolarity[ch] == CAEN_DGTZ_TriggerOnFallingEdge)//CAEN_DGTZ_PulsePolarityNegative)
                exdco[ch] = (uint32_t)(dc_file[ch] * 655.35);
			
            //if (BoardInfo.FamilyCode == CAEN_DGTZ_XX740_FAMILY_CODE)
            //    ret = CAEN_DGTZ_SetGroupDCOffset(handle, ch, exdco[ch]);
            //else
            //    ret = CAEN_DGTZ_SetChannelDCOffset(handle, ch, exdco[ch]);
			
			ret = CAEN_DGTZ_SetChannelDCOffset(handle, ch, exdco[ch]);
            if (ret != CAEN_DGTZ_Success) {
                fprintf(stderr, "Error setting DCOffset for channel %d\n", ch);
                QuitThreshold(handle, buffer, evtbuff);
            }
        }
    }
    // Sleep some time to let the DAC move
    usleep(200*1000);
	
    if ((ret = Get_current_baseline(handle, WDcfg, buffer, evtbuff, BoardInfo, baselines)) != CAEN_DGTZ_Success)
        QuitThreshold(handle, buffer, evtbuff);
    for (ch = 0; ch < (int32_t)BoardInfo.Channels; ch++) {
        if (WDcfg->EnableMask & (1 << ch) && WDcfg->Version_used[ch] == 1) {
            double newdco = 0.0;
            // save results of this round
            dcocalib[ch][0] = baselines[ch];
            dcocalib[ch][1] = exdco[ch];
            // ... and perform a new round, using measured value and theoretical zero
            if (WDcfg->PulsePolarity[ch] == CAEN_DGTZ_TriggerOnRisingEdge)//CAEN_DGTZ_PulsePolarityPositive)
                newdco = linear_interp(0, 65535, baselines[ch], exdco[ch], dc_file[ch]);
            else if (WDcfg->PulsePolarity[ch] == CAEN_DGTZ_TriggerOnFallingEdge)//CAEN_DGTZ_PulsePolarityNegative)
                newdco = linear_interp(0, 0, baselines[ch], exdco[ch], dc_file[ch]);
            if (newdco < 0)
                exdco[ch] = 0;
            else if (newdco > 65535)
                exdco[ch] = 65535;
            else
                exdco[ch] = (uint32_t)newdco;
            //if (BoardInfo.FamilyCode == CAEN_DGTZ_XX740_FAMILY_CODE)
            //    ret = CAEN_DGTZ_SetGroupDCOffset(handle, ch, exdco[ch]);
            //else
            //    ret = CAEN_DGTZ_SetChannelDCOffset(handle, ch, exdco[ch]);
			ret = CAEN_DGTZ_SetChannelDCOffset(handle, ch, exdco[ch]);
            if (ret != CAEN_DGTZ_Success) {
                fprintf(stderr, "Error setting DCOffset for channel %d\n", ch);
                QuitThreshold(handle, buffer, evtbuff);
            }
        }
    }
    // Sleep some time to let the DAC move
    usleep(200*1000);
	
    if ((ret = Get_current_baseline(handle, WDcfg, buffer, evtbuff, BoardInfo, baselines)) != CAEN_DGTZ_Success)
        QuitThreshold(handle, buffer, evtbuff);
    for (ch = 0; ch < (int32_t)BoardInfo.Channels; ch++) {
        if (WDcfg->EnableMask & (1 << ch) && WDcfg->Version_used[ch] == 1) {
            // Now we have two real points to use for interpolation
            double newdco = linear_interp(dcocalib[ch][0], dcocalib[ch][1], baselines[ch], exdco[ch], dc_file[ch]);
            if (newdco < 0)
                exdco[ch] = 0;
            else if (newdco > 65535)
                exdco[ch] = 65535;
            else
                exdco[ch] = (uint32_t)newdco;
			
            //if (BoardInfo.FamilyCode == CAEN_DGTZ_XX740_FAMILY_CODE)
            //    ret = CAEN_DGTZ_SetGroupDCOffset(handle, ch, exdco[ch]);
            //else
            //    ret = CAEN_DGTZ_SetChannelDCOffset(handle, ch, exdco[ch]);
			ret = CAEN_DGTZ_SetChannelDCOffset(handle, ch, exdco[ch]);
            if (ret != CAEN_DGTZ_Success) {
                fprintf(stderr, "Error setting DCOffset for channel %d\n", ch);
                QuitThreshold(handle, buffer, evtbuff);
            }
        }
    }
	
    usleep(200*1000);
	uint32_t thr = 0;
	
    if ((ret = Get_current_baseline(handle, WDcfg, buffer, evtbuff, BoardInfo, baselines)) != CAEN_DGTZ_Success)
        QuitThreshold(handle, buffer, evtbuff);
    for (ch = 0; ch < (int32_t)BoardInfo.Channels; ch++) {
        if (WDcfg->EnableMask & (1 << ch) && WDcfg->Version_used[ch] == 1) {
            if (fabs((baselines[ch] - dc_file[ch]) / dc_file[ch]) > 0.05)
                fprintf(stderr, "WARNING: set BASELINE_LEVEL for ch%d differs from settings for more than 5%c.\n", ch, '%');
            //uint32_t thr = 0;
			thr = 0;
            if (WDcfg->PulsePolarity[ch] == CAEN_DGTZ_TriggerOnRisingEdge)//CAEN_DGTZ_PulsePolarityPositive)
                thr = (uint32_t)(baselines[ch] / 100.0 * (0x1 << WDcfg->Nbit)) + WDcfg->Threshold[ch];
            else
                thr = (uint32_t)((100 - baselines[ch]) / 100.0 * (0x1 << WDcfg->Nbit)) - WDcfg->Threshold[ch];
            //if (BoardInfo.FamilyCode == CAEN_DGTZ_XX740_FAMILY_CODE)
            //    ret = CAEN_DGTZ_SetGroupTriggerThreshold(handle, ch, thr);
            //else
            //    ret = CAEN_DGTZ_SetChannelTriggerThreshold(handle, ch, thr);
			
			ret = CAEN_DGTZ_SetChannelTriggerThreshold(handle, ch, thr);
            if (ret != CAEN_DGTZ_Success) {
                fprintf(stderr, "Error setting DCOffset for channel %d\n", ch);
                QuitThreshold(handle, buffer, evtbuff);
            }
        }
    }
    printf("Relative threshold [%i] correctly set.\n", thr);

//QuitFunction:

    if (evtbuff != NULL)
        CAEN_DGTZ_FreeEvent(handle, (void**)&evtbuff);
    if (buffer != NULL)
        CAEN_DGTZ_FreeReadoutBuffer(&buffer);
  
  
    return ret;
}

CAEN_DGTZ_ErrorCode DoProgramDigitizer(int handle, WaveDumpConfig_t* WDcfg, CAEN_DGTZ_BoardInfo_t BoardInfo)
{
    int i, j;
	CAEN_DGTZ_ErrorCode ret = CAEN_DGTZ_Success;

    // reset the digitizer 
    ret = CAEN_DGTZ_Reset(handle);
    if (ret != 0) {
        printf("Error: Unable to reset digitizer.\nPlease reset digitizer manually then restart the program\n");
        return CAEN_DGTZ_InvalidDigitizerStatus;//-1;
    }

    // Set the waveform test bit for debugging
    if (WDcfg->TestPattern)
        ret = CAEN_DGTZ_WriteRegister(handle, CAEN_DGTZ_BROAD_CH_CONFIGBIT_SET_ADD, 1<<3);
    // custom setting for X742 boards
    //if (BoardInfo.FamilyCode == CAEN_DGTZ_XX742_FAMILY_CODE) {
    //    ret = CAEN_DGTZ_SetFastTriggerDigitizing(handle,WDcfg->FastTriggerEnabled);
    //    ret = CAEN_DGTZ_SetFastTriggerMode(handle,WDcfg->FastTriggerMode);
    //}
    //if ((BoardInfo.FamilyCode == CAEN_DGTZ_XX751_FAMILY_CODE) || (BoardInfo.FamilyCode == CAEN_DGTZ_XX731_FAMILY_CODE)) {
    //    ret = CAEN_DGTZ_SetDESMode(handle, WDcfg->DesMode);
    //}
    ret = CAEN_DGTZ_SetRecordLength(handle, WDcfg->RecordLength);
    ret = CAEN_DGTZ_GetRecordLength(handle, &(WDcfg->RecordLength));

    //if (BoardInfo.FamilyCode == CAEN_DGTZ_XX740_FAMILY_CODE || BoardInfo.FamilyCode == CAEN_DGTZ_XX724_FAMILY_CODE) {
    //    ret = CAEN_DGTZ_SetDecimationFactor(handle, WDcfg->DecimationFactor);
    //}

    ret = CAEN_DGTZ_SetPostTriggerSize(handle, WDcfg->PostTrigger);
    if(BoardInfo.FamilyCode != CAEN_DGTZ_XX742_FAMILY_CODE) {
        uint32_t pt;
        ret = CAEN_DGTZ_GetPostTriggerSize(handle, &pt);
        WDcfg->PostTrigger = pt;
    }
    ret = CAEN_DGTZ_SetIOLevel(handle, WDcfg->FPIOtype);
    if( WDcfg->InterruptNumEvents > 0) {
        // Interrupt handling
        if( CAEN_DGTZ_SetInterruptConfig( handle, CAEN_DGTZ_ENABLE, //ret = 
            VME_INTERRUPT_LEVEL, VME_INTERRUPT_STATUS_ID,
            (uint16_t)WDcfg->InterruptNumEvents, INTERRUPT_MODE)!= 0) { // CAEN_DGTZ_Success
                printf( "\nError configuring interrupts. Interrupts disabled\n\n");
                WDcfg->InterruptNumEvents = 0;
        }
    }
	
    ret = CAEN_DGTZ_SetMaxNumEventsBLT(handle, WDcfg->NumEvents);
    ret = CAEN_DGTZ_SetAcquisitionMode(handle, CAEN_DGTZ_SW_CONTROLLED);
    ret = CAEN_DGTZ_SetExtTriggerInputMode(handle, WDcfg->ExtTriggerMode);

	ret = CAEN_DGTZ_SetChannelEnableMask(handle, WDcfg->EnableMask);
        for (i = 0; i < WDcfg->Nch; i++) {
            if (WDcfg->EnableMask & (1<<i)) {
				if (WDcfg->Version_used[i] == 1)
					//break;
					ret = Set_calibrated_DCO(handle, i, WDcfg, BoardInfo);
				else
					ret = CAEN_DGTZ_SetChannelDCOffset(handle, i, WDcfg->DCoffset[i]);
                
                ret = CAEN_DGTZ_SetChannelSelfTrigger(handle, WDcfg->ChannelTriggerMode[i], (1<<i));
                ret = CAEN_DGTZ_SetChannelTriggerThreshold(handle, i, WDcfg->Threshold[i]);
                ret = CAEN_DGTZ_SetTriggerPolarity(handle, i, WDcfg->PulsePolarity[i]); //.TriggerEdge
            }
        }
		
	   

    if (ret)
        printf("Warning: errors found during the programming of the digitizer.\nSome settings may not be executed\n");

	////
	/*	
	printf(" \n -------DoProgramDigitizer------ \n");
	printf(" RecordLength %i \n", WDcfg->RecordLength);
	printf(" PostTrigger %i \n", WDcfg->PostTrigger);
	printf(" FPIOtype %i \n", WDcfg->FPIOtype);
	printf(" NumEvents %i \n", WDcfg->NumEvents);
	printf(" ExtTriggerMode %i \n", WDcfg->ExtTriggerMode);
	printf(" EnableMask %i \n", WDcfg->EnableMask);
	printf(" Version_used %i  %i \n", WDcfg->Version_used[0], WDcfg->Version_used[1]);
		
	printf(" DCoffset %i %i \n", WDcfg->DCoffset[0], WDcfg->DCoffset[1]);
	printf(" ChannelTriggerMode %i %i \n", WDcfg->ChannelTriggerMode[0], WDcfg->ChannelTriggerMode[1]);
	printf(" Threshold %i %i \n", WDcfg->Threshold[0], WDcfg->Threshold[1]);
	printf(" PulsePolarity %i %i \n", WDcfg->PulsePolarity[0], WDcfg->PulsePolarity[1]);
		
	printf(" \n -------DoProgramDigitizer------ \n");
	*/
	///


    return ret;
}

CAEN_DGTZ_ErrorCode ProgramDigitizerWithRelativeThreshold(int handle, WaveDumpConfig_t* WDcfg, CAEN_DGTZ_BoardInfo_t BoardInfo) {
    CAEN_DGTZ_ErrorCode ret = CAEN_DGTZ_Success;
    if ((ret = DoProgramDigitizer(handle, WDcfg, BoardInfo)) != CAEN_DGTZ_Success)
        return ret;
    if (BoardInfo.FamilyCode != CAEN_DGTZ_XX742_FAMILY_CODE) { //XX742 not considered
		//printf("Set_relative_Threshold \n");
        if ((ret = Set_relative_Threshold(handle, WDcfg, BoardInfo)) != CAEN_DGTZ_Success) {
            fprintf(stderr, "Error setting relative threshold. Fallback to normal ProgramDigitizer.");
            DoProgramDigitizer(handle, WDcfg, BoardInfo); // Rollback
            return ret;
        }
    }
    return CAEN_DGTZ_Success;
}


CAEN_DGTZ_ErrorCode ProgramDigitizer(int handle, WaveDumpConfig_t WDcfg, CAEN_DGTZ_BoardInfo_t BoardInfo) {
    int32_t ch;
    for (ch = 0; ch < (int32_t)BoardInfo.Channels; ch++) {
        if (WDcfg.EnableMask & (1 << ch) && WDcfg.Version_used[ch] == 1){
			//printf("ProgramDigitizerWithRelativeThreshold \n");
            return ProgramDigitizerWithRelativeThreshold(handle, &WDcfg, BoardInfo);
		}	
    }
    return DoProgramDigitizer(handle, &WDcfg, BoardInfo);
}

/*
int32_t BoardSupportsCalibration(CAEN_DGTZ_BoardInfo_t BoardInfo) {
    return
		BoardInfo.FamilyCode == CAEN_DGTZ_XX761_FAMILY_CODE ||
        BoardInfo.FamilyCode == CAEN_DGTZ_XX751_FAMILY_CODE ||
        BoardInfo.FamilyCode == CAEN_DGTZ_XX730_FAMILY_CODE ||
        BoardInfo.FamilyCode == CAEN_DGTZ_XX725_FAMILY_CODE;
}


void calibrate(int handle,  CAEN_DGTZ_BoardInfo_t BoardInfo) {
    printf("\n");
    if (BoardSupportsCalibration(BoardInfo)) {
        //if (WDrun->AcqRun == 0) {
            int32_t ret = CAEN_DGTZ_Calibrate(handle);
            if (ret == CAEN_DGTZ_Success) {
                printf("ADC Calibration check: the board is calibrated.\n");
            }
            else {
                printf("ADC Calibration failed. CAENDigitizer ERR %d\n", ret);
            }
            printf("\n");
        //}
        //else {
        //   printf("Can't run ADC calibration while acquisition is running.\n");
        //}
    }
    else {
        printf("ADC Calibration not needed for this board family.\n");
    }
}
*/


CAEN_DGTZ_ErrorCode InitialiseDigitizer(int handle, WaveDumpConfig_t &WDcfg, ReadoutConfig_t Rcfg) {
	char CName[200];
	CAEN_DGTZ_ErrorCode ret = CAEN_DGTZ_Success;
	CAEN_DGTZ_BoardInfo_t BoardInfo;
	
	ret = CAEN_DGTZ_OpenDigitizer2(WDcfg.LinkType, (WDcfg.LinkType == CAEN_DGTZ_ETH_V4718) ? WDcfg.ipAddress:(void *)&(WDcfg.LinkNum), WDcfg.ConetNode, WDcfg.BaseAddress, &handle);
    if(ret) {
		sprintf(CName, "Can't open digitizer \n ret = %i", ret);
		new TGMsgBox(gClient->GetRoot(), Rcfg.main, "Error", CName, kMBIconStop, kMBOk);
        ret = QuitMain(handle, buffer);
	}

	//GetInfo 
	ret = CAEN_DGTZ_GetInfo(handle, &BoardInfo);
	printf("handle after GetInfo %i \n", handle);
	sprintf(CName, "Connected to CAEN Digitizer Model %s \n ROC FPGA Release is %s\n AMC FPGA Release is %s\n", BoardInfo.ModelName, BoardInfo.ROC_FirmwareRel, BoardInfo.AMC_FirmwareRel);
	
	//new TGMsgBox(gClient->GetRoot(), fMain, "Info", CName, kMBIconAsterisk, kMBOk);
	new TGMsgBox(gClient->GetRoot(), Rcfg.main, "Info", CName, kMBIconAsterisk, kMBOk);
	
	
	    
	// Check firmware revision (DPP firmwares cannot be used with this demo 
	int MajorNumber;
	sscanf(BoardInfo.AMC_FirmwareRel, "%d", &MajorNumber);
	if (MajorNumber >= 128) {
		sprintf(CName, "This digitizer has a DPP firmware! \n");
		new TGMsgBox(gClient->GetRoot(), Rcfg.main, "Error", CName, kMBIconStop, kMBOk);
		ret = QuitMain(handle, buffer);
	}
	
	 // Get Number of Channels, Number of bits, Number of Groups of the board 
    ret = GetMoreBoardInfo(handle, BoardInfo, &WDcfg);
    if (ret) {
		sprintf(CName, "GET_MORE_BOARD_FAILURE \n");
        new TGMsgBox(gClient->GetRoot(), Rcfg.main, "Error", CName, kMBIconStop, kMBOk);
        ret = QuitMain(handle, buffer);
    }

	//Check for possible board internal errors
	ret = CheckBoardFailureStatus(handle);
	if (ret) {
		sprintf(CName, "GET_MORE_BOARD_FAILURE \n");
		new TGMsgBox(gClient->GetRoot(), Rcfg.main, "Error", CName, kMBIconStop, kMBOk);
		ret = QuitMain(handle, buffer);
	}

	//set default DAC calibration coefficients
	//for (int i = 0; i < MAX_SET; i++) {
	//	WDcfg.DAC_Calib.cal[i] = 1;
	//	WDcfg.DAC_Calib.offset[i] = 0;
	//}
		
    // HACK
    for (int ch = 0; ch < 2; ch++) {
        WDcfg.DAC_Calib.cal[ch] = 1.0;
        WDcfg.DAC_Calib.offset[ch] = 0.0;
    }	
    
	WDcfg.EnableMask &= (1<<WDcfg.Nch)-1;	
	  
 
		
	// *************************************************************************************** 
    // program the digitizer                                                                   
    // *************************************************************************************** 

	
    ret = ProgramDigitizer(handle, WDcfg, BoardInfo);
    if (ret) {
		sprintf(CName, "ERR_PROGRAM_DIGITIZER_FAILURE \n");
        new TGMsgBox(gClient->GetRoot(), Rcfg.main, "Error", CName, kMBIconStop, kMBOk);
        ret = QuitMain(handle, buffer);
    }	

	
    //usleep(300*1000);

	//check for possible failures after programming the digitizer
	ret = CheckBoardFailureStatus(handle);
	if (ret) {
		sprintf(CName, "ERR_BOARD_FAILURE \n");
		new TGMsgBox(gClient->GetRoot(), Rcfg.main, "Error", CName, kMBIconStop, kMBOk);
		ret = QuitMain(handle, buffer);
	}

    // Read again the board infos, just in case some of them were changed by the programming
    // (like, for example, the TSample and the number of channels if DES mode is changed)

  	ret = CAEN_DGTZ_GetInfo(handle, &BoardInfo);
    if (ret) {
		sprintf(CName, "GET_INFO_FAILURE \n");
        new TGMsgBox(gClient->GetRoot(), Rcfg.main, "Error", CName, kMBIconStop, kMBOk);
        ret = QuitMain(handle, buffer);
    }
	
	// Get Number of Channels, Number of bits, Number of Groups of the board 
    ret = GetMoreBoardInfo(handle, BoardInfo, &WDcfg);
    if (ret) {
		sprintf(CName, "GET_MORE_BOARDINFO_FAILURE \n");
        new TGMsgBox(gClient->GetRoot(), Rcfg.main, "Error", CName, kMBIconStop, kMBOk);
        ret = QuitMain(handle, buffer);
    }
	
	ret = CAEN_DGTZ_AllocateEvent(handle, (void**)&Event16);
	if (ret) {
		sprintf(CName, "ALLOCATE_EVENT_FAILURE \n");
		new TGMsgBox(gClient->GetRoot(), Rcfg.main, "Error", CName, kMBIconStop, kMBOk);
		ret = QuitMain(handle, buffer);
	}
    // Malloc Readout Buffer.
    //NOTE1: The mallocs must be done AFTER digitizer's configuration!
    uint32_t AllocatedSize;
    ret = CAEN_DGTZ_MallocReadoutBuffer(handle, &buffer, &AllocatedSize);
	if (ret) {
		sprintf(CName, "MALLOC_READOUT_BUFFER_FAILURE \n");
		new TGMsgBox(gClient->GetRoot(), Rcfg.main, "Error", CName, kMBIconStop, kMBOk);
		ret = QuitMain(handle, buffer);
	}
  		
	ret = CAEN_DGTZ_GetChannelTriggerThreshold(handle, 0, &WDcfg.cal_thr[0]);
	printf(" Threshold [0] = %d \n", WDcfg.cal_thr[0]);
	ret = CAEN_DGTZ_GetChannelTriggerThreshold(handle, 1, &WDcfg.cal_thr[1]);
	printf(" Threshold [1] = %d \n", WDcfg.cal_thr[1]);
	
		
	for (int i=0; i<MAX_CH; i++){
		printf(" Trigger for CH[%i] will be: %i  (cal_thr[%i]: %i)\n", i, WDcfg.cal_thr[i] - WDcfg.thr[i], i, WDcfg.cal_thr[i]);
		ret = CAEN_DGTZ_SetChannelTriggerThreshold(handle, i, WDcfg.cal_thr[i] - WDcfg.thr[i]);
		if (ret) {
			sprintf(CName, "SET_CHANNEL_TRIGGER_THRESHOLD_FAILURE \n");
			new TGMsgBox(gClient->GetRoot(), Rcfg.main, "Error", CName, kMBIconStop, kMBOk);
			ret = QuitMain(handle, buffer);
		}
	}
	return ret;
}


CAEN_DGTZ_ErrorCode QuitMain(int handle, char* buffer){
	CAEN_DGTZ_ErrorCode ret = CAEN_DGTZ_Success;
	    ret = CAEN_DGTZ_FreeReadoutBuffer(&buffer);
		ret = CAEN_DGTZ_CloseDigitizer(handle);
				
		exit(0);
		return ret;
}