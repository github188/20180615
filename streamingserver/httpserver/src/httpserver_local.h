/*  Copyright (c) 2003 Vivotek Inc. All rights reserved.
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
 *  File name          :   HTTPServer.h
 *  File description   :   HTTP Server Module API
 *  Author             :   Bill, Shu-Zhe Weng
 *  Created at         :   2003/03/11
 *  Note               :   
 */


/*!
 **********************************************************************
 * Copyright (C) 2003 Vivotek, Inc. All rights reserved.
 *
 * \file
 * HTTPServer.h
 *
 * \brief
 * HTTP Server Module API
 *
 * \date
 * 2003/03/11
 *
 * \author
 * Bill, Shu-Zhe Weng
 *
 *
 **********************************************************************
 */


#ifndef _HTTPSERVERLOCAL_H_
#define _HTTPSERVERLOCAL_H_


#include "osisolate.h"
#include "hs_states.h"
#include "hs_parse.h"
#include "hs_tcp.h"
#include "httpserver.h"
#include "common.h"
#include "encrypt_md5.h"
#include "encrypt_base64.h"


//#define DbgPrint(x)	TelnetShell_DbgPrint x; printf x;
//#define DbgPrint1(x) TelnetShell_DbgPrint x;
#define DbgPrint(x)
#define DbgPrint1(x)


// constants

#define DEFAULT_RECEIVE_BUFFER_SIZE      1460
#define DEFAULT_HTTP_PORT                80
#define DEFAULT_MAX_CONNECTION           10
#define DEFAULT_CONNECTION_TIMEOUT       60	//30
#define DEFAULT_HOSTNAME				 "Network Camera"
#define DEFAULT_LANGUAGE				 "en"

#define MAX_NAME_LENGTH          32
#define MAX_VALUE_LENGTH          256
#define MAX_LINE_LENGTH          256
#define MAX_SESSION_LENGTH       32

#define MAX_SAVE_HEADER_LENGTH    256	//128
#define HTTP_HEADER_BUFFER_SIZE     512
#define HTTP_BODY_BUFFER_SIZE     1460

#define MAX_BOUNDARY_LENGTH      70



#define HTTP_STRING             "http://"

/*
    General Headers
*/
#define STR_HTTP_CONNECTION     "Connection: "
#define STR_HTTP_DATE           "Date: "


// Response Headers
#define STR_HTTP_SERVER                 "Server:"
// modified by Yun, 2004/05/27
//ShengFu 2006/01/27 add digest authentication
#define STR_HTTP_WWW_AUTHENTICATE_BASIC       "WWW-Authenticate: Basic realm=\""
#define STR_HTTP_WWW_AUTHENTICATE_DIGEST      "WWW-Authenticate: Digest"

//#define STR_HTTP_WWW_AUTHENTICATE       "WWW-Authenticate: Digest qop=\"auth\",realm=\"testrealm\",nonce=\"dcd98b7102dd2f0e8b11d0f600bfb0c093"

//#define STR_HTTP_WWW_AUTHENTICATE       "WWW-Authenticate: Basic\x0d\x0a"
//#define KHTTPPROXYAUTHENTICATE          "Proxy-Authenticate:"
//#define KHTTPRETRYAFTER                 "Retry-After:"
//#define kHttpRefresh                    "Refresh: "
//#define STR_SERVER_HEADER               "Server: Vivotek HTTP Server\x0d\x0a"
#define STR_SERVER_HEADER                 "Server: "
//#define kHttpNoCache                    "Pragma: no-cache\x0d\x0a"
#define STR_NO_CACHE					  "Pragma: no-cache\x0d\x0a"
//#define kHttpOneOneNoCache              "Cache-Control: no-cache\x0d\x0a"
#define STR_ONEONE_NO_CACHE				  "Cache-Control: no-cache\x0d\x0a"
#define STR_ONEONE_NO_STORE				  "Cache-Control: no-store\x0d\x0a"
//#define kHttpEtag                       "Etag: \""
#define STR_STREAM_ID                   "SID : "
#define STR_AUDIO_MODE                  "Audio Mode : "
#define STR_AUDIO_NONE					"None"
#define STR_AUDIO_FULLDUPLEX            "Full Duplex"
#define STR_AUDIO_HALFDUPLEX            "Half Duplex"
#define STR_AUDIO_TALKONLY              "Talk Only"
#define STR_AUDIO_LISTENONLY            "Listen Only"

#define STR_USER_PRIVILEGE				"Privilege : "

// Http Request Method
#define HTTP_METHOD_GET     "GET "
#define HTTP_METHOD_HEAD    "HEAD "
#define HTTP_METHOD_POST    "POST "

// HTTP Pattern Headers
#define STR_HTTPPATTERNACCEPT              "accept:"
#define KHTTPPATTERNACCEPTLANGUAGE      "accept-language"

#define HTTP_PATTERN_AUTHORIZATION       "authorization"

#define KHTTPPATTERNBASIC               "basic"
#define KHTTPPATTERNCONNECTION          "connection"
#define HTTP_PATTERN_DISPOSITION  "content-disposition"

#define HTTP_PATTERN_CONTENT_LENGTH       "content-length"
#define HTTP_PATTERN_CONTENT_TYPE         "content-type"

#define KHTTPPATTERNCOOKIE              "cookie"
#define KHTTPPATTERNDATE                "date"
#define KHTTPPATTERNEXPECT              "expect"
#define KHTTPPATTERNEXPIRES             "expires"
#define KHTTPPATTERNEXTENSION           "extension"
#define HTTP_PATTERN_HOST               "host"
#define HTTP_PATTERN_IFMODIFIED         "if-modified-since"
#define KHTTPPATTERNIFNONEMATCH         "if-none-match"
#define KHTTPPATTERNLASTMODIFIED        "last-modified"
#define KHTTPPATTERNLOCATION            "location"
#define KHTTPPATTERNNOCACHE             "no-cache"
#define KHTTPPATTERNPRAGMA              "pragma"
#define KHTTPPATTERNREFERER             "referer"
#define KHTTPPATTERNREFRESH             "refresh"
#define KHTTPPATTERNSERVER              "server"
#define KHTTPPATTERNSETCOOKIE           "set-cookie"
#define KHTTPPATTERNTRANSFERENCODING    "transfer-encoding"
#define KHTTPPATTERNUPDATE              "update"
#define KHTTPPATTERNUSERAGENT           "user-agent"
#define KHTTPPATTERNAUTHENTICATE        "www-authenticate"
#define HTTP_PATTERN_SESSIONCOOKIE      "x-sessioncookie"


// Entity Headers
#define STR_HTTP_ALLOW                  "Allow:"
#define STR_HTTP_CONTENT_LENGTH         "Content-Length: "
#define STR_HTTP_CONTENT_TYPE           "Content-Type: "
#define STR_HTTP_CONTENT_DISPOSITION_ATTACHMENT	"Content-Disposition: attachment;"
//#define KHTTPCONTENTDISPOSITION         "Content-Disposition: attachment; filename=\""
//#define KHTTPCONTENTENCODINGGZIP        "Content-Encoding: gzip\x0d\x0a"
//#define KHTTPTRANSFERENCODINGCHUNKED    "Transfer-Encoding: chunked\x0d\x0a"
#define STR_HTTP_CONTENT_LANGUAGE       "Content-Language: "
#define STR_HTTP_EXPIRES                "Expires: "
#define STR_HTTP_LAST_MODIFIED          "Last-Modified: "

//#define KHTTPURIHEADER                  "URI: <"
//#define KHTTPURL                        "URL="
//#define KHTTPLOCATION                   "Location: "
//#define KHTTPOBJECTVERSION              "Version:"
//#define KHTTPDERIVEDFROM                "Derived-From:"
//#define KHTTPTITLE                      "Title:"
//#define KHTTPLINK                       "Link:"
//#define KFIELDSEPARATOR                 "; "



#define STR_HTTP_KEEP_ALIVE      "Keep-Alive"
#define STR_HTTP_CLOSE           "close"
#define STR_HTTP_BOUNDARY           "boundary"
#define STR_HTTP_FILENAME           "filename"
#define STR_HTTP_NAME               "name"


// Response Status
#define STR_HTTP_VERSION                "HTTP/1.1"		//"HTTP/1.0"	//
#define STR_HTTP_VERSION_1DOT0          "HTTP/1.0"
#define STR_100_CONTINUE                " 100 Continue\x0d\x0a"
#define STR_200_OK                      " 200 OK\x0d\x0a"
//#define kCreated                        " 201 Created\x0d\x0a"
//#define kMultipleChoices                " 300 Multiple Choices\x0d\x0a"
//#define kMoved                          " 302 Found\x0d\x0a"
//#define kSeeOther                       " 303 See Other\x0d\x0a"
#define STR_304_NOT_MODIFIED            " 304 Not Modified\x0d\x0a"
#define STR_400_BAD_REQUEST             " 400 Bad Request\x0d\x0a"
#define STR_401_UNAUTHORIZED            " 401 Unauthorized\x0d\x0a"
//#define kForbidden                      " 403 Forbidden\x0d\x0a"
#define STR_404_PAGE_NOT_FOUND          " 404 Not Found\x0d\x0a"
#define STR_405_METHOD_NOT_ALLOWED      " 405 Method Not Allowed\x0d\x0a"
//#define kNoneAcceptable                 " 406 Not Acceptable\x0d\x0a"
#define STR_413_REQUEST_TOO_LARGE       " 413 Request Entity Too Large\x0d\x0a"
//#define kBadMediaType                   " 415 Unsupported Media Type\x0d\x0a"
//#define kExpectFailed                   " 417 Expectation Failed\x0d\x0a"
#define STR_500_SERVER_ERROR            " 500 Internal Server Error\x0d\x0a"
#define STR_501_NOT_IMPLEMENTED         " 501 Not Implemented\x0d\x0a"
#define STR_503_SERVICE_UNAVAILABLE     " 503 Service Unavailable\x0d\x0a"
//#define kCommaSpace                     ", "
//#define kSpaceGMT                       " GMT"
//#define kTwoDashes                      "--"
//#define kHttp10_OK                      "HTTP/1.0 200 OK"
//#define kHttp11_OK                      "HTTP/1.1 200 OK"

#define K_CRLF               "\x0d\x0a"
#define K_QUOTE_CRLF         "\"\x0d\x0a"
#define K_QUOTE              "\""
#define K_SPACE              " "
#define K_QUESTION           "?"
#define K_COMMA              ","
#define K_PERIOD             "."
#define K_COLON              ":"
#define K_EQUAL              "="
#define K_AMPERSAND          "&"
#define K_OPENANGLE          "<"
#define K_CLOSEANGLE         ">"
#define K_EMPTYLENGTH        "\x30\x0d\x0a"


#define MAX_MIME_TYPE_LENGTH              50

//#define kTypeApplet                     "application/octet-stream"
#define STR_TYPE_HTML                     "text/html"
#define STR_TYPE_TEXT                     "text/plain"
//#define kTypeXmlObject                  "text/xml"
//#define kTypeCssObject                  "text/css"
#define STR_TYPE_GIFIMAGE                 "image/gif"
#define STR_TYPE_JPEGIMAGE                "image/jpeg"
//#define kTypePictImage                  "image/pict"
//#define kTypePictImageAlt               "image/x-pict"
//#define kTypePictImageAlt2              "image/x-macpict"
//#define kTypePngImage                   "image/png"
//#define kTypePngImageAlt                "image/x-png"
//#define kTypeTiffImage                  "image/tiff"
//#define kTypeHttpTrace                  "message/http"
//#define kTypeServerPush                 "multipart/x-mixed-replace;boundary="
//#define kTypeNormalForm                 "application/x-www-form-urlencoded"
#define STR_TYPE_MULTIPARTFORM            "multipart/form-data"
//#define kTypeIppObject                  "application/ipp"
//#define kTypeWavObject                  "audio/wav"
// added by Jason, 2004/12/30
//#define STR_TYPE_TUNNELLED                "application/x-vvtk-tunnelled"
//ShengFu HTTP over RTSP
#define STR_TYPE_TUNNELLED                "application/x-rtsp-tunnelled"
#define STR_TYPE_APPLICATION_OCTETSTREAM  "application/octet-stream"


#define ASCII_Null         0x00
#define ASCII_0            0x30
#define ASCII_9            0x39
#define ASCII_A            0x41
#define ASCII_F            0x46
#define ASCII_Z            0x5A
#define ASCII_a            0x61
#define ASCII_f            0x66
#define ASCII_z            0x7A

#define ASCII_Return       0x0D
#define ASCII_Newline      0x0A
#define ASCII_Space        0x20
#define ASCII_Quote        0x22
#define ASCII_Hash         0x23
#define ASCII_Percent      0x25
#define ASCII_Ampersand    0x26
#define ASCII_Plus         0x2B
#define ASCII_Comma        0x2C
#define ASCII_Hyphen       0x2D
#define ASCII_Dot          0x2E
#define ASCII_Slash        0x2F
#define ASCII_Colon        0x3A
#define ASCII_SemiColon    0x3B
#define ASCII_LeftArrow    0x3C
#define ASCII_OpenAngle    0x3C
#define ASCII_Equal        0x3D
#define ASCII_RightArrow   0x3E
#define ASCII_CloseAngle   0x3E
#define ASCII_Question     0x3F
#define ASCII_Backslash    0x5C

#define DATA_SOURCE_INTERNAL 99

// Http Request structure
typedef struct
{
    HANDLE         hServer;
    UINT           uiConnectionIndex;
    //struct rpData *         fDataPtr;
    //Boolean                 fInUse;
    //char                    fHtmlResponseBufferOne[kHtmlMemorySize];
    
    CHAR               ResponseHeaderBuffer[HTTP_HEADER_BUFFER_SIZE]; // store header
    CHAR               ResponseBodyBuffer[HTTP_BODY_BUFFER_SIZE];
    DWORD              dwResponseBodyLength;
    
    // Data Object information
    THttpDataSource           tDataSourceType;
    DWORD                     dwDataSourceLength;
    char*                     pDataSourceBuffer;
    
    //char                    fHttpTempLineBuffer[kMaxParseLength];
//#if RomPagerLight
    //char                    fInternalResponseBuffer[256];
//#endif
    //Boolean                 fHtmlBufferReady;
    //Signed8                 fHtmlCurrentBuffer;
    //char *                  fHtmlFillPtr;
    //char *                  fHtmlResponsePtr;
    //Unsigned16              fFillBufferAvailable;
    //Unsigned16              fHtmlResponseLength;

    DWORD                   dwAbortTimer;    

//#if RomPagerServer || RomPagerLight
//#if RomPagerFileSystem
    //SfsFileInfoPtr          fFileInfoPtr;
//#endif
//#if RomPagerFileSystem || RomPagerIpp || RomPagerUserExit
    //rpObjectDescription     fLocalObject;
//#endif
//#if RomPagerUserExit
    //rpCgi                   fCgi;
//#endif

    TParsingControl     tParsingCtrl;       // Parsing control structure
    
    
    DWORD                dwPostRequestLength;
    //Unsigned32              fBrowserDate;
    //rpObjectDescriptionPtr  fObjectPtr;
    //rpObjectDescriptionPtr  fCurrentFormDescriptionPtr;
    //rpObjectDescriptionPtr  fRefreshObjectPtr;
    //char *                  fUserErrorPtr;
    //rpDataType              fAcceptType;
    
    
    
    THttpRequestCommand       tHttpCommand;       // Http method
    THttpResponseAction       tHttpResponseAct;   // The respones action
    THttpTransactionState     tHttpState;         // Http state
    TTcpStatus                tTcpState;          // Tcp send and receive state
    
    //rpHtmlState             fItemState;
    
    
    
    //Unsigned16              fRefreshSeconds;
//#if RomPagerQueryIndex || RomPagerImageMapping
    //Signed16                fIndexValues[kRpIndexQueryDepth];
//#endif
//#if RomPagerCaptureUserAgent
    //char                    fAgent[kMaxSaveHeaderLength];
//#endif
//#if RomPagerCaptureLanguage
  //  char                    fLanguage[kMaxSaveHeaderLength];
//#endif
    char                    szHost[MAX_SAVE_HEADER_LENGTH];
    //char                    fIfModified[kMaxSaveHeaderLength];
    
    char                    szPath[MAX_SAVE_HEADER_LENGTH];    // request URI
    char                    szUsername[MAX_NAME_LENGTH];   // User name
    char                    szPassword[MAX_NAME_LENGTH];   // Password
    
    
    
//#if RomPagerExternalPassword
//    rpAccess                fChallengedRealms;
//#endif
    char                    szHttpContentType[MAX_MIME_TYPE_LENGTH];
    // Added by Jason 2004/12/30, for passing proxy server
    char                    szSessionCookie[MAX_SESSION_LENGTH];
//#if RomPagerServerPush || RomMailer
    //char                    fSeparator[kMaxNameLength];
//#endif
//#if RomPagerServer
    //rpItemError             fItemError;
    //rpItemPtr               fItemPtr;
    //rpItemPtr               fNextItemPtr;
    //rpProcessCloseFuncPtr   fProcessCloseFuncPtr;   /* Pointer to optional close handler */
    //Signed32                fSerial;
    //rpNesting               fNestedItems[kRpIndexQueryDepth + 3];
    //char                    fErrorPath[kMaxPathLength + 1];
    //char                    fSubmitButtonValue[kMaxStringLength];
    //char                    fCurrentItemName[kMaxNameLength];
    //char **                 fUserPhrases;
    //Boolean                 fUserPhrasesCanBeCompressed;
//#if RpHtmlTextAreaBuf
    //char                    fCurrentItemValue[kHttpWorkSize];
//#else
    //char                    fCurrentItemValue[kMaxValueLength];
//#endif
//#if RomPagerUrlState
    //char                    fUrlState[kMaxStringLength];
//#endif
//#if RomPagerImageMapping
    //Signed16                fMapHorizontal;
    //Signed16                fMapVertical;
//#endif
//#if RomPagerSecurity
    //rpRealmPtr              fRealmPtr;
//#endif
//#if RomPagerUserDataAreas
    //char                    fUserDataArea[kRpUserDataSize];
//#endif
//#if RomPagerServerPush
    //Boolean                 fServerPushActive;
    //rpObjectDescriptionPtr  fServerPushObjectPtr;
    //Unsigned16              fServerPushSeconds;
//#endif
//#if RpRemoteHostMulti || RpUserExitMulti
    //Unsigned16              fRemoteHostIndex;
//#endif
//#if RpHtmlSelectVariable
    //Unsigned8               fItemNumber;
//#endif


//#if RomPagerFileUpload
    TMultipartState        tMultipartState;
    DWORD                     dwBoundaryLength;
    //Signed32                fMultipartRemainderLength;
    char                    szFilename[MAX_VALUE_LENGTH];
    char                    szItemName[MAX_NAME_LENGTH];
    //char                    fItemValue[kMaxValueLength];
    char                    szBoundary[MAX_BOUNDARY_LENGTH];
    //char                    fMultipartRemainderBuffer[kMaxBoundaryLength + 8];
    //char                    fMultipartWorkBuffer[kStcpReceiveBufferSize + kMaxBoundaryLength];
    //Boolean                 fFileDone;
    //Boolean                 fFinalBoundary;
//#endif


//#endif  /* RomPagerServer */
    //Signed8                 fNestedDepth;
    //Signed8                 fIndexDepth;
//#if RomPagerHttpOneDotOne
    //Signed32                fChunkLength;
    //Signed32                fChunkedContentLength;
    //Signed32                fChunkLengthProcessed;
    //Signed32                fCurrentChunkBufferLength;
    //char *                  fCurrentChunkBufferPtr;
    //rpHttpChunkState        fChunkState;
    THttpVersion           tHttpVersion;         // The request HTTP version
    //Boolean                 fHaveChunk;
    //Boolean                 fObjectIsChunked;
    //Boolean                 fResponseIsChunked;
    BOOL                 bHaveHost;         // Got the host from header
//#endif
    BOOL                 bHaveRequestObject;  // For post data or file upload
    
    BOOL                 bHaveRequestLine;   // Got the request line
    
    
    BOOL                 bKeepAlive;
    BOOL                 bPersistent;
    
    BOOL                 bStreamSocket;
    BOOL                 bStreamSocketClose;
    DWORD                dwStreamID;  
    int                  iAudioMode;  
    DWORD				 dwPrivilege;		// add by Yun, 2003/12/25
    DWORD				 dwLastModified;	// add by Yun, 2004/02/27
    DWORD				 dwIfModifiedSince; // add by Yun, 2004/02/27
    DWORD				 dwOrgLength;		// add by Yun, 2004/11/26
    BOOL				 bForceReloadFile;	// add by Yun, 2004/11/24
  
    THttpDataType        tMimeDataType;
//#if RomPagerDynamicRequestBlocks
    //Boolean                 fDynamicallyAllocated;
//#endif
//#if RomPagerSecurity
  //  rpPasswordState         fPasswordState;
//#endif
//#if RomPagerRemoteHost
    //char                    fOtherMimeType[kMaxMimeTypeLength];
    //char *                  fRemoteDateStringPtr;
//#if RomPagerSecurity
    //char                    fEncodedUserPassword[kMaxNameLength * 3];
//#endif
//#endif
//#if RomPagerPutMethod
    //char *                  fPutBufferPtr;
    //Signed32                fPutBufferLength;
    //rpHttpPutState          fHttpPutState;
//#endif
//#if RpFileInsertItem
    //char *                  fFileNamePtr;
//#endif
//#endif  /* RomPagerServer || RomPagerLight */

} THttpConnection, *PTHttpConnection;


typedef struct
{
	DWORD               dwReceiveBufferSize;
	CHAR                acServerIPAddress[MAX_IPADDR_LEN];
	USHORT              usPort;
	ULONG               ulMaxConnections;
	
	//BOOL bKeepAlive;
	//BOOL bAuthorization;
	//ULONG ulConnectionTimeout;
	DWORD               dwConnectionTimeout;
	DWORD               dwLastTime;
	
    PTHttpConnection    ptHttpConnections;                   // Array of connection data
	HTTPServer_Callback pfnHttpCallback;                     // Callback function
	
	int                 iThreadPriority;                     // Http thread priority
	HANDLE              hThreadObj;                          // Http thread handle
	HANDLE              hRunEvent;                           // Http start/stop control event
	HANDLE              hTerminateEvent;                     // Http task termination event
	DWORD               dwContext;                           // Thread instance
	char				szHostName[MAX_HOST_LEN+1];			 // host name, added by Yun, 2004/05/27
	char				szLanguage[16];						 // language type, added by Yun, 2004/08/18

	//Shengfu 2006/01/16 add digest authentication
	int					iAuthenticateType;
	char				acNonce[33];

} THttpServerInfo, *PTHttpServerInfo;

// HTTP response pattern Structure
typedef void PatternProcedure(PTHttpConnection ptConnection, char *pszStartOfToken,
                              UINT uiTokenLength);

typedef struct
{
    PatternProcedure *  fAction;
    char*               pszPattern;
    UINT                uiPatternLength;
} TPatternTable, *PTPatternTable;


extern const char *gMimeTypes[];
extern TPatternTable  gtPatternTable[];
extern TPatternTable  gtMpPatternTable[];
extern TPatternTable  gtDispPatternTable[];

// hs_common function
extern void StrLenCpy(char *pszTo, char *pszFrom, DWORD dwLength);
extern void StrLenCpyTruncate(char *pszTo, char *pszFrom, DWORD dwLength);
extern void EscapeDecodeString(char* pszEncodedString, DWORD dwEncodeLen,
                 char* pszDecodedString, DWORD* pdwDecodeLen, BOOL bFormData);

// hs_main functions
extern DWORD THREADAPI HttpServerMainTask(DWORD dwInstance);
extern SCODE InitHttpConnections(PTHttpServerInfo pServerInfo);

// hs_base64 function
extern DWORD DecodeBase64Data(char *pszInputBuf, UINT uiInputLen, char *pszOutputBuf);

// hs_header function
extern void HandleHttpHeader(PTPatternTable ptPatternTable, PTHttpConnection ptConnection);

// hs_request function
extern void InitRequestStates(PTHttpConnection ptConnection);
extern BOOL ParseHttpHeaders(PTHttpConnection ptConnection);
extern void PrepareHttpData(PTHttpConnection ptConnection);
extern BOOL GetObjectData(PTHttpConnection ptConnection);
extern BOOL ParseMultipartHeaders(PTHttpConnection ptConnection);
extern BOOL GetMultipartObjectData(PTHttpConnection ptConnection);

// hs_response function
extern SCODE HttpResponseHeader(PTHttpConnection ptConnection);
extern SCODE HttpResponseBody(PTHttpConnection ptConnection);

#endif
