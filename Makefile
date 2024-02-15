# library name
lib.name = simpl

# python libs
CC = g++

uname := $(shell uname -s)

ifeq (MINGW,$(findstring MINGW,$(uname)))
  cflags = -I ../src/simpl/
  ldlibs = -l simpl -l winpthread
  

else ifeq (Linux,$(findstring Linux,$(uname)))
  	cflags = -I simpl/src/simpl/ -I simpl/src/mq -I simpl/src/sms/ -I simpl/src/loris/ -I simpl/src/sndobj -I fftw3 -I gsl -I gslcblas -Wno-unused-parameter -Wno-overloaded-virtual
  	ldlibs = -L simpl/build/ -l simpl
  

else ifeq (Darwin,$(findstring Darwin,$(uname)))
  # cflags = -Wno-cast-function-type -Wincompatible-pointer-types -Wint-conversion
  # ldlibs = -l simpl

else
  $(error "Unknown system type: $(uname)")
  $(shell exit 1)

endif

#╭──────────────────────────────────────╮
#│             Build Simpl              │
#╰──────────────────────────────────────╯

.PHONY: all simpl 
all: simpl 
simpl:
	echo "Building simpl"
	cmake -B ./simpl/build -S ./simpl && cmake --build ./simpl/build
	cp ./simpl/build/libsimpl.so ./libsimpl.so


#╭──────────────────────────────────────╮
#│               Sources                │
#╰──────────────────────────────────────╯

simpl.class.sources = pd-obj/pd_simpl.cpp pd-obj/mqpeaks.cpp pd-obj/get.cpp


#╭──────────────────────────────────────╮
#│                 Data                 │
#╰──────────────────────────────────────╯

# datafiles = 


#╭──────────────────────────────────────╮
#│             pdlibbuilder             │
#╰──────────────────────────────────────╯
PDLIBBUILDER_DIR=./pd-obj/pd-lib-builder/
include $(PDLIBBUILDER_DIR)/Makefile.pdlibbuilder
