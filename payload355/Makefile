PPU_CC = ppu-gcc
PPU_OBJCOPY = ppu-objcopy

all: payload_syscall36_355.bin payload_sky_355.bin umount_355.bin

%.o : %.S
	$(PPU_CC) -c $< -o $@

%.bin : %.o
	$(PPU_OBJCOPY) -O binary $< $@
	cp $@ ../datas/$@

clean:
	rm -f *.o *.bin
