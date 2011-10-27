FLAGS = -g -Wall -Wextra -Wno-unused-parameter -Ddebug      -fmessage-length=0 -fwrapv -funsigned-char -funsigned-bitfields -Werror -Wall -Wextra -Wshadow -Wwrite-strings -Wstrict-overflow=5 -Winit-self -Wcast-align -Wcast-qual -Wpointer-arith -Wstrict-aliasing -Wformat=2 -Wmissing-include-dirs -Wno-unused-parameter -Wuninitialized -Wstrict-prototypes -Wno-error=unused-variable -Wformat-security -Wpointer-to-int-cast -Wint-to-pointer-cast -Wno-missing-field-initializers -Wno-error=unused-label -Wno-format-nonliteral -ansi -pedantic -Dlinux -std=c89
CFLAGS = $(FLAGS)
WCFLAGS = $(FLAGS) -Iwin-includes
LFLAGS = -lSDL -lSDL_mixer -lSDL_image -lSDL_ttf
CC = gcc
ERRORS = 2>&1 | tee errors.err


all: patapon Makefile

patapon: patapon.o data.o
	$(CC) $(LFLAGS) -o patapon patapon.o data.o

patapon.o: patapon.c patapon.h
	$(CC) $(CFLAGS) -c -o patapon.o patapon.c 

data.o: data.c
	$(CC) $(CFLAGS) -c -o data.o data.c 

win: patapon.exe
	cp patapon.exe win
	@rm -f patapon.zip
	zip -r patapon.zip win

patapon.exe: wpatapon.o wdata.o
	i486-mingw32-gcc -o patapon.exe wpatapon.o wdata.o -Lwin -lmingw32 -lSDL_image -lSDL_mixer -lSDL_ttf -lSDLmain -lSDL -mwindows

wpatapon.o: patapon.c patapon.h
	i486-mingw32-gcc $(WCFLAGS) -c -o wpatapon.o patapon.c

wdata.o: data.c patapon.h
	i486-mingw32-gcc $(WCFLAGS) -c -o wdata.o data.c

test: patapon.exe
	cd win && wine ../patapon.exe

clean:
	rm -f *.o
