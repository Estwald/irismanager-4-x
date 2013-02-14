#include "ftp.h"
//    This file is part of OpenPS3FTP.

//    OpenPS3FTP is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

//    OpenPS3FTP is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.

//    You should have received a copy of the GNU General Public License
//    along with OpenPS3FTP.  If not, see <http://www.gnu.org/licenses/>.

// some changes made by Hermes

#define FTPPORT		21	// port to start ftp server on
#define BUFFER_SIZE	16384	// the default buffer size used in file transfers, in bytes
#define DISABLE_PASS	1	// whether or not to disable the checking of the password (1 - yes, 0 - no)

// tested buffer values (smaller buffer size allows for more connections): 
// <= 4096 - doesn't even connect
// == 8192 - works, but transfer speed is a little lesser compared to 16k or 32k
// == 16384 - works great - similar to 32768
// == 32768 - works great - similar to 16384
// >= 65536 - POS, slowest transfer EVER.

const char* VERSION = "1.4";	// used in the welcome message and displayed on-screen

#include <assert.h>
#include <fcntl.h>

#include <lv2/sysfs.h>

#include <sysutil/video.h>
#include <sysutil/sysutil.h>


#include <net/net.h>
#include <sys/thread.h>

#include "common.h"
//#include "sconsole.h"
// default login details
#define D_USER "root"
#define D_PASS "openbox"

int ftp_perms = 0;
static int ftp_initialized = 0;

static sys_ppu_thread_t thread_id, thread_id2;

char ftp_ip_str[256] = "";

static char userpass[128];
static char status[128];
static int exitapp = 0;

static int active_threads = 0;


// temporary method until something new comes up
// will work only if internet connection is available
static void ipaddr_get(void * unused)
{
	// connect to some server and add to the status message
	int ip_s = -1;
	
    //active_threads++;

	sprintf(status, "Status: Listening on Port: %i", FTPPORT);
	
	if(sconnect(&ip_s, "8.8.8.8", 53) == 0)
	{
		netSocketInfo p;
		netGetSockInfo(FD(ip_s), &p, 1);
		sprintf(status, "Status: Listening on IP: %s Port: %i", inet_ntoa(p.local_adr), FTPPORT);
        sprintf(ftp_ip_str, "FTP active (%s:%i)", inet_ntoa(p.local_adr), FTPPORT);
        
	}
	
	sclose(&ip_s);
	
    //active_threads--;
	//sysThreadExit(0);
}

static void handleclient(void * conn_s_p)
{
	int conn_s = (int) (u64) conn_s_p; // main communications socket
	int data_s = -1; // data socket
	
	int connactive = 1; // whether the ftp connection is active or not
	int dataactive = 0; // prevent the data connection from being closed at the end of the loop
	int loggedin = 0; // whether the user is logged in or not
	
	char user[64]; // stores the username that the user entered
	char rnfr[1024]; // stores the path/to/file for the RNFR command
	
	char cwd[1024]; // Current Working Directory
	int rest = 0; // for resuming file transfers
	
	char buffer[1024];

    active_threads++;
	
	// generate pasv output
	netSocketInfo p;
    netGetSockInfo(FD(conn_s), &p, 1);
	
	srand(conn_s);
	int p1 = (rand() % 251) + 4;
	int p2 = rand() % 256;
	
	char pasv_output[64];
	//caca sprintf(pasv_output, "227 Entering Passive Mode (%i,%i,%i,%i,%i,%i)\r\n", NIPQUAD(p.local_adr.s_addr), p1, p2);
	
	// set working directory
	strcpy(cwd, "/");
	
	// welcome message
	ssend(conn_s, "220-OpenPS3FTP by @jjolano\r\n");
	sprintf(buffer, "220 Version %s\r\n", VERSION);
	ssend(conn_s, buffer);
	
	while(exitapp == 0 && connactive == 1 && recv(conn_s, buffer, 1023, 0) > 0)
	{
		// get rid of the newline at the end of the string
		buffer[strcspn(buffer, "\n")] = '\0';
		buffer[strcspn(buffer, "\r")] = '\0';
		
		char cmd[16], param[1024];
		int split = ssplit(buffer, cmd, 15, param, 255);

        
		
		if(loggedin == 1)
		{
			// available commands when logged in
			if(strcasecmp(cmd, "CWD") == 0)
			{
				char tempcwd[1024];
				strcpy(tempcwd, cwd);
				
				if(split == 1)
				{
					absPath(tempcwd, param, cwd);
				}
				
				if(isDir(tempcwd))
				{
					strcpy(cwd, tempcwd);
					sprintf(buffer, "250 Directory change successful: %s\r\n", cwd);
					ssend(conn_s, buffer);
				}
				else
				{
					ssend(conn_s, "550 Cannot access directory\r\n");
				}
			}
			else
			if(strcasecmp(cmd, "CDUP") == 0)
			{
				int pos = strlen(cwd) - 2;
				
				for(int i = pos; i > 0; i--)
				{
					if(cwd[i] == '/' && i < pos)
					{
						break;
					}
					else
					{
						cwd[i] = '\0';
					}
				}
				
				sprintf(buffer, "250 Directory change successful: %s\r\n", cwd);
				ssend(conn_s, buffer);
			}
			else
			if(strcasecmp(cmd, "PASV") == 0)
			{
				rest = 0;
				
				int data_ls = slisten((p1 * 256) + p2, 1);
				
				if(data_ls > 0)
				{
					ssend(conn_s, pasv_output);
					
					data_s = accept(data_ls, NULL, NULL);
					
					if(data_s >= 0)
					{
						dataactive = 1;
					}
					else
					{
						ssend(conn_s, "451 Data connection failed\r\n");
					}
				}
				else
				{
					ssend(conn_s, "451 Cannot create data socket\r\n");
				}
				
				sclose(&data_ls);
			}
			else
			if(strcasecmp(cmd, "PORT") == 0)
			{
				if(split == 1)
				{
					rest = 0;
					
					char data[6][4];
					char *splitstr = strtok(param, ",");
					
					int i = 0;
					while(i < 6 && splitstr != NULL)
					{
						strcpy(data[i++], splitstr);
						splitstr = strtok(NULL, ",");
					}
					
					if(i == 6)
					{
						char ipaddr[16];
						sprintf(ipaddr, "%s.%s.%s.%s", data[0], data[1], data[2], data[3]);
						
						if(sconnect(&data_s, ipaddr, ((atoi(data[4]) * 256) + atoi(data[5]))) == 0)
						{
							ssend(conn_s, "200 PORT command successful\r\n");
							dataactive = 1;
						}
						else
						{
							ssend(conn_s, "451 Data connection failed\r\n");
						}
					}
					else
					{
						ssend(conn_s, "501 Insufficient connection info\r\n");
					}
				}
				else
				{
					ssend(conn_s, "501 No connection info given\r\n");
				}
			}
			else
			if(strcasecmp(cmd, "LIST") == 0)
			{
				if(data_s >= 0)
				{
					char tempcwd[1024];
					strcpy(tempcwd, cwd);
					
					if(split == 1)
					{
						absPath(tempcwd, param, cwd);
					}
					
					void listcb(sysFSDirent entry)
					{
						char filename[1024];
						absPath(filename, entry.d_name, cwd);
						
						sysFSStat buf;
                        int n;
//                        sys8_perm_mode(ftp_perms);
                        n=sysLv2FsStat(filename, &buf);
//                        sys8_perm_mode(0);
						if(n<0) return;
						
						char timebuf[16];
						strftime(timebuf, 15, "%b %d %H:%M", localtime(&buf.st_mtime));
						
						sprintf(buffer, "%s%s%s%s%s%s%s%s%s%s   1 root       root       %llu %s %s\r\n",
							((buf.st_mode & S_IFDIR) != 0) ? "d" : "-", 
							((buf.st_mode & S_IRUSR) != 0) ? "r" : "-",
							((buf.st_mode & S_IWUSR) != 0) ? "w" : "-",
							((buf.st_mode & S_IXUSR) != 0) ? "x" : "-",
							((buf.st_mode & S_IRGRP) != 0) ? "r" : "-",
							((buf.st_mode & S_IWGRP) != 0) ? "w" : "-",
							((buf.st_mode & S_IXGRP) != 0) ? "x" : "-",
							((buf.st_mode & S_IROTH) != 0) ? "r" : "-",
							((buf.st_mode & S_IWOTH) != 0) ? "w" : "-",
							((buf.st_mode & S_IXOTH) != 0) ? "x" : "-",
							(unsigned long long)buf.st_size, timebuf, entry.d_name);
						
						ssend(data_s, buffer);
					}
					
					ssend(conn_s, "150 Accepted data connection\r\n");
					
					if(slist(isDir(tempcwd) ? tempcwd : cwd, listcb) >= 0)
					{
						ssend(conn_s, "226 Transfer complete\r\n");
					}
					else
					{
						ssend(conn_s, "550 Cannot access directory\r\n");
					}
				}
				else
				{
					ssend(conn_s, "425 No data connection\r\n");
				}
			}
			else
			if(strcasecmp(cmd, "MLSD") == 0)
			{
				if(data_s >= 0)
				{
					char tempcwd[1024];
					strcpy(tempcwd, cwd);
					
					if(split == 1)
					{
						absPath(tempcwd, param, cwd);
					}
					
					void listcb(sysFSDirent entry)
					{
						char filename[1024];
						absPath(filename, entry.d_name, cwd);
						
						sysFSStat buf;
						int n;

//                        sys8_perm_mode(ftp_perms);
                        n=sysLv2FsStat(filename, &buf);
//                        sys8_perm_mode(0);
						if(n<0) return;
						
						char timebuf[16];
						strftime(timebuf, 15, "%Y%m%d%H%M%S", localtime(&buf.st_mtime));
						
						char dirtype[2];
						if(strcmp(entry.d_name, ".") == 0)
						{
							strcpy(dirtype, "c");
						}
						else
						if(strcmp(entry.d_name, "..") == 0)
						{
							strcpy(dirtype, "p");
						}
						else
						{
							dirtype[0] = '\0';
						}
						
						sprintf(buffer, "type=%s%s;siz%s=%llu;modify=%s;UNIX.mode=0%i%i%i;UNIX.uid=root;UNIX.gid=root; %s\r\n",
							dirtype,
							((buf.st_mode & S_IFDIR) != 0) ? "dir" : "file",
							((buf.st_mode & S_IFDIR) != 0) ? "d" : "e", (unsigned long long)buf.st_size, timebuf,
							(((buf.st_mode & S_IRUSR) != 0) * 4 +
								((buf.st_mode & S_IWUSR) != 0) * 2 +
								((buf.st_mode & S_IXUSR) != 0) * 1),
							(((buf.st_mode & S_IRGRP) != 0) * 4 +
								((buf.st_mode & S_IWGRP) != 0) * 2 +
								((buf.st_mode & S_IXGRP) != 0) * 1),
							(((buf.st_mode & S_IROTH) != 0) * 4 +
								((buf.st_mode & S_IWOTH) != 0) * 2 +
								((buf.st_mode & S_IXOTH) != 0) * 1),
							entry.d_name);
						
						ssend(data_s, buffer);
					}
					
					ssend(conn_s, "150 Accepted data connection\r\n");
					
					if(slist(isDir(tempcwd) ? tempcwd : cwd, listcb) >= 0)
					{
						ssend(conn_s, "226 Transfer complete\r\n");
					}
					else
					{
						ssend(conn_s, "550 Cannot access directory\r\n");
					}
				}
				else
				{
					ssend(conn_s, "425 No data connection\r\n");
				}
			}
			else
			if(strcasecmp(cmd, "STOR") == 0)
			{
				if(data_s >= 0)
				{
					if(split == 1)
					{
						char filename[1024];
						absPath(filename, param, cwd);
						
						ssend(conn_s, "150 Accepted data connection\r\n");
						
						if(recvfile(data_s, filename, BUFFER_SIZE, (s64)rest) == 0)
						{
							ssend(conn_s, "226 Transfer complete\r\n");
						}
						else
						{
							ssend(conn_s, "451 Transfer failed\r\n");
						}
					}
					else
					{
						ssend(conn_s, "501 No file specified\r\n");
					}
				}
				else
				{
					ssend(conn_s, "425 No data connection\r\n");
				}
			}
			else
			if(strcasecmp(cmd, "RETR") == 0)
			{
				if(data_s >= 0)
				{
					if(split == 1)
					{
						char filename[1024];
						absPath(filename, param, cwd);
						
						if(exists(filename) == 0)
						{
							ssend(conn_s, "150 Accepted data connection\r\n");
							
							if(sendfile(data_s, filename, BUFFER_SIZE, (s64)rest) == 0)
							{
								ssend(conn_s, "226 Transfer complete\r\n");
							}
							else
							{
								ssend(conn_s, "451 Transfer failed\r\n");
							}
						}
						else
						{
							ssend(conn_s, "550 File does not exist\r\n");
						}
					}
					else
					{
						ssend(conn_s, "501 No file specified\r\n");
					}
				}
				else
				{
					ssend(conn_s, "425 No data connection\r\n");
				}
			}
			else
			if(strcasecmp(cmd, "PWD") == 0)
			{
				sprintf(buffer, "257 \"%s\" is the current directory\r\n", cwd);
				ssend(conn_s, buffer);
			}
			else
			if(strcasecmp(cmd, "TYPE") == 0)
			{
				ssend(conn_s, "200 TYPE command successful\r\n");
				dataactive = 1;
			}
			else
			if(strcasecmp(cmd, "REST") == 0)
			{
				if(split == 1)
				{
					ssend(conn_s, "350 REST command successful\r\n");
					rest = atoi(param);
					dataactive = 1;
				}
				else
				{
					ssend(conn_s, "501 No restart point\r\n");
				}
			}
			else
			if(strcasecmp(cmd, "DELE") == 0)
			{
				if(split == 1)
				{
					char filename[1024];
					absPath(filename, param, cwd);

                    int n;
//                    sys8_perm_mode(ftp_perms);
                    n=sysLv2FsUnlink(filename);
//                    sys8_perm_mode(0);
				
					
					if(n == 0)
					{
						ssend(conn_s, "250 File successfully deleted\r\n");
					}
					else
					{
						ssend(conn_s, "550 Cannot delete file\r\n");
					}
				}
				else
				{
					ssend(conn_s, "501 No filename specified\r\n");
				}
			}
			else
			if(strcasecmp(cmd, "MKD") == 0)
			{
				if(split == 1)
				{
					char filename[1024];
					absPath(filename, param, cwd);

                    int n;
//                    sys8_perm_mode(ftp_perms);
                    n=sysLv2FsMkdir(filename, 0755);
//                    sys8_perm_mode(0);	
					
					if(n == 0)
					{
						sprintf(buffer, "257 \"%s\" was successfully created\r\n", param);
						ssend(conn_s, buffer);
					}
					else
					{
						ssend(conn_s, "550 Cannot create directory\r\n");
					}
				}
				else
				{
					ssend(conn_s, "501 No filename specified\r\n");
				}
			}
			else
			if(strcasecmp(cmd, "RMD") == 0)
			{
				if(split == 1)
				{
					char filename[1024];
					absPath(filename, param, cwd);
					
                    int n;
//                    sys8_perm_mode(ftp_perms);
                    n=sysLv2FsRmdir(filename);
//                    sys8_perm_mode(0);	
					
					if(n == 0)
					{
						ssend(conn_s, "250 Directory was successfully removed\r\n");
					}
					else
					{
						ssend(conn_s, "550 Cannot remove directory\r\n");
					}
				}
				else
				{
					ssend(conn_s, "501 No filename specified\r\n");
				}
			}
			else
			if(strcasecmp(cmd, "RNFR") == 0)
			{
				if(split == 1)
				{
					absPath(rnfr, param, cwd);
					
					if(exists(rnfr) == 0)
					{
						ssend(conn_s, "350 RNFR accepted - ready for destination\r\n");
					}
					else
					{
						ssend(conn_s, "550 RNFR failed - file does not exist\r\n");
					}
				}
				else
				{
					ssend(conn_s, "501 No file specified\r\n");
				}
			}
			else
			if(strcasecmp(cmd, "RNTO") == 0)
			{
				if(split == 1)
				{
					char rnto[1024];
					absPath(rnto, param, cwd);
					
                    int n;
//                    sys8_perm_mode(ftp_perms);
                    n=sysLv2FsRename(rnfr, rnto);
//                    sys8_perm_mode(0);	
					
					if(n == 0)
					{
						ssend(conn_s, "250 File was successfully renamed or moved\r\n");
					}
					else
					{
						ssend(conn_s, "550 Cannot rename or move file\r\n");
					}
				}
				else
				{
					ssend(conn_s, "501 No file specified\r\n");
				}
			}
			else
			if(strcasecmp(cmd, "SITE") == 0)
			{
				if(split == 1)
				{
					char param2[1024];
					split = ssplit(param, cmd, 31, param2, 255);
					
					if(strcasecmp(cmd, "CHMOD") == 0)
					{
						if(split == 1)
						{
							char temp[4], filename[1024];
							split = ssplit(param2, temp, 3, filename, 255);
							
							if(split == 1)
							{
								char perms[5];
								sprintf(perms, "0%s", temp);
								
								// jjolano epic failed here :D (problem was ONLY the absolute path..)
								char absFilePath[1024]; // place-holder for absolute path
								absPath(absFilePath, filename, cwd); // making sure that we use the absolute path
								
								//tested and working for both dir and files :0)
                                int n;
//                                sys8_perm_mode(ftp_perms);
                                n=sysLv2FsChmod(absFilePath, strtol(perms, NULL, 8));
//                                sys8_perm_mode(0);	
					
								if(n == 0) //cleaned up
								{
									ssend(conn_s, "250 File permissions successfully set\r\n");
								}
								else
								{
									ssend(conn_s, "550 Cannot set file permissions\r\n");
								}
							}
							else
							{
								ssend(conn_s, "501 Not enough parameters\r\n");
							}
						}
						else
						{
							ssend(conn_s, "501 No parameters given\r\n");
						}
					}
					else
					if(strcasecmp(cmd, "HELP") == 0)
					{
						ssend(conn_s, "214-Special OpenPS3FTP commands:\r\n");
						ssend(conn_s, " SITE PASSWD <newpassword> - Change your password\r\n");
						ssend(conn_s, " SITE EXITAPP - Remotely quit OpenPS3FTP\r\n");
						ssend(conn_s, " SITE HELP - Show this message\r\n");
						ssend(conn_s, "214 End\r\n");
					}
					else
					if(strcasecmp(cmd, "PASSWD") == 0)
					{
						if(split == 1)
						{/*	
							sysLv2FsFile fd;
							u64 written;
						 
							if(sysLv2FsOpen("/dev_hdd0/game/OFTP00001/USRDIR/passwd", SYS_O_WRONLY | SYS_O_CREAT, &fd, 0, NULL, 0) == 0)
							{
								sysLv2FsWrite(fd, param2, strlen(param2), &written);
								sprintf(buffer, "200 New password: %s\r\n", param2);
								ssend(conn_s, buffer);
                                sysLv2FsClose(fd);
							}
							else*/
							{
								ssend(conn_s, "550 Cannot change FTP password\r\n");
							}
						
							
						}
						else
						{
							ssend(conn_s, "501 No password given\r\n");
						}
					}
					else
					if(strcasecmp(cmd, "EXITAPP") == 0)
					{
						ssend(conn_s, "221 Exiting OpenPS3FTP\r\n");
						exitapp = 1;
					}
					else
					{
						ssend(conn_s, "500 Unknown SITE command\r\n");
					}
				}
				else
				{
					ssend(conn_s, "501 No SITE command specified\r\n");
				}
			}
			else
			if(strcasecmp(cmd, "NOOP") == 0)
			{
				ssend(conn_s, "200 NOOP command successful\r\n");
			}
			else
			if(strcasecmp(cmd, "NLST") == 0)
			{
				if(data_s >= 0)
				{
					char tempcwd[1024];
					strcpy(tempcwd, cwd);
					
					if(split == 1)
					{
						absPath(tempcwd, param, cwd);
					}
					
					void listcb(sysFSDirent entry)
					{
						sprintf(buffer, "%s\r\n", entry.d_name);
						ssend(data_s, buffer);
					}
					
					ssend(conn_s, "150 Accepted data connection\r\n");
					
					if(slist(isDir(tempcwd) ? tempcwd : cwd, listcb) >= 0)
					{
						ssend(conn_s, "226 Transfer complete\r\n");
					}
					else
					{
						ssend(conn_s, "550 Cannot access directory\r\n");
					}
				}
				else
				{
					ssend(conn_s, "425 No data connection\r\n");
				}
			}
			else
			if(strcasecmp(cmd, "MLST") == 0)
			{
				char tempcwd[1024];
				strcpy(tempcwd, cwd);
				
				if(split == 1)
				{
					absPath(tempcwd, param, cwd);
				}
				
				void listcb(sysFSDirent entry)
				{
					char filename[1024];
					absPath(filename, entry.d_name, cwd);
					
					sysFSStat buf;
                    int n;
//                    sys8_perm_mode(ftp_perms);
                    n=sysLv2FsStat(filename, &buf);
//                    sys8_perm_mode(0);	
					
					if(n<0) return;
					
					char timebuf[16];
					strftime(timebuf, 15, "%Y%m%d%H%M%S", localtime(&buf.st_mtime));
					
					char dirtype[2];
					if(strcmp(entry.d_name, ".") == 0)
					{
						strcpy(dirtype, "c");
					}
					else
					if(strcmp(entry.d_name, "..") == 0)
					{
						strcpy(dirtype, "p");
					}
					else
					{
						dirtype[0] = '\0';
					}
					
					sprintf(buffer, " type=%s%s;siz%s=%llu;modify=%s;UNIX.mode=0%i%i%i;UNIX.uid=root;UNIX.gid=root; %s\r\n",
						dirtype, ((buf.st_mode & S_IFDIR) != 0) ? "dir" : "file",
						((buf.st_mode & S_IFDIR) != 0) ? "d" : "e", (unsigned long long)buf.st_size, timebuf,
						(((buf.st_mode & S_IRUSR) != 0) * 4 +
							((buf.st_mode & S_IWUSR) != 0) * 2 +
							((buf.st_mode & S_IXUSR) != 0) * 1),
						(((buf.st_mode & S_IRGRP) != 0) * 4 +
							((buf.st_mode & S_IWGRP) != 0) * 2 +
							((buf.st_mode & S_IXGRP) != 0) * 1),
						(((buf.st_mode & S_IROTH) != 0) * 4 +
							((buf.st_mode & S_IWOTH) != 0) * 2 +
							((buf.st_mode & S_IXOTH) != 0) * 1),
						entry.d_name);
					
					ssend(conn_s, buffer);
				}
				
				ssend(conn_s, "250-Directory Listing\r\n");
				slist(isDir(tempcwd) ? tempcwd : cwd, listcb);
				ssend(conn_s, "250 End\r\n");
			}
			else
			if(strcasecmp(cmd, "QUIT") == 0 || strcasecmp(cmd, "BYE") == 0)
			{
				ssend(conn_s, "221 Bye!\r\n");
				connactive = 0;
			}
			else
			if(strcasecmp(cmd, "FEAT") == 0)
			{
				ssend(conn_s, "211-Extensions supported:\r\n");
				
				static char *feat_cmds[] =
				{
					"PASV",
					"PORT",
					"SIZE",
					"CDUP",
					"MLSD",
					"MLST type*;size*;modify*;UNIX.mode*;UNIX.uid*;UNIX.gid*;",
					"REST STREAM",
					"SITE CHMOD",
					"SITE PASSWD",
					"SITE EXITAPP"
				};
				
				const int feat_cmds_count = sizeof(feat_cmds) / sizeof(char *);
				
				for(int i = 0; i < feat_cmds_count; i++)
				{
					sprintf(buffer, " %s\r\n", feat_cmds[i]);
					ssend(conn_s, buffer);
				}
				
				ssend(conn_s, "211 End\r\n");
			}
			else
			if(strcasecmp(cmd, "SIZE") == 0)
			{
				if(split == 1)
				{
					char filename[1024];
					absPath(filename, param, cwd);
					
					sysFSStat buf;
                    int n;
//                    sys8_perm_mode(ftp_perms);
                    n=sysLv2FsStat(filename, &buf);
//                    sys8_perm_mode(0);	
					
					if(n == 0)
					{
						sprintf(buffer, "213 %llu\r\n", (unsigned long long)buf.st_size);
						ssend(conn_s, buffer);
					}
					else
					{
						ssend(conn_s, "550 File does not exist\r\n");
					}
				}
				else
				{
					ssend(conn_s, "501 No file specified\r\n");
				}
			}
			else
			if(strcasecmp(cmd, "SYST") == 0)
			{
				ssend(conn_s, "215 UNIX Type: L8\r\n");
			}
			else
			if(strcasecmp(cmd, "USER") == 0 || strcasecmp(cmd, "PASS") == 0)
			{
				ssend(conn_s, "230 You are already logged in\r\n");
			}
			else
			{
				ssend(conn_s, "500 Unrecognized command\r\n");
			}
			
			if(dataactive == 1)
			{
				dataactive = 0;
			}
			else
			{
				sclose(&data_s);
			}
		}
		else
		{
			// available commands when not logged in
			if(strcasecmp(cmd, "USER") == 0)
			{
				if(split == 1)
				{
					if(DISABLE_PASS == 1)
					{
                        
						//ssend(conn_s, "230 Welcome to OpenPS3FTP!\r\n");
                        sprintf(buffer, "331 User %s OK. Password required\r\n", param);
                        ssend(conn_s, buffer);
                        //loggedin = 1;
					}
					else
					{
						strcpy(user, param);
						sprintf(buffer, "331 User %s OK. Password required\r\n", param);
						ssend(conn_s, buffer);
					}
				}
				else
				{
					ssend(conn_s, "501 No user specified\r\n");
				}
			}
			else
			if(strcasecmp(cmd, "PASS") == 0)
			{
				if(split == 1)
				{
					if(DISABLE_PASS == 1 || (strcmp(D_USER, user) == 0 && strcmp(userpass, param) == 0))
					{
						ssend(conn_s, "230 Welcome to OpenPS3FTP!\r\n");
						loggedin = 1;
					}
					else
					{
						ssend(conn_s, "430 Invalid username or password\r\n");
					}
				}
				else
				{
					ssend(conn_s, "501 No password given\r\n");
				}
			}
			else
			if(strcasecmp(cmd, "QUIT") == 0 || strcasecmp(cmd, "BYE") == 0)
			{
				ssend(conn_s, "221 Bye!\r\n");
				connactive = 0;
			}
			else
			{
				ssend(conn_s, "530 Not logged in\r\n");
			}
		}
	}
	
	sclose(&conn_s);
	sclose(&data_s);
	
    active_threads--;

	sysThreadExit(0);
}

static void handleconnections(void * unused)
{
	int list_s = slisten(FTPPORT, 5);
	
	if(list_s > 0)
	{
		
		
		while(exitapp == 0  || active_threads > 0)
		{
            int conn_s;
		    sys_ppu_thread_t id;

			if((conn_s = accept(list_s, NULL, NULL)) > 0)
			{
            char string[64];

            sprintf(string, "%s%i", "ClientCmdHandler", active_threads);
				sysThreadCreate(&id, handleclient, (void *) (u64) conn_s, 1000, 0x40000 + BUFFER_SIZE, 0, string);
			//	usleep(100000); // this should solve some connection issues
			}

            usleep(10000);
		}
		
		sclose(&list_s);
	}
	
	sysThreadExit(0);
}


int ftp_init()
{

    if(ftp_initialized) return 1;

    netInitialize();

    // format version string
	char version[32];
	sprintf(version, "Version %s", VERSION);

    // check if dev_flash is mounted rw
	//int rwflashmount = (exists("/dev_blind") == 0 || exists("/dev_rwflash") == 0 || exists("/dev_fflash") == 0 || exists("/dev_Alejandro") == 0);
	
	// load default password
	strcpy(userpass, D_PASS);

    #if 0
    // load password file
	sysLv2FsFile fd;
	
	if(sysLv2FsOpen("/dev_hdd0/game/OFTP00001/USRDIR/passwd", SYS_O_RDONLY, &fd, S_IRWXU | S_IRWXG | S_IRWXO, NULL, 0) == 0)
	{
		u64 read;
		sysLv2FsRead(fd, userpass, 31, &read);
	}
	
	sysLv2FsClose(fd);
    #endif

    // start listening for connections
	sysThreadCreate(&thread_id2, ipaddr_get, 0, 1000, 0x10000, 0, "RetrieveIPAddress");
    u64 retval;
    sysThreadJoin(thread_id2, &retval);

	sysThreadCreate(&thread_id, handleconnections, 0, 999, 0x10000, 0, "ServerConnectionHandler");
	
    
    
    //ipaddr_get(0ULL);

    ftp_initialized = 1;
    exitapp = 0;

	return 0;
}

void ftp_deinit()
{
    if(!ftp_initialized) return;

    exitapp = 1;
    
    netDeinitialize();

    u64 retval;
    sysThreadJoin(thread_id, &retval);

    ftp_initialized = 0;

    memset(ftp_ip_str, 0, sizeof(ftp_ip_str));
}
