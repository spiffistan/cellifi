SPUCC = spu-gcc
PPUCC = ppu-gcc
CC = $(PPUCC)
GCC = gcc -m64
CFLAGS = -O3
SPUFLAGS = -O3
PPUFLAGS = -O3
SPULIBS = -lsimdmath -lm 
PPULIBS = -lspe2 -lm 

ENCOBJS = c63enc.o dsp.o tables.o io.o c63_write.o c63.h common.o me.o cell_util.o 
DECOBJS = c63dec.o dsp.o tables.o io.o c63.h common.o me.o

all: spu_part.eo c63enc c63dec

debug: CFLAGS += -DDEBUG -g 
debug: SPUFLAGS += -DDEBUG -g 
debug: PPUFLAGS += -DDEBUG -g 
debug: all

c63enc: $(ENCOBJS) spu_part.eo
	$(PPUCC) $^ -o $@ $(PPULIBS) $(PPUFLAGS) 

c63dec: $(DECOBJS)
	$(GCC) $^ -o $@ $(PPULIBS) $(PPUFLAGS) 

spu_part.o: spu_part.c me_spu.c dct_spu.c spu_tables.c
	$(SPUCC) $^ -o $@ $(SPULIBS) $(SPUFLAGS) 

spu_part.eo: spu_part.o 
	ppu-embedspu -m64 c63_spu $^ $@

clean:
	rm -f *.o c63enc c63dec

run: clean all
	./c63enc -w 352 -h 288 -f 20 -o test.c63 ~/foreman.yuv 

test: clean debug
	ppu-gdb --args ./c63enc -w 352 -h 288 -o test.c63 ~/foreman.yuv

runtest: clean debug
	./c63enc -w 352 -h 288 -o test.c63 ~/foreman.yuv
