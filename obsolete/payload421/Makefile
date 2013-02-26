PPU_CC = ppu-gcc
PPU_OBJCOPY = ppu-objcopy

all: payload_sky_421.bin umount_421.bin

%.o : %.S
	$(PPU_CC) -c $< -o $@

%.bin : %.o
	$(PPU_OBJCOPY) -O binary $< $@
	cp -f $@ ../datas/$@

clean:
	rm -f *.o *.bin
