/*
 *  File:       RpHttp.c
 *
 *  Contains:   RomPager routines for HTTP
 *
 * $History: hs_response.c $
 * 
 * *****************  Version 3  *****************
 * User: Shengfu      Date: 06/01/23   Time: 4:13p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/httpserver/src
 * 
 * *****************  Version 11  *****************
 * User: Yun          Date: 05/07/11   Time: 4:15p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/httpserver/src
 * Add support MIME type application/octet-stream and force it to raise a
 * "File Download" dialog box
 * 
 * *****************  Version 10  *****************
 * User: Yun          Date: 05/02/17   Time: 5:55p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/httpserver/src
 * Add DbgPrint1
 * 
 * *****************  Version 9  *****************
 * User: Jason        Date: 04/12/30   Time: 6:41p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/httpserver/src
 * 
 * *****************  Version 8  *****************
 * User: Yun          Date: 04/11/26   Time: 4:39p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/httpserver/src
 * If the client gets image files, use HTTP/1.0
 * 
 * *****************  Version 7  *****************
 * User: Yun          Date: 04/11/24   Time: 2:10p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/httpserver/src
 * Add forcing reloading file flag
 * 
 * *****************  Version 6  *****************
 * User: Yun          Date: 04/08/18   Time: 8:04p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/httpserver/src
 * Set content language during initialization
 * 
 * *****************  Version 5  *****************
 * User: Joe          Date: 04/08/18   Time: 2:59p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/httpserver/src
 * clear tmblock 
 * 
 * *****************  Version 4  *****************
 * User: Yun          Date: 04/07/19   Time: 7:19p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/httpserver/src
 * 1. Add no cache header in dynamic webpage
 * 2. Add connection close 
 * 
 * *****************  Version 3  *****************
 * User: Yun          Date: 04/07/09   Time: 6:44p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/httpserver/src
 * Add a callback to get server name
 * 
 * *****************  Version 14  *****************
 * User: Yun          Date: 04/03/17   Time: 10:33a
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/network/netap/httpserver/src
 * 1. Add Date header
 * 2. Add Expire header in dynamic page
 * 
 * *****************  Version 13  *****************
 * User: Yun          Date: 04/03/10   Time: 11:11a
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/network/netap/httpserver/src
 * Merge with SOC project
 * 
 * *****************  Version 12  *****************
 * User: Yun          Date: 04/03/02   Time: 9:45a
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/network/netap/httpserver/src
 * 1. Add LastModified and IfModifiedSince two fields
 * 2. Add 304 Not modified response
 * 
 * *****************  Version 11  *****************
 * User: Yun          Date: 04/02/19   Time: 9:13p
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/network/netap/httpserver/src
 * Add Content-Language in reponse header
 * 
 * *****************  Version 10  *****************
 * User: Yun          Date: 04/02/19   Time: 3:15p
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/network/netap/httpserver/src
 * Add the capability of turn off audio downstream 
 * 
 * *****************  Version 9  *****************
 * User: Joe          Date: 04/01/06   Time: 1:28p
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/network/netap/httpserver/src
 * Move DbgPrint() to httpserver_local.h
 * 
 * *****************  Version 8  *****************
 * User: Yun          Date: 03/12/25   Time: 4:04p
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/network/netap/httpserver/src
 * Add privilege field
 * 
 * *****************  Version 7  *****************
 * User: Joe          Date: 03/12/11   Time: 3:39p
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/network/netap/httpserver/src
 * Add debug message using DbgPrint();
 * 
 * *****************  Version 6  *****************
 * User: Yun          Date: 03/10/07   Time: 3:11p
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/network/netap/httpserver/src
 * Remove printf
 * 
 * *****************  Version 5  *****************
 * User: Jason        Date: 03/09/10   Time: 2:14p
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/network/netap/httpserver/src
 * Modify connection timeout control
 * 
 * *****************  Version 4  *****************
 * User: Jason        Date: 03/08/12   Time: 4:40p
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/network/netap/httpserver/src
 * 
 * *****************  Version 3  *****************
 * User: Jason        Date: 03/07/30   Time: 4:49p
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/network/netap/httpserver/src
 * 
 * *****************  Version 2  *****************
 * User: Joe          Date: 03/07/30   Time: 3:26p
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/network/netap/httpserver/src
 * 1. Fix ResponseBody & ResponseHeader TcpSend error.
 *
 */

#include "httpserver_local.h"
#include "hs_file.h"

static void BuildContentType(PTHttpConnection ptConnection, char *pResponseBuf);
static void BuildContentLength(PTHttpConnection ptConnection, char *pResponseBuf);
static void BuildContentLanguage(PTHttpConnection ptConnection, char *pResponseBuf);
static void BuildLastModifiedHeaders(PTHttpConnection ptConnection, char *pResponseBuf);
static void BuildDateHeaders(PTHttpConnection ptConnection, char *pResponseBuf);


void BuildResponseHeader(PTHttpConnection ptConnection);
SCODE SendReplyBuffer(PTHttpConnection ptConnection);

/*
    Methods string
*/
static char gMethodsAllowed[] = " GET, HEAD, POST"
                                "\x0d\x0a";

/*
void RpHandleUserExit(rpConnectionPtr theConnectionPtr)
{
    rpCgiPtr            theCgiPtr;
    rpHttpRequestPtr    theRequestPtr;

    theRequestPtr = theConnectionPtr->fHttpRequestPtr;
    theCgiPtr = &theRequestPtr->fCgi;
    RpExternalCgi(theRequestPtr->fDataPtr, theCgiPtr);
    if (theCgiPtr->fResponseState != eRpCgiPending) {
        theConnectionPtr->fState = eRpConnectionNeedsProtocolAction;
        RpHandleCgiResponse(theRequestPtr);
    }
    return;
}

*/

SCODE HttpResponseHeader(PTHttpConnection ptConnection)
{
    SCODE sResult;
    TTcpStatus tStatus;

    BuildResponseHeader(ptConnection);

    sResult = TcpSend(ptConnection->uiConnectionIndex, ptConnection->ResponseHeaderBuffer,
                      strlen(ptConnection->ResponseHeaderBuffer), &tStatus, NULL);

    if (sResult == S_OK)
    {
        ptConnection->tTcpState = tStatus;
    }
    else
    {
        // Add by Joe 2003/07/30, for client close the connection
        DbgPrint(("[ResponseHeader] Tcp send error!\r\n"));
        DbgPrint1(("[ResponseHeader] Tcp send error!\r\n"));
        return S_FAIL;
        //theResult = RpHandleSendReceiveError(theConnectionPtr, theResult);
    }

    return sResult;
}

SCODE HttpResponseBody(PTHttpConnection ptConnection)
{
    SCODE   sResult;
    DWORD   dwSendLength;
    char *  pSendBuf;
	DWORD   dwReaded;

    sResult = S_OK;
    
    switch (ptConnection->tDataSourceType)
    {
        case DATA_SOURCE_FILE:
            sResult = HsFile_Read(ptConnection->uiConnectionIndex,
                                  ptConnection->ResponseBodyBuffer,
                                  HTTP_BODY_BUFFER_SIZE, &dwReaded);
            ptConnection->dwResponseBodyLength = dwReaded;
            if (dwReaded > 0)
            {
                sResult = SendReplyBuffer(ptConnection);
            }
            else
            {
                if (sResult == S_END_OF_FILE)
                {
					DbgPrint(("  ID [%02d] : Read End Of File\n", ptConnection->uiConnectionIndex));
                    ptConnection->tHttpState = HTTP_STATE_RESPONSE_COMPLETE;
					sResult = S_OK;
                }
                else
                {
					DbgPrint(("  ID [%02d] : Read File Error\n", ptConnection->uiConnectionIndex));
					DbgPrint1(("  ID [%02d] : Read File Error\n", ptConnection->uiConnectionIndex));
                    sResult = S_FAIL;
                }
            }
            
            break;

        case DATA_SOURCE_INTERNAL:
        case DATA_SOURCE_MEMORY:
            if (ptConnection->dwDataSourceLength > 0)
            {
                pSendBuf = ptConnection->pDataSourceBuffer;
                if (ptConnection->dwDataSourceLength > HTTP_BODY_BUFFER_SIZE)
                {
                    dwSendLength = HTTP_BODY_BUFFER_SIZE;
                    ptConnection->dwDataSourceLength -= dwSendLength;
                    ptConnection->pDataSourceBuffer += dwSendLength;
                }
                else
                {
                    dwSendLength = ptConnection->dwDataSourceLength;
                    ptConnection->dwDataSourceLength = 0;
                    ptConnection->tHttpState = HTTP_STATE_RESPONSE_COMPLETE;
                }
                ptConnection->dwResponseBodyLength = dwSendLength;
                memcpy(ptConnection->ResponseBodyBuffer, pSendBuf, dwSendLength);
                sResult = SendReplyBuffer(ptConnection);
            }
            else
            {
                //if (ptConnection->tBufferStatus == DATA_BUFFER_LAST)
                //{
                    ptConnection->tHttpState = HTTP_STATE_RESPONSE_COMPLETE;
                    //sResult = RpSendReplyBuffer(theConnectionPtr);
                //}
                /*
                else
                {
                    theConnectionPtr->fState = eRpConnectionWaitingUserExit;
                }
                */
            }
            break;

        default:
            break;
    }
    
    return sResult;
}

SCODE SendReplyBuffer(PTHttpConnection ptConnection)
{
    SCODE sResult;
    TTcpStatus tStatus;

    if (ptConnection->dwResponseBodyLength != 0)
    {
        sResult = TcpSend(ptConnection->uiConnectionIndex,
                          ptConnection->ResponseBodyBuffer,
                          ptConnection->dwResponseBodyLength,
                          &tStatus, NULL);

        if (sResult == S_OK)
        {
            ptConnection->tTcpState = tStatus;
        }
        else
        {
            // Add by Joe 2003/07/30, for client close the connection
            DbgPrint(("[SendReplyBuffer] Tcp send error!\r\n"));
            DbgPrint1(("[SendReplyBuffer] Tcp send error!\r\n"));
            return S_FAIL;
            //sResult = RpHandleSendReceiveError(theConnectionPtr, theResult);
        }
    }
    else
    {
        sResult = S_FAIL;
    }

    return sResult;
}

#if 0
void RpAnalyzeHttpRequest(rpHttpRequestPtr theRequestPtr)
{
    rpDataPtr               theDataPtr;
    rpDataType              theObjectMimeType;
    rpObjectDescriptionPtr  theObjectPtr;
#if RomPagerServer
    rpObjectExtensionPtr    theExtensionPtr;
    rpProcessDataFuncPtr    theFunctionPtr;
#endif
#if RomPagerForms
    rpObjectFlags           theObjectFlags;
#endif

    theDataPtr = theRequestPtr->fDataPtr;
    theObjectPtr = theRequestPtr->fObjectPtr;
    theObjectMimeType = theObjectPtr->fMimeDataType;

#if RomPagerForms
    if (theObjectMimeType == eRpDataTypeForm ||
            theObjectMimeType == eRpDataTypeFormGet ||
            theObjectMimeType == eRpDataTypeFormMultipart)
    {
        //    We have valid access to a form.  Forms process the data and then
        //    trigger a request to serve up a page. As a default, we set up to
        //    serve the page stored in our object descriptor. In some cases,
        //    the forms processing may change the page to be served.
        theRequestPtr->fCurrentFormDescriptionPtr = theObjectPtr;
        theRequestPtr->fObjectPtr = theObjectPtr->fExtensionPtr->fPagePtr;

        /*
            Go handle all the form elements and set the variables.
            If this is an image map, go figure out the location.
        */
        RpProcessForm(theDataPtr);

        /*
            Get the flags from the form object.
        */
        theObjectFlags = theObjectPtr->fExtensionPtr->fFlags;

        if (theObjectFlags & kRpObjFlag_Direct ||
            theRequestPtr->fItemError != eRpItemError_NoError) {
            //    If kRpObjFlag_Direct is set, we want to serve the next page
            //    directly.  Most of the time this will be used with a Get-Query
            //    request.  Various browsers handle history and Reload functions
            //    for GET-Query and POST requests in different ways.  In general,
            //    using redirection leaves the history log in the browsers in
            //    a cleaner state.

            //    Since we are going to serve the page directly, set up the
            //    dereferenced pointers again, since forms processing may have
            //    changed them.
            theObjectPtr = theRequestPtr->fObjectPtr;
#if RomPagerMimeTypeChecking
            theObjectMimeType = theObjectPtr->fMimeDataType;
#endif
        }
        else {
            /*
                Set the state so we will respond with a redirect to the
                next page to be served.
            */
            theRequestPtr->fHttpResponseState = eRpHttpRedirect;
            return; 
        }
    }
#endif  // RomPagerForms 
    /*
        We have valid access to a page, image, or applet.
    */

#if RomPagerMimeTypeChecking
    /*
         is it the right type?
    */
    if (!CheckAcceptType(theRequestPtr, theObjectMimeType)){
        /*
            the page doesn't match the request type
        */
        theRequestPtr->fHttpResponseState = eRpHttpBadContentType;

        return;     /* just bail out here */
    }
#endif

    /*
        We have the right content type, check the date stamp.
    */
    if (theObjectPtr->fCacheObjectType == eRpObjectTypeStatic &&
            theRequestPtr->fBrowserDate == theDataPtr->fRomSeconds) {
        /*
            The page is static and the date that the browser has matches
            our rom date, so just notify the browser that the page has
            not been modified.
        */
        theRequestPtr->fHttpResponseState = eRpHttpNotModified;
        return;     /* just bail out here */
    }
#if RomPagerFileSystem
    if (theRequestPtr->fObjectSource == eRpFileUrl &&
            theRequestPtr->fBrowserDate ==
                    theRequestPtr->fFileInfoPtr->fFileDate) {

        /*
            The page has been served from the file system already without
            a date change, so just notify the browser that the page has
            not been modified.
        */
        theRequestPtr->fHttpResponseState = eRpHttpNotModified;
        return;     /* just bail out here */
    }
#endif  /* RomPagerFileSystem */

#if RomPagerServer
    /*
        Everything is good, so set up the refresh parameters, and
        do any initial page processing.
    */
    theExtensionPtr = theObjectPtr->fExtensionPtr;
#if RomPagerClientPull
    if (theExtensionPtr == (rpObjectExtensionPtr) 0) {
        theRequestPtr->fRefreshSeconds = 0;
        theRequestPtr->fRefreshObjectPtr = (rpObjectDescriptionPtr) 0;
    }
    else {
        theRequestPtr->fRefreshSeconds = theExtensionPtr->fRefreshSeconds;
        theRequestPtr->fRefreshObjectPtr = theExtensionPtr->fRefreshPagePtr;
        theFunctionPtr = theExtensionPtr->fProcessDataFuncPtr;
        if (theFunctionPtr != (rpProcessDataFuncPtr) 0) {
            theFunctionPtr(theDataPtr, theRequestPtr->fIndexValues);
        }
    }
#else
    if (theExtensionPtr != (rpObjectExtensionPtr) 0) {
        theFunctionPtr = theExtensionPtr->fProcessDataFuncPtr;
        if (theFunctionPtr != (rpProcessDataFuncPtr) 0) {
            theFunctionPtr(theDataPtr, theRequestPtr->fIndexValues);
        }
    }
#endif  /* RomPagerClientPull */
#endif  /* RomPagerServer */

    return;
}
#endif // if 0

//#if RomPagerMimeTypeChecking
#if 0
static Boolean CheckAcceptType(rpHttpRequestPtr theRequestPtr,
            rpDataType theDataType)
{
    rpDataType theAcceptType;
    theAcceptType = theRequestPtr->fAcceptType;
    /*
        We assume HTML is always the right type.
    */
    if (theDataType == eRpDataTypeHtml) {
        return True;
    }
    /*
        We assume plain text is always the right type.
    */
    if (theDataType == eRpDataTypeText) {
        return True;
    }
    /*
        If no Accept statement was received, then we assume everything
        is acceptable.
    */
    if (theAcceptType == eRpDataTypeNone) {
        return True;
    }
    /*
        If the Accept statement specified 'asterisk/asterisk',
        then we can send anything.
    */
    if (theAcceptType == eRpDataTypeAll) {
        return True;
    }
    /*
        If the Accept statement specified multiple images,
        we will send any image.
    */
    if (theAcceptType == eRpDataTypeAnyImage) {
        if (theDataType == eRpDataTypeImageGif ||
                theDataType == eRpDataTypeImageJpeg ||
                theDataType == eRpDataTypeImagePict ||
                theDataType == eRpDataTypeImageTiff ||
                theDataType == eRpDataTypeImagePng) {
            return True;
        }
    }
    /*
        Well, shucks!  This must be a picky browser.
        Signal a rejection.
    */
    return False;
}
#endif


void BuildResponseHeader(PTHttpConnection ptConnection)
{
	PTHttpServerInfo 	pServerInfo = (PTHttpServerInfo) ptConnection->hServer;    
    PCHAR       		pResponseBuf;
    char szTemp[10];
	char szServerName[MAX_NAME_LEN+1];
    
    //rpRealmPtr              theRealmPtr = theRequestPtr->fRealmPtr;

    pResponseBuf = ptConnection->ResponseHeaderBuffer;
    
    // modified by Yun, 2004/11/26
    // If the client gets image files, use HTTP/1.0
    if((ptConnection->tMimeDataType != DATA_TYPE_IMAGE_GIF) && (ptConnection->tMimeDataType !=DATA_TYPE_IMAGE_JPEG))
    {
    	strcpy(pResponseBuf, STR_HTTP_VERSION);	
    }
    else
    {
    	strcpy(pResponseBuf, STR_HTTP_VERSION_1DOT0);
    }
        
    switch (ptConnection->tHttpResponseAct)
    {
        case HTTP_BAD_COMMAND:
        
            strcat(pResponseBuf, STR_405_METHOD_NOT_ALLOWED);
            strcat(pResponseBuf, STR_HTTP_ALLOW);
            strcat(pResponseBuf, gMethodsAllowed);
            break;

        case HTTP_BAD_REQUEST:
        
            strcat(pResponseBuf, STR_400_BAD_REQUEST);
            break;

        case HTTP_NOT_IMPLEMENTED:
        
            strcat(pResponseBuf, STR_501_NOT_IMPLEMENTED);
            break;

//        case eRpHttpForbidden:
//            RP_STRCAT(theResponsePtr, kForbidden);
//            break;

        case HTTP_REQUEST_TOO_LARGE:
        
            strcat(pResponseBuf, STR_413_REQUEST_TOO_LARGE);
            break;

        case HTTP_NEED_AUTHORIZATION:
        
            strcat(pResponseBuf, STR_401_UNAUTHORIZED);

			//ShengFu 2006/01/17 add digest authentication
			if( pServerInfo->iAuthenticateType == HTTPServer_AuthorizationType_Basic )
			{
				strcat(pResponseBuf, STR_HTTP_WWW_AUTHENTICATE_BASIC);
				// modified by Yun, 2004/05/27
				memset(szServerName, 0, MAX_NAME_LEN+1);
				if (pServerInfo->pfnHttpCallback)
				{
					pServerInfo->pfnHttpCallback(pServerInfo->dwContext,
					HTTPServer_Callback_GetServerName, (void *) szServerName);
				}
				if(szServerName[0] != 0)
				{
            		strcat(pResponseBuf, szServerName);
				}
				else
				{
					strcat(pResponseBuf, ((PTHttpServerInfo)(ptConnection->hServer))->szHostName);
				}
				strcat(pResponseBuf, K_QUOTE_CRLF);
			}

			//ShengFu 2006/01/17 add digest authentication
			if( pServerInfo->iAuthenticateType == HTTPServer_AuthorizationType_Digest )
			{
				char	acTemp[60];
				DWORD	dwSec,dwMSec;

				if(pServerInfo->acNonce[0] == 0 )
				{
					memset(acTemp,0,60);
					OSTime_GetTimer(&dwSec, &dwMSec);
					sprintf(acTemp,"%08x%08x",dwSec,dwMSec);
					EncryptionUtl_MD5(pServerInfo->acNonce,acTemp,NULL);
				}
				sprintf(pResponseBuf + strlen(pResponseBuf), "%s qop=\"auth\",realm=\"%s\",nonce=\"%s\"\r\n"
						,STR_HTTP_WWW_AUTHENTICATE_DIGEST
						,pServerInfo->szHostName
						,pServerInfo->acNonce);   			
			}
			
            
            break;

        case HTTP_NO_OBJECT_FOUND:
        
            strcat(pResponseBuf, STR_404_PAGE_NOT_FOUND);
            break;

        case HTTP_SERVICE_UNAVAILABLE:
        
            strcat(pResponseBuf, STR_503_SERVICE_UNAVAILABLE);
            break;
            
//        case eRpHttpFileSystemError:
//            RP_STRCAT(theResponsePtr, kServerError);
//            break;

//        case eRpHttpBadContentType:
//            RP_STRCAT(theResponsePtr, kNoneAcceptable);
//            BuildContentType(theRequestPtr, theResponsePtr);
//            break;

        case HTTP_NOT_MODIFIED:

            strcat(pResponseBuf, STR_304_NOT_MODIFIED);
            break;

        case HTTP_NORMAL:
        default:

            // Build the headers for the normal case
            strcat(pResponseBuf, STR_200_OK);

            // Build the basic Headers
            BuildContentType(ptConnection, pResponseBuf);
            
            // modified by Jason, 2004/12/30
            if (ptConnection->bStreamSocket)
            {
				strcat(pResponseBuf, "Date: Thu, 19 Aug 1982 18:30:00 GMT");
				strcat(pResponseBuf, K_CRLF);
            }
            else
            {
                // Add by Yun, 2004/03/16
                BuildDateHeaders(ptConnection, pResponseBuf);
            }
            
            // Add by Yun, 2004/02/27
			if((ptConnection->tDataSourceType == DATA_SOURCE_FILE) & (ptConnection->dwLastModified != 0))
			{
            	BuildLastModifiedHeaders(ptConnection, pResponseBuf);
			}

            // modified by Jason, 2004/12/30
            if (ptConnection->bStreamSocket)
            {
   				strcat(pResponseBuf, STR_ONEONE_NO_STORE);
    			strcat(pResponseBuf, STR_NO_CACHE);
            }
            else
            {
    			// Add by Yun, 2004/03/16
    			// Build Expires header, let the browser reload dynamic page
    			if((ptConnection->tDataSourceType != DATA_SOURCE_FILE) || 
    			   ((ptConnection->tDataSourceType == DATA_SOURCE_FILE) && (ptConnection->bForceReloadFile == TRUE) ) )
    			{
    				strcat(pResponseBuf, "Expires: Thu, 01 Jan 1970 01:00:00 GMT");
    				strcat(pResponseBuf, K_CRLF);
    //				strcat(pResponseBuf, STR_NO_CACHE);
    				strcat(pResponseBuf, STR_ONEONE_NO_CACHE);
    			}
    		}

            // Add by Yun, 2004/02/19
            if(ptConnection->tMimeDataType == DATA_TYPE_APPLICATION_OCTETSTREAM)
            {
            	strcat(pResponseBuf, STR_HTTP_CONTENT_DISPOSITION_ATTACHMENT);
            	strcat(pResponseBuf, K_CRLF);
            }
            else
            {
            	BuildContentLanguage(ptConnection, pResponseBuf);
            }
            
            BuildContentLength(ptConnection, pResponseBuf);
            if (ptConnection->bStreamSocket)
            {
                // add streamming ID header
                strcat(pResponseBuf, STR_STREAM_ID);
                sprintf(szTemp, "%d", ptConnection->dwStreamID);
                strcat(pResponseBuf, szTemp);
                strcat(pResponseBuf, K_CRLF);
                // add audio mode header
                if (ptConnection->iAudioMode != AUDIO_TRANSMODE_UNKNOWN)
                {
                    strcat(pResponseBuf, STR_AUDIO_MODE);
                    switch(ptConnection->iAudioMode)
                    {
                        case AUDIO_TRANSMODE_FULLDUPLEX:
                            strcat(pResponseBuf, STR_AUDIO_FULLDUPLEX);
                            break;
                        case AUDIO_TRANSMODE_HALFDUPLEX:
                            strcat(pResponseBuf, STR_AUDIO_HALFDUPLEX);
                            break;
                        case AUDIO_TRANSMODE_TALK:
                            strcat(pResponseBuf, STR_AUDIO_TALKONLY);
                            break;
                        case AUDIO_TRANSMODE_LISTEN:
                            strcat(pResponseBuf, STR_AUDIO_LISTENONLY);
                            break;
                    	case AUDIO_TRANSMODE_NONE:
                    		strcat(pResponseBuf, STR_AUDIO_NONE);
                    		break;                            
                    }
                    strcat(pResponseBuf, K_CRLF);
                }
                
                // add by Yun, 2003/12/25
                strcat(pResponseBuf, STR_USER_PRIVILEGE);
                sprintf(szTemp, "%u\x0d\x0a", ptConnection->dwPrivilege);
                strcat(pResponseBuf, szTemp);
            }
            break;
    }

    // Build some more headers and set up the transaction state
    switch (ptConnection->tHttpResponseAct)
    {
        //case eRpHttpRedirect:
        //case eRpHttpRedirectMap:
        case HTTP_NOT_MODIFIED:
        //case eRpHttpPutCompleted:
        //    BuildKeepAliveResponse(theRequestPtr, theResponsePtr);
        case HTTP_BAD_COMMAND:
        case HTTP_BAD_REQUEST:
        case HTTP_NOT_IMPLEMENTED:
        case HTTP_SERVICE_UNAVAILABLE:
        //case eRpHttpExpectFailed:
        //case eRpHttpBadContentType:
        default:
            // There is no object for this response.
            strcat(pResponseBuf, STR_HTTP_CONTENT_LENGTH);
            strcat(pResponseBuf, K_EMPTYLENGTH);
            ptConnection->tHttpState = HTTP_STATE_RESPONSE_COMPLETE;
            break;

        case HTTP_REQUEST_TOO_LARGE:
        //case eRpHttpForbidden:
        case HTTP_NEED_AUTHORIZATION:
        case HTTP_NO_OBJECT_FOUND:
        //case eRpHttpFileSystemError:
        //case eRpHttpMultipleChoices:
            // There is an explanation object for the error response.
            strcat(pResponseBuf, STR_HTTP_CONTENT_TYPE);
            strcat(pResponseBuf, STR_TYPE_HTML);
            strcat(pResponseBuf, K_CRLF);
            if (ptConnection->tHttpVersion == HTTP_ONE_DOT_ONE)
            {
                //strcat(pResponseBuf, kHttpTransferEncodingChunked);
                //ptConnection->fResponseIsChunked = True;
                //ptConnection->fPersistent = False;
            }
            
            // Set source for internal error messages.
            //ptConnection->fObjectSource = eRpUserUrl;
            ptConnection->tHttpState = HTTP_STATE_SEND_BODY;
            break;

        case HTTP_NORMAL:
            /*
                For HTTP 1.0 Keep-Alive, we can only keep a persistent
                connection for objects that we know the Content-Length.
                This eliminates dynamic pages.
            */
            //if (theRequestPtr->fObjectPtr->fCacheObjectType == eRpObjectTypeStatic)
            //{
            //    BuildKeepAliveResponse(theRequestPtr, theResponsePtr);
            //}
            if (ptConnection->tHttpCommand == HTTP_HEAD_COMMAND ||
                ptConnection->tHttpCommand == HTTP_OPTIONS_COMMAND)
            {
                // If the command is "HEAD" or "OPTIONS" all the client wants
                // are the headers.
                ptConnection->tHttpState = HTTP_STATE_RESPONSE_COMPLETE;
            }
            else
            {
                if (ptConnection->bStreamSocket)
                    ptConnection->tHttpState = HTTP_STATE_RESPONSE_COMPLETE;
                else if (ptConnection->tDataSourceType == DATA_SOURCE_PENDING)
                    ptConnection->tHttpState = HTTP_STATE_DATA_PENDING;
                else
                    ptConnection->tHttpState = HTTP_STATE_SEND_BODY;
            }
            break;
    }

    // Build the last headers and set up HTML buffer
    //strcat(pResponseBuf, STR_SERVER_HEADER);
    // modified by Yun, 2004/05/27
    strcat(pResponseBuf, STR_SERVER_HEADER);
    strcat(pResponseBuf, ((PTHttpServerInfo)(ptConnection->hServer))->szHostName);
    strcat(pResponseBuf, K_CRLF);

    // modified by Jason, 2004/12/30 - StreamSocket reply close
    if ((ptConnection->tHttpVersion == HTTP_ONE_DOT_ONE && !ptConnection->bPersistent)
        || ptConnection->bStreamSocket)
    {
        /*
            If we have a HTTP 1.1 request, but for whatever
            reason (usually an error message) we are not going
            to keep the connection open, signal that we will
            close it.
        */
        strcat(pResponseBuf, STR_HTTP_CONNECTION);
        strcat(pResponseBuf, STR_HTTP_CLOSE);
        strcat(pResponseBuf, K_CRLF);
    }
    
    // Signal end of headers
    strcat(pResponseBuf, K_CRLF);

    //RpInitializeHtmlPageReply(theRequestPtr);
    return;
}

static void BuildContentType(PTHttpConnection ptConnection, char *pResponseBuf)
{
    strcat(pResponseBuf, STR_HTTP_CONTENT_TYPE);
    strcat(pResponseBuf, (char *) gMimeTypes[ptConnection->tMimeDataType]);
    strcat(pResponseBuf, K_CRLF);

    return;
}

// Add by Yun, 2004/02/19
static void BuildContentLanguage(PTHttpConnection ptConnection, char *pResponseBuf)
{
    strcat(pResponseBuf, STR_HTTP_CONTENT_LANGUAGE);
    strcat(pResponseBuf, (char *) ((PTHttpServerInfo)(ptConnection->hServer))->szLanguage);
    strcat(pResponseBuf, K_CRLF);	
}

static void BuildLastModifiedHeaders(PTHttpConnection ptConnection, char *pResponseBuf)
{
	TOSDateTimeInfo	 	tDateInfo;
	struct tm 			tmBlock;
	char szDateString[40];
	
	OSTime_TimerToDateTime(ptConnection->dwLastModified, &tDateInfo);
	
	memset(&tmBlock, 0, sizeof(tmBlock));
	tmBlock.tm_sec	= tDateInfo.wSecond;
	tmBlock.tm_min	= tDateInfo.wMinute;
	tmBlock.tm_hour = tDateInfo.wHour;
	tmBlock.tm_mday = tDateInfo.wMonthDay;
	tmBlock.tm_mon	= tDateInfo.wMonth - 1;
	tmBlock.tm_year = tDateInfo.wYear - 1900;
	tmBlock.tm_isdst= -1;
	    
    if(!strftime(szDateString, 40, "%a, %d %b %Y %H:%M:%S GMT", &tmBlock))
    {
        return;
    }   
    
    strcat(pResponseBuf, STR_HTTP_LAST_MODIFIED);
    strcat(pResponseBuf, (char *) szDateString);
    strcat(pResponseBuf, K_CRLF);
}

#if 1
static void BuildDateHeaders(PTHttpConnection ptConnection, char *pResponseBuf)
{
	TOSDateTimeInfo	 	tDateInfo;
	struct tm 			tmBlock;
	char				szDateString[40];
	
	OSTime_GetDateTime(&tDateInfo);

	memset(&tmBlock, 0, sizeof(tmBlock));
	tmBlock.tm_sec	= tDateInfo.wSecond;
	tmBlock.tm_min	= tDateInfo.wMinute;
	tmBlock.tm_hour = tDateInfo.wHour;
	tmBlock.tm_mday = tDateInfo.wMonthDay;
	tmBlock.tm_mon	= tDateInfo.wMonth - 1;
	tmBlock.tm_year = tDateInfo.wYear - 1900;
	tmBlock.tm_isdst= -1;
	    
    if(!strftime(szDateString, 40, "%a, %d %b %Y %H:%M:%S GMT", &tmBlock))
    {
        return;
    }
	
	strcat(pResponseBuf, STR_HTTP_DATE);
	strcat(pResponseBuf, (char *) szDateString);
	strcat(pResponseBuf, K_CRLF);
}
#else
static void BuildDateHeaders(PTHttpConnection ptConnection, char *pResponseBuf)
{
    Unsigned32          theCurrentTime;
    rpDataPtr           theDataPtr;
    char                theDateString[80];
#if !RpNoCache
    rpObjectSource      theObjectSource;
#endif

    theDataPtr = theRequestPtr->fDataPtr;

    /*
        For HTTP 1.0 requests, the "Date","Expires" and "Last-Modified"
        headers are created to obtain optimal caching from various browsers
        with different caching algorithms. The HTTP 1.1 specifications
        more clearly define caching behavior, so the headers we create
        follow the specifications.

        Rom Objects "Date"

        For HTTP 1.0 requests, we report the current time as best we can.
        If we have calendar support, this will be pretty accurate.  If not, we
        look to see if the browser sent us a date in an "If-Modified-Since"
        header.  If it did, we create a current date based on the browser
        date + 1 second, otherwise, we create the date based on the rom date
        plus the time since the system was booted.

        For HTTP 1.1 requests, if we have a calendar date, we report it in
        the "Date" header.  Otherwise, we don't report a date and assume that
        the browser or a proxy along the way will fill in the value.

        Rom Objects "Expires" and "Last-Modified"

        For static ROM objects, we use the ROM date for the
        "Last-Modified" header and do not send an "Expires" header.

        For dynamic ROM objects we send "Expires" header with an old
        date and a "Last-Modified" header with the current date.  For
        HTTP 1.0 requests, we also send the "Pragma: no-cache" header.
        For HTTP 1.1 requests, we send the "Cache-Control: no-cache"
        header, and only send the "Expires" and "Last-Modified" headers
        if we have calendar time.

        User Exit (CGI) Objects

        User Exit objects can be either static or dynamic.  Dynamic objects
        are treated the same as dynamic ROM objects.  Static objects use the
        date from the Cgi control block for the "Last-Modified" header.
    */

    theCurrentTime = RpGetSysTimeInSeconds();
    RpBuildDateString(theDateString, theCurrentTime);

    /*
        Build the Date header for non-Remote Host objects
    */

    RP_STRCAT(theResponsePtr, kHttpDate);
    RP_STRCAT(theResponsePtr, theDateString);

    /*
        Build the caching headers
    */

    theObjectSource = theRequestPtr->fObjectSource;

    if (theRequestPtr->fObjectPtr->fCacheObjectType == eRpObjectTypeDynamic)
    {
        // We have a dynamic object, so force the browser to refresh it.
        if (theRequestPtr->fHttpVersion == eRpHttpOneDotOne)
        {
            RP_STRCAT(theResponsePtr, kHttpOneOneNoCache);
            RP_STRCAT(theResponsePtr, kHttpExpires);
            RP_STRCAT(theResponsePtr, theDataPtr->fExpiresDateString);
        }
        else
        {
            RP_STRCAT(theResponsePtr, kHttpExpires);
            RP_STRCAT(theResponsePtr, theDataPtr->fExpiresDateString);
            RP_STRCAT(theResponsePtr, kHttpLastModified);
            RP_STRCAT(theResponsePtr, theDateString);
            RP_STRCAT(theResponsePtr, kHttpNoCache);
        }
    }
    else
    {
        /*
            We have a static object, so send out the caching headers.
        */
        RP_STRCAT(theResponsePtr, kHttpLastModified);
        switch (theObjectSource)
        {
            case eRpUserUrl:
                RpBuildDateString(theDateString,
                        theRequestPtr->fCgi.fObjectDate);
                RP_STRCAT(theResponsePtr, theDateString);
                break;
            default:
                RP_STRCAT(theResponsePtr, theDataPtr->fRomDateString);
                break;
        }
    }

    return;
}
#endif

static void BuildContentLength(PTHttpConnection ptConnection, char *pResponseBuf)
{
    DWORD dwLength;
    char szTemp[30];

    // Send out Content-Length header, if we know the length.
    // Otherwise, omit the header and the browser will figure
    // it out after we close the connection.
    dwLength = ptConnection->dwDataSourceLength;
    
    if (dwLength)
    {
        strcat(pResponseBuf, STR_HTTP_CONTENT_LENGTH);
        sprintf(szTemp, "%d", dwLength);
        strcat(pResponseBuf, szTemp);
        //RpCatUnsigned32ToString(theLength, theResponsePtr, 0);
        strcat(pResponseBuf, K_CRLF);
    }
    else if (ptConnection->tHttpVersion == HTTP_ONE_DOT_ONE)
    {
        //theRequestPtr->fResponseIsChunked = True;
        //strcat(pResponseBuf, kHttpTransferEncodingChunked);
    }
    return;
}

static void BuildKeepAliveResponse(PTHttpConnection ptConnection, char *pResponseBuf)
{
    if (ptConnection->bKeepAlive)
    {
        // Send the keep-alive headers.
        strcat(pResponseBuf, STR_HTTP_CONNECTION);
        strcat(pResponseBuf, STR_HTTP_KEEP_ALIVE);
        strcat(pResponseBuf, K_CRLF);
        // Mark the connection to stay open.
        ptConnection->bPersistent = TRUE;
    }
    return;
}

/*
void RpSetConnectionClose(void *theServerDataPtr)
{
    ((rpDataPtr) theServerDataPtr)->fCurrentHttpRequestPtr->fPersistent = False;
    return;
}
*/

void BuildHostName(PTHttpConnection ptConnection, char *pResponseBuf)
{
   // strcat(pResponseBuf, ptConnection->szHost);
    return;
}

