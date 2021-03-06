/* net.c
 *
 * This file is part of ftpd.
 *
 * This file handles:
 *
 *      PASV PORT
 *
 *
 * 01/25/95 Initial Release	Michael Temari, <temari@ix.netcom.com>
 */

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <net/netlib.h>
#include <net/hton.h>
#include <net/gen/in.h>
#include <net/gen/inet.h>
#include <net/gen/tcp.h>
#include <net/gen/tcp_io.h>
#include <net/gen/socket.h>
#include <net/gen/netdb.h>

#include "ftpd.h"
#include "access.h"
#include "net.h"

_PROTOTYPE(static void donothing, (int sig));

static char *msg425 = "425-Could not open data connection.\r\n";
static char *msg501 = "501 Syntax error in parameters.\r\n";

static int lpid = -1;

/* they must be behind a firewall or using a web browser */
int doPASV(buff)
char *buff;
{
nwio_tcpconf_t tcpconf;
nwio_tcpcl_t tcplopt;
char *tcp_device;
ipaddr_t ipaddr;
tcpport_t lport;
int s;

   if(ChkLoggedIn())
	return(GOOD);

   /* here we set up a connection to listen on */
   if((tcp_device = getenv("TCP_DEVICE")) == NULL)
	tcp_device = TCP_DEVICE;

   if(ftpdata_fd >= 0)
	close(ftpdata_fd);

   if((ftpdata_fd = open(tcp_device, O_RDWR)) < 0) {
	printf(msg425); 
	printf("425 Could not open tcp_device.  Error %s\r\n", strerror(errno));
	return(GOOD);
   }

   tcpconf.nwtc_flags = NWTC_LP_SEL | NWTC_SET_RA | NWTC_UNSET_RP;

   tcpconf.nwtc_remaddr = rmtipaddr;
   tcpconf.nwtc_remport = ntohs(0);
   tcpconf.nwtc_locport = ntohs(0);

   s = ioctl(ftpdata_fd, NWIOSTCPCONF, &tcpconf);
   if(s < 0) {
	printf(msg425);
	printf("425 Could not ioctl NWIOSTCPCONF. Error %s\r\n", strerror(errno));
	close(ftpdata_fd);
	ftpdata_fd = -1;
	return(GOOD);
   }

   s = ioctl(ftpdata_fd, NWIOGTCPCONF, &tcpconf);
   if(s < 0) {
	printf(msg425);
	printf("425 Could not NWIOGTCPCONF. Error %s\r\n", strerror(errno));
	close(ftpdata_fd);
	ftpdata_fd = -1;
	return(GOOD);
   }
   ipaddr = tcpconf.nwtc_locaddr;
   lport = tcpconf.nwtc_locport;

   /* Now lets fork a child to do the listening :-( */

   tcplopt.nwtcl_flags = 0;

   lpid = fork();
   if(lpid < 0) {
	printf(msg425);
	printf("425 Could not fork listener.  Error %s\r\n", strerror(errno));
	close(ftpdata_fd);
	ftpdata_fd = -1;
	return(GOOD);
   } else if(lpid == 0) {
	s = ioctl(ftpdata_fd, NWIOTCPLISTEN, &tcplopt);
	if(s < 0) 
		exit(-1);	/* tells parent listen failed */
	else
		exit(0);	/* tells parent listen okay */
   }

   /* wait for child to be listening */
   while(1) {
   	signal(SIGALRM, donothing);
   	alarm(1);
   	s = ioctl(ftpdata_fd, NWIOGTCPCONF, &tcpconf);
   	alarm(0);
   	if(s == -1) break;
   }

#define hiword(x)       ((u16_t)((x) >> 16))
#define loword(x)       ((u16_t)(x & 0xffff)) 
#define hibyte(x)       (((x) >> 8) & 0xff)
#define lobyte(x)       ((x) & 0xff)

   printf("227 Entering Passive Mode (%u,%u,%u,%u,%u,%u).\r\n",
		hibyte(hiword(htonl(ipaddr))), lobyte(hiword(htonl(ipaddr))),
		hibyte(loword(htonl(ipaddr))), lobyte(loword(htonl(ipaddr))),
		hibyte(htons(lport)), lobyte(htons(lport)));

   return(GOOD);
}

/* they want us to connect here */
int doPORT(buff)
char *buff;
{
u32_t ipaddr;
u16_t port;
int i;

   ipaddr = (u32_t)0;
   for(i = 0; i < 4; i++) {
	ipaddr = (ipaddr << 8) + (u32_t)atoi(buff);
	if((buff = strchr(buff, ',')) == (char *)0) {
		printf(msg501);
		return(GOOD);
	}
	buff++;
   }
   port = (u16_t)atoi(buff);
   if((buff = strchr(buff, ',')) == (char *)0) {
	printf(msg501);
	return(0);
   }
   buff++;
   port = (port << 8) + (u16_t)atoi(buff);

   dataaddr = ntohl(ipaddr);
   dataport = ntohs(port);

   printf("200 Port command okay.\r\n");

   return(GOOD);
}

/* connect, huh? */
int DataConnect()
{
nwio_tcpconf_t tcpconf;
nwio_tcpcl_t tcpcopt;
nwio_tcpcl_t tcplopt;
char *tcp_device;
int s, cs;

   if(ftpdata_fd >= 0) {
	while(1) {
		s = waitpid(lpid, &cs, 0);
		if((s == lpid) || (s < 0 && errno == ECHILD)) break;
	}
	lpid = -1;
	if(s < 0) {
		printf(msg425);
		printf("425 Child listener vanished.\r\n");
		close(ftpdata_fd);
		ftpdata_fd = -1;
		return(BAD);
	}
	if((cs & 0x00ff)) {
		printf(msg425);
		printf("425 Child listener failed %04x\r\n", cs);
		close(ftpdata_fd);
		ftpdata_fd = -1;
		return(BAD);
	}
	cs = (cs >> 8) & 0x00ff;
	if(cs) {
		printf(msg425);
		printf("425 Child listener returned %02x\r\n", cs);
		close(ftpdata_fd);
		ftpdata_fd = -1;
		return(BAD);
	}
	return(GOOD);
   }

   if((tcp_device = getenv("TCP_DEVICE")) == NULL)
	tcp_device = TCP_DEVICE;

   if(ftpdata_fd >= 0)
	close(ftpdata_fd);

   if((ftpdata_fd = open(tcp_device, O_RDWR)) < 0) {
	printf(msg425);
	printf("425 Could not open tcp_device. Error %s\r\n", strerror(errno));
	return(BAD);
   }

   tcpconf.nwtc_flags = NWTC_LP_SET | NWTC_SET_RA | NWTC_SET_RP;
   tcpconf.nwtc_remaddr = dataaddr;
   tcpconf.nwtc_remport = dataport;
   tcpconf.nwtc_locport = ntohs(20);

   s = ioctl(ftpdata_fd, NWIOSTCPCONF, &tcpconf);
   if(s < 0) {
	printf(msg425);
	printf("425 Could not ioctl NWIOSTCPCONF. Error %s\r\n", strerror(errno));
	close(ftpdata_fd);
	ftpdata_fd = -1;
	return(BAD);
   }

   s = ioctl(ftpdata_fd, NWIOGTCPCONF, &tcpconf);
   if(s < 0) {
	printf(msg425);
	printf("425 Could not ioctl NWIOGTCPCONF. Error %s\r\n", strerror(errno));
	close(ftpdata_fd);
	ftpdata_fd = -1;
	return(BAD);
   }

   tcpcopt.nwtcl_flags = 0;

   s = ioctl(ftpdata_fd, NWIOTCPCONN, &tcpcopt);
   if(s < 0) {
	printf(msg425);
	printf("425 Could not ioctl NWIOTCPCONN. Error %s\r\n", strerror(errno));
	close(ftpdata_fd);
	ftpdata_fd = -1;
	return(BAD);
   }

   s = ioctl(ftpdata_fd, NWIOGTCPCONF, &tcpconf);
   if(s < 0) {
	printf(msg425);
	printf("425 Could not ioctl NWIOGTCPCONF. Error %s\r\n", strerror(errno));
	close(ftpdata_fd);
	ftpdata_fd = -1;
	return(BAD);
   }

   return(GOOD);
}

/* Clean up stuff we did to get a data connection going */
int CleanUpData()
{
int s, cs;

   if(lpid >= 0)
	while(1) {
		s = waitpid(lpid, &cs, 0);
		if(s == lpid || (s == -1 && errno == ECHILD))
			break;
	}

   lpid = -1;

   return(GOOD);
}

void GetNetInfo()
{
nwio_tcpconf_t tcpconf;
int s;
struct hostent *hostent;

   /* Ask the system what our hostname is. */
   if(gethostname(myhostname, sizeof(myhostname)) < 0)
	strcpy(myhostname, "unknown");

   /* lets get our ip address and the clients ip address */
   s = ioctl(0, NWIOGTCPCONF, &tcpconf);
   if(s < 0) {
	printf("421 FTP service unable to get remote ip address. Closing.\r\n");
	fflush(stdout);
	exit(1);
   }

   myipaddr = tcpconf.nwtc_locaddr;
   myport = tcpconf.nwtc_locport;
   rmtipaddr = tcpconf.nwtc_remaddr;
   rmtport = tcpconf.nwtc_remport;

   /* Look up the host name of the remote host. */
   hostent = gethostbyaddr((char *) &rmtipaddr, sizeof(rmtipaddr), AF_INET);
   if(!hostent) {
	printf("421-FTP service unable to translate your IP address (%s)\r\n",
		inet_ntoa(rmtipaddr));
	printf("421 to your host name. Closing.\r\n");
	fflush(stdout);
	exit(1);
   }
   strncpy(rmthostname, hostent->h_name, sizeof(rmthostname)-1);
   rmthostname[sizeof(rmthostname)-1] = '\0';
}

static void donothing(sig)
int sig;
{
}
