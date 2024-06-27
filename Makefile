########################################################################

ROOTINCL  := $(shell root-config --incdir)
ROOTLIB  := $(shell root-config --libdir)


LIBS          := -L -lpthread -lutil -lusb-1.0 -L $(ROOTLIB) -lGeom -lRGL -lGed -lTreePlayer -lCore -lHist -lGraf -lGraf3d -lMathCore -lGpad -lTree -lRint -lRIO -lPostscript -lMatrix -lPhysics -lMinuit -lGui -lASImage -lASImageGui -pthread -lm -ldl -rdynamic -lstdc++
#CFLAGS        = -g -O2 -Wall -Wuninitialized -fno-strict-aliasing -I./include -I/usr/local/include -I $(ROOTINCL) -DLINUX -fPIC -lCAENDigitizer
#CXXFLAGS        = -std=c++17 -g -O2 -Wall -Wuninitialized -fno-strict-aliasing -I./include -I/usr/local/include -I $(ROOTINCL) -DLINUX -fPIC -lCAENDigitizer


#CFLAGS        += -stdlib=libstdc++
  


CXX           = g++ -fPIC -DLINUX -O2 -I./include




.PHONY: all clean
	
all: clean DTRoot
	
clean:
		@rm -rf DTRoot *.o *.cxx *.so

guiDict.cxx : include/DTRoot.h include/DTFrame.h include/DTOpt.h include/LinkDef.h
	@rootcling -f $@ $^
	$(info [-10%]  Dictionary)	

DTRoot: DTFunc.o DTReadout.o DTOpt.o DTFrame.o DTRoot.o 
		$(info [70%]  Linking)
		@$(CXX) -o DTRoot src/DTRoot.o guiDict.cxx src/DTFrame.o src/DTOpt.o src/DTReadout.o src/DTFunc.o $(LIBS) -lCAENDigitizer `root-config --cflags --glibs`
		$(info [100%] Built target DTRoot)
		
DTFrame.o:  src/DTFrame.c
		$(info [40%] Generating DTFrame.o)
		@$(CXX) -o src/DTFrame.o -c src/DTFrame.c `root-config --cflags --glibs`
		
DTFunc.o:  src/DTFunc.c
		$(info [10%] Generating DTFunc.o)
		@$(CXX) -o src/DTFunc.o -c src/DTFunc.c	`root-config --cflags --glibs`	
		
DTReadout.o:  src/DTReadout.c
		$(info [15%] Generating DTReadout.o)
		@$(CXX) -o src/DTReadout.o -c src/DTReadout.c	`root-config --cflags --glibs`			
		
DTOpt.o: src/DTOpt.c guiDict.cxx
		$(info [20%] Generation DTOpt.o)
		@$(CXX) -o src/DTOpt.o -c src/DTOpt.c `root-config --cflags --glibs`

DTRoot.o: src/DTRoot.c guiDict.cxx
		$(info [30%] Generation DTRoot.o)
		@$(CXX) -o src/DTRoot.o -c src/DTRoot.c `root-config --cflags --glibs`
		
