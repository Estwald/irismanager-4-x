#ifndef ARCHIVE_MANAGER_H
#define ARCHIVE_MANAGER_H

#include "ntfs.h"

void archive_manager();

extern const DISC_INTERFACE *disc_ntfs[8];

// mounts from /dev_usb000 to 007
extern ntfs_md *mounts[8];
extern int mountCount[8];
extern int automountCount[8];

extern u32 ports_cnt;
extern u32 old_ports_cnt;

int NTFS_Event_Mount(int id);
int NTFS_UnMount(int id);
int NTFS_UnMount_dev(int id, char * name);
void NTFS_UnMountAll(void);
int NTFS_Test_Device(char *name);

int launch_iso_game(char *path);


#endif