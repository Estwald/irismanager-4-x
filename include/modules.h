#ifndef _MODULES_H
#define _MODULES_H

#define SIZE_SPRX_ISO 13688
extern unsigned char sprx_iso[SIZE_SPRX_ISO];

#define PLUGIN_ISO "%s/sprx_iso" // note: it is also defined as sprx_iso from main.c

#define SIZE_SPRX_SM 7456
extern unsigned char sprx_sm[SIZE_SPRX_SM];
#define PLUGIN_SM "%s/sprx_sm" // note: it is also defined as sprx_sm from main.c

#define SIZE_SPRX_MONITOR 4904
extern unsigned char sprx_monitor[SIZE_SPRX_MONITOR];
#define PLUGIN_MONITOR "%s/sprx_sm" // note: it is also defined as sprx_sm from main.c

#endif