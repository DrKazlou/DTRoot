#include "DTOpt.h"
#include "DTFunc.h"


extern Histograms_t Histo;
extern ReadoutConfig_t Rcfg;


OptMenu::OptMenu(const TGWindow *p, const TGWindow *main, UInt_t w, UInt_t h, UInt_t options)
{
   // Create message box test dialog. Use this dialog to select the different
   // message dialog box styles and show the message dialog by clicking the
   // "Test" button.

   fMain = new TGTransientFrame(p, main, w, h, options);
   fMain->Connect("CloseWindow()", "OptMenu", this, "CloseWindow()");
   fMain->DontCallClose(); // to avoid double deletions.

   // use hierarchical cleaning
   fMain->SetCleanup(kDeepCleanup);


   fMain->ChangeOptions((fMain->GetOptions() & ~kVerticalFrame) | kHorizontalFrame);

   f1 = new TGCompositeFrame(fMain, 300, 200, kVerticalFrame); //| kFixedWidth
	
	const char *titlelabel[] = {"Amplitude range search [ns]", "Integral range calc. [ns]", "PSD", "Waveforms"};		
	
	const char *numlabel[3][2] = { {"Xmin","Xmax"}, {"LBound","RBound"}, {"PSD_BIN", "do nothing" }  };	
	const char *numlabel2[] = { "Xmin","Xmax","Ymin", "Ymax" };	
	
	const Int_t numinit[4][4] = {
		{Histo.ALBound, Histo.ARBound, 2, 3},
		{Histo.ILBound, Histo.IRBound, 2, 3},
		{Histo.PSD_BIN, 500, -10000, 50},
		{Histo.WF_XMIN, Histo.WF_XMAX,Histo.WF_YMIN, Histo.WF_YMAX}
	};	

	int iStyle[]	= {0, 2}; 
	double Lmin[]	= {-20000, -20000, -20000,-20000}; 	
	double Lmax[]	= {20000, 20000, 20000, 20000}; 	

	for (int i = 0;i<4;i++){
		
		fGF[i] = new TGGroupFrame(f1, titlelabel[i], kVerticalFrame);
		fGF[i]->SetTitlePos(TGGroupFrame::kLeft); 
		f1->AddFrame(fGF[i], new TGLayoutHints(kLHintsTop | kLHintsLeft, 5, 5, 5, 5));//

   		// 2 column, n rows
   		fGF[i]->SetLayoutManager(new TGMatrixLayout(fGF[i], 0, 1, 10));
		
		if (i<3) {
			
   			fF[i][0] = new TGHorizontalFrame(fGF[i], 220, 30);
		   	fGF[i]->AddFrame(fF[i][0], new TGLayoutHints(kLHintsCenterY | kLHintsLeft, 2, 2, 2, 2));	
	
	 		for (int j = 0; j < 2; j++) { 
     			fNumericEntriesOpt[i][j] = new TGNumberEntry(fF[i][0], numinit[i][j], 6, 2*i + j  + 30,(TGNumberFormat::EStyle)  iStyle[0]); //
     			//fNumericEntriesOpt[i][j]->SetLimits(TGNumberFormat::kNELLimitMinMax, Lmin[0], Lmax[0]);
				fNumericEntriesOpt[i][j]->Connect("ValueSet(Long_t)", "OptMenu", this, "DoSetVal()");
				//fNumericEntriesOpt[i][j]->GetNumberEntry( )->Connect("ReturnPressed()", "OptMenu", this, "DoSetVal()");
	   
     			fF[i][0]->AddFrame(fNumericEntriesOpt[i][j], new TGLayoutHints(kLHintsCenterY | kLHintsLeft, 2, 2, 2, 2));
     			fLabel[i][j] = new TGLabel(fF[i][0], numlabel[i][j]);
     			fF[i][0]->AddFrame(fLabel[i][j], new TGLayoutHints(kLHintsCenterY | kLHintsLeft, 2, 2, 2, 2));
			}
		}
		else{
			for (int k=0;k<2;k++){
 	  			fF[i][k] = new TGHorizontalFrame(fGF[i], 220, 30);
			   	fGF[i]->AddFrame(fF[i][k], new TGLayoutHints(kLHintsCenterY | kLHintsCenterX, 2, 2, 2, 2));	
	
	 			for (int j = 0; j < 2; j++) { 
     				fNumericEntriesOpt[i][j+2*k] = new TGNumberEntry(fF[i][k], numinit[i][j+2*k], 6, 2*i + j +2*k + 30,(TGNumberFormat::EStyle)  iStyle[0]); //
     				fNumericEntriesOpt[i][j+2*k]->SetLimits(TGNumberFormat::kNELLimitMinMax, Lmin[0], Lmax[0]);
					fNumericEntriesOpt[i][j+2*k]->Connect("ValueSet(Long_t)", "OptMenu", this, "DoSetVal()");
					//fNumericEntriesOpt[i][j+2*k]->GetNumberEntry( )->Connect("ReturnPressed()", "OptMenu", this, "DoSetVal()");
	   
     				fF[i][k]->AddFrame(fNumericEntriesOpt[i][j+2*k], new TGLayoutHints(kLHintsCenterY | kLHintsLeft, 2, 2, 2, 2));
     				fLabel[i][j+2*k] = new TGLabel(fF[i][k], numlabel2[j+2*k]);
     				fF[i][k]->AddFrame(fLabel[i][j+2*k], new TGLayoutHints(kLHintsCenterY | kLHintsLeft, 2, 2, 2, 2));
				}			
			
			
			}
		
		}
			
		fGF[i]->Resize();	
	}
	
	const char *opt_label[] = { "OptStat","OptFit","Print", "TH2D AP", "TH2D COLZ" };	
	
	TGHorizontalFrame *fHFOpt = new TGHorizontalFrame(f1, 200, 60);
	
	fVF[0] = new TGVerticalFrame(fHFOpt, 40, 30);
	fVF[1] = new TGVerticalFrame(fHFOpt, 40, 30);
	
	for (int i=0; i<5; i++){
		if (i<3){
			fC[i] = new TGCheckButton(fVF[0], new TGHotString(opt_label[i]), 10 + i);	
			fVF[0]->AddFrame(fC[i], new TGLayoutHints(kLHintsTop | kLHintsLeft, 2, 2, 2, 2) ); //(kLHintsTop | kLHintsExpandX, 2, 2, 3, 0)
		}	
		else{
			fC[i] = new TGCheckButton(fVF[1], new TGHotString(opt_label[i]), 10 + i);	
			fVF[1]->AddFrame(fC[i], new TGLayoutHints(kLHintsTop | kLHintsLeft, 2, 2, 2, 2) ); //(kLHintsTop | kLHintsExpandX, 2, 2, 3, 0)
		}	
		fC[i]->Connect("Clicked()", "OptMenu", this, "DoCheckBox()");
	}
		
	fC[4]->SetState(kButtonDown);
	
	fVF[0]->Resize( );
	fVF[1]->Resize( );
	fHFOpt->AddFrame(fVF[0], new TGLayoutHints(kLHintsTop | kLHintsLeft, 2, 2, 2, 2) ); //(kLHintsTop | kLHintsExpandX, 2, 2, 3, 0)
	fHFOpt->AddFrame(fVF[1], new TGLayoutHints(kLHintsTop | kLHintsRight, 2, 2, 2, 2) ); //(kLHintsTop | kLHintsExpandX, 2, 2, 3, 0)
	
	f1->AddFrame(fHFOpt, new TGLayoutHints(kLHintsTop | kLHintsLeft, 2, 2, 2, 2) ); //(kLHintsTop | kLHintsExpandX, 2, 2, 3, 0)
	
	f1->Resize();

	fMain->AddFrame(f1, new TGLayoutHints(kLHintsTop | kLHintsRight, 2, 5, 10, 0) );

  


   fMain->MapSubwindows();
   fMain->Resize();

   // position relative to the parent's window
   fMain->CenterOnParent();

   fMain->SetWindowName("Hist options");

   fMain->MapWindow();
   
   //gSystem->ProcessEvents(); 
   //gClient->WaitFor(fMain);
}

// Order is important when deleting frames. Delete children first,
// parents last.

OptMenu::~OptMenu()
{

   fMain->DeleteWindow();  // deletes fMain
}

void OptMenu::CloseWindow()
{
   // Close dialog in response to window manager close.

   delete this;
}

void OptMenu::DoClose()
{
    //Handle Close button.
	
   CloseWindow();
}

void OptMenu::DoSetVal()
{
	Histo.ALBound = fNumericEntriesOpt[0][0]->GetNumber();
	Histo.ARBound = fNumericEntriesOpt[0][1]->GetNumber();
	Histo.ILBound = fNumericEntriesOpt[1][0]->GetNumber();
	Histo.IRBound = fNumericEntriesOpt[1][1]->GetNumber();
	
    Histo.PSD_BIN = fNumericEntriesOpt[2][0]->GetNumber();	
	
	Histo.WF_XMIN = fNumericEntriesOpt[3][0]->GetNumber();	
	Histo.WF_XMAX = fNumericEntriesOpt[3][1]->GetNumber();
	Histo.WF_YMIN = fNumericEntriesOpt[3][2]->GetNumber();	
	Histo.WF_YMAX = fNumericEntriesOpt[3][3]->GetNumber();
	
   //CloseWindow();
}

void OptMenu::DoCheckBox(){
	
	TGButton *btn = (TGButton *) gTQSender;
	Int_t id = btn->WidgetId();
	
	//OptStat
	if (id == 10 ) 
		fC[0]->GetState() == kButtonDown ? gStyle->SetOptStat(1111) : gStyle->SetOptStat(0);
	
	//OptFit     	
	if (id == 11) 
		fC[1]->GetState() == kButtonDown ? gStyle->SetOptFit(1111) : gStyle->SetOptFit(0);
	
	//fPrint	
	if (id == 12) 
		fC[2]->GetState() == kButtonDown ? Rcfg.fPrint = true : Rcfg.fPrint = false;
		
	if (id == 13) { // DrawOption  with marker
		if (fC[3]->GetState() == kButtonDown){
			sprintf(Histo.h2Style, "%s", "");
			fC[4]->SetState(kButtonUp);
		}	
		else{
			fC[4]->SetState(kButtonDown);
			sprintf(Histo.h2Style, "COLZ");
		}	
	}	
	
	if (id == 14) { // DrawOption  COLZ
		if (fC[4]->GetState() == kButtonDown){
			sprintf(Histo.h2Style, "COLZ");
			fC[3]->SetState(kButtonUp);
		}	
		else{
			fC[3]->SetState(kButtonDown);
			sprintf(Histo.h2Style, "%s", "");
		}	
	}	
  
	
	
}

void OptMenu::TryToClose()
{
   // The user try to close the main window,
   //  while a message dialog box is still open.
   printf("Can't close the window '%s' : a message box is still open\n", fMain->GetWindowName());
}
