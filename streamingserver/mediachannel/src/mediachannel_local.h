

#ifndef	_CHANNEL_H
#define _CHANNEL_H

#include "mediachannel.h"
#include "rtprtcp.h"
#include "rtpmediabuf.h"
#ifdef _SHARED_MEM
#include "shmem.h"
#endif

#define CHANNEL_ERR_NOTOPEN				1
#define CHANNEL_ERR_PARAM_NULL			2
#define CHANNEL_ERR_SETCALLBACK_NULL	3


#define	CHANNEL_RUNNING			1
#define	CHANNEL_STOPPING		2
#define	CHANNEL_STOPPED			3

#define CHANNEL_ADD_CONNECTION		11
#define CHANNEL_REMOVE_CONNECTION	12

#define CHANNEL_TYPE_VIDEO	1
#define CHANNEL_TYPE_AUDIO	2

#define SESSION_IDLE        0 
#define SESSION_PAUSED      1 
#define SESSION_PLAYING     2

#define RTPRTCP_REMOVE_SESSION    0
#define RTPRTCP_PAUSE_SESSION     1
#define RTPRTSP_RESUME_SESSION    2
#define RTPRTCP_ADD_SESSION       3 

#ifdef RTSPRTP_MULTICAST
#define RTPRTCP_ADD_MULTICAST     4
#define RTPRTCP_REMOVE_MULTICAST  5

#define RTCP_BUFFER_LENGTH		  100

//20081001 TCP timeout
#define RTPRTCP_TCP_TIMEOUT		  60000
//20101020 Add by danny for support seamless stream TCP/UDP timeout
#define RTPRTCP_SeamlessStream_TCP_TIMEOUT		  10000
#define RTPRTCP_SeamlessStream_MissingRRCount	  2
#define RTPRTCP_MissingRRCount					  10

#ifdef _SHARED_MEM
//20110822 Add by danny for fix alignment trap, if H264 frame size too large cause atIov overflow
#define RTP_MAX_IOV_COMPONENT	  3 //1. Header, 2. Extra data, 3. Bitstream 
#endif

typedef struct
{
	SOCKET	sktRTP;
	SOCKET	sktRTCP;
	HANDLE	hRTPRTCPComposerHandle;	
	int		iStatus;
	int		iVivotekClient;
	int		iCodecIndex;
	//20101018 Add by danny for support multiple channel text on video
	int 	iMultipleChannelChannelIndex;
	/*! Shmem session info */
	TShmemMediaInfo	  *ptShmemMediaInfo;
	//20110627 Add by danny for join/leave multicast group by session start/stop
	unsigned long	ulMulticastAddress;
	struct sockaddr_in RTCPDstAddr;
	//20110725 Add by danny For Multicast RTCP receive report keep alive
	int             iRRAlive;
	DWORD			dwRecvReportTime;
    int             iCSeq;
    int             iCSeqUpdated;    
	
} RTPRTCPCHANNEL_MULTICAST;
#endif

typedef struct
{
	DWORD  dwSessionID;
	//20110706 Modified by danny For TCP/HTTP mode socket not sync in multi thread
	int	   *psktRTP;
	int	   *psktRTCP;
	HANDLE hRTPRTCPComposerHandle;	
	int    iStatus;
	int    iStartRTCPSReport;

	int    iRTPStreamingType;
    int    iEmbeddedRTPID;
    int    iEmbeddedRTCPID;
//    CRITICAL_SEC_PORT csSemaph;
    HANDLE hTCPMuxCS;
	int		iVivotekClient;
	int		iCodecIndex;
	//20101018 Add by danny for support multiple channel text on video
	int 	iMultipleChannelChannelIndex;

	RTSP_SOCKADDR RTPNATAddr;
   	RTSP_SOCKADDR RTCPNATAddr;

	/*! Shmem session info */
	TShmemMediaInfo	  *ptShmemMediaInfo;

	/* Buffer for RTCP sender report */
	char	acRTCPBuf[RTCP_BUFFER_LENGTH];
	int		iRTCPOffset;
	int		iRTCPRemainingLength;

	/* 20090403 Timout */
	DWORD	dwQTTimeoutInitial;

	//20101020 Add by danny for support seamless stream TCP/UDP timeout
	bool				bSeamlessStream;
    //20140819 added by Charles for eventparser API
    int                 nChannelType;
    int                 iCSeq;
    int                 iCSeqUpdated;
    int 				idebug;
} RTPRTCPCHANNEL_SESSION;


typedef struct tagCHANNEL
{   
    RTPRTCPCHANNEL_SESSION *session;
#ifdef RTSPRTP_MULTICAST   
	//20130327 modified by Jimmy to support different video/audio multicast addresses for ONVIF Test Tool 12.12
	//20130830 modified by Charles for ondemand multicast
	RTPRTCPCHANNEL_MULTICAST stMulticast[RTSP_MULTICASTNUMBER + RTSP_ONDEMAND_MULTICASTNUMBER];
#endif
	RTPRTCPCHANNELCALLBACK fCallbackFunct;
	HANDLE           hParentHandle;
	HANDLE           hThread;
	int		         nChannelStatus;
	int		         nChannelType; // video or audio or metadata
	int              iMaxSession;	
	
	char*            pcMp4EncodeParam;
	int              nMp4EncodeLength;
	
	ULONG	         qAddRemoveConnection;
	ULONG            qSessionInfo;
	int		         nAddRemoveonnectionCount;
	int 	         nTotalConnectionCount;

	RTPMEDIABUFFER	 *pRTPTmpMediaBuffer;

	int				iUDPRTPSock;
	int				iUDPRTCPSock;
#ifdef _SHARED_MEM
	//20101018 Add by danny for support multiple channel text on video
	char   acLocation[MULTIPLE_CHANNEL_NUM][LOCATION_LEN];
#endif  
}CHANNEL;

/* for closing session*/
SCODE RTPRTCPChannel_CloseSession(CHANNEL *pChannel, RTPRTCPCHANNEL_SESSION *pSession);
SCODE  RTPRTCPChannel_CloseMulticast(CHANNEL *pChannel, int iMulticastIndex);
/* RTPRTCP compose*/
int	RTPRTCPChannel_ComposeEmbeddedRTPInfo(char* pcBuffer, int iIdentifier, int iLength);
/* for RTCP sender report */
int SendRTCPSenderReport(RTPRTCPCHANNEL_SESSION *pSession,HANDLE hRTPRTCPComposerHandle,CHANNEL *pChannel);
/* for critical section */
SCODE RTPRTCPCs_TryEnter(RTPRTCPCHANNEL_SESSION *pSession, ECritSecStatus eStatus);
SCODE RTPRTCPCs_Leave(RTPRTCPCHANNEL_SESSION *pSession, ECritSecStatus eStatus);
SCODE RTPRTCPCs_Release(RTPRTCPCHANNEL_SESSION *pSession, ECritSecStatus eStatus);

SCODE RTPRTCPCs_GetLockStatus(RTPRTCPCHANNEL_SESSION *pSession, ECritSecStatus* eStatus);
#ifdef _SHARED_MEM
/* for share memory */
SCODE  ChannelShmReleaseBuffer(CHANNEL *pChannel);
SCODE  ChannelShmSelectError(CHANNEL *pChannel);
SCODE  ChannelShmemCheckTimeout(CHANNEL *pChannel);
SCODE  ChannelShmemRequestBuffer(CHANNEL *pChannel);
SCODE  ChannelShmemCutBuffer(CHANNEL *pChannel);
SCODE  ChannelShmResetMediaInfo(TShmemMediaInfo *pMediaInfo);
#ifdef RTSPRTP_MULTICAST
SCODE  ChannelShmemMulticastComposeHeader(CHANNEL *pChannel);
#endif
SCODE  ChannelShmemComposeHeader(CHANNEL *pChannel);
SCODE  ChannelShmemSend(CHANNEL *pChannel);
#endif

#endif  //_CHANNEL_H


