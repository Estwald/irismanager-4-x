#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <math.h>
#include "http/https.h"
#include "utils.h"
#include <sys/file.h>

#include "language.h"

extern char temp_buffer[8192];
extern int firmware;
extern char self_path[MAXPATHLEN];

static msgType mdialogprogress =   MSG_DIALOG_SINGLE_PROGRESSBAR | MSG_DIALOG_MUTE_ON;

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


static void single_bar(char *caption) 
{
    progress_action = 0;

    msgDialogOpen2(mdialogprogress, caption, progress_callback, (void *) 0xadef0044, NULL);
    
    msgDialogProgressBarReset(MSG_PROGRESSBAR_INDEX0);

    sysUtilCheckCallback();tiny3d_Flip(); 
}


int param_sfo_app_ver(char * path, char *app_ver)
{
	s32 fd;
    u64 bytes;
    u64 position = 0LL;
    
    unsigned char *mem = NULL;
	
    if(!sysLv2FsOpen(path, 0, &fd, S_IRWXU | S_IRWXG | S_IRWXO, NULL, 0)) {
		unsigned len, pos, str;
		

        sysLv2FsLSeek64(fd, 0, 2, &position);
		len = (u32) position;

		mem = (unsigned char *) malloc(len+16);
		if(!mem) {sysLv2FsClose(fd); return -2;}

		memset(mem, 0, len+16);

		sysLv2FsLSeek64(fd, 0, 0, &position);
		
        if(sysLv2FsRead(fd, mem, len, &bytes)!=0) bytes =0LL;

        len = (u32) bytes;

		sysLv2FsClose(fd);

		str= (mem[8]+(mem[9]<<8));
		pos= (mem[0xc]+(mem[0xd]<<8));

		int indx=0;

		while(str<len) {
			if(mem[str]==0) break;

            if(!strcmp((char *) &mem[str], "APP_VER")) {
               
				strncpy(app_ver, (char *) &mem[pos], 5);
                app_ver[5] = 0;

                break;
                
			}

			while(mem[str]) str++;str++;
			pos+=(mem[0x1c+indx]+(mem[0x1d+indx]<<8));
			indx+=16;
		}

    if(mem) free(mem);

    return 0;
        
    }

	return -1;

}

#include "event_threads.h"


static volatile struct f_async {
    int flags;
    FILE *fp;
    void * mem;
    int ret;
    int size;

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

             ret = (fwrite(v->mem, 1, v->size, v->fp) != v->size);
             v->ret = -ret;

            free(v->mem); v->mem = 0;

        }

        if(ret) flags|= ASYNC_ERROR;

        flags &= ~ASYNC_ENABLE;

        v->flags = flags;
    }
}
static int use_async_fd = 128;


static int download_update(char *url, char *file, int mode, u64 *size)
{
    int flags = 0;
    int ret = 0;
    void *http_p = NULL;
	void *ssl_p = NULL;
    void *uri_p = NULL;
    void *cert_buffer = NULL;
    httpUri uri;
    s32 transID = 0;
    s32 clientID;
    int pool_size = 0;
    int recv = -1;
    u64 length = 0;
    int cert_size = 0;
    int code = 0;
    FILE *fp = NULL;
    float parts = 0;
    float cpart;
    char buffer[16384];

    use_async_fd = 128;
    my_f_async.flags = 0;

    if(mode == 2) {
        if(size) sprintf(temp_buffer + 4096, "File Size: %u MB\n%s", (u32) (*size/0x100000ULL), strrchr(file, '/') + 1);
        else sprintf(temp_buffer + 4096, "File: %s", strrchr(file, '/') + 1);

        single_bar(temp_buffer + 4096);
    }

    http_p = malloc(0x10000);
    if(!http_p) {ret= -1; goto err;}

    ssl_p = malloc(0x40000);
    if(!ssl_p) {ret= -2; goto err;}

    ret = httpInit(http_p, 0x10000);
	if(ret < 0) goto err;
    flags|= 1;

    ret = sslInit(ssl_p, 0x40000);
    if(ret < 0) goto err;
    flags|= 2; 
    
    ret = sslCertificateLoader(SSL_LOAD_CERT_SCE, NULL, 0, &cert_size);
    if(ret < 0) goto err;

    cert_buffer = malloc(cert_size);
    if(!cert_buffer) {ret = -3; goto err;}

    ret = sslCertificateLoader(SSL_LOAD_CERT_SCE, cert_buffer, cert_size, NULL);
    if(ret < 0) goto err;

    httpsData caList;

    caList.ptr = cert_buffer;
	caList.size = cert_size;

    ret = httpsInit(1, (const httpsData *) &caList);
    if(ret < 0) goto err;
    flags|= 4;

    ret = httpCreateClient(&clientID);
    if(ret < 0) goto err;
    flags|= 8;

	ret = httpUtilParseUri(NULL, url, NULL, 0, &pool_size);
	if (ret < 0) goto err;

	uri_p = malloc(pool_size);
	if (!uri_p) goto err;

	ret = httpUtilParseUri(&uri, url, uri_p, pool_size, NULL);
	if (ret < 0) goto err;

    ret = httpCreateTransaction(&transID, clientID, HTTP_METHOD_GET, &uri);
	if (ret < 0) goto err;
    
    free(uri_p); 
    uri_p = NULL;

	ret = httpSendRequest(transID, NULL, 0, NULL);
	if (ret < 0) goto err;
		
    ret = httpResponseGetStatusCode(transID, &code);
    if (ret < 0) goto err;
		
	if (code == 404 || code == 403) {ret=-4; goto err;}

	ret = httpResponseGetContentLength(transID, &length);
	if (ret < 0) {
		if (ret == HTTP_STATUS_CODE_No_Content) {
            length = 0ULL;
            ret = 0;
		} else goto err;
	}

    if(size) *size = length;

    if(mode == 1) goto err; // get only the size

    if(mode == 2) {
        parts = (length == 0) ? 0.0f : 100.0f / ((double) length / (double) sizeof(buffer));
        cpart = 0;
    }
    
    fp = fopen(file, "wb");
	if(!fp) goto err;
	
    int acum = 0;
	while (recv != 0 && length != 0ULL) {
        memset(buffer, 0x0, sizeof(buffer));
		if (httpRecvResponse(transID, buffer, sizeof(buffer) - 1, &recv) < 0) {fclose(fp); ret = -5; goto err;}
		if (recv == 0) break;
		if (recv > 0) {
			//if(fwrite(buffer, 1, recv, fp) != recv) {fclose(fp); fp = NULL; ret = -6; goto err;}
            ///////////////////

        loop_write:
            if(use_async_fd == 128) {
                use_async_fd = 1;
                my_f_async.flags = 0;
                my_f_async.size = recv;
                my_f_async.fp = fp;
                my_f_async.mem = malloc(recv);
                if(my_f_async.mem) memcpy(my_f_async.mem, (void *) buffer, recv);        
                my_f_async.ret = -1;
                my_f_async.flags = ASYNC_ENABLE;
                event_thread_send(0x555ULL, (u64) my_func_async, (u64) &my_f_async);

            } else {
             
                if(!(my_f_async.flags & ASYNC_ENABLE)) {

                    if(my_f_async.flags & ASYNC_ERROR) {
                        fclose(fp); fp = NULL; ret = -6; goto err;
                    }
                   
                    my_f_async.flags = 0;
                    my_f_async.size = recv;
                    my_f_async.fp = fp;
                    my_f_async.mem = malloc(recv);
                    if(my_f_async.mem) memcpy(my_f_async.mem, (void *) buffer, recv);  
                    my_f_async.ret = -1;
                    my_f_async.flags = ASYNC_ENABLE;
                    event_thread_send(0x555ULL, (u64) my_func_async, (u64) &my_f_async);
                    
                } else {
                    goto loop_write;
                }
            }
            ///////////////////
            length -= recv;
            acum+= recv;
		}

        if(mode == 2 && progress_action == 2) {ret = -0x555; goto err;}

        pad_last_time = 0;
    
        if(mode == 2) {

            if(acum >= sizeof(buffer)) {
                acum-= sizeof(buffer);
                cpart += parts;
                if(cpart >= 1.0f) {
                    update_bar((u32) cpart);
                    cpart-= (float) ((u32) cpart); 
                }
            }
        }
	}

 
	ret = 0;
	
err:

    if(my_f_async.flags & ASYNC_ENABLE){
        wait_event_thread();
        if(my_f_async.flags  & ASYNC_ERROR) {
            
            if(fp) fclose(fp); fp = NULL; ret = -6;
        }
        my_f_async.flags = 0;
    }

    event_thread_send(0x555ULL, (u64) 0, 0);

    if(mode == 2) {
        msgDialogAbort();
    }

    if(fp) {
        fclose(fp);
        if(ret < 0) unlink_secure(file);
    }

    if(transID) httpDestroyTransaction(transID);
    if(flags & 8) httpDestroyClient(clientID);
    if(flags & 4) httpsEnd();
    if(flags & 2) sslEnd();
    if(flags & 1) httpEnd();
    if(http_p) free(http_p);
    if(ssl_p)  free(ssl_p);
    if(cert_buffer) free(cert_buffer);
    if(uri_p) free(uri_p); 
    return ret;
}


static int download_file(char *url, char *file, int mode, u64 *size)
{
    int flags = 0;
    int ret = 0;
    void *http_p = NULL;
    void *uri_p = NULL;
    httpUri uri;
    s32 transID = 0;
    s32 clientID;
    int pool_size = 0;
    int recv = -1;
    u64 length = 0;
    int code = 0;
    FILE *fp = NULL;
    float parts = 0;
    float cpart;
    char buffer[16384];

    use_async_fd = 128;
    my_f_async.flags = 0;

    if(mode == 2) {
        if(size) sprintf(temp_buffer + 4096, "File Size: %u MB\n%s", (u32) (*size/0x100000ULL), strrchr(file, '/') + 1);
        else sprintf(temp_buffer + 4096, "File: %s", strrchr(file, '/') + 1);

        single_bar(temp_buffer + 4096);
    }

    http_p = malloc(0x10000);
    if(!http_p) {ret= -1; goto err;}

    ret = httpInit(http_p, 0x10000);
	if(ret < 0) goto err;
    flags|= 1;

    ret = httpCreateClient(&clientID);
    if(ret < 0) goto err;
    flags|= 2;

    httpClientSetConnTimeout(clientID, 10000000);

	ret = httpUtilParseUri(NULL, url, NULL, 0, &pool_size);
	if (ret < 0) goto err;

	uri_p = malloc(pool_size);
	if (!uri_p) goto err;

	ret = httpUtilParseUri(&uri, url, uri_p, pool_size, NULL);
	if (ret < 0) goto err;

    ret = httpCreateTransaction(&transID, clientID, HTTP_METHOD_GET, &uri);
	if (ret < 0) goto err;
    
    free(uri_p); 
    uri_p = NULL;

	ret = httpSendRequest(transID, NULL, 0, NULL);
	if (ret < 0) goto err;
		
    ret = httpResponseGetStatusCode(transID, &code);
    if (ret < 0) goto err;
		
	if (code == 404 || code == 403) {ret=-4; goto err;}

	ret = httpResponseGetContentLength(transID, &length);
	if (ret < 0) {
		if (ret == HTTP_STATUS_CODE_No_Content) {
            length = 0ULL;
            ret = 0;
		} else goto err;
	}

    if(size) *size = length;

    if(mode == 1) goto err; // get only the size

    if(mode == 2) {
        parts = (length == 0) ? 0.0f : 100.0f / ((double) length / (double) sizeof(buffer));
        cpart = 0;
    }
    
    fp = fopen(file, "wb");
	if(!fp) goto err;
	
    int acum = 0;
	while (recv != 0 && length != 0ULL) {
        memset(buffer, 0x0, sizeof(buffer));
		if (httpRecvResponse(transID, buffer, sizeof(buffer) - 1, &recv) < 0) {fclose(fp); ret = -5; goto err;}
		if (recv == 0) break;
		if (recv > 0) {
			//if(fwrite(buffer, 1, recv, fp) != recv) {fclose(fp); fp = NULL; ret = -6; goto err;}
            ///////////////////

        loop_write:
            if(use_async_fd == 128) {
                use_async_fd = 1;
                my_f_async.flags = 0;
                my_f_async.size = recv;
                my_f_async.fp = fp;
                my_f_async.mem = malloc(recv);
                if(my_f_async.mem) memcpy(my_f_async.mem, (void *) buffer, recv);        
                my_f_async.ret = -1;
                my_f_async.flags = ASYNC_ENABLE;
                event_thread_send(0x555ULL, (u64) my_func_async, (u64) &my_f_async);

            } else {
             
                if(!(my_f_async.flags & ASYNC_ENABLE)) {

                    if(my_f_async.flags & ASYNC_ERROR) {
                        fclose(fp); fp = NULL; ret = -6; goto err;
                    }
                   
                    my_f_async.flags = 0;
                    my_f_async.size = recv;
                    my_f_async.fp = fp;
                    my_f_async.mem = malloc(recv);
                    if(my_f_async.mem) memcpy(my_f_async.mem, (void *) buffer, recv);  
                    my_f_async.ret = -1;
                    my_f_async.flags = ASYNC_ENABLE;
                    event_thread_send(0x555ULL, (u64) my_func_async, (u64) &my_f_async);
                    
                } else {
                    goto loop_write;
                }
            }
            ///////////////////
            length -= recv;
            acum+= recv;
		}

        if(mode == 2 && progress_action == 2) {ret = -0x555; goto err;}

        pad_last_time = 0;
    
        if(mode == 2) {

            if(acum >= sizeof(buffer)) {
                acum-= sizeof(buffer);
                cpart += parts;
                if(cpart >= 1.0f) {
                    update_bar((u32) cpart);
                    cpart-= (float) ((u32) cpart); 
                }
            }
        }
	}

 
	ret = 0;
	
err:

    if(my_f_async.flags & ASYNC_ENABLE){
        wait_event_thread();
        if(my_f_async.flags  & ASYNC_ERROR) {
            
            if(fp) fclose(fp); fp = NULL; ret = -6;
        }
        my_f_async.flags = 0;
    }

    event_thread_send(0x555ULL, (u64) 0, 0);

    if(mode == 2) {
        msgDialogAbort();
    }

    if(fp) {
        fclose(fp);
        if(ret < 0) unlink_secure(file);
    }

    if(transID) httpDestroyTransaction(transID);
    if(flags & 2) httpDestroyClient(clientID);
    if(flags & 1) httpEnd();
    if(http_p) free(http_p);
    if(uri_p) free(uri_p); 
    return ret;
}


int locate_xml(u8 *mem, int pos, int size, char *key, int *last)
{
    int start = 0;
    int end = 0;

    int sig = 0;

    *last = 0;

    while(pos < size) {
        if(mem[pos] == '"' || sig) {
            if(mem[pos] == '"') sig^=1;
            pos++; continue;
        }

        if(mem[pos] == '<') return -1;
        if(!strncmp((char *) &mem[pos], "/>", 2) || mem[pos] == '>') return -2;
        if(!strncmp((char *) &mem[pos], key, strlen(key))) {
            pos+= strlen(key); break;
        }
        
        pos++;
    }

    while(pos < size) {
        if(mem[pos] == '<') return -1;
        if(!strncmp((char *) &mem[pos], "/>", 2) || mem[pos] == '>') return -2;
        if(mem[pos] == '=') {
            pos++; break;
        }
        
        pos++;
    }

    while(pos < size) {
        if(mem[pos] == '<') return -1;
        if(!strncmp((char *) &mem[pos], "/>", 2) || mem[pos] == '>') return -2;
        if(mem[pos] == '"') {
            start = pos;
            pos++;
            break;
        }
        
        pos++;
    }

    while(pos < size) {
        if(mem[pos] == '<') return -1;
        if(!strncmp((char *) &mem[pos], "/>", 2) || mem[pos] == '>') return -2;
        if(mem[pos] == '"') {
            end = pos;
            break;
        }
        
        pos++;
    }


    if(pos >= size) return -3;
    
    *last = end;
    return start;
}

int game_update(char *title_id) 
{
    char id[10];

    char version[6];
    char ver_app[6];
    char system[8];

    int list[128][2];

    int max_list = 0;

    int ret;

    //sprintf(temp_buffer, "http://www.covers-examples.com/ps3/%s.jpg", id);
    //sprintf(temp_buffer + 1024, "%s/temp.jpg", self_path);

    //download_file(temp_buffer, temp_buffer + 1024, 0, NULL);

    if(DrawDialogYesNo("Want you update the Game?") != 1) return 0;

    memcpy(id, title_id, 4);
    id[4] = title_id[5]; id[5] = title_id[6]; id[6] = title_id[7]; id[7] = title_id[8]; id[8] = title_id[9]; id[9] = 0;

    strcpy(ver_app, "00.00");
    
    sprintf(temp_buffer, "/dev_hdd0/game/%s/PARAM.SFO", id);
    param_sfo_app_ver(temp_buffer, ver_app);

    sprintf(temp_buffer, "https://a0.ww.np.dl.playstation.net/tpl/np/%s/%s-ver.xml", id, id);
    sprintf(temp_buffer + 1024, "%s/temp.xml", self_path);

    ret = download_update(temp_buffer, temp_buffer + 1024, 0, NULL);

    if(ret < 0) {
        sprintf(temp_buffer, "Error 0x%x downloading XML", ret);
        DrawDialogOK(temp_buffer);
        return 0;
    }

    int file_size = 0;
    u8 *mem = (u8 *) LoadFile(temp_buffer + 1024, &file_size);

    if(!mem || file_size== 0) {
        DrawDialogOK("No update found for this game");
        return 0;
    }

    int n = 0;
    int m, l;
    int k = 0;

    while(n < file_size) {
        if(mem[n] != '<') {n++; continue;}
        if(!strncmp((char *) &mem[n], "/>", 2) || mem[n] == '>') {n++; continue;}
        
        if(strncmp((char *) &mem[n], "<package ", 9)) {n++; continue;}

        n+= 9;
        
        strcpy(version, "00.00");
        strcpy(system,  "00.0000");

        m = locate_xml(mem, n, file_size, "version", &l);
        if(m < 0) goto no_ver; // not found
        if(l > k) k = l;

        m++; l-= m;
        if(l<=0) goto no_ver; // empty 

        strncpy(version, (char *) &mem[m], 5);
        version[5] = 0;

    no_ver:
        m = locate_xml(mem, n, file_size, "url", &l);
        if(m < 0) continue;
        if(l > k) k = l;

        m++; l-= m;
        if(l<=0) continue; // empty 
        if(l>1023) l = 1023;

        strncpy(temp_buffer, (char *) &mem[m], l);
        temp_buffer[l] = 0;

        list[max_list][0] = m;
        list[max_list][1] = l;

        m = locate_xml(mem, n, file_size, "ps3_system_ver", &l);
        if(m < 0) goto no_system; // not found
        if(l > k) k = l;

        m++; l-= m;
        if(l<=0) goto no_system; // empty 

        strncpy(system, (char *) &mem[m], 7);
        system[7] = 0;


    no_system:

        if(strcmp(version, ver_app)<=0) {n = k; continue;}

        char * o = strrchr(temp_buffer, '/');
        if (o) {
            sprintf(temp_buffer + 1024, "Download this update?\n\nVersion: %s for System Ver %s\n\n%s", version, system, o + 1);

            if(DrawDialogYesNo2(temp_buffer + 1024) == 1) {

                max_list++; if(max_list >=128) break;

            } else break; // to avoid download the next package
        }


        n = k;


    }

    int downloaded = 0;

    if(max_list > 0) {

        DrawDialogOKTimer("Downloading the updates\n\nWait to finish", 2000.0f);

        sprintf(temp_buffer + 1024, "%s/PKG", self_path);
        mkdir_secure(temp_buffer + 1024);

        for(n = 0; n < max_list; n++) {
            struct stat s;
            u64 pkg_size = 0;

            strncpy(temp_buffer, (char *) &mem[list[n][0]], list[n][1]);
            temp_buffer[list[n][1]] = 0;

            char * o = strrchr(temp_buffer, '/');
            if (!o) continue;

            sprintf(temp_buffer + 1024, "%s/PKG%s", self_path, o); 

            if(!stat(temp_buffer + 1024, &s)) {downloaded++; continue;} // if exist skip

            // get size
            ret = download_update(temp_buffer, temp_buffer + 1024, 1, &pkg_size);

            if(ret < 0) {
                sprintf(temp_buffer, "Error 0x%x downloading XML", ret);
                DrawDialogOK(temp_buffer);
                free(mem);
                return downloaded;
            }
            
            {
                u32 blockSize;
                u64 freeSize;
                u64 free_hdd0;
                sysFsGetFreeSize("/dev_hdd0/", &blockSize, &freeSize);
                free_hdd0 = ( ((u64)blockSize * freeSize));
                if((pkg_size + 0x40000000LL) >= (s64) free_hdd0) {
                    sprintf(temp_buffer + 3072, "%s\n\n%s\n\n%s%1.2f GB", "Error: no space in HDD0 to copy it", temp_buffer + 1024, "You need ", 
                        ((double) (pkg_size + 0x40000000LL - free_hdd0))/(1024.0*1024.*1024.0));
                    DrawDialogOK(temp_buffer + 3072);
                    free(mem);
                    return downloaded;
                }
            }


            // download

            ret = download_update(temp_buffer, temp_buffer + 1024, 2, &pkg_size);

            if(ret == -0x555) {
                DrawDialogOK(language[GLUTIL_ABORTEDUSER]);
                free(mem);
                return downloaded;
            } else if(ret < 0) {
                sprintf(temp_buffer, "Error 0x%x downloading XML", ret);
                free(mem);
                return downloaded;
            } else downloaded++;


        }
    } else DrawDialogOK("No update found for this game");

    free(mem);

    return downloaded;
}