#include "DTFrame.h"
#include "DTOpt.h"
#include "DTFunc.h"
#include "DTReadout.h"

#include <vector>
#include "TGWindow.h"

extern WaveDumpConfig_t WDcfg;
extern ReadoutConfig_t Rcfg;
extern Histograms_t Histo;
extern CAEN_DGTZ_ErrorCode ret;

extern int handle;

extern CAEN_DGTZ_UINT16_EVENT_t *Event16;

using namespace std;	
		

   
char *buffer = NULL;
		
	
    //uint32_t cal_thr[2];


MainFrame::MainFrame(const TGWindow *p, UInt_t w, UInt_t h)
{
	
	fMain = new TGMainFrame(p, w, h);
	Rcfg.main = fMain;

   // use hierarchical cleaning
   fMain->SetCleanup(kDeepCleanup);

   fMain->Connect("CloseWindow()", "MainFrame", this, "CloseWindow()");

   // Create menubar and popup menus. The hint objects are used to place
   // and group the different menu widgets with respect to eachother.
   fMenuDock = new TGDockableFrame(fMain);
   fMain->AddFrame(fMenuDock, new TGLayoutHints(kLHintsExpandX, 0, 0, 1, 0));
   fMenuDock->SetWindowName("DTRoot Menu");

   fMenuBarLayout = new TGLayoutHints(kLHintsTop | kLHintsExpandX);
   fMenuBarItemLayout = new TGLayoutHints(kLHintsTop | kLHintsLeft, 0, 4, 0, 0);
   fMenuBarHelpLayout = new TGLayoutHints(kLHintsTop | kLHintsRight);

   fMenuFile = new TGPopupMenu(gClient->GetRoot());
   fMenuFile->AddEntry("&Open...", M_FILE_OPEN);
   fMenuFile->AddEntry("Save &histo", M_FILE_SAVE_HISTO);
   fMenuFile->AddSeparator();
   fMenuFile->AddEntry("E&xit", M_FILE_EXIT);
	
	fMenuFile->DisableEntry(M_FILE_OPEN);
	fMenuFile->DisableEntry(M_FILE_SAVE_TRACES);
   
   fMenuOpt = new TGPopupMenu(gClient->GetRoot());
   fMenuOpt->AddEntry("&Hist options", M_OPT_MENU);


   fMenuHelp = new TGPopupMenu(gClient->GetRoot());
   fMenuHelp->AddEntry("&Manual", M_MANUAL);
   fMenuHelp->AddEntry("&About", M_HELP_ABOUT);

   // Menu button messages are handled by the main frame (i.e. "this")
   // HandleMenu() method.
	fMenuFile->Connect("Activated(Int_t)", "MainFrame", this, "HandleMenu(Int_t)");
	fMenuOpt->Connect("Activated(Int_t)", "MainFrame", this, "HandleMenu(Int_t)");
	fMenuHelp->Connect("Activated(Int_t)", "MainFrame", this, "HandleMenu(Int_t)");

   fMenuBar = new TGMenuBar(fMenuDock, 1, 1, kHorizontalFrame);
   fMenuBar->AddPopup("&File", fMenuFile, fMenuBarItemLayout);
   fMenuBar->AddPopup("&Options", fMenuOpt, fMenuBarItemLayout);
   fMenuBar->AddPopup("&Help", fMenuHelp, fMenuBarHelpLayout);

   fMenuDock->AddFrame(fMenuBar, fMenuBarLayout);


	TGHorizontalFrame *hframe1 = new TGHorizontalFrame(fMain,200,40);
	TGVerticalFrame *vframe1 = new TGVerticalFrame(hframe1,200,40);		
    hframe1->AddFrame(vframe1, new TGLayoutHints(kLHintsTop | kLHintsLeft, 5, 5, 5, 5));//
	
	TGGroupFrame *gframe_store = new TGGroupFrame(hframe1, " - ", kVerticalFrame);
	gframe_store->SetTitlePos(TGGroupFrame::kRight); // right aligned
	vframe1->AddFrame(gframe_store, new TGLayoutHints(kLHintsTop | kLHintsLeft, 5, 5, 5, 5));//
	
	fSTCheck  = new TGCheckButton(gframe_store, new TGHotString("STORE TRACES"), 40);	
	fSTCheck->Connect("Clicked()", "MainFrame", this, "DoCheckBox()");
	gframe_store->AddFrame(fSTCheck, new TGLayoutHints(kLHintsCenterY | kLHintsLeft, 2, 2, 2, 2));
	
	fFStore = new TGHorizontalFrame(gframe_store, 200, 40);
	gframe_store->AddFrame(fFStore, new TGLayoutHints(kLHintsCenterY | kLHintsLeft, 2, 2, 2, 2));
	fSTLabel = new TGLabel(fFStore, "File name");
	fFStore->AddFrame(fSTLabel, new TGLayoutHints(kLHintsCenterY | kLHintsLeft, 2, 2, 2, 2));
	
	//fSTTextEntry = new TGTextEntry(fFStore, fSTTextBuffer = new TGTextBuffer(0)) ;
	fSTTextEntry = new TGTextEntry(gframe_store, fSTTextBuffer = new TGTextBuffer(0)) ;
	fSTTextBuffer->AddText(0, "output.root");
	fSTTextEntry->SetEnabled(0);
	fSTTextEntry->Resize(100, fSTTextEntry->GetDefaultHeight());
	gframe_store->AddFrame(fSTTextEntry, new TGLayoutHints(kLHintsCenterY | kLHintsLeft | kLHintsExpandX, 0, 2, 2, 2));
	
	gframe_store->Resize();
	
	TGGroupFrame *gframe_opt = new TGGroupFrame(hframe1, "Options", kVerticalFrame);
	gframe_opt->SetTitlePos(TGGroupFrame::kRight); // right aligned
	vframe1->AddFrame(gframe_opt, new TGLayoutHints(kLHintsTop | kLHintsLeft, 5, 5, 5, 5));//

   // 2 column, n rows
   gframe_opt->SetLayoutManager(new TGMatrixLayout(gframe_opt, 0, 1, 10));

const char *numlabel[] = {
   "BLine_cut",
   "[1] Threshold",
   "[2] Threshold",
   "DrawTime",
	"CH_2D",
	"Timer"
	};	
	
const Double_t numinit[] = {
   20, (double)WDcfg.Threshold[0], (double)WDcfg.Threshold[1], Rcfg.DrawTime, 0, 300
};	

	
int iStyle[]	= {0, 0, 0, 2, 0,0}; 	
	
   for (int i = 0; i < 6; i++) {
      fF[i] = new TGHorizontalFrame(gframe_opt, 200, 30);
      gframe_opt->AddFrame(fF[i], new TGLayoutHints(kLHintsCenterY | kLHintsLeft, 2, 2, 2, 2));
      fNumericEntries[i] = new TGNumberEntry(fF[i], numinit[i], 8, i + 20, (TGNumberFormat::EStyle) iStyle[i]); //numinit[i], 7, i + 20, (TGNumberFormat::EStyle) iStyle[i]
	  fNumericEntries[i]->Connect("ValueSet(Long_t)", "MainFrame", this, "DoSetVal()");
      fF[i]->AddFrame(fNumericEntries[i], new TGLayoutHints(kLHintsCenterY | kLHintsLeft, 2, 2, 2, 2));
      fLabel[i] = new TGLabel(fF[i], numlabel[i]);
      fF[i]->AddFrame(fLabel[i], new TGLayoutHints(kLHintsCenterY | kLHintsLeft, 2, 2, 2, 2));
	  if ( i == 4)		  
	  	fNumericEntries[i]->SetLimits(TGNumberFormat::kNELLimitMinMax, 0, 1); // channels can be only [0] || [1]
	  if ( i == 5 ) {
		  fNumericEntries[i]->SetState(kFALSE);
		  fCTime = new TGCheckButton(fF[i], new TGHotString(""), 20);	
		  fCTime->Connect("Clicked()", "MainFrame", this, "DoCheckBox()");
		  fF[i]->AddFrame(fCTime, new TGLayoutHints(kLHintsCenterY | kLHintsLeft, 2, 2, 2, 2));
	  } 
   }
	
	WDcfg.thr[0] = fNumericEntries[1]->GetNumber();
	WDcfg.thr[1] = fNumericEntries[2]->GetNumber();
	
   
   gframe_opt->Resize();
   
   TGGroupFrame *gframe_ch = new TGGroupFrame(hframe1, "Channels", kVerticalFrame);
	gframe_ch->SetTitlePos(TGGroupFrame::kRight); 
	vframe1->AddFrame(gframe_ch, new TGLayoutHints(kLHintsTop | kLHintsLeft, 5, 5, 5, 5));//
   
   
	
	Pixel_t p_color[5];
	gClient->GetColorByName("blue", p_color[0]);
	gClient->GetColorByName("red", p_color[1]);
	gClient->GetColorByName("blue", p_color[2]);
	gClient->GetColorByName("red", p_color[3]);
	gClient->GetColorByName("black", p_color[4]);
	
	const char *cb_label[] = {"CH1", "CH2",  "SelfTrg1", "SelfTrg2", "Coincidence"};	

	for (int i = 0; i < 5; ++i) {
		
		fCa[i] = new TGCheckButton(gframe_ch, cb_label[i],	21 + i);
		fCa[i]->SetTextColor(p_color[i]);
		if (i != 4)
			fCa[i]->SetState(kButtonDown); 
		fCa[i]->SetState(kButtonDisabled); 
		fCa[i]->Connect("Clicked()", "MainFrame", this, "DoCheckBox()");
		gframe_ch->AddFrame(fCa[i], new TGLayoutHints(kLHintsTop | kLHintsLeft, 0, 0, 5, 0));
	}
		
		//fCa[4]->SetState(kButtonDisabled); //Coincidence Initialy Disabled
 
	gframe_ch->SetLayoutManager(new TGMatrixLayout(gframe_ch, 0, 2, 3));
	gframe_ch->Resize(); 
   
	TGGroupFrame *gframe_hist = new TGGroupFrame(hframe1, "Hist", kVerticalFrame);
	gframe_hist->SetTitlePos(TGGroupFrame::kRight); 
	vframe1->AddFrame(gframe_hist, new TGLayoutHints(kLHintsTop | kLHintsLeft, 5, 5, 5, 5));//

	const char *cblabel[] = {"BL_CUT", "TRACES", "AMPL_HIST", "INTEGRAL", "dT", "Int vs Ampl", "PSD_ampl"};	
	
	for (int i = 0; i < 7; i++) {
		fC[i] = new TGCheckButton(gframe_hist, new TGHotString(cblabel[i]), 41+i);	
		fC[i]->Connect("Clicked()", "MainFrame", this, "DoCheckBox()");
		gframe_hist->AddFrame(fC[i], new TGLayoutHints(kLHintsCenterY | kLHintsLeft, 2, 2, 2, 2));
	}	
		
	fC[0]->SetState(kButtonDown); //BL_CUT ON
	fC[1]->SetState(kButtonDown); //TRACES ON
		 
	 gframe_hist->Resize();
	 
    fInitButton = new TGTextButton(vframe1, " In&it ", 1);
    fInitButton->SetFont(sFont); 
    fInitButton->Resize(60, 30);
    fInitButton->Connect("Clicked()","MainFrame",this,"InitButton()");
    vframe1->AddFrame( fInitButton, new TGLayoutHints(kLHintsCenterY | kLHintsLeft, 4, 4, 4, 4));	
	
   fClearButton = new TGTextButton(vframe1, " Cle&ar ", 1);
   fClearButton->SetFont(sFont);
   fClearButton->Resize(60, 30);
   fClearButton->SetState (kButtonDisabled);
   fClearButton->Connect("Clicked()","MainFrame",this,"ClearHisto()");
   vframe1->AddFrame(fClearButton, new TGLayoutHints(kLHintsCenterY | kLHintsLeft, 4, 4, 4, 4));	
	
	Rcfg.TLabel = new TGLabel(vframe1, "          Timer          ");
	Rcfg.TLabel->SetTextFont(sFont);
	Rcfg.TLabel->Resize(200, 30);
	
	vframe1->AddFrame(Rcfg.TLabel, new TGLayoutHints(kLHintsCenterY | kLHintsLeft, 2, 2, 2, 2));
	
	vframe1->Resize();
	
	fEcanvas1 = new TRootEmbeddedCanvas("Ecanvas1", hframe1, WDcfg.windx - 300, WDcfg.windy - 120); // 1600 780
	hframe1->AddFrame(fEcanvas1, new TGLayoutHints(kLHintsCenterX, 10,5,25,0));//kLHintsExpandX |   kLHintsExpandY
	hframe1->Resize();

	
    fMain->AddFrame(hframe1, new TGLayoutHints(kLHintsCenterX, 2, 2 , 2, 2) );
	
	Rcfg.can = fEcanvas1->GetCanvas();
		
	// status bar
	Int_t parts[] = {13, 13, 13, 22, 39};
	Rcfg.StatusBar = new TGStatusBar(fMain, 100, 20, kHorizontalFrame); //kHorizontalFrame //kSunkenFrame
	Rcfg.StatusBar->SetParts(parts,5);
	fMain->AddFrame(Rcfg.StatusBar, new TGLayoutHints(kLHintsBottom | kLHintsLeft | kLHintsExpandX, 0, 0, 2, 0));
	
	
	TGHorizontalFrame *hframe2 = new TGHorizontalFrame(fMain, 200, 40);
 	fStartButton = new TGTextButton(hframe2," Sta&rt ", 1);
	fStartButton->SetFont(sFont);
    fStartButton->Resize(60, 30);
	fStartButton->SetState (kButtonDisabled);
  	fStartButton->Connect("Clicked()","MainFrame", this, "StartButton()");
  	hframe2->AddFrame(fStartButton, new TGLayoutHints(kLHintsCenterY |  kLHintsExpandX, 4, 4, 4, 4));

	fStopButton = new TGTextButton(hframe2,"  S&top  ", 1);
    fStopButton->SetFont(sFont);
    fStopButton->Resize(60, 30);
	fStopButton->SetState (kButtonDisabled);
	fStopButton->Connect("Clicked()","MainFrame",this,"StopButton()");	
    hframe2->AddFrame(fStopButton, new TGLayoutHints(kLHintsCenterY | kLHintsLeft | kLHintsExpandX, 4, 4, 4, 4));
	
	hframe2->Resize();
	

	
   fMain->AddFrame(hframe2, new TGLayoutHints(kLHintsCenterX,       2, 2, 20, 2));
   fMain->SetWindowName("DTRoot");
   fMain->MapSubwindows( );

	fMain->Resize( );
	fMain->MapWindow( );
   //fMain->Print();
   Connect("Created()", "MainFrame", this, "Welcome()");
   Created( );
}

MainFrame::~MainFrame()
{
   // Delete all created widgets.

   delete fMenuFile;

   delete fMain;
}

void MainFrame::CloseWindow()
{
   // Got close message for this MainFrame. Terminates the application.
	ret = QuitMain(handle, buffer);
	
   gApplication->Terminate();
}

void MainFrame::DoCheckBox(){
	
	TGButton *btn = (TGButton *) gTQSender;
	Int_t id = btn->WidgetId();
	
	//Store traces checkbox
	if (id == 40){ 
		fSTTextEntry->SetEnabled(fSTCheck->GetState() == kButtonDown ? 1 : 0);
		Rcfg.fStoreTraces = fSTCheck->GetState( ) == kButtonDown ? true : false;
	}	
	
	 //Timer checkbox
	if (id == 20 ) {
	   fNumericEntries[5]->SetState( fCTime->GetState() == kButtonDown ? kTRUE : kFALSE );
	   Rcfg.fTimer = fCTime->GetState( ) == kButtonDown ? true : false;
	}  
	
	if (id == 41)
		Histo.fBL = fC[0]->GetState() == kButtonDown ? true : false;
   	if (id > 41){ 
		Histo.fTrace = fC[1]->GetState() == kButtonDown ? true : false;
		Histo.fAmpl = fC[2]->GetState() == kButtonDown ? true : false;
		Histo.fInt = fC[3]->GetState() == kButtonDown ? true : false;
		Histo.fdT = fC[4]->GetState() == kButtonDown ? true : false;
		Histo.fIA = fC[5]->GetState() == kButtonDown ? true : false;
		Histo.fPSD_ampl = fC[6]->GetState() == kButtonDown ? true : false;
		
	
		fC[id-41]->GetState() == kButtonDown ? Histo.NPad++ : Histo.NPad--;
		
		if (Histo.fAmpl)
			Histo.cAmpl = 1 + (Histo.fTrace ? 1 : 0);
					
		if (Histo.fInt)
			Histo.cInt = 1 + (Histo.fTrace ? 1 : 0) + (Histo.fAmpl ? 1 : 0);
				
		if (Histo.fdT)	
			Histo.cdT = 1 + (Histo.fTrace ? 1 : 0) + (Histo.fAmpl ? 1 : 0) + (Histo.fInt ? 1 : 0);
		
		if (Histo.fIA)	
			Histo.cIA = 1 + (Histo.fTrace ? 1 : 0) + (Histo.fAmpl ? 1 : 0) + (Histo.fInt ? 1 : 0) + (Histo.fdT ? 1 : 0);
		
		if (Histo.fPSD_ampl)	
			Histo.cPSD_ampl = 1 + (Histo.fTrace ? 1 : 0) + (Histo.fAmpl ? 1 : 0) + (Histo.fInt ? 1 : 0) + (Histo.fdT ? 1 : 0) + (Histo.fIA ? 1 : 0);
		
		
		
		Rcfg.can->Clear();
		Rcfg.can->SetGrid( );
		
		
		if (Histo.NPad == 1)
			Rcfg.can->Divide(1, 1, 0.001, 0.001);
		if (Histo.NPad == 2)
			Rcfg.can->Divide(2, 1, 0.001, 0.001);
		if (Histo.NPad > 2 && Histo.NPad < 5)
			Rcfg.can->Divide(2, 2, 0.001, 0.001);
		if (Histo.NPad > 4 && Histo.NPad < 7)
			Rcfg.can->Divide(3, 2, 0.001, 0.001);
	
		Rcfg.can->Modified();
		
	}
		
	
	if (id == 21 ){
		Histo.fDraw[0]	= fCa[0]->GetState() == kButtonDown ? true : false;
		if (fCa[0]->GetState() == kButtonDown){
			printf("Enable CH1\n");
			ret = CAEN_DGTZ_SetChannelEnableMask(handle, 3); // // 11 - CH1 - ON, CH2 - ON
			//ret = CAEN_DGTZ_SetChannelSelfTrigger(handle, CAEN_DGTZ_TRGMODE_ACQ_ONLY, (1 << 0));
			//ret = CAEN_DGTZ_SetChannelSelfTrigger(handle, CAEN_DGTZ_TRGMODE_ACQ_ONLY, (1 << 1));
		}	
		if (fCa[0]->GetState() == kButtonUp){
			printf("Disable CH1\n");
			//ret = CAEN_DGTZ_SetChannelSelfTrigger(handle, CAEN_DGTZ_TRGMODE_DISABLED, (1 << 0));
			printf("Enable CH2\n");
			if (fCa[1]->GetState() == kButtonUp)
				ret = CAEN_DGTZ_SetChannelEnableMask(handle, 2); // 10 - CH1 - OFF, CH2 - ON
			fCa[1]->SetState(kButtonDown);
			
		}	
		
	}
	
	if (id == 22 ){
		Histo.fDraw[1]	= fCa[1]->GetState() == kButtonDown ? true : false;
		if (fCa[1]->GetState() == kButtonDown){
			printf("Enable CH2\n");
			ret = CAEN_DGTZ_SetChannelEnableMask(handle, 3); // 11 - CH1 - ON, CH2 - ON
			//ret = CAEN_DGTZ_SetChannelSelfTrigger(handle, CAEN_DGTZ_TRGMODE_ACQ_ONLY, (1 << 0));
			//ret = CAEN_DGTZ_SetChannelSelfTrigger(handle, CAEN_DGTZ_TRGMODE_ACQ_ONLY, (1 << 1));
		}	
		if (fCa[1]->GetState() == kButtonUp){
			printf("Disable CH2\n");
			//ret = CAEN_DGTZ_SetChannelSelfTrigger(handle, CAEN_DGTZ_TRGMODE_DISABLED, (1 << 1));
			printf("Enable CH1\n");
			if (fCa[0]->GetState() == kButtonUp)
				ret = CAEN_DGTZ_SetChannelEnableMask(handle, 1); // 01 - CH1 - ON, CH2 - OFF
			fCa[0]->SetState(kButtonDown);
			
		}	
		
	}
		
	if (id == 23)	
		ret = CAEN_DGTZ_SetChannelSelfTrigger(handle, fCa[2]->GetState() == kButtonDown ? CAEN_DGTZ_TRGMODE_ACQ_ONLY : CAEN_DGTZ_TRGMODE_DISABLED, (1 << 0));
		
	if (id == 24)	
		ret = CAEN_DGTZ_SetChannelSelfTrigger(handle, fCa[2]->GetState() == kButtonDown ? CAEN_DGTZ_TRGMODE_ACQ_ONLY : CAEN_DGTZ_TRGMODE_DISABLED, (1 << 1));
	
	if (id == 25 ) {
		uint32_t reg_data = 3, shift = 1<<24;
		if (fCa[4]->GetState() == kButtonDown){
		   
		   reg_data = reg_data | shift;
		   ret = CAEN_DGTZ_WriteRegister(handle, 0x810C, reg_data);
		   ret = CAEN_DGTZ_ReadRegister(handle, 0x810C, &reg_data);
		   printf(" Coincidence ON 0x810C: %i \n", reg_data);
	   }    
		else{
			ret = CAEN_DGTZ_WriteRegister(handle, 0x810C, 3);
			ret = CAEN_DGTZ_ReadRegister(handle, 0x810C, &reg_data);
			printf(" Coincidence OFF 0x810C: %i \n", reg_data);
		}	
	}
	
}

void MainFrame::DoSetVal(){
		
	Histo.BL_CUT = fNumericEntries[0]->GetNumber();
	WDcfg.thr[0] = (int)fNumericEntries[1]->GetNumber();
	WDcfg.thr[1] = (int)fNumericEntries[2]->GetNumber();
	Rcfg.DrawTime = fNumericEntries[3]->GetNumber();
	Histo.CH_2D = fNumericEntries[4]->GetNumber();
	Rcfg.timer_val = fNumericEntries[5]->GetNumber();
	
	
   	for (int i=0; i<2; i++){
		Int_t p = WDcfg.PulsePolarity[i] == CAEN_DGTZ_TriggerOnRisingEdge ? 1 : -1;  // to correctly handle both polarity
		printf(" Trigger for CH[%i] will be: %i  (cal_thr[%i]: %i)\n", i, WDcfg.cal_thr[i] + p * WDcfg.thr[i], i, WDcfg.cal_thr[i]); 
		ret = CAEN_DGTZ_SetChannelTriggerThreshold(handle, i, WDcfg.cal_thr[i] + p * WDcfg.thr[i]);
		if (ret) {
			new TGMsgBox(gClient->GetRoot(), fMain, "Error", "SET_CHANNEL_TRIGGER_THRESHOLD_FAILURE \n", kMBIconStop, kMBOk);
			ret = QuitMain(handle, buffer);
		}
	}
	
}


void MainFrame::InitButton()
{
		
	ret = InitialiseDigitizer(handle, WDcfg, Rcfg);
		
	InitHisto();
	
	//enable buttons after DIGI initialisation
	fClearButton->SetState (kButtonUp);
	fStartButton->SetState (kButtonUp);
	fStopButton->SetState (kButtonUp);
	for (int i = 0; i<4; i++)
		fCa[i]->SetState(kButtonDown); // Coincidence CheckBox Enabled
	fCa[4]->SetState(kButtonUp); // Coincidence CheckBox Enabled
	
	fInitButton->SetState (kButtonDisabled);
}

void MainFrame::ClearHisto()
{

	for (int ch = 0; ch < MAX_CH; ch++){		
		Histo.ampl[ch]->Reset("ICESM");
		Histo.integral[ch]->Reset("ICESM");
	}		
	Histo.dt->Reset("ICESM");
	Histo.psd_ampl->Reset("ICESM");
	Histo.int_ampl->Reset("ICESM");
	
	printf("ClearHisto \n");
	
	Rcfg.StartTime = get_time( );
	
}

void MainFrame::StartButton()
{	
	
 	bool fStart = true;
	Rcfg.StartTime = get_time();
	printf("Start button \n");

	
	//Store traces if choosen
	if (Rcfg.fStoreTraces){
				
		int retval;
		// check if such file exist
		
		if( !gSystem->AccessPathName( fSTTextBuffer->GetString( ) ) ){ //such file exist
			sprintf(CName, "File  %s exist. \n It will be overwritten. \n Continue?", fSTTextBuffer->GetString( ));
			new TGMsgBox(gClient->GetRoot(), fMain, "Warning", CName, kMBIconExclamation, kMBNo | kMBYes, &retval); //1 - Yes, No -2 // strange logic
			printf("retval %d \n", retval);
			retval == 2 ? fStart = false: fStart = true; 
			
			if (fStart){
				sprintf(CName, "Traces will be saved in  \n %s", fSTTextBuffer->GetString( ));
				new TGMsgBox(gClient->GetRoot(), fMain, "Info", CName, kMBIconAsterisk, kMBOk);
				Rcfg.ff = TFile::Open(fSTTextBuffer->GetString(),"RECREATE");		
				//ff = new TFile(fSTTextBuffer->GetString(),"WRITE");		
				Rcfg.tree = new TTree("vtree", "vtree");		
				Rcfg.tree->Branch("EventCounter", &Histo.EC);
				Rcfg.tree->Branch("TimeStamp", &Histo.TTT);
				Rcfg.tree->Branch("Trace", &Histo.v_out);	 
			}
		} 
		else{
			sprintf(CName, "Traces will be saved in  \n %s", fSTTextBuffer->GetString( ));
			new TGMsgBox(gClient->GetRoot(), fMain, "Info", CName, kMBIconAsterisk, kMBOk);
			Rcfg.ff = TFile::Open(fSTTextBuffer->GetString(),"RECREATE");		
				//ff = new TFile(fSTTextBuffer->GetString(),"WRITE");		
			Rcfg.tree = new TTree("vtree", "vtree");		
			Rcfg.tree->Branch("EventCounter", &Histo.EC);
			Rcfg.tree->Branch("TimeStamp", &Histo.TTT);
			Rcfg.tree->Branch("Trace", &Histo.v_out);	 
		}
	}
	
	Rcfg.loop = 1;
		
	//ret = CAEN_DGTZ_SWStartAcquisition(handle);
}

void MainFrame::StopButton()
{	
	printf("Stop button \n");
	
	
	Rcfg.loop = 0;
	//ret = CAEN_DGTZ_SWStopAcquisition(handle);
	
	if (Rcfg.fStoreTraces){
		Rcfg.tree->Write();
		Rcfg.ff->Write();
		printf("Data saved as \"%s\"\n", Rcfg.ff->GetName());
	}
}




void MainFrame::HandleMenu(Int_t id)
{
   // Handle menu items.
	const char *filetypes[] = { "All files",     "*",
                            "ROOT files",    "*.root",
                            "ROOT macros",   "*.C",
                            "Text files",    "*.[tT][xX][tT]",
                            0,               0 };	

   switch (id) {

      case M_FILE_OPEN:
         {
            static TString dir(".");
            TGFileInfo fi;
            fi.fFileTypes = filetypes;
            fi.fIniDir    = StrDup(dir);
            //printf("fIniDir = %s\n", fi.fIniDir);
            new TGFileDialog(gClient->GetRoot(), fMain, kFDOpen, &fi);
            printf("Open file: %s (dir: %s)\n", fi.fFilename, fi.fIniDir);
            dir = fi.fIniDir;
         }
         break;

    case M_FILE_SAVE_HISTO:
		{
			const char *filetypes[] = { "All files",     "*",
                            "ROOT files",    "*.root",
                            "ROOT macros",   "*.C",
                            "Text files",    "*.[tT][xX][tT]",
                            0,               0 }; 
			 
		 	static TString dir(".");
            TGFileInfo fi;
            fi.fFileTypes = filetypes;
            fi.fIniDir    = StrDup(dir);
            new TGFileDialog(gClient->GetRoot(), fMain, kFDSave, &fi);
                      
		 	TFile *outfile = new TFile(fi.fFilename,"RECREATE");
			Rcfg.can->Write("can");
			for (int i = 0; i<MAX_CH; i++){
				Histo.ampl[i]->Write(Histo.ampl[i]->GetTitle());
				Histo.integral[i]->Write(Histo.integral[i]->GetTitle());
			}	
			Histo.dt->Write(Histo.dt->GetTitle());
			Histo.int_ampl->Write(Histo.int_ampl->GetTitle());
			Histo.psd_ampl->Write(Histo.psd_ampl->GetTitle());
						
			outfile->Write(); 
         	printf("File saved - %s \n",fi.fFilename);			 
		 }
        break;
	
    case M_FILE_EXIT:
        CloseWindow();   // terminate theApp no need to use SendCloseMessage()
        break;
	 
	case M_OPT_MENU:
        new OptMenu(gClient->GetRoot(), fMain, 400, 200);
		break; 	 
	
	case M_MANUAL:
		new TGMsgBox(gClient->GetRoot(), fMain, "Manual", "After a while it will be added \n but noone knows value of while \n because it's a loop", kMBIconAsterisk, kMBOk);
        break;
	
	case M_HELP_ABOUT:
		new TGMsgBox(gClient->GetRoot(), fMain, "About program", "Handmade spectra and waveform reader \n for CAEN DT5720", kMBIconAsterisk, kMBOk);
        break;
 
      default:
         printf("Menu item %d selected\n", id);
         break;
   }
}


