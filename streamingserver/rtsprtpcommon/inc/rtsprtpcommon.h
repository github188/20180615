/*
 *******************************************************************************
 * $Header: /RD_1/Protocol/RTSP/Server/rtspstreamserver/rtsprtpcommon/src/rtsprtpcommon.h 3     06/06/21 5:16p Shengfu $
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
 * $History: rtsprtpcommon.h $
 * 
 * *****************  Version 3  *****************
 * User: Shengfu      Date: 06/06/21   Time: 5:16p
 * Updated in $/RD_1/Protocol/RTSP/Server/rtspstreamserver/rtsprtpcommon/src
 * 
 * *****************  Version 2  *****************
 * User: Shengfu      Date: 06/05/18   Time: 3:26p
 * Updated in $/RD_1/Protocol/RTSP/Server/rtspstreamserver/rtsprtpcommon/src
 * update to version 1.4.0.0
 * 
 * *****************  Version 11  *****************
 * User: Shengfu      Date: 06/01/24   Time: 3:24p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/rtspstreamserver/rtsprtpcommon/src
 * 
 * *****************  Version 10  *****************
 * User: Shengfu      Date: 06/01/10   Time: 5:48p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/rtspstreamserver/rtsprtpcommon/src
 * update to 1.3.0.6
 * 
 * *****************  Version 8  *****************
 * User: Shengfu      Date: 05/11/30   Time: 6:23p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/rtspstreamserver/rtsprtpcommon/src
 * update to version 1.3.0.4
 * 
 * *****************  Version 7  *****************
 * User: Shengfu      Date: 05/11/03   Time: 11:57a
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/rtspstreamserver/rtsprtpcommon/src
 * update to version 1.3.0.3
 * 
 * *****************  Version 6  *****************
 * User: Shengfu      Date: 05/09/27   Time: 3:25p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/rtspstreamserver/rtsprtpcommon/src
 * 
 * *****************  Version 5  *****************
 * User: Shengfu      Date: 05/09/27   Time: 1:14p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/rtspstreamserver/rtsprtpcommon/src
 * update to version 1,3,0,1
 * 
 * *****************  Version 4  *****************
 * User: Shengfu      Date: 05/08/19   Time: 11:49a
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/rtspstreamserver/rtsprtpcommon/src
 * 
 * *****************  Version 1  *****************
 * User: Shengfu      Date: 05/08/19   Time: 11:29a
 * Created in $/RD_1/Protocol/RTSP/Server/RTSPSTREAMSERVER/rtsprtpcommon/src
 * 
 * *****************  Version 3  *****************
 * User: Kate         Date: 05/08/16   Time: 8:23p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/rtspstreamserver/rtsprtpcommon/src
 * define RTSP_MULTICASTNUMBER as _MULTICAST_NUM
 * 
 * *****************  Version 2  *****************
 * User: Shengfu      Date: 05/08/10   Time: 9:01a
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/rtspstreamserver/rtsprtpcommon/src
 * update rtspstreaming server which enable multicast
 * 
 * *****************  Version 5  *****************
 * User: Shengfu      Date: 05/07/13   Time: 4:34p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/rtspstreamserver/rtsprtpcommon/src
 * 
 * *****************  Version 4  *****************
 * User: Shengfu      Date: 05/07/13   Time: 2:26p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/rtspstreamserver/rtsprtpcommon/src
 * update rtsp streaming server
 * 
 * *****************  Version 2  *****************
 * User: Shengfu      Date: 05/05/13   Time: 4:36p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/rtspstreamserver/rtsprtpcommon/src
 * update for new RTP extension
 * 
 * *****************  Version 1  *****************
 * User: Shengfu      Date: 05/04/15   Time: 1:39p
 * Created in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/rtspstreamserver/RTSPRTPCOMMON/src
 * 
 * *****************  Version 1  *****************
 * User: Shengfu      Date: 05/03/31   Time: 01:58p
 * Created in $/RD_1/Protocol/Module/RTSP6K_SERVER/include
 * 
 *********************************************************************/

#ifndef __RTSPRTPCOMMON_H__
#define __RTSPRTPCOMMON_H__

#include <string.h>
#ifdef _SHARED_MEM
/* 20100428 Added For Media on demand */
#include <stdio.h>
#include "typedef.h"
#endif

#define MAX_CONNECT_NUM	 10

#define RTP_OVER_UDP        1
#define RTP_OVER_TCP        2
#define RTP_OVER_HTTP       3

//20120723 added by Jimmy for metadata
#ifdef _METADATA_ENABLE
#define MEDIA_TYPE_NUMBER	3
#else
#define MEDIA_TYPE_NUMBER	2
#endif



//#define _WISE_SPOT

#ifdef _WIN32_
#define RTSPRTP_MULTICAST
#define RTSP_MULTICASTNUMBER		2
#endif

#ifdef _WISE_SPOT
#define WISE_SPOT_AUTHENTICATE
#endif

//20090325 add default stream number for multiple stream
#ifdef _MULTIPLE_STREAM_TOTAL
#define MULTIPLE_STREAM_NUM		_MULTIPLE_STREAM_TOTAL
#else
#define MULTIPLE_STREAM_NUM		2		
#endif

//20100728 Added by danny For multiple channels videoin/audioin
#ifdef _MULTIPLE_CHANNEL_TOTAL
#define MULTIPLE_CHANNEL_NUM	_MULTIPLE_CHANNEL_TOTAL
#else
#define MULTIPLE_CHANNEL_NUM	1		
#endif

//20100408 Media on demand
#ifdef _MOD_STREAM_TOTAL
#define MOD_STREAM_NUM		_MOD_STREAM_TOTAL
#else
#define MOD_STREAM_NUM		0		
#endif

#define ACCESSNAME_LENGTH		150

//20100408 Media on demand
#define LIVE_STREAM_NUM			(MULTIPLE_STREAM_NUM - MOD_STREAM_NUM)	
#define LIVE_AUDIO_STREAM_NUM	MULTIPLE_CHANNEL_NUM
//20120731 added by Jimmy for metadata
#ifdef _METADATA_ENABLE
#define LIVE_METADATA_STREAM_NUM	MULTIPLE_CHANNEL_NUM
#else
#define LIVE_METADATA_STREAM_NUM   0
#endif


#ifdef _MULTICAST
#define RTSPRTP_MULTICAST
#define RTSP_MULTICASTNUMBER	LIVE_STREAM_NUM		//20090305 multiple stream, multicast number follows the total stream number
//20130327 added by Jimmy to support different video/audio multicast addresses for ONVIF Test Tool 12.12
//#define RTSP_AUDIO_EXTRA_MULTICASTNUMBER	RTSP_MULTICASTNUMBER
//20130830 added by Charles for ondemand multicast
//to handle on demand multicast, max ondemand multicast number is MAX_CONNECT_NUM
#define RTSP_ONDEMAND_MULTICASTNUMBER	MAX_CONNECT_NUM //RTSP_MULTICASTNUMBER

#endif

//Moved from rtppacketizer.c 20080619 by Louis
#define LOCATION_LEN			128
//Modified by Louis 20080722 to avoid packetizer.c initialization error
#define MAXIMUM_TRACK_NUMBER		10		//20090305 multiple stream
//Moved here for share-memory 20080807
#define VIDEO_TRACK_NUMBER          MULTIPLE_STREAM_NUM		//20090305 multiple stream, follows the stream number
//Self test for MOD server not finish
//#define AUDIO_TRACK_NUMBER          (0 + LIVE_AUDIO_STREAM_NUM)
#define AUDIO_TRACK_NUMBER            (MOD_STREAM_NUM + LIVE_AUDIO_STREAM_NUM)
//20120731 added by Jimmy for metadata
#define METADATA_TRACK_NUMBER            (LIVE_METADATA_STREAM_NUM)

//Moved here from rtsps_local.h 20080815
#define MAX_MP4V_PACKET_NUM			120		//20090716 modify to increase size
#define MAX_AUDIO_PACKET_NUM        1
//20120731 added by Jimmy for metadata
#define MAX_METADATA_PACKET_NUM        1
//20111209 Modify by danny for support Detect Tampering Watermark
//Added 20080828 to prevent magic number
#define MAX_MP4V_HEADER_SIZE		80//50
//Moevd here from rtsps_fdipc.c 20080829
#define GENERAL_BUFFER_LENGTH	1024
//Moved here from rtspstreaming_local.h 20081215
#define RTSPSTREAMING_VIDEOBUFFER_MAX_NUMBER		12  //8
#define RTSPSTREAMING_VIDEOBUFFER_SIZE				1400//Changed by the DoCoMo's request. 256
#define RTSPSTREAMING_AUDIOBUFFER_MAX_NUMBER		14  //ShengFu 10			
#define RTSPSTREAMING_AUDIO_PACKETTIME_LOCAL		200 //ShengFu 2003/12/9/ original 40
#ifdef _AAC_AUDIO_IN
#define RTSPSTREAMING_AUDIOBUFFER_SIZE 				1200 //Changed by Yenchun's request, originally 700
#else
#define RTSPSTREAMING_AUDIOBUFFER_SIZE 				(40*RTSPSTREAMING_AUDIO_PACKETTIME_LOCAL/20)
#endif
//20120801 added by Jimmy for metadata
#define RTSPSTREAMING_METADATABUFFER_MAX_NUMBER		14
#define RTSPSTREAMING_METADATAXML_SIZE				8192 //5120
#define RTSPSTREAMING_METADATABUFFER_SIZE			1200
//20090206 H.264 Implementation
#define RTSPSTREAMING_MAX_SPROP_PARAM				256
//20090420 SDP size
#define RTSPSTREAMING_SDP_MAXSIZE					1500

extern char	g_acAliasAccessName[MULTIPLE_STREAM_NUM][ACCESSNAME_LENGTH];

#ifdef _SIP_ENABLE
#define _SIP
#define _SIP_TWO_WAY_AUDIO
#define _SIP_USE_RTSPAUTH
//#define _SIP_VVTKONLY
#define SIP_2WAYAUDIO_OTHERVENDOR_SSRC 0xFFFFFFFC
#endif // _SIP_ENABLE

#define _G711_AUDIOIN
#define _G726_AUDIOIN

#ifdef _IPV6_ENABLE
#define _INET6
#endif

#ifdef _INET6
typedef struct sockaddr_in6 RTSP_SOCKADDR;
#else
typedef struct sockaddr_in RTSP_SOCKADDR;
#endif

#define RTSP_AUTHORIZATION
//total amount must be the multiples of 4 byte and
//there would be 4 bytes used as extension header
//add for diagnostic image
#define RTP_EXTENSION   1280//1024

#ifdef _DEBUG
#define PRINTF printf
#else
#define PRINTF  
#endif

typedef enum {
	//! RTP extension Tag: data packet header
	eRTP_EX_DATAPACKETHEADER = 0x01,
	
	//! RTP extension Tag: location
	eRTP_EX_LOCATION         = 0x02,

	//! RTP extension Tag: Intelligent Video by Louis 2007/11/14
	eRTP_EX_IVAEXTENINFO     = 0x03,

	//! RTP extension Tag: For JPEG over RTP extension 20081229
	eRTP_EX_JPEGAPPDATA		= 0x04

} ERTPExTags;

/* Added by cchuang, 2004/11/22 */
#ifdef _LINUX
#include <sys/syslog.h>
#define SEND_FLAGS  MSG_NOSIGNAL
#define RECV_FLAGS	MSG_NOSIGNAL
#else
#define SEND_FLAGS  0
#define RECV_FLAGS  0
#endif

#define RTSPSTREAMING_AMR_MEDIATYPE					97
#define RTSPSTREAMING_MPEG4_MEDIATYPE				96
#define RTSPSTREAMING_JPEG_MEDIATYPE				26
#define RTSPSTREAMING_H264_MEDIATYPE				98
//20150113 added by Charles for H.265
#define RTSPSTREAMING_H265_MEDIATYPE				99
//20120807 added by Jimmy for metadata
#define RTSPSTREAMING_METADATA_MEDIATYPE			107


/*#define RTSP_VIDEO_ACCESS_NAME1			"trackID=1"    //mpeg4 
#define RTSP_AUDIO_ACCESS_NAME1			"trackID=4"    //amr
#define RTSP_AUDIO_ACCESS_NAME2			"trackID=3"    //aac*/

//20130605 added by Jimmy to support metadata event
#ifdef _METADATA_EVENT_ENABLE
#define SHMEM_HANDLE_MAX_NUM 2
#else
#define SHMEM_HANDLE_MAX_NUM 1
#endif

#ifdef _SHARED_MEM
/* 20100428 Added For Media on demand */
int RtspRtpCommon_TLVStrlen(char *qptr, int iMsgLength);
#endif

#endif

