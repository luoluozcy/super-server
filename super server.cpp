// super server.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "super server.h"

#include <process.h>
#include <winsock2.h>
//#include <fstream>
#include <string>
#include <iostream>

//#include "ftp.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define	UDP_SERV	0
#define	TCP_SERV	1

using namespace std;
struct SocketInfo {
	//char	*sv_name;
//	char	sv_useTCP;
	SOCKET	s;
	bool bPasv;
	bool use;
	sockaddr_in fsin;
	char buffRecv[1024];	/* "input" buffer; any size > 1 packet	*/
	char directory[256];
	SocketInfo();
//	void	(*sv_func)(SOCKET);
};
SocketInfo::SocketInfo()
{
  use=true;
}

struct service {
	char	*sv_name;
	char	sv_useTCP;
	SOCKET	sv_sock;
	void	(*sv_func)(void *);
};



void	TCPechod(void *), TCPchargend(void *), TCPdaytimed(void *),	TCPtimed(void *);

//void	UDPechod(SOCKET), UDPchargend(SOCKET), UDPdaytimed(SOCKET),	UDPtimed(SOCKET);
int  ftp(SOCKET);
int http(SOCKET);
SOCKET	passiveTCP(const char *, int);
SOCKET	passiveUDP(const char *);
void	errexit(const char *, ...);
void	doTCP(struct service *);
void UDPtimed(void *);
void UDPchargend(void * );
void	UDPechod(void *), UDPdaytimed(void *);
struct service svent[] =
	{	{ "echo", TCP_SERV, INVALID_SOCKET, TCPechod },
		{ "chargen", TCP_SERV, INVALID_SOCKET, TCPchargend },
		{ "daytime", TCP_SERV, INVALID_SOCKET, TCPdaytimed },
		{ "time", TCP_SERV, INVALID_SOCKET, TCPtimed },
		{ "echo", UDP_SERV, INVALID_SOCKET, UDPechod },
		{ "chargen", UDP_SERV, INVALID_SOCKET, UDPchargend },
		{ "daytime", UDP_SERV, INVALID_SOCKET, UDPdaytimed },
		{ "time", UDP_SERV, INVALID_SOCKET, UDPtimed },
	//	{ "http", TCP_SERV, INVALID_SOCKET, TCPhttpd },
	//	{ "ftp", TCP_SERV, INVALID_SOCKET, TCPftpd },
		{ 0, 0, 0, 0 },
	};


#define WSVERS		MAKEWORD(2, 0)
#define	QLEN		   6
#define	LINELEN		 128

extern	u_short	portbase;	/* from passivesock()	*/
//char tcproot[128];
//char ftproot[128];
//char user[128];
//char pass[128];
sockaddr addr;
struct sockaddr_in fsin;
struct SocketInfo a[1024];
//--------------------------
/////////////////////////////////////////////////////////////////////////////
// The one and only application object

CWinApp theApp;

using namespace std;

int ftpd();
int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int nRetCode = 0;

	// initialize MFC and print and error on failure
	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		// TODO: change error code to suit your needs
		cerr << _T("Fatal Error: MFC initialization failed") << endl;
		nRetCode = 1;
	}
	else
	{  
	
     
		struct service	*psv;		/* service table pointer	*/
    	fd_set		afds, rfds;	/* readable file descriptors	*/
 	WSADATA		wsdata;


	switch (argc) {
	case 1:
		break;
	case 2:
		portbase = (u_short) atoi(argv[1]);
		break;
	default:
		errexit("usage: superd [portbase]\n");
	}
	if (WSAStartup(WSVERS, &wsdata))
		errexit("WSAStartup failed\n");
SOCKET tmp;
	//CHttpProtocol* http=new CHttpProtocol();
	
	
if (_beginthread((void (*)(void *))http, 0, (void *)&tmp)== (unsigned long) -1)
		errexit("_beginthread: %s\n", strerror(errno));
//if (_beginthread((void (*)(void *))psv->sv_func, 0, (void *)ssock)== (unsigned long) -1)
if (_beginthread((void (*)(void *))ftpd, 0, (void *)&tmp)== (unsigned long) -1)
		errexit("_beginthread: %s\n", strerror(errno));

	FD_ZERO(&afds);
	for (psv = &svent[0]; psv->sv_name; ++psv) {
		if (psv->sv_useTCP)
			psv->sv_sock = passiveTCP(psv->sv_name, QLEN);
		else
			psv->sv_sock = passiveUDP(psv->sv_name);
		FD_SET(psv->sv_sock, &afds);
	}



	while (1) {
		memcpy(&rfds, &afds, sizeof(rfds));
		if (select(FD_SETSIZE, &rfds, (fd_set *)0, (fd_set *)0,(struct timeval *)0) == SOCKET_ERROR)
			errexit("select error: %d\n", GetLastError());
		for (psv=&svent[0]; psv->sv_name; ++psv) {
			if (FD_ISSET(psv->sv_sock, &rfds)) {
				if (psv->sv_useTCP)
					doTCP(psv);
			//	else
				//	psv->sv_func(psv->sv_sock);
         	else{
					int i;
				//	do{
						for(i=0;i<1024;i++) if(a[i].use) break;
				//	}while(i==1024);		//寻找私有变量存储空间
					a[i].s=psv->sv_sock;
					a[i].use=false;
					a[i].bPasv=true;	//为各个线程保存私有变量
					strcpy(a[i].directory,"UDP");
					strcat(a[i].directory,psv->sv_name);
					int alen=sizeof(a[i].fsin);
					if(recvfrom(a[i].s,a[i].buffRecv,sizeof(a[i].buffRecv),0,(struct sockaddr *)&(a[i].fsin),&alen)!=SOCKET_ERROR);
				    	_beginthread((void (*)(void *))psv->sv_func, 0, (void *)&(a[i]));


			}
			}
		}
	}

	
	//	int i=ftpd();

	}

	return nRetCode;
}

/*------------------------------------------------------------------------
 * doTCP - handle a TCP service connection request
 *------------------------------------------------------------------------
 */
	/* the request from address	*/
void doTCP(struct service *psv){
	
	int		alen;		/* from-address length		*/
	SOCKET		ssock;

	alen = sizeof(fsin);
	ssock = accept(psv->sv_sock, (struct sockaddr *)&fsin, &alen);

	char        LocalAddr[80];
	gethostname(LocalAddr,sizeof(LocalAddr));
	LPHOSTENT	lp=gethostbyname(LocalAddr);
	struct in_addr *hostAddr=((LPIN_ADDR)lp->h_addr);


	int len=sizeof(addr);
	getsockname(ssock,&addr,&len);
	

	printf("%s  %d  %s:",inet_ntoa(fsin.sin_addr),fsin.sin_port,LocalAddr);
	printf("%s  %s\n",inet_ntoa(((sockaddr_in *)&addr)->sin_addr),psv->sv_name);
	if (ssock == INVALID_SOCKET)
		errexit("accept: %d\n", GetLastError());
     int i;
do{  
		for( i=0;i<1024;i++) if(a[i].use) break;

	}while(i==1024);

	a[i].s=ssock;
	a[i].use=false;
	a[i].fsin=fsin;
	printf("test1\n");
	if(_beginthread((void (*)(void *))psv->sv_func, 0, (void *)&(a[i]))== (unsigned long) -1)
		errexit("_beginthread: %s\n", strerror(errno));


//	if (_beginthread((void (*)(void *))psv->sv_func, 0, (void *)ssock)== (unsigned long) -1)
	//	errexit("_beginthread: %s\n", strerror(errno));
}




