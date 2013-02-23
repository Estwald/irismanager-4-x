#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <ppu-types.h>
#include <net/net.h>
#include <net/netctl.h>

#include <sys/file.h>
#include <sys/thread.h>

#include "defines.h"
#include "server.h"
#include "functions.h"
#include "ftp.h"

char passwd[64];

int appstate = 0;

static int ftp_initialized = 0;

char ftp_ip_str[256] = "";

static sys_ppu_thread_t thread_id;

int ftp_init()
{

    if(ftp_initialized) return 1;

    netInitialize();
	netCtlInit();

    union net_ctl_info info;
	
	if(netCtlGetInfo(NET_CTL_INFO_IP_ADDRESS, &info) == 0)
	{
		// start server thread
		
        appstate = 0;
        sprintf(ftp_ip_str, "FTP active (%s:%i)", info.ip_address, 21);
		sysThreadCreate(&thread_id, listener_thread, NULL, 1500, 0x400, 0, "listener");
		
		//s32 fd;
		//u64 read = 0;
		
        /*
		if(sysLv2FsOpen(OFTP_PASSWORD_FILE, SYS_O_RDONLY | SYS_O_CREAT, &fd, 0660, NULL, 0) == 0)
		{
			sysLv2FsRead(fd, passwd, 63, &read);
		}
		
		passwd[read] = '\0';
		sysLv2FsClose(fd);
        
		
		// prevent multiline passwords
		strreplace(passwd, '\r', '\0');
		strreplace(passwd, '\n', '\0');

        */
		
		char dlgmsg[256];
		sprintf(dlgmsg, "OpenPS3FTP %s by jjolano (Twitter: @jjolano)\nWebsite: http://jjolano.dashhacks.com\nDonations: http://bit.ly/gB8CJo\nStatus: FTP Server Active (%s port 21)\n\nPress OK to exit this program.",
			OFTP_VERSION, info.ip_address);
		
		//msgDialogOpen2(mt_ok, dlgmsg, dialog_handler, NULL, NULL);
        ftp_initialized = 1;

        return 0;

	}
	else
	{
		//msgDialogOpen2(mt_ok, OFTP_ERRMSG_NETWORK, dialog_handler, NULL, NULL);

        ftp_initialized = 0;
        netDeinitialize();

	}

    return -1;
}

void ftp_deinit()
{
    if(!ftp_initialized) return;

    appstate = 1;

    netCtlTerm();
    netDeinitialize();

    u64 retval;
    sysThreadJoin(thread_id, &retval);

    ftp_initialized = 0;

    memset(ftp_ip_str, 0, sizeof(ftp_ip_str));
}