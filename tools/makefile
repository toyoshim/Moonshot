LK	= hlk
AS	= has
CC	= gcc

all: msconf.x
	./msconf.x

clean:
	rm *.o *.x

msconf.x: msconf.o mslib.o mscmd.o
	${CC} -o $@ -liocs -ldos $^

%.o: %.c *.h mslib.h
	${CC} -O9 -c $<

mslib.o: mslib.has
	${AS} -o $@ $<
