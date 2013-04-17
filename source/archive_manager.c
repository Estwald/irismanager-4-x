/* 
    (c) 2011-2013 Hermes/Estwald <www.elotrolado.net>
    IrisManager (HMANAGER port) (c) 2011 D_Skywalk <http://david.dantoine.org>

    HMANAGER4 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    HMANAGER4 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with HMANAGER4.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <math.h>


#include <sysutil/osk.h>
#include "sysutil/sysutil.h"
#include <sys/memory.h>
#include <ppu-lv2.h>
#include <sys/stat.h>

#include <sys/file.h>
#include <lv2/sysfs.h>

#include "archive_manager.h"
#include "main.h"
#include "gfx.h"
#include "pad.h"
#include "ttf_render.h"
#include "utils.h"
#include "osk_input.h"

#define FS_S_IFMT 0170000
#define FS_S_IFDIR 0040000
#define DT_DIR 1

extern char temp_buffer[8192];
extern int firmware;
extern char self_path[MAXPATHLEN];

int sys_fs_mount(char const* deviceName, char const* deviceFileSystem, char const* devicePath, int writeProt);
int sys_fs_umount(char const* devicePath);

void install_pkg(char *path, char *filename);

char * getlv2error(s32 error)
{
	switch(error) {
		case 0x00000000:
			return "Ok";
        case 0x80010001:
            return "The resource is temporarily unavailable";
        case 0x80010002:
            return "Invalid argument";
        case 0x80010003:
            return "Function not implemented";
        case 0x80010004:
            return "Memory allocation failed";
        case 0x80010005:
            return "No such process";
        case 0x80010006:
            return "No such file or directory";
        case 0x80010007:
            return "Exec format error";
        case 0x80010008:
            return "Deadlock condition";
		case 0x80010009:
            return "Operation not permitted";
        case 0x8001000A:
            return "Device busy";
        case 0x8001000B:
            return "The operation is timed out";
        case 0x8001000C:
            return "The operation is aborted ";
        case 0x8001000D:
            return "Invalid memory access";
        case 0x80010012:
            return "The file is a directory";
		case 0x80010013:
            return "Operation canceled";
        case 0x80010014:
            return "File exists";
		case 0x80010015:
            return "Socket is already connected";
		case 0x80010016:
            return "Socket is not connected";
        case 0x8001001B:
            return "Math arg out of domain of func";
		case 0x8001001C:
            return "Math result not representable";
        case 0x8001001D:
            return "Illegal multi-byte sequence in input";
        case 0x8001001E:
            return "File position error";
		case 0x8001001F:
            return "Syscall was interrupted";
        case 0x80010020:
            return "File too large";
        case 0x80010021:
            return "Too many links";
		case 0x80010022:
            return "Too many open files in system";
		case 0x80010023:
            return "No space left on device";
		case 0x80010024:
            return "Not a typewriter";
		case 0x80010025:
            return "Broken pipe";
		case 0x80010026:
            return "Read only file system";
		case 0x80010027:
            return "Illegal seek";
        case 0x80010029:
            return "Permission denied";
        case 0x8001002A:
            return "Invalid file descriptor";
		case 0x8001002B:
            return "I/O error";
        case 0x8001002C:
            return "Too many open files";
        case 0x8001002D:
            return "No such device";
		case 0x8001002E:
            return "Not a directory";
		case 0x8001002F:
            return "No such device or address";
        case 0x80010030:
            return "Cross-device link";
		case 0x80010031:
            return "Trying to read unreadable message";
		case 0x80010032:
            return "Connection already in progress";
		case 0x80010033:
            return "Message too long";
        case 0x80010034:
            return "File or path name too long";
        case 0x80010035:
            return "No record locks available";
        case 0x80010036:
            return "Directory not empty";
        case 0x80010037:
            return "Not supported";
        case 0x80010039:
            return "Value too large for defined data type";
        case 0x8001003A:
            return "Filesystem not mounted";
		
		default:
			return "Error Unknown";
	}
}

static msgType mdialogprogress =   MSG_DIALOG_SINGLE_PROGRESSBAR | MSG_DIALOG_MUTE_ON;
static msgType mdialogprogress2 =   MSG_DIALOG_DOUBLE_PROGRESSBAR | MSG_DIALOG_MUTE_ON;

static volatile int progress_action = 0;

static void progress_callback(msgButton button, void *userdata)
{
    switch(button) {

        case MSG_DIALOG_BTN_OK:
            progress_action = 1;
            break;
        case MSG_DIALOG_BTN_NO:
        case MSG_DIALOG_BTN_ESCAPE:
            progress_action = 2;
            break;
        case MSG_DIALOG_BTN_NONE:
            progress_action = -1;
            break;
        default:
            
		    break;
    }
}


static void update_bar(u32 cpart)
{
    msgDialogProgressBarInc(MSG_PROGRESSBAR_INDEX0, (u32) cpart);
    sysUtilCheckCallback();tiny3d_Flip();
}

static void update_bar2(u32 cpart)
{
    msgDialogProgressBarInc(MSG_PROGRESSBAR_INDEX1, (u32) cpart);
    sysUtilCheckCallback();tiny3d_Flip();
}

static void single_bar(char *caption) 
{
    progress_action = 0;

    msgDialogOpen2(mdialogprogress, caption, progress_callback, (void *) 0xadef0044, NULL);
    
    msgDialogProgressBarReset(MSG_PROGRESSBAR_INDEX0);

    sysUtilCheckCallback();tiny3d_Flip(); 
}

static int Files_To_Copy=0;
static float progress_0 = 0.0f;

static void double_bar(char *caption) 
{
    progress_action = 0;

    msgDialogOpen2(mdialogprogress2, caption, progress_callback, (void *) 0xadef0042, NULL);
    
    msgDialogProgressBarReset(MSG_PROGRESSBAR_INDEX0);
    msgDialogProgressBarReset(MSG_PROGRESSBAR_INDEX1);
   
    progress_0 = 0.0f;
    sysUtilCheckCallback();tiny3d_Flip(); 
}

void DrawBox2(float x, float y, float z, float w, float h)
{
    tiny3d_SetPolygon(TINY3D_QUADS);
       
    tiny3d_VertexPos(x    , y    , z);
    tiny3d_VertexColor(0x0040ff80);

    tiny3d_VertexPos(x + w, y    , z);
    tiny3d_VertexColor(0x0080ff80);

    tiny3d_VertexPos(x + w, y + h, z);
    tiny3d_VertexColor(0x0060ff80);

    tiny3d_VertexPos(x    , y + h, z);
    tiny3d_VertexColor(0x0070ff80);

    tiny3d_End();
}

sysFSDirent entries1[2048];
sysFSDirent entries2[2048];

int nentries1, nentries2;

static int entry_compare(const void *va, const void *vb)
{
    sysFSDirent * a =  (sysFSDirent *) va;
    sysFSDirent * b =  (sysFSDirent *) vb;

    if((a->d_type & 1)>(b->d_type & 1) || ((a->d_type & 1)==(b->d_type & 1) && strcmp(a->d_name, b->d_name) <0) 
        || !strcmp(a->d_name,"..")) return -1;
    else return 1;
}

static int test_mark_flags(sysFSDirent *ent, int nent, int *nmarked)
{
    int n;
    int ret = 0;
    *nmarked = 0;
    for(n = 0; n < nent; n++)
        if(ent[n].d_type & 2) {ret=1;(*nmarked) ++;}
    return ret;
}

static int reset_copy = 1;

static char *cpy_str="Copy";

static char *dyn_get_name(char *p)
{
    int n= strlen(p); while(n>0 && p[n]!='/') n--;
    return &p[n+1];
}

static int CountFiles(char* path, int *nfiles, u64 *size)
{
	int dfd;
	u64 read;
	sysFSDirent dir;
    int ret = 0;
    int p1=strlen(path);

    {
        sysFSStat stat;
        ret= sysLv2FsStat(path, &stat);
        if (ret<0)
		    return ret;
        (*size)+= stat.st_size;

    }


    ret= sysLv2FsOpenDir(path, &dfd);
	if (ret)
		return ret;
    
    read = sizeof(sysFSDirent);
	while (!sysLv2FsReadDir(dfd, &dir, &read)) {
		
		if (!read)
			break;
		if (!strcmp(dir.d_name, ".") || !strcmp(dir.d_name, ".."))
			continue;

        path[p1]= 0;
        
		strcat(path, "/");
		strcat(path, dir.d_name);
  

		if (dir.d_type & DT_DIR) {
 
			ret= CountFiles(path, nfiles, size);
            if(ret) goto skip;

		} else {
            sysFSStat stat;
            ret= sysLv2FsStat(path, &stat);
            if (ret<0)
                goto skip;
            (*size)+= stat.st_size;
            (*nfiles) ++;
    
		}

        

	}

skip:

    path[p1]= 0;
	sysLv2FsCloseDir(dfd);

    return ret;

}

static char * get_extension(char *path)
{
    int n = strlen(path);
    int m = n;
    
    while(m>1 && path[m]!='.' && path[m]!='/') m--;

    if(path[m]=='.') return &path[m];

    return &path[n];
}

extern u64 lv2peek(u64 addr);
extern u64 lv2poke(u64 addr, u64 value);

static int level_dump(char *path, int mode)
{
    int ret = 0;
    int n;
    s32 fd = -1;

    time_t timer;
    struct tm * timed;

    float parts;
    float cpart;

    time(&timer);
    timed = localtime(&timer);

    if(firmware < 0x421C && mode==1) return (int) 0x80010009;

    u64 *mem = NULL;
    
    if(mode==1) { // LV1

        mem= (u64 *) malloc(0x1000000);
        if(!mem) return (int) 0x80010004;

        memset((void *) mem, 0, 0x1000000);

        lv1_reg regs_i, regs_o;
        
        memset(&regs_i, 0, sizeof(regs_i));
/*
        regs_i.reg11 = 0xB6; 
        sys8_lv1_syscall(&regs_i, &regs_o);

        if(((int) regs_o.reg3) <0) {
            return  (int) 0x80010004;
        }
*/
        single_bar("LV1 Dump process");

        for(n = 0; n < 0x1000000/8; n++) {

            regs_i.reg11 = 0xB6; regs_i.reg3 = (u64) (n<<3);
            sys8_lv1_syscall(&regs_i, &regs_o);
            mem[n]= regs_o.reg4;
        }

        sprintf(temp_buffer, "%s/LV1-%XEX-%04i%02i%02i-%02i%02i%02i.bin", path, firmware,
            timed->tm_year+1900, timed->tm_mon+1,  timed->tm_mday, timed->tm_hour, timed->tm_min, timed->tm_sec);

        ret = sysLv2FsOpen(temp_buffer, SYS_O_WRONLY | SYS_O_CREAT | SYS_O_TRUNC, &fd, 0777, NULL, 0);
        if(ret < 0) goto skip;
        sysLv2FsChmod(temp_buffer, FS_S_IFMT | 0777);

    } else { // LV2
       
        mem= (u64 *) malloc(0x800000);
        if(!mem) return (int) 0x80010004;

        memset((void *) mem, 0, 0x800000);

        single_bar("LV2 Dump process");

        for(n = 0; n < 0x800000/8; n++) mem[n]= lv2peek(0x8000000000000000ULL + (u64)(n<<3));

        sprintf(temp_buffer, "%s/LV2-%XEX-%04i%02i%02i-%02i%02i%02i.bin", path, firmware,
            timed->tm_year+1900, timed->tm_mon+1,  timed->tm_mday, timed->tm_hour, timed->tm_min, timed->tm_sec);

        ret = sysLv2FsOpen(temp_buffer, SYS_O_WRONLY | SYS_O_CREAT | SYS_O_TRUNC, &fd, 0777, NULL, 0);
        if(ret < 0) goto skip;
        sysLv2FsChmod(temp_buffer, FS_S_IFMT | 0777);

     
    }

    u64 pos = 0ULL;
    u64 readed = 0, writed = 0;
    u64 lenght = 0x800000ULL;

    if(mode==1) lenght = 0x1000000ULL;

    parts = (lenght == 0) ? 0.0f : 100.0f / ((double) lenght / (double) 0x100000);
    cpart = 0;

    while(pos < lenght) {
        readed = lenght - pos; if(readed > 0x100000ULL) readed = 0x100000ULL;

        ret=sysLv2FsWrite(fd, &mem[pos>>3], readed, &writed);
        if(ret<0) goto skip;
        if(readed != writed) {ret = 0x8001000C; goto skip;}
        
        pos += readed;

        if(progress_action == 2) {ret = 0x8001000C; goto skip;}

        cpart += parts;
        if(cpart >= 1.0f) {
            update_bar((u32) cpart);
            cpart-= (float) ((u32) cpart); 
        }

    }

skip:
    if(fd>=0) sysLv2FsClose(fd);
    if(ret>0) ret = 0;
    if(mem) free(mem);

    if(progress_action == 2) unlink_secure(temp_buffer);

    msgDialogAbort();
    return ret;
}

static  float copy_parts;
static float copy_cpart;

static int CopyFd(s32 fd, s32 fd2, char *mem, u64 lenght)
{
    int ret = 0;
    int one = 0;
    u64 pos = 0ULL;
    u64 readed = 0, writed = 0;

    while(pos < lenght) {

        readed = lenght - pos; if(readed > 0x100000ULL) readed = 0x100000ULL;
        ret=sysLv2FsRead(fd, mem, readed, &writed);
        if(ret<0) goto skip;
        if(readed != writed) {ret = 0x8001000C; goto skip;}

        ret=sysLv2FsWrite(fd2, mem, readed, &writed);
        if(ret<0) goto skip;
        if(readed != writed) {ret = 0x8001000C; goto skip;}
        
        pos += readed;

        if(progress_action == 2) {ret = 0x8001000C; goto skip;}

        copy_cpart += copy_parts;
        if(copy_cpart >= 1.0f) {
            one= 1;
            update_bar2((u32) copy_cpart);
            copy_cpart-= (float) ((u32) copy_cpart); 
        }

    }
    if(!one)
        update_bar2((u32) 100.0f);

skip:
    return ret;
}

static int CopyFile(char* path, char* path2)
{
    int ret = 0;
    s32 fd = -1;
    s32 fd2 = -1;
    u64 lenght = 0LL;
    
    char *mem = NULL;
    
    sysFSStat stat;

    
    if(Files_To_Copy == 0) Files_To_Copy = 1;

    progress_0 += 100.0f/(float) Files_To_Copy;
    if(progress_0 >= 1.0f) {
        msgDialogProgressBarInc(MSG_PROGRESSBAR_INDEX0, (u32) progress_0);
        progress_0-= (float) ((u32) (100.0f/(float) Files_To_Copy)); 
    }
   
    msgDialogProgressBarSetMsg(MSG_PROGRESSBAR_INDEX0, "Progress");
    msgDialogProgressBarReset(MSG_PROGRESSBAR_INDEX1);
    msgDialogProgressBarSetMsg(MSG_PROGRESSBAR_INDEX1, dyn_get_name(path));

    if(reset_copy) {

        reset_copy = 0;
    
        for(ret=0;ret < 20;ret++) {
            sysUtilCheckCallback();tiny3d_Flip();     
        }
    }
     
    char *ext =get_extension(path);

    if(!strncmp(ext, ".666", 4)) {// split files
        if(strcmp(ext, ".66600")) goto skip;

        int n;

        fd = fd2 = -1;

        mem = malloc(0x100000);
        if(!mem) {ret= (int) 0x80010004; goto skip2;}

        char *ext2 =get_extension(path2);

        if(!strncmp(ext2, ".666", 4)) ext2[0]=0;

        for(n = 0; n < 99; n++) {

            fd = -1; ext[4]= 48 + n/10; ext[5]= 48 + (n % 10);

            msgDialogProgressBarReset(MSG_PROGRESSBAR_INDEX1);
            msgDialogProgressBarSetMsg(MSG_PROGRESSBAR_INDEX1, dyn_get_name(path2));
            
            ret= sysLv2FsStat(path, &stat);
            if(ret < 0 || stat.st_size==0) {ret = 0;goto skip2;}

            lenght = stat.st_size;

            ret = sysLv2FsOpen(path, 0, &fd, S_IRWXU | S_IRWXG | S_IRWXO, NULL, 0);
            if(ret) goto skip2;

            if(n == 0) {
                ret = sysLv2FsOpen(path2, SYS_O_WRONLY | SYS_O_CREAT | SYS_O_TRUNC, &fd2, 0777, NULL, 0);
                if(ret) goto skip2;
                sysLv2FsChmod(path2, FS_S_IFMT | 0777);
            }

            copy_parts = (lenght == 0) ? 0.0f : 100.0f / ((double) lenght / (double) 0x100000);
            copy_cpart = 0;

            ret = CopyFd( fd, fd2, mem, lenght);
            if(ret < 0) goto skip2;

            sysLv2FsClose(fd); fd = -1;
        
        }
    
    
    } else {

        ret= sysLv2FsStat(path, &stat);
        if(ret) goto skip;

        lenght = stat.st_size;

        if(lenght >= 0x100000000LL && strncmp(path2, "/dev_hdd0", 9)) { // split the file
            ret = sysLv2FsOpen(path, 0, &fd, S_IRWXU | S_IRWXG | S_IRWXO, NULL, 0);
            if(ret) goto skip2;

            mem = malloc(0x100000);
            if(!mem) {ret= (int) 0x80010004; goto skip2;}

            u64 pos = 0;
            int n = 0;

            copy_parts = (lenght == 0) ? 0.0f : 100.0f / ((double) lenght / (double) 0x100000);
            copy_cpart = 0;
            
            char *ext2 =&path2[strlen(path2)];

            while(pos < stat.st_size) {
                
                ext2[0]=0;
                sprintf(ext2,".666%2.2i", n);

                msgDialogProgressBarReset(MSG_PROGRESSBAR_INDEX1);
                msgDialogProgressBarSetMsg(MSG_PROGRESSBAR_INDEX1, dyn_get_name(path2));

                lenght = (stat.st_size - pos);
                if(lenght > 0x40000000LL) lenght = 0x40000000LL;
                
                ret = sysLv2FsOpen(path2, SYS_O_WRONLY | SYS_O_CREAT | SYS_O_TRUNC, &fd2, 0777, NULL, 0);
                if(ret) goto skip2;
                sysLv2FsChmod(path2, FS_S_IFMT | 0777);

                ret = CopyFd( fd, fd2, mem, lenght);
                if(ret < 0) goto skip2;

                sysLv2FsClose(fd2); fd2 = -1;

                pos+= lenght;
                
                n++;
            
            }
        
        } else {

            ret = sysLv2FsOpen(path, 0, &fd, S_IRWXU | S_IRWXG | S_IRWXO, NULL, 0);
            if(ret) goto skip;


            ret = sysLv2FsOpen(path2, SYS_O_WRONLY | SYS_O_CREAT | SYS_O_TRUNC, &fd2, 0777, NULL, 0);
            if(ret) {sysLv2FsClose(fd);goto skip;}
            sysLv2FsChmod(path2, FS_S_IFMT | 0777);


            mem = malloc(0x100000);
            if(!mem) {ret= (int) 0x80010004; goto skip2;}

            
            copy_parts = (lenght == 0) ? 0.0f : 100.0f / ((double) lenght / (double) 0x100000);
            copy_cpart = 0;

            ret = CopyFd( fd, fd2, mem, lenght);
        }
    }

    
skip2:
    
    if(mem) free(mem);
    if(fd>=0) sysLv2FsClose(fd);
    if(fd2>=0) sysLv2FsClose(fd2);
    if(ret>0) ret = 0;

    if(progress_action == 2) unlink_secure(path2);

skip:
    //msgDialogAbort();
    return ret;
}

static int CopyDirectory(char* path, char* path2)
{
	int dfd;
	u64 read;
	sysFSDirent dir;
    int ret = 0;
    int p1=strlen(path);
    int p2=strlen(path2);

    ret= sysLv2FsOpenDir(path, &dfd);
	if (ret)
		return ret;

    sysLv2FsMkdir(path2, 0777);
    
    read = sizeof(sysFSDirent);
	while (!sysLv2FsReadDir(dfd, &dir, &read)) {
		
		if (!read)
			break;
		if (!strcmp(dir.d_name, ".") || !strcmp(dir.d_name, ".."))
			continue;

        path[p1]= 0;
        path2[p2]= 0;
		strcat(path, "/");
		strcat(path, dir.d_name);
        strcat(path2, "/");
		strcat(path2, dir.d_name);

		if (dir.d_type & DT_DIR) {
 
			ret= CopyDirectory(path, path2);
            if(ret) goto skip;

		} else {
            
            ret = CopyFile(path, path2);
            if(ret<0) goto skip;
    
		}

        

	}

skip:

    path[p1]= 0;
    path2[p2]= 0;
	sysLv2FsCloseDir(dfd);

    return ret;

}

static int copy_archive_manager(char *path1, char *path2, sysFSDirent *ent, int nent, int sel, u64 free)
{
    int ret = 0;
    int n;

    u64 size = 0;

    reset_copy = 1;
    cpy_str= "Copy";
    Files_To_Copy = 0;

    if(sel>=0) {
        sprintf(temp_buffer, "Copy %s\n\nfrom %s\n\nto %s?", ent[sel].d_name, path1, path2);
        if(DrawDialogYesNo(temp_buffer) == 1) {
            double_bar(cpy_str);
            
            sprintf(temp_buffer, "%s/%s", path1, ent[sel].d_name);
            sprintf(temp_buffer + 2048, "%s/%s", path2, ent[sel].d_name);

            if(ent[sel].d_type & 1) {
                
                ret=CountFiles(temp_buffer, &Files_To_Copy, &size);
                if(ret<0) goto end;
                if(size > free) goto end;
                sprintf(temp_buffer, "%s/%s", path1, ent[sel].d_name);
                ret = CopyDirectory(temp_buffer, temp_buffer + 2048);
            }
            else {
                sysFSStat stat;
                ret= sysLv2FsStat(temp_buffer, &stat);
                size+= stat.st_size;
                if(ret<0) goto end;
                if(size > free) goto end;

                Files_To_Copy = 1;

                ret = CopyFile(temp_buffer, temp_buffer + 2048);
            }
            
            msgDialogAbort();
            usleep(100000);
        }
    } else { // multiple

        sprintf(temp_buffer, "Copy selected Files and Folders\n\nfrom %s\n\nto %s?", path1, path2);
        if(DrawDialogYesNo(temp_buffer) == 1) {
            double_bar(cpy_str);

            for(n = 0; n < nent; n++) {
                if(!(ent[n].d_type & 2)) continue; // skip no marked

                sprintf(temp_buffer, "%s/%s", path1, ent[n].d_name);
                

                if(ent[n].d_type & 1) ret = CountFiles(temp_buffer, &Files_To_Copy, &size);
                else {
                    sysFSStat stat;
                    ret= sysLv2FsStat(temp_buffer, &stat);
                    size+= stat.st_size;

                    Files_To_Copy++;
                }

                if(ret<0) goto end;
                if(size > free) goto end;
                
            }

            if(ret==0)
            for(n = 0; n < nent; n++) {
                if(!(ent[n].d_type & 2)) continue; // skip no marked

                sprintf(temp_buffer, "%s/%s", path1, ent[n].d_name);
                sprintf(temp_buffer + 2048, "%s/%s", path2, ent[n].d_name);

                if(ent[n].d_type & 1) ret = CopyDirectory(temp_buffer, temp_buffer + 2048);
                else ret = CopyFile(temp_buffer, temp_buffer + 2048);
                if(ret <0) break;
            }
        }

        msgDialogAbort();
        usleep(100000);
    
    }

 end:   
    pad_last_time = 0;

    if(ret<0) return ret;
    if(size > free) {
        msgDialogAbort();
        usleep(100000);
        DrawDialogOK("No space to Copy Files/Folders");
    }

    return ret;
}

static int move_archive_manager(char *path1, char *path2, sysFSDirent *ent, int nent, int sel, u64 free)
{
    int ret = 0;
    int n;
    int flag = 0;
   
    u64 size = 0;

    n=1;while(path1[n]!='/' && path1[n]!=0) n++;

    if(!strncmp(path1, path2, n-1)) flag = 1; // can move

    cpy_str ="Move";
    Files_To_Copy = 0;

    reset_copy = 1;
    

    if(sel>=0) {
        sprintf(temp_buffer, "Move %s\n\nfrom %s\n\nto %s?", ent[sel].d_name, path1, path2);
        if(DrawDialogYesNo(temp_buffer) == 1) {
            double_bar(cpy_str);
            
            sprintf(temp_buffer, "%s/%s", path1, ent[sel].d_name);
            sprintf(temp_buffer + 2048, "%s/%s", path2, ent[sel].d_name);


            if(flag)
                ret= sysLv2FsRename(temp_buffer, temp_buffer  + 2048);
            else if(ent[sel].d_type & 1) {
                ret=CountFiles(temp_buffer, &Files_To_Copy, &size);
                if(ret<0) goto end;
                if(size > free) goto end;
                sprintf(temp_buffer, "%s/%s", path1, ent[sel].d_name);
                ret = CopyDirectory(temp_buffer, temp_buffer + 2048);
                if(ret==0) {
                    DeleteDirectory(temp_buffer);
                    ret = rmdir_secure(temp_buffer);
                }
            }
            else {
                sysFSStat stat;
                ret= sysLv2FsStat(temp_buffer, &stat);
                size+= stat.st_size;

                if(ret<0) goto end;
                if(size > free) goto end;
                Files_To_Copy = 1;
                ret = CopyFile(temp_buffer, temp_buffer + 2048);
                if(ret==0) ret = unlink_secure(temp_buffer); 
            }

            msgDialogAbort();
            usleep(100000);
        }
    } else { // multiple

        sprintf(temp_buffer, "Move selected Files and Folders\n\nfrom %s\n\nto %s?", path1, path2);
        if(DrawDialogYesNo(temp_buffer) == 1) {
            double_bar(cpy_str);

            if(!flag)
            for(n = 0; n < nent; n++) {
                if(!(ent[n].d_type & 2)) continue; // skip no marked

                sprintf(temp_buffer, "%s/%s", path1, ent[n].d_name);
                

                if(ent[n].d_type & 1) ret= CountFiles(temp_buffer, &Files_To_Copy, &size);
                else {
                    sysFSStat stat;
                    ret= sysLv2FsStat(temp_buffer, &stat);
                    size+= stat.st_size;
                  
                    Files_To_Copy++;
                }

                if(ret<0) goto end;
                if(size > free) goto end;
                
            }
          
            if(ret==0)
            for(n = 0; n < nent; n++) {
                if(!(ent[n].d_type & 2)) continue; // skip no marked

                sprintf(temp_buffer, "%s/%s", path1, ent[n].d_name);
                sprintf(temp_buffer + 2048, "%s/%s", path2, ent[n].d_name);

                if(flag)
                    ret= sysLv2FsRename(temp_buffer, temp_buffer  + 2048);
                else if(ent[n].d_type & 1) {
                    ret = CopyDirectory(temp_buffer, temp_buffer + 2048);
                    if(ret==0) {
                        DeleteDirectory(temp_buffer);
                        ret = rmdir_secure(temp_buffer);
                    }
                }
                else {
                    ret = CopyFile(temp_buffer, temp_buffer + 2048);
                    if(ret==0) ret = unlink_secure(temp_buffer); 
                }

                if(ret <0) break;
            }
            msgDialogAbort();
            usleep(100000);
        }
    
    }

 end:
    pad_last_time = 0;

    if(ret<0) return ret;
    if(size > free) {
        msgDialogAbort();
        usleep(100000);
        DrawDialogOK("No space to Move Files/Folders");
    }

    return ret;
}

#undef AUTO_BUTTON_REP2
#define AUTO_BUTTON_REP2(v, b) if(v && (old_pad & b)) { \
                                 v++; \
                                 if(v > 10) {v = 0; new_pad |= b;} \
                                 } else v = 0;



extern PngDatas Png_res[16];
extern u32 Png_res_offset[16];

static void display_icon(int x, int y, int z, int icon)
{
    tiny3d_SetTextureWrap(0, Png_res_offset[7 + icon], Png_res[7 + icon].width, 
        Png_res[7 + icon].height, Png_res[7 + icon].wpitch, 
        TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);

    DrawTextBox(x+2, y, z, 18, 18, 0xffffffff);
}


#define FD_LV1 -1
#define FD_LV2 -2

static u64 begin_lv1 = 0x0;
static u64 begin_lv2 = 0x0;
static u64 size_lv1 = 0x10000000ULL;
static u64 size_lv2 = 0x800000ULL;


int read_LV1(u64 pos, char *mem, int size) 
{
    int n = 0;
    u64 temp;

    lv1_reg regs_i, regs_o;

    if(begin_lv1 > pos) return (int) 0x8001002B;

    if(pos + ((u64) size) > size_lv1)  size -= (int) (pos + ((u64) size) - size_lv1);

    if(firmware < 0x421C) return (int) 0x80010009;

    temp = pos;

    if(temp & 7) {
        
        n= 8 - (pos & 7);
        if(n > size) n = size;
        temp&= ~7ULL;
        regs_i.reg3 = temp;
        regs_i.reg11 = 0xB6; 
        sys8_lv1_syscall(&regs_i, &regs_o);
        memcpy(&mem[0], ((char *) &regs_o.reg4) + (pos & 7), n);
        
        temp+=8;
    }
    if(n < size)
    for(; n< size; n+=8) {
        regs_i.reg3 = temp;
        regs_i.reg11 = 0xB6; 
        sys8_lv1_syscall(&regs_i, &regs_o);
        memcpy(&mem[n], ((char *) &regs_o.reg4), ((n+8) > size) ? (size - n) : 8);
        temp+=8;
        
    }

    return 0;
}

int read_LV2(u64 pos, char *mem, int size) 
{
    int n = 0;
    u64 temp;
    u64 temp2;

    if(begin_lv2 > pos) return (int) 0x8001002B;

    if(pos + ((u64) size) > size_lv2)  size -= (int) (pos + ((u64) size) - size_lv2);

    temp = pos;

    if(temp & 7) {
        
        n= 8 - (pos & 7);
        if(n > size) n = size;
        temp&= ~7ULL;
        temp2 = lv2peek(0x8000000000000000ULL + temp);

        memcpy(&mem[0], ((char *) &temp2) + (pos & 7), n);
        
        temp+=8;
    }
    if(n < size)
    for(; n< size; n+=8) {
        temp2 = lv2peek(0x8000000000000000ULL + temp);
        memcpy(&mem[n], ((char *) &temp2), ((n+8) > size) ? (size - n) : 8);
        temp+=8;
        
    }

    return 0;
}

int write_LV1(u64 pos, char *mem, int size) 
{
    int n = 0;
    u64 temp;

    lv1_reg regs_i, regs_o;

    if(begin_lv1 > pos) return (int) 0x8001002B;

    if(pos + ((u64) size) > size_lv1)  size -= (int) (pos + ((u64) size) - size_lv1);

    if(firmware < 0x421C) return (int) 0x80010009;

    temp = pos;

    if(temp & 7) {
        
        n= 8 - (pos & 7);
        if(n > size) n = size;
        temp&= ~7ULL;
        regs_i.reg3 = temp;
        regs_i.reg11 = 0xB6; 
        sys8_lv1_syscall(&regs_i, &regs_o);
        memcpy(((char *) &regs_o.reg4) + (pos & 7), &mem[0], n);

        regs_i.reg4 = regs_o.reg4;
        regs_i.reg11 = 0xB7; 
        sys8_lv1_syscall(&regs_i, &regs_o);
        
        temp+=8;
    }
    if(n < size)
    for(; n< size; n+=8) {
        regs_i.reg3 = temp;
        regs_i.reg11 = 0xB6; 
        sys8_lv1_syscall(&regs_i, &regs_o);
        memcpy(((char *) &regs_o.reg4), &mem[n], ((n+8) > size) ? (size - n) : 8);
        regs_i.reg4 = regs_o.reg4;
        regs_i.reg11 = 0xB7; 
        sys8_lv1_syscall(&regs_i, &regs_o);
        temp+=8;
        
    }

    return 0;
}

int write_LV2(u64 pos, char *mem, int size) 
{
    int n = 0;
    u64 temp;
    u64 temp2;

    temp = pos;

    if(begin_lv2 > pos) return (int) 0x8001002B;

    if(pos + ((u64) size) > size_lv2)  size -= (int) (pos + ((u64) size) - size_lv2);

    if(temp & 7) {
        
        n= 8 - (pos & 7);
        if(n > size) n = size;
        temp&= ~7ULL;

        temp2 = lv2peek(0x8000000000000000ULL + temp);
        memcpy(((char *) &temp2) + (pos & 7), &mem[0], n);
        lv2poke(0x8000000000000000ULL + temp, temp2);
        
        temp+=8;
    }
    if(n < size)
    for(; n< size; n+=8) {
        temp2 = lv2peek(0x8000000000000000ULL + temp);
        memcpy(((char *) &temp2), &mem[n], ((n+8) > size) ? (size - n) : 8);
        lv2poke(0x8000000000000000ULL + temp, temp2);
        temp+=8;
        
    }

    return 0;
}


static int load_hex(s32 fd, u64 pos, void *buffer, u64 readed)
{
    int ret;
    u64 temp = 0;

    if(fd == FD_LV1) {
          
        ret = read_LV1(pos, buffer, (int) readed);
        
        if(ret != 0) {
            sprintf(temp_buffer + 3072, "Read Error: 0x%08x\n\n%s", ret, getlv2error(ret));
            DrawDialogOK(temp_buffer + 3072);
        }

        return ret;
    } else
    if(fd == FD_LV2) {
       
        ret = read_LV2(pos, buffer, (int) readed);
        
        if(ret != 0) {
            sprintf(temp_buffer + 3072, "Read Error: 0x%08x\n\n%s", ret, getlv2error(ret));
            DrawDialogOK(temp_buffer + 3072);
        }
        
        return ret;
    }

    ret= sysLv2FsLSeek64(fd, pos, 0, &temp);

    if(ret < 0 || pos != temp) {
        if(ret == 0) ret = (int) 0x8001001E;
        sprintf(temp_buffer + 3072, "Lseek Error: 0x%08x\n\n%s", ret, getlv2error(ret));
        DrawDialogOK(temp_buffer + 3072);
        
    } else {

        temp = 0;
        ret = sysLv2FsRead(fd, buffer, readed, &temp);
        if(ret < 0 || readed != temp) {
            if(ret == 0) ret = (int) 0x8001002B;
            sprintf(temp_buffer + 3072, "Read Error: 0x%08x\n\n%s", ret, getlv2error(ret));
            DrawDialogOK(temp_buffer + 3072);
            
        }
    }

    return ret;
}

static int save_hex(s32 fd, u64 pos, void *buffer, u64 readed)
{
    int ret;
    u64 temp = 0;

    if(fd == FD_LV1) {
          
        ret = write_LV1(pos, buffer, (int) readed);
        
        if(ret != 0) {
            sprintf(temp_buffer + 3072, "Write Error: 0x%08x\n\n%s", ret, getlv2error(ret));
            DrawDialogOK(temp_buffer + 3072);
        }

        return ret;
    } else
    if(fd == FD_LV2) {
       
        ret = write_LV2(pos, buffer, (int) readed);
        
        if(ret != 0) {
            sprintf(temp_buffer + 3072, "Write Error: 0x%08x\n\n%s", ret, getlv2error(ret));
            DrawDialogOK(temp_buffer + 3072);
        }
        
        return ret;
    } 

    ret= sysLv2FsLSeek64(fd, pos, 0, &temp);

    if(ret < 0 || pos != temp) {
        if(ret == 0) ret = (int) 0x8001001E;
        sprintf(temp_buffer + 3072, "Lseek Error: 0x%08x\n\n%s", ret, getlv2error(ret));
        DrawDialogOK(temp_buffer + 3072);
        
    } else {

        temp = 0;
        ret = sysLv2FsWrite(fd, buffer, readed, &temp);
        if(ret < 0 || readed != temp) {
            if(ret == 0) ret = (int) 0x8001002B;
            sprintf(temp_buffer + 3072, "Write Error: 0x%08x\n\n%s", ret, getlv2error(ret));
            DrawDialogOK(temp_buffer + 3072);
            
        }
    }

    return ret;
}


int memcmp_case(char * p1, char *p2, int len)
{
int n;
char a, b;

    for(n = 0; n < len; n++) {
        a= *p1++; b= *p2++;
        if(a >= 'A' && a <= 'Z') a+= 32;
        if(b >= 'A' && b <= 'Z') b+= 32;
        if(a!=b) return 1;
    }

    return 0;

}
static int find_mode = 0; 

static int find_in_file(s32 fd, u64 pos, u64 size, u64 *finded, void * str, int len, int s)
{

    u64 temp;
    u64 readed = 0;
    u32 n;
    int ret = 0;
    int flag = 2;
    u64 pos0 = pos;

    *finded = -1LL;

    char *mem =  malloc(0x8208);
    if(!mem) return (int) 0x80010004;
    memset(mem, 0, 0x8200);

    single_bar("finding in file...");
    
    float parts;

    if(s < 0) parts = 100.0f / ((double) pos / (double) 0x8000); 
    else parts = 100.0f / ((double) (size-pos) / (double) 0x8000);

    float cpart = (s >= 0) ? parts * ((double) pos / (double) 0x8000) : 0;

    while((s >=0 && pos < size) || (s < 0 && pos >= 0 && flag)) {
        
        if(fd == FD_LV1 || fd == FD_LV2) {
            ret = 0;
            if(fd == FD_LV1) {if(begin_lv1 > pos) ret=(int) 0x8001001E; if(pos >= size_lv1) temp = 0; else  temp = pos;}
            if(fd == FD_LV2) {if(begin_lv2 > pos) ret=(int) 0x8001001E; if(pos >= size_lv2) temp = 0; else  temp = pos;}
   
        } else
            ret= sysLv2FsLSeek64(fd, pos, 0, &temp);

        if(ret < 0 || pos != temp) {
            if(ret == 0) ret = (int) 0x8001001E;
            msgDialogAbort();
            sprintf(temp_buffer + 3072, "Lseek Error: 0x%08x\n\n%s", ret, getlv2error(ret));
            DrawDialogOK(temp_buffer + 3072);
            goto skip;

        } 

        readed = size - pos; if(readed > 0x8200ULL) readed = 0x8200ULL;

        if(fd == FD_LV1) {
           
            ret = read_LV1(pos, mem, (int) readed);
            temp = readed;
        } else
        if(fd == FD_LV2) {
           
            ret = read_LV2(pos, mem, (int) readed);
            temp = readed;
        } else
        
            ret=sysLv2FsRead(fd, mem, readed, &temp);
        
        if(ret < 0 || readed != temp) {
            if(ret == 0) ret = 0x8001000C; 
            msgDialogAbort();
            sprintf(temp_buffer + 3072, "Read Error: 0x%08x\n\n%s", ret, getlv2error(ret));
            DrawDialogOK(temp_buffer + 3072);
            
            goto skip;
            }
        
        readed-= 0x200ULL * (readed == 0x8200ULL); // resta area solapada de busqueda

        for(n = 0; n < (u32) readed; n++) {
            if(find_mode == 2) {
                if(((pos + (u64) n) < pos0 || s>=0) && !memcmp_case(mem + n, str, len)) {
                    *finded = pos + (u64) n;
                    goto skip;

                }

            }
            else 
                if(((pos + (u64) n) < pos0 || s>=0) && !memcmp(mem + n, str, len)) {
                    *finded = pos + (u64) n;
                    goto skip;

                }
        }

        if(s >=0) {
            pos += readed;
        } else {
            if(pos < readed) {pos = 0ULL; flag--;} else  pos -= readed;
        }

        if(progress_action == 2) {ret = 0x8001000C; goto skip;}

        cpart += parts;
        if(cpart >= 1.0f) {
            update_bar((u32) cpart);
            cpart-= (float) ((u32) cpart); 
        }

    }

 


skip:
    if(mem) free(mem);
    msgDialogAbort();

    return ret;
}

static char help2[]= {
    "CROSS - Enter to the selected option\n"
    "UP/DOWN - Select option\n"
    "\n"
    "TRIANGLE - Exit\n"
    "SELECT - Open/Close this menu\n"
    "\n"
    "Special Note: WTF! You needs help up to piss XD\n"
};

static char help3[]= {
    "CROSS - Decrease the selected nibble\n"
    "CIRCLE - Increase the selected nibble\n"
    "L1/R1 - Decrease/increase the selected byte (pressing to auto-repeat)\n"
    "\n"
    "SQUARE - Undo the changes in selected byte\n"
    "START - Undo the current windows changes\n"
    "START (pressing) - Open/close this help window\n"
    "\n"
    "TRIANGLE - Hex Editor exit\n"
    "SELECT - Opens menu selector (go to, find..)\n"
    "\n"
    "UP/DOWN/LEFT/RIGHT - Move the cursor\n"
    "\n"
    "L3/R3 - Find back/forward\n"
    "L2/R2 - Move back/forward from the file\n"
    "\n"
    "Special Note: Changes in the window must be saved to use some actions with implicit changes"
    "in the editor window. Pressing SELECT you can save or discard the window changes.\n"
};

static char help4[]= {
    "CROSS - Set the hex number\n"
    "UP/DOWN/LEFT/RIGHT - Move the cursor to the keyboard\n"
    "L2/R2 - Move back/forward from the number window\n"
    "SQUARE - Delete the last digit\n"
    "\n"
    "CIRCLE - Go to the Address\n"
    "\n"
    "START (pressing) - Open/close this help window\n"
    "TRIANGLE - Hex Editor exit\n"
    "SELECT - Close the window\n"
    "\n"
    "Special Note: jump to the absolute file address"
};

static char help5[]= {
    "CROSS - Set the hex number\n"
    "UP/DOWN/LEFT/RIGHT - Move the cursor to the keyboard\n"
    "L2/R2 - Move back/forward from the number window\n"
    "\n"
    "CIRCLE - Find Hex values in file (forward)\n"
    "L1/R1 - Decrease/increase the number of bytes to find\n"
    "\n"
    "START (pressing) - Open/close this help window\n"
    "TRIANGLE - Hex Editor exit\n"
    "SELECT - Close the window\n"
    "\n"
    "Special Note: Find Hex values in the file. You can use L3/R3 from    "
    "the Hex Editor window to find the previous/next result from the     current file position\n"
};

static int hex_mode = 0;

static int mark_flag = 0;
static u64 mark_ini = 0ULL;
static u32 mark_len = 0x0;

static void *copy_mem = NULL;
static u32 copy_len = 0;

static u64 lv1_pos = 0ULL;
static u64 lv2_pos = 0ULL;

void hex_editor(char *path)
{
    int frame = 0;
    int n, m;

    int help = 0;

    s32 fd = -1;
    u64 pos =0;
    u64 readed = 0;
    u64 temp;

    int read_only = 0;

    int e_x = 0, e_y =0;

    int ret;

    int locked = 0;

    int enable_menu = 0;
    int function_menu = 0;
    int start_status = 0;

    int auto_up = 0, auto_down = 0;
    int auto_left = 0, auto_right = 0;
    int auto_l2 = 0, auto_r2 = 0;
    int auto_l1 = 0, auto_r1 = 0;
    int auto_l22 = 0, auto_r22 = 0;

    int f_key = 0;
    int f_pos = 0;
    int f_len = 8;
    static u8 find[512];
    int find_len = 4;

    mark_flag = 0;
    mark_ini = 0ULL;
    mark_len = 0x0;

    sysFSStat stat1;

    memset((char *) find, 0, 512);

    if(hex_mode == 0) {
    
        if(sysLv2FsStat(path, &stat1)<0) return;

        if(stat1.st_size == 0ULL) return; // ignore zero files

        ret = sysLv2FsOpen(path, SYS_O_RDWR, &fd, S_IRWXU | S_IRWXG | S_IRWXO, NULL, 0);
        if(ret < 0) {
            read_only = 1;
            ret = sysLv2FsOpen(path, 0, &fd, S_IRWXU | S_IRWXG | S_IRWXO, NULL, 0);
        }

        if(ret < 0) return;

    } else if(hex_mode == 1) {
        fd = FD_LV1; pos = lv1_pos;
        stat1.st_size = size_lv1 = 0x10000000ULL;
        begin_lv1 = 0;

    } else if(hex_mode == 2) {
        fd = FD_LV2; pos = lv2_pos;
        stat1.st_size = size_lv2 = 0x800000ULL;
        begin_lv2 = 0;
    }

read_file:

    memset(temp_buffer + 0x800, 0, 384);

    if(fd == FD_LV1 || fd == FD_LV2) {
        ret = 0;
        if(fd == FD_LV1) {if(begin_lv1 > pos) ret=(int) 0x8001001E; if(pos >= size_lv1) temp = 0; else  temp = pos;}
        if(fd == FD_LV2) {if(begin_lv2 > pos) ret=(int) 0x8001001E; if(pos >= size_lv2) temp = 0; else  temp = pos;}
 
    } else
        ret= sysLv2FsLSeek64(fd, pos, 0, &temp);

    if(ret < 0 || pos != temp) {
        if(ret == 0) ret = (int) 0x8001001E;
        sprintf(temp_buffer + 3072, "Lseek Error: 0x%08x\n\n%s", ret, getlv2error(ret));
        DrawDialogOK(temp_buffer + 3072);
        readed = 0;
    } else {

        readed = stat1.st_size - pos;
        if(readed > 384ULL) readed = 384ULL;
        temp = 0;

        if(fd == FD_LV1) {
           
            ret = read_LV1(pos, temp_buffer + 0x800, (int) readed);
            temp = readed;
        } else
        if(fd == FD_LV2) {
           
            ret = read_LV2(pos, temp_buffer + 0x800, (int) readed);
            temp = readed;
        } else
            ret = sysLv2FsRead(fd, temp_buffer + 0x800, readed, &temp);
        if(ret < 0 || readed != temp) {
            if(ret == 0) ret = (int) 0x8001002B;
            sprintf(temp_buffer + 3072, "Read Error: 0x%08x\n\n%s", ret, getlv2error(ret));
            DrawDialogOK(temp_buffer + 3072);
            readed = temp;
        }
    }

    memcpy(temp_buffer + 0xA00, temp_buffer + 0x800, 384); 


    while(1) {
    
    int px =0, py = 0;
    frame++;

    tiny3d_Flip();
    ps3pad_read();

    tiny3d_Project2D();
    cls2();
    update_twat();

    DrawBox(0, 0, 0, 848, 32,0x20a0a8ff);
    DrawBox2(0, 32, 0, 848, 448);

    DrawBox(0, 480, 0, 848, 32,0x20a0a8ff);

    
    SetFontColor(0xffffffff, 0x0);

    SetCurrentFont(FONT_BUTTON);

    SetFontSize(8, 16);

    py = 40;

    #define START_X 80

    px = START_X;  DrawString(px, py, "      Offset ");
    px = START_X + 16 + 17 *8 + 4;

    for(m = 0; m < 16; m++) {
        if(m==8) px+=8;
        DrawFormatString(px, py,  "%2X", m);
        px+= 24;
    }

    py+= 24;

    // 384
    for(n = 0; n < 24; n++) {
       px = START_X;
       u32 color = 0xffffffff;

       SetFontSize(8, 16);
       SetFontColor(color, 0x0);

       // draw hex
       px= DrawFormatString(px, py, " %08X", (u32) (((pos + (u64) (n<<4)))>>32));
       px= DrawFormatString(px, py, "%08X", (u32) (pos + (u64) (n<<4))); px+= 16;

       // draw hex
       for(m = 0; m < 16; m++) {
            int sel = 0;
            if(m==8) px+=8;
            px+=8;

            sel = mark_flag == 2 && (pos + (u64) ((n<<4) + m)) >= mark_ini && (pos + (u64) ((n<<4) + m)) < (mark_ini + (u64) mark_len);
           
            if(temp_buffer[0x800 + (n<<4) + m] == temp_buffer[0xA00 + (n<<4) + m]) color = 0xffffffff;
            else color = 0x8fff00ff;

           // first nibble
            if(((n<<4) + m) >= readed) color = 0x8f8f8fff;

            if((e_x) == (m<<1) && e_y == n) {
               if(frame & 16) SetFontColor(color, (sel) ? 0x400040ff : ((color == 0x8fff00ff) ? 0x40c0ffff : 0xC000C0FF)); 
               else SetFontColor(0x000000ff, color);
            } 
            else SetFontColor(color, (sel) ? 0x400040ff : ((color == 0x8fff00ff) ? 0x40c0ffff : 0x0));

            px= DrawFormatString(px, py, "%X", (temp_buffer[0x800 + (n<<4) + m])>>4);

           // second nibble
            if(((n<<4) + m) >= readed) color = 0x8f8f8fff;

            if((e_x) == (m<<1)+1 && e_y == n) {
               if(frame & 16) SetFontColor(color, (sel) ? 0x400040ff : ((color == 0x8fff00ff) ? 0x40c0ffff : 0xC000C0FF)); 
               else SetFontColor(0x000000ff, color);
            } 
            else SetFontColor(color, (sel) ? 0x400040ff : ((color == 0x8fff00ff) ? 0x40c0ffff : 0x0));

            px= DrawFormatString(px, py, "%X", temp_buffer[0x800 + (n<<4) + m] & 0xF);
       }

       px+=16;

       SetFontColor(0xffffffff, 0x0);

       // draw chars
       for(m = 0; m < 16; m++) {
            u8 ch = temp_buffer[0x800 + (n<<4) + m];
            int sel = 0;

            sel = mark_flag==2 && (pos + (u64) ((n<<4) + m)) >= mark_ini && (pos + (u64) ((n<<4) + m)) < (mark_ini + (u64) mark_len);

            if(temp_buffer[0x800 + (n<<4) + m] == temp_buffer[0xA00 + (n<<4) + m]) color = 0xffffffff;
            else color = 0x8fff00ff;

            if(((n<<4) + m) >= readed) color = 0x8f8f8fff;

            if((e_x>>1) == m && e_y == n) {if(frame & 16) SetFontColor(color, (sel) ? 0x400040ff : ((color == 0x8fff00ff) ? 0x40c0ffff : 0xC000C0FF));
                else SetFontColor(0x000000ff, color);
            }
            else SetFontColor(color, (sel) ? 0x400040ff : ((color == 0x8fff00ff) ? 0x40c0ffff : 0x0));


            px= DrawFormatString(px, py, "%c", ch==0 ? '.' : (ch < 32 ? '?' : (char) ch));
       }



       py+= 16;
    }

    py+= 2;
    SetFontColor(0xffffffff, 0x0);
    SetFontAutoCenter(1);

    SetFontSize(8, 16);
    DrawFormatString(px, py, "Size: %08X%08X", (u32) (stat1.st_size>>32), (u32) (stat1.st_size));
    DrawFormatString(px, py + 14, "File at %1.4f%%", 100.0f * (double) (pos + (u64) (e_y * 16 + (e_x>>1))) / (double) stat1.st_size);
    SetFontAutoCenter(0);


    set_ttf_window(8, 0, 752, 32, WIN_AUTO_LF);
    display_ttf_string(0, 0, (char *) path, 0x2000ffff, 0, 8, 16);


    if(function_menu==1 || function_menu==2) {
        
        px = 64;
        py = 64 + 16;

        SetFontSize(8, 16);
        DrawBox(px, py, 0, 4 * 40 + 4, 4 * 40 + 8 + 32, 0x8f8f8fff);
        
        SetFontColor(0x000000ff, 0x0);
        if(function_menu==1) DrawFormatString(px + 4, py + 4, "Go to Address");
        else DrawFormatString(px + 4, py + 4, "Find Hex Values");

        SetFontColor(0x000000ff, 0xffffffff);

        py+= 16;

        DrawFormatString(px + 20, py + 4, "                ");

        for(n=0; n< f_len; n++) {

            if(f_pos == n  && (frame & 16)) SetFontColor(0x000000ff, 0x00bfcfff);
                else SetFontColor(0x000000ff, 0xffffffff);

            if(function_menu==1) 
                DrawFormatString(px + 20 + (15 - f_len) * 8 + n * 8, py + 4, "%X", (n & 1) ? (temp_buffer[4096 + (n>>1)] & 15) : (temp_buffer[4096 + (n>>1)] >> 4));
            else
                DrawFormatString(px + 20 + n * 8, py + 4, "%X", (n & 1) ? (find[n>>1] & 15) : (find[n>>1] >> 4));
        }

        py+= 20;

        SetFontSize(32, 32);

        for(n = 0; n < 4; n++) {
            for(m = 0; m < 4; m++) {

                if((f_key & 3) == m && ((f_key>>2) & 3) == n && (frame & 16)) SetFontColor(0x000000ff, 0x00bfcfff);
                else SetFontColor(0x000000ff, 0xffffffff);
                
                DrawFormatString(px + 4 + m * 40, py + 4 + n * 40, "%X", ((n<<2) | m));
            }
        }

        SetFontColor(0xffffffff, 0x0);
        SetFontSize(8, 16);
    
    }

    if(enable_menu && function_menu==0) {
        int py = 0;
        
        DrawBox((848 - 224)/2, (512 - 248)/2, 0, 224, 248, 0x602060ff);
        DrawBox((848 - 216)/2, (512 - 240)/2, 0, 216, 240, 0x802080ff);
        set_ttf_window((848 - 200)/2, (512 - 240)/2, 200, 240, 0);

        display_ttf_string(0, py, "Go to Address", (enable_menu==1  && (frame & 16)) ? 0 : 0xffffffff, 0, 16, 24); py+= 24;

        display_ttf_string(0, py, "Find Hex", (enable_menu==2  && (frame & 16)) ? 0 : 0xffffffff, 0, 16, 24); py+= 24;

        display_ttf_string(0, py, "Find String", (enable_menu==3  && (frame & 16)) ? 0 : 0xffffffff, 0, 16, 24); py+= 24;

        display_ttf_string(0, py, "Find String (no case)", (enable_menu==4 && (frame & 16)) ? 0 : 0xffffffff, 0, 16, 24); py+= 24;

        display_ttf_string(0, py, "Find Hex from Datas", (enable_menu==5 && (frame & 16)) ? 0 : 0xffffffff, 0, 16, 24); py+= 24;

        display_ttf_string(0, py, "Mark Begin", (enable_menu==6 && (frame & 16)) ? 0 : 0xffffffff, 0, 16, 24); py+= 24;

        display_ttf_string(0, py, "Mark End", (enable_menu==7 && (frame & 16)) ? 0 : 0xffffffff, 0, 16, 24); py+= 24;

        display_ttf_string(0, py, "Copy Mark Area", (enable_menu==8 && (frame & 16)) ? 0 : 0xffffffff, 0, 16, 24); py+= 24;

        display_ttf_string(0, py, "Paste Copied Datas", (enable_menu==9 && (frame & 16)) ? 0 : 0xffffffff, 0, 16, 24); py+= 24;

        display_ttf_string(0, py, "Exit", (enable_menu==10 && (frame & 16)) ? 0 : 0xffffffff, 0, 16, 24); py+= 24;

    
    }

    if(read_only == 0 && memcmp(temp_buffer + 0xA00, temp_buffer + 0x800, 384)) locked = 1; else locked = 0;

    if(help) {
        
        DrawBox((848 - 624)/2, (512 - 424)/2, 0, 624, 424, 0x602060ff);
        DrawBox((848 - 616)/2, (512 - 416)/2, 0, 616, 416, 0x802080ff);
        set_ttf_window((848 - 600)/2, (512 - 416)/2, 600, 416, WIN_AUTO_LF);
        
        if(enable_menu && function_menu==0) display_ttf_string(0, 0, help2, 0xffffffff, 0, 16, 24);
        else if(function_menu == 1) display_ttf_string(0, 0, help4, 0xffffffff, 0, 16, 24);
        else if(function_menu == 2) display_ttf_string(0, 0, help5, 0xffffffff, 0, 16, 24);
        else display_ttf_string(0, 0, help3, 0xffffffff, 0, 16, 24);


    }


    if(help && (new_pad & BUTTON_START)) {
        help=0;
        start_status = 0;
    }

    if(start_status <= 60 && (old_pad & BUTTON_START)) {
        
        start_status++;
        if(start_status > 60) help=1;

    }

    if((new_pad & BUTTON_TRIANGLE) && help) {help^=1; start_status = 0; new_pad ^= BUTTON_TRIANGLE;}
    
    if(help) continue;


    if(new_pad & BUTTON_TRIANGLE) {

        if(locked && DrawDialogYesNo("Save the changes?") == 1) {
            save_hex(fd, pos, temp_buffer + 0x800, readed);
            memcpy(temp_buffer + 0xA00, temp_buffer + 0x800, 384);
            
        }

        if(DrawDialogYesNo("Exit from Hex Editor?") == 1) {
            if(hex_mode == 1) {
                lv1_pos = pos;
            } else if(hex_mode == 2) {
                lv2_pos = pos;
            }

            break;
            
        }

    }

    if(!enable_menu) {

    AUTO_BUTTON_REP2(auto_up, BUTTON_UP)
    AUTO_BUTTON_REP2(auto_down, BUTTON_DOWN)
    AUTO_BUTTON_REP2(auto_left, BUTTON_LEFT)
    AUTO_BUTTON_REP2(auto_right, BUTTON_RIGHT)
    AUTO_BUTTON_REP2(auto_l2, BUTTON_L2)
    AUTO_BUTTON_REP2(auto_r2, BUTTON_R2)
    AUTO_BUTTON_REP2(auto_l1, BUTTON_L1)
    AUTO_BUTTON_REP2(auto_r1, BUTTON_R1)

    if((new_pad & BUTTON_R3) && find_len) {

        if(locked && DrawDialogYesNo("Save the changes?") == 1) {
            save_hex(fd, pos, temp_buffer + 0x800, readed);
            memcpy(temp_buffer + 0xA00, temp_buffer + 0x800, 384);
            locked = 0;
        }
        if(!locked) {
            u64 finded;

            finded = pos + (u64) (e_y * 16 + (e_x>>1) + 1);
            if(finded >= stat1.st_size) finded = 0ULL;
            find_in_file(fd, finded, stat1.st_size, &finded, find, find_len, 1);
            
            function_menu = enable_menu = 0;

            if(finded == -1LL) {
                if(find_mode == 0) DrawDialogOKTimer("Hex String not found", 2000.0f);
                else DrawDialogOKTimer("String not found", 2000.0f);
            } else {
                pos = finded & ~15ULL;
                e_y = 0;
                e_x = (finded & 15) << 1;
                goto read_file;
            }
        }
        
    }
    
    if((new_pad & BUTTON_L3) && find_len) {

        if(locked && DrawDialogYesNo("Save the changes?") == 1) {
            save_hex(fd, pos, temp_buffer + 0x800, readed);
            memcpy(temp_buffer + 0xA00, temp_buffer + 0x800, 384);
            locked = 0;
        }
        if(!locked) {
            u64 finded;
           
            finded = pos + (u64) (e_y * 16 + (e_x>>1));
           
            find_in_file(fd, finded, stat1.st_size, &finded, find, find_len, -1);
            
            function_menu = enable_menu = 0;

            if(finded == -1LL) {
                if(find_mode == 0) DrawDialogOKTimer("Hex String not found", 2000.0f);
                else DrawDialogOKTimer("String not found", 2000.0f);
            } else {
                pos = finded & ~15ULL;
                e_y = 0;
                e_x = (finded & 15) << 1;
                goto read_file;
            }
        }
        
    }

    if(new_pad & BUTTON_UP) {
        auto_up = 1;
        e_y--; 
        if(e_y < 0) {
            e_y = 0;

            if(locked && DrawDialogYesNo("Save the changes?") == 1) {
                save_hex(fd, pos, temp_buffer + 0x800, readed);
                memcpy(temp_buffer + 0xA00, temp_buffer + 0x800, 384);
                locked = 0;
            }

            if(!locked) {
            if(pos >= 16ULL)  pos -= 16ULL; 
                else {
                    if(stat1.st_size>= 16ULL) 
                        pos = (stat1.st_size - 16ULL) & ~(15ULL);
                    else pos = 0;
                }
                goto read_file;
            }
        }

    }

    if(new_pad & BUTTON_DOWN) {
        auto_down = 1;
        e_y++; 
        if(e_y > 23) {
            e_y = 23;

            if(locked && DrawDialogYesNo("Save the changes?") == 1) {
                save_hex(fd, pos, temp_buffer + 0x800, readed);
                memcpy(temp_buffer + 0xA00, temp_buffer + 0x800, 384);
                locked = 0;
            }

            if(!locked) {
                if(pos + 16ULL < stat1.st_size)  pos += 16ULL;
                else pos = 0ULL;
                goto read_file;
            }
        }
    }

    if(new_pad & BUTTON_LEFT) {
        auto_left = 1;
        e_x--;
        if(e_x < 0) {
            e_x = 0x1f;
            e_y--; 
            if(e_y < 0) {
                e_y = 0;
                if(locked && DrawDialogYesNo("Save the changes?") == 1) {
                    save_hex(fd, pos, temp_buffer + 0x800, readed);
                    memcpy(temp_buffer + 0xA00, temp_buffer + 0x800, 384);
                    locked = 0;
                }

                if(!locked) {
                    if(pos >= 16ULL)  pos -= 16ULL;
                    else {
                        if(stat1.st_size>=16ULL) 
                            pos = (stat1.st_size - 16ULL) & ~(15ULL);
                        else pos = 0;
                    }
                    goto read_file;
                }
            }
        }

    }

    if(new_pad & BUTTON_RIGHT) {
        auto_right = 1;
        e_x++;
        if(e_x > 0x1f) {
            e_x = 0;
            e_y++; 
            if(e_y > 23) {
                e_y = 23;
                if(locked && DrawDialogYesNo("Save the changes?") == 1) {
                    save_hex(fd, pos, temp_buffer + 0x800, readed);
                    memcpy(temp_buffer + 0xA00, temp_buffer + 0x800, 384);
                    locked = 0;
                }
                if(!locked) {
                    if(pos + 16ULL < stat1.st_size)  pos += 16ULL;
                    else pos = 0ULL;
                    goto read_file;
                }
            }
        }
    }

    if((new_pad & BUTTON_L2)) {

        u64 incre = 0x80ULL;

        if(locked && DrawDialogYesNo("Save the changes?") == 1) {
            save_hex(fd, pos, temp_buffer + 0x800, readed);
            memcpy(temp_buffer + 0xA00, temp_buffer + 0x800, 384);
            locked = 0;
        }
       
        auto_l22++;
        incre<<= 4 * (auto_l22/10);
        if(incre > 0x6400000ULL) incre = 0x6400000ULL;

        auto_l2 = 1;
        if(pos >= incre)  pos -= incre;
        else {
                if(stat1.st_size>=16ULL) 
                    pos = (stat1.st_size - 16ULL) & ~(15ULL);
                else pos = 0;
            }
        goto read_file;

    } else if(!(old_pad & BUTTON_L2)) auto_l22 = 0;
    
    if((new_pad & BUTTON_R2)) {
        u64 incre = 128ULL;

        if(locked && DrawDialogYesNo("Save the changes?") == 1) {
            save_hex(fd, pos, temp_buffer + 0x800, readed);
            memcpy(temp_buffer + 0xA00, temp_buffer + 0x800, 384);
            locked = 0;
        }

        auto_r22++;
        incre <<= 4 * (auto_r22/10);

        if(incre > 0x6400000ULL) incre = 0x6400000ULL;

        auto_r2 = 1;
        if(pos + incre < stat1.st_size)  pos += incre;
        else pos = 0ULL;

        goto read_file;
    } else if(!(old_pad & BUTTON_R2)) auto_r22 = 0;

/**********************************************************************************************************/
/* MODIFICATION AREA                                                                                      */
/**********************************************************************************************************/

     // byte ++
    if((new_pad & BUTTON_R1) && read_only == 0 ) {
       
       u8 * p = (u8 *) &temp_buffer[0x800 + (e_y<<4) + (e_x>>1)];
        if(((e_y<<4) + (e_x>>1)) < readed) {
            p[0] = (p[0] + 1);
        }
        auto_r1 = 1;
    }

    // byte --
    if((new_pad & BUTTON_L1) && read_only == 0 ) {
        u8 * p = (u8 *) &temp_buffer[0x800 + (e_y<<4) + (e_x>>1)];
        if(((e_y<<4) + (e_x>>1)) < readed) {
            p[0] = (p[0] - 1);
        }

        auto_l1 = 1;
    }

    // nibble ++

    if((new_pad & BUTTON_CIRCLE) && read_only == 0 ) {
       
       u8 * p = (u8 *) &temp_buffer[0x800 + (e_y<<4) + (e_x>>1)];
        if(((e_y<<4) + (e_x>>1)) < readed) {
            if(e_x & 1)
                p[0] = ((p[0] + 1) & 0xf) | (p[0] & 0xf0);
            else
                p[0] = ((p[0] + 0x10) & 0xf0) | (p[0] & 0xf);

        }
    }

    // nibble --
    if((new_pad & BUTTON_CROSS) && read_only == 0 ) {
        u8 * p = (u8 *) &temp_buffer[0x800 + (e_y<<4) + (e_x>>1)];
        if(((e_y<<4) + (e_x>>1)) < readed) {
            if(e_x & 1)
                p[0] = ((p[0] - 1) & 0xf) | (p[0] & 0xf0);
            else
                p[0] = ((p[0] - 0x10) & 0xf0) | (p[0] & 0xf);
        }
    }

    // undo one
    if((new_pad & BUTTON_SQUARE) && read_only == 0 ) {
        if(((e_y<<4) + (e_x>>1)) < readed) {
            temp_buffer[0x800 + (e_y<<4) + (e_x>>1)] = temp_buffer[0xA00 + (e_y<<4) + (e_x>>1)];
        }
    }

    // undo all
    if((new_pad & BUTTON_START) && read_only == 0 ) {
        memcpy(temp_buffer + 0x800, temp_buffer + 0xA00, 384);
    }

    } // enable menu off
    else {// enable menu on

    if(function_menu==2) {

        if(new_pad & BUTTON_R1) {
           
            //auto_r1 = 1;
            memset((char *) &find[f_len>>1], 0, 8);
            f_len+=2;
            if(f_len > 16) f_len = 16;
            if(f_pos >= f_len) f_pos = (f_len) - 1;
        }

        if(new_pad & BUTTON_L1) {
            
            //auto_l1 = 1;
            memset((char *) &find[f_len>>1], 0, 8);
            f_len-=2;
            if(f_len < 2) f_len = 2;
            if(f_pos >= f_len) f_pos = f_len - 1;
        }
    }

    if(function_menu==1 || function_menu==2) {

        if(new_pad & BUTTON_R2) {
           
            //auto_r2 = 1;
            f_pos++;
            if(f_pos >= f_len) f_pos = (f_len) - 1;

        }

        if(new_pad & BUTTON_L2) {
            
            //auto_l2 = 1;
            f_pos--;
            if(f_pos < 0) f_pos = 0;
        }

        if(new_pad & BUTTON_UP) {
            //auto_up = 1;
            if((f_key & 12) == 0) f_key|=12; else f_key-=4;
        }

        if(new_pad & BUTTON_DOWN) {
            //auto_down = 1;
            if((f_key & 12) == 12) f_key=12; else f_key+=4;
        }

        if(new_pad & BUTTON_LEFT) {
            //auto_left = 1;
            if((f_key & 3) == 0) f_key|=3; else f_key--;
        }

        if(new_pad & BUTTON_RIGHT) {
            //auto_right = 1;
            if((f_key & 3) == 3) f_key^=3; else f_key++;
        }

        if(new_pad & BUTTON_CROSS) {
            u8 * p = (function_menu==1) ? (u8 *) &temp_buffer[4096 + (f_pos >> 1)] : &find[f_pos >> 1];
            if(f_pos & 1)
                    p[0] = (f_key) | (p[0] & 0xf0);
                else
                    p[0] = ((f_key<<4) & 0xf0) | (p[0] & 0xf);

            if(function_menu==1 && f_len < 16 && f_pos == (f_len - 1)) f_len++;
            f_pos++;
            if(f_pos >= f_len) f_pos = (f_len) - 1;
        }
    }

    if(function_menu==1 && (new_pad & BUTTON_SQUARE)){
        f_len--;
        if(f_len < 1) f_len = 1; 
        else {
            u8 * p = (u8 *) &temp_buffer[4096 + (f_len >> 1)];
            // clear the nibble
            if(f_len & 1)
                p[0] = (p[0] & 0xf0);
            else
                p[0] = (p[0] & 0xf);
        }
        if(f_pos >= f_len) f_pos = (f_len) - 1;
    }
    if((function_menu==1 || function_menu==2) && (new_pad & BUTTON_CIRCLE)) {
        switch(function_menu) {
            case 1:
                pos = 0ULL;
                memcpy(((char *) &pos), &temp_buffer[4096], (f_len+1)/2);
                
                //if(!(f_len & 1)) 
                pos>>=(u64) ((16 - f_len) <<2);

                if(pos >= stat1.st_size) pos = 0ULL;

                e_y = 0; e_x = 0;
                pos &= ~15ULL;
                function_menu = enable_menu = 0;
                goto read_file;
                break;
            case 2:
                {
                u64 finded;

                find_len = f_len/2;

                finded = pos + (u64) (e_y * 16 + (e_x>>1) + 1);
                if(finded >= stat1.st_size) finded = 0ULL;
                find_mode = 0;
                find_in_file(fd, finded, stat1.st_size, &finded, find, find_len, 1);
                
                function_menu = enable_menu = 0;

                if(finded == -1LL) {
                    DrawDialogOK("Hex string not found");
                } else {
                    pos = finded & ~15ULL;
                    e_y = 0;
                    e_x = (finded & 15) << 1;
                    goto read_file;
                }
                }
                break;

        }
    
    }

    if(function_menu == 0) {
        if(new_pad & BUTTON_UP) {

        
            if(enable_menu > 1) enable_menu--;  else {enable_menu = 10;} 
        }
    
        if(new_pad & BUTTON_DOWN) {
            if(enable_menu < 10) enable_menu++;  else {enable_menu = 1;}
        }

        if(new_pad & BUTTON_CROSS) {
            function_menu = enable_menu;
            switch(function_menu) {
                case 1:
                    memset(&temp_buffer[4096], 0, 8);
                    f_len = 1;
                    break;

                case 2:
                    f_len = 8;
                    if(find_len > 8) find_len = 8;
                    break;
                case 3:
                    memset(find, 0, 512); find_len = 0;
                    if(Get_OSK_String("Find String", (char *) find, 256)==0) {
                        u64 finded;

                        find_len = strlen((char *) find);

                        if(find_len != 0) {

                            finded = pos + (u64) (e_y * 16 + (e_x>>1) + 1);
                            if(finded >= stat1.st_size) finded = 0ULL;
                            find_mode = 1;
                            find_in_file(fd, finded, stat1.st_size, &finded, find, find_len, 1);
                            
                            function_menu = enable_menu = 0;

                            if(finded == -1LL) {
                                DrawDialogOK("String not found");
                            } else {
                                pos = finded & ~15ULL;
                                e_y = 0;
                                e_x = (finded & 15) << 1;
                                goto read_file;
                            }
                        }
                    }
                    function_menu = enable_menu = 0;

                    break;
                case 4:
                    memset(find, 0, 512); find_len = 0;
                    if(Get_OSK_String("Find String (no case sensitive)", (char *) find, 256)==0) {
                        u64 finded;

                        find_len = strlen((char *) find);

                        if(find_len != 0) {

                            finded = pos + (u64) (e_y * 16 + (e_x>>1) + 1);
                            if(finded >= stat1.st_size) finded = 0ULL;
                            find_mode = 2;
                            find_in_file(fd, finded, stat1.st_size, &finded, find, find_len, 1);
                            
                            function_menu = enable_menu = 0;

                            if(finded == -1LL) {
                                DrawDialogOK("String (no case sensitive) not found");
                            } else {
                                pos = finded & ~15ULL;
                                e_y = 0;
                                e_x = (finded & 15) << 1;
                                goto read_file;
                            }
                        }
                    }
                    function_menu = enable_menu = 0;

                    break;
                case 5:
                    if(copy_mem && copy_len) {
                        find_len = (copy_len > 512) ? 512 : copy_len;
                        memcpy(find, copy_mem, find_len); 
                    
                        u64 finded;

                        if(find_len != 0) {

                            finded = pos + (u64) (e_y * 16 + (e_x>>1) + 1);
                            if(finded >= stat1.st_size) finded = 0ULL;
                            find_mode = 0;
                            find_in_file(fd, finded, stat1.st_size, &finded, find, find_len, 1);
                            
                            function_menu = enable_menu = 0;

                            if(finded == -1LL) {
                                DrawDialogOK("Hex string not found");
                            } else {
                                pos = finded & ~15ULL;
                                e_y = 0;
                                e_x = (finded & 15) << 1;
                                goto read_file;
                            }
                        }
                    }
                    function_menu = enable_menu = 0;

                    break;

                case 6:
                    enable_menu = 0;
                    function_menu = 0;
                    if(pos + (u64) (e_y * 16 + (e_x>>1)) >= stat1.st_size) {
                        DrawDialogOKTimer("Mark is out of filesize / memory", 2000.0f);
                    } else {
                        mark_ini = pos + (u64) (e_y * 16 + (e_x>>1));
                        mark_flag = 1;
                    }
                    break;

                case 7:
                    enable_menu = 0;
                    function_menu = 0;
                    if(!mark_flag) mark_ini = 0;

                    if((pos + (u64) (e_y * 16 + (e_x>>1))) >= mark_ini 
                        && (pos + (u64) (e_y * 16 + (e_x>>1))) < (mark_ini + 0x100000ULL)) {

                    if(pos + (u64) (e_y * 16 + (e_x>>1)) >= stat1.st_size) {
                        DrawDialogOKTimer("Mark is out of filesize / memory: truncating to the end position", 2000.0f);
                        mark_len = stat1.st_size - mark_ini + 1ULL;
                    } else mark_len = (pos + (u64) (e_y * 16 + (e_x>>1))) - mark_ini + 1ULL;
                        mark_flag = 2;
                    } else DrawDialogOKTimer("Mark position out of the range\nyou can select a block of 1 MB max from Mark Begin", 2000.0f);
                    break;

                case 8:
                    enable_menu = 0;
                    function_menu = 0;
                    
                    if(mark_flag == 2) {
                        if(copy_mem) free(copy_mem);
                        copy_mem = malloc(mark_len);
                        copy_len = 0;
                        if(!copy_mem) DrawDialogOKTimer("Out of memory from copy function", 2000.0f);
                        else {copy_len = mark_len;

                            if(load_hex(fd, mark_ini, copy_mem, mark_len)==0) {
                                sprintf(temp_buffer + 3072, "Copied %d bytes", copy_len);
                                DrawDialogOKTimer(temp_buffer + 3072, 2000.0f);
                            }
                        }
                    } else DrawDialogOKTimer("Nothing to Copy", 2000.0f);


                    break;

                case 9:
                    enable_menu = 0;
                    function_menu = 0;

                    if(copy_len == 0 || !copy_mem) {DrawDialogOKTimer("Paste buffer is empty", 2000.0f);}
                    //else if(fd != FD_LV1 && fd != FD_LV2) DrawDialogOKTimer("Paste is Only supported to memory for now", 2000.0f);
                    else {
                        u64 my_pos = (pos + (u64) (e_y * 16 + (e_x>>1)));
                        int my_len = copy_len;

                        if((my_pos + (u64) my_len) > stat1.st_size) {
                             my_len = (u32) (stat1.st_size - my_pos);
                             sprintf(temp_buffer + 3072, "Paste buffer exceeds %d bytes the file / memory\n\nWant you write %d bytes from 0x%08X%08X ?", copy_len - my_len, my_len,
                                (u32) (my_pos>>32), (u32) my_pos);
                        
                        } else {

                            sprintf(temp_buffer + 3072, "Want you write %d bytes from 0x%08X%08X ?", my_len,
                                (u32) (my_pos>>32), (u32) my_pos);
                        }

                        if(DrawDialogYesNo(temp_buffer + 3072) == 1) {
                            int ret = 0;
                            
                            ret = save_hex(fd, my_pos, copy_mem, my_len);

                            if(ret == 0)
                                sprintf(temp_buffer + 3072, "Writed %d bytes to the current position", my_len);
                            
                            DrawDialogOKTimer(temp_buffer + 3072, 2000.0f);
                            goto read_file; 
                        }
                    }

                    break;

                case 10:
                    enable_menu = 0;
                    function_menu = 0;
                    
                    f_key = 0;
                    f_pos = 0;
                    f_len = (find_len > 8) ? 16 : find_len * 2;

                    break;
                default:
                    enable_menu = 0;
                    function_menu = 0;
                    break;
            }
        }
    } // func menu 0


    } // enable menu on

    if(new_pad & BUTTON_SELECT) {
        enable_menu =!enable_menu;
        function_menu = 0;
        
        f_key = 0;
        f_pos = 0;
        f_len = (find_len > 8) ? 16 : find_len * 2;

        if(enable_menu && locked && DrawDialogYesNo("Save the changes?") == 1) {
            save_hex(fd, pos, temp_buffer + 0x800, readed);
            memcpy(temp_buffer + 0xA00, temp_buffer + 0x800, 384);
            locked = 0;

        }
        
        // if locked undo all changes
        if(locked) {memcpy(temp_buffer + 0x800, temp_buffer + 0xA00, 384);locked=0;}
    }
    


    }

    if(fd >=0) sysLv2FsClose(fd); fd = -1;
}

static char help1[]= {
    "CROSS - Action for files/folders\n"
    "CIRCLE - One to one group selection\n"
    "SQUARE - Select/Deselect all files/folders\n"
    "TRIANGLE - Exit\n"
    "SELECT - Opens menu selector (from the device)\n"
    "START - Open/close this help window\n"
    "UP/DOWN - Move the cursor\n"
    "L1/R1 - Changes the window source\n"
    "L3/R3 - Changes to different paths (Iris folder, other window folder,  list of devices)\n"
    "\n"
    "Special Combo: Press CIRCLE to select and CROSS to access to    the Hex Editor in .SELF or .PKG files\n"
};

void archive_manager()
{
    u32 frame = 0;

    int help = 0;

    int pos1 = 0;
    int pos2 = 0;

    int sel1 = 0;
    int sel2 = 0;

    nentries1=0;
    nentries2=0;

    int archive_manager = 0;

    int set_menu2 = 0;

    int change_path1=0, change_path2=0;

    char path1[0x420]="/";
    char path2[0x420]="/";

    u64 free_device1 = 0ULL;
    u64 free_device2 = 0ULL;

    sysFSStat stat1;
    sysFSStat stat2;

    int dev_rewrite = 0;

    if(sysLv2FsStat("/dev_rewrite", &stat1)==0) dev_rewrite = 1;


    int n;
    while(1) {

    frame++;


    s32 fd;

    stat1.st_mode = 0xffffffff;
    stat2.st_mode = 0xffffffff;

    if(nentries1 && path1[1]!=0 && strcmp(entries1[sel1].d_name, "..")!=0) {
        sprintf(temp_buffer, "%s/%s", path1, entries1[sel1].d_name);
        if(sysLv2FsStat(temp_buffer, &stat1)<0) stat1.st_mode = 0xffffffff;
    }

    if(nentries2 && path2[1]!=0 && strcmp(entries2[sel2].d_name, "..")!=0) {
        sprintf(temp_buffer, "%s/%s", path2, entries2[sel2].d_name);
        if(sysLv2FsStat(temp_buffer, &stat2)<0) stat2.st_mode = 0xffffffff;
    }

    if(stat1.st_mode == 0xffffffff) free_device1 = 0ULL;
    if(stat2.st_mode == 0xffffffff) free_device2 = 0ULL;

    if((!nentries1 || path1[1]==0) && sysLv2FsOpenDir(path1, &fd) == 0) {

        u64 read;

        int old_entries = nentries1;
        nentries1 = 0;
  
        while(sysLv2FsReadDir(fd, &entries1[nentries1], &read) == 0 && read > 0) {
            

            if(nentries1 >= 2048) break;
            if(entries1[nentries1].d_name[0]=='.' && entries1[nentries1].d_name[1]==0) continue;

            if(entries1[nentries1].d_type & DT_DIR) {
                entries1[nentries1].d_type = 1;
                if(path1[1]==0) {
                    sysFSStat stat;
                    sprintf(temp_buffer, "%s/%s", path1, entries1[nentries1].d_name);
                    if(sysLv2FsStat(temp_buffer, &stat)<0) entries1[nentries1].d_type |= 128;
                }
            }
            else
                entries1[nentries1].d_type = 0;


            nentries1++;
            

        }

        sysLv2FsCloseDir(fd);

        if(old_entries > nentries1) pos1 = sel1 = 0;

        qsort(entries1, nentries1, sizeof(sysFSDirent), entry_compare);
    }


    if((!nentries2 || path2[1]==0) && sysLv2FsOpenDir(path2, &fd) == 0) {

        u64 read;

        int old_entries = nentries2;
        nentries2 = 0;
  
        while(sysLv2FsReadDir(fd, &entries2[nentries2], &read) == 0 && read > 0) {
            

            if(nentries2 >= 2048) break;
            if(entries2[nentries2].d_name[0]=='.' && entries2[nentries2].d_name[1]==0) continue;

            if(entries2[nentries2].d_type & DT_DIR) {
                entries2[nentries2].d_type = 1;
                if(path2[1]==0) {
                    sysFSStat stat;
                    sprintf(temp_buffer, "%s/%s", path2, entries2[nentries2].d_name);
                    if(sysLv2FsStat(temp_buffer, &stat)<0) entries2[nentries2].d_type |= 128;
                }
            }
            else
                entries2[nentries2].d_type = 0;


            nentries2++;
            

        }

        sysLv2FsCloseDir(fd);

        if(old_entries > nentries2) pos2 = sel2 = 0;

        qsort(entries2, nentries2, sizeof(sysFSDirent), entry_compare);
    }


    tiny3d_Flip();
    ps3pad_read();

    tiny3d_Project2D();
    cls2();
    update_twat();
    
    if(nentries1 && path1[1]!=0) {
        u32 blockSize;
        u64 freeSize;
        int n;

        n=1;while(path1[n]!='/' && path1[n]!=0) n++;

        memcpy(temp_buffer, path1, n);
        temp_buffer[n]='/';
        temp_buffer[n + 1]=0;

        
        sysFsGetFreeSize(temp_buffer, &blockSize, &freeSize);
        free_device1 = ( ((u64)blockSize * freeSize));
         
    }

    if(nentries2 && path2[1]!=0) {
        u32 blockSize;
        u64 freeSize;
        int n;

        n=1;while(path2[n]!='/' && path2[n]!=0) n++;

        memcpy(temp_buffer, path2, n);
        temp_buffer[n]='/';
        temp_buffer[n + 1]=0;

        
        sysFsGetFreeSize(temp_buffer, &blockSize, &freeSize);
        free_device2 = ( ((u64)blockSize * freeSize));
         
    }

    DrawBox(0, 0, 0, 816, 32,0x20a0a8ff);
    DrawBox(816, 0, 0, 48, 32, (!archive_manager && (frame & 32)) ? 0xc0c000ff : 0x000000ff);
    set_ttf_window(8, 0, 752, 32, WIN_AUTO_LF);
    display_ttf_string(0, 0, (char *) path1, 0x2000ffff, 0, 8, 16);

    set_ttf_window(752, 0, 88, 32, WIN_AUTO_LF);
    if(free_device1 < 0x40000000LL)
        sprintf(temp_buffer, "FREE:\n%i MB", (int) (free_device1 / 0x100000LL));
    else
        sprintf(temp_buffer, "FREE:\n%1.2f GB", ((double) free_device1) / (1024.0 * 1024. * 1024.0));
    display_ttf_string(0, 0, (char *) temp_buffer, 0x000000ff, 0, 8, 16);


    set_ttf_window(816, 0, 36, 32, WIN_AUTO_LF);
    display_ttf_string(4, 0, (char *) "A", 0xff0000ff, 0, 32, 32);

    DrawBox2(0, 32, 0, 848, 256 - 32 /*, 0x2080c0ff*/);

    DrawBox(0, 256, 0, 816, 32, 0x20a0a8ff);
    DrawBox(816, 256, 0, 48, 32, (archive_manager && (frame & 32)) ? 0xc0c000ff : 0x000000ff);
    set_ttf_window(8, 256, 752, 32, WIN_AUTO_LF);
    display_ttf_string(0, 0, (char *) path2, 0x2000ffff, 0, 8, 16);

    set_ttf_window(752, 256, 88, 32, WIN_AUTO_LF);

    if(free_device2 < 0x40000000LL)
        sprintf(temp_buffer, "FREE:\n%i MB", (int) (free_device2 / 0x100000LL));
    else
        sprintf(temp_buffer, "FREE:\n%1.2f GB", ((double) free_device2) / (1024.0 * 1024. * 1024.0));
    display_ttf_string(0, 0, (char *) temp_buffer, 0x000000ff, 0, 8, 16);

    set_ttf_window(816, 256, 36, 32, WIN_AUTO_LF);
    display_ttf_string(4, 0, (char *) "B", 0xff0000ff, 0, 32, 32);

    DrawBox2(0, 32 + 256 , 0, 848, 256 - 32/*, 0x2080c0ff*/);
    
    set_ttf_window(24, 32, 848-24, 256 - 32, 0);

    if(nentries1) {
        int py = 0;

        if((sel1 >= pos1) && (frame & 16) && !archive_manager) {
                
            DrawBox(0, py + 32 + 24 * (sel1-pos1), 0, 848, 24, 0x40c04080);
        } else DrawBox(0, py + 32 + 24 * (sel1-pos1), 0, 848, 24, 0x0);

        for(n = 0; n < 9; n++) {
            if(pos1 + n >= nentries1) break;
            

            u32 color = 0xffffffff;

            if(entries1[pos1 + n].d_type & 1) {
                if(entries1[pos1 + n].d_type & 128) color = 0x8f8f00ff;
                else color = 0xffff00ff;
                display_icon(0, py + 34, 0, 0);
            } else {
                char *ext =get_extension(entries1[pos1 + n].d_name);
                int type = 1;

                if(!strcmp(ext, ".pkg") || !strcmp(ext, ".PKG")) type = 2; else
                if(!strcmp(ext, ".self") || !strcmp(ext, ".SELF")) type = 3;

                display_icon(0, py + 34, 0, type);

            }


            if(entries1[pos1 + n].d_type & 2)
                DrawBox(0, py + 36, 0, 848, 16, 0x800080a0);

            int dx= 0;
            if(sel1 == (pos1 + n) && stat1.st_mode != 0xffffffff) {
                
                if(stat1.st_size < 1024LL) {
                    sprintf(temp_buffer, "%i B", (int) stat1.st_size);
                    }
                else
                    if(stat1.st_size < 0x100000LL) {
                        sprintf(temp_buffer, "%i KB", (int) (stat1.st_size  / 1024LL));
                    } else if(stat1.st_size < 0x40000000LL){
                        sprintf(temp_buffer, "%i MB", (int) (stat1.st_size / 0x100000LL));
                    } else sprintf(temp_buffer, "%1.2f GB", ((double) stat1.st_size) / (1024.0 * 1024. * 1024.0));

                dx= display_ttf_string(0, py, (char *) temp_buffer, 0, 0, 8, 24);
            
            }

            set_ttf_window(24, 32, 848 - (dx + 24), 256 - 32, 0);

            display_ttf_string(0, py, (char *) entries1[pos1 + n].d_name, color, 0, 16, 24);

            if(sel1 == (pos1 + n) && stat1.st_mode != 0xffffffff) {

                set_ttf_window(848 - dx, 32, dx, 256 - 32, 0);
                display_ttf_string(0, py, (char *) temp_buffer, 0xffffffff, 0, 8, 24);
            }

            py+= 24;

        }
    }

    set_ttf_window(24, 256 + 32, 848-24, 256 - 32, 0);

    if(nentries2) {
        int py = 0;

        if((sel2 >= pos2) && (frame & 16) && archive_manager) {
                
            DrawBox(0, py + 32 + 256 + 24 * (sel2-pos2), 0, 848, 24, 0x40c04080);
        } else DrawBox(0, py + 32 + 256 + 24 * (sel2-pos2), 0, 848, 24, 0x0);

        for(n = 0; n < 9; n++) {
            if(pos2 + n >= nentries2) break;
            
            u32 color = 0xffffffff;

            if(entries2[pos2 + n].d_type & 1) {
                if(entries2[pos2 + n].d_type & 128) color = 0x8f8f00ff;
                else color = 0xffff00ff;

                display_icon(0, py  + 34 + 256 , 0, 0);

            } else {
                char *ext =get_extension(entries2[pos2 + n].d_name);
                int type = 1;

                if(!strcmp(ext, ".pkg") || !strcmp(ext, ".PKG")) type = 2; else
                if(!strcmp(ext, ".self") || !strcmp(ext, ".SELF")) type = 3;

                display_icon(0, py + 34 + 256, 0, type);

            }
             
            if(entries2[pos2 + n].d_type & 2)
                DrawBox(0, py + 36 + 256, 0, 848, 16, 0x800080a0);

            int dx= 0;

            if(sel2 == (pos2 + n) && stat2.st_mode != 0xffffffff) {
                
                if(stat2.st_size < 1024LL) {
                    sprintf(temp_buffer, "%i B", (int) stat2.st_size);
                    }
                else
                    if(stat2.st_size < 0x100000LL) {
                        sprintf(temp_buffer, "%i KB", (int) (stat2.st_size  / 1024LL));
                    } else if(stat2.st_size < 0x40000000LL){
                        sprintf(temp_buffer, "%i MB", (int) (stat2.st_size / 0x100000LL));
                    } else sprintf(temp_buffer, "%1.2f GB", ((double) stat2.st_size) / (1024.0 * 1024. * 1024.0));

                dx= display_ttf_string(0, py, (char *) temp_buffer, 0, 0, 8, 24);
            
            }

            set_ttf_window(24, 256 + 32, 848 - (dx + 24), 256 - 32, 0);

            display_ttf_string(0, py, (char *) entries2[pos2 + n].d_name, color, 0, 16, 24);

            if(sel2 == (pos2 + n) && stat2.st_mode != 0xffffffff) {

                set_ttf_window(848 - dx, 256 + 32, dx, 256 - 32, 0);
                display_ttf_string(0, py, (char *) temp_buffer, 0xffffffff, 0, 8, 24);
            }

            py+= 24;

        }
    } 

   
    if(set_menu2) {
        int py = 0;
        
        DrawBox((848 - 224)/2, (512 - 296)/2, 0, 224, 296, 0x602060ff);
        DrawBox((848 - 216)/2, (512 - 288)/2, 0, 216, 288, 0x802080ff);
        set_ttf_window((848 - 200)/2, (512 - 288)/2, 200, 288, 0);

        display_ttf_string(0, py, "New Folder", (set_menu2==1  && (frame & 16)) ? 0 : 0xffffffff, 0, 16, 24); py+= 24;

        display_ttf_string(0, py, "Rename", (set_menu2==2  && (frame & 16)) ? 0 : 0xffffffff, 0, 16, 24); py+= 24;

        display_ttf_string(0, py, "Copy", (set_menu2==3  && (frame & 16)) ? 0 : 0xffffffff, 0, 16, 24); py+= 24;

        display_ttf_string(0, py, "Move", (set_menu2==4  && (frame & 16)) ? 0 : 0xffffffff, 0, 16, 24); py+= 24;

        display_ttf_string(0, py, "Delete", (set_menu2==5  && (frame & 16)) ? 0 : 0xffffffff, 0, 16, 24); py+= 24;

        display_ttf_string(0, py, !dev_rewrite ? "Mount /dev_rewrite" : "Unmount /dev_rewrite", (set_menu2==6  && (frame & 16)) ? 0 : 0xffffffff, 0, 16, 24); py+= 24;

        display_ttf_string(0, py, "LV2 Dump", (set_menu2==7  && (frame & 16)) ? 0 : 0xffffffff, 0, 16, 24); py+= 24;

        display_ttf_string(0, py, "LV1 Dump", (set_menu2==8  && (frame & 16)) ? 0 : 0xffffffff, 0, 16, 24); py+= 24;

        display_ttf_string(0, py, "Ram Editor", (set_menu2==9  && (frame & 16)) ? 0 : 0xffffffff, 0, 16, 24); py+= 24;

        display_ttf_string(0, py, "LV2 Editor", (set_menu2==10  && (frame & 16)) ? 0 : 0xffffffff, 0, 16, 24); py+= 24;

        display_ttf_string(0, py, "Paste to New File", (set_menu2==11  && (frame & 16)) ? 0 : 0xffffffff, 0, 16, 24); py+= 24;

        display_ttf_string(0, py, "Exit", (set_menu2==12  && (frame & 16)) ? 0 : 0xffffffff, 0, 16, 24); py+= 24;

    
    }

    if(help) {
        
        DrawBox((848 - 624)/2, (512 - 424)/2, 0, 624, 424, 0x602060ff);
        DrawBox((848 - 616)/2, (512 - 416)/2, 0, 616, 416, 0x802080ff);
        set_ttf_window((848 - 600)/2, (512 - 416)/2, 600, 416, WIN_AUTO_LF);

        if(set_menu2) display_ttf_string(0, 0, help2, 0xffffffff, 0, 16, 24);
        else display_ttf_string(0, 0, help1, 0xffffffff, 0, 16, 24);


    }

    if(new_pad & BUTTON_START) {
        help^=1;
    }

    if((new_pad & BUTTON_TRIANGLE) && help) {help^=1; new_pad ^= BUTTON_TRIANGLE;}
    
    if(help) continue;


    if(new_pad & BUTTON_SELECT) {
        if(!archive_manager && nentries1 && path1[1]!=0) set_menu2=!set_menu2;
        if(archive_manager && nentries2 && path2[1]!=0) set_menu2=!set_menu2;
    }

    if((new_pad & BUTTON_TRIANGLE) && set_menu2) {set_menu2=0; new_pad ^= BUTTON_TRIANGLE;}


    if(set_menu2) {

        if(new_pad & BUTTON_UP) {

        
            if(set_menu2 > 1) set_menu2--;  else {set_menu2 = 12;} 
        }
    
        if(new_pad & BUTTON_DOWN) {
            if(set_menu2 < 12) set_menu2++;  else {set_menu2 = 1;}
        }


    if(new_pad & BUTTON_CROSS) {

        char buffer1[0x420];

        if(set_menu2==1) {// new folder

            sprintf(buffer1, "%s", "New"); 
            
            if(Get_OSK_String("New Folder", buffer1, 256)==0) {

                 if(buffer1[0] == 0) {set_menu2 = 0;goto skip_menu2;}
                
                 sprintf(temp_buffer, "Create new folder %s\nto %s ?", buffer1, !archive_manager ? path1 : path2);
                 if(DrawDialogYesNo(temp_buffer) == 1) {

                     if(!archive_manager) {
                        sprintf(temp_buffer, "%s/%s", path1, buffer1);
                     } else {
                        sprintf(temp_buffer, "%s/%s", path2, buffer1);
                     }

                     int ret= sysLv2FsMkdir(temp_buffer, 0777);
                     if(ret<0) {
                        sprintf(temp_buffer, "New folder error: 0x%08x\n\n%s", ret, getlv2error(ret));
                        DrawDialogOK(temp_buffer);
                     }

                     nentries2=nentries1=0;
                     pos1 = sel1 = 0;
                     pos2 = sel2 = 0;

                 }
                
            }

            set_menu2 = 0;
        }// new folder

        else if(set_menu2==2) {// rename
        
            if(!archive_manager)
                sprintf(buffer1, "%s", entries1[sel1].d_name);
            else
                sprintf(buffer1, "%s", entries2[sel2].d_name);

            if(!strcmp(buffer1, "..")) {set_menu2 = 0;goto skip_menu2;}
            
            if(Get_OSK_String("Rename", buffer1, 256)==0) {
                
                 sprintf(temp_buffer, "Rename %s\nto %s ?", !archive_manager ? entries1[sel1].d_name : entries2[sel2].d_name,
                     buffer1);
                 if(DrawDialogYesNo(temp_buffer) == 1) {

                     if(!archive_manager) {
                        sprintf(temp_buffer, "%s/%s", path1, entries1[sel1].d_name);
                        sprintf(temp_buffer + 2048, "%s/%s", path1, buffer1);
                     } else {
                        sprintf(temp_buffer, "%s/%s", path2, entries2[sel2].d_name);
                        sprintf(temp_buffer + 2048, "%s/%s", path2, buffer1);
                     }

                     int ret= sysLv2FsRename(temp_buffer, temp_buffer  + 2048);
                     if(ret<0) {
                        sprintf(temp_buffer, "Rename error: 0x%08x\n\n%s", ret, getlv2error(ret));
                        DrawDialogOK(temp_buffer);
                     }

                     nentries2=nentries1=0;
                     pos1 = sel1 = 0;
                     pos2 = sel2 = 0;

                 }
                
            }

            set_menu2 = 0;
        }// rename
        else if(set_menu2==5) {// delete
            
            sysFSDirent *entries;
            int nentries, sel;
            char *path;

            int files;
            int ret = 0;
            int cfiles = 0;

            if(!archive_manager) {
                entries = entries1;
                nentries = nentries1;
                sel = sel1;
                path = path1;

            } else {
                entries = entries2;
                nentries = nentries2;
                sel = sel2;
                path = path2;
            } 
                

            if(test_mark_flags(entries, nentries, &files)) {// multiple

                sprintf(temp_buffer, "Delete selected Files and Folders?\n\n(%i) Items", files);
                if(DrawDialogYesNo(temp_buffer) == 1) {
                              
                    single_bar("Deleting...");
                    
                    float parts = 100.0f / (float) files;
                    float cpart = 0;

                    for(n = 0; n < nentries; n++) {
                        if(!(entries[n].d_type & 2)) continue; // skip no marked
                        if(progress_action == 2) break;
   
                        cpart += parts;
                        if(cpart >= 1.0f) {
                            update_bar((u32) cpart);
                            cpart-= (float) ((u32) cpart); 
                        }
                        cfiles++;

                        sprintf(temp_buffer, "%s/%s", path, entries[n].d_name);

                        if(entries[n].d_type & 1) {
                            DeleteDirectory(temp_buffer);
                            ret = rmdir_secure(temp_buffer);
       
                        } else {
                            ret = unlink_secure(temp_buffer); 
                        }

                        if(ret<0) break;
          
                    }
                    sysUtilCheckCallback();tiny3d_Flip();
                    msgDialogAbort();
                    usleep(250000);
                   
                    if(ret<0) {
                        sprintf(temp_buffer, "Delete error: 0x%08x\n\n%s", ret, getlv2error(ret));
                        DrawDialogOK(temp_buffer);    
                    }
                    if(!archive_manager) {nentries1=0;pos1 = sel1 = 0;} else {nentries2=0;pos2 = sel2 = 0;}
                }
                
            } else {
                sprintf(temp_buffer, "Delete %s?", entries[sel].d_name);

                if(!strcmp(entries[sel].d_name, "..")) {set_menu2 = 0;goto skip_menu2;}

                if(DrawDialogYesNo(temp_buffer) == 1) {
                    sprintf(temp_buffer, "%s/%s", path, entries[sel].d_name);

                    if(entries[sel].d_type & 1) {
                        DeleteDirectory(temp_buffer);
                        ret = rmdir_secure(temp_buffer);

                    } else {
                        ret = unlink_secure(temp_buffer);
                    }


                    if(ret<0) {
                        sprintf(temp_buffer, "Delete error: 0x%08x\n\n%s", ret, getlv2error(ret));
                        DrawDialogOK(temp_buffer);
                    }

                    if(!archive_manager) {nentries1=0;pos1 = sel1 = 0;} else {nentries2=0;pos2 = sel2 = 0;}
                }
            }
            
            set_menu2 = 0;
        } // delete
        else if(set_menu2==3) {// copy
            
            int files;
            int ret = 0;
       
            if(!archive_manager) {
                if(test_mark_flags(entries1, nentries1, &files)) {// multiple

                    if(!strcmp(path2, "/") || !strcmp(path1, path2)) {set_menu2 = 0;goto skip_menu2;}
                    ret = copy_archive_manager(path1, path2, entries1, nentries1, -1, free_device2);
                } else {

                    if(!strcmp(entries1[sel1].d_name, "..") || !strcmp(path2, "/")  || !strcmp(path1, path2))
                        {set_menu2 = 0;goto skip_menu2;}

                    ret = copy_archive_manager(path1, path2, entries1, nentries1, sel1, free_device2);
                }
            } else {
                if(test_mark_flags(entries2, nentries2, &files)) {// multiple
                     if(!strcmp(path1, "/") || !strcmp(path1, path2)) {set_menu2 = 0;goto skip_menu2;}
                    ret = copy_archive_manager(path2, path1, entries2, nentries2, -1, free_device1);
                } else {

                    if(!strcmp(entries2[sel2].d_name, "..") || !strcmp(path1, "/")  || !strcmp(path1, path2))
                        {set_menu2 = 0;goto skip_menu2;}
                    ret = copy_archive_manager(path2, path1, entries2, nentries2, sel2, free_device1);
                }
            }
          
             if(ret<0) {
                sprintf(temp_buffer, "Copy error: 0x%08x\n\n%s", ret, getlv2error(ret));
                DrawDialogOK(temp_buffer);
             }

             nentries2=nentries1=0;
             pos1 = sel1 = 0;
             pos2 = sel2 = 0;

            set_menu2 = 0;
        } // copy
        else if(set_menu2==4) {// move
            
            int files;
            int ret = 0;
       
            if(!archive_manager) {
                if(test_mark_flags(entries1, nentries1, &files)) {// multiple

                    if(!strcmp(path2, "/") || !strcmp(path1, path2)) {set_menu2 = 0;goto skip_menu2;}
                    ret = move_archive_manager(path1, path2, entries1, nentries1, -1, free_device2);
                } else {

                    if(!strcmp(entries1[sel1].d_name, "..") || !strcmp(path2, "/")  || !strcmp(path1, path2))
                        {set_menu2 = 0;goto skip_menu2;}

                    ret = move_archive_manager(path1, path2, entries1, nentries1, sel1, free_device2);
                }
            } else {
                if(test_mark_flags(entries2, nentries2, &files)) {// multiple
                     if(!strcmp(path1, "/") || !strcmp(path1, path2)) {set_menu2 = 0;goto skip_menu2;}
                    ret = move_archive_manager(path2, path1, entries2, nentries2, -1, free_device1);
                } else {

                    if(!strcmp(entries2[sel2].d_name, "..") || !strcmp(path1, "/")  || !strcmp(path1, path2))
                        {set_menu2 = 0;goto skip_menu2;}
                    ret = move_archive_manager(path2, path1, entries2, nentries2, sel2, free_device1);
                }
            }
          
             if(ret<0) {
                sprintf(temp_buffer, "Move error: 0x%08x\n\n%s", ret, getlv2error(ret));
                DrawDialogOK(temp_buffer);
             }

             nentries2=nentries1=0;
             pos1 = sel1 = 0;
             pos2 = sel2 = 0;

             set_menu2 = 0;
        } // move
        else if(set_menu2==6) {// mount/umount /dev_rewrite
            
            if(dev_rewrite) {
                sys_fs_umount("/dev_rewrite");dev_rewrite = 0;
                if(!strncmp(path1, "/dev_rewrite", 12)) path1[1] = 0; // truncate to root
                if(!strncmp(path2, "/dev_rewrite", 12)) path2[1] = 0; // truncate to root
            }
            else {
                if(sys_fs_mount("CELL_FS_IOS:BUILTIN_FLSH1", "CELL_FS_FAT", "/dev_rewrite", 0)==0) 
                    dev_rewrite = 1;
                else 
                    dev_rewrite = 0;
            }

            nentries2=nentries1=0;
            pos1 = sel1 = 0;
            pos2 = sel2 = 0;

            set_menu2 = 0;

        
        } // mount/umount /dev_rewrite
        else if(set_menu2==7) {// lv2 dump
            
            int ret = 0;

            if(!archive_manager) { 
                if(free_device1 < 0x800400) ret= (int) 0x80010020;
                nentries1=0;
                pos1 = sel1 = 0;
            } else {
                if(free_device2 < 0x800400) ret= (int) 0x80010020; 
                nentries2=0;
                pos2 = sel2 = 0;
            }

            if(ret==0)     
                ret= level_dump(!archive_manager ? path1 : path2, 2);

            if(ret<0) {
                sprintf(temp_buffer, "Error in LV2 dump: 0x%08x\n\n%s", ret, getlv2error(ret));
                DrawDialogOK(temp_buffer);
             }

            set_menu2 = 0;

        
        } // lv2 dump
        else if(set_menu2==8) {// lv1 dump
            
            int ret = 0;

            if(!archive_manager) { 
                nentries1=0;
                pos1 = sel1 = 0;
                if(free_device1 < 0x1000400) ret= (int) 0x80010020; 
            } else {
                if(free_device2 < 0x1000400) ret= (int) 0x80010020; 
                nentries2=0;
                pos2 = sel2 = 0;
            }

            if(ret==0) 
                ret= level_dump(!archive_manager ? path1 : path2, 1);

            if(ret<0) {
                sprintf(temp_buffer, "Error in LV1 dump: 0x%08x\n\n%s", ret, getlv2error(ret));
                DrawDialogOK(temp_buffer);
             }

            set_menu2 = 0;

        
        } // lv1 dump
        else if(set_menu2==9) { // RAM area editor
            hex_mode = 1;
            hex_editor("RAM Area Editor");
            set_menu2 = 0;
        } // RAM area editor
        else if(set_menu2==10) { // LV2 editor
            hex_mode = 2;
            hex_editor("LV2 Editor");
            set_menu2 = 0;
        } // LV2 Editor
        else if(set_menu2==11) {// Paste to New File

            if(copy_len == 0 || !copy_mem) {DrawDialogOKTimer("Paste buffer is empty", 2000.0f);set_menu2 = 0;goto skip_menu2;}

            sprintf(buffer1, "%s", "Newfile"); 
            
            if(Get_OSK_String("Paste to New File", buffer1, 256)==0) {

                 if(buffer1[0] == 0) {DrawDialogOKTimer("Invalid filename", 2000.0f);set_menu2 = 0;goto skip_menu2;}
                
                 sprintf(temp_buffer, "Create new file %s.bin\nto %s ?", buffer1, !archive_manager ? path1 : path2);
                 if(DrawDialogYesNo(temp_buffer) == 1) {

                     if(!archive_manager) {
                        sprintf(temp_buffer, "%s/%s.bin", path1, buffer1);
                     } else {
                        sprintf(temp_buffer, "%s/%s.bin", path2, buffer1);
                     }
                     
                     s32 fd = -1;
                     int ret = sysLv2FsOpen(temp_buffer, SYS_O_WRONLY | SYS_O_CREAT | SYS_O_TRUNC, &fd, 0777, NULL, 0);

                     if(ret == 0 && fd>=0) {
                        sysLv2FsChmod(temp_buffer, FS_S_IFMT | 0777);
                        ret = save_hex(fd, 0LL, copy_mem, copy_len);
                     }

                     if(ret<0) {
                        sprintf(temp_buffer, "New file error: 0x%08x\n\n%s", ret, getlv2error(ret));
                        DrawDialogOK(temp_buffer);
                     } else {
                        sprintf(temp_buffer, "Writed %d bytes to the current position", copy_len);
                        DrawDialogOKTimer(temp_buffer, 2000.0f);
                     }

                     nentries2=nentries1=0;
                     pos1 = sel1 = 0;
                     pos2 = sel2 = 0;

                 }
                
            }

            set_menu2 = 0;
        }// Paste to New File
        else set_menu2 = 0;
    } 
    skip_menu2:
        ;

    } else {

    static int auto_up = 0, auto_down = 0;

    AUTO_BUTTON_REP2(auto_up, BUTTON_UP)
    AUTO_BUTTON_REP2(auto_down, BUTTON_DOWN)

    if(new_pad & BUTTON_UP) {

        if(!archive_manager) {
            auto_up = 1;if(sel1 > 0) sel1--;  else {sel1 = (nentries1 - 1); pos1 = sel1 - 8;} if(sel1 < pos1 + 4) pos1--; if(pos1 < 0) pos1 = 0;
        } else {
             auto_up = 1;if(sel2 > 0) sel2--;  else {sel2 = (nentries2 - 1); pos2 = sel2 - 8;} if(sel2 < pos2 + 4) pos2--; if(pos2 < 0) pos2 = 0;
        }
    }
    
    if(new_pad & BUTTON_DOWN) {
        if(!archive_manager) {
            auto_down = 1;if(sel1 < (nentries1-1)) sel1++; else {sel1 = 0; pos1 = 0;} if(sel1 > (pos1 + 4)) pos1++; if(pos1 > (nentries1 - 1)) {pos1 = 0;sel1 = 0;}
        } else {
            auto_down = 1;if(sel2 < (nentries2-1)) sel2++; else {sel2 = 0; pos2 = 0;} if(sel2 > (pos2 + 4)) pos2++; if(pos2 > (nentries2 - 1)) {pos2 = 0;sel2 = 0;}
        }
    }
    
    if(new_pad & (BUTTON_L1 | BUTTON_R1)) archive_manager^=1;


    if(new_pad & BUTTON_TRIANGLE) {
        if(DrawDialogYesNo("Exit from Archive Manager?") == 1) {
            break;
            
        }
    }

    if(!archive_manager) {
        if(new_pad & BUTTON_CROSS) {

            if(entries1[sel1].d_type & 1) { // change dir
                
                if(!strcmp(entries1[sel1].d_name,"..")) {
                    n = strlen(path1);
                    while(n>0 && path1[n]!='/') n--;

                    if(n==0) {path1[n] = '/';path1[n+1] = 0;} else path1[n] = 0;

                    if(sysLv2FsOpenDir(path1, &fd) == 0) {
                        sysLv2FsCloseDir(fd);
                    } else path1[1] = 0; // to root

                    nentries1 = 0;
                }
                else {
                    n = strlen(path1);
                    if(path1[n-1]!='/')
                        strcat(path1, "/");
                    strcat(path1, entries1[sel1].d_name);

                    if(sysLv2FsOpenDir(path1, &fd) == 0) {
                        nentries1 = 0;
                        sysLv2FsCloseDir(fd);
                    } else path1[n] = 0;
                    


                }
                pos1 = sel1 = 0;
                
            } else {
                char *ext =get_extension(entries1[sel1].d_name);

                if(!(entries1[sel1].d_type & 2) && (!strcmp(ext, ".pkg") || !strcmp(ext, ".PKG"))) {
            
                    install_pkg(path1, entries1[sel1].d_name);
                } else if(!(entries1[sel1].d_type & 2) && (!strcmp(ext, ".self") || !strcmp(ext, ".SELF"))) {

                    void fun_exit();

                    sprintf(temp_buffer, "%s/%s", path1, entries1[sel1].d_name);
                    fun_exit();
                    sysProcessExitSpawn2(temp_buffer, NULL, NULL, NULL, 0, 3071, SYS_PROCESS_SPAWN_STACK_SIZE_1M);
                    exit(0);
                } else {
                    sprintf(temp_buffer, "%s/%s", path1, entries1[sel1].d_name);
                    hex_mode = 0;
                    hex_editor(temp_buffer);
                }
            }
        } // cross

        if(new_pad & BUTTON_CIRCLE) { // select one file/folder

            if(path1[1]!=0 && strcmp(entries1[sel1].d_name,"..")) entries1[sel1].d_type^=2;
            
        } // circle

        if(new_pad & BUTTON_SQUARE) {

            u32 flag= (entries1[sel1].d_type ^ 2) & 2;

            if(path1[1]!=0) { // select all files/folders
                for(n = 0; n< nentries1; n++)
                    if(strcmp(entries1[n].d_name,"..")) entries1[n].d_type = (entries1[n].d_type & ~2) | flag;
            }
            
        } // square

        if(new_pad & BUTTON_L3) {
            change_path1--; if(change_path1<0) change_path1=2;
            nentries1=0;
            pos1 = sel1 = 0;
                   
            switch(change_path1) {
                case 0:
                    strcpy(path1, "/");
                    break;
                case 1:
                    strcpy(path1, self_path);
                    break;
                 case 2:
                    strcpy(path1, path2);
                    break;
            }
        } // l2
            
        if(new_pad & BUTTON_R3) {
            change_path1++; if(change_path1>2) change_path1=0;
            nentries1=0;
            pos1 = sel1 = 0;

            switch(change_path1) {
                case 0:
                    strcpy(path1, "/");
                    break;
                case 1:
                    strcpy(path1, self_path);
                    break;
                 case 2:
                    strcpy(path1, path2);
                    break;
            }
        } // r2
    } 
    
    else {// archive_manager 1

        if(new_pad & BUTTON_CROSS) {

            if(entries2[sel2].d_type & 1) { // change dir
                
                if(!strcmp(entries2[sel2].d_name,"..")) {
                    n = strlen(path2);
                    while(n>0 && path2[n]!='/') n--;

                    if(n==0) {path2[n] = '/';path2[n+1] = 0;} else path2[n] = 0;

                    if(sysLv2FsOpenDir(path2, &fd) == 0) {
                        sysLv2FsCloseDir(fd);
                    } else path2[1] = 0; // to root

                    nentries2 = 0;
                }
                else {
                    n = strlen(path2);
                    if(path2[n-1]!='/')
                        strcat(path2, "/");
                    strcat(path2, entries2[sel2].d_name);

                    if(sysLv2FsOpenDir(path2, &fd) == 0) {
                        nentries2 = 0;
                        sysLv2FsCloseDir(fd);
                    } else path2[n] = 0;
                    


                }
                pos2 = sel2 = 0;
                
            } else {
                char *ext =get_extension(entries2[sel2].d_name);

                if(!(entries2[sel2].d_type & 2) && (!strcmp(ext, ".pkg") || !strcmp(ext, ".PKG"))) {
            
                    install_pkg(path2, entries2[sel2].d_name);
                } else if(!(entries2[sel2].d_type & 2) && (!strcmp(ext, ".self") || !strcmp(ext, ".SELF"))) {

                    void fun_exit();

                    sprintf(temp_buffer, "%s/%s", path2, entries2[sel2].d_name);
                    fun_exit();
                    sysProcessExitSpawn2(temp_buffer, NULL, NULL, NULL, 0, 3071, SYS_PROCESS_SPAWN_STACK_SIZE_1M);
                    exit(0);
                } else {
                    sprintf(temp_buffer, "%s/%s", path2, entries2[sel2].d_name);
                    hex_mode = 0;
                    hex_editor(temp_buffer);
                }
            }

        } // cross

        if(new_pad & BUTTON_CIRCLE) { // select one file/folder

            if(path2[1]!=0 && strcmp(entries2[sel2].d_name,"..")) entries2[sel2].d_type ^=2;
            
        } // circle

        if(new_pad & BUTTON_SQUARE) {

            u32 flag= (entries2[sel2].d_type ^ 2) & 2;

            if(path2[1]!=0) { // select all files/folders
                for(n = 0; n< nentries2; n++)
                    if(strcmp(entries2[n].d_name,"..")) entries2[n].d_type = (entries2[n].d_type & ~2) | flag;
            }
            
        } // square
        if(new_pad & BUTTON_L3) {
            change_path2--; if(change_path2<0) change_path2=2;
            nentries2=0;
            pos2 = sel2 = 0;

            switch(change_path2) {
                case 0:
                    strcpy(path2, "/");
                    break;
                case 1:
                    strcpy(path2, self_path);
                    break;
                 case 2:
                    strcpy(path2, path1);
                    break;
            }
        } // l2
            
        if(new_pad & BUTTON_R3) {
            change_path2++; if(change_path2>2) change_path2=0;
            nentries2=0;
            pos2 = sel2 = 0;

            switch(change_path2) {
                case 0:
                    strcpy(path2, "/");
                    break;
                case 1:
                    strcpy(path2, self_path);
                    break;
                 case 2:
                    strcpy(path2, path1);
                    break;
            }
        } // r2
    }
    }// set menu

    }


    if(copy_mem) free(copy_mem); copy_mem = NULL;

}

