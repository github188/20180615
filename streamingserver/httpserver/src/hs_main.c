/*
 *  File:       AsMain.c
 *
 *
 *  To Do:
 */

#include "osisolate.h"
#include "httpserver_local.h"
#include "hs_file.h"

DWORD HandleHttpConnection(PTHttpConnection ptHttpConn);
static SCODE HandleWaitingConnection(PTHttpConnection ptHttpConn);
static SCODE HandleAbortConnection(PTHttpConnection ptHttpConn);
static SCODE HandleTcpReceiveData(PTHttpConnection ptHttpConn);
static SCODE HandleTcpSendData(PTHttpConnection ptHttpConn);
static SCODE HandleTcpClose(PTHttpConnection ptHttpConn);
static SCODE HandleTimerActions(PTHttpServerInfo pServerInfo);

const char *gMimeTypes[] = {
    STR_TYPE_TEXT,
    STR_TYPE_HTML,
    STR_TYPE_GIFIMAGE,
    STR_TYPE_JPEGIMAGE,
    // added by Jason, 2004/12/30
    STR_TYPE_TUNNELLED,
    STR_TYPE_APPLICATION_OCTETSTREAM
};

DWORD THREADAPI HttpServerMainTask(DWORD dwInstance)
{
    DWORD dwRet;
    PTHttpServerInfo pThis = (PTHttpServerInfo) dwInstance;
    PTHttpConnection ptHttpConn;
    

    while (1)
    {
		OSSleep_MSec(10);

        // Wait for terminate event
        if (S_OK == OSEvent_WaitFor(pThis->hTerminateEvent, 0))
            break;
        
        // Wait for start event
        if (S_OK == OSEvent_WaitFor(pThis->hRunEvent, INFINITE))
        {
            OSEvent_Set(pThis->hRunEvent); // Signal the run event again
            
			ptHttpConn = pThis->ptHttpConnections;
			while (ptHttpConn < &pThis->ptHttpConnections[pThis->ulMaxConnections])
            {
                dwRet = HandleHttpConnection(ptHttpConn);
                ptHttpConn += 1;
/*
                if (dwRet != S_OK)
                {
                    goto thread_exit;
                }
*/
            }
        }
        HandleTimerActions(pThis);
    }

thread_exit:    
    // Callback to notify http server has stoped
    
    return 0;
}

DWORD HandleHttpConnection(PTHttpConnection ptHttpConn)
{
    BOOL    bReadMoreFlag;
    DWORD   dwResult;
    PTHttpServerInfo pServerInfo = (PTHttpServerInfo) ptHttpConn->hServer;
    THTTPServer_Send tDataSend;

    dwResult = S_OK;
  
    // If we have data pending, do send or receive again
    if (ptHttpConn->tTcpState == TCP_SEND_PENDING)
    {    	
		dwResult = HandleTcpSendData(ptHttpConn);
		if(dwResult != S_OK)
		{
			ptHttpConn->tHttpState = HTTP_STATE_RESPONSE_COMPLETE;
			ptHttpConn->tTcpState = TCP_COMPLETE;
		}
        return S_OK;
//        return dwResult;
    }
    else if (ptHttpConn->tTcpState == TCP_RECEIVE_PENDING)
    {
		dwResult = HandleTcpReceiveData(ptHttpConn);
		if(dwResult != S_OK)
		{
			ptHttpConn->tHttpState = HTTP_STATE_RESPONSE_COMPLETE;
			ptHttpConn->tTcpState = TCP_COMPLETE;
		}
		return S_OK;
//        return dwResult;
    }

    bReadMoreFlag = FALSE;
    
    switch (ptHttpConn->tHttpState)
    {
        case HTTP_STATE_CONNECTION_CLOSED:
        
            InitRequestStates(ptHttpConn);
			DbgPrint(("ID [%02d] : Into HTTP_STATE_WAITING_CONNECTION\n", ptHttpConn->uiConnectionIndex));
//			TelnetShell_DbgPrint("ID [%02d] : Into HTTP_STATE_WAITING_CONNECTION\r\n", ptHttpConn->uiConnectionIndex);
            ptHttpConn->tHttpState =  HTTP_STATE_WAITING_CONNECTION;
            break;
            
        case HTTP_STATE_WAITING_CONNECTION:
        
            dwResult = HandleWaitingConnection(ptHttpConn);
            break;
            
        case HTTP_STATE_PARSING_HEADERS:
        
            bReadMoreFlag = ParseHttpHeaders(ptHttpConn);
            break;

        case HTTP_STATE_GET_OBJECT_BODY:
        
            bReadMoreFlag = GetObjectData(ptHttpConn);
            break;

//#if RomPagerHttpOneDotOne
//        case eRpParsingChunkedObjectBody:
//            theReadMoreFlag = RpGetChunkedObjectData(theRequestPtr);
//            break;

//#endif

        case HTTP_STATE_PARSING_MULTIPART:
        
            bReadMoreFlag = ParseMultipartHeaders(ptHttpConn);
            break;
            
        case HTTP_STATE_MULTIPART_OBJECT:
        
            bReadMoreFlag = GetMultipartObjectData(ptHttpConn);
            break;
            
        case HTTP_STATE_PREPARE_DATA:
        
            PrepareHttpData(ptHttpConn);
            break;

        case HTTP_STATE_DATA_PENDING:
        
            //CheckHttpDataStatus(ptHttpConn);
            break;
            
        //case eRpEndUrlSearch:
        //    RpFinishUrlSearch(theRequestPtr);
        //    break;

        //case eRpAnalyzeHttpRequest:
        //    theRequestPtr->fHttpTransactionState = eRpSendingHttpHeaders;
        //    RpAnalyzeHttpRequest(theRequestPtr);
        //    break;

        case HTTP_STATE_SEND_HEADERS:

			DbgPrint(("ID [%02d] : Into HTTP_STATE_SEND_HEADERS\n", ptHttpConn->uiConnectionIndex));
			//DbgLog((dfTELNET, "ID [%02d] : Into HTTP_STATE_SEND_HEADERS\r\n", ptHttpConn->uiConnectionIndex));
//			TelnetShell_DbgPrint("ID [%02d] : Into HTTP_STATE_SEND_HEADERS\r\n", ptHttpConn->uiConnectionIndex);
            // Modified by Joe 2003/07/30,
            if(HttpResponseHeader(ptHttpConn) != S_OK)  // client close the connection
            {
                ptHttpConn->tHttpState = HTTP_STATE_RESPONSE_COMPLETE;
                ptHttpConn->tTcpState = TCP_COMPLETE;
//                TelnetShell_DbgPrint("ID [%02d] : Parse Error, Into HTTP_STATE_RESPONSE_COMPLETE\r\n", ptHttpConn->uiConnectionIndex);
            }


            break;

        case HTTP_STATE_SEND_BODY:

			DbgPrint(("ID [%02d] : Into HTTP_STATE_SEND_BODY\n", ptHttpConn->uiConnectionIndex));
			//DbgLog((dfTELNET, "ID [%02d] : Into HTTP_STATE_SEND_BODY\r\n", ptHttpConn->uiConnectionIndex));
//			TelnetShell_DbgPrint("ID [%02d] : Into HTTP_STATE_SEND_BODY\r\n", ptHttpConn->uiConnectionIndex);
            // Modified by Joe 2003/07/30,
            if(HttpResponseBody(ptHttpConn) != S_OK)
            {
                ptHttpConn->tHttpState = HTTP_STATE_RESPONSE_COMPLETE;
                ptHttpConn->tTcpState = TCP_COMPLETE;
//                TelnetShell_DbgPrint("ID [%02d] : Response Error Into HTTP_STATE_RESPONSE_COMPLETE\r\n", ptHttpConn->uiConnectionIndex);
            }
            break;

/*
#if RpFileInsertItem
        case eRpOpenInsertFileItem:
            //    start opening the file for a File Insert Item.
            theResult = SfsOpenFile(theConnectionPtr->fIndex,
                    theRequestPtr->fFileNamePtr);
            if (theResult == eRpNoError) {
                theConnectionPtr->fState = eRpConnectionWaitingFileOpen;
                theRequestPtr->fHttpTransactionState = eRpOpeningInsertFileItem;
            }
            else {
                RpSendFileInsertItemError(theConnectionPtr,
                    eRpSendingHttpResponse);
                theResult = eRpNoError;
            }
            break;

        case eRpReadingInsertFileItem:
            //    the file is open, start or continue reading it.
            theConnectionPtr->fFileState = eRpFileReading;
            theResult = SfsReadFile(theConnectionPtr->fIndex,
                                        theRequestPtr->fHtmlFillPtr,
                                        theRequestPtr->fFillBufferAvailable);
            theConnectionPtr->fState = eRpConnectionWaitingFileRead;
            theRequestPtr->fHttpTransactionState = eRpReadingInsertFileItem;
            break;

        case eRpClosingInsertFileItem:
            //    the file read is complete or in error, start closing it.
            theResult = RpFileClose(theConnectionPtr);
            break;
#endif
*/

/*
#if ((RomPagerServer || RomPagerLight) && RomPagerFileSystem)
        case eRpConnectionWaitingFileOpen:
        case eRpConnectionWaitingFileClose:
        case eRpConnectionWaitingFileRead:
        case eRpConnectionWaitingFileCreate:
        case eRpConnectionWaitingFileWrite:
            theResult = RpHandleFileStates(theDataPtr);
            break;
#endif


#if RomPagerUserExit
        case eRpConnectionWaitingUserExit:
            RpHandleUserExit(theConnectionPtr);
            break;
#endif
*/
        case HTTP_STATE_RESPONSE_COMPLETE:

            if (ptHttpConn->bStreamSocket)
            {
                if (ptHttpConn->bStreamSocketClose)
                    dwResult = HandleTcpClose(ptHttpConn);
                else
                    dwResult = TcpClearConnectionSocket(ptHttpConn->uiConnectionIndex);
                ptHttpConn->tHttpState = HTTP_STATE_CONNECTION_CLOSED;
            }
            else
            {
                // Close the file
    		    if (ptHttpConn->tDataSourceType == DATA_SOURCE_FILE)
                {
                    HsFile_Close(ptHttpConn->uiConnectionIndex);
                }
    
                // notify the data send out complete
    		    if (ptHttpConn->tDataSourceType == DATA_SOURCE_MEMORY)
                {
                    tDataSend.dwClientID         = ptHttpConn->uiConnectionIndex;
                    tDataSend.pcSendBuffer       = ptHttpConn->pDataSourceBuffer;
                    tDataSend.dwSendBufferLength = ptHttpConn->dwDataSourceLength;
    
                    if (pServerInfo->pfnHttpCallback)
                    {
                        pServerInfo->pfnHttpCallback(pServerInfo->dwContext,
                           HTTPServer_Callback_Send, (void *) &tDataSend);
                    }
                }
    
                dwResult = HandleTcpClose(ptHttpConn);
            }
            //dwResult = RpConnectionCheckTcpClose(ptHttpConn);
			DbgPrint(("ID [%02d] : Into HTTP_STATE_RESPONSE_COMPLETE\n", ptHttpConn->uiConnectionIndex));
			//DbgLog(("ID [%02d] : Into HTTP_STATE_RESPONSE_COMPLETE\r\n", ptHttpConn->uiConnectionIndex));
//			TelnetShell_DbgPrint("ID [%02d] : Into HTTP_STATE_RESPONSE_COMPLETE\r\n", ptHttpConn->uiConnectionIndex);
            break;

        //case eRpConnectionClosing:
        //    theResult = HandleClosing(theConnectionPtr);
        //    break;

        default:
            break;

    }

    // fire off another receive if we need to
    if (bReadMoreFlag)
    {
        dwResult = HandleTcpReceiveData(ptHttpConn);
		if(dwResult != S_OK)
		{
			ptHttpConn->tHttpState = HTTP_STATE_RESPONSE_COMPLETE;
		}
		return S_OK;
    }
    return dwResult;
}

SCODE InitHttpConnections(PTHttpServerInfo pServerInfo)
{
    int iIndex;
    PTHttpConnection ptHttpConn;
    
    ptHttpConn = pServerInfo->ptHttpConnections;
    for (iIndex = 0; iIndex < (int) pServerInfo->ulMaxConnections; iIndex++)
    {
        ptHttpConn->hServer = (HANDLE) pServerInfo;
        ptHttpConn->uiConnectionIndex = iIndex;
        ptHttpConn->tHttpState = HTTP_STATE_CONNECTION_CLOSED;
        ptHttpConn += 1;
    }
    
        // Initialize date handling.
        //theDataPtr->fRomSeconds = RpGetMonthDayYearInSeconds(kHttpRomMonth,
        //                                    kHttpRomDay, kHttpRomYear);
        //RpBuildDateString(theDataPtr->fRomDateString, theDataPtr->fRomSeconds);
        //theExpiresSeconds = RpGetMonthDayYearInSeconds(10, 26, 1995);
        //RpBuildDateString(theDataPtr->fExpiresDateString, theExpiresSeconds);

    return S_OK;
}

/*
void AllegroTaskDeInit(void *theTaskDataPtr)
{
    rpDataPtr       theDataPtr = (rpDataPtr) theTaskDataPtr;
    rpConnectionPtr theConnectionPtr;
    Unsigned16      theInitFlags;

    if (theDataPtr != (rpDataPtr) 0) {

        theInitFlags = theDataPtr->fInitFlags;

        if (theInitFlags & kRpInitFlagTcp) {
            theConnectionPtr = theDataPtr->fConnections;
            while (theConnectionPtr <
                    &theDataPtr->fConnections[kStcpNumberOfConnections]) {
                if (theConnectionPtr->fState != eRpConnectionClosed) {
                    (void) StcpAbortConnection(theConnectionPtr->fIndex);
                }
                theConnectionPtr += 1;
            }

            (void) StcpDeInit();
        }
    }

    return;
}
*/

static SCODE HandleTimerActions(PTHttpServerInfo pServerInfo)
{
    int iIndex;
    PTHttpConnection ptHttpConn;
    DWORD  dwCurrentTime, dwDiff;
    SCODE  sResult;

    sResult = S_OK;

    ptHttpConn = pServerInfo->ptHttpConnections;

    // Get time in seconds
    //OSTime_GetTimer(&dwCurrentTime, &dwCurrentMSec);
    OSTick_GetMSec(&dwCurrentTime);

    if(dwCurrentTime < pServerInfo->dwLastTime)
    {
        dwDiff = 0xFFFFFFFF - pServerInfo->dwLastTime + dwCurrentTime + 1;
    }
    else
    {
        dwDiff = dwCurrentTime - pServerInfo->dwLastTime;
    }

    // check every 0.5 seconds
    if(dwDiff > 500)
    {
        // A second has passed, so do the once per second tasks.
        pServerInfo->dwLastTime = dwCurrentTime;

        pServerInfo->pfnHttpCallback(pServerInfo->dwContext,
        HTTPServer_Callback_Alive, NULL);

        //RpCheckPasswordTimers(theDataPtr);

        // Check for connections that need time action.
        for (iIndex = 0; iIndex < (int) pServerInfo->ulMaxConnections; iIndex++)
        {
            if (ptHttpConn->dwAbortTimer != 0)
            {
                // If there was a timer started, decrement it.
                ptHttpConn->dwAbortTimer -= 1;
                if (ptHttpConn->dwAbortTimer == 0)
                {
                    ptHttpConn->tTcpState = TCP_COMPLETE;
        			DbgPrint(("ID [%02d] : Receive timeout\n", ptHttpConn->uiConnectionIndex));
        			DbgPrint1(("ID [%02d] : Receive timeout\n", ptHttpConn->uiConnectionIndex));
                    // If the timer expired, abort the connection.
                    sResult = HandleAbortConnection(ptHttpConn);
                }
            }
            /*
            if (theConnectionPtr->fPersistenceTimer != 0)
            {
                // If there was a timer started, decrement it.
                theConnectionPtr->fPersistenceTimer -= 1;
                if (theConnectionPtr->fPersistenceTimer == 0)
                {
                    theResult = RpConnectionAbortTcp(theConnectionPtr);
                }
            }*/
            ptHttpConn += 1;
        }
    }
    
    return sResult;
}

static SCODE HandleWaitingConnection(PTHttpConnection ptHttpConn)
{
    PTHttpServerInfo pServerInfo = (PTHttpServerInfo) ptHttpConn->hServer;
    THTTPServer_AcceptData tCallbackAccept;
    TTcpStatus tStatus;
    DWORD dwClientAddress;
    DWORD dwServerAddress;
    USHORT usClientPort;
    USHORT usServerPort;
    SCODE sResult;

    sResult = TcpListenConnection(ptHttpConn->uiConnectionIndex, &tStatus,
                &dwClientAddress, &usClientPort, &dwServerAddress, &usServerPort);
          
    if (sResult == S_OK)
    {
        if (tStatus == TCP_COMPLETE)
        {
            // callback accept
            tCallbackAccept.dwClientID = ptHttpConn->uiConnectionIndex;
            tCallbackAccept.dwClientIPAddress = dwClientAddress;
            tCallbackAccept.usClientPort = usClientPort;
            if (pServerInfo->pfnHttpCallback)
            {
                pServerInfo->pfnHttpCallback(pServerInfo->dwContext,
                   HTTPServer_Callback_Accept, (void *) &tCallbackAccept);
            }
	        ptHttpConn->tHttpState = HTTP_STATE_PARSING_HEADERS;
	        ptHttpConn->dwAbortTimer = pServerInfo->dwConnectionTimeout;
	        
	        // reset connection data
	        ptHttpConn->dwLastModified = 0;
	        ptHttpConn->dwIfModifiedSince = 0;
	        ptHttpConn->dwOrgLength = 0;
	        
            // Receive data
            sResult = HandleTcpReceiveData(ptHttpConn);
        }
    }
    else
    {
        // We have an error with the connection, so mark it closed.
        ptHttpConn->tHttpState = HTTP_STATE_CONNECTION_CLOSED;
    }

    return sResult;
}

static SCODE HandleAbortConnection(PTHttpConnection ptHttpConn)
{
    SCODE sResult;

    sResult = TcpAbortConnection(ptHttpConn->uiConnectionIndex);
    
    ptHttpConn->tHttpState = HTTP_STATE_CONNECTION_CLOSED;
    ptHttpConn->dwAbortTimer = 0;
    
    //theConnectionPtr->fPersistenceTimer = 0;
    //RpFreeRequestControlBlock(theConnectionPtr);
    
    return sResult;
}

static SCODE HandleTcpReceiveData(PTHttpConnection ptHttpConn)
{
    PTHttpServerInfo pServerInfo = (PTHttpServerInfo) ptHttpConn->hServer;
    TTcpStatus tReceiveState;
    char*      pReceiveBuffer;
    UINT       uiReceiveLength;
    DWORD      dwResult;
    DWORD      dwReadOffset;

    dwReadOffset = ptHttpConn->tParsingCtrl.dwDataBufferLength;
    
    dwResult = TcpReceive(ptHttpConn->uiConnectionIndex, &tReceiveState,
                        dwReadOffset, &pReceiveBuffer, &uiReceiveLength);

    if (dwResult == S_OK)
    {
        if (tReceiveState == TCP_COMPLETE)
        {
            // Reset the receive timers.
            //theConnectionPtr->fAbortTimer = 0;
            //theConnectionPtr->fPersistenceTimer = 0;
            // Save the receive buffer info.
			DbgPrint(("ID [%02d] : Receive data %d\n", ptHttpConn->uiConnectionIndex, uiReceiveLength));
            ptHttpConn->tParsingCtrl.pszBeginBuffer = pReceiveBuffer - dwReadOffset;
            ptHttpConn->tParsingCtrl.pszCurrentBuffer = ptHttpConn->tParsingCtrl.pszBeginBuffer;
            ptHttpConn->tParsingCtrl.dwDataBufferLength += uiReceiveLength;
	        ptHttpConn->dwAbortTimer = 0;
        }
        else if (tReceiveState == TCP_RECEIVE_PENDING)
        {
			DbgPrint(("ID [%02d] : Receive pending\n", ptHttpConn->uiConnectionIndex));
        }
        
        ptHttpConn->tTcpState = tReceiveState;
    }
    else
    {
		DbgPrint(("ID [%02d] : Receive error\n", ptHttpConn->uiConnectionIndex));
		DbgPrint1(("ID [%02d] : Receive error\n", ptHttpConn->uiConnectionIndex));
		HandleTcpClose(ptHttpConn);
        // Handle error
        //dwResult = RpHandleSendReceiveError(theConnectionPtr, theResult);
    }

    return dwResult;
}

static SCODE HandleTcpSendData(PTHttpConnection ptHttpConn)
{
    DWORD      dwResult;
    TTcpStatus tSendState;

    dwResult = TcpSend(ptHttpConn->uiConnectionIndex, NULL, 0, &tSendState, NULL);

    if (dwResult == S_OK)
    {
        ptHttpConn->tTcpState = tSendState;
    }
    else
    {
        // handle error
		DbgPrint(("ID [%02d] : Send error\n", ptHttpConn->uiConnectionIndex));
		DbgPrint1(("ID [%02d] : Send error\n", ptHttpConn->uiConnectionIndex));
        //dwResult = RpHandleSendReceiveError(theConnectionPtr, theResult);
		HandleTcpClose(ptHttpConn);
    }
    
    return dwResult;
}


static SCODE HandleTcpClose(PTHttpConnection ptHttpConn)
{
    SCODE sResult;

    // See if the connection has settled out and closed.
    sResult = TcpCloseConnection(ptHttpConn->uiConnectionIndex);

    ptHttpConn->tHttpState = HTTP_STATE_CONNECTION_CLOSED;
/*
	if (sResult == S_OK)
    {
        //theConnectionPtr->fAbortTimer = 0;
        ptHttpConn->tHttpState = HTTP_STATE_CONNECTION_CLOSED;
    }
    else
    {
        // We got an error on the connection status check, so just slam
        // the connection closed.
        //theResult = RpConnectionAbortTcp(theConnectionPtr);
    }
*/    
    return sResult;
}


#if 0
SCODE RpConnectionCheckTcpClose(rpConnectionPtr theConnectionPtr)
{
    Boolean             theNeedCloseFlag = True;
    RpErrorCode         theResult = eRpNoError;
    rpHttpRequestPtr    theRequestPtr = theConnectionPtr->fHttpRequestPtr;

    /*
        We have sent a complete response, so see if we should
        close the connection and free up the request block.

        If there's been an error on the connection, we will close
        or abort it.

        Otherwise, If we have received a Keep-Alive request from the
        browser that we can honor, we keep the connection open and read
        another request.  Otherwise, we close the connection.
    */
    switch (theConnectionPtr->fError)
    {
        case eRpNoError:
            /*
                There hasn't been a connection error.
                Just do a normal close.
            */
            if (theRequestPtr != (rpHttpRequestPtr) 0 && theRequestPtr->fPersistent)
            {
                theNeedCloseFlag = False;
                /*
                    This is a persistent connection, so clear some error
                    states and get ready for the next request.
                */
                theConnectionPtr->fErrorProcPtr = (rpConnErrorProcPtr) 0;
                theConnectionPtr->fFileSystemError = eRpNoError;
                theConnectionPtr->fFileInfo.fFileType = eRpDataTypeNone;
                if (theConnectionPtr->fParsingControl.fIncomingBufferLength > 0)
                {
                    /*
                        We must have had a pipelined request,
                        so go process it.
                    */
                    RpInitRequestStates(theRequestPtr);
                    theRequestPtr->fParsingControlPtr = &theConnectionPtr->fParsingControl;
                    theRequestPtr->fFileInfoPtr = &theConnectionPtr->fFileInfo;
                    theConnectionPtr->fState = eRpConnectionNeedsProtocolAction;
                }
                else
                {
                    /*
                        Free the request control block,
                        and go wait for another request.
                    */
                    RpFreeRequestControlBlock(theConnectionPtr);
                    theResult = RpConnectionReceiveTcp(theConnectionPtr);
                    /*
                        We are in a persistent connection environment, so
                        set a timer to close the connection gracefully, if
                        no further request is made and clear the abort timer.

                    */
                    theConnectionPtr->fPersistenceTimer =
                            kConnectionMaxIdleTimeout;
                    theConnectionPtr->fAbortTimer = 0;
                }
            }

            if (theNeedCloseFlag)
            {
                theResult = RpConnectionCloseTcp(theConnectionPtr);
            }
            break;

        case eRpTcpNoConnection:
            /*
                There has been a "No Connection" error.  We may have gotten
                a close from the other side.  This is especially possible
                in a persistent connection environment.  Indicate that the
                connection is closed (returning it to the connection pool),
                and free any request blocks.
            */
            theConnectionPtr->fState = eRpConnectionClosed;
            theConnectionPtr->fAbortTimer = 0;
            theConnectionPtr->fPersistenceTimer = 0;
            break;

        default:
            /*
                There's been a connection error (other than "No Connection").
                Abort the connection and free any request blocks.
            */
            theResult = RpConnectionAbortTcp(theConnectionPtr);
            break;
    }

    return theResult;
}

RpErrorCode RpConnectionAbortTcp(rpConnectionPtr theConnectionPtr)
{
    RpErrorCode     theResult;

    /*
        Shut the connection down.
    */
    theResult = StcpAbortConnection(theConnectionPtr->fIndex);
    theConnectionPtr->fState = eRpConnectionClosed;
    theConnectionPtr->fAbortTimer = 0;
#if (RomPagerKeepAlive || RomPagerHttpOneDotOne)
    theConnectionPtr->fPersistenceTimer = 0;
#endif
    RpFreeRequestControlBlock(theConnectionPtr);
    return theResult;
}


/*
    Close a connection in an orderly fashion.
*/

RpErrorCode RpConnectionCloseTcp(rpConnectionPtr theConnectionPtr)
{
    RpErrorCode         theResult;
#if RomPagerServer
    rpHttpRequestPtr    theRequestPtr;
#endif

    theResult = StcpCloseConnection(theConnectionPtr->fIndex);

    if (theResult == eRpTcpAlreadyClosed)
    {
        theConnectionPtr->fState = eRpConnectionClosed;
        RpFreeRequestControlBlock(theConnectionPtr);
        theResult = eRpNoError;
    }
    else
    {
        theConnectionPtr->fState = eRpConnectionClosing;
        /*
            We don't want to stay in this state forever, so
            set up the timeout.
        */
        theConnectionPtr->fAbortTimer = kConnectionMaxIdleTimeout;

    }
    return theResult;
}


RpErrorCode RpHandleSendReceiveError(rpConnectionPtr theConnectionPtr,
                            RpErrorCode theError)
{
    RpErrorCode         theResult;

    theConnectionPtr->fError = theError;
    theConnectionPtr->fAbortTimer = 0;
#if RomPagerKeepAlive || RomPagerHttpOneDotOne
    theConnectionPtr->fPersistenceTimer = 0;
#endif

    /*
        If there's a specific callback routine to handle connection errors,
        call it, otherwise just close the connection.
    */
    if (theConnectionPtr->fErrorProcPtr != (rpConnErrorProcPtr) 0) {
        theResult = (*theConnectionPtr->fErrorProcPtr)((void *) theConnectionPtr);
    }
    else {
        /*
            Just do the default error handling.

            RpConnectionCheckTcpClose will clean up the connection
            and free any request blocks.
        */
        theResult = RpConnectionCheckTcpClose(theConnectionPtr);
    }

    return theResult;
}
#endif
