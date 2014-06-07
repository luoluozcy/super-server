
#include "Http.h"
#include <stdio.h>
#include <time.h>
#include <winsock2.h>
#include <fstream>
#include <string>
#include <iostream>
#include <io.h>
using namespace std;
#define	BUFFERSIZE	4096		/* max read buffer size	*/
//extern char tcproot[128];
//extern  ;
//Struct SocketInfo;
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


void	TCPechod(void *), TCPchargend(void *), TCPdaytimed(void *),TCPtimed(void *);
void UDPchargend(void *);
void UDPtimed(void * );
void	UDPechod(void *),UDPdaytimed(void *);
int ftpd();
void	errexit(const char *, ...);

/*------------------------------------------------------------------------
 * TCPecho - do TCP ECHO on the given socket
 *------------------------------------------------------------------------
 */
void TCPechod(void * f){
    SocketInfo * fd=(SocketInfo *)f;
	//SOCKET s=((SocketInfo *)fd)->s;
	char	buf[BUFFERSIZE];
	int	cc;

	while (cc = recv(fd->s, buf, sizeof(buf), 0)) {
		if (cc == SOCKET_ERROR)
			;//errexit("echo recv: errnum %d\n", GetLastError());
		if (send(fd->s, buf, cc, 0) == SOCKET_ERROR)
			;//errexit("echo send: errnum %d\n", GetLastError());
	}
	closesocket(fd->s);
	fd->use=true;
	
}

void UDPechod(void * f)
{  
    SocketInfo * fd=(SocketInfo *)f;
//	SOCKET s=((SocketInfo *)fd)->s;;
	//char	buf[BUFFERSIZE];
	int alen=sizeof(fd->fsin);


	//while (recvfrom(fd,buf,sizeof(buf),0,(struct sockaddr *)&(((SOCKET_INF *)w)->fsin),&alen)!=SOCKET_ERROR) 
	do{
		if(fd->bPasv){
			fd->bPasv=false;
			char        LocalAddr[80];
		gethostname(LocalAddr,sizeof(LocalAddr));
			LPHOSTENT	lp=gethostbyname(LocalAddr);
			struct in_addr *hostAddr=((LPIN_ADDR)lp->h_addr);

			printf("%s  %d  %s:",inet_ntoa((fd->fsin).sin_addr),(fd->fsin).sin_port,LocalAddr);
			printf("%s  %s\n",inet_ntoa(*hostAddr),fd->directory);

		}
		//(void) sendto(s.s, (char *)&now, sizeof(now), 0,(struct sockaddr *)&s.fsin, sizeof(s.fsin));
		(void) sendto(fd->s,fd->buffRecv,128,0,(struct sockaddr *)&(fd->fsin),sizeof(fd->fsin));
	}while (recvfrom(fd->s,fd->buffRecv,sizeof(fd->buffRecv),0,(struct sockaddr *)&(fd->fsin),&alen)!=SOCKET_ERROR);
//	closesocket(fd->s);
	fd->use=true;
	
}

#define	LINELEN		128

/*------------------------------------------------------------------------
 * TCPchargend - do TCP CHARGEN on the given socket
 *------------------------------------------------------------------------
 */
void TCPchargend(void * f){
   
	SocketInfo * fd=(SocketInfo *)f;
	char	c, buf[LINELEN+2];	/* print LINELEN chars + \r\n */

	c = ' ';
	buf[LINELEN] = '\r';
	buf[LINELEN+1] = '\n';
	while (1) {
		int	i;

		for (i=0; i<LINELEN; ++i) {
			buf[i] = c++;
			if (c > '~')
				c = ' ';
		}
		if (send(fd->s, buf, LINELEN+2, 0) == SOCKET_ERROR)
			break;
	}
	closesocket(fd->s);
		fd->use=true;
}

void UDPchargend(void * f){
	SocketInfo * fd=(SocketInfo *)f;
	char	c, buf[LINELEN+2];	/* print LINELEN chars + \r\n */

	c = ' ';
	buf[LINELEN] = '\r';
	buf[LINELEN+1] = '\n';
	while (1) {
		int	i;

		for (i=0; i<LINELEN; ++i) {
			buf[i] = c++;
			if (c > '~')
				c = ' ';
		}
		if (sendto(fd->s,fd->buffRecv,128,0,(struct sockaddr *)&(fd->fsin),sizeof(fd->fsin)) == SOCKET_ERROR)
			break;
	}
	//closesocket(fd->s);
		fd->use=true;
		
}


/*------------------------------------------------------------------------
 * TCPdaytimed - do TCP DAYTIME protocol
 *------------------------------------------------------------------------
 */
void TCPdaytimed(void * f){
//	printf("in the TCPdaytime\n");
	SocketInfo * fd=(SocketInfo *)f;
	char	buf[LINELEN];
	time_t	now;

	(void) time(&now);
	sprintf(buf, "%s", ctime(&now));
//	printf("%s\n",buf);
	(void) send(fd->s, buf, strlen(buf), 0);
	closesocket(fd->s);
		fd->use=true;
}

void UDPdaytimed(void * f){
		SocketInfo * fd=(SocketInfo *)f;
    //int alen=sizeof(fd->fsin);
	char	buf[2048];
	//if (recvfrom(fd, buf, sizeof(buf), 0,(struct sockaddr *)&(((SOCKET_INF *)w)->fsin), &alen) == SOCKET_ERROR)
	//		errexit("recvfrom: error %d\n", GetLastError());
	time_t	now;
	(void) time(&now);
	//now = htonl((u_long)(now + WINEPOCH));
    sprintf(buf, "%s", ctime(&now));
	char        LocalAddr[80];
	gethostname(LocalAddr,sizeof(LocalAddr));
	LPHOSTENT	lp=gethostbyname(LocalAddr);
	struct in_addr *hostAddr=((LPIN_ADDR)lp->h_addr);

	printf("%s  %d  %s:",inet_ntoa((fd->fsin).sin_addr),(fd->fsin).sin_port,LocalAddr);
	printf("%s  %s\n",inet_ntoa(*hostAddr),fd->directory);

    //printf("%s",buf);
	(void) sendto(fd->s, buf, sizeof(buf), 0,(struct sockaddr *)&(fd->fsin), sizeof(fd->fsin));
	//closesocket(fd->s);
	fd->use=true;
}

#define	WINEPOCH	2208988800	/* Windows epoch, in UCT secs	*/

/*------------------------------------------------------------------------
 * TCPtimed - do TCP TIME protocol
 *------------------------------------------------------------------------
 */
void TCPtimed(void * f){
	SocketInfo * fd=(SocketInfo *)f;
	time_t	now;

	(void) time(&now);
	now = htonl((u_long)(now + WINEPOCH));
	(void) send(fd->s, (char *)&now, sizeof(now), 0);
	closesocket(fd->s);
	fd->use=true;
}

void UDPtimed(void * f){
	SocketInfo * fd=(SocketInfo *)f;
	time_t	now;
	(void) time(&now);
	now = htonl((u_long)(now + WINEPOCH));

	char        LocalAddr[80];
	gethostname(LocalAddr,sizeof(LocalAddr));
	LPHOSTENT	lp=gethostbyname(LocalAddr);
	struct in_addr *hostAddr=((LPIN_ADDR)lp->h_addr);

	printf("%s  %d  %s:",inet_ntoa((fd->fsin).sin_addr),(fd->fsin).sin_port,LocalAddr);
	printf("%s  %s\n",inet_ntoa(*hostAddr),fd->directory);


	(void) sendto(fd->s, (char *)&now, sizeof(now), 0,(struct sockaddr *)&(fd->fsin), sizeof(fd->fsin));
//	closesocket(fd->s);
	fd->use=true;
	/*
	time_t	now;
    (void) time(&now);
	now = htonl((u_long)(now + WINEPOCH));
	(void) sendto(fd->s, (char *)&now, sizeof(now), 0,(struct sockaddr *)&fd->fsin, sizeof(fd->fsin));
	//(void) time(&now);
	//now = htonl((u_long)(now + WINEPOCH));
	//(void) send(fd, (char *)&now, sizeof(now), 0);
	closesocket(fd->s);
		fd->use=true;
		*/
}





/*
void ReadHttpHeaderLine(SOCKET &fd,char *request1,char *buf,int &bufLength){
	int nBytesThisTime=bufLength,nLength=0;
	char *pch1=buf,*pch2;
	do{
		if((pch2=(char*) memchr(pch1,'\n',nBytesThisTime))!=NULL){
			//buf[nBytesThisTime]='\0';
			//printf("%s",buf);

			nLength=pch2-buf+1;
			nLength=(nLength>99)?99:nLength;
			memcpy(request1,buf,nLength);
			bufLength-=nLength;
			memmove(buf,pch2+1,bufLength);
			break;
		}
		pch1+=nBytesThisTime;
		
		TIMEVAL tv={10,0};
		FD_SET fdd={1,fd};
		if(select(0,&fdd,NULL,NULL,&tv)==0){
			//printf("Receive timeout\n");
			return;
		}
		if((nBytesThisTime=recv(fd, buf+bufLength, BUFFERSIZE-bufLength, 0))==SOCKET_ERROR){
			//printf("Receive error\n");
			return;
		}
		
		
		bufLength+=nBytesThisTime;
	}while(TRUE);
	*(request1+nLength)='\0';
}
void Write(SOCKET &fd,const char* pch,const int size){
	int a=0,b;
	const char *p=pch;
	do{
		TIMEVAL tv={10,0};
		FD_SET fdd={1,fd};
		if(select(0,NULL,&fdd,NULL,&tv)==0){
			//printf("Send timeout\n");
			return;
		}
		if((b=send(fd,p,size-a,0))<0){
			//printf("Send error\n");
			return;
		}
		a+=b;
		p+=b;
	}while(a<size);
}
void readsrc(ifstream &f,char *custom,int &i){
	char dire[1024],tmp[4096];
	strcpy(dire,tcproot);
	int j;
	for(j=strlen(dire);dire[j]!='"';j++){
		f.get(dire[j]);
	}
	/*dire[j]='\0';
	ifstream f1(dire,ios::binary);
	if(!f1){
		printf("open src error\n");
		return;
	}
	for(;!f1.eof();i++){
		f1.get(custom[i]);
	}*/
//	custom[i]='"';i++;
//}
/*
/*

void TCPhttpd(SOCKET fd){
	char hdrFmt[]=
		"HTTP/1.0 200 OK\r\n"
		"Server: WY's Socket Server\r\n"
		"Date: %s\r\n"
		"Content-Type: text/html\r\n"
		"Accept-Ranges: bytes\r\n"
		"Content-Length: %d\r\n";
	char CustomHtml[]=
		"<html>\r\n"
		"<head>\r\n"
		"<title></title>\r\n"
		"</head>\r\n"
		"<body>\r\n"
		"<h1>บวบว</h1>\r\n"
		"</body></html>\r\n\r\n";
	char buf[BUFFERSIZE];
	
	int bufLength=0;
	time_t	now;
	struct tm *tm1=NULL;
	(void) time(&now);
	tm1=localtime(&now);
	char t[256],dire[256],custom[BUFFERSIZE];
	strftime(t,sizeof(t),"%a, %d %b %Y %H:%M:%S GMT",tm1);

	char headers[500],request1[100],request2[100],*ptoken1,*ptoken2;
	if(fd!=NULL) ReadHttpHeaderLine(fd,request1,buf,bufLength);
	//printf("%s",request1);
	ptoken1=request1;
	char *tmp=strchr(request1,' ');
	if(tmp){
		*tmp='\0';
		tmp++;
		ptoken2=tmp;
		tmp=strchr(tmp,' ');
		if(tmp){
			*tmp='\0';
		}
	}
	if(!stricmp(ptoken1,"GET")){
		do{
			if(fd!=NULL) ReadHttpHeaderLine(fd,request2,buf,bufLength);
		}while(strcmp(request2,"\r\n"));
		if(!stricmp(ptoken2,"/hehe")){
			wsprintfA(headers, hdrFmt, (const char*) t, strlen(CustomHtml));
			strcat(headers, "\r\n");
			Write(fd,headers,strlen(headers));
			Write(fd,CustomHtml, strlen(CustomHtml));
		}
		else{
			int i,j;
			strcpy(dire,tcproot);
			for(i=0,j=strlen(dire);ptoken2[i]!='\0';i++,j++){
				dire[j]=(ptoken2[i]=='/')?'\\':ptoken2[i];
			}
			dire[j]='\0';
			//printf("%s\n",dire);
			ifstream f(dire,ios::binary);
			if(!f){
				strcat(dire,"index.html");
				f.open(dire,ios::binary);
				if(!f){
					//printf("open root error\n");
					return;
				}
			}
			bool src=false;
			char tmp=' ';
			for(i=0;!f.eof();i++){
				if(src){
					//readsrc(f,custom,i);
					
					src=false;
				}

/////////////////////////////////////////////
				
f.get(custom[i]);
				
			}
			f.close();
			custom[i-1]='\0';
			//printf("%s\n",custom);
			wsprintfA(headers, hdrFmt, (const char*) t, strlen(custom));
			strcat(headers, "\r\n");
			Write(fd,headers,strlen(headers));
			Write(fd,custom, strlen(custom));
		}
	}
	else if(!stricmp(ptoken1,"POST")){
		do{
			if(fd!=NULL) ReadHttpHeaderLine(fd,request2,buf,bufLength);
		}while(strcmp(request2,"\r\n"));
		if(fd!=NULL) ReadHttpHeaderLine(fd,request2,buf,bufLength);
	}
	closesocket(fd);

}
*/
int http(SOCKET fd)
{
  CHttpProtocol* http=new CHttpProtocol();
  http->StartHttpSrv();
delete http;
  return 0;
}

int ftp(SOCKET http)
{
   int i=ftpd();
   return 0;
}
/*
void TCPftpd(SOCKET fd){
	char buf[BUFFERSIZE];
	int nBytesThisTime;
	strcpy(buf,"220 wy ready\r\n");
	(void) send(fd, buf, strlen(buf), 0);

	if((nBytesThisTime=recv(fd, buf, BUFFERSIZE, 0))==SOCKET_ERROR){
		printf("Receive error\n");
		return;
	}
	buf[nBytesThisTime]='\0';
	printf("%s\n\n",buf); //USER anonymous

	strcpy(buf,"331 pass\r\n");
	(void) send(fd, buf, strlen(buf), 0);

	if((nBytesThisTime=recv(fd, buf, BUFFERSIZE, 0))==SOCKET_ERROR){
		printf("Receive error\n");
		return;
	}
	buf[nBytesThisTime]='\0';
	printf("%s\n\n",buf); //PASS 123

	strcpy(buf,"230 login\r\n");
	(void) send(fd, buf, strlen(buf), 0);

	if((nBytesThisTime=recv(fd, buf, BUFFERSIZE, 0))==SOCKET_ERROR){
		printf("Receive error\n");
		return;
	}
	buf[nBytesThisTime]='\0';
	printf("%s\n\n",buf); //PWD

	strcpy(buf,"257 \"e:\\\" is current directory.\r\n");
	(void) send(fd, buf, strlen(buf), 0);

	if((nBytesThisTime=recv(fd, buf, BUFFERSIZE, 0))==SOCKET_ERROR){
		printf("Receive error\n");
		return;
	}
	buf[nBytesThisTime]='\0';
	printf("%s\n\n",buf); //TYPE I

	strcpy(buf,"200 Type set to I\r\n");
	(void) send(fd, buf, strlen(buf), 0);

	if((nBytesThisTime=recv(fd, buf, BUFFERSIZE, 0))==SOCKET_ERROR){
		printf("Receive error\n");
		return;
	}
	buf[nBytesThisTime]='\0';
	printf("%s\n\n",buf);//PASV

	strcpy(buf,"227 Entering Passive Mode(127,0,0,1,14,23)\r\n");
	(void) send(fd, buf, strlen(buf), 0);

	if((nBytesThisTime=recv(fd, buf, BUFFERSIZE, 0))==SOCKET_ERROR){
		printf("Receive error\n");
		return;
	}
	buf[nBytesThisTime]='\0';
	printf("%s\n\n",buf);//LIST

	
	ftp_request_rec r;
	r.so_client=fd;
	r.so_remote=SOCKET_ERROR;
	r.port=23;




	strcpy(buf,"drwxrwxrwx   1 ftp      ftp 0 music\r\n");
	(void) send(fd, buf, strlen(buf), 0);

	if((nBytesThisTime=recv(fd, buf, BUFFERSIZE, 0))==SOCKET_ERROR){
		printf("Receive error\n");
		return;
	}
	buf[nBytesThisTime]='\0';
	printf("%s\n\n",buf);

	closesocket(fd);
}
*/
