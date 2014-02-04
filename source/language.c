/*
 * Language IrisManager by D_Skywalk
 *
 * Copyright (c) 2011 David Colmenero Aka D_Skywalk
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *
 *   Simple code for play with languages
 *    for ps3 scene ;)
 *
**/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "language.h"
#include "utils.h"
#include "language_ini_en_bin.h"
#include "language_ini_sp_bin.h"
#include "language_ini_fr_bin.h"
#include "language_ini_it_bin.h"
#include "language_ini_nw_bin.h"
#include "language_ini_ps_bin.h"
#include "language_ini_chs_bin.h"
#include "language_ini_cht_bin.h"
#include "language_ini_gm_bin.h"
#include "language_ini_por_bin.h"

#define LANGFILE_VERSION 2

typedef struct lngstr
{
  u32 code;
  char * strname;  
  char * strdefault; 
} t_lngstr;

t_lngstr lang_strings[] = 
{
    //MAIN
    // VIDEO - ADJUST
    {VIDEOADJUST_POSITION, "VIDEOADJUST_POSITION"    , "Use LEFT (-X) / RIGHT (+X) / UP (-Y) / DOWN (+Y) to adjust the screen" },
    {VIDEOADJUST_SCALEINFO, "VIDEOADJUST_SCALEINFO"   , "Video Scale X: %i Y: %i" },
    {VIDEOADJUST_EXITINFO, "VIDEOADJUST_EXITINFO"    , "Press CROSS to exit" },
    {VIDEOADJUST_DEFAULTS, "VIDEOADJUST_DEFAULTS"    , "Press CIRCLE to default values" },

    //SELECT - GAME FOLDER
    {GAMEFOLDER_WANTUSE, "GAMEFOLDER_WANTUSE"      , "Want to use" },
    {GAMEFOLDER_TOINSTALLNTR, "GAMEFOLDER_TOINSTALLNTR" , "to install the game?" },
    {GAMEFOLDER_USING, "GAMEFOLDER_USING"        , "Using" },
    {GAMEFOLDER_TOINSTALL, "GAMEFOLDER_TOINSTALL"    , "to install the game" },
    
    //DRAW SCREEN1
    { DRAWSCREEN_FAVSWAP, "DRAWSCREEN_FAVSWAP"      , "Favourites Swap" },
    { DRAWSCREEN_FAVINSERT, "DRAWSCREEN_FAVINSERT"    , "Favourites Insert" },
    { DRAWSCREEN_FAVORITES, "DRAWSCREEN_FAVORITES"    , "Favourites" },
    { DRAWSCREEN_PAGE, "DRAWSCREEN_PAGE"         , "Page" },
    { DRAWSCREEN_GAMES,"DRAWSCREEN_GAMES"        , "Games" },
    { DRAWSCREEN_PLAY, "DRAWSCREEN_PLAY"         , "Play" },
    { DRAWSCREEN_SOPTIONS, "DRAWSCREEN_SOPTIONS"     , "SELECT: Game Options" },
    { DRAWSCREEN_SDELETE, "DRAWSCREEN_SDELETE"      , "SELECT: Delete Favourite" },
    { DRAWSCREEN_STGLOPT, "DRAWSCREEN_STGLOPT"      , "START: Global Options" },
    { DRAWSCREEN_EXITXMB, "DRAWSCREEN_EXITXMB"      , "Exit to XMB?" },
    { DRAWSCREEN_CANRUNFAV, "DRAWSCREEN_CANRUNFAV"    , "Cannot run this favourite" },
    { DRAWSCREEN_MARKNOTEXEC, "DRAWSCREEN_MARKNOTEXEC"  , "Marked as non executable. Trying to install in HDD0 cache" },
    { DRAWSCREEN_MARKNOTEX4G, "DRAWSCREEN_MARKNOTEX4G"  ,"Marked as not executable - Contains splited files >= 4GB" },
    { DRAWSCREEN_GAMEINOFMNT, "DRAWSCREEN_GAMEINOFMNT"  , "I cannot find one folder to mount /dev_hdd0/game from USB" },
    { DRAWSCREEN_GAMEIASKDIR, "DRAWSCREEN_GAMEIASKDIR"  , "Want to create in /dev_usb00" },
    { DRAWSCREEN_GAMEICANTFD, "DRAWSCREEN_GAMEICANTFD"  , "I cannot find an USB device to mount /dev_hdd0/game from USB" },
    { DRAWSCREEN_GAMEIWLAUNCH, "DRAWSCREEN_GAMEIWLAUNCH" , "Want to launch the Game?" },
    { DRAWSCREEN_EXTEXENOTFND, "DRAWSCREEN_EXTEXENOTFND" , "external executable not found" },
    { DRAWSCREEN_EXTEXENOTCPY, "DRAWSCREEN_EXTEXENOTCPY" , "Use 'Copy EBOOT.BIN from USB' to import them." },
    { DRAWSCREEN_REQBR, "DRAWSCREEN_REQBR"        , "Required BR-Disc, Retry?" },

    //DRAW OPTIONS
    { DRAWGMOPT_OPTS, "DRAWGMOPT_OPTS"          , "Options" },
    { DRAWGMOPT_CFGGAME, "DRAWGMOPT_CFGGAME"       , "Config. Game" },
    { DRAWGMOPT_CPYGAME, "DRAWGMOPT_CPYGAME"       , "Copy Game" },
    { DRAWGMOPT_DELGAME, "DRAWGMOPT_DELGAME"       , "Delete Game" },
    { DRAWGMOPT_FIXGAME, "DRAWGMOPT_FIXGAME"       , "Fix File Permissions" },
    { DRAWGMOPT_TSTGAME, "DRAWGMOPT_TSTGAME"       , "Test Game" },
    { DRAWGMOPT_CPYEBOOTGAME, "DRAWGMOPT_CPYEBOOTGAME"  , "Copy EBOOT.BIN from USB" },
    { DRAWGMOPT_CPYTOFAV, "DRAWGMOPT_CPYTOFAV"      , "Copy to Favourites" },
    { DRAWGMOPT_DELFMFAV, "DRAWGMOPT_DELFMFAV"      , "Delete from Favourites" },
    { DRAWGMOPT_EXTRACTISO, "DRAWGMOPT_EXTRACTISO"      , "Extract ISO" },
    { DRAWGMOPT_BUILDISO, "DRAWGMOPT_BUILDISO"      , "Build ISO" },
    { DRAWGMOPT_MKISO, "DRAWGMOPT_MKISO"      , "Select a device to Build the ISO" },
    { DRAWGMOPT_XTISO, "DRAWGMOPT_XTISO"      , "Select a device to Extract the ISO" },
    { DRAWGMOPT_CPYISO, "DRAWGMOPT_CPYISO"      , "Select a device to Copy the ISO" },
    { DRAWGMOPT_GAMEUPDATE, "DRAWGMOPT_GAMEUPDATE"      , "Game Update" },

    { DRAWGMOPT_FIXCOMPLETE, "DRAWGMOPT_FIXCOMPLETE"   , "Fix Permissions Done!" },
    { DRAWGMOPT_CPYOK, "DRAWGMOPT_CPYOK"         , "copied successfully" },
    { DRAWGMOPT_CPYERR, "DRAWGMOPT_CPYERR"        , "Error copying" },
    { DRAWGMOPT_CPYNOTFND, "DRAWGMOPT_CPYNOTFND"     , "not found" },

    //DRAW_PSX
    { DRAWPSX_EMULATOR,  "DRAWPSX_EMULATOR"   , "Emulator" },
    { DRAWPSX_VIDEOPS,   "DRAWPSX_VIDEOPS"    , "PSX Video Options" },
    { DRAWPSX_SAVEASK,   "DRAWPSX_SAVEASK"    , "Save PSX options?" },
    { DRAWPSX_SAVED,     "DRAWPSX_SAVED"      , "PSX Options Saved" },
    { DRAWPSX_VIDEOTHER, "DRAWPSX_VIDEOTHER"  , "Video / Others" },
    { DRAWPSX_VIDEOMODE, "DRAWPSX_VIDEOMODE"  , "Video Mode" },
    { DRAWPSX_VIDEOASP,  "DRAWPSX_VIDEOASP"   , "Video Aspect (480/576)" },
    { DRAWPSX_FULLSCR,   "DRAWPSX_FULLSCR"    , "Full Screen" },
    { DRAWPSX_SMOOTH,    "DRAWPSX_SMOOTH"     , "Smooting" },
    { DRAWPSX_EXTROM,    "DRAWPSX_EXTROM"     , "External ROM" },
    { DRAWPSX_FORMAT,    "DRAWPSX_FORMAT"     , "Format Internal_MC" },
    { DRAWPSX_ASKFORMAT, "DRAWPSX_ASKFORMAT"  , "Want you format Internal_MC?\n\nYou LOSE the saves in this operation" },
    { DRAWPSX_ERRWRITING,"DRAWPSX_ERRWRITING" , "Error writing the file (Device full?)" },

    { DRAWPSX_BUILDISO,  "DRAWPSX_BUILDISO"   , "Building custom ISO..." },
    { DRAWPSX_ASKCHEATS, "DRAWPSX_ASKCHEATS"  , "PSX Cheat disc found\n\nWant you use it?" },
    { DRAWPSX_ERRCHEATS, "DRAWPSX_ERRCHEATS"  , "PSX Disc for Cheats cannot be launched\n\nwithout a PSX game" },
    { DRAWPSX_ERRSECSIZE,"DRAWPSX_ERRSECSIZE" , "Error: Different sector size in ISO files" },
    { DRAWPSX_ERRUNKSIZE,"DRAWPSX_ERRUNKSIZE" , "Error: Unknown Sector Size" },
    { DRAWPSX_DISCEJECT ,"DRAWPSX_DISCEJECT"  , "PSX CD Ejected" },
    { DRAWPSX_DISCORDER , "DRAWPSX_DISCORDER" , "Select Disc Order" },
    { DRAWPSX_PRESSOB   ,"DRAWPSX_PRESSOB"    , "Press CIRCLE to change the order" },
    { DRAWPSX_PRESSXB   ,"DRAWPSX_PRESSXB"    , "Press CROSS to launch de game" },
    { DRAWPSX_CHEATMAKE ,"DRAWPSX_CHEATMAKE"  , "PSX Cheat disc found, but different sector size\n\nWant you build one compatible?" },
    { DRAWPSX_COPYMC    ,"DRAWPSX_COPYMC"     , "Copying Memory Card to HDD0 device..." },
    { DRAWPSX_ERRCOPYMC ,"DRAWPSX_ERRCOPYMC"  , "Error copying the Memory Card to HDD0 device" },
    { DRAWPSX_PUTFNAME  ,"DRAWPSX_PUTFNAME"   , "Put a Folder Name:" },
    { DRAWPSX_FMUSTB    ,"DRAWPSX_FMUSTB"     , "Folder Name must be >=3 chars" },
    { DRAWPSX_PUTADISC  ,"DRAWPSX_PUTADISC"   , "Put a PSX disc and press YES to continue or NO to abort\n\nDisc to copy: " },
    { DRAWPSX_UNREC     ,"DRAWPSX_UNREC"      , "Unrecognized disc" },
    { DRAWPSX_ERROPENING,"DRAWPSX_ERROPENING" , "Error opening BDVD drive" },
    { DRAWPSX_ASKEFOLDER,"DRAWPSX_ASKEFOLDER" , "Folder Exits\n\nContinue?" },
    { DRAWPSX_ISOEXITS  ,"DRAWPSX_ISOEXITS"   , "exists\n\nSkip?" },

    //DRAW CONFIGS
    { DRAWGMCFG_CFGS, "DRAWGMCFG_CFGS"          , "Config. Game" },
    { DRAWGMCFG_DSK, "DRAWGMCFG_DSK"           , "Requires Disc" },
    { DRAWGMCFG_NO, "DRAWGMCFG_NO"            , "No" },
    { DRAWGMCFG_YES,"DRAWGMCFG_YES"           , "Yes" },
    { DRAWGMCFG_UPD, "DRAWGMCFG_UPD"           , "Online Updates" },
    { DRAWGMCFG_ON, "DRAWGMCFG_ON"            , "On" },
    { DRAWGMCFG_OFF, "DRAWGMCFG_OFF"           , "Off" },
    { DRAWGMCFG_EXTBOOT, "DRAWGMCFG_EXTBOOT"       , "Extern EBOOT.BIN" },
    { DRAWGMCFG_BDEMU, "DRAWGMCFG_BDEMU"         , "BD Emu (for USB)" },
    { DRAWGMCFG_EXTHDD0GAME, "DRAWGMCFG_EXTHDD0GAME"   , "Ext /dev_hdd0/game" },
    { DRAWGMCFG_SAVECFG, "DRAWGMCFG_SAVECFG"       , "Save Config" },

    //DRAW GLOBAL OPTIONS
    { DRAWGLOPT_OPTS, "DRAWGLOPT_OPTS"          , "Global Options" },
    { DRAWGLOPT_SCRADJUST, "DRAWGLOPT_SCRADJUST"     , "Video Adjust" },
    { DRAWGLOPT_CHANGEGUI, "DRAWGLOPT_CHANGEGUI"     , "Change Current GUI" },
    { DRAWGLOPT_CHANGEBCK, "DRAWGLOPT_CHANGEBCK"     , "Change Background Color" },
    { DRAWGLOPT_CHANGEDIR, "DRAWGLOPT_CHANGEDIR"     , "Change Game Directory" },
    { DRAWGLOPT_SWMUSICOFF, "DRAWGLOPT_SWMUSICOFF"    , "Switch Music Off" },
    { DRAWGLOPT_SWMUSICON, "DRAWGLOPT_SWMUSICON"     , "Switch Music On" },
    { DRAWGLOPT_INITFTP, "DRAWGLOPT_INITFTP"       , "Initialize FTP server" },
    { DRAWGLOPT_TOOLS, "DRAWGLOPT_TOOLS"         , "Tools" },
    { DRAWGLOPT_CREDITS, "DRAWGLOPT_CREDITS"       , "Credits" },
    { DRAWGLOPT_FTPINITED, "DRAWGLOPT_FTPINITED"     , "Server FTP initialized\nDo you want auto-enable FTP service on init?" },
    { DRAWGLOPT_FTPARINITED, "DRAWGLOPT_FTPARINITED"   , "Server FTP already initialized" },
    { DRAWGLOPT_FTPSTOPED, "DRAWGLOPT_FTPSTOPED"     , "Server FTP Stoped\nRemoved FTP service on init." },

    //DRAW TOOLS
    { DRAWTOOLS_TOOLS, "DRAWTOOLS_TOOLS"         , "Tools" },
    { DRAWTOOLS_DELCACHE, "DRAWTOOLS_DELCACHE"      , "Delete Cache Tool" },
   // { DRAWTOOLS_SECDISABLE, "DRAWTOOLS_SECDISABLE"    , "Press To Disable Syscall Security" },
   // { DRAWTOOLS_SECENABLE, "DRAWTOOLS_SECENABLE"     , "Press To Enable Syscall Security" },
    { DRAWTOOLS_LANGUAGE_1, "DRAWTOOLS_LANGUAGE_1"     , "English" },
    { DRAWTOOLS_LANGUAGE_2, "DRAWTOOLS_LANGUAGE_2"     , "Español" },
    { DRAWTOOLS_LANGUAGE_3, "DRAWTOOLS_LANGUAGE_3"     , "Française" },
    { DRAWTOOLS_LANGUAGE_4, "DRAWTOOLS_LANGUAGE_4"     , "Italiano" },
    { DRAWTOOLS_LANGUAGE_5, "DRAWTOOLS_LANGUAGE_5"     , "Norsk" },
    { DRAWTOOLS_LANGUAGE_6, "DRAWTOOLS_LANGUAGE_6"     , "Deutsch" },
    { DRAWTOOLS_LANGUAGE_7, "DRAWTOOLS_LANGUAGE_7"     , "Português" },
    { DRAWTOOLS_LANGUAGE_8, "DRAWTOOLS_LANGUAGE_8"     , "(test) فارسی" },
    { DRAWTOOLS_LANGUAGE_9, "DRAWTOOLS_LANGUAGE_9"     , "Chinese Simplified"},
    { DRAWTOOLS_LANGUAGE_10, "DRAWTOOLS_LANGUAGE_10"     , "Chinese Traditional"},
    { DRAWTOOLS_LANGUAGE_11, "DRAWTOOLS_LANGUAGE_11"     , "Custom (from file)"},
    
    { DRAWTOOLS_COPYFROM, "DRAWTOOLS_COPYFROM"     , "Copy from /dev_usb/iris to Iris folder"},
    { DRAWTOOLS_WITHBDVD, "DRAWTOOLS_WITHBDVD"     , "With BDVD Controller"},
    { DRAWTOOLS_NOBDVD,   "DRAWTOOLS_NOBDVD"       , "Without BDVD Device"},
    { DRAWTOOLS_NOBDVD2,   "DRAWTOOLS_NOBDVD2"     , "Disc - Less payload"},
    
    { DRAWTOOLS_PKGTOOLS, "DRAWTOOLS_PKGTOOLS"     , ".PKG Install" },
    { DRAWTOOLS_ARCHIVEMAN, "DRAWTOOLS_ARCHIVEMAN" , "Archive Manager" },
    { DRAWTOOLS_COVERSDOWN, "DRAWTOOLS_COVERSDOWN" , "Covers Download" },

    //MAIN - OTHERS
    { DRAWCACHE_CACHE, "DRAWCACHE_CACHE"         , "Delete Cache Tool" },
    { DRAWCACHE_ERRNEEDIT, "DRAWCACHE_ERRNEEDIT"     , "You need %1.2f GB free to install" },
    { DRAWCACHE_ASKTODEL, "DRAWCACHE_ASKTODEL"      , "Delete %s Cache?" },
    { PATCHBEMU_ERRNOUSB, "PATCHBEMU_ERRNOUSB"      , "BDEMU is only for USB devices" },
    { MOVEOBEMU_ERRSAVE, "MOVEOBEMU_ERRSAVE"       , "Error Saving:\n%s" },
    { MOVEOBEMU_ERRMOVE, "MOVEOBEMU_ERRMOVE"       , "Error Moving To:\n%s/PS3_GAME exists" },
    { MOVEOBEMU_MOUNTOK, "MOVEOBEMU_MOUNTOK"       , "BDEMU mounted in:\n%s/PS3_GAME" },
    { MOVETBEMU_ERRMOVE, "MOVETBEMU_ERRMOVE"       , "Error Moving To:\n%s exists" },

    //MAIN - GLOBAL
    { GLOBAL_RETURN, "GLOBAL_RETURN"           , "Return" },
    { GLOBAL_SAVED, "GLOBAL_SAVED"            , "File Saved" },


    //UTILS
    //FAST COPY ADD
    { FASTCPADD_FAILED, "FASTCPADD_FAILED"        , "Failed in fast_copy_process() ret" },
    { FASTCPADD_ERRTMFILES, "FASTCPADD_ERRTMFILES"    , "Too much files" },
    { FASTCPADD_FAILEDSTAT, "FASTCPADD_FAILEDSTAT"    , "Failed in stat()" },
    { FASTCPADD_ERROPEN, "FASTCPADD_ERROPEN"       , "Error Opening0 (write)" },
    { FASTCPADD_COPYING, "FASTCPADD_COPYING"       , "Copying" },
    { FASTCPADD_FAILFASTFILE, "FASTCPADD_FAILFASTFILE"  , "Failed in fast_files(fast_num_files).mem" },

    //FAST COPY PROCESS
    { FASTCPPRC_JOINFILE, "FASTCPPRC_JOINFILE"      , "Joining file" },
    { FASTCPPRC_COPYFILE, "FASTCPPRC_COPYFILE"      , "Copying. File" },
    { FASTCPPTC_OPENERROR, "FASTCPPTC_OPENERROR"     , "Error!!!!!!!!!!!!!!!!!!!!!!!!!\nFiles Opened %i\n Waiting 20 seconds to display fatal error" },

    //GAME TEST FILES
    { GAMETESTS_FOUNDINSTALL, "GAMETESTS_FOUNDINSTALL"  , "Found %s\n\nWant to install?" },
    { GAMETESTS_BIGFILE, "GAMETESTS_BIGFILE"       , "Big file" },
    { GAMETESTS_TESTFILE, "GAMETESTS_TESTFILE"      , "Test File" },
    { GAMETESTS_CHECKSIZE, "GAMETESTS_CHECKSIZE"     , "Checking Size of File" },

    //GAME DELETE FILES
    { GAMEDELFL_DELETED, "GAMEDELFL_DELETED"       , "Deleted" },
    { GAMEDELFL_DELETING, "GAMEDELFL_DELETING"      , "Deleting... File" },

    //GAME COPY
    { GAMECPYSL_GSIZEABCNTASK, "GAMECPYSL_GSIZEABCNTASK" , "Get Size: Aborted - Continue the copy?" },
    { GAMECPYSL_STARTED, "GAMECPYSL_STARTED"       , "Starting... \n copy" },
    { GAMECPYSL_SPLITEDHDDNFO, "GAMECPYSL_SPLITEDHDDNFO" , "%s\n\nSplit game copied in HDD0 (non bootable)" },
    { GAMECPYSL_SPLITEDUSBNFO, "GAMECPYSL_SPLITEDUSBNFO" , "%s\n\nSplit game copied in USB00%c (non bootable)" },
    { GAMECPYSL_DONE, "GAMECPYSL_DONE"          , "Done! Files Copied" },
    { GAMECPYSL_FAILDELDUMP, "GAMECPYSL_FAILDELDUMP"   , "Delete failed dump in" },

    //GAME CACHE COPY
    { GAMECHCPY_ISNEEDONEFILE, "GAMECHCPY_ISNEEDONEFILE" , "Sorry, but you needs to install at least a bigfile" },
    { GAMECHCPY_NEEDMORESPACE, "GAMECHCPY_NEEDMORESPACE" , "You have %.2fGB free and you needs %.2fGB\n\nPlease, delete Cache Entries" },
    { GAMECHCPY_NOSPACE, "GAMECHCPY_NOSPACE"       , "Sorry, you have %.2fGB free\n\nand you needs %.2fGB" },
    { GAMECHCPY_CACHENFOSTART, "GAMECHCPY_CACHENFOSTART" , "Cache Files: %.2fGB - Total Files: %.2fGB\n you save %.2fGB on HDD0 (%.2fGB Total)\n\nPress any button to Start" },
    { GAMECHCPY_FAILDELFROM, "GAMECHCPY_FAILDELFROM"   , "Delete Cache failed dump from" },

    //GAME DELETE
    { GAMEDELSL_WANTDELETE, "GAMEDELSL_WANTDELETE"    , "Want to delete from" },
    { GAMEDELSL_STARTED, "GAMEDELSL_STARTED"       , "Starting... \n delete" },
    { GAMEDELSL_DONE, "GAMEDELSL_DONE"          , "Done!  Files Deleted" },

    //GAME TEST
    // warning! don't translate GAMETSTSL_FINALNFO2 from english
    { GAMETSTSL_FINALNFO2, "GAMETSTSL_FINALNFO2"      , "Directories: %i Files: %i\nBig files: %i Split files: %i" },
    { GAMETSTSL_TESTED, "GAMETSTSL_TESTED"        , "Files Tested" },

    //GLOBAL UTILS
    { GLUTIL_SPLITFILE, "GLUTIL_SPLITFILE"        , "Split file" },
    { GLUTIL_WROTE, "GLUTIL_WROTE"            , "Wrote" },
    { GLUTIL_TIME, "GLUTIL_TIME"             , "Time" },
    { GLUTIL_TIMELEFT, "GLUTIL_TIMELEFT"         , "Time Left" },
    { GLUTIL_HOLDTRIANGLEAB, "GLUTIL_HOLDTRIANGLEAB"   , "Hold TRIANGLE to Abort" },
    { GLUTIL_HOLDTRIANGLESK, "GLUTIL_HOLDTRIANGLESK"   , "Hold TRIANGLE to Skip" },
    { GLUTIL_ABORTEDUSER, "GLUTIL_ABORTEDUSER"      , "Aborted by user" },
    { GLUTIL_ABORTED, "GLUTIL_ABORTED"          , "Aborted!!!" },
    { GLUTIL_XEXIT, "GLUTIL_XEXIT"            , "Press CROSS to Exit" },
    { GLUTIL_WANTCPYFROM, "GLUTIL_WANTCPYFROM"      , "Want to copy from" },
    { GLUTIL_WTO, "GLUTIL_WTO"              , "to" },
    
    // INSTALL .PKG
    { PKG_HEADER, "PKG_HEADER", "Install .PKG Utility -     Use CROSS to select and CIRCLE to exit"},
    { PKG_INSERTUSB, "PKG_INSERTUSB", "Insert the USB mass storage device"},
    { PKG_ERRTOBIG, "PKG_ERRTOBIG", ".PKG size too big or disk space small"},
    { PKG_WANTINSTALL, "PKG_WANTINSTALL", "Want to Install this .PKG file?"},
    { PKG_ERRALREADY, "ERRALREADY", "Error: .PKG already in the stack"},
    { PKG_ERRFULLSTACK, "PKG_ERRFULLSTACK", "Error: stack is full (max 16 entries)"},
    { PKG_ERRBUILD, "PKG_ERRBUILD", "Error Building .PKG process"},
    { PKG_COPYING, "PKG_COPYING", "Copying .PKG file to Iris Manager folder..."},
    { PKG_ERROPENING, "PKG_ERROPENING", "Error Opening .PKG file"},
    { PKG_ERRCREATING, "PKG_ERRCREATING", "Error Creating .PKG file"},
    { PKG_ERRREADING, "PKG_ERRREADING", "Error Reading .PKG file"},
    { PKG_ERRLICON, "PKG_ERRLICON", "Error Loading ICON file"},
    { PKG_ERRMOVING, "PKG_ERRMOVING", "Error moving .PKG"},
   

     // generic
    { OUT_OFMEMORY, "OUT_OFMEMORY", "Out of Memory"},
    { OPERATION_DONE, "OPERATION_DONE"     , "Done!" },
    { PLUG_STORAGE1, "PLUG_STORAGE1" , 
        "Remember you to plug an USB storage massive device to create the fake disc event\n\n"
        "Recuerda enchufar un dispositivo de almacenamiento masivo para crear el evento del falso disco" },
    { PLUG_STORAGE2, "PLUG_STORAGE2" , "Fake Disc Inserted\n\nFalso Disco Insertado" },
    
  
    
    { LANGSTRINGS_COUNT, "", ""}
};

char * language[LANGSTRINGS_COUNT];

static int lang_inited = 0;

int reverse_language = 0;

int open_language (int lang, char * filename) 
{

    int n;//, version;
    struct stat s;

    int elements = sizeof(lang_strings)/sizeof(t_lngstr);

    char get_reverse[64]="";

    reverse_language = 0;

    for (n = 0; n < LANGSTRINGS_COUNT; n++)
    {
        if(!lang_inited) 
            language[n] = (char*) malloc(MAX_CFGLINE_LEN);
        strncpy(language[n], "***ERROR****", MAX_CFGLINE_LEN-1);
        
    }

    lang_inited = 1;

    char * file_external = NULL;
    int file_size = 0;

    if(lang>=10) { // test external filename
        if(!stat(filename, &s))
            file_external = LoadFile(filename, &file_size);
        
        if(!file_external) lang = 0;
    }

    for (n = 0; n < elements; n++)
    {

        if(lang_strings[n].code == LANGSTRINGS_COUNT) break;

        if(lang>=10)
        {

               getConfigMemValueString((char *) file_external, file_size, "Language",
                "REVERSE", get_reverse, 63, "OFF");

               if(!strcasecmp((const char *) get_reverse, "on")) reverse_language = 1;

                // from external file
           
                strncpy(language[lang_strings[n].code], lang_strings[n].strdefault, MAX_CFGLINE_LEN-1);
                getConfigMemValueString((char *) file_external, file_size, "Language",
                    lang_strings[n].strname, language[lang_strings[n].code], MAX_CFGLINE_LEN-1, lang_strings[n].strdefault);
         
        } else {

            char *file_bin = (char *) language_ini_en_bin;
            file_size = language_ini_en_bin_size;

            switch(lang) {
                case 1: // sp
                    file_bin = (char *) language_ini_sp_bin;
                    file_size = language_ini_sp_bin_size;
                    break;
                case 2: // fr
                    file_bin = (char *) language_ini_fr_bin;
                    file_size = language_ini_fr_bin_size;
                    break;
                case 3: // it
                    file_bin = (char *) language_ini_it_bin;
                    file_size = language_ini_it_bin_size;
                    break;
                case 4: // nw
                    file_bin = (char *) language_ini_nw_bin;
                    file_size = language_ini_nw_bin_size;
                    break;
                case 5: // gm
                    file_bin = (char *) language_ini_gm_bin;
                    file_size = language_ini_gm_bin_size;
                    break;
                case 6: // por
                    file_bin = (char *) language_ini_por_bin;
                    file_size = language_ini_por_bin_size;
                    break;
                case 7: // ps
                    file_bin = (char *) language_ini_ps_bin;
                    file_size = language_ini_ps_bin_size;
                    break;
                case 8: // chs
                    file_bin = (char *) language_ini_chs_bin;
                    file_size = language_ini_chs_bin_size;
                    break;
                case 9: // cht
                    file_bin = (char *) language_ini_cht_bin;
                    file_size = language_ini_cht_bin_size;
                    break;
            }

            getConfigMemValueString((char *) file_bin, file_size, "Language",
                "REVERSE", get_reverse, 63, "OFF");

            if(!strcasecmp((const char *) get_reverse, "on")) reverse_language = 1;

            getConfigMemValueString((char *) file_bin, file_size, "Language",
                lang_strings[n].strname, language[lang_strings[n].code], MAX_CFGLINE_LEN-1, lang_strings[n].strdefault);
        }
    }

    if(file_external) free(file_external);

    return 0;

}

void close_language(void)
{
    int n;
    
    if(!lang_inited) return;

    //free memory
    for (n = 0; n < LANGSTRINGS_COUNT; n++)
        free(language[n]);
    lang_inited = 0;
}

