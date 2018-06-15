#ifndef	_RTSP_SERVER_LOCAL_H
#define	_RTSP_SERVER_LOCAL_H


#include <string.h>
#ifdef _LINUX
#include <arpa/inet.h>
#endif
/*#include "ipcam.h"
#include "LOCAL.H"
#include "psos.h"
#include "SOCKET.H"*/

#include "rtspserver.h"
#include "utility.h"
#include "encrypt_md5.h"
#include "encrypt_base64.h"
#include "rtprtcp.h"
#include "streamserver.h"   //20130904 added by Charles for ondemand multicast

//#define RTSP_DEBUG_LOG

#define MAX_CLIENT        5
#define LISTENQ           1024
#define MAX_RECVBUFF      2048
#define MAX_SNDBUFF       4096
#define MAX_LINE_LEN      512
#define STATUS_MSG_LEN    100

#define SETUP_VIDEO       1
#define SETUP_AUDIO       2
//20120726 added by Jimmy for metadata
#define SETUP_METADATA    3
//20120925 added by Jimmy for ONVIF backchannel
#define SETUP_AUDIOBACK    4



#define RTP_PORT          5556

#define INIT_STATE			100 
#define SETUP_STATE			200
#define PLAY_STATE			300
#define PAUSE_STATE			400
#define TEARDOWN_STATE		500

#define MAX_DELQUE_SIZE   4

#define PACKET_VIDEO_PLAYER     100
#define REAL_MEDIA_PLAYER       200
#define	QUICKTIME_PLAYER		300
#define GENETEC_PLAYER          500


#define	NONCE_LENGTH			50
//20131105 added by Charles
#define	CHECKIDLECLIENT_TIMEOUT	70000

//20080611 added for client management
#define RTSP_USERNAME_LENGTH	64
#define RTSP_PASSWORD_LENGTH	64
//For Share memory
#define DEFAULT_PROTECTED_DELTA			5
#define MAX_PROTECTED_DELTA				30
#define RTSPPARSER_DESCRIPTOR_LENGTH	32
#define	RTSPPARSER_BYPAST_LENGTH		4
#define RTSPPARSER_MODE_LENGTH			2
#define RTSPPARSER_REPLY_STRING_LENGTH		128
#define RTSPPARSER_MAX_OFFSET_ADJUST		60
#define RTSPPARSER_CHECK_KEYWORD			"forcechk"
#define RTSPPARSER_MAXSFT_KEYWORD			"maxsft"
#define RTSPPARSER_MINSFT_KEYWORD			"minsft"
#define RTSPPARSER_REFTIME_KEYWORD			"reftime"
#define RTSPPARSER_TSMODE_KEYWORD			"tsmode"
#define RTSPPARSER_NORMAL_MODE_KEYWORD		"normal"
#define RTSPPARSER_ADAPTIVE_MODE_KEYWORD	"adaptive"
//added by neil 11/01/14
#define RTSPPARSER_SVCMODE_KEYWORD							"svcmode"
#define RTSPPARSER_SVC_FRAMEINTERVAL_KEYWORK				"frameinterval"
//#define RTSPPARSER_SVC_MODE_MARKFRAME_ONLY_KEYWORD			"markframeonly"
//modified by neil 11/03/07
#define RTSPPARSER_SVC_MODE_MARKFRAME_ONLY_KEYWORD                    "markframelevel0"
#define RTSPPARSER_SVC_MODE_MARKFRAME_LEVEL1_KEYWORD		"markframelevel1"
#define RTSPPARSER_SVC_MODE_MARKFRAME_LEVEL2_KEYWORD		"markframelevel2"
#define RTSPPARSER_SVC_MODE_MARKFRAME_LEVEL3_KEYWORD		"markframelevel3"
#define RTSPPARSER_SVC_MODE_MARKFRAME_LEVEL4_KEYWORD		"markframelevel4"
#define RTSPPARSER_SVC_MODE_MARKFRAME_LEVEL5_KEYWORD		"markframelevel5"
#define RTSPPARSER_SVC_MODE_MARKFRAME_LEVEL6_KEYWORD		"markframelevel6"
#define RTSPPARSER_SVC_MODE_MARKFRAME_LEVEL7_KEYWORD		"markframelevel7"
#define RTSPPARSER_SVC_MODE_MARKFRAME_LAYERALL_KEYWORD		"markframelevel8"

#define RTSPPARSER_USERNAME_KEYWORD			"username"
#define RTSPPARSER_PASSWORD_KEYWORD			"password"
#define RTSPPARSER_CODEC_KEYWORD			"codectype"
#define RTSPPARSER_RESOLUTION_KEYWORD		"resolution"
#define RTSPPARSER_MP4_INTRA_KEYWORD		"mpeg4_intraperiod"
#define RTSPPARSER_MP4_RATEMODE_KEYWORD		"mpeg4_ratecontrolmode"
#define RTSPPARSER_MP4_QUANT_KEYWORD		"mpeg4_quant"
#define RTSPPARSER_MP4_QVALUE_KEYWORD		"mpeg4_qvalue"
#define RTSPPARSER_MP4_BITRATE_KEYWORD		"mpeg4_bitrate"
#define RTSPPARSER_MP4_FRAMERATE_KEYWORD	"mpeg4_maxframe"
#define RTSPPARSER_MJPEG_QUANT_KEYWORD		"mjpeg_quant"
#define RTSPPARSER_MJPEG_QVALUE_KEYWORD		"mjpeg_qvalue"
#define RTSPPARSER_MJPEG_FRAMERATE_KEYWORD	"mjpeg_maxframe"
//20100222 Added For Liveany Support H.264
#define RTSPPARSER_H264_INTRA_KEYWORD		"h264_intraperiod"
#define RTSPPARSER_H264_RATEMODE_KEYWORD	"h264_ratecontrolmode"
#define RTSPPARSER_H264_QUANT_KEYWORD		"h264_quant"
#define RTSPPARSER_H264_QVALUE_KEYWORD		"h264_qvalue"
#define RTSPPARSER_H264_BITRATE_KEYWORD		"h264_bitrate"
#define RTSPPARSER_H264_FRAMERATE_KEYWORD	"h264_maxframe"
#ifdef _SHARED_MEM
//20100812 Added For Client Side Frame Rate Control
#define RTSPPARSER_FRAME_INTERVAL			"frameinterval"
//20100105 Added For Seamless Recording
#define RTSPPARSER_SEAMLESS_RECORDING_GUID	"guid"
#endif

typedef struct rtsp_client
{
 
	int             iRecvSockfd;                // connectted socket 
	int             iSendSockfd;
	
	unsigned long   ulSessionID;         // rtsp Session ID 
#ifdef WISE_SPOT_AUTHENTICATE
	unsigned long   ulWiseSpotID;
#endif
	//20120723 modified by Jimmy for metadata
	unsigned short  usPlayFlag[MEDIA_TYPE_NUMBER];
	int             iStatus;                // rtsp status
	int             iCSeq;                  // Current Seq No.
	//20120723 modified by Jimmy for metadata
	unsigned short  usPort[MEDIA_TYPE_NUMBER][2];           // for RTP/RTCP port number of client video/audio/metadata stream
	char            acMediaType[MEDIA_TYPE_NUMBER][40];     // for video/audio/metadata track ID(media file name)
	int				rtp_sock[MEDIA_TYPE_NUMBER][2];         // for rtp/rtcp socket (video, audio and metadata)
	int             iTimeOut;               // for kicking out abnormal Client  

	//20120723 modified by Jimmy for metadata
	unsigned short  usInitSequence[MEDIA_TYPE_NUMBER];
	unsigned long   ulInitTimestamp[MEDIA_TYPE_NUMBER];
	unsigned long   ulSSRC[MEDIA_TYPE_NUMBER];
	unsigned long   ulClientAddress;
	//20080620 for recording client port
	WORD			wClientPort;
#ifdef _INET6
	struct sockaddr_in6	 tClientSckAddr;
#endif

	char            acRecvBuffer[MAX_RECVBUFF];   // buffer to recv rtsp msg
	int             iRecvSize;                    // msg size received so far
	char            acBase64Buffer[MAX_RECVBUFF]; // buffer to store base64 encoded rtsp msg
	int             iUnfinishedRTSPSize;            // decoded unfinished RTSP msg size
	
	char            acServerName[RTSP_URL_LEN - RTSP_URL_ACCESSNAME_LEN];
	char            acObjURL[RTSP_URL_LEN];
	char            acObject[RTSP_URL_ACCESSNAME_LEN];
	char			acSessionCookie[RTSP_HTTP_COOKIE_LEN];
	char			acExtraInfo[RTSP_URL_EXTRA_LEN];

	int             iRTPStreamingMode; //RTP_OVER_UDP or RTP_OVER_TCP
	//20120723 modified by Jimmy for metadata
	int             iEmbeddedRTPID[MEDIA_TYPE_NUMBER];
	int             iEmbeddedRTCPID[MEDIA_TYPE_NUMBER];

    int             iSymRTP;                   // symetric RTP

	//20120723 modified by Jimmy for metadata
	RTSP_SOCKADDR NATRTPAddr[MEDIA_TYPE_NUMBER];

    HANDLE          hTCPMuxCS;

	int				iMulticast;			 // which multicast group is used. 0 means no multicast
	//20130830 added by Charles for ondemand multicast
	MULTICASTINFO   tOndemandMulticastInfo;
	bool			bNewMulticast;		   // TRUE means new streaming
	
	int				iVivotekClient;      // TRUE means this client is capable of decoding RTP extension of vivotek
    int             iPlayerType;
#ifndef _SHARED_MEM
	//20091116 support connected UDP
	int				iFixedUDPSourcePort; // For vivotek client and handset with RTSP porxy solution
#endif
	int				iSDPIndex;

	//20120925 added by Jimmy for ONVIF backchannel
	int				iRequire;
	int				iChannelIndex;
	bool			bAudioback;

	DWORD			dwBaseMSec;
	//20080611 added by Louis for client management 
	char			acUserName[RTSP_USERNAME_LENGTH];
	bool			bManagedbySessionMgr;
	//20081121 URL authentication
	char			acPassWord[RTSP_PASSWORD_LENGTH];
	bool			bURLAuthenticated;
	           	
#ifdef _SHARED_MEM
	/* Indicate the client history streaming mode */
	EHSMode				eHSMode;
	/* Indicate number of milli-seconds offset */
	DWORD				dwBypasyMSec;
	/* Indicate the reference time */
	DWORD				dwDescribeMSec;
	/* Indicate the minimum timeshift required */
	DWORD				dwMinSftMSec;
	/* Specific string reply for vivotek share memory client */
	char				acShmemReturnString[RTSPPARSER_REPLY_STRING_LENGTH];
	//20100812 Added For Client Side Frame Rate Control
	int 				iFrameIntervalMSec ;
	//added by neil 10/12/29
	ESVCMode			eSVCMode;
	//added by neil 11/01/14
	BOOL				bForceFrameInterval;
    DWORD				dwFrameInterval;
#endif

#ifdef _SESSION_MGR
	TSessMgrStreamInfo	tSessMgrInfo; 
#endif
	//20090321 for multiple profile
	int					iOrigSDPIndex;

#ifdef _SHARED_MEM
	//20100105 Added For Seamless Recording
	bool				bSeamlessStream;				//Indicate stream for seamless
	char				acSeamlessRecordingGUID[RTSPS_Seamless_Recording_GUID_LENGTH];
	bool				bNormalDisconnected;			//Normal: Teardown or session remove, Abnormal: Others
	int					iSeamlessRecordingSession;		//GUID list index
	TLastFrameInfo		tLastFrameInfo;					//Reserve Last Frame Information from Control Layer SessionList

	/* 20100402 Media on demand */
	TMODInfo			tMODInfo;
	BOOL				bMediaOnDemand;		
#endif
	//20151111 workaround, for cache mod pause state
	int 				iIsPause;
	
	//20160603 add by faber , record client index
	int 				iClientIndex;
	//20160818 Modify by Faber, each client has specify nonce
	char			acAuthenNonce[NONCE_LENGTH];
	struct rtsp_server
	{
		struct rtsp_client  *pClient;
		unsigned long       ulTaskID;
		RTSPSERVER_PARAM    rtsp_param;
		char                acSendBuffer[MAX_SNDBUFF];
		unsigned short      usSendSize;

		//20120925 added by Jimmy for ONVIF backchannel
		int                 iAudiobackSock[MULTIPLE_CHANNEL_NUM][2];// for rtp/rtcp socket (audioback)
		unsigned long       ulAudiobackSessionID[MULTIPLE_CHANNEL_NUM];
		DWORD               dwLastRecvTimeofAudioback[MULTIPLE_CHANNEL_NUM];

		HANDLE              hParentHandle;
		RTSPSERVERCALLBACK  fcallback;

		unsigned long       ulDeleteQueue;
		HANDLE				hTeardownOKQueue;
		HANDLE              hHttpSockQueue;
		HANDLE				hHttpInfoBuffQueue;
		
#ifdef _SHARED_MEM
		/* 20100105 Added For Seamless Recording */
		HANDLE				hResetGUIDQueue;
#endif
		int					iMaxSessionNumber;
		int                 iCurrentSessionNumber;
		// Added by cchuang, 2005/05/23, for normally stop thread
		int                 iTerminateThread;
		int                 iRunning;

#ifdef WISE_SPOT_AUTHENTICATE
		DWORD				dwAuthenticateIP;
#endif

#ifdef 	RTSPRTP_MULTICAST
		//20130327 modified by Jimmy to support different video/audio multicast addresses for ONVIF Test Tool 12.12
		//20130902 modified by Charles for ondemand multicast
		unsigned long	ulMulticastAddress[RTSP_MULTICASTNUMBER + RTSP_ONDEMAND_MULTICASTNUMBER];
		unsigned long	ulMulticastVideoAddress[RTSP_MULTICASTNUMBER + RTSP_ONDEMAND_MULTICASTNUMBER];
		unsigned long	ulMulticastAudioAddress[RTSP_MULTICASTNUMBER + RTSP_ONDEMAND_MULTICASTNUMBER];
		unsigned long	ulMulticastMetadataAddress[RTSP_MULTICASTNUMBER + RTSP_ONDEMAND_MULTICASTNUMBER];

		unsigned short	usMulticastVideoPort[RTSP_MULTICASTNUMBER + RTSP_ONDEMAND_MULTICASTNUMBER];
		unsigned short	usMulticastAudioPort[RTSP_MULTICASTNUMBER + RTSP_ONDEMAND_MULTICASTNUMBER];
		//20120723 modified by Jimmy for metadata
		unsigned short	usMulticastMetadataPort[RTSP_MULTICASTNUMBER + RTSP_ONDEMAND_MULTICASTNUMBER];
		unsigned short  usTTL[RTSP_MULTICASTNUMBER + RTSP_ONDEMAND_MULTICASTNUMBER];
		int             iRTPExtension[RTSP_MULTICASTNUMBER + RTSP_ONDEMAND_MULTICASTNUMBER];
		int             iMulticastSDPIndex[RTSP_MULTICASTNUMBER + RTSP_ONDEMAND_MULTICASTNUMBER];

		int             iMaxMulticastNumber[RTSP_MULTICASTNUMBER + RTSP_ONDEMAND_MULTICASTNUMBER];
		int             iCurrMulticastNumber[RTSP_MULTICASTNUMBER + RTSP_ONDEMAND_MULTICASTNUMBER];

		//20130912 added by Charles for ondemand multicast
        //int            	iAddMulticast[RTSP_ONDEMAND_MULTICASTNUMBER];

		int             iMaxUnicastNumber;		
		int             iCurrUnicastNumber;
		//20110630 Add by danny For Multicast enable/disable
		int             iMulticastEnable[RTSP_MULTICASTNUMBER + RTSP_ONDEMAND_MULTICASTNUMBER];
#endif
		
		int				iAuthenticationType;
		
#ifndef _SHARED_MEM
		//20091116 support connected UDP
		int				iBehindNAT;
#endif
#ifdef	_SIP_TWO_WAY_AUDIO
		unsigned long	ulSSRCofUpStreamAudio;
		unsigned long	ulSessionIDofUpStreamAudio;
		DWORD	dwLastRecvTimeofUpStreamAudio;
		DWORD	dwSuccessiveTimeout ;
#endif
#ifdef _SHARED_MEM
		/* Indicate number of seconds protected by Save-bandwidth feature */
		int				dwProtectedDelta;
#endif
		//20081121 for URL authentication
		int				iURLAuthEnabled;
		/* 20090225 QOS */
		TQosInfo		tQosInfo;

#ifdef _SHARED_MEM
		/* 20100105 Added For Seamless Recording */
		TSeamlessRecordingInfo		tSeamlessRecordingInfo;
		/* 20100428 Added For Media on demand */
		int				iMODConnectionNumber;
		bool			bMODStreamEnable[MOD_STREAM_NUM];
		//20170524
		int 			aiModStream[MOD_STREAM_NUM];
#endif
#ifdef _SESSION_MGR
		/* 20100623 danny, Added for fix sessioninfo corrupted issue */
		HANDLE          hSessionMgrHandle;
#endif
	} *parent;

}RTSP_CLIENT;

/*! Added to distinguish the message */
typedef enum {
	eDescribeMethod = 1,
	eSetupMethod = 2,
	ePlayMethod
} EParseMethod;


typedef struct rtsp_server RTSP_SERVER;

void RTSPServer_MessageHandler(RTSP_CLIENT* pClient,RTSP_SERVER *pServer);
int  RTSPServer_SendReply( int iErr, char *addon, RTSP_CLIENT *pClient,RTSP_SERVER* pServer);
void RTSPServer_InitClient(RTSP_CLIENT *pClient);
int  RTSPServer_SessionStart(RTSP_CLIENT *pClient,RTSPSERVER_SESSIONINFORMATION *pSessionInfo);
int  RTSPServer_SessionStop(RTSP_SERVER *pRTSPServer,DWORD dwSessionID, int iMulticastFlag);
int  RTSPServer_GetSDP(RTSP_SERVER *pRTSPServer,RTSPSERVER_SDPREQUEST *pstSDPRequest);
int  RTSPServer_UpdateRTPSession(RTSP_SERVER *pRTSPServer,RTSPSERVER_SESSIONINFORMATION *pSessionInfo);
int  RTSPServer_SessionPause(RTSP_CLIENT *pClient,DWORD dwSessionID);
int  RTSPServer_SessionResume(RTSP_CLIENT *pClient,DWORD dwSessionID);
int	RTSPServer_ComposeSDP(RTSP_SERVER *pRTSPServer,RTSPSERVER_SDPREQUEST *pstSDPRequest);
int	RTSPServer_ResetCI(RTSP_SERVER *pRTSPServer, int iSDPIndex);
#ifdef _SHARED_MEM
/* 20100105 Added For Seamless Recording */
SCODE RTSPServer_CheckSeamlessAllGUIDsBack(RTSP_SERVER *pServer);
#endif

/*void Init_Client(RTSP_CLIENT *client);
int  IP_Check(RTSP_SERVER *server,RTSPSERVER_CLIENTIP *client_ip);
int  Get_SDP(RTSP_SERVER *server,char *sdp_name,char *sdp);
int  Session_Start(RTSP_SERVER *server,RTSPSERVER_SESSIONINFORMATION *session_info);
int  Session_Stop(RTSP_SERVER *server,DWORD Session_ID);*/

#endif	/* _rtsp_server_local_h */


