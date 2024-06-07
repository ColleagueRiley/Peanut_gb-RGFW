CC = gcc

LIBS := -loleaut32 -I./include -lgdi32 -lm -ggdb -lShcore -lWinmm
EXT = .exe

ifeq ($(CC),x86_64-w64-mingw32-gcc)
	STATIC = --static
endif

ifneq (,$(filter $(CC),winegcc x86_64-w64-mingw32-gcc))
    detected_OS := Windows
	LIB_EXT = .dll
else
	ifeq '$(findstring ;,$(PATH))' ';'
		detected_OS := Windows
	else
		detected_OS := $(shell uname 2>/dev/null || echo Unknown)
		detected_OS := $(patsubst CYGWIN%,Cygwin,$(detected_OS))
		detected_OS := $(patsubst MSYS%,MSYS,$(detected_OS))
		detected_OS := $(patsubst MINGW%,MSYS,$(detected_OS))
	endif
endif

ifeq ($(detected_OS),Windows)
	LIBS := -I./include -lopengl32 -lgdi32 -lm -ggdb -lWinmm
	EXT = .exe
endif
ifeq ($(detected_OS),Darwin)        # Mac OS X
	LIBS := -I./include -lm -framework Foundation -framework AppKit -framework OpenGL -framework CoreVideo -w $(STATIC)
	EXT = 
endif
ifeq ($(detected_OS),Linux)
    LIBS := -I./include -lXrandr -lX11 -lGL -lm $(STATIC)
	EXT = 
endif

all:
	$(CC) peanut_rgfw.c -w $(LIBS) -I./ -Wall -o peanut_rgfw$(EXT)

clean:
	rm -f peanut_rgfw peanut_rgfw$(EXTT)

debug:
	make clean

	$(CC) peanut_rgfw.c -w $(LIBS) -I./ -Wall -D RGFW_DEBUG -o peanut_rgfw
