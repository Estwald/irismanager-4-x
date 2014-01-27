/* cobre.c minimal cobralib support for Iris Manager */

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/systime.h>
#include <lv2/sysfs.h>
#include <errno.h>
#include "syscall8.h"
#include "cobre.h"

LV2_SYSCALL sys_get_version(u32 *version)
{
	lv2syscall2(8, SYSCALL8_OPCODE_GET_VERSION, (u64)version);
	return_to_user_prog(s32);
}

LV2_SYSCALL sys_storage_ext_fake_storage_event(uint64_t event, uint64_t param, uint64_t device)
{
	lv2syscall4(8, SYSCALL8_OPCODE_FAKE_STORAGE_EVENT, event, param, device);
	return_to_user_prog(s32);
}

LV2_SYSCALL sys_storage_ext_mount_ps3_discfile(unsigned int filescount, uint32_t *files)
{
	lv2syscall3(8, SYSCALL8_OPCODE_MOUNT_PS3_DISCFILE, filescount, (uint64_t)files);
	return_to_user_prog(s32);
}

LV2_SYSCALL sys_storage_ext_mount_dvd_discfile(unsigned int filescount, uint32_t *files)
{
	lv2syscall3(8, SYSCALL8_OPCODE_MOUNT_DVD_DISCFILE, filescount, (uint64_t)files);
	return_to_user_prog(s32);
}

LV2_SYSCALL sys_storage_ext_mount_bd_discfile(unsigned int filescount, uint32_t *files)
{
	lv2syscall3(8, SYSCALL8_OPCODE_MOUNT_BD_DISCFILE, filescount, (uint64_t)files);
	return_to_user_prog(s32);
}

LV2_SYSCALL sys_storage_ext_mount_psx_discfile(char *file, unsigned int trackscount, ScsiTrackDescriptor *tracks)
{
	lv2syscall4(8, SYSCALL8_OPCODE_MOUNT_PSX_DISCFILE, (uint64_t)file, trackscount, (uint64_t)tracks);
	return_to_user_prog(s32);
}

LV2_SYSCALL sys_storage_ext_mount_ps2_discfile(unsigned int filescount, uint32_t *files, unsigned int trackscount, ScsiTrackDescriptor *tracks)
{
	lv2syscall5(8, SYSCALL8_OPCODE_MOUNT_PS2_DISCFILE, filescount, (uint64_t)files, trackscount, (uint64_t)tracks);
	return_to_user_prog(s32);
}


LV2_SYSCALL sys_storage_ext_umount_discfile(void)
{
	lv2syscall1(8, SYSCALL8_OPCODE_UMOUNT_DISCFILE);
	return_to_user_prog(s32);
}

LV2_SYSCALL sys_storage_ext_get_disc_type(unsigned int *real_disctype, unsigned int *effective_disctype, unsigned int *fake_disctype)
{
	lv2syscall4(8, SYSCALL8_OPCODE_GET_DISC_TYPE, (uint64_t)real_disctype, (uint64_t)effective_disctype, (uint64_t)fake_disctype);
	return_to_user_prog(s32);
}

LV2_SYSCALL sys_storage_ext_get_emu_state(sys_emu_state_t *state)
{
	lv2syscall2(8, SYSCALL8_OPCODE_GET_EMU_STATE, (uint64_t)state);
	return_to_user_prog(s32);
}


int is_cobra_based(void)
{
    u32 version = 0x99999999;

    if (sys_get_version(&version) < 0)
        return 0;

    if (version != 0x99999999) // If value changed, it is cobra
        return 1;

    return 0;
}

static inline int translate_type(unsigned int type)
{
	if (type == 0)
		return DISC_TYPE_NONE;
	
	else if (type == DEVICE_TYPE_PS3_BD)
		return DISC_TYPE_PS3_BD;
	
	else if (type == DEVICE_TYPE_PS3_DVD)
		return DISC_TYPE_PS3_DVD;
	
	else if (type == DEVICE_TYPE_PS2_DVD)
		return DISC_TYPE_PS2_DVD;
	
	else if (type == DEVICE_TYPE_PS2_CD)
		return DISC_TYPE_PS2_CD;
	
	else if (type == DEVICE_TYPE_PSX_CD)
		return DISC_TYPE_PSX_CD;
	
	else if (type == DEVICE_TYPE_BDROM)
		return DISC_TYPE_BDROM;
	
	else if (type == DEVICE_TYPE_BDMR_SR)
		return DISC_TYPE_BDMR_SR;
	
	else if (type == DEVICE_TYPE_BDMR_RR)
		return DISC_TYPE_BDMR_RR;
	
	else if (type == DEVICE_TYPE_BDMRE)
		return DISC_TYPE_BDMRE;
	
	else if (type == DEVICE_TYPE_DVD)
		return DISC_TYPE_DVD;
	
	else if (type == DEVICE_TYPE_CD)
		return DISC_TYPE_CD;
	
	return DISC_TYPE_UNKNOWN;
}

int cobra_get_disc_type(unsigned int *real_disctype, unsigned int *effective_disctype, unsigned int *iso_disctype)
{
	sys_emu_state_t emu_state;
	unsigned int rdt, edt;
	int ret;
	
	ret = sys_storage_ext_get_disc_type(&rdt, &edt, NULL);
	if (ret != 0)
		return ret;
	
	rdt = translate_type(rdt);
	edt = translate_type(edt);
	
	if (real_disctype)
	{
		*real_disctype = rdt;
	}
	
	if (effective_disctype)
	{
		*effective_disctype = edt;
	}
	
	if (iso_disctype)
	{
		*iso_disctype = DISC_TYPE_NONE;
		
		emu_state.size = sizeof(sys_emu_state_t);
		ret = sys_storage_ext_get_emu_state(&emu_state);
		
		if (ret == 0)
		{
			int disc_emulation = emu_state.disc_emulation;
			
			if (disc_emulation != EMU_OFF)
			{
				switch (disc_emulation)
				{
					case EMU_PS3:
						*iso_disctype = DISC_TYPE_PS3_BD;
					break;
					
					case EMU_PS2_DVD:
						*iso_disctype = DISC_TYPE_PS2_DVD;
					break;
					
					case EMU_PS2_CD:
						*iso_disctype = DISC_TYPE_PS2_CD;
					break;
					
					case EMU_PSX:
						*iso_disctype = DISC_TYPE_PSX_CD;
					break;
					
					case EMU_BD:
						if (edt != DISC_TYPE_NONE)
							*iso_disctype = edt;
						else
							*iso_disctype = DISC_TYPE_BDMR_SR;
					break;
					
					case EMU_DVD:
						*iso_disctype = DISC_TYPE_DVD;
					break;
					
					default:
						*iso_disctype = DISC_TYPE_UNKNOWN;
						
				}
			}
		}
	}
		
	return 0;
}


static unsigned int ejected_realdisc;

int cobra_send_fake_disc_eject_event(void)
{
	sys_storage_ext_get_disc_type(&ejected_realdisc, NULL, NULL);
	
	sys_storage_ext_fake_storage_event(4, 0, BDVD_DRIVE);
	return sys_storage_ext_fake_storage_event(8, 0, BDVD_DRIVE);	
}

int cobra_send_fake_disc_insert_event(void)
{
	uint64_t param;	
	unsigned int real_disctype, effective_disctype, iso_disctype;
	
	cobra_get_disc_type(&real_disctype, &effective_disctype, &iso_disctype);
	
	if (ejected_realdisc == 0 && real_disctype == 0 && effective_disctype == 0 && iso_disctype == 0)
	{
		//printf("Alll disc types 0, aborting\n");
		return -1;//EABORT;
	}
	
	param = (uint64_t)(ejected_realdisc) << 32ULL;	
	sys_storage_ext_get_disc_type(&ejected_realdisc, NULL, NULL);
	sys_storage_ext_fake_storage_event(7, 0, BDVD_DRIVE);
	return sys_storage_ext_fake_storage_event(3, param, BDVD_DRIVE);
}

static uint32_t *translate_str_array(char *array[], unsigned int num)
{
	uint32_t *out = malloc(num * sizeof(uint32_t));
	
	for (unsigned int i = 0; i < num; i++)
	{
		out[i] = (uint32_t)(uint64_t)array[i];
	}
	
	return out;
}

int cobra_mount_ps3_disc_image(char *files[], unsigned int num)
{
	uint32_t *files32;
	int ret;
	
	if (!files)
		return EINVAL;
	
	files32 = translate_str_array(files, num);
	ret = sys_storage_ext_mount_ps3_discfile(num, files32);
	free(files32);
	
	return ret;
}

int cobra_mount_dvd_disc_image(char *files[], unsigned int num)
{
	uint32_t *files32;
	int ret;
	
	if (!files)
		return EINVAL;
	
	files32 = translate_str_array(files, num);
	ret = sys_storage_ext_mount_dvd_discfile(num, files32);
	free(files32);
	
	return ret;
}

int cobra_mount_bd_disc_image(char *files[], unsigned int num)
{
	uint32_t *files32;
	int ret;
	
	if (!files)
		return EINVAL;
	
	files32 = translate_str_array(files, num);
	ret = sys_storage_ext_mount_bd_discfile(num, files32);
	free(files32);
	
	return ret;
}

int cobra_umount_disc_image(void)
{
	int ret = sys_storage_ext_umount_discfile();
	if (ret == -1)
		ret = ENODEV;
	
	return ret;
}

static char *get_blank_iso_path(void)
{
	char *s = malloc(32);
	
	strcpy(s, "/dev_hdd0/");
	s[10] = 'v';
	s[11] = 's';
	s[12] = 'h';
	s[13] = '/';
	s[14] = 't';
	s[15] = 'a';
	s[16] = 's';
	s[17] = 'k';
	s[18] = '.';
	s[19] = 'd';
	s[20] = 'a';
	s[21] = 't';
	s[22] = 0;
	
	return s;
}

char *build_blank_iso(char *title_id)
{
	uint8_t *buf = malloc(128*1024);
	
	memset(buf, 0, 128*1024);
	
	buf[3] = 2;
	buf[0x17] = 0x3F;
	strcpy((char *)buf+0x800, "PlayStation3");
	memcpy(buf+0x810, title_id, 4);
	buf[0x814] = '-';
	memcpy(buf+0x815, title_id+4, 5);
	memset(buf+0x81A, ' ', 0x16);
	buf[0x8000] = 1;
	strcpy((char *)buf+0x8001, "CD001");
	buf[0x8006] = 1;
	memset(buf+0x8008, ' ', 0x20);
	memcpy(buf+0x8028, "PS3VOLUME", 9);
	memset(buf+0x8031, ' ', 0x17);
	buf[0x8050] = buf[0x8057] = 0x40;
	buf[0x8078] = buf[0x807B] = buf[0x807C] = buf[0x807F] = 1;
	buf[0x8081] = buf[0x8082] = 8;
	buf[0x8084] = buf[0x808B] = 0xA;
	buf[0x808C] = 0x14;
	buf[0x8097] = 0x15;
	buf[0x809C] = 0x22;
	buf[0x809E] = buf[0x80A5] = 0x18;
	buf[0x80A7] = buf[0x80AC] = 8;
	buf[0x80AE] = 0x6F;
	buf[0x80AF] = 7;
	buf[0x80B0] = 0x16;
	buf[0x80B1] = 2;
	buf[0x80B2] = 0x2B;
	buf[0x80B3] = buf[0x80B5] = 2;
	buf[0x80B8] = buf[0x80BB] = buf[0x80BC] = 1;
	memcpy(buf+0x80be, "PS3VOLUME", 9);
	memset(buf+0x80C7, ' ', 0x266);
	strcpy((char *)buf+0x832d, "2011072202451800");
	strcpy((char *)buf+0x833e, "0000000000000000");
	strcpy((char *)buf+0x834f, "0000000000000000");
	strcpy((char *)buf+0x8360, "0000000000000000");
	buf[0x8371] = 1;
	buf[0x8800] = 2;
	strcpy((char *)buf+0x8801, "CD001");
	buf[0x8806] = 1;
	buf[0x8829] = 'P';
	buf[0x882B] = 'S';
	buf[0x882D] = '3';
	buf[0x882F] = 'V';
	buf[0x8831] = 'O';
	buf[0x8833] = 'L';
	buf[0x8835] = 'U';
	buf[0x8837] = 'M';
	buf[0x8839] = 'E';
	buf[0x8850] = buf[0x8857] = 0x40;
	strcpy((char *)buf+0x8858, "%/@");
	buf[0x8878] = buf[0x887B] = buf[0x887C] = buf[0x887F] = 1;
	buf[0x8881] = buf[0x8882] = 8;
	buf[0x8884] = buf[0x888B] = 0xA;
	buf[0x888C] = 0x16;
	buf[0x8897] = 0x17;
	buf[0x889C] = 0x22;
	buf[0x889E] = buf[0x88A5] = 0x19;
	buf[0x88A7] = buf[0x88AC] = 8;
	buf[0x88AE] = 0x6F;
	buf[0x88AF] = 7;
	buf[0x88B0] = 0x16;
	buf[0x88B1] = 2;
	buf[0x88B2] = 0x2B;
	buf[0x88B3] = buf[0x88B5] = 2;
	buf[0x88B8] = buf[0x88BB] = buf[0x88BC] = 1;
	buf[0x88BF] = 'P';
	buf[0x88C1] = 'S';
	buf[0x88C3] = '3';
	buf[0x88C5] = 'V';
	buf[0x88C7] = 'O';
	buf[0x88C9] = 'L';
	buf[0x88CB] = 'U';
	buf[0x88CD] = 'M';
	buf[0x88CF] = 'E';
	
	strcpy((char *)buf+0x8B2D, "2011072202451800");
	strcpy((char *)buf+0x8B3E, "0000000000000000");
	strcpy((char *)buf+0x8B4F, "0000000000000000");
	strcpy((char *)buf+0x8b60, "0000000000000000");
	buf[0x8B71] = 1;
	buf[0x9000] = 0xFF;
	strcpy((char *)buf+0x9001, "CD001");
	buf[0xA000] = 1;
	buf[0xA002] = 0x18;
	buf[0xA006] = 1;
	buf[0xA800] = 1;
	buf[0xA805] = 0x18;
	buf[0xA807] = 1;
	buf[0xB000] = 1;
	buf[0xB002] = 0x19;
	buf[0xB006] = 1;
	buf[0xB800] = 1;
	buf[0xB805] = 0x19;
	buf[0xB807] = 1;
	buf[0xC000] = 0x28;
	buf[0xC002] = buf[0xC009] = 0x18;
	buf[0xC00B] = buf[0xC010] = 8;
	buf[0xC012] = 0x6F;
	buf[0xC013] = 7;
	buf[0xC014] = 0x16;
	buf[0xC015] = 2;
	buf[0xC016] = 0x2B;
	buf[0xC017] = buf[0xC019] = 2;
	buf[0xC01C] = buf[0xC01F] = buf[0xC020] = 1;
	buf[0xC028] = 0x28;
	buf[0xC02A] = buf[0xC031] = 0x18;
	buf[0xC033] = buf[0xC038] = 8;
	buf[0xC03A] = 0x6F;
	buf[0xC03B] = 7;
	buf[0xC03C] = 0x16;
	buf[0xC03D] = 2;
	buf[0xC03E] = 0x2B;
	buf[0xC03F] = buf[0xC041] = 2;
	buf[0xC044] = buf[0xC047] = buf[0xC048] = buf[0xC049] = 1;
	buf[0xC800] = 0x28;
	buf[0xC802] = buf[0xC809] = 0x19;
	buf[0xC80B] = buf[0xC810] = 8;
	buf[0xC812] = 0x6F;
	buf[0xC813] = 7;
	buf[0xC814] = 0x16;
	buf[0xC815] = 2;
	buf[0xC816] = 0x2B;
	buf[0xC817] = buf[0xC819] = 2;
	buf[0xC81C] = buf[0xC81F] = buf[0xC820] = 1;
	buf[0xC828] = 0x28;
	buf[0xC82A] = buf[0xC831] = 0x19;
	buf[0xC833] = buf[0xC838] = 8;
	buf[0xC83A] = 0x6F;
	buf[0xC83B] = 7;
	buf[0xC83C] = 0x16;
	buf[0xC83D] = 2;
	buf[0xC83E] = 0x2B;
	buf[0xC83F] = buf[0xC841] = 2;
	buf[0xC844] = buf[0xC847] = buf[0xC848] = buf[0xC849] = 1;
	
	char *ret = get_blank_iso_path();
	
	FILE *f = fopen(ret, "wb");
	if (fwrite(buf, 1, 128*1024, f) != (128*1024))
	{
		fclose(f);
		free(buf);
		free(ret);
		return NULL;
	}
	
	fclose(f);
	free(buf);
	return ret;
}

int cobra_mount_ps2_disc_image(char *files[], int num, TrackDef *tracks, unsigned int num_tracks)
{
	uint32_t *files32;
	int ret;
	ScsiTrackDescriptor scsi_tracks[100];
	
	if (tracks)
	{
		for (int i = 0; i < num_tracks; i++)
		{
			scsi_tracks[i].adr_control = (!tracks[i].is_audio) ? 0x14 : 0x10;
			scsi_tracks[i].track_number = i+1;
			scsi_tracks[i].track_start_addr = tracks[i].lba;
		}	
	}
	
	files32 = translate_str_array(files, num);
	ret = sys_storage_ext_mount_ps2_discfile(num, files32, num_tracks, (tracks) ? scsi_tracks : NULL);
	free(files32);
	
	return ret;
}


int cobra_load_vsh_plugin(unsigned int slot, char *path, void *arg, uint32_t arg_size)
{
	lv2syscall5(8, SYSCALL8_OPCODE_LOAD_VSH_PLUGIN, slot, (uint64_t)path, (uint64_t)arg, arg_size);
	return_to_user_prog(s32);
}

int cobra_unload_vsh_plugin(unsigned int slot)
{
	lv2syscall2(8, SYSCALL8_OPCODE_UNLOAD_VSH_PLUGIN, slot);
	return_to_user_prog(s32);
}

int cobra_mount_psx_disc_image(char *file, TrackDef *tracks, unsigned int num_tracks)
{	
	ScsiTrackDescriptor scsi_tracks[100];	
	
	if (!file || num_tracks >= 100)
		return EINVAL;
	
	memset(scsi_tracks, 0, sizeof(scsi_tracks));
	
	for (int i = 0; i < num_tracks; i++)
	{
		scsi_tracks[i].adr_control = (!tracks[i].is_audio) ? 0x14 : 0x10;
		scsi_tracks[i].track_number = i+1;
		scsi_tracks[i].track_start_addr = tracks[i].lba;
	}
	
	return sys_storage_ext_mount_psx_discfile(file, num_tracks, scsi_tracks);
}




