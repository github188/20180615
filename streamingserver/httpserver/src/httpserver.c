/*  Copyright (c) 2003 Vivotek Inc. All rights reserved.
 *  $Header: /RD_1/Project/PNX1300_PSOS/Farseer/network/netap/httpserver/src/httpserver.c 2     06/01/23 4:13p Shengfu $
 *  +-----------------------------------------------------------------+
 *  | THIS SOFTWARE IS FURNISHED UNDER A LICENSE AND MAY ONLY BE USED |
 *  | AND COPIED IN ACCORDANCE WITH THE TERMS AND CONDITIONS OF SUCH  |
 *  | A LICENSE AND WITH THE INCLUSION OF THE THIS COPY RIGHT NOTICE. |
 *  | THIS SOFTWARE OR ANY OTHER COPIES OF THIS SOFTWARE MAY NOT BE   |
 *  | PROVIDED OR OTHERWISE MADE AVAILABLE TO ANY OTHER PERSON. THE   |
 *  | OWNERSHIP AND TITLE OF THIS SOFTWARE IS NOT TRANSFERRED.        |
 *  |                                                                 |
 *  | THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE WITHOUT   |
 *  | ANY PRIOR NOTICE AND SHOULD NOT BE CONSTRUED AS A COMMITMENT BY |
 *  | VIVOTEK INC.                                                    |
 *  +-----------------------------------------------------------------+
 *  Module name        :   HTTPServer
 *  File name          :   HTTPServer.c
 *  File description   :   HTTP Server Module API
 *  Author             :   Jason Yang
 *  Created at         :   2003/05/26
 *  Note               :
 *
 *  $History: httpserver.c $
 * 
 * *****************  Version 2  *****************
 * User: Shengfu      Date: 06/01/23   Time: 4:13p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/httpserver/src
 * 
 * *****************  Version 3  *****************
 * User: Yun          Date: 04/08/18   Time: 8:04p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/httpserver/src
 * Set content language during initialization
 * 
 * *****************  Version 11  *****************
 * User: Yun          Date: 04/02/19   Time: 10:11a
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/network/netap/httpserver/src
 * Add checking memory allocated condition
 * 
 * *****************  Version 10  *****************
 * User: Joe          Date: 03/12/11   Time: 3:37p
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/network/netap/httpserver/src
 * Add #include "common.h"
 * 
 * *****************  Version 9  *****************
 * User: Yun          Date: 03/09/23   Time: 9:59a
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/network/netap/httpserver/src
 * remove OS_Init(), reset the structure of thread options 
 * 
 * *****************  Version 8  *****************
 * User: Jason        Date: 03/09/10   Time: 3:34p
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/network/netap/httpserver/src
 * 
 * *****************  Version 7  *****************
 * User: Jason        Date: 03/09/10   Time: 2:14p
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/network/netap/httpserver/src
 * Modify connection timeout control
 * 
 * *****************  Version 6  *****************
 * User: Jason        Date: 03/08/12   Time: 4:40p
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/network/netap/httpserver/src
 * 
 * *****************  Version 5  *****************
 * User: Joe          Date: 03/08/12   Time: 11:52a
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/network/netap/httpserver/src
 * 1. Porting to Trimedia MDS card.
 * 
 * *****************  Version 4  *****************
 * User: Jason        Date: 03/08/01   Time: 5:41p
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/network/netap/httpserver/src
 * 
 * *****************  Version 3  *****************
 * User: Joe          Date: 03/07/30   Time: 11:28a
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/network/netap/httpserver/src
 * Fix resource leak problem.
 */


/*!
 **********************************************************************
 * Copyright (C) 2003 Vivotek, Inc. All rights reserved.
 *
 * \file
 * HTTPServer.c
 *
 * \brief
 * HTTP Server Module API
 *
 * \date
 * 2003/05/26
 *
 * \author
 * Jason Yang
 *
 *
 **********************************************************************
 */
#include "osisolate.h"
#include "common.h"
#include "httpserver.h"
#include "httpserver_local.h"
#include "hs_file.h"

SCODE HTTPServer_GetVersion(BYTE *byMajor, BYTE *byMinor, BYTE *byBuild, BYTE *byRevision)
{
	*byMajor = (BYTE)(HTTPSERVER_VERSION & 0x000000FF);
	*byMinor = (BYTE)((HTTPSERVER_VERSION & 0x0000FF00) >> 8);
	*byBuild = (BYTE)((HTTPSERVER_VERSION & 0x00FF0000) >> 16);
	*byRevision = (BYTE)((HTTPSERVER_VERSION & 0xFF000000) >> 24);
	
	return S_OK;
}

SCODE HTTPServer_Initial(HANDLE *phHTTPServerObj, THTTPServer_InitSettings *pstInitSettings)
{
    PTHttpServerInfo 		pThis;
    TOSThreadInitOptions 	tThreadOption;
    DWORD					dwStackSize;

    *phHTTPServerObj = NULL;
    
	// check version
	if (pstInitSettings->dwVersion != HTTPSERVER_VERSION)
		return ERR_HTTPSERVER_VERSION;

    // allocate object data structure		
	pThis = (THttpServerInfo*) malloc(sizeof(THttpServerInfo));	
	
	if (pThis == NULL)
		return ERR_HTTPSERVER_MALLOC;

    // Initial server info structure
    memset(pThis, 0, sizeof(THttpServerInfo));
    pThis->dwReceiveBufferSize = DEFAULT_RECEIVE_BUFFER_SIZE;
    pThis->usPort = DEFAULT_HTTP_PORT;
    pThis->ulMaxConnections = DEFAULT_MAX_CONNECTION;
    pThis->dwConnectionTimeout = DEFAULT_CONNECTION_TIMEOUT;
    pThis->iThreadPriority = 128;

	if( pstInitSettings->iAuthorizationType == HTTPServer_AuthorizationType_Basic )		
		pThis->iAuthenticateType = HTTPServer_AuthorizationType_Basic;
	else if(pstInitSettings->iAuthorizationType == HTTPServer_AuthorizationType_Digest )		
		pThis->iAuthenticateType = HTTPServer_AuthorizationType_Digest;
	else
		pThis->iAuthenticateType = HTTPServer_AuthorizationType_Basic;

    dwStackSize = 8192;
    
    if (pstInitSettings->dwInitSettingsFlag & HTTPServer_ReceiveBufferSizeFlag)
        pThis->dwReceiveBufferSize = pstInitSettings->dwReceiveBufferSize;
    if (pstInitSettings->dwInitSettingsFlag & HTTPServer_PortFlag)
        pThis->usPort = pstInitSettings->usPort;
    if (pstInitSettings->dwInitSettingsFlag & HTTPServer_MaxConnectionsFlag)
        pThis->ulMaxConnections = pstInitSettings->ulMaxConnections;
    if (pstInitSettings->dwInitSettingsFlag & HTTPServer_ThreadPriorityFlag)
        pThis->iThreadPriority = pstInitSettings->iThreadPriority;
    if (pstInitSettings->dwInitSettingsFlag & HTTPServer_StackSizeFlag)
    	dwStackSize = pstInitSettings->dwStackSize;
    if (pstInitSettings->dwInitSettingsFlag & HTTPServer_ConnectionTimeoutFlag)
    	pThis->dwConnectionTimeout = pstInitSettings->dwConnectionTimeout;
    	
	// add by Yun, 2004/05/27
	if (pstInitSettings->dwInitSettingsFlag & HTTPServer_HostNameFlag)
	{
		strncpy(pThis->szHostName, pstInitSettings->pcHostName, sizeof(pThis->szHostName) - 1);
	}
	else
	{
    	strcpy(pThis->szHostName, DEFAULT_HOSTNAME);
    }
    
    // add by Yun, 2004/08/18
	if (pstInitSettings->dwInitSettingsFlag & HTTPServer_LanguageFlag)
	{
		strncpy(pThis->szLanguage, pstInitSettings->pcLanguage, sizeof(pThis->szLanguage) - 1);
	}
	else
	{
    	strcpy(pThis->szLanguage, DEFAULT_LANGUAGE);
    }    

    // Allocate HTTP connection data structure
    pThis->ptHttpConnections = (PTHttpConnection) calloc (pThis->ulMaxConnections, sizeof(THttpConnection));
    if (pThis->ptHttpConnections == NULL)
        return ERR_HTTPSERVER_MALLOC;

    // Init HTTP connection data structure
    InitHttpConnections(pThis);
            		
	// Initial TCP socket interface	
	if( TcpInitial(pThis->ulMaxConnections, pThis->dwReceiveBufferSize) != S_OK)
	{
		return ERR_HTTPSERVER_MALLOC;
	}
	
    TcpOpenServerSocket(pThis->usPort);

	// Initial File handle interface	
    if( HsFile_Initial(pThis->ulMaxConnections) != S_OK)
    {
    	return ERR_HTTPSERVER_MALLOC;
    }
    
//	OS_Init();
	
    // Create thread terminate and run event		
    OSEvent_Initial(&pThis->hRunEvent, FALSE);
    OSEvent_Initial(&pThis->hTerminateEvent, FALSE);
    
    if((pThis->hRunEvent == NULL) || (pThis->hTerminateEvent == NULL))
    {
    	return ERR_HTTPSERVER_MALLOC;
    }
    
	// Create HTTP main task
	memset(&tThreadOption, 0, sizeof(tThreadOption));
    tThreadOption.dwStackSize = dwStackSize;
    tThreadOption.dwInstance = (DWORD) pThis;
    tThreadOption.dwPriority = pThis->iThreadPriority;
    tThreadOption.pThreadProc = HttpServerMainTask;
    if( OSThread_Initial(&pThis->hThreadObj, &tThreadOption) != S_OK)
    {
    	return ERR_HTTPSERVER_MALLOC;
    }
    if( OSThread_Start(pThis->hThreadObj) != S_OK)
    {
    	return S_FAIL;
    }

    *phHTTPServerObj = pThis;
    
    return S_OK;
}

SCODE HTTPServer_SetParameters(HANDLE hHTTPServerObj, THTTPServer_Settings *pstSettings)
{
    PTHttpServerInfo pThis = (PTHttpServerInfo) hHTTPServerObj;
    
    if (pstSettings->dwSettingsFlag & HTTPServer_ConnectionTimeoutFlag)
    	pThis->dwConnectionTimeout = pstSettings->dwConnectionTimeout;
    	
    return S_OK;
}

SCODE HTTPServer_Start(HANDLE hHTTPServerObj)
{
    PTHttpServerInfo pThis = (PTHttpServerInfo) hHTTPServerObj;
    
    OSEvent_Set(pThis->hRunEvent);
    
    return S_OK;
}

SCODE HTTPServer_Stop(HANDLE hHTTPServerObj)
{
    PTHttpServerInfo pThis = (PTHttpServerInfo) hHTTPServerObj;
    
    if (S_OK != OSEvent_WaitFor(pThis->hRunEvent, 5000))
        return S_FAIL;
    
    return S_OK;
}

SCODE HTTPServer_Release(HANDLE *phHTTPServerObj)
{
    DWORD dwExitCode;
    PTHttpServerInfo pThis = (PTHttpServerInfo) *phHTTPServerObj;

    // Add by Joe 2003/07/28, free unused memory
    HsFile_Release();
    TcpRelease();

    // Modified by Joe 2003/07/28, release should give pointer

    // Add by Joe, 2003/07/28, trigger terminate event
    OSEvent_Set(pThis->hTerminateEvent);
    // Wait for thread terminate
    if (S_OK != OSThread_WaitFor(pThis->hThreadObj, 5000, &dwExitCode))
    {
        OSThread_Terminate(pThis->hThreadObj);
    }
    OSThread_Release(&pThis->hThreadObj);
    OSEvent_Release(&pThis->hRunEvent);
    OSEvent_Release(&pThis->hTerminateEvent);

    // Add by Joe, 2003/07/30, free the http connections
    if(pThis->ptHttpConnections)
    {
        free(pThis->ptHttpConnections);
    }

    // free itself
    free(*phHTTPServerObj);
	*phHTTPServerObj = NULL;
    return S_OK;
}

SCODE HTTPServer_SetCallback(HANDLE hHTTPServerObj, HTTPServer_Callback pfnCallback, DWORD dwInstance)
{
    PTHttpServerInfo pThis = (PTHttpServerInfo) hHTTPServerObj;
    
    pThis->pfnHttpCallback = pfnCallback;
    pThis->dwContext = dwInstance;
    
    return S_OK;
}

SCODE HTTPServer_Disconnect(HANDLE hHTTPServerObj, DWORD dwClientID)
{
    return S_OK;
}

SCODE HTTPServer_SendData(HANDLE hHTTPServerObj, DWORD dwClientID, PCHAR pSendBuffer,
                          DWORD dwSendLength, BOOL bLastData, DWORD* pdwByteSent)
{
    PTHttpServerInfo pThis = (PTHttpServerInfo) hHTTPServerObj;
    PTHttpConnection ptHttpConn;
    TTcpStatus pStatus;
    SCODE sResult;

    ptHttpConn = &pThis->ptHttpConnections[dwClientID];
    
	// check connection state
	if ((ptHttpConn->tHttpState == HTTP_STATE_SEND_HEADERS) && 
	    (ptHttpConn->tDataSourceType == DATA_SOURCE_PENDING))
	{
	    sResult = S_HTTPSERVER_SEND_PENDING;
	}
	else if (ptHttpConn->tHttpState == HTTP_STATE_DATA_PENDING)
	{
    	// save data pointer and send 
        sResult = TcpSend(dwClientID, pSendBuffer, dwSendLength, &pStatus, pdwByteSent);
        if (sResult == S_OK)
        {
            if (pStatus == TCP_COMPLETE)
            {
                sResult = S_HTTPSERVER_SEND_COMPLETE;
                if (bLastData)
                {
                    // change http connection state
                    ptHttpConn->tHttpState = HTTP_STATE_RESPONSE_COMPLETE;
                }
            }
            else
            {
                sResult = S_HTTPSERVER_SEND_PENDING;
            }
        }
    }
    else
    {
        sResult = S_FAIL;
    }
    
    return sResult;
}

//SCODE HTTPServer_SetClientParameters(HANDLE hHTTPServerObj, DWORD dwClientID, THTTPServer_ClientSettings *pstClientSettings)
//{
    //return S_OK
//}

SCODE HTTPServer_TakeClientOut(HANDLE hHTTPServerObj, DWORD dwClientID, SOCKET *psSocket)
{
    PTHttpServerInfo pThis = (PTHttpServerInfo) hHTTPServerObj;
    PTHttpConnection ptHttpConn;
//    SCODE sResult;

    ptHttpConn = &pThis->ptHttpConnections[dwClientID];
    ptHttpConn->bStreamSocket = TRUE;
    TcpGetConnectionSocket((UINT) dwClientID, psSocket);

    return S_OK;
}

//SCODE HTTPServer_CompareTime(PCHAR pcClientLatestTime, PCHAR pcServerLatestTime)
//{
    //return S_OK
//}

//SCODE HTTPServer_QueryFieldValue(PCHAR pcQuery, PCHAR pcField, PCHAR pcValue, int iValueLen)
//{
    //return S_OK
//}

