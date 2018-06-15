/*
 *  File:       RpHttpPs.c
 *
 */

#include "httpserver_local.h"

static PatternProcedure ProcessGet;
static PatternProcedure ProcessHead;
static PatternProcedure ProcessPost;
static PatternProcedure ProcessAuthorization;
static PatternProcedure ProcessHost;
static PatternProcedure ProcessContentLength;
static PatternProcedure ProcessContentType;
static PatternProcedure ProcessIfModified;
static PatternProcedure ProcessMpDisposition;
static PatternProcedure ProcessMpContentType;
static PatternProcedure ProcessName;
static PatternProcedure ProcessFilename;
// Added by Jason 2004/12/30, for passing proxy server
static PatternProcedure ProcessSessionCookie;



static void ProcessMethod(PTHttpConnection ptConnection, char *pszStartOfToken,
                          UINT uiTokenLength);

static void StoreHeaderValue(char *pszHeader, char *pszStorage, UINT uiTokenLength);
static void ParseNextItem(PTHttpConnection ptConnection);


// Global Http pattern table
TPatternTable  gtPatternTable[] = {
    {ProcessGet,           HTTP_METHOD_GET,             sizeof(HTTP_METHOD_GET) - 1},
    {ProcessHead,          HTTP_METHOD_HEAD,            sizeof(HTTP_METHOD_HEAD) - 1},
    {ProcessPost,          HTTP_METHOD_POST,            sizeof(HTTP_METHOD_POST) - 1},
    {ProcessAuthorization, HTTP_PATTERN_AUTHORIZATION,  sizeof(HTTP_PATTERN_AUTHORIZATION) - 1},
    {ProcessHost,          HTTP_PATTERN_HOST,           sizeof(HTTP_PATTERN_HOST) - 1},
    {ProcessContentLength, HTTP_PATTERN_CONTENT_LENGTH, sizeof(HTTP_PATTERN_CONTENT_LENGTH) - 1},
    {ProcessContentType,   HTTP_PATTERN_CONTENT_TYPE,   sizeof(HTTP_PATTERN_CONTENT_TYPE) - 1},
    {ProcessIfModified,	   HTTP_PATTERN_IFMODIFIED,		sizeof(HTTP_PATTERN_IFMODIFIED) -1},
    // Added by Jason 2004/12/30, for passing proxy server
    {ProcessSessionCookie, HTTP_PATTERN_SESSIONCOOKIE,  sizeof(HTTP_PATTERN_SESSIONCOOKIE) -1},
    {0, 0, 0}
};

TPatternTable  gtMpPatternTable[] = {
    {ProcessMpDisposition, HTTP_PATTERN_DISPOSITION,    sizeof(HTTP_PATTERN_DISPOSITION) - 1},
    {ProcessMpContentType, HTTP_PATTERN_CONTENT_TYPE,   sizeof(HTTP_PATTERN_CONTENT_TYPE) - 1},
    {0, 0, 0}
};

TPatternTable  gtDispPatternTable[] = {
    {ProcessName,          STR_HTTP_NAME,               sizeof(STR_HTTP_NAME) - 1},
    {ProcessFilename,      STR_HTTP_FILENAME,           sizeof(STR_HTTP_FILENAME) - 1},
    {0, 0, 0}
};

// Parse one http header
void HandleHttpHeader(PTPatternTable ptPatternTable, PTHttpConnection ptConnection)
{
    PTParsingControl ptParsing;
    char *           pszBeginLine;
    char *           pszStartOfToken;
    UINT             uiTokenLength;

    ptParsing = &ptConnection->tParsingCtrl;
    pszBeginLine = ptParsing->pszCurrentBeginLine;
    
    // look for the HTTP header in the header table
    while (ptPatternTable->uiPatternLength != 0)
    {
        if (memcmp(pszBeginLine, ptPatternTable->pszPattern,
                   ptPatternTable->uiPatternLength) == 0)
        {
            // We found a line we need to do something with.
            // Skip over the matching pattern....
            pszBeginLine += ptPatternTable->uiPatternLength;
            
            // find the next token
            pszStartOfToken = FindTokenStart(pszBeginLine);
            uiTokenLength = FindTokenEnd(pszStartOfToken);

            // pass the token to the action routine
            (*ptPatternTable->fAction)(ptConnection, pszStartOfToken, uiTokenLength);
            
            break;
        }
        else
        {
            ptPatternTable += 1;
        }
    }
    return;
}

static void ProcessGet(PTHttpConnection ptConnection, char *pszStartOfToken,
                       UINT uiTokenLength)
{
    ptConnection->tHttpCommand = HTTP_GET_COMMAND;
    ProcessMethod(ptConnection, pszStartOfToken, uiTokenLength);
    return;
}

static void ProcessHead(PTHttpConnection ptConnection, char *pszStartOfToken,
                        UINT uiTokenLength)
{
    ptConnection->tHttpCommand = HTTP_HEAD_COMMAND;
    ProcessMethod(ptConnection, pszStartOfToken, uiTokenLength);
    return;
}

static void ProcessPost(PTHttpConnection ptConnection, char *pszStartOfToken,
                        UINT uiTokenLength)
{
    ptConnection->tHttpCommand = HTTP_POST_COMMAND;
    ProcessMethod(ptConnection, pszStartOfToken, uiTokenLength);
    return;
}

static void ProcessAuthorization(PTHttpConnection ptConnection, char *pszStartOfToken,
                                 UINT uiTokenLength)
{
    char    szDecodedData[MAX_NAME_LENGTH * 3];
    char *  pszPassword;

	THttpServerInfo *PTHttpServerInfo = (THttpServerInfo *)ptConnection->hServer;


	//ShengFu 2006/01/16 add digest authentication
	if( PTHttpServerInfo->iAuthenticateType == HTTPServer_AuthorizationType_Basic )
	{
		// MS IE 3.02 under Windows returns an empty Authorization header if
		// the user cancels out of the Authentication dialog.  So, let's just
		// pretend we never got it.
		if (uiTokenLength < 5)
			return;

		// currently we support only basic authorization
		// Skip past "Basic" (already done above if RomPagerSecurityDigest).
		pszStartOfToken += (uiTokenLength + 1);

		// Parse the authorization string.
		uiTokenLength = FindLineEnd(pszStartOfToken);
		*(pszStartOfToken + uiTokenLength) = '\0';

		// Add by Joe 2003/10/27, zero the username & password to avoid other user to
		// use the same connection without name & password
		//ptConnection->szUsername[0] = 0;
		//ptConnection->szPassword[0] = 0;

		// Test the encoded username/password string size to make sure the
		// decoded string will fit in the work buffer. We could use a larger
		// number to test here, but anything larger will fail the next length
		// checks, so let's just reject it here.

		if (uiTokenLength < (MAX_NAME_LENGTH * 3))
		{
			DecodeBase64Data(pszStartOfToken, uiTokenLength, szDecodedData);

			// Find the separation between the Username and Password.
			pszPassword = szDecodedData + FindTokenDelimited(szDecodedData, ASCII_Colon);

			if (*pszPassword == ASCII_Colon)
			{
				*pszPassword++ = '\0';
			}
			StrLenCpy(ptConnection->szUsername, szDecodedData, MAX_NAME_LENGTH);
			StrLenCpy(ptConnection->szPassword, pszPassword, MAX_NAME_LENGTH);
			DbgPrint(("[HS_HEADER][%02d] user=%s, pass=%s\r\n",
			ptConnection->uiConnectionIndex,
			ptConnection->szUsername, ptConnection->szPassword));
		}

	}

	//ShengFu 2006/01/16 add digest authentication
	if( PTHttpServerInfo->iAuthenticateType == HTTPServer_AuthorizationType_Digest )
	{
		THTTPRAWAUTHORINFO tHttpRawAuthInfo;
		THTTPServer_Author_Info tAuthorInfo;
		char acHashA1[33],acHashA2[33],acResponse[33];
		
		// Parse the authorization string.
		uiTokenLength = FindLineEnd(pszStartOfToken);
		*(pszStartOfToken + uiTokenLength) = '\0';

		memset((void *)&tHttpRawAuthInfo,0,sizeof(THTTPRAWAUTHORINFO));
		memset((void *)&tAuthorInfo,0,sizeof(THTTPServer_Author_Info));

		if( ParseAuthorDigestInfo(pszStartOfToken,&tHttpRawAuthInfo) == 0 )
		{			
			if( strcmp(tHttpRawAuthInfo.acNonce,PTHttpServerInfo->acNonce) != 0 )
			{
				memset(ptConnection->szPassword,0,MAX_NAME_LENGTH);	
				return;
			}

			strcpy(tAuthorInfo.pszUserName,tHttpRawAuthInfo.acUserName);
			PTHttpServerInfo->pfnHttpCallback(PTHttpServerInfo->dwContext,
                           HTTPServer_Callback_Digest_Auth_Request, (void *) &tAuthorInfo);
			
			if( tAuthorInfo.pszPassword[0] == 0 )
				return;

			memset(acHashA1,0,33);
			memset(acHashA2,0,33);
			memset(acResponse,0,33);

			EncryptionUtl_MD5(acHashA1,tAuthorInfo.pszUserName,":",PTHttpServerInfo->szHostName,":",tAuthorInfo.pszPassword,NULL);
			
			if( ptConnection->tHttpCommand == HTTP_GET_COMMAND )
				EncryptionUtl_MD5(acHashA2,"GET",":",ptConnection->szPath,NULL);

			if( ptConnection->tHttpCommand == HTTP_POST_COMMAND )
				EncryptionUtl_MD5(acHashA2,"POST",":",ptConnection->szPath,NULL);

						
			EncryptionUtl_MD5(acResponse,acHashA1,":"
						  ,tHttpRawAuthInfo.acNonce,":"							
						  ,tHttpRawAuthInfo.acNonceCount,":"
						  ,tHttpRawAuthInfo.acCNonce,":"
						  ,"auth",":"
						  ,acHashA2,NULL);
			
			StrLenCpy(ptConnection->szUsername,tHttpRawAuthInfo.acUserName, MAX_NAME_LENGTH);
			if(memcmp(acResponse,tHttpRawAuthInfo.acResponse,32) == 0 )
			{
				StrLenCpy(ptConnection->szPassword,"DIGESTOK" , MAX_NAME_LENGTH);
				memset(PTHttpServerInfo->acNonce,0,33);
			}
			else
			{
				memset(ptConnection->szPassword,0,MAX_NAME_LENGTH);				
			}
		}           
	}

    return;
}

static void ProcessConnection(PTHttpConnection ptConnection, char *pszStartOfToken,
                              UINT uiTokenLength)
{
    *(pszStartOfToken + uiTokenLength) = '\0';
    
    if (ptConnection->tHttpVersion == HTTP_ONE_DOT_ONE)
    {
        if (strcmp(pszStartOfToken, STR_HTTP_CLOSE) == 0)
        {
            ptConnection->bPersistent = FALSE;
        }
    }
    else
    {
        if (strcmp(pszStartOfToken, STR_HTTP_KEEP_ALIVE) == 0)
        {
            ptConnection->bKeepAlive = TRUE;
        }
        else if (strcmp(pszStartOfToken, STR_HTTP_CLOSE) == 0)
        {
            ptConnection->bKeepAlive = FALSE;
        }
    }
    return;
}

static void ProcessHost(PTHttpConnection ptConnection, char *pszStartOfToken,
                        UINT uiTokenLength)
{
    if (!ptConnection->bHaveHost)
	{
        ptConnection->bHaveHost = TRUE;
        StoreHeaderValue(pszStartOfToken, ptConnection->szHost, 0);
    }
    return;
}

static void ProcessContentLength(PTHttpConnection ptConnection, char *pszStartOfToken,
                                 UINT uiTokenLength)
{
    DWORD  dwContentLength;

    *(pszStartOfToken + uiTokenLength) = '\0';
    dwContentLength = atol(pszStartOfToken);
    ptConnection->dwPostRequestLength = dwContentLength;
    if (dwContentLength > 0)
    {
        ptConnection->bHaveRequestObject = TRUE;
    }
    return;
}

static void ProcessContentType(PTHttpConnection ptConnection, char *pszStartOfToken,
                               UINT uiTokenLength)
{
    if (uiTokenLength >= MAX_MIME_TYPE_LENGTH)
    {
        ptConnection->tHttpResponseAct = HTTP_BAD_REQUEST;
        ptConnection->tHttpState = HTTP_STATE_SEND_HEADERS;
    }
    else
    {
        // Save the Content-Type.
        *(pszStartOfToken + uiTokenLength) = '\0';
        strcpy(ptConnection->szHttpContentType, pszStartOfToken);

        // If the Content-Type is multipart/form-data, we need to get
        // the boundary string.  The boundary token must be the next
        // token after the Content-Type value.
        ConvertTokenToLowerCase(pszStartOfToken, uiTokenLength);
        if (strcmp(pszStartOfToken, STR_TYPE_MULTIPARTFORM) == 0)
        {
            pszStartOfToken += (uiTokenLength + 1);
            pszStartOfToken = FindTokenStart(pszStartOfToken);

            // is this the boundary token?
            if (memcmp(pszStartOfToken, STR_HTTP_BOUNDARY, (sizeof(STR_HTTP_BOUNDARY) - 1)) == 0)
            {
                // found the boundary token.  now find the beginning of the
                // boundary value.
                pszStartOfToken = FindTokenDelimitedPtr(pszStartOfToken, ASCII_Equal);
                pszStartOfToken++;

                // find the length of the boundary value and save it
                // in the request structure.
                uiTokenLength = FindLineEnd(pszStartOfToken);
                ptConnection->dwBoundaryLength = uiTokenLength;

                // make the boundary value a C string and save it
                // in the request structure.
                *(pszStartOfToken + uiTokenLength) = '\0';
                StrLenCpy(ptConnection->szBoundary, pszStartOfToken,
                            MAX_BOUNDARY_LENGTH);
            }

            /*
                If the Content Type is multipart/form-data but we didn't
                get a boundary, null out fHttpContentType so the data will
                be processed as regular object data.

                Why you ask would this ever arise?  Because some people's
                Java implementation of SendUrl are incorrect.  Now you know.
            */
            if (*ptConnection->szBoundary == '\0')
            {
                *ptConnection->szHttpContentType = '\0';
            }
        }
    }
    return;
}

void ProcessMpDisposition(PTHttpConnection ptConnection, char *pszStartOfToken,
                          UINT uiTokenLength)
{
    PTParsingControl ptParsing;

    ptParsing = &ptConnection->tParsingCtrl;

    do {
        ParseNextItem(ptConnection);
    } while (*ptParsing->pszCurrentBeginLine != ASCII_Newline);

    ptParsing->pszCurrentBeginLine++;
    
    return;
}

void ProcessMpContentType(PTHttpConnection ptConnection, char *pszStartOfToken,
                          UINT uiTokenLength)
{
    /*
    rpDataType                  theDataType;

    *(theStartOfTokenPtr + theTokenLength) = '\0';
    theDataType = RpStringToMimeType(theStartOfTokenPtr);
    theRequestPtr->fFileInfoPtr->fFileType = theDataType;

    if (theDataType == eRpDataTypeOther)
    {
        StrLenCpyTruncate(theRequestPtr->fFileInfoPtr->fOtherMimeType,
                            theStartOfTokenPtr, kMaxMimeTypeLength);
    }
    */
    return;
}

static void ProcessName(PTHttpConnection ptConnection, char *pszStartOfToken,
                        UINT uiTokenLength)
{
    *(pszStartOfToken + uiTokenLength) = '\0';
    StrLenCpy(ptConnection->szItemName, pszStartOfToken, MAX_NAME_LENGTH);
    
    return;
}

static void ProcessFilename(PTHttpConnection ptConnection, char *pszStartOfToken,
                            UINT uiTokenLength)
{
    char*  pszString;
    char   szTempFilename[MAX_LINE_LENGTH];
    DWORD  dwLength;

    if (uiTokenLength >= MAX_LINE_LENGTH)
    {
        uiTokenLength = MAX_LINE_LENGTH - 1;
    }
    *(pszStartOfToken + uiTokenLength) = '\0';

    // use EscapeDecodeString to copy the filename from the incoming
    // request to the request structure.  this will decode any '%xx'
    // encoded characters.
    EscapeDecodeString(pszStartOfToken, uiTokenLength, szTempFilename, &dwLength, FALSE);
    szTempFilename[dwLength] = '\0';
    pszString = szTempFilename + strlen(szTempFilename);

    // Remove any path from the filename.
    while (pszString >= szTempFilename)
    {
        if (*pszString == ASCII_Slash || *pszString == ASCII_Backslash)
        {
            break;
        }
        pszString--;
    }
    pszString++;

    StrLenCpy(ptConnection->szFilename, pszString, MAX_VALUE_LENGTH);

    return;
}

static void ParseNextItem(PTHttpConnection ptConnection)
{
    char*    pszStartOfValue;
    char*    pszBeginLine;
    char*    pszNextItem;
    UINT     uiValueLength;
    PTParsingControl ptParsing;
    PTPatternTable   ptPatternTable;

    ptPatternTable = gtDispPatternTable;
    ptParsing = &ptConnection->tParsingCtrl;
    pszBeginLine = ptParsing->pszCurrentBeginLine;
    pszNextItem = pszBeginLine;

    /*
        Find the beginning of the next item/value pair, or
        the end of the line so we know how far to skip ahead
        after processing this item.

        Item/value pairs can be separated by commas (in the
        Authorization header), or semi-colons (in the
        Content-Disposition header).
    */
    while (*pszNextItem != ASCII_Newline)
    {
        if (*pszNextItem == ASCII_Comma || *pszNextItem == ASCII_SemiColon)
        {
            pszNextItem++;
            while (*pszNextItem == ASCII_Space)
            {
                pszNextItem++;
            }
            break;
        }
        else
        {
            pszNextItem++;
        }
    }

    while (ptPatternTable->uiPatternLength != 0)
    {
        if (memcmp(pszBeginLine, ptPatternTable->pszPattern,
                    ptPatternTable->uiPatternLength) == 0)
        {
            // We found a line we need to do something with,
            // so skip what we matched.
            pszBeginLine += ptPatternTable->uiPatternLength;

            // Find the value.
            pszStartOfValue = FindValueStart(pszBeginLine);
            uiValueLength = FindValueLength(pszStartOfValue);

            // Pass the value to the action routine.
            (*ptPatternTable->fAction)(ptConnection, pszStartOfValue,
                                        uiValueLength);
            break;
        }
        else
        {
            ptPatternTable += 1;
        }
    }

    ptParsing->pszCurrentBeginLine = pszNextItem;
    return;
}

static void ProcessIfModified(PTHttpConnection ptConnection, char *pszStartOfToken,
                               UINT uiTokenLength)
{
    char szTempPath[MAX_SAVE_HEADER_LENGTH];
    char *pcTmp;
    
    StoreHeaderValue(pszStartOfToken, szTempPath, 0);
            
    if(szTempPath[0] == '\0')
    {
    	ptConnection->dwIfModifiedSince = 0;    	
    }
    else
    {
    	// get length field, add by Yun, 2004/11/26    	
    	if((pcTmp= (char*) strstr(szTempPath, "length=")) != NULL)
    	{
    		ptConnection->dwOrgLength = atoi(pcTmp+7);
    	}
    	ptConnection->dwIfModifiedSince = ParseDate(szTempPath);
    }
    	
	return;
}                               

// Added by Jason 2004/12/30, for passing proxy server
static void ProcessSessionCookie(PTHttpConnection ptConnection, char *pszStartOfToken,
                                 UINT uiTokenLength)
{
    StoreHeaderValue(pszStartOfToken, ptConnection->szSessionCookie, 0);
    return;
}

/*
static void ProcessIfModified(rpHttpRequestPtr theRequestPtr,
        char *theStartOfTokenPtr,
        Unsigned16 theTokenLength)
{
    RpStoreHeader(theStartOfTokenPtr, theRequestPtr->fIfModified, 0);
    if (*theRequestPtr->fIfModified == '\0')
    {
        // the browser didn't pass us a date
        theRequestPtr->fBrowserDate = 0;
    }
    else
    {
        theRequestPtr->fBrowserDate = RpParseDate(theRequestPtr->fIfModified);
    }
    return;
}
*/

static void StoreHeaderValue(char *pszHeader, char *pszStorage, UINT uiTokenLength)
{
    // If no length was provided, use the end of the line.
    if (uiTokenLength == 0)
    {
        uiTokenLength = FindLineEnd(pszHeader);
    }

    *(pszHeader + uiTokenLength) = '\0';

    // Store as much of the header as will fit into the save area.
    StrLenCpyTruncate(pszStorage, pszHeader, MAX_SAVE_HEADER_LENGTH);
    
    return;
}

/*
static void ProcessReferer(rpHttpRequestPtr theRequestPtr,
        char *theStartOfTokenPtr,
        Unsigned16 theTokenLength) {

    RpStoreHeader(theStartOfTokenPtr, theRequestPtr->fRefererUrl,
                    theTokenLength);
    return;
}

static void ProcessPut(rpHttpRequestPtr theRequestPtr,
        char *theStartOfTokenPtr,
        Unsigned16 theTokenLength) {

    theRequestPtr->fHttpCommand = eRpHttpPutCommand;
    ProcessMethod(theRequestPtr, theStartOfTokenPtr, theTokenLength);
    return;
}

static void ProcessOptions(rpHttpRequestPtr theRequestPtr,
        char *theStartOfTokenPtr,
        Unsigned16 theTokenLength) {

    theRequestPtr->fHttpCommand = eRpHttpOptionsCommand;
    theRequestPtr->fHttpResponseState = eRpHttpOptions;
    ProcessMethod(theRequestPtr, theStartOfTokenPtr, theTokenLength);
    return;
}
*/
static void ProcessMethod(PTHttpConnection ptConnection, char *pszStartOfToken,
                          UINT uiTokenLength)
{
    char szTempPath[MAX_SAVE_HEADER_LENGTH];
    DWORD dwLength;
    
    // Save the value for the header (URI path)
    StoreHeaderValue(pszStartOfToken, szTempPath, uiTokenLength);
    EscapeDecodeString(szTempPath, strlen(szTempPath), ptConnection->szPath, &dwLength, FALSE);
    ptConnection->szPath[dwLength] = '\0';

    // Find out what HTTP version the request is
    pszStartOfToken += uiTokenLength + 8;
    if (*pszStartOfToken >= '1')
    {
        ptConnection->tHttpVersion = HTTP_ONE_DOT_ONE;
        
        // HTTP 1.1 or greater connections are assumed to be persistent
        // Let the connection to be close, 2004/07/19 by Yun.
        //ptConnection->bPersistent = TRUE;

        /*
            Well the path given us might be an absolute URI of the form
            http://<hostname>/<absolutepath>.  In HTTP 1.1 we are never
            supposed to receive this, but you never know and in versions
            greater that 1.1 we can "definitely" expect to see this.

            So we need to see if the path is of this form, and if so
            dismember it into it's component parts.
        */
        pszStartOfToken = ptConnection->szPath;
        uiTokenLength = FindTokenDelimited(pszStartOfToken, ASCII_Colon);
        if (*(pszStartOfToken + uiTokenLength) == '\0')
        {
            // No colons, so this is a normal path
            // and we can just return.
            return;
        }
        if (*(pszStartOfToken + uiTokenLength + 1) == ASCII_Slash &&
            *(pszStartOfToken + uiTokenLength + 2) == ASCII_Slash)
        {
            /*
                Well, we have a "://" in the path, a sure sign of an
                absolute URI.  Sigh....

                Now we have to dismember it.

                First see if it is a http request.  You never know there
                might be somebody directing "ftp://" requests to us.

                Then, strip off the <hostname> section and store it away.

                Finally, get rid of the front absolute URI stuff so that
                the stored path is just an absolute path.
            */
            ConvertTokenToLowerCase(pszStartOfToken, uiTokenLength);
            if (memcmp(pszStartOfToken, HTTP_STRING, sizeof(HTTP_STRING) - 1) == 0)
            {
                // We have HTTP, so set the path pointer to the <hostname>.
                pszStartOfToken += uiTokenLength + 3;
                uiTokenLength = FindTokenDelimited(pszStartOfToken, ASCII_Slash);
                memcpy(ptConnection->szHost, pszStartOfToken, uiTokenLength);
                ptConnection->bHaveHost = TRUE;
                // now set the path pointer to the <absolutepath>.
                pszStartOfToken += uiTokenLength;
                strcpy(ptConnection->szPath, pszStartOfToken);
            }
            else
            {
                /*
                    it's not HTTP, so blow up the command, and
                    let's swallow the rest of the headers.
                */
                ptConnection->tHttpCommand = HTTP_NO_COMMAND;
            }
        }
    }
    else
    {
        ptConnection->tHttpVersion = HTTP_ONE_DOT_ZERO;
    }
    
    return;
}


//#if RomPagerMimeTypeChecking
#if 0
static void ProcessAccept(rpHttpRequestPtr theRequestPtr,
        char *theStartOfTokenPtr,
        Unsigned16 theTokenLength) {
    rpDataType  theDataType;
    char        *theTokenPtr;
    char        *theEndPtr;

    /*
        There can be multiple accept statements and multiple accept arguments
        per accept statement.

        Make the Accept line a C string.

        The reason that kHttpPatternAccept includes the colon is to deal with
        bad browsers that send in "Accept:<CR><LF>" headers.  We use less code
        in dealing with this situation by having RpFindLineEnd return a length
        of 0.
    */
    theTokenLength = RpFindLineEnd(theStartOfTokenPtr);
    *(theStartOfTokenPtr + theTokenLength) = '\0';
    /*
        Look for Accept tokens.
    */
    theTokenPtr = theStartOfTokenPtr;
    theEndPtr = theStartOfTokenPtr + theTokenLength;
    while (theTokenPtr < theEndPtr) {
        theDataType = SetAcceptType(theTokenPtr);
        StoreAcceptType(theRequestPtr, theDataType);
        theTokenPtr += RpFindTokenDelimited(theTokenPtr, kAscii_Comma);
        if (theTokenPtr < theEndPtr) {
            theTokenPtr += 1;
            theTokenPtr = RpFindTokenStart(theTokenPtr);
        }
    }
    return;
}

static rpDataType SetAcceptType(char *theTokenPtr) {
    rpDataType  theDataType;
    char        *thePtr;

    theDataType = eRpDataTypeNone;
    thePtr = theTokenPtr;
    if (*thePtr == '*') {
        theDataType = eRpDataTypeAll;
    }
    else if (*thePtr == 't') {
        thePtr += 5;
        if (*thePtr == 'h') {
            theDataType = eRpDataTypeHtml;
        }
        else if (*thePtr == 'p') {
            theDataType = eRpDataTypeText;
        }
    }
    else if (*thePtr == 'i') {
        thePtr += 6;
        if (*thePtr == 'g') {
            theDataType = eRpDataTypeImageGif;
        }
        else if (*thePtr == 'p') {
            theDataType = eRpDataTypeImagePict;
        }
        else if (*thePtr == 'j') {
            theDataType = eRpDataTypeImageJpeg;
        }
        else if (*thePtr == 't') {
            theDataType = eRpDataTypeImageTiff;
        }
        else if (*thePtr == '*') {
            theDataType = eRpDataTypeAnyImage;
        }
    }
    else if (*thePtr == 'a') {
        theDataType = eRpDataTypeApplet;
    }
    return theDataType;
}

static void StoreAcceptType(rpHttpRequestPtr theRequestPtr,
        rpDataType  theDataType) {
    rpDataType  theAcceptType;

    /*
        Store the acceptable data type and create compound
        types if necessary to support multiple accept arguments.
    */
    theAcceptType = theRequestPtr->fAcceptType;
    if (theAcceptType == eRpDataTypeNone) {
        theAcceptType = theDataType;
    }
    else if (theAcceptType != eRpDataTypeAll) {
        /*
            We have assume we have a type that we didn't
            have before.  Set up the new accept type
            based on the old one plus this one.
        */
        switch (theDataType) {
            /*
                For image types, if the previous type
                was an image, set the type to any image,
                otherwise set the type to send anything.
            */
            case eRpDataTypeImageGif:
            case eRpDataTypeImagePict:
            case eRpDataTypeImageJpeg:
            case eRpDataTypeImageTiff:
            case eRpDataTypeAnyImage:
                if (theAcceptType == eRpDataTypeHtml ||
                        theAcceptType == eRpDataTypeApplet) {
                    theAcceptType = eRpDataTypeAll;
                }
                else {
                    theAcceptType = eRpDataTypeAnyImage;
                }
                break;

            /*
                For applets and specific requests of all,
                set the type to send anything.
            */
            case eRpDataTypeApplet:
            case eRpDataTypeAll:
                theAcceptType = eRpDataTypeAll;
                break;

            /*
                For other types, don't bother to set compound
                types, since we always pass HTML and plain text,
                and we might actually want to stop other types.
            */
            default:
                break;
        }
    }
    theRequestPtr->fAcceptType = theAcceptType;
    return;
}

#endif  /* RomPagerMimeTypeChecking */

/*
static void ProcessAgent(rpHttpRequestPtr theRequestPtr,
        char *theStartOfTokenPtr,
        Unsigned16 theTokenLength) {

    RpStoreHeader(theStartOfTokenPtr, theRequestPtr->fAgent, 0);
    return;
}

char * RpGetUserAgent(void *theServerDataPtr) {

    return ((rpDataPtr) theServerDataPtr)->fCurrentHttpRequestPtr->fAgent;
}
*/

/*
static void ProcessLanguage(rpHttpRequestPtr theRequestPtr,
        char *theStartOfTokenPtr,
        Unsigned16 theTokenLength) {

    RpStoreHeader(theStartOfTokenPtr, theRequestPtr->fLanguage, 0);
    return;
}

char * RpGetAcceptLanguage(void *theServerDataPtr) {

    return ((rpDataPtr) theServerDataPtr)->fCurrentHttpRequestPtr->fLanguage;
}
*/


//char * GetHostName(void *theServerDataPtr)
//{
//    return ((rpDataPtr) theServerDataPtr)->fCurrentHttpRequestPtr->fHost;
//}

/*
static void ProcessTransferEncoding(rpHttpRequestPtr theRequestPtr,
        char *theStartOfTokenPtr,
        Unsigned16 theTokenLength)
{

    *(theStartOfTokenPtr + theTokenLength) = '\0';
    RpConvertTokenToLowerCase(theStartOfTokenPtr, theTokenLength);
    if (RP_STRCMP(theStartOfTokenPtr, kHttpChunked) == 0) {
        theRequestPtr->fObjectIsChunked = True;
        theRequestPtr->fHaveRequestObject = True;
    }
    else
    {
        // we got a transfer encoding we don't understand,
        // so reject with 501 Unimplemented.
        theRequestPtr->fHttpResponseState = eRpHttpNotImplemented;
        theRequestPtr->fHttpTransactionState = eRpSendingHttpHeaders;
        // Mark the connection to close after we send the response,
        // since we don't want to read all the data attached to this request.
        theRequestPtr->fPersistent = False;
    }
    return;
}
*/





/*
void RpInitPatternTable(rpPatternTablePtr thePatternTablePtr)
{


    thePatternTablePtr += 1;
    thePatternTablePtr->fPattern       = kHttpPatternIfModified;
    thePatternTablePtr->fPatternLength = sizeof(kHttpPatternIfModified) - 1;
    thePatternTablePtr->fAction        = ProcessIfModified;

    thePatternTablePtr += 1;
    thePatternTablePtr->fPattern       = kHttpPatternContentLength;
    thePatternTablePtr->fPatternLength = sizeof(kHttpPatternContentLength) - 1;
    thePatternTablePtr->fAction        = ProcessContentLength;

    thePatternTablePtr += 1;
    thePatternTablePtr->fPattern       = kHttpPatternReferer;
    thePatternTablePtr->fPatternLength = sizeof(kHttpPatternReferer) - 1;
    thePatternTablePtr->fAction        = ProcessReferer;

#if RomPagerMimeTypeChecking
    thePatternTablePtr += 1;
    thePatternTablePtr->fPattern       = kHttpPatternAccept;
    thePatternTablePtr->fPatternLength = sizeof(kHttpPatternAccept) - 1;
    thePatternTablePtr->fAction        = ProcessAccept;
#endif

#if RomPagerCaptureUserAgent
    thePatternTablePtr += 1;
    thePatternTablePtr->fPattern       = kHttpPatternUserAgent;
    thePatternTablePtr->fPatternLength = sizeof(kHttpPatternUserAgent) - 1;
    thePatternTablePtr->fAction        = ProcessAgent;
#endif

#if RomPagerKeepAlive || RomPagerHttpOneDotOne
    thePatternTablePtr += 1;
    thePatternTablePtr->fPattern       = kHttpPatternConnection;
    thePatternTablePtr->fPatternLength = sizeof(kHttpPatternConnection) - 1;
    thePatternTablePtr->fAction        = ProcessConnection;
#endif

#if RomPagerHttpOneDotOne
    thePatternTablePtr += 1;
    thePatternTablePtr->fPattern       = kHttpPatternTransferEncoding;
    thePatternTablePtr->fPatternLength = sizeof(kHttpPatternTransferEncoding) - 1;
    thePatternTablePtr->fAction        = ProcessTransferEncoding;
#endif

#if RomPagerCaptureLanguage
    thePatternTablePtr += 1;
    thePatternTablePtr->fPattern       = kHttpPatternAcceptLanguage;
    thePatternTablePtr->fPatternLength = sizeof(kHttpPatternAcceptLanguage) - 1;
    thePatternTablePtr->fAction        = ProcessLanguage;
#endif

#if RomPagerFileUpload || RomPagerIpp || RomPagerRemoteHost || RomPagerPutMethod
    thePatternTablePtr += 1;
    thePatternTablePtr->fPattern       = kHttpPatternContentType;
    thePatternTablePtr->fPatternLength = sizeof(kHttpPatternContentType) - 1;
    thePatternTablePtr->fAction        = ProcessContentType;
#endif

#if RomPagerSecurityDigest
    thePatternTablePtr += 1;
    thePatternTablePtr->fPattern       = kHttpPatternExtension;
    thePatternTablePtr->fPatternLength = sizeof(kHttpPatternExtension) - 1;
    thePatternTablePtr->fAction        = ProcessExtension;
#endif

#if RomPagerPutMethod
    thePatternTablePtr += 1;
    thePatternTablePtr->fPattern       = kHttpPut;
    thePatternTablePtr->fPatternLength = sizeof(kHttpPut) - 1;
    thePatternTablePtr->fAction        = ProcessPut;
#endif

#if RomPagerOptionsMethod
    thePatternTablePtr += 1;
    thePatternTablePtr->fPattern       = kHttpOptions;
    thePatternTablePtr->fPatternLength = sizeof(kHttpOptions) - 1;
    thePatternTablePtr->fAction        = ProcessOptions;
#endif

#if RomPagerTraceMethod
    thePatternTablePtr += 1;
    thePatternTablePtr->fPattern       = kHttpTrace;
    thePatternTablePtr->fPatternLength = sizeof(kHttpTrace) - 1;
    thePatternTablePtr->fAction        = ProcessTrace;
#endif

#if RomPagerTLS
    thePatternTablePtr += 1;
    thePatternTablePtr->fPattern       = kHttpPatternUpdate;
    thePatternTablePtr->fPatternLength = sizeof(kHttpPatternUpdate) - 1;
    thePatternTablePtr->fAction        = ProcessUpdate;
#endif

#if RpExpectHeader
    thePatternTablePtr += 1;
    thePatternTablePtr->fPattern       = kHttpPatternExpect;
    thePatternTablePtr->fPatternLength = sizeof(kHttpPatternExpect) - 1;
    thePatternTablePtr->fAction        = ProcessExpect;
#endif

#if RpEtagHeader
    thePatternTablePtr += 1;
    thePatternTablePtr->fPattern       = kHttpPatternIfNoneMatch;
    thePatternTablePtr->fPatternLength = sizeof(kHttpPatternIfNoneMatch) - 1;
    thePatternTablePtr->fAction        = ProcessIfNoneMatch;
#endif


    thePatternTablePtr += 1;
    thePatternTablePtr->fPatternLength = 0;
    return;
}

*/


/*
#if RpEtagHeader
void RpBuildEtagString(char *theEtagStringPtr, Unsigned32 theTag) {
    Unsigned8   theByte;
    Unsigned8   theLength;
    char        *theTagPtr;

    theLength = 4;
    theTagPtr = (char *) &theTag;
    while(theLength > 0) {
        theByte = *theTagPtr++;
        *theEtagStringPtr++ = NIBBLE_TO_HEX((theByte >> 4) & 0x0f);
        *theEtagStringPtr++ = NIBBLE_TO_HEX(theByte & 0x0f);
        theLength -= 1;
    }
    return;
}


static void ProcessIfNoneMatch(rpHttpRequestPtr theRequestPtr,
        char *theStartOfTokenPtr,
        Unsigned16 theTokenLength) {
    rpDataPtr           theDataPtr;

    theDataPtr = theRequestPtr->fDataPtr;

    //    See if the tag matches our Rom tag.
    theStartOfTokenPtr +=1;
    if (RP_MEMCMP(theStartOfTokenPtr, theDataPtr->fRomEtagString, 8) == 0) {
        theRequestPtr->fClientHasObject = True;
    }
    return;
}
#endif


*/
