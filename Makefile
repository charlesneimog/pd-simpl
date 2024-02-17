# library name
lib.name = simpl

# python libs
CC = g++

uname := $(shell uname -s)

#╭──────────────────────────────────────╮
#│               Sources                │
#╰──────────────────────────────────────╯

simpl.class.sources = \
		pd-obj/pd-simpl.cpp \
		pd-obj/s-peaks~.cpp \
		pd-obj/s-partials.cpp \
		pd-obj/s-synth~.cpp \
		pd-obj/s-get.cpp \
		pd-obj/s-create.cpp

#╭──────────────────────────────────────╮
#│                 Data                 │
#╰──────────────────────────────────────╯

# datafiles = 


#╭──────────────────────────────────────╮
#│             Build Simpl              │
#╰──────────────────────────────────────╯

.PHONY: all simpl pd-obj 
all: simpl pd-obj
simpl:
	echo "Building simpl"
	cmake -B ./simpl/build -S ./simpl && cmake --build ./simpl/build
	cp ./simpl/build/libsimpl.so ./libsimpl.so

#╭──────────────────────────────────────╮
#│            Build pd-simpl            │
#╰──────────────────────────────────────╯

pd-obj:
	ifeq (MINGW,$(findstring MINGW,$(uname)))
	  cflags = -I ../src/simpl/
	  ldlibs = -l simpl -l winpthread
	  

	else ifeq (Linux,$(findstring Linux,$(uname)))
		cflags = -I simpl/src/simpl/ -I simpl/src/mq -I simpl/src/sms/ -I simpl/src/loris/ -I simpl/src/sndobj -I fftw3 -I gsl -I gslcblas -Wno-unused-parameter -Wno-overloaded-virtual -Wdeprecated-declarations
		ldlibs = -L simpl/build/ -l simpl
	  

	else ifeq (Darwin,$(findstring Darwin,$(uname)))
		cflags = -I simpl/src/simpl/ -I simpl/src/mq -I simpl/src/sms/ -I simpl/src/loris/ -I simpl/src/sndobj -I fftw3 -I gsl -I gslcblas -Wno-unused-parameter -Wno-overloaded-virtual -Wdeprecated-declarations
	  ldlibs = -l simpl

	else
	  $(error "Unknown system type: $(uname)")
	  $(shell exit 1)

	endif

#╭──────────────────────────────────────╮
#│             pdlibbuilder             │
#╰──────────────────────────────────────╯
PDLIBBUILDER_DIR=./pd-obj/pd-lib-builder/
include $(PDLIBBUILDER_DIR)/Makefile.pdlibbuilder
