LK	= hlk
AS	= has
CC	= gcc

all: msconf.x
	./msconf.x

clean:
	rm -f *.o *.x msconf.html

wasm: msconf.html
	cp msconf.wasm ../docs/

msconf.x: msconf.o mslib.o mscmd.o
	${CC} -o $@ -liocs -ldos $^

%.o: %.c *.h mslib.h
	${CC} -O9 -c $<

mslib.o: mslib.has
	${AS} -o $@ $<

%.wasm.o: %.c wasm/*.h
	iconv --from-code=cp932 --to-code=utf8 $< | emcc -c -O3 -Iwasm -o $@ -x c -

msconf.html: msconf.wasm.o
	emcc -s WASM=1 -s EXPORTED_RUNTIME_METHODS='["ccall"]' -s NO_EXIT_RUNTIME=1 \
		-s WARN_ON_UNDEFINED_SYMBOLS=0 -o $@ $<