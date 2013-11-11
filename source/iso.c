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

#include "iso.h"

#include "utils.h"
#include <stdio.h>
#include <malloc.h>
#include <string.h>

#define ISODCL(from, to) (to - from + 1)

int isonum_731 (unsigned char * p)
{
	return ((p[0] & 0xff)
		| ((p[1] & 0xff) << 8)
		| ((p[2] & 0xff) << 16)
		| ((p[3] & 0xff) << 24));
}


void set731(unsigned char *p,int n)
{
    *p++=(n & 0xff);*p++=((n>>8) & 0xff);*p++=((n>>16) & 0xff);*p++=((n>>24) & 0xff);
}

void set733(unsigned char *p,int n)
{
    *p++=(n & 0xff);*p++=((n>>8) & 0xff);*p++=((n>>16) & 0xff);*p++=((n>>24) & 0xff);
    *p++=((n>>24) & 0xff);*p++=((n>>16) & 0xff);*p++=((n>>8) & 0xff);*p++=(n & 0xff);
}

void set732(unsigned char *p,int n)
{
    *p++=((n>>24) & 0xff);*p++=((n>>16) & 0xff);*p++=((n>>8) & 0xff);*p++=(n & 0xff);
}

void set721(unsigned char *p,int n)
{
    *p++=(n & 0xff);*p++=((n>>8) & 0xff);
}

void set722(unsigned char *p,int n)
{
    *p++=((n>>8) & 0xff);*p++=(n & 0xff);
}

void set723(unsigned char *p,int n)
{
    *p++=(n & 0xff);*p++=((n>>8) & 0xff);*p++=((n>>8) & 0xff);*p++=(n & 0xff);
}

int isonum_732 (unsigned char * p)
{
	return ((p[3] & 0xff)
		| ((p[2] & 0xff) << 8)
		| ((p[1] & 0xff) << 16)
		| ((p[0] & 0xff) << 24));
}


int isonum_733 (unsigned char * p)
{
	return (isonum_731 (p));
}


int isonum_721 (unsigned char * p)
{
	return ((p[0] & 0xff) | ((p[1] & 0xff) << 8));
}


int isonum_723 (unsigned char * p)
{
	return (isonum_721 (p));
}

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
    if(ps3ntfs_seek64(fd, 0x8000LL, SEEK_SET)<0) goto err; 
    if(ps3ntfs_read(fd, (void *) &sect_descriptor, sizeof(struct iso_primary_descriptor)) != sizeof(struct iso_primary_descriptor)) goto err;      
    #else
    if(fseek(fp, 0x8000, SEEK_SET)!=0) goto err;
    if(fread((void *) &sect_descriptor, 1, sizeof(struct iso_primary_descriptor), fp) != sizeof(struct iso_primary_descriptor)) goto err;
    #endif

    if(strncmp((void *) sect_descriptor.id, "CD001", 5)) goto err;

    u32 lba0 = isonum_731(&sect_descriptor.type_l_path_table[0]); // lba
    u32 size0 = isonum_733(&sect_descriptor.path_table_size[0]); // tamaño
    
    //char string[256];
    //sprintf(string, "lba0 %u size %u\n", lba0, size0);
    //DrawDialogOK(string);
    //return -4;

    int size1 = ((size0 + 2047)/2048) * 2048;
    sectors = malloc(size1 + 2048);
    if(!sectors) return -3;

    #ifdef USE_64BITS_LSEEK
    if(ps3ntfs_seek64(fd, ((s64) lba0) * 2048LL, SEEK_SET)<0) goto err; 
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
        u32 parent_name =isonum_721(&sectors[p]);
        p+=2;

        if(cur_parent==1 && folder_split[nsplit][0] == 0 && nsplit==0) {lba_folder = lba; break;}

        if(last_parent == parent_name && snamelen == folder_split[nsplit][2] 
            && !strncmp((void *) &sectors[p], &path[folder_split[nsplit][1]], snamelen)) {
            
            //printf("p: %s %u %u\n", &path[folder_split[nsplit][1]], folder_split[nsplit][2], snamelen);
            last_parent = cur_parent;
            
            nsplit++;
            if(folder_split[nsplit][0] == 0) {lba_folder = lba; break;}
        }
        
        p+= snamelen;
        cur_parent++;
        if(snamelen & 1) {p++;}
    }
    
    if(lba_folder == 0xffffffff) goto err;
    
    #ifdef USE_64BITS_LSEEK
    if(ps3ntfs_seek64(fd, ((s64) lba_folder) * 2048LL, SEEK_SET)<0) goto err; 
    if(ps3ntfs_read(fd, (void *) sectors, 2048) != 2048) goto err;      
    #else
    if(fseek(fp, lba_folder * 2048, SEEK_SET)!=0) goto err;
    if(fread((void *) sectors, 1, 2048, fp) != 2048) goto err;
    #endif

    p = 0;
    while(1) {
        if(nsplit >= nfolder_split) break;
        idr = (struct iso_directory_record *) &sectors[p];
        if(idr->length[0] == 0 && sizeof(struct iso_directory_record) + p > 2048) {
            lba_folder++;
           
            #ifdef USE_64BITS_LSEEK
            if(ps3ntfs_seek64(fd, ((s64) lba_folder) * 2048LL, SEEK_SET)<0) goto err;
            if(ps3ntfs_read(fd, (void *) sectors, 2048) != 2048) goto err;
            #else
            if(fseek(fp, lba_folder * 2048, SEEK_SET)!=0) goto err;
            if(fread((void *) sectors, 1, 2048, fp) != 2048) goto err;
            #endif
            p = 0; continue;
        }

        if(idr->length[0] == 0) break;
    
        if((int) idr->name_len[0] == folder_split[nsplit][2]
            && !strncmp((char *) idr->name, &path[folder_split[nsplit][1]], (int) idr->name_len[0])) {
            if(file_lba == 0xffffffff) file_lba = isonum_733(&idr->extent[0]);
            
            *size+= (u64) (u32) isonum_733(&idr->size[0]);     
          
        } else if(file_lba != 0xffffffff) break;

        p+= idr->length[0];
    }
    
    *flba = file_lba;

    if(file_lba == 0xffffffff) return -2;

    #ifdef USE_64BITS_LSEEK
    if(ps3ntfs_seek64(fd, ((s64) file_lba) * 2048LL, SEEK_SET)<0) goto err; 
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


void UTF8_to_UTF16(u8 *stb, u16 *stw);

int create_fake_file_iso(char *path, char *filename, u64 size)
{
    int len_string;
    u8 *mem = malloc(sizeof(build_iso_data));
    u16 *string = (u16 *) malloc(256);
    if(!mem || !string) return -1;

    char name[65];
    strncpy(name, filename, 64);

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

    return (char *) mem;
}