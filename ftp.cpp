// Mini FtpServer.cpp : Defines the entry point for the console application.
#include "stdafx.h"
#include <string.h>
//#include <ws2tcpip.h>
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#define WSA_RECV         0
#define WSA_SEND         1

#define DATA_BUFSIZE    8192
#define MAX_NAME_LEN    128
#define MAX_PWD_LEN     128
#define MAX_RESP_LEN    1024
#define MAX_REQ_LEN     256
#define MAX_ADDR_LEN    80

#define FTP_PORT        21     // FTP 控制端口
#define DATA_FTP_PORT   20     // FTP 数据端口

#define USER_OK         331
#define LOGGED_IN       230
#define LOGIN_FAILED    530
#define CMD_OK          200
#define OPENING_AMODE   150
#define TRANS_COMPLETE  226
#define CANNOT_FIND     550
#define FTP_QUIT        221
#define CURR_DIR        257
#define DIR_CHANGED     250
#define OS_TYPE         215
#define REPLY_MARKER    504
#define PASSIVE_MODE    227

#define FTP_USER		"luoluo"
#define FTP_PASS		"luo"
#define DEFAULT_HOME_DIR    "E:"

#define MAX_FILE_NUM        1024

#define MODE_PORT       0
#define MODE_PASV       1

#define PORT_BIND   1821

typedef struct {
   CHAR   buffRecv[DATA_BUFSIZE];
   CHAR   buffSend[DATA_BUFSIZE];
   WSABUF wsaBuf;
   SOCKET s;
   WSAOVERLAPPED o;
   DWORD dwBytesSend;
   DWORD dwBytesRecv;
   int   nStatus;
} SOCKET_INF, *LPSOCKET_INF;

typedef struct {
	TCHAR    szFileName[MAX_PATH];
	DWORD    dwFileAttributes; 
    FILETIME ftCreationTime; 
    FILETIME ftLastAccessTime; 
    FILETIME ftLastWriteTime; 
    DWORD    nFileSizeHigh; 
    DWORD    nFileSizeLow; 
} FILE_INF, *LPFILE_INF;

DWORD WINAPI ProcessTreadIO( LPVOID lpParam ) ;
BOOL WelcomeInfo( SOCKET s );
int LoginIn( LPSOCKET_INF pSocketInfo  );
int SendRes( LPSOCKET_INF pSI );
int RecvReq( LPSOCKET_INF pSI );
int DealCommand(LPSOCKET_INF pSI );

int GetFileList( LPFILE_INF pFI, UINT nArraySize, const char* szPath  );
char* GetLocalAddress();
char* GetIPAddress();
//GetAddress()
char* HostToNet( char* szPath ) ;
char* NetToHost( char* szPath ) ;
char* RelativeDirectory( char* szDir );
char* AbsoluteDirectory( char* szDir );
DWORD g_dwEventTotal = 0;
DWORD g_index;
WSAEVENT g_events[WSA_MAXIMUM_WAIT_EVENTS];
LPSOCKET_INF g_sockets[WSA_MAXIMUM_WAIT_EVENTS];
CRITICAL_SECTION g_cs;  
char  g_szLocalAddr[MAX_ADDR_LEN]; 
BOOL  g_bLoggedIn;
char szIpAddr[20];
char reName[256];
// 主函数，控制台程序开始的地方
//int http(SOCKET http)
int ftpd()
{
//InterlockedIncrement(&lCount);

   WSADATA wsaData;
   SOCKET sListen, sAccept;
   SOCKADDR_IN inetAddr;
   DWORD dwFlags;
   DWORD dwThreadId;
   DWORD dwRecvBytes;
   INT   nRet;

   InitializeCriticalSection(&g_cs);
   if (( nRet = WSAStartup(0x0202,&wsaData)) != 0 ) {
      printf("错误：WSAStartup failed with error %d\n", nRet);
      return 0;
   }

   // 先取得本地地址
   sprintf( g_szLocalAddr,"%s",GetLocalAddress() );
   if ((sListen = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, 
      WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET) 
   {
      printf("错误：Failed to get a socket %d\n", WSAGetLastError());
	  WSACleanup();
      return 0;
   }

   inetAddr.sin_family = AF_INET;
   inetAddr.sin_addr.s_addr = htonl(INADDR_ANY);
   inetAddr.sin_port = htons(FTP_PORT);

   if (bind(sListen, (PSOCKADDR) &inetAddr, sizeof(inetAddr)) == SOCKET_ERROR)
   {
      printf("错误：bind() failed with error %d\n", WSAGetLastError());
      return 0;
   }

   if (listen(sListen, SOMAXCONN))
   {
      printf("错误：listen() failed with error %d\n", WSAGetLastError());
      return 0;
   }

   printf("Mini Ftpserver已经启动 \n");
   printf("Mini Ftpserver开始侦听 \n");
 
   if ((sAccept = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0,
      WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET) 
   {
      printf("错误：Failed to get a socket %d\n", WSAGetLastError());
      return 0;
   }

   //创建第一个手动重置对象 
   if ((g_events[0] = WSACreateEvent()) == WSA_INVALID_EVENT)
   {
      printf("错误：WSACreateEvent failed with error %d\n", WSAGetLastError());
      return 0;
   }

   // 创建一个线程处理请求
   if (CreateThread(NULL, 0, ProcessTreadIO, NULL, 0, &dwThreadId) == NULL)
   {
      printf("错误：CreateThread failed with error %d\n", GetLastError());
      return 0;
   } 

   g_dwEventTotal = 1;

   while(TRUE)
   {
       //处理入站连接
      if ((sAccept = accept(sListen, NULL, NULL)) == INVALID_SOCKET)
      {
          printf("错误：accept failed with error %d\n", WSAGetLastError());
          return 0;
      }
       
      sockaddr addr;
     int len=sizeof(addr);
	 getsockname(sAccept,&addr,&len);
	 // char szPort[10];
//	sprintf( szPort,"%d,%d",wPort/256,wPort%256 );

//	sprintf( szIpAddr,"%s,",szAddress );
	sprintf( szIpAddr,"%s",inet_ntoa(((sockaddr_in *)&addr)->sin_addr));
	printf(szIpAddr);
	//return szIpAddr;

      //回传欢迎消息
	  if( !WelcomeInfo( sAccept ) ) break;
      //设置ftp根目录
	  if( !SetCurrentDirectory( DEFAULT_HOME_DIR ) ) break;

	  //操作临界区，防止出错
      EnterCriticalSection(&g_cs);
      //创建一个新的SOCKET_INF结构处理接受的数据socket.
      if ((g_sockets[g_dwEventTotal] = (LPSOCKET_INF) 
		  GlobalAlloc(GPTR,sizeof(SOCKET_INF))) == NULL)
      {
         printf("错误：GlobalAlloc() failed with error %d\n", GetLastError());
         return 0;
      } 

      //初始化新的SOCKET_INF结构
	  char buff[DATA_BUFSIZE]; memset( buff,0,DATA_BUFSIZE );
	  g_sockets[g_dwEventTotal]->wsaBuf.buf = buff;  
	  g_sockets[g_dwEventTotal]->wsaBuf.len = DATA_BUFSIZE;
      g_sockets[g_dwEventTotal]->s = sAccept;
      memset(&(g_sockets[g_dwEventTotal]->o),0, sizeof(OVERLAPPED));
      g_sockets[g_dwEventTotal]->dwBytesSend = 0;
      g_sockets[g_dwEventTotal]->dwBytesRecv = 0;
	  g_sockets[g_dwEventTotal]->nStatus     = WSA_RECV;    // 接收
   
     //创建事件
      if ((g_sockets[g_dwEventTotal]->o.hEvent = g_events[g_dwEventTotal] = 
          WSACreateEvent()) == WSA_INVALID_EVENT)
      {
         printf("WSACreateEvent() failed with error %d\n", WSAGetLastError());
         return 0;
      }

      //发出接受请求
      dwFlags = 0;
      if (WSARecv(g_sockets[g_dwEventTotal]->s, 
         &(g_sockets[g_dwEventTotal]->wsaBuf), 1, &dwRecvBytes, &dwFlags,
         &(g_sockets[g_dwEventTotal]->o), NULL) == SOCKET_ERROR)
      {
         if (WSAGetLastError() != ERROR_IO_PENDING)
         {
            printf("错误：WSARecv() failed with error %d\n", WSAGetLastError());
            return 0;
         }
      }
      g_dwEventTotal++;

	  //离开临界区
      LeaveCriticalSection(&g_cs);

	  //使第一个事件有信号。使工作者线程处理其他的事件
      if (WSASetEvent(g_events[0]) == FALSE)
      {
         printf("错误：WSASetEvent failed with error %d\n", WSAGetLastError());
         return 0;
      }
   }
   getchar();
   return 0;
}

//工作者线程处理函数
DWORD WINAPI ProcessTreadIO(LPVOID lpParameter)
{
   DWORD dwFlags;
   LPSOCKET_INF pSI;
   DWORD dwBytesTransferred;
   DWORD i;  

   //处理异步的WSASend, WSARecv等请求等
   while(TRUE)
   {
      if ((g_index = WSAWaitForMultipleEvents(g_dwEventTotal, g_events, FALSE,
                                      WSA_INFINITE, FALSE)) == WSA_WAIT_FAILED)
      {
         printf("错误：WSAWaitForMultipleEvents failed %d\n", WSAGetLastError());
         return 0;
      }
      
      if ((g_index - WSA_WAIT_EVENT_0) == 0)
      {
         WSAResetEvent(g_events[0]);
         continue;
      }

      pSI = g_sockets[g_index - WSA_WAIT_EVENT_0];
      WSAResetEvent(g_events[g_index - WSA_WAIT_EVENT_0]);

      if (WSAGetOverlappedResult(pSI->s, &(pSI->o), &dwBytesTransferred,
                    FALSE, &dwFlags) == FALSE || dwBytesTransferred == 0)
      {
         printf("Closing socket %d\n", pSI->s);

         if (closesocket(pSI->s) == SOCKET_ERROR)
         {
            printf("错误：closesocket() failed with error %d\n", WSAGetLastError());
         }

         GlobalFree(pSI);
         WSACloseEvent(g_events[g_index - WSA_WAIT_EVENT_0]);

         // Cleanup g_sockets and g_events by removing the socket event handle
         // and socket information structure if they are not at the end of the
         // arrays.

         EnterCriticalSection(&g_cs);

         if ((g_index - WSA_WAIT_EVENT_0) + 1 != g_dwEventTotal)
            for (i = g_index - WSA_WAIT_EVENT_0; i < g_dwEventTotal; i++) 
			{
               g_events[i] = g_events[i + 1];
			   g_sockets[i] = g_sockets[i + 1];
            }

         g_dwEventTotal--;

         LeaveCriticalSection(&g_cs);

         continue;
      }

	  // 已经有数据传递
	  if( pSI->nStatus == WSA_RECV )
	  {
		  memcpy( &pSI->buffRecv[pSI->dwBytesRecv],pSI->wsaBuf.buf,dwBytesTransferred);
		  pSI->dwBytesRecv += dwBytesTransferred;
		  printf( "接受:%s\n",pSI->buffRecv);
		  if( pSI->buffRecv[pSI->dwBytesRecv-2] == '\r'      // 要保证最后是\r\n
				&& pSI->buffRecv[pSI->dwBytesRecv-1] == '\n' 
				&& pSI->dwBytesRecv > 2 )  
		  {                 
			 if( !g_bLoggedIn )
			 {
				if( LoginIn(pSI) == LOGGED_IN )
					g_bLoggedIn = TRUE;
			 } 
			 else 
			 {
				  if(DealCommand( pSI )==FTP_QUIT)
				  continue;
			 }
			 // 缓冲区清除
			 memset( pSI->buffRecv,0,sizeof(pSI->buffRecv) );
			 pSI->dwBytesRecv = 0;
		  }
	  } 
	  else
	  {
		  pSI->dwBytesSend += dwBytesTransferred;
	  }
	  
 	  // 继续接收以后到来的数据
	  if( RecvReq( pSI ) == -1 ) 
		  return -1; 
   }
   return 0;
}

// 由于只是简单的出现一个登录信息，直接用send就可以了
int SendRes( LPSOCKET_INF pSI )
{
	static DWORD dwSendBytes = 0;
	pSI->nStatus = WSA_SEND;
    memset(&(pSI->o), 0,sizeof(WSAOVERLAPPED));
    pSI->o.hEvent = g_events[g_index - WSA_WAIT_EVENT_0];
    pSI->wsaBuf.buf = pSI->buffSend + pSI->dwBytesSend;
    pSI->wsaBuf.len = strlen( pSI->buffSend ) - pSI->dwBytesSend;
    if (WSASend(pSI->s, &(pSI->wsaBuf), 1,&dwSendBytes,
		        0,&(pSI->o), NULL) == SOCKET_ERROR) 
	{
        if (WSAGetLastError() != ERROR_IO_PENDING) 
		{
			printf("WSASend() failed with error %d\n", WSAGetLastError());
			return -1;
        }
    }
	return 0;
}
//接受数据
int RecvReq( LPSOCKET_INF pSI )
{
	static DWORD dwRecvBytes = 0;	
	pSI->nStatus = WSA_RECV;	

	DWORD dwFlags = 0;
	memset(&(pSI->o), 0,sizeof(WSAOVERLAPPED));
	pSI->o.hEvent = g_events[g_index - WSA_WAIT_EVENT_0];
	pSI->wsaBuf.len = DATA_BUFSIZE;

	if (WSARecv(pSI->s, &(pSI->wsaBuf), 1, &dwRecvBytes,
                &dwFlags,&(pSI->o), NULL) == SOCKET_ERROR)
	{
		if (WSAGetLastError() != ERROR_IO_PENDING)
		{
		   printf("WSARecv() failed with error %d\n", WSAGetLastError());
		   return -1;
		}
	}
	return 0;
}

//显示欢迎消息
BOOL WelcomeInfo( SOCKET s )
{
	char* szWelcomeInfo = "220 欢迎您登录到Mini FtpServer...\r\n";
	if( send( s,szWelcomeInfo,strlen(szWelcomeInfo),0 ) == SOCKET_ERROR ) 
	{
		printf("Ftp client error:%d\n", WSAGetLastError() );
		return FALSE;
	}
	// 刚进来，还没连接，故设置初始状态为false
	g_bLoggedIn = FALSE;
	return TRUE;
}
//登录函数
int LoginIn( LPSOCKET_INF pSocketInfo  )
{
	const char* szUserOK = "331 User name okay, need password.\r\n"; 
	const char* szLoggedIn = "230 User logged in, proceed.\r\n";

	int  nRetVal = 0;
	static char szUser[MAX_NAME_LEN], szPwd[MAX_PWD_LEN];
	LPSOCKET_INF pSI = pSocketInfo;
	// 取得登录用户名
	if( strstr(strupr(pSI->buffRecv),"USER") ) 
	{		
		sprintf(szUser,"%s",pSI->buffRecv+strlen("USER")+1);
		strtok( szUser,"\r\n");		
		// 响应信息
		sprintf(pSI->buffSend,"%s",szUserOK );
		if( SendRes(pSI) == -1 ) return -1;
		return USER_OK;
	}
	if( strstr(strupr(pSI->buffRecv),"PASS") || strstr(pSI->buffRecv,"pass") ) 
	{
		sprintf(szPwd,"%s",pSI->buffRecv+strlen("PASS")+1 );
		strtok( szPwd,"\r\n");
		// 判断用户名跟口令正确性
		if( stricmp( szPwd,FTP_PASS) || stricmp(szUser,FTP_USER) ) 
		{
			sprintf(pSI->buffSend,"530 User %s cannot log in.\r\n",szUser );
			printf("User %s cannot log in\n",szUser );
			nRetVal = LOGIN_FAILED;
		} 
		else 
		{
			sprintf(pSI->buffSend,"%s",szLoggedIn);
			printf("User %s logged in\n",szUser );
			nRetVal = LOGGED_IN;
		}
		if( SendRes( pSI ) == -1 ) 
			return -1;
	}
	return nRetVal;
}

char* ConvertCommaAddress( char* szAddress, WORD wPort )
{    // sockaddr addr;
     // int len=sizeof(addr);
//	 getsockname(sAccept,&addr,&len);
	char szPort[10];
	sprintf( szPort,"%d,%d",wPort/256,wPort%256 );
	char szAddr[20];
	sprintf( szAddr,"%s,",szAddress );
//	sprintf( szIpAddr,"%s,",inet_ntoa(((sockaddr_in *)&addr)->sin_addr));

	int idx = 0;
	while( szAddr[idx] ) 
	{
		if( szAddr[idx] == '.' )
			szAddr[idx] = ',';
		idx ++;
	}
	sprintf( szAddress,"%s%s",szAddr,szPort );
	printf(szAddress);
	return szAddress;
}

int ConvertDotAddress( char* szAddress, LPDWORD pdwIpAddr, LPWORD pwPort ) 
{
	int  idx = 0,i = 0, iCount = 0;
	char szIpAddr[MAX_ADDR_LEN]; memset( szIpAddr,0,sizeof(szIpAddr) );
	char szPort[MAX_ADDR_LEN];   memset( szPort,0,  sizeof(szPort)   );

	*pdwIpAddr = 0; *pwPort = 0;

	while( szAddress[idx]  )
	{
		if( szAddress[idx] == ',' )
		{
			iCount ++;
			szAddress[idx] ='.';
		}
		if( iCount < 4 )
			szIpAddr[idx] = szAddress[idx];
		else
			szPort[i++] =   szAddress[idx];
		idx++;
	}

	if( iCount != 5 ) return -1;
	*pdwIpAddr = inet_addr( szIpAddr );
	if( *pdwIpAddr  == INADDR_NONE ) return -1;
	char *pToken = strtok( szPort+1,"." );
	if( pToken == NULL ) return -1;
	*pwPort = (WORD)(atoi(pToken) * 256);
	pToken = strtok(NULL,".");
	if( pToken == NULL ) return -1;
	*pwPort += (WORD)atoi(pToken);
		
	return 0;
}

UINT FileListToString( char* buff, UINT nBuffSize,BOOL bDetails )
{
	FILE_INF   fi[MAX_FILE_NUM];
	int nFiles = GetFileList( fi, MAX_FILE_NUM, "*.*" );
	char szTemp[128];
	sprintf( buff,"%s","" );
	if( bDetails ) {
		for( int i=0; i<nFiles; i++) {
			if( strlen(buff)>nBuffSize-128 )   break;
			if(!strcmp(fi[i].szFileName,"."))  continue;
			if(!strcmp(fi[i].szFileName,"..")) continue;
			// 时间
			SYSTEMTIME st;
			FileTimeToSystemTime(&(fi[i].ftLastWriteTime), &st);
			char  *szNoon = "AM";
			if( st.wHour > 12 ) 
			{ 
				st.wHour -= 12;
				szNoon = "PM"; 
			}
			if( st.wYear >= 2000 )
				st.wYear -= 2000;
			else st.wYear -= 1900;
			sprintf( szTemp,"%02u-%02u-%02u  %02u:%02u%s       ",
						st.wMonth,st.wDay,st.wYear,st.wHour,st.wMonth,szNoon );
			strcat( buff,szTemp );
			if( fi[i].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
			{
				strcat(buff,"<DIR>");
				strcat(buff,"          ");
			}
			else 
			{
				strcat(buff,"     ");
				// 文件大小
				sprintf( szTemp,"% 9d ",fi[i].nFileSizeLow );
				strcat( buff,szTemp );
			}
			// 文件名
			strcat( buff,fi[i].szFileName );
			strcat( buff,"\r\n");
		}
	} 
	else
	{ 
		for( int i=0; i<nFiles; i++)
		{
			if( strlen(buff) + strlen( fi[i].szFileName ) + 2 < nBuffSize )
			{ 
				strcat( buff, fi[i].szFileName );
				strcat( buff, "\r\n");
			} 
			else
				break;
		}
	}
	return strlen( buff );
}

DWORD ReadFileToBuffer( const char* szFile, char *buff, DWORD nFileSize )
{
	DWORD  idx = 0;
	DWORD  dwBytesLeft = nFileSize;
	DWORD  dwNumOfBytesRead = 0;
	char lpFileName[MAX_PATH];
	GetCurrentDirectory( MAX_PATH,lpFileName );
	strcat( lpFileName,"\\" );
	strcat(lpFileName,szFile );
	HANDLE hFile = CreateFile( lpFileName,
							   GENERIC_READ,
							   FILE_SHARE_READ,
							   NULL,
							   OPEN_EXISTING,
							   FILE_ATTRIBUTE_NORMAL,
							   NULL );
	if( hFile != INVALID_HANDLE_VALUE )
	{
		while( dwBytesLeft > 0 ) 
		{
			if( !ReadFile( hFile,&buff[idx],dwBytesLeft,&dwNumOfBytesRead,NULL ) )
			{
				printf("读文件出错.\n");
				CloseHandle( hFile );
				return 0;
			}
			idx += dwNumOfBytesRead;
			dwBytesLeft -= dwNumOfBytesRead;
		}
		CloseHandle( hFile );
	}
	return idx;
}

DWORD WriteToFile( SOCKET s , const char* szFile )
{
	DWORD  idx = 0;
	DWORD  dwNumOfBytesWritten = 0;
	DWORD  nBytesLeft = DATA_BUFSIZE;
	char   buf[DATA_BUFSIZE];
	char   lpFileName[MAX_PATH];
	GetCurrentDirectory( MAX_PATH,lpFileName );
	strcat( lpFileName,"\\" );
	strcat(lpFileName,szFile );
	HANDLE hFile = CreateFile( lpFileName,
							   GENERIC_WRITE,
							   FILE_SHARE_WRITE,
							   NULL,
							   OPEN_ALWAYS,
							   FILE_ATTRIBUTE_NORMAL,
							   NULL );
	if( hFile == INVALID_HANDLE_VALUE ) 
	{ 
		printf("打开文件出错.\n");
		return 0;
	}
	
	while( TRUE )
	{
		int nBytesRecv = 0;
		idx = 0; nBytesLeft = DATA_BUFSIZE;
		while( nBytesLeft > 0 )
		{
			nBytesRecv = recv( s,&buf[idx],nBytesLeft,0 );
			if( nBytesRecv == SOCKET_ERROR ) 
			{
				printf("Failed to send buffer to socket %d\n",WSAGetLastError() );
				return -1;
			}
			if( nBytesRecv == 0 ) break;
		
			idx += nBytesRecv;
			nBytesLeft -= nBytesRecv;
		}
		nBytesLeft = idx;   // 要写入文件中的字节数
		idx = 0;			// 索引清0,指向开始位置
		while( nBytesLeft > 0 ) 
		{
			// 移动文件指针到文件末尾
			if( !SetEndOfFile(hFile) ) return 0;
			if( !WriteFile( hFile,&buf[idx],nBytesLeft,&dwNumOfBytesWritten,NULL ) ) 
			{
				printf("写文件出错.\n");
				CloseHandle( hFile );
				return 0;
			}
			idx += dwNumOfBytesWritten;
			nBytesLeft -= dwNumOfBytesWritten;
		}
		// 如果没有数据可接收，退出循环
		if( nBytesRecv == 0 ) break;
	}

	CloseHandle( hFile );
	return idx;
}
int CombindFileNameSize( const char* szFileName,char* szFileNS )
{
	// 假定文件的大小不超过4GB,只处理低位
	int nFileSize = -1;
	FILE_INF fi[1];
	int nFiles = GetFileList( fi,1,szFileName );
	if( nFiles != 1 ) return -1;
	sprintf( szFileNS, "%s<%d bytes>",szFileName,fi[0].nFileSizeLow );
	nFileSize = fi[0].nFileSizeLow;
	return nFileSize;
}

int	DataConn( SOCKET& s, DWORD dwIp, WORD wPort, int nMode ) 
{
	// 创建一个socket
	s = socket( AF_INET,SOCK_STREAM,0 );
	if( s == INVALID_SOCKET )
	{
		 printf("Failed to get a socket %d\n", WSAGetLastError());
		 return -1;
	}

	struct sockaddr_in inetAddr;
	inetAddr.sin_family = AF_INET;
	if( nMode == MODE_PASV )
	{
		 inetAddr.sin_port = htons( wPort );
		 inetAddr.sin_addr.s_addr = dwIp;
	}
	else
	{ 
		inetAddr.sin_port = htons( DATA_FTP_PORT );
		inetAddr.sin_addr.s_addr = inet_addr(GetLocalAddress());
	}

	BOOL optval = TRUE;
	if( setsockopt(s,SOL_SOCKET,SO_REUSEADDR,
				(char*)&optval,sizeof(optval) ) == SOCKET_ERROR ) 
	{
		printf("Failed to setsockopt %d.\n",WSAGetLastError() );
		closesocket(s);
		return -1;
	}

	if( bind( s,(struct sockaddr*)&inetAddr,sizeof(inetAddr)) == SOCKET_ERROR )
	{
		printf("Failed to bind a socket %d.\n",WSAGetLastError() );
		closesocket(s);
		return -1;
	}

	if( MODE_PASV == nMode )
	{
		if( listen( s,SOMAXCONN ) == SOCKET_ERROR ) 
		{
			printf("Failed to listen a socket %d.\n",WSAGetLastError() );
			closesocket(s);
			return -1;
		}
	} 
	else if( MODE_PORT == nMode ) 
	{
		struct sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_port   = htons( wPort );
		addr.sin_addr.s_addr   = dwIp;
		if( connect( s, (const sockaddr*)&addr,sizeof(addr) ) == SOCKET_ERROR ) 
		{
			printf("Failed to connect a socket %d\n",WSAGetLastError() );
			closesocket( s );
			return -1;
		}
	}
	return 0;
}

int DataSend( SOCKET s, char* buff,int nBufSize )
{
	int nBytesLeft = nBufSize;
	int idx = 0, nBytes = 0;
	while( nBytesLeft > 0 ) {
		nBytes = send( s,&buff[idx],nBytesLeft,0);
		if( nBytes == SOCKET_ERROR )
		{
			printf("Failed to send buffer to socket %d\n",WSAGetLastError() );
			closesocket( s );
			return -1;
		}
		nBytesLeft -= nBytes;
		idx += nBytes;
	}
	return idx;
}
int DataRecv( SOCKET s, const char* szFileName )
{
	return WriteToFile( s, szFileName );	
}

SOCKET DataAccept( SOCKET& s )
{
	SOCKET sAccept = accept( s ,NULL,NULL );
	if( sAccept != INVALID_SOCKET ) 
	{
		closesocket( s );
	}
	return sAccept;
}
int DealCommand(LPSOCKET_INF pSI )
{
	int nRetVal = 0;
	static SOCKET sAccept = INVALID_SOCKET;
	static SOCKET s       = INVALID_SOCKET;
	static BOOL   bPasv   = FALSE;
		
	char  szCmd[MAX_REQ_LEN]; 
	char  szCurrDir[MAX_PATH];
	strcpy( szCmd, pSI->buffRecv );
	if( strtok( szCmd," \r\n") == NULL ) return -1;
	strupr( szCmd );

	const char*  szOpeningAMode = "150 Opening ASCII mode data connection for ";
	static DWORD  dwIpAddr = 0;
	static WORD   wPort    = 0;
	// ?PORT n1,n2,n3,n4,n5,n6
		//sprintf(pSI->buffSend,"%s",szCmd);
	printf("%s \n",szCmd);
	if( strstr(szCmd,"PORT") )
	{
		if( ConvertDotAddress( pSI->buffRecv+strlen("PORT")+1,&dwIpAddr,&wPort) == -1 ) 
			return -1;
		const char*  szPortCmdOK    = "200 PORT Command successful.\r\n";
		sprintf(pSI->buffSend,"%s",szPortCmdOK );
		if( SendRes( pSI ) == -1 ) return -1;
		bPasv = FALSE;
		return CMD_OK;
	}
	if( strstr( szCmd,"PASV") ) 
	{
		if( DataConn( s, htonl(INADDR_ANY), PORT_BIND, MODE_PASV ) == -1 )
			return -1;
		char *szCommaAddress = ConvertCommaAddress(GetIPAddress(),PORT_BIND );
		sprintf( pSI->buffSend,"227 Entering Passive Mode (%s).\r\n",szCommaAddress );
		if( SendRes( pSI ) == -1 ) 
			return -1;
    	bPasv = TRUE;
		return PASSIVE_MODE;		
	}
	if( strstr( szCmd, "NLST") || strstr( szCmd,"LIST") ) 
	{
		if( bPasv ) sAccept = DataAccept( s );
		if( !bPasv )
			sprintf(pSI->buffSend,"%s/bin/ls.\r\n",szOpeningAMode );
		else 
			strcpy(pSI->buffSend,"125 Data connection already open; Transfer starting.\r\n");		
		
		if( SendRes( pSI ) == -1 ) 
			return -1;
		// 取得文件列表信息，并转换成字符串
		BOOL bDetails = strstr(szCmd,"LIST")?TRUE:FALSE;
		char buff[DATA_BUFSIZE];
		UINT nStrLen = FileListToString( buff,sizeof(buff),bDetails);
		if( !bPasv ) 
		{
			if( DataConn( s,dwIpAddr,wPort,MODE_PORT ) == -1 )
				return -1;
			if( DataSend( s, buff,nStrLen ) == -1 )
				return -1;
			closesocket(s);
		}
		else 
		{
			DataSend( sAccept,buff,nStrLen );
			closesocket( sAccept );
		}
		sprintf( pSI->buffSend,"%s","226 Transfer complete.\r\n" );
		if( SendRes( pSI ) == -1 )
			return -1;

		return TRANS_COMPLETE;
	}
	if( strstr( szCmd, "RETR") ) 
	{
		if( bPasv ) sAccept = DataAccept(s);	
		char szFileNS[MAX_PATH];
		char *szFile = strtok( NULL," \r\n" );
		int nFileSize = CombindFileNameSize( szFile,szFileNS );
		if( nFileSize == -1  ) 
		{
			sprintf( pSI->buffSend,"550 %s: 系统找不到指定的文件.\r\n",szFile);
			if( SendRes( pSI ) == -1 )
				return -1;
			if( !bPasv ) closesocket( sAccept );
			else closesocket( s );
			
			return CANNOT_FIND; 
		}
		else 
			sprintf(pSI->buffSend,"%s%s.\r\n",szOpeningAMode,szFileNS);

		if( SendRes( pSI ) == -1 )
			return -1;
	
		char* buff = new char[nFileSize];
		if( NULL ==buff ) 
		{
			printf("Not enough memory error!\n");
			return -1;
		}
		if( ReadFileToBuffer( szFile,buff, nFileSize ) == (DWORD)nFileSize ) 
		{
			// 处理Data FTP连接
			Sleep( 10 );
			if( bPasv ) 
			{
				DataSend( sAccept,buff,nFileSize );
				closesocket( sAccept );
			} 
			else 
			{
				if( DataConn( s,dwIpAddr,wPort,MODE_PORT ) == -1 )
					return -1;
				DataSend( s, buff, nFileSize );
				closesocket( s );
			}
		}
		if( buff != NULL )
			delete[] buff;

		sprintf( pSI->buffSend,"%s","226 Transfer complete.\r\n" );
		if( SendRes( pSI ) == -1 )
			return -1;
		
				
		return TRANS_COMPLETE;
	}
	if( strstr( szCmd, "STOR") ) 
	{
		if( bPasv ) sAccept = DataAccept(s);		
		char *szFile = strtok( NULL," \r\n" );
		if( NULL == szFile ) return -1;
		sprintf(pSI->buffSend,"%s%s.\r\n",szOpeningAMode,szFile);
		if( SendRes( pSI ) == -1 )
			return -1;

		// 处理Data FTP连接
		if( bPasv ) 
			DataRecv( sAccept,szFile );
		else 
		{
			if( DataConn( s,dwIpAddr,wPort,MODE_PORT ) == -1 )
				return -1;
			DataRecv( s, szFile );
		}
		
		sprintf( pSI->buffSend,"%s","226 Transfer complete.\r\n" );
		if( SendRes( pSI ) == -1 )
			return -1;
				
		return TRANS_COMPLETE;
	}

		/*	if( strstr( szCmd, "TOK_TYPE"))
		{
			SendResponse("200 Type set to " + strArguments);
		}
		
*/
		
	if( strstr( szCmd,"QUIT" ) )
	{
		sprintf( pSI->buffSend,"%s","221 Good bye,欢迎下次再来.\r\n" );
		if( SendRes( pSI ) == -1 )
			return -1;
		
		return FTP_QUIT; 
	}
	if( strstr( szCmd,"XPWD" ) || strstr( szCmd,"PWD") ) 
	{
		GetCurrentDirectory( MAX_PATH,szCurrDir );
		sprintf( pSI->buffSend,"257 \"%s\" is current directory.\r\n",
			RelativeDirectory(szCurrDir) );
		if( SendRes( pSI ) == -1 ) return -1;

		return CURR_DIR;
	}
	if( strstr( szCmd,"CWD" ) || strstr(szCmd,"CDUP") ) 
	{
		char *szDir = strtok( NULL,"\r\n" );
		if( szDir == NULL ) szDir = "\\";
		char szSetDir[MAX_PATH];
		if( strstr(szCmd,"CDUP") ) 
			strcpy(szSetDir,"..");
		 else 
			strcpy(szSetDir,AbsoluteDirectory( szDir ) );
		if( !SetCurrentDirectory( szSetDir ) ) 
		{
			sprintf(szCurrDir,"\\%s",szSetDir); 
			sprintf( pSI->buffSend,"550 %s No such file or Directory.\r\n",
					RelativeDirectory(szCurrDir) );
			nRetVal = CANNOT_FIND;
		} 
		else 
		{
			GetCurrentDirectory( MAX_PATH,szCurrDir );
			sprintf( pSI->buffSend,"250 Directory changed to /%s.\r\n",
					RelativeDirectory(szCurrDir) );
			nRetVal = DIR_CHANGED;
		}
		if( SendRes( pSI ) == -1 ) return -1;

		return nRetVal;
	}

	if( strstr( szCmd,"DELE" )) 
	{
		char *file = strtok( NULL,"\r\n" );
		GetCurrentDirectoryA( MAX_PATH,szCurrDir );
		if(szCurrDir[strlen(szCurrDir)-1]!='\\')	strcat(szCurrDir,"\\");
		strcat(szCurrDir,file);
		if(DeleteFileA((LPCSTR)szCurrDir))
			sprintf( pSI->buffSend,"250 %s has deleted.\r\n",file);
		else sprintf( pSI->buffSend,"250 %s has not deleted.\r\n",file);
		if( SendRes( pSI ) == -1 ) return -1;

		return 250;
	}
	if( strstr( szCmd,"MKD" )) 
	{
		char *file = strtok( NULL,"\r\n" );
		GetCurrentDirectoryA( MAX_PATH,szCurrDir );
		if(szCurrDir[strlen(szCurrDir)-1]!='\\')	strcat(szCurrDir,"\\");
		strcat(szCurrDir,file);
		if(CreateDirectoryA((LPCSTR)szCurrDir,NULL))
			sprintf( pSI->buffSend,"250 %s has created.\r\n",file);
		else sprintf( pSI->buffSend,"250 %s has not created.\r\n",file);
			
		if( SendRes( pSI ) == -1 ) return -1;

		return 250;
	}


if( strstr( szCmd,"RMD" )) {
		char *file = strtok( NULL,"\r\n" );
		GetCurrentDirectoryA( MAX_PATH,szCurrDir );
		//strcpy(szCurrDir,pSI->directory);
		if(szCurrDir[strlen(szCurrDir)-1]!='\\')	strcat(szCurrDir,"\\");
		strcat(szCurrDir,file);
	// CString strResult;
	//	int nResult = theServer.m_UserManager.GetDirectory(m_strUserName, strArguments, m_strCurrentDir, FTP_DELETE, strResult);
		BOOL fbRet=FALSE;
		fbRet= RemoveDirectoryA((LPCSTR)szCurrDir);
		if(!fbRet){
			/*if (GetLastError() == ERROR_DIR_NOT_EMPTY)
					sprintf( pSI->buffSend,"550 Directory not empty.");
			else*/
				sprintf( pSI->buffSend,"450 Internal error deleting the directory.\r\n");
		}
		else{
			sprintf(pSI->buffSend,"250 Directory deleted successfully\r\n");
		}
		if( SendRes( pSI ) == -1 ) return -1;

		return 250;		
	}

	
/*if( strstr( szCmd,"RMD" )) 
		{
		   char *file = strtok( NULL,"\r\n" );
		   GetCurrentDirectoryA( MAX_PATH,szCurrDir );	
		   	if(szCurrDir[strlen(szCurrDir)-1]!='\\')	strcat(szCurrDir,"\\");
	      	strcat(szCurrDir,file);
	    // CString strResult;
		//	int nResult = theServer.m_UserManager.GetDirectory(m_strUserName, strArguments, m_strCurrentDir, FTP_DELETE, strResult);
		  BOOL fbRet=FALSE;
		   fbRet= RemoveDirectoryA((LPCSTR)szCurrDir);
		 	if(!fbRet)
					{
						if (GetLastError() == ERROR_DIR_NOT_EMPTY)
								sprintf( pSI->buffSend,"550 Directory not empty.\r\n");
						else
							sprintf( pSI->buffSend,"450 Internal error deleting the directory.\r\n");
					}
					else
					{
						sprintf(pSI->buffSend,"250 Directory deleted successfully.\r\n");
					}
				
			}
		*/

	if( strstr( szCmd,"RNFR" )||strstr(szCmd,"RNTO")){
		char *file = strtok( NULL,"\r\n" );

		if(strstr(szCmd,"RNFR")){
			GetCurrentDirectoryA( MAX_PATH,reName );
			if(reName[strlen(reName)-1]!='\\')	strcat(reName,"\\");
			strcat(reName,file);
			sprintf( pSI->buffSend,"350 %s name copyd.\r\n",file);
		}
		else{
			GetCurrentDirectoryA( MAX_PATH,szCurrDir );
			if(szCurrDir[strlen(szCurrDir)-1]!='\\')	strcat(szCurrDir,"\\");
			strcat(szCurrDir,file);
			if(rename(reName,szCurrDir)==0)
				sprintf( pSI->buffSend,"250 %s has renamed.\r\n",file);
			else sprintf( pSI->buffSend,"250 %s has not renamed.\r\n",file);
		}
		
		if( SendRes( pSI ) == -1 ) return -1;
		return 250;
	}

	if( strstr( szCmd,"SYST" ) ) 
	{
		sprintf( pSI->buffSend,"%s","215 Windows_NT Version 4.0\r\n");
		if( SendRes( pSI ) == -1 ) return -1;
		return OS_TYPE;
	}
	if( strstr( szCmd,"TYPE") ) 
	{
		char *szType = strtok(NULL,"\r\n");
		if( szType == NULL ) szType = "A";
		sprintf(pSI->buffSend,"200 Type set to %s.\r\n",szType );
		printf(pSI->buffSend);
		if( SendRes( pSI ) == -1 ) 
			return -1;
		return CMD_OK;		
	}
	if( strstr( szCmd,"REST" ) )
	{
		sprintf( pSI->buffSend,"504 Reply marker must be 0.\r\n");
		if( SendRes( pSI ) == -1 ) 
			return -1;
		return REPLY_MARKER;		
	}
	if( strstr( szCmd,"NOOP") ) 
	{
		sprintf( pSI->buffSend,"200 NOOP command successful.\r\n");
		if( SendRes( pSI ) == -1 ) 
			return -1;
		return CMD_OK;		
	}
	//其余都是无效的命令
	sprintf(pSI->buffSend,"500 '%s' command not understand.\r\n",szCmd );
	if( SendRes( pSI ) == -1 ) return -1;	
	return nRetVal;
}

///////////////////////////////////////////////////////////////////////////////////////////
//其他函数
char* GetLocalAddress()
{
    struct in_addr *pinAddr;
    LPHOSTENT	lpHostEnt;
	int			nRet;
	int			nLen;
	char        szLocalAddr[80];
	memset( szLocalAddr,0,sizeof(szLocalAddr) );
	// Get our local name
    nRet = gethostname(szLocalAddr,sizeof(szLocalAddr) );
	if (nRet == SOCKET_ERROR)
	{
		return NULL;
	}
	// "Lookup" the local name
	lpHostEnt = gethostbyname(szLocalAddr);
    if (NULL == lpHostEnt)	
	{
		return NULL;
	}
    // Format first address in the list
	pinAddr = ((LPIN_ADDR)lpHostEnt->h_addr);
	nLen = strlen(inet_ntoa(*pinAddr));
	if ((DWORD)nLen > sizeof(szLocalAddr))
	{
		WSASetLastError(WSAEINVAL);
		return NULL;
	}
	return inet_ntoa(*pinAddr);
}

char* GetIPAddress()
{    
	return szIpAddr;
}

int GetFileList( LPFILE_INF pFI, UINT nArraySize, const char* szPath  )
{
	WIN32_FIND_DATA  wfd;
	int idx = 0;
	CHAR lpFileName[MAX_PATH];
	GetCurrentDirectory( MAX_PATH,lpFileName );
	strcat( lpFileName,"\\" );
	strcat( lpFileName, szPath );
	HANDLE hFile = FindFirstFile( lpFileName, &wfd );
	if ( hFile != INVALID_HANDLE_VALUE ) 
	{
		pFI[idx].dwFileAttributes = wfd.dwFileAttributes;
		lstrcpy( pFI[idx].szFileName, wfd.cFileName );
		pFI[idx].ftCreationTime = wfd.ftCreationTime;
		pFI[idx].ftLastAccessTime = wfd.ftLastAccessTime;
		pFI[idx].ftLastWriteTime  = wfd.ftLastWriteTime;
		pFI[idx].nFileSizeHigh    = wfd.nFileSizeHigh;
		pFI[idx++].nFileSizeLow   = wfd.nFileSizeLow;
		while( FindNextFile( hFile,&wfd ) && idx < (int)nArraySize ) 
		{
			pFI[idx].dwFileAttributes = wfd.dwFileAttributes;
			lstrcpy( pFI[idx].szFileName, wfd.cFileName );
			pFI[idx].ftCreationTime = wfd.ftCreationTime;
			pFI[idx].ftLastAccessTime = wfd.ftLastAccessTime;
			pFI[idx].ftLastWriteTime  = wfd.ftLastWriteTime;
			pFI[idx].nFileSizeHigh    = wfd.nFileSizeHigh;
			pFI[idx++].nFileSizeLow   = wfd.nFileSizeLow;
		}
		FindClose( hFile );
	}
	return idx;
}
char* HostToNet( char* szPath ) 
{
	int idx = 0;
	if( NULL == szPath ) return NULL;
	strlwr( szPath );
	while( szPath[idx] ) 
	{ 
		if( szPath[idx] == '\\' )
			szPath[idx] = '/';
		idx ++;
	}
	return szPath;
}

char* NetToHost( char* szPath )
{
	int idx = 0;
	if( NULL == szPath ) return NULL;
	strlwr( szPath );
	while( szPath[idx] ) 
	{ 
		if( '/' == szPath[idx]  )
			szPath[idx] = '\\';
		idx ++;
	}
	return szPath;
}
char* RelativeDirectory( char* szDir )
{
	int nStrLen = strlen(DEFAULT_HOME_DIR);
	if( !strnicmp( szDir,DEFAULT_HOME_DIR, nStrLen ) ) 
		szDir += nStrLen;

	if( szDir && szDir[0] == '\0' ) szDir = "/";
	
	return HostToNet(szDir);
}
char* AbsoluteDirectory( char* szDir )
{
	char szTemp[MAX_PATH];
	strcpy( szTemp,DEFAULT_HOME_DIR+2 );
	if( NULL == szDir ) return NULL;
	if( '/' == szDir[0] )
		strcat( szTemp, szDir );
	szDir = szTemp ;	
	return NetToHost(szDir);
}



