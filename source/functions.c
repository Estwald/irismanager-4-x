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

#include <fcntl.h>

#include "common.h"
#include "ftp.h"
#include "syscall8.h"

void absPath(char* absPath, const char* path, const char* cwd)
{
    if(path[0] == '/')
    {
        strcpy(absPath, path);
    }
    else
    {
        strcpy(absPath, cwd);
        
        if(cwd[strlen(cwd) - 1] != '/')
        {
            strcat(absPath, "/");
        }
        
        strcat(absPath, path);
    }
}

int exists(const char* path)
{
    sysFSStat entry;

//    sys8_perm_mode(ftp_perms);
    int r = sysLv2FsStat(path, &entry);
//    sys8_perm_mode(0);
    return r;
}

int isDir(const char* path)
{
   sysFSStat entry;

//   sys8_perm_mode(ftp_perms);
   sysLv2FsStat(path, &entry);
//   sys8_perm_mode(0);
   return ((entry.st_mode & S_IFDIR) != 0);
}

/*void stoupper(char *s)
{
    do if (96 == (224 & *s)) *s &= 223;
    while (*s++);
}*/

int ssplit(const char* str, char* left, int lmaxlen, char* right, int rmaxlen)
{
    size_t ios = strcspn(str, " ");
    int ret = (ios < strlen(str) - 1);
    int lmax = (ios < lmaxlen) ? ios : lmaxlen;
    
    strncpy(left, str, lmax);
    left[lmax] = '\0';
    
    if(ret)
    {
        strncpy(right, str + ios + 1, rmaxlen);
        right[rmaxlen] = '\0';
    }
    else
    {
        right[0] = '\0';
    }
    
    return ret;
}

