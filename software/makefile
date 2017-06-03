#================GENERAL==============
TARGET=adc
TARGEX=.exe
TARGETEX=$(TARGET)$(TARGEX)

SRCS=opendevice.c
OBJS=$(SRCS:.c=.o)
FLAGS=
FLAGS+=-Wall
#FLAGS+=-v
#FLAGS+=-w
#FLAGS+=-Wl,-subsystem,windows

#================COMPILER=============
DEFINES=-D FREEGLUT_STATIC
INCDIRS=-I. -I$(TOT_LIB)\c\freeglut\include \
-I$(TOT_LIB)\c\libusb-win32-bin-1.2.6.0\include
CFLAGS=$(DEFINES) $(INCDIRS)

#================LINKER===============
LIBDIRS=-L. -L$(TOT_LIB)\c\freeglut\lib \
-L$(TOT_LIB)\c\libusb-win32-bin-1.2.6.0\lib\gcc
LIBS=-lfreeglut_static -lopengl32 -lwinmm -lgdi32 \
-lusb
LFLAGS=$(LIBDIRS) $(LIBS)
LFLAGS+=-Wl,--subsystem,windows

#================TOOLS================
EXEEX=.exe

CC=gcc$(EXEEX)
RM=rm -f
SIZE=size$(EXEEX)

#================TARGETS==============
build: $(TARGETEX)

clean:
	$(RM) $(TARGETEX)
	$(RM) $(OBJS)

deps: $(TARGET).d

.PHONY: build clean deps
.PRECIOUS: %.o

%$(TARGEX): %.o $(OBJS)
	$(CC) $^ \
$(FLAGS) \
$(LFLAGS) \
-o $@
	$(SIZE) $@

%.o: %.c
	$(CC) $< \
$(FLAGS) \
$(CFLAGS) \
-c -o $@

%.d: %.c
	$(CC) $< -MD -o $@
