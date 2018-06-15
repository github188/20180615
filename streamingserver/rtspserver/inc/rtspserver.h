/*
 *  Copyright (c) 2002 Vivotek Inc. All rights reserved.
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
 *
 *  Project name        :   Pikachu
 *  Module name         :   RTSPServerAPI.h
 *  Module description  :   Handle RTSP request from client site
 *  Author              :   Simon
 *  Created at          :   2002/4/24
 *  Revision            :   1.0
 ******************************************************************************
 *                        Revision history
 ******************************************************************************
 */
 
/*!
************************************************************
* Copyright (c) 2002 Vivotek Inc. All rights reserved.
*
* \file
* RTSPServerAPI.h
* 
* \brief
* Handle RTSP request from client site
*
* \date
* 2002/05/20
* 
* \author
* ShengFu Cheng
* 
*************************************************************
*/


#ifndef _RTSPSERVER_H_
#define _RTSPSERVER_H_

#include "osisolate.h"
#include "typedef.h"
#include "common.h"
#include "sockdef.h"

#include "rtsprtpcommon.h"
#include "streamserver.h"
#ifdef _SESSION_MGR
#include "session_mgr.h"
#endif

/*!
* Constant to define and setup RTSP server for aggregate or 
* non-aggregate control
*/
//20120801 modified by Jimmy for metadata
#define RTSPSERVER_NO_MEDIA                      0x00000000
#define RTSPSERVER_MEDIATYPE_VIDEO				0x00000001
#define RTSPSERVER_MEDIATYPE_AUDIO				0x00000002
#define RTSPSERVER_MEDIATYPE_METADATA			0x00000004
//20120925 added by Jimmy for ONVIF backchannel
#define RTSPSERVER_MEDIATYPE_AUDIOBACK			0x00000008

//20120925 added by Jimmy for ONVIF backchannel
#define REQUIRE_NONE                 0
#define REQUIRE_ONVIF_BACKCHANNEL    1

//20120925 added by Jimmy for ONVIF backchannel
#define RTSPSERVER_AUDIOBACK_TIMEOUT 30


/*
#define RTSPSERVER_MEDIATYPE_AUDIOVIDEO		1
#define RTSPSERVER_MEDIATYPE_AUDIOONLY		2
#define RTSPSERVER_MEDIATYPE_VIDEOONLY		3
*/

#define TCP_REQUEST_CS		1
#define TCP_RELEASE_CS		2

#define RTSP_HTTP_ADD_SINGLE	1
#define RTSP_HTTP_ADD_PAIR		2
//20140418 added by Charles for http port support rtsp describe command
#define RTSP_HTTP_ADD_DESCRIBE	3

/* 20100225 fixed sessioncookie size */
#define RTSP_HTTP_COOKIE_LEN	64+1
/* 20150427 Modified by Charles*/
#define RTSP_HTTP_MESSAGE_LEN	1024 //512

#define	RTSP_AUTH_DISABLE		0
#define RTSP_AUTH_BASIC			1
#define RTSP_AUTH_DIGEST		2


#define RTSPSERVER_REALM		"streaming_server"

#ifdef _SHARED_MEM
/* 20100428 Added For Media on demand */
#define RTSPMOD_SDP_KEYWORD		"mod.sdp"
#define RTSPMOD_REQUEST_NOWAIT	1
#endif
											//Note: 20100402 for long URI for RTSP
#define	RTSP_URL_LEN			1536		//This is the total length. Example: rtsp://server[:port/]filename?ExtraInfo
#define RTSP_URL_ACCESSNAME_LEN	168			//This is the length for server[:port] part, so it is actually for IP address(hostname) and port
											//The length for filename?ExtraInfo is actually RTSP_URL_LEN - RTSP_URL_ACCESSNAME_LEN
#define RTSP_URL_EXTRA_LEN		RTSP_URL_LEN - RTSP_URL_ACCESSNAME_LEN		//This is the maximum length for ExtraInfo after the file name, should be close to RTSP_URL_LEN - RTSP_URL_ACCESSNAME_LEN

//20111124 modify by danny support Backchannel Multicast for ONVIF test tool 1.02.4
#define	ONVIF_TEST_TOOL			400

typedef struct tokens
{
   char  *token;
   int   code;
} TKN;

/*!
* a structure that store the parameters of HTTP socket for RTSP over HTTP mode
*/
typedef struct
{
	/*! Socket for Send RTSP over HTTP */
	int		iSendSock;
	/*! Socket for Receive RTSP over HTTP */
	int		iRecvSock;
	/*! Session cookie */
	char	acSessionCookie[RTSP_HTTP_COOKIE_LEN];
	/*! RTSP Message buffer */
	char	acMessageBuffer[RTSP_HTTP_MESSAGE_LEN];
}THTTPCONNINFO;

/*!
* a structure that store the parameter of RTSP sever
*/
typedef struct
{
    /*! 
    * Parameter that define the attribute of aggregate-control 
    * of RTSP server 
    */
	//int             StreamingMode;	
	/*!
	* Parameter that define the port number of RTSP server
	*/
    unsigned short  rtsp_port;

	/*! port number of RTP video port */
	unsigned short usRTPVPort;
	/*! port number of RTP auido port */
	unsigned short usRTPAPort;
	//20120723 modified by Jimmy for metadata
	/*! port number of RTP metadata port */
	unsigned short usRTPMPort;

	/*! port number of RTCP video port */
	unsigned short usRTCPVPort;
	/*! port number of RTCP audio port */
	unsigned short usRTCPAPort;
	//20120723 modified by Jimmy for metadata
	/*! port number of RTCP metadata port */
	unsigned short usRTCPMPort;


	/*! fixed UDP video socket for Symmetric RTP */
	int		iUDPRTPVSock;
	/*! fixed UDP audio socket for Symmetric RTP */
	int		iUDPRTPASock;
	//20120723 modified by Jimmy for metadata
	/*! fixed UDP metadata socket for Symmetric RTP */
	int		iUDPRTPMSock;

	/*! Thread Priority for RTSP Server */
	unsigned long   ulThreadPriority;

#ifdef 	RTSPRTP_MULTICAST
	//20130327 modified by Jimmy to support different video/audio multicast addresses for ONVIF Test Tool 12.12
	/*! Multicast address*/
	unsigned long	ulMulticastAddress[RTSP_MULTICASTNUMBER ];

	unsigned long ulMulticastAudioAddress[RTSP_MULTICASTNUMBER];
	unsigned long ulMulticastMetadataAddress[RTSP_MULTICASTNUMBER];

	/*! Multicast video port*/
	unsigned short	usMulticastVideoPort[RTSP_MULTICASTNUMBER];
	/*! Multicast audio port*/
	unsigned short	usMulticastAudioPort[RTSP_MULTICASTNUMBER ];
	//20120723 modified by Jimmy for metadata
	/*! Multicast metadata port*/
	unsigned short	usMulticastMetadataPort[RTSP_MULTICASTNUMBER];
	/*! Multicast time to live*/
	unsigned short  usTTL[RTSP_MULTICASTNUMBER];
	/*! Whether or not RTP extension is needed for this stream*/
	int             iRTPExtension[RTSP_MULTICASTNUMBER];
	/*! SDP index for this stream*/
	int             iMulticastSDPIndex[RTSP_MULTICASTNUMBER];
	//20110630 Add by danny For Multicast enable/disable
	int             iMulticastEnable[RTSP_MULTICASTNUMBER];
#endif
	/*! IP address of RTSP Server*/
	unsigned long	ulIP;

#ifdef _SHARED_MEM
	DWORD			dwProtectedDelta;
#endif

	//20081121 for URL authentication
	int				iURLAuthEnabled;

} RTSPSERVER_PARAM;

/*!
* a structure that store the parameter of client socket information
* which is delivered to system control by callback function
*/
typedef struct
{
    /*! 
    * Parameter of client IP which is 32bit unsigned long   
    * of network byte order 
    */
	unsigned long  ulIP;	// in network order
	/*! 
    * Parameter of client port number which is 16bit unsigned    
    * integer of network byte order 
    */
	unsigned short usPort;	// in network order

} RTSPSERVER_CLIENTIP;

#ifdef _SHARED_MEM
/* 20100428 Added For Media on demand */
/*!
* a structure that store the parameter of MOD information
* which is delivered to system control by callback function
*/
typedef struct
{
	/* Index of SDP file */
	int					iSDPIndex;
	/* MOD server return code if error encountered */
	EMODRunCode 		eMODRunCode;
	/* MOD command type set by client*/
    EMODCommandType 	eMODSetCommandType;
	/* MOD command type return from MOD server*/
    EMODCommandType 	eMODReturnCommandType;
	/* MOD command value set by client*/
    char 				acMODSetCommandValue[RTSPMOD_COMMAND_VALUE_LENGTH];
    /* MOD command value return from MOD server*/
    char 				acMODReturnCommandValue[RTSPMOD_COMMAND_VALUE_LENGTH];
    /* MOD Play Queue Full will set this Flag*/
    BOOL                bIsPlayQueueFull;
	/* Translate Rate-Control="no" to Scale or Speed */
	BOOL				bIsRateControl2Scale;

} RTSPSERVER_MODREQUEST;
#endif

/*!
* a structure that store the parameter of Describe information
* which is delivered to system control by callback function
*/
typedef struct
{
    /*! 
    * Parameter of client IP which is 32bit unsigned long   
    * of network byte order 
    */
	unsigned long  ulIP;	// in network order
	/*! 
    * Parameter of client port number which is 16bit unsigned    
    * integer of network byte order 
    */
	unsigned short usPort;	// in network order
	
	/*!
	* describe file name (*.sdp)
	*/
	char *pcDescribe;
#ifdef _SHARED_MEM
	/* 20100428 Added For Media on demand */
	char 				*pcExtraInfo;
	EMODRunCode 		eMODRunCode;
#endif
	/*! 
	* pointer to the buffer to store SDP file
	*/
	char *pSDPBuffer;

	/*! 
	* length of SDP buffer	
	*/
	int	iSDPBuffLen;

	/*!
	* index of SDP file	
	*/
	int  iSDPindex;
	/*! 1 means Vivotek client */
	int	 iVivotekClient;

	//20111124 modify by danny support Backchannel Multicast for ONVIF test tool 1.02.4
	int	 iPlayerType;

	//20120925 added by Jimmy for ONVIF backchannel
	int  iRequire;

	/*! For multiple stream reset CI */
	BOOL bResetCI;
	char acResolution[16];
	char acCodecType[16];

} RTSPSERVER_SDPREQUEST;

/*!
* a structure that store the parameter of session information
* which is delivered to system control by callback function after
* PLAY message is done. ( Except Session ID and client IP address,
* All the other session informations are in pairs for video and audio
* respectively.  
*/
typedef struct
{
    /*! 
    * Session ID of 32 bit long number 
    */
	DWORD  dwSessionID;	
	/*! 
    * Client UDP socket which is connected and ready to receive   
    * RTP media data. 
    */
	//20111205 Modified by danny For UDP mode socket leak
	//20120726 modified by Jimmy for metadata
	SOCKET *psktRTP[MEDIA_TYPE_NUMBER];
	/*! 
    * Client UDP socket which is connected and ready to exchange   
    * RTCP sender and receiver report. 
    */
    //20120726 modified by Jimmy for metadata
	SOCKET *psktRTCP[MEDIA_TYPE_NUMBER];
	/*! 
    * TimeStamp of the first media packet    
    */
    //20120726 modified by Jimmy for metadata
	DWORD  dwInitialTimeStamp[MEDIA_TYPE_NUMBER];
	/*! 
    * SSRC of each session of the server    
    */
    //20120726 modified by Jimmy for metadata
	DWORD  dwSSRC[MEDIA_TYPE_NUMBER];
	/*! 
    * Sequence number of the first media packet    
    */
    //20120726 modified by Jimmy for metadata
	WORD   wInitialSequenceNumber[MEDIA_TYPE_NUMBER];
	/*! 
    * Request media name of each session    
    */
    //20120726 modified by Jimmy for metadata
	char   cMediaName[MEDIA_TYPE_NUMBER][25];
	/*! 
    * Client IP address in network byte order    
    */
	unsigned long  ulClientIP;	

#ifdef _INET6
	/*! 
    * Client IPv6 address in socket address    
    */
	struct sockaddr_in6	 tClientSckAddr;
#endif

	/*! 
    * Timeout value for RTCP receiver report     
    */
	int  iRTCPTimeOut;	

	/*!
	* RTP packet streaming type
	*/
	int iRTPStreamingType;

	/*!
	* RTP/RTCP channel ID
	*/
	//20120726 modified by Jimmy for metadata
	int  iEmbeddedRTPID[MEDIA_TYPE_NUMBER];
	int  iEmbeddedRTCPID[MEDIA_TYPE_NUMBER];

	/*!
	* Embedded RTSP socket
	*/
	//20110706 Modified by danny For TCP/HTTP mode socket not sync in multi thread
	SOCKET *psktRTSPSocket;

#ifdef 	RTSPRTP_MULTICAST
	/*! 1 means multicast*/
	int	iMulticast;
#endif
	/*! 1 means Vivotek client */
	int		iVivotekClient;
	/*! SDP Index*/
	int		iSDPIndex;
    /*! Cseq Index*/
    int     iCseq;
	/*! Address for RTP in NAT */
	//20120726 modified by Jimmy for metadata
   	RTSP_SOCKADDR NATRTPAddr[MEDIA_TYPE_NUMBER];
	/*! Address for RTCP in NAT*/
	//20120726 modified by Jimmy for metadata
	RTSP_SOCKADDR NATRTCPAddr[MEDIA_TYPE_NUMBER];

#ifdef _SHARED_MEM
	/* Indicate the client history streaming mode */
	EHSMode				eHSMode;
	//added by neil 10/12/30
	ESVCMode			eSVCMode;
	//added by neil 11/01/14
	BOOL				bForceFrameInterval;
    DWORD				dwFrameInterval;
	/* Indicate number of seconds offset */
	int					dwBypasyMSec;
	/* Indicate number of seconds protected by Save-bandwidth feature */
	int					dwProtectedDelta;
	//20100812 Added For Client Side Frame Rate Control
	int 				iFrameIntervalMSec ;
	//20101208 Modified by danny For GUID format change
	/* 20100105 Added For Seamless Recording */
	//DWORD  			dwSessionGUID;
	char				acSeamlessRecordingGUID[RTSPS_Seamless_Recording_GUID_LENGTH];
	//20101020 Add by danny for support seamless stream TCP/UDP timeout
	bool				bSeamlessStream;
    //20140812 Added by Charles for mod no drop frame
    BOOL                bMediaOnDemand;
#endif
    //20140819 added by Charles for eventparser API
    BOOL    bAnalytics;  //send scene data or not
    char    acVideoAnalyticsConfigToken[RTSPSTREAMING_TOKEN_LENGTH];

    //20160603 add by faber, maintain tcpmux index by client
	int 			iClientIndex;
	//20170517	add by faber, notify source in the thread of mediachannel, cause output stop is sent in mediachannel
	int 			iNotifySource;
} RTSPSERVER_SESSIONINFORMATION;

/*!
* Flag of CallBack ( the authentication of client IP CallBack )
*/
#define RTSPSERVER_CALLBACKFLAG_ACCESSIP_CHECK		1
/*!
* Flag of CallBack ( the request of SDP CallBack )
*/
#define RTSPSERVER_CALLBACKFLAG_SDP_REQUEST			2
/*!
* Flag of CallBack ( the notification of session start )
*/
#define RTSPSERVER_CALLBACKFLAG_SESSION_START		3
/*!
* Flag of CallBack ( the notification of session stop )
*/
#define RTSPSERVER_CALLBACKFLAG_SESSION_STOP		4
/*!
* Flag of CallBack ( the notification of session pause )
*/
#define RTSPSERVER_CALLBACKFLAG_SESSION_PAUSE		5
/*!
* Flag of CallBack ( the notification of session resume )
*/
#define RTSPSERVER_CALLBACKFLAG_SESSION_RESUME		6
/*!
* Flag of CallBack ( the notification of RTP session update )
*/
#define RTSPSERVER_CALLBACKFLAG_SESSION_RTPUPDATE	7
/*!
* Flag of CallBack ( the callback of authorization )
*/
#define RTSPSERVER_CALLBACKFLAG_AUTHORIZATION   	8
/*!
* Flag of Callback (the callback of request I frame
*/
#define RTSPSERVER_CALLBACKFLAG_FORCE_I_FRAME		9

//20120809 modified by Jimmy for metadata
/*! check if video track or not*/
//#define RTSPSERVER_CALLBACKFLAG_CHECK_VIDEO_TRACK	10
#define RTSPSERVER_CALLBACKFLAG_CHECK_TRACK_MEDIATYPE	10

/*! check video only, audio only, both of the stream*/
#define	RTSPSERVER_CALLBACKFLAG_CHECK_STREAM_MODE	11
/*! Receive Updated Session Info*/
#define RTSPSERVER_CALLBACKFLAG_UPDATE_SESSIONINFO	12
/*! Check if the access name is correct*/
#define RTSPSERVER_CALLBACKFLAG_CHECK_ACCESSNAME	13
/*! Upload audio data for SIP-2-WAY*/
#define RTSPSERVER_CALLBACKFLAG_UPLOAD_AUDIODATA    14
/*! Upload audio data timeout for SIP-2-WAY*/
#define RTSPSERVER_CALLBACKFLAG_UPLOAD_AUDIODATA_TIMEOUT    15

/*!Remove multicast back channel(when more than 1 clients still exist for this stream) Added by Louis to fix multicast bug!*/
#define RTSPSERVER_CALLBACKFLAG_SESSION_REMOVE_BACKCHANNEL	16

/*! For share-memory query available seconds */
#define RTSPSERVER_CALLBACKFLAG_SHMEM_QUERYTIMESHIFT		17

/*! For multiple stream break RTSPSERVER_CALLBACKFLAG_SDP_REQUEST into 2 parts */
#define RTSPSERVER_CALLBACKFLAG_COMPOSE_SDP					18

/*! For multiple stream reset CI */
#define RTSPSERVER_CALLBACKFLAG_RESET_CI					19

#ifdef _SHARED_MEM
/*! 20100105 Added For Seamless Recording update guid list info */
#define RTSPSERVER_CALLBACKFLAG_UPDATE_GUIDLISTINFO			20
#define RTSPSERVER_CALLBACKFLAG_UPDATE_RECODERSTATE			21
#endif

/*! 20100319 Added for OPTION used as keep-alive */
#define RTSPSERVER_CALLBACKFLAG_KEEP_ALIVE					22
#ifdef _SHARED_MEM
/* 20100428 Added For Media on demand */
#define RTSPSERVER_CALLBACKFLAG_SET_MODCONTROLINFO			23
/* 20110225 Added for SVC-T */
#define RTSPSERVER_CALLBACKFLAG_SET_SVCLEVEL				25
#endif

//20110401 Added by danny For support RTSPServer thread watchdog
#define RTSPSERVER_CALLBACKFLAG_KICK_WATCHDOG				24

//20120925 added by Jimmy for ONVIF backchannel
#define RTSPSERVER_CALLBACKFLAG_GET_CHANNEL_INDEX			26
#define RTSPSERVER_CALLBACKFLAG_UPDATE_AUDIODATA_SDPINFO	27
//20130916 added by Charles for ondemand multicast
#define	RTSPSERVER_CALLBACKFLAG_CLOSE_MULTICASTSOCKET		28
//20141110 added by Charles for ONVIF Profile G
#define RTSPSERVER_CALLBACKFLAG_SET_PLAY_CSEQ               29
//20151110 Added by faber for stop/start mod
#define RTSPSERVER_CALLBACKFLAG_SET_MOD_STOP 				30
#define RTSPSERVER_CALLBACKFLAG_SET_MOD_START 				31

//20161212 add by Faber, check web attraction or factory default mode
#define RTSPSERVER_CALLBACKFLAG_CHECK_NEED_AUTHORIZATOIN	32

//20170524
#define RTSPSERVER_CALLBACKFLAG_GET_NEW_MOD_STREAM			33
#define RTSPSERVER_CALLBACKFLAG_RELEASE_MOD_STREAM			34
/*!
********************************************************************************************************
* \brief
* CallBack Function of RTSP server to Control Module
* \param hParentHandle
* \a (i) handle of control module which created RTSP server
*
* \param uMsgFlag
* \a (i) the flag used to identify which action should take to the callback function
* 
* \param pvParam1
* \a (i/o) the first parameter use to callback to control module
* 
* \param pvParam1
* \a (i/o) the second parameter use to callback to control module
*
* \note
*
* in uMsgFlag case : \b RTSPSERVER_CALLBACKFLAG_ACCESSIP_CHECK
*
*     pvParam1: pointer to the RTSPSERVER_CLIENTIP structure containing client site 
*               IP and port information to check or autheticate. 
*                              
*     pvParam2: no used.
*
*     return value: 0 means this client IP has access right, others mean access deny          
*
*  in uMsgFlag case : RTSPSERVER_CALLBACKFLAG_SDP_REQUEST
* 
*    pvParam1: pinter to RTSPSERVER_SDPREQUEST structure containing Describe information. 
               
*    pvParam2: pointer to the buffer where SDP information is copied to (memory allocated by RTSP server)
               
*    return value: 0 or negative value means ERROR, positive value means the length of sdp          
*
*  in uMsgFlag case : RTSPSERVER_CALLBACKFLAG_SESSION_START
*
*    pvParam1: pointer to RTSPSERVER_SESSIONINFORMATIO structure containing the session information
*                            
*    pvParam2: no used.
*
*    return value: 0 is OK, negative value means error
*
* in uMsgFlag case : RTSPSERVER_CALLBACKFLAG_SESSION_STOP
*
*    pvParam1: (DWORD)session ID of the terminated session. 
                              
*    pvParam2: no used.

*    return value: 0 is OK, negative value means error
*
* in uMsgFlag case : RTSPSERVER_CALLBACKFLAG_SESSION_PAUSE
*
*    pvParam1: (DWORD)session ID of the paused session. 
                              
*    pvParam2: no used.

*    return value: 0 is OK, negative value means error
*
* in uMsgFlag case : RTSPSERVER_CALLBACKFLAG_SESSION_RESUME
*
*    pvParam1: (DWORD)session ID of the resumed session. 
                              
*    pvParam2: no used.

*    return value: 0 is OK, negative value means error
*
*  in uMsgFlag case : RTSPSERVER_CALLBACKFLAG_SESSION_RTPUPDATE
*
*    pvParam1: pointer to RTSPSERVER_SESSIONINFORMATIO structure containing the session information
*              to be updated
*                            
*    pvParam2: no used.
*
*    return value: 0 is OK, negative value means error
*
*
*  in uMsgFlag case : RTSPSERVER_CALLBACKFLAG_AUTHORIZATION
*
*    pvParam1: pointer to TAuthorInfo structure containing the authentication information
*                            
*    pvParam2: no used.
*
*    return value: 0 is OK, negative value means error
*
*
*  in uMsgFlag case : RTSPSERVER_CALLBACKFLAG_FORCE_I_FRAME
*
*    pvParam1: pointer to RTSPSERVER_SESSIONINFORMATIO structure containing the session information
*              to be updated
*                            
*    pvParam2: no used.
*
*    return value: 0 is OK, negative value means error
*
*
*  in uMsgFlag case : RTSPSERVER_CALLBACKFLAG_CHECK_VIDEO_TRACK
*
*    pvParam1: index of the stream to be callbacked to.
*                            
*    pvParam2: no used.
*
*    return value: 0 is OK, negative value means error
*
*
*  in uMsgFlag case : RTSPSERVER_CALLBACKFLAG_CHECK_STREAM_MODE
*
*    pvParam1: index of the stream to be callbacked to.
*                            
*    pvParam2: no used.
*
*    return value: 0 is OK, negative value means error
*
*
*  in uMsgFlag case : RTSPSERVER_CALLBACKFLAG_UPDATE_SESSIONINFO
*
*    pvParam1: pointer to buffer to store the session info
*                            
*    pvParam2: no used.
*
*    return value: 0 is OK, negative value means error
*
*
*  in uMsgFlag case : RTSPSERVER_CALLBACKFLAG_CHECK_ACCESSNAME
*
*    pvParam1: Access name string
*                            
*    pvParam2: index of the stream to be callbacked to.
*
*    return value: 0 is OK, negative value means error
*
*
*  in uMsgFlag case : RTSPSERVER_CALLBACKFLAG_UPLOAD_AUDIODATA
*
*    pvParam1: pointer to PROTOCOL_MEDIABUFFER structure containing the audio info
*                            
*    pvParam2: no used.
*
*    return value: 0 is OK, negative value means error
*
*
*  in uMsgFlag case : RTSPSERVER_CALLBACKFLAG_UPLOAD_AUDIODATA_TIMEOUT
*
*    pvParam1: Session ID of Upload Stream Audio
*                            
*    pvParam2: no used.
*
*    return value: 0 is OK, negative value means error
*
*
*  in uMsgFlag case : RTSPSERVER_CALLBACKFLAG_SESSION_REMOVE_BACKCHANNEL
*
*    pvParam1: (DWORD)session ID of the back channel that wish to be removed.
*				
*    pvParam2: no used.
*
*    return value: 0 is OK, negative value means error
*
*********************************************************************************************
*/

typedef int (*RTSPSERVERCALLBACK)(HANDLE hParentHandle, UINT uMsgFlag, void * pvParam1, void * pvParam2);


/*!
***************************************************************************************************
* \brief
* Create handle of RTSP server
*
* \param iMaxSessionNumber
* \a (i) the maximum number of clients that RTSP server can accept
*
* \param pstRTSPServerParameter
* \a (i) a pointer to \b RTSPSERVER_PARAM for setting the parameter of RTSP server
*
* \return handle of RTSP server
*
****************************************************************************************************
*/
HANDLE RTSPServer_Create( int iMaxSessionNumber, RTSPSERVER_PARAM *pstRTSPServerParameter);

/*!
***************************************************************************************************
* \brief
* Start RTSP server
*
* \param hRTSPServerHandle
* \a (i) the handle of RTSP server 
*
* \param ulThreadPriority
* \a (i) the priority of RTSP server task
*
* \retval 0 
* RTSP server task start OK
*
* \retval 1
* RTSP server task start failed
*
****************************************************************************************************
*/
int RTSPServer_Start(HANDLE hRTSPServerHandle);

/*!
***************************************************************************************************
* \brief
* Stop RTSP server
*
* \param hRTSPServerHandle
* \a (i) the handle of RTSP server 
*
* \retval 0 
* RTSP server task stop OK
*
* \retval 1
* RTSP server task stop failed
*
* \note
* this API will make RTSP server stop the server main loop
****************************************************************************************************
*/
int RTSPServer_Stop(HANDLE hRTSPServerHandle);

/*!
***************************************************************************************************
* \brief
* close RTSP server
*
* \param hRTSPServerHandle
* \a (i) the handle of RTSP server 
*
* \retval 0 
* RTSP server resource free OK
*
* \retval 1
* RTSP server resource free failed
*
* \note
* this API will release all the resource RTSP server take
*
****************************************************************************************************
*/
int RTSPServer_Close(HANDLE hRTSPServerHandle);

/*!
***************************************************************************************************
* \brief
* Set CallBack Function to RTSP server 
*
* \param hRTSPServerHandle
* \a (i) the handle of RTSP server 
*
* \param fCallback
* \a (i) the pointer to \b RTSPSERVERCALLBACK for assigning callback function
* to thr RTSP server
*
* \param  hParentHandle
* \a (i) the handle of the parent boject which created the RTSP server object
*
* \retval 0 
* set callback function to RTSP server OK
*
* \retval 1
* set callback function to RTSP server failed
*
***************************************************************************************************
*/
int RTSPServer_SetCallback(HANDLE hRTSPServerHandle, RTSPSERVERCALLBACK fCallback, HANDLE hParentHandle);

/*!
***************************************************************************************************
* \brief
* Set RTSP server parameter
*
* \param hRTSPServerHandle
* \a (i) the handle of RTSP server 
*
* \param pstVideoEncodingParameter
* \a (i) the pointer to \b RTSPSERVER_PARAM to set into RTSP server
* to thr RTSP server
*
* \retval 0 
* RTSP server parameter setting OK
*
* \retval 1
* RTSP server parameter setting failed
*
***************************************************************************************************
*/
int RTSPServer_SetParameters(HANDLE hRTSPServerHandle, RTSPSERVER_PARAM *pstVideoEncodingParameter);

/*!
***************************************************************************************************
* \brief
* Remove one sesison from RTSP server 
*
* \param hRTSPServerHandle 
* \a (i) the handle of RTSP server 
*
* \param dwSessionID
* \a (i) the session ID would be removed 
*
* \retval 0 
* the removal of the session is queued 
*
* \retval 1
* the removal of session failed
*
***************************************************************************************************
*/
int RTSPServer_RemoveSession(HANDLE hRTSPServerHandle, DWORD dwSessionID);

//int RTSPServer_SetMediaStreamMode(HANDLE hRTSPServerHandle, DWORD dwMediaStreamMode);
/*!
***************************************************************************************************
* \brief
* Add RTP over HTTP socket to RTSP Server
*
* \param hRTSPServerHandle 
* \a (i) the handle of RTSP server 
*
* \param ptHTTPConnInfo
* \a (i) pointer to  THTTPCONNINFO structure which contains HTTP connection info
*
* \param dwFlag
* \a (i) Flag to indicate RTSP_HTTP_ADD_PAIR or RTSP_HTTP_ADD_SINGLE
*
* \retval 0 
* Add RTP over HTTP success
*
* \retval 1
* Add RTP over HTTP success failed
*
***************************************************************************************************
*/
int RTSPServer_AddRTPOverHTTPSock(HANDLE hRTSPServerHandle, THTTPCONNINFO *ptHTTPConnInfo, DWORD dwFlag);
/*!
***************************************************************************************************
* \brief
* Add critical section handle to RTSP Server
*
* \param hRTSPServerHandle 
* \a (i) the handle of RTSP server 
*
* \param hTCPMuxCS
* \a (i) handle of critical section
*
* \retval 0 
* Add critical section success
*
* \retval 1
* Add critical section failed
*
***************************************************************************************************/
int RTSPServer_AddTCPMuxHandle(HANDLE hRTSPServerHandle, HANDLE hTCPMuxCS);
/*!
***************************************************************************************************
* \brief
* Get current session number from RTSP Server
*
* \param hRTSPServerHandle 
* \a (i) the handle of RTSP server 
*
* \retval Number of sessions 
*
***************************************************************************************************/
int RTSPServer_GetCurrentSessionNumber(HANDLE hRTSPServerHandle);
/*!
***************************************************************************************************
* \brief
* Report Session Tear down ok from Media Channel
*
* \param hRTSPServerHandle 
* \a (i) the handle of RTSP server 
*
* \param dwSessionID
* \a (i) Session ID of the session which has been teared down
*
* \retval 0 
* Report teared down success
*
* \retval 1
* Report teared down failed
*
***************************************************************************************************/
void RTSPServer_TeardownSessionOK(HANDLE hRTSPServerHandle, DWORD dwSessionID);
/*!
***************************************************************************************************
* \brief
* Set the authentication type of RTSP Server
*
* \param hRTSPServerHandle 
* \a (i) the handle of RTSP server 
*
* \param iAuthType
* \a (i) Authentication type of RTSP (Disabled, basic, or digest)
*
* \retval 0 
* Set the authentication type of RTSP Server success
*
* \retval 1
* Set the authentication type of RTSP Server failed
*
***************************************************************************************************/
int	RTSPServer_SetAuthenticationType(HANDLE hRTSPServerHandle,int iAuthType);

#ifdef	_SIP_TWO_WAY_AUDIO
int RTSPServer_SetSSRCForUpStreamAudio(HANDLE hRTSPServerHandle,unsigned long ulSSRC);
int RTSPServer_SetSessionIDForUpStreamAudio(HANDLE hRTSPServerHandle,unsigned long ulSessionID);
int RTSPServer_SetLastRecvTimeForUpStreamAudio(HANDLE hRTSPServerHandle, DWORD dwLastRecvSec);
int RTSPServer_SetLastRecvTimeForUpStreamAudioNow(HANDLE hRTSPServerHandle);
int RTSPServer_CheckTimeoutForUpStreamAudio(HANDLE hRTSPServerHandle);
#endif

/*! 20090224 QOS
***************************************************************************************************
* \brief
* Set the QOS type for socket
*
* \param iSockFd 
* \a (i) Socket to be set
*
* \param TQosInfo
* \a (i) QOS parameter structure
*
* \param eMediaType
* \a (i) 0 stands for control, 1 stands for video while 2 stands for audio
*
* \retval S_OK
* Set the authentication type of RTSP Server success
*
* \retval S_FAIL
* Set the authentication type of RTSP Server failed
*
***************************************************************************************************/
SCODE RTSPServer_SetQosToSocket(int	iSockFd, TQosInfo *ptQosInfo, EQosMediaType eMediaType);
SCODE RTSPServer_UpdateQosParameters(HANDLE hRTSPServerHandle, TQosInfo *ptQosInfo);

int RTSPServer_RemoveSessionFromAPI(HANDLE hRTSPServerHandle, DWORD dwSessionID);
#ifdef _SHARED_MEM
//20101208 Modified by danny For GUID format change
/* 20100105 Added For Seamless Recording */
SCODE RTSPServer_UpdateSeamlessRecordingParameters(HANDLE hRTSPServerHandle, TSeamlessRecordingInfo *ptSeamlessRecordingInfo);
int RTSPServer_ResetGUID(HANDLE hRTSPServerHandle, char* pcSessionGUID);
SCODE RTSPServer_SetLastFrameTimeForAVStream(HANDLE hRTSPServerHandle, HANDLE hShmemSessionInfo, unsigned long ulSessionID);
#endif

#ifdef _SESSION_MGR
/* 20100623 danny, Added for fix sessioninfo corrupted issue */
int RTSPServer_AddSessionMgrHandle(HANDLE hRTSPServerHandle, HANDLE hSessionMgrHandle);
/*
	20120906 added by Jimmy to fix sessioninfo not concsistent issue:
	The seesion number in sessioninfo must be concsistent with the managed session number( pClient->bManagedbySessionMgr==TRUE ), not iCurrentSessionNumber.
*/
int RTSPServer_GetManagedSessionNumber(HANDLE hRTSPServerHandle);
#endif

#ifdef RTSPRTP_MULTICAST
//20100720 Added by danny to fix Backchannel multicast session terminated, rtsp server has not stopped sending video/audio RTP/RTCP
int RTSPServer_GetMulticastCurrentSessionNumber(HANDLE hRTSPServerHandle, int iMulticastindex);
//20100714 Added by danny For Multicast parameters load dynamically
SCODE RTSPServer_UpdateMulticastParameters(HANDLE hRTSPServerHandle, int iMulticastIndex, MULTICASTINFO *ptMulticastInfo);
#endif

#endif  //_RTSPSERVER_H_

