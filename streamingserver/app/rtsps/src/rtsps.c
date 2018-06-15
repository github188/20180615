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
 * rtsps.c
 *
 * \brief
 * rtsp streaming server for Kilrogg.
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
 
#include <arpa/inet.h>

#include <stdlib.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include "osisolate.h"
#include "rtsps_local.h"
#include "rtsps_fdipc.h"
#include "rtsps_ubuffer.h"
#include "rtsps_callback.h"
#include "xmlsparser.h"
//20090305 Multiple Stream
char	acVideoTrackName[VIDEO_TRACK_NUMBER][MEDIA_TRACK_NAME_LEN];
char	acAudioTrackName[AUDIO_TRACK_NUMBER][MEDIA_TRACK_NAME_LEN];
//20120731 added by Jimmy for metadata
char	acMetadataTrackName[METADATA_TRACK_NUMBER][MEDIA_TRACK_NAME_LEN];


int QueryCameraMode(); //20161212 add by Faber, check auth status

//20100714 Modified by danny For Multicast parameters load dynamically
//#define	VIDEOMEDIACOMPONENTSNUMBER	12		//Define how many components in g_VideoMediaParseMap
//20120815 modified by Jimmy for metadata
#ifdef _METADATA_ENABLE
#define	VIDEOMEDIACOMPONENTSNUMBER	7		//Define how many components in g_VideoMediaParseMap
#else
#define	VIDEOMEDIACOMPONENTSNUMBER	6		//Define how many components in g_VideoMediaParseMap
#endif
#define	AUDIOMEDIACOMPONENTSNUMBER	2		//Define how many components in g_AudioMediaParseMap
//20120810 added by Jimmy for metadata
#define	METADATAMEDIACOMPONENTSNUMBER	2	//Define how many components in g_MetadataMediaParseMap
#ifdef _SHARED_MEM
//20101210 Added by danny For Media shmem config
#define	VIDEOSHMEMCOMPONENTSNUMBER	2		//Define how many components in g_VideoShmemParseMap
#define	AUDIOSHMEMCOMPONENTSNUMBER	2		//Define how many components in g_AudioShmemParseMap
//20120810 added by Jimmy for metadata
#define	METADATASHMEMCOMPONENTSNUMBER	2	//Define how many components in g_MetadataShmemParseMap
#endif
//20110725 Add by danny For Multicast RTCP receive report keep alive
//20110630 Add by danny For Multicast enable/disable
//20100714 Added by danny For Multicast parameters load dynamically
//20120810 modified by Jimmy for metadata
//20160127 Modified by Faber, add audio/metadata multicast address
#define	MULTICASTCOMPONENTSNUMBER	11		//Define how many components in g_MulticastParseMap
#define XMLPATHLENGTH			256

#ifdef _SHARED_MEM
//20100105 Added For Seamless Recording
#define	GUIDLISTINFOCOMPONENTSNUMBER	2		//Define how many components in g_GUIDListInfoParseMap
#endif

TSTREAMSERVERINFO  tStreamServerInfo;
TQosInfo		   tQosInfo;

#ifdef _SHARED_MEM
//20100105 Added For Seamless Recording
TSeamlessRecordingInfo 	tSeamlessRecordingInfo;
#endif

//20090305 Multiple Stream
SCODE	SetupTrackName()
{
	/* eg. Video track is 1,2,3,4 audio is 5,6,7,8...*/
	int		i = 0;
	for(i = 0; i < VIDEO_TRACK_NUMBER; i++)
	{
		snprintf(acVideoTrackName[i], MEDIA_TRACK_NAME_LEN - 1, "trackID=%d", i + 1);
		acVideoTrackName[i][MEDIA_TRACK_NAME_LEN - 1] = 0;
	}
	for(i = 0; i < AUDIO_TRACK_NUMBER; i++)
	{
		snprintf(acAudioTrackName[i], MEDIA_TRACK_NAME_LEN - 1, "trackID=%d", i + VIDEO_TRACK_NUMBER+ 1);
		acAudioTrackName[i][MEDIA_TRACK_NAME_LEN - 1] = 0;
	}
	//20120731 added by Jimmy for metadata
	for(i = 0; i < METADATA_TRACK_NUMBER; i++)
	{
		snprintf(acMetadataTrackName[i], MEDIA_TRACK_NAME_LEN - 1, "trackID=%d", i + VIDEO_TRACK_NUMBER + AUDIO_TRACK_NUMBER+ 1);
		acMetadataTrackName[i][MEDIA_TRACK_NAME_LEN - 1] = 0;
	}	

	return S_OK;
}

SCODE CfgParser_GetUnixDomainSocket(void *pData, void *pParam)
{
	int	iFStatus;
	SOCKET sckSocket ;

    //printf("socket path is %s\r\n",pData);   
    if (( *(int*)pParam = create_unix_socket(pData)) == -1)
	{
		printf("Create UNIX socket %s failed!\n", (char *)pData);
		return S_FAIL;
	}

	sckSocket = *(SOCKET*)pParam ;

	iFStatus = fcntl(sckSocket, F_GETFL);
	fcntl(sckSocket, F_SETFL, iFStatus | O_NONBLOCK);
    return S_OK;    
}

SCODE CfgParser_GetFIFO(char *pData, int *piFIFO, int iFlag)
{
    int iFIFO,res;
    
    printf("FIFO path = %s\r\n",(char*)pData);
    
    if(access((char*)pData, F_OK) == -1)
    {
        res = mkfifo((char*)pData, 0777);
        if (res != 0)
        {
            printf("Could not create fifo %s\n",(char*)pData);
            return S_FAIL;
        }
    }    
    
    //use blocking mode to open write or it will return -1 
    //if open read is not ready yet 
    //20090416 Modified again to be non-blocking
    if( iFlag == O_RDONLY )
	{
    	iFIFO = open( pData, O_RDONLY | O_NONBLOCK);
	}
	else    	
	{
		while((iFIFO = open( pData, O_WRONLY| O_NONBLOCK)) <= 0)
		{
			OSSleep_MSec(100);
		}
	}
		
		
    if(  iFIFO  < 0  )
    {
        printf("open fifo %s failed %d\r\n",pData, errno);
        return S_FAIL;    
    }
    else 
    {
        printf("FIFO %s is %d\r\n",pData, iFIFO);
        *(int*)piFIFO = iFIFO;
        return S_OK;    
    }
}

SCODE CfgParser_GetIPAddress(void *pData, void *pParam)
{
    
    if( (*(DWORD*)pParam = inet_addr(pData)) == 0)
    {
        printf("get IP address failed for %s\r\n", (char *)pData);
        return S_FAIL;
    } 
    else    
        return S_OK;
}   

SCODE CfgParser_GetMulticastIPAddress(void *pData, void *pParam)
{
    
	*(DWORD*)pParam = inet_addr(pData);
	//printf("get Multicast IP address for %s, %u\r\n", (char *)pData, *(DWORD*)pParam);
	return S_OK;
}   

SCODE CfgParser_GetAuthenticateMode(void *pData, void *pParam)
{
    if( strcmp(pData,"basic") == 0 )
    {   
        *(DWORD*)pParam = RTSPSTREAMING_AUTHENTICATION_BASIC;
    }
    else if( strcmp(pData,"digest") == 0 )
    {
        *(DWORD*)pParam = RTSPSTREAMING_AUTHENTICATION_DIGEST;
    }
    else if( strcmp(pData,"disable") == 0 )
    {
        *(DWORD*)pParam = RTSPSTREAMING_AUTHENTICATION_DISABLE;
    }
    else
    {
        printf("get auth mode failed\r\n");
        return S_FAIL;
    }
    return S_OK;
}

TCfgParseMap g_AcsParseMap[] = {
{"/root/ipfilter/allow/i0/start", csmGetbyFunc, 4, &tStreamServerInfo.tAllowIP[0].ulStartIP	,CfgParser_GetIPAddress},
{"/root/ipfilter/allow/i0/end"	, csmGetbyFunc, 4, &tStreamServerInfo.tAllowIP[0].ulEndIP	,CfgParser_GetIPAddress},
{"/root/ipfilter/allow/i1/start", csmGetbyFunc, 4, &tStreamServerInfo.tAllowIP[1].ulStartIP	,CfgParser_GetIPAddress},
{"/root/ipfilter/allow/i1/end"	, csmGetbyFunc, 4, &tStreamServerInfo.tAllowIP[1].ulEndIP	,CfgParser_GetIPAddress},
{"/root/ipfilter/allow/i2/start", csmGetbyFunc, 4, &tStreamServerInfo.tAllowIP[2].ulStartIP	,CfgParser_GetIPAddress},
{"/root/ipfilter/allow/i2/end"	, csmGetbyFunc, 4, &tStreamServerInfo.tAllowIP[2].ulEndIP	,CfgParser_GetIPAddress},
{"/root/ipfilter/allow/i3/start", csmGetbyFunc, 4, &tStreamServerInfo.tAllowIP[3].ulStartIP	,CfgParser_GetIPAddress},
{"/root/ipfilter/allow/i3/end"	, csmGetbyFunc, 4, &tStreamServerInfo.tAllowIP[3].ulEndIP	,CfgParser_GetIPAddress},
{"/root/ipfilter/allow/i4/start", csmGetbyFunc, 4, &tStreamServerInfo.tAllowIP[4].ulStartIP	,CfgParser_GetIPAddress},
{"/root/ipfilter/allow/i4/end"	, csmGetbyFunc, 4, &tStreamServerInfo.tAllowIP[4].ulEndIP	,CfgParser_GetIPAddress},
{"/root/ipfilter/allow/i5/start", csmGetbyFunc, 4, &tStreamServerInfo.tAllowIP[5].ulStartIP	,CfgParser_GetIPAddress},
{"/root/ipfilter/allow/i5/end"	, csmGetbyFunc, 4, &tStreamServerInfo.tAllowIP[5].ulEndIP	,CfgParser_GetIPAddress},
{"/root/ipfilter/allow/i6/start", csmGetbyFunc, 4, &tStreamServerInfo.tAllowIP[6].ulStartIP	,CfgParser_GetIPAddress},
{"/root/ipfilter/allow/i6/end"	, csmGetbyFunc, 4, &tStreamServerInfo.tAllowIP[6].ulEndIP	,CfgParser_GetIPAddress},
{"/root/ipfilter/allow/i7/start", csmGetbyFunc, 4, &tStreamServerInfo.tAllowIP[7].ulStartIP	,CfgParser_GetIPAddress},
{"/root/ipfilter/allow/i7/end"	, csmGetbyFunc, 4, &tStreamServerInfo.tAllowIP[7].ulEndIP	,CfgParser_GetIPAddress},
{"/root/ipfilter/allow/i8/start", csmGetbyFunc, 4, &tStreamServerInfo.tAllowIP[8].ulStartIP	,CfgParser_GetIPAddress},
{"/root/ipfilter/allow/i8/end"	, csmGetbyFunc, 4, &tStreamServerInfo.tAllowIP[8].ulEndIP	,CfgParser_GetIPAddress},
{"/root/ipfilter/allow/i9/start", csmGetbyFunc, 4, &tStreamServerInfo.tAllowIP[9].ulStartIP	,CfgParser_GetIPAddress},
{"/root/ipfilter/allow/i9/end"	, csmGetbyFunc, 4, &tStreamServerInfo.tAllowIP[9].ulEndIP	,CfgParser_GetIPAddress},
{"/root/ipfilter/deny/i0/start"	, csmGetbyFunc, 4, &tStreamServerInfo.tDenyIP[0].ulStartIP	,CfgParser_GetIPAddress},
{"/root/ipfilter/deny/i0/end"	, csmGetbyFunc, 4, &tStreamServerInfo.tDenyIP[0].ulEndIP	,CfgParser_GetIPAddress},
{"/root/ipfilter/deny/i1/start"	, csmGetbyFunc, 4, &tStreamServerInfo.tDenyIP[1].ulStartIP	,CfgParser_GetIPAddress},
{"/root/ipfilter/deny/i1/end"	, csmGetbyFunc, 4, &tStreamServerInfo.tDenyIP[1].ulEndIP	,CfgParser_GetIPAddress},
{"/root/ipfilter/deny/i2/start"	, csmGetbyFunc, 4, &tStreamServerInfo.tDenyIP[2].ulStartIP	,CfgParser_GetIPAddress},
{"/root/ipfilter/deny/i2/end"	, csmGetbyFunc, 4, &tStreamServerInfo.tDenyIP[2].ulEndIP	,CfgParser_GetIPAddress},
{"/root/ipfilter/deny/i3/start"	, csmGetbyFunc, 4, &tStreamServerInfo.tDenyIP[3].ulStartIP	,CfgParser_GetIPAddress},
{"/root/ipfilter/deny/i3/end"	, csmGetbyFunc, 4, &tStreamServerInfo.tDenyIP[3].ulEndIP	,CfgParser_GetIPAddress},
{"/root/ipfilter/deny/i4/start"	, csmGetbyFunc, 4, &tStreamServerInfo.tDenyIP[4].ulStartIP	,CfgParser_GetIPAddress},
{"/root/ipfilter/deny/i4/end"	, csmGetbyFunc, 4, &tStreamServerInfo.tDenyIP[4].ulEndIP	,CfgParser_GetIPAddress},
{"/root/ipfilter/deny/i5/start"	, csmGetbyFunc, 4, &tStreamServerInfo.tDenyIP[5].ulStartIP	,CfgParser_GetIPAddress},
{"/root/ipfilter/deny/i5/end"	, csmGetbyFunc, 4, &tStreamServerInfo.tDenyIP[5].ulEndIP	,CfgParser_GetIPAddress},
{"/root/ipfilter/deny/i6/start"	, csmGetbyFunc, 4, &tStreamServerInfo.tDenyIP[6].ulStartIP	,CfgParser_GetIPAddress},
{"/root/ipfilter/deny/i6/end"	, csmGetbyFunc, 4, &tStreamServerInfo.tDenyIP[6].ulEndIP	,CfgParser_GetIPAddress},
{"/root/ipfilter/deny/i7/start"	, csmGetbyFunc, 4, &tStreamServerInfo.tDenyIP[7].ulStartIP	,CfgParser_GetIPAddress},
{"/root/ipfilter/deny/i7/end"	, csmGetbyFunc, 4, &tStreamServerInfo.tDenyIP[7].ulEndIP	,CfgParser_GetIPAddress},
{"/root/ipfilter/deny/i8/start"	, csmGetbyFunc, 4, &tStreamServerInfo.tDenyIP[8].ulStartIP	,CfgParser_GetIPAddress},
{"/root/ipfilter/deny/i8/end"	, csmGetbyFunc, 4, &tStreamServerInfo.tDenyIP[8].ulEndIP	,CfgParser_GetIPAddress},
{"/root/ipfilter/deny/i9/start"	, csmGetbyFunc, 4, &tStreamServerInfo.tDenyIP[9].ulStartIP	,CfgParser_GetIPAddress},
{"/root/ipfilter/deny/i9/end"	, csmGetbyFunc, 4, &tStreamServerInfo.tDenyIP[9].ulEndIP	,CfgParser_GetIPAddress},
{NULL, 0, 0, NULL, NULL}
};
 
//20110725 Add by danny For Multicast RTCP receive report keep alive
TCfgParseMap g_CfgParseMap[] = {
#ifdef _SIP
{"/root/network/sip/port", citInteger|csmGetbyVal, 2, &tStreamServerInfo.tSIPInfo.usPort, NULL},
{"/root/network/sip/registrarip", csmGetbyFunc, 4, &tStreamServerInfo.tSIPInfo.ulRegistrarIP,CfgParser_GetIPAddress},
{"/root/network/sip/registrarport", citInteger|csmGetbyVal, 2, &tStreamServerInfo.tSIPInfo.usRegistrarPort, NULL},
{"/root/network/sip/outboundproxyip", csmGetbyFunc, 4, &tStreamServerInfo.tSIPInfo.ulOBProxyIP,CfgParser_GetIPAddress},
{"/root/network/sip/outboundproxyport", citInteger|csmGetbyVal, 2, &tStreamServerInfo.tSIPInfo.usOBProxyPort,NULL},
{"/root/network/sip/displayname", citString|csmGetbyVal, 254, tStreamServerInfo.tSIPInfo.szDisplayName, NULL},
{"/root/network/sip/username", citString|csmGetbyVal, 254, tStreamServerInfo.tSIPInfo.szUserName, NULL},
{"/root/network/sip/password", citString|csmGetbyVal, 254, tStreamServerInfo.tSIPInfo.szPassword, NULL},
{"/root/network/sip/domain", citString|csmGetbyVal, 254, tStreamServerInfo.tSIPInfo.szSIPDomain, NULL},
#ifndef _SIP_USE_RTSPAUTH
{"/root/network/sip/authmode", csmGetbyFunc, 4, &tStreamServerInfo.tSIPInfo.iAuthenticateMode,CfgParser_GetAuthenticateMode},
#endif
{"/root/network/sip/maxconnnum", citInteger|csmGetbyVal, 4, &tStreamServerInfo.tSIPInfo.iMaxConnectionNum,NULL},
{"/root/network/sip/sessioninfopath", citString|csmGetbyVal, 254, tStreamServerInfo.tSIPInfo.szSessionInfoPath,NULL},
#endif
{"/root/network/rtsp/port", citInteger|csmGetbyVal, 2, &tStreamServerInfo.tRTSPInfo.usRTSPPort,NULL},
{"/root/network/rtsp/authmode", csmGetbyFunc, 4, &tStreamServerInfo.tRTSPInfo.iAuthenticateMode,CfgParser_GetAuthenticateMode},
{"/root/network/rtsp/maxconnnum", citInteger|csmGetbyVal, 4, &tStreamServerInfo.tRTSPInfo.iMaxConnectionNum,NULL},
#ifdef RTSPRTP_MULTICAST
{"/root/network/rtsp/multicasttimeout", citInteger|csmGetbyVal, 4, &g_iMulticastTimeout, NULL},
#endif
{"/root/network/rtsp/sessioninfopath", citString|csmGetbyVal, 254, tStreamServerInfo.tRTSPInfo.szSessionInfoPath,NULL},
{"/root/network/rtsp/httpfdipcsock", citString|csmGetbyVal, 254, tStreamServerInfo.tRTSPInfo.szHTTPfdipcSock,NULL},
{"/root/network/rtsp/controlipc", citString|csmGetbyVal, 254, tStreamServerInfo.tRTSPInfo.szContorlipcFIFO,NULL},
{"/root/network/rtsp/urlauthenable", citInteger|csmGetbyVal, 4, &tStreamServerInfo.tRTSPInfo.iURLAuthEnabled, NULL},	 //20081121 URL auth
{"/root/network/rtsp/a0/decodesocket", citString|csmGetbyVal, 254, &tStreamServerInfo.tAudioDstInfo.acSockPathName,NULL},
{"/root/network/rtsp/a0/decodefifo", citString|csmGetbyVal, 254, &tStreamServerInfo.tAudioDstInfo.acFIFOPathName,NULL},
{"/root/network/rtp/videoport", citInteger|csmGetbyVal, 2, &tStreamServerInfo.tRTPInfo.usRTPVideoPort,NULL},
{"/root/network/rtp/audioport", citInteger|csmGetbyVal, 2, &tStreamServerInfo.tRTPInfo.usRTPAudioPort,NULL},
//20120810 added by Jimmy for metadata
{"/root/network/rtp/metadataport", citInteger|csmGetbyVal, 2, &tStreamServerInfo.tRTPInfo.usRTPMetadataPort,NULL},
{NULL, 0, 0, NULL, NULL}
};
 
//20090306 Media Parameter for multiple streams
TCfgParseMap g_VideoMediaParseMap[] = {
{"/root/network/rtsp/v%d/socket", citString|csmGetbyVal, 254, "&tStreamServerInfo.tVideoSrcInfo[%d].acSockPathName", NULL},  
{"/root/network/rtsp/v%d/fifo", citString|csmGetbyVal, 254, "&tStreamServerInfo.tVideoSrcInfo[%d].acFIFOPathName",NULL},
{"/root/network/rtsp/s%d/enable", citInteger|csmGetbyVal, 4, "&tStreamServerInfo.tStreamInfo[%d].iEnable",NULL},
{"/root/network/rtsp/s%d/accessname", citString|csmGetbyVal, 149, "&tStreamServerInfo.tStreamInfo[%d].szAccessName",NULL},
{"/root/network/rtsp/s%d/videotrack", citInteger|csmGetbyVal, 4, "&tStreamServerInfo.tStreamInfo[%d].iVideoSrcIndex",NULL},
{"/root/network/rtsp/s%d/audiotrack", citInteger|csmGetbyVal, 4, "&tStreamServerInfo.tStreamInfo[%d].iAudioSrcIndex",NULL},
#ifdef _METADATA_ENABLE
//20120810 added by Jimmy for metadata
{"/root/network/rtsp/s%d/metadatatrack", citInteger|csmGetbyVal, 4, "&tStreamServerInfo.tStreamInfo[%d].iMetadataSrcIndex",NULL},
#endif
//20100714 Modified by danny For Multicast parameters load dynamically
/*{"/root/network/rtsp/s%d/multicast/alwaysmulticast", citInteger|csmGetbyVal, 4, "&tStreamServerInfo.tMulticastInfo[%d].iAlwaysMulticast",NULL},
{"/root/network/rtsp/s%d/multicast/videoport", citInteger|csmGetbyVal, 2, "&tStreamServerInfo.tMulticastInfo[%d].usMulticastVideoPort",NULL},
{"/root/network/rtsp/s%d/multicast/audioport", citInteger|csmGetbyVal, 2, "&tStreamServerInfo.tMulticastInfo[%d].usMulticastAudioPort",NULL},
{"/root/network/rtsp/s%d/multicast/ipaddress", citInteger|csmGetbyFunc, 4, "&tStreamServerInfo.tMulticastInfo[%d].ulMulticastAddress",CfgParser_GetIPAddress},
{"/root/network/rtsp/s%d/multicast/ttl", citInteger|csmGetbyVal, 4, "&tStreamServerInfo.tMulticastInfo[%d].usTTL",NULL},
{"/root/network/rtsp/s%d/multicast/vvtkextension", citInteger|csmGetbyVal, 4, "&tStreamServerInfo.tMulticastInfo[%d].iRTPExtension",NULL},*/
{NULL, 0, 0, NULL, NULL}
};

TCfgParseMap g_AudioMediaParseMap[] = {
{"/root/network/rtsp/a%d/socket", citString|csmGetbyVal, 254, "&tStreamServerInfo.tAudioSrcInfo[%d].acSockPathName", NULL},
{"/root/network/rtsp/a%d/fifo", citString|csmGetbyVal, 254, "&tStreamServerInfo.tAudioSrcInfo[%d].acFIFOPathName", NULL},
{NULL, 0, 0, NULL, NULL}
};

//20120810 added by Jimmy for metadata
TCfgParseMap g_MetadataMediaParseMap[] = {
{"/root/network/rtsp/m%d/socket", citString|csmGetbyVal, 254, "&tStreamServerInfo.tMetadataSrcInfo[%d].acSockPathName", NULL},
{"/root/network/rtsp/m%d/fifo", citString|csmGetbyVal, 254, "&tStreamServerInfo.tMetadataSrcInfo[%d].acFIFOPathName", NULL},
{NULL, 0, 0, NULL, NULL}
};


#ifdef _SHARED_MEM
//20101210 Added by danny For Media shmem config
TCfgParseMap g_VideoShmemParseMap[] = {
{"/root/network/rtsp/v%d/block", citInteger|csmGetbyVal, 4, "&tStreamServerInfo.tVideoSrcInfo[%d].iBlockIndex", NULL},
{"/root/network/rtsp/v%d/sector", citInteger|csmGetbyVal, 4, "&tStreamServerInfo.tVideoSrcInfo[%d].iSectorIndex", NULL},
{NULL, 0, 0, NULL, NULL}
};
TCfgParseMap g_AudioShmemParseMap[] = {
{"/root/network/rtsp/a%d/block", citInteger|csmGetbyVal, 4, "&tStreamServerInfo.tAudioSrcInfo[%d].iBlockIndex", NULL},
{"/root/network/rtsp/a%d/sector", citInteger|csmGetbyVal, 4, "&tStreamServerInfo.tAudioSrcInfo[%d].iSectorIndex", NULL},
{NULL, 0, 0, NULL, NULL}
};
//20120810 added by Jimmy for metadata
TCfgParseMap g_MetadataShmemParseMap[] = {
{"/root/network/rtsp/m%d/block", citInteger|csmGetbyVal, 4, "&tStreamServerInfo.tMetadataSrcInfo[%d].iBlockIndex", NULL},
{"/root/network/rtsp/m%d/sector", citInteger|csmGetbyVal, 4, "&tStreamServerInfo.tMetadataSrcInfo[%d].iSectorIndex", NULL},
{NULL, 0, 0, NULL, NULL}
};
#endif

#ifdef RTSPRTP_MULTICAST
//20110725 Add by danny For Multicast RTCP receive report keep alive
//20110630 Add by danny For Multicast enable/disable
//20100714 Added by danny For Multicast parameters load dynamically
TCfgParseMap g_MulticastParseMap[] = {
{"/root/network/rtsp/s%d/multicast/enable", citInteger|csmGetbyVal, 4, "&tStreamServerInfo.tMulticastInfo[%d].iEnable",NULL},
{"/root/network/rtsp/s%d/multicast/rralive", citInteger|csmGetbyVal, 4, "&tStreamServerInfo.tMulticastInfo[%d].iRRAlive",NULL},
{"/root/network/rtsp/s%d/multicast/alwaysmulticast", citInteger|csmGetbyVal, 4, "&tStreamServerInfo.tMulticastInfo[%d].iAlwaysMulticast",NULL},
{"/root/network/rtsp/s%d/multicast/videoport", citInteger|csmGetbyVal, 2, "&tStreamServerInfo.tMulticastInfo[%d].usMulticastVideoPort",NULL},
{"/root/network/rtsp/s%d/multicast/audioport", citInteger|csmGetbyVal, 2, "&tStreamServerInfo.tMulticastInfo[%d].usMulticastAudioPort",NULL},
//20120810 added by Jimmy for metadata
{"/root/network/rtsp/s%d/multicast/metadataport", citInteger|csmGetbyVal, 2, "&tStreamServerInfo.tMulticastInfo[%d].usMulticastMetadataPort",NULL},
{"/root/network/rtsp/s%d/multicast/videoipaddress", citInteger|csmGetbyFunc, 4, "&tStreamServerInfo.tMulticastInfo[%d].ulMulticastAddress",CfgParser_GetMulticastIPAddress},//CfgParser_GetIPAddress},
//20160127 added by Faber, for parsing audio/metadata multicast address
{"/root/network/rtsp/s%d/multicast/audioipaddress", citInteger|csmGetbyFunc, 4, "&tStreamServerInfo.tMulticastInfo[%d].ulMulticastAudioAddress",CfgParser_GetMulticastIPAddress},//CfgParser_GetIPAddress},
{"/root/network/rtsp/s%d/multicast/metaipaddress", citInteger|csmGetbyFunc, 4, "&tStreamServerInfo.tMulticastInfo[%d].ulMulticastMetadataAddress",CfgParser_GetMulticastIPAddress},//CfgParser_GetIPAddress},
{"/root/network/rtsp/s%d/multicast/ttl", citInteger|csmGetbyVal, 4, "&tStreamServerInfo.tMulticastInfo[%d].usTTL",NULL},
{"/root/network/rtsp/s%d/multicast/vvtkextension", citInteger|csmGetbyVal, 4, "&tStreamServerInfo.tMulticastInfo[%d].iRTPExtension",NULL},
{NULL, 0, 0, NULL, NULL}
};
//20130327 added by Jimmy to support different video/audio multicast addresses for ONVIF Test Tool 12.12
TCfgParseMap g_AudioExtraMulticastParseMap[] = {
{"/root/network/rtsp/s%d/multicast/enable", citInteger|csmGetbyVal, 4, "&tStreamServerInfo.tMulticastInfo[%d].iEnable",NULL},
{"/root/network/rtsp/s%d/multicast/rralive", citInteger|csmGetbyVal, 4, "&tStreamServerInfo.tMulticastInfo[%d].iRRAlive",NULL},
{"/root/network/rtsp/s%d/multicast/alwaysmulticast", citInteger|csmGetbyVal, 4, "&tStreamServerInfo.tMulticastInfo[%d].iAlwaysMulticast",NULL},
{"/root/network/rtsp/s%d/multicast/videoport", citInteger|csmGetbyVal, 2, "&tStreamServerInfo.tMulticastInfo[%d].usMulticastVideoPort",NULL},
{"/root/network/rtsp/s%d/multicast/audioport", citInteger|csmGetbyVal, 2, "&tStreamServerInfo.tMulticastInfo[%d].usMulticastAudioPort",NULL},
{"/root/network/rtsp/s%d/multicast/metadataport", citInteger|csmGetbyVal, 2, "&tStreamServerInfo.tMulticastInfo[%d].usMulticastMetadataPort",NULL},
{"/root/network/rtsp/s%d/multicast/audioextraipaddress", citInteger|csmGetbyFunc, 4, "&tStreamServerInfo.tMulticastInfo[%d].ulMulticastAddress",CfgParser_GetMulticastIPAddress},//CfgParser_GetIPAddress},
{"/root/network/rtsp/s%d/multicast/ttl", citInteger|csmGetbyVal, 4, "&tStreamServerInfo.tMulticastInfo[%d].usTTL",NULL},
{"/root/network/rtsp/s%d/multicast/vvtkextension", citInteger|csmGetbyVal, 4, "&tStreamServerInfo.tMulticastInfo[%d].iRTPExtension",NULL},
{NULL, 0, 0, NULL, NULL}
};

#endif

//Added by Louis 2008/01/29 for DVTEL multicast
TCfgParseMap g_DVTELMulticastParseMap[] = {
{"/root/network/rtsp/s0/multicast/audio", citInteger|csmGetbyVal, 4, &tStreamServerInfo.tMulticastInfo[0].iAlwaysMulticastAudio, NULL},
{"/root/network/rtsp/s0/multicast/video", citInteger|csmGetbyVal, 4, &tStreamServerInfo.tMulticastInfo[0].iAlwaysMulticastVideo, NULL},
//20120810 added by Jimmy for metadata
{"/root/network/rtsp/s0/multicast/metadata", citInteger|csmGetbyVal, 4, &tStreamServerInfo.tMulticastInfo[0].iAlwaysMulticastMetadata, NULL},
{"/root/network/rtsp/s1/multicast/audio", citInteger|csmGetbyVal, 4, &tStreamServerInfo.tMulticastInfo[1].iAlwaysMulticastAudio, NULL},
{"/root/network/rtsp/s1/multicast/video", citInteger|csmGetbyVal, 4, &tStreamServerInfo.tMulticastInfo[1].iAlwaysMulticastVideo, NULL},
//20120810 added by Jimmy for metadata
{"/root/network/rtsp/s1/multicast/metadata", citInteger|csmGetbyVal, 4, &tStreamServerInfo.tMulticastInfo[1].iAlwaysMulticastMetadata, NULL},
{NULL, 0, 0, NULL, NULL}
};

// 20090223 QOS
TCfgParseMap g_QosParseMap[] = {
{"/root/network/qos/cos/enable", citInteger|csmGetbyVal, 4, &tQosInfo.iCosEnabled, NULL},
{"/root/network/qos/cos/video",  citInteger|csmGetbyVal, 4, &tQosInfo.iCosVideoPriority, NULL},
{"/root/network/qos/cos/audio",  citInteger|csmGetbyVal, 4, &tQosInfo.iCosAudioPriority, NULL},
//20120810 added by Jimmy for metadata
{"/root/network/qos/cos/metadata",  citInteger|csmGetbyVal, 4, &tQosInfo.iCosMetadataPriority, NULL},
{"/root/network/qos/dscp/enable", citInteger|csmGetbyVal, 4, &tQosInfo.iDscpEnabled, NULL},
{"/root/network/qos/dscp/video",  citInteger|csmGetbyVal, 4, &tQosInfo.iDscpVideoPriority, NULL},
{"/root/network/qos/dscp/audio",  citInteger|csmGetbyVal, 4, &tQosInfo.iDscpAudioPriority, NULL},
//20120810 added by Jimmy for metadata
{"/root/network/qos/dscp/metadata",  citInteger|csmGetbyVal, 4, &tQosInfo.iDscpMetadataPriority, NULL},
{NULL, 0, 0, NULL, NULL}
};

#ifdef _SHARED_MEM
//20101208 Modified by danny For GUID format change
//20100105 Added For Seamless Recording
TCfgParseMap g_GUIDListInfoParseMap[] = {
{"/root/seamlessrecording/guid%d/id", citString|csmGetbyVal, RTSPS_Seamless_Recording_GUID_LENGTH, "&tSeamlessRecordingInfo.tGUIDListInfo[%d].acGUID", NULL},  
{"/root/seamlessrecording/guid%d/number", citInteger|csmGetbyVal, 4, "&tSeamlessRecordingInfo.tGUIDListInfo[%d].iNumber", NULL},
{NULL, 0, 0, NULL, NULL}
};

//20110915 Modify by danny for support Genetec MOD
//20100105 Added For Media on demand
//20141110 modified by Charles for ONVIF Profile G
TKN g_MODCommand [] =
{
   {"playspeed=", MOD_PLAYSPEED},
   {"playscale=", MOD_PLAYSCALE},
   {"pause=", MOD_PAUSE},
   {"pause=", MOD_RESUME},
   {"immediate=", MOD_IMMEDIATE},
   {"syncframeonly", MOD_SYNCFRAMEONLY}, //20160623 add by Faber, intra only
   {"", MOD_RANGE},
   {0, -1}
};

char  				g_acInvld_command[] = "Invalid Command";
unsigned long    	_needs_escape[(NEEDS_ESCAPE_BITS + NEEDS_ESCAPE_WORD_LENGTH - 1) / NEEDS_ESCAPE_WORD_LENGTH];
#endif

SCODE SetUpRTSPServer(TSTREAMSERVERINFO *pThis)
{
	int i;
	//20130605 added by Jimmy to support metadata event
	int j;
	TRTSPSTREAMING_PARAM		     stRTSPStreamingParam;
	//20080509 Initialize Thread priority
	memset(&stRTSPStreamingParam, 0, sizeof(TRTSPSTREAMING_PARAM));

	stRTSPStreamingParam.usRTSPPort = pThis->tRTSPInfo.usRTSPPort;
	stRTSPStreamingParam.usRTPVPort = pThis->tRTPInfo.usRTPVideoPort;
	stRTSPStreamingParam.usRTPAPort = pThis->tRTPInfo.usRTPAudioPort;
	//20120801 added by Jimmy for metadata
	stRTSPStreamingParam.usRTPMPort = pThis->tRTPInfo.usRTPMetadataPort;
	stRTSPStreamingParam.usRTCPVPort= pThis->tRTPInfo.usRTPVideoPort + 1;
	stRTSPStreamingParam.usRTCPAPort= pThis->tRTPInfo.usRTPAudioPort + 1;
	//20120801 added by Jimmy for metadata
	stRTSPStreamingParam.usRTCPMPort= pThis->tRTPInfo.usRTPMetadataPort + 1;

	printf("rtsp port %d\r\n",pThis->tRTSPInfo.usRTSPPort);
	printf("rtp video port %d\r\n",pThis->tRTPInfo.usRTPVideoPort);
	printf("rtp audio port %d\r\n",pThis->tRTPInfo.usRTPAudioPort);
	//20120801 added by Jimmy for metadata
	printf("rtp metadata port %d\r\n",pThis->tRTPInfo.usRTPMetadataPort);
	
#ifdef _MULTICAST
	//20130327 modified by Jimmy to support different video/audio multicast addresses for ONVIF Test Tool 12.12
	for( i=0 ; i<(RTSP_MULTICASTNUMBER) ; i++)
	{
		//20110630 Add by danny For Multicast enable/disable
		stRTSPStreamingParam.stMulticastInfo[i].iEnable = pThis->tMulticastInfo[i].iEnable;
		//20110725 Add by danny For Multicast RTCP receive report keep alive
		stRTSPStreamingParam.stMulticastInfo[i].iRRAlive = pThis->tMulticastInfo[i].iRRAlive;
		stRTSPStreamingParam.stMulticastInfo[i].ulMulticastAddress = pThis->tMulticastInfo[i].ulMulticastAddress;
		//20160127 Add by Faber
		stRTSPStreamingParam.stMulticastInfo[i].ulMulticastAudioAddress = pThis->tMulticastInfo[i].ulMulticastAudioAddress;
		stRTSPStreamingParam.stMulticastInfo[i].ulMulticastMetadataAddress = pThis->tMulticastInfo[i].ulMulticastMetadataAddress;
		stRTSPStreamingParam.stMulticastInfo[i].usMulticastVideoPort = pThis->tMulticastInfo[i].usMulticastVideoPort;
		stRTSPStreamingParam.stMulticastInfo[i].usMulticastAudioPort = pThis->tMulticastInfo[i].usMulticastAudioPort;
		//20120801 added by Jimmy for metadata
		stRTSPStreamingParam.stMulticastInfo[i].usMulticastMetadataPort= pThis->tMulticastInfo[i].usMulticastMetadataPort;
		stRTSPStreamingParam.stMulticastInfo[i].usTTL = pThis->tMulticastInfo[i].usTTL;
		stRTSPStreamingParam.stMulticastInfo[i].iAlwaysMulticast = pThis->tMulticastInfo[i].iAlwaysMulticast;
		stRTSPStreamingParam.stMulticastInfo[i].iRTPExtension = pThis->tMulticastInfo[i].iRTPExtension;
		//20130327 modified by Jimmy to support different video/audio multicast addresses for ONVIF Test Tool 12.12
		if(i >= RTSP_MULTICASTNUMBER && i < (RTSP_MULTICASTNUMBER))
		{
			stRTSPStreamingParam.stMulticastInfo[i].iSDPIndex = pThis->tMulticastInfo[i].iSDPIndex = (i-RTSP_MULTICASTNUMBER)+1;
		}
		else
		{
			stRTSPStreamingParam.stMulticastInfo[i].iSDPIndex = pThis->tMulticastInfo[i].iSDPIndex = i+1 ;
		}
		//Added by Louis 2008/01/29 for always multicast audio/video only
		stRTSPStreamingParam.stMulticastInfo[i].iAlwaysMulticastAudio = pThis->tMulticastInfo[i].iAlwaysMulticastAudio;
		stRTSPStreamingParam.stMulticastInfo[i].iAlwaysMulticastVideo = pThis->tMulticastInfo[i].iAlwaysMulticastVideo;
		//20120801 added by Jimmy for metadata
		stRTSPStreamingParam.stMulticastInfo[i].iAlwaysMulticastMetadata= pThis->tMulticastInfo[i].iAlwaysMulticastMetadata;
	}
#endif

	for (i=0; i<pThis->dwStreamNumber; i++)
	{
		//20120801 modified by Jimmy for metadata
		stRTSPStreamingParam.iRTSPStreamingMediaType[i] = RTSPSTREAMING_NO_MEDIA;
		if( pThis->tStreamInfo[i].iVideoSrcIndex >= 0 )
		{
			stRTSPStreamingParam.iRTSPStreamingMediaType[i] |= RTSPSTREAMING_MEDIATYPE_VIDEO;
		}
		if( pThis->tStreamInfo[i].iAudioSrcIndex >= 0 )
		{
			stRTSPStreamingParam.iRTSPStreamingMediaType[i] |= RTSPSTREAMING_MEDIATYPE_AUDIO;
		}
		if( pThis->tStreamInfo[i].iMetadataSrcIndex>= 0 )
		{
			stRTSPStreamingParam.iRTSPStreamingMediaType[i] |= RTSPSTREAMING_MEDIATYPE_METADATA;
		}
		
		//CID:811, CHECKER:SECURE_CODING	
		strncpy(stRTSPStreamingParam.acAccessName[i], pThis->tStreamInfo[i].szAccessName, sizeof(stRTSPStreamingParam.acAccessName[i]) - 1);
		stRTSPStreamingParam.acAccessName[i][sizeof(stRTSPStreamingParam.acAccessName[i]) - 1] = 0;
	}
	stRTSPStreamingParam.ulLocalSubnetMask = inet_addr(pThis->szSubnetMask);
	//CID:219, CHECKER: NO EFFECT 
	if((stRTSPStreamingParam.ulLocalIP = inet_addr(pThis->szIPAddr)) == INADDR_NONE)
	{
	    printf("illegal IP address for Streaming server!\r\n");
	    return S_FAIL;
	}
	
	stRTSPStreamingParam.ulNATIP = 0;
	stRTSPStreamingParam.iRTSPMaxConnectionNum = pThis->tRTSPInfo.iMaxConnectionNum;
	stRTSPStreamingParam.iRTSPAuthentication = pThis->tRTSPInfo.iAuthenticateMode;
	
#ifdef _SIP
	stRTSPStreamingParam.tSIPParameter.iSIPMaxConnectionNum = pThis->tSIPInfo.iMaxConnectionNum;
#ifndef _SIP_USE_RTSPAUTH
	stRTSPStreamingParam.tSIPParameter.iAuthenticateMode = pThis->tSIPInfo.iAuthenticateMode;
#else
	stRTSPStreamingParam.tSIPParameter.iAuthenticateMode = pThis->tRTSPInfo.iAuthenticateMode;
#endif
	stRTSPStreamingParam.tSIPParameter.usPort          = pThis->tSIPInfo.usPort;
	stRTSPStreamingParam.tSIPParameter.ulRegistrarIP   = pThis->tSIPInfo.ulRegistrarIP;
	stRTSPStreamingParam.tSIPParameter.usRegistrarPort = pThis->tSIPInfo.usRegistrarPort;
	stRTSPStreamingParam.tSIPParameter.ulOBProxyIP     = pThis->tSIPInfo.ulOBProxyIP;
	stRTSPStreamingParam.tSIPParameter.usOBProxyPort   = pThis->tSIPInfo.usOBProxyPort;
	//CID:811, CHECKER:SECURE_CODING
	strncpy(stRTSPStreamingParam.tSIPParameter.szDisplayName,pThis->tSIPInfo.szDisplayName, sizeof(stRTSPStreamingParam.tSIPParameter.szDisplayName));
	strncpy(stRTSPStreamingParam.tSIPParameter.szUserName,pThis->tSIPInfo.szUserName, sizeof(stRTSPStreamingParam.tSIPParameter.szUserName));
	strncpy(stRTSPStreamingParam.tSIPParameter.szPassword, pThis->tSIPInfo.szPassword, sizeof(stRTSPStreamingParam.tSIPParameter.szPassword));      
	strncpy(stRTSPStreamingParam.tSIPParameter.szSIPDomain,pThis->tSIPInfo.szSIPDomain, sizeof(stRTSPStreamingParam.tSIPParameter.szSIPDomain));
	
	printf("---SIP display name:%s!!---\n",stRTSPStreamingParam.tSIPParameter.szDisplayName);
#endif
	
#ifdef _SHARED_MEM
	for (i = 0; i < VIDEO_TRACK_NUMBER ; i++)
	{
		//20130605 modified by Jimmy to support metadata event
		stRTSPStreamingParam.ahShmemVideoHandle[i] = pThis->tVideoSrcInfo[i].ahSharedMem[0];
	}
	for (i = 0; i < AUDIO_TRACK_NUMBER ; i++)
	{
		//20130605 modified by Jimmy to support metadata event
		stRTSPStreamingParam.ahShmemAudioHandle[i] = pThis->tAudioSrcInfo[i].ahSharedMem[0];
	}
	//20120801 added by Jimmy for metadata
	for (i = 0; i < METADATA_TRACK_NUMBER ; i++)
	{
		//stRTSPStreamingParam.ahShmemMetadataHandle[i] = pThis->tMetadataSrcInfo[i].ahSharedMem;
		//20130605 modified by Jimmy to support metadata event
		for (j = 0; j < SHMEM_HANDLE_MAX_NUM; j++)
		{
			stRTSPStreamingParam.ahShmemMetadataHandle[i][j] = pThis->tMetadataSrcInfo[i].ahSharedMem[j];
		}
	}

	//FIXME: should read from config file
	stRTSPStreamingParam.dwProtectedDelta = 6;

#endif

	//20081121 URL authentication
	if(pThis->tRTSPInfo.iURLAuthEnabled == 1)
	{
		printf("URL authentication supported!\n");
		stRTSPStreamingParam.iURLAuthEnabled = 1;
	}

	//20101123 Added by danny For support advanced system log 
	stRTSPStreamingParam.bAdvLogSupport = pThis->bAdvLogSupport;
	
	if(  (pThis->hRTSPServer = RTSPStreaming_Create(&stRTSPStreamingParam)) == NULL  )
	{
		printf("Create rtsp streaming server fail !\n");
		return S_FAIL;
	}
 
	//set RTSP stream server options
   	RTSPStreaming_SetSDPETag(pThis->hRTSPServer, "1234567890");
   	RTSPStreaming_SetHostName(pThis->hRTSPServer, "RTSP server");
//	RTSPStreaming_AddAccessList(pThis->hRTSPServer, inet_addr("192.168.0.1"), inet_addr("192.168.255.254"));

	
	TRTSPSTREAMING_VIDENCODING_PARAM stRTSPVideoParam;
	TRTSPSTREAMING_AUDENCODING_PARAM stRTSPAudioParam;
	//20120801 added by Jimmy for metadata
	TRTSPSTREAMING_METADATAENCODING_PARAM stRTSPMetadataParam;
	//20120925 added by Jimmy for ONVIF backchannel
	TRTSPSTREAMING_AUDDECODING_PARAM stRTSPAudiobackParam;
	DWORD   dwAudioFlag;
	//20120801 added by Jimmy for metadata
	DWORD   dwMetadataFlag;

    memset(&stRTSPVideoParam, 0, sizeof(TRTSPSTREAMING_VIDENCODING_PARAM));
	memset(&stRTSPAudioParam, 0, sizeof(TRTSPSTREAMING_AUDENCODING_PARAM));
	memset(&stRTSPMetadataParam, 0, sizeof(TRTSPSTREAMING_METADATAENCODING_PARAM));

	stRTSPVideoParam.iProfileLevel = 3;
	stRTSPVideoParam.iMPEG4HeaderLen = 28;
	stRTSPVideoParam.acMPEG4Header[28] = 0;
	stRTSPVideoParam.iClockRate = 30000;
	stRTSPVideoParam.iDecoderBufferSize = 76800;
	
	//20090608 For multiple stream
	for (i = 0; i < MULTIPLE_STREAM_NUM ; i++)
	{
		RTSPStreaming_SetVideoParameters(pThis->hRTSPServer, i + 1, &stRTSPVideoParam, 
									RTSPSTREAMING_VIDEO_PROLEVE  | RTSPSTREAMING_VIDEO_BITRATE  | 
						            RTSPSTREAMING_VIDEO_CLOCKRATE| RTSPSTREAMING_VIDEO_MPEG4CI  | 
						            RTSPSTREAMING_VIDEO_WIDTH    | RTSPSTREAMING_VIDEO_HEIGHT   | 
						            RTSPSTREAMING_VIDEO_DECODEBUFF);
	}

	for( i = 0 ; i < IPFILTER_NUMBER ; i++)
	{
		if( (tStreamServerInfo.tAllowIP[i].ulStartIP) != 0 
			&& (tStreamServerInfo.tAllowIP[i].ulEndIP) != 0 )
 		{
			RTSPStreaming_AddAccessList(pThis->hRTSPServer
									, tStreamServerInfo.tAllowIP[i].ulStartIP
									, tStreamServerInfo.tAllowIP[i].ulEndIP);
		}
		
		if( (tStreamServerInfo.tDenyIP[i].ulStartIP) != 0 
			&& (tStreamServerInfo.tDenyIP[i].ulEndIP) != 0 )
 		{
			RTSPStreaming_AddDenyList(pThis->hRTSPServer
									, tStreamServerInfo.tDenyIP[i].ulStartIP
									, tStreamServerInfo.tDenyIP[i].ulEndIP);
		}
	}

    stRTSPAudioParam.iAudioCodecType = ractGAMR;
	stRTSPAudioParam.iBitRate=4750;
	stRTSPAudioParam.iClockRate=8000;
	stRTSPAudioParam.iPacketTime=200;
	stRTSPAudioParam.iOctetAlign=1;
	stRTSPAudioParam.iAMRcrc=0;
	stRTSPAudioParam.iRobustSorting=0;
	stRTSPAudioParam.bIsBigEndian = FALSE;
	
	dwAudioFlag = RTSPSTREAMING_AUDIO_BITRATE|RTSPSTREAMING_AUDIO_CLOCKRATE|
				RTSPSTREAMING_AUDIO_PACKETTIME|RTSPSTREAMING_AUDIO_OCTECTALIGN|
				RTSPSTREAMING_AUDIO_AMRCRC|RTSPSTREAMING_AUDIO_ROBUSTSORT|
				RTSPSTREAMING_AUDIO_PACKINGMODE;

	//20090608 For multiple stream
	for (i = 0; i < MULTIPLE_STREAM_NUM ; i++)
	{
		RTSPStreaming_SetAudioParameters(pThis->hRTSPServer, i + 1,&stRTSPAudioParam,dwAudioFlag);
	}

	//20120801 added by Jimmy for metadata
	stRTSPMetadataParam.iMetadataCodecType = ractGAMR;
	stRTSPMetadataParam.iBitRate=4750;
	stRTSPMetadataParam.iClockRate=90000;
	stRTSPMetadataParam.iPacketTime=200;
	stRTSPMetadataParam.iOctetAlign=1;
	stRTSPMetadataParam.iAMRcrc=0;
	stRTSPMetadataParam.iRobustSorting=0;
	
	dwMetadataFlag = RTSPSTREAMING_METADATA_BITRATE|RTSPSTREAMING_METADATA_CLOCKRATE|
				RTSPSTREAMING_METADATA_PACKETTIME|RTSPSTREAMING_METADATA_OCTECTALIGN|
				RTSPSTREAMING_METADATA_AMRCRC|RTSPSTREAMING_METADATA_ROBUSTSORT;
	rtspstrcpy(stRTSPMetadataParam.acTrackName, pThis->tMetadataSrcInfo[0].acTrackName, sizeof(stRTSPMetadataParam.acTrackName));
	for (i = 0; i < MULTIPLE_STREAM_NUM ; i++)
	{
		RTSPStreaming_SetMetadataParameters(pThis->hRTSPServer, i + 1, &stRTSPMetadataParam, dwMetadataFlag);
	}

	//20120925 added by Jimmy for ONVIF backchannel
	for (i = 0; i < MULTIPLE_CHANNEL_NUM; i++)
	{
		snprintf(stRTSPAudiobackParam.acTrackName, RTSPSTREAMING_TRACK_NAME_LEN - 1, "audioback%d", i + 1);
		RTSPStreaming_SetAudiobackParameters(pThis->hRTSPServer, i + 1, &stRTSPAudiobackParam);
	}


	RTSPStreaming_SetControlCallback(pThis->hRTSPServer,(FControlChannel_Callback)StreamSvrCtrlChCallback, pThis);
	RTSPStreaming_SetVideoCallback(pThis->hRTSPServer, (MEDIA_CALLBACK)StreamSvrVideoCallback, pThis);
	RTSPStreaming_SetAudioCallback(pThis->hRTSPServer, (MEDIA_CALLBACK)StreamSvrAudioInCallback, pThis);
	//20120801 added by Jimmy for metadata
	RTSPStreaming_SetMetadataCallback(pThis->hRTSPServer, (MEDIA_CALLBACK)StreamSvrMetadataCallback, pThis);
	
#ifdef _SHARED_MEM
	if(RTSPStreaming_SetShmemVideoCallback(pThis->hRTSPServer, (MEDIA_CALLBACK)StreamSvrShmemVideoCallback, pThis) != S_OK)
	{
		printf("Share memory set video callback failed!\n");
		return S_FAIL;
	}
	if(RTSPStreaming_SetShmemAudioCallback(pThis->hRTSPServer, (MEDIA_CALLBACK)StreamSvrShmemAudioCallback, pThis) != S_OK)
	{
		printf("Share memory set audio callback failed!\n");
		return S_FAIL;
	}
	//20120801 added by Jimmy for metadata
	if(RTSPStreaming_SetShmemMetadataCallback(pThis->hRTSPServer, (MEDIA_CALLBACK)StreamSvrShmemMetadataCallback, pThis) != S_OK)
	{
		printf("Share memory set metadata callback failed!\n");
		return S_FAIL;
	}

#endif

	return S_OK;
}

int StreamingServer_ParseTrackID(char* pcTrackName)
{
    int iTrackID;
    
    if( pcTrackName == NULL )
        return -1;
        
    if( strncmp(pcTrackName,"trackID=",strlen("trackID=")) == 0 )
    {
        if( sscanf(pcTrackName+strlen("trackID="),"%d",&iTrackID) == 0 )
            return -1;
        else
           return iTrackID;
    }
    else
        return -1;
}

SCODE StreamingServer_ParseMediaConfig(TSTREAMSERVERINFO   *pThis, char* pzConfigFile)
{
	int					iStreamCount = 0, iComponentCount = 0;
	TCfgParseMap		atVideoXmlTable[VIDEO_TRACK_NUMBER*VIDEOMEDIACOMPONENTSNUMBER + 1];
	TCfgParseMap		atAudioXmlTable[AUDIO_TRACK_NUMBER*AUDIOMEDIACOMPONENTSNUMBER + 1];
	//20120810 added by Jimmy for metadata
	TCfgParseMap		atMetadataXmlTable[METADATA_TRACK_NUMBER*METADATAMEDIACOMPONENTSNUMBER + 1];
	char				acVideoQueryString[VIDEO_TRACK_NUMBER][VIDEOMEDIACOMPONENTSNUMBER][XMLPATHLENGTH];
	char				acAudioQueryString[AUDIO_TRACK_NUMBER][AUDIOMEDIACOMPONENTSNUMBER][XMLPATHLENGTH];
	//20120810 added by Jimmy for metadata
	char				acMetadataQueryString[METADATA_TRACK_NUMBER][METADATAMEDIACOMPONENTSNUMBER][XMLPATHLENGTH];
	void				*apVideoParam[VIDEO_TRACK_NUMBER][VIDEOMEDIACOMPONENTSNUMBER];
	void				*apAudioParam[AUDIO_TRACK_NUMBER][AUDIOMEDIACOMPONENTSNUMBER];
	//20120810 added by Jimmy for metadata
	void				*apMetadataParam[METADATA_TRACK_NUMBER][METADATAMEDIACOMPONENTSNUMBER];
	TSTREAMINFO			*ptStreamInfo;
	TMEDIASRCINFO		*ptVideoSrcInfo;
	TMEDIASRCINFO		*ptAudioSrcInfo;
	//20120810 added by Jimmy for metadata
	TMEDIASRCINFO		*ptMetadataSrcInfo;
	//20100714 Modified by danny For Multicast parameters load dynamically
	//MULTICASTINFO		*ptMulticastInfo;
	
	//Struct the XML table for xmlsparser, the component count indicate how many components in g_VideoMediaParseMap
	for(iStreamCount = 0; iStreamCount < VIDEO_TRACK_NUMBER; iStreamCount++)
	{
		ptStreamInfo = &pThis->tStreamInfo[iStreamCount];
		ptVideoSrcInfo = &pThis->tVideoSrcInfo[iStreamCount];
		//20100714 Modified by danny For Multicast parameters load dynamically
		//ptMulticastInfo = &pThis->tMulticastInfo[iStreamCount];
		//StreamingServer_FillInfo(apVideoParam[iStreamCount], ptStreamInfo, ptVideoSrcInfo, ptMulticastInfo);
		StreamingServer_FillInfo(apVideoParam[iStreamCount], ptStreamInfo, ptVideoSrcInfo);
		for(iComponentCount = 0; iComponentCount < VIDEOMEDIACOMPONENTSNUMBER; iComponentCount++)
		{
			//Fill in the XML path
			snprintf(&acVideoQueryString[iStreamCount][iComponentCount][0], XMLPATHLENGTH - 1, (char *)g_VideoMediaParseMap[iComponentCount].pszXMLPath, iStreamCount);
			acVideoQueryString[iStreamCount][iComponentCount][XMLPATHLENGTH - 1] = 0;
			atVideoXmlTable[VIDEOMEDIACOMPONENTSNUMBER * iStreamCount + iComponentCount].pszXMLPath = acVideoQueryString[iStreamCount][iComponentCount];
			atVideoXmlTable[VIDEOMEDIACOMPONENTSNUMBER * iStreamCount + iComponentCount].dwFlags = g_VideoMediaParseMap[iComponentCount].dwFlags;
			atVideoXmlTable[VIDEOMEDIACOMPONENTSNUMBER * iStreamCount + iComponentCount].dwSize = g_VideoMediaParseMap[iComponentCount].dwSize;
			atVideoXmlTable[VIDEOMEDIACOMPONENTSNUMBER * iStreamCount + iComponentCount].pParam = apVideoParam[iStreamCount][iComponentCount];
			atVideoXmlTable[VIDEOMEDIACOMPONENTSNUMBER * iStreamCount + iComponentCount].pfnGetParam = g_VideoMediaParseMap[iComponentCount].pfnGetParam;
		}
	}
	//Fill in the last NULL line
	atVideoXmlTable[VIDEO_TRACK_NUMBER*VIDEOMEDIACOMPONENTSNUMBER].pszXMLPath  = NULL ;
	atVideoXmlTable[VIDEO_TRACK_NUMBER*VIDEOMEDIACOMPONENTSNUMBER].dwFlags     = 0 ;	
	atVideoXmlTable[VIDEO_TRACK_NUMBER*VIDEOMEDIACOMPONENTSNUMBER].dwSize      = 0 ;
	atVideoXmlTable[VIDEO_TRACK_NUMBER*VIDEOMEDIACOMPONENTSNUMBER].pParam      = NULL ;
	atVideoXmlTable[VIDEO_TRACK_NUMBER*VIDEOMEDIACOMPONENTSNUMBER].pfnGetParam = NULL ;

	//Proceed to parse
	if(XMLSParser_ReadAll(pzConfigFile, atVideoXmlTable) != S_OK)
	{
		printf("Parse video media info failed for config file %s!\n", pzConfigFile);
		return S_FAIL;
	}

	/*for(iStreamCount = 0; iStreamCount < VIDEO_TRACK_NUMBER; iStreamCount++)
	{
		printf("%s\n", pThis->tVideoSrcInfo[iStreamCount].acSockPathName);
		printf("%s\n", pThis->tVideoSrcInfo[iStreamCount].acFIFOPathName);
		printf("%d\n", pThis->tStreamInfo[iStreamCount].iEnable);
		printf("%s\n", pThis->tStreamInfo[iStreamCount].szAccessName);
		printf("%d\n", pThis->tStreamInfo[iStreamCount].iVideoSrcIndex);
		printf("%d\n", pThis->tStreamInfo[iStreamCount].iAudioSrcIndex);
	}*/

	//Struct the XML table for xmlsparser, the component count indicate how many components in g_AudioMediaParseMap
	for(iStreamCount = 0; iStreamCount < AUDIO_TRACK_NUMBER; iStreamCount++)
	{
		ptStreamInfo = &pThis->tStreamInfo[iStreamCount];
		ptAudioSrcInfo = &pThis->tAudioSrcInfo[iStreamCount];
		StreamingServer_FillAudioMediaInfo(apAudioParam[iStreamCount], ptStreamInfo, ptAudioSrcInfo);
		for(iComponentCount = 0; iComponentCount < AUDIOMEDIACOMPONENTSNUMBER; iComponentCount++)
		{
			//Fill in the XML path
			snprintf(&acAudioQueryString[iStreamCount][iComponentCount][0], XMLPATHLENGTH - 1, (char *)g_AudioMediaParseMap[iComponentCount].pszXMLPath, iStreamCount);
			acAudioQueryString[iStreamCount][iComponentCount][XMLPATHLENGTH - 1] = 0;
			atAudioXmlTable[AUDIOMEDIACOMPONENTSNUMBER * iStreamCount + iComponentCount].pszXMLPath = acAudioQueryString[iStreamCount][iComponentCount];
			atAudioXmlTable[AUDIOMEDIACOMPONENTSNUMBER * iStreamCount + iComponentCount].dwFlags = g_AudioMediaParseMap[iComponentCount].dwFlags;
			atAudioXmlTable[AUDIOMEDIACOMPONENTSNUMBER * iStreamCount + iComponentCount].dwSize = g_AudioMediaParseMap[iComponentCount].dwSize;
			atAudioXmlTable[AUDIOMEDIACOMPONENTSNUMBER * iStreamCount + iComponentCount].pParam = apAudioParam[iStreamCount][iComponentCount];
			atAudioXmlTable[AUDIOMEDIACOMPONENTSNUMBER * iStreamCount + iComponentCount].pfnGetParam = g_AudioMediaParseMap[iComponentCount].pfnGetParam;
		}
	}
	//Fill in the last NULL line
	atAudioXmlTable[AUDIO_TRACK_NUMBER*AUDIOMEDIACOMPONENTSNUMBER].pszXMLPath  = NULL ;
	atAudioXmlTable[AUDIO_TRACK_NUMBER*AUDIOMEDIACOMPONENTSNUMBER].dwFlags     = 0 ;	
	atAudioXmlTable[AUDIO_TRACK_NUMBER*AUDIOMEDIACOMPONENTSNUMBER].dwSize      = 0 ;
	atAudioXmlTable[AUDIO_TRACK_NUMBER*AUDIOMEDIACOMPONENTSNUMBER].pParam      = NULL ;
	atAudioXmlTable[AUDIO_TRACK_NUMBER*AUDIOMEDIACOMPONENTSNUMBER].pfnGetParam = NULL ;

	//Proceed to parse
	if(XMLSParser_ReadAll(pzConfigFile, atAudioXmlTable) != S_OK)
	{
		printf("Parse audio media info failed for config file %s!\n", pzConfigFile);
		return S_FAIL;
	}
	for(iStreamCount = 0; iStreamCount < AUDIO_TRACK_NUMBER; iStreamCount++)
	{
		printf("%s\n", pThis->tAudioSrcInfo[iStreamCount].acSockPathName);
		printf("%s\n", pThis->tAudioSrcInfo[iStreamCount].acFIFOPathName);
	}

	//20120810 added by Jimmy for metadata
	//Struct the XML table for xmlsparser, the component count indicate how many components in g_MetadataMediaParseMap
	for(iStreamCount = 0; iStreamCount < METADATA_TRACK_NUMBER; iStreamCount++)
	{
		ptStreamInfo = &pThis->tStreamInfo[iStreamCount];
		ptMetadataSrcInfo = &pThis->tMetadataSrcInfo[iStreamCount];
		StreamingServer_FillMetadataMediaInfo(apMetadataParam[iStreamCount], ptStreamInfo, ptMetadataSrcInfo);
		for(iComponentCount = 0; iComponentCount < METADATAMEDIACOMPONENTSNUMBER; iComponentCount++)
		{
			//Fill in the XML path
			snprintf(&acMetadataQueryString[iStreamCount][iComponentCount][0], XMLPATHLENGTH - 1, (char *)g_MetadataMediaParseMap[iComponentCount].pszXMLPath, iStreamCount);
			acMetadataQueryString[iStreamCount][iComponentCount][XMLPATHLENGTH - 1] = 0;
			atMetadataXmlTable[METADATAMEDIACOMPONENTSNUMBER * iStreamCount + iComponentCount].pszXMLPath = acMetadataQueryString[iStreamCount][iComponentCount];
			atMetadataXmlTable[METADATAMEDIACOMPONENTSNUMBER * iStreamCount + iComponentCount].dwFlags = g_MetadataMediaParseMap[iComponentCount].dwFlags;
			atMetadataXmlTable[METADATAMEDIACOMPONENTSNUMBER * iStreamCount + iComponentCount].dwSize = g_MetadataMediaParseMap[iComponentCount].dwSize;
			atMetadataXmlTable[METADATAMEDIACOMPONENTSNUMBER * iStreamCount + iComponentCount].pParam = apMetadataParam[iStreamCount][iComponentCount];
			atMetadataXmlTable[METADATAMEDIACOMPONENTSNUMBER * iStreamCount + iComponentCount].pfnGetParam = g_MetadataMediaParseMap[iComponentCount].pfnGetParam;
		}
	}
	//Fill in the last NULL line
	atMetadataXmlTable[METADATA_TRACK_NUMBER*METADATAMEDIACOMPONENTSNUMBER].pszXMLPath  = NULL ;
	atMetadataXmlTable[METADATA_TRACK_NUMBER*METADATAMEDIACOMPONENTSNUMBER].dwFlags     = 0 ;	
	atMetadataXmlTable[METADATA_TRACK_NUMBER*METADATAMEDIACOMPONENTSNUMBER].dwSize      = 0 ;
	atMetadataXmlTable[METADATA_TRACK_NUMBER*METADATAMEDIACOMPONENTSNUMBER].pParam      = NULL ;
	atMetadataXmlTable[METADATA_TRACK_NUMBER*METADATAMEDIACOMPONENTSNUMBER].pfnGetParam = NULL ;

	//Proceed to parse
	if(XMLSParser_ReadAll(pzConfigFile, atMetadataXmlTable) != S_OK)
	{
		printf("Parse metadata media info failed for config file %s!\n", pzConfigFile);
		return S_FAIL;
	}
	for(iStreamCount = 0; iStreamCount < METADATA_TRACK_NUMBER; iStreamCount++)
	{
		printf("%s\n", pThis->tMetadataSrcInfo[iStreamCount].acSockPathName);
		printf("%s\n", pThis->tMetadataSrcInfo[iStreamCount].acFIFOPathName);
	}	
	return S_OK;
}

#ifdef RTSPRTP_MULTICAST
//20100714 Added by danny For Multicast parameters load dynamically
SCODE StreamingServer_ParseMulticastInfoConfig(TSTREAMSERVERINFO   *pThis, char* pzConfigFile)
{
	int					iMulticastCount = 0, iComponentCount = 0;
	TCfgParseMap		atMulticastXmlTable[RTSP_MULTICASTNUMBER*MULTICASTCOMPONENTSNUMBER + 1];
	char				acMulticastQueryString[RTSP_MULTICASTNUMBER][MULTICASTCOMPONENTSNUMBER][XMLPATHLENGTH];
	void				*apMulticastParam[RTSP_MULTICASTNUMBER][MULTICASTCOMPONENTSNUMBER];
	MULTICASTINFO		*ptMulticastInfo;
	
	//Struct the XML table for xmlsparser, the component count indicate how many components in g_MulticastParseMap
	for(iMulticastCount = 0; iMulticastCount < RTSP_MULTICASTNUMBER; iMulticastCount++)
	{
		ptMulticastInfo = &pThis->tMulticastInfo[iMulticastCount];
		//20110630 Add by danny For Multicast enable/disable
		ptMulticastInfo->iEnable = 1;
		StreamingServer_FillMulticastInfo(apMulticastParam[iMulticastCount], ptMulticastInfo);
		for(iComponentCount = 0; iComponentCount < MULTICASTCOMPONENTSNUMBER; iComponentCount++)
		{
			//Fill in the XML path
			snprintf(&acMulticastQueryString[iMulticastCount][iComponentCount][0], XMLPATHLENGTH - 1, (char *)g_MulticastParseMap[iComponentCount].pszXMLPath, iMulticastCount);
			acMulticastQueryString[iMulticastCount][iComponentCount][XMLPATHLENGTH - 1] = 0;
			atMulticastXmlTable[MULTICASTCOMPONENTSNUMBER * iMulticastCount + iComponentCount].pszXMLPath = acMulticastQueryString[iMulticastCount][iComponentCount];
			atMulticastXmlTable[MULTICASTCOMPONENTSNUMBER * iMulticastCount + iComponentCount].dwFlags = g_MulticastParseMap[iComponentCount].dwFlags;
			atMulticastXmlTable[MULTICASTCOMPONENTSNUMBER * iMulticastCount + iComponentCount].dwSize = g_MulticastParseMap[iComponentCount].dwSize;
			atMulticastXmlTable[MULTICASTCOMPONENTSNUMBER * iMulticastCount + iComponentCount].pParam = apMulticastParam[iMulticastCount][iComponentCount];
			atMulticastXmlTable[MULTICASTCOMPONENTSNUMBER * iMulticastCount + iComponentCount].pfnGetParam = g_MulticastParseMap[iComponentCount].pfnGetParam;
		}
	}
	//Fill in the last NULL line
	atMulticastXmlTable[RTSP_MULTICASTNUMBER*MULTICASTCOMPONENTSNUMBER].pszXMLPath  = NULL ;
	atMulticastXmlTable[RTSP_MULTICASTNUMBER*MULTICASTCOMPONENTSNUMBER].dwFlags     = 0 ;	
	atMulticastXmlTable[RTSP_MULTICASTNUMBER*MULTICASTCOMPONENTSNUMBER].dwSize      = 0 ;
	atMulticastXmlTable[RTSP_MULTICASTNUMBER*MULTICASTCOMPONENTSNUMBER].pParam      = NULL ;
	atMulticastXmlTable[RTSP_MULTICASTNUMBER*MULTICASTCOMPONENTSNUMBER].pfnGetParam = NULL ;
	
	//Proceed to parse
	if(XMLSParser_ReadAll(pzConfigFile, atMulticastXmlTable) != S_OK)
	{
		printf("Parse multicast info failed for config file %s!\n", pzConfigFile);
		return S_FAIL;
	}
	for(iMulticastCount = 0; iMulticastCount < RTSP_MULTICASTNUMBER; iMulticastCount++)
	{
		//20110725 Add by danny For Multicast RTCP receive report keep alive
		if(pThis->tMulticastInfo[iMulticastCount].iAlwaysMulticast)
		{
			pThis->tMulticastInfo[iMulticastCount].iRRAlive = 0;
		}
		if(!pThis->tMulticastInfo[iMulticastCount].ulMulticastAudioAddress)
		{
			printf("Can't get audio address, default by video = %lu\n",  pThis->tMulticastInfo[iMulticastCount].ulMulticastAddress);
			pThis->tMulticastInfo[iMulticastCount].ulMulticastAudioAddress = pThis->tMulticastInfo[iMulticastCount].ulMulticastAddress;
		}
		if(!pThis->tMulticastInfo[iMulticastCount].ulMulticastMetadataAddress)
		{
			printf("Can't get metadata address, default by video = %lu\n",  pThis->tMulticastInfo[iMulticastCount].ulMulticastAddress);
			pThis->tMulticastInfo[iMulticastCount].ulMulticastMetadataAddress = pThis->tMulticastInfo[iMulticastCount].ulMulticastAddress;
		}
//		printf("%d\n", pThis->tMulticastInfo[iMulticastCount].iEnable);
//		printf("%d\n", pThis->tMulticastInfo[iMulticastCount].iAlwaysMulticast);
//		printf("%d\n", pThis->tMulticastInfo[iMulticastCount].iRRAlive);
//		printf("%d\n", pThis->tMulticastInfo[iMulticastCount].usMulticastVideoPort);
//		printf("%d\n", pThis->tMulticastInfo[iMulticastCount].usMulticastAudioPort);
//		printf("%lu\n", pThis->tMulticastInfo[iMulticastCount].ulMulticastAddress);
//		printf("%d\n", pThis->tMulticastInfo[iMulticastCount].usTTL);
//		printf("%d\n\n", pThis->tMulticastInfo[iMulticastCount].iRTPExtension);
	}
	
	return S_OK;
}

#endif

#ifdef _SHARED_MEM
//20101210 Added by danny For Media shmem config
SCODE StreamingServer_ParseShmemConfig(TSTREAMSERVERINFO   *pThis, char* pzConfigFile)
{
	int					iStreamCount = 0, iComponentCount = 0;
	TCfgParseMap		atVideoXmlTable[VIDEO_TRACK_NUMBER*VIDEOSHMEMCOMPONENTSNUMBER + 1];
	TCfgParseMap		atAudioXmlTable[AUDIO_TRACK_NUMBER*AUDIOSHMEMCOMPONENTSNUMBER + 1];
	//20120810 added by Jimmy for metadata
	TCfgParseMap		atMetadataXmlTable[METADATA_TRACK_NUMBER*METADATASHMEMCOMPONENTSNUMBER + 1];
	char				acVideoQueryString[VIDEO_TRACK_NUMBER][VIDEOSHMEMCOMPONENTSNUMBER][XMLPATHLENGTH];
	char				acAudioQueryString[AUDIO_TRACK_NUMBER][AUDIOSHMEMCOMPONENTSNUMBER][XMLPATHLENGTH];
	//20120810 added by Jimmy for metadata
	char				acMetadataQueryString[METADATA_TRACK_NUMBER][METADATASHMEMCOMPONENTSNUMBER][XMLPATHLENGTH];
	void				*apVideoParam[VIDEO_TRACK_NUMBER][VIDEOSHMEMCOMPONENTSNUMBER];
	void				*apAudioParam[AUDIO_TRACK_NUMBER][AUDIOSHMEMCOMPONENTSNUMBER];
	//20120810 added by Jimmy for metadata
	void				*apMetadataParam[METADATA_TRACK_NUMBER][METADATASHMEMCOMPONENTSNUMBER];
	TMEDIASRCINFO		*ptVideoSrcInfo;
	TMEDIASRCINFO		*ptAudioSrcInfo;
	//20120810 added by Jimmy for metadata
	TMEDIASRCINFO		*ptMetadataSrcInfo;

	
	//Struct the XML table for xmlsparser, the component count indicate how many components in g_VideoShmemParseMap
	for(iStreamCount = 0; iStreamCount < VIDEO_TRACK_NUMBER; iStreamCount++)
	{
		ptVideoSrcInfo = &pThis->tVideoSrcInfo[iStreamCount];
		StreamingServer_FillVideoShmemInfo(apVideoParam[iStreamCount], ptVideoSrcInfo);
		for(iComponentCount = 0; iComponentCount < VIDEOSHMEMCOMPONENTSNUMBER; iComponentCount++)
		{
			//Fill in the XML path
			snprintf(&acVideoQueryString[iStreamCount][iComponentCount][0], XMLPATHLENGTH - 1, (char *)g_VideoShmemParseMap[iComponentCount].pszXMLPath, iStreamCount);
			acVideoQueryString[iStreamCount][iComponentCount][XMLPATHLENGTH - 1] = 0;
			atVideoXmlTable[VIDEOSHMEMCOMPONENTSNUMBER * iStreamCount + iComponentCount].pszXMLPath = acVideoQueryString[iStreamCount][iComponentCount];
			atVideoXmlTable[VIDEOSHMEMCOMPONENTSNUMBER * iStreamCount + iComponentCount].dwFlags = g_VideoShmemParseMap[iComponentCount].dwFlags;
			atVideoXmlTable[VIDEOSHMEMCOMPONENTSNUMBER * iStreamCount + iComponentCount].dwSize = g_VideoShmemParseMap[iComponentCount].dwSize;
			atVideoXmlTable[VIDEOSHMEMCOMPONENTSNUMBER * iStreamCount + iComponentCount].pParam = apVideoParam[iStreamCount][iComponentCount];
			atVideoXmlTable[VIDEOSHMEMCOMPONENTSNUMBER * iStreamCount + iComponentCount].pfnGetParam = g_VideoShmemParseMap[iComponentCount].pfnGetParam;
		}
	}
	//Fill in the last NULL line
	atVideoXmlTable[VIDEO_TRACK_NUMBER*VIDEOSHMEMCOMPONENTSNUMBER].pszXMLPath  = NULL ;
	atVideoXmlTable[VIDEO_TRACK_NUMBER*VIDEOSHMEMCOMPONENTSNUMBER].dwFlags     = 0 ;	
	atVideoXmlTable[VIDEO_TRACK_NUMBER*VIDEOSHMEMCOMPONENTSNUMBER].dwSize      = 0 ;
	atVideoXmlTable[VIDEO_TRACK_NUMBER*VIDEOSHMEMCOMPONENTSNUMBER].pParam      = NULL ;
	atVideoXmlTable[VIDEO_TRACK_NUMBER*VIDEOSHMEMCOMPONENTSNUMBER].pfnGetParam = NULL ;

	//Proceed to parse
	if(XMLSParser_ReadAll(pzConfigFile, atVideoXmlTable) != S_OK)
	{
		printf("Parse video shmem info failed for config file %s!\n", pzConfigFile);
		return S_FAIL;
	}
	for(iStreamCount = 0; iStreamCount < VIDEO_TRACK_NUMBER; iStreamCount++)
	{
		printf("%d\n", pThis->tVideoSrcInfo[iStreamCount].iBlockIndex);
		printf("%d\n", pThis->tVideoSrcInfo[iStreamCount].iSectorIndex);
	}

	//Struct the XML table for xmlsparser, the component count indicate how many components in g_AudioShmemParseMap
	for(iStreamCount = 0; iStreamCount < AUDIO_TRACK_NUMBER; iStreamCount++)
	{
		ptAudioSrcInfo = &pThis->tAudioSrcInfo[iStreamCount];
		StreamingServer_FillAudioShmemInfo(apAudioParam[iStreamCount], ptAudioSrcInfo);
		for(iComponentCount = 0; iComponentCount < AUDIOSHMEMCOMPONENTSNUMBER; iComponentCount++)
		{
			//Fill in the XML path
			snprintf(&acAudioQueryString[iStreamCount][iComponentCount][0], XMLPATHLENGTH - 1, (char *)g_AudioShmemParseMap[iComponentCount].pszXMLPath, iStreamCount);
			acAudioQueryString[iStreamCount][iComponentCount][XMLPATHLENGTH - 1] = 0;
			atAudioXmlTable[AUDIOSHMEMCOMPONENTSNUMBER * iStreamCount + iComponentCount].pszXMLPath = acAudioQueryString[iStreamCount][iComponentCount];
			atAudioXmlTable[AUDIOSHMEMCOMPONENTSNUMBER * iStreamCount + iComponentCount].dwFlags = g_AudioShmemParseMap[iComponentCount].dwFlags;
			atAudioXmlTable[AUDIOSHMEMCOMPONENTSNUMBER * iStreamCount + iComponentCount].dwSize = g_AudioShmemParseMap[iComponentCount].dwSize;
			atAudioXmlTable[AUDIOSHMEMCOMPONENTSNUMBER * iStreamCount + iComponentCount].pParam = apAudioParam[iStreamCount][iComponentCount];
			atAudioXmlTable[AUDIOSHMEMCOMPONENTSNUMBER * iStreamCount + iComponentCount].pfnGetParam = g_AudioShmemParseMap[iComponentCount].pfnGetParam;
		}
	}
	//Fill in the last NULL line
	atAudioXmlTable[AUDIO_TRACK_NUMBER*AUDIOSHMEMCOMPONENTSNUMBER].pszXMLPath  = NULL ;
	atAudioXmlTable[AUDIO_TRACK_NUMBER*AUDIOSHMEMCOMPONENTSNUMBER].dwFlags     = 0 ;	
	atAudioXmlTable[AUDIO_TRACK_NUMBER*AUDIOSHMEMCOMPONENTSNUMBER].dwSize      = 0 ;
	atAudioXmlTable[AUDIO_TRACK_NUMBER*AUDIOSHMEMCOMPONENTSNUMBER].pParam      = NULL ;
	atAudioXmlTable[AUDIO_TRACK_NUMBER*AUDIOSHMEMCOMPONENTSNUMBER].pfnGetParam = NULL ;

	//Proceed to parse
	if(XMLSParser_ReadAll(pzConfigFile, atAudioXmlTable) != S_OK)
	{
		printf("Parse audio media info failed for config file %s!\n", pzConfigFile);
		return S_FAIL;
	}
	for(iStreamCount = 0; iStreamCount < AUDIO_TRACK_NUMBER; iStreamCount++)
	{
		printf("%d\n", pThis->tAudioSrcInfo[iStreamCount].iBlockIndex);
		printf("%d\n", pThis->tAudioSrcInfo[iStreamCount].iSectorIndex);
	}

	//20120810 added by Jimmy for metadata
	//Struct the XML table for xmlsparser, the component count indicate how many components in g_MetadataShmemParseMap
	for(iStreamCount = 0; iStreamCount < METADATA_TRACK_NUMBER; iStreamCount++)
	{
		ptMetadataSrcInfo = &pThis->tMetadataSrcInfo[iStreamCount];
		StreamingServer_FillMetadataShmemInfo(apMetadataParam[iStreamCount], ptMetadataSrcInfo);
		for(iComponentCount = 0; iComponentCount < METADATASHMEMCOMPONENTSNUMBER; iComponentCount++)
		{
			//Fill in the XML path
			snprintf(&acMetadataQueryString[iStreamCount][iComponentCount][0], XMLPATHLENGTH - 1, (char *)g_MetadataShmemParseMap[iComponentCount].pszXMLPath, iStreamCount);
			acMetadataQueryString[iStreamCount][iComponentCount][XMLPATHLENGTH - 1] = 0;
			atMetadataXmlTable[METADATASHMEMCOMPONENTSNUMBER * iStreamCount + iComponentCount].pszXMLPath = acMetadataQueryString[iStreamCount][iComponentCount];
			atMetadataXmlTable[METADATASHMEMCOMPONENTSNUMBER * iStreamCount + iComponentCount].dwFlags = g_MetadataShmemParseMap[iComponentCount].dwFlags;
			atMetadataXmlTable[METADATASHMEMCOMPONENTSNUMBER * iStreamCount + iComponentCount].dwSize = g_MetadataShmemParseMap[iComponentCount].dwSize;
			atMetadataXmlTable[METADATASHMEMCOMPONENTSNUMBER * iStreamCount + iComponentCount].pParam = apMetadataParam[iStreamCount][iComponentCount];
			atMetadataXmlTable[METADATASHMEMCOMPONENTSNUMBER * iStreamCount + iComponentCount].pfnGetParam = g_MetadataShmemParseMap[iComponentCount].pfnGetParam;
		}
	}
	//Fill in the last NULL line
	atMetadataXmlTable[METADATA_TRACK_NUMBER*METADATASHMEMCOMPONENTSNUMBER].pszXMLPath  = NULL ;
	atMetadataXmlTable[METADATA_TRACK_NUMBER*METADATASHMEMCOMPONENTSNUMBER].dwFlags     = 0 ;	
	atMetadataXmlTable[METADATA_TRACK_NUMBER*METADATASHMEMCOMPONENTSNUMBER].dwSize      = 0 ;
	atMetadataXmlTable[METADATA_TRACK_NUMBER*METADATASHMEMCOMPONENTSNUMBER].pParam      = NULL ;
	atMetadataXmlTable[METADATA_TRACK_NUMBER*METADATASHMEMCOMPONENTSNUMBER].pfnGetParam = NULL ;

	//Proceed to parse
	if(XMLSParser_ReadAll(pzConfigFile, atMetadataXmlTable) != S_OK)
	{
		printf("Parse metadata media info failed for config file %s!\n", pzConfigFile);
		return S_FAIL;
	}
	for(iStreamCount = 0; iStreamCount < METADATA_TRACK_NUMBER; iStreamCount++)
	{
		printf("%s\n", pThis->tMetadataSrcInfo[iStreamCount].acSockPathName);
		printf("%s\n", pThis->tMetadataSrcInfo[iStreamCount].acFIFOPathName);
	}	
	return S_OK;
}

//20100105 Added For Seamless Recording
SCODE StreamingServer_ParseGUIDListInfoConfig(int iMaxConnectionNum)
{
	int					iSessionCount = 0, iComponentCount = 0;
	TCfgParseMap		atGUIDListXmlTable[MAX_CONNECT_NUM * GUIDLISTINFOCOMPONENTSNUMBER + 1];
	char				acQueryString[MAX_CONNECT_NUM][GUIDLISTINFOCOMPONENTSNUMBER][XMLPATHLENGTH];
	void				*apParam[MAX_CONNECT_NUM][GUIDLISTINFOCOMPONENTSNUMBER];
	TGUIDListInfo		*ptGUIDListInfo;
	
	if ( iMaxConnectionNum > MAX_CONNECT_NUM )
	{
		printf("[%s] Error! Disable Seamless Disk Mode, iMaxConnectionNum=%d > MAX_CONNECT_NUM=%d\n", __FUNCTION__, iMaxConnectionNum, MAX_CONNECT_NUM);
		return S_FAIL;
	}
	
	//Struct the XML table for xmlsparser, the component count indicate how many components in g_GUIDListInfoParseMap
	for( iSessionCount = 0; iSessionCount < iMaxConnectionNum; iSessionCount++ )
	{
		ptGUIDListInfo = &tSeamlessRecordingInfo.tGUIDListInfo[iSessionCount];
		StreamingServer_FillGUIDListInfo(apParam[iSessionCount], ptGUIDListInfo);
		for( iComponentCount = 0; iComponentCount < GUIDLISTINFOCOMPONENTSNUMBER; iComponentCount++ )
		{
			//Fill in the XML path
			snprintf(&acQueryString[iSessionCount][iComponentCount][0], XMLPATHLENGTH - 1, (char *)g_GUIDListInfoParseMap[iComponentCount].pszXMLPath, iSessionCount);
			acQueryString[iSessionCount][iComponentCount][XMLPATHLENGTH - 1] = 0;
			atGUIDListXmlTable[iSessionCount * GUIDLISTINFOCOMPONENTSNUMBER + iComponentCount].pszXMLPath = acQueryString[iSessionCount][iComponentCount];
			atGUIDListXmlTable[iSessionCount * GUIDLISTINFOCOMPONENTSNUMBER + iComponentCount].dwFlags = g_GUIDListInfoParseMap[iComponentCount].dwFlags;
			atGUIDListXmlTable[iSessionCount * GUIDLISTINFOCOMPONENTSNUMBER + iComponentCount].dwSize = g_GUIDListInfoParseMap[iComponentCount].dwSize;
			atGUIDListXmlTable[iSessionCount * GUIDLISTINFOCOMPONENTSNUMBER + iComponentCount].pParam = apParam[iSessionCount][iComponentCount];
			atGUIDListXmlTable[iSessionCount * GUIDLISTINFOCOMPONENTSNUMBER + iComponentCount].pfnGetParam = g_GUIDListInfoParseMap[iComponentCount].pfnGetParam;
		}
	}
	//Fill in the last NULL line
	atGUIDListXmlTable[iMaxConnectionNum * GUIDLISTINFOCOMPONENTSNUMBER].pszXMLPath  = NULL ;
	atGUIDListXmlTable[iMaxConnectionNum * GUIDLISTINFOCOMPONENTSNUMBER].dwFlags     = 0 ;	
	atGUIDListXmlTable[iMaxConnectionNum * GUIDLISTINFOCOMPONENTSNUMBER].dwSize      = 0 ;
	atGUIDListXmlTable[iMaxConnectionNum * GUIDLISTINFOCOMPONENTSNUMBER].pParam      = NULL ;
	atGUIDListXmlTable[iMaxConnectionNum * GUIDLISTINFOCOMPONENTSNUMBER].pfnGetParam = NULL ;

	//Proceed to parse
	if( XMLSParser_ReadAll(SEAMLESSRECORDING_CONF, atGUIDListXmlTable) != S_OK )
	{
		printf("[%s] Parse GUID List Info failed!\n", __FUNCTION__);
		return S_FAIL;
	}
	
	tSeamlessRecordingInfo.iSeamlessConnectionNumber = 0;
	for( iSessionCount = 0; iSessionCount < iMaxConnectionNum; iSessionCount++ )
	{
		if( (tSeamlessRecordingInfo.tGUIDListInfo[iSessionCount].iNumber == 0) && (tSeamlessRecordingInfo.tGUIDListInfo[iSessionCount].acGUID[0] != 0) )
		{
			tSeamlessRecordingInfo.tGUIDListInfo[iSessionCount].iUnderRecording = 1;
		}
		else
		{
			tSeamlessRecordingInfo.tGUIDListInfo[iSessionCount].iUnderRecording = 0;
		}
		tSeamlessRecordingInfo.iSeamlessConnectionNumber += tSeamlessRecordingInfo.tGUIDListInfo[iSessionCount].iNumber;
		tSeamlessRecordingInfo.iSeamlessConnectionNumber += tSeamlessRecordingInfo.tGUIDListInfo[iSessionCount].iUnderRecording;
	}

	if( tSeamlessRecordingInfo.iSeamlessConnectionNumber > tSeamlessRecordingInfo.iSeamlessMaxConnection )
	{
		printf("[%s] Seamless Recording current connection number=%d > Max connection=%d\n", __FUNCTION__, 
																							tSeamlessRecordingInfo.iSeamlessConnectionNumber, 
																							tSeamlessRecordingInfo.iSeamlessMaxConnection);
		return S_FAIL;
	}
	
	return S_OK;
}
#endif

SCODE StreamingServer_ParseQosFile(TSTREAMSERVERINFO   *pThis, char* pzQosFile)
{
	int		iResult = 0;

	if(pzQosFile[0] == 0 || (access(pzQosFile, R_OK) != 0))
	{
		//The QOS file does not exist
		printf("QOS file not found. QOS features are disabled!\n");
		tQosInfo.iCosEnabled = 0;
		tQosInfo.iDscpEnabled = 0;
		return S_OK;
	}

	if((iResult = XMLSParser_ReadAll(pzQosFile, g_QosParseMap)) != S_OK)
	{
		//The QOS file does not exist
		printf("QOS config file is malformed!\n");
		tQosInfo.iCosEnabled = 0;
		tQosInfo.iDscpEnabled = 0;
		return S_FAIL;
	}

	return S_OK;
}

#ifdef _SHARED_MEM
//20100105 Added For Seamless Recording
SCODE StreamingServer_ParseSeamlessRecordingFile(TSTREAMSERVERINFO   *pThis)
{
	char	*pcDiskMode = NULL, *pcMaxConnection = NULL, *pcStream = NULL, *pcEnable = NULL;

	if((pcDiskMode = XMLSParser_ReadContent(SEAMLESSRECORDING_DISK_CONF, SEAMLESSRECORDING_DISKMODE_XPATH)) != NULL)
	{
		if(strcmp("seamless", pcDiskMode) == 0)
		{
			tSeamlessRecordingInfo.iSeamlessDiskMode = 1;
			printf("[%s] Seamless Disk Mode!\n", __FUNCTION__);
		}
		else
		{
			tSeamlessRecordingInfo.iSeamlessDiskMode = 0;
			tSeamlessRecordingInfo.iSeamlessMaxConnection = 0;
			tSeamlessRecordingInfo.iSeamlessStreamNumber = 0;
			tSeamlessRecordingInfo.iRecordingEnable = 0;
			tSeamlessRecordingInfo.iSeamlessConnectionNumber = 0;
			printf("[%s] Manageable Disk Mode!\n", __FUNCTION__);
			return S_OK;
		}
	}
	else
	{
		printf("[%s] Disk Mode not defined, default Manageable!\n", __FUNCTION__);
		tSeamlessRecordingInfo.iSeamlessDiskMode = 0;
		tSeamlessRecordingInfo.iSeamlessMaxConnection = 0;
		tSeamlessRecordingInfo.iSeamlessStreamNumber = 0;
		tSeamlessRecordingInfo.iRecordingEnable = 0;
		tSeamlessRecordingInfo.iSeamlessConnectionNumber = 0;
		return S_OK;
	}

	if((pcMaxConnection = XMLSParser_ReadContent(SEAMLESSRECORDING_CAPABILITY_CONF, SEAMLESSRECORDING_MAXCONNECTION_XPATH)) != NULL)
	{
		tSeamlessRecordingInfo.iSeamlessMaxConnection = strtoul(pcMaxConnection, NULL, 0);
		if (tSeamlessRecordingInfo.iSeamlessMaxConnection <= 0)
		{
			tSeamlessRecordingInfo.iSeamlessMaxConnection = SEAMLESSRECORDING_MAXCONNECTION_DEFAULT;
		}
		printf("[%s] Seamless Max Connection=%d\n", __FUNCTION__, tSeamlessRecordingInfo.iSeamlessMaxConnection);
	}
	else
	{
		tSeamlessRecordingInfo.iSeamlessMaxConnection = SEAMLESSRECORDING_MAXCONNECTION_DEFAULT;
		printf("[%s] Seamless Max Connection not defined, default=%d\n", __FUNCTION__, tSeamlessRecordingInfo.iSeamlessMaxConnection);
	}

	if((pcStream = XMLSParser_ReadContent(SEAMLESSRECORDING_CONF, SEAMLESSRECORDING_STREAM_XPATH)) != NULL)
	{
		tSeamlessRecordingInfo.iSeamlessStreamNumber = strtoul(pcStream, NULL, 0);
		if((tSeamlessRecordingInfo.iSeamlessStreamNumber <= MULTIPLE_STREAM_NUM) && (tSeamlessRecordingInfo.iSeamlessStreamNumber > 0))
		{
			printf("[%s] Recording stream number=%d\n", __FUNCTION__, tSeamlessRecordingInfo.iSeamlessStreamNumber);
		}
		else
		{
			tSeamlessRecordingInfo.iSeamlessStreamNumber = SEAMLESSRECORDING_STREAM_DEFAULT;
			printf("[%s] Recording stream number not support, default=%d\n", __FUNCTION__, tSeamlessRecordingInfo.iSeamlessStreamNumber);
		}
	}
	else
	{
		tSeamlessRecordingInfo.iSeamlessStreamNumber = SEAMLESSRECORDING_STREAM_DEFAULT;
		printf("[%s] Recording stream number not defined, default=%d\n", __FUNCTION__, tSeamlessRecordingInfo.iSeamlessStreamNumber);
	}

	if((pcEnable = XMLSParser_ReadContent(SEAMLESSRECORDING_CONF, SEAMLESSRECORDING_ENABLE_XPATH)) != NULL)
	{
		if(strcmp("1", pcEnable) == 0)
		{
			tSeamlessRecordingInfo.iRecordingEnable = 1;
			printf("[%s] Seamless Recording Enable!\n", __FUNCTION__);
		}
		else
		{
			tSeamlessRecordingInfo.iRecordingEnable = 0;
			printf("[%s] Seamless Recording Disable!\n", __FUNCTION__);
		}
	}
	else
	{
		printf("[%s] Warning! Seamless Recording State Unknow, default Disable!\n", __FUNCTION__);
		return S_FAIL;
	}

	if(StreamingServer_ParseGUIDListInfoConfig(tSeamlessRecordingInfo.iSeamlessMaxConnection) != S_OK)
	{
		return S_FAIL;
	}
	
	return S_OK;
}
#endif

SCODE StreamingServer_ParseAccessFile(TSTREAMSERVERINFO   *pThis, char* pzAccessFile)
{
	int     iResult;
    
	if(pzAccessFile[0] == 0 || (access(pzAccessFile, R_OK) != 0))
	{
    	printf("access list file not found! Use default all-allow mode\n");
		//20090625 access list file not specified
		tStreamServerInfo.tAllowIP[0].ulStartIP = inet_addr("1.0.0.0");
		tStreamServerInfo.tAllowIP[0].ulEndIP = inet_addr("255.255.255.255");
		return 0;
	}

    if( (iResult= XMLSParser_ReadAll(pzAccessFile, g_AcsParseMap)) != S_OK)
    {
    	printf("access list file parse failed! Use default all-allow mode\n");
		//20081231 Access list not found. Use defauly all-allow
		tStreamServerInfo.tAllowIP[0].ulStartIP = inet_addr("1.0.0.0");
		tStreamServerInfo.tAllowIP[0].ulEndIP = inet_addr("255.255.255.255");
		return 0;
 	}
 	else
 	{
 		/*
		int i;

		for(i=0 ; i<IPFILTER_NUMBER ; i++)
 		{
 			if( tStreamServerInfo.tAllowIP[i].ulStartIP != 0 && tStreamServerInfo.tAllowIP[i].ulEndIP != 0 )
 			{
 				printf("Access IP: from %s ",inet_ntoa(*((struct in_addr*)&(tStreamServerInfo.tAllowIP[i].ulStartIP))));
 				printf("to %s\n",inet_ntoa(*((struct in_addr*)&(tStreamServerInfo.tAllowIP[i].ulEndIP))));
 			}
 			
 			if( tStreamServerInfo.tDenyIP[i].ulStartIP != 0 && tStreamServerInfo.tDenyIP[i].ulEndIP != 0 )
 			{
 				printf("Deny IP: from %s ",inet_ntoa(*((struct in_addr*)&(tStreamServerInfo.tDenyIP[i].ulStartIP))));
 				printf("to %s\n",inet_ntoa(*((struct in_addr*)&(tStreamServerInfo.tDenyIP[i].ulEndIP)))); 
 			}
 		}*/
 		return 0;
 	}

}

int QueryCameraMode() //20161212 add by Faber, check auth status
{
	int iRet = 0;
	FILE *fpPipe;
	char szValue[255];
	memset(szValue, 0, 255);

	fpPipe = popen( "sysparam get flashstatus", "r" );
	fgets( szValue, 255 , fpPipe );
	if(atoi(szValue) == 250)
	{
		iRet=1;
	}
		
	pclose(fpPipe);
	return iRet;
}


SCODE StreamingServer_ParseConfigFile(TSTREAMSERVERINFO   *pThis, char* pzConfigFile)
{
	int     iResult,i;
	char	*pcWebAttraction = NULL;

    if( (iResult= XMLSParser_ReadAll(pzConfigFile, g_CfgParseMap)) == S_OK)
    {
		if(StreamingServer_ParseMediaConfig(pThis, pzConfigFile) != S_OK)
		{
			return S_FAIL;
		}
		
#ifdef RTSPRTP_MULTICAST
		//20100714 Added by danny For Multicast parameters load dynamically
		if(StreamingServer_ParseMulticastInfoConfig(pThis, pzConfigFile) != S_OK)
		{
			return S_FAIL;
		}
		//20130327 added by Jimmy to support different video/audio multicast addresses for ONVIF Test Tool 12.12
//		if(StreamingServer_ParseAudioExtraMulticastInfoConfig(pThis, pzConfigFile) != S_OK)
//		{
//			return S_FAIL;
//		}
        printf("config file parse OK!!\r\n");

		//20110725 Add by danny For Multicast RTCP receive report keep alive
		if( g_iMulticastTimeout <= 0 )
		{
			g_iMulticastTimeout = MULTICAST_TIMEOUT;
		}
#endif

		//20080925 Connect unix socket to encoder here
		for( i=0; i< VIDEO_TRACK_NUMBER; i++ )
		{
			if(CfgParser_GetUnixDomainSocket(&pThis->tVideoSrcInfo[i].acSockPathName, &pThis->tVideoSrcInfo[i].iFdSock) != S_OK)
			{
				printf("Get unix domain socket %s failed!\n", pThis->tVideoSrcInfo[i].acSockPathName);
				return S_FAIL;
			}
		}
		for( i=0; i< AUDIO_TRACK_NUMBER; i++ )
		{
			if(CfgParser_GetUnixDomainSocket(&pThis->tAudioSrcInfo[i].acSockPathName, &pThis->tAudioSrcInfo[i].iFdSock) != S_OK)
			{
				printf("Get unix domain socket %s failed!\n", pThis->tAudioSrcInfo[i].acSockPathName);
				return S_FAIL;
			}
		}
		//20120810 added by Jimmy for metadata
		for( i=0; i< METADATA_TRACK_NUMBER; i++ )
		{
			if(CfgParser_GetUnixDomainSocket(&pThis->tMetadataSrcInfo[i].acSockPathName, &pThis->tMetadataSrcInfo[i].iFdSock) != S_OK)
			{
				printf("Get unix domain socket %s failed!\n", pThis->tMetadataSrcInfo[i].acSockPathName);
				return S_FAIL;
			}
		}

       	if( initClientSocket(&(pThis->tAudioDstInfo.iFdSock)) == S_OK )
       	{       		
       		connectClientSocket(pThis->tAudioDstInfo.iFdSock,pThis->tAudioDstInfo.acSockPathName);
       	}
       	else
       	{
       		pThis->tAudioDstInfo.iFdSock = -1;
       		printf("Error!! Can not create audio decoder socket\n");	
       		syslog(LOG_ALERT,"Open audio decoder socket failed\n");
       	}
	      	
       	if ( CfgParser_GetFIFO(pThis->tRTSPInfo.szContorlipcFIFO,
        		 					&(pThis->tRTSPInfo.iControlFIFO),O_RDONLY) != S_OK )
        {
        	printf("Control FIFO failed for Streaming Server!\n");        	
        	syslog(LOG_ALERT,"Control FIFO open failed\n");
        }
		
        
		
		//Added by Louis 2008/01/29 for Multicast audio/video only
		if((iResult = XMLSParser_ReadAll(pzConfigFile, g_DVTELMulticastParseMap)) != S_OK)
		{
			printf("iResult is %d\n", iResult);
			//20120810 modified by Jimmy for metadata
			printf("%d %d %d %d %d %d\n",pThis->tMulticastInfo[0].iAlwaysMulticastAudio,
				pThis->tMulticastInfo[0].iAlwaysMulticastVideo,
				pThis->tMulticastInfo[0].iAlwaysMulticastMetadata,
				pThis->tMulticastInfo[1].iAlwaysMulticastAudio,
				pThis->tMulticastInfo[1].iAlwaysMulticastVideo,
				pThis->tMulticastInfo[1].iAlwaysMulticastMetadata);
			
		}
		//for( i=0 ; i<MULTIPLE_STREAM_NUM ; i++)
		for(i=0; i<RTSP_MULTICASTNUMBER; i++)
		{
			//20120810 modified by Jimmy for metadata
			if((pThis->tMulticastInfo[i].iAlwaysMulticastAudio == 0) && (pThis->tMulticastInfo[i].iAlwaysMulticastVideo == 0) && (pThis->tMulticastInfo[i].iAlwaysMulticastMetadata== 0))
			{
				pThis->tMulticastInfo[i].iAlwaysMulticastAudio = 1;
				pThis->tMulticastInfo[i].iAlwaysMulticastVideo = 1;
				//20120810 added by Jimmy for metadata
				pThis->tMulticastInfo[i].iAlwaysMulticastMetadata= 1;
			}

			//20130327 added by Jimmy to support different video/audio multicast addresses for ONVIF Test Tool 12.12
			pThis->tMulticastInfo[i + RTSP_MULTICASTNUMBER].iAlwaysMulticastAudio = 1;
			pThis->tMulticastInfo[i + RTSP_MULTICASTNUMBER].iAlwaysMulticastVideo = 0;
			pThis->tMulticastInfo[i + RTSP_MULTICASTNUMBER].iAlwaysMulticastMetadata = 0;

			if((pThis->tMulticastInfo[i + RTSP_MULTICASTNUMBER].ulMulticastAddress == 0) || (pThis->tMulticastInfo[i].ulMulticastAddress == pThis->tMulticastInfo[i + RTSP_MULTICASTNUMBER].ulMulticastAddress))
			{
				pThis->tMulticastInfo[i + RTSP_MULTICASTNUMBER].iAlwaysMulticast = FALSE;
			}

			pThis->tMulticastInfo[i + RTSP_MULTICASTNUMBER].iEnable = pThis->tMulticastInfo[i + RTSP_MULTICASTNUMBER].iAlwaysMulticast;
		}	

        for( i=0 ; i<MULTIPLE_STREAM_NUM ; i++)
        {        	
        	if( pThis->tStreamInfo[i].iEnable != 0 )
        	{	
				//20101210 Added by danny For Media shmem config
				//Modified 20081119 to fit single FIFO by Louis
				if((i > 0) && (i < LIVE_STREAM_NUM) && (pThis->tStreamInfo[i].iVideoSrcIndex >= 0)
					&& (strcmp(pThis->tVideoSrcInfo[pThis->tStreamInfo[i-1].iVideoSrcIndex - 1].acFIFOPathName, pThis->tVideoSrcInfo[pThis->tStreamInfo[i].iVideoSrcIndex].acFIFOPathName) == 0)
					&& pThis->tVideoSrcInfo[pThis->tStreamInfo[i-1].iVideoSrcIndex - 1].iFdFIFO > 0)
				{
					pThis->tVideoSrcInfo[pThis->tStreamInfo[i].iVideoSrcIndex].iFdFIFO = pThis->tVideoSrcInfo[pThis->tStreamInfo[i-1].iVideoSrcIndex - 1].iFdFIFO;
					printf("Video: Track %d Src %s Track %d Src %s the Same!\n", pThis->tStreamInfo[i-1].iVideoSrcIndex - 1, pThis->tVideoSrcInfo[pThis->tStreamInfo[i-1].iVideoSrcIndex - 1].acFIFOPathName,
																		  pThis->tStreamInfo[i].iVideoSrcIndex, pThis->tVideoSrcInfo[pThis->tStreamInfo[i].iVideoSrcIndex].acFIFOPathName);
				}
				//20100428 Added For Media on demand
				else if((i >= LIVE_STREAM_NUM) && (pThis->tStreamInfo[i].iVideoSrcIndex >= 0) 
					&& (strcmp(pThis->tVideoSrcInfo[pThis->tStreamInfo[i-1].iVideoSrcIndex - 1].acFIFOPathName, pThis->tVideoSrcInfo[pThis->tStreamInfo[i].iVideoSrcIndex].acFIFOPathName) == 0)
					&& pThis->tVideoSrcInfo[pThis->tStreamInfo[i-1].iVideoSrcIndex - 1].iFdFIFO > 0)
				{
					pThis->tVideoSrcInfo[pThis->tStreamInfo[i].iVideoSrcIndex].iFdFIFO = pThis->tVideoSrcInfo[pThis->tStreamInfo[i-1].iVideoSrcIndex - 1].iFdFIFO;
					printf("MOD Video: Track %d Src %s Track %d Src %s the Same!\n", pThis->tStreamInfo[i-1].iVideoSrcIndex - 1, pThis->tVideoSrcInfo[pThis->tStreamInfo[i-1].iVideoSrcIndex - 1].acFIFOPathName,
																		  pThis->tStreamInfo[i].iVideoSrcIndex, pThis->tVideoSrcInfo[pThis->tStreamInfo[i].iVideoSrcIndex].acFIFOPathName);
				}
				else
				{
			        if( pThis->tStreamInfo[i].iVideoSrcIndex >= 0 )
        				CfgParser_GetFIFO(pThis->tVideoSrcInfo[pThis->tStreamInfo[i].iVideoSrcIndex].acFIFOPathName,
        		 						&(pThis->tVideoSrcInfo[pThis->tStreamInfo[i].iVideoSrcIndex].iFdFIFO),O_WRONLY);
				}
				
				if((i > 0) && (i < LIVE_STREAM_NUM) && (pThis->tStreamInfo[i].iAudioSrcIndex >= 0)
					&& (strcmp(pThis->tAudioSrcInfo[pThis->tStreamInfo[i-1].iAudioSrcIndex - 1].acFIFOPathName, pThis->tAudioSrcInfo[pThis->tStreamInfo[i].iAudioSrcIndex].acFIFOPathName) == 0)
					&& pThis->tAudioSrcInfo[pThis->tStreamInfo[i-1].iAudioSrcIndex - 1].iFdFIFO > 0)
				{
					pThis->tAudioSrcInfo[pThis->tStreamInfo[i].iAudioSrcIndex].iFdFIFO = pThis->tAudioSrcInfo[pThis->tStreamInfo[i-1].iAudioSrcIndex - 1].iFdFIFO;
					printf("Audio: Track %d Src %s Track %d Src %s the Same!\n", pThis->tStreamInfo[i-1].iAudioSrcIndex - 1, pThis->tAudioSrcInfo[pThis->tStreamInfo[i-1].iAudioSrcIndex - 1].acFIFOPathName,
																		  pThis->tStreamInfo[i].iAudioSrcIndex, pThis->tAudioSrcInfo[pThis->tStreamInfo[i].iAudioSrcIndex].acFIFOPathName);
				}
				//20100428 Added For Media on demand
				else if((i >= LIVE_STREAM_NUM) && (pThis->tStreamInfo[i].iAudioSrcIndex >= 0) 
					&& (strcmp(pThis->tAudioSrcInfo[pThis->tStreamInfo[i-1].iAudioSrcIndex - 1].acFIFOPathName, pThis->tAudioSrcInfo[pThis->tStreamInfo[i].iAudioSrcIndex].acFIFOPathName) == 0)
					&& pThis->tAudioSrcInfo[pThis->tStreamInfo[i-1].iAudioSrcIndex - 1].iFdFIFO > 0)
				{
					pThis->tAudioSrcInfo[pThis->tStreamInfo[i].iAudioSrcIndex].iFdFIFO = pThis->tAudioSrcInfo[pThis->tStreamInfo[i-1].iAudioSrcIndex - 1].iFdFIFO;
					printf("MOD Audio: Track %d Src %s Track %d Src %s the Same!\n", pThis->tStreamInfo[i-1].iAudioSrcIndex - 1, pThis->tAudioSrcInfo[pThis->tStreamInfo[i-1].iAudioSrcIndex - 1].acFIFOPathName,
																		  pThis->tStreamInfo[i].iAudioSrcIndex, pThis->tAudioSrcInfo[pThis->tStreamInfo[i].iAudioSrcIndex].acFIFOPathName);
				}
				else
				{
					if( pThis->tStreamInfo[i].iAudioSrcIndex >=0 )       		 					
        				CfgParser_GetFIFO(pThis->tAudioSrcInfo[pThis->tStreamInfo[i].iAudioSrcIndex].acFIFOPathName,
        		 						&(pThis->tAudioSrcInfo[pThis->tStreamInfo[i].iAudioSrcIndex].iFdFIFO),O_WRONLY);
				}

				//20120810 added by Jimmy for metadata
				if((i > 0) && (i < LIVE_STREAM_NUM) && (pThis->tStreamInfo[i].iMetadataSrcIndex>= 0)
					&& (strcmp(pThis->tMetadataSrcInfo[pThis->tStreamInfo[i-1].iMetadataSrcIndex - 1].acFIFOPathName, pThis->tMetadataSrcInfo[pThis->tStreamInfo[i].iMetadataSrcIndex].acFIFOPathName) == 0)
					&& pThis->tMetadataSrcInfo[pThis->tStreamInfo[i-1].iMetadataSrcIndex- 1].iFdFIFO > 0)
				{
					pThis->tMetadataSrcInfo[pThis->tStreamInfo[i].iMetadataSrcIndex].iFdFIFO = pThis->tMetadataSrcInfo[pThis->tStreamInfo[i-1].iMetadataSrcIndex- 1].iFdFIFO;
					printf("Metadata: Track %d Src %s Track %d Src %s the Same!\n", pThis->tStreamInfo[i-1].iMetadataSrcIndex - 1, pThis->tMetadataSrcInfo[pThis->tStreamInfo[i-1].iMetadataSrcIndex - 1].acFIFOPathName,
																		  pThis->tStreamInfo[i].iMetadataSrcIndex, pThis->tMetadataSrcInfo[pThis->tStreamInfo[i].iMetadataSrcIndex].acFIFOPathName);
				}
				//20100428 Added For Media on demand
				else if((i >= LIVE_STREAM_NUM) && (pThis->tStreamInfo[i].iMetadataSrcIndex>= 0) 
					&& (strcmp(pThis->tMetadataSrcInfo[pThis->tStreamInfo[i-1].iMetadataSrcIndex - 1].acFIFOPathName, pThis->tMetadataSrcInfo[pThis->tStreamInfo[i].iMetadataSrcIndex].acFIFOPathName) == 0)
					&& pThis->tMetadataSrcInfo[pThis->tStreamInfo[i-1].iMetadataSrcIndex - 1].iFdFIFO > 0)
				{
					pThis->tMetadataSrcInfo[pThis->tStreamInfo[i].iMetadataSrcIndex].iFdFIFO = pThis->tMetadataSrcInfo[pThis->tStreamInfo[i-1].iMetadataSrcIndex - 1].iFdFIFO;
					printf("MOD Metadata: Track %d Src %s Track %d Src %s the Same!\n", pThis->tStreamInfo[i-1].iMetadataSrcIndex - 1, pThis->tMetadataSrcInfo[pThis->tStreamInfo[i-1].iMetadataSrcIndex - 1].acFIFOPathName,
																		  pThis->tStreamInfo[i].iMetadataSrcIndex, pThis->tMetadataSrcInfo[pThis->tStreamInfo[i].iMetadataSrcIndex].acFIFOPathName);
				}
				else
				{
					if( pThis->tStreamInfo[i].iMetadataSrcIndex >=0 )
					{
						open( pThis->tMetadataSrcInfo[pThis->tStreamInfo[i].iMetadataSrcIndex].acFIFOPathName, O_RDONLY | O_NONBLOCK);
        				CfgParser_GetFIFO(pThis->tMetadataSrcInfo[pThis->tStreamInfo[i].iMetadataSrcIndex].acFIFOPathName,
        		 						&(pThis->tMetadataSrcInfo[pThis->tStreamInfo[i].iMetadataSrcIndex].iFdFIFO),O_WRONLY);
					}
				}

				rtspstrcpy(pThis->tStreamInfo[i].szSDPFullPathName,SDP_PATH, sizeof(pThis->tStreamInfo[i].szSDPFullPathName));
            	rtspstrcat(pThis->tStreamInfo[i].szSDPFullPathName,pThis->tStreamInfo[i].szAccessName, sizeof(pThis->tStreamInfo[i].szSDPFullPathName));        		 					
        	}
        	else
        	{
        		pThis->tStreamInfo[i].szAccessName[0] = 0;
        	}
        	
        	if( (pThis->tStreamInfo[i].iVideoSrcIndex >=0) && 
        	    (pThis->tVideoSrcInfo[pThis->tStreamInfo[i].iVideoSrcIndex].iFdSock < 0 ))
        	{
        		printf("video failed\n");
        	    return -1;
        	}
        	    
			if( (pThis->tStreamInfo[i].iAudioSrcIndex >=0) && 
        	    (pThis->tAudioSrcInfo[pThis->tStreamInfo[i].iAudioSrcIndex].iFdSock < 0 ))
        	{
        		printf("audio failed %d %d\n",pThis->tStreamInfo[i].iAudioSrcIndex,pThis->tAudioSrcInfo[pThis->tStreamInfo[i].iAudioSrcIndex].iFdSock);
        	    return -1;        	    
        	}

			//20120810 added by Jimmy for metadata
			if( (pThis->tStreamInfo[i].iMetadataSrcIndex>=0) && 
        	    (pThis->tMetadataSrcInfo[pThis->tStreamInfo[i].iMetadataSrcIndex].iFdSock < 0 ))
        	{
        		printf("metadata failed %d %d\n",pThis->tStreamInfo[i].iMetadataSrcIndex,pThis->tMetadataSrcInfo[pThis->tStreamInfo[i].iMetadataSrcIndex].iFdSock);
        	    return -1;        	    
        	}

			//20120810 modified by Jimmy for metadata
			printf("Stream %d (%s): video track %d, audio track %d, metadata track %d\r\n",i+1,pThis->tStreamInfo[i].szAccessName,pThis->tStreamInfo[i].iVideoSrcIndex,pThis->tStreamInfo[i].iAudioSrcIndex,pThis->tStreamInfo[i].iMetadataSrcIndex);
			
            if( pThis->tStreamInfo[i].iVideoSrcIndex >=0 )                                     
                pThis->tStreamInfo[i].iVideoSrcIndex++;                                     
            
            if( pThis->tStreamInfo[i].iAudioSrcIndex >= 0 )
                pThis->tStreamInfo[i].iAudioSrcIndex++;

			//20120810 added by Jimmy for metadata
            if( pThis->tStreamInfo[i].iMetadataSrcIndex >= 0 )
                pThis->tStreamInfo[i].iMetadataSrcIndex++;

        }               
        return S_OK;
    } 
    else
    {
        printf("config file parse failed!!\r\n");
        return S_FAIL;    
    } 

}

#ifdef _SHARED_MEM
//20100428 Added For Media on demand
SCODE StreamingServer_EscapeString(const char* pinput, char* pbuf)
{       
	char *ix;    
	unsigned char c;    

	if( pinput == NULL || pbuf == NULL )        
	{
		return S_FAIL;
	}
	//printf("[%s]pinput=%s\n", __FUNCTION__, pinput);
	ix = pbuf;    
	while( (c = *pinput++) ) 
	{       
		if( needs_escape((unsigned int) c) ) 
		{            
			*ix++ = '%';            
			*ix++ = INT_TO_HEX((c >> 4) & 0xf);            
			*ix++ = INT_TO_HEX(c & 0xf);        
		} 
		else
		{ 
			*ix++ = c;
		}
	}   
	*ix = '\0';    
	//printf("[%s]pbuf=%s\n", __FUNCTION__, pbuf);
	return S_OK;
}

char* StreamingServer_GetMODCommand( int iCode )
{
    TKN   *pTKN;
   
    for ( pTKN = g_MODCommand; pTKN->code != -1; pTKN++ )
    {  
        if ( pTKN->code == iCode )
        {
            return pTKN->token ;
        }
    }

	printf("Invalid MOD Command Code used!\n");
    DbgLog((dfCONSOLE|dfINTERNAL,"??? Invalid MOD Command Code used." ));
    return g_acInvld_command ;
}

SCODE StreamingServer_SetMODControlParam(HANDLE hObject, RTSPSERVER_MODREQUEST *pstRTSPServerMODRequest)
{
	TSTREAMSERVERINFO		*pThis;
	int                     i,iCount,iRes, iWritten;
    char                    acTemp[256];
	
	pThis = (TSTREAMSERVERINFO *) hObject;

	memset(acTemp, 0, sizeof(acTemp));
	acTemp[0] = 0x02;

	if( pstRTSPServerMODRequest == NULL )
	{
		printf("[%s]No ptMODInfo!\n", __FUNCTION__);
		return S_FAIL;
	}
	i = pstRTSPServerMODRequest->iSDPIndex - 1;

	if( pThis->tVideoSrcInfo[i].iFdFIFO > 0 )
    {
		iWritten = 0;
		iWritten = snprintf(acTemp + 2, sizeof(acTemp) - iWritten - 2 - 1,  "<control id=\"0\" stream=\"%d\"><video/>", pThis->tStreamInfo[i].iVideoSrcIndex - LIVE_STREAM_NUM);
		if( pstRTSPServerMODRequest->eMODSetCommandType != 0 )
		{
			//20110915 Modify by danny for support Genetec MOD
			iWritten += snprintf(acTemp + 2 + iWritten, sizeof(acTemp) - iWritten - 2 - 1, "<trickplay>%s%s", 
										StreamingServer_GetMODCommand(pstRTSPServerMODRequest->eMODSetCommandType), pstRTSPServerMODRequest->acMODSetCommandValue);
		}
		iWritten += snprintf(acTemp + 2 + iWritten, sizeof(acTemp) - iWritten - 2 - 1, "%s", CONTROL_MSG_MOD_TRICKPLAY);
		acTemp[1] = iWritten;

		iCount = 0;
    	while(1)
    	{
    		if( (iRes = write(pThis->tVideoSrcInfo[i].iFdFIFO,acTemp,acTemp[1]+2)) > 0 )
			{
				printf("%d bytes %s sent to MOD server!\n",iRes, acTemp);
				break;
			}
			else
			{
				iCount ++;
				usleep(1000);
			}    

			if( iCount > 5 )
			{
				printf("[%s]Send MODControlParam to Video fifo failed!\n", __FUNCTION__);
				return S_FAIL;
			}
		}
	}
	else
	{
		printf("[%s]tVideoSrcInfo[%d].iFdFIFO <= 0\n", __FUNCTION__, i);
		return S_FAIL;
	}

	memset(acTemp, 0, sizeof(acTemp));
	acTemp[0] = 0x02;
	
	if( pThis->tAudioSrcInfo[pstRTSPServerMODRequest->iSDPIndex - LIVE_STREAM_NUM].iFdFIFO > 0 )
	{
		iWritten = 0;
		iWritten = snprintf(acTemp + 2, sizeof(acTemp) - iWritten - 2 - 1,  "<control id=\"0\" stream=\"%d\"><audio/>", pThis->tStreamInfo[i].iAudioSrcIndex - LIVE_AUDIO_STREAM_NUM);
		if( pstRTSPServerMODRequest->eMODSetCommandType != 0 )
		{
			//20110915 Modify by danny for support Genetec MOD
			iWritten += snprintf(acTemp + 2 + iWritten, sizeof(acTemp) - iWritten - 2 - 1, "<trickplay>%s%s", 
										StreamingServer_GetMODCommand(pstRTSPServerMODRequest->eMODSetCommandType), pstRTSPServerMODRequest->acMODSetCommandValue);
		}
		iWritten += snprintf(acTemp + 2 + iWritten, sizeof(acTemp) - iWritten - 2 - 1, "%s", CONTROL_MSG_MOD_TRICKPLAY);
		acTemp[1] = iWritten;
		
		iCount = 0;
		while(1)
		{
			if( (iRes = write(pThis->tAudioSrcInfo[pstRTSPServerMODRequest->iSDPIndex - LIVE_STREAM_NUM].iFdFIFO,acTemp,acTemp[1]+2)) > 0 )
			{
				printf("%d bytes %s sent to MOD server!\n",iRes,acTemp);
				break;
			}
			else
			{
				iCount ++;
				usleep(1000);
			}   

			if( iCount > 5 )
			{
				printf("[%s]Send MODControlParam to Audio fifo failed!\n", __FUNCTION__);
				return S_FAIL;
			}
		}
	}
	else
	{
		printf("[%s]tAudioSrcInfo[%d].iFdFIFO <= 0\n", __FUNCTION__, pstRTSPServerMODRequest->iSDPIndex - LIVE_STREAM_NUM);
		return S_FAIL;
	}

	//20120829 added by Jimmy for metadata
	/*
	if( pThis->tMetadataSrcInfo[i].iFdFIFO > 0 )
	{
		iWritten = 0;
		iWritten = snprintf(acTemp + 2, sizeof(acTemp) - iWritten - 2 - 1,  "<control id=\"0\" stream=\"%d\"><metadata/>", pThis->tStreamInfo[i].iMetadataSrcIndex - LIVE_METADATA_STREAM_NUM);
		if( pstRTSPServerMODRequest->eMODSetCommandType != 0 )
		{
			iWritten += snprintf(acTemp + 2 + iWritten, sizeof(acTemp) - iWritten - 2 - 1, "<trickplay>%s=%s", 
										StreamingServer_GetMODCommand(pstRTSPServerMODRequest->eMODSetCommandType), pstRTSPServerMODRequest->acMODSetCommandValue);
		}
		iWritten += snprintf(acTemp + 2 + iWritten, sizeof(acTemp) - iWritten - 2 - 1, "%s", CONTROL_MSG_MOD_TRICKPLAY);
		acTemp[1] = iWritten;
		
		iCount = 0;
		while(1)
		{
			if( (iRes = write(pThis->tMetadataSrcInfo[i].iFdFIFO,acTemp,acTemp[1]+2)) > 0 )
			{
				printf("%d bytes %s sent to MOD server!\n",iRes,acTemp);
				break;
			}
			else
			{
				iCount ++;
				usleep(1000);
			}   

			if( iCount > 5 )
			{
				printf("[%s]Send MODControlParam to Metadata fifo failed!\n", __FUNCTION__);
				return S_FAIL;
			}
		}
	}
	else
	{
		printf("[%s]tMetadataSrcInfo[%d].iFdFIFO <= 0\n", __FUNCTION__, i);
		return S_FAIL;
	}
	*/
	
	return S_OK;
}

SCODE StreamingServer_SetMODMediaTrackParam(HANDLE hObject, TMultipleStreamCIInfo *ptCIInfo)
{
	TSTREAMSERVERINFO		*pThis;
	int                     i,iCount,iRes, iWritten, iTLVRet;
    char                    acEscapeOutput[RTSP_URL_EXTRA_LEN * 3], acTemp[RTSP_URL_EXTRA_LEN * 4], acTLVDate[RTSP_URL_EXTRA_LEN * 3 + 4];
	char 					*pQueryString;
	   
	pThis = (TSTREAMSERVERINFO *) hObject;
	memset(acTLVDate, 0, sizeof(acTLVDate));
	
	if( ptCIInfo == NULL )
	{
		printf("[%s]No ptCIInfo!\n", __FUNCTION__);
		return S_FAIL;
	}
	i = ptCIInfo->iSDPIndex - 1;

	pQueryString = &acTLVDate[0];
	*pQueryString++ = 0x02;
	
	if( StreamingServer_EscapeString(ptCIInfo->pcExtraInfo, acEscapeOutput) != S_OK )
	{
		printf("[%s]Escape String failed!\n", __FUNCTION__);
		return S_FAIL;
	}
	
	if( pThis->tVideoSrcInfo[i].iFdFIFO > 0 )
    {
		iWritten = 0;
		iWritten = snprintf(acTemp, sizeof(acTemp) - iWritten - 1,  "<control id=\"0\" stream=\"%d\"><video/>", pThis->tStreamInfo[i].iVideoSrcIndex - LIVE_STREAM_NUM);
		iWritten += snprintf(acTemp + iWritten, sizeof(acTemp) - iWritten - 1, "<querystring>%s</querystring>", acEscapeOutput);
		iWritten += snprintf(acTemp + iWritten, sizeof(acTemp) - iWritten - 1, "%s", CONTROL_MSG_FORCECI);

		iTLVRet = 0;
		iTLVRet = RtspRtpCommon_TLVStrlen(pQueryString, iWritten);
		pQueryString += iTLVRet;
		rtspstrcpy(pQueryString, acTemp, sizeof(acTLVDate) - 4);
		
        OSCriticalSection_Enter(pThis->tAudioSrcInfo[ptCIInfo->iSDPIndex - LIVE_STREAM_NUM].hMediaSrcMutex);
        printf("video before write\n");

		iCount = 0;
    	while(1)
    	{
    		if( (iRes = write(pThis->tVideoSrcInfo[i].iFdFIFO, acTLVDate, iWritten + iTLVRet + 1)) > 0 )
			{
				printf("%d bytes %s sent to MOD server!\n", iRes, acTLVDate);
				break;
			}
			else
			{
				iCount ++;
				usleep(1000);
			}    

			if( iCount > 5 )
			{
				printf("[%s]Send FORCECI to Video fifo failed!\n", __FUNCTION__);
                OSCriticalSection_Leave(pThis->tAudioSrcInfo[ptCIInfo->iSDPIndex - LIVE_STREAM_NUM].hMediaSrcMutex);
				return S_FAIL;
			}
		}
        OSCriticalSection_Leave(pThis->tAudioSrcInfo[ptCIInfo->iSDPIndex - LIVE_STREAM_NUM].hMediaSrcMutex);
	}
	else
	{
		printf("[%s]tVideoSrcInfo[%d].iFdFIFO <= 0\n", __FUNCTION__, i);
		return S_FAIL;
	}

	pQueryString = &acTLVDate[0];
	*pQueryString++ = 0x02;
	
	if( pThis->tAudioSrcInfo[ptCIInfo->iSDPIndex - LIVE_STREAM_NUM].iFdFIFO > 0 )
	{
		iWritten = 0;
		iWritten = snprintf(acTemp, sizeof(acTemp) - iWritten - 1,  "<control id=\"0\" stream=\"%d\"><audio/>", pThis->tStreamInfo[i].iAudioSrcIndex - LIVE_AUDIO_STREAM_NUM);
		iWritten += snprintf(acTemp + iWritten, sizeof(acTemp) - iWritten - 1, "<querystring>%s</querystring>", acEscapeOutput);
		iWritten += snprintf(acTemp + iWritten, sizeof(acTemp) - iWritten - 1, "%s", CONTROL_MSG_FORCECI);

		iTLVRet = 0;
		iTLVRet = RtspRtpCommon_TLVStrlen(pQueryString, iWritten);
		pQueryString += iTLVRet;
		rtspstrcpy(pQueryString, acTemp, sizeof(acTLVDate) - 4);
		
		iCount = 0;
        OSCriticalSection_Enter(pThis->tAudioSrcInfo[ptCIInfo->iSDPIndex - LIVE_STREAM_NUM].hMediaSrcMutex);
        //printf("audio before write\n");
		while(1)
		{
			if( (iRes = write(pThis->tAudioSrcInfo[ptCIInfo->iSDPIndex - LIVE_STREAM_NUM].iFdFIFO, acTLVDate, iWritten + iTLVRet + 1)) > 0 )
			{
				printf("%d bytes %s sent to MOD server!\n", iRes, acTLVDate);
				break;
			}
			else
			{
				iCount ++;
				usleep(1000);
			}   

			if( iCount > 5 )
			{
				printf("[%s]Send FORCECI to Audio fifo failed!\n", __FUNCTION__);
                OSCriticalSection_Leave(pThis->tAudioSrcInfo[ptCIInfo->iSDPIndex - LIVE_STREAM_NUM].hMediaSrcMutex);
				return S_FAIL;
			}
		}
        OSCriticalSection_Leave(pThis->tAudioSrcInfo[ptCIInfo->iSDPIndex - LIVE_STREAM_NUM].hMediaSrcMutex);
	}
	else
	{
		printf("[%s]tAudioSrcInfo[%d].iFdFIFO <= 0\n", __FUNCTION__, ptCIInfo->iSDPIndex - LIVE_STREAM_NUM);
		return S_FAIL;
	}

	//20120829 added by Jimmy for metadata
	/*
	pQueryString = &acTLVDate[0];
	*pQueryString++ = 0x02;
	
	if( pThis->tMetadataSrcInfo[ptCIInfo->iSDPIndex - LIVE_STREAM_NUM].iFdFIFO > 0 )
	{
		iWritten = 0;
		iWritten = snprintf(acTemp, sizeof(acTemp) - iWritten - 1,  "<control id=\"0\" stream=\"%d\"><audio/>", pThis->tStreamInfo[i].iMetadataSrcIndex - LIVE_METADATA_STREAM_NUM);
		iWritten += snprintf(acTemp + iWritten, sizeof(acTemp) - iWritten - 1, "<querystring>%s</querystring>", acEscapeOutput);
		iWritten += snprintf(acTemp + iWritten, sizeof(acTemp) - iWritten - 1, "%s", CONTROL_MSG_FORCECI);

		iTLVRet = 0;
		iTLVRet = RtspRtpCommon_TLVStrlen(pQueryString, iWritten);
		pQueryString += iTLVRet;
		rtspstrcpy(pQueryString, acTemp, sizeof(acTLVDate) - 4);
		
		iCount = 0;
		while(1)
		{
			if( (iRes = write(pThis->tMetadataSrcInfo[ptCIInfo->iSDPIndex - LIVE_STREAM_NUM].iFdFIFO, acTLVDate, iWritten + iTLVRet + 1)) > 0 )
			{
				printf("%d bytes %s sent to MOD server!\n", iRes, acTLVDate);
				break;
			}
			else
			{
				iCount ++;
				usleep(1000);
			}   

			if( iCount > 5 )
			{
				printf("[%s]Send FORCECI to Metadata fifo failed!\n", __FUNCTION__);
				return S_FAIL;
			}
		}
	}
	else
	{
		printf("[%s]tMetadataSrcInfo[%d].iFdFIFO <= 0\n", __FUNCTION__, i);
		return S_FAIL;
	}
	*/
	
	return S_OK;
}
#endif

//20100728 Added by danny For multiple channels videoin/audioin
int StreamingServer_GetMultipleChannelStreamIndex(HANDLE hObject, int iIndex)
{
	TSTREAMSERVERINFO		*pThis;
	div_t	answer;
	
	pThis = (TSTREAMSERVERINFO *) hObject;
	
	if( pThis->dwChannelNumber == 1 )
	{
		return iIndex;
	}
	else
	{
		answer = div(iIndex - 1, pThis->dwPerChannelStreamNumber);
		answer.rem += 1;
		return answer.rem;
	}
}

int StreamingServer_GetMultipleChannelChannelIndex(HANDLE hObject, int iIndex)
{
	TSTREAMSERVERINFO		*pThis;
	div_t	answer;
	
	pThis = (TSTREAMSERVERINFO *) hObject;
	
	if( pThis->dwChannelNumber == 1 )
	{
		return pThis->dwChannelNumber;
	}
	else
	{
		answer = div(iIndex - 1, pThis->dwPerChannelStreamNumber);
		answer.quot += 1;
		return answer.quot;
	}
}

SCODE StreamingServer_SetMediaTrackParam(HANDLE hObject, TMultipleStreamCIInfo *ptCIInfo)
{
	TSTREAMSERVERINFO		*pThis;
	int                     i,iCount,iRes;
    char                    acTemp[256];
	//20100728 Added by danny For multiple channels videoin/audioin
	int 	iStreamIndex;
       
	pThis = (TSTREAMSERVERINFO *) hObject;
	          
    acTemp[0] = 0x02;

	//20100428 Added For Media on demand
    for( i=0; i< VIDEO_TRACK_NUMBER - MOD_STREAM_NUM; i++ )
    {
        if( pThis->tVideoSrcInfo[i].iFdFIFO > 0 )
        {
			//20100728 Added by danny For multiple channels videoin/audioin
			iStreamIndex = StreamingServer_GetMultipleChannelStreamIndex((HANDLE)pThis, i+1);
				
			//20090330 modified again
			if(ptCIInfo != NULL && i == (ptCIInfo->iSDPIndex - 1))
			{
				int		iWritten = 0;

				//20100728 Modified by danny For multiple channels videoin/audioin
				iWritten = snprintf(acTemp+2, sizeof(acTemp) - iWritten - 2,  "<control id=\"0\" stream=\"%d\">", iStreamIndex);
				if(ptCIInfo->acResolution[0] != 0)
				{
					iWritten += snprintf(acTemp + 2 + iWritten, sizeof(acTemp) - iWritten - 2, "<resolution>%s</resolution>", ptCIInfo->acResolution);
				}
				if(ptCIInfo->acCodecType[0] != 0)
				{
					iWritten += snprintf(acTemp + 2 + iWritten, sizeof(acTemp) - iWritten - 2, "<codectype>%s</codectype>", ptCIInfo->acCodecType);
				}
				iWritten += snprintf(acTemp + 2 + iWritten, sizeof(acTemp) - iWritten - 2, "%s", CONTROL_MSG_FORCECI);
				acTemp[1] = iWritten;
			}
			else
			{
				//20100728 Modified by danny For multiple channels videoin/audioin
				acTemp[1] = snprintf(acTemp+2, sizeof(acTemp) - 2,"<control id=\"0\" stream=\"%d\">%s", iStreamIndex, CONTROL_MSG_FORCECI);
			}

            iCount = 0;
            while(1)
            {
                if( (iRes = write(pThis->tVideoSrcInfo[i].iFdFIFO,acTemp,acTemp[1]+2)) > 0 )
				{
					printf("%d bytes %s sent to videoslave!\n",iRes, acTemp);
					break;
				}
                else        
                {                    
                    iCount ++;
                    usleep(1000);                    
                }    
                
                if( iCount > 5 )
                    return S_FAIL;
            }    
        }
    }

	//20100428 Added For Media on demand
    for( i=0; i< AUDIO_TRACK_NUMBER - MOD_STREAM_NUM; i++ )
    {
        if( pThis->tAudioSrcInfo[i].iFdFIFO > 0 )
        {
			//20100728 Added by danny For multiple channels videoin/audioin
			iStreamIndex = StreamingServer_GetMultipleChannelStreamIndex((HANDLE)pThis, i+1);
			
			//20100728 Modified by danny For multiple channels videoin/audioin
			//Modified and moved here by Louis 20081201
			acTemp[1] = snprintf(acTemp+2, sizeof(acTemp) - 2,"<control id=\"0\" stream=\"%d\">%s", iStreamIndex, CONTROL_MSG_FORCECI);

            iCount = 0;
            while(1)            
            {
                if( (iRes = write(pThis->tAudioSrcInfo[i].iFdFIFO,acTemp,acTemp[1]+2)) > 0 )               
                {
                    printf("%d bytes %s sent to audioslave!\n",iRes,acTemp);
                    break;
                }
                else        
                {                    
                    iCount ++;
                    usleep(1000);                    
                }   
                
                if( iCount > 5 )
                    return S_FAIL;
            }        
        }
    }

	//20120801 added by Jimmy for metadata
	//metadata not support mod
    for( i=0; i< METADATA_TRACK_NUMBER; i++ )
    {
        if( pThis->tMetadataSrcInfo[i].iFdFIFO > 0 )
        {
        	//20100728 Added by danny For multiple channels videoin/audioin
			iStreamIndex = StreamingServer_GetMultipleChannelStreamIndex((HANDLE)pThis, i+1);

			//20100728 Modified by danny For multiple channels videoin/audioin
			//Modified and moved here by Louis 20081201
			acTemp[1] = snprintf(acTemp+2, sizeof(acTemp) - 2,"<control id=\"0\" stream=\"%d\">%s", iStreamIndex, CONTROL_MSG_FORCECI);

            iCount = 0;
            while(1)            
            {
                if( (iRes = write(pThis->tMetadataSrcInfo[i].iFdFIFO,acTemp,acTemp[1]+2)) > 0 )               
                {
                    printf("%d bytes %s sent to metadataslave!\n",iRes,acTemp);
                    break;
                }
                else        
                {                    
                    iCount ++;
                    usleep(1000);                    
                }   
                
                if( iCount > 5 )
                    return S_FAIL;
            }        
        }
    }

#ifdef RTSPRTP_MULTICAST
	//20100714 Added by danny For Multicast parameters load dynamically, return for ForceCI only
	if( ptCIInfo != NULL )
	{
		return S_OK;
	}

    acTemp[0] = 0x02;

	//20100428 Added For Media on demand
    for( i=0; i<MULTIPLE_STREAM_NUM - MOD_STREAM_NUM; i++ )
    {
		//20110630 Add by danny For Multicast enable/disable
		if( (pThis->tStreamInfo[i].iEnable == TRUE) &&
            (pThis->tMulticastInfo[i].iAlwaysMulticast == TRUE) &&
            (pThis->tMulticastInfo[i].iEnable != 0) &&
            (pThis->tMulticastInfo[i].ulMulticastAddress != 0) )
        {
            if( pThis->tStreamInfo[i].iVideoSrcIndex > 0 )
            {
				//20100714 Added by danny For Multicast parameters load dynamically
				pThis->tVideoSrcInfo[pThis->tStreamInfo[i].iVideoSrcIndex-1].iNumberOfWorkingSDP++;

				printf("[%s]Video %d iNumberOfWorkingSDP %d\r\n", __FUNCTION__, pThis->tStreamInfo[i].iVideoSrcIndex-1, 
						pThis->tVideoSrcInfo[pThis->tStreamInfo[i].iVideoSrcIndex-1].iNumberOfWorkingSDP);
				if( pThis->tVideoSrcInfo[pThis->tStreamInfo[i].iVideoSrcIndex-1].iNumberOfWorkingSDP == 1)
				{
					//20100728 Added by danny For multiple channels videoin/audioin
					iStreamIndex = StreamingServer_GetMultipleChannelStreamIndex((HANDLE)pThis, pThis->tStreamInfo[i].iVideoSrcIndex);
					
					//Modified and moved here by Louis to add stream ID 20081119
					acTemp[1] = snprintf(acTemp+2, sizeof(acTemp) - 2,"<control id=\"0\" stream=\"%d\">%s", iStreamIndex, CONTROL_MSG_START);
                	if( (iRes=write(pThis->tVideoSrcInfo[pThis->tStreamInfo[i].iVideoSrcIndex-1].iFdFIFO,acTemp,acTemp[1]+2)) < 0 )               
                	{
                    	printf("start video track %d for scalable multicast error %d!\n",i+1,errno);
                	}
                	else
                	{
                    	printf("start video track %d for scalable multicast OK!\n",i+1);    
                	}
				}
            }
            
			//20100714 Modified by danny For Multicast parameters load dynamically
			//if( pThis->tStreamInfo[i].iAudioSrcIndex >= 0 )
			if( pThis->tStreamInfo[i].iAudioSrcIndex > 0 )
            {
				//20100714 Added by danny For Multicast parameters load dynamically
				pThis->tAudioSrcInfo[pThis->tStreamInfo[i].iAudioSrcIndex-1].iNumberOfWorkingSDP++;
				//20130327 added by Jimmy to support different video/audio multicast addresses for ONVIF Test Tool 12.12
				if(pThis->tMulticastInfo[i + RTSP_MULTICASTNUMBER].iAlwaysMulticast)
				{
					pThis->tAudioSrcInfo[pThis->tStreamInfo[i].iAudioSrcIndex-1].iNumberOfWorkingSDP++;
				}

				printf("[%s]Audio %d iNumberOfWorkingSDP %d\r\n", __FUNCTION__, pThis->tStreamInfo[i].iAudioSrcIndex-1, 
						pThis->tAudioSrcInfo[pThis->tStreamInfo[i].iAudioSrcIndex-1].iNumberOfWorkingSDP);
				//20130327 modified by Jimmy to support different video/audio multicast addresses for ONVIF Test Tool 12.12
				if( pThis->tAudioSrcInfo[pThis->tStreamInfo[i].iAudioSrcIndex-1].iNumberOfWorkingSDP == (pThis->tMulticastInfo[i + RTSP_MULTICASTNUMBER].iAlwaysMulticast? 2:1))
				{
					//20100728 Added by danny For multiple channels videoin/audioin
					iStreamIndex = StreamingServer_GetMultipleChannelStreamIndex((HANDLE)pThis, pThis->tStreamInfo[i].iAudioSrcIndex);
					
					//Modified and moved here by Louis to add stream ID 20081119
					acTemp[1] = snprintf(acTemp+2, sizeof(acTemp) - 2,"<control id=\"0\" stream=\"%d\">%s", iStreamIndex, CONTROL_MSG_START);
                	if( (iRes=write(pThis->tAudioSrcInfo[pThis->tStreamInfo[i].iAudioSrcIndex-1].iFdFIFO,acTemp,acTemp[1]+2)) < 0 )               
                	{
                    	printf("start audio track %d for scalable multicast error %d! %d\n",i+1,errno,pThis->tAudioSrcInfo[i].iFdFIFO);
                	}
                	else
                	{
                    	printf("start audio track %d for scalable multicast OK!\n",i+1);    
                	}
				}
            }
			//20120801 added by Jimmy for metadata
			if( pThis->tStreamInfo[i].iMetadataSrcIndex > 0 )
            {
				//20100714 Added by danny For Multicast parameters load dynamically
				pThis->tMetadataSrcInfo[pThis->tStreamInfo[i].iMetadataSrcIndex-1].iNumberOfWorkingSDP++;

				printf("[%s]Metadata %d iNumberOfWorkingSDP %d\r\n", __FUNCTION__, pThis->tStreamInfo[i].iMetadataSrcIndex-1, 
						pThis->tMetadataSrcInfo[pThis->tStreamInfo[i].iMetadataSrcIndex-1].iNumberOfWorkingSDP);
				if( pThis->tMetadataSrcInfo[pThis->tStreamInfo[i].iMetadataSrcIndex-1].iNumberOfWorkingSDP == 1)
				{
					//20100728 Added by danny For multiple channels videoin/audioin
					iStreamIndex = StreamingServer_GetMultipleChannelStreamIndex((HANDLE)pThis, pThis->tStreamInfo[i].iMetadataSrcIndex);
					
					//Modified and moved here by Louis to add stream ID 20081119
					acTemp[1] = snprintf(acTemp+2, sizeof(acTemp) - 2,"<control id=\"0\" stream=\"%d\">%s", iStreamIndex, CONTROL_MSG_START);
                	if( (iRes=write(pThis->tMetadataSrcInfo[pThis->tStreamInfo[i].iMetadataSrcIndex-1].iFdFIFO,acTemp,acTemp[1]+2)) < 0 )               
                	{
                    	printf("start metadata track %d for scalable multicast error %d! %d\n",i+1,errno,pThis->tMetadataSrcInfo[i].iFdFIFO);
                	}
                	else
                	{
                    	printf("start metadata track %d for scalable multicast OK!\n",i+1);    
                	}
				}
            }

        }
    }
 #endif
 
    return S_OK;            
}

#ifdef _SHARED_MEM
//20100105 Added For Seamless Recording
SCODE StreamingServer_SetToConfigure(char* pzCmd)
{
	int fd;
	struct sockaddr_un sunx;
	char szbuff[256];

	/* Create the unix socket */
	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if( fd < 0 )	
	{	
		printf("[%s] Cannot create socket error=%d(%s)\n", __FUNCTION__, errno, strerror(errno));
		return S_FAIL;	
	}

	memset(&sunx, 0, sizeof(sunx));
	sunx.sun_family = AF_UNIX;
	(void) strncpy(sunx.sun_path, "/tmp/configer", sizeof(sunx.sun_path));
	if( connect(fd, (struct sockaddr *)&sunx, sizeof(sunx.sun_family) + strlen(sunx.sun_path)) < 0 )
	{
		printf("[%s] Connect socket failed!\n", __FUNCTION__);		
		close(fd);		
		fd = -1;		
		return S_FAIL;	
	}
	
	memset(szbuff, 0, sizeof(szbuff));	
	write(fd, pzCmd, strlen(pzCmd));
	
	while( read(fd, szbuff, sizeof(szbuff)-1) > 0 )	
	{		
		printf("[%s] Configure reply=%s\n", __FUNCTION__, szbuff);	
	}	
	close(fd);	
	fd = -1;
	
	return S_OK;
}

SCODE StreamingServer_SetRecoderState(char* pzCmd)
{
	TMessageUtilOption      tMsgUtilInfo;
	SCODE					scResult;
	
	memset(&tMsgUtilInfo, 0, sizeof(tMsgUtilInfo));
	tMsgUtilInfo.iControlType = 0;
	sprintf(tMsgUtilInfo.szSocketPath, "%s", EVENTMGR_SVR_SCK);
	tMsgUtilInfo.iBufLenOut = strlen(pzCmd);
	memcpy(tMsgUtilInfo.pcBuffer, pzCmd, tMsgUtilInfo.iBufLenOut);
	//printf("[%s] tMsgUtilInfo.pcBuffer=%s\n", __FUNCTION__, tMsgUtilInfo.pcBuffer);
	
	scResult = Message_Util_SendbySocket(&tMsgUtilInfo);
	
	if(scResult == S_OK)
	{
		printf("[%s] Set Recoder State OK!\n", __FUNCTION__);
		return S_OK;
	}
	else
	{
		printf("[%s] Set Recoder State failed!\n", __FUNCTION__);
		return S_FAIL;
	}
}
#endif

void StreamingServer_AccountManagerParse(HANDLE hObject)
{
	TSTREAMSERVERINFO *pThis;
		
	pThis = (TSTREAMSERVERINFO *)hObject;

	AM_ParsePasswordFile(&(pThis->tAMConf)) ;
	if(QueryCameraMode()) //20161212 factory default mode
    {
        pThis->bFactoryMode = TRUE;
    }
    else
    {
        pThis->bFactoryMode = FALSE;
    }
    //Modified 20080619 to correct names, 2008/01/18 for Web-Attraction Application
    char* pcWebAttraction = NULL;
    if((pcWebAttraction = XMLSParser_ReadContent("/etc/conf.d/config_streamserver.xml", "/root/network/rtsp/anonymousviewing")) != NULL)
    {
        if(strcmp("1", pcWebAttraction) == 0)
        {
            printf("Anonymouse viewing enabled!\n");
            pThis->bWebAttraction = TRUE;
        }
        else
        {
            printf("Anonymouse viewing disabled with content :%s:!\n", pcWebAttraction);
            pThis->bWebAttraction = FALSE;
        }
    }
    else
    {
        printf("Anonymouse viewing not defined!\n");
        pThis->bWebAttraction = FALSE;
    }
	return ;	
}

int StreamingServer_AccountManagerInit(TSTREAMSERVERINFO *ptStreamServerInfo)
{
	rtspstrcpy(ptStreamServerInfo->tAMConf.szConfFilePath, STR_DEFAULT_CONFIG, sizeof(ptStreamServerInfo->tAMConf.szConfFilePath));
	rtspstrcpy(ptStreamServerInfo->tAMConf.szPassFilePath, STR_DEFAULT_PASSWD, sizeof(ptStreamServerInfo->tAMConf.szPassFilePath));
	rtspstrcpy(ptStreamServerInfo->tAMConf.szGroupFilePath, STR_DEFAULT_GROUP, sizeof(ptStreamServerInfo->tAMConf.szGroupFilePath));
	rtspstrcpy(ptStreamServerInfo->tAMConf.szTempD, STR_DEFAULT_TEMPD, sizeof(ptStreamServerInfo->tAMConf.szTempD));
	rtspstrcpy(ptStreamServerInfo->tAMConf.szGenericGID, STR_DEFAULT_USERGID, sizeof(ptStreamServerInfo->tAMConf.szGenericGID));	

	if( AM_ParseConfigFile(&(ptStreamServerInfo->tAMConf)) )
	{
		printf("AM_ParseConfigFile failed\n");
		return 1;
	}
	else
	{
		return 0;
	}
	
}

//20101123 Added by danny For support advanced system log 
BOOL StreamingServer_IsAdvLogSupport()
{	
	FILE *fp;	
	char szSrch[256];	

	if( (fp = fopen(SYSLOG_CONF, "r")) == NULL )	
	{		
		return FALSE;	
	}		

	while( fgets(szSrch, sizeof(szSrch), fp) != NULL )	
	{ 		
		if( strstr(szSrch, ACCESS_LOG) != NULL ) 		
		{ 			
			return TRUE;	 		
		}	
	}		

	return FALSE;
}

SCODE StreamingServer_Initial(HANDLE *phObject, TRTSPSInitOptions *pInitOpts, char* pzConfigFile, char* pzAccessFile, char* pzQosFile)
{
	int i;
	TSTREAMSERVERINFO *pThis;
#ifdef _SHARED_MEM
	//20100728 Added by danny For multiple channels videoin/audioin
	int 	iBlockIndex, iSectorIndex;
	//20101210 Added by danny For Media shmem config
	//div_t	answer;
#ifdef _METADATA_ENABLE
	//20140819 added by Charles for eventparser API
	TEPOption	tEventOpt;
	memset(&tEventOpt, 0, sizeof(TEPOption));   
    tEventOpt.uiDtatFormat = DATA_FMT_ONVIFANALYTICS;
#ifdef _METADATA_EVENT_ENABLE
    tEventOpt.uiDtatFormat |= DATA_FMT_ONVIFNOTIFY;
#endif
#endif
#endif

	if (((pInitOpts->dwVersion&0x00FF00FF)!=(RTSPS_VERSION&0x00FF00FF)) |
		((pInitOpts->dwVersion&0x0000FF00)<(RTSPS_VERSION&0x0000FF00)))
    {
        return ERR_INVALID_VERSION;
    }

	*phObject = &tStreamServerInfo;
	memset((void*)&tStreamServerInfo, 0, sizeof(TSTREAMSERVERINFO));
	pThis = (TSTREAMSERVERINFO *) &tStreamServerInfo;

	//20090305 Multiple Stream
	SetupTrackName();

	//20100728 Modified by danny For multiple channels videoin/audioin
	pThis->dwChannelNumber = MULTIPLE_CHANNEL_NUM;
	pThis->dwPerChannelStreamNumber = LIVE_STREAM_NUM / MULTIPLE_CHANNEL_NUM;
	if (pThis->dwPerChannelStreamNumber <= 0)
	{
		printf("pThis->dwPerChannelStreamNumber <= 0\n");
		return S_FAIL;
	}
	printf("[%s]ChannelNumber %d, PerChannelStreamNumber %d\n", __FUNCTION__, pThis->dwChannelNumber, pThis->dwPerChannelStreamNumber);
#ifdef _SHARED_MEM
	if (pThis->dwChannelNumber == 1)
	{
		if (SharedMemBlock_Attach(&pThis->hSharedMemVideoBlock[0], SHM_RDONLY, SHAREDMEM_VENCOUT_BLOCK, esmatDiffProcess) != S_OK)
		{
			return S_FAIL;
		}

		if (SharedMemBlock_Attach(&pThis->hSharedMemAudioBlock[0], SHM_RDONLY, SHAREDMEM_AENCOUT_BLOCK, esmatDiffProcess) != S_OK)
		{
			return S_FAIL;
		}
		//20120731 added by Jimmy for metadata
		//20140819 modified by Charles for eventparser API
/*#ifdef _METADATA_ENABLE
		if (SharedMemBlock_Attach(&pThis->hSharedMemMetadataBlock[0], SHM_RDONLY, SHAREDMEM_METADATA_BLOCK, esmatDiffProcess) != S_OK)
		{
			return S_FAIL;
		}
#endif*/
	}
	else
	{
		for( i=0; i < pThis->dwChannelNumber; i++ )
		{
			if (SharedMemBlock_Attach(&pThis->hSharedMemVideoBlock[i], SHM_RDONLY, SHAREDMEM_VENCOUT_BLOCK_C0 + (i*SHAREDMEM_NEXT_CH_OFFSET), esmatDiffProcess) != S_OK)
			{
				return S_FAIL;
			}

			if (SharedMemBlock_Attach(&pThis->hSharedMemAudioBlock[i], SHM_RDONLY, SHAREDMEM_AENCOUT_BLOCK_C0 + (i*SHAREDMEM_NEXT_CH_OFFSET), esmatDiffProcess) != S_OK)
			{
				return S_FAIL;
			}
			//20120731 added by Jimmy for metadata
			//20140819 modified by Charles for eventparser API
/*#ifdef _METADATA_ENABLE
			if (SharedMemBlock_Attach(&pThis->hSharedMemMetadataBlock[i], SHM_RDONLY, SHAREDMEM_METADATA_BLOCK_C0 + (i*SHAREDMEM_NEXT_CH_OFFSET), esmatDiffProcess) != S_OK)
			{
				return S_FAIL;
			}
#endif*/
		}
	}
	
	//20100428 Added For Media on demand
	if (MOD_STREAM_NUM > 0)
	{
		if (SharedMemBlock_Attach(&pThis->hSharedMemMODBlock, SHM_RDONLY, SHAREDMEM_MOD_BLOCK, esmatDiffProcess) != S_OK)
		{
			return S_FAIL;
		}
	}
#endif
	
	for(i=0;i<pInitOpts->dwStreamNumber;i++)
	{
	    pThis->tStreamInfo[i].iVideoSrcIndex = -1;
	    pThis->tStreamInfo[i].iAudioSrcIndex = -1;
		//20120731 added by Jimmy for metadata
		pThis->tStreamInfo[i].iMetadataSrcIndex = -1;
	}
	
	pThis->iGotVideoFlag = FALSE;
    pThis->iGotAudioFlag = FALSE;
	//20120731 added by Jimmy for metadata
	pThis->iGotMetadataFlag = FALSE;

#ifdef _SHARED_MEM    
	//20101210 Added by danny For Media shmem config
	if (pThis->dwChannelNumber > 1)
	{
		if( StreamingServer_ParseShmemConfig(pThis, pzConfigFile) != S_OK)
    	{
    		printf("shmem config file %s parse failed\n", pzConfigFile);
        	return S_FAIL;
    	}
	}
#endif

	for( i=0; i<VIDEO_TRACK_NUMBER ; i++)
	//for( i=0; i<2 ; i++)
	{
	    pThis->tVideoSrcInfo[i].iFdSock = -1;
	    pThis->tVideoSrcInfo[i].iFdFIFO = -1;	    
#ifdef _SHARED_MEM
		//20100428 Added For Media on demand
		if( i < LIVE_STREAM_NUM )
		{
			//20100728 Modified by danny For multiple channels videoin/audioin
			if( pThis->dwChannelNumber == 1 )
			{
				//20130605 modified by Jimmy to support metadata event
				if (SharedMem_Initial(pThis->hSharedMemVideoBlock[0], &pThis->tVideoSrcInfo[i].ahSharedMem[0], i , SHAREDMEM_ATTACHID_RTSP) != S_OK)
				{
					return S_FAIL;
				}
			}
			else
			{
				if (pThis->dwPerChannelStreamNumber <= 0)
				{
					printf("pThis->dwPerChannelStreamNumber <= 0\n");
					return S_FAIL;
				}
				//20101210 Added by danny For Media shmem config
				/*answer = div(i, pThis->dwPerChannelStreamNumber);
				iBlockIndex = answer.quot;
				iSectorIndex = answer.rem;*/
				iBlockIndex = pThis->tVideoSrcInfo[i].iBlockIndex;
				iSectorIndex = pThis->tVideoSrcInfo[i].iSectorIndex;
				//20130605 modified by Jimmy to support metadata event
				if (SharedMem_Initial(pThis->hSharedMemVideoBlock[iBlockIndex], &pThis->tVideoSrcInfo[i].ahSharedMem[0], iSectorIndex , SHAREDMEM_ATTACHID_RTSP) != S_OK)
				{
					return S_FAIL;
				}
				//printf("Initial VideoBlock %d Sector %d hSharedMem %p\n", iBlockIndex, iSectorIndex, pThis->tVideoSrcInfo[i].hSharedMem);
			}
		}
		else
		{
			//20120731 modified by Jimmy for metadata
			//20130605 modified by Jimmy to support metadata event
			//Use 2 because MOD not support metadata
			if (SharedMem_Initial(pThis->hSharedMemMODBlock, &pThis->tVideoSrcInfo[i].ahSharedMem[0], (i-LIVE_STREAM_NUM)*2, SHAREDMEM_ATTACHID_RTSP) != S_OK)
			{
				return S_FAIL;
			}
		}
#endif
	}
	
	for( i=0; i<AUDIO_TRACK_NUMBER ; i++)
	{
	    pThis->tAudioSrcInfo[i].iFdSock = -1;
	    pThis->tAudioSrcInfo[i].iFdFIFO = -1;	
#ifdef _SHARED_MEM
		//20100428 Added For Media on demand
		if( i < LIVE_AUDIO_STREAM_NUM )
		{
			//20100728 Modified by danny For multiple channels videoin/audioin
			if( pThis->dwChannelNumber == 1 )
			{
				//20130605 modified by Jimmy to support metadata event
				if (SharedMem_Initial(pThis->hSharedMemAudioBlock[0], &pThis->tAudioSrcInfo[i].ahSharedMem[0], i , SHAREDMEM_ATTACHID_RTSP) != S_OK)
				{
					return S_FAIL;
				}
			}
			else
			{
				if (pThis->dwPerChannelStreamNumber <= 0)
				{
					printf("pThis->dwPerChannelStreamNumber <= 0\n");
					return S_FAIL;
				}
				//20101210 Added by danny For Media shmem config
				/*answer = div(i, pThis->dwPerChannelStreamNumber);
				//Modify for VS4CH EMI workaround
				iBlockIndex = answer.quot;
				iSectorIndex = answer.rem;
				if( pThis->dwChannelNumber == 4 )
				{
					iBlockIndex = 0;
					iSectorIndex = 0;
				}
				else
				{
					iBlockIndex = answer.quot;
					iSectorIndex = answer.rem;
				}*/
				iBlockIndex = pThis->tAudioSrcInfo[i].iBlockIndex;
				iSectorIndex = pThis->tAudioSrcInfo[i].iSectorIndex;
				//20130605 modified by Jimmy to support metadata event
				if (SharedMem_Initial(pThis->hSharedMemAudioBlock[iBlockIndex], &pThis->tAudioSrcInfo[i].ahSharedMem[0], iSectorIndex , SHAREDMEM_ATTACHID_RTSP) != S_OK)
				{printf("Initial AudioBlock %d failed Sector %d\n", iBlockIndex, iSectorIndex);
					return S_FAIL;
				}
				//printf("Initial AudioBlock %d Sector %d hSharedMem %p\n", iBlockIndex, iSectorIndex, pThis->tAudioSrcInfo[i].hSharedMem);
			}
		}
		else
		{
			//20120731 modified by Jimmy for metadata
			//20130605 modified by Jimmy to support metadata event
			//Use 2 because MOD not support metadata
			if (SharedMem_Initial(pThis->hSharedMemMODBlock, &pThis->tAudioSrcInfo[i].ahSharedMem[0], (i-LIVE_AUDIO_STREAM_NUM)*2+1, SHAREDMEM_ATTACHID_RTSP) != S_OK)
			{
				return S_FAIL;
			}
		}
#endif
	}

	//20120731 added by Jimmy for metadata
	//20140819 modified by Charles for eventparser API
#ifdef _METADATA_ENABLE
	for( i=0; i < METADATA_TRACK_NUMBER; i++)
	{
		pThis->tMetadataSrcInfo[i].iFdSock = -1;
		pThis->tMetadataSrcInfo[i].iFdFIFO = -1;
#ifdef _SHARED_MEM
		//20100428 Added For Media on demand
		if( i < LIVE_METADATA_STREAM_NUM )
		{
			//20100728 Modified by danny For multiple channels videoin/audioin
			if( pThis->dwChannelNumber == 1 )
			{
			    /*
				if (SharedMem_Initial(pThis->hSharedMemMetadataBlock[0], &pThis->tMetadataSrcInfo[i].hSharedMem, i , SHAREDMEM_ATTACHID_RTSP) != S_OK)
				{
					return S_FAIL;
				}
				*/
				//20130605 modified by Jimmy to support metadata event
				/*if (SharedMem_Initial(pThis->hSharedMemMetadataBlock[0], &pThis->tMetadataSrcInfo[i].ahSharedMem[0], i*2 , SHAREDMEM_ATTACHID_RTSP) != S_OK)
				{
					return S_FAIL;
				}
#ifdef _METADATA_EVENT_ENABLE
				if (SharedMem_Initial(pThis->hSharedMemMetadataBlock[0], &pThis->tMetadataSrcInfo[i].ahSharedMem[1], i*2 + 1 , SHAREDMEM_ATTACHID_RTSP) != S_OK)
				{
					return S_FAIL;
				}
#endif*/
                if (EventParser_Initial(&pThis->tMetadataSrcInfo[i].ahSharedMem[0], tEventOpt) != S_OK)
                {
                    return S_FAIL;
                }
			}
			else
			{
				if (pThis->dwPerChannelStreamNumber <= 0)
				{
					printf("pThis->dwPerChannelStreamNumber <= 0\n");
					return S_FAIL;
				}
				iBlockIndex = pThis->tMetadataSrcInfo[i].iBlockIndex;
				iSectorIndex = pThis->tMetadataSrcInfo[i].iSectorIndex;
                /*
				if (SharedMem_Initial(pThis->hSharedMemMetadataBlock[iBlockIndex], &pThis->tMetadataSrcInfo[i].ahSharedMem, iSectorIndex , SHAREDMEM_ATTACHID_RTSP) != S_OK)
				{printf("Initial MetadataBlock %d failed Sector %d\n", iBlockIndex, iSectorIndex);
					return S_FAIL;
				}
				*/

				//20130605 modified by Jimmy to support metadata event
				/*if (SharedMem_Initial(pThis->hSharedMemMetadataBlock[iBlockIndex], &pThis->tMetadataSrcInfo[i].ahSharedMem[0], iSectorIndex*2 , SHAREDMEM_ATTACHID_RTSP) != S_OK)
				{printf("Initial MetadataBlock %d failed Sector %d\n", iBlockIndex, iSectorIndex*2);
					return S_FAIL;
				}
#ifdef _METADATA_EVENT_ENABLE
				if (SharedMem_Initial(pThis->hSharedMemMetadataBlock[iBlockIndex], &pThis->tMetadataSrcInfo[i].ahSharedMem[1], iSectorIndex*2 + 1, SHAREDMEM_ATTACHID_RTSP) != S_OK)
				{printf("Initial MetadataBlock %d failed Sector %d\n", iBlockIndex, iSectorIndex*2 + 1);
					return S_FAIL;
				}
#endif*/

			}
		}
		else
		{
		    /*
			if (SharedMem_Initial(pThis->hSharedMemMODBlock, &pThis->tMetadataSrcInfo[i].ahSharedMem, (i-LIVE_METADATA_STREAM_NUM)*MEDIA_TYPE_NUMBER+2, SHAREDMEM_ATTACHID_RTSP) != S_OK)
			{
				return S_FAIL;
			}
			*/
			return S_FAIL;
		}
#endif
	}
#endif
    int iStreamCount = 0;
    for(iStreamCount = 0; iStreamCount < VIDEO_TRACK_NUMBER; iStreamCount++)
    {
        OSCriticalSection_Initial(&pThis->tVideoSrcInfo[iStreamCount].hMediaSrcMutex);
    }

    for(iStreamCount = 0; iStreamCount < AUDIO_TRACK_NUMBER; iStreamCount++)
    {
        OSCriticalSection_Initial(&pThis->tAudioSrcInfo[iStreamCount].hMediaSrcMutex);
    }

    for(iStreamCount = 0; iStreamCount < METADATA_TRACK_NUMBER; iStreamCount++)
    {
        OSCriticalSection_Initial(&pThis->tMetadataSrcInfo[iStreamCount].hMediaSrcMutex);
    }

	//CID:813, CHECKER:SECURE_CODING
	strncpy(pThis->szIPAddr, pInitOpts->szIPAddr, sizeof(pThis->szIPAddr) - 1);
	pThis->szIPAddr[sizeof(pThis->szIPAddr) - 1] = 0;
	strncpy(pThis->szSubnetMask, pInitOpts->szSubnetMask, sizeof(pThis->szSubnetMask) - 1);
	pThis->szSubnetMask[sizeof(pThis->szSubnetMask) - 1] = 0;
	/* get stream number */
	pThis->dwStreamNumber = pInitOpts->dwStreamNumber;
	
	/* create communication socket to video and audio stream */
	mkdir(VSTREAM_SOCK_DIR, 0777);
	mkdir(ASTREAM_SOCK_DIR, 0777);

    if( StreamingServer_AccountManagerInit(pThis) != 0 )
    {       
        return S_FAIL;
    }
    //20170116 Modified by Faber, stead of parsing anaoymouseviewing
    StreamingServer_AccountManagerParse((HANDLE)pThis);

    if( StreamingServer_ParseConfigFile(pThis, pzConfigFile) != S_OK)
    {
    	printf("config file %s parse failed\n", pzConfigFile);
        return S_FAIL;
    }
	
	if( StreamingServer_ParseAccessFile(pThis, pzAccessFile) != S_OK)
	{
		printf("access list parse failed\n");
        return S_FAIL;
    }
       
	strncpy(pThis->acQosFilePath, pzQosFile, sizeof(pThis->acQosFilePath) - 1);
	pThis->acQosFilePath[sizeof(pThis->acQosFilePath) -1] = 0;


		
	for( i=0; i<VIDEO_TRACK_NUMBER ; i++)
	{
   		rtspstrcpy(pThis->tVideoSrcInfo[i].acTrackName,acVideoTrackName[i], sizeof(pThis->tVideoSrcInfo[i].acTrackName));
	    if((pThis->tVideoSrcInfo[i].iMediaIndex = StreamingServer_ParseTrackID(acVideoTrackName[i])) < 0 )
		{
		    printf("pasring video track number failed\r\n");
		    return S_FAIL;
		}
		if( pThis->tVideoSrcInfo[i].iFdSock > 0 )
		    pThis->iGotVideoFlag = TRUE;
	}
	
	for( i=0; i<AUDIO_TRACK_NUMBER ; i++)
	{
   		rtspstrcpy(pThis->tAudioSrcInfo[i].acTrackName,acAudioTrackName[i], sizeof(pThis->tAudioSrcInfo[i].acTrackName));				 				
	    if((pThis->tAudioSrcInfo[i].iMediaIndex = StreamingServer_ParseTrackID(acAudioTrackName[i])) < 0 )
		{
   		    printf("pasring audio track number failed\r\n");
		    return S_FAIL;
		} 
		if( pThis->tAudioSrcInfo[i].iFdSock > 0 )
		    pThis->iGotAudioFlag = TRUE;
	}	

	//20120731 added by Jimmy for metadata
	for( i=0; i<METADATA_TRACK_NUMBER ; i++)
	{
   		rtspstrcpy(pThis->tMetadataSrcInfo[i].acTrackName,acMetadataTrackName[i], sizeof(pThis->tMetadataSrcInfo[i].acTrackName));				 				
	    if((pThis->tMetadataSrcInfo[i].iMediaIndex = StreamingServer_ParseTrackID(acMetadataTrackName[i])) < 0 )
		{
   		    printf("pasring metadata track number failed\r\n");
		    return S_FAIL;
		}
		if( pThis->tMetadataSrcInfo[i].iFdSock > 0 )
		    pThis->iGotMetadataFlag = TRUE;
	}

	/* initial vide/audi bitstream buffer */
#ifndef _SHARED_MEM
	pThis->pVideBitstreamBuf = (TBitstreamBuffer *)malloc(sizeof(TBitstreamBuffer) + MAX_BITSTREAM_SIZE + (MAX_MP4V_PACKET_NUM+1) * sizeof(DWORD));
	pThis->pVideBitstreamBuf->dwBufSize = MAX_BITSTREAM_SIZE;
	pThis->pVideBitstreamBuf->pdwPacketSize = (DWORD*) ((BYTE*) pThis->pVideBitstreamBuf + sizeof(TBitstreamBuffer));
	pThis->pVideBitstreamBuf->pbyBuffer = (BYTE*) pThis->pVideBitstreamBuf + sizeof(TBitstreamBuffer) + (MAX_MP4V_PACKET_NUM + 1) * sizeof(DWORD);

	pThis->pAudiBitstreamBuf = (TBitstreamBuffer *)malloc(sizeof(TBitstreamBuffer) + MAX_BITSTREAM_SIZE);
	pThis->pAudiBitstreamBuf->dwBufSize = MAX_BITSTREAM_SIZE;
	pThis->pAudiBitstreamBuf->pdwPacketSize = (DWORD*) ((BYTE*) pThis->pAudiBitstreamBuf + sizeof(TBitstreamBuffer));
	pThis->pAudiBitstreamBuf->pbyBuffer = (BYTE*) pThis->pAudiBitstreamBuf + sizeof(TBitstreamBuffer) + (MAX_AUDIO_PACKET_NUM + 1) * sizeof(DWORD);

	//20120731 added by Jimmy for metadata
	pThis->pMetadataBitstreamBuf = (TBitstreamBuffer *)malloc(sizeof(TBitstreamBuffer) + MAX_BITSTREAM_SIZE);
	pThis->pMetadataBitstreamBuf->dwBufSize = MAX_BITSTREAM_SIZE;
	pThis->pMetadataBitstreamBuf->pdwPacketSize = (DWORD*) ((BYTE*) pThis->pMetadataBitstreamBuf + sizeof(TBitstreamBuffer));
	pThis->pMetadataBitstreamBuf->pbyBuffer = (BYTE*) pThis->pMetadataBitstreamBuf + sizeof(TBitstreamBuffer) + (MAX_METADATA_PACKET_NUM + 1) * sizeof(DWORD);

    if ((pThis->pbyVideUBuffer = (BYTE *)calloc(1, VIDEO_UBUFFER_SIZE)) == NULL)
	{
		printf("Allocate video ubuffer fail\n");
		return ERR_OUT_OF_MEMORY;
	}
	
	if ((pThis->pbyAudiUBuffer = (BYTE *)calloc(1, AUDIO_UBUFFER_SIZE)) == NULL)
	{
		printf("Allocate ubuffer fail\n");
		return ERR_OUT_OF_MEMORY;
	}

	//20120731 added by Jimmy for metadata
	if ((pThis->pbyMetadataUBuffer = (BYTE *)calloc(1, METADATA_UBUFFER_SIZE)) == NULL)
	{
		printf("Allocate ubuffer fail\n");
		return ERR_OUT_OF_MEMORY;
	}

#else
	if ((pThis->pbyVideUBuffer = (BYTE *)calloc(1, RTSPS_VIDEO_UBUFFER_HEADERSIZE)) == NULL)
	{
		printf("Allocate video ubuffer fail\n");
		return ERR_OUT_OF_MEMORY;
	}
	
	if ((pThis->pbyAudiUBuffer = (BYTE *)calloc(1, RTSPS_AUDIO_UBUFFER_HEADERSIZE)) == NULL)
	{
		printf("Allocate ubuffer fail\n");
		return ERR_OUT_OF_MEMORY;
	}

	//20120731 added by Jimmy for metadata
	if ((pThis->pbyMetadataUBuffer = (BYTE *)calloc(1, RTSPS_METADATA_UBUFFER_HEADERSIZE)) == NULL)
	{
		printf("Allocate ubuffer fail\n");
		return ERR_OUT_OF_MEMORY;
	}

#endif
	if( (pThis->pbyAudioUploadBuff= (BYTE *)calloc(1, AUDIO_UBUFFER_SIZE)) == NULL)
	{
		printf("Allocate audio upload ubuffer fail\n");
		return ERR_OUT_OF_MEMORY;
	}

	//20080619 Initialize XML parser
	pThis->pXMLHandle = XML_ParserCreate(NULL);
	if (!pThis->pXMLHandle) 
	{
    	printf("Couldn't allocate memory for XML parser\n");
    	return ERR_OUT_OF_MEMORY;
  	}

	//20101123 Added by danny For support advanced system log 
	pThis->bAdvLogSupport = StreamingServer_IsAdvLogSupport();
	
	if (SetUpRTSPServer(pThis) != S_OK)
	{
		printf("RTSPServer setup failed\n");		
		return S_FAIL;
	}

	//20110401 Added by danny For support RTSPServer thread watchdog
	OSTime_GetTimer(&pThis->dwRTSPServerLastKick, NULL);
	
	return S_OK;
}

SCODE StreamingServer_Start(HANDLE hObject)
{
	TSTREAMSERVERINFO		*pThis;
	//TOSThreadInitOptions	tOSThreadInitOpt;

	pThis = (TSTREAMSERVERINFO *) hObject;

	pThis->bRunning = TRUE;
	if (RTSPStreaming_Start(pThis->hRTSPServer) != S_OK)
	{
		printf("start rtsp streaming fail\n");
		return S_FAIL;
	}
	/* open ipc socket */
	RTSPSSetupFdIPCSocket(hObject);
    
	/* setup rtsp <---> http socket communication process (for rtp over http socket exchange) */
	/* 20080829 eliminate fdipc thread
	memset(&tOSThreadInitOpt, 0, sizeof(TOSThreadInitOptions));
    tOSThreadInitOpt.dwStackSize = UBUFFER_PROCESS_STACKSIZE;
    tOSThreadInitOpt.dwInstance  = (DWORD)(pThis);
    tOSThreadInitOpt.dwPriority  = UBUFFER_PROCESS_PRIORITY;
    tOSThreadInitOpt.pThreadProc = RTPOverHttpSocketExchanger;
    tOSThreadInitOpt.dwFlags = T_TSLICE;
    if(OSThread_Initial(&pThis->hSocketExchangerThread, &tOSThreadInitOpt) != S_OK)
    {
        return S_FAIL;
    }
    if(OSThread_Start(pThis->hSocketExchangerThread) != S_OK)
    {
    	return S_FAIL;
    }
	*/
	/******************************************************************************************/
	/* 20090225 QOS */
	StreamingServer_UpdateQosParameters(hObject, pThis->acQosFilePath);

#ifdef _SHARED_MEM
	/* 20100105 Added For Seamless Recording */
	StreamingServer_UpdateSeamlessRecordingParameters(hObject);
#endif

	return S_OK;
}

SCODE StreamingServer_Stop(HANDLE hObject)
{
	//DWORD dwRet;
	TSTREAMSERVERINFO *pThis;

	pThis = (TSTREAMSERVERINFO *) hObject;
	pThis->bRunning = FALSE;

    /*if(OSThread_WaitFor(pThis->hVUBufReaderThread, 300, &dwRet) != S_OK)
	{
		printf("wait for osthread termination fail\n");
		return S_FAIL;
	}
    if(OSThread_WaitFor(pThis->hAUBufReaderThread, 300, &dwRet) != S_OK)
	{
		printf("wait for osthread termination fail\n");
		return S_FAIL;
	}*/

	/*if(OSThread_WaitFor(pThis->hSocketExchangerThread, 300, &dwRet) != S_OK)
	{
		printf("wait for osthread termination fail\n");
		return S_FAIL;
	}*/
    /* Stop the RTSP streaimng server task */
	//printf("try stop\n");
    RTSPStreaming_Stop(pThis->hRTSPServer);
   	//printf("before fdipc thread\n");
	//OSThread_Terminate(pThis->hSocketExchangerThread);
	//printf("after fdipc thread\n");

	//printf("try ok\n");

	return S_OK;
}

SCODE StreamingServer_Release(HANDLE *phObject)
{
	int i;
	TSTREAMSERVERINFO *pThis;

	pThis = (TSTREAMSERVERINFO *) *phObject;
	/* release thread */
	/*if (OSThread_Release(&pThis->hVUBufReaderThread) != S_OK)
	{
		printf("release osthread fail\n");
		return S_FAIL;
	}
	if (OSThread_Release(&pThis->hAUBufReaderThread) != S_OK)
	{
		printf("release osthread fail\n");
		return S_FAIL;
	}
	if (OSThread_Release(&pThis->hSocketExchangerThread) != S_OK)
	{
		printf("release osthread fail\n");
		return S_FAIL;
	}*/
	/* close rtsp streaming */
	RTSPStreaming_Close(&pThis->hRTSPServer);
	
	/* free memory */
	free(pThis->pbyVideUBuffer);
	free(pThis->pbyAudiUBuffer);
	//20120801 added by Jimmy for metadata
	free(pThis->pbyMetadataUBuffer);
#ifndef _SHARED_MEM
	free(pThis->pVideBitstreamBuf);
	free(pThis->pAudiBitstreamBuf);
	//20120801 added by Jimmy for metadata
	free(pThis->pMetadataBitstreamBuf);
#endif
	free(pThis->pbyAudioUploadBuff);

	/* destroy semaphore */
//	sem_destroy(&pThis->semWriteVideData);
//	sem_destroy(&pThis->semVideDataReady);
//	sem_destroy(&pThis->semWriteAudiData);
//	sem_destroy(&pThis->semAudiDataReady);

	for (i=0; i<VIDEO_TRACK_NUMBER; i++)
	{
	    if( pThis->tVideoSrcInfo[i].iFdFIFO > 0 )
		    close(pThis->tVideoSrcInfo[i].iFdFIFO);

        if( pThis->tVideoSrcInfo[i].iFdSock > 0 )
            close(pThis->tVideoSrcInfo[i].iFdSock);                       		    

#ifdef _SHARED_MEM
		//20130605 modified by Jimmy to support metadata event
		SharedMem_Release(&pThis->tVideoSrcInfo[i].ahSharedMem[0]);
#endif

	}
	
	for (i=0; i<AUDIO_TRACK_NUMBER; i++)
    {
        if( pThis->tAudioSrcInfo[i].iFdSock > 0 )
		    close(pThis->tAudioSrcInfo[i].iFdSock);
				
        if( pThis->tAudioSrcInfo[i].iFdFIFO > 0 )		    
		    close(pThis->tAudioSrcInfo[i].iFdFIFO);
#ifdef _SHARED_MEM
		//20130605 modified by Jimmy to support metadata event
		SharedMem_Release(&pThis->tAudioSrcInfo[i].ahSharedMem[0]);
#endif
	}

	//20120801 added by Jimmy for metadata
	for (i=0; i<METADATA_TRACK_NUMBER; i++)
    {
        if( pThis->tMetadataSrcInfo[i].iFdSock > 0 )
		    close(pThis->tMetadataSrcInfo[i].iFdSock);
				
        if( pThis->tMetadataSrcInfo[i].iFdFIFO > 0 )		    
		    close(pThis->tMetadataSrcInfo[i].iFdFIFO);
#ifdef _SHARED_MEM
		//20130605 modified by Jimmy to support metadata event
		//20140819 modified by Charles for eventparser API
		/*SharedMem_Release(&pThis->tMetadataSrcInfo[i].ahSharedMem[0]);
#ifdef _METADATA_EVENT_ENABLE
		SharedMem_Release(&pThis->tMetadataSrcInfo[i].ahSharedMem[1]);
#endif   */
        EventParser_Release(&pThis->tMetadataSrcInfo[i].ahSharedMem[0]);
#endif
	}

#ifdef _SHARED_MEM
	//20100728 Modified by danny For multiple channels videoin/audioin
	for( i=0; i < pThis->dwChannelNumber; i++ )
	{
		SharedMemBlock_Detach(&pThis->hSharedMemVideoBlock[i]);
		SharedMemBlock_Detach(&pThis->hSharedMemAudioBlock[i]);
		//20120801 added by Jimmy for metadata
		//20140819 modified by Charles for eventparser API
		//SharedMemBlock_Detach(&pThis->hSharedMemMetadataBlock[i]);
	}
	
	//20100428 Added For Media on demand
	SharedMemBlock_Detach(&pThis->hSharedMemMODBlock);
#endif

	if( pThis->tAudioDstInfo.iFdSock > 0 )
		close(pThis->tAudioDstInfo.iFdSock);
		

	remove(pThis->tRTSPInfo.szSessionInfoPath); 
	//free (*phObject);
	*phObject = NULL;

	return S_OK;
}

/* 20090224 Update QOS parameters */
int	StreamingServer_UpdateQosParameters(HANDLE hRTSPS, char* pzQosFile)
{
	TSTREAMSERVERINFO *pThis = (TSTREAMSERVERINFO *)hRTSPS;

	if( StreamingServer_ParseQosFile(pThis, pzQosFile) != S_OK)
	{
		printf("Qos File parse failed\n");
        return S_FAIL;
    }

	if(RTSPStreaming_SetQosParameters(pThis->hRTSPServer, &tQosInfo) != S_OK)
	{
		printf("QOS commit failed!\n");
		return S_FAIL;
	}

	return S_OK;
}

#ifdef _SHARED_MEM
/* 20100105 Added For Seamless Recording */
int	StreamingServer_UpdateSeamlessRecordingParameters(HANDLE hRTSPS)
{
	TSTREAMSERVERINFO *pThis = (TSTREAMSERVERINFO *)hRTSPS;

	if( StreamingServer_ParseSeamlessRecordingFile(pThis) != S_OK)
	{
		printf("[%s] Seamless Recording File parse failed!\n", __FUNCTION__);
        return S_FAIL;
    }

	if(RTSPStreaming_SetSeamlessRecordingParameters(pThis->hRTSPServer, &tSeamlessRecordingInfo) != S_OK)
	{
		printf("[%s] Seamless Recording commit failed!\n", __FUNCTION__);
		return S_FAIL;
	}

	return S_OK;
}
#endif

#ifdef RTSPRTP_MULTICAST
//20100714 Added by danny For Multicast parameters load dynamically
int StreamingServer_UpdateMulticastParameters(HANDLE hRTSPS, char* pzConfigFile)
{
	TSTREAMSERVERINFO					*pThis = (TSTREAMSERVERINFO *)hRTSPS;
	int 								iMulticastCount, iTemp, iSDPSize=0, iStreamIndex;
	char        						pcSDPBuffer[RTSPSTREAMING_SDP_MAXSIZE];
	
	if( StreamingServer_ParseMulticastInfoConfig(pThis, pzConfigFile) != S_OK )
	{
		return S_FAIL;
	}
	//20130327 added by Jimmy to support different video/audio multicast addresses for ONVIF Test Tool 12.12
//	if( StreamingServer_ParseAudioExtraMulticastInfoConfig(pThis, pzConfigFile) != S_OK )
//	{
//		return S_FAIL;
//	}
    printf("Reload multicast info for config file %s OK!!\r\n", pzConfigFile);
	//20130327 modified by Jimmy to support different video/audio multicast addresses for ONVIF Test Tool 12.12
	//20160127 modified by Faber, rm audioextra
	for( iMulticastCount = 0; iMulticastCount < RTSP_MULTICASTNUMBER ; iMulticastCount++ )
	{
		iStreamIndex = iMulticastCount;
		//pThis->tMulticastInfo[iMulticastCount].iEnable = pThis->tMulticastInfo[iMulticastCount].iAlwaysMulticast;

		if( RTSPStreaming_CheckIfMulticastParametersChanged(pThis->hRTSPServer, iMulticastCount, &pThis->tMulticastInfo[iMulticastCount]) == S_OK)
		{
			//Set iAlwaysMulticast FALSE temporarily for Remove Specific Stream Multicast Clients sucessfully
			iTemp = pThis->tMulticastInfo[iMulticastCount].iAlwaysMulticast;
			pThis->tMulticastInfo[iMulticastCount].iAlwaysMulticast = FALSE;
			pThis->tMulticastInfo[iMulticastCount].iSDPIndex = iStreamIndex+1;
			//20130315 added by Jimmy to log more information
			syslog(LOG_DEBUG, "[StreamingServer_UpdateMulticastParameters]: Remove Specific Stream Multicast Clients, iSDPindex = %d\n", pThis->tMulticastInfo[iMulticastCount].iSDPIndex);
			RTSPStreaming_RemoveSpecificStreamMulticastClients(pThis->hRTSPServer, iMulticastCount + 1);
			pThis->tMulticastInfo[iMulticastCount].iAlwaysMulticast = iTemp;

			RTSPStreaming_SetMulticastParameters(pThis->hRTSPServer, iMulticastCount, &pThis->tMulticastInfo[iMulticastCount]);

			if(iStreamIndex == iMulticastCount)
			{
				//20120925 modified by Jimmy for ONVIF backchannel
				//20111124 modify by danny support Backchannel Multicast for ONVIF test tool 1.02.4
				iSDPSize = RTSPStreaming_ComposeAVSDP(pThis->hRTSPServer, iMulticastCount+1 , (unsigned long)NULL , FALSE, pThis->tMulticastInfo[iMulticastCount].iAlwaysMulticast, pcSDPBuffer, RTSPSTREAMING_SDP_MAXSIZE, FALSE, REQUIRE_NONE);
				//printf("\r\n%s\r\n",pcSDPBuffer);
				if( pThis->tStreamInfo[iStreamIndex].iEnable == TRUE )
				{
					StreamSvrWriteFile(pThis->tStreamInfo[iStreamIndex].szSDPFullPathName, pcSDPBuffer, iSDPSize);
				}
				else
				{
					remove(pThis->tStreamInfo[iStreamIndex].szSDPFullPathName);
				}

			}

			if( (pThis->tStreamInfo[iStreamIndex].iEnable == TRUE) &&
            	(pThis->tMulticastInfo[iMulticastCount].iAlwaysMulticast == TRUE) &&
            	(pThis->tMulticastInfo[iMulticastCount].iEnable!= 0) &&
            	(pThis->tMulticastInfo[iMulticastCount].ulMulticastAddress != 0) )
        	{
        		RTSPStreaming_StartAlwaysMulticast(pThis->hRTSPServer, iMulticastCount + 1);
			}
		}
	}
	
	return S_OK;
}
#endif

int	StreamingServer_UpdateDynamicPamater(HANDLE hRTSPS, char* pzConfigFile)
{
	TSTREAMSERVERINFO					*pThis = (TSTREAMSERVERINFO *)hRTSPS;
	char								*pcParam = NULL;
	TRTSPSTREAMING_DYNAMIC_PARAM		tDynaParam; 
	int									iFlag, i= 0;
	char								acTempPath[ACCESSNAME_LENGTH];
	
	//CID:1178, CHECKER: UNINIT
	memset(&tDynaParam, 0, sizeof(TRTSPSTREAMING_DYNAMIC_PARAM));

	//20090519 Update access name for all streams
    for( i=0; i<MULTIPLE_STREAM_NUM; i++ )
    {
		char		acXMLPath[64];
		
		memset(acXMLPath, 0, sizeof(acXMLPath));
		snprintf(acXMLPath, sizeof(acXMLPath) -1, "/root/network/rtsp/s%d/accessname", i);

		pcParam = XMLSParser_ReadContent(pzConfigFile, acXMLPath);

		if( pcParam != NULL && pThis->tStreamInfo[i].iEnable == 1)
		{
    		memset((void*)tDynaParam.acAccessName[i], 0, ACCESSNAME_LENGTH);
    		strncpy(tDynaParam.acAccessName[i], pcParam, ACCESSNAME_LENGTH-1);
		
			strncpy(acTempPath, SDP_PATH, sizeof(acTempPath));
		    
			if( strlen(SDP_PATH) + strlen(tDynaParam.acAccessName[i]) < ACCESSNAME_LENGTH-1)
       			rtspstrcat(acTempPath,tDynaParam.acAccessName[i], sizeof(acTempPath));
			else
    			return -1;
		    
			rename(pThis->tStreamInfo[i].szSDPFullPathName, acTempPath);
		    
			printf("access name of stream %d set ok %s -> %s\n", i + 1, pThis->tStreamInfo[i].szSDPFullPathName,acTempPath);
			strncpy(pThis->tStreamInfo[i].szSDPFullPathName, acTempPath, sizeof(pThis->tStreamInfo[i].szSDPFullPathName));
		}
	}

	pcParam = XMLSParser_ReadContent(pzConfigFile, "/root/network/rtsp/authmode");
	
	if( pcParam != NULL )
		CfgParser_GetAuthenticateMode((void*)pcParam, (void *)&tDynaParam.iRTSPAuthentication);
	
	printf("RTSP authmode is %d\n",tDynaParam.iRTSPAuthentication);

#ifdef _SIP
#ifdef _SIP_USE_RTSPAUTH
	tDynaParam.iSIPUASAuthentication = tDynaParam.iRTSPAuthentication;
	printf("No SIP authmode. Follow RTSP setting %d\n",tDynaParam.iSIPUASAuthentication);
#else
	pcParam = XMLSParser_ReadContent(pzConfigFile, "/root/network/sip/authmode");
	
	if( pcParam != NULL )
	{
		CfgParser_GetAuthenticateMode((void*)pcParam, (void *)&tDynaParam.iSIPUASAuthentication);
		printf("SIP authmode is %d\n",tDynaParam.iSIPUASAuthentication);
	}
#endif
#endif
	iFlag = RTSPSTREAMING_ACCESSNAME_SETFLAG | RTSPSTREAMING_RTSP_AUTHENTICATE_SETFLAG;
	 
	RTSPStreaming_SetDynamicParameters(pThis->hRTSPServer,&tDynaParam, iFlag);

	/* 20090225 QOS */
	StreamingServer_UpdateQosParameters(hRTSPS, pThis->acQosFilePath);
#ifdef _SHARED_MEM
	/* 20100105 Added For Seamless Recording */
	StreamingServer_UpdateSeamlessRecordingParameters(hRTSPS);
#endif

#ifdef RTSPRTP_MULTICAST
	//20100714 Added by danny For Multicast parameters load dynamically
	StreamingServer_UpdateMulticastParameters(hRTSPS, pzConfigFile);
	//20110725 Add by danny For Multicast RTCP receive report keep alive
	if((pcParam = XMLSParser_ReadContent(pzConfigFile, "/root/network/rtsp/multicasttimeout")) != NULL)
	{
		if( (g_iMulticastTimeout = strtoul(pcParam, NULL, 0)) <= 0 )
		{
			g_iMulticastTimeout = MULTICAST_TIMEOUT;
		}
		
	}
#endif

	return 0;	
}

