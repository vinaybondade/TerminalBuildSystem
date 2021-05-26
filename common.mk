# Makefile
SHELL=/bin/bash
ifndef prj
prj=../..
endif
include $(prj)/dev.mk

#show verbose output
VERB=@
ifdef V
 VERB=
endif

TARGET_DIR = $(prj)/overlay/fs-$(dev)/opt/$(dev)
INSTALL_DIR = $(TARGET_DIR)/bin
ifndef TARGET
	TARGET=$(notdir $(CURDIR))
endif

RELLIBS += -s

ifndef PLAIN_C
 CPP_FLAGS=-fpermissive
 COMPILE=$(cross)g++ $(CFLAGS) -c
 LINK=$(cross)g++ -o 
 DEPEND=$(cross)g++
else
 COMPILE=$(cross)gcc $(CFLAGS) -c
 LINK=$(cross)gcc -o
 DEPEND=$(cross)gcc 
endif 
STRIP=$(cross)strip

ifeq ($(IS_LIB),y) 
 INSTALL_DIR = $(TARGET_DIR)/lib
 ifndef MAJOR_VER
  MAJOR_VER=1
 endif
 ifndef MINOR_VER
  MINOR_VER=0
 endif
 ifndef COMPILE_VER
  COMPILE_VER=0
 endif

 ifeq ($(IS_STATIC_LIB),y)
 POSTFIX=.a
 LINK=$(cross)ar rc
 RELLIBS=
 else
 POSTFIX=.so.$(MAJOR_VER).$(MINOR_VER).$(COMPILE_VER)
 LDFLAGS = -fPIC -shared -L$(INSTALL_DIR)
 endif
 CFLAGS += -fPIC
 DBGTARGET = debug/$(dev)/lib$(TARGET)_d$(POSTFIX)
 RELTARGET = release/$(dev)/lib$(TARGET)$(POSTFIX)
else
 IS_LIB=
 DBGTARGET = debug/$(dev)/$(TARGET)_d
 RELTARGET = release/$(dev)/$(TARGET)
 LDFLAGS = --sysroot=$(staging) -L$(TARGET_DIR)/lib -L$(staging)/usr/lib
endif

CFLAGS += -Wall -I./include -I$(staging)/usr/include -fno-strict-aliasing $(CPP_FLAGS) -D$(dev)

DBGCFLAGS += -g -D__DEBUG -D_DEBUG 
RELCFLAGS = -O2

ifndef VPATH
VPATH = src
endif
ifndef CPP_SRC
CPP_SRC += $(notdir $(wildcard src/*.cpp)) 
endif
ifndef SRC
SRC += $(notdir $(wildcard src/*.c)) 
endif

DBGOBJFILES_CPP = $(CPP_SRC:%.cpp=debug/$(dev)/%.o)
DBGOBJFILES_C = $(SRC:%.c=debug/$(dev)/%.o)
RELOBJFILES_CPP = $(CPP_SRC:%.cpp=release/$(dev)/%.o)
RELOBJFILES_C = $(SRC:%.c=release/$(dev)/%.o)

all: 
	$(MAKE) dirs
	$(MAKE) depend
	$(MAKE) debug
	$(MAKE) release
	$(MAKE) install

dirs: test_env
	@mkdir -p debug/$(dev); \
	mkdir -p release/$(dev); \
	mkdir -p $(INSTALL_DIR)

install: release
	@echo "Copy RELEASE to $(INSTALL_DIR)"
	$(VERB)$(STRIP) $(RELTARGET)
	$(VERB)cp $(RELTARGET) $(INSTALL_DIR)
	@echo "Copy DEBUG to $(INSTALL_DIR)"
	$(VERB)cp $(DBGTARGET) $(INSTALL_DIR)
	@if [ ! -d $(IS_LIB) ]; \
	then \
		cd $(INSTALL_DIR); \
		ln -f -s $(notdir $(RELTARGET)) $(notdir $(basename $(basename $(basename $(RELTARGET))))); \
		ln -f -s $(notdir $(DBGTARGET)) $(notdir $(basename $(basename $(basename $(DBGTARGET))))); \
		cd -; \
	fi

release: test_env dirs $(RELTARGET)

debug: test_env dirs $(DBGTARGET)


$(RELTARGET):	$(RELOBJFILES_CPP) $(RELOBJFILES_C)
	$(VERB)$(LINK) $@ $^ $(LDFLAGS) $(RELLIBS)

$(DBGTARGET):	$(DBGOBJFILES_CPP) $(DBGOBJFILES_C)
	$(VERB)$(LINK) $@ $^ $(LDFLAGS) $(DBGLIBS)

$(RELOBJFILES_CPP):	release/$(dev)/%.o: %.cpp
	$(VERB)$(COMPILE) $(RELCFLAGS) -o release/$(dev)/$(notdir $@) $< \

$(RELOBJFILES_C):	release/$(dev)/%.o: %.c
	$(VERB)$(COMPILE) $(RELCFLAGS) -o release/$(dev)/$(notdir $@) $<


$(DBGOBJFILES_CPP):	debug/$(dev)/%.o: %.cpp
	$(VERB)$(COMPILE) $(DBGCFLAGS) -o debug/$(dev)/$(notdir $@) $<

$(DBGOBJFILES_C):	debug/$(dev)/%.o: %.c
	$(VERB)$(COMPILE) $(DBGCFLAGS) -o debug/$(dev)/$(notdir $@) $<

clean:
	@echo "Cleaning the $(TARGET) ***********"
	@rm -rf release/$(dev)/* debug/$(dev)/* *~ .*dep*

distclean:
	@rm -rf $(INSTALL_DIR)/*$(TARGET)*

depend:	$(CPP_SRC) $(SRC)
	$(VERB)$(DEPEND) $(CFLAGS) -MM $^ > .$(dev)_depend_debug
	$(VERB)$(DEPEND) $(CFLAGS) -MM $^ > .$(dev)_depend_release
	@#@sed -i -e "s/\([a-z]*\)/debug\/$(plat)\/$(dev)\/\1/" .$(plat)_$(dev)_depend_debug
	@#@sed -i -e "s/\([a-z]*\)/release\/$(plat)\/$(dev)\/\1/" .$(plat)_$(dev)_depend_release
	@#@sed -i -e "s/debug\/ / /" .$(plat)_$(dev)_depend_debug
	@#@sed -i -e "s/release\/ / /" .$(plat)_$(dev)_depend_release

ifeq (.$(plat)_$(dev)_depend_debug,$(wildcard .$(dev)_depend_debug))
 include .$(dev)_depend_debug
endif
ifeq (.$(dev)_depend_release,$(wildcard .$(dev)_depend_release))
 include .$(dev)_depend_release
endif
