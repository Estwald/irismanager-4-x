/* 
    (c) 2011 Hermes/Estwald <www.elotrolado.net>
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
    apayloadlong with HMANAGER4.  If not, see <http://www.gnu.org/licenses/>.

*/

/* 
    (c) 2013 Estwald/Hermes <www.elotrolado.net>

    MAKEPS3ISO, EXTRACTPS3ISO and PATCHPS3ISO is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MAKEPS3ISO, EXTRACTPS3ISO and PATCHPS3ISO is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    apayloadlong with MAKEPS3ISO, EXTRACTPS3ISO and PATCHPS3ISO .  If not, see <http://www.gnu.org/licenses/>.

*/

#include "iso.h"

#include "utils.h"
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <ctype.h>


//#define ALIGNED32SECTORS 1

#define NOPS3_UPDATE 1

#define TICKS_PER_SEC 0x4c1a6bdULL

static u16 wstring[1024];

static char temp_string[1024];

static inline u64 get_ticks(void)
{
    u64 ticks;
    asm volatile("mftb %0" : "=r" (ticks));
    return ticks;
}

#define SWAP16(x) (x)

#define SWAP32(x) (x)

static int get_input_char()
{
    pad_last_time = 0;

    while(1) {
   
        tiny3d_Flip();
        ps3pad_read();

        tiny3d_Project2D();
        DbgDraw();

        if(new_pad & (BUTTON_CROSS | BUTTON_CIRCLE)) return (int) 'y';
        if(new_pad & BUTTON_TRIANGLE) return (int) 'n';
    }

    return 0;
}

static int get_input_abort()
{
    pad_last_time = 0;

    ps3pad_read();

    if(new_pad & BUTTON_TRIANGLE) return 1;

    return 0;
}

void UTF8_to_UTF16(u8 *stb, u16 *stw);
void UTF16_to_UTF8(u16 *stw, u8 *stb);

static void utf8_to_ansiname(char *utf8, char *ansi, int len)
{
u8 *ch= (u8 *) utf8;
u8 c;
int is_space = 1;

char *a = ansi;

    *ansi = 0;

	while(*ch!=0 && len>0){

	// 3, 4 bytes utf-8 code 
	if(((*ch & 0xF1)==0xF0 || (*ch & 0xF0)==0xe0) && (*(ch+1) & 0xc0) == 0x80){

	if(!is_space) {
        *ansi++=' '; // ignore
        len--;
        is_space = 1;
    }
	
	ch+= 2+1*((*ch & 0xF1) == 0xF0);
	
	}
	else 
	// 2 bytes utf-8 code	
	if((*ch & 0xE0)==0xc0 && (*(ch+1) & 0xc0)==0x80){
	
        c= (((*ch & 3)<<6) | (*(ch+1) & 63));

        if(c>=0xC0 && c<=0xC5) c='A';
        else if(c==0xc7) c='C';
        else if(c>=0xc8 && c<=0xcb) c='E';
        else if(c>=0xcc && c<=0xcf) c='I';
        else if(c==0xd1) c='N';
        else if(c>=0xd2 && c<=0xd6) c='O';
        else if(c>=0xd9 && c<=0xdc) c='U';
        else if(c==0xdd) c='Y';
        else if(c>=0xe0 && c<=0xe5) c='a';
        else if(c==0xe7) c='c';
        else if(c>=0xe8 && c<=0xeb) c='e';
        else if(c>=0xec && c<=0xef) c='i';
        else if(c==0xf1) c='n';
        else if(c>=0xf2 && c<=0xf6) c='o';
        else if(c>=0xf9 && c<=0xfc) c='u';
        else if(c==0xfd || c==0xff) c='y';
        else if(c>127) c=*(++ch+1); //' ';

        if(!is_space || c!= 32) {
           *ansi++=c;
            len--;
            if(c == 32) is_space = 1; else is_space = 0;
        }

	    ch++;
	
	} else {
	
        if(*ch<32) *ch=32;

        if(!is_space || *ch!= 32) {
           *ansi++=*ch;
        
            len--;

            if(*ch == 32) is_space = 1; else is_space = 0;
        }
	
	}

	ch++;
	}
	
    while(len>0) {
	    *ansi++=0;
	    len--;
	}

    if(a[0] == 0 || a[0] == ' ') strcpy(a, "PS3");

}


extern int firmware;

#define ISODCL(from, to) (to - from + 1)

static void setdaterec(unsigned char *p,int dd,int mm,int aa,int ho,int mi,int se)
{
    *p++=(unsigned char) ((aa-1900) & 255);*p++=(char) (mm & 15) ;*p++=(char) (dd & 31);*p++=(char) ho;*p++=(char) mi;*p++=(char) se;*p++=(char) 0;

}

static void set731(unsigned char *p,int n)
{
    *p++=(n & 0xff);*p++=((n>>8) & 0xff);*p++=((n>>16) & 0xff);*p++=((n>>24) & 0xff);
}

static void set733(unsigned char *p,int n)
{
    *p++=(n & 0xff);*p++=((n>>8) & 0xff);*p++=((n>>16) & 0xff);*p++=((n>>24) & 0xff);
    *p++=((n>>24) & 0xff);*p++=((n>>16) & 0xff);*p++=((n>>8) & 0xff);*p++=(n & 0xff);
}

static void set732(unsigned char *p,int n)
{
    *p++=((n>>24) & 0xff);*p++=((n>>16) & 0xff);*p++=((n>>8) & 0xff);*p++=(n & 0xff);
}

static void set721(unsigned char *p,int n)
{
    *p++=(n & 0xff);*p++=((n>>8) & 0xff);
}

static void set722(unsigned char *p,int n)
{
    *p++=((n>>8) & 0xff);*p++=(n & 0xff);
}

static void set723(unsigned char *p,int n)
{
    *p++=(n & 0xff);*p++=((n>>8) & 0xff);*p++=((n>>8) & 0xff);*p++=(n & 0xff);
}

static int isonum_731 (unsigned char * p)
{
	return ((p[0] & 0xff)
		| ((p[1] & 0xff) << 8)
		| ((p[2] & 0xff) << 16)
		| ((p[3] & 0xff) << 24));
}

/*
static int isonum_732 (unsigned char * p)
{
	return ((p[3] & 0xff)
		| ((p[2] & 0xff) << 8)
		| ((p[1] & 0xff) << 16)
		| ((p[0] & 0xff) << 24));
}
*/

static int isonum_733 (unsigned char * p)
{
	return (isonum_731 (p));
}


static int isonum_721 (unsigned char * p)
{
	return ((p[0] & 0xff) | ((p[1] & 0xff) << 8));
}

/*
static int isonum_723 (unsigned char * p)
{
	return (isonum_721 (p));
}
*/

struct iso_primary_descriptor {
	unsigned char type			[ISODCL (  1,   1)]; /* 711 */
	unsigned char id				[ISODCL (  2,   6)];
	unsigned char version			[ISODCL (  7,   7)]; /* 711 */
	unsigned char unused1			[ISODCL (  8,   8)];
	unsigned char system_id			[ISODCL (  9,  40)]; /* aunsigned chars */
	unsigned char volume_id			[ISODCL ( 41,  72)]; /* dunsigned chars */
	unsigned char unused2			[ISODCL ( 73,  80)];
	unsigned char volume_space_size		[ISODCL ( 81,  88)]; /* 733 */
	unsigned char unused3			[ISODCL ( 89, 120)];
	unsigned char volume_set_size		[ISODCL (121, 124)]; /* 723 */
	unsigned char volume_sequence_number	[ISODCL (125, 128)]; /* 723 */
	unsigned char logical_block_size		[ISODCL (129, 132)]; /* 723 */
	unsigned char path_table_size		[ISODCL (133, 140)]; /* 733 */
	unsigned char type_l_path_table		[ISODCL (141, 144)]; /* 731 */
	unsigned char opt_type_l_path_table	[ISODCL (145, 148)]; /* 731 */
	unsigned char type_m_path_table		[ISODCL (149, 152)]; /* 732 */
	unsigned char opt_type_m_path_table	[ISODCL (153, 156)]; /* 732 */
	unsigned char root_directory_record	[ISODCL (157, 190)]; /* 9.1 */
	unsigned char volume_set_id		[ISODCL (191, 318)]; /* dunsigned chars */
	unsigned char publisher_id		[ISODCL (319, 446)]; /* achars */
	unsigned char preparer_id		[ISODCL (447, 574)]; /* achars */
	unsigned char application_id		[ISODCL (575, 702)]; /* achars */
	unsigned char copyright_file_id		[ISODCL (703, 739)]; /* 7.5 dchars */
	unsigned char abstract_file_id		[ISODCL (740, 776)]; /* 7.5 dchars */
	unsigned char bibliographic_file_id	[ISODCL (777, 813)]; /* 7.5 dchars */
	unsigned char creation_date		[ISODCL (814, 830)]; /* 8.4.26.1 */
	unsigned char modification_date		[ISODCL (831, 847)]; /* 8.4.26.1 */
	unsigned char expiration_date		[ISODCL (848, 864)]; /* 8.4.26.1 */
	unsigned char effective_date		[ISODCL (865, 881)]; /* 8.4.26.1 */
	unsigned char file_structure_version	[ISODCL (882, 882)]; /* 711 */
	unsigned char unused4			[ISODCL (883, 883)];
	unsigned char application_data		[ISODCL (884, 1395)];
	unsigned char unused5			[ISODCL (1396, 2048)];
};

struct iso_directory_record {
	unsigned char length			[ISODCL (1, 1)]; /* 711 */
	unsigned char ext_attr_length		[ISODCL (2, 2)]; /* 711 */
	unsigned char extent			[ISODCL (3, 10)]; /* 733 */
	unsigned char size			[ISODCL (11, 18)]; /* 733 */
	unsigned char date			[ISODCL (19, 25)]; /* 7 by 711 */
	unsigned char flags			[ISODCL (26, 26)];
	unsigned char file_unit_size		[ISODCL (27, 27)]; /* 711 */
	unsigned char interleave			[ISODCL (28, 28)]; /* 711 */
	unsigned char volume_sequence_number	[ISODCL (29, 32)]; /* 723 */
	unsigned char name_len		[1]; /* 711 */
	unsigned char name			[1];
};

struct iso_path_table{
	unsigned char  name_len[2];	/* 721 */
	char extent[4];		/* 731 */
	char  parent[2];	/* 721 */
	char name[1];
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void get_rand(void *bfr, u32 size)
{
	int n;
	
	if (size == 0)
		return;

	srand(get_ticks());

    for(n = 0; n < size; n++)
        *(((char *) bfr) + n) = rand() & 0xFF;
}

u64 get_disk_free_space(char *path)
{
    
    struct statvfs svfs;
    u32 blockSize;
    u64 freeSize = 0;
    int is_ntfs = 0; 

    if(!strncmp(path, "/ntfs", 5) || !strncmp(path, "/ext", 4)) is_ntfs = 1;

    if(!is_ntfs) {
        if(sysFsGetFreeSize(path, &blockSize, &freeSize) !=0) return (u64) (-1LL);
        return (((u64)blockSize * freeSize));
    } else {
    
        if(ps3ntfs_statvfs((const char *) path, &svfs)!=0)
            return (u64) (-1LL);
    }

    return ( ((u64)svfs.f_bsize * svfs.f_bfree));


}

#ifdef USE_64BITS_LSEEK
int get_iso_file_pos(int fd, char *path, u32 *flba, u64 *size)
#else
int get_iso_file_pos(FILE *fp, char *path, u32 *flba, u64 *size)
#endif
{
    static struct iso_primary_descriptor sect_descriptor;
    struct iso_directory_record * idr;
    static int folder_split[64][3];
    int nfolder_split = 0;

    u32 file_lba = 0xffffffff;
   
    u8 *sectors = NULL;

    #ifdef USE_64BITS_LSEEK
    if(fd <= 0 || !size || !flba) return -1;
    #else
    if(!fp || !size || !flba) return -1;
    #endif

    *size = 0;
  
    folder_split[nfolder_split][0] = 0;
    folder_split[nfolder_split][1] = 0;
    folder_split[nfolder_split][2] = 0;
    int i = 0;

    while(path[i]!=0) {
        if(path[i]=='/') {

            folder_split[nfolder_split][2] = i - folder_split[nfolder_split][1];
            while(path[i]=='/') i++;
            if(folder_split[nfolder_split][2]==0) {folder_split[nfolder_split][1] = i; continue;}
            folder_split[nfolder_split][0] = 1;
            nfolder_split++;
            folder_split[nfolder_split][0] = 0;
            folder_split[nfolder_split][1] = i;
            folder_split[nfolder_split][2] = 0;

        } else i++;
    }

    folder_split[nfolder_split][0] = 0;
    folder_split[nfolder_split][2] = i - folder_split[nfolder_split][1];
    nfolder_split++;


    #ifdef USE_64BITS_LSEEK
    if(ps3ntfs_seek64(fd, 0x8800LL, SEEK_SET)!=0x8800LL) goto err; 
    if(ps3ntfs_read(fd, (void *) &sect_descriptor, sizeof(struct iso_primary_descriptor)) != sizeof(struct iso_primary_descriptor)) goto err;      
    #else
    if(fseek(fp, 0x8800, SEEK_SET)!=0) goto err;
    if(fread((void *) &sect_descriptor, 1, sizeof(struct iso_primary_descriptor), fp) != sizeof(struct iso_primary_descriptor)) goto err;
    #endif

    if(sect_descriptor.type[0]!=2 || strncmp((void *) sect_descriptor.id, "CD001", 5)) goto err;

    u32 lba0 = isonum_731(&sect_descriptor.type_l_path_table[0]); // lba
    u32 size0 = isonum_733(&sect_descriptor.path_table_size[0]); // tamaño
    
    //char string[256];
    //sprintf(string, "lba0 %u size %u\n", lba0, size0);
    //DrawDialogOK(string);
    //return -4;

    int size1 = ((size0 + 2047)/2048) * 2048;
    sectors = malloc(size1 + 2048);
    if(!sectors) return -3;

    memset(sectors, 0, size1 + 2048);

    #ifdef USE_64BITS_LSEEK
    if(ps3ntfs_seek64(fd, ((s64) lba0) * 2048LL, SEEK_SET) != ((s64) lba0) * 2048LL) goto err; 
    if(ps3ntfs_read(fd, (void *) sectors, size1) != size1) goto err;
    #else
    if(fseek(fp, lba0 * 2048, SEEK_SET)!=0) goto err;
    if(fread((void *) sectors, 1, size1, fp) != size1) goto err;
    #endif
    
    u32 p = 0;

    u32 lba_folder = 0xffffffff;
    u32 lba  = 0xffffffff;

    int nsplit = 0;
    int last_parent = 1;
    int cur_parent = 1;

    while(p < size0) {
      
        if(nsplit >= nfolder_split) break;
        if(folder_split[nsplit][0] == 0 && nsplit!=0) {lba_folder = lba; break;}
        if(folder_split[nsplit][2] == 0) continue;

        u32 snamelen = isonum_721(&sectors[p]);
        if(snamelen == 0) p= ((p/2048) * 2048) + 2048; //break;
        p+=2;
        lba = isonum_731(&sectors[p]);
        p+=4;
        u32 parent_name = isonum_721(&sectors[p]);
        p+=2;

        memset(wstring, 0, 512 * 2);
        memcpy(wstring, &sectors[p], snamelen);

        UTF16_to_UTF8(wstring, (u8 *) temp_string);

        if(cur_parent==1 && folder_split[nsplit][0] == 0 && nsplit==0) {lba_folder = lba; break;}

        if(last_parent == parent_name && strlen(temp_string) == folder_split[nsplit][2] 
            && !strncmp((void *) temp_string, &path[folder_split[nsplit][1]], folder_split[nsplit][2])) {
            
            //DPrintf("p: %s %u %u\n", &path[folder_split[nsplit][1]], folder_split[nsplit][2], snamelen);
            last_parent = cur_parent;
            
            nsplit++;
            if(folder_split[nsplit][0] == 0) {lba_folder = lba; break;}
        }
        
        p+= snamelen;
        cur_parent++;
        if(snamelen & 1) {p++;}
    }
    
    if(lba_folder == 0xffffffff) goto err;

    memset(sectors, 0, 4096);
    
    #ifdef USE_64BITS_LSEEK
    if(ps3ntfs_seek64(fd, ((s64) lba_folder) * 2048LL, SEEK_SET)!=((s64) lba_folder) * 2048LL) goto err; 
    if(ps3ntfs_read(fd, (void *) sectors, 2048) != 2048) goto err;      
    #else
    if(fseek(fp, lba_folder * 2048, SEEK_SET)!=0) goto err;
    if(fread((void *) sectors, 1, 2048, fp) != 2048) goto err;
    #endif

    p = 0;

    int size_directory = -1;
    int p2 = 0;
    while(1) {
        if(nsplit >= nfolder_split) break;
        idr = (struct iso_directory_record *) &sectors[p];

        if(size_directory == -1) {
                
            if((int) idr->name_len[0] == 1 && idr->name[0]== 0 && lba == isonum_731((void *) idr->extent) && idr->flags[0] == 0x2) {
                size_directory = isonum_733((void *) idr->size);
             
            } 
        }

        if(idr->length[0] == 0 && sizeof(struct iso_directory_record) + p > 2048) {
            lba_folder++;
           
            #ifdef USE_64BITS_LSEEK
            if(ps3ntfs_seek64(fd, ((s64) lba_folder) * 2048LL, SEEK_SET)!=((s64) lba_folder) * 2048LL) goto err;
            if(ps3ntfs_read(fd, (void *) sectors, 2048) != 2048) goto err;
            #else
            if(fseek(fp, lba_folder * 2048, SEEK_SET)!=0) goto err;
            if(fread((void *) sectors, 1, 2048, fp) != 2048) goto err;
            #endif

            p = 0; p2= (p2 & ~2047) + 2048;
            
            idr = (struct iso_directory_record *) &sectors[p];
            if((int) idr->length[0] == 0) break;
            if((size_directory == -1 && idr->length[0] == 0) || (size_directory != -1 && p2 >= size_directory)) break;
            continue;
        }

        if((size_directory == -1 && idr->length[0] == 0) || (size_directory != -1 && p2 >= size_directory)) break;

        if((int) idr->length[0] == 0) break;

        memset(wstring, 0, 512 * 2);
        memcpy(wstring, (char *) idr->name, idr->name_len[0]);

        UTF16_to_UTF8(wstring, (u8 *) temp_string);
    
        if(strlen(temp_string) == folder_split[nsplit][2]
            && !strncmp((char *) temp_string, &path[folder_split[nsplit][1]], (int) folder_split[nsplit][2])) {
            if(file_lba == 0xffffffff) file_lba = isonum_733(&idr->extent[0]);
            
            *size+= (u64) (u32) isonum_733(&idr->size[0]);     
          
        } else if(file_lba != 0xffffffff) break;
        

        p+= idr->length[0]; p2+= idr->length[0];
    }
    
    *flba = file_lba;

    if(file_lba == 0xffffffff) goto err;

    #ifdef USE_64BITS_LSEEK
    if(ps3ntfs_seek64(fd, ((s64) file_lba) * 2048LL, SEEK_SET)!=((s64) file_lba) * 2048LL) goto err; 
    #else
    if(fseek(fp, file_lba * 2048, SEEK_SET)!=0) goto err;
    #endif

    if(sectors) free(sectors);

    return 0;

 err:
    if(sectors) free(sectors);

    return -4;
}

static char * get_extension(char *path)
{
    int n = strlen(path);
    int m = n;
    
    while(m>1 && path[m]!='.' && path[m]!='/') m--;

    if(path[m]=='.') return &path[m];

    return &path[n];
}


int create_fake_file_iso(char *path, char *filename, u64 size)
{
    int len_string;
    u8 *mem = malloc(sizeof(build_iso_data));
    u16 *string = (u16 *) malloc(256);
    if(!mem || !string) return -1;

    char name[65];
    strncpy(name, filename, 64);
    name[64] = 0;

    if(strlen(filename) > 64) { // break the string
        int pos = 63 - strlen(get_extension(filename));
        while(pos > 0 && (name[pos] & 192) == 128) pos--; // skip UTF extra codes
        strcpy(&name[pos], get_extension(filename));
    }

    UTF8_to_UTF16((u8 *) name, string);

    for(len_string = 0; len_string < 512; len_string++) if(string[len_string] == 0) break;

    memcpy(mem, build_iso_data, sizeof(build_iso_data));

    struct iso_primary_descriptor *ipd = (struct iso_primary_descriptor *) &mem[0x8000];
    struct iso_primary_descriptor *ipd2 = (struct iso_primary_descriptor *) &mem[0x8800];
    struct iso_directory_record * idr = (struct iso_directory_record *) &mem[0xB840];
    struct iso_directory_record * idr2 = (struct iso_directory_record *) &mem[0xC044];

    u32 last_lba = isonum_733 (ipd->volume_space_size);

    u64 size0 = size;
    
    while(size > 0) {
        
        u8 flags = 0;

        if(size > 0xFFFFF800ULL) {flags = 0x80; size0 = 0xFFFFF800ULL;} else size0 = size;
        idr->name_len[0] = strlen(name);
        memcpy(idr->name, name, idr->name_len[0]);
        idr->length[0] = (idr->name_len[0] + sizeof(struct iso_directory_record) + 1) & ~1;
        idr->ext_attr_length[0] = 0;
        set733(idr->extent, last_lba);
        set733(idr->size, size0);
        idr->date[0] = 0x71; idr->date[1] = 0x0B; 
        idr->date[2] = 0x0A; idr->date[3] = 0x0D;
        idr->date[4] = 0x38; idr->date[5] = 0x00;
        idr->date[6] = 0x04;
        idr->flags[0] = flags;
        idr->file_unit_size[0] = 0;
        idr->interleave[0] = 0;
        set723(idr->volume_sequence_number, 1);

        idr = (struct iso_directory_record *) (((char *) idr) + idr->length[0]);

        /////////////////////

        idr2->name_len[0] = len_string * 2;
        
        memcpy(idr2->name, string, idr2->name_len[0]);

        idr2->length[0] = (idr2->name_len[0] + sizeof(struct iso_directory_record) + 1) & ~1;
        idr2->ext_attr_length[0] = 0;
        set733(idr2->extent, last_lba);
        set733(idr2->size, size0);
        idr2->date[0] = 0x71; idr2->date[1] = 0x0B; 
        idr2->date[2] = 0x0A; idr2->date[3] = 0x0D;
        idr2->date[4] = 0x38; idr2->date[5] = 0x00;
        idr2->date[6] = 0x04;
        idr2->flags[0] = flags;
        idr2->file_unit_size[0] = 0;
        idr2->interleave[0] = 0;
        set723(idr2->volume_sequence_number, 1);

        idr2 = (struct iso_directory_record *) (((char *) idr2) + idr2->length[0]);

        /////////////////////
        last_lba += (size0 + 0x7ffULL)/ 0x800ULL;
        size-= size0;

    }

    last_lba += (size + 2047)/2048;
    set733(ipd->volume_space_size, last_lba);
    set733(ipd2->volume_space_size, last_lba);

    FILE *fp2 =fopen(path, "wb");
   
    if(fp2) {
        fwrite((void *) mem, 1, sizeof(build_iso_data), fp2);
        fclose(fp2);
    }

    free(mem);
    free(string);

    return 0;
}

char * create_fake_file_iso_mem(char *filename, u64 size, u32 *nsize)
{
    int len_string;

    if(!nsize) return NULL;
    *nsize = sizeof(build_iso_data);

    u8 *mem = malloc(sizeof(build_iso_data));
    u16 *string = (u16 *) malloc(256);
    if(!mem || !string) return NULL;

    char name[65];
    strncpy(name, filename, 64);
    name[64] = 0;
    
    if(strlen(filename) > 64) { // break the string
        int pos = 63 - strlen(get_extension(filename));
        while(pos > 0 && (name[pos] & 192) == 128) pos--; // skip UTF extra codes
        strcpy(&name[pos], get_extension(filename));
    }

    UTF8_to_UTF16((u8 *) name, string);

    for(len_string = 0; len_string < 512; len_string++) if(string[len_string] == 0) break;

    memcpy(mem, build_iso_data, sizeof(build_iso_data));

    struct iso_primary_descriptor *ipd = (struct iso_primary_descriptor *) &mem[0x8000];
    struct iso_primary_descriptor *ipd2 = (struct iso_primary_descriptor *) &mem[0x8800];
    struct iso_directory_record * idr = (struct iso_directory_record *) &mem[0xB840];
    struct iso_directory_record * idr2 = (struct iso_directory_record *) &mem[0xC044];

    u32 last_lba = isonum_733 (ipd->volume_space_size);

    u64 size0 = size;
    
    while(size > 0) {
        
        u8 flags = 0;

        if(size > 0xFFFFF800ULL) {flags = 0x80; size0 = 0xFFFFF800ULL;} else size0 = size;
        idr->name_len[0] = strlen(name);
        memcpy(idr->name, name, idr->name_len[0]);
        idr->length[0] = (idr->name_len[0] + sizeof(struct iso_directory_record) + 1) & ~1;
        idr->ext_attr_length[0] = 0;
        set733(idr->extent, last_lba);
        set733(idr->size, size0);
        idr->date[0] = 0x71; idr->date[1] = 0x0B; 
        idr->date[2] = 0x0A; idr->date[3] = 0x0D;
        idr->date[4] = 0x38; idr->date[5] = 0x00;
        idr->date[6] = 0x04;
        idr->flags[0] = flags;
        idr->file_unit_size[0] = 0;
        idr->interleave[0] = 0;
        set723(idr->volume_sequence_number, 1);

        idr = (struct iso_directory_record *) (((char *) idr) + idr->length[0]);

        /////////////////////

        idr2->name_len[0] = len_string * 2;
        
        memcpy(idr2->name, string, idr2->name_len[0]);

        idr2->length[0] = (idr2->name_len[0] + sizeof(struct iso_directory_record) + 1) & ~1;
        idr2->ext_attr_length[0] = 0;
        set733(idr2->extent, last_lba);
        set733(idr2->size, size0);
        idr2->date[0] = 0x71; idr2->date[1] = 0x0B; 
        idr2->date[2] = 0x0A; idr2->date[3] = 0x0D;
        idr2->date[4] = 0x38; idr2->date[5] = 0x00;
        idr2->date[6] = 0x04;
        idr2->flags[0] = flags;
        idr2->file_unit_size[0] = 0;
        idr2->interleave[0] = 0;
        set723(idr2->volume_sequence_number, 1);

        idr2 = (struct iso_directory_record *) (((char *) idr2) + idr2->length[0]);

        /////////////////////
        last_lba += (size0 + 0x7ffULL)/ 0x800ULL;
        size-= size0;

    }

    last_lba += (size + 2047)/2048;
    set733(ipd->volume_space_size, last_lba);
    set733(ipd2->volume_space_size, last_lba);
    
    free(string);
    return (char *) mem;
}

/***********************************************************************************************************/
/* MAKEPS3ISO - EXTRACTPS3ISO - PATCHPS3ISO                                                                */
/***********************************************************************************************************/

static int param_patched = 0;
static int self_sprx_patched = 0;

static int cur_isop = -1;

static int lpath;
static int wpath;

static u32 llba0 = 0; // directory path0
static u32 llba1 = 0; // directory path1
static u32 wlba0 = 0; // directory pathw0
static u32 wlba1 = 0; // directory pathw1

static u32 dllba = 0; // dir entries
static u32 dwlba = 0; // dir entriesw
static u32 dlsz = 0; // dir entries size (sectors)
static u32 dwsz = 0; // dir entriesw size (sectors)
static u32 flba = 0; // first lba for files
static u32 toc = 0;  // TOC of the iso

static char iso_split = 0;
static char output_name[0x420];
static char output_name2[0x420];

static int pos_lpath0 = 0;
static int pos_lpath1 = 0;
static int pos_wpath0 = 0;
static int pos_wpath1 = 0;

static int dd = 1, mm = 1, aa = 2013, ho = 0, mi = 0, se = 2;

static u8 *sectors = NULL;
static u8 *sectors3 = NULL;


#define MAX_ISO_PATHS 4096

typedef struct {
    u32 ldir;
    u32 wdir;
    u32 llba;
    u32 wlba;
    int parent;
    char *name;

} _directory_iso;

typedef struct {
    int parent;
    char *name;

} _directory_iso2;

typedef struct {
    u32 size;
    char path[0x420];

} _split_file;

static _split_file *split_file = NULL;

static int fd_split = -1;
static int fd_split0 = -1;

static int split_index = 0;
static int split_files = 0;

static _directory_iso *directory_iso = NULL;
static _directory_iso2 *directory_iso2 = NULL;

static char dbhead[64];

static void memcapscpy(void *dest, void *src, int size)
{
    char *d = dest;
    char *s = src;
    char c;

    int n;

    for(n = 0; n < size; n++) {c = *s++; *d++ = (char) toupper((int) c);}
}

static int iso_parse_param_sfo(char * file, char *title_id, char *title_name)
{
	int fd;
    int bytes;
    int ct = 0;
    
    fd = ps3ntfs_open(file, O_RDONLY, 0766);

	if(fd >= 0)
		{
		int len, pos, str;
		unsigned char *mem=NULL;

        len = (int) ps3ntfs_seek64(fd, 0, SEEK_END);

		mem= (unsigned char *) malloc(len+16);
		if(!mem) {ps3ntfs_close(fd); return -2;}

		memset(mem, 0, len+16);

		ps3ntfs_seek64(fd, 0, SEEK_SET);

        bytes = ps3ntfs_read(fd, (void *) mem, len);

        ps3ntfs_close(fd);

        if(bytes != len) {
            free(mem);
            return -2;
        }

		str= (mem[8]+(mem[9]<<8));
		pos=(mem[0xc]+(mem[0xd]<<8));

		int indx=0;
        
		while(str<len) {
			if(mem[str]==0) break;

            if(!strcmp((char *) &mem[str], "TITLE")) {
                strncpy(title_name, (char *) &mem[pos], 63);
                title_name[63] = 0;
                ct++;
            }
            else 
			if(!strcmp((char *) &mem[str], "TITLE_ID")) {
                memcpy(title_id, (char *) &mem[pos], 4);
                title_id[4] = '-';
				strncpy(&title_id[5], (char *) &mem[pos + 4], 58);
                title_id[63] = 0;
                ct++;
				
		    }

            if(ct == 2) {
                free(mem);
				return 0;
            }

			while(mem[str]) str++;str++;
			pos+=(mem[0x1c+indx]+(mem[0x1d+indx]<<8));
			indx+=16;
		}

		if(mem) free(mem);
        
		}

	
	return -1;

}

static int calc_entries(char *path, int parent)
{
    DIR  *dir;
    int len_string;
    struct stat s;

    int cldir = 0;
    int ldir = sizeof(struct iso_directory_record) + 6; // ..
    ldir = (ldir + 7) & ~7;
    ldir += sizeof(struct iso_directory_record) + 6; // .
    ldir += ldir & 1;
    
    int cwdir = 0;
    int wdir = sizeof(struct iso_directory_record) + 6; // ..
    wdir = (wdir + 7) & ~7;
    wdir += sizeof(struct iso_directory_record) + 6; // .
    wdir += wdir & 1;

    cldir = ldir;
    cwdir = wdir;

    lpath+= (lpath & 1);
    wpath+= (wpath & 1);

    int cur = cur_isop;

    if(cur >= MAX_ISO_PATHS) return -444;

    directory_iso[cur].ldir = ldir;
    directory_iso[cur].wdir = wdir;
    directory_iso[cur].parent = parent;
    if(!cur) {
        directory_iso[cur].name = malloc(16); 
        if(!directory_iso[cur].name) return -1;
        directory_iso[cur].name[0] = 0;
    }

    int cur2 = cur;

    cur_isop++;

    // files
    dir = opendir (path);
    if(dir) {
        while(1) {
        
        struct dirent *entry = readdir (dir);
            
        if(!entry) break;
        if(entry->d_name[0]=='.' && (entry->d_name[1]=='.' || entry->d_name[1]== 0)) continue;

        int len = strlen(path);
        strcat(path,"/");
        strcat(path, entry->d_name);

        if(stat(path, &s)<0) {closedir(dir); return -669;}
        
        path[len] = 0;

        if(!S_ISDIR(s.st_mode)) {
            
            int lname = strlen(entry->d_name);

            if(lname >=6 && !strcmp(&entry->d_name[lname -6], ".66600")) { // build size of .666xx files
                u64 size = s.st_size;
                lname -= 6;
                int n;

                memcpy(temp_string, entry->d_name, lname);
                temp_string[lname] = 0;

                if(lname > 222) {closedir(dir); return -555;}

                UTF8_to_UTF16((u8 *) temp_string, wstring);

                for(len_string = 0; len_string < 512; len_string++) if(wstring[len_string] == 0) break;

                if(len_string > 222) {closedir(dir); return -555;}


                path[len] = 0;

                for(n = 1; n < 100; n++) {

                    int len2 = strlen(path);
                    strcat(path,"/");

                    int l = strlen(path);

                    memcpy(path + l, entry->d_name, lname);
                    

                    sprintf(&path[l + lname], ".666%2.2u", n);

                    if(stat(path, &s)<0) {s.st_size = size; path[len2] = 0; break;}
                    
                    path[len2] = 0;
                    
                    size += s.st_size;     
                    
                }

                path[len] = 0;

            
            } else
                if(lname >=6 && !strncmp(&entry->d_name[lname -6], ".666", 4)) continue; // ignore .666xx files
                else {
                
                    if(strlen(entry->d_name) > 222) {closedir(dir); return -555;}

                    UTF8_to_UTF16((u8 *) entry->d_name, wstring);

                    for(len_string = 0; len_string < 512; len_string++) if(wstring[len_string] == 0) break;

                    if(len_string > 222) {closedir(dir); return -555;}
                }


            int parts = s.st_size ? (int) ((((u64) s.st_size) + 0xFFFFF7FFULL)/0xFFFFF800ULL) : 1;

            int n;

            for(n = 0; n < parts; n++) {

                int add;

                add = sizeof(struct iso_directory_record) + lname - 1 + 8; // add ";1"
                add+= add & 1;
                cldir += add;

                if(cldir > 2048) {
                
                    ldir = (ldir & ~2047) + 2048;
                    cldir = add;
                }

                ldir += add;
                //ldir += ldir & 1;

                add = sizeof(struct iso_directory_record) + len_string * 2 - 1 + 4 + 6;  // add "\0;\01"
                add+= add & 1;
                cwdir += add;

                if(cwdir > 2048) {
                 
                    wdir= (wdir & ~2047) + 2048;
                    cwdir = add;
                }

                wdir += add;
                //wdir += wdir & 1;
            }
        }

    }

    closedir (dir);
    
    // directories
    dir = opendir (path);
    if(dir) {
        while(1) {
        
            struct dirent *entry = readdir (dir);
                
            if(!entry) break;
            if(entry->d_name[0]=='.' && (entry->d_name[1]=='.' || entry->d_name[1]== 0)) continue;

            int len = strlen(path);
            strcat(path,"/");
            strcat(path, entry->d_name);

            if(stat(path, &s)<0) {closedir(dir); return -669;}
            
            path[len] = 0;

            if(S_ISDIR(s.st_mode)) {

                if(strlen(entry->d_name) > 222) {closedir(dir); return -555;}

                UTF8_to_UTF16((u8 *) entry->d_name, wstring);

                for(len_string = 0; len_string < 512; len_string++) if(wstring[len_string] == 0) break;

                if(len_string > 222) {closedir(dir); return -555;}
                

                lpath += sizeof(struct iso_path_table) + strlen(entry->d_name) - 1;
                lpath += (lpath & 1);

                int add;

                add = sizeof(struct iso_directory_record) + strlen(entry->d_name) - 1 + 6;
                add+= add & 1;
                cldir += add;

                if(cldir > 2048) {
                    
                    ldir = (ldir & ~2047) + 2048;
                    cldir = add;
                }
                
                ldir += add;
                //ldir += ldir & 1;

                wpath += sizeof(struct iso_path_table) + len_string * 2 - 1;
                wpath += (wpath & 1);

                add = sizeof(struct iso_directory_record) + len_string * 2 - 1 + 6;
                add+= add & 1;
                cwdir += add;

                if(cwdir > 2048) {

                    wdir= (wdir & ~2047) + 2048;
                    cwdir = add;
                }


                wdir += add;
                //wdir += wdir & 1;
                
            }

        }

        closedir (dir);
    }
   
    directory_iso[cur].ldir = (ldir + 2047)/2048;
    directory_iso[cur].wdir = (wdir + 2047)/2048;

    }

    // directories add
    dir = opendir (path);
    if(dir) {
        while(1) {
        
        struct dirent *entry = readdir (dir);
            
        if(!entry) break;
        if(entry->d_name[0]=='.' && (entry->d_name[1]=='.' || entry->d_name[1]== 0)) continue;
        
        int len = strlen(path);

        strcat(path,"/");
        strcat(path, entry->d_name);

        if(stat(path, &s)<0) {closedir(dir); return -669;}
           

        if(!(S_ISDIR(s.st_mode))) {path[len] = 0; continue;}

        //DPrintf("ss %s\n", path);

        directory_iso[cur_isop].name = malloc(strlen(entry->d_name) + 1);
        if(!directory_iso[cur_isop].name) {closedir(dir); return -1;}
        strcpy(directory_iso[cur_isop].name, entry->d_name);
        
        int ret = calc_entries(path, cur2 + 1);

        if(ret < 0) {closedir(dir); return ret;}

        path[len] = 0;

        }
        closedir (dir);
    }

    if(cur == 0) {

        llba0 = 20;
        llba1 = llba0 + ((lpath + 2047)/2048);
        wlba0 = llba1 + ((lpath + 2047)/2048);
        wlba1 = wlba0 + ((wpath + 2047)/2048);
        dllba = wlba1 + ((wpath + 2047)/2048);
        if(dllba < 32) dllba = 32;

        int n;
        
        int m, l;

        // searching...

        for(n = 1; n < cur_isop - 1; n++)
            for(m = n + 1; m < cur_isop; m++) {
            
                if(directory_iso[n].parent > directory_iso[m].parent) {

                    directory_iso[cur_isop] = directory_iso[n]; directory_iso[n] = directory_iso[m]; directory_iso[m] = directory_iso[cur_isop];

                    for(l = n; l < cur_isop; l++) {

                        if(n + 1 == directory_iso[l].parent) 
                            directory_iso[l].parent = m + 1;
                        else if(m + 1 == directory_iso[l].parent) 
                            directory_iso[l].parent = n + 1;
                    }

                }
        }
        
        for(n = 0; n < cur_isop; n++)  {
            dlsz+= directory_iso[n].ldir;
            dwsz+= directory_iso[n].wdir;
        }

        #ifdef ALIGNED32SECTORS
        dwlba = ((dllba + dlsz) + 31) & ~31;

        flba = ((dwlba + dwsz) + 31) & ~31;
        #else
        dwlba = (dllba + dlsz);
        flba = (dwlba + dwsz);
        #endif

        u32 lba0 = dllba;
        u32 lba1 = dwlba;

        for(n = 0; n < cur_isop; n++)  {
            
            directory_iso[n].llba = lba0;
            directory_iso[n].wlba = lba1;
            lba0 += directory_iso[n].ldir;
            lba1 += directory_iso[n].wdir;
            
        }
    }


/*
if(cur == 0) {
    int n;
        for(n = 0; n < cur_isop; n++)  {
            DPrintf("list %i %s\n", directory_iso[n].parent, directory_iso[n].name);
        }
}
*/

    return 0;

}

static int fill_dirpath(void)
{
    int n;
    struct iso_path_table *iptl0;
    struct iso_path_table *iptl1;
    struct iso_path_table *iptw0;
    struct iso_path_table *iptw1;

    for(n = 0; n < cur_isop; n++) {

        iptl0 = (void *) &sectors[pos_lpath0];
        iptl1 = (void *) &sectors[pos_lpath1];
        iptw0 = (void *) &sectors[pos_wpath0];
        iptw1 = (void *) &sectors[pos_wpath1];

        if(!n) {
            set721((void *) iptl0->name_len, 1);
            set731((void *) iptl0->extent, directory_iso[n].llba);
            set721((void *) iptl0->parent, directory_iso[n].parent);
            iptl0->name[0] = 0;
            pos_lpath0 += sizeof(struct iso_path_table) - 1 + 1;
            pos_lpath0 += pos_lpath0 & 1;
            iptl0 = (void *) &sectors[pos_lpath0];

            set721((void *) iptl1->name_len, 1);
            set732((void *) iptl1->extent, directory_iso[n].llba);
            set722((void *) iptl1->parent, directory_iso[n].parent);
            iptl1->name[0] = 0;
            pos_lpath1 += sizeof(struct iso_path_table) - 1 + 1;
            pos_lpath1 += pos_lpath1 & 1;
            iptl1 = (void *) &sectors[pos_lpath1];

            set721((void *) iptw0->name_len, 1);
            set731((void *) iptw0->extent, directory_iso[n].wlba);
            set721((void *) iptw0->parent, directory_iso[n].parent);
            iptw0->name[0] = 0;
            pos_wpath0 += sizeof(struct iso_path_table) - 1 + 1;
            pos_wpath0 += pos_wpath0 & 1;
            iptw0 = (void *) &sectors[pos_wpath0];

            set721((void *) iptw1->name_len, 1);
            set732((void *) iptw1->extent, directory_iso[n].wlba);
            set722((void *) iptw1->parent, directory_iso[n].parent);
            iptw1->name[0] = 0;
            pos_wpath1 += sizeof(struct iso_path_table) - 1 + 1;
            pos_wpath1 += pos_wpath1 & 1;
            iptw1 = (void *) &sectors[pos_wpath1];
            continue;
            
        }

        //////
        UTF8_to_UTF16((u8 *) directory_iso[n].name, wstring);

        int len_string;

        for(len_string = 0; len_string < 512; len_string++) if(wstring[len_string] == 0) break;


        set721((void *) iptl0->name_len, strlen(directory_iso[n].name));
        set731((void *) iptl0->extent, directory_iso[n].llba);
        set721((void *) iptl0->parent, directory_iso[n].parent);
        memcapscpy(&iptl0->name[0], directory_iso[n].name, strlen(directory_iso[n].name));
        pos_lpath0 += sizeof(struct iso_path_table) - 1 + strlen(directory_iso[n].name);
        pos_lpath0 += pos_lpath0 & 1;
        iptl0 = (void *) &sectors[pos_lpath0];

        set721((void *) iptl1->name_len, strlen(directory_iso[n].name));
        set732((void *) iptl1->extent, directory_iso[n].llba);
        set722((void *) iptl1->parent, directory_iso[n].parent);
        memcapscpy(&iptl1->name[0], directory_iso[n].name, strlen(directory_iso[n].name));
        pos_lpath1 += sizeof(struct iso_path_table) - 1 + strlen(directory_iso[n].name);
        pos_lpath1 += pos_lpath1 & 1;
        iptl1 = (void *) &sectors[pos_lpath1];

        set721((void *) iptw0->name_len, len_string * 2);
        set731((void *) iptw0->extent, directory_iso[n].wlba);
        set721((void *) iptw0->parent, directory_iso[n].parent);
        memcpy(&iptw0->name[0], wstring, len_string * 2);
        pos_wpath0 += sizeof(struct iso_path_table) - 1 + len_string * 2;
        pos_wpath0 += pos_wpath0 & 1;
        iptw0 = (void *) &sectors[pos_wpath0];

        set721((void *) iptw1->name_len, len_string * 2);
        set732((void *) iptw1->extent, directory_iso[n].wlba);
        set722((void *) iptw1->parent, directory_iso[n].parent);
        memcpy(&iptw1->name[0], wstring, len_string * 2);
        pos_wpath1 += sizeof(struct iso_path_table) - 1 + len_string * 2;
        pos_wpath1 += pos_wpath1 & 1;
        iptw1 = (void *) &sectors[pos_wpath1];     

        //////

    }
    
    return 0;
}


static int fill_entries(char *path1, char *path2, int level)
{
    DIR  *dir;

    int n;
    int len_string;

    int len1 = strlen(path1);
    int len2 = strlen(path2);

    struct iso_directory_record *idrl = (void *) &sectors[directory_iso[level].llba * 2048];
    struct iso_directory_record *idrw = (void *) &sectors[directory_iso[level].wlba * 2048];
    struct iso_directory_record *idrl0 = idrl;
    struct iso_directory_record *idrw0 = idrw;

    struct tm *tm;
    struct stat s;

    memset((void *) idrl, 0, 2048);
    memset((void *) idrw, 0, 2048);

    u32 count_sec1 = 1, count_sec2 = 1, max_sec1, max_sec2;

    int first_file = 1;
    
    int aux_parent = directory_iso[level].parent - 1;

    if(level!=0) {
        strcat(path2, "/");
        strcat(path2, directory_iso[level].name);
        strcat(path1, path2);
    } else {path2[0] = 0; fill_dirpath();}


    //DPrintf("q %s LBA %X\n", path2, directory_iso[level].llba);

    if(stat(path1, &s)<0) {return -669;}

    tm = localtime(&s.st_mtime);
    dd = tm->tm_mday;
    mm = tm->tm_mon + 1;
    aa = tm->tm_year + 1900;
    ho = tm->tm_hour;
    mi = tm->tm_min;
    se = tm->tm_sec;

    idrl->length[0] = sizeof(struct iso_directory_record) + 6;
    idrl->length[0] += idrl->length[0] & 1;
    idrl->ext_attr_length[0] = 0;
    set733((void *) idrl->extent, directory_iso[level].llba);
    set733((void *) idrl->size, directory_iso[level].ldir * 2048);
    setdaterec(idrl->date, dd, mm, aa, ho, mi, se);
    idrl->flags[0] = 0x2;
    idrl->file_unit_size[0] = 0x0;
    idrl->interleave[0] = 0x0;
    set723(idrl->volume_sequence_number, 1);
    idrl->name_len[0] = 1;
    idrl->name[0] = 0;
    idrl = (void *) ((char *) idrl) + idrl->length[0];

    max_sec1 = directory_iso[level].ldir;

    idrw->length[0] = sizeof(struct iso_directory_record) + 6;
    idrw->length[0] += idrw->length[0] & 1;
    idrw->ext_attr_length[0] = 0;
    set733((void *) idrw->extent, directory_iso[level].wlba);
    set733((void *) idrw->size, directory_iso[level].wdir * 2048);
    setdaterec(idrw->date, dd, mm, aa, ho, mi, se);
    idrw->flags[0] = 0x2;
    idrw->file_unit_size[0] = 0x0;
    idrw->interleave[0] = 0x0;
    set723(idrw->volume_sequence_number, 1);
    idrw->name_len[0] = 1;
    idrw->name[0] = 0;
    idrw = (void *) ((char *) idrw) + idrw->length[0];

    max_sec2 = directory_iso[level].wdir;

    if(level) {
        int len = strlen(path1);
        strcat(path1,"/..");
        if(stat(path1, &s)<0) {return -669;}
        path1[len] = 0;

        tm = localtime(&s.st_mtime);
        dd = tm->tm_mday;
        mm = tm->tm_mon + 1;
        aa = tm->tm_year + 1900;
        ho = tm->tm_hour;
        mi = tm->tm_min;
        se = tm->tm_sec;
    }

    idrl->length[0] = sizeof(struct iso_directory_record) + 6;
    idrl->length[0] += idrl->length[0] & 1;
    idrl->ext_attr_length[0] = 0;
    set733((void *) idrl->extent, directory_iso[!level ? 0 : aux_parent].llba);
    set733((void *) idrl->size, directory_iso[!level ? 0 : aux_parent].ldir * 2048);
    setdaterec(idrl->date, dd, mm, aa, ho, mi, se);
    idrl->flags[0] = 0x2;
    idrl->file_unit_size[0] = 0x0;
    idrl->interleave[0] = 0x0;
    set723(idrl->volume_sequence_number, 1);
    idrl->name_len[0] = 1;
    idrl->name[0] = 1;
    idrl = (void *) ((char *) idrl) + idrl->length[0];

    idrw->length[0] = sizeof(struct iso_directory_record) + 6;
    idrw->length[0] += idrw->length[0] & 1;
    idrw->ext_attr_length[0] = 0;
    set733((void *) idrw->extent, directory_iso[!level ? 0 : aux_parent].wlba);
    set733((void *) idrw->size, directory_iso[!level ? 0 : aux_parent].wdir * 2048);
    setdaterec(idrw->date, dd, mm, aa, ho, mi, se);
    idrw->flags[0] = 0x2;
    idrw->file_unit_size[0] = 0x0;
    idrw->interleave[0] = 0x0;
    set723(idrw->volume_sequence_number, 1);
    idrw->name_len[0] = 1;
    idrw->name[0] = 1;
    idrw = (void *) ((char *) idrw) + idrw->length[0];

    // files
    dir = opendir (path1);
    if(dir) {
        while(1) {
        
            struct dirent *entry = readdir (dir);
                
            if(!entry) break;
            if(entry->d_name[0]=='.' && (entry->d_name[1]=='.' || entry->d_name[1]== 0)) continue;
            
            int len = strlen(path1);

            #ifdef NOPS3_UPDATE
            if(!strcmp(&path1[len - 10], "PS3_UPDATE")) continue;
            #endif

            strcat(path1,"/");
            strcat(path1, entry->d_name);

            if(stat(path1, &s)<0) {closedir(dir); return -669;}

            path1[len] = 0;

            if(S_ISDIR(s.st_mode)) continue;

            int lname = strlen(entry->d_name);

            if(lname >=6 && !strcmp(&entry->d_name[lname -6], ".66600")) { // build size of .666xx files
                u64 size = s.st_size;
                lname -= 6;
                int n;

                memcpy(temp_string, entry->d_name, lname);
                temp_string[lname] = 0;

                if(lname > 222) {closedir(dir); return -555;}

                UTF8_to_UTF16((u8 *) temp_string, wstring);

                for(len_string = 0; len_string < 512; len_string++) if(wstring[len_string] == 0) break;

                if(len_string > 222) {closedir(dir); return -555;}

                path1[len] = 0;

                for(n = 1; n < 100; n++) {

                    int len2 = strlen(path1);
                    strcat(path1,"/");

                    int l = strlen(path1);

                    memcpy(path1 + l, entry->d_name, lname);
                    
                    sprintf(&path1[l + lname], ".666%2.2u", n);

                    if(stat(path1, &s)<0) {s.st_size = size; path1[len2] = 0; break;}
                    
                    path1[len2] = 0;
                    
                    size += s.st_size;     
                    
                }

                path1[len] = 0;

            
            } else
                if(lname >=6 && !strncmp(&entry->d_name[lname -6], ".666", 4)) continue; // ignore .666xx files
                else {
                
                    if(strlen(entry->d_name) > 222) {closedir(dir); return -555;}

                    UTF8_to_UTF16((u8 *) entry->d_name, wstring);

                    for(len_string = 0; len_string < 512; len_string++) if(wstring[len_string] == 0) break;

                    if(len_string > 222) {closedir(dir); return -555;}
                }


            #ifdef ALIGNED32SECTORS
            if(first_file) flba= (flba + 31) & ~31;
            #endif

            first_file = 0;

            int parts = s.st_size ? (u32) ((((u64) s.st_size) + 0xFFFFF7FFULL)/0xFFFFF800ULL) : 1;

            int n;

            tm = gmtime(&s.st_mtime);
            time_t t = mktime(tm);
            tm = localtime(&t);
            dd = tm->tm_mday;
            mm = tm->tm_mon + 1;
            aa = tm->tm_year + 1900;
            ho = tm->tm_hour;
            mi = tm->tm_min;
            se = tm->tm_sec;


            for(n = 0; n < parts; n++) {

                u32 fsize;
                if(parts > 1 && (n + 1) != parts) {fsize = 0xFFFFF800; s.st_size-= fsize;}
                else fsize = s.st_size;

                int add;

                add = sizeof(struct iso_directory_record) - 1 + (lname + 8);
                add+= (add & 1);

                // align entry data with sector

                int cldir = (((int) (s64) idrl) - ((int) (s64) idrl0)) & 2047;
                
                cldir += add;

                if(cldir > 2048) {

                    //DPrintf("gapl1 lba 0x%X %s/%s %i\n", directory_iso[level].llba, path1, entry->d_name, cldir);
                    //getchar();

                    count_sec1++;
                    if(count_sec1 > max_sec1) {
                        closedir (dir);
                        DPrintf("Error!: too much entries in directory:\n%s\n", path1);
                        return -444;
                    }

                    idrl = (void *) ((char *) idrl) + (add - (cldir - 2048));

                    memset((void *) idrl, 0, 2048);
                    
                }

                idrl->length[0] = add;
                idrl->length[0] += idrl->length[0] & 1;
                idrl->ext_attr_length[0] = 0;
                set733((void *) idrl->extent, flba);
                set733((void *) idrl->size, fsize);
                
                //DPrintf("a %i/%i/%i %i:%i:%i %s LBA: %X\n", dd, mm, aa, ho, mi, se, entry->d_name, flba);
                setdaterec(idrl->date, dd, mm, aa, ho, mi, se);
                idrl->flags[0] = ((n + 1) != parts) ? 0x80 : 0x0; // fichero
                idrl->file_unit_size[0] = 0x0;
                idrl->interleave[0] = 0x0;
                set723(idrl->volume_sequence_number, 1);
                idrl->name_len[0] = lname + 2;
                memcapscpy(idrl->name, entry->d_name, lname);
                idrl->name[lname + 0] = ';';
                idrl->name[lname + 1] = '1';
                idrl = (void *) ((char *) idrl) + idrl->length[0];
                
                //

                add = sizeof(struct iso_directory_record) - 1 + len_string * 2 + 4 + 6;
                add+= (add & 1);

                // align entry data with sector
                
                int cwdir = (((int) (s64)idrw) - ((int) (s64)idrw0)) & 2047;
                
                cwdir += add;

                if(cwdir > 2048) {

                    //DPrintf("gapw1 lba 0x%X %s/%s %i\n", directory_iso[level].wlba, path1, entry->d_name, cwdir);
                    //getchar();

                    count_sec2++;
                    if(count_sec2 > max_sec2) {
                        closedir (dir);
                        DPrintf("Error!: too much entries in directory:\n%s\n", path1);
                        return -444;
                    }

                    idrw = (void *) ((char *) idrw) + (add - (cwdir - 2048));

                    memset((void *) idrw, 0, 2048);
                    
                }


                idrw->length[0] = add;
                idrw->length[0] += idrw->length[0] & 1;
                idrw->ext_attr_length[0] = 0;
                set733((void *) idrw->extent, flba);
                set733((void *) idrw->size, fsize);

                setdaterec(idrw->date, dd, mm, aa, ho, mi, se);
                idrw->flags[0] = ((n + 1) != parts) ? 0x80 : 0x0; // fichero
                idrw->file_unit_size[0] = 0x0;
                idrw->interleave[0] = 0x0;
                set723(idrw->volume_sequence_number, 1);
                idrw->name_len[0] = len_string * 2 + 4;
                memcpy(idrw->name, wstring, len_string * 2);
                idrw->name[len_string * 2 + 0] = 0;
                idrw->name[len_string * 2 + 1] = ';';
                idrw->name[len_string * 2 + 2] = 0;
                idrw->name[len_string * 2 + 3] = '1';
                idrw = (void *) ((char *) idrw) + idrw->length[0];

                flba+= ((fsize + 2047) & ~2047) / 2048;
            }

        }

    closedir (dir);
    }

    // folders
    for(n = 1; n < cur_isop; n++)
        if(directory_iso[n].parent == level + 1) {

            int len = strlen(path1);

            strcat(path1,"/");
            strcat(path1, directory_iso[n].name);

            if(stat(path1, &s)<0) {return -669;}

            path1[len] = 0;

            tm = localtime(&s.st_mtime);
            dd = tm->tm_mday;
            mm = tm->tm_mon + 1;
            aa = tm->tm_year + 1900;
            ho = tm->tm_hour;
            mi = tm->tm_min;
            se = tm->tm_sec;

            //DPrintf("dir %i/%i/%i %i:%i:%i %s\n", dd, mm, aa, ho, mi, se, directory_iso[n].name);

            int add;

            add = sizeof(struct iso_directory_record) - 1 + (strlen(directory_iso[n].name) + 6);
            add+= (add & 1);

            // align entry data with sector
            
            int cldir = (((int)(s64)idrl) - ((int) (s64)idrl0)) & 2047;
            
            cldir += add;

            if(cldir > 2048) {

                //DPrintf("gapl0 lba 0x%X %s/%s %i\n", directory_iso[level].llba, path1, directory_iso[n].name, cldir);
                //getchar();

                count_sec1++;
                if(count_sec1 > max_sec1) {
                    closedir (dir);
                    DPrintf("Error!: too much entries in directory:\n%s\n", path1);
                    return -444;
                }

                idrl = (void *) ((char *) idrl) + (add - (cldir - 2048));

                memset((void *) idrl, 0, 2048);
                
            }

            idrl->length[0] = add;
            idrl->length[0] += idrl->length[0] & 1;
            idrl->ext_attr_length[0] = 0;
            set733((void *) idrl->extent, directory_iso[n].llba);
            set733((void *) idrl->size, directory_iso[n].ldir * 2048);
            setdaterec(idrl->date, dd, mm, aa, ho, mi, se);
            idrl->flags[0] = 0x2;
            idrl->file_unit_size[0] = 0x0;
            idrl->interleave[0] = 0x0;
            set723(idrl->volume_sequence_number, 1);
            idrl->name_len[0] = strlen(directory_iso[n].name);
            memcapscpy(idrl->name, directory_iso[n].name, strlen(directory_iso[n].name));
            idrl = (void *) ((char *) idrl) + idrl->length[0];

            //
            UTF8_to_UTF16((u8 *) directory_iso[n].name, wstring);

            for(len_string = 0; len_string < 512; len_string++) if(wstring[len_string] == 0) break;

            add = sizeof(struct iso_directory_record) - 1 + len_string * 2 + 6;
            add+= (add & 1);

            // align entry data with sector
            
            int cwdir = (((int) (s64)idrw) - ((int) (s64)idrw0)) & 2047;
            
            cwdir += add;

            if(cwdir > 2048) {

                //DPrintf("gapw0 lba 0x%X %s/%s %i\n", directory_iso[level].wlba, path1, directory_iso[n].name, cwdir);
                //getchar();

                count_sec2++;
                if(count_sec2 > max_sec2) {
                    closedir (dir);
                    DPrintf("Error!: too much entries in directory:\n%s\n", path1);
                    return -444;
                }

                idrw = (void *) ((char *) idrw) + (add - (cwdir - 2048));

                memset((void *) idrw, 0, 2048);
                
            }

            idrw->length[0] = add;
            idrw->length[0] += idrw->length[0] & 1;
            idrw->ext_attr_length[0] = 0;
            set733((void *) idrw->extent, directory_iso[n].wlba);
            set733((void *) idrw->size, directory_iso[n].wdir * 2048);
            setdaterec(idrw->date, dd, mm, aa, ho, mi, se);
            idrw->flags[0] = 0x2;
            idrw->file_unit_size[0] = 0x0;
            idrw->interleave[0] = 0x0;
            set723(idrw->volume_sequence_number, 1);
            idrw->name_len[0] = len_string * 2;
            memcpy(idrw->name, wstring, len_string * 2);
            idrw = (void *) ((char *) idrw) + idrw->length[0];

    }

    path1[len1] = 0;
    
    // iteration
    for(n = 1; n < cur_isop; n++)
        if(directory_iso[n].parent == level + 1) {
            int ret = fill_entries(path1, path2, n);
            if(ret < 0) {path2[len2] = 0; return ret;}
    }
    
    path2[len2] = 0;
   
    return 0;

}

#define SPLIT_LBA 0x1FFFE0


static int write_split0(int *fd, u32 lba, u8 *mem, int sectors, int sel)
{
    char filename[0x420];

    if(!iso_split) {

        if(ps3ntfs_write(*fd, (void *) mem, (int) sectors * 2048) != sectors * 2048) return -667;
        return 0;
    }
    
    int cur = lba / SPLIT_LBA;
    int cur2 = (lba + sectors) / SPLIT_LBA;

    if(cur == cur2 && (iso_split - 1) == cur) {
        if(ps3ntfs_write(*fd, (void *) mem, (int) sectors * 2048) != sectors * 2048) return -667;
        return 0;
    }

    u32 lba2 = lba + sectors;
    u32 pos = 0;

    for(; lba < lba2; lba++) {

        int cur = lba / SPLIT_LBA;

        if(iso_split - 1 != cur) {
            if (*fd >= 0) ps3ntfs_close(*fd); *fd = -1;

            if(sel == 0) return 0;

            if(iso_split == 1) {
                sprintf(filename, "%s.0", output_name);
                ps3ntfs_unlink(filename);
                rename(output_name, filename);
            }

            iso_split = cur + 1;

            sprintf(filename, "%s.%i", output_name, iso_split - 1);

            *fd = ps3ntfs_open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0766);
            if(*fd < 0) return -1;
            
        }
        
        if(ps3ntfs_write(*fd, (void *) mem + pos, 2048) != 2048) return -667;
        pos += 2048;

    }

    return 0;
}

#include "event_threads.h"


static volatile struct f_async {
    int flags;
    int *fd;
    void * mem;
    u32 ret;
    u32 lba;
    u32 nsectors;
    int sel;

} my_f_async;

#define ASYNC_ENABLE 128
#define ASYNC_ERROR 16

static void my_func_async(struct f_async * v)
{
    int ret = -1;
    
    if(v && v->flags & ASYNC_ENABLE) {
        v->ret = -1;
        int flags = v->flags;
        if(v->mem) {

             ret = write_split0(v->fd, v->lba, (u8 *) v->mem, v->nsectors, v->sel);
             v->ret = ret;

            free(v->mem); v->mem = 0;

        }

        if(ret) flags|= ASYNC_ERROR;

        flags &= ~ASYNC_ENABLE;

        v->flags = flags;
    }
}
static int use_async_fd = 128;

static int write_split(int *fd, u32 lba, u8 *mem, int sectors, int sel)
{
    loop_write:

    if(use_async_fd == 128) {
        use_async_fd = 1;
        my_f_async.flags = 0;
        my_f_async.sel = sel;
        my_f_async.lba = lba;
        my_f_async.nsectors = sectors;
        my_f_async.fd = fd;
        my_f_async.mem = malloc(sectors * 2048);
        if(my_f_async.mem) memcpy(my_f_async.mem, (void *) mem, sectors * 2048);        
        my_f_async.ret = -1;
        my_f_async.flags = ASYNC_ENABLE;
        event_thread_send(0x555ULL, (u64) my_func_async, (u64) &my_f_async);

    } else {
     
        if(!(my_f_async.flags & ASYNC_ENABLE)) {

            if(my_f_async.flags & ASYNC_ERROR) {
                return my_f_async.ret;
            }
           
            my_f_async.flags = 0;
            my_f_async.sel = sel;
            my_f_async.lba = lba;
            my_f_async.nsectors = sectors;
            my_f_async.fd = fd;
            my_f_async.mem = malloc(sectors * 2048);
            if(my_f_async.mem) memcpy(my_f_async.mem, (void *) mem, sectors * 2048);
            my_f_async.ret = -1;
            my_f_async.flags = ASYNC_ENABLE;
            event_thread_send(0x555ULL, (u64) my_func_async, (u64) &my_f_async);
            
        } else {
            //wait_event_thread();
            goto loop_write;
        }
    }

    return 0;
}

static int build_file_iso(int *fd, char *path1, char *path2, int level)
{
    DIR  *dir;
    struct stat s;

    int n;
    int first_file = 1;
    int len1 = strlen(path1);
    int len2 = strlen(path2);

    sprintf(dbhead, "%s - done (%i/100)", "MAKEPS3ISO Utility", flba * 100 / toc);

    DbgHeader(dbhead);

    if(level!=0) {
        strcat(path2, "/");
        strcat(path2, directory_iso[level].name);
        strcat(path1, path2);
    } else path2[0] = 0;

    if(level)
        DPrintf("</%s>\n", directory_iso[level].name);
    else
        DPrintf("</>\n");

    //DPrintf("q %s LBA %X\n", path2, directory_iso[level].llba);

    // files
    dir = opendir (path1);
    if(dir) {
        while(1) {
        
            struct dirent *entry=readdir (dir);
                
            if(!entry) break;
            if(entry->d_name[0]=='.' && (entry->d_name[1]=='.' || entry->d_name[1]== 0)) continue;
            
            int len = strlen(path1);

            #ifdef NOPS3_UPDATE
            if(!strcmp(&path1[len - 10], "PS3_UPDATE")) continue;
            #endif

            strcat(path1,"/");
            strcat(path1, entry->d_name);

            if(stat(path1, &s)<0) {path1[len] = 0;closedir(dir); return -669;}

            if(S_ISDIR(s.st_mode)) {path1[len] = 0; continue;}

            int is_file_split = 0;

            int lname = strlen(entry->d_name);

            if(lname >=6 && !strcmp(&entry->d_name[lname -6], ".66600")) { // build size of .666xx files
                u64 size = s.st_size;
                lname -= 6;
                int n;

                is_file_split = 1;

                memcpy(temp_string, entry->d_name, lname);
                temp_string[lname] = 0;

                if(lname > 222) {path1[len] = 0;closedir(dir); return -555;}

                path1[len] = 0;

                for(n = 1; n < 100; n++) {

                    int len2 = strlen(path1);
                    strcat(path1,"/");

                    int l = strlen(path1);

                    memcpy(path1 + l, entry->d_name, lname);
                    
                    sprintf(&path1[l + lname], ".666%2.2u", n);

                    if(stat(path1, &s)<0) {s.st_size = size; path1[len2] = 0; break;}
                    
                    path1[len2] = 0;
                    
                    size += s.st_size;     
                    
                }

                path1[len] = 0;
                strcat(path1,"/");
                strcat(path1, entry->d_name); // restore .66600 file

            
            } else
                if(lname >=6 && !strncmp(&entry->d_name[lname -6], ".666", 4)) {path1[len] = 0;continue;} // ignore .666xx files
                else {
                
                    if(strlen(entry->d_name) > 222) {path1[len] = 0; closedir(dir); return -555;}
   
                }

          
            int  fd2 = ps3ntfs_open(path1, O_RDONLY, 0766);
            path1[len] = 0;

            if(fd2 < 0) {closedir(dir); return -666;}
            
            u32 flba0 = flba;

            #ifdef ALIGNED32SECTORS
            if(first_file) flba= (flba + 31) & ~31;
            #endif

            first_file = 0;

            if(flba0 != flba) {
                //DPrintf("gap: %i\n", (flba - flba0));
                
                int f = (flba - flba0);
                int f2 = 0;
                int z = 0;
                
                memset(sectors, 0, ((f > 128) ? 128 : f ) * 2048);
                
                while(f > 0) { 
                    if(f > 128) f2 = 128; else f2 = f;
                  
                    int ret = write_split(fd, flba + z, sectors, f2, 1);
                    if(ret < 0) {
                        ps3ntfs_close(fd2); closedir(dir); return ret;

                    } 

                    f -= f2;
                    z += f2;
                }
            }

            sprintf(dbhead, "%s - done (%i/100)", "MAKEPS3ISO Utility", flba * 100 / toc);
            DbgHeader(dbhead);

            if(is_file_split) {
                if(s.st_size < 1024ULL) 
                    DPrintf("  -> %s LBA %u size %u Bytes\n", temp_string, flba, (u32) s.st_size);
                else if(s.st_size < 0x100000LL) 
                    DPrintf("  -> %s LBA %u size %u KB\n", temp_string, flba, (u32) (s.st_size/1024));
                else
                    DPrintf("  -> %s LBA %u size %u MB\n", temp_string, flba, (u32) (s.st_size/0x100000LL));
            } else {
                if(s.st_size < 1024ULL) 
                    DPrintf("  -> %s LBA %u size %u Bytes\n", entry->d_name, flba, (u32) s.st_size);
                else if(s.st_size < 0x100000LL) 
                    DPrintf("  -> %s LBA %u size %u KB\n", entry->d_name, flba, (u32) (s.st_size/1024));
                else
                    DPrintf("  -> %s LBA %u size %u MB\n", entry->d_name, flba, (u32) (s.st_size/0x100000LL));
            }

            u32 count = 0, percent = (u32) (s.st_size / 0x40000ULL);
            if(percent == 0) percent = 1;

            int cx = con_x, cy = con_y;

            u64 t_one, t_two;

            t_one = get_ticks();

            if(get_input_abort()) {
                ps3ntfs_close(fd2); closedir(dir); return -999;
            }

            while(s.st_size > 0) {
                u32 fsize;
                u32 lsize;

                con_x = cx; con_y = cy;

                t_two = get_ticks();

                if(((t_two - t_one) >= TICKS_PER_SEC/2)) {
                    t_one = t_two;
                    DPrintf("*** Writing... %u %%", count * 100 / percent);
                    con_x = cx; con_y = cy;

                    if(get_input_abort()) {
                        ps3ntfs_close(fd2); closedir(dir); return -999;
                    }
                }
                
                if(s.st_size > 0x40000) fsize = 0x40000;
                else fsize = (u32) s.st_size;

                count++;
                
                if(fsize < 0x40000) memset(sectors, 0, 0x40000);

                if(is_file_split) {
                    int read = ps3ntfs_read(fd2, (void *) sectors, (int) fsize);
                    if(read < 0) {
                        ps3ntfs_close(fd2); closedir(dir); return -668;
                    } else if(read < fsize) {
                        ps3ntfs_close(fd2);
                        path1[len] = 0;
                        strcat(path1,"/");

                        int l = strlen(path1);

                        memcpy(path1 + l, entry->d_name, lname);
                        
                        sprintf(&path1[l + lname], ".666%2.2u", is_file_split);
                        //DPrintf("split: %s\n", path1);
                        is_file_split++;

                        fd2 = ps3ntfs_open(path1, O_RDONLY, 0766);
                        path1[len] = 0;

                        if(fd2 < 0) {closedir(dir); return -666;}

                        if(ps3ntfs_read(fd2, (void *) (sectors + read), (int) (fsize - read)) != (fsize - read)) {
                            ps3ntfs_close(fd2); closedir(dir); return -668;
                        }
                        
                    } 

                } else

                if(ps3ntfs_read(fd2, (void *) sectors, (int) fsize) != fsize) {
                    ps3ntfs_close(fd2); closedir(dir); return -668;
                }

                lsize = (fsize + 2047) & ~2047;

                int ret = write_split(fd, flba, sectors, lsize/2048, 1);

                if(ret < 0) {
                    DPrintf("\n");
                    ps3ntfs_close(fd2); closedir(dir); return ret;

                }

                flba += lsize/2048;
                //DPrintf("flba %i\n", flba * 2048);

                s.st_size-= fsize;
            }

            con_x = cx; con_y = cy;
            DPrintf("                             ");
            con_x = cx; con_y = cy;

            ps3ntfs_close(fd2);

        }

    closedir (dir);
    }

    path1[len1] = 0;
    
    // iteration
    for(n = 1; n < cur_isop; n++)
        if(directory_iso[n].parent == level + 1) {
            int ret = build_file_iso(fd, path1, path2, n);
            if(ret < 0) {path2[len2] = 0; return ret;}
    }
    
    path2[len2] = 0;

    if(level == 0) {

        int ret;

        if(toc != flba) {
            //DPrintf("End gap: %i\n", (toc - flba));
            
            int f = (toc - flba);
            int f2 = 0;
            int z = 0;

            memset(sectors, 0, ((f > 128) ? 128 : f ) * 2048);
            
            while(f > 0) { 
                if(f > 128) f2 = 128; else f2 = f;

                ret = write_split(fd, flba + z, sectors, f2, 1);

                if(ret < 0) {
                   return ret;

                }

                f-= f2;
                z+= f2;
            }
        }

        ret = 0;

        if(my_f_async.flags & ASYNC_ENABLE){
            wait_event_thread();
            if(my_f_async.flags  & ASYNC_ERROR) {
                
                ret = my_f_async.ret;
            }
            my_f_async.flags = 0;
        }

        event_thread_send(0x555ULL, (u64) 0, 0);

        sprintf(dbhead, "%s - done (%i/100)", "MAKEPS3ISO Utility", 100);

        DbgHeader(dbhead);

        return ret;

    }
    
    
    return 0;

}

static void fixpath(char *p)
{
    u8 * pp = (u8 *) p;

    if(*p == '"') {
        p[strlen(p) -1] = 0;
        memcpy(p, p + 1, strlen(p));
    }

    #ifdef __CYGWIN__
    if(p[0]!=0 && p[1] == ':') {
        p[1] = p[0];
        memmove(p + 9, p, strlen(p) + 1);
        memcpy(p, "/cygdrive/", 10);
    }
    #endif

    while(*pp) {
        if(*pp == '"') {*pp = 0; break;}
        else
        if(*pp == '\\') *pp = '/';
        else
        if(*pp > 0 && *pp < 32) {*pp = 0; break;}
        pp++;
    }

}

static void fixtitle(char *p)
{
    while(*p) {
        if(*p & 128) *p = 0;
        else
        if(*p == ':' || *p == '?' || *p == '"' || *p == '<' || *p == '>' || *p == '|') *p = '_';
        else
        if(*p == '\\' || *p == '/') *p = '-';
        else
        if(((u8)*p) > 0 && ((u8)*p) < 32) {*p = 0; break;}
        p++;
    }
}

int makeps3iso(char *g_path, char *f_iso, int split)
{

    struct stat s;
    char path1[0x420];
    char path2[0x420];
    char title_id[64];
    u64 t_start, t_finish;

    int ask_del = 0;

    use_async_fd = 128;
    my_f_async.flags = 0;

    initConsole();

    sprintf(dbhead, "%s - done (0/100)", "MAKEPS3ISO Utility");

    DbgHeader(dbhead);

    DPrintf("\nMAKEPS3ISO (c) 2013, Estwald (Hermes)\n\n");

    strcpy(path1, g_path);
    
    // libc test
    if(sizeof(s.st_size) != 8) {

        DPrintf("Error!: stat st_size must be a 64 bit number!  (size %i)\n\nPress TRIANGLE button to exit\n\n", (int) sizeof(s.st_size));
        get_input_char();
        return -1;
    }


    fixpath(path1);

    if(stat(path1, &s)<0 || !(S_ISDIR(s.st_mode))) {DPrintf("Invalid Path Folder!\n\nPress TRIANGLE button to exit\n"); get_input_char();return -1;}

    strcpy(path2, path1);
    strcat(path2, "/PS3_GAME/PARAM.SFO");

    if(iso_parse_param_sfo(path2, title_id, output_name)<0) {
        DPrintf("Error!: PARAM.SFO not found!\n");
        DPrintf("\nPress TRIANGLE button to exit\n");
        get_input_char();
        return -1;
    } else {
        utf8_to_ansiname(output_name, path2, 32);
        path2[32]= 0;
        fixtitle(path2);
        strcat(path2, "-");
        strcat(path2, title_id);
    }

    if(f_iso) strcpy(output_name, f_iso); else output_name[0] = 0;

  
    fixpath(output_name);

    // create path for get free space from destination file
    char dest_path[0x420];

    strcpy(dest_path, output_name);

    u64 avail = get_disk_free_space(dest_path);

    int nlen = strlen(output_name);

    if(nlen < 1) {strcpy(output_name, path2);/*DPrintf("ISO name too short!\n\nPress TRIANGLE button to exit\n"); get_input_char();return -1;*/}
    else {

         if(stat(output_name, &s) == 0 && (S_ISDIR(s.st_mode))) {
            strcat(output_name, "/"); strcat(output_name, path2);
         }
        
    }

    nlen = strlen(output_name);

    if(nlen < 4 || (strcmp(&output_name[nlen - 4], ".iso") && strcmp(&output_name[nlen - 4], ".ISO"))) {
        strcat(output_name, ".iso");
    }

    if(!stat(output_name, &s)) {
        DPrintf("\nFile Exits. Overwrite? (CROSS - Yes/ TRIANGLE - No):\n");
    
        if(get_input_char()!='y') return -1;
    }

    strcpy(output_name2, output_name);

    
    if(split == 2) {
        
        DPrintf("\nSplit in 4GB parts? (CROSS - Yes/ TRIANGLE - No):\n");
    
        iso_split = get_input_char();

        if(iso_split == 'y' || iso_split == 'Y') iso_split = 1; else iso_split = 0;

    } else
        iso_split = (split!=0);

    if(iso_split) DPrintf("Using Split File Mode\n"); else DPrintf("Using Complete File Mode\n");

    DPrintf("\n");

    t_start = get_ticks();
  
    cur_isop = 0;

    directory_iso = malloc((MAX_ISO_PATHS + 1) * sizeof(_directory_iso));

    if(!directory_iso) {
        DPrintf("Out of Memory (directory_iso mem)\n");
        DPrintf("\nPress TRIANGLE button to exit\n");
        get_input_char();
        return -1;
    }

    memset(directory_iso, 0, (MAX_ISO_PATHS + 1) * sizeof(_directory_iso));

    lpath = sizeof(struct iso_path_table);
    wpath = sizeof(struct iso_path_table);

    llba0 = 0;
    llba1 = 0;
    wlba0 = 0;
    wlba1 = 0;

    dllba = 0; // dir entries
    dwlba = 0; // dir entriesw
    dlsz = 0; // dir entries size (sectors)
    dwsz = 0; // dir entriesw size (sectors)
    flba = 0; // first lba for files

    u32 flba2 = 0;

    int ret;

    ret = calc_entries(path1, 1);
    if(ret < 0 ) {
        switch(ret) {
            case -1:
                DPrintf("Out of Memory (calc_entries())\n");
                goto err;
            case -444:
                DPrintf("Too much folders (max %i) (calc_entries())\n", MAX_ISO_PATHS);
                goto err;
            case -555:
                DPrintf("Folder Name Too Long (calc_entries())\n");
                goto err;
            case -669:
                DPrintf("Error Input File Not Exists (calc_entries())\n");
            case -666:
                DPrintf("Error Opening Input File (calc_entries())\n");
                goto err;
            case -667:
                DPrintf("Error Writing Output File (calc_entries())\n");
                goto err;
            case -668:
                DPrintf("Error Reading Input File (calc_entries())\n");
                goto err;
            case -777:
                DPrintf("Error Creating Split file (calc_entries())\n");
                goto err;

        }
    }

    /*
    DPrintf("llba0 0x%X - offset: 0x%X\n", llba0, llba0 * 2048);
    DPrintf("llba1 0x%X - offset: 0x%X\n", llba1, llba1 * 2048);
    DPrintf("wlba0 0x%X - offset: 0x%X\n", wlba0, wlba0 * 2048);
    DPrintf("wlba1 0x%X - offset: 0x%X\n", wlba1, wlba1 * 2048);

    DPrintf("\ndllba0 0x%X - offset: 0x%X, size 0x%X\n", dllba, dllba * 2048, dlsz * 2048);
    DPrintf("dwlba0 0x%X - offset: 0x%X, size 0x%X\n", dwlba, dwlba * 2048, dwsz * 2048);
    DPrintf("flba0 0x%X - offset: 0x%X\n", flba, flba * 2048);
    */

    flba2 = flba;

    sectors = malloc((flba > 128) ? flba * 2048 + 2048 : 128 * 2048 + 2048);

    if(!sectors) {
        DPrintf("Out of Memory (sectors mem)\n");
        DPrintf("\nPress TRIANGLE button to exit\n");
        get_input_char();
        goto err;
    }

    memset(sectors, 0, flba * 2048);

    pos_lpath0 = llba0 * 2048;
    pos_lpath1 = llba1 * 2048;
    pos_wpath0 = wlba0 * 2048;
    pos_wpath1 = wlba1 * 2048;

    path2[0] = 0;
    ret = fill_entries(path1, path2, 0);

    if(ret < 0 ) {
        switch(ret) {
            case -1:
                DPrintf("Out of Memory (fill_entries())\n");
                goto err;
            case -555:
                DPrintf("File Name Too Long (fill_entries())\n");
                goto err;
            case -669:
                DPrintf("Error Input File Not Exists (fill_entries())\n");
            case -666:
                DPrintf("Error Opening Input File (fill_entries())\n");
                goto err;
            case -667:
                DPrintf("Error Writing Output File (fill_entries())\n");
                goto err;
            case -668:
                DPrintf("Error Reading Input File (fill_entries())\n");
                goto err;
            case -777:
                DPrintf("Error Creating Split file (fill_entries())\n");
                goto err;

        }
    }
   
    #ifdef ALIGNED32SECTORS
    flba= (flba + 31) & ~31;
    #endif
    toc = flba;

    if((((u64)toc) * 2048ULL) > (avail - 0x100000ULL)) {
        DPrintf("Error!: Insufficient Disk Space in Destination\n");
        goto err;
    }

    sectors[0x3] = 1; // one range
    set732((void *) &sectors[0x8], 0); // first unencrypted sector
    set732((void *) &sectors[0xC], toc - 1); // last unencrypted sector

    strcpy((void *) &sectors[0x800], "PlayStation3");

    memset((void *) &sectors[0x810], 32, 0x20);
    memcpy((void *) &sectors[0x810], title_id, 10);

    get_rand(&sectors[0x840], 0x1B0);
    get_rand(&sectors[0x9F0], 0x10);


    struct iso_primary_descriptor *isd = (void  *) &sectors[0x8000];
    struct iso_directory_record * idr;

    isd->type[0]=1;
    memcpy(&isd->id[0],"CD001",5);
    isd->version[0]=1;
    
    memset(&isd->system_id[0],32, 32);
    memcpy(&isd->volume_id[0],"PS3VOLUME                       ",32);

    set733((void *) &isd->volume_space_size[0], toc);
    set723(&isd->volume_set_size[0],1);
    set723(&isd->volume_sequence_number[0],1);
    set723(&isd->logical_block_size[0],2048);

    set733((void *) &isd->path_table_size[0], lpath);
    set731((void *) &isd->type_l_path_table[0], llba0); // lba
    set731((void *) &isd->opt_type_l_path_table[0], 0); //lba
    set732((void *) &isd->type_m_path_table[0], llba1); //lba
    set732((void *) &isd->opt_type_m_path_table[0], 0);//lba

    idr = (struct iso_directory_record *) &isd->root_directory_record;
    idr->length[0]=34;
    idr->ext_attr_length[0]=0;
    set733((void *) &idr->extent[0], directory_iso[0].llba); //lba
    set733((void *) &idr->size[0], directory_iso[0].ldir * 2048); // tamaño
    //setdaterec(&idr->date[0],dd,mm,aa,ho,mi,se);
    struct iso_directory_record * aisdr = (void *) &sectors[directory_iso[0].llba * 2048];
    memcpy(idr->date, aisdr->date, 7);

    idr->flags[0]=2;
    idr->file_unit_size[0]=0;
    idr->interleave[0]=0;
    set723(&idr->volume_sequence_number[0],1); //lba
    idr->name_len[0]=1;
    idr->name[0]=0;

    memset(&isd->volume_set_id[0],32,128);
    memcpy(&isd->volume_set_id[0],"PS3VOLUME", 9);
    memset(&isd->publisher_id[0],32,128);
    memset(&isd->preparer_id[0],32,128);
    memset(&isd->application_id[0],32,128);
    memset(&isd->copyright_file_id[0],32,37);
    memset(&isd->abstract_file_id[0],32,37);
    memset(&isd->bibliographic_file_id,32,37);

    unsigned dd1,mm1,aa1,ho1,mi1,se1;

    time_t t;
    struct tm *tm;

    time(&t);

    tm = localtime(&t);
    dd = tm->tm_mday;
    mm = tm->tm_mon + 1;
    aa = tm->tm_year + 1900;
    ho = tm->tm_hour;
    mi = tm->tm_min;
    se = tm->tm_sec;

    dd1=dd;mm1=mm;aa1=aa;ho1=ho;mi1=mi;se1=se;
    if(se1>59) {se1=0;mi1++;}
    if(mi1>59) {mi1=0;ho1++;}
    if(ho1>23) {ho1=0;dd1++;}
    char fecha[64];
    sprintf(fecha,"%4.4u%2.2u%2.2u%2.2u%2.2u%2.2u00",aa1,mm1,dd1,ho1,mi1,se1);

    memcpy(&isd->creation_date[0], fecha,16);
    memcpy(&isd->modification_date[0],"0000000000000000",16);
    memcpy(&isd->expiration_date[0],"0000000000000000",16);
    memcpy(&isd->effective_date[0],"0000000000000000",16);
    isd->file_structure_version[0]=1;

    int len_string;

    isd = (void  *) &sectors[0x8800];

    isd->type[0] = 2;
    memcpy(&isd->id[0],"CD001",5);
    isd->version[0]=1;
    UTF8_to_UTF16((u8 *) "PS3VOLUME", wstring);
    for(len_string = 0; len_string < 512; len_string++) if(wstring[len_string] == 0) break;
    
    
    memset(&isd->system_id[0],0, 32);
    memset(&isd->volume_id[0],0, 32);
    memcpy(&isd->volume_id[0], wstring, len_string * 2);

    set733((void *) &isd->volume_space_size[0],toc);
    set723(&isd->volume_set_size[0],1);
    isd->unused3[0] = 0x25;
    isd->unused3[1] = 0x2f;
    isd->unused3[2] = 0x40;
    set723(&isd->volume_sequence_number[0],1);
    set723(&isd->logical_block_size[0],2048);

    set733((void *) &isd->path_table_size[0], wpath);
    set731((void *) &isd->type_l_path_table[0], wlba0); // lba
    set731((void *) &isd->opt_type_l_path_table[0], 0); //lba
    set732((void *) &isd->type_m_path_table[0], wlba1); //lba
    set732((void *) &isd->opt_type_m_path_table[0], 0);//lba

    idr = (struct iso_directory_record *) &isd->root_directory_record;
    idr->length[0]=34;
    idr->ext_attr_length[0]=0;
    set733((void *) &idr->extent[0], directory_iso[0].wlba); //lba
    set733((void *) &idr->size[0], directory_iso[0].wdir * 2048); // tamaño
    //setdaterec(&idr->date[0],dd,mm,aa,ho,mi,se);
    aisdr = (void *) &sectors[directory_iso[0].wlba * 2048];
    memcpy(idr->date, aisdr->date, 7);

    idr->flags[0]=2;
    idr->file_unit_size[0]=0;
    idr->interleave[0]=0;
    set723(&idr->volume_sequence_number[0],1); //lba
    idr->name_len[0]=1;
    idr->name[0]=0;

    memset(&isd->volume_set_id[0],0,128);
    memcpy(&isd->volume_set_id[0], wstring, len_string * 2);
    memset(&isd->publisher_id[0],0,128);
    memset(&isd->preparer_id[0],0,128);
    memset(&isd->application_id[0],0,128);
    memset(&isd->copyright_file_id[0],0,37);
    memset(&isd->abstract_file_id[0],0,37);
    memset(&isd->bibliographic_file_id,0,37);
    memcpy(&isd->creation_date[0], fecha,16);
    memcpy(&isd->modification_date[0],"0000000000000000",16);
    memcpy(&isd->expiration_date[0],"0000000000000000",16);
    memcpy(&isd->effective_date[0],"0000000000000000",16);
    isd->file_structure_version[0]=1;

    isd = (void  *) &sectors[0x9000];
    isd->type[0] = 255;
    memcpy(&isd->id[0],"CD001",5);
    

    int fd2 = ps3ntfs_open(output_name, O_WRONLY | O_CREAT | O_TRUNC, 0766);

    if(fd2 >= 0) {

        ask_del = 1;

        ps3ntfs_write(fd2, (void *) sectors, (int) flba2 * 2048);
        flba = flba2;
        int ret = build_file_iso(&fd2, path1, path2, 0);

        if(my_f_async.flags & ASYNC_ENABLE){
            wait_event_thread();
            if(my_f_async.flags  & ASYNC_ERROR) {
                
                if(ret == 0) ret = my_f_async.ret;
            }
            my_f_async.flags = 0;
        }

        event_thread_send(0x555ULL, (u64) 0, 0);

        if(/*iso_split < 2 && */ret!= -777) { 
            if(fd2 >= 0) ps3ntfs_close(fd2); fd2 = -1;
        }

        if(ret < 0 ) {
        switch(ret) {
            case -1:
                DPrintf("Out of Memory (build_file_iso())\n");
                goto err;
            case -555:
                DPrintf("File Name Too Long (build_file_iso())\n");
                goto err;
            case -669:
                DPrintf("Error Input File Not Exists (build_file_iso())\n");
            case -666:
                DPrintf("Error Opening Input File (build_file_iso())\n");
                goto err;
            case -667:
                DPrintf("Error Writing Output File (build_file_iso())\n");
                goto err;
            case -668:
                DPrintf("Error Reading Input File (build_file_iso())\n");
                goto err;
            case -777:
                DPrintf("Error Creating Split file (build_file_iso())\n");
                goto err;
            case -999:
                DPrintf("\nError!: Aborted by User\n\n");
                goto err;

        }

    }
    } else {
        DPrintf("Error Creating ISO file %s\n", output_name);
        goto err;
    }
    
    int n;
    for(n = 0; n < cur_isop; n++)
        if(directory_iso[n].name) {free(directory_iso[n].name); directory_iso[n].name = NULL;}
    
    free(directory_iso); directory_iso = NULL;
    free(sectors);  sectors = NULL;

    t_finish = get_ticks();    

    DPrintf("Finish!\n\nISO TOC: %i\n\n", toc);
    DPrintf("Total Time (HH:MM:SS): %2.2u:%2.2u:%2.2u.%u\n\n", (u32) (t_finish - t_start)/(TICKS_PER_SEC * 3600),
        (u32) ((t_finish - t_start)/(TICKS_PER_SEC * 60)) % 60, (u32) ((t_finish - t_start)/(TICKS_PER_SEC)) % 60,
        (u32) ((t_finish - t_start)/(TICKS_PER_SEC/100)) % 100);

    
    DPrintf("\nPress TRIANGLE button to exit\n");
    get_input_char();
    

    return 0;

err:
    if(directory_iso) {
        int n;

        for(n = 0; n < cur_isop; n++)
            if(directory_iso[n].name) {free(directory_iso[n].name); directory_iso[n].name = NULL;}

        free(directory_iso); directory_iso = NULL;
    }

    if(sectors) free(sectors); sectors = NULL;

    if(ask_del) {

        DPrintf("\nDelete partial ISO?  (CROSS - Yes/ TRIANGLE - No):\n");
        if(get_input_char()=='y') {
            int n;
            DPrintf("Yes\n");
            ps3ntfs_unlink(output_name2);
            
            for(n = 0; n < 64; n++) {
                sprintf(output_name, "%s.%i", output_name2, n);

                if(!ps3ntfs_stat(output_name, &s))
                    ps3ntfs_unlink(output_name);
                else 
                    break;
            }

        } else DPrintf("No\n");

    }

    DPrintf("\nPress TRIANGLE button to exit\n");
    get_input_char();

    return -1;
}

/*************************************************************************************/

static void get_iso_path(char *path, int indx) 
{
    char aux[0x420];

    path[0] = 0;

    if(!indx) {path[0] = '/'; path[1] = 0; return;}

    while(1) {
        strcpy(aux, directory_iso2[indx].name);
        strcat(aux, path);
        strcpy(path, aux);
       
        indx = directory_iso2[indx].parent - 1;
        if(indx == 0) break;     
    }

}

static int read_split(u64 position, u8 *mem, int size)
{
    int n;

    if(!split_file[1].size) {

        if(fd_split0 < 0) fd_split0 = ps3ntfs_open(split_file[0].path, O_RDONLY, 0766);
        if(fd_split0 < 0) return -666;

        if(ps3ntfs_seek64(fd_split0, position, SEEK_SET)<0) {
            DPrintf("Error!: in ISO file fseek\n\n");
            return -668;
        }

        if(ps3ntfs_read(fd_split0, (void *) mem, size) != size) return -667;
        return 0;
    }

    u64 relpos0 = 0;
    u64 relpos1 = 0;

    for(n = 0; n < 64; n++){
        if(!split_file[n].size) return -669;
        if(position < (relpos0 + (u64) split_file[n].size)) {
            relpos1 = relpos0 + (u64) split_file[n].size;
            break;
        }

        relpos0 += split_file[n].size;
    }
    
    if(fd_split < 0) split_index = 0;

    if(n == 0) {
        if(split_index && fd_split >= 0) {ps3ntfs_close(fd_split); fd_split = -1;}
        split_index = 0;
        fd_split = fd_split0;

    } else {

        if(n != split_index) {
            if(split_index && fd_split >= 0) {ps3ntfs_close(fd_split); fd_split = -1;}

            split_index = n;

            fd_split = ps3ntfs_open(split_file[split_index].path, O_RDONLY, 0766);
            if(fd_split < 0) return -666;

        }
    }

    //int cur = lba / SPLIT_LBA;
    //int cur2 = (lba + sectors) / SPLIT_LBA;

    if(ps3ntfs_seek64(fd_split, (position - relpos0), SEEK_SET)<0) {
        DPrintf("Error!: in ISO file fseek\n\n");
        return -668;
    }

    if(position >= relpos0 && (position + size) < relpos1) {

        if(ps3ntfs_read(fd_split, (void *) mem, (int) size) != size) return -667;
        return 0;
    }

    int lim = (int) (relpos1 - position);

    if(ps3ntfs_read(fd_split, (void *) mem, (int) lim) != lim) return -667;

    mem += lim; size-= lim;

    if(split_index && fd_split >= 0) {ps3ntfs_close(fd_split); fd_split = -1;}

    split_index++;

    fd_split = ps3ntfs_open(split_file[split_index].path, O_RDONLY, 0766);
    if(fd_split < 0) return -666;

    if(ps3ntfs_read(fd_split, (void *) mem, (int) size) != size) return -667;

    return 0;
}

int extractps3iso(char *f_iso, char *g_path, int split)
{

    struct stat s;
    int n;

    char path1[0x420];
    char path2[0x420];
    char path3[0x420];

    int len_path2;

    int fd2 = -1;

    u8 *sectors2 = NULL;

    char string[0x420];
    char string2[0x420];
    u16 wstring[1024];

    struct iso_primary_descriptor sect_descriptor;
    struct iso_directory_record * idr;
    int idx = -1;

    directory_iso2 = NULL;

    fd_split = -1;
    fd_split0 = -1;
    split_index = 0;
    split_files = 0; 

    u64 t_start, t_finish;

    initConsole();

    sprintf(dbhead, "%s - done (0/100)", "EXTRACTPS3ISO Utility");

    DbgHeader(dbhead);

    DPrintf("\nEXTRACTPS3ISO (c) 2013, Estwald (Hermes)\n\n");

    // libc test
    if(sizeof(s.st_size) != 8) {

        DPrintf("Error!: stat st_size must be a 64 bit number!  (size %i)\n\nPress TRIANGLE button to exit\n\n", (int) sizeof(s.st_size));
        get_input_char();
        return -1;
    }

    split_file = malloc(sizeof(_split_file) * 64);
    if(!split_file) {

        DPrintf("Error!: out of memory! (split_file)\n\nPress TRIANGLE button to exit\n\n");
        get_input_char();
        return -1;
    }

    strcpy(path1, f_iso);
    
    if(path1[0] == 0) {
         free(split_file); split_file = NULL;
         DPrintf("Error: ISO file don't exists!\n\nPress TRIANGLE button to exit\n"); get_input_char();return -1;
    }

    fixpath(path1);

    n = strlen(path1);


    if(n >= 4 && (!strcmp(&path1[n - 4], ".iso") || !strcmp(&path1[n - 4], ".ISO"))) {

        sprintf(split_file[0].path, "%s", path1);
        if(stat(split_file[0].path, &s)<0) {
            free(split_file); split_file = NULL;
            DPrintf("Error: ISO file don't exists!\n\nPress TRIANGLE button to exit\n"); get_input_char();return -1;

        }

        split_file[0].size = s.st_size;
        split_file[1].size = 0; // split off
       
    } else if(n >= 6 && (!strcmp(&path1[n - 6], ".iso.0") || !strcmp(&path1[n - 6], ".ISO.0"))) {

        int m;

        for(m = 0; m < 64; m++) {
            strcpy(string2, path1);
            string2[n - 2] = 0; 
            sprintf(split_file[m].path, "%s.%i", string2, m);
            if(stat(split_file[m].path, &s)<0) break;
            split_file[m].size = s.st_size;
        }

        for(; m < 64; m++) {
            split_file[m].size = 0;
        }

       
    } else {
        free(split_file); split_file = NULL;
        DPrintf("Error: file must be with .iso, .ISO .iso.0 or .ISO.0 extension\n\nPress TRIANGLE button to exit\n"); get_input_char();return -1;
    }

    path2[0] = 0;
    strcpy(path2, g_path);

    fixpath(path2);

    if(path2[0] == 0) {
        free(split_file); split_file = NULL;
        DPrintf("Error: Invalid game path\n\nPress TRIANGLE button to exit\n"); get_input_char();return -1;
  
    } else if(stat(path2, &s)==0) {

        strcat(path2, "/");

        char * o = strrchr(path1, '/');
        if(!o)
            strcat(path2, path1);
        else {
            strcat(path2, o + 1);
        }

        n= strlen(path2);

        if(!strcmp(&path2[n - 2], ".0")) path2[n - 6] = 0; else path2[n - 4] = 0;
        
        fixpath(path2);

        DPrintf("path: %s\n", path2);

    }

    strcpy(path3, path2);
    
    split_files = split;

    if(split_files) DPrintf("Using Split File Mode\n");

    DPrintf("\n");

    
    len_path2 = strlen(path2);

    ps3ntfs_mkdir(path2, 0766); // make directory

    u64 avail = get_disk_free_space(path2);

    int fd = ps3ntfs_open(path1, O_RDONLY, 0766);
    if(fd < 0) {
        free(split_file); split_file = NULL;
        DPrintf("Error!: Cannot open ISO file\n\nPress TRIANGLE button to exit\n\n");
        get_input_char();
        return -1;
    }

    t_start = get_ticks();

    
    if(ps3ntfs_seek64(fd, 0x8800, SEEK_SET)<0) {
        DPrintf("Error!: in sect_descriptor fseek\n\n");
        goto err;
    }

    if(ps3ntfs_read(fd, (void *) &sect_descriptor, 2048) != 2048) {
        DPrintf("Error!: reading sect_descriptor\n\n");
        goto err;
    }

    if(!(sect_descriptor.type[0] == 2 && !strncmp((void *) &sect_descriptor.id[0], "CD001",5))) {
        DPrintf("Error!: UTF16 descriptor not found\n\nPress TRIANGLE button to exit\n\n");
        goto err;
    }

    u32 toc = isonum_733(&sect_descriptor.volume_space_size[0]);

    if((((u64)toc) * 2048ULL) > (avail - 0x100000ULL)) {
        DPrintf("Error!: Insufficient Disk Space in Destination\n");
        goto err;
    }

    u32 lba0 = isonum_731(&sect_descriptor.type_l_path_table[0]); // lba
    u32 size0 = isonum_733(&sect_descriptor.path_table_size[0]); // tamaño
    //DPrintf("lba0 %u size %u %u\n", lba0, size0, ((size0 + 2047)/2048) * 2048);
    
    if(ps3ntfs_seek64(fd, lba0 * 2048, SEEK_SET)<0) {
        DPrintf("Error!: in path_table fseek\n\n");
        goto err;
    }

    directory_iso2 = malloc((MAX_ISO_PATHS + 1) * sizeof(_directory_iso2));

    if(!directory_iso2) {
        DPrintf("Error!: in directory_is malloc()\n\n");
        goto err;
    }

    memset(directory_iso2, 0, (MAX_ISO_PATHS + 1) * sizeof(_directory_iso2));
 
    sectors = malloc(((size0 + 2047)/2048) * 2048);

    if(!sectors) {
        DPrintf("Error!: in sectors malloc()\n\n");
        goto err;
    }

    sectors2 = malloc(2048 * 2);

    if(!sectors2) {
        DPrintf("Error!: in sectors2 malloc()\n\n");
        goto err;
    }

    sectors3 = malloc(128 * 2048);

    if(!sectors3) {
        DPrintf("Error!: in sectors3 malloc()\n\n");
        goto err;
    }

    if(ps3ntfs_read(fd, (void *) sectors, size0) != size0) {
        DPrintf("Error!: reading path_table\n\n");
        goto err;
    }


    u32 p = 0;

    string2[0] = 0;

    fd_split = -1;
    fd_split0 = -1;

    split_index = 0;


    idx = 0;

    directory_iso2[idx].name = NULL;

    while(p < size0) {

        u32 lba;

        u32 snamelen = isonum_721(&sectors[p]);
        if(snamelen == 0) p= ((p/2048) * 2048) + 2048;
        p+=2;
        lba = isonum_731(&sectors[p]);
        p+=4;
        u32 parent =isonum_721(&sectors[p]);
        p+=2;

        memset(wstring, 0, 512 * 2);
        memcpy(wstring, &sectors[p], snamelen);
        
        UTF16_to_UTF8(wstring, (u8 *) string);

        if(idx >= MAX_ISO_PATHS){
            DPrintf("Too much folders (max %i)\n\n", MAX_ISO_PATHS);
            goto err;
        }

        directory_iso2[idx].name = malloc(strlen(string) + 2);
        if(!directory_iso2[idx].name) {
            DPrintf("Error!: in directory_iso2.name malloc()\n\n");
            goto err;
        }

        strcpy(directory_iso2[idx].name, "/");
        strcat(directory_iso2[idx].name, string);
        
        directory_iso2[idx].parent = parent;
        
        get_iso_path(string2, idx);

        strcat(path2, string2);

        ps3ntfs_mkdir(path2, 0766); // make directory
     
        path2[len_path2] = 0;
     
        DPrintf("</%s>\n", string);
   
        u32 file_lba = 0, old_file_lba = 0;
        u64 file_size = 0;

        char file_aux[0x420];

        file_aux[0] = 0;

        int q2 = 0;
        int size_directory = 0;
        
        while(1) {

            if(ps3ntfs_seek64(fd, lba * 2048, SEEK_SET)<0) {
                DPrintf("Error!: in directory_record fseek\n\n");
                goto err;
            }

            memset(sectors2 + 2048, 0, 2048);

            if(ps3ntfs_read(fd, (void *) sectors2, 2048) != 2048) {
                DPrintf("Error!: reading directory_record sector\n\n");
                goto err;
            }

            int q = 0;

            if(q2 == 0) {
                idr = (struct iso_directory_record *) &sectors2[q];
                if((int) idr->name_len[0] == 1 && idr->name[0]== 0 && lba == isonum_731((void *) idr->extent) && idr->flags[0] == 0x2) {
                    size_directory = isonum_733((void *) idr->size);
                 
                } else {
                    DPrintf("Error!: Bad first directory record! (LBA %i)\n\n", lba);
                    goto err;
                }
            }

            int signal_idr_correction = 0;

            while(1) {

                if(signal_idr_correction) {
                    signal_idr_correction = 0;
                    q-= 2048; // sector correction
                    // copy next sector to first
                    memcpy(sectors2, sectors2 + 2048, 2048);
                    memset(sectors2 + 2048, 0, 2048);
                    lba++;

                    q2 += 2048;

                }

                if(q2 >= size_directory) goto end_dir_rec;

                idr = (struct iso_directory_record *) &sectors2[q];

                if(idr->length[0]!=0 && (idr->length[0] + q) > 2048) {
                
                    DPrintf("Warning! Entry directory break the standard ISO 9660\n\nPress TRIANGLE button\n\n");
                    get_input_char();

                    if(ps3ntfs_seek64(fd, lba * 2048 + 2048, SEEK_SET)<0) {
                        DPrintf("Error!: in directory_record fseek\n\n");
                        goto err;
                    }

                    if(ps3ntfs_read(fd, (void *) (sectors2 + 2048), 2048) != 2048) {
                        DPrintf("Error!: reading directory_record sector\n\n");
                        goto err;
                    }

                    signal_idr_correction = 1;
               
                }

                if(idr->length[0] == 0 && (2048 - q) > 255) goto end_dir_rec;

                if((idr->length[0] == 0 && q != 0) || q == 2048)  { 
                    
                    lba++;
                    q2 += 2048;

                    if(q2 >= size_directory) goto end_dir_rec;

                    if(ps3ntfs_seek64(fd, lba * 2048, SEEK_SET)<0) {
                        DPrintf("Error!: in directory_record fseek\n\n");
                        goto err;
                    }

                    if(ps3ntfs_read(fd, (void *) (sectors2), 2048) != 2048) {
                        DPrintf("Error!: reading directory_record sector\n\n");
                        goto err;
                    }

                    memset(sectors2 + 2048, 0, 2048);

                    q = 0;
                    idr = (struct iso_directory_record *) &sectors2[q];

                    if(idr->length[0] == 0 || ((int) idr->name_len[0] == 1 && !idr->name[0])) goto end_dir_rec;
                    
                }

                if((int) idr->name_len[0] > 1 && idr->flags[0] != 0x2 &&
                    idr->name[idr->name_len[0] - 1]== '1' && idr->name[idr->name_len[0] - 3]== ';') { // skip directories
                    
                    memset(wstring, 0, 512 * 2);
                    memcpy(wstring, idr->name, idr->name_len[0]);
                
                    UTF16_to_UTF8(wstring, (u8 *) string); 

                    if(file_aux[0]) {
                        if(strcmp(string, file_aux)) {
    
                            DPrintf("Error!: in batch file %s\n\nPress TRIANGLE button to exit\n\n", file_aux);
                            goto err;
                        }

                        file_size += (u64) (u32) isonum_733(&idr->size[0]);
                        if(idr->flags[0] == 0x80) {// get next batch file
                            q+= idr->length[0]; 
                            continue;
                        } 

                        file_aux[0] = 0; // stop batch file

                    } else {

                        file_lba = isonum_733(&idr->extent[0]);
                        file_size = (u64) (u32) isonum_733(&idr->size[0]);
                        if(idr->flags[0] == 0x80) {
                            strcpy(file_aux, string);
                            q+= idr->length[0];
                            continue;  // get next batch file
                        }
                    }

                    int len = strlen(string);

                    string[len - 2] = 0; // break ";1" string
                    
                    len = strlen(string2);
                    strcat(string2, "/");
                    strcat(string2, string);

                    if(old_file_lba < file_lba) {
                        sprintf(dbhead, "%s - done (%i/100)", "EXTRACTPS3ISO Utility", file_lba * 100 / toc);
                        DbgHeader(dbhead);
                        old_file_lba = file_lba;
                    }

                    if(file_size < 1024ULL) 
                        DPrintf("  -> %s LBA %u size %u Bytes\n", string, file_lba, (u32) file_size);
                    else if(file_size < 0x100000LL) 
                        DPrintf("  -> %s LBA %u size %u KB\n", string, file_lba, (u32) (file_size/1024));
                    else
                        DPrintf("  -> %s LBA %u size %u MB\n", string, file_lba, (u32) (file_size/0x100000LL));

                    //DPrintf("f %s\n", string2);

                    // writing procedure
                 
                    strcat(path2, string2);

                    int use_split = 0;

                    if(split_files && file_size >= 0xFFFF0001LL) {
                        use_split = 1;
                        sprintf(string, "%s.666%2.2u", path2, 0);
                        fd2 = ps3ntfs_open(string, O_WRONLY | O_CREAT | O_TRUNC, 0766);

                    } else
                        fd2 = ps3ntfs_open(path2, O_WRONLY | O_CREAT | O_TRUNC, 0766);

                    if(fd2 >= 0) {
                        
                        fd_split0 = fd;
                        
                        u32 count = 0, percent = (u32) (file_size / 0x40000ULL);
                        if(percent == 0) percent = 1;

                        int count_split = 0;

                        int cx = con_x, cy = con_y;

                        u64 t_one, t_two;

                        t_one = get_ticks();

                        if(get_input_abort()) {
                            DPrintf("\nError!: Aborted by User\n\n");
                            goto err;
                        }

                        while(file_size > 0) {
                            u32 fsize;

                            con_x = cx; con_y = cy;
                            t_two = get_ticks();

                            if(((t_two - t_one) >= TICKS_PER_SEC/2)) {
                                t_one = t_two;
                                DPrintf("*** Writing... %u %%", count * 100 / percent);
                                con_x = cx; con_y = cy;

                                if(get_input_abort()) {
                                    DPrintf("\nError!: Aborted by User\n\n");
                                    goto err;
                                }
                            }
  
                            if(use_split && count_split >= 0x40000000) {
                                count_split = 0;
                                ps3ntfs_close(fd2);
                                
                                sprintf(string, "%s.666%2.2u", path2, use_split);
                                use_split++;
                                fd2 = ps3ntfs_open(string, O_WRONLY | O_CREAT | O_TRUNC, 0766);

                                if(fd2 < 0) {
                                    DPrintf("\nError!: creating extract file\n\n");
                                    goto err;
                                }

                            }

                            if(file_size > 0x40000) fsize = 0x40000;
                            else fsize = (u32) file_size;

                            count++;
                            
                            if(use_split) count_split+= fsize;

                            if(read_split(((u64) file_lba) * 2048ULL, (void *) sectors3, (int) fsize) < 0) {
                                DPrintf("\nError!: reading ISO file\n\n");
                                goto err;
                            }

                            if(ps3ntfs_write(fd2, (void *) sectors3, (int) fsize) != fsize) {
                                DPrintf("\nError!: writing ISO file\n\n");
                                goto err;
                            }

                            file_size-= (u64) fsize;

                            file_lba += (fsize + 2047)/ 2048;
                        }

                        con_x = cx; con_y = cy;
                        DPrintf("                             ");
                        con_x = cx; con_y = cy;

                        ps3ntfs_close(fd2); fd2 = -1;
                    } else {

                        DPrintf("\nError!: creating extract file\n\n");
                        goto err;

                    }

                    path2[len_path2] = 0;
                    string2[len] = 0;

                   
                }

                q+= idr->length[0];
            }

            lba ++;
            q2+= 2048;
            if(q2 >= size_directory) goto end_dir_rec;

        }

        end_dir_rec:

        p+= snamelen;
        if(snamelen & 1) p++;

        idx++;

    }

    sprintf(dbhead, "%s - done (100/100)", "EXTRACTPS3ISO Utility");
    DbgHeader(dbhead);

    if(fd) ps3ntfs_close(fd);
    if(fd2) ps3ntfs_close(fd2);
    if(split_index && fd_split) {ps3ntfs_close(fd_split); fd_split = -1;}
    if(sectors) free(sectors); sectors = NULL;
    if(sectors2) free(sectors2); sectors2 = NULL;
    if(sectors3) free(sectors3); sectors3 = NULL;

    for(n = 0; n <= idx; n++)
        if(directory_iso2[n].name) {free(directory_iso2[n].name); directory_iso2[n].name = NULL;}
    
    if(directory_iso2) free(directory_iso2); directory_iso2 = NULL;

    free(split_file); split_file = NULL;

    t_finish = get_ticks();    

    DPrintf("Finish!\n\n");
    DPrintf("Total Time (HH:MM:SS): %2.2u:%2.2u:%2.2u.%u\n\n", (u32) (t_finish - t_start)/(TICKS_PER_SEC * 3600),
        (u32) ((t_finish - t_start)/(TICKS_PER_SEC * 60)) % 60, (u32) ((t_finish - t_start)/(TICKS_PER_SEC)) % 60,
        (u32) ((t_finish - t_start)/(TICKS_PER_SEC/100)) % 100);
    
    DPrintf("\nPress TRIANGLE button to exit\n");
    get_input_char();
 
    return 0;

err:

    if(fd) ps3ntfs_close(fd);
    if(fd2) ps3ntfs_close(fd2);
    if(split_index && fd_split) {ps3ntfs_close(fd_split); fd_split = -1;}

    if(sectors) free(sectors); sectors = NULL;
    if(sectors2) free(sectors2); sectors2 = NULL;
    if(sectors3) free(sectors3); sectors3 = NULL;

    for(n = 0; n <= idx; n++)
        if(directory_iso2[n].name) {free(directory_iso2[n].name); directory_iso2[n].name = NULL;}
    
    if(directory_iso2) free(directory_iso2); directory_iso2 = NULL;

    free(split_file); split_file = NULL;

    DPrintf("\nDelete partial GAME?  (CROSS - Yes/ TRIANGLE - No):\n");
    if(get_input_char()=='y') {
    
        DPrintf("Yes\n");
        DeleteDirectory(path3);
        ps3ntfs_unlink(path3);  

    } else DPrintf("No\n");

    DPrintf("\nPress TRIANGLE button to exit\n");
    get_input_char();

    return -1;
}

static int write_split2(u64 position, u8 *mem, int size)
{
    int n;

    if(!split_file[1].size) {

        if(fd_split0 < 0) fd_split0 = ps3ntfs_open(split_file[0].path, O_RDWR, 0766);
        if(fd_split0 < 0) return -666;

        if(ps3ntfs_seek64(fd_split0, position, SEEK_SET)<0) {
            DPrintf("Error!: in ISO file fseek\n\n");
            return -668;
        }

        if(ps3ntfs_write(fd_split0, (void *) mem, size) != size) return -667;
        return 0;
    }

    u64 relpos0 = 0;
    u64 relpos1 = 0;

    for(n = 0; n < 64; n++){
        if(!split_file[n].size) return -669;
        if(position < (relpos0 + (u64) split_file[n].size)) {
            relpos1 = relpos0 + (u64) split_file[n].size;
            break;
        }

        relpos0 += split_file[n].size;
    }
    
    if(fd_split < 0) split_index = 0;

    if(n == 0) {
        if(split_index && fd_split >= 0) {ps3ntfs_close(fd_split); fd_split = -1;}
        split_index = 0;
        fd_split = fd_split0;

    } else {

        if(n != split_index) {
            if(split_index && fd_split >= 0) {ps3ntfs_close(fd_split); fd_split = -1;}

            split_index = n;

            fd_split = ps3ntfs_open(split_file[split_index].path, O_RDWR, 0766);
            if(fd_split < 0) return -666;

        }
    }

    //int cur = lba / SPLIT_LBA;
    //int cur2 = (lba + sectors) / SPLIT_LBA;

    if(ps3ntfs_seek64(fd_split, (position - relpos0), SEEK_SET)<0) {
        DPrintf("Error!: in ISO file fseek\n\n");
        return -668;
    }

    if(position >= relpos0 && (position + size) < relpos1) {

        if(ps3ntfs_write(fd_split, (void *) mem, (int) size) != size) return -667;
        return 0;
    }

    int lim = (int) (relpos1 - position);

    if(ps3ntfs_write(fd_split, (void *) mem, (int) lim) != lim) return -667;

    mem += lim; size-= lim;

    if(split_index && fd_split > 0) {ps3ntfs_close(fd_split); fd_split = -1;}

    split_index++;

    fd_split = ps3ntfs_open(split_file[split_index].path, O_RDWR, 0766);
    if(fd_split < 0) return -666;

    if(ps3ntfs_write(fd_split, (void *) mem, (int) size) != size) return -667;

    return 0;
}

static int iso_param_sfo_util(u32 lba, u32 len)
{
    u32 pos, str;

    char str_version[8];
    
    param_patched = 0;

    u16 cur_firm = ((firmware>>12) & 0xF) * 10000 + ((firmware>>8) & 0xF) * 1000 + ((firmware>>4) & 0xF) * 100;

    sprintf(str_version, "%2.2u.%4.4u", cur_firm/10000, cur_firm % 10000 );
    
    unsigned char *mem = (void *) sectors3;

    u64 file_pos = ((u64) lba * 2048ULL);

    if(read_split(file_pos, (void *) mem, (int) len) < 0) {
        DPrintf("\nError!: reading in ISO PARAM.SFO file\n\n");
        return -1;
    }
    
    str= (mem[8]+(mem[9]<<8));
    pos=(mem[0xc]+(mem[0xd]<<8));

    int indx=0;

    while(str<len) {
        if(mem[str]==0) break;

        if(!strcmp((char *) &mem[str], "PS3_SYSTEM_VER")) {
           
            if(strcmp((char *) &mem[pos], str_version) > 0) {
                
                DPrintf("PARAM.SFO patched to version: %s from %s\n\n", str_version, &mem[pos]);
                    memcpy(&mem[pos], str_version, 8);
                param_patched = 1;
                break;
            }
            
        }

        while(mem[str]) str++;str++;
        pos+=(mem[0x1c+indx]+(mem[0x1d+indx]<<8));
        indx+=16;
    }

    if(param_patched) {
        if(write_split2(file_pos, (void *) mem, (int) len) < 0) {
            DPrintf("\nError!: reading in ISO PARAM.SFO file\n\n");
            return -1;
        }
    }


    return 0;
}

static int iso_patch_exe_error_09(u32 lba, char *filename)
{
    
    u16 fw_421 = 42100;
    u16 fw_460 = 46000;
    u32 offset_fw;
    u16 ver = 0;
    int flag = 0;

    u64 file_pos = ((u64) lba * 2048ULL);

    //if(firmware < 0x421C || firmware >= 0x460C) return 0;

    // open self/sprx and changes the fw version
    
    // set to offset position
    
    if(read_split(file_pos + 0xCULL, (void *) &offset_fw, (int) 4) < 0) {
        DPrintf("\nError!: reading in ISO SPRX/SELF file\n\n");
        return -1;
    }

    offset_fw = SWAP32(offset_fw);

    offset_fw+= 0x1E;

    if(read_split(file_pos + ((u64) offset_fw), (void *) &ver, (int) 2) < 0) {
        DPrintf("\nError!: reading in ISO SPRX/SELF file\n\n");
        return -1;
    }

    ver = SWAP16(ver);

    u16 cur_firm = ((firmware>>12) & 0xF) * 10000 + ((firmware>>8) & 0xF) * 1000 + ((firmware>>4) & 0xF) * 100;
                    
    if(firmware >= 0x421C && firmware < 0x460C && ver > fw_421 && ver <= fw_460 && ver > cur_firm) {

        DPrintf("Version changed from %u.%u to %u.%u in %s\n\n", ver/10000, (ver % 10000)/100, cur_firm/10000, (cur_firm % 10000)/100, filename);
        cur_firm = SWAP16(cur_firm);
        if(write_split2(file_pos + ((u64) offset_fw), (void *) &cur_firm, (int) 2) < 0) {
            DPrintf("\nError!: writing ISO file\n\n");
            return -1;
        }

        flag = 1;
        self_sprx_patched++;

    } else if(ver > cur_firm) {
         DPrintf("\nError!: this SELF/SPRX uses a bigger version of %u.%uC (%u.%u)\n\n", cur_firm/10000, (cur_firm % 10000)/100, ver/10000, (ver % 10000)/100);
        flag = -1; // 
    }
                   

   return flag;
}

int patchps3iso(char *f_iso)
{

    struct stat s;
    int n;

    char path1[0x420];

    u8 *sectors2 = NULL;
    sectors3 = NULL;

    char string[0x420];
    char string2[0x420];
    u16 wstring[1024];

    int num_files = 0;
    int num_dir = 0;

    struct iso_primary_descriptor sect_descriptor;
    struct iso_directory_record * idr;
    int idx = -1;

    directory_iso2 = NULL;

    fd_split = -1;
    fd_split0 = -1;
    split_index = 0;
    param_patched = 0;
    self_sprx_patched = 0;

    u64 t_start, t_finish;

    initConsole();

    DbgHeader("PATCHPS3ISO Utility");

    DPrintf("\nPATCHPS3ISO (c) 2013, Estwald (Hermes)\n\n");

    // libc test
    if(sizeof(s.st_size) != 8) {

        DPrintf("Error!: stat st_size must be a 64 bit number!  (size %i)\n\nPress TRIANGLE button to exit\n\n", (int) sizeof(s.st_size));
        get_input_char();
        return -1;
    }

    split_file = malloc(sizeof(_split_file) * 64);
    if(!split_file) {

        DPrintf("Error!: out of memory! (split_file)\n\nPress TRIANGLE button to exit\n\n");
        get_input_char();
        return -1;
    }

    strcpy(path1, f_iso);

    if(path1[0] == 0) {
        free(split_file); split_file = NULL;
        DPrintf("Error: ISO file don't exists!\n\nPress TRIANGLE button to exit\n"); get_input_char();return -1;
    }

    fixpath(path1);

    n = strlen(path1);


    if(n >= 4 && (!strcmp(&path1[n - 4], ".iso") || !strcmp(&path1[n - 4], ".ISO"))) {

        sprintf(split_file[0].path, "%s", path1);
        if(stat(split_file[0].path, &s)<0) {
            free(split_file); split_file = NULL;
            DPrintf("Error: ISO file don't exists!\n\nPress TRIANGLE button to exit\n"); get_input_char();return -1;

        }

        split_file[0].size = s.st_size;
        split_file[1].size = 0; // split off
       
    } else if(n >= 6 && (!strcmp(&path1[n - 6], ".iso.0") || !strcmp(&path1[n - 6], ".ISO.0"))) {

        int m;

        for(m = 0; m < 64; m++) {
            strcpy(string2, path1);
            string2[n - 2] = 0; 
            sprintf(split_file[m].path, "%s.%i", string2, m);
            if(stat(split_file[m].path, &s)<0) break;
            split_file[m].size = s.st_size;
        }

        for(; m < 64; m++) {
            split_file[m].size = 0;
        }

       
    } else {
        DPrintf("Error: file must be with .iso, .ISO .iso.0 or .ISO.0 extension\n\nPress TRIANGLE button to exit\n"); get_input_char();return -1;
    }
   
    DPrintf("\n");

    int fd = ps3ntfs_open(path1, O_RDWR, 0766);
    if(fd < 0) {
        free(split_file); split_file = NULL;
        DPrintf("Error!: Cannot open ISO file\n\nPress TRIANGLE button to exit\n\n");
        get_input_char();
        return -1;
    }

    t_start = get_ticks();

    
    if(ps3ntfs_seek64(fd, 0x8800, SEEK_SET)<0) {
        DPrintf("Error!: in sect_descriptor fseek\n\n");
        goto err;
    }

    if(ps3ntfs_read(fd, (void *) &sect_descriptor, 2048) != 2048) {
        DPrintf("Error!: reading sect_descriptor\n\n");
        goto err;
    }

    if(!(sect_descriptor.type[0] == 2 && !strncmp((void *) &sect_descriptor.id[0], "CD001",5))) {
        DPrintf("Error!: UTF16 descriptor not found\n\nPress TRIANGLE button to exit\n\n");
        goto err;
    }
    
    toc = isonum_733((void *) &sect_descriptor.volume_space_size[0]);
    u32 lba0 = isonum_731(&sect_descriptor.type_l_path_table[0]); // lba
    u32 size0 = isonum_733(&sect_descriptor.path_table_size[0]); // tamaño
    //DPrintf("lba0 %u size %u %u\n", lba0, size0, ((size0 + 2047)/2048) * 2048);
    
    if(ps3ntfs_seek64(fd, lba0 * 2048, SEEK_SET)<0) {
        DPrintf("Error!: in path_table fseek\n\n");
        goto err;
    }

    directory_iso2 = malloc((MAX_ISO_PATHS + 1) * sizeof(_directory_iso2));

    if(!directory_iso2) {
        DPrintf("Error!: in directory_is malloc()\n\n");
        goto err;
    }

    memset(directory_iso2, 0, (MAX_ISO_PATHS + 1) * sizeof(_directory_iso2));
 
    sectors = malloc(((size0 + 2047)/2048) * 2048);

    if(!sectors) {
        DPrintf("Error!: in sectors malloc()\n\n");
        goto err;
    }

    sectors2 = malloc(2048 * 2);

    if(!sectors2) {
        DPrintf("Error!: in sectors2 malloc()\n\n");
        goto err;
    }

    sectors3 = malloc(128 * 2048);

    if(!sectors3) {
        DPrintf("Error!: in sectors3 malloc()\n\n");
        goto err;
    }

    if(ps3ntfs_read(fd, (void *) sectors, size0) != size0) {
        DPrintf("Error!: reading path_table\n\n");
        goto err;
    }


    u32 p = 0;

    string2[0] = 0;

    fd_split = -1;
    fd_split0 = -1;

    split_index = 0;


    idx = 0;

    directory_iso2[idx].name = NULL;

    while(p < size0) {

        u32 lba;

        u32 snamelen = isonum_721(&sectors[p]);
        if(snamelen == 0) p= ((p/2048) * 2048) + 2048;
        p+=2;
        lba = isonum_731(&sectors[p]);
        p+=4;
        u32 parent =isonum_721(&sectors[p]);
        p+=2;

        memset(wstring, 0, 512 * 2);
        memcpy(wstring, &sectors[p], snamelen);
        
        UTF16_to_UTF8(wstring, (u8 *) string);
        
        if(idx >= MAX_ISO_PATHS){
            DPrintf("Too much folders (max %i)\n\n", MAX_ISO_PATHS);
            goto err;
        }

   
        directory_iso2[idx].name = malloc(strlen(string) + 2);
        if(!directory_iso2[idx].name) {
            DPrintf("Error!: in directory_iso2.name malloc()\n\n");
            goto err;
        }

        strcpy(directory_iso2[idx].name, "/");
        strcat(directory_iso2[idx].name, string);
        
        directory_iso2[idx].parent = parent;
        
        get_iso_path(string2, idx);
 
        DPrintf("</%s>\n", string);
   
        u32 file_lba = 0;
        u64 file_size = 0;

        char file_aux[0x420];

        file_aux[0] = 0;

        int q2 = 0;
        int size_directory = 0;
        
        while(1) {

            if(ps3ntfs_seek64(fd, lba * 2048, SEEK_SET)<0) {
                DPrintf("Error!: in directory_record fseek\n\n");
                goto err;
            }

            memset(sectors2 + 2048, 0, 2048);

            if(ps3ntfs_read(fd, (void *) sectors2, 2048) != 2048) {
                DPrintf("Error!: reading directory_record sector\n\n");
                goto err;
            }

            int q = 0;

            if(q2 == 0) {
                idr = (struct iso_directory_record *) &sectors2[q];
                if((int) idr->name_len[0] == 1 && idr->name[0]== 0 && lba == isonum_731((void *) idr->extent) && idr->flags[0] == 0x2) {
                    size_directory = isonum_733((void *) idr->size);
                 
                } else {
                    DPrintf("Error!: Bad first directory record! (LBA %i)\n\n", lba);
                    goto err;
                }
            }

            int signal_idr_correction = 0;

            while(1) {

                if(signal_idr_correction) {
                    signal_idr_correction = 0;
                    q-= 2048; // sector correction
                    // copy next sector to first
                    memcpy(sectors2, sectors2 + 2048, 2048);
                    memset(sectors2 + 2048, 0, 2048);
                    lba++;

                    q2 += 2048;

                }

                if(q2 >= size_directory) goto end_dir_rec;

                idr = (struct iso_directory_record *) &sectors2[q];

                if(idr->length[0]!=0 && (idr->length[0] + q) > 2048) {
                
                    DPrintf("Warning! Entry directory break the standard ISO 9660\n\nPress TRIANGLE button\n\n");
                    get_input_char();

                    if(ps3ntfs_seek64(fd, lba * 2048 + 2048, SEEK_SET)<0) {
                        DPrintf("Error!: in directory_record fseek\n\n");
                        goto err;
                    }

                    if(ps3ntfs_read(fd, (void *) (sectors2 + 2048), 2048) != 2048) {
                        DPrintf("Error!: reading directory_record sector\n\n");
                        goto err;
                    }

                    signal_idr_correction = 1;
               
                }

                if(idr->length[0] == 0 && (2048 - q) > 255) goto end_dir_rec;

                if((idr->length[0] == 0 && q != 0) || q == 2048)  { 
                    
                    lba++;
                    q2 += 2048;

                    if(q2 >= size_directory) goto end_dir_rec;

                    if(ps3ntfs_seek64(fd, lba * 2048, SEEK_SET)<0) {
                        DPrintf("Error!: in directory_record fseek\n\n");
                        goto err;
                    }

                    if(ps3ntfs_read(fd, (void *) (sectors2), 2048) != 2048) {
                        DPrintf("Error!: reading directory_record sector\n\n");
                        goto err;
                    }

                    memset(sectors2 + 2048, 0, 2048);

                    q = 0;
                    idr = (struct iso_directory_record *) &sectors2[q];

                    if(idr->length[0] == 0 || ((int) idr->name_len[0] == 1 && !idr->name[0])) goto end_dir_rec;
                    
                }
                
                if((int) idr->name_len[0] > 1 && idr->flags[0] != 0x2 &&
                    idr->name[idr->name_len[0] - 1]== '1' && idr->name[idr->name_len[0] - 3]== ';') { // skip directories

                    if(idr->flags[0] != 0x80) num_files++;
                    
                    memset(wstring, 0, 512 * 2);
                    memcpy(wstring, idr->name, idr->name_len[0]);
                
                    UTF16_to_UTF8(wstring, (u8 *) string); 

                    if(file_aux[0]) {
                        if(strcmp(string, file_aux)) {
    
                            DPrintf("Error!: in batch file %s\n\nPress TRIANGLE button to exit\n\n", file_aux);
                            goto err;
                        }

                        file_size += (u64) (u32) isonum_733(&idr->size[0]);
                        if(idr->flags[0] == 0x80) {// get next batch file
                            q+= idr->length[0]; 
                            continue;
                        } 

                        file_aux[0] = 0; // stop batch file

                    } else {

                        file_lba = isonum_733(&idr->extent[0]);
                        file_size = (u64) (u32) isonum_733(&idr->size[0]);
                        if(idr->flags[0] == 0x80) {
                            strcpy(file_aux, string);
                            q+= idr->length[0];
                            continue;  // get next batch file
                        }
                    }

                    int len = strlen(string);

                    string[len - 2] = 0; // break ";1" string
                    
                    len = strlen(string2);
                    strcat(string2, "/");
                    strcat(string2, string);

                    if(file_size < 1024ULL) 
                        DPrintf("  -> %s LBA %u size %u Bytes\n", string, file_lba, (u32) file_size);
                    else if(file_size < 0x100000LL) 
                        DPrintf("  -> %s LBA %u size %u KB\n", string, file_lba, (u32) (file_size/1024));
                    else
                        DPrintf("  -> %s LBA %u size %u MB\n", string, file_lba, (u32) (file_size/0x100000LL));

                    //DPrintf("f %s\n", string2);

                    // writing procedure;

                    
                        
                    fd_split0 = fd;

                    if(get_input_abort()) {
                        DPrintf("\nError!: Aborted by User\n\n");
                        goto err;
                    }

                    if(!strcmp(string, "PARAM.SFO")) {
                        iso_param_sfo_util(file_lba, file_size);
                        goto next_file;
                    }

                    int ext = strlen(string) - 4;

                    if(ext <= 1) {goto next_file;}

                    if((string[ ext - 1 ] == '.' && ((string[ ext ] == 's' && string[ ext + 1 ] == 'p' && string[ ext + 2 ] == 'r' && string[ ext + 3 ] == 'x') 
                        || (string[ ext ] == 'S' && string[ ext + 1 ] == 'P' && string[ ext + 2 ] == 'R' && string[ ext + 3 ] == 'X')
                        || (string[ ext ] == 's' && string[ ext + 1 ] == 'e' && string[ ext + 2 ] == 'l' && string[ ext + 3 ] == 'f')
                        || (string[ ext ] == 'S' && string[ ext + 1 ] == 'E' && string[ ext + 2 ] == 'L' && string[ ext + 3 ] == 'F')))
                        ||  strcmp( string, "EBOOT.BIN" ) == 0) {

                        if(iso_patch_exe_error_09(file_lba, string) < 0) {
                            DPrintf("Press TRIANGLE button to exit\n\n");
                            goto err;
                            
                        }
                    }

                next_file:
                    string2[len] = 0;
                   
                }

                q+= idr->length[0];
            }

            lba ++;
            q2+= 2048;
            if(q2 >= size_directory) goto end_dir_rec;

        }

        end_dir_rec:

        p+= snamelen;
        if(snamelen & 1) p++;

        idx++;
        num_dir++;

    }

    if(fd) ps3ntfs_close(fd);
    if(split_index && fd_split) {ps3ntfs_close(fd_split); fd_split = -1;}
    if(sectors) free(sectors); sectors = NULL;
    if(sectors2) free(sectors2); sectors2 = NULL;
    if(sectors3) free(sectors3); sectors3 = NULL;

    for(n = 0; n <= idx; n++)
        if(directory_iso2[n].name) {free(directory_iso2[n].name); directory_iso2[n].name = NULL;}
    
    if(directory_iso2) free(directory_iso2); directory_iso2 = NULL;
    free(split_file); split_file = NULL;

    t_finish = get_ticks();    

    DPrintf("Finish!\n\n");
    DPrintf("Total Time (HH:MM:SS): %2.2u:%2.2u:%2.2u.%u\nPARAM.SFO patched: %c\nSPRX/SELF patched: %i\n\n", (u32) (t_finish - t_start)/(TICKS_PER_SEC * 3600),
        (u32) ((t_finish - t_start)/(TICKS_PER_SEC * 60)) % 60, (u32) ((t_finish - t_start)/(TICKS_PER_SEC)) % 60,
        (u32) ((t_finish - t_start)/(TICKS_PER_SEC/100)) % 100, param_patched ? 'Y' : 'N', self_sprx_patched);

    u64 file_size = ((u64) toc) * 2048ULL;
    if(file_size < 1024ULL) 
        DPrintf("Total ISO Size %u Bytes\n", (u32) file_size);
    else if(file_size < 0x100000LL) 
        DPrintf("Total ISO Size %u KB\n", (u32) (file_size/1024));
    else
        DPrintf("Total ISO Size %u MB\nTotal folders %i\nTotal files %i\n", (u32) (file_size/0x100000LL), num_dir - 1, num_files);
    
    DPrintf("\nPress TRIANGLE button to exit\n");
    get_input_char();
  
    return 0;

err:

    if(fd) ps3ntfs_close(fd);
    if(split_index && fd_split) {ps3ntfs_close(fd_split); fd_split = -1;}

    if(sectors) free(sectors); sectors = NULL;
    if(sectors2) free(sectors2); sectors2 = NULL;
    if(sectors3) free(sectors3); sectors3 = NULL;

    for(n = 0; n <= idx; n++)
        if(directory_iso2[n].name) {free(directory_iso2[n].name); directory_iso2[n].name = NULL;}
    
    if(directory_iso2) free(directory_iso2); directory_iso2 = NULL;
    free(split_file); split_file = NULL;

    DPrintf("\nPress TRIANGLE button to exit\n");
    get_input_char();

    return -1;
}

int delps3iso(char *f_iso)
{
    int len, n;
    struct stat s;
    
    strcpy(output_name, f_iso);

    len = strlen(output_name);

    initConsole();

    DbgHeader("DELPS3ISO Utility");

    DPrintf("\nDeleting...\n\n");

    if(len >= 6 && (!strcmp(&output_name[len - 6], ".iso.0") || !strcmp(&output_name[len - 6], ".ISO.0"))) {
        output_name[len - 2] = 0;
                    
        for(n = 0; n < 64; n++) {
            sprintf(output_name2, "%s.%i", output_name, n);

            if(!ps3ntfs_stat(output_name2, &s)) {
                DPrintf("%s - Deleted\n", output_name2);
                ps3ntfs_unlink(output_name2);
            }
            else 
                break;
        }

    } else {
        DPrintf("%s - Deleted\n", output_name);
        ps3ntfs_unlink(output_name);
    }

    DPrintf("\nPress TRIANGLE button to exit\n");
    get_input_char();

    return 0;
}

