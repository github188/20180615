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
 *  Module name         :   RTSPStreaming.h
 *  Module description  :   Handle RTSP request and media streaming action
 *  Author              :   Simon
 *  Created at          :   2002/4/24
 *  Revision            :   1.0
 ******************************************************************************
 *                        Revision history
 ******************************************************************************
 */

#ifndef _RTSPSTREAMING_H_
#define _RTSPSTREAMING_H_

#include "rtspstreamingserver.h"
#ifndef _SHARED_MEM
#include "rtpmediaqueue.h"
#include "rtppacketizer.h"
#endif
#include "rtprtcp.h"
#include "rtspserver.h"
#include "mediachannel.h"
#include "rtpmediabuf.h"

#ifdef _SIP
#include "encrypt_vivostrcodec.h"
#include "sipua.h"
#include "sdpdecoder.h"
#endif

//20140819 added by Charles for eventparser API
#include "xmlprocessor.h"
#include "onvifbasic.h"


#define SERVER_PRIORITY         120

#define	RTSPSTREAMING_IPACCESSCHECK_MAX_IPLIST		20

//#define RTSPSTREAMING_AUDIOBUFFER_MAX_NUMBER		10
//#define RTSPSTREAMING_AUDIOBUFFER_SIZE				160
#define RTSPSTREAMING_AMR_OCTET_ALIGN				1
#define RTSPSTREAMING_AMR_MODECHANGE_PERIOD			0
#define RTSPSTREAMING_AMR_MODECHANGE_NEIGHBOR		0
#define RTSPSTREAMING_AMR_CRC						0
#define RTSPSTREAMING_AMR_INTERLEAVING				0
#define RTSPSTREAMING_AMR_ROBUST_SORTING			0


#define RTSPSTREAMING_AMR_CLOCKRATE					8000

#define RTSPSTREAMING_G711_ULAW_MEDIATYPE			101
#define RTSPSTREAMING_G711_CLOCKRATE				8000

#define RTSPSTREAMING_MPEG4_CLOCKRATE				1000

#define RTSPSTREAMING_AUDIO_MEDIATYPE				RTSPSTREAMING_AMR_MEDIATYPE
#define RTSPSTREAMING_VIDEO_MEDIATYPE				RTSPSTREAMING_MPEG4_MEDIATYPE



//#define RTSPSTREAMING_NO_MEDIA                       0
#define RTSPSTREAMING_VIDEO_ONLY                     1
#define RTSPSTREAMING_AUDIO_ONLY                     2
#define RTSPSTREAMING_BOTH_MEDIA                     3

/* 20100225 fixed sessioncookie size, unify to use RTSP_HTTP_COOKIE_LEN */
//#define RTPSTREAMING_SESSIONCOOKIE_SIZE              32+1
#define	RTPSTREAMING_SDP_LENGTH						RTSPSTREAMING_SDP_MAXSIZE

#ifdef _SIP
#define	RTSPSTREAMING_SIP_SESSION					100
#endif
#define RTSPSTREAMING_RTSP_SESSION					101

#ifdef 	RTSPRTP_MULTICAST
//20100714 Modified by danny For Multicast parameters load dynamically
#define RTSPSTREAMING_MULTICAST_ADDWAITING_INTERVAL_MSEC	200
#define RTSPSTREAMING_MULTICAST_ADDWAITING_COUNT			15
#endif

//20140819 added by Charles for eventparser API
#define METADATA_CONF_FILE			"/etc/conf.d/config_onvif/config_onvif_metadata.xml"
#define MEDIA_PROFILE_FILE			"/etc/conf.d/config_onvif/config_onvif_media_profile.xml"
#define MAX_NO_OF_PROFILES			8
#define MAX_XML_ELEM_NUM              2048

typedef struct tagRTSPSTREAMING_MULTICAST
{
	unsigned long	ulMulticastAddress;
	//20160127 Add audio and metadata address for multicast
	unsigned long 	ulMulticastAudioAddress;
	unsigned long 	ulMulticastMetadataAddress;

	unsigned short	usMulticastVideoPort;
	unsigned short	usMulticastAudioPort;
	//20120801 added by Jimmy for metadata
	unsigned short	usMulticastMetadataPort;
	unsigned short  usTTL;

	HANDLE			hRTPRTCPAudioComposerHandle;
	HANDLE			hRTPRTCPVideoComposerHandle;
	//20120801 added by Jimmy for metadata
	HANDLE			hRTPRTCPMetadataComposerHandle;

	int             iAlreadyMulticastVideo;
	int             iAlreadyMulticastAudio;
	//20120801 added by Jimmy for metadata
	int             iAlreadyMulticastMetadata;	
	int             iAlwaysMulticast;
	/* Added by Louis 2008/01/29 for DVTEL multicast audio / video only */
	int				iAlwaysMulticastAudio;
	int				iAlwaysMulticastVideo;
	//20120801 added by Jimmy for metadata
	int				iAlwaysMulticastMetadata;

	//20120801 modified by Jimmy for metadata
	int				aiMulticastSocket[MEDIA_TYPE_NUMBER*2];  //1:video rtp; 2:video rtcp; 3:audio rtp; 4: audio rtcp; 5:metadata rtp; 6:metadata rtcp;
	int 			iRTPExtension;
	int				iSDPIndex;
#ifdef _SHARED_MEM
	/* Shared memory information*/
	//TShmemSessionInfo	tShmemSessionInfo;
	HANDLE				hShmemSessionInfo;
#endif
	//20110630 Add by danny For Multicast enable/disable
	int				iEnable;
	//20110725 Add by danny For Multicast RTCP receive report keep alive
	int             iRRAlive;

}RTSPSTREAMING_MULTICAST;

typedef struct
{
    int     iSendSock;
    int     iRecvSock;
    char    acSessionCookie[RTSP_HTTP_COOKIE_LEN];
    int     iCheckCount;
}TRTP_OVER_HTTPINFO;

typedef struct tagRTSPSTREAMING_SESSION
{
	DWORD				dwSessionID;
	unsigned long		ulClientIP;
#ifdef _INET6
	struct sockaddr_in6	 tClientSckAddr;
#endif
	BYTE				byMediaStatus;
	int					iSDPIndex;
	
	HANDLE				hRTPRTCPAudioComposerHandle;
	HANDLE				hRTPRTCPVideoComposerHandle;
	//20120801 added by Jimmy for metadata
	HANDLE				hRTPRTCPMetadataComposerHandle;
	HANDLE				hTCPMuxCS;

	int					iSessionType;
#ifdef _SHARED_MEM
	/* Shared memory information*/
	//TShmemSessionInfo	tShmemSessionInfo;
	HANDLE				hShmemSessionInfo;
	//20101208 Modified by danny For GUID format change
	/* 20100105 Added For Seamless Recording */
	//DWORD  				dwSessionGUID;
	char				acSeamlessRecordingGUID[RTSPS_Seamless_Recording_GUID_LENGTH];
#endif

#ifdef 	RTSPRTP_MULTICAST
	//20100720 Added by danny to fix Backchannel multicast session terminated, rtsp server has not stopped sending video/audio RTP/RTCP
	int					iMulticast;
#endif

}RTSPSTREAMING_SESSION;

typedef struct tagRTSPSTREAMING
{
	HANDLE hRTSPServerHandle;
	HANDLE hRTPRTCPChannelVideoHandle;
	HANDLE hRTPRTCPChannelAudioHandle;
	//20120801 added by Jimmy for metadata
	HANDLE hRTPRTCPChannelMetadataHandle;
	
	HANDLE	hVideoDataQueueHandle;
	HANDLE	hVideoEmptyQueueHandle;
	HANDLE	hAudioDataQueueHandle;
	HANDLE	hAudioEmptyQueueHandle;
	//20120801 added by Jimmy for metadata
	HANDLE	hMetadataDataQueueHandle;
	HANDLE	hMetadataEmptyQueueHandle;
	
	HANDLE	hIPAccessCheckHandle;
		
	HANDLE	ulSessionListSemaphore;		
	
	RTSPSTREAMING_SESSION   *pstSessionList;
	TRTP_OVER_HTTPINFO      *pstRTPOverHTTPInfo;
	
	int		iSessionListNumber;
	int     iRTPOverHTTPNumber;
	
	int		iMaximumSessionCount;

	unsigned short usRTSPPort;
#ifdef _SIP
	unsigned short usRTPVideoPort;
	unsigned short usRTPAudioPort;
	//20120801 added by Jimmy for metadata
	unsigned short usRTPMetadataPort;
#endif
	unsigned long  ulLocalIP;
	
	// for NAT problem
	unsigned long	ulLocalSubnetMask;	
	unsigned long	ulNATIP;
	
	int									iVideoSessionNumberOfStreamType[MULTIPLE_STREAM_NUM];
	int                                 iAudioSessionNumberOfStreamType[MULTIPLE_STREAM_NUM];
	//20120801 added by Jimmy for metadata
	int                                 iMetadataSessionNumberOfStreamType[MULTIPLE_STREAM_NUM];
	char								acAccessName[MULTIPLE_STREAM_NUM][ACCESSNAME_LENGTH]; 
	int									iRTSPStreamingMediaType[MULTIPLE_STREAM_NUM];
	TRTSPSTREAMING_AUDENCODING_PARAM	tAudioEncodeParam[MULTIPLE_STREAM_NUM];
	TRTSPSTREAMING_VIDENCODING_PARAM	tVideoEncodeParam[MULTIPLE_STREAM_NUM];
	//20120801 added by Jimmy for metadata
	TRTSPSTREAMING_METADATAENCODING_PARAM	tMetadataEncodeParam[MULTIPLE_STREAM_NUM];
	//20120925 added by Jimmy for ONVIF backchannel
	TRTSPSTREAMING_AUDDECODING_PARAM	tAudioDecodeParam[MULTIPLE_CHANNEL_NUM];


	char acHostName[20]; //max number is 12
	char acSDPETag[60];	// max number is 60, 40(serial number)+20(MAC)
	
	/*int		iAudioCodecType;  
	int     iAudioBitRate;      //1.5M == 15000, 33.6k=33600
    int     iAudioClockRate;    //8000
    int     iAMRPacketTime;   //200
    int     iAMROctetAlign;   //1
    int     iAMRcrc;          //0
    int     iAMRRobustSorting;//0  
	BYTE	abyM4AHeader[16];
	int		iM4AHeaderLen;
	int		iM4AProfileLevel;
	int		iChanNum;

	BYTE	abyMPEG4Header[100];
	int		iMPEG4HeaderLength;
	int		iMPEG4ProfileLevel;
	int     iMPEG4ClockRate;
	int 	iVideoBitRate;
	int		iVideoWidth;
	int		iVideoHeight;
	int		iVideoDecoderBufferSize;*/


	char	acSDPContent[RTPSTREAMING_SDP_LENGTH];
	int		iSDPContentLength;
	
	FControlChannel_Callback  fControlCallBack;
	MEDIA_CALLBACK            fVideoCallBack;
	MEDIA_CALLBACK            fAudioCallBack;
	//20120801 added by Jimmy for metadata
	MEDIA_CALLBACK            fMetadataCallBack;

#ifdef _SHARED_MEM
	MEDIA_CALLBACK            fShmemVideoCallBack;
	MEDIA_CALLBACK            fShmemAudioCallBack;
	//20120801 added by Jimmy for metadata
	MEDIA_CALLBACK			  fShmemMetadataCallBack;

#endif
		
	HANDLE  hParentControlHandle;	
	HANDLE  hParentVideoHandle;
	HANDLE  hParentAudioHandle;
	//20120801 added by Jimmy for metadata
	HANDLE  hParentMetadataHandle;
	
	HANDLE  hVideoPacketizer;
	HANDLE  hAudioPacketizer;
	//20120801 added by Jimmy for metadata
	HANDLE  hMetadataPacketizer;

#ifdef _SIP
	HANDLE	hSIPUAHandle;	
	HANDLE	hSDPDecoder;
#endif

/* 20100623 danny, Added for fix sessioninfo corrupted issue */
#ifdef _SESSION_MGR
	HANDLE			hSessionMgrHandle;
#endif

	DWORD           dwRTSPServerPriority;
	DWORD           dwVideoChannelPriority;
	DWORD           dwAudioChannelPriority;
	//20120801 added by Jimmy for metadata
	DWORD           dwMetadataChannelPriority;
	DWORD           dwVideoPacketizerPriority;
	DWORD           dwAudioPacketizerPriority;
	//20120801 added by Jimmy for metadata
	DWORD           dwMetadataPacketizerPriority;

	
#ifdef RTSPRTP_MULTICAST
	//20130327 modified by Jimmy to support different video/audio multicast addresses for ONVIF Test Tool 12.12
	//20130902 modified by Charles for ondemand multicast
	RTSPSTREAMING_MULTICAST stMulticast[RTSP_MULTICASTNUMBER + RTSP_ONDEMAND_MULTICASTNUMBER];
#endif

    HANDLE          hMediaParamSemaphore;

	//UDP mode RTSP server behind NAT sockets
	int				iRTPVSock;
	int				iRTPASock;
	//20120801 added by Jimmy for metadata
	int				iRTPMSock;
	int				iRTCPVSock;
	int				iRTCPASock;
	//20120801 added by Jimmy for metadata
	int				iRTCPMSock;

#ifdef _SHARED_MEM
	HANDLE			ahShmemVideoHandle[VIDEO_TRACK_NUMBER];
	HANDLE			ahShmemAudioHandle[AUDIO_TRACK_NUMBER];
	//20120806 added by Jimmy for metadata
	//20130605 modified by Jimmy to support metadata event
	HANDLE			ahShmemMetadataHandle[METADATA_TRACK_NUMBER][SHMEM_HANDLE_MAX_NUM];
	/* 20100428 Added For Media on demand */
	TRTSPSTREAMING_MODCONTROL_PARAM	tModControlParam[MOD_STREAM_NUM];
#endif

	//20101123 Added by danny For support advanced system log
	BOOL  			bAdvLogSupport;

}RTSPSTREAMING;

/*! For critical section*/
#ifdef _SHARED_MEM
SCODE RTSPCriticalSection_Initial(HANDLE *phObject);
#endif

#endif  //_RTSPSTREAMING_H_




