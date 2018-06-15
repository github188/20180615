/*  Copyright (c) 2003 Vivotek Inc. All rights reserved.
 *
 *  $Header: /RD_1/Project/PNX1300_PSOS/Farseer/network/netap/httpserver/src/httpserver.h 3     06/01/23 4:13p Shengfu $
 *
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
 *
 * $History: httpserver.h $
 * 
 * *****************  Version 3  *****************
 * User: Shengfu      Date: 06/01/23   Time: 4:13p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/httpserver/src
 * 
 * *****************  Version 9  *****************
 * User: Yun          Date: 05/07/11   Time: 4:13p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/httpserver/src
 * Add DATA_TYPE_APPLICATION_OCTETSTREAM
 * 
 * *****************  Version 8  *****************
 * User: Yun          Date: 05/01/05   Time: 1:47p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/httpserver/src
 * Modify the length of hostname to MAX_HOST_LEN
 * 
 * *****************  Version 7  *****************
 * User: Jason        Date: 04/12/30   Time: 6:41p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/httpserver/src
 * 
 * *****************  Version 6  *****************
 * User: Yun          Date: 04/11/26   Time: 4:39p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/httpserver/src
 * Add original length field
 * 
 * *****************  Version 5  *****************
 * User: Yun          Date: 04/11/24   Time: 2:10p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/httpserver/src
 * Add forcing reloading file flag
 * 
 * *****************  Version 4  *****************
 * User: Yun          Date: 04/08/18   Time: 8:04p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/httpserver/src
 * Set content language during initialization
 * 
 * *****************  Version 3  *****************
 * User: Yun          Date: 04/07/09   Time: 6:44p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/httpserver/src
 * Add a callback type to get server name
 * (HTTPServer_Callback_GetServerName)
 * 
 * *****************  Version 12  *****************
 * User: Yun          Date: 04/03/02   Time: 9:45a
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/network/netap/httpserver/src
 * 1. Add LastModified and IfModifiedSince two fields
 * 2. Add 304 Not modified response
 * 
 * *****************  Version 11  *****************
 * User: Yun          Date: 03/12/25   Time: 4:04p
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/network/netap/httpserver/src
 * Add privilege field
 * 
 * *****************  Version 10  *****************
 * User: Joe          Date: 03/12/11   Time: 3:37p
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/network/netap/httpserver/src
 * Add #include "common.h"
 * 
 * *****************  Version 9  *****************
 * User: Joe          Date: 03/10/23   Time: 2:56p
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/network/netap/httpserver/src
 * Add keep alive callback
 * 
 * *****************  Version 8  *****************
 * User: Jason        Date: 03/09/10   Time: 2:14p
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/network/netap/httpserver/src
 * Modify connection timeout control
 * 
 * *****************  Version 7  *****************
 * User: Jason        Date: 03/08/22   Time: 3:43p
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/network/netap/httpserver/src
 * Modify file upload mechenism
 * 
 * *****************  Version 6  *****************
 * User: Joe          Date: 03/08/18   Time: 11:24a
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/network/netap/httpserver/src
 * Using definitions in netdef.h
 *
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


#ifndef _HTTPSERVER_H_
#define _HTTPSERVER_H_

#include "netdef.h"
#include "vssdef.h"
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif


// constants
#define HTTPSERVER_VERSION MAKEFOURCC( 1, 0, 0, 0 )

// Modified by Joe 2003/08/18, using definitions in netdef.h
//#define MAX_IPADDR_LEN 16
#define MAX_USERNAME_LEN        (MAX_NAME_LEN + 1)
#define MAX_PASSWORD_LEN        (MAX_PASS_LEN + 1)
#define MAX_HTTPMETHOD_LEN      8
#define MAX_HTTPVERSION_LEN     6   
#define MAX_URL_LEN             256
#define MAX_QUERY_LEN           MAX_EXTRAINFO_LEN
#define MAX_FILENAME_LEN        (MAX_PATH_LEN + 1)

// error codes
#define ERR_HTTPSERVER_VERSION  0x80010001
#define ERR_HTTPSERVER_MALLOC   0x80010002
// success codes
#define S_HTTPSERVER_SEND_COMPLETE  0x00010001
#define S_HTTPSERVER_SEND_PENDING   0x00010002 

SCODE HTTPServer_GetVersion(BYTE *byMajor, BYTE *byMinor, BYTE *byBuild, BYTE *byRevision);


// flags to select valid field(s) of structures THTTPServer_InitialSettings, 
// THTTPServer_Settings and THTTPServer_ClientSettings

/*! initial settings flag to set receive buffer size */
#define HTTPServer_ReceiveBufferSizeFlag 0x001
/*! initial settings flag to set local IP address */
#define HTTPServer_ServerIPAddressFlag 0x002
/*! initial settings flag to set local port */
#define HTTPServer_PortFlag 0x004
/*! initial settings flag to set maximum number of clients */
#define HTTPServer_MaxConnectionsFlag 0x008
/*! settings flag to set the service thread's priority */
#define HTTPServer_ThreadPriorityFlag 0x010
/*! settings flag to set stack size of service thread */
#define HTTPServer_StackSizeFlag	0x020
/*! settings flag to select connection keep-alive or close */
#define HTTPServer_KeepAliveFlag 0x040
/*! settings flag to select authorization or not */
#define HTTPServer_AuthorizationFlag		0x080
/*! settings flag to set connection timeout value */
#define HTTPServer_ConnectionTimeoutFlag 0x100
/*! settings flag to set host name */
#define HTTPServer_HostNameFlag 0x200
/*! settings flag to set language type */
#define HTTPServer_LanguageFlag 0x400

//ShengFu 2006/01/16 add digest authentication
#define HTTPServer_AuthorizationType_Basic		0x001
#define HTTPServer_AuthorizationType_Digest		0x002


/*! Initial settings data structure for \b HTTPServer_Initial */
typedef struct
{
    /*! set this value to \b HTTPServer_VERSION */
	DWORD dwVersion;

    /*! combinations of settings flags to choose 
	    which field is valid in the structure */
	DWORD dwInitSettingsFlag;

	/*! set each receive buffer's size, default is 1460 bytes (Ethernet MTU) */
	DWORD dwReceiveBufferSize;

	/*! set server IP address in dotted decimal notation 
		(default: bind with INADDR_ANY) */
	CHAR acServerIPAddress[MAX_IPADDR_LEN];

	/*! set local port (in host byte order), default is 80 */
	USHORT usPort;

	/*! set maximum number of connections concurrently, default is 10 */
	ULONG ulMaxConnections;

	/*! set the service thread's stack size */
	DWORD dwStackSize;
	/*! set the service thread's priority, 
		default: THREAD_PRIORITY_NORMAL under Windows, 150 under pSOS */
	int iThreadPriority;

	/*! connection keep-alive or close selection
	(0 for close, 1 for keep-alive), default is 1 */
	BOOL bKeepAlive;

	/*! authorization or not selection
	(0: No, 1: Yes), default is 0 */
	int iAuthorizationType;

	/*! set connection timeout value in millisecond(s), 
		default is 900000 (15 minutes) */
	DWORD dwConnectionTimeout;
	
	/*! the host name, show in server header and realm, the maximum length is MAX_HOST_LEN */
	char *pcHostName;
	
	/*! the language type of webpage, default is en, the maximum length is 15 */
	char *pcLanguage;

} THTTPServer_InitSettings;

/*!
 *******************************************************************************
 * \brief
 * Initialize a HTTPServer instance
 *
 * \param phHTTPServerObject
 * \a (i/o) handle pointer to receive the created instance
 * \param pstInitialSettings
 * \a (i) pointer to a \b THTTPServer_InitialSettings structure variable storing 
 *        initial settings to create the HTTPServer instance
 *
 * \retval S_OK
 * HTTPServer instance created successfully.
 * \retval others
 * HTTPServer instance creation failed. check error codes.
 *
 * \see HTTPServer_Release
 *
 **************************************************************************** */
SCODE HTTPServer_Initial(HANDLE *phHTTPServerObj, THTTPServer_InitSettings *pstInitSettings);


/*! Settings data structure for \b HTTPServer_SetParameters */
typedef struct
{
    /*! combinations of settings flags to choose 
	    which field is valid in the structure */
	DWORD dwSettingsFlag;

	/*! set the service thread's priority, 
		default: unchanged */
	int iThreadPriority;

	/*! connection keep-alive or close selection
	(0 for close, 1 for keep-alive), default: unchanged */
	BOOL bKeepAlive;

	/*! authorization or not selection
	(0: No, 1: Yes), default: unchanged */
	int iAuthorizationType;

	/*! set connection timeout value in millisecond(s), 
		default: unchanged */
	DWORD dwConnectionTimeout;

} THTTPServer_Settings;

/*!
 *******************************************************************************
 * \brief
 * Set a HTTPServer instance's parameters
 *
 * \param hHTTPServerObject
 * \a (i) the HTTPServer instance's handle to set parameters
 * \param pstSettings
 * \a (i) pointer to a \b THTTPServer_Settings structure variable storing 
 *        settings for the HTTPServer instance
 *
 * \retval S_OK
 * parameters set successfully.
 * \retval others
 * set parameters failed. check error codes.
 *
 **************************************************************************** */
SCODE HTTPServer_SetParameters(HANDLE hHTTPServerObj, THTTPServer_Settings *pstSettings);


/*!
 *******************************************************************************
 * \brief
 * Start a HTTPServer instance's service
 *
 * \param hHTTPServerObject
 * \a (i) the HTTPServer instance's handle
 *
 * \retval S_OK
 * HTTPServer instance starts successfully.
 * \retval others
 * start HTTPServer instance failed. check error codes.
 *
 * \see HTTPServer_Stop
 *
 **************************************************************************** */
SCODE HTTPServer_Start(HANDLE hHTTPServerObj);


/*!
 *******************************************************************************
 * \brief
 * Stop a HTTPServer instance
 *
 * \param hHTTPServerObject
 * \a (i) the HTTPServer instance's handle
 *
 * \retval S_OK
 * instance stopped successfully.
 * \retval others
 * instance stop failed. check error codes.
 *
 * \see HTTPServer_Start
 *
 **************************************************************************** */
SCODE HTTPServer_Stop(HANDLE hHTTPServerObj);


/*!
 *******************************************************************************
 * \brief
 * Release a HTTPServer instance
 *
 * \param phHTTPServerObject
 * \a (i) handle pointer pointing to a HTTPServer instance to release
 *
 * \retval S_OK
 * HTTPServer instance released successfully.
 * \retval others
 * release HTTPServer instance failed. check error codes.
 *
 * \see HTTPServer_Initial
 *
 **************************************************************************** */
SCODE HTTPServer_Release(HANDLE *phHTTPServerObj);


// callback type definitions

#define HTTPServer_Callback_Accept               1
#define HTTPServer_Callback_Authorize            2
#define HTTPServer_Callback_Request              3
#define HTTPServer_Callback_Disconnect           4
#define HTTPServer_Callback_Send                 5
#define HTTPServer_Callback_Multipart_Head       6
#define HTTPServer_Callback_Multipart_Data       7
#define HTTPServer_Callback_Multipart_Request    8
// Add by Joe 2003/10/22, to monitor whether the HTTP server is still alive
#define HTTPServer_Callback_Alive                9
// Add by Yun 2004/07/09, to get the current server name
#define HTTPServer_Callback_GetServerName		 10
// ShengFu 2006/01/16 add digest authentication
#define HTTPServer_Callback_Digest_Auth_Request	 11

/*!
 *******************************************************************************
 * \brief
 * callback function definiton
 *
 * \param dwInstance
 * \a (i) parent instance
 * \param dwCallbackType
 * \a (i) callback type, includes
 *
 *  1) HTTPServer_Callback_Accept: after a new connection is accepted
 *
 *  2) HTTPServer_Callback_Authorize: after authorization info received, return
 *     0 if user info is accepted, return other values if access denied.
 *
 *  3) HTTPServer_Callback_Request: after a HTTP request received. Note: when 
 *     callback on request, an internal buffer will be called back, after 
 *     callback returns, module will assume the buffer is returned to module.
 *     So save all information or process them before return from callback. To
 *     send response, assign the send buffer pointer in field
 *     pcSendBuffer and set buffer size in field dwBufLen before return from 
 *     callback. After send done, the send buffer pointer will be 
 *     called back on type HTTPServer_Callback_Send, you can reuse or do anything
 *     to that buffer then.
 *
 *  4) HTTPServer_Callback_Disconnect: after a connection is disconnected
 *
 *  5) HTTPServer_Callback_Send: after a buffer is sent to a client
 *
 * \param pvCallbackData
 * \a (i) pointer to a block of data, each callback type has a specific callback 
 * data structure. Cast this pointer to appropriate structure pointer according 
 * to parameter dwCallbackType.
 *
 **************************************************************************** */
typedef int (*HTTPServer_Callback)(DWORD dwInstance, DWORD dwCallbackType, void* pvCallbackData);

/*!
 *******************************************************************************
 * \brief
 * set a HTTPServer instance's callback function
 *
 * \param hHTTPServerObject
 * \a (i) handle of the HTTPServer instance to set callback
 * \param pfnCallback
 * \a (i) the callback function
 * \param dwInstance
 * \a (i) the parent instance
 *
 * \retval S_OK
 * callback function set successfully.
 * \retval others
 * set callback function failed. check error codes.
 *
 **************************************************************************** */
SCODE HTTPServer_SetCallback(HANDLE hHTTPServerObj, HTTPServer_Callback pfnCallback, DWORD dwInstance);

/*! callback on accept data structure */
typedef struct
{
	DWORD dwClientID;
	DWORD dwClientIPAddress;
	USHORT usClientPort;
	DWORD dwErrorCode;
} THTTPServer_AcceptData;

typedef enum {
    HTTP_NO_COMMAND,
    HTTP_GET_COMMAND,
    HTTP_HEAD_COMMAND,
    HTTP_POST_COMMAND,
    HTTP_OPTIONS_COMMAND,
    HTTP_PUT_COMMAND,
    HTTP_DELETE_COMMAND,
    HTTP_TRACE_COMMAND
} THttpRequestCommand;


typedef enum {
    HTTP_URL_OK,            /*  Cgi returns HTTP 200 Ok                   */
    HTTP_URL_OK_STATIC,     /*  Cgi returns HTTP 200 Ok - Static Object   */
    HTTP_URL_REDIRECT,      /*  Cgi returns HTTP 302 Moved Temp           */
    HTTP_URL_NOT_MODIFIED,  /*  Cgi returns HTTP 304 Not Modified         */
    HTTP_URL_UNAUTHORIZED,  /*  Cgi returns HTTP 401 Unauthorized         */
    HTTP_URL_NOT_FOUND,     /*  Cgi returns HTTP 404 Not Found            */
    HTTP_URL_UNAVAILABLE,   /*  Cgi returns HTTP 503 Service Unavailable  */
    // added by Jason, 2004/12/30
    HTTP_URL_NOT_RESPONSE   /*  Tell http server do not response          */
} THttpUrlResponse;

typedef enum {
    DATA_TYPE_TEXT,
    DATA_TYPE_HTML,
    DATA_TYPE_IMAGE_GIF,
    DATA_TYPE_IMAGE_JPEG,
    // added by Jason, 2004/12/30
    DATA_TYPE_TUNNELLED,
    // added by Yun, 2005/07/11
    DATA_TYPE_APPLICATION_OCTETSTREAM
} THttpDataType;

typedef enum {
    DATA_SOURCE_MEMORY,
    DATA_SOURCE_FILE,
    DATA_SOURCE_PENDING
} THttpDataSource;


typedef struct
{
    //-- Basic Request information from client
	DWORD dwClientID;                    // Client Connection ID
	THttpRequestCommand tHttpMethod;     // Request method, GET, POST
	PCHAR pszURL;                        // Request URL
	PCHAR pszUserName;                   // User name 
	PCHAR pszPassword;                   // Password
	DWORD dwIfModifiedSince;			 // If-Modified-Since, add by Yun, 2004/02/27
	DWORD dwOriginalLength;				 // length field in If-Modified-Since, add by Yun, 2004/11/26
	PCHAR pszSessionCookie;              // Session cookie ID, Added by Jason, 2004/12/30

	//-- The response for the URL (fill by application on callback Authorize)
	THttpUrlResponse tResponseStatus;    // Authorize result
	DWORD dwStreamID;                    // The streamming connection ID
	int   iAudioMode;                    // The streamming server audio mode
	DWORD dwPrivilege;					 // The privilege of this user

    //-- The multipart header info
	PCHAR pszContentName;                // The content name
	PCHAR pszUpFileName;                 // The upload file name
	
    //-- The get query or post data or multipart data
	PCHAR pszObjectBuffer;               // The data buffer pointer
    DWORD dwObjectLength;                // The data length
    BOOL bObjectComplete;                // Indicate the end of the data

	//-- The response data (fill by application on callback request)
	THttpDataType tDataType;             // The data type, text/image/html
    THttpDataSource tDataSource;         // Data source, file/memory
	CHAR szFileName[MAX_FILENAME_LEN];   // The file name if data source is file
	PCHAR pSendBuffer;                   // The data buffer if data source is memory
	DWORD dwSendBufferLength;            // The length of data buffer
	DWORD dwLastModified;				 // the time of file last modified, add by Yun, 2004/02/27
	BOOL  bForceReloadFile;				 // the flag indicating to force reloading file, add by Yun, 2004/11/24
	
	//-- Error code
	DWORD dwErrorCode;                   // Reserved

	//ShengFu 2006/01/18 add digest authentication
	int	iAuthenticateType;				//call back authenticate type
} THTTPServer_RequestData;


//ShengFu 1006/01/18 add digest authentication
typedef struct
{
	CHAR pszUserName[MAX_USERNAME_LEN];                   // User name 
	CHAR pszPassword[MAX_PASSWORD_LEN];                   // Password
}THTTPServer_Author_Info;


/*! callback on disconnect data structure */
typedef struct
{
	/*! client ID */
	DWORD dwClientID;

	/*! indicate if the disconnect is caused by timed out */
	BOOL bTimedOut;

	/*! error code, reserved */
	DWORD dwErrorCode;

} THTTPServer_Disconnect;

typedef struct
{
	DWORD dwClientID;
	PCHAR pcSendBuffer;
	DWORD dwSendBufferLength;
	DWORD dwErrorCode;
} THTTPServer_Send;

/*!
 *******************************************************************************
 * \brief
 * Disconnect a HTTPServer instance's client
 *
 * \param hHTTPServerObject
 * \a (i) the HTTPServer instance's handle
 * \param dwClientID
 * \a (i) the client id
 *
 * \retval S_OK
 * disconnect request queued successfully.
 * \retval others
 * queue disconnect request failed. check error codes.
 *
 * \note
 * It is a non-blocking function.
 *
 **************************************************************************** */
SCODE HTTPServer_Disconnect(HANDLE hHTTPServerObj, DWORD dwClientID);

SCODE HTTPServer_SendData(HANDLE hHTTPServerObj, DWORD dwClientID, PCHAR pSendBuffer,
                          DWORD dwSendLength, BOOL bLastData, DWORD* pdwByteSent);

/*! Client settings data structure for \b HTTPServer_SetClientParameters */
typedef struct
{
    /*! combinations of settings flags to choose 
	    which field is valid in the structure */
	DWORD dwClientSettingsFlag;

	/*! connection keep-alive or close selection
	(0 for close, 1 for keep-alive), default: unchanged */
	BOOL bKeepAlive;

	/*! authorization or not selection
	(0: No, 1: Yes), default: unchanged */
	int iAuthorizationType;

	/*! set connection timeout value in millisecond(s), 
		default: unchanged */
	ULONG ulConnectionTimeout;

} THTTPServer_ClientSettings;

/*!
 *******************************************************************************
 * \brief
 * Set a HTTPServer instance's individual client's parameters
 *
 * \param hHTTPServerObject
 * \a (i) the HTTPServer instance's handle
 * \param dwClientID
 * \a (i) the client id
 * \param pstClientSettings
 * \a (i) pointer to a \b THTTPServer_ClientSettings structure variable storing 
 *        individual client settings
 *
 * \retval S_OK
 * client parameters set successfully.
 * \retval others
 * set client parameters failed. check error codes.
 *
 **************************************************************************** */
//SCODE HTTPServer_SetClientParameters( HANDLE hHTTPServerObject, DWORD dwClientID, THTTPServer_ClientSettings *pstClientSettings );


/*!
 *******************************************************************************
 * \brief
 * Get a HTTPServer instance's client's socket descriptor and take the client out
 *
 * \param hHTTPServerObject
 * \a (i) the HTTPServer instance's handle
 * \param dwClientID
 * \a (i) the client id
 * \param psSocketID
 * \a (i/o) pointer to a SOCKET to receive the returned socket descriptor
 *
 * \retval S_OK
 * function done successfully.
 * \retval others
 * function failed. check error codes.
 *
 **************************************************************************** */
SCODE HTTPServer_TakeClientOut(HANDLE hHTTPServerObj, DWORD dwClientID, SOCKET *psSocket);


/*!
 *******************************************************************************
 * \brief
 * compare two HTTP accepted time strings (utility function)
 *
 * \param pcClientLatestTime
 * \a (i) first time string to compare, need not be client latest time.
 * \param pcServerLatestTime
 * \a (i) second time string to compare, need not be server latest time.
 *
 * \retval S_OK
 * two strings represent the same time.
 * \retval others
 * two strings represent different time.
 *
 **************************************************************************** */
//SCODE HTTPServer_CompareTime( PCHAR pcClientLatestTime, PCHAR pcServerLatestTime );


/*!
 *******************************************************************************
 * \brief
 * query a field value of a HTTP query string (utility function)
 *
 * \param pcQuery
 * \a (i) pointer to the query string.
 * \param pcField
 * \a (i) pointer to the field string.
 * \param pcValue
 * \a (i/o) pointer to a string to receive the returned value string.
 * \param iValueLen
 * \a (i) specify the length of the pcValue buffer.
 *
 * \retval S_OK
 * two strings represent the same time.
 * \retval others
 * two strings represent different time.
 *
 **************************************************************************** */
//SCODE HTTPServer_QueryFieldValue( PCHAR pcQuery, PCHAR pcField, PCHAR pcValue, int iValueLen );


#ifdef __cplusplus
}
#endif


#endif // end #ifndef _HTTPSERVER_H_
