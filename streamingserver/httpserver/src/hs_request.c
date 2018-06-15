/*
 *  File:       RpHttpRq.c
 *
 * $History: hs_request.c $
 * 
 * *****************  Version 5  *****************
 * User: Shengfu      Date: 06/01/23   Time: 4:13p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/httpserver/src
 * 
 * *****************  Version 9  *****************
 * User: Yun          Date: 05/02/17   Time: 5:55p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/httpserver/src
 * Add DbgPrint1
 * 
 * *****************  Version 8  *****************
 * User: Jason        Date: 04/12/31   Time: 3:28p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/httpserver/src
 * 
 * *****************  Version 7  *****************
 * User: Yun          Date: 04/12/31   Time: 3:13p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/httpserver/src
 * 
 * *****************  Version 6  *****************
 * User: Jason        Date: 04/12/30   Time: 6:41p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/httpserver/src
 * 
 * *****************  Version 5  *****************
 * User: Yun          Date: 04/11/26   Time: 4:40p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/httpserver/src
 * Set dwOrgLength
 * 
 * *****************  Version 4  *****************
 * User: Yun          Date: 04/11/24   Time: 2:10p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/httpserver/src
 * Add forcing reloading file flag
 * 
 * *****************  Version 3  *****************
 * User: Yun          Date: 04/11/12   Time: 2:29p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/httpserver/src
 * Add send Content-Length when the source is file, but the version must
 * be HTTP/1.0
 * 
 * *****************  Version 2  *****************
 * User: Yun          Date: 04/07/15   Time: 4:47p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/httpserver/src
 * 1. Set default data type as DATA_TYPE_HTML
 * 3. Update data type from callback data
 * 
 * *****************  Version 20  *****************
 * User: Yun          Date: 04/03/02   Time: 9:45a
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/network/netap/httpserver/src
 * 1. Add LastModified and IfModifiedSince two fields
 * 2. Add 304 Not modified response
 * 
 * *****************  Version 19  *****************
 * User: Joe          Date: 04/01/06   Time: 1:28p
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/network/netap/httpserver/src
 * Move DbgPrint() to httpserver_local.h
 * 
 * *****************  Version 18  *****************
 * User: Joe          Date: 03/12/25   Time: 5:47p
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/network/netap/httpserver/src
 * Initialize szUsername & szPassword to zero.
 * 
 * *****************  Version 17  *****************
 * User: Yun          Date: 03/12/25   Time: 4:03p
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/network/netap/httpserver/src
 * Add privilege field
 * 
 * *****************  Version 16  *****************
 * User: Joe          Date: 03/12/11   Time: 3:39p
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/network/netap/httpserver/src
 * Add debug message using DbgPrint();
 * 
 * *****************  Version 15  *****************
 * User: Joe          Date: 03/11/20   Time: 3:07p
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/network/netap/httpserver/src
 * Update Mutipart request to have pszObjectBuffer
 * 
 * *****************  Version 14  *****************
 * User: Yun          Date: 03/10/07   Time: 3:11p
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/network/netap/httpserver/src
 * Remove printf
 * 
 * *****************  Version 13  *****************
 * User: Jason        Date: 03/09/10   Time: 2:14p
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/network/netap/httpserver/src
 * Modify connection timeout control
 * 
 * *****************  Version 12  *****************
 * User: Jason        Date: 03/08/25   Time: 5:04p
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/network/netap/httpserver/src
 * 
 * *****************  Version 11  *****************
 * User: Jason        Date: 03/08/22   Time: 3:42p
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/network/netap/httpserver/src
 * Modify file upload mechenism
 * 
 * *****************  Version 10  *****************
 * User: Jason        Date: 03/08/18   Time: 3:11p
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/network/netap/httpserver/src
 * 
 * *****************  Version 9  *****************
 * User: Jason        Date: 03/08/13   Time: 7:08p
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/network/netap/httpserver/src
 * 
 * *****************  Version 8  *****************
 * User: Jason        Date: 03/08/12   Time: 4:40p
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/network/netap/httpserver/src
 * 
 * *****************  Version 7  *****************
 * User: Joe          Date: 03/08/12   Time: 11:52a
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/network/netap/httpserver/src
 * 1. Porting to Trimedia MDS card.
 * 
 * *****************  Version 6  *****************
 * User: Jason        Date: 03/08/08   Time: 3:42p
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/network/netap/httpserver/src
 * Null terminate the Post data buffer
 * 
 * *****************  Version 5  *****************
 * User: Jason        Date: 03/08/04   Time: 11:24a
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/network/netap/httpserver/src
 * 
 * *****************  Version 4  *****************
 * User: Jason        Date: 03/08/01   Time: 5:41p
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/network/netap/httpserver/src
 * 
 * *****************  Version 3  *****************
 * User: Joe          Date: 03/07/31   Time: 10:27a
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/network/netap/httpserver/src
 * Fill complete RequestData structure in GetObjectData() &
 * PrepareHttpData() functions.
 *
 */

#include "httpserver_local.h"
#include "hs_file.h"

static void FinishParseHeaders(PTHttpConnection ptConnection);
static void CheckGetQuery(PTHttpConnection ptConnection);
static void HandleUrlAuthorize(PTHttpConnection ptConnection);
static void HandleRequestResult(PTHttpConnection ptConnection, THTTPServer_RequestData* ptRequest);

char gHttpNoObjectFound[] =
    "<HTML>\n<HEAD>\n<TITLE>Object Not Found</TITLE></HEAD><BODY>\n"
    "<H1>Object Not Found</H1>The requested URL was not found"
    ".<P>\n</BODY></HTML>";
    
char gHttpProtectedObject[] =
    "<HTML>\n<HEAD>\n<TITLE>Protected Object</TITLE></HEAD><BODY>\n"
    "<H1>Protected Object</H1>This object is protected.<P>\n</BODY></HTML>";

char gHttpInputTooLarge[] =
    "<HTML>\n<HEAD>\n<TITLE>Request Too Large</TITLE></HEAD><BODY>\n"
    "<H1>Request Too Large</H1>The \"POST\" request is too "
    "large for the internal work buffer.<P>\n</BODY></HTML>";

void InitRequestStates(PTHttpConnection ptConnection)
{
    ptConnection->tHttpCommand = HTTP_NO_COMMAND;
    ptConnection->tHttpResponseAct = HTTP_NORMAL;
    ptConnection->tTcpState = TCP_COMPLETE;
    ptConnection->dwPostRequestLength = 0;
    //ptHttpConn->fAcceptType = eRpDataTypeNone;
    ptConnection->bKeepAlive = FALSE;
    ptConnection->bPersistent = FALSE;
    ptConnection->bHaveRequestLine = FALSE;

    //theRequestPtr->fObjectIsChunked = False;
    //theRequestPtr->fResponseIsChunked = False;
    //theRequestPtr->fHaveChunk = False;
    //theRequestPtr->fChunkState = eRpGettingChunkedLength;
    ptConnection->bHaveRequestObject = FALSE;
    ptConnection->bHaveHost = FALSE;
    //theRequestPtr->fPasswordState = eRpPasswordNotAuthorized;
    ptConnection->bStreamSocket = FALSE;
    ptConnection->bStreamSocketClose = FALSE;
    ptConnection->dwStreamID = 0;
    ptConnection->dwAbortTimer = 0;

    //theObjectPtr = &theRequestPtr->fLocalObject;
    ptConnection->tMimeDataType = DATA_TYPE_HTML;
            //theConnectionPtr->fIpRemote = 0;
    ptConnection->tParsingCtrl.dwDataBufferLength = 0;
    ptConnection->tParsingCtrl.uiPartialLineLength = 0;
    ptConnection->tParsingCtrl.dwHttpObjectLengthToRead = 0;
    strcpy(ptConnection->szHttpContentType, "");
            //theConnectionPtr->fError = eRpNoError;
            //theConnectionPtr->fAbortTimer = 0;
            //theConnectionPtr->fPersistenceTimer = 0;
            //theConnectionPtr->fProtocolTimer = 0;
//#if RomPagerFileSystem
            //theConnectionPtr->fFileSystemError = eRpNoError;
            //theConnectionPtr->fFileInfo.fFileType = eRpDataTypeNone;
            //theConnectionPtr->fFileInfo.fFileDate = 0;
//#endif
    // Add by Joe 2003/12/25
    memset(ptConnection->szUsername, 0, sizeof(ptConnection->szUsername));
    memset(ptConnection->szPassword, 0, sizeof(ptConnection->szPassword));
    // Added by Jason 2004/12/30
    memset(ptConnection->szSessionCookie, 0, sizeof(ptConnection->szSessionCookie));

    return;
}

BOOL ParseHttpHeaders(PTHttpConnection ptConnection)
{
    TLineState tState;
    BOOL  bReadMoreFlag;
	//char szTemp[1024];

    bReadMoreFlag = FALSE;
    tState = GetLineFromBuffer(&ptConnection->tParsingCtrl);

    switch (tState)
    {
        case LINE_COMPLETE:
			//strncpy(szTemp, ptConnection->tParsingCtrl.pszCurrentBeginLine, ptConnection->tParsingCtrl.uiCompleteLineLength);
			//szTemp[ptConnection->tParsingCtrl.uiCompleteLineLength]=0;
			//DbgPrint(("   ID [%02d] Header: %s", ptConnection->uiConnectionIndex, szTemp));
            DbgPrint(("[HS_REQUEST][%02d] user=%s, pass=%s\r\n", ptConnection->uiConnectionIndex,
            ptConnection->szUsername, ptConnection->szPassword));
            if (ptConnection->bHaveRequestLine)
            {
                ConvertHeaderToLowerCase(&ptConnection->tParsingCtrl);
            }
            else
            {
                // we got the request line
                ptConnection->bHaveRequestLine = TRUE;
            }

            // Look up the header table and do some process for this header
            HandleHttpHeader(gtPatternTable, ptConnection);
            break;

        case LINE_PARTIAL:
            bReadMoreFlag = TRUE;
            break;

        case LINE_ERROR:
            ptConnection->tHttpResponseAct = HTTP_BAD_REQUEST;
            ptConnection->tHttpState = HTTP_STATE_SEND_HEADERS;

            break;

        case LINE_EMPTY:
            // Reach the end of the headers
            FinishParseHeaders(ptConnection);
            break;
    }
    
    return bReadMoreFlag;
}

static void FinishParseHeaders(PTHttpConnection ptConnection)
{
    PTHttpServerInfo pServerInfo = (PTHttpServerInfo) ptConnection->hServer;
    PTParsingControl ptParsing = &ptConnection->tParsingCtrl;
    
    if (ptConnection->tHttpCommand == HTTP_NO_COMMAND && !ptConnection->bHaveRequestLine)
    {
        /*
            Well, the only thing we've read so far is a CRLF and we
            didn't even get a request line.  We're just going to ignore
            this and go read some more headers.  How could this happen you ask?

            Well some browsers (who shall remain nameless but their
            initials are MSIE) append a CRLF to the object in a POST
            command.  In the bad old days of HTTP 1.0 we could just
            ignore it after reading the data we want, because we
            would close the connection anyway.  In these modern days
            of HTTP 1.1 or Keep-Alive, we need to support pipelined
            requests, so anything in the buffer must handled in a
            gracious manner in order to keep the connection open.
        */
        return;
    }

    if (!ptConnection->bHaveHost && ptConnection->tHttpVersion == HTTP_ONE_DOT_ONE)
    {
        // All HTTP/1.1 clients MUST send in the Host header field.
        ptConnection->tHttpResponseAct = HTTP_BAD_REQUEST;
        ptConnection->tHttpState = HTTP_STATE_SEND_HEADERS;
        return;
    }
    
    if (ptConnection->tHttpCommand == HTTP_NO_COMMAND)
    {
        /*
            We got at least one header, and no request object,
            but we didn't find a method we support in the parsing,
            so just reject the request. GET, POST, and HEAD are the
            supported methods for HTTP/1.0, and HTTP/1.1 adds
            optional support for OPTIONS, PUT, DELETE, and TRACE.
        */
        ptConnection->tHttpResponseAct = HTTP_BAD_COMMAND;
        ptConnection->tHttpState = HTTP_STATE_SEND_HEADERS;
        return;
    }
    
    if (ptConnection->tHttpCommand == HTTP_POST_COMMAND && !ptConnection->bHaveRequestObject)
    {
        // If the POST request is a HTTP 1.1 request and we don't
        // have Content-Length or Transfer-Coding of chunked, then
        // we have a bad request.
        ptConnection->tHttpResponseAct = HTTP_BAD_REQUEST;
        ptConnection->tHttpState = HTTP_STATE_SEND_HEADERS;
        return;
    }
    
    // If we got POST method(not file upload)
    if (ptConnection->tHttpCommand == HTTP_POST_COMMAND &&
        (strcmp(ptConnection->szHttpContentType, STR_TYPE_MULTIPARTFORM) != 0) &&
        (strcmp(ptConnection->szHttpContentType, STR_TYPE_TUNNELLED) != 0)) 
    {
        // check the data length greater than buffer size
        if (ptConnection->dwPostRequestLength > pServerInfo->dwReceiveBufferSize)
        {
            ptConnection->tHttpResponseAct = HTTP_REQUEST_TOO_LARGE;
            ptConnection->tHttpState = HTTP_STATE_SEND_HEADERS;
            // Modified by Joe 2003/08/11, to avoid warning
            ptConnection->tDataSourceType = (THttpDataSource)(DATA_SOURCE_INTERNAL);
            ptConnection->pDataSourceBuffer = gHttpInputTooLarge;
            ptConnection->dwDataSourceLength = strlen(gHttpInputTooLarge);
            return;
        }
        if (ptParsing->dwDataBufferLength > 0)
        {
            // move the object data to the begin of the buffer
            memmove(ptParsing->pszBeginBuffer, ptParsing->pszCurrentBuffer, 
                    ptParsing->dwDataBufferLength);
            ptParsing->pszCurrentBuffer = ptParsing->pszBeginBuffer;
        }
    }
    
    // We got normal request, callback for authorization
    HandleUrlAuthorize(ptConnection);
    
    return;
}

static void HandleUrlAuthorize(PTHttpConnection ptConnection)
{
    PTHttpServerInfo pServerInfo = (PTHttpServerInfo) ptConnection->hServer;
    THTTPServer_RequestData tRequest;
    
    CheckGetQuery(ptConnection);
    
    // callback for authorize
    tRequest.dwClientID      = ptConnection->uiConnectionIndex;
    tRequest.tHttpMethod     = ptConnection->tHttpCommand;
    tRequest.pszURL          = ptConnection->szPath;
    tRequest.pszUserName     = ptConnection->szUsername;
    tRequest.pszPassword     = ptConnection->szPassword;
    tRequest.tResponseStatus = HTTP_URL_NOT_FOUND;
	tRequest.dwStreamID      = 99;
	tRequest.iAudioMode      = AUDIO_TRANSMODE_NONE;
    tRequest.pszObjectBuffer = ptConnection->ResponseHeaderBuffer;
    tRequest.dwObjectLength  = strlen(ptConnection->ResponseHeaderBuffer);
    tRequest.bObjectComplete = TRUE;
    
    tRequest.dwIfModifiedSince = ptConnection->dwIfModifiedSince;
    // add by Yun, 2004/11/26
    tRequest.dwOriginalLength  = ptConnection->dwOrgLength;
    // added by Jason, 2004/12/30
    tRequest.pszSessionCookie  = ptConnection->szSessionCookie;
        
	// ShengFu 2006/01/18 add digest authenticate
	tRequest.iAuthenticateType = pServerInfo->iAuthenticateType;

    if (pServerInfo->pfnHttpCallback)
    {
        pServerInfo->pfnHttpCallback(pServerInfo->dwContext,
           HTTPServer_Callback_Authorize, (void *) &tRequest);
    }
    
    // check response status
    if (tRequest.tResponseStatus == HTTP_URL_NOT_MODIFIED)
    {
        ptConnection->tHttpResponseAct = HTTP_NOT_MODIFIED;
        ptConnection->tHttpState = HTTP_STATE_SEND_HEADERS;
        return;
    }
    else if (tRequest.tResponseStatus == HTTP_URL_UNAUTHORIZED)
    {
        ptConnection->tHttpResponseAct = HTTP_NEED_AUTHORIZATION;
        ptConnection->tHttpState = HTTP_STATE_SEND_HEADERS;
        
        // Modified by Joe 2003/08/11, to avoid warning
        ptConnection->tDataSourceType = (THttpDataSource)(DATA_SOURCE_INTERNAL);
        ptConnection->pDataSourceBuffer = gHttpProtectedObject;
        ptConnection->dwDataSourceLength = strlen(gHttpProtectedObject);
        return;
    }
    else if (tRequest.tResponseStatus == HTTP_URL_NOT_FOUND)
    {
        ptConnection->tHttpResponseAct = HTTP_NO_OBJECT_FOUND;
        ptConnection->tHttpState = HTTP_STATE_SEND_HEADERS;

		// Modified by Joe 2003/08/11, to avoid warning        
        ptConnection->tDataSourceType = (THttpDataSource)(DATA_SOURCE_INTERNAL);
        ptConnection->pDataSourceBuffer = gHttpNoObjectFound;
        ptConnection->dwDataSourceLength = strlen(gHttpNoObjectFound);
        return;
    }
    else if (tRequest.tResponseStatus == HTTP_URL_UNAVAILABLE)
    {
        if (ptConnection->bStreamSocket)
        {
            ptConnection->bStreamSocketClose = TRUE;
        }
        
        ptConnection->tHttpResponseAct = HTTP_SERVICE_UNAVAILABLE;
        ptConnection->tHttpState = HTTP_STATE_SEND_HEADERS;
        return;
    }
    else if (tRequest.tResponseStatus == HTTP_URL_REDIRECT)
    {
        //ptConnection->fHttpResponseState = eRpHttpRedirect;
        //theRequestPtr->fObjectPtr->fURL = theCgiPtr->fResponseBufferPtr;
        return;
    }
    else if (tRequest.tResponseStatus == HTTP_URL_NOT_RESPONSE)
    {
        ptConnection->tHttpState = HTTP_STATE_RESPONSE_COMPLETE;
        return;
    }
    else if (tRequest.tResponseStatus == HTTP_URL_OK)
    {
        //theObjectPtr->fCacheObjectType = eRpObjectTypeDynamic;
    }
    else if (tRequest.tResponseStatus == HTTP_URL_OK_STATIC)
    {
        //theObjectPtr->fCacheObjectType = eRpObjectTypeStatic;
    }

    // Response ok
    if (ptConnection->bHaveRequestObject)
    {
        if (strcmp(ptConnection->szHttpContentType, STR_TYPE_MULTIPARTFORM) == 0)
        {
    		DbgPrint(("ID [%02d] : Into HTTP_STATE_PARSING_MULTIPART\n", ptConnection->uiConnectionIndex));
            ptConnection->tHttpState = HTTP_STATE_PARSING_MULTIPART;
        }
        else
        {
            // POST
    		DbgPrint(("ID [%02d] : Into HTTP_STATE_GET_OBJECT_BODY\n", ptConnection->uiConnectionIndex));
            ptConnection->tHttpState = HTTP_STATE_GET_OBJECT_BODY;
        }
    }
    else
    {
        if (ptConnection->bStreamSocket)
        {
        	// add by Yun, 2003/12/25
        	ptConnection->dwPrivilege = tRequest.dwPrivilege;
            ptConnection->dwStreamID = tRequest.dwStreamID;
            ptConnection->iAudioMode = tRequest.iAudioMode;
            ptConnection->tHttpState = HTTP_STATE_SEND_HEADERS;
			// Add by Yun, 2004/12/31
			ptConnection->tMimeDataType = tRequest.tDataType;            
        }
        else
        {
    		DbgPrint(("ID [%02d] : Into HTTP_STATE_PREPARE_DATA\n", ptConnection->uiConnectionIndex));
            ptConnection->tHttpState = HTTP_STATE_PREPARE_DATA;
        }
    }
}

void PrepareHttpData(PTHttpConnection ptConnection)
{
    PTHttpServerInfo pServerInfo = (PTHttpServerInfo) ptConnection->hServer;
    THTTPServer_RequestData tRequest;
    
    // add by Yun, 2004/11/24
    memset(&tRequest, 0, sizeof(THTTPServer_RequestData));
    
    tRequest.dwClientID      = ptConnection->uiConnectionIndex;
    tRequest.tHttpMethod     = ptConnection->tHttpCommand;
    tRequest.pszURL          = ptConnection->szPath;
    tRequest.pszUserName     = ptConnection->szUsername;
    tRequest.pszPassword     = ptConnection->szPassword;
   
    tRequest.pszObjectBuffer = ptConnection->ResponseHeaderBuffer;
    tRequest.dwObjectLength  = strlen(ptConnection->ResponseHeaderBuffer);
    tRequest.bObjectComplete = TRUE;
    
    tRequest.dwIfModifiedSince = ptConnection->dwIfModifiedSince;
    // add by Yun, 2004/11/26
    tRequest.dwOriginalLength  = ptConnection->dwOrgLength;    
    // added by Jason, 2004/12/30
    tRequest.pszSessionCookie  = ptConnection->szSessionCookie;
    
    // Add by Yun 2004/07/15, set default value
    tRequest.tDataType = DATA_TYPE_HTML;
    
    
    // Add by Joe 2003/07/31, for complete reference
    // tRequest.pszUpFileName   = ptConnection->szFilename;

    // Callback for asking resource
    if (pServerInfo->pfnHttpCallback)
    {
        pServerInfo->pfnHttpCallback(pServerInfo->dwContext,
           HTTPServer_Callback_Request, (void *) &tRequest);
    }

    HandleRequestResult(ptConnection, &tRequest);

    return;
}

BOOL GetObjectData(PTHttpConnection ptConnection)
{
    PTHttpServerInfo pServerInfo = (PTHttpServerInfo) ptConnection->hServer;
    THTTPServer_RequestData tRequest;
    DWORD            dwDataLength;
	DWORD            dwLength;
    PTParsingControl ptParsing;
    BOOL             bReadMoreFlag;
    
    // add by Yun, 2004/11/24
    memset(&tRequest, 0, sizeof(THTTPServer_RequestData));    

    // If we didn't get passed an object length, or we got passed a
    // length of 0, mark the request unparseable and return.  We could
    // just use what's left in the buffer, possibly searching for CRLF
    // which some incorrect browsers add on, but that would be wrong.
    if (ptConnection->dwPostRequestLength < 1)
    {
        ptConnection->tHttpResponseAct = HTTP_BAD_REQUEST;
        ptConnection->tHttpState = HTTP_STATE_SEND_HEADERS;
        return FALSE;
    }

    ptParsing = &ptConnection->tParsingCtrl;

    // On the first pass, set up the global object length
    // to be used for multi-buffer collection of object data.
    if (ptParsing->dwHttpObjectLengthToRead == 0)
    {
        ptParsing->dwHttpObjectLengthToRead = ptConnection->dwPostRequestLength;
    }

    if (ptParsing->dwDataBufferLength >= ptParsing->dwHttpObjectLengthToRead)
    {
        // The buffer contains all we need for this object, so set the
        // length to the remaining length of the object.  In a HTTP 1.1
        // pipelined environment, there may be more in the buffer than
        // just our object. Some browsers also incorrectly append a CRLF
        // to the object.
        dwDataLength = ptParsing->dwHttpObjectLengthToRead;
        tRequest.bObjectComplete = TRUE;
        bReadMoreFlag = FALSE;
    }
    else
    {
        // The buffer has less than what we need for the object, so
        // take everything and go get some more.
        dwDataLength = ptParsing->dwDataBufferLength;
        //ptParsing->dwHttpObjectLengthToRead -= dwDataLength;
        tRequest.bObjectComplete = FALSE;
        bReadMoreFlag = TRUE;
    }

    // Do the hex transfer if post data complete
    if (!bReadMoreFlag)
    {
        EscapeDecodeString(ptParsing->pszCurrentBuffer, dwDataLength, 
                           ptParsing->pszCurrentBuffer, &dwLength, TRUE);
        // the length will change after transfer
        dwDataLength = dwLength;
        ptParsing->pszCurrentBuffer[dwDataLength] = '\0';
        
        // Callback for object data
        tRequest.dwClientID      = ptConnection->uiConnectionIndex;
        tRequest.tHttpMethod     = ptConnection->tHttpCommand;
        tRequest.pszURL          = ptConnection->szPath;
        tRequest.pszUserName     = ptConnection->szUsername;
        tRequest.pszPassword     = ptConnection->szPassword;
        
        tRequest.pszObjectBuffer = ptParsing->pszCurrentBuffer;
        tRequest.dwObjectLength  = dwDataLength;
        tRequest.bObjectComplete = TRUE;
        
        tRequest.dwIfModifiedSince = ptConnection->dwIfModifiedSince;
	    // add by Yun, 2004/11/26
	    tRequest.dwOriginalLength  = ptConnection->dwOrgLength;
        // added by Jason, 2004/12/30
        tRequest.pszSessionCookie  = ptConnection->szSessionCookie;
        
	    // Add by Yun 2004/07/15, set default value
	    tRequest.tDataType = DATA_TYPE_HTML;        
    
        // default value
        tRequest.tDataSource   = DATA_SOURCE_MEMORY;
        tRequest.dwSendBufferLength = 0;
        // Add by Joe 2003/07/31, for complete reference
        //tRequest.pszUpFileName = ptConnection->szFilename;
    
        if (pServerInfo->pfnHttpCallback)
        {
            pServerInfo->pfnHttpCallback(pServerInfo->dwContext,
               HTTPServer_Callback_Request, (void *) &tRequest);            
        }
        
        // set up to process the rest of the buffer
        ptParsing->pszCurrentBuffer += dwDataLength;
        ptParsing->dwDataBufferLength -= dwDataLength;
        
        // check the response data source if object complete
        HandleRequestResult(ptConnection, &tRequest);
    }
    
    return bReadMoreFlag;
}

// only for multipart data (file upload) 2003-08-20 Jason
BOOL GetMultipartObjectData(PTHttpConnection ptConnection)
{
    PTHttpServerInfo pServerInfo = (PTHttpServerInfo) ptConnection->hServer;
    THTTPServer_RequestData tRequest;
    DWORD            dwDataLength;
    DWORD            dwFindLen;
    PCHAR            pCurrentChar;
    PTParsingControl ptParsing;
    BOOL             bBoundaryFound = FALSE;
    BOOL             bReadMoreFlag = FALSE;

    ptParsing = &ptConnection->tParsingCtrl;

    // look for the next boundary
    pCurrentChar = ptParsing->pszCurrentBuffer;
    if (ptParsing->dwDataBufferLength >= ptParsing->dwHttpObjectLengthToRead)
        dwFindLen = ptParsing->dwHttpObjectLengthToRead;
    else
        dwFindLen = ptParsing->dwDataBufferLength;
        
    while (dwFindLen >= (ptConnection->dwBoundaryLength + 4))
    {
        if (*(pCurrentChar    ) == ASCII_Return &&
            *(pCurrentChar + 1) == ASCII_Newline &&
            *(pCurrentChar + 2) == ASCII_Hyphen &&
            *(pCurrentChar + 3) == ASCII_Hyphen &&
            memcmp(pCurrentChar + 4, ptConnection->szBoundary, ptConnection->dwBoundaryLength) == 0)
        {
            // boundary found
            bBoundaryFound = TRUE;
            break;
        }
        
        dwFindLen--;
        pCurrentChar++;
    }
    
    // if we have reach the end of the object and no boundary found, error occurred
    if (ptParsing->dwDataBufferLength >= ptParsing->dwHttpObjectLengthToRead &&
        !bBoundaryFound)
    {
        DbgPrint(("   Error occurred\n"));
        DbgPrint1(("[HTTPServer]   Error occurred\n"));
        ptConnection->tHttpResponseAct = HTTP_BAD_REQUEST;
        ptConnection->tHttpState = HTTP_STATE_SEND_HEADERS;
        return FALSE;
    }
    
    if (!bBoundaryFound)
    {
        // read more
        tRequest.bObjectComplete = FALSE;
        bReadMoreFlag = TRUE;
    }
    else
    {
        // change to parse header state
        ptConnection->tHttpState =  HTTP_STATE_PARSING_MULTIPART;
        tRequest.bObjectComplete = TRUE;
        bReadMoreFlag = FALSE;
    }
        
    // callback object data
    dwDataLength = (DWORD)(pCurrentChar - ptParsing->pszCurrentBuffer);
    if (dwDataLength > 0)
    {
        // Callback for object data
        tRequest.dwClientID      = ptConnection->uiConnectionIndex;
        tRequest.tHttpMethod     = ptConnection->tHttpCommand;
        tRequest.pszURL          = ptConnection->szPath;
        tRequest.pszUserName     = ptConnection->szUsername;
        tRequest.pszPassword     = ptConnection->szPassword;
        
        tRequest.pszObjectBuffer = ptParsing->pszCurrentBuffer;
        tRequest.dwObjectLength  = dwDataLength;
        tRequest.pszContentName  = ptConnection->szItemName;
        tRequest.pszUpFileName   = ptConnection->szFilename;
    
        if (pServerInfo->pfnHttpCallback)
        {
            pServerInfo->pfnHttpCallback(pServerInfo->dwContext,
               HTTPServer_Callback_Multipart_Data, (void *) &tRequest);
        }
        
        // set up to process the rest of the buffer
        ptParsing->dwHttpObjectLengthToRead -= dwDataLength;
        ptParsing->pszCurrentBuffer += dwDataLength;
        ptParsing->dwDataBufferLength -= dwDataLength;
    }
    
    if (!bBoundaryFound)
    {
        // memory move the reserve data to the begin of the buffer
        memmove(ptParsing->pszBeginBuffer, ptParsing->pszCurrentBuffer, 
                ptParsing->dwDataBufferLength);
        ptParsing->pszCurrentBuffer = ptParsing->pszBeginBuffer;
    }
    else
    {
        // pass the 0x0d and 0x0a two bytes
        ptParsing->dwHttpObjectLengthToRead -= 2;
        ptParsing->pszCurrentBuffer += 2;
        ptParsing->dwDataBufferLength -= 2;
    }
    
    return bReadMoreFlag;
}

static void HandleRequestResult(PTHttpConnection ptConnection,
                                THTTPServer_RequestData* ptRequest)
{
	SCODE sResult;

	ptConnection->tDataSourceType = ptRequest->tDataSource;
	// Add by Yun, 2004/07/15
	ptConnection->tMimeDataType = ptRequest->tDataType;
	// Add by Yun, 2004/11/24
	ptConnection->bForceReloadFile = ptRequest->bForceReloadFile;
	
    if (ptRequest->tDataSource == DATA_SOURCE_PENDING)
    {
        ptConnection->tHttpState = HTTP_STATE_SEND_HEADERS;
    }
    else if (ptRequest->tDataSource == DATA_SOURCE_FILE)
    {
        // Open the file
        sResult = HsFile_Open(ptConnection->uiConnectionIndex, ptRequest->szFileName);
		if (sResult != S_OK)
		{
			DbgPrint(("  ID [%02d] : Open File Error\n", ptConnection->uiConnectionIndex));
			DbgPrint1(("  ID [%02d] : Open File Error\n", ptConnection->uiConnectionIndex));
            ptConnection->tHttpResponseAct = HTTP_NO_OBJECT_FOUND;
            ptConnection->tDataSourceType = (THttpDataSource)(DATA_SOURCE_INTERNAL);
            ptConnection->pDataSourceBuffer = gHttpNoObjectFound;
            ptConnection->dwDataSourceLength = strlen(gHttpNoObjectFound);
		}
		else
		{
			// added by Yun
			ptConnection->dwDataSourceLength = ptRequest->dwSendBufferLength;
		}
		
	    if (ptRequest->tResponseStatus == HTTP_URL_NOT_MODIFIED)
	    {
	        ptConnection->tHttpResponseAct = HTTP_NOT_MODIFIED;
	    }
		
		ptConnection->dwLastModified = ptRequest->dwLastModified;
        ptConnection->tHttpState =  HTTP_STATE_SEND_HEADERS;
        
        
        
    }
    else
    {
        ptConnection->pDataSourceBuffer = ptRequest->pSendBuffer;
        ptConnection->dwDataSourceLength = ptRequest->dwSendBufferLength;
        ptConnection->tHttpState =  HTTP_STATE_SEND_HEADERS;
    }
}

static void CheckGetQuery(PTHttpConnection ptConnection)
{
    char*  pszPath;
    UINT   uiTokenLength;

    pszPath = ptConnection->szPath;
    *(ptConnection->ResponseHeaderBuffer) = '\0';

    uiTokenLength = FindTokenDelimited(pszPath, ASCII_Question);
    if (*(pszPath + uiTokenLength) == '\0')
    {
        return;
    }

    // We have a query, so process it.
    pszPath += uiTokenLength;
    *(pszPath++) = '\0';
    // Use response header buffer for temp use
    strcpy(ptConnection->ResponseHeaderBuffer, pszPath);
   
    return;
}

BOOL ParseMultipartHeaders(PTHttpConnection ptConnection)
{
    TLineState tState;
    BOOL  bReadMoreFlag;
	//char szTemp[1024];
	char szHeadBoundary[MAX_BOUNDARY_LENGTH + 4];
	DWORD dwLen;
    PTParsingControl ptParsing;
    PTHttpServerInfo pServerInfo = (PTHttpServerInfo) ptConnection->hServer;
    THTTPServer_RequestData tRequest;

    ptParsing = &ptConnection->tParsingCtrl;
    if (ptParsing->dwHttpObjectLengthToRead == 0)
    {
        ptParsing->dwHttpObjectLengthToRead = ptConnection->dwPostRequestLength;
    }

    bReadMoreFlag = FALSE;
    tState = GetLineFromBuffer(&ptConnection->tParsingCtrl);

    switch (tState)
    {
        case LINE_COMPLETE:

			//strncpy(szTemp, ptParsing->pszCurrentBeginLine, ptParsing->uiCurrentLineLength);
			//szTemp[ptParsing->uiCurrentLineLength]=0;
			//DbgPrint(("   ID [%02d] MP Header: %s", ptConnection->uiConnectionIndex, szTemp));

            // First line is the boundary
            if (ptConnection->tMultipartState == MP_STATE_FIND_BOUNDARY)
            {
                // first two bytes are '-' and followings are boundary string
                sprintf(szHeadBoundary, "--%s--", ptConnection->szBoundary);
                dwLen = strlen(szHeadBoundary);

                // The current line length include the od oa
                if ((ptParsing->uiCurrentLineLength -2) == dwLen &&
                    strncmp(ptParsing->pszCurrentBeginLine, szHeadBoundary, dwLen) == 0)
                {
                    // end boundary found
                    // callback for request data
                    tRequest.dwClientID      = ptConnection->uiConnectionIndex;
                    tRequest.tHttpMethod     = ptConnection->tHttpCommand;
                    tRequest.pszURL          = ptConnection->szPath;
                    tRequest.pszUserName     = ptConnection->szUsername;
                    tRequest.pszPassword     = ptConnection->szPassword;
                    // Add by Joe 2003/11/20, to prevent request handle access invalid address
                    tRequest.pszObjectBuffer = ptConnection->ResponseHeaderBuffer;
                    tRequest.dwObjectLength  = 0;

                    if (pServerInfo->pfnHttpCallback)
                    {
                        pServerInfo->pfnHttpCallback(pServerInfo->dwContext,
                           HTTPServer_Callback_Multipart_Request, (void *) &tRequest);
                    }
                    
                    // check the response data source
                    HandleRequestResult(ptConnection, &tRequest);
                }
                else if ((ptParsing->uiCurrentLineLength - 2) == (dwLen - 2) &&
                         strncmp(ptParsing->pszCurrentBeginLine, szHeadBoundary, dwLen - 2) == 0)
                {
                    // normal boundary
                    ptConnection->tMultipartState = MP_STATE_PARSE_HEADER;
                }
                else
                {
                    ptConnection->tHttpResponseAct = HTTP_BAD_REQUEST;
                    ptConnection->tHttpState = HTTP_STATE_SEND_HEADERS;
                }
            }
            else if (ptConnection->tMultipartState == MP_STATE_PARSE_HEADER)
            {
                ConvertHeaderToLowerCase(&ptConnection->tParsingCtrl);
                // Look up the header table and do some process for this header
                HandleHttpHeader(gtMpPatternTable, ptConnection);
            }
            
            break;

        case LINE_PARTIAL:
            bReadMoreFlag = TRUE;
            break;

        case LINE_ERROR:
            ptConnection->tHttpResponseAct = HTTP_BAD_REQUEST;
            ptConnection->tHttpState = HTTP_STATE_SEND_HEADERS;
            ptConnection->tMultipartState = MP_STATE_FIND_BOUNDARY;
            break;

        case LINE_EMPTY:
            // Reach the end of the multipart headers
            ptConnection->tMultipartState = MP_STATE_FIND_BOUNDARY;
            // callback for authorization and change state
            ptConnection->tHttpState = HTTP_STATE_MULTIPART_OBJECT;
            
            tRequest.dwClientID      = ptConnection->uiConnectionIndex;
            tRequest.tHttpMethod     = ptConnection->tHttpCommand;
            tRequest.pszURL          = ptConnection->szPath;
            tRequest.pszUserName     = ptConnection->szUsername;
            tRequest.pszPassword     = ptConnection->szPassword;
            
            tRequest.pszContentName  = ptConnection->szItemName;
            tRequest.pszUpFileName   = ptConnection->szFilename;
        
            if (pServerInfo->pfnHttpCallback)
            {
                pServerInfo->pfnHttpCallback(pServerInfo->dwContext,
                   HTTPServer_Callback_Multipart_Head, (void *) &tRequest);
            }
            //HandleUrlAuthorize(ptConnection);
            break;
    }
    
    return bReadMoreFlag;
}
