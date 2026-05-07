EASYLOCAL = /home/fabio/easylocal-3
FLAGS = -Wall -Wfatal-errors -Wno-sign-compare -Wno-deprecated-declarations -O3
COMPOPTS = -I$(EASYLOCAL)/include $(FLAGS)
LINKOPTS = -lboost_program_options -pthread

SOURCE_FILES = BT_Data.cc BT_Helpers.cc BT_Main.cc
OBJECT_FILES = BT_Data.o BT_Helpers.o BT_Main.o
HEADER_FILES = BT_Data.hh BT_Helpers.hh

bt_main: $(OBJECT_FILES)
	g++ $(OBJECT_FILES) $(LINKOPTS) -o bt_main

BT_Data.o: BT_Data.cc BT_Data.hh
	g++ -c $(COMPOPTS) BT_Data.cc

BT_Helpers.o: BT_Helpers.cc BT_Helpers.hh BT_Data.hh
	g++ -c $(COMPOPTS) BT_Helpers.cc

BT_Main.o: BT_Main.cc BT_Helpers.hh BT_Data.hh
	g++ -c $(COMPOPTS) BT_Main.cc

clean:
	rm -f $(OBJECT_FILES) bt_main