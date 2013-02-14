#ifndef FTP_H
#define FTP_H

#include "utils.h"

extern int ftp_perms;
extern char ftp_ip_str[256];
int ftp_init();
void ftp_deinit();

#endif
