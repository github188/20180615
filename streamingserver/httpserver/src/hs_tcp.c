/*
 *  File:       Stcp.c for Windows 95/98/NT
 *
 *  Contains:   Simple TCP interface routines
 *
 *  $History: hs_tcp.c $
 * 
 * *****************  Version 6  *****************
 * User: Shengfu      Date: 06/01/23   Time: 4:13p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/httpserver/src
 * 
 * *****************  Version 12  *****************
 * User: Yun          Date: 04/03/19   Time: 2:48p
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/network/netap/httpserver/src
 * Using select() to check available input data instead of ioctlsocket()
 * 
 * *****************  Version 11  *****************
 * User: Yun          Date: 04/03/10   Time: 11:11a
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/network/netap/httpserver/src
 * Merge with SOC project
 * 
 * *****************  Version 10  *****************
 * User: Yun          Date: 03/10/28   Time: 4:37p
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/network/netap/httpserver/src
 * closesocket if ioctlsocket() failed
 * 
 * *****************  Version 9  *****************
 * User: Yun          Date: 03/09/29   Time: 4:41p
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/network/netap/httpserver/src
 * Modify the error check of tcp send
 * 
 * *****************  Version 8  *****************
 * User: Yun          Date: 03/09/23   Time: 9:58a
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/network/netap/httpserver/src
 * Let variable of struct sockaddr_in four-byte alignment
 * 
 * *****************  Version 7  *****************
 * User: Jason        Date: 03/08/22   Time: 11:33a
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/network/netap/httpserver/src
 * 
 * *****************  Version 6  *****************
 * User: Jason        Date: 03/08/12   Time: 4:40p
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/network/netap/httpserver/src
 * 
 * *****************  Version 5  *****************
 * User: Joe          Date: 03/08/12   Time: 11:52a
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/network/netap/httpserver/src
 * 1. Porting to Trimedia MDS card.
 *  To Do:
 */

// Modified by Joe 
/* 
#ifdef _WIN32_
#include <Winsock2.h>
#endif
*/
#include "sockdef.h"

#include "httpserver_local.h"
#include "hs_tcp.h"

#define  TCP_MAX_BACKLOG 5

/*
 *  The Stcp routines are the interface between RomPager and the device
 *  TCP routines.  The interface assumes that multiple connections to
 *  TCP can be maintained to support overlapping requests.  This is
 *  especially necessary to support the Netscape browser, as it fires
 *  off multiple requests to speed up graphics display.
 *
 *  The constant kStcpNumberOfConnections in Stcp.h defines how many
 *  connections are to be supported.
 *
 *  The StcpInit and StcpDeInit routines operate on all connections.
 *  They allocate/free memory for all connections and set up any
 *  necessary states for the entire interface.
 *
 *  The other routines all are passed a connection id which will range
 *  from 0 to (kStcpNumberOfConnections - 1).  The easiest way to use
 *  the connection id is as an index to an array of per connection data.
 */

typedef enum {
    SEND_PENDING,
    SEND_OK,
    SEND_ERROR
} TSendStatus;

/*
 * structure for BSD/Winsock style TCP/IP sockets
 *
 *  There is a tradeoff between generality and resource use here:
 *  RomPager opens a single listener port, and accepts multiple connections
 *  on the port.  We take advantage of this, and create a single 'master'
 *  server socket, which we use to retrieve client connections.
 */

typedef struct {
    SOCKET      skClientSocket;
    void*       pReceiveBuffer;
    TSendStatus tSendStatus;
    char*       pPendingSendBuffer;
    UINT        uiPendingSendBufferLen;
    DWORD       dwCurrentByteSent;
} TTCPConnectInfo, *PTTCPConnectInfo;

static PTTCPConnectInfo gTcpInfo = NULL;
static SOCKET           gTcpServerSocket = SOCKET_ERROR;
static ULONG            gMaxConnection   = DEFAULT_MAX_CONNECTION;
static DWORD            gReceiveBufSize  = DEFAULT_TCP_RECEIVE_SIZE;
static int              gTcpError;

static int SendBuffer(PTTCPConnectInfo ptConnection, char *pSendBuf,
                        UINT uiSendLength);


/*
 *  The StcpInit routine is called once when RomPager starts up, so that
 *  the TCP interface can initialize any internal variables and processes.
 *
 *  Returns:
 *      theResult:
 *          eRpNoError          - no error
 *          eRpOutOfMemory      - can't allocate memory
 *          eRpTcpInitError     - can't initialize TCP
 */

SCODE TcpInitial(ULONG ulMaxConnection, DWORD dwReceiveBufferSize)
{
    int     iIndex;
/*
#ifdef _WIN32_
	WSADATA wsaData;
#endif
*/
    // Allocate space for connection info.
    gMaxConnection = ulMaxConnection;
    gReceiveBufSize = dwReceiveBufferSize;
    gTcpInfo = (PTTCPConnectInfo) calloc(gMaxConnection, sizeof(TTCPConnectInfo));

    // Initialize the structure, we have no connections yet.
    if (gTcpInfo)
    {
        for (iIndex = 0; iIndex < (int) gMaxConnection; iIndex++)
        {
            gTcpInfo[iIndex].skClientSocket = SOCKET_ERROR;
            gTcpInfo[iIndex].tSendStatus = SEND_OK;
            gTcpInfo[iIndex].uiPendingSendBufferLen = 0;
            gTcpInfo[iIndex].dwCurrentByteSent = 0;
            // allocate 4 bytes more for post data termination
            gTcpInfo[iIndex].pReceiveBuffer = (void *)malloc(gReceiveBufSize + 4);
            if (!gTcpInfo[iIndex].pReceiveBuffer)
            {
                return S_FAIL;
            }
        }
    }
    else
    {
        return S_FAIL;
    }

    // marked off by Joe 2003/07/28, using Network_Init externally
/*
#ifdef _WIN32_
	// initialize use of ws2_32.lib
	if (WSAStartup(0x202, &wsaData) == SOCKET_ERROR)
	{
        gTcpError = WSAGetLastError();
		return S_FAIL;
	}
#endif
*/
    return S_OK;
}

/*
 *  The StcpDeInit routine is called once at when RomPager finishes so that
 *  the TCP interface can deinitialize any internal variables and processes.
 *  Any receive buffers that are still allocated, should be deallocated here.
 *
 *  Returns:
 *      theResult:
 *          eRpNoError          - no error
 *          eRpTcpDeInitError   - can't deinitialize TCP
 */
void TcpRelease(void)
{
    int iIndex;

    if (gTcpInfo)
    {
        for (iIndex = 0; iIndex < (int) gMaxConnection; iIndex++)
        {
            // Modified by Joe 2003/07/28, to avoid unsigned integer
            if (gTcpInfo[iIndex].pReceiveBuffer)
            {
                free(gTcpInfo[iIndex].pReceiveBuffer);
                gTcpInfo[iIndex].pReceiveBuffer = NULL;
            }
            if ((int)gTcpInfo[iIndex].skClientSocket >= 0)
            {
                closesocket(gTcpInfo[iIndex].skClientSocket);
            }
        }
        free(gTcpInfo);
    }

    if (gTcpServerSocket != SOCKET_ERROR)
    {
        closesocket(gTcpServerSocket);
        gTcpServerSocket = SOCKET_ERROR;
    }
}

/*
 *  The StcpOpenPassive routine is called to setup a passive TCP connection
 *  on the supplied port.  The routine returns when a connection has been
 *  set up, not when the connection has been made.  The StcpConnectionStatus
 *  call is used to detect an incoming connection.
 *
 *  Inputs:
 *      theConnection:              - connection id
 *      thePort:                    - value of the local port
 *
 *  Returns:
 *      theResult:
 *          eRpNoError              - no error
 *          eRpTcpCannotOpenPassive - can't open a passive connection
 */

SCODE TcpOpenServerSocket(USHORT usHttpPort)
{
    int one = 1;
    struct sockaddr_in  saServerAddr;

    if (gTcpServerSocket == SOCKET_ERROR)
    {
        gTcpServerSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (gTcpServerSocket == SOCKET_ERROR)
        {
            return S_FAIL;
        }

        if (setsockopt(gTcpServerSocket, SOL_SOCKET, SO_REUSEADDR,
                        (char *) &one, sizeof(one)) == SOCKET_ERROR)
        {
            gTcpError = WSAGetLastError();
            closesocket(gTcpServerSocket);
            return S_FAIL;
        }

        // Zero out the sock_addr structures.
        // This MUST be done before the socket calls.
        memset(&saServerAddr, 0, sizeof(saServerAddr));
        saServerAddr.sin_family = AF_INET;
        saServerAddr.sin_port = htons((unsigned short) usHttpPort);
        if (bind(gTcpServerSocket, (struct sockaddr *) &saServerAddr,
                    sizeof(saServerAddr)) == SOCKET_ERROR)
        {
            gTcpError = WSAGetLastError();
            closesocket(gTcpServerSocket);
            return S_FAIL;
        }

        // Enable connections, with a small allowed backlog.
        if (listen(gTcpServerSocket, TCP_MAX_BACKLOG) == SOCKET_ERROR)
        {
            gTcpError = WSAGetLastError();
            closesocket(gTcpServerSocket);
            return S_FAIL;
        }
    }

    return S_OK;
}


/*
 *  The StcpConnectionStatus routine is called to determine when the
 *  connection has been made.
 *
 *  Inputs:
 *      theConnection:              - connection id
 *      theCompletionStatusPtr:     - pointer to the connection status
 *      theRemoteAddressPtr:        - pointer to an IP address
 *      theLocalAddressPtr:         - pointer to an IP address
 *
 *  Returns:
 *      theCompletionStatusPtr:     - eRpTcpComplete, if a connection
 *                                  - eRpTcpPending, if no connection yet
 *                                  - eRpTcpOpenPassiveTimeout, if open
 *                                          passive connection has timed out
 *      theRemoteAddressPtr:        - the IP address of the remote connection
 *      theLocalAddressPtr:         - the IP address of the local connection
 *      theResult:
 *          eRpNoError              - no error
 *          eRpTcpCannotOpenPassive - can't open a passive connection (an I/O error occurred)
 */

SCODE TcpListenConnection(UINT uiConnectionIndex, TTcpStatus *ptStatus,
                          DWORD *pdwClientAddress, USHORT *pusClientPort,
                          DWORD *pdwServerAddress, USHORT *pusServerPort)
{
    fd_set              fdset;
    int                 iResult;
    struct timeval      timeVal;
    struct linger       lingerVal;    
    int                 iLocalAddrLength;
    struct sockaddr_in  saLocalAddr;
    struct sockaddr_in  saPeerAddr;
    int                 iPeerLength = sizeof(saPeerAddr);
    int                 fd;
    int                 on = TRUE;
    int                 one = 1;

    timeVal.tv_sec = 0;
    timeVal.tv_usec = 0;
    lingerVal.l_onoff = 0;
    lingerVal.l_linger = 0;

    FD_ZERO(&fdset);
    FD_SET(gTcpServerSocket, &fdset);

    /*
        See if we can get a connection by polling
        the listener socket, without blocking.
    */
    iResult = select(FD_SETSIZE, &fdset, NULL, NULL, &timeVal);

    if (iResult == 1)
    {
        // Get a connection now.
        fd = (int) accept(gTcpServerSocket, (struct sockaddr *) &saPeerAddr, &iPeerLength);
        if (fd == SOCKET_ERROR)
        {
            gTcpError = WSAGetLastError();
            return S_FAIL;
        }
        else
        {
            // We're unpleasantly surprised if the connection already exists!
            iResult = ioctlsocket(fd, FIONBIO, (unsigned long *) &on);
            if (iResult == SOCKET_ERROR)
            {
                gTcpError = WSAGetLastError();
                closesocket(fd);
                return S_FAIL;
            }

            if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *) &one,
                            sizeof(one)) == SOCKET_ERROR)
            {
                gTcpError = WSAGetLastError();
                closesocket(fd);
                return S_FAIL;
            }
            // Set the socket to close immediately on a close call.
            if (setsockopt(fd, SOL_SOCKET, SO_LINGER, (char *) &lingerVal,
                            sizeof(lingerVal)) == SOCKET_ERROR)
            {
                gTcpError = WSAGetLastError();
                closesocket(fd);
                return S_FAIL;
            }
            iLocalAddrLength = sizeof(saLocalAddr);
            iResult = getsockname(fd, (struct sockaddr *) &saLocalAddr,
                                    &iLocalAddrLength);
            if (iResult == SOCKET_ERROR)
            {
                return S_FAIL;
            }
            gTcpInfo[uiConnectionIndex].skClientSocket = fd;
            
            *ptStatus = TCP_COMPLETE;
            *pdwClientAddress = saPeerAddr.sin_addr.s_addr;
            *pusClientPort = ntohs(saPeerAddr.sin_port);
            *pdwServerAddress = saLocalAddr.sin_addr.s_addr;
            *pusServerPort = ntohs(saLocalAddr.sin_port);
            return S_OK;
        }
    }
    else if (iResult == 0)
    {
        *ptStatus = TCP_LISTEN_PENDING;
        return S_OK;
    }
    else
    {
        if (iResult == SOCKET_ERROR)
        {
            gTcpError = WSAGetLastError();
        }

        return S_FAIL;
    }
}

/*
 *  The StcpSend routine is called to send a buffer of information over
 *  the connection.  The call is an asynchronous call and completes when
 *  the send has been started.  The StcpSendStatus call is used to detect
 *  the completion of the send.  The last buffer received with the
 *  StcpReceiveStatus call can be deallocated in this call.
 *
 *  Inputs:
 *      theConnection:          - connection id
 *      theSendPtr:             - pointer to buffer to be sent
 *      theSendLength:          - length of characters to be sent
 *
 *  Returns:
 *      theResult:
 *          eRpNoError          - no error
 *          eRpTcpSendError     - can't send the buffer
 *          eRpTcpNoConnection  - there isn't a currently opened connection
 */

SCODE TcpSend(UINT uiConnectionIndex, char *pSendBuf, UINT uiSendLength, 
              TTcpStatus *pStatus, DWORD* pdwByteSent)
{
    PTTCPConnectInfo  ptConnection;
    int iSentLen;
    
    ptConnection = &gTcpInfo[uiConnectionIndex];
    
    if (pdwByteSent != NULL)
        *pdwByteSent = ptConnection->dwCurrentByteSent;
    
    if (ptConnection->skClientSocket == SOCKET_ERROR)
    {
        return S_FAIL;
    }
    else
    {
        if (ptConnection->tSendStatus == SEND_PENDING)
        {
            iSentLen = SendBuffer(ptConnection, ptConnection->pPendingSendBuffer,
                                  ptConnection->uiPendingSendBufferLen);
        }
        else
        {
            ptConnection->dwCurrentByteSent = 0;
            iSentLen = SendBuffer(ptConnection, pSendBuf, uiSendLength);
        }
        
        ptConnection->dwCurrentByteSent += iSentLen;
        if (pdwByteSent != NULL)
            *pdwByteSent = ptConnection->dwCurrentByteSent;
            
        if (ptConnection->tSendStatus == SEND_ERROR)
        {
            return S_FAIL;
        }
        else if (ptConnection->tSendStatus == SEND_OK)
        {
            *pStatus = TCP_COMPLETE;
        }
        else
        {
            *pStatus = TCP_SEND_PENDING;
        }
    }

    return S_OK;
}

static int SendBuffer(PTTCPConnectInfo ptConnection, char *pSendBuf,
                        UINT uiSendLength)
{
    int  iResult;

    iResult = send(ptConnection->skClientSocket, pSendBuf, uiSendLength, 0);

    if (iResult == SOCKET_ERROR)
    {
        gTcpError = WSAGetLastError();
#if defined(_WIN32_)
        if (gTcpError == WSAEWOULDBLOCK /* SendQueueFull */ )
#elif defined(_PSOS_TRIMEDIA)
        if (gTcpError == EWOULDBLOCK /* SendQueueFull */ )
#elif defined(_LINUX)
        if (gTcpError == EWOULDBLOCK /* SendQueueFull */ )
#endif
        {
            ptConnection->tSendStatus = SEND_PENDING;
            ptConnection->pPendingSendBuffer = pSendBuf;
            ptConnection->uiPendingSendBufferLen = uiSendLength;
        }
        else
        {
            ptConnection->tSendStatus = SEND_ERROR;
        }
    }
    // Modified by Joe 2003/07/28
    else if (iResult != (int)uiSendLength)
    {
    	// Modified by Yun, 2003/09/29
        if (iResult >= 0 && iResult < (int) uiSendLength)
        {
            ptConnection->tSendStatus = SEND_PENDING;
            ptConnection->pPendingSendBuffer = pSendBuf + iResult;
            ptConnection->uiPendingSendBufferLen = uiSendLength - iResult;
        }
        else
        {
            // some other kind of wierd error
            ptConnection->tSendStatus = SEND_ERROR;
        }
    }
    else
    {
        ptConnection->tSendStatus = SEND_OK;
    }

    return iResult;
}


/*
 *  The StcpReceiveStatus routine is called to determine whether a buffer
 *  has been received on the connection.
 *
 *  Inputs:
 *      theConnection:          - connection id
 *      theCompletionStatusPtr: - pointer to the receive status
 *      theReceivePtrPtr:       - pointer to a buffer pointer
 *      theReceiveLengthPtr:    - pointer to the received length
 *
 *  Returns:
 *      theCompletionStatusPtr: - eRpTcpComplete, if a buffer has been received
 *                              - eRpTcpPending, if no buffer has been received yet
 *      theReceivePtrPtr:       - a pointer to the buffer contents
 *      theReceiveLengthPtr:    - the length of the received buffer
 *      theResult:
 *          eRpNoError          - no error
 *          eTpTcpReceiveError  - can't receive a buffer (an I/O error occurred)
 */

SCODE TcpReceive(UINT uiConnectionIndex, TTcpStatus *pStatus,
                 DWORD dwReadOffset, char **ppReceiveBuf, UINT *puiReceiveLength)
{
    int  iBytesAvail;
    int  iRecLen;
    int  iStatus;
    PTTCPConnectInfo ptConnection;
    SCODE sResult;
    fd_set fdReadSet;
    struct timeval timeout;    

    ptConnection = &gTcpInfo[uiConnectionIndex];
    sResult = S_OK;

    if (ptConnection->skClientSocket == SOCKET_ERROR)
    {
        sResult = S_FAIL;
    }
    else
    {	
		FD_ZERO( &fdReadSet );
		FD_SET( ptConnection->skClientSocket, &fdReadSet );
	
		timeout.tv_sec = 0;
		timeout.tv_usec = 0;
		    
	    iStatus = select( ptConnection->skClientSocket+1, &fdReadSet, NULL, NULL, &timeout ) ;
	
	    if( iStatus < 0 )
	    {
            gTcpError = WSAGetLastError();
            sResult = S_FAIL;
	    }
	    else if( iStatus == 0 )
	    {
	    	*pStatus = TCP_RECEIVE_PENDING;
	    }
	    else
	    {
	    	if ( FD_ISSET( ptConnection->skClientSocket, &fdReadSet ) )
	    	{
                iRecLen = recv(ptConnection->skClientSocket,
                               (char*)ptConnection->pReceiveBuffer + dwReadOffset,
                               gReceiveBufSize - dwReadOffset, 0);

                if (iRecLen == SOCKET_ERROR || iRecLen == 0)
                {
					gTcpError = WSAGetLastError();
                    sResult = S_FAIL;
                }
                else
                {
                    *pStatus = TCP_COMPLETE;
                    *ppReceiveBuf = (char *) ptConnection->pReceiveBuffer + dwReadOffset;
                    *puiReceiveLength = iRecLen;		
					//*((char*)ptConnection->pReceiveBuffer+dwReadOffset) = 0;
                }	    		
	    	}	    	
	    }
/*    	
        iBytesAvail = 0;
        iStatus = ioctlsocket(ptConnection->skClientSocket, FIONREAD,
                                (unsigned long *) &iBytesAvail);

        if (iStatus == SOCKET_ERROR)
        {
            gTcpError = WSAGetLastError();
            sResult = S_FAIL;
        }
        else
        {
            if (iBytesAvail)
            {
                // Modified by Joe 2003/07/28
                if (iBytesAvail > (int)(gReceiveBufSize - dwReadOffset))
                {
                    iBytesAvail = (int) (gReceiveBufSize - dwReadOffset);
                }

                iRecLen = recv(ptConnection->skClientSocket,
                               (char*)ptConnection->pReceiveBuffer + dwReadOffset,
                               iBytesAvail, 0);

                if (iRecLen == SOCKET_ERROR || (iRecLen > iBytesAvail))
                {
                    if (iRecLen == SOCKET_ERROR)
                    {
                        gTcpError = WSAGetLastError();
                    }
                    sResult = S_FAIL;
                }
                else
                {
                    *pStatus = TCP_COMPLETE;
                    *ppReceiveBuf = (char *) ptConnection->pReceiveBuffer + dwReadOffset;
                    *puiReceiveLength = iRecLen;
                }
            }
            else
            {
                *pStatus = TCP_RECEIVE_PENDING;
            }
        }
*/        
    }

    return sResult;
}

/*
 *  The StcpCloseConnection routine is called to gracefully close an active
 *  connection.  The call is asynchronous and completes when the closing
 *  process has been started.  The StcpCloseStatus routine is used to
 *  detect when the close has completed.
 *
 *  Inputs:
 *      theConnection:          - connection id
 *
 *  Returns:
 *      theResult:
 *          eRpNoError          - no error
 *          eRpTcpAlreadyClosed - the other side already closed the connection
 *          eRpTcpCloseError    - can't close the connection
 */

SCODE TcpCloseConnection(UINT uiConnectionIndex)
{
    PTTCPConnectInfo ptConnection;
    SCODE sResult;

    ptConnection = &gTcpInfo[uiConnectionIndex];
    sResult = S_OK;
    
    if (ptConnection->skClientSocket == SOCKET_ERROR)
    {
        return S_FAIL;
    }

    if (closesocket(ptConnection->skClientSocket) == SOCKET_ERROR)
    {
        gTcpError = WSAGetLastError();
        sResult = S_FAIL;
    }
    
    ptConnection->skClientSocket = SOCKET_ERROR;

    return sResult;
}

SCODE TcpAbortConnection(UINT uiConnectionIndex)
{
    PTTCPConnectInfo ptConnection;
    SCODE sResult;

    ptConnection = &gTcpInfo[uiConnectionIndex];
    sResult = S_OK;

    if (ptConnection->skClientSocket == SOCKET_ERROR)
    {
        sResult = S_FAIL;
    }
    else if (shutdownsocket(ptConnection->skClientSocket, SD_BOTH) == SOCKET_ERROR)
    {
        sResult = S_FAIL;
    }
    else if (closesocket(ptConnection->skClientSocket) == SOCKET_ERROR)
    {
        gTcpError = WSAGetLastError();
        sResult = S_FAIL;
    }

    ptConnection->skClientSocket = SOCKET_ERROR;
    return sResult;
}

SCODE TcpGetConnectionSocket(UINT uiConnectionIndex, SOCKET *psSocket)
{
    PTTCPConnectInfo ptConnection;

    ptConnection = &gTcpInfo[uiConnectionIndex];
    *psSocket = ptConnection->skClientSocket;
    
    return S_OK;
}

SCODE TcpClearConnectionSocket(UINT uiConnectionIndex)
{
    PTTCPConnectInfo ptConnection;

    ptConnection = &gTcpInfo[uiConnectionIndex];
    ptConnection->skClientSocket = SOCKET_ERROR;
    
    return S_OK;
}
