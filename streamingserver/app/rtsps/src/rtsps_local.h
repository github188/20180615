/*
 *******************************************************************************
 * $Header: $
 *
 *  Copyright (c) 2000-2006 Vivotek Inc. All rights reserved.
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
 * $History: $
 * 
 *******************************************************************************
 */

/*!
 *******************************************************************************
 * Copyright 2000-2006 Vivotek, Inc. All rights reserved.
 *
 * \file
 * rtsps_local.h
 *
 * \brief
 * rtsp streaming server for Kilrogg (local include file).
 *
 * \date
 * 2006/04/21
 *
 * \author
 * Rey Cheng
 *
 *
 *******************************************************************************
 */

#ifndef _RTSPS_LOCAL_H_
#define _RTSPS_LOCAL_H_

#include <stdlib.h>
#include <stdio.h>
#include "rtsps.h"

#ifdef _SHARED_MEM
#include "shmem.h"
#endif

//20140819 added by Charles for eventparser API
#ifdef _METADATA_ENABLE
#include "eventparser.h"
#endif

/* socket that receive video and audio stream */
#define VSTREAM_SOCK_DIR			"/tmp/venc/"
#define ASTREAM_SOCK_DIR			"/tmp/aenc/"
/* socket communicates with http server */
#define HTTP_FDIPC_SOCK				"/tmp/httpfdipc.sck"

/* full path name of SDP file*/
#define FILE_NAME_SDP1              "/tmp/livea.sdp"
#define FILE_NAME_SDP2              "/tmp/liveb.sdp"

#define SDP_PATH					"/tmp/"

#ifdef _SHARED_MEM
//20100105 Added For Seamless Recording
#define SEAMLESSRECORDING_CONF						"/etc/conf.d/config_seamlessrecording.xml"
#define SEAMLESSRECORDING_DISK_CONF					"/etc/conf.d/config_seamlessrecording.xml"
#define SEAMLESSRECORDING_CAPABILITY_CONF			"/etc/conf.d/config_seamlessrecording.xml"
#define SEAMLESSRECORDING_DISKMODE_XPATH			"/root/seamlessrecording/diskmode"
#define SEAMLESSRECORDING_MAXCONNECTION_XPATH		"/root/seamlessrecording/maxconnection"
#define SEAMLESSRECORDING_STREAM_XPATH				"/root/seamlessrecording/stream"
#define SEAMLESSRECORDING_ENABLE_XPATH				"/root/seamlessrecording/enable"
#define SEAMLESSRECORDING_MAXCONNECTION_DEFAULT		1
#define SEAMLESSRECORDING_STREAM_DEFAULT			1
#endif
//20120830 modified by Jimmy for metadata
/*
#define MEDIA_TYPE_VIDEO            123
#define MEDIA_TYPE_AUDIO            456
*/
#define UBUFFER_PROCESS_PRIORITY	120
#define UBUFFER_PROCESS_STACKSIZE	16384

#define VIDEO_UBUFFER_SIZE			64*1024
#define AUDIO_UBUFFER_SIZE			MAX_CONNECT_NUM*1024
//20120731 added by Jimmy for metadata
#define METADATA_UBUFFER_SIZE		MAX_CONNECT_NUM*1024


#define PRIORITY_STREAMCONTROL		145
#define MAX_BITSTREAM_SIZE			(262144 + 8192)

#define SDP_FULL_PATH_NAME_LEN      255
#define MEDIA_TRACK_NAME_LEN        30

#define	IPFILTER_NUMBER				10

#define RTSPS_MESSAGE_NUMBER		10

/* Modified by Louis 20081119*/
#define CONTROL_MSG_START           "<output>start</output></control>"
#define CONTROL_MSG_STOP            "<output>stop</output></control>"
//20160623 Add by Faber, stop MOD before set parameter
#define CONTROL_MSG_PAUSE			"<output>pause</output></control>"
#define CONTROL_MSG_RESUME			"<output>resume</output></control>"
#define CONTROL_MSG_FORCECI         "<forceCI/></control>"
#define CONTROL_MSG_FORCEINTRA      "<forceIntra/></control>"
#ifdef _SHARED_MEM
/* 20100428 Added For Media on demand */
#define CONTROL_MSG_MOD_TRICKPLAY   "</trickplay></control>"
#endif

//20100714 Modified by danny For Multicast parameters load dynamically
/*#define StreamingServer_FillInfo(apParam, ptStreamInfo, ptVideoSrcInfo, ptMulticastinfo) \
		apParam[0] = ptVideoSrcInfo->acSockPathName; \
		apParam[1] = ptVideoSrcInfo->acFIFOPathName; \
		apParam[2] = &ptStreamInfo->iEnable; \
		apParam[3] = &ptStreamInfo->szAccessName; \
		apParam[4] = &ptStreamInfo->iVideoSrcIndex; \
		apParam[5] = &ptStreamInfo->iAudioSrcIndex; \
		apParam[6] = &ptMulticastinfo->iAlwaysMulticast;\
		apParam[7] = &ptMulticastinfo->usMulticastVideoPort; \
		apParam[8] = &ptMulticastinfo->usMulticastAudioPort; \
		apParam[9] = &ptMulticastinfo->ulMulticastAddress; \
		apParam[10] = &ptMulticastinfo->usTTL; \
		apParam[11] = &ptMulticastinfo->iRTPExtension;*/

//20120816 modified by Jimmy for metadata
#define StreamingServer_FillInfo(apParam, ptStreamInfo, ptVideoSrcInfo) \
		apParam[0] = ptVideoSrcInfo->acSockPathName; \
		apParam[1] = ptVideoSrcInfo->acFIFOPathName; \
		apParam[2] = &ptStreamInfo->iEnable; \
		apParam[3] = &ptStreamInfo->szAccessName; \
		apParam[4] = &ptStreamInfo->iVideoSrcIndex; \
		apParam[5] = &ptStreamInfo->iAudioSrcIndex; \
		apParam[6] = &ptStreamInfo->iMetadataSrcIndex;

#define StreamingServer_FillAudioMediaInfo(apParam, ptStreamInfo, ptAudioSrcInfo) \
		apParam[0] = ptAudioSrcInfo->acSockPathName; \
		apParam[1] = ptAudioSrcInfo->acFIFOPathName;

//20120810 added by Jimmy for metadata
#define StreamingServer_FillMetadataMediaInfo(apParam, ptStreamInfo, ptMetadataSrcInfo) \
		apParam[0] = ptMetadataSrcInfo->acSockPathName; \
		apParam[1] = ptMetadataSrcInfo->acFIFOPathName;

#ifdef _SHARED_MEM
//20101210 Added by danny For Media shmem config
#define StreamingServer_FillVideoShmemInfo(apParam, ptVideoSrcInfo) \
		apParam[0] = &ptVideoSrcInfo->iBlockIndex; \
		apParam[1] = &ptVideoSrcInfo->iSectorIndex;
#define StreamingServer_FillAudioShmemInfo(apParam, ptAudioSrcInfo) \
		apParam[0] = &ptAudioSrcInfo->iBlockIndex; \
		apParam[1] = &ptAudioSrcInfo->iSectorIndex;
//20120810 added by Jimmy for metadata
#define StreamingServer_FillMetadataShmemInfo(apParam, ptMetadataSrcInfo) \
		apParam[0] = &ptMetadataSrcInfo->iBlockIndex; \
		apParam[1] = &ptMetadataSrcInfo->iSectorIndex;
#endif

#ifdef RTSPRTP_MULTICAST
//20110725 Add by danny For Multicast RTCP receive report keep alive
//20110630 Add by danny For Multicast enable/disable
//20100714 Added by danny For Multicast parameters load dynamically
//20120810 modified by Jimmy for metadata
//20160127 add by Faber, for audio/metadata address
#define StreamingServer_FillMulticastInfo(apParam, ptMulticastinfo) \
		apParam[0] = &ptMulticastinfo->iEnable;\
		apParam[1] = &ptMulticastinfo->iRRAlive;\
		apParam[2] = &ptMulticastinfo->iAlwaysMulticast;\
		apParam[3] = &ptMulticastinfo->usMulticastVideoPort; \
		apParam[4] = &ptMulticastinfo->usMulticastAudioPort; \
		apParam[5] = &ptMulticastinfo->usMulticastMetadataPort; \
		apParam[6] = &ptMulticastinfo->ulMulticastAddress; \
		apParam[7] = &ptMulticastinfo->ulMulticastAudioAddress; \
		apParam[8] = &ptMulticastinfo->ulMulticastMetadataAddress; \
		apParam[9] = &ptMulticastinfo->usTTL; \
		apParam[10] = &ptMulticastinfo->iRTPExtension;
#endif

#ifdef _SHARED_MEM
//20100105 Added For Seamless Recording
#define StreamingServer_FillGUIDListInfo(apParam, ptGUIDListInfo) \
		apParam[0] = ptGUIDListInfo->acGUID; \
		apParam[1] = &ptGUIDListInfo->iNumber;

/* 20100428 Added For Media on demand */
#define INT_TO_HEX(x) 				( ((x)>9) ? (('a'-10)+(x)):('0'+(x)) )
#define NEEDS_ESCAPE_BITS 			128
#ifndef NEEDS_ESCAPE_SHIFT
#define NEEDS_ESCAPE_SHIFT 			5    /* 1 << 5 is 32 bits */
#endif
#define NEEDS_ESCAPE_WORD_LENGTH 	( 1 << NEEDS_ESCAPE_SHIFT )
#define NEEDS_ESCAPE_INDEX(c) 		( (c) >> NEEDS_ESCAPE_SHIFT )
#define NEEDS_ESCAPE_MASK(c)  		( 1 << ((c) & (NEEDS_ESCAPE_WORD_LENGTH - 1)) )
#define needs_escape(c) 			( (c) >= NEEDS_ESCAPE_BITS || (c) == 38 || _needs_escape[NEEDS_ESCAPE_INDEX(c)] & NEEDS_ESCAPE_MASK(c) )
extern unsigned long    			_needs_escape[(NEEDS_ESCAPE_BITS + NEEDS_ESCAPE_WORD_LENGTH - 1) / NEEDS_ESCAPE_WORD_LENGTH];
#endif

extern char	acVideoTrackName[VIDEO_TRACK_NUMBER][MEDIA_TRACK_NAME_LEN]; 
extern char	acAudioTrackName[AUDIO_TRACK_NUMBER][MEDIA_TRACK_NAME_LEN];
//20120810 added by Jimmy for metadata
extern char	acMetadataTrackName[METADATA_TRACK_NUMBER][MEDIA_TRACK_NAME_LEN];

              
extern int g_iSaveBandwidth ;
              
typedef struct              
{
	unsigned long 	ulStartIP;
	unsigned long	ulEndIP;
}TIPFILTER;
                  
typedef struct
{
    unsigned short      usPort;
    unsigned long       ulRegistrarIP;
    unsigned short      usRegistrarPort;
    unsigned long       ulOBProxyIP;
    unsigned short      usOBProxyPort;
    char                szDisplayName[255];
    char                szUserName[255];
    char                szPassword[255];
    char                szSIPDomain[255];
    int                 iAuthenticateMode;
    int                 iMaxConnectionNum;
    char                szSessionInfoPath[255];  
}TSIPINFO;                  
        
typedef struct
{
	unsigned short 		usRTSPPort;
    int                 iAuthenticateMode;
    int                 iMaxConnectionNum;
    char                szSessionInfoPath[255];  
    char                szHTTPfdipcSock[255];
    char                szContorlipcFIFO[255];    
    int					iControlFIFO;
	//20081121 for URL authentication
	int					iURLAuthEnabled;
}TRTSPINFO;                                   

typedef struct
{
	unsigned short		usRTPVideoPort;
	unsigned short		usRTPAudioPort;
	//20120801 added by Jimmy for metadata
	unsigned short		usRTPMetadataPort;
}TRTPINFO;


typedef struct
{
    // the socket(media source), 20080925 added socket path name
    int     iFdSock;   
	char	acSockPathName[255];
    // the index used to fill in each packets while packetizing
    int     iMediaIndex;
    // the track name used to mapping to media index
    char    acTrackName[MEDIA_TRACK_NAME_LEN];
    // the number of stream/SDP which uses it
    int     iNumberOfWorkingSDP;
	// the number of frames in one UBuffer (used for AMR)
	int		iFramesPerUBuffer;
	// the  fifo to notify media track action
	int     iFdFIFO;
	char	acFIFOPathName[255];

#ifdef _SHARED_MEM
	//20130605 modified by Jimmy to support metadata event
	HANDLE ahSharedMem[SHMEM_HANDLE_MAX_NUM];
	//20101210 Added by danny For Media shmem config
	int     iBlockIndex;
	int     iSectorIndex;
#endif
	HANDLE	hMediaSrcMutex;
}TMEDIASRCINFO;

typedef struct
{
    // the socket(media decoder)
    int     iFdSock;   
   	char	acSockPathName[255];

    // fifo to notify decoder
	int     iFdFIFO;	 
	char	acFIFOPathName[255];
}TMEDIADSTINFO;

typedef struct
{
    int             iEnable;                
    char            szSDPFullPathName[255];
	//CID:1082, CHECKER: STRING_OVERFLOW
    char            szAccessName[ACCESSNAME_LENGTH];
    int             iVideoSrcIndex;
    int             iAudioSrcIndex;
	//20120731 added by Jimmy for metadata
    int             iMetadataSrcIndex;
}TSTREAMINFO;

typedef struct rtsp_info
{
	DWORD				dwStreamNumber;
	//DWORD               dwRTSPPort;

	//20101123 Added by danny For support advanced system log 
	BOOL  				bAdvLogSupport;
	
	//20100728 Added by danny For multiple channels videoin/audioin
	DWORD				dwChannelNumber;
	DWORD				dwPerChannelStreamNumber;
	
	TSIPINFO            tSIPInfo;
	TRTSPINFO           tRTSPInfo;
	TSTREAMINFO         tStreamInfo[MULTIPLE_STREAM_NUM];
	
	TMEDIASRCINFO       tVideoSrcInfo[VIDEO_TRACK_NUMBER];
	TMEDIASRCINFO       tAudioSrcInfo[AUDIO_TRACK_NUMBER];
	//20120731 added by Jimmy for metadata
	TMEDIASRCINFO       tMetadataSrcInfo[METADATA_TRACK_NUMBER];
	
	TMEDIADSTINFO		tAudioDstInfo;
	//20130327 modified by Jimmy to support different video/audio multicast addresses for ONVIF Test Tool 12.12
	MULTICASTINFO       tMulticastInfo[RTSP_MULTICASTNUMBER];
    TRTPINFO            tRTPInfo;
    
    TIPFILTER			tAllowIP[IPFILTER_NUMBER];
    TIPFILTER			tDenyIP[IPFILTER_NUMBER];
    
	char				szIPAddr[20];
	char				szSubnetMask[20];

	int					iSDPSocket;
	BOOL				bRunning;
 
	/* buffer to receive video and audio stream */
	BYTE				*pbyVideUBuffer;
	BYTE				*pbyAudiUBuffer;
	//20120731 added by Jimmy for metadata
	BYTE				*pbyMetadataUBuffer;

	/* buffer to send audio stream to decoder */
	BYTE				*pbyAudioUploadBuff;
	
	HANDLE				hRTSPServer;
	HANDLE				hSocketExchangerThread;
	TBitstreamBuffer	*pVideBitstreamBuf;
	TBitstreamBuffer	*pAudiBitstreamBuf;
	//20120731 added by Jimmy for metadata
	TBitstreamBuffer	*pMetadataBitstreamBuf;
	
	int                 iGotVideoFlag;
	int                 iGotAudioFlag;
	//20120731 added by Jimmy for metadata
	int                 iGotMetadataFlag;
	
	TAMConfig   		tAMConf;

	/* Added by Louis 2008/01/18 for web-attraction feature */
	BOOL				bWebAttraction;

	// Added by Faber, for flash status
	BOOL 				bFactoryMode;
	/* Added 20080619 for XMLs parser */
	XML_Parser 			pXMLHandle;

#ifdef _SHARED_MEM
	//20100728 Modified by danny For multiple channels videoin/audioin
	HANDLE				hSharedMemVideoBlock[MULTIPLE_CHANNEL_NUM];
	HANDLE				hSharedMemAudioBlock[MULTIPLE_CHANNEL_NUM];
	//20120731 added by Jimmy for metadata
	HANDLE				hSharedMemMetadataBlock[MULTIPLE_CHANNEL_NUM];
	/* 20100428 Added For Media on demand */
	HANDLE				hSharedMemMODBlock;
#endif
	/* Added by Louis 20080829 to merge fdipc thread to app thread */
	TMessageInfo		*ptMsgInfo;
	TStreamServer_ConnectionSettings	tSSConnSettings;

	/* 20090225 QOS */
	char				acQosFilePath[64];
	
	/* 20090626 software watchdog */
	BOOL				bSWatchDogEnabled;
	DWORD				dwSwatchDogLastKick;
	int					iSWatchDogID;
	
	//20110401 Added by danny For support RTSPServer thread watchdog
	DWORD				dwRTSPServerLastKick;
	BOOL				bRTSPServerRestarting;
	int 				aiModStream[MOD_STREAM_NUM];
} TSTREAMSERVERINFO;

SCODE StreamingServer_Initial(HANDLE *phObject, TRTSPSInitOptions *pInitOpts, char* pzConfigFile, char* pzAccessFile, char* pzQosFile) ;
SCODE StreamingServer_Release(HANDLE *phObject) ;
SCODE StreamingServer_Start(HANDLE hObject) ;
SCODE StreamingServer_Stop(HANDLE hObject) ;
void StreamingServer_AccountManagerParse(HANDLE hObject) ;
int	StreamingServer_UpdateDynamicPamater(HANDLE hRTSPS, char* pzConfigFile) ;
SCODE StreamingServer_SetMediaTrackParam(HANDLE hObject, TMultipleStreamCIInfo *ptCIInfo) ;
/* moved here 20091118 */
SCODE StreamSvr_CheckUnixSocket(TSTREAMSERVERINFO *pThis);
SCODE CfgParser_GetUnixDomainSocket(void *pData, void *pParam); //20080925 for unix socket reconnect

#ifdef _SHARED_MEM
/* 20100428 Added For Media on demand */
SCODE StreamingServer_EscapeString(const char *inp, char *buf);
#endif

//20100728 Added by danny For multiple channels videoin/audioin
int StreamingServer_GetMultipleChannelStreamIndex(HANDLE hObject, int iIndex);
int StreamingServer_GetMultipleChannelChannelIndex(HANDLE hObject, int iIndex);

#endif // _RTSPS_LOCAL_H_
