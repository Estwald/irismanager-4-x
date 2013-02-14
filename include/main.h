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


void load_gamecfg (int current_dir);

#endif

