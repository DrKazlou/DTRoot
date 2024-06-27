#include <stdlib.h>
#include <vector>

#include <TROOT.h>
#include <TClass.h>
#include <TApplication.h>
#include <TVirtualX.h>
#include <TVirtualPadEditor.h>
#include <TGResourcePool.h>
#include <TGListBox.h>
#include <TGListTree.h>
#include <TGFSContainer.h>
#include <TGClient.h>
#include <TGFrame.h>
#include <TGIcon.h>
#include <TGLabel.h>
#include <TGButton.h>
#include <TGTextEntry.h>
#include <TGNumberEntry.h>
#include <TGMsgBox.h>
#include <TGMenu.h>
#include <TGCanvas.h>
#include <TGComboBox.h>
#include <TGTab.h>
#include <TGSlider.h>
#include <TGStatusBar.h>
#include <TGDoubleSlider.h>
#include <TGFileDialog.h>
#include <TGTextEdit.h>
#include <TGShutter.h>
#include <TGProgressBar.h>
#include <TGColorSelect.h>
#include <RQ_OBJECT.h>
#include <TRootEmbeddedCanvas.h>
#include <TCanvas.h>
#include <TColor.h>
#include <TH1.h>
#include <TH2.h>
#include <TMath.h>
#include <TLatex.h>

#include <TRandom.h>
#include <TSystem.h>
#include <TSystemDirectory.h>
#include <TEnv.h>

#include "TFile.h"
#include "TTree.h"
#include "TBranch.h"
#include "TROOT.h"

#include <TKey.h>
#include <TGDockableFrame.h>
#include <TGFontDialog.h>


enum ETestCommandIdentifiers {
   M_FILE_OPEN,
   M_FILE_SAVE_HISTO,
   M_FILE_SAVE_TRACES,
   M_FILE_PRINT,
   M_FILE_PRINTSETUP,
   M_FILE_EXIT,

   M_OPT_MENU,	

   M_MANUAL,
   M_HELP_ABOUT,

};




class MainFrame {

RQ_OBJECT("MainFrame")

private:
   TGMainFrame        *fMain;
   TGDockableFrame    *fMenuDock;
   TGCanvas           *fCanvasWindow;
   TRootEmbeddedCanvas	*fEcanvas1;
   TRootEmbeddedCanvas	*fEcanvas2;	

   TGTextEntry        *fTestText;
   TGButton           *fTestButton1;
   TGButton           *fTestButton2;	
   TGColorSelect      *fColorSel;

	
	

	TGHorizontalFrame    *fFStore, *fF[8];
   TGLabel              *fSTLabel, *fLabel[8];
   TGNumberEntry        *fNumericEntries[8];
   
   TGTextEntry *fSTTextEntry;
   TGTextBuffer *fSTTextBuffer;
   
	//TGRadioButton *fR[4];
	TGCheckButton *fSTCheck, *fCTime,*fC[8], *fCa[6];	
	
	TGTextButton 	* fInitButton, *fClearButton, *fStartButton, *fStopButton;	
	
	//TGStatusBar *fStatusBar;
	
   TGMenuBar          *fMenuBar;
   TGPopupMenu        *fMenuFile, *fMenuOpt, *fMenuHelp;
  
   TGLayoutHints      *fMenuBarLayout, *fMenuBarItemLayout, *fMenuBarHelpLayout;

      
   //TCanvas 			*c1;
	
		
	
public:
	
   MainFrame(const TGWindow *p, UInt_t w, UInt_t h);
   virtual ~MainFrame();

   // slots
  	Color_t color[3] = {kBlue, kRed, kBlack}; 			
	
	char CName[300];
	const char *sFont = "-Ubuntu-bold-r-*-*-16-*-*-*-*-*-iso8859-1";
	
  
   void CloseWindow();
   void InitButton();
   void ClearHisto();	
   void StartButton();	
   void StopButton();
   
   void DoSetVal();
   void DoCheckBox();
  
   void HandleMenu(Int_t id);
   void HandlePopup() { printf("menu popped up\n"); }
   void HandlePopdown() { printf("menu popped down\n"); }

   void Created() { Emit("Created()"); } //*SIGNAL*
   void Welcome() { printf("DTRoot. Welcome on board!\n"); }
};
