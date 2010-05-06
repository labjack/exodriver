#
# Makefile for U12 examples
#
U12AISAMPLE_SRC=u12AISample.c
U12AISAMPLE_OBJ=$(U12AISAMPLE_SRC:.c=.o)

SRCS=$(wildcard *.c)
HDRS=$(wildcard *.h)

CFLAGS=-Wall -g
LDFLAGS=
LIBS=-lm -llabjackusb

all: u12AISample

u12AISample: $(U12AISAMPLE_OBJ)
	$(CC) -o u12AISample $(U12AISAMPLE_OBJ) $(LDFLAGS) $(LIBS)

clean:
	rm -f *.o u12AISample
