LK	= hlk
AS	= has
CC	= gcc
SHELL	= zsh

all: msconf.x
	./msconf.x

clean:
	rm -f *.o msconf.html msconf.js msconf.wasm

wasm: msconf.html
	cp msconf.wasm msconf.js ../docs/

msconf.x: msconf.o mslib.o mscmd.o comm.o
	${CC} -o $@ -liocs -ldos $^

%.o: %.c *.h mslib.h
	${CC} -O9 -c $<

mslib.o: mslib.has
	${AS} -o $@ $<

%.wasm.o: %.c wasm/*.h
	iconv --from-code=cp932 --to-code=utf-8 $< | emcc -c -O3 -Iwasm -o $@ -x c -

%.wasm.o: wasm/%.c wasm/*.h
	emcc -c -O3 -o $@ $<

msconf.html: msconf.wasm.o mslib.wasm.o mscmd.wasm.o comm.wasm.o
	emcc -s WASM=1 -s ASYNCIFY -s ASYNCIFY_STACK_SIZE=8192 -o $@ $^