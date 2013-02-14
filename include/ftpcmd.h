// Functions specifically for socket/ftp use

#ifndef _openps3ftp_cmdfunc_
#define _openps3ftp_cmdfunc_

#define ssend(socket, str) send(socket, str, strlen(str), 0)

#define FD(socket) (socket & ~SOCKET_FD_MASK)

#define NIPQUAD(addr) \
         ((unsigned char *)&addr)[0], \
         ((unsigned char *)&addr)[1], \
         ((unsigned char *)&addr)[2], \
         ((unsigned char *)&addr)[3]

typedef void (*listcb)(sysFSDirent entry);

int recvline(int socket, char* str, int maxlen);
int slisten(int port, int backlog);
int sconnect(int *ret_s, const char ipaddr[16], int port);
void sclose(int *socket);
int recvfile(int socket, const char filename[1024], int bufsize, s64 startpos);
int sendfile(int socket, const char filename[1024], int bufsize, s64 startpos);
int slist(const char dir[1024], void (*listcb)(sysFSDirent entry));

#endif /* _openps3ftp_cmdfunc_ */
