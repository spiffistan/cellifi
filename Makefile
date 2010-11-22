SPUCC = spu-gcc
PPUCC = ppu-gcc
CC = $(PPUCC)
CFLAGS = -g
SPUFLAGS = -g 
PPUFLAGS = -g 
SPULIBS = -lm 
PPULIBS = -lspe2 -lm

ENCOBJS = c63enc.o dsp.o tables.o io.o c63_write.o c63.h common.o me.o cell_util.o
DECOBJS = c63dec.o dsp.o tables.o io.o c63.h common.o me.o

all: spu_part.eo c63enc c63dec

c63enc: $(ENCOBJS) spu_part.eo
	$(PPUCC) $^ -o $@ $(PPULIBS) $(PPUFLAGS) 

c63dec: $(DECOBJS)
	$(PPUCC) $^ -o $@ $(PPULIBS) $(PPUFLAGS) 

spu_part.o: spu_part.c
	$(SPUCC) $^ -o $@ $(SPULIBS) $(SPUFLAGS) 

spu_part.eo: spu_part.o
	ppu-embedspu -m64 c63_spu $^ $@

clean:
	rm -f *.o c63enc c63dec
