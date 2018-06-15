/*
 *******************************************************************************
 * $Header: /RD_1/Protocol/RTSP/Server/rtspstreamserver/rtspstreamingserver/src/rtspstreamingserver.h 3     06/06/21 5:16p Shengfu $
 *
 *  Copyright (c) 2000-2002 Vivotek Inc. All rights reserved.
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
 *  Module name         :   RTSPStreamingAPI.h
 *  Module description  :   Handle RTSP request and media streaming action
 *  Author              :   Simon
 *  Created at          :   2002/4/24
 *  Revision            :   1.0
 ******************************************************************************
 *                        Revision history
 ******************************************************************************
 *
 *  $History: rtspstreamingserver.h $
 * 
 * *****************  Version 3  *****************
 * User: Shengfu      Date: 06/06/21   Time: 5:16p
 * Updated in $/RD_1/Protocol/RTSP/Server/rtspstreamserver/rtspstreamingserver/src
 * 
 * *****************  Version 2  *****************
 * User: Shengfu      Date: 06/05/18   Time: 3:26p
 * Updated in $/RD_1/Protocol/RTSP/Server/rtspstreamserver/rtspstreamingserver/src
 * update to version 1.4.0.0
 * 
 * *****************  Version 12  *****************
 * User: Shengfu      Date: 06/01/23   Time: 4:21p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/rtspstreamserver/rtspstreamingserver/src
 * upgrade to ver 1.3.0.7
 * 
 * *****************  Version 11  *****************
 * User: Shengfu      Date: 06/01/10   Time: 5:49p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/rtspstreamserver/rtspstreamingserver/src
 * update to 1.3.0.6
 * 
 * *****************  Version 9  *****************
 * User: Shengfu      Date: 05/11/30   Time: 6:23p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/rtspstreamserver/rtspstreamingserver/src
 * update to version 1.3.0.4
 * 
 * *****************  Version 8  *****************
 * User: Shengfu      Date: 05/11/03   Time: 11:57a
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/rtspstreamserver/rtspstreamingserver/src
 * update to version 1.3.0.3
 * 
 * *****************  Version 7  *****************
 * User: Shengfu      Date: 05/10/07   Time: 8:48a
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/rtspstreamserver/rtspstreamingserver/src
 * 
 * *****************  Version 6  *****************
 * User: Shengfu      Date: 05/09/27   Time: 1:15p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/rtspstreamserver/rtspstreamingserver/src
 * update to version 1,3,0,1
 * 
 * *****************  Version 4  *****************
 * User: Shengfu      Date: 05/08/19   Time: 11:49a
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/rtspstreamserver/rtspstreamingserver/src
 * 
 * *****************  Version 1  *****************
 * User: Shengfu      Date: 05/08/19   Time: 11:31a
 * Created in $/RD_1/Protocol/RTSP/Server/RTSPSTREAMSERVER/rtspstreamingserver/src
 * 
 * *****************  Version 3  *****************
 * User: Kate         Date: 05/08/15   Time: 10:35p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/rtspstreamserver/rtspstreamingserver/src
 * Add ClearAccessList function
 * 
 * *****************  Version 2  *****************
 * User: Shengfu      Date: 05/08/10   Time: 9:01a
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/rtspstreamserver/rtspstreamingserver/src
 * update rtspstreaming server which enable multicast
 * 
 * *****************  Version 9  *****************
 * User: Yun          Date: 05/07/14   Time: 2:38p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/rtspstreamserver/rtspstreamingserver/src
 * Modify the interface of RTSPStreaming_SetAudioParameters and
 * RTSPStreaming_SetVideoParameters
 * 
 * *****************  Version 8  *****************
 * User: Shengfu      Date: 05/07/13   Time: 2:26p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/rtspstreamserver/rtspstreamingserver/src
 * update rtsp streaming server
 * 
 * *****************  Version 6  *****************
 * User: Shengfu      Date: 05/05/13   Time: 4:36p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/rtspstreamserver/rtspstreamingserver/src
 * update for new RTP extension
 * 
 * *****************  Version 5  *****************
 * User: Shengfu      Date: 05/04/15   Time: 1:35p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/rtspstreamserver/rtspstreamingserver/src
 * 1. multicast added, but disable
 * 2. RTP extension added
 * 
 * *****************  Version 3  *****************
 * User: Shengfu      Date: 04/12/20   Time: 2:34p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/rtspstreamserver/rtspstreamingserver/src
 * update to version 1.1.0.0
 * 
 * *****************  Version 2  *****************
 * User: Joe          Date: 04/10/07   Time: 5:34p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/rtspstreamserver/rtspstreamingserver/src
 * 1. Remove LeadTek and docomo stream from SDP.
 * 2. Add G.722.1 capability
 * 3. Add more parameters for Video and Audio settings.
 * 
 * *****************  Version 1  *****************
 * User: Shengfu      Date: 04/09/14   Time: 9:40a
 * Created in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/RTSPSTREAMSERVER/rtspstreamingserver/src
 * 
 * *****************  Version 1  *****************
 * User: Shengfu      Date: 04/09/14   Time: 9:22a
 * Created in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/RTSPSTREAMSERVER/rtspstreamingserver/src
 * 
 * *****************  Version 8  *****************
 * User: Shengfu      Date: 03/12/05   Time: 5:27p
 * Updated in $/rd_2/project/TM1300_PSOS/Pikachu/Firmware/PikachuPV/RTSPSTREAMING/Include
 * update to version 0102j
 * 
 * *****************  Version 5  *****************
 * User: Shengfu      Date: 03/10/24   Time: 4:07p
 * Updated in $/rd_2/project/TM1300_PSOS/Pikachu/Firmware/PikachuPV/RTSPSTREAMING/Include
 * 
 * *****************  Version 7  *****************
 * User: Shengfu      Date: 03/08/27   Time: 3:51p
 * Updated in $/rd_2/project/TM1300_PSOS/Pikachu/Firmware/PIKACHU/RTSPStreaming/Include
 * 
 * *****************  Version 2  *****************
 * User: Shengfu      Date: 03/04/17   Time: 2:44p
 * Updated in $/rd_2/project/TM1300_PSOS/Pikachu/Firmware/PikachuPV/RTSPSTREAMING/Include
 * update to 0102b
 * 
 * *****************  Version 3  *****************
 * User: Simon        Date: 02/07/04   Time: 11:05a
 * Updated in $/rd_2/project/TM1300_PSOS/Pikachu/Firmware/PIKACHU/RTSPStreaming/Include
 * 1. modify for NAT problem
 * 2. access name can be set by user
 */

/*!
 *******************************************************************************
 * Copyright 2000-2002 Vivotek, Inc. All rights reserved.
 *
 * \file
 * RTSPStreamingAPI.h
 *
 * \brief
 * Handle the RTSP request from RTSP client and send the media stream to client.
 *
 * \date
 * 2002/05/28
 *
 * \author
 * Simon Chen
 *
 *
 *******************************************************************************
 */

/*!
 ******************************************************************************
 *                        Revision history
 ******************************************************************************
 * Version 1.6.0.1, 2007/09/20, YenChun
 * Updated in $streamingserver\rtspserver\src\parser.c
 *		- RTSPServer_ParseURL() fix no access name will segmentation fail
 * Updated in $streamingserver\rtspstreamingserver\src\rtspstreaming.c
 * Updated in $streamingserver\rtspstreamingserver\inc\rtspstreamingserver.h
 *		- RTSPStreaming_CreateMulticastSocket() modified to support Mulitcast TTL
 *		- Update to Version 1.6.0.1
 */

#ifndef _RTSPSTREAMINGAPI_H_
#define _RTSPSTREAMINGAPI_H_

#include "rtsprtpcommon.h"
#include "osisolate.h"
#include "typedef.h"
#include "common.h"
#include "sockdef.h"
#include "bitstreambufdef.h"
#include "streamserver.h"


#define RTSPSTREAMINGSERVER_VERSION MAKEFOURCC( 2, 5, 8, 23 ) 

//20120801 modified by Jimmy for metadata
#define RTSPSTREAMING_NO_MEDIA                      0x00000000
#define RTSPSTREAMING_MEDIATYPE_VIDEO				0x00000001
#define RTSPSTREAMING_MEDIATYPE_AUDIO				0x00000002
#define RTSPSTREAMING_MEDIATYPE_METADATA			0x00000004
//20120925 added by Jimmy for ONVIF backchannel
#define RTSPSTREAMING_MEDIATYPE_AUDIOBACK			0x00000008
/*
#define RTSPSTREAMING_RTSPSERVER_MEDIATYPE_BASE		0
#define RTSPSTREAMING_MEDIATYPE_AUDIOVIDEO			1+RTSPSTREAMING_RTSPSERVER_MEDIATYPE_BASE
#define RTSPSTREAMING_MEDIATYPE_AUDIOONLY			2+RTSPSTREAMING_RTSPSERVER_MEDIATYPE_BASE
#define RTSPSTREAMING_MEDIATYPE_VIDEOONLY			3+RTSPSTREAMING_RTSPSERVER_MEDIATYPE_BASE
*/

#define RTSPSTREAMING_VIDEO_PROLEVE                 0x00000001
#define RTSPSTREAMING_VIDEO_BITRATE                 0x00000002
#define RTSPSTREAMING_VIDEO_CLOCKRATE               0x00000004
#define RTSPSTREAMING_VIDEO_MPEG4CI                 0x00000008
#define RTSPSTREAMING_VIDEO_WIDTH                   0x00000010
#define RTSPSTREAMING_VIDEO_HEIGHT                  0x00000020
#define RTSPSTREAMING_VIDEO_DECODEBUFF              0x00000040
#define	RTSPSTREAMING_VIDEO_SET_CI					0x00000080
#define RTSPSTREAMING_VIDEO_CODECTYPE				0x00000100
#define RTSPSTREAMING_VIDEO_H264SPROP				0x00000200
//20150113 added by Charles for H.265
#define RTSPSTREAMING_VIDEO_H265SPROP				0x00000400

#define RTSPSTREAMING_AUDIO_BITRATE                 0x00000001
#define RTSPSTREAMING_AUDIO_CLOCKRATE               0x00000002
#define RTSPSTREAMING_AUDIO_PACKETTIME              0x00000004
#define RTSPSTREAMING_AUDIO_OCTECTALIGN             0x00000008
#define RTSPSTREAMING_AUDIO_AMRCRC                  0x00000010
#define RTSPSTREAMING_AUDIO_ROBUSTSORT              0x00000020
#define RTSPSTREAMING_AUDIO_CODECTYPE               0x00000040
#define	RTSPSTREAMING_AUDIO_SET_CI					0x00000080
#define RTSPSTREAMING_AUDIO_PACKINGMODE             0x00000100

//20120801 added by Jimmy for metadata
#define RTSPSTREAMING_METADATA_BITRATE              0x00000001
#define RTSPSTREAMING_METADATA_CLOCKRATE            0x00000002
#define RTSPSTREAMING_METADATA_PACKETTIME           0x00000004
#define RTSPSTREAMING_METADATA_OCTECTALIGN          0x00000008
#define RTSPSTREAMING_METADATA_AMRCRC               0x00000010
#define RTSPSTREAMING_METADATA_ROBUSTSORT           0x00000020
#define RTSPSTREAMING_METADATA_CODECTYPE            0x00000040
#define	RTSPSTREAMING_METADATA_SET_CI				0x00000080

/* Move to streamserver.h by YenChun 070419
#define	RTSPSTREAMING_AUTHENTICATION_DISABLE		0
#define RTSPSTREAMING_AUTHENTICATION_BASIC			1
#define	RTSPSTREAMING_AUTHENTICATION_DIGEST			2
*/

#define RTSPSTREAMING_ACCESSNAME_SETFLAG			0x00000001
#define RTSPSTREAMING_MEDIAMODE_SETFLAG				0x00000002
#define	RTSPSTREAMING_RTSP_AUTHENTICATE_SETFLAG		0x00000004

#define	RTSPSTREAMING_TRACK_NAME_LEN				20

/*! Supported Audio Codec Type */
typedef enum 
{
	ractGAMR = 0,
	ractG7221 = 1,
	ractG711u = 2,
	ractG711a = 3,
	ractAAC4 = 4,
	ractG726
} ERTSPAudioCodecType;

#ifdef RTSPRTP_MULTICAST
//20100714 Modified by danny For Multicast parameters load dynamically
/*! Multicast Information*/
//typedef struct
//{
	/*! Multicast Address*/
	//unsigned long	ulMulticastAddress;
	/*! Multicast Video port*/
	//unsigned short  usMulticastVideoPort;
	/*! Multicast Audio port*/
	//unsigned short 	usMulticastAudioPort;
	/*! Multicast Time to Live value*/
	//int             usTTL;
	/*! Whether or not to use always multicast*/
	//int             iAlwaysMulticast;
	/*! Whether or not to use RTP extension, If Vivotek client is detected then this value is true*/
	//int             iRTPExtension;
	/*! SDP index number*/
	//int				iSDPIndex;
	/*! Added by Louis 2008/01/29 for multicast audio / video only */
	//int				iAlwaysMulticastAudio;
	//int				iAlwaysMulticastVideo;

//}MULTICASTINFO;
#endif

#ifdef _SIP
/*! Parameters for SIP module */
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
    int                 iSIPMaxConnectionNum;
	
}TSIP_PARAM;                  
#endif

/*! The data structure describes the parameters of RTSPStreaming object. Used in
 * \b RTSPStreaming_Create and \b RTSPStreaming_SetParameters functions to set 
 * the object parameters. */
typedef struct 
{
	//20101123 Added by danny For support advanced system log 
	BOOL  			bAdvLogSupport;
	
	/*! RTSP streaming server port number in host order. */	
	unsigned short	usRTSPPort;

	/*! fixed RTP port for video */
	unsigned short	usRTPVPort;
	/*! fixed RTP port for audio */
	unsigned short	usRTPAPort;
	//20120801 added by Jimmy for metadata
	/*! fixed RTP port for metadata */
	unsigned short	usRTPMPort;	
	/*! fixed RTCP port for video */
	unsigned short	usRTCPVPort;
	/*! fixed RTCP port for audio */
	unsigned short	usRTCPAPort;
	//20120801 added by Jimmy for metadata
	/*! fixed RTCP port for metadata */
	unsigned short	usRTCPMPort;	

    /*! Local IP address in host order. */	
	unsigned long   ulLocalIP;		
	
	// for NAT problem
    /*! Local subnet mask in host order. */	
	unsigned long	ulLocalSubnetMask;	

	// for NAT problem
    /*! Public IP address in host order when server inside NAT environment.*/	
	unsigned long	ulNATIP;	

    /*! The access name of RTSP server for AV media. Maximun 20 characters  .*/	
	char			acAccessName[MULTIPLE_STREAM_NUM][ACCESSNAME_LENGTH];

	/*! RTSP streaming server type. Now, not support video or audio only mode.
     * - RTSPSTREAMING_MEDIATYPE_AUDIOVIDEO => Support audio & video streaming.
     * - RTSPSTREAMING_MEDIATYPE_AUDIOONLY => Just support audio streaming.
     * - RTSPSTREAMING_MEDIATYPE_VIDEOONLY => Just support video streaming.
     */	
	int iRTSPStreamingMediaType[MULTIPLE_STREAM_NUM];

	/*!Authentication method of RTSP server 
	*/
	int	iRTSPAuthentication;
	/*! RTSP Signaling Server Thread Priority*/
	DWORD           dwRTSPServerPriority;
	/*! Video Media Channel Thread Priority*/
	DWORD           dwVideoChannelPriority;
	/*! Audio Media Channel Thread Priority*/
	DWORD           dwAudioChannelPriority;
	//20120801 added by Jimmy for metadata
	/*! Metadata Media Channel Thread Priority*/
	DWORD           dwMetadataChannelPriority;
	/*! Video Packetizer Thread Priority*/
	DWORD           dwVideoPacketizerPriority;
	/*! Audio Packetizer Thread Priority*/
	DWORD           dwAudioPacketizerPriority;
	//20120801 added by Jimmy for metadata
	/*! Metadata Packetizer Thread Priority*/
	DWORD           dwMetadataPacketizerPriority;

#ifdef RTSPRTP_MULTICAST
	/*! For each stream we must maintain one multicast information */
	//20130327 modified by Jimmy to support different video/audio multicast addresses for ONVIF Test Tool 12.12
	MULTICASTINFO	stMulticastInfo[RTSP_MULTICASTNUMBER];
#endif
	/*! Maximum number of allowed connections*/
    int             iRTSPMaxConnectionNum;

#ifdef _SIP
	/*! SIP parameter*/
	TSIP_PARAM		tSIPParameter;
#endif				

#ifdef _SHARED_MEM
	/* Share memory handle */
	HANDLE			ahShmemVideoHandle[VIDEO_TRACK_NUMBER];
	HANDLE			ahShmemAudioHandle[AUDIO_TRACK_NUMBER];
	//20120801 added by Jimmy for metadata
	//20130605 modified by Jimmy to support metadata event
	HANDLE			ahShmemMetadataHandle[METADATA_TRACK_NUMBER][SHMEM_HANDLE_MAX_NUM];
	/* Indicate number of seconds protected by Save-bandwidth feature */
	int				dwProtectedDelta;
#endif

	//20081121 for URL authentication
	int				iURLAuthEnabled;

}TRTSPSTREAMING_PARAM;


/*! This data structure states the parameters that can be modified at runtime */
typedef struct
{
    /*! The access name of RTSP server for AV media. Maximun 20 characters  .*/	
	char	acAccessName[MULTIPLE_STREAM_NUM][ACCESSNAME_LENGTH];

	/*! RTSP streaming server type. Now, not support video or audio only mode.
     * - RTSPSTREAMING_MEDIATYPE_AUDIOVIDEO => Support audio & video streaming.
     * - RTSPSTREAMING_MEDIATYPE_AUDIOONLY => Just support audio streaming.
     * - RTSPSTREAMING_MEDIATYPE_VIDEOONLY => Just support video streaming.
     */	
	int		iRTSPStreamingMediaType[MULTIPLE_STREAM_NUM];

	/*!Authentication method of RTSP server 
	*/
	int		iRTSPAuthentication;

#ifdef _SIP
	/*! Authentication method of SIP server*/
	int    iSIPUASAuthentication;
#endif

}TRTSPSTREAMING_DYNAMIC_PARAM;

/*! This structure maintains current parameters for video stream*/
typedef struct
{
    int		iProfileLevel;       
    int		iBitRate;  // 1.5M == 15000000, 33.6k=33600
    int		iClockRate;
    char	acMPEG4Header[MAX_MP4V_HEADER_SIZE];
    int		iMPEG4HeaderLen;
    int		iWidth;
    int		iHeight;
    int		iDecoderBufferSize;
	char	acTrackName[RTSPSTREAMING_TRACK_NAME_LEN];   
	int		iCIReady;
	// Media Source Type 20081224
	EMediaCodecType	eVideoCodecType;
	DWORD			dwJPEGComponentNum;
	//20090105 H264
	int		iPacketizationMode;
	char	acProfileLevelID[7];
	char	acSpropParamSet[RTSPSTREAMING_MAX_SPROP_PARAM];
	//20100428 Added For Media on demand
	DWORD	dwFileLength;
	//20140605 added by Charles for ONVIF track token
	char	acTrackToken[RTSPSTREAMING_TRACK_NAME_LEN];
    //20150113 added by Charles for H.265
    char    acSpropVPSParam[RTSPSTREAMING_MAX_SPROP_PARAM];
    char    acSpropSPSParam[RTSPSTREAMING_MAX_SPROP_PARAM];
    char    acSpropPPSParam[RTSPSTREAMING_MAX_SPROP_PARAM];
	
} TRTSPSTREAMING_VIDENCODING_PARAM;


/*! The data structure describes the parameters of AMR audio encoder in 
 * RTSPStreaming object. Used in \b RTSPStreaming_SetAudioParameters function 
 * to set the AMR audi encoder parameters. */
typedef struct
{
    int     iBitRate;      //1.5M == 15000, 33.6k=33600
    int     iClockRate;    //8000
    int     iPacketTime;   //200
    int     iOctetAlign;   //1
    int     iAMRcrc;       //0
    int     iRobustSorting;//0  
	int		iAudioCodecType;	    
	// Only AAC needs following fields
    int		iM4AProfileLevel;
    char	acM4ASpecConf[16];
    int		iM4ASpecConfLen;
	int		iChanNum;
	char	acTrackName[RTSPSTREAMING_TRACK_NAME_LEN];
	int		iCIReady;
	BOOL	bIsBigEndian;	//Bit stream packing mode (FALSE:litte-endian(RFC 3551) or TRUE:big-endian(ATM AAL2))
	//20140605 added by Charles for ONVIF track token
	char	acTrackToken[RTSPSTREAMING_TRACK_NAME_LEN];   
} TRTSPSTREAMING_AUDENCODING_PARAM;

//20120801 added by Jimmy for metadata
/*! This structure maintains current parameters for Metadata stream*/
typedef struct
{
   	int     iBitRate;      //1.5M == 15000, 33.6k=33600
    int     iClockRate;    //90000
    int     iPacketTime;   //200
    int     iOctetAlign;   //1
    int     iAMRcrc;       //0
    int     iRobustSorting;//0  
	int		iMetadataCodecType;	    
	// Only AAC needs following fields
    int		iM4AProfileLevel;
    char	acM4ASpecConf[16];
    int		iM4ASpecConfLen;
	int		iChanNum;
	char	acTrackName[RTSPSTREAMING_TRACK_NAME_LEN];
	int		iCIReady;
	//20140605 added by Charles for ONVIF track token
	char	acTrackToken[RTSPSTREAMING_TRACK_NAME_LEN];   
	
} TRTSPSTREAMING_METADATAENCODING_PARAM;

//20120925 added by Jimmy for ONVIF backchannel
typedef struct
{
	char	acTrackName[RTSPSTREAMING_TRACK_NAME_LEN];	
} TRTSPSTREAMING_AUDDECODING_PARAM;



#ifdef _SHARED_MEM
/* 20100428 Added For Media on demand */
/*! The data structure describes the parameters of MOD Control in 
 * RTSPStreaming object. Used in \b RTSPStreaming_UpdateMODControlParameters function 
 * to set the MOD Control parameters. */
typedef struct
{
	/* MOD server return code if error encountered */
	EMODRunCode 		eMODRunCode;
	/* MOD server return rtsp server set control info state */
	int					iMODSetControlInfoReady;
	/* MOD command type return from MOD server*/
    EMODCommandType 	eMODReturnCommandType;
    /* MOD command value return from MOD server*/
    char 				acMODReturnCommandValue[RTSPMOD_COMMAND_VALUE_LENGTH];
	
} TRTSPSTREAMING_MODCONTROL_PARAM;
#endif

#define RTSPSTREAMING_VIDEOENCODING_VIDEOSIGNA_BASE		0
#define	RTSPSTREAMING_VIDEOSIGNAL_NTSC		0+RTSPSTREAMING_VIDEOENCODING_VIDEOSIGNA_BASE
#define RTSPSTREAMING_VIDEOSIGNAL_PAL		1+RTSPSTREAMING_VIDEOENCODING_VIDEOSIGNA_BASE

#define RTSPSTREAMING_VIDEOENCODING_PICSIZE_BASE		0
#define RTSPSTREAMING_PICSIZE_QSIF	1+RTSPSTREAMING_VIDEOENCODING_PICSIZE_BASE
#define RTSPSTREAMING_PICSIZE_QCIF	2+RTSPSTREAMING_VIDEOENCODING_PICSIZE_BASE
#define RTSPSTREAMING_PICSIZE_CIF		3+RTSPSTREAMING_VIDEOENCODING_PICSIZE_BASE
#define RTSPSTREAMING_PICSIZE_SIF		4+RTSPSTREAMING_VIDEOENCODING_PICSIZE_BASE

/*typedef enum
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
	ccctWaitHTTPSocket		// 10
} EControlChannelCallbackType;

#define MEDIA_CALLBACK_REQUEST_BUFFER 1
#define MEDIA_CALLBACK_RELEASE_BUFFER 2

typedef SCODE (*FControlChannel_Callback)(HANDLE hParentHandle, DWORD dwConnectionID, DWORD dwCallbackType, DWORD dwCallbackData);
typedef SCODE (*MEDIA_CALLBACK)(HANDLE hParentHandle, DWORD dwCallbackType, void* pvCallbackData);*/


/*!
 *******************************************************************************
 * \brief
 * Get the Version of RTSP Streaming server
 *
 * \param byMajor
 * \a (o) Pointer to byte to store the Major version of RTSP Streaming server.
 *
 * \param byMinor
 * \a (o) Pointer to byte to store the Minor version of RTSP Streaming server.
 *
 * \param byBuild
 * \a (o) Pointer to byte to store the Build version of RTSP Streaming server.
 *
 * \param byRevision
 * \a (o) Pointer to byte to store the Revision version of RTSP Streaming server.
 *
 *
 **************************************************************************** */
int RTSPStreaming_GetVersion(BYTE *byMajor, BYTE *byMinor, BYTE *byBuild, BYTE *byRevision);

/*!
 *******************************************************************************
 * \brief
 * Create \b RTSPStreaming instance and initialize it.
 *
 * \param iMaxSessionNumber
 * \a (i) Set the maximum acceptable number of RTSP client.
 *
 * \param pstRTSPStreamingParameter
 * \a (i) the pointer of data structure \b RTSPSTREAMING_PARAM for setting
 * the initialization parameters of the \b RTSPStreaming instance. 
 *
 * \retval NULL
 * Create \b RTSPStreaming object failed.
 * \retval Others
 * Create \b RTSPStreaming object ok. The return value is its handle.
 *
 * \note
 * This function should be called before using this instance. 
 *
 * \see RTSPStreaming_Close
 *
 **************************************************************************** */
HANDLE RTSPStreaming_Create(TRTSPSTREAMING_PARAM *pstRTSPStreamingParameter);

/*!
 *******************************************************************************
 * \brief
 * Start the \b RTSPStreaming server.
 *
 * \param hRTSPStreamingHandle
 * \a (i) Handle of RTSPStreaming object.
 *
 *
 * \retval 0
 * Start the RSPStreaming server ok.
 * \retval Others
 * Start the RSPStreaming server failed.
 *
 * \note
 * After this function is called, RTSPStreaming server start to accept RTSP
 * client request and send the media bitstream to client site. 
 *
 * \see RTSPStreaming_Stop
 *
 **************************************************************************** */
int RTSPStreaming_Start(HANDLE hRTSPStreamingHandle);

/*!
 *******************************************************************************
 * \brief
 * Stop the \b RTSPStreaming server.
 *
 * \param hRTSPStreamingHandle
 * \a (i) Handle of RTSPStreaming object.
 *
 *
 * \retval 0
 * Stop the RSPStreaming server ok.
 * \retval Others
 * Stop the RSPStreaming server failed.
 *
 * \note
 * After this function is called, RTSPStreaming server will be stopped. 
 *
 * \see RTSPStreaming_Start
 *
 **************************************************************************** */
int RTSPStreaming_Stop(HANDLE hRTSPStreamingHandle);

/*!
 *******************************************************************************
 * \brief
 * Delete an instance of the \b RTSPStreaming object.
 *
 * \param hRTSPStreamingHandle
 * \a (i) Handle of RTSPStreaming object.
 *
 *
 * \retval 0
 * delete the RSPStreaming server ok.
 * \retval Others
 * delete the RSPStreaming server failed.
 *
 * \note
 * After the instance is closed, the handle of this instance can't be accessed.
 * 
 *
 * \see RTSPStreaming_Create
 *
 **************************************************************************** */
int RTSPStreaming_Close(HANDLE* phRTSPStreamingHandle);

/*!
 *******************************************************************************
 * \brief
 * Set the parameter of \b RTSPStreaming server.
 *
 * \param hRTSPStreamingHandle
 * \a (i) Handle of RTSPStreaming object.
 *
 * \param pstRTSPStreamingParameter
 * \a (i) the pointer of data structure \b RTSPSTREAMING_PARAM for setting
 * the parameters of the \b RTSPStreaming server instance. 
 *
 * \retval 0
 * set the parameter of RSPStreaming server ok.
 * \retval Others
 * set the parameter of RSPStreaming server failed. 
 *
 * \note
 * This function can change the parameter of RTSPStreaming server.
 *
 *
 **************************************************************************** */
int RTSPStreaming_SetDynamicParameters(HANDLE hRTSPStreamingHandle, TRTSPSTREAMING_DYNAMIC_PARAM *pstRTSPStreamingDynamicParam, DWORD dwSetFlag);

/*!
 *******************************************************************************
 * \brief
 * Set the Video parameter of \b RTSPStreaming server.
 *
 * \param hRTSPStreamingHandle
 * \a (i) Handle of RTSPStreaming object.
 *
 * \param iSDPIndex
 * \a (i) SDP Index of the video 
 *
 * \param pstVideoEncodingParameter
 * \a (i) the pointer of data structure \b TRTSPSTREAMING_VIDENCODING_PARAM for setting
 * the parameters of the \b RTSPStreaming server instance. 
 *
 * \retval 0
 * set the parameter of RSPStreaming server ok.
 * \retval Others
 * set the parameter of RSPStreaming server failed. 
 *
 * \note
 * This function can change the parameter of RTSPStreaming server.
 *
 *
 **************************************************************************** */
int RTSPStreaming_SetVideoParameters(HANDLE hRTSPStreamingHandle,int iSDPIndex, TRTSPSTREAMING_VIDENCODING_PARAM *pstVideoEncodingParameter,DWORD dwFlag);

/*!
 *******************************************************************************
 * \brief
 * Set the Audio parameter of \b RTSPStreaming server.
 *
 * \param hRTSPStreamingHandle
 * \a (i) Handle of RTSPStreaming object.
 *
 * \param iSDPIndex
 * \a (i) SDP Index of the audio 
 *
 * \param pstAudioEncodingParameter
 * \a (i) the pointer of data structure \b TRTSPSTREAMING_AUDENCODING_PARAM for setting
 * the parameters of the \b RTSPStreaming server instance. 
 *
 * \retval 0
 * set the parameter of RSPStreaming server ok.
 * \retval Others
 * set the parameter of RSPStreaming server failed. 
 *
 * \note
 * This function can change the parameter of RTSPStreaming server.
 *
 *
 **************************************************************************** */
int RTSPStreaming_SetAudioParameters(HANDLE hRTSPStreamingHandle,int iSDPIndex, TRTSPSTREAMING_AUDENCODING_PARAM *pstAudioEncodingParameter,DWORD dwFlag);

//20120801 added by Jimmy for metadata
/*!
 *******************************************************************************
 * \brief
 * Set the Metadata parameter of \b RTSPStreaming server.
 *
 * \param hRTSPStreamingHandle
 * \a (i) Handle of RTSPStreaming object.
 *
 * \param iSDPIndex
 * \a (i) SDP Index of the metadata 
 *
 * \param pstMetadataEncodingParameter
 * \a (i) the pointer of data structure \b TRTSPSTREAMING_METADATAENCODING_PARAM for setting
 * the parameters of the \b RTSPStreaming server instance. 
 *
 * \retval 0
 * set the parameter of RSPStreaming server ok.
 * \retval Others
 * set the parameter of RSPStreaming server failed. 
 *
 * \note
 * This function can change the parameter of RTSPStreaming server.
 *
 *
 **************************************************************************** */
int RTSPStreaming_SetMetadataParameters(HANDLE hRTSPStreamingHandle,int iSDPIndex, TRTSPSTREAMING_METADATAENCODING_PARAM *pstMetadataEncodingParameter,DWORD dwFlag);

//20120925 added by Jimmy for ONVIF backchannel
/*!
 *******************************************************************************
 * \brief
 * Set the Audioback parameter of \b RTSPStreaming server.
 *
 * \param hRTSPStreamingHandle
 * \a (i) Handle of RTSPStreaming object.
 *
 * \param iChannelIndex
 * \a (i) Channel Index of the audioback 
 *
 * \param pstAudioDecodingParameter
 * \a (i) the pointer of data structure \b TRTSPSTREAMING_AUDDECODING_PARAM for setting
 * the parameters of the \b RTSPStreaming server instance. 
 *
 * \retval 0
 * set the parameter of RSPStreaming server ok.
 * \retval Others
 * set the parameter of RSPStreaming server failed. 
 *
 * \note
 * This function can change the parameter of RTSPStreaming server.
 *
 *
 **************************************************************************** */
int RTSPStreaming_SetAudiobackParameters(HANDLE hRTSPStreamingHandle,int iChannelIndex, TRTSPSTREAMING_AUDDECODING_PARAM *pstAudioDecodingParameter);

//20140605 added by Charles for ONVIF track token
/*!
 *******************************************************************************
 * \brief
 * Set the MOD track token parameter of \b RTSPStreaming server.
 *
 * \param hRTSPStreamingHandle
 * \a (i) Handle of RTSPStreaming object.
 *
 * \param iSDPIndex
 * \a (i) SDP Index  
 *
 * \param pcTrackToken
 * \a (i) the pointer of data array \b pMODConfUBuffer->acTrackToken for setting
 * the parameters of the \b RTSPStreaming server instance. 
 *
 * \retval 0
 * set the parameter of RSPStreaming server ok.
 * \retval Others
 * set the parameter of RSPStreaming server failed. 
 *
 * \note
 * This function can change the parameter of RTSPStreaming server.
 *
 *
 **************************************************************************** */
int RTSPStreaming_SetMODTrackTokenParameters(HANDLE hRTSPStreaming,int iSDPIndex, char *pcTrackToken);

/*!
 *******************************************************************************
 * \brief
 * Set the control callback function of \b RTSPStreaming server.
 *
 * \param hRTSPStreamingHandle
 * \a (i) Handle of RTSPStreaming object.
 *
 * \param pfnCallback
 * \a (i) Callback function
 *
 * \param hParentHandle
 * \a (i) callback parent handle
 *
 * \retval 0
 * set the parameter of RSPStreaming server ok.
 * \retval Others
 * set the parameter of RSPStreaming server failed. 
 *
 *
 **************************************************************************** */
SCODE RTSPStreaming_SetControlCallback(HANDLE hRTSPStreamingHandle, FControlChannel_Callback pfnCallback, HANDLE hParentHandle);
/*!
 *******************************************************************************
 * \brief
 * Set the Video callback function of \b RTSPStreaming server.
 *
 * \param hRTSPStreamingHandle
 * \a (i) Handle of RTSPStreaming object.
 *
 * \param pfnCallback
 * \a (i) Callback function
 *
 * \param hParentHandle
 * \a (i) callback parent handle
 *
 * \retval 0
 * set the parameter of RSPStreaming server ok.
 * \retval Others
 * set the parameter of RSPStreaming server failed. 
 *
 *
 **************************************************************************** */
SCODE RTSPStreaming_SetVideoCallback(HANDLE hRTSPStreamingHandle, MEDIA_CALLBACK pfnCallback, HANDLE hParentHandle);
/*!
 *******************************************************************************
 * \brief
 * Set the Audio callback function of \b RTSPStreaming server.
 *
 * \param hRTSPStreamingHandle
 * \a (i) Handle of RTSPStreaming object.
 *
 * \param pfnCallback
 * \a (i) Callback function
 *
 * \param hParentHandle
 * \a (i) callback parent handle
 *
 * \retval 0
 * set the parameter of RSPStreaming server ok.
 * \retval Others
 * set the parameter of RSPStreaming server failed. 
 *
 *
 **************************************************************************** */
SCODE RTSPStreaming_SetAudioCallback(HANDLE hRTSPStreamingHandle, MEDIA_CALLBACK pfnCallback, HANDLE hParentHandle);
//20120801 added by Jimmy for metadata
/*!
 *******************************************************************************
 * \brief
 * Set the Metadata callback function of \b RTSPStreaming server.
 *
 * \param hRTSPStreamingHandle
 * \a (i) Handle of RTSPStreaming object.
 *
 * \param pfnCallback
 * \a (i) Callback function
 *
 * \param hParentHandle
 * \a (i) callback parent handle
 *
 * \retval 0
 * set the parameter of RSPStreaming server ok.
 * \retval Others
 * set the parameter of RSPStreaming server failed. 
 *
 *
 **************************************************************************** */
SCODE RTSPStreaming_SetMetadataCallback(HANDLE hRTSPStreamingHandle, MEDIA_CALLBACK pfnCallback, HANDLE hParentHandle);

/*!
 *******************************************************************************
 * \brief
 * Set the host name of \b RTSPStreaming server.
 *
 * \param hRTSPStreamingHandle
 * \a (i) Handle of RTSPStreaming object.
 *
 * \param pcRTSPStreamingHostName
 * \a (i) the pointer of \b char for setting the host name of the \b 
 * RTSPStreaming server. 
 *
 * \retval 0
 * set the host name of RSPStreaming server ok.
 * \retval Others
 * set the host name of RSPStreaming server failed. 
 *
 * \note
 * This function can change the host name of RTSPStreaming server.
 *
 *
 **************************************************************************** */
int RTSPStreaming_SetHostName(HANDLE hRTSPStreamingHandle, char * pcRTSPStreamingHostName);

/*!
 *******************************************************************************
 * \brief
 * Set the etag string in SDP of \b RTSPStreaming server.
 *
 * \param hRTSPStreamingHandle
 * \a (i) Handle of RTSPStreaming object.
 *
 * \param pcSDPETag
 * \a (i) the pointer of \b char for setting the etag string in SDP of the \b 
 * RTSPStreaming server. 
 *
 * \retval 0
 * set the etag string in SDP of RSPStreaming server ok.
 * \retval Others
 * set the etag string in SDP of RSPStreaming server failed. 
 *
 * \note
 * This function can change the etag string in SDP of RTSPStreaming server. 
 * The etag in SDP is used to distinguish the version of session description.
 * We use frimware version combined with MAC address as the etag string. The
 * maximun length of wtag is 60 bytes.
 *
 **************************************************************************** */
int RTSPStreaming_SetSDPETag(HANDLE hRTSPStreaming, char * pcSDPETag);

//int RTSPStreaming_SetVideoParameters(HANDLE hRTSPStreamingHandle, RTSPSTREAMING_VIDENCODING_PARAM *pstVideoEncodingParameter);
//int RTSPStreaming_SetAudioParameters(HANDLE hRTSPStreamingHandle, RTSPSTREAMING_AUDENCODING_PARAM *pstAudioEncodingParameter);

/*!
 *******************************************************************************
 * \brief
 * Add the acceptable IP range of \b RTSPStreaming server.
 *
 * \param hRTSPStreamingHandle
 * \a (i) Handle of RTSPStreaming object.
 *
 * \param ulStartIP
 * \a (i) the start IP address of acceptable IP range in network order. 
 *
 * \param ulEndIP
 * \a (i) the end IP address of acceptable IP range in network order. 
 *
 * \retval 0
 * set acceptable IP range ok.
 * \retval Others
 * set acceptable IP range failed. 
 *
 * \note
 * This function sets the acceptable IP range of RTSP client for RTSPStreaming 
 * server.
 *
 *
 **************************************************************************** */
int RTSPStreaming_AddAccessList(HANDLE hRTSPStreamingHandle, unsigned long ulStartIP, unsigned long ulEndIP); // in network order

/*!
 *******************************************************************************
 * \brief
 * Remove the acceptable IP range of \b RTSPStreaming server.
 *
 * \param hRTSPStreamingHandle
 * \a (i) Handle of RTSPStreaming object.
 *
 * \param ulStartIP
 * \a (i) the start IP address of acceptable IP range in network order. 
 *
 * \param ulEndIP
 * \a (i) the end IP address of acceptable IP range in network order. 
 *
 * \retval 0
 * remove acceptable IP range ok.
 * \retval Others
 * remove acceptable IP range failed. 
 *
 * \note
 * This function removes the acceptable IP range of RTSP client for RTSPStreaming 
 * server.
 *
 *
 **************************************************************************** */
int RTSPStreaming_RemoveAccessList(HANDLE hRTSPStreamingHandle, unsigned long ulStartIP, unsigned long ulEndIP); // in network order

/*!
 *******************************************************************************
 * \brief
 * Add the rejective IP range of \b RTSPStreaming server.
 *
 * \param hRTSPStreamingHandle
 * \a (i) Handle of RTSPStreaming object.
 *
 * \param ulStartIP
 * \a (i) the start IP address of rejective IP range in network order. 
 *
 * \param ulEndIP
 * \a (i) the end IP address of rejective IP range in network order. 
 *
 * \retval 0
 * set rejective IP range ok.
 * \retval Others
 * set rejective IP range failed. 
 *
 * \note
 * This function sets the rejective IP range of RTSP client for RTSPStreaming 
 * server.
 *
 *
 **************************************************************************** */
int RTSPStreaming_AddDenyList(HANDLE hRTSPStreamingHandle, unsigned long ulStartIP, unsigned long ulEndIP); // in network order

/*!
 *******************************************************************************
 * \brief
 * Remove the rejective IP range of \b RTSPStreaming server.
 *
 * \param hRTSPStreamingHandle
 * \a (i) Handle of RTSPStreaming object.
 *
 * \param ulStartIP
 * \a (i) the start IP address of rejective IP range in network order. 
 *
 * \param ulEndIP
 * \a (i) the end IP address of rejective IP range in network order. 
 *
 * \retval 0
 * Remove rejective IP range ok.
 * \retval Others
 * Remove rejective IP range failed. 
 *
 * \note
 * This function removes the rejective IP range of RTSP client for RTSPStreaming 
 * server.
 *
 *
 **************************************************************************** */
int RTSPStreaming_RemoveDenyList(HANDLE hRTSPStreamingHandle, unsigned long ulStartIP, unsigned long ulEndIP); // in network order

//20101018 Add by danny for support multiple channel text on video
int RTSPStreaming_SendLocation(HANDLE hRTSPStreaming, char* pcLocation, int iMultipleChannelChannelIndex);

/*!
 *******************************************************************************
 * \brief
 * compose SDP for http server.
 *
 * \param hRTSPStreamingHandle
 * \a (i) Handle of RTSPStreaming object.
 *
 * \param ulSDPIP
 * \a (i) the IP address of server. ( if server is behind NAT and aware 
 *        of public IP address, input the public IP address instead of private one
 *
 * \param iVivotekClient
 * \a (i) TURE means the SDP request is from vivotek client otherwise is FALSE. 
 *
 * \param iMulticast
 * \a (i) TURE means requested SDP is for multicast, otherwise is FALSE. 
 *
 * \param ppcSDPBuffer
 * \a (i) pointer of SDP buffer.  
 *
 * \param iSDPBufferLen
 * \a (i) Length of SDP buffer.  
 *
 * \retval > 0
 * the length of composed SDP .
 * \retval Others
 * compase SDP failed. 
 *
 * \note
 * user needs to prepare the buffer to store SDP. 
 *
 *
 **************************************************************************** */
//20120925 modified by Jimmy for ONVIF backchannel
//20111124 modify by danny support Backchannel Multicast for ONVIF test tool 1.02.4
int RTSPStreaming_ComposeAVSDP(HANDLE hRTSPStreaming,int iSDPIndex, unsigned long ulSDPIP,int iVivotekClient,int iMulticast, char *pcSDPBuffer, int iSDPBufferLen, int iPlayerType, int iRequire);

// add by Yun, 2005/06/19
int RTSPStreaming_ClearAccessList(HANDLE hRTSPStreamingHandle);
// add by Kate, 2005/08/26
HANDLE RTSPStreaming_GetIPAccessHandle(HANDLE hRTSPStreaming);

int RTSPStreaming_SetMediaStreamMode(HANDLE hRTSPStreaming,DWORD dwMediaStreamMode, int iCodecIndex);

int RTSPStreaming_AddRTPOverHTTPSock(HANDLE hRTSPStreaming,TStreamServer_ConnectionSettings *ptConnectionSetting);

int RTSPStreaming_GetCurrentSessionNumber(HANDLE hRTSPStreaming);

//this API must be called in the callback of sessionstop sessionstart sessionerror (coz semaphore protection)
int RTSPStreaming_GetRTSPSessionInfo(HANDLE hRTSPStreaming,char* pSessionInfoBuf,int iLength);

//Remove Specific Session by Session ID 20080621
int RTSPStreaming_RemoveSession(HANDLE hRTSPStreaming, DWORD dwSessionID);

#ifdef _SHARED_MEM
//20101208 Modified by danny For GUID format change
//20100105 Added For Seamless Recording
int RTSPStreaming_RemoveGUIDSessions(HANDLE hRTSPStreaming, char* pcSessionGUID);
/* 20081008 Share memory callback for Video*/
SCODE RTSPStreaming_SetShmemVideoCallback(HANDLE hRTSPStreamingHandle, MEDIA_CALLBACK pfnCallback, HANDLE hParentHandle);
/* 20081008 Share memory callback for Audio*/
SCODE RTSPStreaming_SetShmemAudioCallback(HANDLE hRTSPStreamingHandle, MEDIA_CALLBACK pfnCallback, HANDLE hParentHandle);
//20120801 added by Jimmy for metadata
/* 20120801 Share memory callback for Metadata*/
SCODE RTSPStreaming_SetShmemMetadataCallback(HANDLE hRTSPStreamingHandle, MEDIA_CALLBACK pfnCallback, HANDLE hParentHandle);
#endif

/* 20090224 Qos */
SCODE RTSPStreaming_SetQosParameters(HANDLE hRTSPStreaming, TQosInfo *ptQosInfo);

#ifdef _SHARED_MEM
/* 20100105 Added For Seamless Recording */ 
SCODE RTSPStreaming_SetSeamlessRecordingParameters(HANDLE hRTSPStreaming, TSeamlessRecordingInfo *ptSeamlessRecordingInfo);

/* 20100428 Added For Media on demand */ 
SCODE RTSPStreaming_UpdateMODControlParameters(HANDLE hRTSPStreaming, TUBufferConfMOD  *pMODConfUBuffer);
int RTSPStreaming_RemoveSpecificStreamClients(HANDLE hRTSPStreaming,int iStreamIndex);
#endif

#ifdef RTSPRTP_MULTICAST
//20100720 Added by danny to fix Backchannel multicast session terminated, rtsp server has not stopped sending video/audio RTP/RTCP
int RTSPStreaming_GetMulticastCurrentSessionNumber(HANDLE hRTSPStreaming, int iMulticastIndex);
//20100714 Added by danny For Multicast parameters load dynamically
SCODE RTSPStreaming_CheckIfMulticastParametersChanged(HANDLE hRTSPStreaming, int iMulticastIndex, MULTICASTINFO	*ptMulticastInfo);
SCODE RTSPStreaming_SetMulticastParameters(HANDLE hRTSPStreaming, int iStreamIndex, MULTICASTINFO *ptMulticastInfo);
SCODE RTSPStreaming_AddMulticast(HANDLE hRTSPStreaming, int iMulticastCount);
SCODE RTSPStreaming_StartAlwaysMulticast(HANDLE hRTSPStreaming, int iMulticastIndex);
int RTSPStreaming_RemoveSpecificStreamMulticastClients(HANDLE hRTSPStreaming,int iMulticastIndex);
#endif

#endif  //_RTSPSTREAMINGAPI_H_




