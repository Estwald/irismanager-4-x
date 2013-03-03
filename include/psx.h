#ifndef PSX_H
#define PSX_H


typedef struct {
    u32 version;
    char mc1[256];
    char mc2[256];
    u32 video;
    u32 flags;

} psx_opt;

extern psx_opt psx_options;

void unload_psx_payload();

void LoadPSXOptions(char *path);
int SavePSXOptions(char *path);

void draw_psx_options(float x, float y, int index);
void draw_psx_options2(float x, float y, int index);

int get_psx_memcards(void);
void psx_launch(void);

int psx_iso_prepare(char *path, char *name);
int psx_cd_with_cheats(void);

void Reset1_BDVD(void);
void Reset2_BDVD(void);

extern u8 psx_id[32];

int get_disc_ready(void);
u8 get_psx_region_cd(void);
u8 get_psx_region_file(char *path);


#define STOP_BDVD  0
#define START_BDVD 1
#define EJECT_BDVD 2
#define LOAD_BDVD  3
#define NOWAIT_BDVD 128

void Eject_BDVD(int mode);

#endif