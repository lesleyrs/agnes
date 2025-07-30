LIBC = ../../../wasmlite/libc
WCC = ../../../xcc/wcc
CLANG = clang

DEBUG = 0
OUT = agnes.wasm wcc-agnes.wasm

ifeq ($(DEBUG),0)
CFLAGS = -Oz -ffast-math -flto #-Wall -Wextra
else
CFLAGS = -g -Wall -Wextra
endif

agnes.wasm: CC = $(CLANG) --target=wasm32 --sysroot=$(LIBC) -Wl,--export-table -Wl,--export=malloc -nodefaultlibs -lc
wcc-agnes.wasm: CC = $(WCC) -isystem=$(LIBC)/include -L$(LIBC)/lib -Wl,--export-table -e=malloc --stack-size=64000

all: $(OUT) serve

$(OUT): wasm.c ../agnes.c
	$(CC) $(CFLAGS) -o $@ $^
ifeq ($(DEBUG),0)
	wasm-opt $@ -o $@ -Oz -all && wasm-strip $@
else
	../../emscripten/tools/wasm-sourcemap.py $@ -w $@ -p $(CURDIR) -s -u ./$@.map -o $@.map --dwarfdump=/usr/bin/llvm-dwarfdump
endif

serve:
	esbuild --servedir=.
