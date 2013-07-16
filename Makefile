KWD_CONFIG 	= kwbimage.cfg
TEXT_BASE 	= 0x00000000

OBJCOPY 		= $(CROSS_COMPILE)objcopy
OBJCFLAGS 	= --gap-fill=0xff

CC  				= $(CROSS_COMPILE)gcc
CCFLAGS			= -D__ASSEMBLY__  -g -Os -fno-common -ffixed-r8 -msoft-float -DSYS_TEXT_BASE=$(TEXT_BASE) -fno-builtin -ffreestanding -nostdinc -isystem /home/bart/abc/arm-2009q3/bin/../lib/gcc/arm-none-eabi/4.4.1/include -pipe -DCONFIG_ARM -D__ARM__ -marm -mabi=aapcs-linux -mno-thumb-interwork -march=armv5te -std=c99
INCLUDE_DIR	=	include

LD					= $(CROSS_COMPILE)ld
LDFLAGS			= -pie -Bstatic -lgcc -Ttext $(TEXT_BASE) -T loader.lds -L /home/bart/abc/arm-2009q3/bin/../lib/gcc/arm-none-eabi/4.4.1 -Map loader.map 

ALL 	+= loader.kwb
OBJS	+= start.o main.o kirkwood_nand.o timer.o _udivsi3.o _divsi3.o div0.o 

all: $(ALL)

loader.kwb: loader.bin
	tools/mkimage -n tools/$(KWD_CONFIG) -T kwbimage -a $(TEXT_BASE) -e $(TEXT_BASE) -d $< $@

loader.bin: loader
	$(OBJCOPY) ${OBJCFLAGS} -O binary $< $@

loader: $(OBJS)
	$(LD)  ${LDFLAGS} $(OBJS) -o $@

%.o: %.S
	$(CC) -c $< ${CCFLAGS} -I${INCLUDE_DIR}

%.o: %.c
	$(CC) -c $< ${CCFLAGS} -I${INCLUDE_DIR}

clean:
	rm -f *.o loader loader.bin loader.kwb
