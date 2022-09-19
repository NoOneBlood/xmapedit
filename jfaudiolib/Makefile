-include Makefile.user

ifeq (0,$(RELEASE))
 OPTLEVEL=-Og
else
 OPTLEVEL=-fomit-frame-pointer -O2
endif

CC?=gcc
AR?=ar
CFLAGS=-g $(OPTLEVEL) -W -Wall -Wno-unused-but-set-variable
CPPFLAGS=-Iinclude -Isrc
LDFLAGS=
o=o

SOURCES=src/drivers.c \
        src/fx_man.c \
        src/cd.c \
        src/multivoc.c \
        src/mix.c \
        src/mixst.c \
        src/pitch.c \
        src/vorbis.c \
        src/music.c \
        src/midi.c \
        src/driver_nosound.c \
        src/asssys.c

include Makefile.shared

ifeq (mingw32,$(findstring mingw32,$(machine)))
 CPPFLAGS+= -Ithird-party/mingw32/include
 SOURCES+= src/driver_directsound.c src/driver_winmm.c

 CPPFLAGS+= -DHAVE_VORBIS
else
 ifeq (apple-darwin,$(findstring apple-darwin,$(machine)))
  SOURCES+= src/driver_coreaudio.c
  LDFLAGS+= -framework AudioToolbox -framework AudioUnit -framework Foundation
 endif
 ifneq (0,$(JFAUDIOLIB_HAVE_SDL))
  CPPFLAGS+= -DHAVE_SDL=2 $(shell $(SDL2CONFIG) --cflags)
  ifeq (1,$(JFAUDIOLIB_USE_SDLMIXER))
   CPPFLAGS+= -DUSE_SDLMIXER
   SOURCES+= src/driver_sdlmixer.c
  else
   SOURCES+= src/driver_sdl.c
  endif
 endif
 ifeq (1,$(JFAUDIOLIB_HAVE_ALSA))
  CPPFLAGS+= -DHAVE_ALSA $(shell $(PKGCONFIG) --cflags alsa)
  SOURCES+= src/driver_alsa.c
 endif
 ifeq (1,$(JFAUDIOLIB_HAVE_FLUIDSYNTH))
  CPPFLAGS+= -DHAVE_FLUIDSYNTH $(shell $(PKGCONFIG) --cflags fluidsynth)
  SOURCES+= src/driver_fluidsynth.c
 endif
 ifeq (1,$(JFAUDIOLIB_HAVE_VORBIS))
  CPPFLAGS+= -DHAVE_VORBIS $(shell $(PKGCONFIG) --cflags vorbisfile)
 endif
endif

OBJECTS=$(SOURCES:%.c=%.o)

.PHONY: all
all: $(JFAUDIOLIB) test

include Makefile.deps

$(JFAUDIOLIB): $(OBJECTS)
	$(AR) cr $@ $^

$(OBJECTS): %.o: %.c
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@

test: src/test.o $(JFAUDIOLIB);
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(JFAUDIOLIB_LDFLAGS) -lm

.PHONY: clean
clean:
	-rm -f $(OBJECTS) $(JFAUDIOLIB)
