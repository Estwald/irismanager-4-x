PPU_CC = ppu-gcc
PPU_OBJCOPY = ppu-objcopy

TARGET		:=	$(notdir $(CURDIR))
PAYLOADS := 355 355dex 421 421dex 430 430dex 431 440 441 446 450 450dex 453 455 460 460dex 460deh 465 465dex 466 470 470dex 475 475dex 476
SPECIALPAYLOADS:= psx_storage ps3_storage ps3_controlfan

all: CFW341 $(SPECIALPAYLOADS) $(PAYLOADS)

define do_payload
	$(PPU_CC) -c payload_sky.S -o payload_sky_$@.o -DCFW_$@
	@$(PPU_OBJCOPY) -O binary payload_sky_$@.o ../datas/payload_sky_$@.bin
endef

define do_payload2
	$(PPU_CC) -c $@.S -o $@.o
	@$(PPU_OBJCOPY) -O binary $@.o ../datas/$@.bin
endef

define do_umount
	$(PPU_CC) -c umount.S -o umount_$@.o -DCFW_$@
	@$(PPU_OBJCOPY) -O binary umount_$@.o ../datas/umount_$@.bin

endef

   
payload_groove_hermes:
	$(do_payload2)
341:
	$(do_umount)
	@rm -f *.o
	@echo ----------------------------------------------------------------

CFW341: payload_groove_hermes 341

$(SPECIALPAYLOADS):
	$(do_payload2)
	@rm -f *.o
	@echo ----------------------------------------------------------------

$(PAYLOADS):
	$(do_payload)
	$(do_umount)
	@rm -f *.o
	@echo ----------------------------------------------------------------


