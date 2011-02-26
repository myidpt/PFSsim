#
# OMNeT++/OMNEST Makefile for PFSsim_v0.13
#
# This file was generated with the command:
#  opp_makemake -f --deep -O out
#

# Name of target to be created (-o option)
TARGET = PFSsim_v0.13$(EXE_SUFFIX)

# User interface (uncomment one) (-u option)
USERIF_LIBS = $(ALL_ENV_LIBS) # that is, $(TKENV_LIBS) $(CMDENV_LIBS)
#USERIF_LIBS = $(CMDENV_LIBS)
#USERIF_LIBS = $(TKENV_LIBS)

# C++ include paths (with -I)
INCLUDE_PATH = \
    -I. \
    -Iclient \
    -Ipacket \
    -Irequest \
    -Iresults \
    -Irouter \
    -Ischeduleinfo \
    -Ischeduler \
    -Iserver \
    -Itraces

# Additional object and library files to link with
EXTRA_OBJS =

# Additional libraries (-L, -l options)
LIBS =

# Output directory
PROJECT_OUTPUT_DIR = out
PROJECTRELATIVE_PATH =
O = $(PROJECT_OUTPUT_DIR)/$(CONFIGNAME)/$(PROJECTRELATIVE_PATH)

# Object files for local .cc and .msg files
OBJS = \
    $O/client/Client.o \
    $O/request/Request.o \
    $O/router/Queue.o \
    $O/router/Routing.o \
    $O/scheduler/SFQ.o \
    $O/scheduler/FIFO.o \
    $O/scheduler/IQueue.o \
    $O/server/Mserver.o \
    $O/server/Dserver.o \
    $O/packet/GPacket_m.o \
    $O/packet/QPacket_m.o \
    $O/packet/SPacket_m.o

# Message files
MSGFILES = \
    packet/GPacket.msg \
    packet/QPacket.msg \
    packet/SPacket.msg

#------------------------------------------------------------------------------

# Pull in OMNeT++ configuration (Makefile.inc or configuser.vc)

ifneq ("$(OMNETPP_CONFIGFILE)","")
CONFIGFILE = $(OMNETPP_CONFIGFILE)
else
ifneq ("$(OMNETPP_ROOT)","")
CONFIGFILE = $(OMNETPP_ROOT)/Makefile.inc
else
CONFIGFILE = $(shell opp_configfilepath)
endif
endif

ifeq ("$(wildcard $(CONFIGFILE))","")
$(error Config file '$(CONFIGFILE)' does not exist -- add the OMNeT++ bin directory to the path so that opp_configfilepath can be found, or set the OMNETPP_CONFIGFILE variable to point to Makefile.inc)
endif

include $(CONFIGFILE)

# Simulation kernel and user interface libraries
OMNETPP_LIB_SUBDIR = $(OMNETPP_LIB_DIR)/$(TOOLCHAIN_NAME)
OMNETPP_LIBS = -L"$(OMNETPP_LIB_SUBDIR)" -L"$(OMNETPP_LIB_DIR)" $(USERIF_LIBS) $(KERNEL_LIBS) $(SYS_LIBS)

COPTS = $(CFLAGS)  $(INCLUDE_PATH) -I$(OMNETPP_INCL_DIR)
MSGCOPTS = $(INCLUDE_PATH)

#------------------------------------------------------------------------------
# User-supplied makefile fragment(s)
# >>>
# <<<
#------------------------------------------------------------------------------

# Main target
all: $(TARGET)

$(TARGET) : $O/$(TARGET)
	$(LN) $O/$(TARGET) .

$O/$(TARGET): $(OBJS)  $(wildcard $(EXTRA_OBJS)) Makefile
	@$(MKPATH) $O
	$(CXX) $(LDFLAGS) -o $O/$(TARGET)  $(OBJS) $(EXTRA_OBJS) $(WHOLE_ARCHIVE_ON) $(LIBS) $(WHOLE_ARCHIVE_OFF) $(OMNETPP_LIBS)

.PHONY:

.SUFFIXES: .cc

$O/%.o: %.cc
	@$(MKPATH) $(dir $@)
	$(CXX) -c $(COPTS) -o $@ $<

%_m.cc %_m.h: %.msg
	$(MSGC) -s _m.cc $(MSGCOPTS) $?

msgheaders: $(MSGFILES:.msg=_m.h)

clean:
	-rm -rf $O
	-rm -f PFSsim_v0.13 PFSsim_v0.13.exe libPFSsim_v0.13.so libPFSsim_v0.13.a libPFSsim_v0.13.dll libPFSsim_v0.13.dylib
	-rm -f ./*_m.cc ./*_m.h
	-rm -f client/*_m.cc client/*_m.h
	-rm -f packet/*_m.cc packet/*_m.h
	-rm -f request/*_m.cc request/*_m.h
	-rm -f results/*_m.cc results/*_m.h
	-rm -f router/*_m.cc router/*_m.h
	-rm -f scheduleinfo/*_m.cc scheduleinfo/*_m.h
	-rm -f scheduler/*_m.cc scheduler/*_m.h
	-rm -f server/*_m.cc server/*_m.h
	-rm -f traces/*_m.cc traces/*_m.h

cleanall: clean
	-rm -rf $(PROJECT_OUTPUT_DIR)

depend:
	$(MAKEDEPEND) $(INCLUDE_PATH) -f Makefile -P\$$O/ -- $(MSG_CC_FILES)  ./*.cc client/*.cc packet/*.cc request/*.cc results/*.cc router/*.cc scheduleinfo/*.cc scheduler/*.cc server/*.cc traces/*.cc

# DO NOT DELETE THIS LINE -- make depend depends on it.
$O/client/Client.o: client/Client.cc \
	request/Request.h \
	packet/GPacket_m.h \
	DServerspecs.h \
	packet/QPacket_m.h \
	packet/SPacket_m.h \
	Clientspecs.h \
	General.h \
	client/Client.h
$O/packet/QPacket_m.o: packet/QPacket_m.cc \
	packet/QPacket_m.h
$O/packet/SPacket_m.o: packet/SPacket_m.cc \
	packet/SPacket_m.h
$O/packet/GPacket_m.o: packet/GPacket_m.cc \
	packet/GPacket_m.h
$O/request/Request.o: request/Request.cc \
	request/Request.h \
	packet/GPacket_m.h \
	DServerspecs.h \
	packet/QPacket_m.h \
	packet/SPacket_m.h \
	Clientspecs.h \
	General.h
$O/router/Queue.o: router/Queue.cc \
	router/Queue.h
$O/router/Routing.o: router/Routing.cc \
	packet/GPacket_m.h \
	DServerspecs.h \
	packet/QPacket_m.h \
	router/Routing.h \
	packet/SPacket_m.h \
	Clientspecs.h \
	General.h
$O/scheduler/SFQ.o: scheduler/SFQ.cc \
	packet/GPacket_m.h \
	DServerspecs.h \
	packet/QPacket_m.h \
	scheduler/IQueue.h \
	packet/SPacket_m.h \
	Clientspecs.h \
	scheduler/SFQ.h \
	General.h
$O/scheduler/IQueue.o: scheduler/IQueue.cc \
	packet/GPacket_m.h \
	DServerspecs.h \
	packet/QPacket_m.h \
	scheduler/IQueue.h \
	packet/SPacket_m.h \
	Clientspecs.h \
	General.h
$O/scheduler/FIFO.o: scheduler/FIFO.cc \
	packet/GPacket_m.h \
	DServerspecs.h \
	packet/QPacket_m.h \
	scheduler/IQueue.h \
	packet/SPacket_m.h \
	Clientspecs.h \
	scheduler/FIFO.h \
	General.h
$O/server/Mserver.o: server/Mserver.cc \
	packet/GPacket_m.h \
	DServerspecs.h \
	packet/QPacket_m.h \
	packet/SPacket_m.h \
	server/Mserver.h \
	Clientspecs.h \
	General.h
$O/server/Dserver.o: server/Dserver.cc \
	server/Dserver.h \
	packet/GPacket_m.h \
	DServerspecs.h \
	packet/QPacket_m.h \
	scheduler/IQueue.h \
	packet/SPacket_m.h \
	scheduler/FIFO.h \
	Clientspecs.h \
	scheduler/SFQ.h \
	General.h

