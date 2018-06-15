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
 *  Module name        :   Streaming Server
 *  File name          :   streamserver.h
 *  File description   :   Streaming Server Module API
 *  Author             :   Jason Yang
 *  Created at         :   2003/08/11
 *  Note               :   
 */



#ifndef _STREAMSERVER_H_
#define _STREAMSERVER_H_

#include "mediatypedef.h"
#include "bitstreambufdef.h"
#include "rtpmediabuf.h"
#ifdef _SHARED_MEM
/* 20100428 Added For Media on demand */
#include "ubuffer.h"
#endif
#ifdef _METADATA_ENABLE
#include "eventparser.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif


// constants
#define STREAMSERVER_VERSION MAKEFOURCC( 1, 0, 0, 0 )

// error codes
#define ERR_STREAMSERVER_VERSION            0x80020001
#define ERR_STREAMSERVER_MALLOC             0x80020002
#define ERR_STREAMSERVER_QUEUE              0x80020003
#define ERR_STREAMSERVER_CREATE_MEDIA       0x80020004
#define ERR_STREAMSERVER_INIT_CONTROL       0x80020005

// success codes
//#define S_HTTPSERVER_SEND_COMPLETE  0x00010001
//#define S_HTTPSERVER_SEND_PENDING   0x00010002 

// the control channel thread is already running
#define CONTROLCHANNEL_S_RUNNING			0x00050000
// the control channel thread is already stopped
#define CONTROLCHANNEL_S_STOPPED			0x00050001
#define CONTROLCHANNEL_S_WAIT2NDSOCKET		0x00050002

#define CONTROLCHANNEL_E_VERSION			0x80050001
#define CONTROLCHANNEL_E_TASKSTACKSIZE		0x80050002
#define CONTROLCHANNEL_E_TASKPRIORITY		0x80050003
#define CONTROLCHANNEL_E_MALLOC				0x80050004
#define CONTROLCHANNEL_E_MAXCONNECTION		0x80050005
#define CONTROLCHANNEL_E_AUDIOENCNUM		0x80050006
#define CONTROLCHANNEL_E_VIDEOENCNUM		0x80050007
#define CONTROLCHANNEL_E_AUDIODECNUM		0x80050008
#define CONTROLCHANNEL_E_NOCALLBACK			0x80050009
#define CONTROLCHANNEL_E_EVENTCREATE		0x8005000A
#define CONTROLCHANNEL_E_THREADCREATE		0x8005000B
#define CONTROLCHANNEL_E_THREADSTART		0x8005000C
// no empty connection, already reach the maximum connection
#define CONTROLCHANNEL_E_FULLLOAD			0x8005000D
// create critical section failed
#define CONTROLCHANNEL_E_CSCREATE			0x8005000E
#define CONTROLCHANNEL_E_INVALIDID			0x8005000F
#define CONTROLCHANNEL_E_SOCKETCREATE		0x80050010
#define CONTROLCHANNEL_E_AUDIOMODE			0x80050011


// media callback type definitions
#define MEDIA_CALLBACK_REQUEST_BUFFER			100
#define MEDIA_CALLBACK_RELEASE_BUFFER			200
#define MEDIA_CALLBACK_FLUSH_BUFFER				300
#define MEDIA_CALLBACK_CHECK_CODEC_INDEX		400
#define MEDIA_CALLBACK_SHM_REQUEST_BUFFER		500
#define MEDIA_CALLBACK_SHM_REQUEST_BUFFER_OLD 	501
#define MEDIA_CALLBACK_SHM_SELECT_SOCKET		600
#define MEDIA_CALLBACK_SHM_CHECKBUFFER			700

// control channel definition
#define CTRLCH_MAXDIALERT		4
#define CTRLCH_MAXMDWINDOW		4

#define SS_HTTPMOTHOD_GET		0
#define SS_HTTPMOTHOD_POST		1
//20140418 added by Charles for http port support rtsp describe command
#define	SS_HTTPMOTHOD_DESCRIBE	2 

/***************** Setting Flag ********************/
#define SS_VIDEO_TASKPRIORITY_FLAG	    0x00000001
#define SS_AUDIO_TASKPRIORITY_FLAG	    0x00000002
#define SS_CONTROL_TASKPRIORITY_FLAG	0x00000004
#define SS_SEND_TASKPRIORITY_FLAG	    0x00000008
#define SS_RECV_TASKPRIORITY_FLAG	    0x00000010

#define SS_SET_AUDIOENC_FLAG			0x00000020
#define SS_SET_VIDEOENC_FLAG			0x00000040
#define SS_SET_AUDIODEC_FLAG			0x00000080
#define SS_SET_RESPONSETIMEOUT_FLAG	    0x00000100
#define SS_SET_KEEPALIVETIMEOUT_FLAG	0x00000200
#define SS_SET_AUDIOUDPPORT_FLAG		0x00000400
#define SS_SET_VIDEOUDPPORT_FLAG		0x00000800
#define SS_SET_AUDIOMODE_FLAG           0x00001000
#define SS_SET_UDPSLOWSTART_FLAG		0x00002000

#define SS_GET_ONLINE_NUM				0x00010001

#define SS_AUTH_USER_LENGTH				128
#define SS_AUTH_PASS_LENGTH				128
#define	SS_ACCESS_NAME_LENGTH			150

//20140107 moved to rtsprtpcommon.h by Charles
//#define MAX_CONNECT_NUM				10

/*Added for share memory*/
#ifdef _SHARED_MEM
//20100428 Added For Media on demand, allocate TUBufferConf MAX size, if your sizeof(TUBufferConf) > sizeof(TUBufferConfMOD), please modify!!!
//#define RTSPS_VIDEO_UBUFFER_HEADERSIZE	sizeof(TUBufferConfMP4V) + MAX_MP4V_HEADER_SIZE
#define RTSPS_VIDEO_UBUFFER_HEADERSIZE	sizeof(TUBufferConfMOD)
#define RTSPS_AUDIO_UBUFFER_HEADERSIZE	sizeof(TUBufferConfGAMR)
//20120731 added by Jimmy for metadata
#define RTSPS_METADATA_UBUFFER_HEADERSIZE	sizeof(TUBufferConfGAMR)

/* 20091009 Added for writev */
#define RTSPS_VIDEO_RTPBUFFER_NUM		500
#define RTSPS_AUDIO_RTPBUFFER_NUM		1
//20120806 added by Jimmy for metadata
#define RTSPS_METADATA_RTPBUFFER_NUM	8 /*4*/

/* ******************************* 20100105 Added For Seamless Recording ********************************** */
//20101208 Modified by danny For GUID format change
#define RTSPS_Seamless_Recording_GUID_LENGTH	36+1
#define UNSIGNED_INT_NUMBER			0xffffffff	// 2^32-1
//20100812 Added For Client Side Frame Rate Control
#define UNSIGNED_SHORT_NUMBER		0xffff		// 2^16-1
#endif

//20110725 Add by danny For Multicast RTCP receive report keep alive
#ifdef RTSPRTP_MULTICAST
#define MULTICAST_TIMEOUT	60
extern int g_iMulticastTimeout ;
#endif

//20140819 added by Charles for eventparser API
#define RTSPSTREAMING_TOKEN_LENGTH                   64


// protocol type
typedef enum
{
	ccptHTTP,
	ccptUDP
} EControlChannelProtocolType;

// control channel audio mode
typedef enum
{	
	ccamFullDuplex,
	ccamHalfDuplex,
	ccamSimplexTalkOnly,
	ccamSimplexListenOnly,
	ccamNone
} EControlChannelAudioMode;

// control channel callback type
typedef enum
{
	ccctCreateMediaChannel,	// 0
	ccctReleaseMediaChannel,// 1
	ccctGetLocation,		// 2
	ccctForceIntra,			// 3
	ccctPacketLoss,			// 4
	ccctTimeout,			// 5
	ccctClose,				// 6
	ccctSocketError,		// 7
	ccctThreadExit,			// 8
	ccctSystemFail,			// 9
	ccctWaitHTTPSocket,		// 10
	ccctAuthorization,      // 11
	ccctNeedAuthorization,		//20161212 add by Faber, check webattaction or factory default mode
	ccctStreamTypeVideoStart,	// 
	ccctStreamTypeAudioStart,   // 
	ccctStraemTypeVideoPause, //20160620 add by Faber, for MOD set parameter
	ccctStraemTypeAudioPause,
	ccctStraemTypeVideoResume,
	ccctStraemTypeAudioResume,
	//20120806 added by Jimmy for metadata
	ccctStreamTypeMetadataStart,// 
	ccctStreamTypeVideoStop,    //
	ccctStreamTypeAudioStop,    // 
	//20120806 added by Jimmy for metadata
	ccctStreamTypeMetadataStop, // 
	ccctRTSPSessionStart,		// 
	ccctRTSPSessionStop,		// 
	ccctRTSPSessionError,		// 
	ccctRTSPSessionInfoUpdate,
	ccctRTPAudioDataUpload,
	ccctAudioUploadSDPInfo,
	ccctForceCI,
#ifdef _SHARED_MEM
	//20110915 Modify by danny for support Genetec MOD
	ccctSetToConfiger,
	ccctRecoderStateUpdate,
	ccctMODForceCI,
	ccctSetMODControlInfo,
#endif
	ccctSIPUAessionInfoUpdate,
	ccctGetMultipleChannelChannelIndex,
	ccctRTSPServerKickWatchdog,
	//20170524
	cccGetNemModStream,
	cccReleaseModStream
} EControlChannelCallbackType;

// stream direction
typedef enum
{
	ccsdDownstream	= 0,
	ccsdUpstream	= 1
} EControlChannelStreamDir;

// DI status
typedef enum
{
	ccdisNoChange = 0,
	ccdisOn,
	ccdisOff,
	ccdisRising,
	ccdisFalling
} ECtrlChDIStatus;

typedef enum
{
	ssrcpVideoCodecType = 0,
	ssrcpKeyFrameInterval,
	ssrcpVideoFrameRate,
}EStreamServerRateControlParam;

// codec information
typedef struct{
	EMediaCodecType		eCodecType;
	DWORD				dwBitrate;
	WORD				wSamplingFreq;
	WORD				wChannelNumber;
	//20100708 Added by Danny for optional channels output data
	DWORD 				dwChannelBitmap;
} TCtrlChCodecInfo;

 //Move from rtspstreamingserver.h by YenChun 070419
#define	RTSPSTREAMING_AUTHENTICATION_DISABLE		0
#define RTSPSTREAMING_AUTHENTICATION_BASIC			1
#define	RTSPSTREAMING_AUTHENTICATION_DIGEST			2

// Authorization information
typedef struct
{
	int		iAuthType;
    char	acUserName[SS_AUTH_USER_LENGTH];
    char	acPasswd[SS_AUTH_PASS_LENGTH];
    char	acAccessName[SS_ACCESS_NAME_LENGTH];
} TAuthorInfo;

// Alert message data structure
typedef struct
{
	BYTE	abyDIStatus[CTRLCH_MAXDIALERT];
	BYTE	abyMDWindow[CTRLCH_MAXMDWINDOW];
	BOOL	bNoSignal;
} TCtrlChAlertMsg;

// Streaming server initial settings 
typedef struct
{
	/*! Streaming server version*/
	DWORD dwVersion;                   
	/*! Maximum connections*/
	DWORD dwMaxConnections;            
    
    //! Media channel option
	/*! Video channel task priority*/
	DWORD dwVideoTaskPriority;         
	/*! Audio channel task priority*/
	DWORD dwAudioTaskPriority;         
	/*! Send task priority*/
	DWORD dwSendTaskPriority;          
	/*! Receive task priority*/
	DWORD dwRecvTaskPriority;          
	/*! Video channel task stack size*/
	DWORD dwVideoTaskStackSize;         
	/*! Audio channel task stack size*/
	DWORD dwAudioTaskStackSize;         
	/*! Send task stack size*/
	DWORD dwSendTaskStackSize;         
	/*! Receive task stack size*/
	DWORD dwRecvTaskStackSize;         
	
	//! Control channel option
	/*! Control channel task priority*/
	DWORD dwControlTaskPriority; 
	/*! Control channel task stack size*/
	DWORD dwControlTaskStackSize;    
	/*! Maximum number of audio encoder*/
	DWORD dwMaxAudioEncNum;          
	/*!Maximum number of audio decoder */
	DWORD dwMaxAudioDecNum;           
	/*!Maximum number of video encoder */
	DWORD dwMaxVideoEncNum;             
} TStreamServer_InitSettings;

// Streaming server set options
typedef struct
{
	DWORD dwOptionFlag;
    /*! Video channel task priority*/
	DWORD dwVideoTaskPriority;
	/*! Audio channel task priority*/
	DWORD dwAudioTaskPriority;
	/*! Send task priority*/
	DWORD dwSendTaskPriority;
	/*! Receive task priority*/
	DWORD dwRecvTaskPriority;
	/*! Control channel task priority*/
	DWORD dwControlTaskPriority;
	
	DWORD dwAudioEncNum;
	DWORD dwAudioDecNum;
	DWORD dwVideoEncNum;
	TCtrlChCodecInfo *ptAudioEncList;
	TCtrlChCodecInfo *ptAudioDecList;
	TCtrlChCodecInfo *ptVideoEncList;
	
	DWORD dwResponseTimeout;
	DWORD dwKeepAliveTimeout;
	WORD wAudioUDPPort;
	WORD wVideoUDPPort;
	EControlChannelAudioMode eAudioMode;
	BOOL  bUDPSlowStart;
} TStreamServer_Options;

typedef struct
{
	SOCKET	sckControl;
	DWORD	dwConnectionID;
	DWORD	dwPrivilege;
	int		iHTTPMethod;
	char	*pszSessionCookie;	
	char	pcRecvBuffer[GENERAL_BUFFER_LENGTH];
	DWORD	dwRecvLength;
} TStreamServer_ConnectionSettings;

/* ***************************For Share Memory********************************** */
typedef enum
{
	eHSLiveStreaming = 0,
	eHSASAP = 1,
	eHSAdaptiveRecording = 2,
	eHSHistory = 3,
	eHSModeMax = 4

} EHSMode;
//added by neil 10/12/29
typedef enum
{
    eSVCNull = 0,
	eSVCMarkFrameLevel1 = 1,
	eSVCMarkFrameLevel2 = 2,
 	eSVCMarkFrameLevel3 = 3,
	eSVCMarkFrameLevel4 = 4,
	eSVCMarkFrameLevel5 = 5,
	eSVCMarkFrameLevel6 = 6,
	eSVCMarkFrameLevel7 = 7,
	eSVCMarkFrameOnly = 8, 
	eSVCInvalid = 9

} ESVCMode;
typedef enum
{
	eCSReleased = 0,
	eCSVideoRTP = 1,
	eCSVideoRTCP = 2,
	eCSAudioRTP = 3,
	eCSAudioRTCP = 4,
	//20120816 added by Jimmy for metadata
	eCSMetadataRTP = 5,
	eCSMetadataRTCP = 6,
	eCSRTSP = 7

} ECritSecStatus;

typedef struct{		//20091009 Writev

	/*! Number of allocated media buffer */
	int					iBufferNumber;

	/*! Total size of all RTP packets (including Header, extension, bitstream) */
	DWORD				dwTotalSize;

	/*! Total size of buffers remanis to be sent*/
	DWORD				 dwTotalRemaining;

	/*! Extra Buffer for RTP extension Data & RTPExtra data, including H.264 FU-A and RFC2435 MJPEG header */
	BYTE				*pbyRTPExtraData;

	/*! Used for send timeout calculation*/
	DWORD				dwBaseTime;

	/*! Buffer for RTP Packet , size is sizeof(RTPMEDIABUFFER) * iBufferNumber */
	RTPMEDIABUFFER		*ptRTPBuffer;

	/*! Processing RTP index */
	int					iProcessRTPIndex;

}TAggregateMediaBuffer;

typedef struct
{
	/*! Parent structure for access */
	HANDLE				hParentHandle;
	/*! Handle for share memory (corresponding to the stream) */
	//20130605 modified by Jimmy to support metadata event
	HANDLE				ahShmemHandle[SHMEM_HANDLE_MAX_NUM];
	/*! Handle for share memory buffer-client */
	//20130605 modified by Jimmy to support metadata event
	HANDLE				ahClientBuf[SHMEM_HANDLE_MAX_NUM];
	/*! Buffer to store the frame */
	TBitstreamBuffer	tStreamBuffer;
	/*! Media Buffer for writev*/
	TAggregateMediaBuffer	tAggreMediaBuffer;
	/*! Remaining size to be sent for the whole frame*/ 
	int					iRemainingSize;
	/*! Indicate whether frame is generated or not */
	BOOL				bFrameGenerated;
	/*! Indicate the critical section is newly acquired */
	BOOL				bCSAcquiredButNotSelected;
	/*! Indicate Consecutive ASAP send number*/
	int					iASAPCount;
	/*! Timestamp used for calculate timeout */
	DWORD				dwTimeoutInitial;
	//20130603 added by Jimmy to follow RFC 3551, only set the marker bit in the first audio packet
	/*! Indicate first audio packet is packetized or not*/
	BOOL				bFirstAudioPacketized;
    //20140812 Added by Charles for mod no drop frame
    BOOL                bMediaOnDemand;
    //20140819 added by Charles for eventparser API
    /*! send scene data or not*/
    BOOL                bAnalytics;
    BOOL                bGetNewData;
    char                acVideoAnalyticsConfigToken[RTSPSTREAMING_TOKEN_LENGTH];
    /*! eventparser need to separate different shmem client buffer*/
    int                 iShmemClientBufID;
    int                 iProcessIndex;
} TShmemMediaInfo;

typedef struct
{
	/*! Structure for audio/video media channel */
	TShmemMediaInfo		tShmemVideoMediaInfo;
	TShmemMediaInfo		tShmemAudioMediaInfo;
	//20120806 added by Jimmy for metadata
	TShmemMediaInfo		tShmemMetadataMediaInfo;
	/*! Indicate the client history streaming mode */
	EHSMode				eHSMode;
	//added by neil 10/12/30
	ESVCMode			eSVCMode;
	//20110309
	ESVCMode			eSVCTempMode;
	//added by neil 11/01/14
	BOOL				bForceFrameInterval;
    DWORD				dwFrameInterval;
	/*! Indicate number of seconds offset */
	DWORD				dwBypasyMSec;
	/*! Indicate number of seconds protected by Save-bandwidth feature */
	DWORD				dwProtectedDelta;
	/*! Indicate the reference time for share memory API*/
	DWORD				dwRefTime;
	/*! Indicate whether the Critical section is already obtained for TCP mode*/
	ECritSecStatus		eCritSecStatus;
	//20100812 Added For Client Side Frame Rate Control
	int 				iFrameIntervalMSec ;
	//Test TCP broken image
	//char acUBuffer[500000];
	//20140819 added by Charles for eventparser API
#ifdef _METADATA_ENABLE
	TEPInfo             tGetEventInfo;
#endif  

} TShmemSessionInfo;

typedef struct
{
	/*! Write set of the select */
	fd_set				*pfdsWrite;
	/*! Maximum socket number */
	int					iMaxSck;
	/*! Return value of select */
	int					iResult;
	/*! Return value indicating the channel with newly encoded frames */
	int					aiNewFrame[VIDEO_TRACK_NUMBER];

} TShmemSelectInfo;

/**********************************For QOS*****************************************/
typedef enum
{
	eQosMuxedType = 0,
	eQosVideoType = 1, 
	eQosAudioType = 2,
	//20120726 added by Jimmy for metadata
	eQosMetadataType = 3
} EQosMediaType;

typedef struct
{
	/* Whether COS is enabled */
	int					iCosEnabled;
	/* Video priority for COS*/
	int					iCosVideoPriority;
	/* Audio priority for COS*/
	int					iCosAudioPriority;
	//20120809 added by Jimmy for metadata
	/* Metadata priority for COS*/
	int					iCosMetadataPriority;
	/* Whether DSCP is enabled*/
	int					iDscpEnabled;
	/* Video priority for DSCP*/
	int					iDscpVideoPriority;
	/* Audio priority for DSCP*/
	int					iDscpAudioPriority;
	//20120809 added by Jimmy for metadata
	/* Metadata priority for DSCP*/
	int					iDscpMetadataPriority;

} TQosInfo;

/********************************** Multiple Stream ***********************************/
typedef struct
{
	int					iSDPIndex;
	char				acResolution[16];
	char				acCodecType[16];
#ifdef _SHARED_MEM
	/* 20100428 Added For Media on demand */
	char 				*pcExtraInfo;
#endif

} TMultipleStreamCIInfo;

#ifdef RTSPRTP_MULTICAST
//20100714 Moved from rtspstreamingserver.h by danny For Multicast parameters load dynamically
/*! Multicast Information*/
typedef struct
{
	/*! Multicast Address*/
	unsigned long	ulMulticastAddress;
	//20160127 Add by Faber, for separate multicast IP
	//Audio multicast address
	unsigned long	ulMulticastAudioAddress;
	//Metadata multicast address
	unsigned long	ulMulticastMetadataAddress;

	/*! Multicast Video port*/
	unsigned short  usMulticastVideoPort;
	/*! Multicast Audio port*/
	unsigned short 	usMulticastAudioPort;
	//20120726 added by Jimmy for metadata
	/*! Multicast Metadata port*/
	unsigned short 	usMulticastMetadataPort;
	/*! Multicast Time to Live value*/
	int             usTTL;
	/*! Whether or not to use always multicast*/
	int             iAlwaysMulticast;
	/*! Whether or not to use RTP extension, If Vivotek client is detected then this value is true*/
	int             iRTPExtension;
	/*! SDP index number*/
	int				iSDPIndex;
	/*! Added by Louis 2008/01/29 for multicast audio / video only */
	int				iAlwaysMulticastAudio;
	int				iAlwaysMulticastVideo;
	//20120726 added by Jimmy for metadata
	int				iAlwaysMulticastMetadata;
	//20110630 Add by danny For Multicast enable/disable
	int             iEnable;
	//20110725 Add by danny For Multicast RTCP receive report keep alive
	int             iRRAlive;

}MULTICASTINFO;
#endif

#ifdef _SHARED_MEM
/* ******************************* 20100105 Added For Seamless Recording ********************************** */
typedef struct
{
	/* Seamless Recording unique ID, Positive Integer */
	char				acGUID[RTSPS_Seamless_Recording_GUID_LENGTH];
	/* Number of use the same unique ID */
	int					iNumber;
	/* GUID under Recording state 1: True, 0: False */
	int					iUnderRecording;

} TGUIDListInfo;

typedef struct
{
	/* Recording Disk mode, 1: Seamless, 0: Manageable */
	int					iSeamlessDiskMode;
	/* Seamless Recording max connection number */
	int					iSeamlessMaxConnection;
	/* Seamless Recording stream number */
	int					iSeamlessStreamNumber;
	/* Whether Seamless Recording is enabled */
	int					iRecordingEnable;
	/* Access Seamless Recording GUID info */
	TGUIDListInfo		tGUIDListInfo[MAX_CONNECT_NUM];
	/* Seamless Recording current connection number tGUIDListInfo.iNumber + tGUIDListInfo.iUnderRecording */
	int					iSeamlessConnectionNumber;

} TSeamlessRecordingInfo;

//20120726 modified by Jimmy for metadata
typedef struct
{
	/* Video and Audio Last Frame timeslot in Sec, 0: Video, 1: Audio, 2: Metadata*/
	unsigned long   	ulLastFrameTimeSec[MEDIA_TYPE_NUMBER];
	/* Video and Audio Last Frame timeslot in MSec, 0: Video, 1: Audio, 2: Metadata*/
	unsigned long   	ulLastFrameTimeMSec[MEDIA_TYPE_NUMBER];

} TLastFrameInfo;

/* ******************************* 20100428 Added For Media on demand ********************************** */
#define RTSPMOD_STIME_LENGTH			20				//String
#define RTSPMOD_ETIME_LENGTH			20				//String
#define RTSPMOD_FILE_LENGTH				1024			//String
#define RTSPMOD_LOC_LENGTH				64				//String
#define RTSPMOD_COMMAND_VALUE_LENGTH	256+1			//String

typedef enum
{
	eMOD_Normal = 0,
	eMOD_Download = 1,
	eMOD_Sync = 2,
	eMOD_Invalid = 3

} ERtspMODMode;

typedef struct
{
	/* Starting time of media */
	char				acStime[RTSPMOD_STIME_LENGTH];
	/* End time of media*/
	char				acEtime[RTSPMOD_ETIME_LENGTH];
	/* Specify the time of interest in local camera time */
	BOOL				bLocTime;
	/* Length of media to be played */
	DWORD				dwLength;
	/* File name of media */
	char				acFile[RTSPMOD_FILE_LENGTH];
	/* Location of the media */
	char				acLoc[RTSPMOD_LOC_LENGTH];
	/* Mode of the playing */
	ERtspMODMode		eMODMode;
	//20110915 Modify by danny for support Genetec MOD
	/* MOD command type set by client*/
    EMODCommandType 	eMODSetCommandType[MOD_COMMAND_TYPE_LAST];
	/* MOD command value set by client*/
    char 				acMODSetCommandValue[MOD_COMMAND_TYPE_LAST][RTSPMOD_COMMAND_VALUE_LENGTH];

} TMODInfo;
#endif

/* ******************************* Callback Function ******************************* */
typedef SCODE (*FControlChannel_Callback)(DWORD dwInstance, DWORD dwConnectionID, DWORD dwCallbackType, DWORD dwCallbackData);
typedef SCODE (*MEDIA_CALLBACK)(DWORD dwInstance, DWORD dwCallbackType, void* pvCallbackData);


SCODE StreamServer_GetVersion(BYTE *byMajor, BYTE *byMinor, BYTE *byBuild, BYTE *byRevision);
SCODE StreamServer_Initial(HANDLE *phStreamServer, TStreamServer_InitSettings *ptInitSettings);
SCODE StreamServer_Release(HANDLE *phStreamServer);
SCODE StreamServer_SetOptions(HANDLE hStreamServer, TStreamServer_Options *ptOptions);
SCODE StreamServer_Start(HANDLE hStreamServer);
SCODE StreamServer_Stop(HANDLE hStreamServer);
SCODE StreamServer_SetControlCallback(HANDLE hStreamServer, FControlChannel_Callback pfnCallback, DWORD dwInstance);
SCODE StreamServer_SetVideoCallback(HANDLE hStreamServer, MEDIA_CALLBACK pfnCallback, DWORD dwInstance);
SCODE StreamServer_SetAudioCallback(HANDLE hStreamServer, MEDIA_CALLBACK pfnCallback, DWORD dwInstance);
SCODE StreamServer_SetAudioOutCallback(HANDLE hStreamServer, MEDIA_CALLBACK pfnCallback, DWORD dwInstance);
SCODE StreamServer_AddConnection(HANDLE hStreamServer, TStreamServer_ConnectionSettings *ptConnSettings);
SCODE StreamServer_RemoveConnection(HANDLE hStreamServer, DWORD dwConnectionID);
SCODE StreamServer_AddHttpMediaChannel(HANDLE hStreamServer, DWORD dwConnectionID, EControlChannelStreamDir eDir, SOCKET sckHTTP);
SCODE StreamServer_AlertMsg(HANDLE hStreamServer, TCtrlChAlertMsg *ptAlertMsg);
SCODE StreamServer_SendLocation(HANDLE hStreamServer, CHAR *pcLocation);
SCODE StreamServer_SetRateControlParam(HANDLE hStreamServer, EStreamServerRateControlParam eParam, DWORD dwValue);
SCODE StreamServer_GetOption(HANDLE hStreamServer, DWORD dwType, DWORD *pdwParam1, DWORD *pdwParam2);
//20090403
DWORD rtspCheckTimeDifference(DWORD dwStartTime, DWORD dwEndTime);
char *rtspstrcpy(char *acDst, const char *pcSrc, unsigned int uiSize);
char *rtspstrcat(char *acDst, const char *pcSrc, unsigned int uiSize);
#ifdef __cplusplus
}
#endif


#endif // _STREAMSERVER_H_
