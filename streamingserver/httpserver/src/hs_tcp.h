/*
 *  File:       Stcp.h
 *
 *  Contains:   Prototypes for simple interface routines to TCP/IP
 *
 */



#ifndef _HS_TCP_
#define _HS_TCP_

#include "typedef.h"

#define DEFAULT_TCP_RECEIVE_SIZE      1460
#define DEFAULT_HTTP_PORT             80


/*
    Simple Tcp call completion states
*/

typedef enum {
    TCP_COMPLETE,
    TCP_LISTEN_PENDING,
    TCP_SEND_PENDING,
    TCP_RECEIVE_PENDING
} TTcpStatus;


SCODE TcpInitial(ULONG ulMaxConnection, DWORD dwReceiveBufferSize);
void  TcpRelease(void);
SCODE TcpOpenServerSocket(USHORT usHttpPort);
SCODE TcpListenConnection(UINT uiConnectionIndex, TTcpStatus *ptStatus,
                          DWORD *pdwClientAddress, USHORT *pusClientPort,
                          DWORD *pdwServerAddress, USHORT *pusServerPort);
SCODE TcpSend(UINT uiConnectionIndex, char *pSendBuf, UINT uiSendLength, 
              TTcpStatus *pStatus, DWORD* pdwByteSent);
SCODE TcpReceive(UINT uiConnectionIndex, TTcpStatus *pStatus,
                 DWORD dwReadOffset, char **ppReceiveBuf, UINT *puiReceiveLength);
SCODE TcpCloseConnection(UINT uiConnectionIndex);
SCODE TcpAbortConnection(UINT uiConnectionIndex);
SCODE TcpGetConnectionSocket(UINT uiConnectionIndex, SOCKET *psSocket);
SCODE TcpClearConnectionSocket(UINT uiConnectionIndex);


#endif
