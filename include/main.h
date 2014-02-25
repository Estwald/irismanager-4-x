#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>

typedef struct PngDatas {
	
	void * png_in;		// ignored except if char *filename == NULL in LoadPNG()
	uint32_t png_size;  // ignored except if char *filename == NULL  in LoadPNG()
	
	void * bmp_out;		// internally allocated (bmp 32 bits color ARGB format)

	int	wpitch;			// output width pitch in bytes
	int width;			// output
	int height;			// output
	
} PngDatas;

typedef struct JpgDatas {
	
	void * jpg_in;		// ignored except if char *filename == NULL in LoadPNG()
	uint32_t jpg_size;  // ignored except if char *filename == NULL  in LoadPNG()
	
	void * bmp_out;		// internally allocated (bmp 32 bits color ARGB format)

	int	wpitch;			// output width pitch in bytes
	int width;			// output
	int height;			// output
	
} JpgDatas;

int LoadPNG(PngDatas *png, const char *filename);
int LoadJPG(JpgDatas *jpg, char *filename);


// manager config options
#define OPTFLAGS_FTP                    (1 << 0)
#define OPTFLAGS_PLAYMUSIC              (1 << 1)


#define AUTO_BUTTON_REP(v, b) if(v && (old_pad & b)) { \
                                 v++; \
                                 if(v > 20) {v = 0; new_pad |= b;} \
                                 } else v = 0;

#define AUTO_BUTTON_REP3(v, b) if(v && (old_pad & b)) { \
                                 v++; \
                                 if(v > 9) {v = 0; new_pad |= b;} \
                                 } else v = 0;


void load_gamecfg (int current_dir);

#define BIG_PICT 32

extern int scr_grid_games;
extern int scr_grid_w;
extern int scr_grid_h;

extern u8 * png_texture;
extern PngDatas Png_datas[BIG_PICT + 2];
extern u32 Png_offset[BIG_PICT + 2];
extern int Png_iscover[BIG_PICT + 2];

extern PngDatas Png_res[24];
extern u32 Png_res_offset[24];

// HOMEBREW / BDISO / DVDISO / MKV
#define D_FLAG_HOMEB_DPL (31)
#define D_FLAG_HOMEB     (1<<31)
#define D_FLAG_HOMEB_BD  (1<<24)
#define D_FLAG_HOMEB_DVD (1<<23)
#define D_FLAG_HOMEB_MKV (D_FLAG_HOMEB_BD | D_FLAG_HOMEB_DVD)
#define D_FLAG_HOMEB_GROUP (D_FLAG_HOMEB | D_FLAG_HOMEB_MKV)

// DEVICES
#define D_FLAG_HDD0 1
#define D_FLAG_BDVD (1<<11)
#define D_FLAG_NTFS (1<<15)
#define D_FLAG_USB (0x3ff<<1)

// TYPES
#define D_FLAG_PS3_ISO  (1<<24)
#define D_FLAG_PS2_ISO  (D_FLAG_PS3_ISO | (1<<23))
#define D_FLAG_PSX_ISO  (1<<23) // also can be used to identify PS2 game!
#define D_FLAG_MASK_ISO (D_FLAG_PS3_ISO | D_FLAG_HOMEB | D_FLAG_BDVD)

/*

EXAMPLES:

PS3 GAME from BDVD: D_FLAG_BDVD
PS3 GAME from HDD0 (jailbreak) : D_FLAG_HDD0
PS3 GAME from USBx (jailbreak) : (1 << (x+1))

PS3 GAME from HDD0 (ISO) : D_FLAG_PS3_ISO | D_FLAG_HDD0
PS3 GAME from USBx (ISO) : D_FLAG_PS3_ISO | (1 << (x+1))
PS3 GAME from NTFS (ISO) : D_FLAG_PS3_ISO | D_FLAG_NTFS (from USB device with NTFS/EXTx partition)

PS2 GAME from HDD0 (ISO) : D_FLAG_PS2_ISO | D_FLAG_HDD0 (only supported in CFW 4.46 Cobra)

PS1 GAME from BDVD : D_FLAG_PSX_ISO | D_FLAG_BDVD
PS1 GAME from HDD0 (ISO) : D_FLAG_PSX_ISO | D_FLAG_HDD0
PS1 GAME from USBx (ISO) : D_FLAG_PSX_ISO | (1 << (x+1))
PS1 GAME from NTFS (ISO) : D_FLAG_PSX_ISO | D_FLAG_NTFS (from USB device with NTFS/EXTx partition)

Homebrew Mode:

PS3 Homebrew/PS3 : D_FLAG_HOMEB_DPL | (1 << (x+1)) (from USBx depacked in path /dev_usb00x/game/xxxxx)

BD ISO   : D_FLAG_HOMEB_DPL | D_FLAG_HOMEB_BD  | x (from /BDISO  , x = D_FLAG_HDD0 or D_FLAG_NTFS or (1 << (usb_port+1)))
DVD ISO  : D_FLAG_HOMEB_DPL | D_FLAG_HOMEB_DVD | x (from /BDVDISO, x = D_FLAG_HDD0 or D_FLAG_NTFS or (1 << (usb_port+1)))
MKV file : D_FLAG_HOMEB_DPL | D_FLAG_HOMEB_MKV | x (from /MKV    , x = D_FLAG_HDD0 or D_FLAG_NTFS or (1 << (usb_port+1)))

*/

#endif

