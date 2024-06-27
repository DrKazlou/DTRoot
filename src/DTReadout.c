#include "DTReadout.h"
#include "DTFunc.h"
#include "TROOT.h"
#include "TSystem.h"
#include <TGMsgBox.h>

extern WaveDumpConfig_t WDcfg;
extern Histograms_t Histo;
extern ReadoutConfig_t Rcfg;
//extern int handle; 
extern char *buffer;
extern CAEN_DGTZ_UINT16_EVENT_t *Event16;


long get_time()
{
    long time_ms;
    struct timeval t1;
    struct timezone tz;
    gettimeofday(&t1, &tz);
    time_ms = (t1.tv_sec) * 1000 + t1.tv_usec / 1000;

    return time_ms;
}

int DeltaT(TH1D *hist[2]){
	Double_t  min_val[2], min_bin[2];	

	for (int i =0; i<2; i++){
		min_val[i] = 5000;
		min_bin[i] = 0;
	
		for (int bin = 1; bin<hist[i]->GetNbinsX(); bin++){
			if (hist[i]->GetBinContent(bin)<min_val[i]){
				min_bin[i] = bin;
				min_val[i] = hist[i]->GetBinContent(bin);
			}	
			
		}
	}
	
	return (min_bin[1] - min_bin[0]) * b_width;
}

void CalcRate(uint64_t &PrevRateTime){
	char CName[50];
	uint64_t CurrentTime, ElapsedTime, DAQTime;
	
    CurrentTime = get_time();
    ElapsedTime = CurrentTime - PrevRateTime;
	DAQTime = (CurrentTime - Rcfg.StartTime) / 1000;
	if (Rcfg.fTimer && DAQTime > Rcfg.timer_val)
		Rcfg.loop = 0;

	if (ElapsedTime > 1000) { // 1000 - 1 sec
		sprintf(CName,"T: %li s",  DAQTime );
		Rcfg.TLabel->SetText(CName);
		gSystem->ProcessEvents(); 
       	if (Rcfg.Nb == 0){
			//printf("No data...\n");
           	//if (ret == CAEN_DGTZ_Timeout) printf ("Timeout...\n"); else printf("No data...\n");
			//printf("No data...\n");
			sprintf(CName, "No data...");
			Rcfg.StatusBar->SetText(CName, 0);
			Rcfg.StatusBar->SetText(CName, 1);
		}	
    	else{
			//printf("Reading at %.2f MB/s (Trg Rate: %.2f Hz)\n", (float)Nb/((float)ElapsedTime*1048.576f), (float)Ne*1000.0f/(float)ElapsedTime);
			sprintf(CName,"Reading at %.2f MB/s ", (float)Rcfg.Nb/((float)ElapsedTime*1048.576f) );
			Rcfg.StatusBar->SetText(CName, 0);
			sprintf(CName, "Trg Rate: %.2f Hz ", (float)Rcfg.Nev*1000.0f/(float)ElapsedTime);
			Rcfg.StatusBar->SetText(CName, 1);
		}
    Rcfg.Nb = 0;
    Rcfg.Nev = 0;
    PrevRateTime = CurrentTime;
	gSystem->ProcessEvents(); 
    }

}

void InitReadoutConfig( ){
	
	Rcfg.fPrint = false; // common print flag for debug
	Rcfg.fStoreTraces = false; // flag to store traces
	Rcfg.fTimer = false; // flag for timer usage
	
	Rcfg.loop = -1; // ReadoutLoop flag
	Rcfg.DrawTime = 0.5; // time between histograms drawing in sec
	//for (int i = 0; i<MAX_CH; i++)  // MAX_CH = 2
	//	Rcfg.TrgCnt[i] = 0;
	
}

void InitHisto( ){
char CName[30];
	Histo.PSD_BIN = 2;
	Histo.CH_2D = 0;
	Histo.WF_XMIN = 0; Histo.WF_XMAX = WDcfg.RecordLength * b_width;
	//Histo.WF_YMIN = -5000; Histo.WF_YMAX = 100;
	
	Histo.ALBound = 0, Histo.ARBound = WDcfg.RecordLength * b_width;
	Histo.ILBound = 0, Histo.IRBound = WDcfg.RecordLength * b_width;
	
	Histo.NPad = 1;
	Histo.BL_CUT = 20;
	Histo.fBL = true;
	Histo.fTrace = true;
	
	sprintf(Histo.h2Style, "COLZ");
	
	for (int ch = 0; ch < 2; ch++){ //
		sprintf(CName, "h_trace%i", ch);
		Histo.trace[ch]= new TH1D(CName, CName, WDcfg.RecordLength, 0, WDcfg.RecordLength * b_width);
		sprintf(CName, "h_ampl%i", ch);
		Histo.ampl[ch]= new TH1D(CName, CName, Histo.ampl_set.xbins, Histo.ampl_set.xmin, Histo.ampl_set.xmax); // 1000 0 2000
		sprintf(CName, "h_integral%i", ch);
		Histo.integral[ch]= new TH1D(CName, CName, Histo.integral_set.xbins, Histo.integral_set.xmin, Histo.integral_set.xmax); // 1000 0 20000
		Histo.fDraw[ch] = true;
	}
	
	Histo.dt = new TH1D("h_dt","h_dt", Histo.dt_set.xbins, Histo.dt_set.xmin, Histo.dt_set.xmax); // 400 -200 200
	Histo.psd_ampl = new TH2D("h_psd_ampl", "h_psd_ampl", Histo.psd_ampl_set.xbins, Histo.psd_ampl_set.xmin, Histo.psd_ampl_set.xmax, // 1000 0 2000
							  							  Histo.psd_ampl_set.ybins, Histo.psd_ampl_set.ymin, Histo.psd_ampl_set.ymax); // 1000 -2 2
	
	Histo.int_ampl = new TH2D("h_int_ampl", "h_int_ampl", Histo.ampl_set.xbins, Histo.ampl_set.xmin, Histo.ampl_set.xmax, // 1000 0 2000
							  							  Histo.integral_set.xbins, Histo.integral_set.xmin, Histo.integral_set.xmax); // 1000 -2 2

}

void FillHisto(CAEN_DGTZ_EventInfo_t EventInfo){ 

	
	Double_t BL_mean = 0, integral = 0;
	Int_t ampl = 0, m_stamp;
	Double_t psd_val =0, psd_sum1 = 0, psd_sum2 = 0;
		
	
	
	Histo.EC = (int)EventInfo.EventCounter;
	Histo.TTT = (int)EventInfo.TriggerTimeTag;
	if (Rcfg.fPrint)
		printf("Evt:%i TTT %i RTTT %i",Histo.EC, Histo.TTT, EventInfo.TriggerTimeTag);
	
	for (int ch = 0; ch<2; ch++){ 
		BL_mean = 0; ampl = 0; integral = 0;
		Int_t p = WDcfg.PulsePolarity[ch] == CAEN_DGTZ_TriggerOnRisingEdge ? 1 : -1;   // [0] put ch variable for every cases
		
		Histo.trace[ch]->Reset("ICESM");
		
		for (int j=0; j<(int)Event16->ChSize[ch]; j++)
			Histo.vec_bl[ch].push_back((double)Event16->DataChannel[ch][j]);
	
		for ( int j=0; j<Histo.BL_CUT; j++)
			BL_mean += Histo.vec_bl[ch][j];	
		BL_mean /= Histo.BL_CUT;	
		
		
		for ( int j=0; j<Histo.vec_bl[ch].size( ); j++){
			Histo.vec[ch].push_back(Histo.vec_bl[ch][j] - BL_mean);
			
			if (Histo.vec[ch][j] * p > ampl && j * b_width > Histo.ALBound && j * b_width < Histo.ARBound){
				ampl = Histo.vec[ch][j] * p;
				m_stamp = j;
			}
			
			if (j * b_width > Histo.ILBound && j * b_width < Histo.IRBound)
				integral += Histo.vec[ch][j] * p;
						
			Histo.trace[ch]->Fill(j * b_width, Histo.fBL ? Histo.vec[ch][j] : Histo.vec_bl[ch][j]);
		}
		
		if (Histo.fPSD_ampl && ch == Histo.CH_2D){
			for (int j=m_stamp; j<Histo.vec[ch].size( ); j++){
				if (j<(m_stamp + Histo.PSD_BIN) )
					psd_sum1 = psd_sum1 + p * Histo.vec[ch][j];
				if (j>(m_stamp + Histo.PSD_BIN) )
					psd_sum2 = psd_sum2 + p * Histo.vec[ch][j];
			}
			psd_val = 1 - (psd_sum1/(psd_sum1 + psd_sum2));
			Histo.psd_ampl->Fill(ampl, psd_val);	
		}
			
		Histo.integral[ch]->Fill(integral);
		Histo.ampl[ch]->Fill(ampl);
		
		if (Histo.fIA && ch == Histo.CH_2D)
			Histo.int_ampl->Fill(ampl, integral);
		
		if (Rcfg.fStoreTraces)
			Histo.v_out.push_back(Histo.vec[ch]);
		
		Histo.vec[ch].clear();
		Histo.vec_bl[ch].clear();
		
		if (Rcfg.fPrint)
			printf(" Ampl[%i] %i Int[%i] %f", ch, ampl, ch, integral);
	}// ch loop	
		if (Rcfg.fPrint)
			printf("\n");
	
	if (Rcfg.fStoreTraces){
		Rcfg.tree->Fill();	
		Histo.v_out.clear();
	}	
	
	if (Histo.fdT)
		Histo.dt->Fill(DeltaT(Histo.trace));
		
}

void DrawHisto(TCanvas *c1){
	Color_t color[3] = {kBlue, kRed, kBlack}; 	
	
	if (Histo.fTrace){
		c1->cd(1);
		for (int ch=0; ch<2; ch++){ //(int)BoardInfo.Channels
			Histo.trace[ch]->SetLineColor(color[ch]);
			if (ch == 0 && Histo.fDraw[ch]){
				Histo.trace[ch]->Draw("HIST");
				Histo.trace[ch]->GetYaxis()->SetRangeUser(Histo.WF_YMIN, Histo.WF_YMAX);
				Histo.trace[ch]->GetYaxis()->SetTitleOffset(1.1);
				Histo.trace[ch]->GetYaxis()->SetTitle("Amplitude, lsb");
				Histo.trace[ch]->GetXaxis()->SetRangeUser(Histo.WF_XMIN, Histo.WF_XMAX);
				Histo.trace[ch]->GetXaxis()->SetTitle(" Time, ns");
			}	
			else if (ch !=0 && Histo.fDraw[ch]){	
				Histo.trace[ch]->Draw(Histo.fDraw[0] ? "HIST SAME" : "HIST");
				if (!Histo.fDraw[0]){
					Histo.trace[ch]->GetYaxis()->SetRangeUser(Histo.WF_YMIN, Histo.WF_YMAX);
					Histo.trace[ch]->GetYaxis()->SetTitleOffset(1.1);
					Histo.trace[ch]->GetYaxis()->SetTitle("Amplitude, lsb");
					Histo.trace[ch]->GetXaxis()->SetRangeUser(Histo.WF_XMIN, Histo.WF_XMAX);
					Histo.trace[ch]->GetXaxis()->SetTitle(" Time, ns");
				}
					
			}	
		}
	}	
	
	if (Histo.fAmpl){
		c1->cd(Histo.cAmpl);
		for (int ch = 0; ch < 2; ch++ ){
			Histo.ampl[ch]->SetLineColor(color[ch]);
			if (ch == 0 && Histo.fDraw[ch])
				Histo.ampl[ch]->Draw("HIST");
			else if (ch !=0 && Histo.fDraw[ch])
					Histo.ampl[ch]->Draw(Histo.fDraw[0] ? "HIST SAME" : "HIST");
		}	

	}
		
	if (Histo.fInt){		
		c1->cd(Histo.cInt);
		for (int ch = 0; ch < 2; ch++ ){
			Histo.integral[ch]->SetLineColor(color[ch]);
			if (ch == 0 && Histo.fDraw[ch])
				Histo.integral[ch]->Draw("HIST");
			else if (ch!=0 && Histo.fDraw[ch])
					Histo.integral[ch]->Draw(Histo.fDraw[0] ? "HIST SAME" : "HIST");
		}	
	}
	
	if (Histo.fdT){
		c1->cd(Histo.cdT);
		Histo.dt->SetLineColor(color[2]);
		Histo.dt->Draw("HIST");
		Histo.dt->GetXaxis()->SetTitle(" Time, ns");
	}		
	
	if (Histo.fIA){
		c1->cd(Histo.cIA);
		Histo.int_ampl->SetMarkerStyle(21);
		Histo.int_ampl->SetMarkerSize(0.4);
		Histo.int_ampl->SetMarkerColor(kBlue);
		Histo.int_ampl->Draw(Histo.h2Style);
	}		
	
	if (Histo.fPSD_ampl){
		c1->cd(Histo.cPSD_ampl);
		Histo.psd_ampl->SetMarkerStyle(21);
		Histo.psd_ampl->SetMarkerSize(0.4);
		Histo.psd_ampl->SetMarkerColor(kBlue);
		Histo.psd_ampl->Draw(Histo.h2Style); //COLZ
	}		
			
	c1->Update();
	
		
}


CAEN_DGTZ_ErrorCode ReadoutLoop(int handle){
char CName[100];
	
	CAEN_DGTZ_ErrorCode ret = CAEN_DGTZ_Success;
	CAEN_DGTZ_EventInfo_t EventInfo;
	char * EventPtr = NULL;	
	uint32_t BufferSize, NumEvents;
	
	uint64_t PrevRateTime = get_time(), PrevDrawTime;
	Rcfg.Nb = 0;
	Rcfg.Nev = 0;
		
	
	while(Rcfg.loop == 1 && !ret) {
			   
		ret = CAEN_DGTZ_ReadData(handle,CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT,buffer,&BufferSize); // Read the buffer from the digitizer 
		if (ret) {
			printf("ERR_READ_DATA \n");
			sprintf(CName, "ERR_READ_DATA");
		}
		
			
        NumEvents = 0;
        if (BufferSize != 0) {
			ret = CAEN_DGTZ_GetNumEvents(handle, buffer, BufferSize, &NumEvents);
            if (ret) 
            	sprintf(CName, "GET_NUM_EVENTS");
			
        }
		else {
			uint32_t lstatus;
			ret = CAEN_DGTZ_ReadRegister(handle, CAEN_DGTZ_ACQ_STATUS_ADD, &lstatus);
			if (ret) 
				sprintf(CName, "Warning: Failure reading reg:%x \n (%d)", CAEN_DGTZ_ACQ_STATUS_ADD, ret);
			else {
				if (lstatus & (0x1 << 19)) 
					sprintf(CName, "ERR_OVERTEMP");
			}
		}
		
		Rcfg.Nb += BufferSize;
        Rcfg.Nev += NumEvents;
		
		// Calculate throughput and trigger rate (every second) 		
		CalcRate(PrevRateTime);
							
			
		for (uint32_t i=0;i<(int)NumEvents;i++) {
        	ret = CAEN_DGTZ_GetEventInfo(handle, buffer, BufferSize, i, &EventInfo, &EventPtr);
			if (ret)
				sprintf(CName, "ERR_GET_EVENT_INFO");
			
            ret = CAEN_DGTZ_DecodeEvent(handle,EventPtr,(void**)&Event16);
			if (ret)
				sprintf(CName, "ERR_DECODE_EVENT");
					
			FillHisto(EventInfo); // all data performance
			
			if ( (get_time() - PrevDrawTime) >= Rcfg.DrawTime*1000 && Rcfg.Nev!=0){ // sec*1000 = ms 
				DrawHisto(Rcfg.can);
				PrevDrawTime = get_time();
			}
			ret = CAEN_DGTZ_FreeEvent(handle,(void**)&Event16);
				
			gSystem->ProcessEvents(); 
		}
			

		
    } // end of while readout loop		
				
	if(ret){
		printf("Error: %s\n", CName);
		new TGMsgBox(gClient->GetRoot(), Rcfg.main, "Error", CName, kMBIconStop, kMBOk);
		ret = QuitMain(handle, buffer);
	}
	
	return ret;
}


CAEN_DGTZ_ErrorCode DataAcquisition(int handle){
	CAEN_DGTZ_ErrorCode ret = CAEN_DGTZ_Success;
	char CName[30];
	
	while(1) {
		gSystem->ProcessEvents(); 
		if (Rcfg.loop == 1){
			ret = CAEN_DGTZ_SWStartAcquisition(handle);
			
			if (ret) {
				sprintf(CName, "ERR_START_ACQUISITION %i \n", ret);
				new TGMsgBox(gClient->GetRoot(), Rcfg.main, "Error", CName, kMBIconStop, kMBOk);
				//ret = QuitMain(handle, buffer);
				return ret;
			}
			ret = ReadoutLoop(handle);
		}
		
		if (Rcfg.loop == 0){
			ret = CAEN_DGTZ_SWStopAcquisition(handle);
			printf("Acquisition stopped \n");
			Rcfg.loop = -1;
		}
	}
	return ret;
}