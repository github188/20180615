/*
 *  File:       RpStates.h
 */

#ifndef _HS_STATES_
#define _HS_STATES_


/*
    RomPager HTTP commands
*/


/*
    character hex escape states
*/
typedef enum {
    ESCAPE_LOOK_FOR_PERCENT,
    ESCAPE_GET_FIRST_HEX_BYTE,
    ESCAPE_GET_SECOND_HEX_BYTE
} THexEscapeState;


/*
    RomPager HTTP response actions
*/
typedef enum {
    HTTP_NORMAL,
    
    HTTP_BAD_COMMAND,
    
    HTTP_BAD_REQUEST,
    
    //eRpHttpForbidden,
    HTTP_NOT_IMPLEMENTED,
    
    HTTP_NEED_AUTHORIZATION,
    
    //eRpHttpNeedDigestAuthorization,
    //eRpHttpNeedCgiAuthorization,
    
    HTTP_NO_OBJECT_FOUND,
    
    HTTP_SERVICE_UNAVAILABLE,
    //eRpHttpMultipleChoices,
    
    //eRpHttpFileSystemError,
    //eRpHttpBadContentType,
    //eRpHttpExpectFailed,
    //eRpHttpRedirect,
    //eRpHttpRedirectMap,
    HTTP_NOT_MODIFIED,
    
    HTTP_REQUEST_TOO_LARGE,
    
    //eRpHttpTracing,
    //eRpHttpOptions,
    //eRpHttpIppNormal,
    //eRpHttpPutCompleted,
    //eRpHttpFormProcessed
} THttpResponseAction;


/*
    HTTP request states
*/
typedef enum {
    HTTP_STATE_CONNECTION_CLOSED,
    
    HTTP_STATE_WAITING_CONNECTION,
    
    
    HTTP_STATE_PARSING_HEADERS,
    
    HTTP_STATE_GET_OBJECT_BODY,
    
    //eRpParsingChunkedObjectBody,
    HTTP_STATE_PARSING_MULTIPART,
    HTTP_STATE_MULTIPART_OBJECT,
    //eRpParsingHtml,
    //eRpEndSecurityCheck,
    //eRpAnalyzeHttpRequest,
    HTTP_STATE_PREPARE_DATA,
    HTTP_STATE_SEND_HEADERS,
	HTTP_STATE_SEND_BODY,
    
	HTTP_STATE_DATA_PENDING,
    //eRpSendingHttpContinue,
    //eRpHandlePut,
    //eRpSendingHttpResponse,
    
   
    //eRpSendingTraceResponse,
    //eRpSendingLastDataBuffer,
    //eRpServerPushStartTimer,
    //eRpServerPushTimer,
    HTTP_STATE_RESPONSE_COMPLETE
    //eRpHttpResponseComplete2
} THttpTransactionState;


/*
    Multipart Form Data states
*/
typedef enum {
    MP_STATE_FIND_BOUNDARY,
    MP_STATE_PARSE_HEADER
//    eRpMpParsingItemValue,
//    eRpMpCreateFile,
//    eRpMpCreateFileDone,
//    eRpMpParsingData,
//    eRpMpWriteFileDone,
//    eRpMpCloseFile,
//    eRpMpCloseFileDone,
//    eRpMpBadRequest,
//    eRpMpFileSystemError,
//    eRpMpConnectionError,
//    eRpMpFlush
} TMultipartState;



/*
    HTTP PUT command states
*/
typedef enum {
    eRpHttpPutStart,
    eRpHttpPutCreateFileDone,
    eRpHttpPutFileSystemError,
    eRpHttpPutParsingData,
    eRpHttpPutWriteFileDone,
    eRpHttpPutCloseFile,
    eRpHttpPutCloseFileDone,
    eRpHttpPutConnectionError,
    eRpHttpPutFlush
} rpHttpPutState;


/*
    Password states
*/
typedef enum {
    eRpPasswordNotAuthorized,
    eRpPasswordAuthorized,
    eRpPasswordPending,
    eRpPasswordDone
} rpPasswordState;


/*
    HTTP request versions
*/
typedef enum {
    HTTP_ONE_DOT_ZERO,
    HTTP_ONE_DOT_ONE
} THttpVersion;


/*
    HTTP Object Sources
*/
//typedef enum {
//    SOURCE_FILE,
    //eRpFileUploadUrl,
    //eRpPutUrl,
//    SOURCE_MEMORY
//} TDataSource;






#endif
