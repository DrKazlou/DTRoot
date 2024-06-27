#include <stdlib.h>


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

#include "TROOT.h"
#include "TStyle.h"

#include <TRandom.h>
#include <TSystem.h>
#include <TSystemDirectory.h>
#include <TEnv.h>
#include <TFile.h>
#include <TKey.h>
#include <TGDockableFrame.h>
#include <TGFontDialog.h>

class OptMenu {
	
RQ_OBJECT("OptMenu")

private:
   TGTransientFrame     *fMain;
   TGCompositeFrame     *f1, *f2, *f3;
   TGVerticalFrame  *fVF[2];
   TGCheckButton *fC[5];	
   TGTextButton      *fCloseButton;
   
	
	TGGroupFrame *fGF[4];	
	TGHorizontalFrame    *fF[4][2];
	TGLabel              *fLabel[4][4];
    TGNumberEntry        *fNumericEntriesOpt[4][4];
	
   	

public:
   OptMenu(const TGWindow *p, const TGWindow *main, UInt_t w, UInt_t h,
              UInt_t options = kVerticalFrame);

   virtual ~OptMenu();

	const char *sFont = "-Ubuntu-bold-r-*-*-16-*-*-*-*-*-iso8859-1";
	const char *labelFont = "-Ubuntu-bold-r-*-*-14-*-*-*-*-*-iso8859-1";
	const char *paramFont = "-adobe-courier-bold-r-*-*-15-*-*-*-*-*-iso8859-1";
	
   // slots
   void TryToClose();
   void CloseWindow();
   void DoClose();
 
   void DoCheckBox();
   void DoSetVal();
};
