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
 *  Module name         :   RTSPStreaming.c
 *  Module description  :   Handle RTSP request and media streaming action
 *  Author              :   Simon
 *  Created at          :   2002/4/29
 *  Revision            :   1.0
 ******************************************************************************
 *                        Revision history
 ******************************************************************************
 */



#ifdef _LINUX
#include <sys/syslog.h>
#include <arpa/inet.h>
#endif // _LINUX

#include <stdlib.h>
#include <string.h>

#ifdef _SHARED_MEM
#include <pthread.h>
#endif

#include "ipfilter.h"
#include "rtspstreaming_local.h"

char	g_acAliasAccessName[MULTIPLE_STREAM_NUM][ACCESSNAME_LENGTH];

#ifdef _SIP
int RTSPStreaming_ComposeSIPSDP(HANDLE			hRTSPStreaming,
							   unsigned long	ulSDPIP,
							   int				iSDPIndex,	
							   int				iVivotekClient,							   
							   int				iStreamingMode,
							   char*			pcSDPBuffer,
							   int				iSDPBufferLen);

#ifdef _SIP_TWO_WAY_AUDIO 
int RTSPStreaming_ComposeSIPSDPFor2WayAudio(HANDLE			hRTSPStreaming,
											unsigned long	ulSDPIP,
											int				iG711CodecType,							   
											char*			pcSDPBuffer,
											int				iSDPBufferLen) ;
#endif
#endif

//Initialize the Alias access name
int	RTSPStreaming_InitializeAliasAccessName(RTSPSTREAMING *pRTSPStreaming)
{
	int i = 0;

	for(i = 0 ; i < MULTIPLE_STREAM_NUM; i++)
	{
		snprintf(g_acAliasAccessName[i], ACCESSNAME_LENGTH - 1, "s%d.sdp", i + 1);
	}

	return 0;
}

//return TRUE for video track, FALSE for audio track
int RTSPStreaming_CheckIfVideoTrack(RTSPSTREAMING *pRTSPStreaming,char *pcMediaName ,int iSDPIndex)
{
	if( (pRTSPStreaming == NULL) ||
	    (pcMediaName == NULL )   ||
	    (iSDPIndex > MULTIPLE_STREAM_NUM ) )
		return -1;

	if( strncmp(pRTSPStreaming->tVideoEncodeParam[iSDPIndex-1].acTrackName,pcMediaName,RTSPSTREAMING_TRACK_NAME_LEN) == 0 )
		return TRUE;
	else 
	{
		if( strncmp(pRTSPStreaming->tAudioEncodeParam[iSDPIndex-1].acTrackName,pcMediaName,RTSPSTREAMING_TRACK_NAME_LEN) == 0 )
			return FALSE;
		else
			return -1;
	}
	
}
//20120807 added by Jimmy for metadata
int RTSPStreaming_CheckTrackMediaType(RTSPSTREAMING *pRTSPStreaming,char *pcMediaName ,int iSDPIndex)
{
	//20120925 added by Jimmy for ONVIF backchannel
	int iChannelIndex;

	if( (pRTSPStreaming == NULL) ||
	    (pcMediaName == NULL )   ||
	    (iSDPIndex > MULTIPLE_STREAM_NUM ) )
		return -1;
	printf("%s pcMediaName = %s, iSDPIndex = %d\n", __func__, pcMediaName, iSDPIndex);
	//20120925 added by Jimmy for ONVIF backchannel
	iChannelIndex = pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle,
													0,
													ccctGetMultipleChannelChannelIndex,
													(DWORD)iSDPIndex);

	if( *pcMediaName == '\0' )
	{
		return RTSPSTREAMING_NO_MEDIA;
	}
	
	if( strncmp(pRTSPStreaming->tVideoEncodeParam[iSDPIndex-1].acTrackName,pcMediaName,RTSPSTREAMING_TRACK_NAME_LEN) == 0 )
	{
		return RTSPSTREAMING_MEDIATYPE_VIDEO;
	}
	else if( strncmp(pRTSPStreaming->tAudioEncodeParam[iSDPIndex-1].acTrackName,pcMediaName,RTSPSTREAMING_TRACK_NAME_LEN) == 0 ) 
	{
		return RTSPSTREAMING_MEDIATYPE_AUDIO;
	}
	else if( strncmp(pRTSPStreaming->tMetadataEncodeParam[iSDPIndex-1].acTrackName,pcMediaName,RTSPSTREAMING_TRACK_NAME_LEN) == 0 ) 
	{
		return RTSPSTREAMING_MEDIATYPE_METADATA;
	}
	//20120925 added by Jimmy for ONVIF backchannel
	else if( strncmp(pRTSPStreaming->tAudioDecodeParam[iChannelIndex-1].acTrackName,pcMediaName,RTSPSTREAMING_TRACK_NAME_LEN) == 0 )
	{
		return RTSPSTREAMING_MEDIATYPE_AUDIOBACK;
	}
	else
	{
		return RTSPSTREAMING_NO_MEDIA;
	}
}
//20160603 modify by faber, set tcp mutex
int	RTSPStreaming_AddRTPRTCPAudioSession(RTSPSTREAMING *pRTSPStreaming,
										RTSPSERVER_SESSIONINFORMATION *pstRTSPServerSessionInformation,
										int		iMediaIndex, HANDLE hTCPMuxCS) 
{
	RTPRTCPCOMPOSER_PARAM		stRTPRTCPComposerParam;
	RTPRTCPCHANNEL_CONNECTION	stRTPRTCPSessionParam;
	int iResult = -1,iMulticastIndex = -1;
#ifdef _SHARED_MEM
	//20100728 Modifed by danny For multiple channels videoin/audioin
	int iTrackIndex = 0;
#endif
	// reset Audio RTP/RTCP composer
#ifdef _G711_AUDIOIN
	if (pRTSPStreaming->tAudioEncodeParam[pstRTSPServerSessionInformation->iSDPIndex-1].iAudioCodecType==ractG711u)
	{
		stRTPRTCPComposerParam.iMediaType = 0 ;
	}
	else if (pRTSPStreaming->tAudioEncodeParam[pstRTSPServerSessionInformation->iSDPIndex-1].iAudioCodecType==ractG711a)
	{
		stRTPRTCPComposerParam.iMediaType = 8 ;
	}
	else
#endif
	stRTPRTCPComposerParam.iMediaType = RTSPSTREAMING_AMR_MEDIATYPE;

	if( pRTSPStreaming->tAudioEncodeParam[pstRTSPServerSessionInformation->iSDPIndex-1].iClockRate != 0 )
	{
		stRTPRTCPComposerParam.iSampleFrequency = pRTSPStreaming->tAudioEncodeParam[pstRTSPServerSessionInformation->iSDPIndex-1].iClockRate;
	}
	else
	{
		TelnetShell_DbgPrint("[RTSPStreaming Control]: Audio %d sample rate is 0!!\r\n",pstRTSPServerSessionInformation->iSDPIndex);
		return -1;
	}
	stRTPRTCPComposerParam.dwSessionID=pstRTSPServerSessionInformation->dwSessionID;
	stRTPRTCPComposerParam.dwInitialTimeStamp=pstRTSPServerSessionInformation->dwInitialTimeStamp[iMediaIndex];
	stRTPRTCPComposerParam.wInitialSequenceNumber=pstRTSPServerSessionInformation->wInitialSequenceNumber[iMediaIndex];
	stRTPRTCPComposerParam.dwSSRC=pstRTSPServerSessionInformation->dwSSRC[iMediaIndex];

#ifdef RTSPRTP_MULTICAST
    //20130904 modified by Charles for ondemand multicast
	if((pstRTSPServerSessionInformation->iMulticast > 0 &&
		pstRTSPServerSessionInformation->iMulticast	<= RTSP_MULTICASTNUMBER)
		|| (pstRTSPServerSessionInformation->iMulticast > RTSP_MULTICASTNUMBER  &&
		pstRTSPServerSessionInformation->iMulticast <= RTSP_MULTICASTNUMBER + RTSP_ONDEMAND_MULTICASTNUMBER))
	{

		iMulticastIndex = pstRTSPServerSessionInformation->iMulticast-1;
		if( pRTSPStreaming->stMulticast[iMulticastIndex].iAlreadyMulticastAudio == 0 &&
			pRTSPStreaming->stMulticast[iMulticastIndex].iAlwaysMulticast == 0)
		{
#ifdef _SHARED_MEM
			TShmemSessionInfo *pShmSessionInfo = (TShmemSessionInfo *)pRTSPStreaming->stMulticast[iMulticastIndex].hShmemSessionInfo;

			pShmSessionInfo->dwProtectedDelta = pstRTSPServerSessionInformation->dwProtectedDelta;
			pShmSessionInfo->dwBypasyMSec = pstRTSPServerSessionInformation->dwBypasyMSec;
			pShmSessionInfo->eHSMode = pstRTSPServerSessionInformation->eHSMode;
			//20100728 Modifed by danny For multiple channels videoin/audioin
			iTrackIndex = pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle,
					                    					pstRTSPServerSessionInformation->dwSessionID,
					                    					ccctGetMultipleChannelChannelIndex,
					                    					(DWORD)pstRTSPServerSessionInformation->iSDPIndex) - 1;
			//20130605 modified by Jimmy to support metadata event
			pShmSessionInfo->tShmemAudioMediaInfo.ahShmemHandle[0] = pRTSPStreaming->ahShmemAudioHandle[iTrackIndex];

			//added by neil 10/12/30
			pShmSessionInfo->eSVCMode = pstRTSPServerSessionInformation->eSVCMode;
			pShmSessionInfo->eSVCTempMode = eSVCInvalid;
			printf("*********share memory SVC mode = %d**************\n", pShmSessionInfo->eSVCMode);
			//20120328 Marked by danny For audio shmem handle disordered
			//pShmSessionInfo->tShmemAudioMediaInfo.hShmemHandle = pRTSPStreaming->ahShmemAudioHandle[0];

			pShmSessionInfo->tShmemAudioMediaInfo.bFrameGenerated = TRUE;
#endif
			RTPRTCPComposer_Reset(pRTSPStreaming->stMulticast[iMulticastIndex].hRTPRTCPAudioComposerHandle,&stRTPRTCPComposerParam);
//							stRTPRTCPSessionParam.dwSessionID=pstRTSPServerSessionInformation->dwSessionID;

			stRTPRTCPSessionParam.iRTPStreamingType = RTP_OVER_UDP;
			//20111205 Modified by danny For UDP mode socket leak
			stRTPRTCPSessionParam.psktRTP = &pRTSPStreaming->stMulticast[iMulticastIndex].aiMulticastSocket[2];
			stRTPRTCPSessionParam.psktRTCP = &pRTSPStreaming->stMulticast[iMulticastIndex].aiMulticastSocket[3];
			stRTPRTCPSessionParam.iVivotekClient = pRTSPStreaming->stMulticast[iMulticastIndex].iRTPExtension;
			//20110627 Add by danny for join/leave multicast group by session start/stop
			//20160127 Add by faber, Audio channel will using address of audio
			stRTPRTCPSessionParam.ulMulticastAddress = pRTSPStreaming->stMulticast[iMulticastIndex].ulMulticastAudioAddress;
			stRTPRTCPSessionParam.usMulticastRTCPPort = pRTSPStreaming->stMulticast[iMulticastIndex].usMulticastAudioPort + 1;
			//20110725 Add by danny For Multicast RTCP receive report keep alive
			stRTPRTCPSessionParam.iRRAlive = pRTSPStreaming->stMulticast[iMulticastIndex].iRRAlive;
            //20141110 added by Charles for ONVIF Profile G
            stRTPRTCPSessionParam.iCseq = pstRTSPServerSessionInformation->iCseq;
			//stRTPRTCPSessionParam.iCodecIndex = pstRTSPServerSessionInformation->iSDPIndex;
			stRTPRTCPSessionParam.iCodecIndex = pRTSPStreaming->fAudioCallBack((DWORD) pRTSPStreaming->hParentAudioHandle,
						                                   MEDIA_CALLBACK_CHECK_CODEC_INDEX,
						                                   (void*)pstRTSPServerSessionInformation->iSDPIndex);
														   //(void*)pstRTSPServerSessionInformation->cMediaName[iMediaIndex]);
			stRTPRTCPSessionParam.dwSessionID =  pstRTSPServerSessionInformation->dwSessionID;
#ifdef _SHARED_MEM
			//20101018 Add by danny for support multiple channel text on video
			if( pstRTSPServerSessionInformation->iSDPIndex <= LIVE_STREAM_NUM)
			{
				stRTPRTCPSessionParam.iMultipleChannelChannelIndex = iTrackIndex + 1;
			}
#endif
			
			stRTPRTCPSessionParam.hRTPRTCPComposerHandle=pRTSPStreaming->stMulticast[iMulticastIndex].hRTPRTCPAudioComposerHandle;
#ifdef _SHARED_MEM
			stRTPRTCPSessionParam.ptShmemMediaInfo = &pShmSessionInfo->tShmemAudioMediaInfo;
#endif		
			stRTPRTCPSessionParam.iNotifySource = pstRTSPServerSessionInformation->iNotifySource;
			stRTPRTCPSessionParam.iSdpIndex = pstRTSPServerSessionInformation->iSDPIndex;
			iResult = RTPRTCPChannel_AddMulticastSession(pRTSPStreaming->hRTPRTCPChannelAudioHandle, &stRTPRTCPSessionParam,pstRTSPServerSessionInformation->iMulticast);
			
			//20111205 Modified by danny For UDP mode socket leak
			printf("[RTSPStreamServer]:Add Audio multicast channel ID:%ul Socket:%d\n",stRTPRTCPSessionParam.dwSessionID, *stRTPRTCPSessionParam.psktRTP); 
			pRTSPStreaming->stMulticast[iMulticastIndex].iAlreadyMulticastAudio = 1;
			/* 20080828 marked out by Louis to prevent start channel 2 times
			if( pRTSPStreaming->iAudioSessionNumberOfStreamType[iMulticastIndex] == 0 )
			{
				TelnetShell_DbgPrint("[RTSP Streaming Control]: Audio of Streaming Type %d should start !!!\r\n",pstRTSPServerSessionInformation->iMulticast);
			    pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle,0,ccctStreamTypeAudioStart,pstRTSPServerSessionInformation->iMulticast);
			}*/
		}
		else
			iResult = 0;		
	}

	if( pstRTSPServerSessionInformation->iMulticast <= 0 )
#endif
	{
#ifdef _SHARED_MEM				
		TShmemSessionInfo *pShmSessionInfo = (TShmemSessionInfo *)pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].hShmemSessionInfo;

		pShmSessionInfo->dwProtectedDelta = pstRTSPServerSessionInformation->dwProtectedDelta;
		pShmSessionInfo->dwBypasyMSec = pstRTSPServerSessionInformation->dwBypasyMSec;
		pShmSessionInfo->eHSMode = pstRTSPServerSessionInformation->eHSMode;
		//20100727 Modify by danny to fix Rtsp server no stream MOD audio issue
		//pShmSessionInfo->tShmemAudioMediaInfo.hShmemHandle = pRTSPStreaming->ahShmemAudioHandle[0];
		if( pstRTSPServerSessionInformation->iSDPIndex > LIVE_STREAM_NUM)
		{
			//20100728 Modifed by danny For multiple channels videoin/audioin
			iTrackIndex = pstRTSPServerSessionInformation->iSDPIndex - LIVE_STREAM_NUM + (LIVE_AUDIO_STREAM_NUM - 1);
			//20130605 modified by Jimmy to support metadata event
			pShmSessionInfo->tShmemAudioMediaInfo.ahShmemHandle[0] = pRTSPStreaming->ahShmemAudioHandle[iTrackIndex];
		}
		else
		{	
			//20100728 Modifed by danny For multiple channels videoin/audioin
			iTrackIndex = pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle,
					                    					pstRTSPServerSessionInformation->dwSessionID,
					                    					ccctGetMultipleChannelChannelIndex,
					                    					(DWORD)pstRTSPServerSessionInformation->iSDPIndex) - 1;
			//20130605 modified by Jimmy to support metadata event
			pShmSessionInfo->tShmemAudioMediaInfo.ahShmemHandle[0] = pRTSPStreaming->ahShmemAudioHandle[iTrackIndex];
			//printf("Audio iTrackIndex %d hShmemHandle %p\n", iTrackIndex, pShmSessionInfo->tShmemAudioMediaInfo.hShmemHandle);
		}
		//added by neil 10/12/30
		pShmSessionInfo->eSVCMode = pstRTSPServerSessionInformation->eSVCMode;
		pShmSessionInfo->eSVCTempMode = eSVCInvalid;
		printf("*********share memory SVC mode = %d**************\n", pShmSessionInfo->eSVCMode);
		//20120328 Marked by danny For audio shmem handle disordered
		//pShmSessionInfo->tShmemAudioMediaInfo.hShmemHandle = pRTSPStreaming->ahShmemAudioHandle[0];

		pShmSessionInfo->tShmemAudioMediaInfo.bFrameGenerated = TRUE;
        //20140812 Added by Charles for mod no drop frame
        pShmSessionInfo->tShmemAudioMediaInfo.bMediaOnDemand = pstRTSPServerSessionInformation->bMediaOnDemand;
#endif
		RTPRTCPComposer_Reset(pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].hRTPRTCPAudioComposerHandle,&stRTPRTCPComposerParam);

		stRTPRTCPSessionParam.dwSessionID=pstRTSPServerSessionInformation->dwSessionID;

		if( pstRTSPServerSessionInformation->iRTPStreamingType == RTP_OVER_UDP )
		{
			stRTPRTCPSessionParam.iRTPStreamingType = RTP_OVER_UDP;
			//20111205 Modified by danny For UDP mode socket leak
			stRTPRTCPSessionParam.psktRTP=pstRTSPServerSessionInformation->psktRTP[iMediaIndex];
			stRTPRTCPSessionParam.psktRTCP=pstRTSPServerSessionInformation->psktRTCP[iMediaIndex];
			memcpy((void*)&stRTPRTCPSessionParam.RTPNATAddr,(void*)&pstRTSPServerSessionInformation->NATRTPAddr[iMediaIndex],sizeof(RTSP_SOCKADDR));
			memcpy((void*)&stRTPRTCPSessionParam.RTCPNATAddr,(void*)&pstRTSPServerSessionInformation->NATRTCPAddr[iMediaIndex],sizeof(RTSP_SOCKADDR));
		}
		else
		{
			stRTPRTCPSessionParam.iRTPStreamingType = RTP_OVER_TCP;
			stRTPRTCPSessionParam.iEmbeddedRTPID = pstRTSPServerSessionInformation->iEmbeddedRTPID[iMediaIndex];
			stRTPRTCPSessionParam.iEmbeddedRTCPID = pstRTSPServerSessionInformation->iEmbeddedRTCPID[iMediaIndex];
			//20110706 Modified by danny For TCP/HTTP mode socket not sync in multi thread 
			stRTPRTCPSessionParam.psktRTSPSocket = pstRTSPServerSessionInformation->psktRTSPSocket;
			//20160603 add by Faber, set tcp mutex
			stRTPRTCPSessionParam.hTCPMuxCS = hTCPMuxCS;
		}
		stRTPRTCPSessionParam.iVivotekClient = pstRTSPServerSessionInformation->iVivotekClient;
        //20141110 added by Charles for ONVIF Profile G
        stRTPRTCPSessionParam.iCseq = pstRTSPServerSessionInformation->iCseq;
		stRTPRTCPSessionParam.iCodecIndex = pRTSPStreaming->fAudioCallBack((DWORD) pRTSPStreaming->hParentAudioHandle,
						                                   MEDIA_CALLBACK_CHECK_CODEC_INDEX,
														   (void*)pstRTSPServerSessionInformation->iSDPIndex);
														   //(void*)pstRTSPServerSessionInformation->cMediaName[iMediaIndex]);
		//stRTPRTCPSessionParam.iCodecIndex = pstRTSPServerSessionInformation->iSDPIndex;
		
#ifdef _SHARED_MEM
		//20101020 Add by danny for support seamless stream TCP/UDP timeout
		stRTPRTCPSessionParam.bSeamlessStream = pstRTSPServerSessionInformation->bSeamlessStream;
		//20101018 Add by danny for support multiple channel text on video
		if( pstRTSPServerSessionInformation->iSDPIndex <= LIVE_STREAM_NUM)
		{
			stRTPRTCPSessionParam.iMultipleChannelChannelIndex = iTrackIndex + 1;
		}
        else
        {
            //mod not support multiple channel yet, always set channel 1
            stRTPRTCPSessionParam.iMultipleChannelChannelIndex = 1;
        }
#endif

//		if(stRTPRTCPSessionParam.iVivotekClient != 0)
//			TelnetShell_DbgPrint("--[Control] RTPEX Player added to audio session %d--\r\n",stRTPRTCPSessionParam.iVivotekClient);

		stRTPRTCPSessionParam.hRTPRTCPComposerHandle=pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].hRTPRTCPAudioComposerHandle;
#ifdef _SHARED_MEM
		stRTPRTCPSessionParam.ptShmemMediaInfo = &pShmSessionInfo->tShmemAudioMediaInfo;
#endif	
//20170517 // pass the flag of notify source 	
		stRTPRTCPSessionParam.iNotifySource = pstRTSPServerSessionInformation->iNotifySource;
		stRTPRTCPSessionParam.iSdpIndex = pstRTSPServerSessionInformation->iSDPIndex;
		iResult= RTPRTCPChannel_AddOneSession(pRTSPStreaming->hRTPRTCPChannelAudioHandle, &stRTPRTCPSessionParam);
	
		//20111205 Modified by danny For UDP mode socket leak
		DbgLog((dfCONSOLE|dfINTERNAL,"Add Audio channel ID:%ul Socket:%d\n",stRTPRTCPSessionParam.dwSessionID, *stRTPRTCPSessionParam.psktRTP));   
	}

	return iResult;
}

//20160603 Modify by Faber , set ,mutex into channel session

int	RTSPStreaming_AddRTPRTCPVideoSession(RTSPSTREAMING *pRTSPStreaming,
										RTSPSERVER_SESSIONINFORMATION * pstRTSPServerSessionInformation,
										int		iMediaIndex, HANDLE hTCPMuxCS)
{
	RTPRTCPCOMPOSER_PARAM		stRTPRTCPComposerParam;
	RTPRTCPCHANNEL_CONNECTION	stRTPRTCPSessionParam;
	int iResult = -1,iMulticastIndex = -1;
	int iCodecIndex = -1;
	printf("RTSPStreaming_AddRTPRTCPVideoSession\n");
	//20090105 JPEG/H264
	if(pRTSPStreaming->tVideoEncodeParam[pstRTSPServerSessionInformation->iSDPIndex-1].eVideoCodecType == mctMP4V)
	{
		stRTPRTCPComposerParam.iMediaType=RTSPSTREAMING_MPEG4_MEDIATYPE;
	}
	else if(pRTSPStreaming->tVideoEncodeParam[pstRTSPServerSessionInformation->iSDPIndex-1].eVideoCodecType == mctJPEG)
	{
		stRTPRTCPComposerParam.iMediaType=RTSPSTREAMING_JPEG_MEDIATYPE;
	}
	else if(pRTSPStreaming->tVideoEncodeParam[pstRTSPServerSessionInformation->iSDPIndex-1].eVideoCodecType == mctH264)
	{
		stRTPRTCPComposerParam.iMediaType=RTSPSTREAMING_H264_MEDIATYPE;
	}
    else if(pRTSPStreaming->tVideoEncodeParam[pstRTSPServerSessionInformation->iSDPIndex-1].eVideoCodecType == mctH265)
	{
		stRTPRTCPComposerParam.iMediaType=RTSPSTREAMING_H265_MEDIATYPE;
	}

	if( pRTSPStreaming->tVideoEncodeParam[pstRTSPServerSessionInformation->iSDPIndex-1].iClockRate != 0 )
		stRTPRTCPComposerParam.iSampleFrequency = pRTSPStreaming->tVideoEncodeParam[pstRTSPServerSessionInformation->iSDPIndex-1].iClockRate;//RTSPSTREAMING_MPEG4_CLOCKRATE;
	else
	{
		TelnetShell_DbgPrint("[RTSPStreaming Control]: Video %d sample rate is 0!!\r\n",pstRTSPServerSessionInformation->iSDPIndex);
		return -1;
	}

	stRTPRTCPComposerParam.dwSessionID=pstRTSPServerSessionInformation->dwSessionID;
	stRTPRTCPComposerParam.dwInitialTimeStamp=pstRTSPServerSessionInformation->dwInitialTimeStamp[iMediaIndex];
	stRTPRTCPComposerParam.wInitialSequenceNumber=pstRTSPServerSessionInformation->wInitialSequenceNumber[iMediaIndex];
	stRTPRTCPComposerParam.dwSSRC=pstRTSPServerSessionInformation->dwSSRC[iMediaIndex];

#ifdef RTSPRTP_MULTICAST
    //20130904 modified by Charles for ondemand multicast
	if((pstRTSPServerSessionInformation->iMulticast > 0 &&
		pstRTSPServerSessionInformation->iMulticast	<= RTSP_MULTICASTNUMBER)
		|| (pstRTSPServerSessionInformation->iMulticast > RTSP_MULTICASTNUMBER &&
		pstRTSPServerSessionInformation->iMulticast <= RTSP_MULTICASTNUMBER + RTSP_ONDEMAND_MULTICASTNUMBER))
	{
		iMulticastIndex = pstRTSPServerSessionInformation->iMulticast-1;

		//20120917 modified by Jimmy to fix that multicast sessions start without forcing I frame
		if(pRTSPStreaming->stMulticast[iMulticastIndex].iAlwaysMulticast == 0)
		{
			if(pRTSPStreaming->stMulticast[iMulticastIndex].iAlreadyMulticastVideo == 0)
			{
#ifdef _SHARED_MEM				
				TShmemSessionInfo *pShmSessionInfo = (TShmemSessionInfo *)pRTSPStreaming->stMulticast[iMulticastIndex].hShmemSessionInfo;
	
				//20100812 Added For Client Side Frame Rate Control
				pShmSessionInfo->iFrameIntervalMSec = -1;
				pShmSessionInfo->dwProtectedDelta = pstRTSPServerSessionInformation->dwProtectedDelta;
				pShmSessionInfo->dwBypasyMSec = pstRTSPServerSessionInformation->dwBypasyMSec;
				pShmSessionInfo->eHSMode = pstRTSPServerSessionInformation->eHSMode;
				//added by neil 10/12/30
				pShmSessionInfo->eSVCMode = pstRTSPServerSessionInformation->eSVCMode;
				pShmSessionInfo->eSVCTempMode = eSVCInvalid;
				printf("*********share memory SVC mode = %d**************\n", pShmSessionInfo->eSVCMode);			
				//20130605 modified by Jimmy to support metadata event
				pShmSessionInfo->tShmemVideoMediaInfo.ahShmemHandle[0] = pRTSPStreaming->ahShmemVideoHandle[(pstRTSPServerSessionInformation->iSDPIndex - 1)];
				pShmSessionInfo->tShmemVideoMediaInfo.bFrameGenerated = TRUE;
#endif
				RTPRTCPComposer_Reset(pRTSPStreaming->stMulticast[iMulticastIndex].hRTPRTCPVideoComposerHandle,&stRTPRTCPComposerParam);
//							stRTPRTCPSessionParam.dwSessionID=pstRTSPServerSessionInformation->dwSessionID;
	
				stRTPRTCPSessionParam.iRTPStreamingType = RTP_OVER_UDP;
				//20111205 Modified by danny For UDP mode socket leak
				stRTPRTCPSessionParam.psktRTP = &pRTSPStreaming->stMulticast[iMulticastIndex].aiMulticastSocket[0];
				stRTPRTCPSessionParam.psktRTCP = &pRTSPStreaming->stMulticast[iMulticastIndex].aiMulticastSocket[1];
				stRTPRTCPSessionParam.iVivotekClient = pRTSPStreaming->stMulticast[iMulticastIndex].iRTPExtension;
				//20110627 Add by danny for join/leave multicast group by session start/stop
				stRTPRTCPSessionParam.ulMulticastAddress = pRTSPStreaming->stMulticast[iMulticastIndex].ulMulticastAddress;
				stRTPRTCPSessionParam.usMulticastRTCPPort = pRTSPStreaming->stMulticast[iMulticastIndex].usMulticastVideoPort + 1;
				//20110725 Add by danny For Multicast RTCP receive report keep alive
				stRTPRTCPSessionParam.iRRAlive = pRTSPStreaming->stMulticast[iMulticastIndex].iRRAlive;
                //20141110 added by Charles for ONVIF Profile G
                stRTPRTCPSessionParam.iCseq = pstRTSPServerSessionInformation->iCseq;
				//stRTPRTCPSessionParam.iCodecIndex = pstRTSPServerSessionInformation->iSDPIndex;
				stRTPRTCPSessionParam.iCodecIndex = pRTSPStreaming->fVideoCallBack((DWORD) pRTSPStreaming->hParentVideoHandle,
														   MEDIA_CALLBACK_CHECK_CODEC_INDEX,
														   (void*)pstRTSPServerSessionInformation->iSDPIndex);
														   //(void*)pstRTSPServerSessionInformation->cMediaName[iMediaIndex]);
				stRTPRTCPSessionParam.dwSessionID =  pstRTSPServerSessionInformation->dwSessionID;
	
#ifdef _SHARED_MEM													   
				//20101018 Add by danny for support multiple channel text on video
				if( pstRTSPServerSessionInformation->iSDPIndex <= LIVE_STREAM_NUM)
				{
					stRTPRTCPSessionParam.iMultipleChannelChannelIndex = pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle,
																				pstRTSPServerSessionInformation->dwSessionID,
																				ccctGetMultipleChannelChannelIndex,
																				(DWORD)pstRTSPServerSessionInformation->iSDPIndex);
				}
#endif
				
				stRTPRTCPSessionParam.hRTPRTCPComposerHandle=pRTSPStreaming->stMulticast[iMulticastIndex].hRTPRTCPVideoComposerHandle;
#ifdef _SHARED_MEM
				stRTPRTCPSessionParam.ptShmemMediaInfo = &pShmSessionInfo->tShmemVideoMediaInfo;
#endif
				stRTPRTCPSessionParam.iSdpIndex = pstRTSPServerSessionInformation->iSDPIndex;
				stRTPRTCPSessionParam.iNotifySource = pstRTSPServerSessionInformation->iNotifySource;
				printf("video output start, notify = %d\n", stRTPRTCPSessionParam.iNotifySource);
				iResult = RTPRTCPChannel_AddMulticastSession(pRTSPStreaming->hRTPRTCPChannelVideoHandle, &stRTPRTCPSessionParam,pstRTSPServerSessionInformation->iMulticast);

				//20120917 added by Jimmy to fix that multicast sessions start without forcing I frame
				// Force I frame
				//20160621 modify by Faber, we force intrac in ChannelLoopAddMulticast
				// pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle,
				// 								pstRTSPServerSessionInformation->dwSessionID,
				// 								ccctForceIntra,
				// 								(DWORD)stRTPRTCPSessionParam.iCodecIndex);
	
				//20111205 Modified by danny For UDP mode socket leak
				if( iResult == S_OK )
					printf("[RTSPStreamServer]:Add Video multicast channel ID:%ul Socket:%d\n",stRTPRTCPSessionParam.dwSessionID, *stRTPRTCPSessionParam.psktRTP);
				pRTSPStreaming->stMulticast[iMulticastIndex].iAlreadyMulticastVideo = 1;
				
				/* 20080828 marked out by Louis to prevent start channel 2 times
				if( pRTSPStreaming->iVideoSessionNumberOfStreamType[iMulticastIndex] == 0 )
				{
					TelnetShell_DbgPrint("[RTSP Streaming Control]: Video of Streaming Type %d should start !!!\r\n",pstRTSPServerSessionInformation->iMulticast);
					pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle,0,ccctStreamTypeVideoStart,pstRTSPServerSessionInformation->iMulticast);
				}*/

			}
			//20120917 added by Jimmy to fix that multicast sessions start without forcing I frame
			else
			{
				iCodecIndex = pRTSPStreaming->fVideoCallBack((DWORD) pRTSPStreaming->hParentVideoHandle,
														   MEDIA_CALLBACK_CHECK_CODEC_INDEX,
														   (void*)pstRTSPServerSessionInformation->iSDPIndex);
				// Force I frame
				pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle,
												pstRTSPServerSessionInformation->dwSessionID,
												ccctForceIntra,
												(DWORD)iCodecIndex);
				iResult = 0;

			}
		}
		else
		{
			iResult = 0;
		}
	}

	if( pstRTSPServerSessionInformation->iMulticast <= 0 )
#endif
	{
#ifdef _SHARED_MEM		
		TShmemSessionInfo *pShmSessionInfo = (TShmemSessionInfo *)pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].hShmemSessionInfo;

		pShmSessionInfo->dwProtectedDelta = pstRTSPServerSessionInformation->dwProtectedDelta;
		pShmSessionInfo->dwBypasyMSec = pstRTSPServerSessionInformation->dwBypasyMSec;
		pShmSessionInfo->eHSMode = pstRTSPServerSessionInformation->eHSMode;
		//added by neil 10/12/30
		pShmSessionInfo->eSVCMode = pstRTSPServerSessionInformation->eSVCMode;
		pShmSessionInfo->eSVCTempMode = eSVCInvalid;
		printf("*********share memory SVC mode = %d**************\n", pShmSessionInfo->eSVCMode);		
		//added by neil 11/01/14
		pShmSessionInfo->bForceFrameInterval = pstRTSPServerSessionInformation->bForceFrameInterval;
        pShmSessionInfo->dwFrameInterval = pstRTSPServerSessionInformation->dwFrameInterval;
		printf("*********share memory frameinterval = %d**************\n", pShmSessionInfo->dwFrameInterval);

		//20100812 Added For Client Side Frame Rate Control
		if( pstRTSPServerSessionInformation->eHSMode == eHSAdaptiveRecording )
		{
			if( pstRTSPServerSessionInformation->iFrameIntervalMSec >= 0 )
			{
				pShmSessionInfo->iFrameIntervalMSec = pstRTSPServerSessionInformation->iFrameIntervalMSec;
			}
			else
			{
				if( pRTSPStreaming->tVideoEncodeParam[pstRTSPServerSessionInformation->iSDPIndex-1].eVideoCodecType == mctJPEG )
				{
					pShmSessionInfo->iFrameIntervalMSec = 0;	//Default Value: 1fps
				}
				else
				{
					pShmSessionInfo->iFrameIntervalMSec = 1;
				}
			}
		}
		else
		{
			if( pRTSPStreaming->tVideoEncodeParam[pstRTSPServerSessionInformation->iSDPIndex-1].eVideoCodecType == mctJPEG )
			{
				pShmSessionInfo->iFrameIntervalMSec = pstRTSPServerSessionInformation->iFrameIntervalMSec;
			}
			else
			{
				pShmSessionInfo->iFrameIntervalMSec = -1;
			}
		}
		
		//20130605 modified by Jimmy to support metadata event
		pShmSessionInfo->tShmemVideoMediaInfo.ahShmemHandle[0] = pRTSPStreaming->ahShmemVideoHandle[(pstRTSPServerSessionInformation->iSDPIndex - 1)];
		pShmSessionInfo->tShmemVideoMediaInfo.bFrameGenerated = TRUE;
        //20140812 Added by Charles for mod no drop frame
        pShmSessionInfo->tShmemVideoMediaInfo.bMediaOnDemand = pstRTSPServerSessionInformation->bMediaOnDemand;
#endif
		RTPRTCPComposer_Reset(pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].hRTPRTCPVideoComposerHandle,&stRTPRTCPComposerParam);
			
		// Add session to Video RTP/RTCP channel
		stRTPRTCPSessionParam.dwSessionID=pstRTSPServerSessionInformation->dwSessionID;

		if( pstRTSPServerSessionInformation->iRTPStreamingType == RTP_OVER_UDP )
		{
			stRTPRTCPSessionParam.iRTPStreamingType = RTP_OVER_UDP;
			//20111205 Modified by danny For UDP mode socket leak
			stRTPRTCPSessionParam.psktRTP=pstRTSPServerSessionInformation->psktRTP[iMediaIndex];
			stRTPRTCPSessionParam.psktRTCP=pstRTSPServerSessionInformation->psktRTCP[iMediaIndex];
			memcpy((void*)&stRTPRTCPSessionParam.RTPNATAddr,(void*)&pstRTSPServerSessionInformation->NATRTPAddr[iMediaIndex],sizeof(RTSP_SOCKADDR));
			memcpy((void*)&stRTPRTCPSessionParam.RTCPNATAddr,(void*)&pstRTSPServerSessionInformation->NATRTCPAddr[iMediaIndex],sizeof(RTSP_SOCKADDR));
		}
		else
		{
			stRTPRTCPSessionParam.iRTPStreamingType = RTP_OVER_TCP;
			stRTPRTCPSessionParam.iEmbeddedRTPID = pstRTSPServerSessionInformation->iEmbeddedRTPID[iMediaIndex];
			stRTPRTCPSessionParam.iEmbeddedRTCPID = pstRTSPServerSessionInformation->iEmbeddedRTCPID[iMediaIndex];
			//20110706 Modified by danny For TCP/HTTP mode socket not sync in multi thread 
			stRTPRTCPSessionParam.psktRTSPSocket = pstRTSPServerSessionInformation->psktRTSPSocket;

			//20160602 add by Faber, set mutex into session param
			stRTPRTCPSessionParam.hTCPMuxCS = hTCPMuxCS;
		}

		stRTPRTCPSessionParam.iVivotekClient = pstRTSPServerSessionInformation->iVivotekClient;
        //20141110 added by Charles for ONVIF Profile G
        stRTPRTCPSessionParam.iCseq = pstRTSPServerSessionInformation->iCseq;
		stRTPRTCPSessionParam.iCodecIndex = pRTSPStreaming->fVideoCallBack((DWORD) pRTSPStreaming->hParentVideoHandle,
					                                   MEDIA_CALLBACK_CHECK_CODEC_INDEX,
					                                   (void*)pstRTSPServerSessionInformation->iSDPIndex);
													   //(void*)pstRTSPServerSessionInformation->cMediaName[iMediaIndex]);
		//stRTPRTCPSessionParam.iCodecIndex = pstRTSPServerSessionInformation->iSDPIndex;

#ifdef _SHARED_MEM
		//20101020 Add by danny for support seamless stream TCP/UDP timeout
		stRTPRTCPSessionParam.bSeamlessStream = pstRTSPServerSessionInformation->bSeamlessStream;
		//20101018 Add by danny for support multiple channel text on video
		if( pstRTSPServerSessionInformation->iSDPIndex <= LIVE_STREAM_NUM)
		{
			stRTPRTCPSessionParam.iMultipleChannelChannelIndex = pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle,
					                    								pstRTSPServerSessionInformation->dwSessionID,
					                    								ccctGetMultipleChannelChannelIndex,
					                    								(DWORD)pstRTSPServerSessionInformation->iSDPIndex);
		}
        else
        {
            //mod not support multiple channel yet, always set channel 1
            stRTPRTCPSessionParam.iMultipleChannelChannelIndex = 1;
        }
#endif

		/*if(stRTPRTCPSessionParam.iVivotekClient != 0)
			TelnetShell_DbgPrint("--[Control] RTPEX Player added to video session %d--\r\n",stRTPRTCPSessionParam.iVivotekClient);*/
			
		stRTPRTCPSessionParam.hRTPRTCPComposerHandle=pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].hRTPRTCPVideoComposerHandle;
#ifdef _SHARED_MEM
		stRTPRTCPSessionParam.ptShmemMediaInfo = &pShmSessionInfo->tShmemVideoMediaInfo;
#endif
		//20170517 // pass the flag of notify source 	
		stRTPRTCPSessionParam.iNotifySource = pstRTSPServerSessionInformation->iNotifySource;
		stRTPRTCPSessionParam.iSdpIndex = pstRTSPServerSessionInformation->iSDPIndex;
		iResult = RTPRTCPChannel_AddOneSession(pRTSPStreaming->hRTPRTCPChannelVideoHandle, &stRTPRTCPSessionParam);			
		
		//20160621 modify by Faber, we force intra at channel thread.
		// Force I frame
		// pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle,
		// 			                    pstRTSPServerSessionInformation->dwSessionID,
		// 			                    ccctForceIntra,
		// 			                    (DWORD)stRTPRTCPSessionParam.iCodecIndex);
	    //20111205 Modified by danny For UDP mode socket leak
		DbgLog((dfCONSOLE|dfINTERNAL,"Add video channel ID:%ul Socket:%d\n",stRTPRTCPSessionParam.dwSessionID, *stRTPRTCPSessionParam.psktRTP));
	}

	return iResult;
}
//20120806 added by Jimmy for metadata
//20160601 modify by faber, set mutex into 
int	RTSPStreaming_AddRTPRTCPMetadataSession(RTSPSTREAMING *pRTSPStreaming,
										RTSPSERVER_SESSIONINFORMATION * pstRTSPServerSessionInformation,
										int		iMediaIndex, HANDLE hTCPMuxCS)
{
	RTPRTCPCOMPOSER_PARAM		stRTPRTCPComposerParam;
	RTPRTCPCHANNEL_CONNECTION	stRTPRTCPSessionParam;
	int iResult = -1,iMulticastIndex = -1;
	//20130605 added by Jimmy to support metadata event
	int i;
#ifdef _SHARED_MEM
	//20100728 Modifed by danny For multiple channels videoin/audioin
	int iTrackIndex = 0;
#endif
	// reset Metadata RTP/RTCP composer
	stRTPRTCPComposerParam.iMediaType = RTSPSTREAMING_METADATA_MEDIATYPE;

	if( pRTSPStreaming->tMetadataEncodeParam[pstRTSPServerSessionInformation->iSDPIndex-1].iClockRate != 0 )
	{
		stRTPRTCPComposerParam.iSampleFrequency = pRTSPStreaming->tMetadataEncodeParam[pstRTSPServerSessionInformation->iSDPIndex-1].iClockRate;
	}
	else
	{
		TelnetShell_DbgPrint("[RTSPStreaming Control]: Metadata %d sample rate is 0!!\r\n",pstRTSPServerSessionInformation->iSDPIndex);
		return -1;
	}
	stRTPRTCPComposerParam.dwSessionID=pstRTSPServerSessionInformation->dwSessionID;
	stRTPRTCPComposerParam.dwInitialTimeStamp=pstRTSPServerSessionInformation->dwInitialTimeStamp[iMediaIndex];
	stRTPRTCPComposerParam.wInitialSequenceNumber=pstRTSPServerSessionInformation->wInitialSequenceNumber[iMediaIndex];
	stRTPRTCPComposerParam.dwSSRC=pstRTSPServerSessionInformation->dwSSRC[iMediaIndex];

#ifdef RTSPRTP_MULTICAST	
    //20140106 modified by Charles for ondemand multicast
	if((pstRTSPServerSessionInformation->iMulticast > 0 &&
		pstRTSPServerSessionInformation->iMulticast	<= RTSP_MULTICASTNUMBER)
		|| (pstRTSPServerSessionInformation->iMulticast > RTSP_MULTICASTNUMBER &&
		pstRTSPServerSessionInformation->iMulticast <= RTSP_MULTICASTNUMBER + RTSP_ONDEMAND_MULTICASTNUMBER))
	{

		iMulticastIndex = pstRTSPServerSessionInformation->iMulticast-1;
		if( pRTSPStreaming->stMulticast[iMulticastIndex].iAlreadyMulticastMetadata == 0 &&
			pRTSPStreaming->stMulticast[iMulticastIndex].iAlwaysMulticast == 0)
		{
#ifdef _SHARED_MEM
			TShmemSessionInfo *pShmSessionInfo = (TShmemSessionInfo *)pRTSPStreaming->stMulticast[iMulticastIndex].hShmemSessionInfo;

			pShmSessionInfo->dwProtectedDelta = pstRTSPServerSessionInformation->dwProtectedDelta;
			pShmSessionInfo->dwBypasyMSec = pstRTSPServerSessionInformation->dwBypasyMSec;
			pShmSessionInfo->eHSMode = pstRTSPServerSessionInformation->eHSMode;
			//20100728 Modifed by danny For multiple channels videoin/audioin
			iTrackIndex = pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle,
					                    					pstRTSPServerSessionInformation->dwSessionID,
					                    					ccctGetMultipleChannelChannelIndex,
					                    					(DWORD)pstRTSPServerSessionInformation->iSDPIndex) - 1;
			//20130605 modified by Jimmy to support metadata event
			for ( i = 0; i < SHMEM_HANDLE_MAX_NUM; i++)
			{
				pShmSessionInfo->tShmemMetadataMediaInfo.ahShmemHandle[i] = pRTSPStreaming->ahShmemMetadataHandle[iTrackIndex][i];
			}

			//added by neil 10/12/30
			pShmSessionInfo->eSVCMode = pstRTSPServerSessionInformation->eSVCMode;
			pShmSessionInfo->eSVCTempMode = eSVCInvalid;
			printf("*********share memory SVC mode = %d**************\n", pShmSessionInfo->eSVCMode);
			//20120328 Marked by danny For audio shmem handle disordered
			//pShmSessionInfo->tShmemAudioMediaInfo.hShmemHandle = pRTSPStreaming->ahShmemAudioHandle[0];

			pShmSessionInfo->tShmemMetadataMediaInfo.bFrameGenerated = TRUE;
            //20140819 added by Charles for eventparser API
            pShmSessionInfo->tShmemMetadataMediaInfo.bGetNewData = TRUE;
            pShmSessionInfo->tShmemMetadataMediaInfo.bAnalytics = pstRTSPServerSessionInformation->bAnalytics;
            strncpy(pShmSessionInfo->tShmemMetadataMediaInfo.acVideoAnalyticsConfigToken, pstRTSPServerSessionInformation->acVideoAnalyticsConfigToken, RTSPSTREAMING_TOKEN_LENGTH-1);
#endif
			RTPRTCPComposer_Reset(pRTSPStreaming->stMulticast[iMulticastIndex].hRTPRTCPMetadataComposerHandle,&stRTPRTCPComposerParam);
//							stRTPRTCPSessionParam.dwSessionID=pstRTSPServerSessionInformation->dwSessionID;

			stRTPRTCPSessionParam.iRTPStreamingType = RTP_OVER_UDP;
			//20111205 Modified by danny For UDP mode socket leak
			stRTPRTCPSessionParam.psktRTP = &pRTSPStreaming->stMulticast[iMulticastIndex].aiMulticastSocket[4];
			stRTPRTCPSessionParam.psktRTCP = &pRTSPStreaming->stMulticast[iMulticastIndex].aiMulticastSocket[5];
			stRTPRTCPSessionParam.iVivotekClient = pRTSPStreaming->stMulticast[iMulticastIndex].iRTPExtension;
			//20110627 Add by danny for join/leave multicast group by session start/stop
			//20160127 Add by faber, metadata stream by using ulMulticastMetadataAddress
			stRTPRTCPSessionParam.ulMulticastAddress = pRTSPStreaming->stMulticast[iMulticastIndex].ulMulticastMetadataAddress;
			stRTPRTCPSessionParam.usMulticastRTCPPort = pRTSPStreaming->stMulticast[iMulticastIndex].usMulticastMetadataPort + 1;
			//20110725 Add by danny For Multicast RTCP receive report keep alive
			stRTPRTCPSessionParam.iRRAlive = pRTSPStreaming->stMulticast[iMulticastIndex].iRRAlive;
            //20141110 added by Charles for ONVIF Profile G
            stRTPRTCPSessionParam.iCseq = pstRTSPServerSessionInformation->iCseq;
			//stRTPRTCPSessionParam.iCodecIndex = pstRTSPServerSessionInformation->iSDPIndex;
			stRTPRTCPSessionParam.iCodecIndex = pRTSPStreaming->fMetadataCallBack((DWORD) pRTSPStreaming->hParentMetadataHandle,
						                                   MEDIA_CALLBACK_CHECK_CODEC_INDEX,
						                                   (void*)pstRTSPServerSessionInformation->iSDPIndex);
														   //(void*)pstRTSPServerSessionInformation->cMediaName[iMediaIndex]);
			stRTPRTCPSessionParam.dwSessionID =  pstRTSPServerSessionInformation->dwSessionID;
#ifdef _SHARED_MEM
			//20101018 Add by danny for support multiple channel text on video
			if( pstRTSPServerSessionInformation->iSDPIndex <= LIVE_STREAM_NUM)
			{
				stRTPRTCPSessionParam.iMultipleChannelChannelIndex = iTrackIndex + 1;
			}
#endif
			
			stRTPRTCPSessionParam.hRTPRTCPComposerHandle=pRTSPStreaming->stMulticast[iMulticastIndex].hRTPRTCPMetadataComposerHandle;
#ifdef _SHARED_MEM
			stRTPRTCPSessionParam.ptShmemMediaInfo = &pShmSessionInfo->tShmemMetadataMediaInfo;
#endif		
			iResult = RTPRTCPChannel_AddMulticastSession(pRTSPStreaming->hRTPRTCPChannelMetadataHandle, &stRTPRTCPSessionParam,pstRTSPServerSessionInformation->iMulticast);
			
			//20111205 Modified by danny For UDP mode socket leak
			printf("[RTSPStreamServer]:Add Metadata multicast channel ID:%ul Socket:%d\n",stRTPRTCPSessionParam.dwSessionID, *stRTPRTCPSessionParam.psktRTP); 
			pRTSPStreaming->stMulticast[iMulticastIndex].iAlreadyMulticastMetadata = 1;
			/* 20080828 marked out by Louis to prevent start channel 2 times
			if( pRTSPStreaming->iAudioSessionNumberOfStreamType[iMulticastIndex] == 0 )
			{
				TelnetShell_DbgPrint("[RTSP Streaming Control]: Audio of Streaming Type %d should start !!!\r\n",pstRTSPServerSessionInformation->iMulticast);
			    pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle,0,ccctStreamTypeAudioStart,pstRTSPServerSessionInformation->iMulticast);
			}*/
		}
		else
			iResult = 0;		
	}

	if( pstRTSPServerSessionInformation->iMulticast <= 0 )
#endif
	{
#ifdef _SHARED_MEM				
		TShmemSessionInfo *pShmSessionInfo = (TShmemSessionInfo *)pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].hShmemSessionInfo;

		pShmSessionInfo->dwProtectedDelta = pstRTSPServerSessionInformation->dwProtectedDelta;
		pShmSessionInfo->dwBypasyMSec = pstRTSPServerSessionInformation->dwBypasyMSec;
		pShmSessionInfo->eHSMode = pstRTSPServerSessionInformation->eHSMode;
		//20100727 Modify by danny to fix Rtsp server no stream MOD audio issue
		//pShmSessionInfo->tShmemAudioMediaInfo.hShmemHandle = pRTSPStreaming->ahShmemAudioHandle[0];
		if( pstRTSPServerSessionInformation->iSDPIndex > LIVE_STREAM_NUM)
		{
			//20100728 Modifed by danny For multiple channels videoin/audioin
			iTrackIndex = pstRTSPServerSessionInformation->iSDPIndex - LIVE_STREAM_NUM + (LIVE_METADATA_STREAM_NUM - 1);
			//20130605 modified by Jimmy to support metadata event
			for ( i = 0; i < SHMEM_HANDLE_MAX_NUM; i++)
			{
				pShmSessionInfo->tShmemMetadataMediaInfo.ahShmemHandle[i] = pRTSPStreaming->ahShmemMetadataHandle[0][i];
			}
		}
		else
		{	
			//20100728 Modifed by danny For multiple channels videoin/audioin
			iTrackIndex = pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle,
					                    					pstRTSPServerSessionInformation->dwSessionID,
					                    					ccctGetMultipleChannelChannelIndex,
					                    					(DWORD)pstRTSPServerSessionInformation->iSDPIndex) - 1;
			//20130605 modified by Jimmy to support metadata event
			for ( i = 0; i < SHMEM_HANDLE_MAX_NUM; i++)
			{
				pShmSessionInfo->tShmemMetadataMediaInfo.ahShmemHandle[i] = pRTSPStreaming->ahShmemMetadataHandle[iTrackIndex][i];
				//printf("Audio iTrackIndex %d hShmemHandle %p\n", iTrackIndex, pShmSessionInfo->tShmemAudioMediaInfo.hShmemHandle);
			}
		}
		//added by neil 10/12/30
		pShmSessionInfo->eSVCMode = pstRTSPServerSessionInformation->eSVCMode;
		pShmSessionInfo->eSVCTempMode = eSVCInvalid;
		printf("*********share memory SVC mode = %d**************\n", pShmSessionInfo->eSVCMode);
		//20120328 Marked by danny For audio shmem handle disordered
		//pShmSessionInfo->tShmemAudioMediaInfo.hShmemHandle = pRTSPStreaming->ahShmemAudioHandle[0];

		pShmSessionInfo->tShmemMetadataMediaInfo.bFrameGenerated = TRUE;
        //20140812 Added by Charles for mod no drop frame
        pShmSessionInfo->tShmemMetadataMediaInfo.bMediaOnDemand = pstRTSPServerSessionInformation->bMediaOnDemand;
        //20140819 added by Charles for eventparser API
        pShmSessionInfo->tShmemMetadataMediaInfo.bGetNewData = TRUE;
        pShmSessionInfo->tShmemMetadataMediaInfo.bAnalytics = pstRTSPServerSessionInformation->bAnalytics;
        strncpy(pShmSessionInfo->tShmemMetadataMediaInfo.acVideoAnalyticsConfigToken, pstRTSPServerSessionInformation->acVideoAnalyticsConfigToken, RTSPSTREAMING_TOKEN_LENGTH-1);
#endif
		RTPRTCPComposer_Reset(pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].hRTPRTCPMetadataComposerHandle,&stRTPRTCPComposerParam);

		stRTPRTCPSessionParam.dwSessionID=pstRTSPServerSessionInformation->dwSessionID;

		if( pstRTSPServerSessionInformation->iRTPStreamingType == RTP_OVER_UDP )
		{
			stRTPRTCPSessionParam.iRTPStreamingType = RTP_OVER_UDP;
			//20111205 Modified by danny For UDP mode socket leak
			stRTPRTCPSessionParam.psktRTP=pstRTSPServerSessionInformation->psktRTP[iMediaIndex];
			stRTPRTCPSessionParam.psktRTCP=pstRTSPServerSessionInformation->psktRTCP[iMediaIndex];
			memcpy((void*)&stRTPRTCPSessionParam.RTPNATAddr,(void*)&pstRTSPServerSessionInformation->NATRTPAddr[iMediaIndex],sizeof(RTSP_SOCKADDR));
			memcpy((void*)&stRTPRTCPSessionParam.RTCPNATAddr,(void*)&pstRTSPServerSessionInformation->NATRTCPAddr[iMediaIndex],sizeof(RTSP_SOCKADDR));
		}
		else
		{
			stRTPRTCPSessionParam.iRTPStreamingType = RTP_OVER_TCP;
			stRTPRTCPSessionParam.iEmbeddedRTPID = pstRTSPServerSessionInformation->iEmbeddedRTPID[iMediaIndex];
			stRTPRTCPSessionParam.iEmbeddedRTCPID = pstRTSPServerSessionInformation->iEmbeddedRTCPID[iMediaIndex];
			//20110706 Modified by danny For TCP/HTTP mode socket not sync in multi thread 
			stRTPRTCPSessionParam.psktRTSPSocket = pstRTSPServerSessionInformation->psktRTSPSocket;

			//20160602 add by Faber, set tcp mutex
			stRTPRTCPSessionParam.hTCPMuxCS = hTCPMuxCS;
		}
		stRTPRTCPSessionParam.iVivotekClient = pstRTSPServerSessionInformation->iVivotekClient;
        //20141110 added by Charles for ONVIF Profile G
        stRTPRTCPSessionParam.iCseq = pstRTSPServerSessionInformation->iCseq;
		stRTPRTCPSessionParam.iCodecIndex = pRTSPStreaming->fMetadataCallBack((DWORD) pRTSPStreaming->hParentMetadataHandle,
						                                   MEDIA_CALLBACK_CHECK_CODEC_INDEX,
														   (void*)pstRTSPServerSessionInformation->iSDPIndex);
														   //(void*)pstRTSPServerSessionInformation->cMediaName[iMediaIndex]);
		//stRTPRTCPSessionParam.iCodecIndex = pstRTSPServerSessionInformation->iSDPIndex;
		
#ifdef _SHARED_MEM
		//20101020 Add by danny for support seamless stream TCP/UDP timeout
		stRTPRTCPSessionParam.bSeamlessStream = pstRTSPServerSessionInformation->bSeamlessStream;
		//20101018 Add by danny for support multiple channel text on video
		if( pstRTSPServerSessionInformation->iSDPIndex <= LIVE_STREAM_NUM)
		{
			stRTPRTCPSessionParam.iMultipleChannelChannelIndex = iTrackIndex + 1;
		}
        else
        {
            //mod not support multiple channel yet, always set channel 1
            stRTPRTCPSessionParam.iMultipleChannelChannelIndex = 1;
        }
#endif

//		if(stRTPRTCPSessionParam.iVivotekClient != 0)
//			TelnetShell_DbgPrint("--[Control] RTPEX Player added to audio session %d--\r\n",stRTPRTCPSessionParam.iVivotekClient);

		stRTPRTCPSessionParam.hRTPRTCPComposerHandle=pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].hRTPRTCPMetadataComposerHandle;
#ifdef _SHARED_MEM
		stRTPRTCPSessionParam.ptShmemMediaInfo = &pShmSessionInfo->tShmemMetadataMediaInfo;
#endif		
		iResult= RTPRTCPChannel_AddOneSession(pRTSPStreaming->hRTPRTCPChannelMetadataHandle, &stRTPRTCPSessionParam);
	
		//20111205 Modified by danny For UDP mode socket leak
		DbgLog((dfCONSOLE|dfINTERNAL,"Add Metadata channel ID:%ul Socket:%d\n",stRTPRTCPSessionParam.dwSessionID, *stRTPRTCPSessionParam.psktRTP));   
	}

	return iResult;
}

//20140819 added by Charles for eventparser API
void RTSPStreaming_ParseMediaProfile(char *pcProfileFile, char *pcVideoEncoderConfigToken, TMetadataConfiguration *ptMetadataConfigFromProfile)
{
	int iProfileElem = 3; //only parse the needed element
	int iConfigMedia = 3; //only parse videoencoderconfiguration, videoanalytics, metadata config token
	int sReturn = 0, iIndex = 0, iLen = 0, iTokenLen = 0;
	char acConfigToken[MAX_NO_OF_PROFILES][iConfigMedia][RTSPSTREAMING_TOKEN_LENGTH];
	char acBuf[iProfileElem * MAX_NO_OF_PROFILES][256];
	TCfgParseMap tConfMap[iProfileElem * MAX_NO_OF_PROFILES + 1];
	
	iTokenLen = strlen(pcVideoEncoderConfigToken);
	if (iTokenLen <= 0)
    {        
		return;
    }   
	if (pcVideoEncoderConfigToken == NULL) 
    {
		return;
    }   
	
	memset(&tConfMap, 0, (iProfileElem * MAX_NO_OF_PROFILES + 1) * sizeof(TCfgParseMap));
	memset(acConfigToken, 0, sizeof(acConfigToken));
	
	// Parse VideoAnalytics & metadata config token from profile 
	for (iIndex = 0; iIndex < MAX_NO_OF_PROFILES; iIndex++)
	{
        snprintf( acBuf[0+iIndex*iProfileElem], sizeof(acBuf[0+iIndex*iProfileElem]) - 1, "/mediaconf/profile/i%d/videoencoderconfiguration/token", iIndex);
		tConfMap[0+iIndex*iProfileElem].pszXMLPath 	= acBuf[0+iIndex*iProfileElem];
		tConfMap[0+iIndex*iProfileElem].dwFlags		= citString|csmGetbyVal;
		tConfMap[0+iIndex*iProfileElem].dwSize		= RTSPSTREAMING_TOKEN_LENGTH;
		tConfMap[0+iIndex*iProfileElem].pParam		= acConfigToken[iIndex][0];
		tConfMap[0+iIndex*iProfileElem].pfnGetParam = NULL;

        snprintf( acBuf[1+iIndex*iProfileElem], sizeof(acBuf[1+iIndex*iProfileElem]) - 1, "/mediaconf/profile/i%d/videoanalyticsconfiguration/token", iIndex);
		tConfMap[1+iIndex*iProfileElem].pszXMLPath 	= acBuf[1+iIndex*iProfileElem];
		tConfMap[1+iIndex*iProfileElem].dwFlags		= citString|csmGetbyVal;
		tConfMap[1+iIndex*iProfileElem].dwSize		= RTSPSTREAMING_TOKEN_LENGTH;
		tConfMap[1+iIndex*iProfileElem].pParam		= acConfigToken[iIndex][1];
		tConfMap[1+iIndex*iProfileElem].pfnGetParam = NULL;
		
		snprintf( acBuf[2+iIndex*iProfileElem], sizeof(acBuf[2+iIndex*iProfileElem]) - 1, "/mediaconf/profile/i%d/metadataconfiguration/token", iIndex);
		tConfMap[2+iIndex*iProfileElem].pszXMLPath 	= acBuf[2+iIndex*iProfileElem];
		tConfMap[2+iIndex*iProfileElem].dwFlags		= citString|csmGetbyVal;
		tConfMap[2+iIndex*iProfileElem].dwSize		= RTSPSTREAMING_TOKEN_LENGTH;
		tConfMap[2+iIndex*iProfileElem].pParam		= acConfigToken[iIndex][2];
		tConfMap[2+iIndex*iProfileElem].pfnGetParam = NULL;
	}
	
	tConfMap[iProfileElem * MAX_NO_OF_PROFILES].pszXMLPath	= NULL;
	tConfMap[iProfileElem * MAX_NO_OF_PROFILES].dwFlags		= 0;
	tConfMap[iProfileElem * MAX_NO_OF_PROFILES].dwSize		= 0;
	tConfMap[iProfileElem * MAX_NO_OF_PROFILES].pParam		= NULL;
	tConfMap[iProfileElem * MAX_NO_OF_PROFILES].pfnGetParam	= NULL;
	
	sReturn = XMLSParser_ReadAll(pcProfileFile, tConfMap);
	printf("******GetVideoAnalyticsTokenFromProfile*********XMLSParser sReturn = %d\n",sReturn);
	
	// Get VideoAnalytics Token by metadata token
	for (iIndex = 0; iIndex < MAX_NO_OF_PROFILES; iIndex++)
	{
		iLen = strlen(acConfigToken[iIndex][0]); //videoencoderconfiguration token from profile
		if (iLen > 0)
		{
            printf("**compare : %s vs %s\n", acConfigToken[iIndex][0], pcVideoEncoderConfigToken);
            //Modify by Faber, multiple profiles go to one VideoEncoderConfigToken,
            // so it would continue search if the videoanalyticsconfiguration is set to be blank
			if ((iLen == iTokenLen) && (strncmp(acConfigToken[iIndex][0], pcVideoEncoderConfigToken, iLen) == 0) && strlen(acConfigToken[iIndex][1]))
			{
				strncpy(ptMetadataConfigFromProfile->tVideoAnalyticsConfigToken.acString, acConfigToken[iIndex][1], sizeof(ptMetadataConfigFromProfile->tVideoAnalyticsConfigToken.acString)-1);
                strncpy(ptMetadataConfigFromProfile->tEntity.tToken.acString, acConfigToken[iIndex][2], sizeof(ptMetadataConfigFromProfile->tEntity.tToken.acString)-1);
				printf("***Get videoanalytics token from profile : %s\n", ptMetadataConfigFromProfile->tVideoAnalyticsConfigToken.acString);
                printf("***Get metadataconfiguration token from profile : %s\n", ptMetadataConfigFromProfile->tEntity.tToken.acString);
				break;
			}
		}
	}

}

//20140819 added by Charles for eventparser API
SCODE RTSPStreaming_ParseMetadataConfiguration(TMetadataConfiguration **patMetadataConfiguration, TMetadataConfiguration *ptMetadataConfigFromProfile,int *iMetadataConfigNum, BOOL *pbMatchProfileToken)
{
	HANDLE 					hXMLProcessor = NULL;
	TMetadataConfiguration	*ptMetadataConfig;
	TXMLParams				tXmlParam;
	int						iMetadataNum, iMetadataTotalNum;
	char					szFileBuf[8192];
	DWORD					dwBufLen = sizeof(szFileBuf);
	
	memset(&tXmlParam, 0, sizeof(TXMLParams));
	tXmlParam.dwVersion = XMLPROCESSOR_VERSION;
	tXmlParam.iMaxElem = MAX_XML_ELEM_NUM;
	
	if(XML_Initial(&hXMLProcessor, &tXmlParam) != S_OK)
	{
		printf("[ONVIF_AS] :: %s : %d XML processor initialization failed!\n", __func__, __LINE__);
		return S_FAIL;
	}
	
	XML_ResetAll(hXMLProcessor);
	memset(szFileBuf, 0, dwBufLen);
	if (Onvifbasic_ReadFile(METADATA_CONF_FILE, szFileBuf, dwBufLen) == S_FAIL)
	{
		printf("[ONVIF_AS] :: %s : %d Read config file failed : %s!\n", __func__, __LINE__, METADATA_CONF_FILE);
		return S_FAIL;
	}
	
	if ((XML_SetBuffer(hXMLProcessor, szFileBuf, dwBufLen, dwBufLen)) != S_OK)
	{
		printf("XMLProcessor Set buffer failed! :: %s : %d\n", __func__, __LINE__);
		return S_FAIL;
	}
	
	Onvif_ParseChildIntLite(hXMLProcessor, "metadatamaxnumber", &iMetadataTotalNum);
	
	//allocate memory
	if((*patMetadataConfiguration = (TMetadataConfiguration*)calloc(iMetadataTotalNum, sizeof(TMetadataConfiguration))) == NULL)
	{
		printf("[debug]Unable to allocate memory for metadata configuration structure !\n");
		iMetadataConfigNum = 0;
		return S_FAIL;
	}
	
	ptMetadataConfig = *patMetadataConfiguration;
	iMetadataNum = 0;
	while (TRUE)
	{
		if (!XML_FindChildElem(hXMLProcessor, "metadataconfiguration"))
		{
			printf("End of parse metadata configuration.\n");
			break;
		}
		if (iMetadataNum >= iMetadataTotalNum)
		{
			printf("Metadata configuration occupy is full.\n");
			break;
		}
        if (Onvifbasic_Parse_MetadataConfiguration(hXMLProcessor, ptMetadataConfig) == S_OK)
        {
            if(strncmp(ptMetadataConfigFromProfile->tEntity.tToken.acString, ptMetadataConfig->tEntity.tToken.acString, RTSPSTREAMING_TOKEN_LENGTH) == 0)
            {
                strncpy(ptMetadataConfig->tVideoAnalyticsConfigToken.acString, ptMetadataConfigFromProfile->tVideoAnalyticsConfigToken.acString, RTSPSTREAMING_TOKEN_LENGTH-1);
                *pbMatchProfileToken = TRUE;
                break;
            }
            ptMetadataConfig++;
		    iMetadataNum++;
        }
        
	}
	
	if (hXMLProcessor)
	{
		XML_Release(&hXMLProcessor);
	}
	*iMetadataConfigNum = iMetadataNum;
    
    return S_OK;

}

//20140819 added by Charles for eventparser API
SCODE RTSPStreaming_CheckMetadataConfigFile(RTSPSTREAMING *pRTSPStreaming, RTSPSERVER_SESSIONINFORMATION * pstRTSPServerSessionInformation)
{
    TMetadataConfiguration	tMetadataConfigFromProfile;
    TMetadataConfiguration  *ptMetadataConfig = NULL;
    int						iMetadataConfigNum;
    int                     iChannelIndex;
	BOOL					bMatchProfileToken = FALSE;
    int                     iSDPIndex = pstRTSPServerSessionInformation->iSDPIndex - 1;
    char                    acVideoEncoderConfigToken[RTSPSTREAMING_TOKEN_LENGTH];
    if (pRTSPStreaming == NULL)
    {
        return S_FAIL;
    }
	memset((void*)&tMetadataConfigFromProfile, 0, sizeof(TMetadataConfiguration));
    memset((void*)acVideoEncoderConfigToken, 0, sizeof(acVideoEncoderConfigToken));

    iChannelIndex = pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle,
														0,
														ccctGetMultipleChannelChannelIndex,
														(DWORD)iSDPIndex) -1;
   
    snprintf(acVideoEncoderConfigToken, RTSPSTREAMING_TOKEN_LENGTH, "VideoEncoderConfiguration%d_%d",iChannelIndex, iSDPIndex);
    RTSPStreaming_ParseMediaProfile(MEDIA_PROFILE_FILE, acVideoEncoderConfigToken, &tMetadataConfigFromProfile);
    if (RTSPStreaming_ParseMetadataConfiguration(&ptMetadataConfig, &tMetadataConfigFromProfile, &iMetadataConfigNum, &bMatchProfileToken) == S_OK)
	{
		if(bMatchProfileToken)
		{
			strncpy(pstRTSPServerSessionInformation->acVideoAnalyticsConfigToken, ptMetadataConfig[iMetadataConfigNum].tVideoAnalyticsConfigToken.acString, RTSPSTREAMING_TOKEN_LENGTH-1);
        	pstRTSPServerSessionInformation->bAnalytics = ptMetadataConfig[iMetadataConfigNum].bAnalytics;
            printf("match profile token!: bAnalytics=%d, acVideoAnalyticsConfigToken=%s\n",
                   pstRTSPServerSessionInformation->bAnalytics, pstRTSPServerSessionInformation->acVideoAnalyticsConfigToken);
		}
		else
		{
			pstRTSPServerSessionInformation->bAnalytics = FALSE;
            printf("no match profile token!\n");
		}

		// free metadataconfiguration
		free(ptMetadataConfig);	
        return S_OK;
    }

    return S_FAIL;
}

int	DeleteOneSessionFromList(RTSPSTREAMING *pRTSPStreaming,DWORD dwSessionID)
{
	int		i;
	HANDLE	hTemp;
#ifndef _SHARED_MEM
	int iResult;
	RTPMEDIABUFFER	*pMediaBuffer;
#endif
	for( i = 0; i < pRTSPStreaming->iSessionListNumber ; i++)
	{
		if( pRTSPStreaming->pstSessionList[i].dwSessionID == dwSessionID )
		{
			//printf("Stop one session, IP=%s  SessionID=%u \n", inet_ntoa(*((struct in_addr*)&(pRTSPStreaming->pstSessionList[i].ulClientIP))),dwSessionID);   		
#ifdef _LINUX
#ifdef _INET6
			//20101123 Added by danny For support advanced system log 
			if (pRTSPStreaming->pstSessionList[i].ulClientIP == 0)
			{
				char szPresentString[64]="";
				if ( pRTSPStreaming->bAdvLogSupport == TRUE )
				{
					openlog("[RTSP SERVER]",0, LOG_LOCAL0);
					syslog(LOG_INFO,"Stop one session, IP=%s\n", inet_ntop(AF_INET6, &pRTSPStreaming->pstSessionList[i].tClientSckAddr.sin6_addr, szPresentString, sizeof(szPresentString)));
					openlog("[RTSP SERVER]",0, LOG_USER);
				}
				else
				{
					syslog(LOG_INFO,"Stop one session, IP=%s\n", inet_ntop(AF_INET6, &pRTSPStreaming->pstSessionList[i].tClientSckAddr.sin6_addr, szPresentString, sizeof(szPresentString)));
				}
			}
			else
#endif
			{
				if ( pRTSPStreaming->bAdvLogSupport == TRUE )
				{
					openlog("[RTSP SERVER]",0, LOG_LOCAL0);
					syslog(LOG_INFO,"Stop one session, IP=%s\n", inet_ntoa(*((struct in_addr*)&(pRTSPStreaming->pstSessionList[i].ulClientIP))));
					openlog("[RTSP SERVER]",0, LOG_USER);
				}
				else
				{
					syslog(LOG_INFO,"Stop one session, IP=%s\n", inet_ntoa(*((struct in_addr*)&(pRTSPStreaming->pstSessionList[i].ulClientIP))));
				}
			}
#endif
			pRTSPStreaming->pstSessionList[i].dwSessionID=pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber-1].dwSessionID;
			pRTSPStreaming->pstSessionList[i].ulClientIP=pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber-1].ulClientIP;	
#ifdef _INET6
			memcpy(&pRTSPStreaming->pstSessionList[i].tClientSckAddr,&pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber-1].tClientSckAddr, sizeof(RTSP_SOCKADDR));
#endif
#ifdef RTSPRTP_MULTICAST
			//20110215 Added by danny to fix Backchannel multicast session terminated, iMulticast not sync
			pRTSPStreaming->pstSessionList[i].iMulticast=pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber-1].iMulticast;
#endif
			pRTSPStreaming->pstSessionList[i].byMediaStatus=pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber-1].byMediaStatus;
			pRTSPStreaming->pstSessionList[i].iSDPIndex = pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber-1].iSDPIndex;
			pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber-1].byMediaStatus = RTSPSTREAMING_NO_MEDIA;

			hTemp=pRTSPStreaming->pstSessionList[i].hRTPRTCPAudioComposerHandle;
			pRTSPStreaming->pstSessionList[i].hRTPRTCPAudioComposerHandle=pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber-1].hRTPRTCPAudioComposerHandle;
			pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber-1].hRTPRTCPAudioComposerHandle=hTemp;

			hTemp=pRTSPStreaming->pstSessionList[i].hRTPRTCPVideoComposerHandle;
			pRTSPStreaming->pstSessionList[i].hRTPRTCPVideoComposerHandle=pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber-1].hRTPRTCPVideoComposerHandle;
			pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber-1].hRTPRTCPVideoComposerHandle=hTemp;

			//20120806 added by Jimmy for metadata
			hTemp=pRTSPStreaming->pstSessionList[i].hRTPRTCPMetadataComposerHandle;
			pRTSPStreaming->pstSessionList[i].hRTPRTCPMetadataComposerHandle=pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber-1].hRTPRTCPMetadataComposerHandle;
			pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber-1].hRTPRTCPMetadataComposerHandle=hTemp;

#ifdef _SHARED_MEM
			hTemp = pRTSPStreaming->pstSessionList[i].hShmemSessionInfo;
			pRTSPStreaming->pstSessionList[i].hShmemSessionInfo = pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber-1].hShmemSessionInfo;
			pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber-1].hShmemSessionInfo = hTemp;
#endif
			pRTSPStreaming->iSessionListNumber--;
			printf("========Remove one session for RTSPServer callback, %d remains========\n",pRTSPStreaming->iSessionListNumber);
#ifdef _LINUX					
			syslog(LOG_DEBUG,"Remove one session, %d remains !\n",pRTSPStreaming->iSessionListNumber);
#endif	
			//20120806 modified by Jimmy for metadata
			printf("Total Session %d Type1: video %d audio %d metadata %d Type2: video %d audio %d metadata %d\r\n"
				                        ,pRTSPStreaming->iSessionListNumber
				                        ,pRTSPStreaming->iVideoSessionNumberOfStreamType[0]
				                        ,pRTSPStreaming->iAudioSessionNumberOfStreamType[0]
				                        ,pRTSPStreaming->iMetadataSessionNumberOfStreamType[0]
				                        ,pRTSPStreaming->iVideoSessionNumberOfStreamType[1]
				                        ,pRTSPStreaming->iAudioSessionNumberOfStreamType[1]
				                        ,pRTSPStreaming->iMetadataSessionNumberOfStreamType[1]);
#ifdef _LINUX
			//20120806 modified by Jimmy for metadata
			syslog(LOG_DEBUG,"Total Session %d Type1: video %d audio %d metadata %d Type2: video %d audio %d metadata %d\n"
				                        ,pRTSPStreaming->iSessionListNumber
				                        ,pRTSPStreaming->iVideoSessionNumberOfStreamType[0]
				                        ,pRTSPStreaming->iAudioSessionNumberOfStreamType[0]
				                        ,pRTSPStreaming->iMetadataSessionNumberOfStreamType[0]
				                        ,pRTSPStreaming->iVideoSessionNumberOfStreamType[1]
				                        ,pRTSPStreaming->iAudioSessionNumberOfStreamType[1]
				                        ,pRTSPStreaming->iMetadataSessionNumberOfStreamType[1]);
#endif
#ifndef _SHARED_MEM
			if(pRTSPStreaming->iSessionListNumber==0)
			{
				// send a audio, video and metadata databuffer to empty buffer
				iResult=0;
				pMediaBuffer=NULL;
				iResult=MediaBufQueue_GetMediaBuffer(pRTSPStreaming->hVideoDataQueueHandle,0,(void **)&pMediaBuffer);
				if(	iResult==0 && pMediaBuffer )
				{
					MediaBufQueue_AddMediaBuffer(pRTSPStreaming->hVideoEmptyQueueHandle,pMediaBuffer);
				}						

				iResult=0;
				pMediaBuffer=NULL;
				iResult=MediaBufQueue_GetMediaBuffer(pRTSPStreaming->hAudioDataQueueHandle,0,(void **)&pMediaBuffer);
				if(	iResult==0 && pMediaBuffer )
				{
					MediaBufQueue_AddMediaBuffer(pRTSPStreaming->hAudioEmptyQueueHandle,pMediaBuffer);
				}

				//20120806 added by Jimmy for metadata
				iResult=0;
				pMediaBuffer=NULL;
				iResult=MediaBufQueue_GetMediaBuffer(pRTSPStreaming->hMetadataDataQueueHandle,0,(void **)&pMediaBuffer);
				if( iResult==0 && pMediaBuffer )
				{
					MediaBufQueue_AddMediaBuffer(pRTSPStreaming->hMetadataEmptyQueueHandle,pMediaBuffer);
				}
				return 0;
			}		
#endif
		}
	}

	return 0;
}

int RTSPStreaming_RemoveSessionListBySessionID(RTSPSTREAMING *pRTSPStreaming, DWORD dwSessionID,int iMediaType)
{
	int	i;
	HANDLE	hTemp;

#ifndef _SHARED_MEM
	int iResult;
	RTPMEDIABUFFER	*pMediaBuffer;
#endif
	printf("%s \n", __func__);
	for( i = 0; i < pRTSPStreaming->iSessionListNumber ; i++)
	{
		if( pRTSPStreaming->pstSessionList[i].dwSessionID == dwSessionID )
		{	
			//20120806 modified by Jimmy for metadata
			if( ( pRTSPStreaming->pstSessionList[i].byMediaStatus & RTSPSTREAMING_MEDIATYPE_VIDEO && iMediaType == RTSPSTREAMING_MEDIATYPE_VIDEO) ||
				( pRTSPStreaming->pstSessionList[i].byMediaStatus & RTSPSTREAMING_MEDIATYPE_AUDIO && iMediaType == RTSPSTREAMING_MEDIATYPE_AUDIO) ||
				( pRTSPStreaming->pstSessionList[i].byMediaStatus & RTSPSTREAMING_MEDIATYPE_METADATA && iMediaType == RTSPSTREAMING_MEDIATYPE_METADATA) )
			{
				if(iMediaType == RTSPSTREAMING_MEDIATYPE_VIDEO ) 
				{
					pRTSPStreaming->iVideoSessionNumberOfStreamType[pRTSPStreaming->pstSessionList[i].iSDPIndex-1]--;
					if(pRTSPStreaming->iVideoSessionNumberOfStreamType[pRTSPStreaming->pstSessionList[i].iSDPIndex-1] == 0)
				    {
				    	TelnetShell_DbgPrint("[%s]: Video of Streaming Type %d should stop !!!\r\n", __FUNCTION__, pRTSPStreaming->pstSessionList[i].iSDPIndex);
					    pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle,0,ccctStreamTypeVideoStop,pRTSPStreaming->pstSessionList[i].iSDPIndex);
				    }
				}
				else if(iMediaType == RTSPSTREAMING_MEDIATYPE_AUDIO)
				{
					pRTSPStreaming->iAudioSessionNumberOfStreamType[pRTSPStreaming->pstSessionList[i].iSDPIndex-1]--;
					if(pRTSPStreaming->iAudioSessionNumberOfStreamType[pRTSPStreaming->pstSessionList[i].iSDPIndex-1] == 0)
				    {
					    TelnetShell_DbgPrint("[%s]: Audio of Streaming Type %d should stop !!!\r\n", __FUNCTION__, pRTSPStreaming->pstSessionList[i].iSDPIndex);
					    pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle,0,ccctStreamTypeAudioStop,pRTSPStreaming->pstSessionList[i].iSDPIndex);
				    }
				}
				//20120806 added by Jimmy for metadata
				else if(iMediaType == RTSPSTREAMING_MEDIATYPE_METADATA)
				{
					pRTSPStreaming->iMetadataSessionNumberOfStreamType[pRTSPStreaming->pstSessionList[i].iSDPIndex-1]--;
					if(pRTSPStreaming->iMetadataSessionNumberOfStreamType[pRTSPStreaming->pstSessionList[i].iSDPIndex-1] == 0)
				    {
					    TelnetShell_DbgPrint("[%s]: Metadata of Streaming Type %d should stop !!!\r\n", __FUNCTION__, pRTSPStreaming->pstSessionList[i].iSDPIndex);
					    pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle,0,ccctStreamTypeMetadataStop,pRTSPStreaming->pstSessionList[i].iSDPIndex);
				    }
				}

				pRTSPStreaming->pstSessionList[i].byMediaStatus ^= iMediaType;

				if ( pRTSPStreaming->pstSessionList[i].byMediaStatus != RTSPSTREAMING_NO_MEDIA)
				{
					return 0;
				}

				//printf("Stop one session, IP=%s  SessionID=%u \n", inet_ntoa(*((struct in_addr*)&(pRTSPStreaming->pstSessionList[i].ulClientIP))),dwSessionID);			
#ifdef _LINUX
#ifdef _INET6
				//20101123 Added by danny For support advanced system log 
				if (pRTSPStreaming->pstSessionList[i].ulClientIP == 0)
				{
					char szPresentString[64]="";
					if ( pRTSPStreaming->bAdvLogSupport == TRUE )
					{
						openlog("[RTSP SERVER]",0, LOG_LOCAL0);
						syslog(LOG_INFO,"Stop one session, IP=%s\n", inet_ntop(AF_INET6, &pRTSPStreaming->pstSessionList[i].tClientSckAddr.sin6_addr, szPresentString, sizeof(szPresentString)));
						openlog("[RTSP SERVER]",0, LOG_USER);
					}
					else
					{
						syslog(LOG_INFO,"Stop one session, IP=%s\n", inet_ntop(AF_INET6, &pRTSPStreaming->pstSessionList[i].tClientSckAddr.sin6_addr, szPresentString, sizeof(szPresentString)));
					}
				}
				else
#endif
				{
					if ( pRTSPStreaming->bAdvLogSupport == TRUE )
					{
						openlog("[RTSP SERVER]",0, LOG_LOCAL0);
						syslog(LOG_INFO,"Stop one session, IP=%s\n", inet_ntoa(*((struct in_addr*)&(pRTSPStreaming->pstSessionList[i].ulClientIP))));
						openlog("[RTSP SERVER]",0, LOG_USER);
					}
					else
					{
						syslog(LOG_INFO,"Stop one session, IP=%s\n", inet_ntoa(*((struct in_addr*)&(pRTSPStreaming->pstSessionList[i].ulClientIP))));
					}
				}

#endif
#ifdef _SHARED_MEM
				//20100105 Added For Seamless Recording, 20100316 add define by Louis to avoid non-sharememory build fail
				RTSPServer_SetLastFrameTimeForAVStream(pRTSPStreaming->hRTSPServerHandle, pRTSPStreaming->pstSessionList[i].hShmemSessionInfo, dwSessionID);
				//20101208 Modified by danny For GUID format change
				//pRTSPStreaming->pstSessionList[i].dwSessionGUID=pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber-1].dwSessionGUID;
				strncpy(pRTSPStreaming->pstSessionList[i].acSeamlessRecordingGUID, pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber-1].acSeamlessRecordingGUID, RTSPS_Seamless_Recording_GUID_LENGTH - 1);
				pRTSPStreaming->pstSessionList[i].acSeamlessRecordingGUID[RTSPS_Seamless_Recording_GUID_LENGTH - 1] = 0;
#endif

				pRTSPStreaming->pstSessionList[i].dwSessionID=pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber-1].dwSessionID;
				pRTSPStreaming->pstSessionList[i].ulClientIP=pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber-1].ulClientIP;	
#ifdef _INET6
				memcpy(&pRTSPStreaming->pstSessionList[i].tClientSckAddr,&pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber-1].tClientSckAddr, sizeof(RTSP_SOCKADDR));
#endif
#ifdef RTSPRTP_MULTICAST
				//20100720 Added by danny to fix Backchannel multicast session terminated, rtsp server has not stopped sending video/audio RTP/RTCP
				pRTSPStreaming->pstSessionList[i].iMulticast=pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber-1].iMulticast;
#endif
				pRTSPStreaming->pstSessionList[i].byMediaStatus=pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber-1].byMediaStatus;
				pRTSPStreaming->pstSessionList[i].iSDPIndex = pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber-1].iSDPIndex;
				pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber-1].byMediaStatus = RTSPSTREAMING_NO_MEDIA;

				hTemp=pRTSPStreaming->pstSessionList[i].hRTPRTCPAudioComposerHandle;
				pRTSPStreaming->pstSessionList[i].hRTPRTCPAudioComposerHandle=pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber-1].hRTPRTCPAudioComposerHandle;
				pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber-1].hRTPRTCPAudioComposerHandle=hTemp;

				hTemp=pRTSPStreaming->pstSessionList[i].hRTPRTCPVideoComposerHandle;
				pRTSPStreaming->pstSessionList[i].hRTPRTCPVideoComposerHandle=pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber-1].hRTPRTCPVideoComposerHandle;
				pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber-1].hRTPRTCPVideoComposerHandle=hTemp;

				//20120806 added by Jimmy for metadata
				hTemp=pRTSPStreaming->pstSessionList[i].hRTPRTCPMetadataComposerHandle;
				pRTSPStreaming->pstSessionList[i].hRTPRTCPMetadataComposerHandle=pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber-1].hRTPRTCPMetadataComposerHandle;
				pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber-1].hRTPRTCPMetadataComposerHandle=hTemp;

#ifdef _SHARED_MEM
				hTemp = pRTSPStreaming->pstSessionList[i].hShmemSessionInfo;
				pRTSPStreaming->pstSessionList[i].hShmemSessionInfo = pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber-1].hShmemSessionInfo;
				pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber-1].hShmemSessionInfo = hTemp;
#endif
				pRTSPStreaming->iSessionListNumber--;

				printf("========Remove one session from RTPRTCP callback, %d remains========\n",pRTSPStreaming->iSessionListNumber);

#ifdef _LINUX					
				syslog(LOG_DEBUG,"Remove one session, %d remains !\n",pRTSPStreaming->iSessionListNumber);
#endif					
				//20120806 modified by Jimmy for metadata
				printf("Total Session %d Type1: video %d audio %d metadata %d Type2: video %d audio %d metadata %d\r\n"
										,pRTSPStreaming->iSessionListNumber
										,pRTSPStreaming->iVideoSessionNumberOfStreamType[0]
										,pRTSPStreaming->iAudioSessionNumberOfStreamType[0]
										,pRTSPStreaming->iMetadataSessionNumberOfStreamType[0]
										,pRTSPStreaming->iVideoSessionNumberOfStreamType[1]
										,pRTSPStreaming->iAudioSessionNumberOfStreamType[1]
										,pRTSPStreaming->iMetadataSessionNumberOfStreamType[1]);
#ifdef _LINUX
				//20120806 modified by Jimmy for metadata
				syslog(LOG_DEBUG,"Total Session %d Type1: video %d audio %d metadata %d Type2: video %d audio %d metadata %d\n"
										,pRTSPStreaming->iSessionListNumber
										,pRTSPStreaming->iVideoSessionNumberOfStreamType[0]
										,pRTSPStreaming->iAudioSessionNumberOfStreamType[0]
										,pRTSPStreaming->iMetadataSessionNumberOfStreamType[0]
										,pRTSPStreaming->iVideoSessionNumberOfStreamType[1]
										,pRTSPStreaming->iAudioSessionNumberOfStreamType[1]
										,pRTSPStreaming->iMetadataSessionNumberOfStreamType[1]);
#endif
										

			}
		}
	}

	return 0;
}


int RTSPStreaming_RemoveRTPRTCPSessionBySessionID(RTSPSTREAMING *pRTSPStreaming, DWORD dwSessionID)
{
	int i, iResult = -1;

	for(i=0; i<pRTSPStreaming->iSessionListNumber; i++)
	{
		if( pRTSPStreaming->pstSessionList[i].dwSessionID == dwSessionID )
		{
			//20120806 modified by Jimmy for metadata
			if( pRTSPStreaming->pstSessionList[i].byMediaStatus & RTSPSTREAMING_MEDIATYPE_AUDIO)
			{
				iResult=RTPRTCPChannel_RemoveOneSession(pRTSPStreaming->hRTPRTCPChannelAudioHandle, dwSessionID);													    
    			if( iResult != 0 )
	    		{
			    	return -1;	
				}    
			}	

			//20120806 modified by Jimmy for metadata
			if( pRTSPStreaming->pstSessionList[i].byMediaStatus & RTSPSTREAMING_MEDIATYPE_VIDEO)
			{
				iResult=RTPRTCPChannel_RemoveOneSession(pRTSPStreaming->hRTPRTCPChannelVideoHandle, dwSessionID);						
				if( iResult != 0 )
				{
					return -1;		
				}    
			}

			//20120806 added by Jimmy for metadata
			if( pRTSPStreaming->pstSessionList[i].byMediaStatus & RTSPSTREAMING_MEDIATYPE_METADATA)
			{
				iResult=RTPRTCPChannel_RemoveOneSession(pRTSPStreaming->hRTPRTCPChannelMetadataHandle, dwSessionID);						
				if( iResult != 0 )
				{
					return -1;		
				}    
			}			
			return 0;
		}	
	}

	return -1;
}

// in host order
int RTSPStreaming_CheckClientIPDomainInHostOrder(unsigned long ulLocalIP, unsigned long ulLocalSubnetMask, unsigned long ulClientIP)
{
	unsigned long	ulLocalIPUpperBound;
	unsigned long	ulLocalIPLowerBound;
	
	ulLocalIPLowerBound=ulLocalIP&ulLocalSubnetMask;	
	ulLocalIPUpperBound=ulLocalIPLowerBound|(~ulLocalSubnetMask);

	if( ulClientIP > ulLocalIPUpperBound || ulClientIP < ulLocalIPLowerBound )
	{
		// Client in different domain
		return -1;	
	}	
	else
	{
		//Client in same domain
		return 0;
	}	
}

#ifdef _SIP

void RTSPStreaming_SIPFillSessionInfo(RTSPSTREAMING	*pRTSPStreaming,
									  TSIPUASessionInfo *ptSIPSessionInfo,
									  RTSPSERVER_SESSIONINFORMATION *ptRTPSessionInfo)
{
	int i;
	DWORD dwMilliSecond;
	struct sockaddr_in sockaddr;

	ptRTPSessionInfo->dwSessionID = ptSIPSessionInfo->dwSessionID;
	ptRTPSessionInfo->iRTPStreamingType = RTP_OVER_UDP;
	ptRTPSessionInfo->iSDPIndex = 1;

	for(i=0 ; i<2 ; i++)
	{
		OSTick_GetMSec(&dwMilliSecond);
		ptRTPSessionInfo->dwSSRC[i] = dwMilliSecond;
	}

	if( ptSIPSessionInfo->usVideoPort > 0 )
	{
		//20111205 Modified by danny For UDP mode socket leak
		*ptRTPSessionInfo->psktRTP[0] = -1;
		*ptRTPSessionInfo->psktRTCP[0] = -1;

		memset(&sockaddr,0,sizeof(struct sockaddr_in));
		sockaddr.sin_family = AF_INET;
		sockaddr.sin_addr.s_addr =ptSIPSessionInfo->ulClientIP;
		sockaddr.sin_port = htons(ptSIPSessionInfo->usVideoPort);
		memcpy((void*)&ptRTPSessionInfo->NATRTPAddr[0],(void*)&sockaddr,sizeof(sockaddr));
		memset(&sockaddr,0,sizeof(struct sockaddr_in));
		sockaddr.sin_family = AF_INET;
		sockaddr.sin_addr.s_addr =ptSIPSessionInfo->ulClientIP;
		sockaddr.sin_port =  htons(ptSIPSessionInfo->usVideoPort+1);
		memcpy((void*)&ptRTPSessionInfo->NATRTCPAddr[0],(void*)&sockaddr,sizeof(sockaddr));
	}

	if( ptSIPSessionInfo->usAudioPort > 0 )
	{
		//20111205 Modified by danny For UDP mode socket leak
		*ptRTPSessionInfo->psktRTP[1] = -1;
		*ptRTPSessionInfo->psktRTCP[1] = -1;

		memset(&sockaddr,0,sizeof(struct sockaddr_in));
		sockaddr.sin_family = AF_INET;
		sockaddr.sin_addr.s_addr =ptSIPSessionInfo->ulClientIP;
		sockaddr.sin_port = htons(ptSIPSessionInfo->usAudioPort);
		memcpy((void*)&ptRTPSessionInfo->NATRTPAddr[1],(void*)&sockaddr,sizeof(sockaddr));        	
		memset(&sockaddr,0,sizeof(struct sockaddr_in));
		sockaddr.sin_family = AF_INET;
		sockaddr.sin_addr.s_addr =ptSIPSessionInfo->ulClientIP;
		sockaddr.sin_port =  htons(ptSIPSessionInfo->usAudioPort+1);
		memcpy((void*)&ptRTPSessionInfo->NATRTCPAddr[1],(void*)&sockaddr,sizeof(sockaddr));
	}
	
}

int RTSPStreaming_SIPUAServerCallback(DWORD dwCallbackInstance, ESIPUACallbackType eCallbackType, void * pvParam1, void * pvParam2)
{	
	RTSPSTREAMING	*pRTSPStreaming;
	TSIPUASDPInfo	*ptSDPRequest; 
	TSIPUASessionInfo *ptSIPSessionInfo;
	RTSPSERVER_SESSIONINFORMATION tRTPSessionInfo;
	TMULTICASTINFO	tSIPUAInfo;
	int				iResult,iStreamMode;

#ifdef _SIP_TWO_WAY_AUDIO
	TCtrlChCodecInfo	tAudioCodecInfo;
	TAUDIOFORMATLIST 	tAudioCodecFormat;
#endif

	pRTSPStreaming=(RTSPSTREAMING *)dwCallbackInstance;		

	switch(eCallbackType)
	{
		case SIPUA_CALLBACKFLAG_FORCE_TURNOFF:
			break;
		
		case SIPUA_CALLBACKFLAG_SDP_REQUEST:
			ptSDPRequest = (TSIPUASDPInfo*) pvParam1;
			
			SDPDecoder_Reset(pRTSPStreaming->hSDPDecoder);

			if( (iResult = SDPDecoder_Decode(ptSDPRequest->pcSDPBuffer,pRTSPStreaming->hSDPDecoder,SDPFromMem)) != S_OK)
			{
				printf("[StreamingServer(SIP)]: Decode SDP failed %x\r\n",iResult);
				return -1;
			}
			
			memset(&tSIPUAInfo,0,sizeof(TMULTICASTINFO));
			//SDP of SIP is kinda like mulitcast SDP
			if( SDPDeocder_Multicast(pRTSPStreaming->hSDPDecoder, &tSIPUAInfo) != S_OK )
				return -1;

			if( tSIPUAInfo.dwVideoAddress != 0 )
				// Modified by Jeffrey 2006/12/13
				//ptSDPRequest->ulClientIP = tSIPUAInfo.dwVideoAddress;
				ptSDPRequest->ulPeerIP = tSIPUAInfo.dwVideoAddress;
			else
				// Modified by Jeffrey 2006/12/13
				//ptSDPRequest->ulClientIP = tSIPUAInfo.dwAudioAddress;
				ptSDPRequest->ulPeerIP = tSIPUAInfo.dwAudioAddress;

			ptSDPRequest->usAudioPort = tSIPUAInfo.usAudioPort;
			ptSDPRequest->usVideoPort = tSIPUAInfo.usVideoPort;

			// Modified by Jeffrey 2006/12/13
			//if( (ptSDPRequest->ulClientIP == 0) ||
			if( (ptSDPRequest->ulPeerIP == 0) ||
				((ptSDPRequest->usAudioPort == 0) && (ptSDPRequest->usVideoPort == 0) ))
				return -1;

			//20120830 modified by Jimmy for metadata
			iStreamMode = RTSPSTREAMING_NO_MEDIA;
			if( ptSDPRequest->usVideoPort > 0 )
				iStreamMode |= RTSPSTREAMING_MEDIATYPE_VIDEO;
			if( ptSDPRequest->usAudioPort > 0 )
				iStreamMode |= RTSPSTREAMING_MEDIATYPE_AUDIO;
				
			/*
			if( ptSDPRequest->usAudioPort == 0 )
				iStreamMode = RTSPSTREAMING_MEDIATYPE_VIDEOONLY;
			else if( ptSDPRequest->usVideoPort == 0 )
				iStreamMode = RTSPSTREAMING_MEDIATYPE_AUDIOONLY;
			else
				iStreamMode = RTSPSTREAMING_MEDIATYPE_AUDIOVIDEO;
			*/

#ifdef _SIP_TWO_WAY_AUDIO 
			//callback audio upload stream SDP info
			memset((void*)&tAudioCodecInfo,0,sizeof(TCtrlChCodecInfo));

			if (SDPDecoder_VerifyAudioCodec(pRTSPStreaming->hSDPDecoder,"pcmu", &tAudioCodecFormat)==S_OK)
			{
				tAudioCodecInfo.eCodecType = mctG711U;
				/*tAudioCodecInfo.wSamplingFreq = tAudioCodecFormat.ulSampleRate;
				tAudioCodecInfo.wChannelNumber = tAudioCodecFormat.usChannel;*/
			}
			else if (SDPDecoder_VerifyAudioCodec(pRTSPStreaming->hSDPDecoder,"pcma", &tAudioCodecFormat)==S_OK)
			{
				tAudioCodecInfo.eCodecType = mctG711A;
				/*tAudioCodecInfo.wSamplingFreq = tAudioCodecFormat.ulSampleRate;
				tAudioCodecInfo.wChannelNumber = tAudioCodecFormat.usChannel;*/
			}
			else
			{
				printf("Found no supported Audio Codec!\n");
				return -1;
			}

			tAudioCodecInfo.wSamplingFreq = tAudioCodecFormat.ulSampleRate;
			tAudioCodecInfo.wChannelNumber = tAudioCodecFormat.usChannel;
			
			//20100708 Added by Danny for optional channels output data
			tAudioCodecInfo.dwChannelBitmap = ptSDPRequest->dwChannelBitmap;
			
			pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle
											,0
											,ccctAudioUploadSDPInfo
											,(DWORD)&tAudioCodecInfo);

			{
				int iDecodeLen = 30;
				char acSSRC[30],acDecode[30];
				unsigned long ulSSRC = 0;

#ifdef _SIP_VVTKONLY
				ERECVSEND eDirection;

				iResult = SDPDecoder_GetRecvSendDirection(pRTSPStreaming->hSDPDecoder, eAudio, &eDirection);
				if( (iResult != S_OK) || (eDirection != eSendonly ))
				{
					return -1;
				}
#endif //_SIP_VVTKONLY

				if (SDPDeocder_SessionID(pRTSPStreaming->hSDPDecoder, acSSRC, 30) == S_OK)
				{
					if (VIVOStrCodec_Decode(acSSRC, strlen(acSSRC), acDecode, &iDecodeLen) > 0)
					{
#ifdef _LINUX
						ulSSRC = atoll(acDecode); 
#else
						ulSSRC = atol(acDecode); 
#endif
					}
				}

				if( ulSSRC != 0 )
				{
					RTSPServer_SetSSRCForUpStreamAudio(pRTSPStreaming->hRTSPServerHandle,ulSSRC);
				}
				else
				{
#ifdef _SIP_VVTKONLY
					printf("SSRC error of Upstream Audio!\n");
					return -1;
#else
					RTSPServer_SetSSRCForUpStreamAudio(pRTSPStreaming->hRTSPServerHandle, SIP_2WAYAUDIO_OTHERVENDOR_SSRC);
#endif //_SIP_VVTKONLY

				}
			}
#endif //_SIP_TWO_WAY_AUDIO 
			printf("[SIP control]: SIP Client IP %s Video port %d , audio port %d\r\n",inet_ntoa(*((struct in_addr*)&(ptSDPRequest->ulPeerIP))), ptSDPRequest->usVideoPort,ptSDPRequest->usAudioPort);

#ifdef _SIP_TWO_WAY_AUDIO 
			iResult = RTSPStreaming_ComposeSIPSDPFor2WayAudio(pRTSPStreaming,
											    pRTSPStreaming->ulLocalIP,
												tAudioCodecInfo.eCodecType,
												ptSDPRequest->pcSDPBuffer,
												ptSDPRequest->iSDPBufLen); 		
#else
			//!!!!SIP default SDP is 1st!!!
			iResult = RTSPStreaming_ComposeSIPSDP(pRTSPStreaming,
											    pRTSPStreaming->ulLocalIP,
												1,
												FALSE,
												iStreamMode,
												ptSDPRequest->pcSDPBuffer,
												ptSDPRequest->iSDPBufLen); 		
#endif

			ptSDPRequest->iSDPBufLen = iResult;
			return 0;

			break;
	
		case SIPUA_CALLBACKFLAG_DIALOG_START:

			if(!pvParam1)
				return -1;	

			ptSIPSessionInfo = (TSIPUASessionInfo*) pvParam1;
			printf("[RTSPStreaming] Callback recv SIPUA_CALLBACKFLAG_DIALOG_START\r\n");
#ifdef _SIP_TWO_WAY_AUDIO 
			RTSPServer_SetSessionIDForUpStreamAudio(pRTSPStreaming->hRTSPServerHandle, ptSIPSessionInfo->dwSessionID);
			RTSPServer_SetLastRecvTimeForUpStreamAudioNow(pRTSPStreaming->hRTSPServerHandle);
			return 0;
#endif

            OSSemaphore_Get((HANDLE)pRTSPStreaming->ulSessionListSemaphore,INFINITE);						

			if(pRTSPStreaming->iSessionListNumber >= pRTSPStreaming->iMaximumSessionCount)
			{
				TelnetShell_DbgPrint(" [RTSPServer] Over Max access count IP=%s  SessionID=%u \n", inet_ntoa((*((struct in_addr*)&(ptSIPSessionInfo->ulClientIP)))),ptSIPSessionInfo->dwSessionID);   				
                OSSemaphore_Post((HANDLE)pRTSPStreaming->ulSessionListSemaphore);
				return -1;			
			}
			else
			{			
				iResult = 0;
				memset(&tRTPSessionInfo,0,sizeof(RTSPSERVER_SESSIONINFORMATION));
				RTSPStreaming_SIPFillSessionInfo(pRTSPStreaming,ptSIPSessionInfo,&tRTPSessionInfo);

				if( ptSIPSessionInfo->usAudioPort != 0 )
				{
					iResult = RTSPStreaming_AddRTPRTCPAudioSession(pRTSPStreaming,&tRTPSessionInfo,1, NULL); //FIXME 20160601 modify by Faber, although we are not support sip in tcp 
				}				

				if( ptSIPSessionInfo->usVideoPort != 0 && iResult == 0 )
				{
					iResult = RTSPStreaming_AddRTPRTCPVideoSession(pRTSPStreaming,&tRTPSessionInfo,0, NULL); //FIXME 20160601 modify by Faber, although we are not support sip in tcp 
				}
				
				if( iResult == 0 )
				{
#ifdef RTSPRTP_MULTICAST				
 //   				if( pstRTSPServerSessionInformation->iMulticast == 0 )				
#endif
	    			{
		    			pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].dwSessionID	=tRTPSessionInfo.dwSessionID;
	    			    pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].ulClientIP	=tRTPSessionInfo.ulClientIP;
						pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].iSessionType =RTSPSTREAMING_SIP_SESSION; 
						//!!!!SIP default SDP is 1st!!!
						pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].iSDPIndex = 1;
						
		    		    if( ptSIPSessionInfo->usAudioPort != 0 && ptSIPSessionInfo->usVideoPort == 0 ) // Audio only!
						{
			    		    pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].byMediaStatus = RTSPSTREAMING_AUDIO_ONLY ;				
    						pRTSPStreaming->iAudioSessionNumberOfStreamType[pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].iSDPIndex-1]++;
						}
				        else if( ptSIPSessionInfo->usAudioPort == 0 && ptSIPSessionInfo->usVideoPort != 0 ) // Video only!
						{
					        pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].byMediaStatus = RTSPSTREAMING_VIDEO_ONLY ;				
       						pRTSPStreaming->iVideoSessionNumberOfStreamType[pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].iSDPIndex-1]++;
						}
				        else
						{
					        pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].byMediaStatus = RTSPSTREAMING_BOTH_MEDIA ;				
       						pRTSPStreaming->iAudioSessionNumberOfStreamType[pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].iSDPIndex-1]++;
							pRTSPStreaming->iVideoSessionNumberOfStreamType[pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].iSDPIndex-1]++;
						}
				     
						if( pRTSPStreaming->iAudioSessionNumberOfStreamType[pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].iSDPIndex-1] == 1 )
						{
							TelnetShell_DbgPrint("[%s]: Audio of Streaming Type %d should start !!!\r\n", __FUNCTION__, pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].iSDPIndex);
					        pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle,0,ccctStreamTypeAudioStart,pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].iSDPIndex);
						}

				        pRTSPStreaming->iSessionListNumber++;
						printf("========Session list number %d========\n",pRTSPStreaming->iSessionListNumber);
#ifdef _LINUX					
						syslog(LOG_DEBUG,"Session list number %d !\n",pRTSPStreaming->iSessionListNumber);
#endif							
				    }
			    }
			    else
			    	printf("[SIP UA]: Add SIP/RTP session failed\n");
			}

			{
				unsigned long ulNetIP = htonl(ptSIPSessionInfo->ulClientIP);
				printf("[SIP User Agent Server]:Start streaming media, IP=%s  SessionID=%u \n", inet_ntoa((*((struct in_addr*)&ulNetIP))),ptSIPSessionInfo->dwSessionID);   
			}
            OSSemaphore_Post((HANDLE)pRTSPStreaming->ulSessionListSemaphore);
			return iResult;		

		    break;

		case SIPUA_CALLBACKFLAG_DIALOG_STOP:
			ptSIPSessionInfo = (TSIPUASessionInfo*) pvParam1;
			printf("[RTSPStreaming] Callback recv SIPUA_CALLBACKFLAG_DIALOG_STOP\r\n");

            OSSemaphore_Get((HANDLE)pRTSPStreaming->ulSessionListSemaphore,INFINITE);										

#ifdef _SIP_TWO_WAY_AUDIO 
			RTSPServer_SetSSRCForUpStreamAudio(pRTSPStreaming->hRTSPServerHandle, 0);
			RTSPServer_SetSessionIDForUpStreamAudio(pRTSPStreaming->hRTSPServerHandle, 0);
#endif
			
			if(pRTSPStreaming->iSessionListNumber == 0 )
			{
				DbgLog((dfCONSOLE|dfINTERNAL," [RTSPServer] Can't find session ID to remove , SessionID=%u \n", ptSIPSessionInfo->dwSessionID));   
                OSSemaphore_Post((HANDLE)pRTSPStreaming->ulSessionListSemaphore);

				return -1;			
			}
			else
			{
				iResult = RTSPStreaming_RemoveRTPRTCPSessionBySessionID(pRTSPStreaming, ptSIPSessionInfo->dwSessionID) ;	
			}
	
            OSSemaphore_Post((HANDLE)pRTSPStreaming->ulSessionListSemaphore);			
			return iResult;		

			break;

		case SIPUA_CALLBACKFLAG_REQUEST_AUTHINFO:
        
            if( (pvParam1 != NULL) && (pRTSPStreaming->fControlCallBack != NULL) )
            {
				TSIPUAAuthorInfo* ptSIPAuthInfo = (TSIPUAAuthorInfo*) pvParam1 ;
				TAuthorInfo tAuthInfo ;
				SCODE scRet ;

				memset(&tAuthInfo, 0, sizeof(TAuthorInfo));
				rtspstrcpy(tAuthInfo.acUserName, ptSIPAuthInfo->szAuthName, sizeof(tAuthInfo.acUserName));
				tAuthInfo.iAuthType = RTSPSTREAMING_AUTHENTICATION_DIGEST;
   	            scRet = pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle,0,ccctAuthorization,(DWORD)&tAuthInfo);
				strncpy(ptSIPAuthInfo->szAuthPassword, tAuthInfo.acPasswd, sizeof(ptSIPAuthInfo->szAuthPassword) -1);
				return scRet ;
            }
            return -1;

			break;
			
		//20100604 Added by Danny for sessioninfo
		case SIPUA_CALLBACKFLAG_UPDATE_SESSIONINFO:
        
            if( pvParam1 != NULL && pRTSPStreaming->fControlCallBack != NULL )
            {
   	            return pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle,0,ccctSIPUAessionInfoUpdate,(DWORD)pvParam1);
            }
			break;

		default:
			break;
	}

	return S_OK;
}

#endif

int append_in_sdp(char* szSdp, const int iSdpLen, const char* szAppend)
{
	int iLen = strlen(szSdp);
	
	iLen += snprintf(szSdp + iLen, iSdpLen, "%s\r\n", szAppend);

	return iLen;
}

int RTSPStreaming_RTSPServerCallback(HANDLE hParentHandle, UINT uMsg, void * pvParam1, void * pvParam2)
{
	RTSPSTREAMING *pRTSPStreaming;
	RTSPSERVER_CLIENTIP *	pstRTSPServerClientIP;
	RTSPSERVER_SESSIONINFORMATION * pstRTSPServerSessionInformation;
	RTPRTCPCOMPOSER_PARAM  stRTPRTCPComposerParam;
	RTPRTCPCHANNEL_CONNECTION  stRTPRTCPSessionParam;
	int i,iSDPIndex;
	unsigned long	ulResult=S_FAIL;
	DWORD dwSessionID;
	//HANDLE	hTemp;
	char * pcTemp;
	//RTPMEDIABUFFER	*pMediaBuffer;
	int iResult;
//	char * pcResult,pcResultA, pcResultV;	
//	unsigned long ulClientIP;
	//20120807 modified by Jimmy for metadata
	int iAudioPosition = -1;
	int iVideoPosition = -1;
	//20120807 added by Jimmy for metadata
	int iMetadataPosition = -1;
	//int iTrackMediaType[MEDIA_TYPE_NUMBER];
	RTSPSERVER_SDPREQUEST	*pstRTSPServerSDPRequest;
	int						iMulticastIndex,iMulticastFlag;

	if(!hParentHandle)
		return -1;
			
	pRTSPStreaming=(RTSPSTREAMING *)hParentHandle;		
	
	memset(&stRTPRTCPComposerParam,0,sizeof(RTPRTCPCOMPOSER_PARAM));
	memset(&stRTPRTCPSessionParam,0,sizeof(RTPRTCPCHANNEL_CONNECTION));

	switch(uMsg)
	{	
		case RTSPSERVER_CALLBACKFLAG_ACCESSIP_CHECK	:
		
			if(!pvParam1)
				return -1;
				
			pstRTSPServerClientIP=(RTSPSERVER_CLIENTIP *)pvParam1;
//			ulClientIP=ntohl(pstRTSPServerClientIP->ulIP);
			iResult=IPAccessCheck_CheckIP(pRTSPStreaming->hIPAccessCheckHandle, pstRTSPServerClientIP->ulIP);
			
/*			if(iResult==0)
			{
				unsigned long ulNetIP = htonl(ulClientIP);
				//printf(" [RTSPServer] Accept client access, IP=%s  port=%d  \n",(char*) inet_ntoa((*((struct in_addr*)&(ulNetIP)))),ntohs(pstRTSPServerClientIP->usPort));
#ifdef _LINUX
				//syslog(LOG_WARNING, "SS: Connected from %s", (char*)inet_ntoa((*((struct in_addr*)&ulNetIP))));
#endif //_LINUX
	        }    
			else
			{
                //printf(" [RTSPServer] Deny client access, IP=%s  port=%d \n", inet_ntoa((*((struct in_addr*)&(ulClientIP)))),ntohs(pstRTSPServerClientIP->usPort));
	        }    */
				            
			return iResult;
	
		    break;
		
		case RTSPSERVER_CALLBACKFLAG_SDP_REQUEST :
		
		    if( pvParam1 == 0 )
			    return -1;
			
		    pstRTSPServerSDPRequest=(RTSPSERVER_SDPREQUEST *)pvParam1;	
		    pcTemp=pstRTSPServerSDPRequest->pcDescribe;
		    iSDPIndex=-1;
			for( i=0 ; i< MULTIPLE_STREAM_NUM; i ++)
			{
			    OSSemaphore_Get(pRTSPStreaming->hMediaParamSemaphore,INFINITE);
			    printf("compare %s to %s or %s\n",pcTemp, pRTSPStreaming->acAccessName[i], g_acAliasAccessName[i]);
			    
				if( (strcmp(pcTemp,pRTSPStreaming->acAccessName[i]) == 0 ) ||
				    (strcmp(pcTemp,g_acAliasAccessName[i]) == 0 ) )
				{
					iSDPIndex = pstRTSPServerSDPRequest->iSDPindex =  i+1;
			        OSSemaphore_Post(pRTSPStreaming->hMediaParamSemaphore);
					break;
				}
		        OSSemaphore_Post(pRTSPStreaming->hMediaParamSemaphore);
			}

		    if( iSDPIndex < 0 )
		    {
                TelnetShell_DbgPrint("[RTSPServer] SDP Describe name error \n");   
			    return -2;
		    }
				
			return 0;
			break;

		//20090312 multiple stream: break callback into 2 functions!	
		case RTSPSERVER_CALLBACKFLAG_COMPOSE_SDP:

		    if( pvParam1 == 0 )
			    return -1;
	
		    pstRTSPServerSDPRequest=(RTSPSERVER_SDPREQUEST *)pvParam1;	
			iSDPIndex = pstRTSPServerSDPRequest->iSDPindex; 

		    pcTemp=pstRTSPServerSDPRequest->pcDescribe;
		    if( pRTSPStreaming->ulNATIP )
		    {
			    iResult=RTSPStreaming_CheckClientIPDomainInHostOrder(pRTSPStreaming->ulLocalIP, pRTSPStreaming->ulLocalSubnetMask, ntohl(pstRTSPServerSDPRequest->ulIP));
		    }
		    else
		    {
			    iResult=0; // no public IP
		    }
		
#ifdef RTSPRTP_MULTICAST
			iMulticastFlag = TRUE;
#else 
			iMulticastFlag = FALSE;
#endif	

#ifdef _LINUX			
			//wait for correct CI is set otherwise SDP will be wrong
			i=0;
			while(1)
			{
#ifdef _SHARED_MEM
				//Self test for MOD server not finish
				/*if( strncmp(pcTemp, RTSPMOD_SDP_KEYWORD, strlen(RTSPMOD_SDP_KEYWORD)) == 0 )
				{
					TMultipleStreamCIInfo		tCIInfo;
					
					memset(&tCIInfo, 0, sizeof(TMultipleStreamCIInfo));
					tCIInfo.iSDPIndex = iSDPIndex;
					tCIInfo.pcExtraInfo = pstRTSPServerSDPRequest->pcExtraInfo;

					pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle,0 ,ccctMODForceCI, (DWORD)&tCIInfo);
					OSSleep_MSec(100);
				}*/
#endif			
				if( pRTSPStreaming->tVideoEncodeParam[iSDPIndex-1].iCIReady == 1 )
				{
#ifdef _SHARED_MEM
					//20100428 Added For Media on demand
					if( strncmp(pcTemp, RTSPMOD_SDP_KEYWORD, strlen(RTSPMOD_SDP_KEYWORD)) == 0 )
					{
						pRTSPStreaming->tVideoEncodeParam[iSDPIndex-1].iCIReady = 0;
					}
#endif
					break;
				}
				else if( pstRTSPServerSDPRequest->bResetCI )		//Multiple stream reset CI does not require force CI
				{		
					TMultipleStreamCIInfo		tCIInfo;
					
					memset(&tCIInfo, 0, sizeof(TMultipleStreamCIInfo));
					tCIInfo.iSDPIndex = iSDPIndex;
					
					if(pstRTSPServerSDPRequest->acResolution[0] != 0)
					{
						rtspstrcpy(tCIInfo.acResolution, pstRTSPServerSDPRequest->acResolution, sizeof(tCIInfo.acResolution));
					}
					if(pstRTSPServerSDPRequest->acCodecType[0] != 0)
					{
						rtspstrcpy(tCIInfo.acCodecType, pstRTSPServerSDPRequest->acCodecType, sizeof(tCIInfo.acCodecType));
					}

					pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle,0 ,ccctForceCI, (DWORD)&tCIInfo);
					OSSleep_MSec(100);
				}
#ifdef _SHARED_MEM
				//20100428 Added For Media on demand
				else if( strncmp(pcTemp, RTSPMOD_SDP_KEYWORD, strlen(RTSPMOD_SDP_KEYWORD)) == 0 )
				{
					TMultipleStreamCIInfo		tCIInfo;
					
					memset(&tCIInfo, 0, sizeof(TMultipleStreamCIInfo));
					tCIInfo.iSDPIndex = iSDPIndex;
					tCIInfo.pcExtraInfo = pstRTSPServerSDPRequest->pcExtraInfo;

					if( ulResult == S_FAIL )
					{
						ulResult = pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle,0 ,ccctMODForceCI, (DWORD)&tCIInfo);
					}
					OSSleep_MSec(100);

					if( pRTSPStreaming->tModControlParam[iSDPIndex - LIVE_STREAM_NUM - 1].eMODRunCode != MOD_INFO_OK )
					{
						pstRTSPServerSDPRequest->eMODRunCode = pRTSPStreaming->tModControlParam[iSDPIndex - LIVE_STREAM_NUM - 1].eMODRunCode;
						printf("[%s]Stream %d eMODRunCode %d\n", __FUNCTION__, iSDPIndex, pstRTSPServerSDPRequest->eMODRunCode);
						memset(&pRTSPStreaming->tModControlParam[iSDPIndex - LIVE_STREAM_NUM - 1], 0, sizeof(TRTSPSTREAMING_MODCONTROL_PARAM));
						return -2;
					}
				}
#endif
				else	//If not multiple stream reset CI, we will send ForceCI
				{
					pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle, 0 ,ccctForceCI, 0);
					OSSleep_MSec(100);
				}
					
				i++;
				
				if( i > 60 )
				{
					syslog(LOG_ALERT,"No CI from video! can not compose correct SDP\n");
					return -3;		
				}
			}
			//20120807 modified by Jimmy for metadata
			if( pRTSPStreaming->iRTSPStreamingMediaType[iSDPIndex-1] & RTSPSTREAMING_MEDIATYPE_AUDIO)
			{
				i=0;
				while(1)
				{
					if( pRTSPStreaming->tAudioEncodeParam[iSDPIndex-1].iCIReady == 1 )
					{
						//20140605 modified by Charles
						if( strncmp(pcTemp, RTSPMOD_SDP_KEYWORD, strlen(RTSPMOD_SDP_KEYWORD)) == 0 )
						{
							pRTSPStreaming->tAudioEncodeParam[iSDPIndex-1].iCIReady = 0;
						}
						break;
					}
					else		
					{		
						OSSleep_MSec(100);
					}
					
					i++;
				
					if( i > 25 )
					{
						syslog(LOG_ALERT,"No CI from audio! can not compose correct SDP\n");
						return -4;		
					}
				}
			}
			//20140605 modified by Charles for ONVIF track token
			if( strncmp(pcTemp, RTSPMOD_SDP_KEYWORD, strlen(RTSPMOD_SDP_KEYWORD)) == 0 )
			{
				i=0;
				while(1)
				{
					if( pRTSPStreaming->tModControlParam[iSDPIndex - LIVE_STREAM_NUM - 1].iMODSetControlInfoReady == 1 )
					{
						pRTSPStreaming->tModControlParam[iSDPIndex - LIVE_STREAM_NUM - 1].iMODSetControlInfoReady = 0;
						break;
					}
					else		
					{		
						OSSleep_MSec(100);
					}
					
					i++;
				
					if( i > 25 )
					{
						syslog(LOG_ALERT,"No ONVIF Track Token from MOD! can not compose correct SDP\n");
						printf("No ONVIF Track Token from MOD! can not compose correct SDP\n"); //FOR TEST
						return -4;		
					}
				}
			}
#endif
			
		    if(iResult!=0) 
		    {
		    	//20120925 modified by Jimmy for ONVIF backchannel
		    	//20111124 modify by danny support Backchannel Multicast for ONVIF test tool 1.02.4
			    // Client in Different domain, use public IP						
				iResult=RTSPStreaming_ComposeAVSDP(hParentHandle,iSDPIndex,pRTSPStreaming->ulNATIP,pstRTSPServerSDPRequest->iVivotekClient,iMulticastFlag,0,0,pstRTSPServerSDPRequest->iPlayerType,pstRTSPServerSDPRequest->iRequire); 		
		    }
		    else
		    {
		    	//20120925 modified by Jimmy for ONVIF backchannel
		    	//20111124 modify by danny support Backchannel Multicast for ONVIF test tool 1.02.4
				// Client in same domain, use local IP		
				iResult=RTSPStreaming_ComposeAVSDP(hParentHandle,iSDPIndex,pRTSPStreaming->ulLocalIP,pstRTSPServerSDPRequest->iVivotekClient,iMulticastFlag,0,0,pstRTSPServerSDPRequest->iPlayerType,pstRTSPServerSDPRequest->iRequire);
				// pRTSPStreaming->acSDPContent[0] = '\0';
				// append_in_sdp(pRTSPStreaming->acSDPContent, sizeof(pRTSPStreaming->acSDPContent), "v=0");
				// append_in_sdp(pRTSPStreaming->acSDPContent, sizeof(pRTSPStreaming->acSDPContent), "o=- 82367805780 82367805780 IN IP6 ::");
				// append_in_sdp(pRTSPStreaming->acSDPContent, sizeof(pRTSPStreaming->acSDPContent), "s=Media Presentation");
				// append_in_sdp(pRTSPStreaming->acSDPContent, sizeof(pRTSPStreaming->acSDPContent), "e=NONE");
				// append_in_sdp(pRTSPStreaming->acSDPContent, sizeof(pRTSPStreaming->acSDPContent), "b=AS:5050");
				// append_in_sdp(pRTSPStreaming->acSDPContent, sizeof(pRTSPStreaming->acSDPContent), "a=control:rtsp://172.16.7.66:554/0/");
				// append_in_sdp(pRTSPStreaming->acSDPContent, sizeof(pRTSPStreaming->acSDPContent), "m=video 0 RTP/AVP 96");
				// append_in_sdp(pRTSPStreaming->acSDPContent, sizeof(pRTSPStreaming->acSDPContent), "b=AS:0");
				// append_in_sdp(pRTSPStreaming->acSDPContent, sizeof(pRTSPStreaming->acSDPContent), "a=control:rtsp://172.16.7.182:554/0/trackID=1");
				// append_in_sdp(pRTSPStreaming->acSDPContent, sizeof(pRTSPStreaming->acSDPContent), "a=rtpmap:96 H264/90000");
				// append_in_sdp(pRTSPStreaming->acSDPContent, sizeof(pRTSPStreaming->acSDPContent), "a=fmtp:96 profile-level-id=420029; packetization-mode=1; sprop-parameter-sets=Z00AKZpnA8ARPy4C3AQEBQAAAwPoAADDUOhgAE+7AABPuku8uNDAAJ92AACfdJd5cKA=,aO48gA==");

				printf("faber \n%s\n", pRTSPStreaming->acSDPContent);
	    	}		

			if( iResult < 0 )
				return -5;

			pstRTSPServerSDPRequest->iSDPBuffLen = iResult;		
			strncpy(pstRTSPServerSDPRequest->pSDPBuffer,pRTSPStreaming->acSDPContent, RTSPSTREAMING_SDP_MAXSIZE - 1);
			pstRTSPServerSDPRequest->pSDPBuffer[RTSPSTREAMING_SDP_MAXSIZE - 1] = 0;

		    return 0;	

		    break;
		
		//20090316 multiple stream
		case RTSPSERVER_CALLBACKFLAG_RESET_CI:

		    if( pvParam1 <= 0 )
			    return -1;
			
			iSDPIndex = (int)pvParam1;
			printf("Reset CI for SDPIndex %d\n", iSDPIndex);
			pRTSPStreaming->tVideoEncodeParam[iSDPIndex - 1].iCIReady = 0;

				return 0;
			break;

    	case RTSPSERVER_CALLBACKFLAG_SESSION_START :
		
			if(!pvParam1)
				return -1;	
				
            iResult = OSSemaphore_Get((HANDLE)pRTSPStreaming->ulSessionListSemaphore,INFINITE);						
			pstRTSPServerSessionInformation=(RTSPSERVER_SESSIONINFORMATION *)pvParam1;

            TelnetShell_DbgPrint("[Control] callback session info %d\r\n",pstRTSPServerSessionInformation->iVivotekClient);            

			if(pRTSPStreaming->iSessionListNumber >= pRTSPStreaming->iMaximumSessionCount)
			{
                TelnetShell_DbgPrint(" [RTSPServer] Over Max access count IP=%s  SessionID=%u \n", inet_ntoa((*((struct in_addr*)&(pstRTSPServerSessionInformation->ulClientIP)))),pstRTSPServerSessionInformation->dwSessionID);   
                OSSemaphore_Post((HANDLE)pRTSPStreaming->ulSessionListSemaphore);
				return -1;			
			}
			else
			{
				//iResult=strcmp((char*)&(pstRTSPServerSessionInformation->cMediaName[0]),RTSP_VIDEO_ACCESS_NAME1);
				/*
				iResult = RTSPStreaming_CheckIfVideoTrack(pRTSPStreaming,
														(char*)&(pstRTSPServerSessionInformation->cMediaName[0])
														,pstRTSPServerSessionInformation->iSDPIndex);
												
				if(iResult == FALSE) 
				{
					iAudioPosition=0;
					
					if( pstRTSPServerSessionInformation->cMediaName[1][0] != 0 ) 
						iVideoPosition=1;	
					else
						iVideoPosition=2; // Audio only!		
				}				
				else if( iResult == TRUE)
				{
					iVideoPosition=0;
					
					if( pstRTSPServerSessionInformation->cMediaName[1][0] != 0 ) 					
						iAudioPosition=1;
					else
						iAudioPosition=2; // Video only!		
				}
				else
				{
					printf(" [RTSPServer]: Wrong Track name for media\r\n");   
					OSSemaphore_Post((HANDLE)pRTSPStreaming->ulSessionListSemaphore);
					return -1;
				}
				
				*/
				//20120807 modified by Jimmy for metadata
				for(i = 0; i < MEDIA_TYPE_NUMBER; i++)
				{
					iResult= RTSPStreaming_CheckTrackMediaType(pRTSPStreaming,
																(char*)&(pstRTSPServerSessionInformation->cMediaName[i]),
																pstRTSPServerSessionInformation->iSDPIndex);
					if(iResult == RTSPSTREAMING_MEDIATYPE_VIDEO)
					{
						iVideoPosition = i;
					}
					else if(iResult == RTSPSTREAMING_MEDIATYPE_AUDIO)
					{
						iAudioPosition = i;
					}
					else if(iResult == RTSPSTREAMING_MEDIATYPE_METADATA)
					{
						iMetadataPosition = i;
					}
				}

				if( (iVideoPosition < 0) && (iAudioPosition < 0) && (iMetadataPosition < 0) )
				{
					printf(" [RTSPServer]: Wrong Track name for media\r\n");   
					OSSemaphore_Post((HANDLE)pRTSPStreaming->ulSessionListSemaphore);
					return -1;
				}					
				printf("session start , index = %d\n", pstRTSPServerSessionInformation->iClientIndex);
				iResult = 0;

				 //20120807 modified by Jimmy for metadata
				//if session is not video only

				pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].iSDPIndex=pstRTSPServerSessionInformation->iSDPIndex;
				//20080922 Fix miscount aenc number
				int	iOrigVideo = -1, iOrigAudio = -1;
				//20120807 added by Jimmy for metadata
				int iOrigMetadata = -1;
								//20080922 Fix miscount aenc number
				iOrigVideo = pRTSPStreaming->iVideoSessionNumberOfStreamType[pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].iSDPIndex-1];
				iOrigAudio = pRTSPStreaming->iAudioSessionNumberOfStreamType[pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].iSDPIndex-1];
				//20120807 added by Jimmy for metadata
				iOrigMetadata = pRTSPStreaming->iMetadataSessionNumberOfStreamType[pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].iSDPIndex-1];

								//20120807 modified by Jimmy for metadata
				pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].byMediaStatus = RTSPSTREAMING_NO_MEDIA;
				if( iVideoPosition >=0 )
				{
					pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].byMediaStatus |= RTSPSTREAMING_MEDIATYPE_VIDEO;
					pRTSPStreaming->iVideoSessionNumberOfStreamType[pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].iSDPIndex-1]++;
				}
				if( iAudioPosition >=0 )
				{
					pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].byMediaStatus |= RTSPSTREAMING_MEDIATYPE_AUDIO;
					pRTSPStreaming->iAudioSessionNumberOfStreamType[pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].iSDPIndex-1]++;
				}
				if( iMetadataPosition >=0 )
				{
					pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].byMediaStatus |= RTSPSTREAMING_MEDIATYPE_METADATA;
					pRTSPStreaming->iMetadataSessionNumberOfStreamType[pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].iSDPIndex-1]++;
				}

				//20080922 Fix miscount aenc number
				if(iOrigVideo == 0 && pRTSPStreaming->iVideoSessionNumberOfStreamType[pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].iSDPIndex-1] == 1 )
				{
					TelnetShell_DbgPrint("[%s]: Video of Streaming Type %d should start !!!\r\n", __FUNCTION__, pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].iSDPIndex);
#ifdef _LINUX					
					syslog(LOG_DEBUG,"Video of Streaming Type %d should start !\n",pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].iSDPIndex);
#endif  
					pstRTSPServerSessionInformation->iNotifySource = 1;
					// pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle,0,ccctStreamTypeVideoStart,(DWORD)pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].iSDPIndex);
				}
				//20080922 Fix miscount aenc number
                if(iOrigAudio == 0 && pRTSPStreaming->iAudioSessionNumberOfStreamType[pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].iSDPIndex-1] == 1 )
				{
					TelnetShell_DbgPrint("[%s]: Audio of Streaming Type %d should start !!!\r\n", __FUNCTION__, pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].iSDPIndex);
#ifdef _LINUX					
					syslog(LOG_DEBUG,"Audio of Streaming Type %d should start !\n",pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].iSDPIndex);
#endif						 
					pstRTSPServerSessionInformation->iNotifySource = 1;
			        // pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle,0,ccctStreamTypeAudioStart,(DWORD)pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].iSDPIndex);
				}
				//20120807 added by Jimmy for metadata
				if(iOrigMetadata== 0 && pRTSPStreaming->iMetadataSessionNumberOfStreamType[pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].iSDPIndex-1] == 1 )
				{
					TelnetShell_DbgPrint("[%s]: Metadata of Streaming Type %d should start !!!\r\n", __FUNCTION__, pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].iSDPIndex);
#ifdef _LINUX					
					syslog(LOG_DEBUG,"Metadata of Streaming Type %d should start !\n",pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].iSDPIndex);
#endif
					pstRTSPServerSessionInformation->iNotifySource = 1;
					// pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle,0,ccctStreamTypeMetadataStart,(DWORD)pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].iSDPIndex);
				}

				if( iAudioPosition >= 0 ) 
				{

					iResult = RTSPStreaming_AddRTPRTCPAudioSession(pRTSPStreaming,
																pstRTSPServerSessionInformation,
																iAudioPosition, pRTSPStreaming->pstSessionList[pstRTSPServerSessionInformation->iClientIndex].hTCPMuxCS); //20160603 modify by faber, set tcp mutex
				}	
				//20120807 modified by Jimmy for metadata
				//if session is not audio only and add audio session successfully
				if( iVideoPosition >= 0 && iResult == 0)
				{
					iResult = RTSPStreaming_AddRTPRTCPVideoSession(pRTSPStreaming,
																    pstRTSPServerSessionInformation,
																    iVideoPosition, pRTSPStreaming->pstSessionList[pstRTSPServerSessionInformation->iClientIndex].hTCPMuxCS); //20160603 modify by faber, set tcp mutex
				}
				//20120807 added by Jimmy for metadata
				if( iMetadataPosition >= 0 && iResult == 0)
				{
				    //20140819 added by Charles for eventparser API
				    RTSPStreaming_CheckMetadataConfigFile(pRTSPStreaming, pstRTSPServerSessionInformation);
					iResult = RTSPStreaming_AddRTPRTCPMetadataSession(pRTSPStreaming,
																    pstRTSPServerSessionInformation,
																    iMetadataPosition, pRTSPStreaming->pstSessionList[pstRTSPServerSessionInformation->iClientIndex].hTCPMuxCS); //20160603 modify by faber, set tcp mutex
				}

    			
    			pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].dwSessionID=pstRTSPServerSessionInformation->dwSessionID;
			    pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].ulClientIP=pstRTSPServerSessionInformation->ulClientIP;
#ifdef _SHARED_MEM
				//20101208 Modified by danny For GUID format change
				//20100105 Added For Seamless Recording
				//pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].dwSessionGUID=pstRTSPServerSessionInformation->dwSessionGUID;
				strncpy(pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].acSeamlessRecordingGUID, pstRTSPServerSessionInformation->acSeamlessRecordingGUID, RTSPS_Seamless_Recording_GUID_LENGTH - 1);
				pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].acSeamlessRecordingGUID[RTSPS_Seamless_Recording_GUID_LENGTH - 1] = 0;
#endif
#ifdef _INET6
				memcpy(&pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].tClientSckAddr,&pstRTSPServerSessionInformation->tClientSckAddr, sizeof(RTSP_SOCKADDR));
#endif
				
				pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].iSessionType =RTSPSTREAMING_RTSP_SESSION; 
				//20160603 Modify by Faber, the same session must use the same mutex
				*(void**)pvParam2 = (HANDLE)pRTSPStreaming->pstSessionList[pstRTSPServerSessionInformation->iClientIndex].hTCPMuxCS;
				printf("Get %dth TCP_CS handle %lu\n"
						,pRTSPStreaming->iSessionListNumber
						,(unsigned long)pRTSPStreaming->pstSessionList[pstRTSPServerSessionInformation->iClientIndex].hTCPMuxCS);
				


				/*
				if( iVideoPosition == 2 ) // Audio only!
				{
					pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].byMediaStatus = RTSPSTREAMING_AUDIO_ONLY ;				
					pRTSPStreaming->iAudioSessionNumberOfStreamType[pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].iSDPIndex-1]++;
				}
				else if(iAudioPosition == 2 ) // Video only!
				{
					pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].byMediaStatus = RTSPSTREAMING_VIDEO_ONLY ;				
					pRTSPStreaming->iVideoSessionNumberOfStreamType[pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].iSDPIndex-1]++;
				}
				else // Both video/audio !	
				{
					pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].byMediaStatus = RTSPSTREAMING_BOTH_MEDIA ;				
					pRTSPStreaming->iAudioSessionNumberOfStreamType[pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].iSDPIndex-1]++;
					pRTSPStreaming->iVideoSessionNumberOfStreamType[pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].iSDPIndex-1]++;
				}
				*/



#ifdef RTSPRTP_MULTICAST
				//20100720 Added by danny to fix Backchannel multicast session terminated, rtsp server has not stopped sending video/audio RTP/RTCP
				pRTSPStreaming->pstSessionList[pRTSPStreaming->iSessionListNumber].iMulticast = pstRTSPServerSessionInformation->iMulticast;
#endif
		        pRTSPStreaming->iSessionListNumber++;	
				printf("========Session list number %d========\n",pRTSPStreaming->iSessionListNumber);
#ifdef _LINUX					
				syslog(LOG_DEBUG,"Session list number %d !\n",pRTSPStreaming->iSessionListNumber);
#endif						

			   
			}
			
#ifdef _LINUX
#ifdef _INET6
			//20101123 Added by danny For support advanced system log 
			if (pstRTSPServerSessionInformation->ulClientIP == 0)
			{
				char szPresentString[64]="";
				if ( pRTSPStreaming->bAdvLogSupport == TRUE )
				{
					openlog("[RTSP SERVER]",0, LOG_LOCAL0);
					syslog(LOG_INFO,"Start one session, IP=%s\n", inet_ntop(AF_INET6, &pstRTSPServerSessionInformation->tClientSckAddr.sin6_addr, szPresentString, sizeof(szPresentString)));
					openlog("[RTSP SERVER]",0, LOG_USER);
				}
				else
				{
					syslog(LOG_INFO,"Start one session, IP=%s\n", inet_ntop(AF_INET6, &pstRTSPServerSessionInformation->tClientSckAddr.sin6_addr, szPresentString, sizeof(szPresentString)));
				}
			}
			else
#endif
			{
				if ( pRTSPStreaming->bAdvLogSupport == TRUE )
				{
					openlog("[RTSP SERVER]",0, LOG_LOCAL0);
					syslog(LOG_INFO,"Start one session, IP=%s\n", inet_ntoa(*((struct in_addr*)&(pstRTSPServerSessionInformation->ulClientIP))));
					openlog("[RTSP SERVER]",0, LOG_USER);
				}
				else
				{
					syslog(LOG_INFO,"Start one session, IP=%s\n", inet_ntoa(*((struct in_addr*)&(pstRTSPServerSessionInformation->ulClientIP))));
				}
			}
#endif

#ifdef RTSPRTP_MULTICAST
			if( pstRTSPServerSessionInformation->iMulticast )
			{
				printf("[RTSPServer] Start multicasting media, IP=%s  SessionID=%u \n", (char*)inet_ntoa((*((struct in_addr*)&(pRTSPStreaming->stMulticast[pstRTSPServerSessionInformation->iMulticast-1].ulMulticastAddress)))),pstRTSPServerSessionInformation->dwSessionID);   
			}
			else
#endif
			{
				//unsigned long ulNetIP = htonl(pstRTSPServerSessionInformation->ulClientIP);
                //DbgLog((dfCONSOLE|dfINTERNAL," [RTSPServer] Start streaming media, IP=%s  SessionID=%u \n", inet_ntoa((*((struct in_addr*)&ulNetIP))),pstRTSPServerSessionInformation->dwSessionID));   
				/*if( iResult == 0 )
				{
					pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle,
					                                 pstRTSPServerSessionInformation->dwSessionID,
					                                 ccctRTSPSessionStart,
													 pstRTSPServerSessionInformation->ulClientIP);
				}*/
			}
            OSSemaphore_Post((HANDLE)pRTSPStreaming->ulSessionListSemaphore);
			return iResult;		

		    break;
		
			//Case added by Louis to fix multicast bug!
			case RTSPSERVER_CALLBACKFLAG_SESSION_REMOVE_BACKCHANNEL :

				dwSessionID = (DWORD)pvParam1;
				iMulticastIndex = (DWORD)pvParam2;

				ulResult = OSSemaphore_Get((HANDLE)pRTSPStreaming->ulSessionListSemaphore,INFINITE);
				//printf("Remove Back Channel Only!\n");

				if( iMulticastIndex > 0 )
				{
					for( i = 0; i < pRTSPStreaming->iSessionListNumber ; i++)
					{
						if( pRTSPStreaming->pstSessionList[i].dwSessionID == dwSessionID )
						{		
							/*
							if( pRTSPStreaming->pstSessionList[i].byMediaStatus == RTSPSTREAMING_BOTH_MEDIA )
							{
								pRTSPStreaming->iVideoSessionNumberOfStreamType[iMulticastIndex-1]--;
								pRTSPStreaming->iAudioSessionNumberOfStreamType[iMulticastIndex-1]--;
							}
							else if(pRTSPStreaming->pstSessionList[i].byMediaStatus == RTSPSTREAMING_VIDEO_ONLY)
							{
								pRTSPStreaming->iVideoSessionNumberOfStreamType[iMulticastIndex-1]--;
							}
							else if(pRTSPStreaming->pstSessionList[i].byMediaStatus == RTSPSTREAMING_AUDIO_ONLY)
							{
								pRTSPStreaming->iAudioSessionNumberOfStreamType[iMulticastIndex-1]--;
							}
							*/
							//20120807 modified by Jimmy for metadata
							//20130327 modified by Jimmy to support different video/audio multicast addresses for ONVIF Test Tool 12.12
							if(pRTSPStreaming->pstSessionList[i].byMediaStatus & RTSPSTREAMING_MEDIATYPE_VIDEO)
							{
								pRTSPStreaming->iVideoSessionNumberOfStreamType[pRTSPStreaming->stMulticast[iMulticastIndex-1].iSDPIndex-1]--;
							}
							if(pRTSPStreaming->pstSessionList[i].byMediaStatus & RTSPSTREAMING_MEDIATYPE_AUDIO)
							{
								pRTSPStreaming->iAudioSessionNumberOfStreamType[pRTSPStreaming->stMulticast[iMulticastIndex-1].iSDPIndex-1]--;
							}
							if(pRTSPStreaming->pstSessionList[i].byMediaStatus & RTSPSTREAMING_MEDIATYPE_METADATA)
							{
								pRTSPStreaming->iMetadataSessionNumberOfStreamType[pRTSPStreaming->stMulticast[iMulticastIndex-1].iSDPIndex-1]--;
							}
						}
					}
					//Delete the session info from Sessino List
					//iResult = RTSPStreaming_RemoveRTPRTCPSessionBySessionID(pRTSPStreaming, dwSessionID);	
					DeleteOneSessionFromList(pRTSPStreaming,dwSessionID);
				}
	
            OSSemaphore_Post((HANDLE)pRTSPStreaming->ulSessionListSemaphore);		

			break;

		case RTSPSERVER_CALLBACKFLAG_SESSION_STOP :
		
			dwSessionID = (DWORD)pvParam1;
			iMulticastIndex = (DWORD)pvParam2;

            ulResult = OSSemaphore_Get((HANDLE)pRTSPStreaming->ulSessionListSemaphore,INFINITE);										
			//printf("Entering RTSP SERVER SESSION STOP CALLBACK!\n");

#ifdef RTSPRTP_MULTICAST
                                
			if( iMulticastIndex > 0 )
			{
				printf("stop multicast %lu %d %d\r\n"
            		,(long) dwSessionID
                    ,pRTSPStreaming->stMulticast[iMulticastIndex-1].iAlreadyMulticastVideo
                    ,pRTSPStreaming->stMulticast[iMulticastIndex-1].iAlwaysMulticast);
				
				//modified by Louis 2007/11/15
				for( i = 0; i < pRTSPStreaming->iSessionListNumber ; i++)
				{
					if( pRTSPStreaming->pstSessionList[i].dwSessionID == dwSessionID )
					{		

						if(pRTSPStreaming->pstSessionList[i].byMediaStatus & RTSPSTREAMING_MEDIATYPE_VIDEO)
						{
							pRTSPStreaming->iVideoSessionNumberOfStreamType[pRTSPStreaming->stMulticast[iMulticastIndex-1].iSDPIndex-1]--;
						}
						if(pRTSPStreaming->pstSessionList[i].byMediaStatus & RTSPSTREAMING_MEDIATYPE_AUDIO)
						{
							pRTSPStreaming->iAudioSessionNumberOfStreamType[pRTSPStreaming->stMulticast[iMulticastIndex-1].iSDPIndex-1]--;
						}
						if(pRTSPStreaming->pstSessionList[i].byMediaStatus & RTSPSTREAMING_MEDIATYPE_METADATA)
						{
							pRTSPStreaming->iMetadataSessionNumberOfStreamType[pRTSPStreaming->stMulticast[iMulticastIndex-1].iSDPIndex-1]--;
						}

					}
				}

				if( pRTSPStreaming->stMulticast[iMulticastIndex-1].iAlreadyMulticastVideo &&
					pRTSPStreaming->stMulticast[iMulticastIndex-1].iAlwaysMulticast == 0)
				{
					RTPRTCPChannel_RemoveMulticastSession(pRTSPStreaming->hRTPRTCPChannelVideoHandle,iMulticastIndex);
					pRTSPStreaming->stMulticast[iMulticastIndex-1].iAlreadyMulticastVideo = 0;
					printf("Stop video multicast\r\n");
					//20130327 modified by Jimmy to support different video/audio multicast addresses for ONVIF Test Tool 12.12
					//20130912 modified by Charles for ondemand multicast
					if( pRTSPStreaming->iVideoSessionNumberOfStreamType[pRTSPStreaming->stMulticast[iMulticastIndex-1].iSDPIndex-1] == 0 )
			        {
				        TelnetShell_DbgPrint("[%s]: Video of Streaming Type %d should stop!!!\r\n", __FUNCTION__, iMulticastIndex);
			            pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle,0,ccctStreamTypeVideoStop,pRTSPStreaming->stMulticast[iMulticastIndex-1].iSDPIndex);
			        }
				}

				if( pRTSPStreaming->stMulticast[iMulticastIndex-1].iAlreadyMulticastAudio &&
					pRTSPStreaming->stMulticast[iMulticastIndex-1].iAlwaysMulticast == 0)
				{
					RTPRTCPChannel_RemoveMulticastSession(pRTSPStreaming->hRTPRTCPChannelAudioHandle,iMulticastIndex);
					pRTSPStreaming->stMulticast[iMulticastIndex-1].iAlreadyMulticastAudio = 0;
					printf("Stop audio multicast\r\n");
					//20130327 modified by Jimmy to support different video/audio multicast addresses for ONVIF Test Tool 12.12
					//20130912 modified by Charles for ondemand multicast
					if( pRTSPStreaming->iAudioSessionNumberOfStreamType[pRTSPStreaming->stMulticast[iMulticastIndex-1].iSDPIndex-1] == 0 )
			        {
				        TelnetShell_DbgPrint("[%s]: Audio of Streaming Type %d should stop!!!\r\n", __FUNCTION__, iMulticastIndex);
			            pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle,0,ccctStreamTypeAudioStop,pRTSPStreaming->stMulticast[iMulticastIndex-1].iSDPIndex);
			        }
				}
				//20120807 added by Jimmy for metadata
				if( pRTSPStreaming->stMulticast[iMulticastIndex-1].iAlreadyMulticastMetadata &&
					pRTSPStreaming->stMulticast[iMulticastIndex-1].iAlwaysMulticast == 0)
				{
					RTPRTCPChannel_RemoveMulticastSession(pRTSPStreaming->hRTPRTCPChannelMetadataHandle,iMulticastIndex);
					pRTSPStreaming->stMulticast[iMulticastIndex-1].iAlreadyMulticastMetadata = 0;
					printf("Stop metadata multicast\r\n");
					//20130327 modified by Jimmy to support different video/audio multicast addresses for ONVIF Test Tool 12.12
					//20130912 modified by Charles for ondemand multicast
					if( pRTSPStreaming->iMetadataSessionNumberOfStreamType[pRTSPStreaming->stMulticast[iMulticastIndex-1].iSDPIndex-1] == 0 )
			        {
				        TelnetShell_DbgPrint("[%s]: Metadata of Streaming Type %d should stop!!!\r\n", __FUNCTION__, iMulticastIndex);
			            pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle,0,ccctStreamTypeMetadataStop,pRTSPStreaming->stMulticast[iMulticastIndex-1].iSDPIndex);
			        }
				}

				//20110215 Removed by danny to fix double post cause semaphore invalid, it will cause iSessionListNumber confused/segmentation fault
                //OSSemaphore_Post((HANDLE)pRTSPStreaming->ulSessionListSemaphore);
				//return 0;	
			}
#endif

			if(pRTSPStreaming->iSessionListNumber == 0 )
			{
                DbgLog((dfCONSOLE|dfINTERNAL," [RTSPServer] Can't find session ID to remove , SessionID=%u \n", dwSessionID));   
                OSSemaphore_Post((HANDLE)pRTSPStreaming->ulSessionListSemaphore);

				return -1;			
			}
			else
			{
				iResult = RTSPStreaming_RemoveRTPRTCPSessionBySessionID(pRTSPStreaming, dwSessionID);				
				if( iMulticastIndex > 0 )
				{
					printf("===remove backchannel multicast list===\n");
					DeleteOneSessionFromList(pRTSPStreaming,dwSessionID);
				}

			}
	
            OSSemaphore_Post((HANDLE)pRTSPStreaming->ulSessionListSemaphore);			
			return iResult;		
		
		    break;

		case RTSPSERVER_CALLBACKFLAG_SESSION_PAUSE :
		
			dwSessionID=(DWORD)pvParam1;
			
            ulResult = OSSemaphore_Get((HANDLE)pRTSPStreaming->ulSessionListSemaphore,INFINITE);																
			
			if(pRTSPStreaming->iSessionListNumber == 0 )
			{
                DbgLog((dfCONSOLE|dfINTERNAL," [RTSPServer] NO more session to pause , SessionID=%u \n", dwSessionID));   
                OSSemaphore_Post((HANDLE)pRTSPStreaming->ulSessionListSemaphore);
				return -1;			
			}
			else
			{
				for(i=0; i<pRTSPStreaming->iSessionListNumber; i++)
				{
					if( pRTSPStreaming->pstSessionList[i].dwSessionID == dwSessionID )
					{
						RTPRTCPChannel_PauseOneSession(pRTSPStreaming->hRTPRTCPChannelVideoHandle, dwSessionID);
						RTPRTCPChannel_PauseOneSession(pRTSPStreaming->hRTPRTCPChannelAudioHandle, dwSessionID);
						//20120807 added by Jimmy for metadata
						RTPRTCPChannel_PauseOneSession(pRTSPStreaming->hRTPRTCPChannelMetadataHandle, dwSessionID);
						
    	                OSSemaphore_Post((HANDLE)pRTSPStreaming->ulSessionListSemaphore);
						return 0;
					}
				}
			}
			
            OSSemaphore_Post((HANDLE)pRTSPStreaming->ulSessionListSemaphore);
            DbgLog((dfCONSOLE|dfINTERNAL," [RTSPServer] Can't find session ID to Pause , SessionID=%u \n", dwSessionID));   
			
			return -1;
			break;
				
		case RTSPSERVER_CALLBACKFLAG_SESSION_RESUME :
		
			dwSessionID=(DWORD)pvParam1;
			
            ulResult = OSSemaphore_Get((HANDLE)pRTSPStreaming->ulSessionListSemaphore,INFINITE);																							
			
			if(pRTSPStreaming->iSessionListNumber == 0 )
			{
                DbgLog((dfCONSOLE|dfINTERNAL," [RTSPServer] NO more session to resume, SessionID=%u \n", dwSessionID));   
                OSSemaphore_Post((HANDLE)pRTSPStreaming->ulSessionListSemaphore);

				return -1;			
			}
			else
			{
				for(i=0; i<pRTSPStreaming->iSessionListNumber; i++)
				{
					if( pRTSPStreaming->pstSessionList[i].dwSessionID == dwSessionID )
					{
						RTPRTCPChannel_ResumeOneSession(pRTSPStreaming->hRTPRTCPChannelVideoHandle, dwSessionID);
						RTPRTCPChannel_ResumeOneSession(pRTSPStreaming->hRTPRTCPChannelAudioHandle, dwSessionID);
						//20120807 added by Jimmy for metadata
						RTPRTCPChannel_ResumeOneSession(pRTSPStreaming->hRTPRTCPChannelMetadataHandle, dwSessionID);

    	                OSSemaphore_Post((HANDLE)pRTSPStreaming->ulSessionListSemaphore);				
						return 0;
					}
				}
			}
			
            OSSemaphore_Post(pRTSPStreaming->ulSessionListSemaphore);
            DbgLog((dfCONSOLE|dfINTERNAL," [RTSPServer] Can't find session ID to Resume , SessionID=%u \n", dwSessionID));   
			
			return -1;	
			
		case RTSPSERVER_CALLBACKFLAG_SESSION_RTPUPDATE :	
		    //this call back will fill out the current timestamp and seq# of media channel
		    //to RTSP server for resume RTSP message
		    
			if(!pvParam1)
				return -1;	
	
            ulResult = OSSemaphore_Get(pRTSPStreaming->ulSessionListSemaphore,INFINITE);																															
			
			pstRTSPServerSessionInformation=(RTSPSERVER_SESSIONINFORMATION *)pvParam1;
			
			if(pRTSPStreaming->iSessionListNumber == 0 )
			{
                DbgLog((dfCONSOLE|dfINTERNAL," [RTSPServer] No session to update RTP, SessionID=%u \n", pstRTSPServerSessionInformation->dwSessionID));   
                OSSemaphore_Post(pRTSPStreaming->ulSessionListSemaphore);
				return -1;			
			}
			else
			{
				for(i=0; i<pRTSPStreaming->iSessionListNumber; i++)
				{
					if( pRTSPStreaming->pstSessionList[i].dwSessionID == pstRTSPServerSessionInformation->dwSessionID )
					{
						RTPRTCPComposer_Update(pRTSPStreaming->pstSessionList[i].hRTPRTCPVideoComposerHandle,&stRTPRTCPComposerParam);
						
						pstRTSPServerSessionInformation->dwInitialTimeStamp[0] = stRTPRTCPComposerParam.dwInitialTimeStamp;
						pstRTSPServerSessionInformation->wInitialSequenceNumber[0] = stRTPRTCPComposerParam.wInitialSequenceNumber;
								
						RTPRTCPComposer_Update(pRTSPStreaming->pstSessionList[i].hRTPRTCPAudioComposerHandle,&stRTPRTCPComposerParam);
						
						pstRTSPServerSessionInformation->dwInitialTimeStamp[1] = stRTPRTCPComposerParam.dwInitialTimeStamp;
						pstRTSPServerSessionInformation->wInitialSequenceNumber[1] = stRTPRTCPComposerParam.wInitialSequenceNumber;

						//20120807 added by Jimmy for metadata
#ifdef _METADATA_ENABLE
						RTPRTCPComposer_Update(pRTSPStreaming->pstSessionList[i].hRTPRTCPMetadataComposerHandle,&stRTPRTCPComposerParam);
						
						pstRTSPServerSessionInformation->dwInitialTimeStamp[2] = stRTPRTCPComposerParam.dwInitialTimeStamp;
						pstRTSPServerSessionInformation->wInitialSequenceNumber[2] = stRTPRTCPComposerParam.wInitialSequenceNumber;	
#endif
		                OSSemaphore_Post(pRTSPStreaming->ulSessionListSemaphore);
						return 0;
					}
				}
			}
	
            OSSemaphore_Post(pRTSPStreaming->ulSessionListSemaphore);		
            DbgLog((dfCONSOLE|dfINTERNAL," [RTSPServer] Can't find session ID to update RTP , SessionID=%u \n", pstRTSPServerSessionInformation->dwSessionID));   
			
			return -1;	
			
        case RTSPSERVER_CALLBACKFLAG_AUTHORIZATION:
        
            if( (pvParam1 != NULL) && (pRTSPStreaming->fControlCallBack != NULL) )
            {
   	            return pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle,0,ccctAuthorization,(DWORD)pvParam1);
            }
            return -1;               	 	           
        case RTSPSERVER_CALLBACKFLAG_CHECK_NEED_AUTHORIZATOIN: //20161212 add by Faber, for webattraction or factory default mode
        	if( (pvParam1 != NULL) && (pRTSPStreaming->fControlCallBack != NULL) )
            {
   	            return pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle,0,ccctNeedAuthorization,(DWORD)pvParam1);
            }
            return -1;  
		case RTSPSERVER_CALLBACKFLAG_FORCE_I_FRAME:

			//Modified by Louis 20071231 to work
            if( (pvParam1 != NULL) && (pRTSPStreaming->fControlCallBack != NULL) )
            {
   	            return pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle,0,ccctForceIntra,(DWORD)pvParam1);
            }
            return -1;  

		//20120809 modified by Jimmy for metadata
		/*
		case RTSPSERVER_CALLBACKFLAG_CHECK_VIDEO_TRACK:

			return RTSPStreaming_CheckIfVideoTrack(pRTSPStreaming,(char*)pvParam2 ,(int)pvParam1);

			break;
		*/
		case RTSPSERVER_CALLBACKFLAG_CHECK_TRACK_MEDIATYPE:

			return RTSPStreaming_CheckTrackMediaType(pRTSPStreaming,(char*)pvParam2 ,(int)pvParam1);

			break;

		case RTSPSERVER_CALLBACKFLAG_CHECK_STREAM_MODE:
			{
				DWORD dwStreamIndex = (DWORD) pvParam1;
				
				if( dwStreamIndex > MULTIPLE_STREAM_NUM )
					return -1;
				//printf("[%s] dwStreamIndex=%d, pRTSPStreaming->iRTSPStreamingMediaType[dwStreamIndex-1]=%d\n", __FUNCTION__, dwStreamIndex, pRTSPStreaming->iRTSPStreamingMediaType[dwStreamIndex-1]);	
				return pRTSPStreaming->iRTSPStreamingMediaType[dwStreamIndex-1];
			}
			
		case RTSPSERVER_CALLBACKFLAG_UPDATE_SESSIONINFO:
			if( pvParam1 != NULL && pRTSPStreaming->fControlCallBack != NULL )
            {
   	            pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle,0,ccctRTSPSessionInfoUpdate,(DWORD)pvParam1);
            }
			break;	
			
		case RTSPSERVER_CALLBACKFLAG_CHECK_ACCESSNAME:
			{
				char *pcAccessName = (char *) pvParam1;
				DWORD dwStreamIndex = (int) pvParam2;
				
				if( dwStreamIndex > MULTIPLE_STREAM_NUM )
				{
					return -1;
				}
				
				if( ( strcmp((char*)pRTSPStreaming->acAccessName[dwStreamIndex-1],pcAccessName) == 0 ) ||
					( strcmp(g_acAliasAccessName[dwStreamIndex-1],pcAccessName) == 0 ) )
				{
					return 0;
				}
				else
				{
					return -1;				
				}
			}
			break;	
			
		case RTSPSERVER_CALLBACKFLAG_UPLOAD_AUDIODATA:
		
			if( pvParam1 != NULL && pRTSPStreaming->fControlCallBack != NULL )
            {
   	            pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle,0,ccctRTPAudioDataUpload,(DWORD)pvParam1);
            }
			break;
#ifdef _SIP		
		case RTSPSERVER_CALLBACKFLAG_UPLOAD_AUDIODATA_TIMEOUT:
		
			if( pvParam1 != NULL )
            {
#ifdef _LINUX
				syslog(LOG_WARNING, "two-way audio received timeout, tear down the connection!\n");
#endif
				SIPUA_RemoveDialog(pRTSPStreaming->hSIPUAHandle, (DWORD) pvParam1);
			}
			break;
#endif
#ifdef _SHARED_MEM
		case RTSPSERVER_CALLBACKFLAG_SHMEM_QUERYTIMESHIFT:
			{
				int		iSDPIndex = -1;
				DWORD	dwTSRemain = 0;
				char	*pcTemp = (char	*)pvParam1;
				//First We need to check for SDPIndex
				for( i=0 ; i< MULTIPLE_STREAM_NUM; i ++)
				{
					OSSemaphore_Get(pRTSPStreaming->hMediaParamSemaphore,INFINITE);
			    
					//printf("%s %s\n",g_acAliasAccessName[i],pcTemp);
					printf("compare %s to %s or %s\n",pcTemp, pRTSPStreaming->acAccessName[i], g_acAliasAccessName[i]);
			    
					if( (strcmp(pcTemp,pRTSPStreaming->acAccessName[i]) == 0 ) ||
					 (strcmp(pcTemp,g_acAliasAccessName[i]) == 0 ) )
					{
						iSDPIndex =  i+1;
						OSSemaphore_Post(pRTSPStreaming->hMediaParamSemaphore);
						break;
					}
					OSSemaphore_Post(pRTSPStreaming->hMediaParamSemaphore);
				}

				if( iSDPIndex < 0 )
				{
					TelnetShell_DbgPrint("[RTSPServer] SDP Describe name error \n");   
					return -1;
				}
				//Callback to check media buffer
				dwTSRemain = pRTSPStreaming->fShmemVideoCallBack((DWORD) pRTSPStreaming->hParentVideoHandle,
	                                        MEDIA_CALLBACK_SHM_CHECKBUFFER,
											(void*)pRTSPStreaming->ahShmemVideoHandle[iSDPIndex - 1]);
				//Write back the value
				*(DWORD *)pvParam2 = dwTSRemain;
				return 0;
			}
			break;

		//20100105 Added For Seamless Recording
		case RTSPSERVER_CALLBACKFLAG_UPDATE_GUIDLISTINFO:
			{
				TSeamlessRecordingInfo *pstSeamlessRecordingInfo = NULL;
				char 	szCmd[256];
				int		iSessionCount = 0, iLength = 0;
				
				if( pvParam1 == NULL || (int)pvParam2 < 0 )
				{
					printf("[%s] pvParam error!\n", __FUNCTION__);
					return -1;
				}
				pstSeamlessRecordingInfo = (TSeamlessRecordingInfo *)pvParam1;
				iSessionCount = (int) pvParam2;

				printf("[%s] pstSeamlessRecordingInfo->iRecordingEnable=%d, pstSeamlessRecordingInfo->iSeamlessConnectionNumber=%d, iSessionCount=%d\n",
						__FUNCTION__, pstSeamlessRecordingInfo->iRecordingEnable, pstSeamlessRecordingInfo->iSeamlessConnectionNumber, iSessionCount);
				iLength = snprintf(szCmd, sizeof(szCmd) - 1, "set Value 99 0 0\nseamlessrecording_enable=%d"
															"&seamlessrecording_guid%d_id=%s"
															"&seamlessrecording_guid%d_number=%d",
															pstSeamlessRecordingInfo->iRecordingEnable,
															iSessionCount,
															pstSeamlessRecordingInfo->tGUIDListInfo[iSessionCount].acGUID,
															iSessionCount,
															pstSeamlessRecordingInfo->tGUIDListInfo[iSessionCount].iNumber);
				szCmd[iLength] = '\0';
				printf("[%s] szCmd=%s\n",__FUNCTION__, szCmd);
				//20110915 Modify by danny for support Genetec MOD
				return pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle,0,ccctSetToConfiger,(DWORD)szCmd);
			}
			break;

		case RTSPSERVER_CALLBACKFLAG_UPDATE_RECODERSTATE:
			{
				TSeamlessRecordingInfo *pstSeamlessRecordingInfo = NULL;
				TLastFrameInfo *pLastFrameInfo = NULL;
				char 	szBuffer[256], szMessage[512];
				int		iLength = 0;
				unsigned long		ulSecond;
				unsigned long		ulMilliSecond;
			
				if( pvParam1 == NULL )
				{
					printf("[%s] pvParam error!\n", __FUNCTION__);
					return -1;
				}
				
				pstSeamlessRecordingInfo = (TSeamlessRecordingInfo *)pvParam1;

				if( pvParam2 != NULL )
				{
					pLastFrameInfo = (TLastFrameInfo *)pvParam2;
					//20100917 Modified by danny for fix assign LastFrameTime before mediachannel thread set
					i = 0; 
					while(1)
					{
						if( (pLastFrameInfo->ulLastFrameTimeSec[0] != 0) || (pLastFrameInfo->ulLastFrameTimeMSec[0] != 0) )
						{
							break;
						}
						else
						{
							OSSleep_MSec(100);
							i++;
				
							if( i > 30 )
							{
								printf("[%s] Assign LastFrameTime before mediachannel thread set\n", __FUNCTION__);
								syslog(LOG_ALERT, "Assign LastFrameTime before mediachannel thread set\n");
								break;
							}
						}
					}
					//printf("[%s] RTSPServer assign LastFrameTime OK!\n", __FUNCTION__);
					ulSecond = pLastFrameInfo->ulLastFrameTimeSec[0];
					ulMilliSecond = pLastFrameInfo->ulLastFrameTimeMSec[0];
					//printf("[%s] Last video frame dwSecond=%lu, dwMilliSecond=%lu\n", __FUNCTION__, ulSecond, ulMilliSecond);
				}
				else
				{
					OSTime_GetTimer((DWORD *)&ulSecond, (DWORD *)&ulMilliSecond);
					//printf("[%s] All guids back at dwSecond=%lu, dwMilliSecond=%lu\n", __FUNCTION__, ulSecond, ulMilliSecond);
				}
				
				if( pstSeamlessRecordingInfo->iRecordingEnable == 1 )
				{
					printf("[%s] Compose Recording Start Info!\n",__FUNCTION__);
					iLength = snprintf(szBuffer, sizeof(szBuffer) - 1, 
						"<message><seamless id=\"0\">trigger</seamless><time><sec>%lu</sec><msec>%lu</msec></time></message>",
						ulSecond, ulMilliSecond);
				}
				else
				{
					printf("[%s] Compose Recording Stop Info!\n",__FUNCTION__);
					iLength = snprintf(szBuffer, sizeof(szBuffer) - 1, 
						"<message><seamless id=\"0\">normal</seamless><time><sec>%lu</sec><msec>%lu</msec></time></message>",
						ulSecond, ulMilliSecond);
				}
				//20100915 Modified by danny for fix TLV format error
				/*iLength = snprintf(szMessage, sizeof(szMessage) - 1, "0x01 0x%02x %s", iLength, szBuffer);
				szMessage[iLength] = '\0';*/
				szMessage[0] = 0x01;
				szMessage[1] = iLength;
				iLength = snprintf(szMessage + 2, sizeof(szMessage) - 2 - 1, "%s", szBuffer);
				szMessage[iLength + 2] = '\0';
				printf("[%s] szMessage=%s\n",__FUNCTION__, szMessage);
				
				return pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle,0,ccctRecoderStateUpdate,(DWORD)szMessage);
			}
			break;

		//20100428 Added For Media on demand
		case RTSPSERVER_CALLBACKFLAG_SET_MODCONTROLINFO:
			{
				RTSPSERVER_MODREQUEST *pstRTSPServerMODRequest = NULL;
				int		iMODSDPIndex;
//                EMODCommandType eTempSetCommandType;
                char acTempMODSetCommandValue[RTSPMOD_COMMAND_VALUE_LENGTH];
                memset(acTempMODSetCommandValue, '\0', sizeof(acTempMODSetCommandValue));
				
				if( pvParam1 == NULL )
				{
					printf("[%s] pvParam error!\n", __FUNCTION__);
					return -1;
				}

				pstRTSPServerMODRequest=(RTSPSERVER_MODREQUEST *)pvParam1;
				iMODSDPIndex = pstRTSPServerMODRequest->iSDPIndex - LIVE_STREAM_NUM;
                //20141110 modified by Charles for ONVIF Profile G
                if(pstRTSPServerMODRequest->eMODSetCommandType == MOD_RATECONTROL)
                {
                    if(strncmp(pstRTSPServerMODRequest->acMODSetCommandValue, "no", strlen("no")) == 0)
                    {
                        //translate Rate-Control: "no" header to playspeed=100 when sending to MOD
//                        eTempSetCommandType = MOD_RATECONTROL;
                        rtspstrcpy(acTempMODSetCommandValue, pstRTSPServerMODRequest->acMODSetCommandValue, sizeof(acTempMODSetCommandValue));
						if(pstRTSPServerMODRequest->bIsRateControl2Scale)
						{
							pstRTSPServerMODRequest->eMODSetCommandType = MOD_PLAYSCALE;
							rtspstrcpy(pstRTSPServerMODRequest->acMODSetCommandValue, "5", sizeof(pstRTSPServerMODRequest->acMODSetCommandValue));
						}
						else
						{
							pstRTSPServerMODRequest->eMODSetCommandType = MOD_PLAYSPEED;
							rtspstrcpy(pstRTSPServerMODRequest->acMODSetCommandValue, "100", sizeof(pstRTSPServerMODRequest->acMODSetCommandValue));
						}
                    }
                    else
                    {
                        //Rate-Control: "yes"
                        pstRTSPServerMODRequest->eMODReturnCommandType = MOD_RATECONTROL;
                        rtspstrcpy(pstRTSPServerMODRequest->acMODReturnCommandValue, pstRTSPServerMODRequest->acMODSetCommandValue, sizeof(pstRTSPServerMODRequest->acMODReturnCommandValue));
                        return S_OK;
                    }
                }
				//20160623 Add by Faber, set syncframeonly=yes if i frame only
				if(pstRTSPServerMODRequest->eMODSetCommandType == MOD_SYNCFRAMEONLY)
				{
					rtspstrcpy(pstRTSPServerMODRequest->acMODSetCommandValue, "=yes", sizeof(pstRTSPServerMODRequest->acMODSetCommandValue));
				}
				pRTSPStreaming->tModControlParam[iMODSDPIndex - 1].iMODSetControlInfoReady = 0;
				if( (int)pvParam2 == RTSPMOD_REQUEST_NOWAIT )
				{
					pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle,0,ccctSetMODControlInfo,(DWORD)pstRTSPServerMODRequest);
				}
				else
				{
					//wait for MOD Command Result otherwise response will be wrong
					i=0;
					while(1)
					{
						if( pRTSPStreaming->tModControlParam[iMODSDPIndex - 1].iMODSetControlInfoReady == 1 )
						{
							pRTSPStreaming->tModControlParam[iMODSDPIndex - 1].iMODSetControlInfoReady = 0;
							break;
						}
						else
						{
							if( ulResult == S_FAIL )
							{
								ulResult = pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle,0,ccctSetMODControlInfo,(DWORD)pstRTSPServerMODRequest);
							}
							OSSleep_MSec(100);

							if( pRTSPStreaming->tModControlParam[iMODSDPIndex - 1].eMODRunCode != MOD_INFO_OK )
							{
								pstRTSPServerMODRequest->eMODRunCode = pRTSPStreaming->tModControlParam[iMODSDPIndex - 1].eMODRunCode;
								printf("[%s]Stream %d eMODRunCode %d\n", __FUNCTION__, iMODSDPIndex + LIVE_STREAM_NUM, pstRTSPServerMODRequest->eMODRunCode);
								memset(&pRTSPStreaming->tModControlParam[iMODSDPIndex - 1], 0, sizeof(TRTSPSTREAMING_MODCONTROL_PARAM));
                                if(pstRTSPServerMODRequest->eMODRunCode == MOD_ERR_SERVER_PLAYQUEUE_FULL)
                                {
                                    pstRTSPServerMODRequest->bIsPlayQueueFull = TRUE;
                                }
								return -2;
							}
						}
					
						i++;
				
						if( i > 60 )
						{
							printf("[%s] No Command Result from MOD server! can not compose correct response\n", __FUNCTION__);
							syslog(LOG_ALERT,"No Command Result from MOD server! can not compose correct response\n");
							return -3;		
						}
					}

					pstRTSPServerMODRequest->eMODReturnCommandType = pRTSPStreaming->tModControlParam[iMODSDPIndex - 1].eMODReturnCommandType;
					rtspstrcpy(pstRTSPServerMODRequest->acMODReturnCommandValue, pRTSPStreaming->tModControlParam[iMODSDPIndex - 1].acMODReturnCommandValue, sizeof(pRTSPStreaming->tModControlParam[iMODSDPIndex - 1].acMODReturnCommandValue));
                    //20141110 modified by Charles for ONVIF Profile G
                    if(acTempMODSetCommandValue[0] != '\0')
                    {
                        //assign back Rate-Control command
                        pstRTSPServerMODRequest->eMODSetCommandType = MOD_RATECONTROL;
                        pstRTSPServerMODRequest->eMODReturnCommandType = MOD_RATECONTROL;
                        rtspstrcpy(pstRTSPServerMODRequest->acMODSetCommandValue, acTempMODSetCommandValue, sizeof(pstRTSPServerMODRequest->acMODSetCommandValue));
                        rtspstrcpy(pstRTSPServerMODRequest->acMODReturnCommandValue, acTempMODSetCommandValue, sizeof(pstRTSPServerMODRequest->acMODReturnCommandValue));
                    }
				}
				return S_OK;
			}
			break;
		case RTSPSERVER_CALLBACKFLAG_SET_SVCLEVEL:
			{
				DWORD		dwSessionID = (DWORD)pvParam1;
				ESVCMode	eSVCMode = (ESVCMode)pvParam2;

				if(pRTSPStreaming->iSessionListNumber == 0 )
				{
					DbgLog((dfCONSOLE|dfINTERNAL," [RTSPServer] No session to set SVC level, SessionID=%u \n", pstRTSPServerSessionInformation->dwSessionID));   
					return -1;			
				}
				else
				{
					for(i=0; i<pRTSPStreaming->iSessionListNumber; i++)
					{
						if( pRTSPStreaming->pstSessionList[i].dwSessionID == dwSessionID )
						{
							TShmemSessionInfo *pShmSessionInfo = (TShmemSessionInfo *)pRTSPStreaming->pstSessionList[i].hShmemSessionInfo;
							pShmSessionInfo->eSVCTempMode = eSVCMode;
	
							printf("Session ID %d set SVC level %d\n", dwSessionID, eSVCMode);
							return 0;
						}
					}
				}
	
				DbgLog((dfCONSOLE|dfINTERNAL," [RTSPServer] Can't find session ID to  set SVC level, SessionID=%u \n", pstRTSPServerSessionInformation->dwSessionID));   
	
				return -1;

			}
			break;
			
#endif
		case RTSPSERVER_CALLBACKFLAG_KEEP_ALIVE:
			{
				//int		iSDPIndex = (int)pvParam1;
				DWORD	dwSessionID = (DWORD)pvParam2;

				//printf("Receive OPTION from SDPIndex %d SessionID %u\n", iSDPIndex, dwSessionID);
				RTPRTCPChannel_SessionKeepAlive(pRTSPStreaming->hRTPRTCPChannelVideoHandle, dwSessionID);
				RTPRTCPChannel_SessionKeepAlive(pRTSPStreaming->hRTPRTCPChannelAudioHandle, dwSessionID);
				//20120807 added by Jimmy for metadata
				RTPRTCPChannel_SessionKeepAlive(pRTSPStreaming->hRTPRTCPChannelMetadataHandle, dwSessionID);
			}	
			break;

		//20110401 Added by danny For support RTSPServer thread watchdog
		case RTSPSERVER_CALLBACKFLAG_KICK_WATCHDOG:
			{
				return pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle, 0, ccctRTSPServerKickWatchdog, 0);
			}	
			break;
		//20120925 added by Jimmy for ONVIF backchannel
		case RTSPSERVER_CALLBACKFLAG_GET_CHANNEL_INDEX:
			{
				iSDPIndex = (int)pvParam1;
				return pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle,
														0,
														ccctGetMultipleChannelChannelIndex,
														(DWORD)iSDPIndex);
			}
			break;			
		case RTSPSERVER_CALLBACKFLAG_UPDATE_AUDIODATA_SDPINFO:
			{
				int iChannelIndex = (int)pvParam1;
				TCtrlChCodecInfo	tAudioCodecInfo;

				if( iChannelIndex <= 0)
				{
					return S_FAIL;
				}
				
				tAudioCodecInfo.eCodecType = mctG711U;
				tAudioCodecInfo.wSamplingFreq = 8000;
				tAudioCodecInfo.wChannelNumber = 1;
				tAudioCodecInfo.dwChannelBitmap = 1 << (iChannelIndex - 1);
				
				return pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle
														,0
														,ccctAudioUploadSDPInfo
														,(DWORD)&tAudioCodecInfo);
			}
			break;

		//20130916 added by Charles for ondemand multicast
		case RTSPSERVER_CALLBACKFLAG_CLOSE_MULTICASTSOCKET:
			{
				iMulticastIndex = (DWORD)pvParam1;
				for(i=0 ; i<MEDIA_TYPE_NUMBER*2; i++)
				{
					if(pRTSPStreaming->stMulticast[iMulticastIndex-1].aiMulticastSocket[i] > 0) 
					{
						closesocket((int)pRTSPStreaming->stMulticast[iMulticastIndex-1].aiMulticastSocket[i]);
						pRTSPStreaming->stMulticast[iMulticastIndex-1].aiMulticastSocket[i] = -1;
					}

				}
				printf("close ondemand multicast socket success!\n"); 
                
 			}
			break;
        //20141110 added by Charles for ONVIF Profile G    
        case RTSPSERVER_CALLBACKFLAG_SET_PLAY_CSEQ:
    		{
    			if(!pvParam1)
                {
				    return -1;	
                }         
				
    			pstRTSPServerSessionInformation = (RTSPSERVER_SESSIONINFORMATION *)pvParam1;

    			RTPRTCPChannel_SetPlayCseq(pRTSPStreaming->hRTPRTCPChannelVideoHandle, pstRTSPServerSessionInformation->dwSessionID, pstRTSPServerSessionInformation->iMulticast, pstRTSPServerSessionInformation->iCseq);
    			RTPRTCPChannel_SetPlayCseq(pRTSPStreaming->hRTPRTCPChannelAudioHandle, pstRTSPServerSessionInformation->dwSessionID, pstRTSPServerSessionInformation->iMulticast, pstRTSPServerSessionInformation->iCseq);
    			RTPRTCPChannel_SetPlayCseq(pRTSPStreaming->hRTPRTCPChannelMetadataHandle, pstRTSPServerSessionInformation->dwSessionID, pstRTSPServerSessionInformation->iMulticast, pstRTSPServerSessionInformation->iCseq);
    		}	
    		break;
    	case RTSPSERVER_CALLBACKFLAG_SET_MOD_START: //20151109 Added by faber, add start MOD
    	{
    		int* iModIndex = (int*)pvParam1;
    		printf("resume mod stream %d\n", *iModIndex);
    		pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle, 0, ccctStraemTypeVideoResume, (DWORD)(*iModIndex));
    		pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle, 0, ccctStraemTypeAudioResume, (DWORD)(*iModIndex));
    		break;
    	}
    	case RTSPSERVER_CALLBACKFLAG_SET_MOD_STOP: //20151109 Added by faber, add stop MOD
    	{
    		int* iModIndex = (int*)pvParam1;
    		printf("pause mod stream %d\n", *iModIndex);
    		pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle, 0, ccctStraemTypeVideoPause, (DWORD)(*iModIndex));
    		pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle, 0, ccctStraemTypeAudioPause, (DWORD)(*iModIndex));
    		break;
    	}
    	//20170524
    	case RTSPSERVER_CALLBACKFLAG_GET_NEW_MOD_STREAM:
    	{
    		return pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle, 0, cccGetNemModStream, pvParam1);
    	}
    	case RTSPSERVER_CALLBACKFLAG_RELEASE_MOD_STREAM:
    	{
    		return pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle, 0, cccReleaseModStream, pvParam1);	
    	}
        default:            				
            break;
	}
	
	return 0;
}
#ifndef _SHARED_MEM
int RTSPStreaming_PacketizerVideoCallBack(HANDLE hParentHandle, UINT uMsgFlag, void *pvParam1, void * pvParam2)
{
    //TBitstreamBuffer*   pBitStreamBuf = NULL;
    RTSPSTREAMING*      pRTSPStreaming;
	int                 i,iMulticsat;
	//static  DWORD       dwBaseTime = 0;
	//DWORD               dwCurrentTime =0;  
    
    if( hParentHandle!= NULL )
        pRTSPStreaming = (RTSPSTREAMING *)hParentHandle;    
    else
        return -1;
                        
    switch(uMsgFlag )
	{	
	    case RTPPACKETIZER_CALLBACK_REQUESTBUFFER:	  
			
	        pRTSPStreaming->fVideoCallBack((DWORD) pRTSPStreaming->hParentVideoHandle,
	                                        MEDIA_CALLBACK_REQUEST_BUFFER,
	        	                            (void**)pvParam2);
			
			if ((pRTSPStreaming->iSessionListNumber == 0) && 
					(*((void**)pvParam2) != NULL))
			{
#ifdef RTSPRTP_MULTICAST

				iMulticsat = 0;
				for(i=0;i<RTSP_MULTICASTNUMBER;i++)
				{
					if( pRTSPStreaming->stMulticast[i].iAlreadyMulticastVideo == 1 ||
						pRTSPStreaming->stMulticast[i].iAlwaysMulticast == 1 )
					{
						iMulticsat = 1;
						break;
					}	
				}
				if(iMulticsat == 0)
#endif
				{
					pRTSPStreaming->fVideoCallBack((DWORD) pRTSPStreaming->hParentVideoHandle,
	                                        MEDIA_CALLBACK_RELEASE_BUFFER,
	        	                            (void*)pvParam2);

					*((TBitstreamBuffer**)pvParam2) = 0;								
				}
			}
	        break;
	    case RTPPACKETIZER_CALLBACK_RETURNBUFFER:

	        pRTSPStreaming->fVideoCallBack((DWORD) pRTSPStreaming->hParentVideoHandle,
	                                        MEDIA_CALLBACK_RELEASE_BUFFER,
	        	                            (void*)pvParam2);
	        break;
	    default:
	        break;    
	    
    }
	return 0;       
}

int RTSPStreaming_PacketizerAudioCallBack(HANDLE hParentHandle, UINT uMsgFlag, void *pvParam1, void * pvParam2)
{
    //TBitstreamBuffer* pBitStreamBuf = NULL;
    RTSPSTREAMING *pRTSPStreaming;
   	int  i,iMulticsat;

    if( hParentHandle!= NULL )
        pRTSPStreaming = (RTSPSTREAMING *)hParentHandle;    
    else
        return -1;
        
    switch(uMsgFlag )
	{	
	    case RTPPACKETIZER_CALLBACK_REQUESTBUFFER:	       			
	        pRTSPStreaming->fAudioCallBack((DWORD) pRTSPStreaming->hParentAudioHandle,
	                                        MEDIA_CALLBACK_REQUEST_BUFFER,
	        	                            (void**)pvParam2);

			if ((pRTSPStreaming->iSessionListNumber == 0) && 
					(*((void**)pvParam2) != NULL))
			{
#ifdef RTSPRTP_MULTICAST
				iMulticsat = 0;
				//20130327 modified by Jimmy to support different video/audio multicast addresses for ONVIF Test Tool 12.12
				for(i=0;i<(RTSP_MULTICASTNUMBER+RTSP_AUDIO_EXTRA_MULTICASTNUMBER);i++)
				{
					if( pRTSPStreaming->stMulticast[i].iAlreadyMulticastAudio == 1 ||
						pRTSPStreaming->stMulticast[i].iAlwaysMulticast == 1 )
					{
						iMulticsat = 1;
						break;
					}	
				}
				if(iMulticsat == 0)
#endif
				{
					pRTSPStreaming->fAudioCallBack((DWORD) pRTSPStreaming->hParentAudioHandle,
		                                        MEDIA_CALLBACK_RELEASE_BUFFER,
			    	                            (void*)pvParam2);

					*((TBitstreamBuffer**)pvParam2) = 0;				
				}
			}	

	        break;
	    case RTPPACKETIZER_CALLBACK_RETURNBUFFER:
	        pRTSPStreaming->fAudioCallBack((DWORD) pRTSPStreaming->hParentAudioHandle,
	                                        MEDIA_CALLBACK_RELEASE_BUFFER,
	        	                            (void*)pvParam2);
	        break;
	    default:
	        break;    
	    
    }
	return 0;       
}
//20120801 added by Jimmy for metadata
int RTSPStreaming_PacketizerMetadataCallBack(HANDLE hParentHandle, UINT uMsgFlag, void *pvParam1, void * pvParam2)
{
    //TBitstreamBuffer* pBitStreamBuf = NULL;
    RTSPSTREAMING *pRTSPStreaming;
   	int  i,iMulticsat;

    if( hParentHandle!= NULL )
        pRTSPStreaming = (RTSPSTREAMING *)hParentHandle;    
    else
        return -1;
        
    switch(uMsgFlag )
	{	
	    case RTPPACKETIZER_CALLBACK_REQUESTBUFFER:	       			
	        pRTSPStreaming->fMetadataCallBack((DWORD) pRTSPStreaming->hParentMetadataHandle,
	                                        MEDIA_CALLBACK_REQUEST_BUFFER,
	        	                            (void**)pvParam2);

			if ((pRTSPStreaming->iSessionListNumber == 0) && 
					(*((void**)pvParam2) != NULL))
			{
#ifdef RTSPRTP_MULTICAST
				iMulticsat = 0;
				for(i=0;i<RTSP_MULTICASTNUMBER;i++)
				{
					if( pRTSPStreaming->stMulticast[i].iAlreadyMulticastMetadata == 1 ||
						pRTSPStreaming->stMulticast[i].iAlwaysMulticast == 1 )
					{
						iMulticsat = 1;
						break;
					}	
				}
				if(iMulticsat == 0)
#endif
				{
					pRTSPStreaming->fMetadataCallBack((DWORD) pRTSPStreaming->hParentMetadataHandle,
		                                        MEDIA_CALLBACK_RELEASE_BUFFER,
			    	                            (void*)pvParam2);

					*((TBitstreamBuffer**)pvParam2) = 0;				
				}
			}	

	        break;
	    case RTPPACKETIZER_CALLBACK_RETURNBUFFER:
	        pRTSPStreaming->fMetadataCallBack((DWORD) pRTSPStreaming->hParentMetadataHandle,
	                                        MEDIA_CALLBACK_RELEASE_BUFFER,
	        	                            (void*)pvParam2);
	        break;
	    default:
	        break;    
	    
    }
	return 0;       
}

#endif //_SHARED_MEM

int RTSPStreaming_VideoRTPRTCPChannelCallback(HANDLE hParentHandle, UINT uMsg, void * pvParam1, void * pvParam2)
{
	int iResult;
	RTSPSTREAMING *pRTSPStreaming;
#ifndef _SHARED_MEM
	RTPMEDIABUFFER * pMediaBuffer;
	int nTimeoutInMilliSecond;
#endif
	DWORD dwSessionID;

//	HANDLE hRTPRTCPComposerHandle;
	//HANDLE hTemp;
#ifdef _SHARED_MEM
	TShmemMediaInfo	*ptShmMediaInfo;
	int				iStreamIndex = -1, i = 0;
	//20110711 Modified by danny For Avoid frame type not match CI
	int		iCodecIndex;
	DWORD 	dwFrameType;
#endif

	if(!hParentHandle)
		return 0;
	
	pRTSPStreaming=(RTSPSTREAMING *)hParentHandle;	
			
	switch(uMsg)
	{
	    case RTPRTCPCHANNEL_CALLBACKFLAG_FORCE_I_FRAME:
	        pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle,0,ccctForceIntra,(DWORD)pvParam1);
	        break;
#ifndef _SHARED_MEM	        
		case RTPRTCPCHANNEL_CALLBACKFLAG_GET_MEDIABUF:
		
			nTimeoutInMilliSecond = (int)pvParam1;
			
            iResult=MediaBufQueue_GetMediaBuffer(pRTSPStreaming->hVideoDataQueueHandle,100,pvParam2);
			
			if (iResult!=0)
			{
				*((RTPMEDIABUFFER **)pvParam2)=NULL;
				return -1;
			}
			else
			{
				return 0;
			}					
		    break;
		
		case RTPRTCPCHANNEL_CALLBACKFLAG_SEND_EMPTYBUF:
		
			if(pvParam1)
			{
				pMediaBuffer=(RTPMEDIABUFFER *)pvParam1;
				pMediaBuffer->dwBytesUsed=0;
				pMediaBuffer->pbDataStart=pMediaBuffer->pbBufferStart;				
				iResult = MediaBufQueue_AddMediaBuffer(pRTSPStreaming->hVideoEmptyQueueHandle,pvParam1);
				
				if( iResult != 0 )
                    TelnetShell_DbgPrint("[RTPRTCPChannel] return media buffer failed!!\r\n");   
			}
			return 0;
						
		    break;
#endif //_SHARED_MEM
		    //20170517
		case RTPRTCPCHANNEL_CALLBACKFLAG_SESSION_START:
		{
			pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle,0,ccctStreamTypeVideoStart,(DWORD)pvParam1);
			break;
		}
		case RTPRTCPCHANNEL_CALLBACKFLAG_SESSION_TIMEOUT:

			dwSessionID=(DWORD)pvParam1;
	
            OSSemaphore_Get(pRTSPStreaming->ulSessionListSemaphore,INFINITE);
			
			if(pRTSPStreaming->iSessionListNumber == 0 )
			{
                DbgLog((dfCONSOLE|dfINTERNAL," [RTPRTCPChannel] Video no more session ID (Timeout) , SessionID=%u \n", dwSessionID));   
                OSSemaphore_Post(pRTSPStreaming->ulSessionListSemaphore);
				return -1;			
			}
			else
			{

				iResult =  RTSPStreaming_RemoveRTPRTCPSessionBySessionID(pRTSPStreaming, dwSessionID);
				//20120807 modified by Jimmy for metadata
				RTSPStreaming_RemoveSessionListBySessionID(pRTSPStreaming, dwSessionID,RTSPSTREAMING_MEDIATYPE_VIDEO);
			}

			//if( iResult == 0 )
			{
				RTSPServer_RemoveSession(pRTSPStreaming->hRTSPServerHandle,dwSessionID);
#ifdef _SIP
				SIPUA_RemoveDialog(pRTSPStreaming->hSIPUAHandle, dwSessionID);
#endif
			}

            OSSemaphore_Post(pRTSPStreaming->ulSessionListSemaphore);

			return iResult;						
		    break;

		case RTPRTCPCHANNEL_CALLBACKFLAG_SESSION_CLOSED:
		
			dwSessionID=(DWORD)pvParam1;
#ifdef _LINUX
            //syslog(LOG_ERR," [RTPRTCPChannel] before OSSemaphore_Get, SessionID=%u \n", dwSessionID);   
#endif //_LINUX
            OSSemaphore_Get(pRTSPStreaming->ulSessionListSemaphore,INFINITE);
#ifdef _LINUX
            //syslog(LOG_ERR," [RTPRTCPChannel] Session Closed , SessionID=%u \n", dwSessionID);   
#endif //_LINUX
			
			if(pRTSPStreaming->iSessionListNumber == 0 )
			{
                OSSemaphore_Post(pRTSPStreaming->ulSessionListSemaphore);
				printf("[%s]Video no more session (Closed), SessionID=%d\n",__FUNCTION__, dwSessionID);
                DbgLog((dfCONSOLE|dfINTERNAL," [RTPRTCPChannel] Video no more session (Closed) , SessionID=%u \n", dwSessionID));
				return -1;			
			}
			else
			{

				iResult =  RTSPStreaming_RemoveRTPRTCPSessionBySessionID(pRTSPStreaming, dwSessionID);
				//20120807 modified by Jimmy for metadata
				RTSPStreaming_RemoveSessionListBySessionID(pRTSPStreaming, dwSessionID,RTSPSTREAMING_MEDIATYPE_VIDEO);
			}

			//if( iResult == 0 )
			{
				RTSPServer_RemoveSession(pRTSPStreaming->hRTSPServerHandle,dwSessionID);
#ifdef _SIP
				SIPUA_RemoveDialog(pRTSPStreaming->hSIPUAHandle, dwSessionID);
#endif
			}

            OSSemaphore_Post(pRTSPStreaming->ulSessionListSemaphore);

			return iResult;
		    break;

		case RTPRTCPCHANNEL_CALLBACKFLAG_RECEIVERREPORT:
		
		    // maybe change bit rate
			
			break;

		case RTPRTCPCHANNEL_CALLBACKFLAG_REMOVE_OK:
		
			dwSessionID=(DWORD)pvParam1;		
			OSSemaphore_Get(pRTSPStreaming->ulSessionListSemaphore,INFINITE);
			
			if(pRTSPStreaming->iSessionListNumber == 0 )
			{
                OSSemaphore_Post(pRTSPStreaming->ulSessionListSemaphore);
				printf("[%s]Video no more session (REMOVE_OK), SessionID=%d\n",__FUNCTION__, dwSessionID);
                DbgLog((dfCONSOLE|dfINTERNAL," [RTPRTCPChannel] Video no more session (REMOVE_OK) , SessionID=%u \n", dwSessionID));   
				return -1;			
			}
			else
			{
				//20120807 modified by Jimmy for metadata
				RTSPStreaming_RemoveSessionListBySessionID(pRTSPStreaming, dwSessionID,RTSPSTREAMING_MEDIATYPE_VIDEO);
			}

            OSSemaphore_Post(pRTSPStreaming->ulSessionListSemaphore);

			TelnetShell_DbgPrint("Control: Channel removed done %lu\r\n",(long)dwSessionID);
		    RTSPServer_TeardownSessionOK(pRTSPStreaming->hRTSPServerHandle, dwSessionID); 
			break;

#ifdef _SHARED_MEM
		case RTPRTCPCHANNEL_CALLBACKFLAG_SHMEM_GETBUF:

			ptShmMediaInfo = (TShmemMediaInfo *)pvParam1;

			return pRTSPStreaming->fShmemVideoCallBack((DWORD) pRTSPStreaming->hParentVideoHandle,
	                                        MEDIA_CALLBACK_SHM_REQUEST_BUFFER,
											(void*)ptShmMediaInfo->hParentHandle);
			break;

		case RTPRTCPCHANNEL_CALLBACKFLAG_SHMEM_SELECT:

			iStreamIndex = -1;

			//Callback to poll the unix socket
			if(pRTSPStreaming->fShmemVideoCallBack((DWORD) pRTSPStreaming->hParentVideoHandle,
	                                        MEDIA_CALLBACK_SHM_SELECT_SOCKET,
											(void*)pvParam1) != S_OK)
			{
				//printf("Select return error!\n");
				return S_FAIL;
			}

			//Check for StreamIndex and update the corresponding information
			TShmemSelectInfo	*ptSelectInfo = (TShmemSelectInfo *)pvParam1;

			for(iStreamIndex = 0 ; iStreamIndex < VIDEO_TRACK_NUMBER; iStreamIndex++)
			{
				if(ptSelectInfo->aiNewFrame[iStreamIndex] == 1)
				{
					for(i = 0; i < pRTSPStreaming->iSessionListNumber; i++)
					{
						TShmemSessionInfo *pShmSessionInfo = (TShmemSessionInfo *)pRTSPStreaming->pstSessionList[i].hShmemSessionInfo;

						if(pRTSPStreaming->pstSessionList[i].iSDPIndex == (iStreamIndex + 1) && pShmSessionInfo->tShmemVideoMediaInfo.bFrameGenerated == FALSE)
						{
							pShmSessionInfo->tShmemVideoMediaInfo.bFrameGenerated = TRUE;
						}
					}
				}
			}
			//We should also check multicast
			//20130905 modified by Charles for ondemand multicast
			for(iStreamIndex = 0 ; iStreamIndex < VIDEO_TRACK_NUMBER; iStreamIndex++)
			{
				if(ptSelectInfo->aiNewFrame[iStreamIndex] == 1)
				{
					for(i = 0; i < RTSP_MULTICASTNUMBER+RTSP_ONDEMAND_MULTICASTNUMBER; i++)
					{   
					    // if(i >= RTSP_MULTICASTNUMBER && i < RTSP_MULTICASTNUMBER)
         //                {
         //                    continue;
         //                }               
						TShmemSessionInfo *pShmSessionInfo = (TShmemSessionInfo *)pRTSPStreaming->stMulticast[i].hShmemSessionInfo;

						if(pRTSPStreaming->stMulticast[i].iSDPIndex == (iStreamIndex + 1) && pShmSessionInfo->tShmemVideoMediaInfo.bFrameGenerated == FALSE)
						{
							pShmSessionInfo->tShmemVideoMediaInfo.bFrameGenerated = TRUE;
						}
					}
				}
			}

			return S_OK;

			break;

		//20110711 Modified by danny For Avoid frame type not match CI
		case RTPRTCPCHANNEL_CALLBACKFLAG_CHECK_FRAMETYPE_MATCH_CI:

			iCodecIndex = (int)pvParam1;
			dwFrameType = (DWORD)pvParam2;
			
			if( iCodecIndex <=0 || dwFrameType <= 0 )
			{
				return S_FAIL;
			}
			
			if ( pRTSPStreaming->tVideoEncodeParam[iCodecIndex-1].eVideoCodecType != dwFrameType )
			{
				printf("[%s]iCodecIndex %d, dwFrameType %u, eVideoCodecType %u\n", __func__, iCodecIndex, dwFrameType, pRTSPStreaming->tVideoEncodeParam[iCodecIndex-1].eVideoCodecType);
				syslog(LOG_ERR," [%s] Stream %d FrameType %u not match CI %u\n", __FUNCTION__, iCodecIndex, dwFrameType, pRTSPStreaming->tVideoEncodeParam[iCodecIndex-1].eVideoCodecType);  
				pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle, 0 ,ccctForceCI, 0);
				OSSleep_MSec(100);
				
				return S_FAIL;
			}

			return S_OK;
			
			break;
#endif
#ifdef RTSPRTP_MULTICAST
		//20110725 Add by danny For Multicast RTCP receive report keep alive
		case RTPRTCPCHANNEL_CALLBACKFLAG_MULTICAST_CLOSED:
			
			RTSPStreaming_RemoveSpecificStreamMulticastClients(hParentHandle, (int)pvParam1);
			
			return S_OK;
			
			break;
#endif
		    
	}	

	return 0;
}


int RTSPStreaming_AudioRTPRTCPChannelCallback(HANDLE hParentHandle, UINT uMsg, void * pvParam1, void * pvParam2)
{
	RTSPSTREAMING *pRTSPStreaming;
#ifndef _SHARED_MEM
	RTPMEDIABUFFER *pMediaBuffer;
	int nTimeoutInMilliSecond;
#endif
	DWORD dwSessionID;
	int iResult;
	//HANDLE hTemp;	
//	unsigned long ulResult;
//	char acLogContent[150];
	//int i;
//	struct in_addr InAddr;
#ifdef _SHARED_MEM
	TShmemMediaInfo	*ptShmMediaInfo;
	int				iStreamIndex = -1, i = 0;
#endif

	if(!hParentHandle)
		return 0;
	
	pRTSPStreaming=(RTSPSTREAMING *)hParentHandle;	
			
	switch(uMsg)
	{
#ifndef _SHARED_MEM
		case RTPRTCPCHANNEL_CALLBACKFLAG_GET_MEDIABUF:
				
			nTimeoutInMilliSecond = (int)pvParam1;
			iResult=MediaBufQueue_GetMediaBuffer(pRTSPStreaming->hAudioDataQueueHandle,nTimeoutInMilliSecond,pvParam2);
			
			if (iResult!=0)
			{
				*((RTPMEDIABUFFER **)pvParam2)=NULL;
				return -1;
			}
			else
			{
				return 0;
			}
					
		    break;
		
		case RTPRTCPCHANNEL_CALLBACKFLAG_SEND_EMPTYBUF:
		
			if(pvParam1)
			{
				pMediaBuffer=(RTPMEDIABUFFER *)pvParam1;
				pMediaBuffer->dwBytesUsed=0;
				pMediaBuffer->pbDataStart=pMediaBuffer->pbBufferStart;
				
				iResult = MediaBufQueue_AddMediaBuffer(pRTSPStreaming->hAudioEmptyQueueHandle,pvParam1);
				if( iResult != 0 )
                    DbgLog((dfCONSOLE|dfINTERNAL,"[RTPRTCPChannel] return audio media buffer failed!!\n"));   
			}
			return 0;		
		    break;
#endif //_SHARED_MEM
		case RTPRTCPCHANNEL_CALLBACKFLAG_SESSION_START:
		{
			pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle,0,ccctStreamTypeAudioStart,(DWORD)pvParam1);
			break;
		}
		case RTPRTCPCHANNEL_CALLBACKFLAG_SESSION_TIMEOUT:
			dwSessionID=(DWORD)pvParam1;

           OSSemaphore_Get(pRTSPStreaming->ulSessionListSemaphore,INFINITE);
			
			if(pRTSPStreaming->iSessionListNumber == 0 )
			{
                DbgLog((dfCONSOLE|dfINTERNAL," [RTPRTCPChannel] Audio no more session (Timeout) , SessionID=%u \n", dwSessionID));   
                OSSemaphore_Post(pRTSPStreaming->ulSessionListSemaphore);

				return -1;			
			}
			else
			{
				iResult =  RTSPStreaming_RemoveRTPRTCPSessionBySessionID(pRTSPStreaming, dwSessionID);
				//20120807 modified by Jimmy for metadata
				RTSPStreaming_RemoveSessionListBySessionID(pRTSPStreaming, dwSessionID,RTSPSTREAMING_MEDIATYPE_AUDIO);
								
			}

			//if( iResult == 0)
			{
				RTSPServer_RemoveSession(pRTSPStreaming->hRTSPServerHandle,dwSessionID);
#ifdef _SIP
				SIPUA_RemoveDialog(pRTSPStreaming->hSIPUAHandle, dwSessionID);
#endif
			}

            OSSemaphore_Post(pRTSPStreaming->ulSessionListSemaphore);

			return iResult;						
		    break;

		case RTPRTCPCHANNEL_CALLBACKFLAG_SESSION_CLOSED:
		
			dwSessionID=(DWORD)pvParam1;

            OSSemaphore_Get(pRTSPStreaming->ulSessionListSemaphore,INFINITE);
			
			if(pRTSPStreaming->iSessionListNumber == 0 )
			{
                OSSemaphore_Post(pRTSPStreaming->ulSessionListSemaphore);
				printf("[%s]Audio no more session (Closed), SessionID=%d!\n",__FUNCTION__, dwSessionID);
                DbgLog((dfCONSOLE|dfINTERNAL," [RTPRTCPChannel] Audio no more session (Closed) , SessionID=%u \n", dwSessionID));   
				return -1;			
			}
			else
			{
				iResult =  RTSPStreaming_RemoveRTPRTCPSessionBySessionID(pRTSPStreaming, dwSessionID);
				//20120807 modified by Jimmy for metadata
				RTSPStreaming_RemoveSessionListBySessionID(pRTSPStreaming, dwSessionID,RTSPSTREAMING_MEDIATYPE_AUDIO);				
			}

			//if( iResult == 0 )
			{
				RTSPServer_RemoveSession(pRTSPStreaming->hRTSPServerHandle,dwSessionID);
#ifdef _SIP
				SIPUA_RemoveDialog(pRTSPStreaming->hSIPUAHandle, dwSessionID);
#endif
			}

            OSSemaphore_Post(pRTSPStreaming->ulSessionListSemaphore);			

			return iResult;
		    break;

		case RTPRTCPCHANNEL_CALLBACKFLAG_RECEIVERREPORT:
		
		// do nothing
				
		    break;

		case RTPRTCPCHANNEL_CALLBACKFLAG_REMOVE_OK:
		
			dwSessionID=(DWORD)pvParam1;
			OSSemaphore_Get(pRTSPStreaming->ulSessionListSemaphore,INFINITE);
			
			if(pRTSPStreaming->iSessionListNumber == 0 )
			{
                OSSemaphore_Post(pRTSPStreaming->ulSessionListSemaphore);
				printf("[%s]Audio no more session (REMOVE_OK), SessionID=%d\n",__FUNCTION__, dwSessionID);
                DbgLog((dfCONSOLE|dfINTERNAL," [RTPRTCPChannel] Audio no more session (REMOVE_OK) , SessionID=%u \n", dwSessionID));   
				return -1;			
			}
			else
			{
				//20120807 modified by Jimmy for metadata
				RTSPStreaming_RemoveSessionListBySessionID(pRTSPStreaming, dwSessionID,RTSPSTREAMING_MEDIATYPE_AUDIO);
				/*pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle,
					                                    dwSessionID,
					                                    ccctRTSPSessionError,
								      				    0);*/
			}
             OSSemaphore_Post(pRTSPStreaming->ulSessionListSemaphore);

			
			TelnetShell_DbgPrint("Control : Channel removed done %lu\r\n",(long)dwSessionID);
		    RTSPServer_TeardownSessionOK(pRTSPStreaming->hRTSPServerHandle, dwSessionID); 

		    break;
#ifdef _SHARED_MEM
		case RTPRTCPCHANNEL_CALLBACKFLAG_SHMEM_GETBUF:

			ptShmMediaInfo = (TShmemMediaInfo *)pvParam1;

			return pRTSPStreaming->fShmemAudioCallBack((DWORD) pRTSPStreaming->hParentAudioHandle,
	                                        MEDIA_CALLBACK_SHM_REQUEST_BUFFER,
											(void*)ptShmMediaInfo->hParentHandle);
			break;

		case RTPRTCPCHANNEL_CALLBACKFLAG_SHMEM_SELECT:

			iStreamIndex = -1;

			//Callback to poll the unix socket
			if(pRTSPStreaming->fShmemAudioCallBack((DWORD) pRTSPStreaming->hParentAudioHandle,
	                                        MEDIA_CALLBACK_SHM_SELECT_SOCKET,
											(void*)pvParam1) != S_OK)
			{
				//printf("Select return error!\n");
				return S_FAIL;
			}
			//Check for StreamIndex and update the corresponding information
			TShmemSelectInfo	*ptSelectInfo = (TShmemSelectInfo *)pvParam1;

			for(iStreamIndex = 0 ; iStreamIndex < AUDIO_TRACK_NUMBER; iStreamIndex++)
			{
				if(ptSelectInfo->aiNewFrame[iStreamIndex] == 1)
				{
					for(i = 0; i < pRTSPStreaming->iSessionListNumber; i++)
					{
						TShmemSessionInfo *pShmSessionInfo = (TShmemSessionInfo *)pRTSPStreaming->pstSessionList[i].hShmemSessionInfo;

						if(pShmSessionInfo->tShmemAudioMediaInfo.bFrameGenerated == FALSE)
						{
							pShmSessionInfo->tShmemAudioMediaInfo.bFrameGenerated = TRUE;
						}
					}
				}
			}
			//We should also check multicast
			for(iStreamIndex = 0 ; iStreamIndex < AUDIO_TRACK_NUMBER; iStreamIndex++)
			{
				if(ptSelectInfo->aiNewFrame[iStreamIndex] == 1)
				{
					//20130327 modified by Jimmy to support different video/audio multicast addresses for ONVIF Test Tool 12.12
					//20130905 modified by Charles for ondemand multicast
					for(i = 0; i < (RTSP_MULTICASTNUMBER+RTSP_ONDEMAND_MULTICASTNUMBER); i++)
					{
						TShmemSessionInfo *pShmSessionInfo = (TShmemSessionInfo *)pRTSPStreaming->stMulticast[i].hShmemSessionInfo;

						if(pShmSessionInfo->tShmemAudioMediaInfo.bFrameGenerated == FALSE)
						{
							pShmSessionInfo->tShmemAudioMediaInfo.bFrameGenerated = TRUE;
						}
					}
				}
			}
			return S_OK;

			break;
#endif
#ifdef RTSPRTP_MULTICAST
		//20110725 Add by danny For Multicast RTCP receive report keep alive
		case RTPRTCPCHANNEL_CALLBACKFLAG_MULTICAST_CLOSED:
			
			RTSPStreaming_RemoveSpecificStreamMulticastClients(hParentHandle, (int)pvParam1);
			
			return S_OK;
			
			break;
#endif

	}	

	return 0;
}
//20120801 added by Jimmy for metadata
int RTSPStreaming_MetadataRTPRTCPChannelCallback(HANDLE hParentHandle, UINT uMsg, void * pvParam1, void * pvParam2)
{
	RTSPSTREAMING *pRTSPStreaming;
#ifndef _SHARED_MEM
	RTPMEDIABUFFER *pMediaBuffer;
	int nTimeoutInMilliSecond;
#endif
	DWORD dwSessionID;
	int iResult;
	//HANDLE hTemp;	
//	unsigned long ulResult;
//	char acLogContent[150];
	//int i;
//	struct in_addr InAddr;
#ifdef _SHARED_MEM
	TShmemMediaInfo	*ptShmMediaInfo;
	int				iStreamIndex = -1, i = 0;
#endif

	if(!hParentHandle)
		return 0;
	
	pRTSPStreaming=(RTSPSTREAMING *)hParentHandle;	
			
	switch(uMsg)
	{
#ifndef _SHARED_MEM
		case RTPRTCPCHANNEL_CALLBACKFLAG_GET_MEDIABUF:
				
			nTimeoutInMilliSecond = (int)pvParam1;
			iResult=MediaBufQueue_GetMediaBuffer(pRTSPStreaming->hMetadataDataQueueHandle,nTimeoutInMilliSecond,pvParam2);
			
			if (iResult!=0)
			{
				*((RTPMEDIABUFFER **)pvParam2)=NULL;
				return -1;
			}
			else
			{
				return 0;
			}
					
		    break;
		
		case RTPRTCPCHANNEL_CALLBACKFLAG_SEND_EMPTYBUF:
		
			if(pvParam1)
			{
				pMediaBuffer=(RTPMEDIABUFFER *)pvParam1;
				pMediaBuffer->dwBytesUsed=0;
				pMediaBuffer->pbDataStart=pMediaBuffer->pbBufferStart;
				
				iResult = MediaBufQueue_AddMediaBuffer(pRTSPStreaming->hMetadataEmptyQueueHandle,pvParam1);
				if( iResult != 0 )
                    DbgLog((dfCONSOLE|dfINTERNAL,"[RTPRTCPChannel] return metadata media buffer failed!!\n"));   
			}
			return 0;		
		    break;
#endif //_SHARED_MEM
		
		case RTPRTCPCHANNEL_CALLBACKFLAG_SESSION_TIMEOUT:
			dwSessionID=(DWORD)pvParam1;

            OSSemaphore_Get(pRTSPStreaming->ulSessionListSemaphore,INFINITE);
			
			if(pRTSPStreaming->iSessionListNumber == 0 )
			{
                DbgLog((dfCONSOLE|dfINTERNAL," [RTPRTCPChannel] Metadata no more session (Timeout) , SessionID=%u \n", dwSessionID));
                OSSemaphore_Post(pRTSPStreaming->ulSessionListSemaphore);

				return -1;
			}
			else
			{
				iResult =  RTSPStreaming_RemoveRTPRTCPSessionBySessionID(pRTSPStreaming, dwSessionID);
				//20120807 modified by Jimmy for metadata
				RTSPStreaming_RemoveSessionListBySessionID(pRTSPStreaming, dwSessionID,RTSPSTREAMING_MEDIATYPE_METADATA);

			}

			//if( iResult == 0)
			{
				RTSPServer_RemoveSession(pRTSPStreaming->hRTSPServerHandle,dwSessionID);
#ifdef _SIP
				SIPUA_RemoveDialog(pRTSPStreaming->hSIPUAHandle, dwSessionID);
#endif
			}

            OSSemaphore_Post(pRTSPStreaming->ulSessionListSemaphore);

			return iResult;
		    break;

		case RTPRTCPCHANNEL_CALLBACKFLAG_SESSION_CLOSED:
		
			dwSessionID=(DWORD)pvParam1;

            OSSemaphore_Get(pRTSPStreaming->ulSessionListSemaphore,INFINITE);
			
			if(pRTSPStreaming->iSessionListNumber == 0 )
			{
                OSSemaphore_Post(pRTSPStreaming->ulSessionListSemaphore);
				printf("[%s]Metadata no more session (Closed), SessionID=%d!\n",__FUNCTION__, dwSessionID);
                DbgLog((dfCONSOLE|dfINTERNAL," [RTPRTCPChannel] Metadata no more session (Closed) , SessionID=%u \n", dwSessionID));   
				return -1;			
			}
			else
			{
				iResult =  RTSPStreaming_RemoveRTPRTCPSessionBySessionID(pRTSPStreaming, dwSessionID);
				//20120807 modified by Jimmy for metadata
				RTSPStreaming_RemoveSessionListBySessionID(pRTSPStreaming, dwSessionID,RTSPSTREAMING_MEDIATYPE_METADATA);				
			}

			//if( iResult == 0 )
			{
				RTSPServer_RemoveSession(pRTSPStreaming->hRTSPServerHandle,dwSessionID);
#ifdef _SIP
				SIPUA_RemoveDialog(pRTSPStreaming->hSIPUAHandle, dwSessionID);
#endif
			}

            OSSemaphore_Post(pRTSPStreaming->ulSessionListSemaphore);			

			return iResult;
		    break;

		case RTPRTCPCHANNEL_CALLBACKFLAG_RECEIVERREPORT:
		
		// do nothing
				
		    break;

		case RTPRTCPCHANNEL_CALLBACKFLAG_REMOVE_OK:
		
			dwSessionID=(DWORD)pvParam1;
			OSSemaphore_Get(pRTSPStreaming->ulSessionListSemaphore,INFINITE);

			if(pRTSPStreaming->iSessionListNumber == 0 )
			{
                OSSemaphore_Post(pRTSPStreaming->ulSessionListSemaphore);
				printf("[%s]Metadata no more session (REMOVE_OK), SessionID=%d\n",__FUNCTION__, dwSessionID);
                DbgLog((dfCONSOLE|dfINTERNAL," [RTPRTCPChannel] Metadata no more session (REMOVE_OK) , SessionID=%u \n", dwSessionID));
				return -1;
			}
			else
			{
				//20120807 modified by Jimmy for metadata
				RTSPStreaming_RemoveSessionListBySessionID(pRTSPStreaming, dwSessionID,RTSPSTREAMING_MEDIATYPE_METADATA);
				/*pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle,
					                                    dwSessionID,
					                                    ccctRTSPSessionError,
								      				    0);*/
			}
             OSSemaphore_Post(pRTSPStreaming->ulSessionListSemaphore);

			
			TelnetShell_DbgPrint("Control : Channel removed done %lu\r\n",(long)dwSessionID);
		    RTSPServer_TeardownSessionOK(pRTSPStreaming->hRTSPServerHandle, dwSessionID); 

		    break;
#ifdef _SHARED_MEM
		case RTPRTCPCHANNEL_CALLBACKFLAG_SHMEM_GETBUF:

			ptShmMediaInfo = (TShmemMediaInfo *)pvParam1;

			return pRTSPStreaming->fShmemMetadataCallBack((DWORD) pRTSPStreaming->hParentMetadataHandle,
	                                        MEDIA_CALLBACK_SHM_REQUEST_BUFFER,
											(void*)ptShmMediaInfo->hParentHandle);
			break;

		case RTPRTCPCHANNEL_CALLBACKFLAG_SHMEM_SELECT:

			iStreamIndex = -1;

			//Callback to poll the unix socket
			if(pRTSPStreaming->fShmemMetadataCallBack((DWORD) pRTSPStreaming->hParentMetadataHandle,
	                                        MEDIA_CALLBACK_SHM_SELECT_SOCKET,
											(void*)pvParam1) != S_OK)
			{
				//printf("Select return error!\n");
				return S_FAIL;
			}
			//Check for StreamIndex and update the corresponding information
			TShmemSelectInfo	*ptSelectInfo = (TShmemSelectInfo *)pvParam1;

			for(iStreamIndex = 0 ; iStreamIndex < METADATA_TRACK_NUMBER; iStreamIndex++)
			{
				if(ptSelectInfo->aiNewFrame[iStreamIndex] == 1)
				{
					for(i = 0; i < pRTSPStreaming->iSessionListNumber; i++)
					{
						TShmemSessionInfo *pShmSessionInfo = (TShmemSessionInfo *)pRTSPStreaming->pstSessionList[i].hShmemSessionInfo;

						if(pShmSessionInfo->tShmemMetadataMediaInfo.bFrameGenerated == FALSE)
						{
							pShmSessionInfo->tShmemMetadataMediaInfo.bFrameGenerated = TRUE;
						}
					}
				}
			}
			//We should also check multicast
			//20140106 modified by Charles for ondemand multicast
			for(iStreamIndex = 0 ; iStreamIndex < METADATA_TRACK_NUMBER; iStreamIndex++)
			{
				if(ptSelectInfo->aiNewFrame[iStreamIndex] == 1)
				{
					for(i = 0; i < RTSP_MULTICASTNUMBER+RTSP_ONDEMAND_MULTICASTNUMBER; i++)
					{   
					    if(i >= RTSP_MULTICASTNUMBER && i < RTSP_MULTICASTNUMBER)
                        {
                            continue;
                        }         
						TShmemSessionInfo *pShmSessionInfo = (TShmemSessionInfo *)pRTSPStreaming->stMulticast[i].hShmemSessionInfo;

						if(pShmSessionInfo->tShmemMetadataMediaInfo.bFrameGenerated == FALSE)
						{
							pShmSessionInfo->tShmemMetadataMediaInfo.bFrameGenerated = TRUE;
						}
					}
				}
			}
			return S_OK;

			break;
#endif
#ifdef RTSPRTP_MULTICAST
		//20110725 Add by danny For Multicast RTCP receive report keep alive
		case RTPRTCPCHANNEL_CALLBACKFLAG_MULTICAST_CLOSED:
			
			RTSPStreaming_RemoveSpecificStreamMulticastClients(hParentHandle, (int)pvParam1);
			
			return S_OK;
			
			break;
#endif

	}	

	return 0;

}



RTPMEDIABUFFER * RTSPStreaming_AllocateMediaBuffer(int nBufferSize)
{

	RTPMEDIABUFFER *pNextBuffer=NULL;

	int nSize=sizeof(RTPMEDIABUFFER);

#ifdef _SHARED_MEM
	pNextBuffer=(RTPMEDIABUFFER *)malloc(nSize);	//Modified 20091014 for Writev
#else
	pNextBuffer=(RTPMEDIABUFFER *)malloc(nSize+nBufferSize);
#endif

	if(pNextBuffer)
	{	
		memset(pNextBuffer,0,nSize+nBufferSize);
#ifdef _SHARED_MEM
		pNextBuffer->pbBufferStart=pNextBuffer->pbDataStart=(BYTE *)pNextBuffer->acPadBuffer + 4;	//Modified 20091022 for Writev
#else
		pNextBuffer->pbBufferStart=pNextBuffer->pbDataStart=(BYTE *)(pNextBuffer+1);
#endif
		pNextBuffer->dwBufferLength=nBufferSize;
	} 
	return pNextBuffer;
}

#ifdef _SIP

#ifdef	_SIP_TWO_WAY_AUDIO
int RTSPStreaming_ComposeSIPSDPFor2WayAudio(HANDLE			hRTSPStreaming,
											unsigned long	ulSDPIP,
											int				iG711CodecType,							   
											char*			pcSDPBuffer,
											int				iSDPBufferLen)
{
	int iResult=0;
	DWORD dwSec=0,dwMSec=0;
	RTSPSTREAMING *pRTSPStreaming;
	char *pcSDPContent;
		
	if( hRTSPStreaming == NULL )
	    return -1;
	    
	pRTSPStreaming=(RTSPSTREAMING *)hRTSPStreaming;		
	
	if( pcSDPBuffer != NULL )
	    pcSDPContent = pcSDPBuffer;
	else    
    	pcSDPContent=pRTSPStreaming->acSDPContent;
    
    OSSemaphore_Get(pRTSPStreaming->hMediaParamSemaphore,INFINITE);

	OSTime_GetTimer(&dwSec,&dwMSec);
	
	iResult=snprintf(pcSDPContent, iSDPBufferLen - 1,"v=0\r\no=- 0 0 IN IP4 %s\r\n",inet_ntoa(*((struct in_addr*)&(ulSDPIP))));
	iResult+=snprintf(pcSDPContent+iResult, iSDPBufferLen - iResult - 1, "s=VVTK Server\r\n");
	iResult+=snprintf(pcSDPContent+iResult, iSDPBufferLen - iResult - 1, "c=IN IP4 %s\r\nt=0 0\r\n",inet_ntoa(*((struct in_addr*)&(ulSDPIP))));

	if( iG711CodecType == mctG711U )
	{
		iResult+=snprintf(pcSDPContent+iResult, iSDPBufferLen - iResult - 1, "m=audio %d RTP/AVP 0\r\na=rtpmap:0 pcmu/8000\r\n"
												,pRTSPStreaming->usRTPAudioPort);
	}
	else
	{
		iResult+=snprintf(pcSDPContent+iResult, iSDPBufferLen - iResult - 1, "m=audio %d RTP/AVP 8\r\na=rtpmap:8 pcma/8000\r\n"
												,pRTSPStreaming->usRTPAudioPort);
	}

	iResult+=snprintf(pcSDPContent+iResult, iSDPBufferLen - iResult - 1,"a=recvonly\r\n");
																
    OSSemaphore_Post(pRTSPStreaming->hMediaParamSemaphore);
	
    if( ( iResult > RTSPSTREAMING_SDP_MAXSIZE && pcSDPBuffer == NULL ) ||
        ( iResult > iSDPBufferLen && pcSDPBuffer != NULL) )
    {
        TelnetShell_DbgPrint("----Warning!!! SDP exceed buffer!!!\r\n---");
        return -2;
    }    
        
	return iResult;
	
}

#endif

int RTSPStreaming_ComposeSIPSDP(HANDLE			hRTSPStreaming,
							   unsigned long	ulSDPIP,
							   int				iSDPIndex,	
							   int				iVivotekClient,							   
							   int				iStreamingMode,
							   char*			pcSDPBuffer,
							   int				iSDPBufferLen)
{
	int i;
	int iResult=0;
	DWORD dwSec=0,dwMSec=0;
	RTSPSTREAMING *pRTSPStreaming;

	char *pcSDPContent;
	BYTE * pbyMPEG4Header;
	int iMPEG4HeaderLength,iMPEG4ProfileLevel;
		
	if( hRTSPStreaming == NULL )
	    return -1;
	    
	pRTSPStreaming=(RTSPSTREAMING *)hRTSPStreaming;		
	
	if( pcSDPBuffer != NULL )
	    pcSDPContent = pcSDPBuffer;
	else    
    	pcSDPContent=pRTSPStreaming->acSDPContent;
    
	pbyMPEG4Header = (BYTE *)pRTSPStreaming->tVideoEncodeParam[iSDPIndex-1].acMPEG4Header;
	iMPEG4HeaderLength = pRTSPStreaming->tVideoEncodeParam[iSDPIndex-1].iMPEG4HeaderLen;
	iMPEG4ProfileLevel = pRTSPStreaming->tVideoEncodeParam[iSDPIndex-1].iProfileLevel;

    OSSemaphore_Get(pRTSPStreaming->hMediaParamSemaphore,INFINITE);

	OSTime_GetTimer(&dwSec,&dwMSec);
	
	//YenChun 060926 Reduce SIP SDP
	iResult=snprintf(pcSDPContent, iSDPBufferLen - 1, "v=0\r\no=- 0 0 IN IP4 %s\r\n",inet_ntoa(*((struct in_addr*)&(ulSDPIP))));
	iResult+=snprintf(pcSDPContent+iResult, iSDPBufferLen - iResult - 1, "s=VVTK Server\r\n");
	iResult+=snprintf(pcSDPContent+iResult, iSDPBufferLen - iResult - 1, "c=IN IP4 %s\r\nt=0 0\r\n",inet_ntoa(*((struct in_addr*)&(ulSDPIP))));

	//YenChun 060530 Default video first then audio
	//20120807 modified by Jimmy for metadata
	if( iStreamingMode & RTSPSTREAMING_MEDIATYPE_VIDEO)
	{
		//20120807 modified by Jimmy for metadata
		iResult+=snprintf(pcSDPContent+iResult, iSDPBufferLen - iResult - 1,"m=video %d RTP/AVP %d\r\na=rtpmap:%d MP4V-ES/%d\r\n"
											,pRTSPStreaming->usRTPVideoPort
											,RTSPSTREAMING_MPEG4_MEDIATYPE
											,RTSPSTREAMING_MPEG4_MEDIATYPE
											,pRTSPStreaming->tVideoEncodeParam[iSDPIndex-1].iClockRate);
		//20120807 modified by Jimmy for metadata
		iResult+=snprintf(pcSDPContent+iResult, iSDPBufferLen - iResult - 1,"a=fmtp:%d profile-level-id=%d;config="
											,RTSPSTREAMING_MPEG4_MEDIATYPE
											,iMPEG4ProfileLevel);

		for(i=0;i<iMPEG4HeaderLength;i++)
			iResult+=snprintf(pcSDPContent+iResult, iSDPBufferLen - iResult - 1,"%2.2X",pbyMPEG4Header[i]);

		iResult+=snprintf(pcSDPContent+iResult, iSDPBufferLen - iResult - 1,";decode_buf=%d\r\n", pRTSPStreaming->tVideoEncodeParam[iSDPIndex-1].iDecoderBufferSize);
	}

	pbyMPEG4Header = (BYTE *)pRTSPStreaming->tAudioEncodeParam[iSDPIndex-1].acM4ASpecConf;
	iMPEG4HeaderLength = pRTSPStreaming->tAudioEncodeParam[iSDPIndex-1].iM4ASpecConfLen;
	iMPEG4ProfileLevel = pRTSPStreaming->tAudioEncodeParam[iSDPIndex-1].iM4AProfileLevel;
	//20120807 modified by Jimmy for metadata
	if( iStreamingMode & RTSPSTREAMING_MEDIATYPE_AUDIO)
	{
		if(pRTSPStreaming->tAudioEncodeParam[iSDPIndex-1].iAudioCodecType == ractGAMR)
		{

			/*iResult+=sprintf(pcSDPContent+iResult,"m=audio %d RTP/AVP %d\r\na=rtpmap:%d AMR/%d\r\n"
												,pRTSPStreaming->usRTPAudioPort
												,RTSPSTREAMING_AUDIO_MEDIATYPE
												,RTSPSTREAMING_AUDIO_MEDIATYPE
												,pRTSPStreaming->tAudioEncodeParam[iSDPIndex-1].iClockRate);*/

			iResult+=snprintf(pcSDPContent+iResult, iSDPBufferLen - iResult - 1,"m=audio %d RTP/AVP 8\r\na=rtpmap:8 pcma/%d\r\n"
												,pRTSPStreaming->usRTPAudioPort
												,pRTSPStreaming->tAudioEncodeParam[iSDPIndex-1].iClockRate);
			//20120807 modified by Jimmy for metadata
			iResult+=snprintf(pcSDPContent+iResult, iSDPBufferLen - iResult - 1,"a=fmtp:%d\r\n",RTSPSTREAMING_AMR_MEDIATYPE);
															
		}
		else if (pRTSPStreaming->tAudioEncodeParam[iSDPIndex-1].iAudioCodecType == ractAAC4)
		{
			//20120807 modified by Jimmy for metadata
			iResult += snprintf (pcSDPContent + iResult, iSDPBufferLen - iResult - 1, "m=audio %d RTP/AVP %d\r\n"
													,pRTSPStreaming->usRTPAudioPort
													,RTSPSTREAMING_AMR_MEDIATYPE);
			
			/*iResult += sprintf (pcSDPContent + iResult,
					"a=rtpmap:%d mpeg4-generic/%d/%d\r\n" 
					"a=fmtp:%d config=",
					RTSPSTREAMING_AUDIO_MEDIATYPE,
					pRTSPStreaming->tAudioEncodeParam[iSDPIndex-1].iClockRate,
					pRTSPStreaming->tAudioEncodeParam[iSDPIndex-1].iChanNum, //2: stereo, 1: mono
					RTSPSTREAMING_AUDIO_MEDIATYPE);

			for(i=0;i<iMPEG4HeaderLength;i++)
			{
				iResult += sprintf(pcSDPContent + iResult,
					"%2.2X", pbyMPEG4Header[i]);
			}*/
			
			iResult+=snprintf(pcSDPContent+iResult, iSDPBufferLen - iResult - 1,"m=audio %d RTP/AVP 0\r\na=rtpmap:0 pcmu/%d\r\n"
												,pRTSPStreaming->usRTPAudioPort
												,pRTSPStreaming->tAudioEncodeParam[iSDPIndex-1].iClockRate);

			iResult += snprintf (pcSDPContent + iResult, iSDPBufferLen - iResult - 1,"; \r\n");			
		}
	}	

    OSSemaphore_Post(pRTSPStreaming->hMediaParamSemaphore);
	
    if( ( iResult > RTSPSTREAMING_SDP_MAXSIZE && pcSDPBuffer == NULL ) ||
        ( iResult > iSDPBufferLen && pcSDPBuffer != NULL) )
    {
        TelnetShell_DbgPrint("----Warning!!! SDP exceed buffer!!!\r\n---");
        return -2;
    }    
        
	return iResult;
	
}

#endif
//20120925 modified by Jimmy for ONVIF backchannel
//20111124 modify by danny support Backchannel Multicast for ONVIF test tool 1.02.4
int RTSPStreaming_ComposeAVSDP(HANDLE hRTSPStreaming
							   ,int iSDPIndex
							   ,unsigned long ulSDPIP
							   ,int iVivotekClient
							   ,int iMulticast
							   , char* pcSDPBuffer
							   , int iSDPBufferLen
							   , int iPlayerType
							   , int iRequire)
{
	int i;
	int iResult=0;
	DWORD dwSec=0,dwMSec=0;
    DWORD dwMulticastAddress = 0;
    unsigned short	usMulticastVideoPort = 0;
	unsigned short	usMulticastAudioPort = 0;
	//20120807 added by Jimmy for metadata
	unsigned short	usMulticastMetadataPort = 0;
	unsigned short  usTTL = 0;
	RTSPSTREAMING *pRTSPStreaming;

	char *pcSDPContent,*pcHostName;
	BYTE * pbyMPEG4Header;
	int iMPEG4HeaderLength,iMPEG4ProfileLevel,iStreamingMode;
	//20120925 added by Jimmy for ONVIF backchannel
	int iChannelIndex;
	//20140605 added by Charles for ONVIF track token
	char *pcTrackToken;

	if( hRTSPStreaming == NULL )
	    return -1;

	if( iSDPIndex < 1 || iSDPIndex > MULTIPLE_STREAM_NUM )
		return -1;
	    
	pRTSPStreaming=(RTSPSTREAMING *)hRTSPStreaming;		
	
	if( pcSDPBuffer != NULL )
	    pcSDPContent = pcSDPBuffer;
	else    
    	pcSDPContent=pRTSPStreaming->acSDPContent;
    
	pbyMPEG4Header=(BYTE *)pRTSPStreaming->tVideoEncodeParam[iSDPIndex-1].acMPEG4Header;
	iMPEG4HeaderLength=pRTSPStreaming->tVideoEncodeParam[iSDPIndex-1].iMPEG4HeaderLen;
	iMPEG4ProfileLevel=pRTSPStreaming->tVideoEncodeParam[iSDPIndex-1].iProfileLevel;
	pcHostName=pRTSPStreaming->acHostName;
	iStreamingMode=pRTSPStreaming->iRTSPStreamingMediaType[iSDPIndex-1];

    OSSemaphore_Get(pRTSPStreaming->hMediaParamSemaphore,INFINITE);

#ifdef RTSPRTP_MULTICAST

    for(i=0;i<RTSP_MULTICASTNUMBER;i++)
    {
        if( pRTSPStreaming->stMulticast[i].iSDPIndex == iSDPIndex)
        {
            dwMulticastAddress =  pRTSPStreaming->stMulticast[i].ulMulticastAddress;  
            usMulticastVideoPort= pRTSPStreaming->stMulticast[i].usMulticastVideoPort;
            usMulticastAudioPort= pRTSPStreaming->stMulticast[i].usMulticastAudioPort;
			//20120807 added by Jimmy for metadata
            usMulticastMetadataPort= pRTSPStreaming->stMulticast[i].usMulticastMetadataPort;
            usTTL               = pRTSPStreaming->stMulticast[i].usTTL;
        }
    }
    
    /*if( iVivotekClient == TRUE )
    {
        for(i=0;i<RTSP_MULTICASTNUMBER;i++)
        {
            if( pRTSPStreaming->stMulticast[i].iRTPExtension == TRUE )
                break;
        } 
        if( i == RTSP_MULTICASTNUMBER )
            i = 0;
        dwMulticastAddress = pRTSPStreaming->stMulticast[i].ulMulticastAddress;  
        usMulticastVideoPort=pRTSPStreaming->stMulticast[i].usMulticastVideoPort;
        usMulticastAudioPort=pRTSPStreaming->stMulticast[i].usMulticastAudioPort;        
        usTTL = pRTSPStreaming->stMulticast[i].usTTL;
    }
    else
    {
        for(i=0;i<RTSP_MULTICASTNUMBER;i++)
        {
            if( pRTSPStreaming->stMulticast[i].iRTPExtension == FALSE )
                break;
        } 
        if( i == RTSP_MULTICASTNUMBER )
            i = 0;
        dwMulticastAddress = pRTSPStreaming->stMulticast[i].ulMulticastAddress;  
        usMulticastVideoPort=pRTSPStreaming->stMulticast[i].usMulticastVideoPort;
        usMulticastAudioPort=pRTSPStreaming->stMulticast[i].usMulticastAudioPort;        
        usTTL = pRTSPStreaming->stMulticast[i].usTTL;
    }*/
#endif        
	OSTime_GetTimer(&dwSec,&dwMSec);

//	iResult=sprintf(pcSDPContent,"v=0\r\no=RTSP %u %u IN IP4 %s\r\n",dwSec,dwMSec,inet_ntoa(*((struct in_addr*)&(ulSDPIP))));
	iResult=snprintf(pcSDPContent, iSDPBufferLen - iResult - 1,"v=0\r\no=RTSP %u %u IN IP4 0.0.0.0\r\n",dwSec,dwMSec);
	iResult+=snprintf(pcSDPContent+iResult, iSDPBufferLen - iResult - 1,"s=%s\r\n",pcHostName);
	

	//20111124 modify by danny support Backchannel Multicast for ONVIF test tool 1.02.4
	// only vivotek client will reply SDP support backchannel multicast
	if( ( iMulticast == FALSE ) ||
		(( iMulticast == TRUE ) && ( iVivotekClient == FALSE) && ( iPlayerType != ONVIF_TEST_TOOL) && (pcSDPBuffer == NULL) ))
	{
		iResult+=snprintf(pcSDPContent+iResult, iSDPBufferLen - iResult - 1,"c=IN IP4 0.0.0.0\r\nt=0 0\r\na=charset:Shift_JIS\r\n");
	}
#ifdef RTSPRTP_MULTICAST	
	else
	{
	    iResult+=snprintf(pcSDPContent+iResult, iSDPBufferLen - iResult - 1,"c=IN IP4 %s/%d\r\nt=0 0\r\na=charset:Shift_JIS\r\n",(char*)inet_ntoa(*((struct in_addr*)&(dwMulticastAddress))),usTTL);
	}   
#endif    
#ifdef _SHARED_MEM  	
	//20100428 Added For Media on demand
	if( (iSDPIndex > LIVE_STREAM_NUM && iSDPIndex <= MULTIPLE_STREAM_NUM) && pRTSPStreaming->tVideoEncodeParam[iSDPIndex-1].dwFileLength > 0 )
	{
		iResult+=snprintf(pcSDPContent+iResult, iSDPBufferLen - iResult - 1,"a=range:npt=0-%d\r\na=control:*\r\na=etag:%s\r\n", pRTSPStreaming->tVideoEncodeParam[iSDPIndex-1].dwFileLength, pRTSPStreaming->acSDPETag);
	}
	else
#endif
	{
		//iResult+=snprintf(pcSDPContent+iResult, iSDPBufferLen - iResult - 1,"a=range:npt=0-\r\na=control:rtsp://%s/%s/\r\na=etag:%s\r\n",inet_ntoa(*((struct in_addr*)&(ulSDPIP))),pRTSPStreaming->acAccessName,pRTSPStreaming->acSDPETag);
		iResult+=snprintf(pcSDPContent+iResult, iSDPBufferLen - iResult - 1,"a=range:npt=0-\r\na=control:*\r\na=etag:%s\r\n",pRTSPStreaming->acSDPETag);
	}

	//20120807 modified by Jimmy for metadata
	if( iStreamingMode & RTSPSTREAMING_MEDIATYPE_VIDEO)
	{  
		//20140605 added by Charles for ONVIF track token
#ifdef _SHARED_MEM 
		if( iSDPIndex > LIVE_STREAM_NUM && iSDPIndex <= MULTIPLE_STREAM_NUM )
		{
			pcTrackToken = pRTSPStreaming->tVideoEncodeParam[iSDPIndex-1].acTrackToken;
		}
		else
#endif
		{
			pcTrackToken = pRTSPStreaming->tVideoEncodeParam[iSDPIndex-1].acTrackName;
		}		

		if(pRTSPStreaming->tVideoEncodeParam[iSDPIndex-1].eVideoCodecType == mctMP4V)
		{
			if( ( iMulticast == FALSE ) || (( iMulticast == TRUE ) && ( iVivotekClient == FALSE) && (pcSDPBuffer == NULL) ))
			{
				//20120807 modified by Jimmy for metadata
				iResult+=snprintf(pcSDPContent+iResult, iSDPBufferLen - iResult - 1,"m=video 0 RTP/AVP %d\r\nb=AS:%.0f\r\na=rtpmap:%d MP4V-ES/%d\r\n",RTSPSTREAMING_MPEG4_MEDIATYPE,(float)pRTSPStreaming->tVideoEncodeParam[iSDPIndex-1].iBitRate/(float)1000,RTSPSTREAMING_MPEG4_MEDIATYPE,pRTSPStreaming->tVideoEncodeParam[iSDPIndex-1].iClockRate);
			}
#ifdef RTSPRTP_MULTICAST		
			else
			{
				//20120807 modified by Jimmy for metadata
				iResult+=snprintf(pcSDPContent+iResult, iSDPBufferLen - iResult - 1,"m=video %d RTP/AVP %d\r\nb=AS:%.0f\r\na=rtpmap:%d MP4V-ES/%d\r\n",usMulticastVideoPort,RTSPSTREAMING_MPEG4_MEDIATYPE,(float)pRTSPStreaming->tVideoEncodeParam[iSDPIndex-1].iBitRate/(float)1000,RTSPSTREAMING_MPEG4_MEDIATYPE,pRTSPStreaming->tVideoEncodeParam[iSDPIndex-1].iClockRate);        
			}
#endif        			   
			//20130221 modified by Jimmy to support ONVIF TrackReference
			//20120807 modified by Jimmy for metadata
			//iResult+=snprintf(pcSDPContent+iResult, iSDPBufferLen - iResult - 1,"a=control:%s\r\na=fmtp:%d profile-level-id=%d;config=",pRTSPStreaming->tVideoEncodeParam[iSDPIndex-1].acTrackName,RTSPSTREAMING_MPEG4_MEDIATYPE,iMPEG4ProfileLevel);
			iResult+=snprintf(pcSDPContent+iResult, iSDPBufferLen - iResult - 1,"a=control:%s\r\na=x-onvif-track:%s\r\na=fmtp:%d profile-level-id=%d;config=",pRTSPStreaming->tVideoEncodeParam[iSDPIndex-1].acTrackName,pcTrackToken,RTSPSTREAMING_MPEG4_MEDIATYPE,iMPEG4ProfileLevel);

			for(i=0;i<iMPEG4HeaderLength;i++)
				iResult+=snprintf(pcSDPContent+iResult, iSDPBufferLen - iResult - 1,"%2.2X",pbyMPEG4Header[i]);
			
			iResult+=snprintf(pcSDPContent+iResult, iSDPBufferLen - iResult - 1,";decode_buf=%d\r\n", pRTSPStreaming->tVideoEncodeParam[iSDPIndex-1].iDecoderBufferSize);
		}
		else if(pRTSPStreaming->tVideoEncodeParam[iSDPIndex-1].eVideoCodecType == mctJPEG)
		{
			if( ( iMulticast == FALSE ) || (( iMulticast == TRUE ) && ( iVivotekClient == FALSE) && (pcSDPBuffer == NULL) ))
			{
				iResult+=snprintf(pcSDPContent+iResult, iSDPBufferLen - iResult - 1,"m=video 0 RTP/AVP %d\r\n",RTSPSTREAMING_JPEG_MEDIATYPE);
			}
#ifdef RTSPRTP_MULTICAST		
			else
			{
				iResult+=snprintf(pcSDPContent+iResult, iSDPBufferLen - iResult - 1,"m=video %d RTP/AVP %d\r\n",usMulticastVideoPort,RTSPSTREAMING_JPEG_MEDIATYPE);        
			}
#endif        			   
			//20130221 modified by Jimmy to support ONVIF TrackReference
			//iResult+=snprintf(pcSDPContent+iResult, iSDPBufferLen - iResult - 1,"a=control:%s\r\n",pRTSPStreaming->tVideoEncodeParam[iSDPIndex-1].acTrackName);
			iResult+=snprintf(pcSDPContent+iResult, iSDPBufferLen - iResult - 1,"a=control:%s\r\na=x-onvif-track:%s\r\n",pRTSPStreaming->tVideoEncodeParam[iSDPIndex-1].acTrackName,pcTrackToken);
		}
		else if(pRTSPStreaming->tVideoEncodeParam[iSDPIndex-1].eVideoCodecType == mctH264)
		{
			if( ( iMulticast == FALSE ) || (( iMulticast == TRUE ) && ( iVivotekClient == FALSE) && (pcSDPBuffer == NULL) ))
			{
				iResult+=snprintf(pcSDPContent+iResult, iSDPBufferLen - iResult - 1,"m=video 0 RTP/AVP %d\r\nb=AS:%.0f\r\na=rtpmap:%d H264/%d\r\n",RTSPSTREAMING_H264_MEDIATYPE,(float)pRTSPStreaming->tVideoEncodeParam[iSDPIndex-1].iBitRate/(float)1000,RTSPSTREAMING_H264_MEDIATYPE,pRTSPStreaming->tVideoEncodeParam[iSDPIndex-1].iClockRate);
			}
#ifdef RTSPRTP_MULTICAST		
			else
			{
				iResult+=snprintf(pcSDPContent+iResult, iSDPBufferLen - iResult - 1,"m=video %d RTP/AVP %d\r\nb=AS:%.0f\r\na=rtpmap:%d H264/%d\r\n",usMulticastVideoPort,RTSPSTREAMING_H264_MEDIATYPE,(float)pRTSPStreaming->tVideoEncodeParam[iSDPIndex-1].iBitRate/(float)1000,RTSPSTREAMING_H264_MEDIATYPE,pRTSPStreaming->tVideoEncodeParam[iSDPIndex-1].iClockRate);        
			}
#endif
			//20130221 modified by Jimmy to support ONVIF TrackReference          
			/*
			iResult+=snprintf(pcSDPContent+iResult, iSDPBufferLen - iResult - 1,"a=control:%s\r\na=fmtp:%d packetization-mode=%d; profile-level-id=%s; sprop-parameter-sets=%s\r\n",
				pRTSPStreaming->tVideoEncodeParam[iSDPIndex-1].acTrackName,
				RTSPSTREAMING_H264_MEDIATYPE,
				pRTSPStreaming->tVideoEncodeParam[iSDPIndex-1].iPacketizationMode,
				pRTSPStreaming->tVideoEncodeParam[iSDPIndex-1].acProfileLevelID,
				pRTSPStreaming->tVideoEncodeParam[iSDPIndex-1].acSpropParamSet);
			*/
			iResult+=snprintf(pcSDPContent+iResult, iSDPBufferLen - iResult - 1,"a=control:%s\r\na=fmtp:%d packetization-mode=%d; profile-level-id=%s; sprop-parameter-sets=%s\r\n",
				pRTSPStreaming->tVideoEncodeParam[iSDPIndex-1].acTrackName,
				RTSPSTREAMING_H264_MEDIATYPE,
				pRTSPStreaming->tVideoEncodeParam[iSDPIndex-1].iPacketizationMode,
				pRTSPStreaming->tVideoEncodeParam[iSDPIndex-1].acProfileLevelID,
				pRTSPStreaming->tVideoEncodeParam[iSDPIndex-1].acSpropParamSet);
		}
        //20150113 added by Charles for H.265
        else if(pRTSPStreaming->tVideoEncodeParam[iSDPIndex-1].eVideoCodecType == mctH265)
        {
            if( ( iMulticast == FALSE ) || (( iMulticast == TRUE ) && ( iVivotekClient == FALSE) && (pcSDPBuffer == NULL) ))
            {
                iResult+=snprintf(pcSDPContent+iResult, iSDPBufferLen - iResult - 1,"m=video 0 RTP/AVP %d\r\nb=AS:%.0f\r\na=rtpmap:%d H265/%d\r\n",RTSPSTREAMING_H265_MEDIATYPE,(float)pRTSPStreaming->tVideoEncodeParam[iSDPIndex-1].iBitRate/(float)1000,RTSPSTREAMING_H265_MEDIATYPE,pRTSPStreaming->tVideoEncodeParam[iSDPIndex-1].iClockRate);
            }
#ifdef RTSPRTP_MULTICAST		
            else
            {
                iResult+=snprintf(pcSDPContent+iResult, iSDPBufferLen - iResult - 1,"m=video %d RTP/AVP %d\r\nb=AS:%.0f\r\na=rtpmap:%d H265/%d\r\n",usMulticastVideoPort,RTSPSTREAMING_H265_MEDIATYPE,(float)pRTSPStreaming->tVideoEncodeParam[iSDPIndex-1].iBitRate/(float)1000,RTSPSTREAMING_H265_MEDIATYPE,pRTSPStreaming->tVideoEncodeParam[iSDPIndex-1].iClockRate);        
            }
#endif
            iResult+=snprintf(pcSDPContent+iResult, iSDPBufferLen - iResult - 1,
                "a=control:%s\r\na=x-onvif-track:%s\r\na=fmtp:%d sprop-vps=%s; sprop-sps=%s; sprop-pps=%s\r\n",
				pRTSPStreaming->tVideoEncodeParam[iSDPIndex-1].acTrackName,
				pcTrackToken,
				RTSPSTREAMING_H265_MEDIATYPE,
				pRTSPStreaming->tVideoEncodeParam[iSDPIndex-1].acSpropVPSParam,
				pRTSPStreaming->tVideoEncodeParam[iSDPIndex-1].acSpropSPSParam,
				pRTSPStreaming->tVideoEncodeParam[iSDPIndex-1].acSpropPPSParam);
        }
		//20120925 added by Jimmy for ONVIF backchannel
		if( iRequire == REQUIRE_ONVIF_BACKCHANNEL)
		{
			iResult+=snprintf(pcSDPContent+iResult, iSDPBufferLen - iResult - 1,"a=recvonly\r\n");
		}
	}

	pbyMPEG4Header = (BYTE *)pRTSPStreaming->tAudioEncodeParam[iSDPIndex-1].acM4ASpecConf;
	iMPEG4HeaderLength = pRTSPStreaming->tAudioEncodeParam[iSDPIndex-1].iM4ASpecConfLen;
	iMPEG4ProfileLevel = pRTSPStreaming->tAudioEncodeParam[iSDPIndex-1].iM4AProfileLevel;

	//20120807 modified by Jimmy for metadata
	if( iStreamingMode & RTSPSTREAMING_MEDIATYPE_AUDIO)
	{
		//20140605 added by Charles for ONVIF track token
#ifdef _SHARED_MEM 
		if( iSDPIndex > LIVE_STREAM_NUM && iSDPIndex <= MULTIPLE_STREAM_NUM )
		{
			pcTrackToken = pRTSPStreaming->tAudioEncodeParam[iSDPIndex-1].acTrackToken;
		}
		else
#endif
		{
			pcTrackToken = pRTSPStreaming->tAudioEncodeParam[iSDPIndex-1].acTrackName;
		}		

		//if(pRTSPStreaming->iAudioCodecType == ractGAMR)
		if(pRTSPStreaming->tAudioEncodeParam[iSDPIndex-1].iAudioCodecType == ractGAMR)
		{

			if( ( iMulticast == FALSE ) ||
				(( iMulticast == TRUE ) && ( iVivotekClient == FALSE) && (pcSDPBuffer == NULL) ))
			{
				//20130221 modified by Jimmy to support ONVIF TrackReference
				//20120807 modified by Jimmy for metadata
				/*
				iResult+=snprintf(pcSDPContent+iResult, iSDPBufferLen - iResult - 1,"m=audio 0 RTP/AVP %d\r\nb=AS:%.0f\r\na=rtpmap:%d AMR/%d\r\na=control:%s\r\n"
													,RTSPSTREAMING_AMR_MEDIATYPE
													,(float)pRTSPStreaming->tAudioEncodeParam[iSDPIndex-1].iBitRate/(float)1000
													,RTSPSTREAMING_AMR_MEDIATYPE
													,pRTSPStreaming->tAudioEncodeParam[iSDPIndex-1].iClockRate
													,pRTSPStreaming->tAudioEncodeParam[iSDPIndex-1].acTrackName);
				*/
				//20140605 modified by Charles for ONVIF track token
				iResult+=snprintf(pcSDPContent+iResult, iSDPBufferLen - iResult - 1,"m=audio 0 RTP/AVP %d\r\nb=AS:%.0f\r\na=rtpmap:%d AMR/%d\r\na=control:%s\r\na=x-onvif-track:%s\r\n"
													,RTSPSTREAMING_AMR_MEDIATYPE
													,(float)pRTSPStreaming->tAudioEncodeParam[iSDPIndex-1].iBitRate/(float)1000
													,RTSPSTREAMING_AMR_MEDIATYPE
													,pRTSPStreaming->tAudioEncodeParam[iSDPIndex-1].iClockRate
													,pRTSPStreaming->tAudioEncodeParam[iSDPIndex-1].acTrackName
													,pcTrackToken);
			}
#ifdef RTSPRTP_MULTICAST			
			else
			{
				//20130221 modified by Jimmy to support ONVIF TrackReference
				//20120807 modified by Jimmy for metadata
				/*
			    iResult+=snprintf(pcSDPContent+iResult, iSDPBufferLen - iResult - 1,"m=audio %d RTP/AVP %d\r\nb=AS:%.0f\r\na=rtpmap:%d AMR/%d\r\na=control:%s\r\n"
													,usMulticastAudioPort
													,RTSPSTREAMING_AMR_MEDIATYPE
													,(float)pRTSPStreaming->tAudioEncodeParam[iSDPIndex-1].iBitRate/(float)1000
													,RTSPSTREAMING_AMR_MEDIATYPE
													,pRTSPStreaming->tAudioEncodeParam[iSDPIndex-1].iClockRate
													,pRTSPStreaming->tAudioEncodeParam[iSDPIndex-1].acTrackName);
				*/
				//20140605 modified by Charles for ONVIF track token
			    iResult+=snprintf(pcSDPContent+iResult, iSDPBufferLen - iResult - 1,"m=audio %d RTP/AVP %d\r\nb=AS:%.0f\r\na=rtpmap:%d AMR/%d\r\na=control:%s\r\na=x-onvif-track:%s\r\n"
													,usMulticastAudioPort
													,RTSPSTREAMING_AMR_MEDIATYPE
													,(float)pRTSPStreaming->tAudioEncodeParam[iSDPIndex-1].iBitRate/(float)1000
													,RTSPSTREAMING_AMR_MEDIATYPE
													,pRTSPStreaming->tAudioEncodeParam[iSDPIndex-1].iClockRate
													,pRTSPStreaming->tAudioEncodeParam[iSDPIndex-1].acTrackName
													,pcTrackToken);
			}
#endif			
						    
/*			iResult+=sprintf(pcSDPContent+iResult,"a=maxptime:%d\r\na=fmtp:%d octet-align=%d;crc=%d;robust-sorting=%d;decode_buf=400\r\n",
				pRTSPStreaming->tAudioEncodeParam[iSDPIndex-1].iPacketTime,RTSPSTREAMING_AUDIO_MEDIATYPE,pRTSPStreaming->tAudioEncodeParam[iSDPIndex-1].iOctetAlign,pRTSPStreaming->tAudioEncodeParam[iSDPIndex-1].iAMRcrc,pRTSPStreaming->tAudioEncodeParam[iSDPIndex-1].iRobustSorting);*/
			//20120807 modified by Jimmy for metadata
			iResult+=snprintf(pcSDPContent+iResult, iSDPBufferLen - iResult - 1,"a=maxptime:%d\r\na=fmtp:%d decode_buf=400;octet-align=%d\r\n"
												,pRTSPStreaming->tAudioEncodeParam[iSDPIndex-1].iPacketTime
												,RTSPSTREAMING_AMR_MEDIATYPE
												,pRTSPStreaming->tAudioEncodeParam[iSDPIndex-1].iOctetAlign);
		}
		else if (pRTSPStreaming->tAudioEncodeParam[iSDPIndex-1].iAudioCodecType == ractAAC4)
		{

			if( ( iMulticast == FALSE ) ||
				(( iMulticast == TRUE ) && ( iVivotekClient == FALSE) && (pcSDPBuffer == NULL) ))
			{
				//20120807 modified by Jimmy for metadata
				iResult += snprintf (pcSDPContent + iResult, iSDPBufferLen - iResult - 1,	"m=audio 0 RTP/AVP %d\r\n",RTSPSTREAMING_AMR_MEDIATYPE);
			}
#ifdef RTSPRTP_MULTICAST			
			else
			{
				//20120807 modified by Jimmy for metadata
				iResult += snprintf (pcSDPContent + iResult, iSDPBufferLen - iResult - 1,	"m=audio %d RTP/AVP %d\r\n",usMulticastAudioPort,RTSPSTREAMING_AMR_MEDIATYPE);				
			}
#endif			
			//20130221 modified by Jimmy to support ONVIF TrackReference
			//20120807 modified by Jimmy for metadata
			/*
			iResult += snprintf (pcSDPContent + iResult, iSDPBufferLen - iResult - 1,
				"a=control:%s\r\n" 
				"a=rtpmap:%d mpeg4-generic/%d/%d\r\n" 
				"a=fmtp:%d streamtype=5; profile-level-id=%d; "
				"mode=AAC-hbr; config=", 
				pRTSPStreaming->tAudioEncodeParam[iSDPIndex-1].acTrackName,
				RTSPSTREAMING_AMR_MEDIATYPE,
				pRTSPStreaming->tAudioEncodeParam[iSDPIndex-1].iClockRate,
				pRTSPStreaming->tAudioEncodeParam[iSDPIndex-1].iChanNum, //2: stereo, 1: mono
				RTSPSTREAMING_AMR_MEDIATYPE, 
				iMPEG4ProfileLevel); // profile-level-id
			*/
			//20140605 modified by Charles for ONVIF track token
			iResult += snprintf (pcSDPContent + iResult, iSDPBufferLen - iResult - 1,
				"a=control:%s\r\n" 
				"a=x-onvif-track:%s\r\n"
				"a=rtpmap:%d mpeg4-generic/%d/%d\r\n" 
				"a=fmtp:%d streamtype=5; profile-level-id=%d; "
				"mode=AAC-hbr; config=", 
				pRTSPStreaming->tAudioEncodeParam[iSDPIndex-1].acTrackName,
				pcTrackToken,
				RTSPSTREAMING_AMR_MEDIATYPE,
				pRTSPStreaming->tAudioEncodeParam[iSDPIndex-1].iClockRate,
				pRTSPStreaming->tAudioEncodeParam[iSDPIndex-1].iChanNum, //2: stereo, 1: mono
				RTSPSTREAMING_AMR_MEDIATYPE, 
				iMPEG4ProfileLevel); // profile-level-id
			for(i=0;i<iMPEG4HeaderLength;i++)
			{
				iResult += snprintf(pcSDPContent + iResult,iSDPBufferLen - iResult - 1,
					"%2.2X", pbyMPEG4Header[i]);
			}
			iResult += snprintf (pcSDPContent + iResult, iSDPBufferLen - iResult - 1,
				";SizeLength=13; "
				"IndexLength=3; IndexDeltaLength=3; "
				"CTSDeltaLength=0; DTSDeltaLength=0; \r\n");
		}
#ifdef _G711_AUDIOIN
		else if (pRTSPStreaming->tAudioEncodeParam[iSDPIndex-1].iAudioCodecType==ractG711u || pRTSPStreaming->tAudioEncodeParam[iSDPIndex-1].iAudioCodecType==ractG711a)
		{
			int iAudioRTPType = (pRTSPStreaming->tAudioEncodeParam[iSDPIndex-1].iAudioCodecType == ractG711u) ? 0 : 8 ;

			if( ( iMulticast == FALSE ) ||
				(( iMulticast == TRUE ) && ( iVivotekClient == FALSE) && (pcSDPBuffer == NULL) ))
			{
				iResult += snprintf (pcSDPContent + iResult, iSDPBufferLen - iResult - 1,	"m=audio 0 RTP/AVP %d\r\n",iAudioRTPType);
			}
#ifdef RTSPRTP_MULTICAST			
			else
			{
				iResult += snprintf (pcSDPContent + iResult, iSDPBufferLen - iResult - 1,	"m=audio %d RTP/AVP %d\r\n",usMulticastAudioPort,iAudioRTPType);				
			}
#endif			
			//20130221 modified by Jimmy to support ONVIF TrackReference
			/*
			iResult += snprintf (pcSDPContent + iResult, iSDPBufferLen - iResult - 1,
				"a=control:%s\r\n" 
				"a=rtpmap:%d %s/8000\r\n",
				pRTSPStreaming->tAudioEncodeParam[iSDPIndex-1].acTrackName,
				iAudioRTPType,
				(iAudioRTPType == 0) ? "pcmu" : "pcma"
				); // profile-level-id
			*/
			//20140605 modified by Charles for ONVIF track token
			iResult += snprintf (pcSDPContent + iResult, iSDPBufferLen - iResult - 1,
				"a=control:%s\r\n" 
				"a=x-onvif-track:%s\r\n"
				"a=rtpmap:%d %s/8000\r\n",
				pRTSPStreaming->tAudioEncodeParam[iSDPIndex-1].acTrackName,
				pcTrackToken,
				iAudioRTPType,
				(iAudioRTPType == 0) ? "pcmu" : "pcma"
				); // profile-level-id
		}
#endif
#ifdef _G726_AUDIOIN
		else if (pRTSPStreaming->tAudioEncodeParam[iSDPIndex-1].iAudioCodecType==ractG726)
		{		
			if( ( iMulticast == FALSE ) ||
				(( iMulticast == TRUE ) && ( iVivotekClient == FALSE) && (pcSDPBuffer == NULL) ))
			{
				iResult += snprintf (pcSDPContent + iResult, iSDPBufferLen - iResult - 1,	"m=audio 0 RTP/AVP %d\r\n",RTSPSTREAMING_AMR_MEDIATYPE);
			}
#ifdef RTSPRTP_MULTICAST			
			else
			{
				iResult += snprintf (pcSDPContent + iResult, iSDPBufferLen - iResult - 1,	"m=audio %d RTP/AVP %d\r\n",usMulticastAudioPort,RTSPSTREAMING_AMR_MEDIATYPE);				
			}
#endif			
			//20140605 modified by Charles for ONVIF track token
			//20130221 modified by Jimmy to support ONVIF TrackReference
			iResult += snprintf (pcSDPContent + iResult, iSDPBufferLen - iResult - 1,
				"a=control:%s\r\n" 
				"a=x-onvif-track:%s\r\n"
				"a=rtpmap:%d %sG726-%d/8000\r\n",
				pRTSPStreaming->tAudioEncodeParam[iSDPIndex-1].acTrackName,
				pcTrackToken,
				RTSPSTREAMING_AMR_MEDIATYPE,
				(pRTSPStreaming->tAudioEncodeParam[iSDPIndex-1].bIsBigEndian? "AAL2-":""),
				(pRTSPStreaming->tAudioEncodeParam[iSDPIndex-1].iBitRate/1000)
				); 
		}
#endif


		//20120925 added by Jimmy for ONVIF backchannel
		if( iRequire == REQUIRE_ONVIF_BACKCHANNEL )
		{
			iResult+=snprintf(pcSDPContent+iResult, iSDPBufferLen - iResult - 1,"a=recvonly\r\n");
		}

	}	

	//20120807 added by Jimmy for metadata
	if( iStreamingMode & RTSPSTREAMING_MEDIATYPE_METADATA)
	{
		if( ( iMulticast == FALSE ) ||
			(( iMulticast == TRUE ) && ( iVivotekClient == FALSE) && (pcSDPBuffer == NULL) ))
		{
			iResult += snprintf (pcSDPContent + iResult, iSDPBufferLen - iResult - 1,	"m=application 0 RTP/AVP %d\r\n",RTSPSTREAMING_METADATA_MEDIATYPE);
		}
#ifdef RTSPRTP_MULTICAST			
		else
		{
			iResult += snprintf (pcSDPContent + iResult, iSDPBufferLen - iResult - 1,	"m=application %d RTP/AVP %d\r\n",usMulticastMetadataPort,RTSPSTREAMING_METADATA_MEDIATYPE);
		}
#endif
		iResult += snprintf (pcSDPContent + iResult, iSDPBufferLen - iResult - 1,
			"a=control:%s\r\n" 
			"a=rtpmap:%d vnd.onvif.metadata/%d\r\n",
			pRTSPStreaming->tMetadataEncodeParam[iSDPIndex-1].acTrackName,
			RTSPSTREAMING_METADATA_MEDIATYPE,
			pRTSPStreaming->tMetadataEncodeParam[iSDPIndex-1].iClockRate
			); // profile-level-id

	
		//20120925 added by Jimmy for ONVIF backchannel
		if( iRequire == REQUIRE_ONVIF_BACKCHANNEL )
		{
			iResult+=snprintf(pcSDPContent+iResult, iSDPBufferLen - iResult - 1,"a=recvonly\r\n");
		}

	}	

	//20120925 added by Jimmy for ONVIF backchannel
	if( (iRequire == REQUIRE_ONVIF_BACKCHANNEL) && (iSDPIndex <= LIVE_STREAM_NUM) )
	{

		iChannelIndex = pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle,
														0,
														ccctGetMultipleChannelChannelIndex,
														(DWORD)iSDPIndex);
		iResult += snprintf (pcSDPContent + iResult, iSDPBufferLen - iResult - 1,
			"m=audio 0 RTP/AVP 0\r\n"
			"a=control:%s\r\n" 
			"a=rtpmap:0 PCMU/8000\r\n"
			"a=sendonly\r\n",
			pRTSPStreaming->tAudioDecodeParam[iChannelIndex-1].acTrackName
			); // profile-level-id
	}

	
    OSSemaphore_Post(pRTSPStreaming->hMediaParamSemaphore);
	
    if( ( iResult > RTPSTREAMING_SDP_LENGTH && pcSDPBuffer == NULL ) ||
        ( iResult > iSDPBufferLen && pcSDPBuffer != NULL) )
    {
        TelnetShell_DbgPrint("----Warning!!! SDP exceed buffer!!!\r\n---");
        return -2;
    }    
        
	return iResult;
	
}


#ifdef RTSPRTP_MULTICAST
//20120806 modified by Jimmy for metadata
//20110725 Add by danny For Multicast RTCP receive report keep alive
int RTSPStreaming_BindLocalPortForMulticastSocetks(int* pSocketArray, unsigned short usVideoPort, unsigned short usAudioPort, unsigned short usMetadataPort, unsigned short usLocalPortOffset)
{
	int				iIndex=0,n=0,i;
	unsigned short 	usPort;
	struct sockaddr_in addr;
    //int iInput;

	//20120806 modified by Jimmy for metadata
	for( i=0; i<MEDIA_TYPE_NUMBER*2; i++ )
	{
		if((*(pSocketArray+i) = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		{
			return -1;
		}	
		
#ifdef _VRNT
		//20120807 Add by danny For support re-used port in Multicast UDP socket
		int iOpt = 1;
		if( (setsockopt(*(pSocketArray+i), SOL_SOCKET, SO_REUSEADDR, (char *) &iOpt, sizeof(iOpt))) !=0 )
		{
			
			printf("[%s] Could not set SO_REUSEADDR: %s\n", __FUNCTION__, strerror(errno));
			return -1;
		}
#endif
	}

	//20120806 modified by Jimmy for metadata
	for( i=0; i<MEDIA_TYPE_NUMBER; i++ )
	{		
		iIndex = 0;
		while( iIndex<2 )
    	{      		
			memset(&addr,0,sizeof(addr));
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = htonl(INADDR_ANY);

			if( n > 200 )
    	    {
        	    return -1;
        	}
			
    		iIndex = 0;
			//20110725 Add by danny For Multicast RTCP receive report keep alive
			if( i == 0 )
			{
    			usPort = usVideoPort + n + usLocalPortOffset;//8888 + n;;
			}
			else if( i == 1)
			{
				usPort = usAudioPort + n + usLocalPortOffset;
			}
			//20120806 added by Jimmy for metadata
			else
			{
				usPort = usMetadataPort + n + usLocalPortOffset;
			}
				
        	addr.sin_port = htons(usPort);

        	if( bind(*(pSocketArray+i*2),(struct sockaddr *) &addr,sizeof(struct sockaddr_in)) == 0 )
        	{
            	iIndex ++;
            	//printf("bind %d ok\n",*(pSocketArray+i*2));
        	}
			else
			{
				n = n + 2 ;
				printf("[%s]Bind socket %d port %d failed, port shift %d\n", __func__, *(pSocketArray+i*2), usPort, n);
				//20140103 modified by Charles for ondemand multicast
				syslog(LOG_DEBUG," [%s] Bind socket %d port %d failed, port shift %d\n", __FUNCTION__, *(pSocketArray+i*2), usPort, n);
				continue ;
			}
  
  			addr.sin_port = htons(usPort+1);
        	if( bind(*(pSocketArray+(i*2+1)),(struct sockaddr *) &addr,sizeof(struct sockaddr_in)) == 0 )
        	{
            	iIndex ++;
            	//printf("bind %d ok\n",*(pSocketArray+i*2+1));
        	}
			else
			{
				closesocket(*(pSocketArray+i*2));
				*(pSocketArray+i*2) = socket(AF_INET, SOCK_DGRAM, 0) ;
			}
        
        	//n = n + 2 ;
    	}
	}
	
	return 0;
}

//20070920 YenChun Modified to support Mulitcast TTL
//20120801 modified by Jimmy for metadata
//20160127 modified by Faber,
int  RTSPStreaming_CreateMulticastSocket(unsigned long ulMulticastAddress
										,unsigned long ulMulticastAudioAddress
										,unsigned long ulMulticastMetadataAddres
										 ,unsigned short usVideoPort
										 ,unsigned short usAudioPort
										 ,unsigned short usMetadataPort
										 ,int            *pSocketArray
										 ,unsigned char ucTTL
										 ,unsigned short usLocalPortOffset)
{
    SOCKET sckSSDP = INVALID_SOCKET;
    int    iIndexA;
    //struct sockaddr_in  ssdpAddr;
	//20110627 Remove by danny for join/leave multicast group by session start/stop
	/*int    iResult;
    struct ip_mreq      ssdpMcastAddr;*/
    struct sockaddr_in destaddr;
	//Added by Louis 20071221
	u_char loop;

	//20110725 Add by danny For Multicast RTCP receive report keep alive
	//20120807 modified by Jimmy for metadata
	if( RTSPStreaming_BindLocalPortForMulticastSocetks(pSocketArray, usVideoPort, usAudioPort, usMetadataPort, usLocalPortOffset) != 0 )
		return -1;

	//20120801 modified by Jimmy for metadata
	for(iIndexA=0 ; iIndexA<MEDIA_TYPE_NUMBER*2; iIndexA++)
	{
		/*if( iIndexA == 0 )
			ssdpAddr.sin_port = htons(usVideoPort);			
		if( iIndexA == 1 )
			ssdpAddr.sin_port = htons(usVideoPort+1);
		if( iIndexA == 2 )
			ssdpAddr.sin_port = htons(usAudioPort);
		if( iIndexA == 3 )
			ssdpAddr.sin_port = htons(usAudioPort+1);

		if(bind(sckSSDP, (struct sockaddr *)&ssdpAddr, sizeof(ssdpAddr)) != 0)
		{
			shutdown(sckSSDP, 2);
			closesocket(sckSSDP);
			return -1;
		}*/
       
		//20110627 Remove by danny for join/leave multicast group by session start/stop
		//join multicast address for RTCP only;		
		/*if( iIndexA%2 != 0 )
		{		            
    		memset((void *)&ssdpMcastAddr, 0, sizeof(ssdpMcastAddr));
	    	ssdpMcastAddr.imr_interface.s_addr = INADDR_ANY;
		    ssdpMcastAddr.imr_multiaddr.s_addr = ulMulticastAddress;

		    if((iResult = setsockopt(sckSSDP, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                (char *)&ssdpMcastAddr, sizeof(ssdpMcastAddr))) != 0 )
		    {
			    TelnetShell_DbgPrint("Error in joing multicast group %d!!\r\n",WSAGetLastError());				
			    shutdown(sckSSDP, 2);
			    closesocket(sckSSDP);
			    return -1;
		    }
		}*/    
		
		sckSSDP = *(pSocketArray+iIndexA);
		//printf("multicast sokcet %d\n",*(pSocketArray+iIndexA));			
		 // connect to remote multicast address port
        memset(&destaddr, 0, sizeof(destaddr));
		destaddr.sin_family = AF_INET;
		
		//20160127 Modify by Faber, separate the address of audio/metadata
		if( iIndexA == 0 )
		{
			destaddr.sin_addr.s_addr = ulMulticastAddress;
			destaddr.sin_port = htons(usVideoPort);
		}
		if( iIndexA == 1 )
		{
			destaddr.sin_addr.s_addr = ulMulticastAddress;
			destaddr.sin_port = htons(usVideoPort+1);
		}
		if( iIndexA == 2 )
		{
			destaddr.sin_addr.s_addr = ulMulticastAudioAddress;
			destaddr.sin_port = htons(usAudioPort);
		}

		if( iIndexA == 3 )
		{
			destaddr.sin_addr.s_addr = ulMulticastAudioAddress;
			destaddr.sin_port = htons(usAudioPort+1);
		}

		//20120801 added by Jimmy for metadata
		if( iIndexA == 4 )
		{
			destaddr.sin_addr.s_addr = ulMulticastMetadataAddres;
			destaddr.sin_port = htons(usMetadataPort);
		}

		if( iIndexA == 5 )
		{
			destaddr.sin_addr.s_addr = ulMulticastMetadataAddres;
			destaddr.sin_port = htons(usMetadataPort+1);
		}


        //printf("connect to remote multicast address %lu port %d\n",ulMulticastAddress, destaddr.sin_port);   
        //20110725 Add by danny For Multicast RTCP receive report keep alive
        //20120801 modified by Jimmy for metadata
		if( iIndexA == 0 || iIndexA == 2 ||  iIndexA == 4 )
		{
			if( connect(sckSSDP,(struct sockaddr*)&destaddr,sizeof(destaddr)) != 0 )
			{
				TelnetShell_DbgPrint("Error in connect multicast group %d!!\r\n",errno);				
				shutdown(sckSSDP, 2);
				closesocket(sckSSDP);

				shutdown(*(pSocketArray+iIndexA + 1), 2);
				closesocket(*(pSocketArray+iIndexA + 1));
				return -1;
			}
		}

		//20070920 YenChun Modified to support Mulitcast TTL
		if(setsockopt(sckSSDP, IPPROTO_IP,IP_MULTICAST_TTL,&ucTTL,sizeof(ucTTL)) != 0 )
		{
			TelnetShell_DbgPrint("Error set IP_MULTICAST_TTL!!\r\n");
		}
		//20071221 Louis Modified for Multicast Loopback Test
		loop = 0;
		if(setsockopt(sckSSDP, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop)) != 0)
		{
			TelnetShell_DbgPrint("Error set IP_MULTICAST_LOOP!!\r\n");
		}
		// result is not checked becuase it will fail in WinMe and Win9x.
		//ShengFu
	    //ttl = FALSE;	
	    //setsockopt( sckSSDP, IPPROTO_IP,
	    //          IP_MULTICAST_LOOP, &ttl, sizeof( ttl ) );

		//printf("set broadcast\n");
		/*i = 1;
		if(setsockopt(sckSSDP, SOL_SOCKET, SO_BROADCAST, (char *)&i, sizeof(i) ) != 0)
		{
			DbgPrint(("["MOD_NAME"] Error set broadcasting!!\r\n"));
			shutdown(sckSSDP, 2);
			closesocket(sckSSDP);
			return -1;
		}*/
	}
	
	return 0;
	//return RTSPStreaming_BindLocalPortForMulticastSocetks(pSocketArray);

}
#endif

//20120801 modified by Jimmy for metadata
int RTSPStreaming_CreateFixedUDPSocket(RTSPSTREAMING *pRTSPStreaming, unsigned short usRTPVPort, unsigned short usRTPAPort, unsigned short usRTPMPort, unsigned short usRTCPVPort, unsigned short usRTCPAPort,  unsigned short usRTCPMPort)
{
#ifdef _INET6
	int iAddressFamily = AF_INET6;
#else
	int iAddressFamily = AF_INET;
#endif
	RTSP_SOCKADDR addr;

	int		iInput;
#ifdef _SHARED_MEM
	//20091116 support connected UDP
	int 	iOpt;
#endif
	if( (pRTSPStreaming->iRTPVSock = socket(iAddressFamily,SOCK_DGRAM,0)) <= 0 )
    {
        return -1;
    }

	if( (pRTSPStreaming->iRTPASock = socket(iAddressFamily,SOCK_DGRAM,0)) <= 0 )
    {
        return -1;
    }      

	if( (pRTSPStreaming->iRTCPVSock = socket(iAddressFamily,SOCK_DGRAM,0)) <= 0 )
    {
        return -1;
    }        
    
	if( (pRTSPStreaming->iRTCPASock = socket(iAddressFamily,SOCK_DGRAM,0)) <= 0 )
    {
        return -1;
    }  
    //20120801 added by Jimmy for metadata
#ifdef _METADATA_ENABLE
	if( (pRTSPStreaming->iRTPMSock = socket(iAddressFamily,SOCK_DGRAM,0)) <= 0 )
    {
        return -1;
    }

	if( (pRTSPStreaming->iRTCPMSock = socket(iAddressFamily,SOCK_DGRAM,0)) <= 0 )
    {
        return -1;
    }
#else
	//20130403 added by Jimmy to fix high CPU usage issue when the metadata build option is not enbaled
	pRTSPStreaming->iRTPMSock = -1;
	pRTSPStreaming->iRTCPMSock = -1;
#endif
	//20120801 added by Jimmy for metadata
	printf("[%s] pRTSPStreaming->iRTPVSock = %d, pRTSPStreaming->iRTPASock = %d, pRTSPStreaming->iRTPMSock = %d, pRTSPStreaming->iRTCPVSock = %d, pRTSPStreaming->iRTCPASock = %d, pRTSPStreaming->iRTCPMSock = %d\n", __FUNCTION__, pRTSPStreaming->iRTPVSock, pRTSPStreaming->iRTPASock, pRTSPStreaming->iRTPMSock, pRTSPStreaming->iRTCPVSock, pRTSPStreaming->iRTCPASock, pRTSPStreaming->iRTCPMSock);

#ifdef _SHARED_MEM
	//20091116 support connected UDP
	iOpt = 1;
	if( (setsockopt(pRTSPStreaming->iRTPVSock, SOL_SOCKET, SO_REUSEADDR, (char *) &iOpt, sizeof(iOpt))) !=0 )
	{
		
		printf("[%s] Could not set SO_REUSEADDR pRTSPStreaming->iRTPVSock: %s\n", __FUNCTION__, strerror(errno));
		return S_FAIL;
	}

	if( (setsockopt(pRTSPStreaming->iRTPASock, SOL_SOCKET, SO_REUSEADDR, (char *) &iOpt, sizeof(iOpt))) !=0 )
	{
		
		printf("[%s] Could not set SO_REUSEADDR pRTSPStreaming->iRTPASock: %s\n", __FUNCTION__, strerror(errno));
		return S_FAIL;
	}

	if( (setsockopt(pRTSPStreaming->iRTCPVSock, SOL_SOCKET, SO_REUSEADDR, (char *) &iOpt, sizeof(iOpt))) !=0 )
	{
		
		printf("[%s] Could not set SO_REUSEADDR pRTSPStreaming->iRTCPVSock: %s\n", __FUNCTION__, strerror(errno));
		return S_FAIL;
	}

	if( (setsockopt(pRTSPStreaming->iRTCPASock, SOL_SOCKET, SO_REUSEADDR, (char *) &iOpt, sizeof(iOpt))) !=0 )
	{
		
		printf("[%s] Could not set SO_REUSEADDR pRTSPStreaming->iRTCPASock: %s\n", __FUNCTION__, strerror(errno));
		return S_FAIL;
	}
	//20120801 added by Jimmy for metadata
#ifdef _METADATA_ENABLE
	if( (setsockopt(pRTSPStreaming->iRTPMSock, SOL_SOCKET, SO_REUSEADDR, (char *) &iOpt, sizeof(iOpt))) !=0 )
	{

		printf("[%s] Could not set SO_REUSEADDR pRTSPStreaming->iRTPMSock: %s\n", __FUNCTION__, strerror(errno));
		return S_FAIL;
	}

	if( (setsockopt(pRTSPStreaming->iRTCPMSock, SOL_SOCKET, SO_REUSEADDR, (char *) &iOpt, sizeof(iOpt))) !=0 )
	{
		
		printf("[%s] Could not set SO_REUSEADDR pRTSPStreaming->iRTCPMSock: %s\n", __FUNCTION__, strerror(errno));
		return S_FAIL;
	}
#endif

#endif

	memset(&addr,0,sizeof(addr));
	iInput =  sizeof(addr);	//CID:1176, CHECKER:UNINIT
    getsockname(pRTSPStreaming->iRTPVSock,(struct sockaddr *)&addr, (unsigned int *)&iInput);//sizeof(struct sockaddr_in));

#ifdef _INET6
	addr.sin6_family  = AF_INET6;
    addr.sin6_port = htons(usRTPVPort);
#else
    addr.sin_family = AF_INET;
    addr.sin_port = htons(usRTPVPort);
#endif
        
    if( bind(pRTSPStreaming->iRTPVSock,(struct sockaddr *) &addr,sizeof(addr)) != 0 )
    {
		return -2;
    }
  
    memset(&addr,0,sizeof(addr));
	iInput =  sizeof(addr);	//CID:1176, CHECKER:UNINIT
    getsockname(pRTSPStreaming->iRTPASock,(struct sockaddr *)&addr,(unsigned int *)&iInput);//sizeof(struct sockaddr_in));

#ifdef _INET6
	addr.sin6_family  = AF_INET6;
    addr.sin6_port = htons(usRTPAPort);
#else
    addr.sin_family = AF_INET;
    addr.sin_port = htons(usRTPAPort);
#endif
        
    if( bind(pRTSPStreaming->iRTPASock,(struct sockaddr *) &addr,sizeof(addr)) != 0 )
    {
		return -2;
    }

	memset(&addr,0,sizeof(addr));
	iInput =  sizeof(addr);	//CID:1176, CHECKER:UNINIT
    getsockname(pRTSPStreaming->iRTCPVSock,(struct sockaddr *)&addr,(unsigned int *)&iInput);//sizeof(struct sockaddr_in));

#ifdef _INET6
	addr.sin6_family  = AF_INET6;
    addr.sin6_port = htons(usRTCPVPort);
#else
    addr.sin_family = AF_INET;
    addr.sin_port = htons(usRTCPVPort);
#endif
        
    if( bind(pRTSPStreaming->iRTCPVSock,(struct sockaddr *) &addr,sizeof(addr)) != 0)
    {
		return -2;
    }

	memset(&addr,0,sizeof(addr));
	iInput =  sizeof(addr);	//CID:1176, CHECKER:UNINIT
    getsockname(pRTSPStreaming->iRTCPASock,(struct sockaddr *)&addr,(unsigned int *)&iInput);//sizeof(struct sockaddr_in));

#ifdef _INET6
	addr.sin6_family  = AF_INET6;
    addr.sin6_port = htons(usRTCPAPort);
#else
    addr.sin_family = AF_INET;
    addr.sin_port = htons(usRTCPAPort);
#endif
        
    if( bind(pRTSPStreaming->iRTCPASock,(struct sockaddr *) &addr,sizeof(addr)) != 0)
    {
		return -2;
    }

	//20120801 added by Jimmy for metadata
#ifdef _METADATA_ENABLE
	memset(&addr,0,sizeof(addr));
	iInput =  sizeof(addr); //CID:1176, CHECKER:UNINIT
	getsockname(pRTSPStreaming->iRTPMSock,(struct sockaddr *)&addr,(unsigned int *)&iInput);//sizeof(struct sockaddr_in));
	
#ifdef _INET6
	addr.sin6_family  = AF_INET6;
	addr.sin6_port = htons(usRTPMPort);
#else
	addr.sin_family = AF_INET;
	addr.sin_port = htons(usRTPMPort);
#endif

	if( bind(pRTSPStreaming->iRTPMSock,(struct sockaddr *) &addr,sizeof(addr)) != 0 )
	{
		return -2;
	}

    memset(&addr,0,sizeof(addr));
    iInput =  sizeof(addr); //CID:1176, CHECKER:UNINIT
    getsockname(pRTSPStreaming->iRTCPMSock,(struct sockaddr *)&addr,(unsigned int *)&iInput);//sizeof(struct sockaddr_in));

#ifdef _INET6
    addr.sin6_family  = AF_INET6;
    addr.sin6_port = htons(usRTCPMPort);
#else
    addr.sin_family = AF_INET;
    addr.sin_port = htons(usRTCPMPort);
#endif

    if( bind(pRTSPStreaming->iRTCPMSock,(struct sockaddr *) &addr,sizeof(addr)) != 0)
    {
		return -2;
    }
#endif

    return 0;
}

#ifdef _SHARED_MEM
SCODE RTSPStreaming_InitShmSessionInfo(HANDLE *phHandle)
{
	TShmemSessionInfo		*pShmInfo = NULL;
	int						i = 0;

	if(phHandle == NULL)
	{
		return S_FAIL;
	}
	//Allocate for the Structure
	pShmInfo =  (TShmemSessionInfo *)calloc(1, sizeof(TShmemSessionInfo));
	if(pShmInfo == NULL)
	{
		printf("Allocate shmem structure fail!\n");
		return S_FAIL;
	}

	//Allocate buffer for TBitStreambuffer
	pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.pdwPacketSize = (DWORD *)calloc(MAX_MP4V_PACKET_NUM+1, sizeof(DWORD));
	pShmInfo->tShmemAudioMediaInfo.tStreamBuffer.pdwPacketSize = (DWORD *)calloc(MAX_AUDIO_PACKET_NUM+1, sizeof(DWORD));
	//20120806 added by Jimmy for metadata
	pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.pdwPacketSize = (DWORD *)calloc(MAX_METADATA_PACKET_NUM+1, sizeof(DWORD));

	//Allocate buffer for AUDIO TBitstreambbuffer pbyBuffer
	pShmInfo->tShmemAudioMediaInfo.tStreamBuffer.pbyBuffer = (BYTE *)calloc(1, RTSPSTREAMING_AUDIOBUFFER_SIZE);
	//20120806 added by Jimmy for metadata
	//Allocate buffer for METADATA TBitstreambbuffer pbyBuffer
	pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.pbyBuffer = (BYTE *)calloc(1, RTSPSTREAMING_METADATAXML_SIZE);

	//Allocate buffer for RTP media buffer for VIDEO
	pShmInfo->tShmemVideoMediaInfo.tAggreMediaBuffer.iBufferNumber = RTSPS_VIDEO_RTPBUFFER_NUM;
	pShmInfo->tShmemVideoMediaInfo.tAggreMediaBuffer.dwTotalSize = 0;
	if((pShmInfo->tShmemVideoMediaInfo.tAggreMediaBuffer.ptRTPBuffer = (RTPMEDIABUFFER *)calloc(RTSPS_VIDEO_RTPBUFFER_NUM, sizeof(RTPMEDIABUFFER))) == NULL)
	{
		printf("Allocate video buffer fail!\n");
		return S_FAIL;
	}
	for(i = 0; i < 	pShmInfo->tShmemVideoMediaInfo.tAggreMediaBuffer.iBufferNumber; i++)
	{
		pShmInfo->tShmemVideoMediaInfo.tAggreMediaBuffer.ptRTPBuffer[i].pbBufferStart=pShmInfo->tShmemVideoMediaInfo.tAggreMediaBuffer.ptRTPBuffer[i].pbDataStart=(BYTE *)pShmInfo->tShmemVideoMediaInfo.tAggreMediaBuffer.ptRTPBuffer[i].acPadBuffer + 4;
		pShmInfo->tShmemVideoMediaInfo.tAggreMediaBuffer.ptRTPBuffer[i].dwBufferLength = RTSPSTREAMING_VIDEOBUFFER_SIZE;
	}
	//Allocate for RTP extra data 20091022
	if((pShmInfo->tShmemVideoMediaInfo.tAggreMediaBuffer.pbyRTPExtraData = (BYTE *)calloc(RTP_EXTENSION, 1)) == NULL)
	{
		printf("Allocate video RTP extra buffer fail!\n");
		return S_FAIL;
	}
	//Allocate buffer for RTP media buffer for AUDIO
	pShmInfo->tShmemAudioMediaInfo.tAggreMediaBuffer.iBufferNumber = RTSPS_AUDIO_RTPBUFFER_NUM;
	pShmInfo->tShmemAudioMediaInfo.tAggreMediaBuffer.dwTotalSize = 0;
	if((pShmInfo->tShmemAudioMediaInfo.tAggreMediaBuffer.ptRTPBuffer = (RTPMEDIABUFFER *)calloc(RTSPS_AUDIO_RTPBUFFER_NUM, sizeof(RTPMEDIABUFFER))) == NULL)
	{
		printf("Allocate audio buffer fail!\n");
		return S_FAIL;
	}
	for(i = 0; i < 	pShmInfo->tShmemAudioMediaInfo.tAggreMediaBuffer.iBufferNumber; i++)
	{
		pShmInfo->tShmemAudioMediaInfo.tAggreMediaBuffer.ptRTPBuffer[i].pbBufferStart=pShmInfo->tShmemAudioMediaInfo.tAggreMediaBuffer.ptRTPBuffer[i].pbDataStart=(BYTE *)pShmInfo->tShmemAudioMediaInfo.tAggreMediaBuffer.ptRTPBuffer[i].acPadBuffer + 4;
		pShmInfo->tShmemAudioMediaInfo.tAggreMediaBuffer.ptRTPBuffer[i].dwBufferLength = RTSPSTREAMING_AUDIOBUFFER_SIZE;
	}
	//Allocate for RTP extra data 20091022
	if((pShmInfo->tShmemAudioMediaInfo.tAggreMediaBuffer.pbyRTPExtraData = (BYTE *)calloc(RTP_EXTENSION, 1)) == NULL)
	{
		printf("Allocate audio RTP extra buffer fail!\n");
		return S_FAIL;
	}
	//20120806 added by Jimmy for metadata
	//Allocate buffer for RTP media buffer for METADATA
	pShmInfo->tShmemMetadataMediaInfo.tAggreMediaBuffer.iBufferNumber = RTSPS_METADATA_RTPBUFFER_NUM;
	pShmInfo->tShmemMetadataMediaInfo.tAggreMediaBuffer.dwTotalSize = 0;
	if((pShmInfo->tShmemMetadataMediaInfo.tAggreMediaBuffer.ptRTPBuffer = (RTPMEDIABUFFER *)calloc(RTSPS_METADATA_RTPBUFFER_NUM, sizeof(RTPMEDIABUFFER))) == NULL)
	{
		printf("Allocate metadata buffer fail!\n");
		return S_FAIL;
	}
	for(i = 0; i < 	pShmInfo->tShmemMetadataMediaInfo.tAggreMediaBuffer.iBufferNumber; i++)
	{
		pShmInfo->tShmemMetadataMediaInfo.tAggreMediaBuffer.ptRTPBuffer[i].pbBufferStart=pShmInfo->tShmemMetadataMediaInfo.tAggreMediaBuffer.ptRTPBuffer[i].pbDataStart=(BYTE *)pShmInfo->tShmemMetadataMediaInfo.tAggreMediaBuffer.ptRTPBuffer[i].acPadBuffer + 4;
		pShmInfo->tShmemMetadataMediaInfo.tAggreMediaBuffer.ptRTPBuffer[i].dwBufferLength = RTSPSTREAMING_METADATABUFFER_SIZE;
	}
	//Allocate for RTP extra data 20091022
	if((pShmInfo->tShmemMetadataMediaInfo.tAggreMediaBuffer.pbyRTPExtraData = (BYTE *)calloc(RTP_EXTENSION, 1)) == NULL)
	{
		printf("Allocate metadata RTP extra buffer fail!\n");
		return S_FAIL;
	}


	//Set Parent Handle for the structure
	pShmInfo->tShmemVideoMediaInfo.hParentHandle = (HANDLE)(pShmInfo);
	pShmInfo->tShmemAudioMediaInfo.hParentHandle = (HANDLE)(pShmInfo);
	//20120806 added by Jimmy for metadata
	pShmInfo->tShmemMetadataMediaInfo.hParentHandle = (HANDLE)(pShmInfo);

	//Init critical section
	pShmInfo->eCritSecStatus = eCSReleased;

	//Init SVC temp value
	pShmInfo->eSVCTempMode = eSVCInvalid;

	//Assign Handle
	*phHandle = pShmInfo;

	return S_OK;
}
SCODE RTSPStreaming_ReleaseShmSessionInfo(HANDLE *phHandle)
{
	TShmemSessionInfo		*pShmInfo = NULL;

	if(phHandle == NULL || *phHandle == NULL)
	{
		return S_FAIL;
	}
	//Assign structure
	pShmInfo = (TShmemSessionInfo *)(*phHandle);

	//Release buffer for TBitStreambuffer
	if(pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.pdwPacketSize != NULL)
	{
		free(pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.pdwPacketSize);
	}
	if(pShmInfo->tShmemAudioMediaInfo.tStreamBuffer.pdwPacketSize != NULL)
	{
		free(pShmInfo->tShmemAudioMediaInfo.tStreamBuffer.pdwPacketSize);
	}
	//20120806 added by Jimmy for metadata
	if(pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.pdwPacketSize != NULL)
	{
		free(pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.pdwPacketSize);
	}

	//Release buffer for AUDIO TBitstreambbuffer pbyBuffer
	if(pShmInfo->tShmemAudioMediaInfo.tStreamBuffer.pbyBuffer != NULL)
	{
		free(pShmInfo->tShmemAudioMediaInfo.tStreamBuffer.pbyBuffer);
	}
	//20120806 added by Jimmy for metadata
	//Release buffer for METADATA TBitstreambbuffer pbyBuffer
	if(pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.pbyBuffer != NULL)
	{
		free(pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.pbyBuffer);
	}

	//Release buffer for RTP aggregate data
	if(pShmInfo->tShmemVideoMediaInfo.tAggreMediaBuffer.ptRTPBuffer != NULL)
	{
		free(pShmInfo->tShmemVideoMediaInfo.tAggreMediaBuffer.ptRTPBuffer);
	}

	if(pShmInfo->tShmemAudioMediaInfo.tAggreMediaBuffer.ptRTPBuffer != NULL)
	{
		free(pShmInfo->tShmemAudioMediaInfo.tAggreMediaBuffer.ptRTPBuffer);
	}
	//20120806 added by Jimmy for metadata
	if(pShmInfo->tShmemMetadataMediaInfo.tAggreMediaBuffer.ptRTPBuffer != NULL)
	{
		free(pShmInfo->tShmemMetadataMediaInfo.tAggreMediaBuffer.ptRTPBuffer);
	}


	//Release buffer for RTP extra buffer
	if(pShmInfo->tShmemVideoMediaInfo.tAggreMediaBuffer.pbyRTPExtraData != NULL)
	{
		free(pShmInfo->tShmemVideoMediaInfo.tAggreMediaBuffer.pbyRTPExtraData);
	}
	if(pShmInfo->tShmemAudioMediaInfo.tAggreMediaBuffer.pbyRTPExtraData != NULL)
	{
		free(pShmInfo->tShmemAudioMediaInfo.tAggreMediaBuffer.pbyRTPExtraData);
	}
	//20120806 added by Jimmy for metadata
	if(pShmInfo->tShmemMetadataMediaInfo.tAggreMediaBuffer.pbyRTPExtraData != NULL)
	{
		free(pShmInfo->tShmemMetadataMediaInfo.tAggreMediaBuffer.pbyRTPExtraData);
	}
	
	//Release Structure
	free(pShmInfo);
	*phHandle = NULL;

	return S_OK;
}
#endif

HANDLE RTSPStreaming_Create(TRTSPSTREAMING_PARAM *pstRTSPStreamingParameter)
{

	int i, iIndex;
	//20130605 added by Jimmy to support metadata event
	int j;
	int iResult;
	unsigned long ulResult;
	RTPRTCPCHANNEL_PARAM	stRTPRTCPChannelParam;
	RTSPSERVER_PARAM		stRTSPServerParam;

	RTSPSTREAMING *pRTSPStreaming;
	HANDLE hTCPMuxCS;
	int iMaxSessionNumber = 0; 
#ifndef _SHARED_MEM
	RTPMEDIABUFFER * pMediaBuffer;
	//20120801 added by Jimmy for metadata
	stRTPPACKETIZERPARAM PacketizerVideoParam,PacketizerAudioParam,PacketizerMetadataParam;
#endif

#ifdef _SIP
	TSIPUAParam	tSIPUAParam;
#endif

	unsigned short usLocalPortOffset;

	iMaxSessionNumber = pstRTSPStreamingParameter->iRTSPMaxConnectionNum ;
#ifdef _SIP
	iMaxSessionNumber = iMaxSessionNumber +	pstRTSPStreamingParameter->tSIPParameter.iSIPMaxConnectionNum;
#endif
						
	pRTSPStreaming=(RTSPSTREAMING *)malloc(sizeof(RTSPSTREAMING));

	if(!pRTSPStreaming)
		return 0;
		
	memset(pRTSPStreaming,0,sizeof(RTSPSTREAMING));
	
#ifdef	PIKACHU_MEDIATEST_
 AcceptSocket();
#endif	
	
	if(iMaxSessionNumber <= 0)
	{
        printf("[StreamingServer]:Max number of sessions is invalid\n");   
	}
	//printf("iMaxSessionNumber=%d\r\n", iMaxSessionNumber);
	pRTSPStreaming->iMaximumSessionCount = iMaxSessionNumber;
	pRTSPStreaming->iSessionListNumber = 0 ;
	pRTSPStreaming->iRTPOverHTTPNumber = 0;

	//20101123 Added by danny For support advanced system log 
	pRTSPStreaming->bAdvLogSupport = pstRTSPStreamingParameter->bAdvLogSupport;
	
	pRTSPStreaming->dwAudioChannelPriority = pstRTSPStreamingParameter->dwAudioChannelPriority;
	pRTSPStreaming->dwVideoChannelPriority = pstRTSPStreamingParameter->dwVideoChannelPriority;
	//20120801 added by Jimmy for metadata
	pRTSPStreaming->dwMetadataChannelPriority = pstRTSPStreamingParameter->dwMetadataChannelPriority;
	pRTSPStreaming->dwAudioPacketizerPriority = pstRTSPStreamingParameter->dwAudioPacketizerPriority;
	pRTSPStreaming->dwVideoPacketizerPriority = pstRTSPStreamingParameter->dwVideoPacketizerPriority;
	//20120801 added by Jimmy for metadata
	pRTSPStreaming->dwMetadataPacketizerPriority = pstRTSPStreamingParameter->dwMetadataPacketizerPriority;
	pRTSPStreaming->dwRTSPServerPriority = pstRTSPStreamingParameter->dwRTSPServerPriority;

	//20120807 modified by Jimmy for metadata
	if( RTSPStreaming_CreateFixedUDPSocket(pRTSPStreaming,
										pstRTSPStreamingParameter->usRTPVPort,
										pstRTSPStreamingParameter->usRTPAPort,
										pstRTSPStreamingParameter->usRTPMPort,
										pstRTSPStreamingParameter->usRTCPVPort,
										pstRTSPStreamingParameter->usRTCPAPort,
										pstRTSPStreamingParameter->usRTCPMPort) != 0 )
	{
	    printf("[StreamingServer]:create Fixed UDP socket failed\r\n");
		free(pRTSPStreaming);		//CID:408, CHECKER:RESOURCE_LEAK
		return 0;
	}

#ifdef RTSPRTP_MULTICAST

#ifdef _SHARED_MEM
	for (i = 0; i < VIDEO_TRACK_NUMBER ; i++)
	{
		pRTSPStreaming->ahShmemVideoHandle[i] = pstRTSPStreamingParameter->ahShmemVideoHandle[i];
	}
	for (i = 0; i < AUDIO_TRACK_NUMBER ; i++)
	{
		pRTSPStreaming->ahShmemAudioHandle[i] = pstRTSPStreamingParameter->ahShmemAudioHandle[i];
	}
	//20120801 added by Jimmy for metadata
	for (i = 0; i < METADATA_TRACK_NUMBER ; i++)
	{
		//pRTSPStreaming->ahShmemMetadataHandle[i] = pstRTSPStreamingParameter->ahShmemMetadataHandle[i];
		//20130605 modified by Jimmy to support metadata event
		for (j = 0; j < SHMEM_HANDLE_MAX_NUM; j++)
		{
			pRTSPStreaming->ahShmemMetadataHandle[i][j] = pstRTSPStreamingParameter->ahShmemMetadataHandle[i][j];
		}
	}	
#endif
	//20130327 modified by Jimmy to support different video/audio multicast addresses for ONVIF Test Tool 12.12
	for( i=0; i<(RTSP_MULTICASTNUMBER) ; i++ )
	{
		if( pstRTSPStreamingParameter->stMulticastInfo[i].iSDPIndex < 1 || pstRTSPStreamingParameter->stMulticastInfo[i].iSDPIndex > MULTIPLE_STREAM_NUM )
		{
			printf("[%s] Multicast iSDPIndex error\n", __FUNCTION__);
			free(pRTSPStreaming);		//CID:408, CHECKER:RESOURCE_LEAK
			return 0;
        }
		pRTSPStreaming->stMulticast[i].hRTPRTCPAudioComposerHandle=RTPRTCPComposer_Create();
		pRTSPStreaming->stMulticast[i].hRTPRTCPVideoComposerHandle=RTPRTCPComposer_Create();
		//20120801 added by Jimmy for metadata
		pRTSPStreaming->stMulticast[i].hRTPRTCPMetadataComposerHandle=RTPRTCPComposer_Create();

		if(pRTSPStreaming->stMulticast[i].hRTPRTCPAudioComposerHandle == NULL ||
			pRTSPStreaming->stMulticast[i].hRTPRTCPVideoComposerHandle == NULL ||
			pRTSPStreaming->stMulticast[i].hRTPRTCPMetadataComposerHandle == NULL )
		{
			printf("[%s] create Multicast RTP/RTCP composer fail\n", __FUNCTION__);
			free(pRTSPStreaming);		//CID:408, CHECKER:RESOURCE_LEAK
			return 0;
        }
		
        //20110630 Add by danny For Multicast enable/disable
        pRTSPStreaming->stMulticast[i].iEnable = pstRTSPStreamingParameter->stMulticastInfo[i].iEnable;
		//20110725 Add by danny For Multicast RTCP receive report keep alive
		pRTSPStreaming->stMulticast[i].iRRAlive = pstRTSPStreamingParameter->stMulticastInfo[i].iRRAlive;
		pRTSPStreaming->stMulticast[i].ulMulticastAddress = pstRTSPStreamingParameter->stMulticastInfo[i].ulMulticastAddress;
		//20160127 Add by Faber for separate multicast address
		pRTSPStreaming->stMulticast[i].ulMulticastAudioAddress = pstRTSPStreamingParameter->stMulticastInfo[i].ulMulticastAudioAddress;
		pRTSPStreaming->stMulticast[i].ulMulticastAudioAddress = pstRTSPStreamingParameter->stMulticastInfo[i].ulMulticastAudioAddress;

		pRTSPStreaming->stMulticast[i].usMulticastVideoPort = pstRTSPStreamingParameter->stMulticastInfo[i].usMulticastVideoPort;
		pRTSPStreaming->stMulticast[i].usMulticastAudioPort = pstRTSPStreamingParameter->stMulticastInfo[i].usMulticastAudioPort;
		//20120801 added by Jimmy for metadata
		pRTSPStreaming->stMulticast[i].usMulticastMetadataPort = pstRTSPStreamingParameter->stMulticastInfo[i].usMulticastMetadataPort;
		pRTSPStreaming->stMulticast[i].usTTL = pstRTSPStreamingParameter->stMulticastInfo[i].usTTL;

		pRTSPStreaming->stMulticast[i].iAlwaysMulticast = pstRTSPStreamingParameter->stMulticastInfo[i].iAlwaysMulticast;
		pRTSPStreaming->stMulticast[i].iAlreadyMulticastVideo = 0;
		pRTSPStreaming->stMulticast[i].iAlreadyMulticastAudio = 0;
		//20120801 added by Jimmy for metadata
		pRTSPStreaming->stMulticast[i].iAlreadyMulticastMetadata = 0;
		pRTSPStreaming->stMulticast[i].iRTPExtension = pstRTSPStreamingParameter->stMulticastInfo[i].iRTPExtension;
		pRTSPStreaming->stMulticast[i].iSDPIndex = pstRTSPStreamingParameter->stMulticastInfo[i].iSDPIndex;
		//Added by Louis 2008/01/29 for always multicast video/audio only
		pRTSPStreaming->stMulticast[i].iAlwaysMulticastAudio = pstRTSPStreamingParameter->stMulticastInfo[i].iAlwaysMulticastAudio;
		pRTSPStreaming->stMulticast[i].iAlwaysMulticastVideo = pstRTSPStreamingParameter->stMulticastInfo[i].iAlwaysMulticastVideo;
		//20120801 added by Jimmy for metadata
		pRTSPStreaming->stMulticast[i].iAlwaysMulticastMetadata = pstRTSPStreamingParameter->stMulticastInfo[i].iAlwaysMulticastMetadata;

	}
    //20130905 added by Charles for ondemand multicast
    for(iIndex = RTSP_MULTICASTNUMBER; iIndex < RTSP_MULTICASTNUMBER+RTSP_ONDEMAND_MULTICASTNUMBER; iIndex++)
    {

        pRTSPStreaming->stMulticast[iIndex].hRTPRTCPAudioComposerHandle=RTPRTCPComposer_Create();
		pRTSPStreaming->stMulticast[iIndex].hRTPRTCPVideoComposerHandle=RTPRTCPComposer_Create();
		//20120801 added by Jimmy for metadata
		pRTSPStreaming->stMulticast[iIndex].hRTPRTCPMetadataComposerHandle=RTPRTCPComposer_Create();

		if(pRTSPStreaming->stMulticast[iIndex].hRTPRTCPAudioComposerHandle == NULL ||
			pRTSPStreaming->stMulticast[iIndex].hRTPRTCPVideoComposerHandle == NULL ||
			pRTSPStreaming->stMulticast[iIndex].hRTPRTCPMetadataComposerHandle == NULL )
		{
			printf("[%s] create Multicast RTP/RTCP composer fail\n", __FUNCTION__);
			free(pRTSPStreaming);		//CID:408, CHECKER:RESOURCE_LEAK
			return 0;
        }
    }
    //Initialize Shm info for multicast
    //20130904 modified by Charles for ondemand multicast
#ifdef _SHARED_MEM
    for( i=0; i<(RTSP_MULTICASTNUMBER+RTSP_ONDEMAND_MULTICASTNUMBER) ; i++ )
    {
        RTSPStreaming_InitShmSessionInfo(&pRTSPStreaming->stMulticast[i].hShmemSessionInfo);
    }
#endif
#endif

	pRTSPStreaming->pstSessionList     = (RTSPSTREAMING_SESSION *)malloc(iMaxSessionNumber*sizeof(RTSPSTREAMING_SESSION));
	pRTSPStreaming->pstRTPOverHTTPInfo = (TRTP_OVER_HTTPINFO *)malloc(iMaxSessionNumber*sizeof(TRTP_OVER_HTTPINFO));
	
	if( pRTSPStreaming->pstSessionList == NULL || pRTSPStreaming->pstRTPOverHTTPInfo == NULL )
	{
		free(pRTSPStreaming);		//CID:408, CHECKER:RESOURCE_LEAK
		return 0;
	}
#ifdef _SHARED_MEM
	memset(pRTSPStreaming->pstSessionList,0,iMaxSessionNumber*sizeof(RTSPSTREAMING_SESSION));
	for(i = 0; i < iMaxSessionNumber; i++)
	{
		//Initialize Shm info for each session
		RTSPStreaming_InitShmSessionInfo(&pRTSPStreaming->pstSessionList[i].hShmemSessionInfo);
	}
#else
	memset(pRTSPStreaming->pstSessionList,0,iMaxSessionNumber*sizeof(RTSPSTREAMING_SESSION));
#endif
	memset(pRTSPStreaming->pstRTPOverHTTPInfo,0,iMaxSessionNumber*sizeof(TRTP_OVER_HTTPINFO));
	
	for(i=0;i<iMaxSessionNumber;i++)
	{
	    pRTSPStreaming->pstRTPOverHTTPInfo[i].iSendSock = -1;
	    pRTSPStreaming->pstRTPOverHTTPInfo[i].iRecvSock = -1;	    
	}

	ulResult=OSSemaphore_Initial(&pRTSPStreaming->ulSessionListSemaphore,1,1);
	
	if(ulResult)
	{
		free(pRTSPStreaming);
        DbgLog((dfCONSOLE|dfINTERNAL,"\n [RTSPStreaming] semaphore create failure"));   
		return 0;
	}	
	
	ulResult=OSSemaphore_Initial(&pRTSPStreaming->hMediaParamSemaphore,1,1);

	if(ulResult)
	{
		free(pRTSPStreaming);
        DbgLog((dfCONSOLE|dfINTERNAL,"\n [RTSPStreaming] semaphore create failure"));   
		return 0;
	}
		
// RTP/RTCP composer
	for(i=0;i<iMaxSessionNumber;i++)
	{	                    
	  
		pRTSPStreaming->pstSessionList[i].hRTPRTCPAudioComposerHandle = NULL;
		pRTSPStreaming->pstSessionList[i].hRTPRTCPAudioComposerHandle=RTPRTCPComposer_Create();
		if(pRTSPStreaming->pstSessionList[i].hRTPRTCPAudioComposerHandle == NULL )
		{
			free(pRTSPStreaming);		//CID:408, CHECKER:RESOURCE_LEAK
	        DbgLog((dfCONSOLE|dfINTERNAL,"\n [RTSPStreaming] create RTP/RTCP composer fail\n "));   
			return 0;
		}
				
		pRTSPStreaming->pstSessionList[i].hRTPRTCPVideoComposerHandle = NULL;
		pRTSPStreaming->pstSessionList[i].hRTPRTCPVideoComposerHandle=RTPRTCPComposer_Create();
		if(pRTSPStreaming->pstSessionList[i].hRTPRTCPVideoComposerHandle == NULL )
		{
			free(pRTSPStreaming);		//CID:408, CHECKER:RESOURCE_LEAK
		    DbgLog((dfCONSOLE|dfINTERNAL,"\n [RTSPStreaming] create RTP/RTCP composer fail \n"));   
			return 0;		    
		}

		//20120801 added by Jimmy for metadata
		pRTSPStreaming->pstSessionList[i].hRTPRTCPMetadataComposerHandle = NULL;
		pRTSPStreaming->pstSessionList[i].hRTPRTCPMetadataComposerHandle=RTPRTCPComposer_Create();
		if(pRTSPStreaming->pstSessionList[i].hRTPRTCPMetadataComposerHandle == NULL )
		{
			free(pRTSPStreaming);		//CID:408, CHECKER:RESOURCE_LEAK
			DbgLog((dfCONSOLE|dfINTERNAL,"\n [RTSPStreaming] create RTP/RTCP composer fail \n"));	
			return 0;			
		}

	}	

	// IP Access Check
	pRTSPStreaming->hIPAccessCheckHandle=NULL;
	pRTSPStreaming->hIPAccessCheckHandle=IPAccessCheck_Create(RTSPSTREAMING_IPACCESSCHECK_MAX_IPLIST);
	if(pRTSPStreaming->hIPAccessCheckHandle == NULL )
	{
		free(pRTSPStreaming);		//CID:408, CHECKER:RESOURCE_LEAK
        printf("[StreamingServer]:create IP Access Check fail \n");   
		return 0;
	}

	//Eliminate VIDEO PACKETIZER by share-memory

//	if(pstRTSPStreamingParameter->iRTSPStreamingMediaType != RTSPSTREAMING_MEDIATYPE_AUDIOONLY)
	{
#ifndef _SHARED_MEM
// Video data and empty queue
		pRTSPStreaming->hVideoDataQueueHandle=NULL;
//		while(pRTSPStreaming->hVideoDataQueueHandle == NULL)
//		{
			pRTSPStreaming->hVideoDataQueueHandle=MediaBufQueue_Create(RTSPSTREAMING_VIDEOBUFFER_MAX_NUMBER);
			if(pRTSPStreaming->hVideoDataQueueHandle == NULL )
			{
				free(pRTSPStreaming);		//CID:408, CHECKER:RESOURCE_LEAK
	  		    DbgLog((dfCONSOLE|dfINTERNAL,"\n [RTSPStreaming] create Video data queue fail "));   
	  		    TelnetShell_DbgPrint("[RTSPStreaming] create Video data queue fail\r\n");
				return 0;
			}
//		}


		pRTSPStreaming->hVideoEmptyQueueHandle=NULL;
//		while(pRTSPStreaming->hVideoEmptyQueueHandle == NULL)
//		{
			pRTSPStreaming->hVideoEmptyQueueHandle=MediaBufQueue_Create(RTSPSTREAMING_VIDEOBUFFER_MAX_NUMBER);
			if(pRTSPStreaming->hVideoEmptyQueueHandle == NULL )
			{
				free(pRTSPStreaming);		//CID:408, CHECKER:RESOURCE_LEAK
   	  		    DbgLog((dfCONSOLE|dfINTERNAL,"\n [RTSPStreaming] create Video empty queue fail "));   
   	  		    return 0;
//    	        OSSleep_MSec(100);
			}
//		}
//buffer allocated by streaming server and feed in empty buffer queue
		for(i=0;i<RTSPSTREAMING_VIDEOBUFFER_MAX_NUMBER;i++)
		{
			pMediaBuffer=NULL;
//			while(pMediaBuffer == NULL)
//			{
				pMediaBuffer=RTSPStreaming_AllocateMediaBuffer(RTSPSTREAMING_VIDEOBUFFER_SIZE);
				if(pMediaBuffer)
				{
					iResult= MediaBufQueue_AddMediaBuffer(pRTSPStreaming->hVideoEmptyQueueHandle, (void *)pMediaBuffer);
					if(iResult)
					{
						free(pRTSPStreaming);		//CID:408, CHECKER:RESOURCE_LEAK
	       	  		    DbgLog((dfCONSOLE|dfINTERNAL,"\n [RTSPStreaming] Create and add video empty buffer to queue fail"));   
	       	  		    return 0;
					}
				}
				else
				{
					free(pRTSPStreaming);		//CID:408, CHECKER:RESOURCE_LEAK
				    DbgLog((dfCONSOLE|dfINTERNAL,"\n [RTSPStreaming] allocate Video buffer fail "));   
				    return 0;
				}
//			}
		}

// create video packetizer
        PacketizerVideoParam.hRTPMediaDataQueue = pRTSPStreaming->hVideoDataQueueHandle;
        PacketizerVideoParam.hRTPMediaEmptyQueue= pRTSPStreaming->hVideoEmptyQueueHandle;
		PacketizerVideoParam.dwThreadPriority = pRTSPStreaming->dwVideoPacketizerPriority;

        RTPPacketizer_Create(&pRTSPStreaming->hVideoPacketizer,&PacketizerVideoParam);
        if( pRTSPStreaming->hVideoPacketizer != NULL )
        {
            RTPPacketizer_SetCallBakck(pRTSPStreaming->hVideoPacketizer,(RTPPACKETIZERCALLBACK)RTSPStreaming_PacketizerVideoCallBack,(HANDLE)pRTSPStreaming);
        }    
        else
        {
			free(pRTSPStreaming);		//CID:408, CHECKER:RESOURCE_LEAK
            DbgLog((dfCONSOLE|dfINTERNAL,"\n [RTSPStreaming] Create video packetizer fail\n"));   
            return 0;
        }

#endif // ifndef _SHARED_MEM
// RTP/RTCP	Video Channel	
	    // get Video Start bit stream first;
		stRTPRTCPChannelParam.iRTPRTCPMediaType=RTPRTCPCHANNEL_MEDIATYPE_VIDEO;
		stRTPRTCPChannelParam.pbyMPEG4StartBitStream=NULL;
		stRTPRTCPChannelParam.iMPEG4StartBitStreamLength=0;
		stRTPRTCPChannelParam.ulThreadPriority = pRTSPStreaming->dwVideoChannelPriority;
		stRTPRTCPChannelParam.iUDPRTPSock = pRTSPStreaming->iRTPVSock;
		stRTPRTCPChannelParam.iUDPRTCPSock = pRTSPStreaming->iRTCPVSock;

		pRTSPStreaming->hRTPRTCPChannelVideoHandle = NULL;
//		while(pRTSPStreaming->hRTPRTCPChannelVideoHandle == NULL)
//		{
			pRTSPStreaming->hRTPRTCPChannelVideoHandle=RTPRTCPChannel_Create( iMaxSessionNumber, &stRTPRTCPChannelParam);
			if(pRTSPStreaming->hRTPRTCPChannelVideoHandle == NULL )
			{
				free(pRTSPStreaming);		//CID:408, CHECKER:RESOURCE_LEAK
	  		    printf("[StreamingServer]: create RTP/RTCP video channel fail \r\n");   
	  		    return 0;
		}
//		}
		
		iResult=RTPRTCPChannel_SetCallback(pRTSPStreaming->hRTPRTCPChannelVideoHandle, (RTPRTCPCHANNELCALLBACK)RTSPStreaming_VideoRTPRTCPChannelCallback, (HANDLE)pRTSPStreaming);
	}


//	if(pstRTSPStreamingParameter->iRTSPStreamingMediaType != RTSPSTREAMING_MEDIATYPE_VIDEOONLY)
	{
#ifndef _SHARED_MEM
// Audio data and empty queue
		pRTSPStreaming->hAudioDataQueueHandle=NULL;
//		while(pRTSPStreaming->hAudioDataQueueHandle == NULL)
//		{
			pRTSPStreaming->hAudioDataQueueHandle=MediaBufQueue_Create(RTSPSTREAMING_AUDIOBUFFER_MAX_NUMBER);
			if(pRTSPStreaming->hAudioDataQueueHandle == NULL )
			{
				free(pRTSPStreaming);		//CID:408, CHECKER:RESOURCE_LEAK
   	  		    printf("[StreamingServer]: create Audio data queue fail \r\n");   
   	  		    return 0;
			}
//		}

		pRTSPStreaming->hAudioEmptyQueueHandle=NULL;
//		while(pRTSPStreaming->hAudioEmptyQueueHandle == NULL)
//		{
			pRTSPStreaming->hAudioEmptyQueueHandle=MediaBufQueue_Create(RTSPSTREAMING_AUDIOBUFFER_MAX_NUMBER);
			if(pRTSPStreaming->hAudioEmptyQueueHandle == NULL )
			{
				free(pRTSPStreaming);		//CID:408, CHECKER:RESOURCE_LEAK
			    printf("[StreamingServer]: create Audio empty queue fail\r\n");   
			    return 0;
			}
//		}

		for(i=0;i<RTSPSTREAMING_AUDIOBUFFER_MAX_NUMBER;i++)
		{
			pMediaBuffer=NULL;
//			while(pMediaBuffer == NULL)
//			{
				pMediaBuffer=RTSPStreaming_AllocateMediaBuffer(RTSPSTREAMING_AUDIOBUFFER_SIZE);
				if(pMediaBuffer)
				{
					iResult= MediaBufQueue_AddMediaBuffer(pRTSPStreaming->hAudioEmptyQueueHandle, (void *)pMediaBuffer);
					if(iResult)
					{
						free(pRTSPStreaming);		//CID:408, CHECKER:RESOURCE_LEAK
	    			    printf("[StreamingServer]: Add Audio empty buffer to queue fail\r\n");   
	    			    return 0;
					}
				}
				else
				{
					free(pRTSPStreaming);		//CID:408, CHECKER:RESOURCE_LEAK
    			    DbgLog((dfCONSOLE|dfINTERNAL,"\n [RTSPStreaming] allocate Audio buffer fail\n"));   
    			    return 0;
				}
//			}
		}
		
// create audio packetizer
        PacketizerAudioParam.hRTPMediaDataQueue = pRTSPStreaming->hAudioDataQueueHandle;
        PacketizerAudioParam.hRTPMediaEmptyQueue= pRTSPStreaming->hAudioEmptyQueueHandle;
		PacketizerAudioParam.dwThreadPriority = pRTSPStreaming->dwAudioPacketizerPriority;

        RTPPacketizer_Create(&pRTSPStreaming->hAudioPacketizer,&PacketizerAudioParam);
        if( pRTSPStreaming->hAudioPacketizer != NULL )
        {
            RTPPacketizer_SetCallBakck(pRTSPStreaming->hAudioPacketizer,(RTPPACKETIZERCALLBACK)RTSPStreaming_PacketizerAudioCallBack,(HANDLE)pRTSPStreaming);
        }    
        else
        {
			free(pRTSPStreaming);		//CID:408, CHECKER:RESOURCE_LEAK
            printf("[StreamingServer]: Create Audio packetizer fail\r\n");   
            return 0;
        }

#endif //ifndef _SHARED_MEM
// RTP/RTCP	Audio Channel
		stRTPRTCPChannelParam.iRTPRTCPMediaType=RTPRTCPCHANNEL_MEDIATYPE_AUDIO;
		stRTPRTCPChannelParam.pbyMPEG4StartBitStream=NULL;
		stRTPRTCPChannelParam.iMPEG4StartBitStreamLength=0;
		stRTPRTCPChannelParam.ulThreadPriority = pRTSPStreaming->dwAudioChannelPriority;
		stRTPRTCPChannelParam.iUDPRTPSock = pRTSPStreaming->iRTPASock;
		stRTPRTCPChannelParam.iUDPRTCPSock = pRTSPStreaming->iRTCPASock;

		pRTSPStreaming->hRTPRTCPChannelAudioHandle = NULL;
//		while(pRTSPStreaming->hRTPRTCPChannelAudioHandle == NULL)
//		{
			pRTSPStreaming->hRTPRTCPChannelAudioHandle=RTPRTCPChannel_Create( iMaxSessionNumber, &stRTPRTCPChannelParam);
			if(pRTSPStreaming->hRTPRTCPChannelAudioHandle == NULL )
			{
				free(pRTSPStreaming);		//CID:408, CHECKER:RESOURCE_LEAK
   			    printf("[StreamingServer]: create RTP/RTCP Audio channel fail\r\n");   
   			    return 0;
		}
//		}
		
		iResult=RTPRTCPChannel_SetCallback(pRTSPStreaming->hRTPRTCPChannelAudioHandle, (RTPRTCPCHANNELCALLBACK)RTSPStreaming_AudioRTPRTCPChannelCallback, (HANDLE)pRTSPStreaming);
	}

	//20120801 added by Jimmy for metadata
	{
#ifndef _SHARED_MEM
// Metadata data and empty queue
		pRTSPStreaming->hMetadataDataQueueHandle=NULL;
//		while(pRTSPStreaming->hMetadataDataQueueHandle == NULL)
//		{
			pRTSPStreaming->hMetadataDataQueueHandle=MediaBufQueue_Create(RTSPSTREAMING_METADATABUFFER_MAX_NUMBER);
			if(pRTSPStreaming->hMetadataDataQueueHandle == NULL )
			{
				free(pRTSPStreaming);		//CID:408, CHECKER:RESOURCE_LEAK
   	  		    printf("[StreamingServer]: create Metadata data queue fail \r\n");   
   	  		    return 0;
			}
//		}

		pRTSPStreaming->hMetadataEmptyQueueHandle=NULL;
//		while(pRTSPStreaming->hMetadataEmptyQueueHandle == NULL)
//		{
			pRTSPStreaming->hMetadataEmptyQueueHandle=MediaBufQueue_Create(RTSPSTREAMING_METADATABUFFER_MAX_NUMBER);
			if(pRTSPStreaming->hMetadataEmptyQueueHandle == NULL )
			{
				free(pRTSPStreaming);		//CID:408, CHECKER:RESOURCE_LEAK
			    printf("[StreamingServer]: create Metadata empty queue fail\r\n");   
			    return 0;
			}
//		}

		for(i=0;i<RTSPSTREAMING_METADATABUFFER_MAX_NUMBER;i++)
		{
			pMediaBuffer=NULL;
//			while(pMediaBuffer == NULL)
//			{
				pMediaBuffer=RTSPStreaming_AllocateMediaBuffer(RTSPSTREAMING_METADATAXML_SIZE);
				if(pMediaBuffer)
				{
					iResult= MediaBufQueue_AddMediaBuffer(pRTSPStreaming->hMetadataEmptyQueueHandle, (void *)pMediaBuffer);
					if(iResult)
					{
						free(pRTSPStreaming);		//CID:408, CHECKER:RESOURCE_LEAK
	    			    printf("[StreamingServer]: Add Metadata empty buffer to queue fail\r\n");   
	    			    return 0;
					}
				}
				else
				{
					free(pRTSPStreaming);		//CID:408, CHECKER:RESOURCE_LEAK
    			    DbgLog((dfCONSOLE|dfINTERNAL,"\n [RTSPStreaming] allocate Metadata buffer fail\n"));   
    			    return 0;
				}
//			}
		}
		
// create metadata packetizer
        PacketizerMetadataParam.hRTPMediaDataQueue = pRTSPStreaming->hMetadataDataQueueHandle;
        PacketizerMetadataParam.hRTPMediaEmptyQueue= pRTSPStreaming->hMetadataEmptyQueueHandle;
		PacketizerMetadataParam.dwThreadPriority = pRTSPStreaming->dwMetadataPacketizerPriority;

        RTPPacketizer_Create(&pRTSPStreaming->hMetadataPacketizer,&PacketizerMetadataParam);
        if( pRTSPStreaming->hMetadataPacketizer != NULL )
        {
            RTPPacketizer_SetCallBakck(pRTSPStreaming->hMetadataPacketizer,(RTPPACKETIZERCALLBACK)RTSPStreaming_PacketizerMetadataCallBack,(HANDLE)pRTSPStreaming);
        }    
        else
        {
			free(pRTSPStreaming);		//CID:408, CHECKER:RESOURCE_LEAK
            printf("[StreamingServer]: Create Audio packetizer fail\r\n");   
            return 0;
        }

#endif //ifndef _SHARED_MEM
// RTP/RTCP	Metadata Channel
		stRTPRTCPChannelParam.iRTPRTCPMediaType=RTPRTCPCHANNEL_MEDIATYPE_METADATA;
		stRTPRTCPChannelParam.pbyMPEG4StartBitStream=NULL;
		stRTPRTCPChannelParam.iMPEG4StartBitStreamLength=0;
		stRTPRTCPChannelParam.ulThreadPriority = pRTSPStreaming->dwMetadataChannelPriority;
		stRTPRTCPChannelParam.iUDPRTPSock = pRTSPStreaming->iRTPMSock;
		stRTPRTCPChannelParam.iUDPRTCPSock = pRTSPStreaming->iRTCPMSock;

		pRTSPStreaming->hRTPRTCPChannelMetadataHandle = NULL;
//		while(pRTSPStreaming->hRTPRTCPChannelMetadataHandle == NULL)
//		{
			pRTSPStreaming->hRTPRTCPChannelMetadataHandle=RTPRTCPChannel_Create( iMaxSessionNumber, &stRTPRTCPChannelParam);
			if(pRTSPStreaming->hRTPRTCPChannelMetadataHandle == NULL )
			{
				free(pRTSPStreaming);		//CID:408, CHECKER:RESOURCE_LEAK
   			    printf("[StreamingServer]: create RTP/RTCP Metadata channel fail\r\n");   
   			    return 0;
			}
//		}
		
		iResult=RTPRTCPChannel_SetCallback(pRTSPStreaming->hRTPRTCPChannelMetadataHandle, (RTPRTCPCHANNELCALLBACK)RTSPStreaming_MetadataRTPRTCPChannelCallback, (HANDLE)pRTSPStreaming);
	}
	if(iResult != S_OK)
	{
		printf("[%s] RTPRTCPChannel_SetCallback failed\n", __func__);
	}

// RTSP server
	for(i=0;i<MULTIPLE_STREAM_NUM;i++)
		pRTSPStreaming->iRTSPStreamingMediaType[i]=pstRTSPStreamingParameter->iRTSPStreamingMediaType[i];

	pRTSPStreaming->usRTSPPort = pstRTSPStreamingParameter->usRTSPPort;

#ifdef _SIP
	pRTSPStreaming->usRTPVideoPort = pstRTSPStreamingParameter->usRTPVPort;
	pRTSPStreaming->usRTPAudioPort = pstRTSPStreamingParameter->usRTPAPort;
	//20120801 added by Jimmy for metadata
	pRTSPStreaming->usRTPMetadataPort = pstRTSPStreamingParameter->usRTPMPort;
#endif

#ifdef _SIP
	memset(&tSIPUAParam,0,sizeof(TSIPUAParam));

	tSIPUAParam.dwVersion = SIPUA_VERSION ;
	tSIPUAParam.iUASAuthenticateMode = (pstRTSPStreamingParameter->tSIPParameter.iAuthenticateMode != 0)  ;
	tSIPUAParam.eUACTransportMode = SIPPROTO_UDP ;
	tSIPUAParam.eUASTransportMode = SIPPROTO_TCPUDP ;
	rtspstrcpy( tSIPUAParam.szAuthName, pstRTSPStreamingParameter->tSIPParameter.szUserName, sizeof(tSIPUAParam.szAuthName));
	rtspstrcpy( tSIPUAParam.szAuthPassword ,pstRTSPStreamingParameter->tSIPParameter.szPassword, sizeof(tSIPUAParam.szAuthPassword));
	rtspstrcpy( tSIPUAParam.szURIDomain , pstRTSPStreamingParameter->tSIPParameter.szSIPDomain, sizeof(tSIPUAParam.szURIDomain));
	rtspstrcpy( tSIPUAParam.szURIName  , pstRTSPStreamingParameter->tSIPParameter.szDisplayName, sizeof(tSIPUAParam.szURIName));
	tSIPUAParam.ulLocalIP = pstRTSPStreamingParameter->ulLocalIP;
	tSIPUAParam.usSIPPort = pstRTSPStreamingParameter->tSIPParameter.usPort;
	tSIPUAParam.ulOBProxyIP = pstRTSPStreamingParameter->tSIPParameter.ulOBProxyIP ;
	tSIPUAParam.usOBProxyPort = pstRTSPStreamingParameter->tSIPParameter.usOBProxyPort;
	tSIPUAParam.ulPublicIP = tSIPUAParam.usPublicPort = 0;
	tSIPUAParam.iMaxUASSessionNumber = pstRTSPStreamingParameter->tSIPParameter.iSIPMaxConnectionNum;

	printf("SIP parameters:\n"
		"Auth Mode:  %d\n"
		"Auth name:   %s\n"
		"Auth passwd: %s\n"
		"sip address:  %s@%s\n"
		"OB IP: %lu\n"
		"OB port:%d\n",
		tSIPUAParam.iUASAuthenticateMode,
		tSIPUAParam.szAuthName,tSIPUAParam.szAuthPassword,
		tSIPUAParam.szURIName, tSIPUAParam.szURIDomain,
		tSIPUAParam.ulOBProxyIP,tSIPUAParam.usOBProxyPort);

	tSIPUAParam.bCreateThread = TRUE;

	if( SIPUA_Initial(&pRTSPStreaming->hSIPUAHandle,&tSIPUAParam) != S_OK )
	{
		free(pRTSPStreaming);		//CID:408, CHECKER:RESOURCE_LEAK
		printf("[RTSPStreamingControl]: SIP UA create failed!\r\n");
		return 0;
	}

	if( SIPUA_SetCallback( pRTSPStreaming->hSIPUAHandle, (PFSIPUACallback)RTSPStreaming_SIPUAServerCallback, (DWORD)pRTSPStreaming) != S_OK)
	{
		free(pRTSPStreaming);		//CID:408, CHECKER:RESOURCE_LEAK
		printf("[RTSPStreamingControl]: SIPUA_SetCallback fail\r\n");
		return 0;
	}

	if( SDPDecoder_Initial(&pRTSPStreaming->hSDPDecoder, SDPDECODER_VERSION) != S_OK )
	{
		free(pRTSPStreaming);		//CID:408, CHECKER:RESOURCE_LEAK
		printf("[RTSPStreamingControl]: SDP decoder create failed \r\n");
		return 0;
	}
#endif //_SIP

	pRTSPStreaming->ulLocalIP=pstRTSPStreamingParameter->ulLocalIP;
	pRTSPStreaming->ulLocalSubnetMask=pstRTSPStreamingParameter->ulLocalSubnetMask;	
	pRTSPStreaming->ulNATIP=pstRTSPStreamingParameter->ulNATIP;

	//20081121 URL authentication by Louis
	stRTSPServerParam.iURLAuthEnabled = pstRTSPStreamingParameter->iURLAuthEnabled;

	for( i=0 ; i< MULTIPLE_STREAM_NUM ; i++)
	{
		memset(pRTSPStreaming->acAccessName[i],0,ACCESSNAME_LENGTH);
	    strncpy(pRTSPStreaming->acAccessName[i], pstRTSPStreamingParameter->acAccessName[i],ACCESSNAME_LENGTH-1);
	}

	//stRTSPServerParam.StreamingMode=pstRTSPStreamingParameter->iRTSPStreamingMediaType;

	stRTSPServerParam.ulIP = pRTSPStreaming->ulLocalIP;
	stRTSPServerParam.rtsp_port=pstRTSPStreamingParameter->usRTSPPort;
	stRTSPServerParam.usRTPVPort=pstRTSPStreamingParameter->usRTPVPort;
	stRTSPServerParam.usRTPAPort=pstRTSPStreamingParameter->usRTPAPort;
	//20120801 added by Jimmy for metadata
	stRTSPServerParam.usRTPMPort=pstRTSPStreamingParameter->usRTPMPort;
	stRTSPServerParam.usRTCPVPort=pstRTSPStreamingParameter->usRTCPVPort;
	stRTSPServerParam.usRTCPAPort=pstRTSPStreamingParameter->usRTCPAPort;
	//20120801 added by Jimmy for metadata
	stRTSPServerParam.usRTCPMPort=pstRTSPStreamingParameter->usRTCPMPort;

	stRTSPServerParam.ulThreadPriority = pstRTSPStreamingParameter->dwRTSPServerPriority;
	
	stRTSPServerParam.iUDPRTPVSock = pRTSPStreaming->iRTPVSock;
	stRTSPServerParam.iUDPRTPASock = pRTSPStreaming->iRTPASock;
	//20120801 added by Jimmy for metadata
	stRTSPServerParam.iUDPRTPMSock = pRTSPStreaming->iRTPMSock;

#ifdef _SHARED_MEM
	stRTSPServerParam.dwProtectedDelta = pstRTSPStreamingParameter->dwProtectedDelta;
#endif

#ifdef RTSPRTP_MULTICAST
	//20130327 modified by Jimmy to support different video/audio multicast addresses for ONVIF Test Tool 12.12
	for( i=0 ; i< (RTSP_MULTICASTNUMBER) ; i++ )
	{
		//20120801 modified by Jimmy for metadata
		//20110630 Add by danny For Multicast enable/disable
		if( pstRTSPStreamingParameter->stMulticastInfo[i].iEnable == 0
			|| pstRTSPStreamingParameter->stMulticastInfo[i].ulMulticastAddress == 0
			|| pstRTSPStreamingParameter->stMulticastInfo[i].usMulticastAudioPort == 0
			|| pstRTSPStreamingParameter->stMulticastInfo[i].usMulticastVideoPort == 0 
#ifdef _METADATA_ENABLE
			|| pstRTSPStreamingParameter->stMulticastInfo[i].usMulticastMetadataPort == 0
#endif
			)
		{
			stRTSPServerParam.iMulticastEnable[i] = pstRTSPStreamingParameter->stMulticastInfo[i].iEnable;
			stRTSPServerParam.ulMulticastAddress[i] = pstRTSPStreamingParameter->stMulticastInfo[i].ulMulticastAddress;
			TelnetShell_DbgPrint("[RTSPStreamControl]Multicast %d disabled!!\r\n", i);    
			continue;//return 0;
		}
		else
		{

			if((i >= RTSP_MULTICASTNUMBER) && (i < (RTSP_MULTICASTNUMBER)))
			{
				usLocalPortOffset = RTSP_MULTICASTNUMBER*MEDIA_TYPE_NUMBER*2;
			}
			else
			{
				usLocalPortOffset = 0;
			}

			//20120801 modified by Jimmy for metadata
			//20160127 modified by faber for separate multicast IP
			if( RTSPStreaming_CreateMulticastSocket(pstRTSPStreamingParameter->stMulticastInfo[i].ulMulticastAddress,
												pstRTSPStreamingParameter->stMulticastInfo[i].ulMulticastAudioAddress,
												pstRTSPStreamingParameter->stMulticastInfo[i].ulMulticastMetadataAddress,
												pstRTSPStreamingParameter->stMulticastInfo[i].usMulticastVideoPort,
												pstRTSPStreamingParameter->stMulticastInfo[i].usMulticastAudioPort,
												pstRTSPStreamingParameter->stMulticastInfo[i].usMulticastMetadataPort,
												(int*)pRTSPStreaming->stMulticast[i].aiMulticastSocket,
												(unsigned char) pstRTSPStreamingParameter->stMulticastInfo[i].usTTL,
												usLocalPortOffset) == 0)
			{
				stRTSPServerParam.iMulticastEnable[i] = pstRTSPStreamingParameter->stMulticastInfo[i].iEnable;
				stRTSPServerParam.ulMulticastAddress[i] = pstRTSPStreamingParameter->stMulticastInfo[i].ulMulticastAddress;
				//20160127 add by Faber, for separate multicast address
				stRTSPServerParam.ulMulticastAudioAddress[i] = pstRTSPStreamingParameter->stMulticastInfo[i].ulMulticastAudioAddress;
				stRTSPServerParam.ulMulticastMetadataAddress[i] = pstRTSPStreamingParameter->stMulticastInfo[i].ulMulticastMetadataAddress;

				stRTSPServerParam.usMulticastAudioPort[i] = pstRTSPStreamingParameter->stMulticastInfo[i].usMulticastAudioPort;
				stRTSPServerParam.usMulticastVideoPort[i] = pstRTSPStreamingParameter->stMulticastInfo[i].usMulticastVideoPort;
				//20120801 added by Jimmy for metadata
				stRTSPServerParam.usMulticastMetadataPort[i] = pstRTSPStreamingParameter->stMulticastInfo[i].usMulticastMetadataPort;				
				stRTSPServerParam.usTTL[i] = pstRTSPStreamingParameter->stMulticastInfo[i].usTTL;
				stRTSPServerParam.iRTPExtension[i] = pstRTSPStreamingParameter->stMulticastInfo[i].iRTPExtension;
				stRTSPServerParam.iMulticastSDPIndex[i] = pstRTSPStreamingParameter->stMulticastInfo[i].iSDPIndex;

				//20120801 modified by Jimmy for metadata
#ifdef _METADATA_ENABLE
				TelnetShell_DbgPrint("[RTSPStreamControl]Multicast Socket = %d %d %d %d %d %d create OK!!\r\n", 
											(int)pRTSPStreaming->stMulticast[i].aiMulticastSocket[0], 
											(int)pRTSPStreaming->stMulticast[i].aiMulticastSocket[1],
											(int)pRTSPStreaming->stMulticast[i].aiMulticastSocket[2],
											(int)pRTSPStreaming->stMulticast[i].aiMulticastSocket[3],
											(int)pRTSPStreaming->stMulticast[i].aiMulticastSocket[4],
											(int)pRTSPStreaming->stMulticast[i].aiMulticastSocket[5]);
#else
				TelnetShell_DbgPrint("[RTSPStreamControl]Multicast Socket = %d %d %d %d create OK!!\r\n", 
											(int)pRTSPStreaming->stMulticast[i].aiMulticastSocket[0], 
											(int)pRTSPStreaming->stMulticast[i].aiMulticastSocket[1],
											(int)pRTSPStreaming->stMulticast[i].aiMulticastSocket[2],
											(int)pRTSPStreaming->stMulticast[i].aiMulticastSocket[3]);

#endif
			}
			else
			{
				stRTSPServerParam.ulMulticastAddress[0] = 0;
				pRTSPStreaming->stMulticast[i].ulMulticastAddress = 0;
				TelnetShell_DbgPrint("[RTSPStreamControl]Multicast Socket create error!!\r\n");    
			}	
		}
	}
#endif

	pRTSPStreaming->hRTSPServerHandle=NULL;
//	while(pRTSPStreaming->hRTSPServerHandle == NULL)
//	{
		pRTSPStreaming->hRTSPServerHandle=RTSPServer_Create(pstRTSPStreamingParameter->iRTSPMaxConnectionNum, &stRTSPServerParam);
		if(pRTSPStreaming->hRTSPServerHandle == NULL )
		{
			free(pRTSPStreaming);		//CID:408, CHECKER:RESOURCE_LEAK
			printf("[StreamingServer]: create RTSP server fail \r\n");   
			return 0;
	}
//	}
	
	iResult=RTSPServer_SetCallback(pRTSPStreaming->hRTSPServerHandle, (RTSPSERVERCALLBACK)RTSPStreaming_RTSPServerCallback, (HANDLE)pRTSPStreaming);
	if(iResult != S_OK)
	{
		printf("[%s] RTSPServer_SetCallback failed\n", __func__);
	}
/* 20100623 danny, Added for fix sessioninfo corrupted issue */
#ifdef _SESSION_MGR
	if( SessionMgr_Initial(&pRTSPStreaming->hSessionMgrHandle) != S_OK )
	{
		free(pRTSPStreaming);		//CID:408, CHECKER:RESOURCE_LEAK
		printf("[RTSPStreamingControl]: SessionMgr create failed!\r\n");
		return 0;
	}
	RTSPServer_AddSessionMgrHandle(pRTSPStreaming->hRTSPServerHandle, pRTSPStreaming->hSessionMgrHandle);
#endif

	for( i = 0 ; i < iMaxSessionNumber ; i++ )
	{
	    hTCPMuxCS = NULL;  
#ifdef _SHARED_MEM
		if( RTSPCriticalSection_Initial(&hTCPMuxCS) == S_OK)
#else
		if( OSCriticalSection_Initial(&hTCPMuxCS) == S_OK)
#endif
	    
        {
        	//20160603 Modify by faber, no need to assign at init
            // RTPRTCPChannel_AddTCPMuxHandle(pRTSPStreaming->hRTPRTCPChannelVideoHandle,hTCPMuxCS);
            // RTPRTCPChannel_AddTCPMuxHandle(pRTSPStreaming->hRTPRTCPChannelAudioHandle,hTCPMuxCS);
			//20120801 added by Jimmy for metadata
            // RTPRTCPChannel_AddTCPMuxHandle(pRTSPStreaming->hRTPRTCPChannelMetadataHandle,hTCPMuxCS);

            pRTSPStreaming->pstSessionList[i].hTCPMuxCS = hTCPMuxCS;
            //printf("%dth TCPMux handle %lu\n",i,(unsigned long)hTCPMuxCS);
            //RTSPServer_AddTCPMuxHandle(pRTSPStreaming->hRTSPServerHandle, hTCPMuxCS);               
        }
        else
            TelnetShell_DbgPrint("TCP CS create Fail!!! \r\n");
    }
    
	if( pstRTSPStreamingParameter->iRTSPAuthentication == RTSPSTREAMING_AUTHENTICATION_BASIC )
	{
		printf("rtsp basic authentication\n");
		RTSPServer_SetAuthenticationType(pRTSPStreaming->hRTSPServerHandle,RTSP_AUTH_BASIC);
	}
	else if( pstRTSPStreamingParameter->iRTSPAuthentication == RTSPSTREAMING_AUTHENTICATION_DIGEST )
	{
		printf("rtsp digest authentication\n");
		RTSPServer_SetAuthenticationType(pRTSPStreaming->hRTSPServerHandle,RTSP_AUTH_DIGEST);
	}
	else
	{
		printf("rtsp no authentication\n");
		RTSPServer_SetAuthenticationType(pRTSPStreaming->hRTSPServerHandle,RTSP_AUTH_DISABLE);
	}

	return (HANDLE)pRTSPStreaming;	
}


int RTSPStreaming_Start(HANDLE hRTSPStreaming)
{
	RTSPSTREAMING *pRTSPStreaming;
	RTPRTCPCHANNEL_PARAM stRTPRTCPChannelParam;

	//20100714 Modified by danny For Multicast parameters load dynamically
	//RTPRTCPCOMPOSER_PARAM  stRTPRTCPComposerParam;
	//RTPRTCPCHANNEL_CONNECTION  stRTPRTCPSessionParam;

//	int	iResult;
	int i;
	
	if(!hRTSPStreaming)
		return -1;
	
	pRTSPStreaming=(RTSPSTREAMING *)hRTSPStreaming;	

//  Initialize Alias access name 20091009
	RTSPStreaming_InitializeAliasAccessName(pRTSPStreaming);

// audio channel start
//	if(pRTSPStreaming->iRTSPStreamingMediaType != RTSPSTREAMING_MEDIATYPE_VIDEOONLY)
	{
		RTPRTCPChannel_Start(pRTSPStreaming->hRTPRTCPChannelAudioHandle);
		printf("Audio Channel start\n");
	}
		
// video channel start
//	if(pRTSPStreaming->iRTSPStreamingMediaType != RTSPSTREAMING_MEDIATYPE_AUDIOONLY)
	{
		RTPRTCPChannel_Start(pRTSPStreaming->hRTPRTCPChannelVideoHandle);
		printf("Video Channel start\n");
	}

//20120806 added by Jimmy for metadata
// metadata channel start
//	if(pRTSPStreaming->iRTSPStreamingMediaType != RTSPSTREAMING_MEDIATYPE_AUDIOONLY)
	{
#ifdef _METADATA_ENABLE
		RTPRTCPChannel_Start(pRTSPStreaming->hRTPRTCPChannelMetadataHandle);
		printf("Metadata Channel start\n");
#endif
	}


	stRTPRTCPChannelParam.iRTPRTCPMediaType=RTPRTCPCHANNEL_MEDIATYPE_VIDEO;
	//stRTPRTCPChannelParam.pbyMPEG4StartBitStream=pRTSPStreaming->tVideoEncodeParam.acMPEG4Header;
	//stRTPRTCPChannelParam.iMPEG4StartBitStreamLength=pRTSPStreaming->tVideoEncodeParam.iMPEG4HeaderLen;
	RTPRTCPChannel_SetParameters( pRTSPStreaming->hRTPRTCPChannelVideoHandle, &stRTPRTCPChannelParam);
	
// RTSP server
	RTSPServer_Start(pRTSPStreaming->hRTSPServerHandle);
	printf("RTSP server start\n");

#ifdef _SIP
	if( SIPUA_Start(pRTSPStreaming->hSIPUAHandle) == S_OK)
		printf("SIP UA server start\r\n");
#endif

#ifndef _SHARED_MEM
//Video and Audio packetizer	
    RTPPacketizer_Start(pRTSPStreaming->hVideoPacketizer);
    RTPPacketizer_Start(pRTSPStreaming->hAudioPacketizer);
	//20120806 added by Jimmy for metadata
    RTPPacketizer_Start(pRTSPStreaming->hMetadataPacketizer);
	printf("video/audio/metadata packetizer start\n");
#endif

#ifdef RTSPRTP_MULTICAST
    //printf("SS start0 OK\r\n");

	OSSleep_Sec(2);
    //printf("SS start1 OK\r\n");
    //20130327 modified by Jimmy to support different video/audio multicast addresses for ONVIF Test Tool 12.12
	for( i = 0 ; i < (RTSP_MULTICASTNUMBER) ; i ++ )
	{
		//20100714 Modified by danny For Multicast parameters load dynamically
		// if( RTSPStreaming_AddMulticast(hRTSPStreaming, i) != S_OK )
		// {
		// 	return S_FAIL;
		// }
		if( pRTSPStreaming->stMulticast[i].iAlwaysMulticast &&
			pRTSPStreaming->acAccessName[pRTSPStreaming->stMulticast[i].iSDPIndex-1][0] != 0 &&
		    pRTSPStreaming->stMulticast[i].ulMulticastAddress != 0 )
		{
			RTSPStreaming_StartAlwaysMulticast(hRTSPStreaming, i + 1);
		}
		/*memset((void*)&stRTPRTCPSessionParam,0,sizeof(stRTPRTCPSessionParam));
        //printf("SS start2 OK\r\n");

		if( pRTSPStreaming->stMulticast[i].iAlwaysMulticast &&
			pRTSPStreaming->acAccessName[pRTSPStreaming->stMulticast[i].iSDPIndex-1][0] != 0 &&
		    pRTSPStreaming->stMulticast[i].ulMulticastAddress != 0 )
		{
            printf("always multicast %d start Audio %d Video %d\r\n", i, pRTSPStreaming->stMulticast[i].iAlwaysMulticastAudio, pRTSPStreaming->stMulticast[i].iAlwaysMulticastVideo);
			//Modified by Louis 2008/01/29 for always multicast video/audio only
			if((pRTSPStreaming->iRTSPStreamingMediaType[pRTSPStreaming->stMulticast[i].iSDPIndex-1] != RTSPSTREAMING_MEDIATYPE_VIDEOONLY) 
				&& (pRTSPStreaming->stMulticast[i].iAlwaysMulticastAudio == 1))
			{
#ifdef _SHARED_MEM
				TShmemSessionInfo *pShmSessionInfo = (TShmemSessionInfo *)pRTSPStreaming->stMulticast[i].hShmemSessionInfo;
#endif
#ifdef _G711_AUDIOIN
				if (pRTSPStreaming->tAudioEncodeParam[pRTSPStreaming->stMulticast[i].iSDPIndex -1].iAudioCodecType==ractG711u)
				{
					stRTPRTCPComposerParam.iMediaType = 0 ;
				}
				else if (pRTSPStreaming->tAudioEncodeParam[pRTSPStreaming->stMulticast[i].iSDPIndex -1].iAudioCodecType==ractG711a)
				{
					stRTPRTCPComposerParam.iMediaType = 8 ;
				}
				else
#endif
				stRTPRTCPComposerParam.iMediaType=RTSPSTREAMING_AMR_MEDIATYPE;

				if( pRTSPStreaming->tAudioEncodeParam[pRTSPStreaming->stMulticast[i].iSDPIndex -1 ].iClockRate == 0 )
				{
					printf("Audio clock rate of stream %d is zero!\n", pRTSPStreaming->stMulticast[i].iSDPIndex);
					return -1;
				}
				else
					stRTPRTCPComposerParam.iSampleFrequency=pRTSPStreaming->tAudioEncodeParam[pRTSPStreaming->stMulticast[i].iSDPIndex -1 ].iClockRate;

				stRTPRTCPComposerParam.dwInitialTimeStamp=0;
				stRTPRTCPComposerParam.wInitialSequenceNumber=0;
				stRTPRTCPComposerParam.dwSSRC=rand();

				RTPRTCPComposer_Reset(pRTSPStreaming->stMulticast[i].hRTPRTCPAudioComposerHandle,&stRTPRTCPComposerParam);
				stRTPRTCPSessionParam.iRTPStreamingType = RTP_OVER_UDP;
				stRTPRTCPSessionParam.sktRTP = pRTSPStreaming->stMulticast[i].aiMulticastSocket[2];
				stRTPRTCPSessionParam.sktRTCP = pRTSPStreaming->stMulticast[i].aiMulticastSocket[3];		
				stRTPRTCPSessionParam.hRTPRTCPComposerHandle=pRTSPStreaming->stMulticast[i].hRTPRTCPAudioComposerHandle;
                stRTPRTCPSessionParam.iVivotekClient = pRTSPStreaming->stMulticast[i].iRTPExtension;
#ifdef _SHARED_MEM
				pShmSessionInfo->dwProtectedDelta = 0;
				pShmSessionInfo->dwBypasyMSec = 0;
				pShmSessionInfo->tShmemAudioMediaInfo.hShmemHandle = pRTSPStreaming->ahShmemAudioHandle[0];
				pShmSessionInfo->tShmemAudioMediaInfo.bFrameGenerated = TRUE;
				stRTPRTCPSessionParam.ptShmemMediaInfo = &pShmSessionInfo->tShmemAudioMediaInfo;
#endif
				//stRTPRTCPSessionParam.iCodecIndex = pRTSPStreaming->stMulticast[i].iSDPIndex;
				printf("[StreamingServer]: Audio of stream type %d for scalable multicast start\n",pRTSPStreaming->stMulticast[i].iSDPIndex);
				stRTPRTCPSessionParam.iCodecIndex = pRTSPStreaming->fAudioCallBack((DWORD) pRTSPStreaming->hParentAudioHandle,
					                                   MEDIA_CALLBACK_CHECK_CODEC_INDEX,
													   (void*)pRTSPStreaming->stMulticast[i].iSDPIndex);
        		RTPRTCPChannel_AddMulticastSession(pRTSPStreaming->hRTPRTCPChannelAudioHandle, &stRTPRTCPSessionParam,i+1);
				//20100714 Added by danny For Multicast parameters load dynamically
				printf("[RTSPStreamServer]:Add Audio multicast channel ID:%ul Socket:%d\n",stRTPRTCPSessionParam.dwSessionID,stRTPRTCPSessionParam.sktRTP);   
				pRTSPStreaming->stMulticast[i].iAlreadyMulticastAudio = 1;
			}
			//Modified by Louis 2008/01/29 for always multicast video/audio only
			if((pRTSPStreaming->iRTSPStreamingMediaType[pRTSPStreaming->stMulticast[i].iSDPIndex-1] != RTSPSTREAMING_MEDIATYPE_AUDIOONLY )
				&& (pRTSPStreaming->stMulticast[i].iAlwaysMulticastVideo == 1))
			{
#ifdef _SHARED_MEM
				TShmemSessionInfo *pShmSessionInfo = (TShmemSessionInfo *)pRTSPStreaming->stMulticast[i].hShmemSessionInfo;
#endif
				//20090105 JPEG/H264, default is still MPEG4
				stRTPRTCPComposerParam.iMediaType=RTSPSTREAMING_MPEG4_MEDIATYPE;
				if(pRTSPStreaming->tVideoEncodeParam[pRTSPStreaming->stMulticast[i].iSDPIndex-1].eVideoCodecType == mctMP4V)
				{
					stRTPRTCPComposerParam.iMediaType=RTSPSTREAMING_MPEG4_MEDIATYPE;
				}
				else if(pRTSPStreaming->tVideoEncodeParam[pRTSPStreaming->stMulticast[i].iSDPIndex-1].eVideoCodecType == mctJPEG)
				{
					stRTPRTCPComposerParam.iMediaType=RTSPSTREAMING_JPEG_MEDIATYPE;
				}
				else if(pRTSPStreaming->tVideoEncodeParam[pRTSPStreaming->stMulticast[i].iSDPIndex-1].eVideoCodecType == mctH264)
				{
					stRTPRTCPComposerParam.iMediaType=RTSPSTREAMING_H264_MEDIATYPE;
				}
				
				if(pRTSPStreaming->tVideoEncodeParam[pRTSPStreaming->stMulticast[i].iSDPIndex -1 ].iClockRate == 0 )
				{
					printf("Video Clock rate of stream %d is zero!\n", pRTSPStreaming->stMulticast[i].iSDPIndex);
					return -1;
				}
				else
					stRTPRTCPComposerParam.iSampleFrequency=pRTSPStreaming->tVideoEncodeParam[pRTSPStreaming->stMulticast[i].iSDPIndex -1 ].iClockRate;

				stRTPRTCPComposerParam.dwInitialTimeStamp=0;
				stRTPRTCPComposerParam.wInitialSequenceNumber=0;
				stRTPRTCPComposerParam.dwSSRC=rand();
	
				RTPRTCPComposer_Reset(pRTSPStreaming->stMulticast[i].hRTPRTCPVideoComposerHandle,&stRTPRTCPComposerParam);
				stRTPRTCPSessionParam.iRTPStreamingType = RTP_OVER_UDP;
				stRTPRTCPSessionParam.sktRTP = pRTSPStreaming->stMulticast[i].aiMulticastSocket[0];
				stRTPRTCPSessionParam.sktRTCP = pRTSPStreaming->stMulticast[i].aiMulticastSocket[1];		
				stRTPRTCPSessionParam.hRTPRTCPComposerHandle=pRTSPStreaming->stMulticast[i].hRTPRTCPVideoComposerHandle;
	            stRTPRTCPSessionParam.iVivotekClient = pRTSPStreaming->stMulticast[i].iRTPExtension;
//				stRTPRTCPSessionParam.iCodecIndex = pRTSPStreaming->stMulticast[i].iSDPIndex;
#ifdef _SHARED_MEM
				pShmSessionInfo->dwProtectedDelta = 0;
				pShmSessionInfo->dwBypasyMSec = 0;
				pShmSessionInfo->tShmemVideoMediaInfo.hShmemHandle = pRTSPStreaming->ahShmemVideoHandle[pRTSPStreaming->stMulticast[i].iSDPIndex-1];
				pShmSessionInfo->tShmemVideoMediaInfo.bFrameGenerated = TRUE;
				stRTPRTCPSessionParam.ptShmemMediaInfo = &pShmSessionInfo->tShmemVideoMediaInfo;
#endif
                printf("[StreamingServer]: Video of stream type %d for scalable multicast start\n",pRTSPStreaming->stMulticast[i].iSDPIndex);				
                stRTPRTCPSessionParam.iCodecIndex = pRTSPStreaming->fVideoCallBack((DWORD) pRTSPStreaming->hParentVideoHandle,
					                                   MEDIA_CALLBACK_CHECK_CODEC_INDEX,
													   (void*)pRTSPStreaming->stMulticast[i].iSDPIndex);

				RTPRTCPChannel_AddMulticastSession(pRTSPStreaming->hRTPRTCPChannelVideoHandle, &stRTPRTCPSessionParam,i+1);
				//20100714 Added by danny For Multicast parameters load dynamically
				printf("[RTSPStreamServer]:Add Video multicast channel ID:%ul Socket:%d\n",stRTPRTCPSessionParam.dwSessionID,stRTPRTCPSessionParam.sktRTP);   
				pRTSPStreaming->stMulticast[i].iAlreadyMulticastVideo = 1;
			}
		}*/
	}
#endif
    //printf("SS start OK\r\n",i);
    return 0;
}


int RTSPStreaming_Stop(HANDLE hRTSPStreaming)
{
	RTSPSTREAMING *pRTSPStreaming;
	int				i;

	if(!hRTSPStreaming)
		return -1;
	
	pRTSPStreaming=(RTSPSTREAMING *)hRTSPStreaming;	
#ifndef _SHARED_MEM
	//printf("before stop packetizer \n");
	// RTP Packetizer
    RTPPacketizer_STOP(pRTSPStreaming->hVideoPacketizer);
    RTPPacketizer_STOP(pRTSPStreaming->hAudioPacketizer);
    //20120806 added by Jimmy for metadata
    RTPPacketizer_STOP(pRTSPStreaming->hMetadataPacketizer);
	//printf("after stop packetizer \n");
#endif
	//printf("before stop rtsp \n");
// RTSP server
	RTSPServer_Stop(pRTSPStreaming->hRTSPServerHandle);
	//printf("after stop rtsp \n");
	
	//printf("before stop sip \n");
#ifdef _SIP
	SIPUA_Stop(pRTSPStreaming->hSIPUAHandle);
#endif
	//printf("after stop sip \n");
	
	//printf("before stop media channel \n");
// audio channel stop
    RTPRTCPChannel_Stop(pRTSPStreaming->hRTPRTCPChannelAudioHandle);
		
// video channel stop
	RTPRTCPChannel_Stop(pRTSPStreaming->hRTPRTCPChannelVideoHandle);

//20120806 added by Jimmy for metadata
// metadata channel stop
#ifdef _METADATA_ENABLE
	RTPRTCPChannel_Stop(pRTSPStreaming->hRTPRTCPChannelMetadataHandle);
#endif

	//printf("after stop media channel \n");
#ifdef _LINUX
	for( i = 0; i < pRTSPStreaming->iSessionListNumber ; i++)
	{
#ifdef _INET6
		//20101123 Added by danny For support advanced system log 
		if (pRTSPStreaming->pstSessionList[i].ulClientIP == 0)
		{
			char szPresentString[64]="";
			if ( pRTSPStreaming->bAdvLogSupport == TRUE )
			{
				openlog("[RTSP SERVER]",0, LOG_LOCAL0);
				syslog(LOG_INFO,"Stop one session, IP=%s\n", inet_ntop(AF_INET6, &pRTSPStreaming->pstSessionList[i].tClientSckAddr.sin6_addr, szPresentString, sizeof(szPresentString)));
				openlog("[RTSP SERVER]",0, LOG_USER);
			}
			else
			{
				syslog(LOG_INFO,"Stop one session, IP=%s\n", inet_ntop(AF_INET6, &pRTSPStreaming->pstSessionList[i].tClientSckAddr.sin6_addr, szPresentString, sizeof(szPresentString)));
			}
		}
		else
#endif
		{
			if ( pRTSPStreaming->bAdvLogSupport == TRUE )
			{
				openlog("[RTSP SERVER]",0, LOG_LOCAL0);
				syslog(LOG_INFO,"Stop one session, IP=%s\n", inet_ntoa(*((struct in_addr*)&(pRTSPStreaming->pstSessionList[i].ulClientIP))));
				openlog("[RTSP SERVER]",0, LOG_USER);
			}
			else
			{
				syslog(LOG_INFO,"Stop one session, IP=%s\n", inet_ntoa(*((struct in_addr*)&(pRTSPStreaming->pstSessionList[i].ulClientIP))));
			}
		}
	}
#endif	
    return 0;
}

int RTSPStreaming_Close(HANDLE* phRTSPStreaming)
{
	RTSPSTREAMING *pRTSPStreaming;
#ifndef _SHARED_MEM
	RTPMEDIABUFFER	* pMediaBuf;
#endif
	int i,j;

	if( *phRTSPStreaming == NULL)
		return -1;
	
	pRTSPStreaming=(RTSPSTREAMING *)*phRTSPStreaming;	
  
   	//close fixed port UDP sockets and multicast sockets   	
   	closesocket(pRTSPStreaming->iRTPVSock);
   	closesocket(pRTSPStreaming->iRTPASock);
   	closesocket(pRTSPStreaming->iRTCPVSock);
   	closesocket(pRTSPStreaming->iRTCPASock);
	//20120806 added by Jimmy for metadata
#ifdef _METADATA_ENABLE
	closesocket(pRTSPStreaming->iRTPMSock);
   	closesocket(pRTSPStreaming->iRTCPMSock);
#endif

//	printf("before release sip \n");
#ifdef _SIP
	SIPUA_Release(&pRTSPStreaming->hSIPUAHandle);
	SDPDecoder_Release(&pRTSPStreaming->hSDPDecoder);
#endif
//	printf("after release sip \n");

/* 20100623 danny, Added for fix sessioninfo corrupted issue */
#ifdef _SESSION_MGR
	SessionMgr_Release(&pRTSPStreaming->hSessionMgrHandle);
#endif

#ifdef RTSPRTP_MULTICAST    	
	//20130327 modified by Jimmy to support different video/audio multicast addresses for ONVIF Test Tool 12.12
	//20130905 modified by Charles for ondemand multicast
	for(i=0;i<(RTSP_MULTICASTNUMBER+RTSP_ONDEMAND_MULTICASTNUMBER);i++)
	{
		//20120806 modified by Jimmy for metadata
		for(j=0;j<MEDIA_TYPE_NUMBER*2;j++)
		{
			if(pRTSPStreaming->stMulticast[i].aiMulticastSocket[j]>0)
			{
				closesocket(pRTSPStreaming->stMulticast[i].aiMulticastSocket[j]);
			}
		}
   	}
#endif   	
//	printf("before release rtsp \n");
// RTSP server
	RTSPServer_Close(pRTSPStreaming->hRTSPServerHandle);
//	printf("after release rtsp \n");
// audio channel stop
	RTPRTCPChannel_Release(pRTSPStreaming->hRTPRTCPChannelAudioHandle);
// video channel stop
    RTPRTCPChannel_Release(pRTSPStreaming->hRTPRTCPChannelVideoHandle);
//20120806 added by Jimmy for metadata
// metadata channel stop
    RTPRTCPChannel_Release(pRTSPStreaming->hRTPRTCPChannelMetadataHandle);

	//printf("after release mediachannel \n");
	// modified by cchuang, 2005/05/24
	IPAccessCheck_Close(pRTSPStreaming->hIPAccessCheckHandle);

	for(i=0;i<pRTSPStreaming->iMaximumSessionCount; i++)
	{
		RTPRTCPComposer_Close(pRTSPStreaming->pstSessionList[i].hRTPRTCPAudioComposerHandle);
		RTPRTCPComposer_Close(pRTSPStreaming->pstSessionList[i].hRTPRTCPVideoComposerHandle);
		//20120806 added by Jimmy for metadata
		RTPRTCPComposer_Close(pRTSPStreaming->pstSessionList[i].hRTPRTCPMetadataComposerHandle);
		OSCriticalSection_Release((HANDLE*)&(pRTSPStreaming->pstSessionList[i].hTCPMuxCS));
	}
#ifdef RTSPRTP_MULTICAST
	//20130327 modified by Jimmy to support different video/audio multicast addresses for ONVIF Test Tool 12.12
	//20130905 modified by Charles for ondemand multicast
	for( i=0; i<(RTSP_MULTICASTNUMBER+RTSP_ONDEMAND_MULTICASTNUMBER) ; i++ )
	{
		RTPRTCPComposer_Close(pRTSPStreaming->stMulticast[i].hRTPRTCPAudioComposerHandle);
		RTPRTCPComposer_Close(pRTSPStreaming->stMulticast[i].hRTPRTCPVideoComposerHandle);
		//20120806 added by Jimmy for metadata
		RTPRTCPComposer_Close(pRTSPStreaming->stMulticast[i].hRTPRTCPMetadataComposerHandle);
#ifdef _SHARED_MEM
		RTSPStreaming_ReleaseShmSessionInfo(&pRTSPStreaming->stMulticast[i].hShmemSessionInfo);
#endif
	}
#endif
	//printf("after release rtp/rtcp \n");
#ifdef _SHARED_MEM
	for(i = 0; i < pRTSPStreaming->iMaximumSessionCount; i++)
	{
		//Initialize Shm info for each session
		RTSPStreaming_ReleaseShmSessionInfo(&pRTSPStreaming->pstSessionList[i].hShmemSessionInfo);
	}
#else
// RTP Packetizer
   	RTPPacketizer_Release(&pRTSPStreaming->hVideoPacketizer);
    RTPPacketizer_Release(&pRTSPStreaming->hAudioPacketizer);
	//20120806 added by Jimmy for metadata
    RTPPacketizer_Release(&pRTSPStreaming->hMetadataPacketizer);

	//printf("after release packetizer \n");
	
	while( !MediaBufQueue_GetMediaBuffer(pRTSPStreaming->hVideoDataQueueHandle, 0, (void **)&pMediaBuf) ) 
	{
		if(pMediaBuf != NULL )
		{
			free(pMediaBuf);				
		}	
	}
	//printf("after release video data queue \n");
	
	while( !MediaBufQueue_GetMediaBuffer(pRTSPStreaming->hVideoEmptyQueueHandle, 0, (void **)&pMediaBuf) ) 
	{
		if(pMediaBuf != NULL )
		{
			free(pMediaBuf);				
		}	
	}
	//printf("after release video buffer queue \n");

	while( !MediaBufQueue_GetMediaBuffer(pRTSPStreaming->hAudioDataQueueHandle, 0, (void **)&pMediaBuf) ) 
	{
		if(pMediaBuf != NULL )
		{
			free(pMediaBuf);				
		}	
	}

	//printf("after release audio data queue\n");

	while( !MediaBufQueue_GetMediaBuffer(pRTSPStreaming->hAudioEmptyQueueHandle, 0, (void **)&pMediaBuf) ) 
	{
		if(pMediaBuf != NULL )
		{
			free(pMediaBuf);				
		}	
	}

	//printf("after release audio buffer queue \n");

	//20120806 added by Jimmy for metadata
	while( !MediaBufQueue_GetMediaBuffer(pRTSPStreaming->hMetadataDataQueueHandle, 0, (void **)&pMediaBuf) ) 
	{
		if(pMediaBuf != NULL )
		{
			free(pMediaBuf);				
		}	
	}
	//printf("after release metadata data queue \n");

	//20120806 added by Jimmy for metadata
	while( !MediaBufQueue_GetMediaBuffer(pRTSPStreaming->hMetadataEmptyQueueHandle, 0, (void **)&pMediaBuf) ) 
	{
		if(pMediaBuf != NULL )
		{
			free(pMediaBuf);				
		}	
	}

	//printf("after release metadata buffer queue \n");


	MediaBufQueue_Delete(pRTSPStreaming->hVideoDataQueueHandle);
	MediaBufQueue_Delete(pRTSPStreaming->hVideoEmptyQueueHandle);
	
	MediaBufQueue_Delete(pRTSPStreaming->hAudioEmptyQueueHandle);
	MediaBufQueue_Delete(pRTSPStreaming->hAudioDataQueueHandle);

	//20120806 added by Jimmy for metadata
	MediaBufQueue_Delete(pRTSPStreaming->hMetadataEmptyQueueHandle);
	MediaBufQueue_Delete(pRTSPStreaming->hMetadataDataQueueHandle);

#endif //ifndef _SHARED_MEM
	//printf("after release data buffer\n");
	
    OSSemaphore_Release(&pRTSPStreaming->hMediaParamSemaphore);
    OSSemaphore_Release(&pRTSPStreaming->ulSessionListSemaphore);

    free(pRTSPStreaming->pstSessionList);
    free(pRTSPStreaming->pstRTPOverHTTPInfo);
    
	free((void*)pRTSPStreaming);
    *phRTSPStreaming = NULL;

	return 0;
}
#ifdef _SHARED_MEM
SCODE RTSPStreaming_SetShmemVideoCallback(HANDLE hRTSPStreamingHandle, MEDIA_CALLBACK pfnCallback, HANDLE hParentHandle)
{
    RTSPSTREAMING *pRTSPStreaming;
   
    if(!hRTSPStreamingHandle || !pfnCallback)
		return S_FAIL;
		
    pRTSPStreaming=(RTSPSTREAMING *)hRTSPStreamingHandle;	

    pRTSPStreaming->fShmemVideoCallBack = pfnCallback;
    
    return 0;
}
SCODE RTSPStreaming_SetShmemAudioCallback(HANDLE hRTSPStreamingHandle, MEDIA_CALLBACK pfnCallback, HANDLE hParentHandle)
{
    RTSPSTREAMING *pRTSPStreaming;
   
    if(!hRTSPStreamingHandle || !pfnCallback)
		return S_FAIL;
		
    pRTSPStreaming=(RTSPSTREAMING *)hRTSPStreamingHandle;	

    pRTSPStreaming->fShmemAudioCallBack = pfnCallback;
    
    return 0;
}
//20120801 added by Jimmy for metadata
SCODE RTSPStreaming_SetShmemMetadataCallback(HANDLE hRTSPStreamingHandle, MEDIA_CALLBACK pfnCallback, HANDLE hParentHandle)
{
    RTSPSTREAMING *pRTSPStreaming;
   
    if(!hRTSPStreamingHandle || !pfnCallback)
		return S_FAIL;
		
    pRTSPStreaming=(RTSPSTREAMING *)hRTSPStreamingHandle;	

    pRTSPStreaming->fShmemMetadataCallBack = pfnCallback;
    
    return 0;
}
#endif

SCODE RTSPStreaming_SetControlCallback(HANDLE hRTSPStreamingHandle, FControlChannel_Callback pfnCallback, HANDLE hParentHandle)
{
    RTSPSTREAMING *pRTSPStreaming;
   
    if(!hRTSPStreamingHandle || !pfnCallback)
		return S_FAIL;
		
    pRTSPStreaming=(RTSPSTREAMING *)hRTSPStreamingHandle;	

    pRTSPStreaming->fControlCallBack = pfnCallback;
    pRTSPStreaming->hParentControlHandle = hParentHandle;
    
    return 0;
}
    
SCODE RTSPStreaming_SetVideoCallback(HANDLE hRTSPStreamingHandle, MEDIA_CALLBACK pfnCallback, HANDLE hParentHandle)
{
    RTSPSTREAMING *pRTSPStreaming;
   
    if(!hRTSPStreamingHandle || !pfnCallback)
		return S_FAIL;
		
    pRTSPStreaming=(RTSPSTREAMING *)hRTSPStreamingHandle;	

    pRTSPStreaming->fVideoCallBack = pfnCallback;
    pRTSPStreaming->hParentVideoHandle = hParentHandle;
    
    return 0;
}
    
SCODE RTSPStreaming_SetAudioCallback(HANDLE hRTSPStreamingHandle, MEDIA_CALLBACK pfnCallback, HANDLE hParentHandle)
{
    RTSPSTREAMING *pRTSPStreaming;
   
    if(!hRTSPStreamingHandle || !pfnCallback)
		return S_FAIL;
		
    pRTSPStreaming=(RTSPSTREAMING *)hRTSPStreamingHandle;	

    pRTSPStreaming->fAudioCallBack = pfnCallback;
    pRTSPStreaming->hParentAudioHandle = hParentHandle;
    
    return 0;
}

//20120801 added by Jimmy for metadata
SCODE RTSPStreaming_SetMetadataCallback(HANDLE hRTSPStreamingHandle, MEDIA_CALLBACK pfnCallback, HANDLE hParentHandle)
{
    RTSPSTREAMING *pRTSPStreaming;
   
    if(!hRTSPStreamingHandle || !pfnCallback)
		return S_FAIL;
		
    pRTSPStreaming=(RTSPSTREAMING *)hRTSPStreamingHandle;	

    pRTSPStreaming->fMetadataCallBack= pfnCallback;
    pRTSPStreaming->hParentMetadataHandle= hParentHandle;
    
    return 0;
}



	
int RTSPStreaming_SetHostName(HANDLE hRTSPStreaming, char * pcRTSPStreamingHostName)
{
	RTSPSTREAMING *pRTSPStreaming;

	if(!hRTSPStreaming)
		return -1;
	
	pRTSPStreaming=(RTSPSTREAMING *)hRTSPStreaming;	

	memset(pRTSPStreaming->acHostName,0,20);
	strncpy(pRTSPStreaming->acHostName,pcRTSPStreamingHostName,19);
	
	return 0;
	
}	

int RTSPStreaming_SetSDPETag(HANDLE hRTSPStreaming, char * pcSDPETag)
{
	RTSPSTREAMING *pRTSPStreaming;

	if(!hRTSPStreaming || !pcSDPETag)
		return -1;
	
	pRTSPStreaming=(RTSPSTREAMING *)hRTSPStreaming;		
	rtspstrcpy(pRTSPStreaming->acSDPETag,pcSDPETag, sizeof(pRTSPStreaming->acSDPETag));
	
	return 0;
	
}	

//20090224 QOS 
SCODE RTSPStreaming_SetQosParameters(HANDLE hRTSPStreaming, TQosInfo *ptQosInfo)
{
	RTSPSTREAMING	*pRTSPStreaming;
	int				i = 0;

	if(!hRTSPStreaming)
		return S_FAIL;
	
	pRTSPStreaming=(RTSPSTREAMING *)hRTSPStreaming;

	//Set fixed UDP socket

	RTSPServer_SetQosToSocket(pRTSPStreaming->iRTPVSock, ptQosInfo, eQosVideoType);
	RTSPServer_SetQosToSocket(pRTSPStreaming->iRTCPVSock, ptQosInfo, eQosVideoType);
	RTSPServer_SetQosToSocket(pRTSPStreaming->iRTPASock, ptQosInfo, eQosAudioType);
	RTSPServer_SetQosToSocket(pRTSPStreaming->iRTCPASock, ptQosInfo, eQosAudioType);
	//20120801 added by Jimmy for metadata
	RTSPServer_SetQosToSocket(pRTSPStreaming->iRTPMSock, ptQosInfo, eQosMetadataType);
	RTSPServer_SetQosToSocket(pRTSPStreaming->iRTCPMSock, ptQosInfo, eQosMetadataType);

	//Set fixed multicast socket
	//20130327 modified by Jimmy to support different video/audio multicast addresses for ONVIF Test Tool 12.12
	//20130904 modified by Charles for ondemand multicast
	for( i=0 ; i< (RTSP_MULTICASTNUMBER + RTSP_ONDEMAND_MULTICASTNUMBER) ; i++ )
	{
		RTSPServer_SetQosToSocket(pRTSPStreaming->stMulticast->aiMulticastSocket[0], ptQosInfo, eQosVideoType);
		RTSPServer_SetQosToSocket(pRTSPStreaming->stMulticast->aiMulticastSocket[1], ptQosInfo, eQosVideoType);
		RTSPServer_SetQosToSocket(pRTSPStreaming->stMulticast->aiMulticastSocket[2], ptQosInfo, eQosAudioType);
		RTSPServer_SetQosToSocket(pRTSPStreaming->stMulticast->aiMulticastSocket[3], ptQosInfo, eQosAudioType);
		//20120801 added by Jimmy for metadata
#ifdef _METADATA_ENABLE
		RTSPServer_SetQosToSocket(pRTSPStreaming->stMulticast->aiMulticastSocket[4], ptQosInfo, eQosMetadataType);
		RTSPServer_SetQosToSocket(pRTSPStreaming->stMulticast->aiMulticastSocket[5], ptQosInfo, eQosMetadataType);
#endif
	}
	//Update the qos parameters in RTSP server module
	RTSPServer_UpdateQosParameters(pRTSPStreaming->hRTSPServerHandle, ptQosInfo);

	return S_OK;
}

#ifdef _SHARED_MEM
/* 20100105 Added For Seamless Recording */ 
SCODE RTSPStreaming_SetSeamlessRecordingParameters(HANDLE hRTSPStreaming, TSeamlessRecordingInfo *ptSeamlessRecordingInfo)
{
	RTSPSTREAMING	*pRTSPStreaming;

	if(!hRTSPStreaming)
		return S_FAIL;
	
	pRTSPStreaming=(RTSPSTREAMING *)hRTSPStreaming;

	//Update the Seamless Recording parameters in RTSP server module
	RTSPServer_UpdateSeamlessRecordingParameters(pRTSPStreaming->hRTSPServerHandle, ptSeamlessRecordingInfo);

	return S_OK;
}

/* 20100428 Added For Media on demand */
SCODE RTSPStreaming_UpdateMODControlParameters(HANDLE hRTSPStreaming, TUBufferConfMOD  *pMODConfUBuffer)
{
	RTSPSTREAMING	*pRTSPStreaming;

	if(!hRTSPStreaming)
		return S_FAIL;
	
	pRTSPStreaming=(RTSPSTREAMING *)hRTSPStreaming;

	//20140605 added by Charles for ONVIF track token
	if((EMODRunCode)pMODConfUBuffer->dwRuncode == MOD_INFO_TRACKTOKEN)
	{
		RTSPStreaming_SetMODTrackTokenParameters(hRTSPStreaming, pMODConfUBuffer->dwStreamID + LIVE_STREAM_NUM, pMODConfUBuffer->acTrackToken);
		
		pRTSPStreaming->tModControlParam[pMODConfUBuffer->dwStreamID - 1].iMODSetControlInfoReady = 1;
		
	}
	else if( pMODConfUBuffer->bIsFromMODActively == TRUE )
	{
	    //20140812 modified by Charles, Don't close session when receiving MOD_INFO_ALLPLAYED from MOD
		if( (EMODRunCode)pMODConfUBuffer->dwRuncode != MOD_INFO_OK && 
		    (EMODRunCode)pMODConfUBuffer->dwRuncode != MOD_INFO_ALLPLAYED) 
		{
			printf("Removing client for stream %d from MOD server, eRuncode %d\n", pMODConfUBuffer->dwStreamID + LIVE_STREAM_NUM, (EMODRunCode)pMODConfUBuffer->dwRuncode);
			//20130315 added by Jimmy to log more information
			syslog(LOG_DEBUG, "Removing client for stream %d from MOD server, eRuncode %d\n", pMODConfUBuffer->dwStreamID + LIVE_STREAM_NUM, (EMODRunCode)pMODConfUBuffer->dwRuncode);
			RTSPStreaming_RemoveSpecificStreamClients(hRTSPStreaming, pMODConfUBuffer->dwStreamID + LIVE_STREAM_NUM);
		}
	}
	else
	{
		if( (EMODCommandType)pMODConfUBuffer->dwCommandType != 0)
		{
			pRTSPStreaming->tModControlParam[pMODConfUBuffer->dwStreamID - 1].iMODSetControlInfoReady = 1;
		}
		pRTSPStreaming->tModControlParam[pMODConfUBuffer->dwStreamID - 1].eMODRunCode = (EMODRunCode)pMODConfUBuffer->dwRuncode;
		pRTSPStreaming->tModControlParam[pMODConfUBuffer->dwStreamID - 1].eMODReturnCommandType = (EMODCommandType)pMODConfUBuffer->dwCommandType;
		rtspstrcpy(pRTSPStreaming->tModControlParam[pMODConfUBuffer->dwStreamID - 1].acMODReturnCommandValue, 
			pMODConfUBuffer->acCommandResult, sizeof(pRTSPStreaming->tModControlParam[pMODConfUBuffer->dwStreamID - 1].acMODReturnCommandValue));

		printf("[%s]Set stream %d eMODRunCode %d, eMODReturnCommandType %d, acMODReturnCommandValue %s\n",
											__FUNCTION__, pMODConfUBuffer->dwStreamID + LIVE_STREAM_NUM, 
											pRTSPStreaming->tModControlParam[pMODConfUBuffer->dwStreamID - 1].eMODRunCode, 
											pRTSPStreaming->tModControlParam[pMODConfUBuffer->dwStreamID - 1].eMODReturnCommandType,
											pRTSPStreaming->tModControlParam[pMODConfUBuffer->dwStreamID - 1].acMODReturnCommandValue);
	}
	
	return S_OK;
}
#endif

int RTSPStreaming_SetDynamicParameters(HANDLE hRTSPStreaming, TRTSPSTREAMING_DYNAMIC_PARAM *pstRTSPStreamingDynamicParam,DWORD dwSetFlag)
{
	RTSPSTREAMING *pRTSPStreaming;
	//RTSPSERVER_PARAM stRTSPServerParam;
	int i;

	if(!hRTSPStreaming)
		return -1;
	
	pRTSPStreaming=(RTSPSTREAMING *)hRTSPStreaming;
	
	if(!pRTSPStreaming->hRTSPServerHandle)
		return -1;
		
	if( dwSetFlag & RTSPSTREAMING_ACCESSNAME_SETFLAG )
	{		
		for( i=0 ; i< MULTIPLE_STREAM_NUM ; i++)
		{
		    OSSemaphore_Get(pRTSPStreaming->hMediaParamSemaphore,INFINITE);
			memset(pRTSPStreaming->acAccessName[i],0,ACCESSNAME_LENGTH);
			strncpy(pRTSPStreaming->acAccessName[i], pstRTSPStreamingDynamicParam->acAccessName[i],ACCESSNAME_LENGTH-1);
	        OSSemaphore_Post(pRTSPStreaming->hMediaParamSemaphore);
		}
	}

	if( dwSetFlag & RTSPSTREAMING_MEDIAMODE_SETFLAG )
	{
		for( i=0 ; i< MULTIPLE_STREAM_NUM ; i++)
		{
			//20120807 modified by Jimmy for metadata
			pRTSPStreaming->iRTSPStreamingMediaType[i] = pstRTSPStreamingDynamicParam->iRTSPStreamingMediaType[i];
			/*
			if( pstRTSPStreamingDynamicParam->iRTSPStreamingMediaType[i] == RTSPSTREAMING_MEDIATYPE_AUDIOVIDEO )
				pRTSPStreaming->iRTSPStreamingMediaType[i] = RTSPSTREAMING_MEDIATYPE_AUDIOVIDEO;

			if( pstRTSPStreamingDynamicParam->iRTSPStreamingMediaType[i] == RTSPSTREAMING_MEDIATYPE_AUDIOONLY )
				pRTSPStreaming->iRTSPStreamingMediaType[i] = RTSPSTREAMING_MEDIATYPE_AUDIOONLY;

			if( pstRTSPStreamingDynamicParam->iRTSPStreamingMediaType[i] == RTSPSTREAMING_MEDIATYPE_VIDEOONLY )
				pRTSPStreaming->iRTSPStreamingMediaType[i] = RTSPSTREAMING_MEDIATYPE_VIDEOONLY;
			*/

		}
	}

	if( dwSetFlag & RTSPSTREAMING_RTSP_AUTHENTICATE_SETFLAG )
	{
		if (pstRTSPStreamingDynamicParam->iRTSPAuthentication == RTSPSTREAMING_AUTHENTICATION_BASIC )
			RTSPServer_SetAuthenticationType(pRTSPStreaming->hRTSPServerHandle,RTSP_AUTH_BASIC);
		else if (pstRTSPStreamingDynamicParam->iRTSPAuthentication == RTSPSTREAMING_AUTHENTICATION_DIGEST )
			RTSPServer_SetAuthenticationType(pRTSPStreaming->hRTSPServerHandle,RTSP_AUTH_DIGEST);
		else
			RTSPServer_SetAuthenticationType(pRTSPStreaming->hRTSPServerHandle,RTSP_AUTH_DISABLE);
			
#ifdef _SIP
		if (pstRTSPStreamingDynamicParam->iSIPUASAuthentication == RTSPSTREAMING_AUTHENTICATION_DISABLE)
		{
			SIPUA_SetAuthenticationType(pRTSPStreaming->hSIPUAHandle, FALSE);
		}
		else
		{
			SIPUA_SetAuthenticationType(pRTSPStreaming->hSIPUAHandle, TRUE);
		}
#endif
	}

	return 0;	
}	

int RTSPStreaming_RemoveSpecificStreamClients(HANDLE hRTSPStreaming,int iStreamIndex)
{
	RTSPSTREAMING	*pRTSPStreaming;
	int				i = 0;

	pRTSPStreaming=(RTSPSTREAMING *)hRTSPStreaming;

	//20100204 fixed session remove incompletely when change codec type 
	for( i = 0; i < MAX_CONNECT_NUM ; i++)
	{
		if( pRTSPStreaming->pstSessionList[i].dwSessionID != 0 &&  pRTSPStreaming->pstSessionList[i].iSDPIndex == iStreamIndex)
		{
			RTSPStreaming_RemoveSession(hRTSPStreaming, pRTSPStreaming->pstSessionList[i].dwSessionID);
		}
	}

	return 0;
}

#ifdef RTSPRTP_MULTICAST
//20100720 Added by danny to fix Backchannel multicast session terminated, rtsp server has not stopped sending video/audio RTP/RTCP
int RTSPStreaming_GetMulticastCurrentSessionNumber(HANDLE hRTSPStreaming, int iMulticastIndex)
{
   	RTSPSTREAMING *pRTSPStreaming;

	if(!hRTSPStreaming)
		return -1;
	
	pRTSPStreaming=(RTSPSTREAMING *)hRTSPStreaming;	
	
	return RTSPServer_GetMulticastCurrentSessionNumber(pRTSPStreaming->hRTSPServerHandle, iMulticastIndex);

}

//20100714 Added by danny For Multicast parameters load dynamically
SCODE RTSPStreaming_CheckIfMulticastParametersChanged(HANDLE hRTSPStreaming, int iMulticastCount, MULTICASTINFO	*ptMulticastInfo)
{
   	RTSPSTREAMING *pRTSPStreaming;

	if(!hRTSPStreaming)
		return S_FAIL;
	
	pRTSPStreaming=(RTSPSTREAMING *)hRTSPStreaming;	

	//20120807 modified by Jimmy for metadata
	//20110725 Add by danny For Multicast RTCP receive report keep alive
	//20110630 Add by danny For Multicast enable/disable
	if( pRTSPStreaming->stMulticast[iMulticastCount].iEnable != ptMulticastInfo->iEnable ||
		pRTSPStreaming->stMulticast[iMulticastCount].iRRAlive != ptMulticastInfo->iRRAlive ||
		pRTSPStreaming->stMulticast[iMulticastCount].iAlwaysMulticast != ptMulticastInfo->iAlwaysMulticast ||
		pRTSPStreaming->stMulticast[iMulticastCount].usMulticastVideoPort != ptMulticastInfo->usMulticastVideoPort ||
		pRTSPStreaming->stMulticast[iMulticastCount].usMulticastAudioPort != ptMulticastInfo->usMulticastAudioPort ||
		pRTSPStreaming->stMulticast[iMulticastCount].usMulticastMetadataPort != ptMulticastInfo->usMulticastMetadataPort ||
		pRTSPStreaming->stMulticast[iMulticastCount].ulMulticastAddress != ptMulticastInfo->ulMulticastAddress ||
		pRTSPStreaming->stMulticast[iMulticastCount].ulMulticastAudioAddress != ptMulticastInfo->ulMulticastAudioAddress ||
		pRTSPStreaming->stMulticast[iMulticastCount].ulMulticastMetadataAddress != ptMulticastInfo->ulMulticastMetadataAddress ||
		pRTSPStreaming->stMulticast[iMulticastCount].usTTL != ptMulticastInfo->usTTL ||
		pRTSPStreaming->stMulticast[iMulticastCount].iRTPExtension != ptMulticastInfo->iRTPExtension )
    {
		return S_OK;
    }
    
    return S_FAIL;

}

SCODE RTSPStreaming_SetMulticastParameters(HANDLE hRTSPStreaming, int iMulticastCount, MULTICASTINFO *ptMulticastInfo)
{
	RTSPSTREAMING	*pRTSPStreaming;
	int				i;
	unsigned short	usLocalPortOffset = 0;

	if(!hRTSPStreaming)
		return S_FAIL;
	
	pRTSPStreaming = (RTSPSTREAMING *)hRTSPStreaming;

	//20110630 Add by danny For Multicast enable/disable
	pRTSPStreaming->stMulticast[iMulticastCount].iEnable = ptMulticastInfo->iEnable;
	//20110725 Add by danny For Multicast RTCP receive report keep alive
	pRTSPStreaming->stMulticast[iMulticastCount].iRRAlive = ptMulticastInfo->iRRAlive;
	pRTSPStreaming->stMulticast[iMulticastCount].iAlwaysMulticast = ptMulticastInfo->iAlwaysMulticast;
	pRTSPStreaming->stMulticast[iMulticastCount].usMulticastVideoPort = ptMulticastInfo->usMulticastVideoPort;
	pRTSPStreaming->stMulticast[iMulticastCount].usMulticastAudioPort = ptMulticastInfo->usMulticastAudioPort;
	//20120801 added by Jimmy for metadata
	pRTSPStreaming->stMulticast[iMulticastCount].usMulticastMetadataPort = ptMulticastInfo->usMulticastMetadataPort;
	pRTSPStreaming->stMulticast[iMulticastCount].ulMulticastAddress = ptMulticastInfo->ulMulticastAddress;
	//20160127 Add by Faber, separate multicast address
	pRTSPStreaming->stMulticast[iMulticastCount].ulMulticastAudioAddress =  ptMulticastInfo->ulMulticastAudioAddress;
	pRTSPStreaming->stMulticast[iMulticastCount].ulMulticastMetadataAddress = ptMulticastInfo->ulMulticastMetadataAddress;

	pRTSPStreaming->stMulticast[iMulticastCount].usTTL = ptMulticastInfo->usTTL;
	pRTSPStreaming->stMulticast[iMulticastCount].iRTPExtension = ptMulticastInfo->iRTPExtension;
	pRTSPStreaming->stMulticast[iMulticastCount].iSDPIndex = ptMulticastInfo->iSDPIndex;
	//20120801 modified by Jimmy for metadata
	for( i=0; i<MEDIA_TYPE_NUMBER*2; i++)
	{
		if(pRTSPStreaming->stMulticast[iMulticastCount].aiMulticastSocket[i] > 0)
		closesocket(pRTSPStreaming->stMulticast[iMulticastCount].aiMulticastSocket[i]);
	}

	//20110630 Add by danny For Multicast enable/disable
	//20120801 modified by Jimmy for metadata
	//20160406 removed by Faber, ondemand multicast could be 0
// 	if( ptMulticastInfo->iEnable == 0
// 		|| ptMulticastInfo->ulMulticastAddress == 0
// 		|| ptMulticastInfo->usMulticastAudioPort == 0
// 		|| ptMulticastInfo->usMulticastVideoPort == 0
// #ifdef _METADATA_ENABLE
// 		|| ptMulticastInfo->usMulticastMetadataPort == 0
// #endif
// 		)
// 	{
// 		TelnetShell_DbgPrint("[%s]Multicast %d disabled!!\r\n", __FUNCTION__, iMulticastCount);
// 		//Update the Multicast parameters in RTSP server module
// 		RTSPServer_UpdateMulticastParameters(pRTSPStreaming->hRTSPServerHandle, iMulticastCount, ptMulticastInfo);
// 		return S_FAIL;
// 	}

	
// 	if((iMulticastCount >= RTSP_MULTICASTNUMBER ) && (iMulticastCount < (RTSP_MULTICASTNUMBER)))
// 	{
// 		usLocalPortOffset = RTSP_MULTICASTNUMBER*MEDIA_TYPE_NUMBER*2;
// 	}
// 	else
// 	{
// 		usLocalPortOffset = 0;
// 	}


	//20120801 modified by Jimmy for metadata
	//20160127 modified by faber for separate multicast IP 
	if( RTSPStreaming_CreateMulticastSocket(pRTSPStreaming->stMulticast[iMulticastCount].ulMulticastAddress,
											pRTSPStreaming->stMulticast[iMulticastCount].ulMulticastAudioAddress,
											pRTSPStreaming->stMulticast[iMulticastCount].ulMulticastMetadataAddress,
											pRTSPStreaming->stMulticast[iMulticastCount].usMulticastVideoPort,
											pRTSPStreaming->stMulticast[iMulticastCount].usMulticastAudioPort,
											pRTSPStreaming->stMulticast[iMulticastCount].usMulticastMetadataPort,
											(int*)pRTSPStreaming->stMulticast[iMulticastCount].aiMulticastSocket,
											(unsigned char) pRTSPStreaming->stMulticast[iMulticastCount].usTTL,
											usLocalPortOffset) == 0)
	{
		//20120801 modified by Jimmy for metadata
#ifdef _METADATA_ENABLE
		TelnetShell_DbgPrint("[%s]Multicast Socket = %d %d %d %d %d %d create OK!!\r\n", __FUNCTION__, 
										(int)pRTSPStreaming->stMulticast[iMulticastCount].aiMulticastSocket[0], 
										(int)pRTSPStreaming->stMulticast[iMulticastCount].aiMulticastSocket[1],
										(int)pRTSPStreaming->stMulticast[iMulticastCount].aiMulticastSocket[2],
										(int)pRTSPStreaming->stMulticast[iMulticastCount].aiMulticastSocket[3],
										(int)pRTSPStreaming->stMulticast[iMulticastCount].aiMulticastSocket[4],
										(int)pRTSPStreaming->stMulticast[iMulticastCount].aiMulticastSocket[5]);
#else
		TelnetShell_DbgPrint("[%s]Multicast Socket = %d %d %d %d create OK!!\r\n", __FUNCTION__,
									(int)pRTSPStreaming->stMulticast[iMulticastCount].aiMulticastSocket[0], 
									(int)pRTSPStreaming->stMulticast[iMulticastCount].aiMulticastSocket[1],
									(int)pRTSPStreaming->stMulticast[iMulticastCount].aiMulticastSocket[2],
									(int)pRTSPStreaming->stMulticast[iMulticastCount].aiMulticastSocket[3]);

#endif
	}
	else
	{
		pRTSPStreaming->stMulticast[iMulticastCount].ulMulticastAddress = 0;
		TelnetShell_DbgPrint("[%s]Multicast Socket create error!!\r\n", __FUNCTION__);
		return S_FAIL;
	}

	//Update the Multicast parameters in RTSP server module
	RTSPServer_UpdateMulticastParameters(pRTSPStreaming->hRTSPServerHandle, iMulticastCount, ptMulticastInfo);

	return S_OK;
}

SCODE RTSPStreaming_AddMulticast(HANDLE hRTSPStreaming, int iMulticastCount)
{
   	RTSPSTREAMING *pRTSPStreaming;
	RTPRTCPCOMPOSER_PARAM  stRTPRTCPComposerParam;
	RTPRTCPCHANNEL_CONNECTION  stRTPRTCPSessionParam;
#ifdef _SHARED_MEM
	//20100728 Modifed by danny For multiple channels videoin/audioin
	int iTrackIndex;
#endif
	//20100714 Modified by danny For Multicast parameters load dynamically
	int i;


	if(!hRTSPStreaming)
		return S_FAIL;
	
	pRTSPStreaming=(RTSPSTREAMING *)hRTSPStreaming;	

	memset((void*)&stRTPRTCPSessionParam,0,sizeof(stRTPRTCPSessionParam));

	if( pRTSPStreaming->stMulticast[iMulticastCount].iAlwaysMulticast &&
		pRTSPStreaming->acAccessName[pRTSPStreaming->stMulticast[iMulticastCount].iSDPIndex-1][0] != 0 &&
		pRTSPStreaming->stMulticast[iMulticastCount].ulMulticastAddress != 0 )
	{
		printf("[%s]always multicast %d start Audio %d Video %d\r\n", __FUNCTION__, iMulticastCount, 
			pRTSPStreaming->stMulticast[iMulticastCount].iAlwaysMulticastAudio, pRTSPStreaming->stMulticast[iMulticastCount].iAlwaysMulticastVideo);
		//Modified by Louis 2008/01/29 for always multicast video/audio only
		//if((pRTSPStreaming->iRTSPStreamingMediaType[pRTSPStreaming->stMulticast[iMulticastCount].iSDPIndex-1] != RTSPSTREAMING_MEDIATYPE_VIDEOONLY) && (pRTSPStreaming->stMulticast[iMulticastCount].iAlwaysMulticastAudio == 1))
		//20120806 modified by Jimmy for metadata
		if((pRTSPStreaming->iRTSPStreamingMediaType[pRTSPStreaming->stMulticast[iMulticastCount].iSDPIndex-1] & RTSPSTREAMING_MEDIATYPE_AUDIO) && (pRTSPStreaming->stMulticast[iMulticastCount].iAlwaysMulticastAudio == 1))
		{
			//20100714 Modified by danny For Multicast parameters load dynamically, avoid start always multicast sometimes failed
			for( i = 0; i < RTSPSTREAMING_MULTICAST_ADDWAITING_COUNT; i++ )
			{
				if( RTPRTCPChannel_CheckMulticastAddAvailable(pRTSPStreaming->hRTPRTCPChannelAudioHandle, iMulticastCount) == S_OK)
				{
					break;
				}
				else
				{
					OSSleep_MSec(RTSPSTREAMING_MULTICAST_ADDWAITING_INTERVAL_MSEC);
				}
			}
			if( i == RTSPSTREAMING_MULTICAST_ADDWAITING_COUNT )
			{
				printf("[%s]Waitting Multicast Add Audio Available Timeout!\n", __FUNCTION__);
				syslog(LOG_ERR," [%s] Waitting Multicast Add Audio Available Timeout\n", __FUNCTION__);   
			}
			
#ifdef _SHARED_MEM
			TShmemSessionInfo *pShmSessionInfo = (TShmemSessionInfo *)pRTSPStreaming->stMulticast[iMulticastCount].hShmemSessionInfo;
#endif
#ifdef _G711_AUDIOIN
			if (pRTSPStreaming->tAudioEncodeParam[pRTSPStreaming->stMulticast[iMulticastCount].iSDPIndex -1].iAudioCodecType==ractG711u)
			{
				stRTPRTCPComposerParam.iMediaType = 0 ;
			}
			else if (pRTSPStreaming->tAudioEncodeParam[pRTSPStreaming->stMulticast[iMulticastCount].iSDPIndex -1].iAudioCodecType==ractG711a)
			{
				stRTPRTCPComposerParam.iMediaType = 8 ;
			}
			else
#endif
			stRTPRTCPComposerParam.iMediaType=RTSPSTREAMING_AMR_MEDIATYPE;

			if( pRTSPStreaming->tAudioEncodeParam[pRTSPStreaming->stMulticast[iMulticastCount].iSDPIndex -1 ].iClockRate == 0 )
			{
				printf("Audio clock rate of stream %d is zero!\n", pRTSPStreaming->stMulticast[iMulticastCount].iSDPIndex);
				return S_FAIL;
			}
			else
			{
				stRTPRTCPComposerParam.iSampleFrequency=pRTSPStreaming->tAudioEncodeParam[pRTSPStreaming->stMulticast[iMulticastCount].iSDPIndex -1 ].iClockRate;
			}
			
			stRTPRTCPComposerParam.dwInitialTimeStamp=0;
			stRTPRTCPComposerParam.wInitialSequenceNumber=0;
			stRTPRTCPComposerParam.dwSSRC=rand();

			RTPRTCPComposer_Reset(pRTSPStreaming->stMulticast[iMulticastCount].hRTPRTCPAudioComposerHandle,&stRTPRTCPComposerParam);
			stRTPRTCPSessionParam.iRTPStreamingType = RTP_OVER_UDP;
			//20111205 Modified by danny For UDP mode socket leak
			stRTPRTCPSessionParam.psktRTP = &pRTSPStreaming->stMulticast[iMulticastCount].aiMulticastSocket[2];
			stRTPRTCPSessionParam.psktRTCP = &pRTSPStreaming->stMulticast[iMulticastCount].aiMulticastSocket[3];
			stRTPRTCPSessionParam.hRTPRTCPComposerHandle=pRTSPStreaming->stMulticast[iMulticastCount].hRTPRTCPAudioComposerHandle;
			stRTPRTCPSessionParam.iVivotekClient = pRTSPStreaming->stMulticast[iMulticastCount].iRTPExtension;
			//20110627 Add by danny for join/leave multicast group by session start/stop
			stRTPRTCPSessionParam.ulMulticastAddress = pRTSPStreaming->stMulticast[iMulticastCount].ulMulticastAddress;
			stRTPRTCPSessionParam.usMulticastRTCPPort = pRTSPStreaming->stMulticast[iMulticastCount].usMulticastAudioPort + 1;
			//20110725 Add by danny For Multicast RTCP receive report keep alive
			stRTPRTCPSessionParam.iRRAlive = pRTSPStreaming->stMulticast[iMulticastCount].iRRAlive;
#ifdef _SHARED_MEM
			pShmSessionInfo->dwProtectedDelta = 0;
			pShmSessionInfo->dwBypasyMSec = 0;
			//20100728 Modifed by danny For multiple channels videoin/audioin
			iTrackIndex = pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle,
					                    					stRTPRTCPSessionParam.dwSessionID,
					                    					ccctGetMultipleChannelChannelIndex,
					                    					(DWORD)pRTSPStreaming->stMulticast[iMulticastCount].iSDPIndex) - 1;
			//20130605 modified by Jimmy to support metadata event
			pShmSessionInfo->tShmemAudioMediaInfo.ahShmemHandle[0] = pRTSPStreaming->ahShmemAudioHandle[iTrackIndex];
			if(pShmSessionInfo->tShmemAudioMediaInfo.ahShmemHandle[0] == NULL)
			{
				printf("[%s]pShmSessionInfo->tShmemAudioMediaInfo.ahShmemHandle[0](%p) == NULL, iTrackIndex %d\n", __FUNCTION__, 
					&pShmSessionInfo->tShmemAudioMediaInfo.ahShmemHandle[0], iTrackIndex);
				syslog(LOG_ERR," [%s] pShmSessionInfo->tShmemAudioMediaInfo.ahShmemHandle[0](%p) == NULL, iTrackIndex %d\n", __FUNCTION__, 
					&pShmSessionInfo->tShmemAudioMediaInfo.ahShmemHandle[0], iTrackIndex);
			}
			pShmSessionInfo->tShmemAudioMediaInfo.bFrameGenerated = TRUE;
			stRTPRTCPSessionParam.ptShmemMediaInfo = &pShmSessionInfo->tShmemAudioMediaInfo;
#endif
			//stRTPRTCPSessionParam.iCodecIndex = pRTSPStreaming->stMulticast[i].iSDPIndex;
			printf("[%s]: Audio of stream type %d for scalable multicast start\n", __FUNCTION__, pRTSPStreaming->stMulticast[iMulticastCount].iSDPIndex);
			stRTPRTCPSessionParam.iCodecIndex = pRTSPStreaming->fAudioCallBack((DWORD) pRTSPStreaming->hParentAudioHandle,
														MEDIA_CALLBACK_CHECK_CODEC_INDEX, 
														(void*)pRTSPStreaming->stMulticast[iMulticastCount].iSDPIndex);

#ifdef _SHARED_MEM
			//20101018 Add by danny for support multiple channel text on video
			if( pRTSPStreaming->stMulticast[iMulticastCount].iSDPIndex <= LIVE_STREAM_NUM)
			{
				stRTPRTCPSessionParam.iMultipleChannelChannelIndex = iTrackIndex + 1;
			}
#endif
			//20130327 modified by Jimmy to support different video/audio multicast addresses for ONVIF Test Tool 12.12
			//20170727 multicast 
			stRTPRTCPSessionParam.iSdpIndex = pRTSPStreaming->stMulticast[iMulticastCount].iSDPIndex;
			RTPRTCPChannel_AddMulticastSession(pRTSPStreaming->hRTPRTCPChannelAudioHandle, &stRTPRTCPSessionParam, iMulticastCount + 1);
			//20111205 Modified by danny For UDP mode socket leak
			//20100714 Add by danny For Multicast parameters load dynamically
			printf("[%s]:Add Audio multicast channel ID:%ul Socket:%d\n", __FUNCTION__, stRTPRTCPSessionParam.dwSessionID, *stRTPRTCPSessionParam.psktRTP);
			pRTSPStreaming->stMulticast[iMulticastCount].iAlreadyMulticastAudio = 1;
		}

		//Modified by Louis 2008/01/29 for always multicast video/audio only
		//if((pRTSPStreaming->iRTSPStreamingMediaType[pRTSPStreaming->stMulticast[iMulticastCount].iSDPIndex-1] != RTSPSTREAMING_MEDIATYPE_AUDIOONLY ) && (pRTSPStreaming->stMulticast[iMulticastCount].iAlwaysMulticastVideo == 1))
		//20120806 modified by Jimmy for metadata
		if((pRTSPStreaming->iRTSPStreamingMediaType[pRTSPStreaming->stMulticast[iMulticastCount].iSDPIndex-1] & RTSPSTREAMING_MEDIATYPE_VIDEO) && (pRTSPStreaming->stMulticast[iMulticastCount].iAlwaysMulticastVideo == 1))
		{
			//20100714 Modified by danny For Multicast parameters load dynamically, avoid start always multicast sometimes failed
			for( i = 0; i < RTSPSTREAMING_MULTICAST_ADDWAITING_COUNT; i++ )
			{
				if( RTPRTCPChannel_CheckMulticastAddAvailable(pRTSPStreaming->hRTPRTCPChannelVideoHandle, iMulticastCount) == S_OK)
				{
					break;
				}
				else
				{
					OSSleep_MSec(RTSPSTREAMING_MULTICAST_ADDWAITING_INTERVAL_MSEC);
				}
			}
			if( i == RTSPSTREAMING_MULTICAST_ADDWAITING_COUNT )
			{
				printf("[%s]Waitting Multicast Add Video Available Timeout!\n", __FUNCTION__);
				syslog(LOG_ERR," [%s] Waitting Multicast Add Video Available Timeout\n", __FUNCTION__);   
			}
			
#ifdef _SHARED_MEM
			TShmemSessionInfo *pShmSessionInfo = (TShmemSessionInfo *)pRTSPStreaming->stMulticast[iMulticastCount].hShmemSessionInfo;
#endif
			//20090105 JPEG/H264, default is still MPEG4
			stRTPRTCPComposerParam.iMediaType=RTSPSTREAMING_MPEG4_MEDIATYPE;
			if(pRTSPStreaming->tVideoEncodeParam[pRTSPStreaming->stMulticast[iMulticastCount].iSDPIndex-1].eVideoCodecType == mctMP4V)
			{
				stRTPRTCPComposerParam.iMediaType=RTSPSTREAMING_MPEG4_MEDIATYPE;
			}
			else if(pRTSPStreaming->tVideoEncodeParam[pRTSPStreaming->stMulticast[iMulticastCount].iSDPIndex-1].eVideoCodecType == mctJPEG)
			{
				stRTPRTCPComposerParam.iMediaType=RTSPSTREAMING_JPEG_MEDIATYPE;
			}
			else if(pRTSPStreaming->tVideoEncodeParam[pRTSPStreaming->stMulticast[iMulticastCount].iSDPIndex-1].eVideoCodecType == mctH264)
			{
				stRTPRTCPComposerParam.iMediaType=RTSPSTREAMING_H264_MEDIATYPE;
			}
            //20150113 added by Charles for H.265
            else if(pRTSPStreaming->tVideoEncodeParam[pRTSPStreaming->stMulticast[iMulticastCount].iSDPIndex-1].eVideoCodecType == mctH265)
			{
				stRTPRTCPComposerParam.iMediaType=RTSPSTREAMING_H265_MEDIATYPE;
			}
				
			if(pRTSPStreaming->tVideoEncodeParam[pRTSPStreaming->stMulticast[iMulticastCount].iSDPIndex -1 ].iClockRate == 0 )
			{
				printf("Video Clock rate of stream %d is zero!\n", pRTSPStreaming->stMulticast[iMulticastCount].iSDPIndex);
				return S_FAIL;
			}
			else
			{
				stRTPRTCPComposerParam.iSampleFrequency=pRTSPStreaming->tVideoEncodeParam[pRTSPStreaming->stMulticast[iMulticastCount].iSDPIndex -1 ].iClockRate;
			}
			
			stRTPRTCPComposerParam.dwInitialTimeStamp=0;
			stRTPRTCPComposerParam.wInitialSequenceNumber=0;
			stRTPRTCPComposerParam.dwSSRC=rand();

			RTPRTCPComposer_Reset(pRTSPStreaming->stMulticast[iMulticastCount].hRTPRTCPVideoComposerHandle,&stRTPRTCPComposerParam);
			stRTPRTCPSessionParam.iRTPStreamingType = RTP_OVER_UDP;
			//20111205 Modified by danny For UDP mode socket leak
			stRTPRTCPSessionParam.psktRTP = &pRTSPStreaming->stMulticast[iMulticastCount].aiMulticastSocket[0];
			stRTPRTCPSessionParam.psktRTCP = &pRTSPStreaming->stMulticast[iMulticastCount].aiMulticastSocket[1];		
			stRTPRTCPSessionParam.hRTPRTCPComposerHandle=pRTSPStreaming->stMulticast[iMulticastCount].hRTPRTCPVideoComposerHandle;
			stRTPRTCPSessionParam.iVivotekClient = pRTSPStreaming->stMulticast[iMulticastCount].iRTPExtension;
			//20110627 Add by danny for join/leave multicast group by session start/stop
			stRTPRTCPSessionParam.ulMulticastAddress = pRTSPStreaming->stMulticast[iMulticastCount].ulMulticastAddress;
			stRTPRTCPSessionParam.usMulticastRTCPPort = pRTSPStreaming->stMulticast[iMulticastCount].usMulticastVideoPort + 1;
			//20110725 Add by danny For Multicast RTCP receive report keep alive
			stRTPRTCPSessionParam.iRRAlive = pRTSPStreaming->stMulticast[iMulticastCount].iRRAlive;
//			stRTPRTCPSessionParam.iCodecIndex = pRTSPStreaming->stMulticast[i].iSDPIndex;
#ifdef _SHARED_MEM
			//20100812 Added For Client Side Frame Rate Control
			pShmSessionInfo->iFrameIntervalMSec = -1;
			pShmSessionInfo->dwProtectedDelta = 0;
			pShmSessionInfo->dwBypasyMSec = 0;
			//20130605 modified by Jimmy to support metadata event
			pShmSessionInfo->tShmemVideoMediaInfo.ahShmemHandle[0] = pRTSPStreaming->ahShmemVideoHandle[pRTSPStreaming->stMulticast[iMulticastCount].iSDPIndex-1];
			if(pShmSessionInfo->tShmemVideoMediaInfo.ahShmemHandle[0] == NULL)
			{
				printf("[%s]pShmSessionInfo->tShmemVideoMediaInfo.ahShmemHandle[0](%p) == NULL, %d\n", __FUNCTION__, 
					&pShmSessionInfo->tShmemVideoMediaInfo.ahShmemHandle[0], pRTSPStreaming->stMulticast[iMulticastCount].iSDPIndex-1);
				syslog(LOG_ERR," [%s] pShmSessionInfo->tShmemVideoMediaInfo.ahShmemHandle[0](%p) == NULL, %d\n", __FUNCTION__, 
					&pShmSessionInfo->tShmemVideoMediaInfo.ahShmemHandle[0], pRTSPStreaming->stMulticast[iMulticastCount].iSDPIndex-1);
			}
			pShmSessionInfo->tShmemVideoMediaInfo.bFrameGenerated = TRUE;
			stRTPRTCPSessionParam.ptShmemMediaInfo = &pShmSessionInfo->tShmemVideoMediaInfo;
#endif
			printf("[%s]: Video of stream type %d for scalable multicast start\n", __FUNCTION__, pRTSPStreaming->stMulticast[iMulticastCount].iSDPIndex);
			stRTPRTCPSessionParam.iCodecIndex = pRTSPStreaming->fVideoCallBack((DWORD) pRTSPStreaming->hParentVideoHandle,
														MEDIA_CALLBACK_CHECK_CODEC_INDEX, 
														(void*)pRTSPStreaming->stMulticast[iMulticastCount].iSDPIndex);

#ifdef _SHARED_MEM
			//20101018 Add by danny for support multiple channel text on video
			if( pRTSPStreaming->stMulticast[iMulticastCount].iSDPIndex <= LIVE_STREAM_NUM)
			{
				stRTPRTCPSessionParam.iMultipleChannelChannelIndex = pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle,
					                    								stRTPRTCPSessionParam.dwSessionID,
					                    								ccctGetMultipleChannelChannelIndex,
					                    								(DWORD)pRTSPStreaming->stMulticast[iMulticastCount].iSDPIndex);
			}
#endif
			//20130327 modified by Jimmy to support different video/audio multicast addresses for ONVIF Test Tool 12.12
			//20170727 multicast 
			stRTPRTCPSessionParam.iSdpIndex = pRTSPStreaming->stMulticast[iMulticastCount].iSDPIndex;
			RTPRTCPChannel_AddMulticastSession(pRTSPStreaming->hRTPRTCPChannelVideoHandle, &stRTPRTCPSessionParam, iMulticastCount + 1);
			//20111205 Modified by danny For UDP mode socket leak
			//20100714 Add by danny For Multicast parameters load dynamically
			printf("[%s]:Add Video multicast channel ID:%ul Socket:%d\n", __FUNCTION__, stRTPRTCPSessionParam.dwSessionID, *stRTPRTCPSessionParam.psktRTP);
			pRTSPStreaming->stMulticast[iMulticastCount].iAlreadyMulticastVideo = 1;
		}

		//20120806 added by Jimmy for metadata
#ifdef _METADATA_ENABLE
		if((pRTSPStreaming->iRTSPStreamingMediaType[pRTSPStreaming->stMulticast[iMulticastCount].iSDPIndex-1] & RTSPSTREAMING_MEDIATYPE_METADATA) && (pRTSPStreaming->stMulticast[iMulticastCount].iAlwaysMulticastMetadata== 1))
		{
			//20100714 Modified by danny For Multicast parameters load dynamically, avoid start always multicast sometimes failed
			for( i = 0; i < RTSPSTREAMING_MULTICAST_ADDWAITING_COUNT; i++ )
			{
				if( RTPRTCPChannel_CheckMulticastAddAvailable(pRTSPStreaming->hRTPRTCPChannelMetadataHandle, iMulticastCount) == S_OK)
				{
					break;
				}
				else
				{
					OSSleep_MSec(RTSPSTREAMING_MULTICAST_ADDWAITING_INTERVAL_MSEC);
				}
			}
			if( i == RTSPSTREAMING_MULTICAST_ADDWAITING_COUNT )
			{
				printf("[%s]Waitting Multicast Add Metadata Available Timeout!\n", __FUNCTION__);
				syslog(LOG_ERR," [%s] Waitting Multicast Add Metadata Available Timeout\n", __FUNCTION__);   
			}
			
#ifdef _SHARED_MEM
			TShmemSessionInfo *pShmSessionInfo = (TShmemSessionInfo *)pRTSPStreaming->stMulticast[iMulticastCount].hShmemSessionInfo;
#endif
			stRTPRTCPComposerParam.iMediaType=RTSPSTREAMING_MEDIATYPE_METADATA;

			if( pRTSPStreaming->tMetadataEncodeParam[pRTSPStreaming->stMulticast[iMulticastCount].iSDPIndex -1 ].iClockRate == 0 )
			{
				printf("Metadata clock rate of stream %d is zero!\n", pRTSPStreaming->stMulticast[iMulticastCount].iSDPIndex);
				return S_FAIL;
			}
			else
			{
				stRTPRTCPComposerParam.iSampleFrequency=pRTSPStreaming->tMetadataEncodeParam[pRTSPStreaming->stMulticast[iMulticastCount].iSDPIndex -1 ].iClockRate;
			}
			
			stRTPRTCPComposerParam.dwInitialTimeStamp=0;
			stRTPRTCPComposerParam.wInitialSequenceNumber=0;
			stRTPRTCPComposerParam.dwSSRC=rand();

			RTPRTCPComposer_Reset(pRTSPStreaming->stMulticast[iMulticastCount].hRTPRTCPMetadataComposerHandle,&stRTPRTCPComposerParam);
			stRTPRTCPSessionParam.iRTPStreamingType = RTP_OVER_UDP;
			//20111205 Modified by danny For UDP mode socket leak
			stRTPRTCPSessionParam.psktRTP = &pRTSPStreaming->stMulticast[iMulticastCount].aiMulticastSocket[4];
			stRTPRTCPSessionParam.psktRTCP = &pRTSPStreaming->stMulticast[iMulticastCount].aiMulticastSocket[5];
			stRTPRTCPSessionParam.hRTPRTCPComposerHandle=pRTSPStreaming->stMulticast[iMulticastCount].hRTPRTCPMetadataComposerHandle;
			stRTPRTCPSessionParam.iVivotekClient = pRTSPStreaming->stMulticast[iMulticastCount].iRTPExtension;
			//20110627 Add by danny for join/leave multicast group by session start/stop
			stRTPRTCPSessionParam.ulMulticastAddress = pRTSPStreaming->stMulticast[iMulticastCount].ulMulticastAddress;
			stRTPRTCPSessionParam.usMulticastRTCPPort = pRTSPStreaming->stMulticast[iMulticastCount].usMulticastAudioPort + 1;
			//20110725 Add by danny For Multicast RTCP receive report keep alive
			stRTPRTCPSessionParam.iRRAlive = pRTSPStreaming->stMulticast[iMulticastCount].iRRAlive;
#ifdef _SHARED_MEM
			pShmSessionInfo->dwProtectedDelta = 0;
			pShmSessionInfo->dwBypasyMSec = 0;
			//20100728 Modifed by danny For multiple channels videoin/audioin
			iTrackIndex = pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle,
															stRTPRTCPSessionParam.dwSessionID,
															ccctGetMultipleChannelChannelIndex,
															(DWORD)pRTSPStreaming->stMulticast[iMulticastCount].iSDPIndex) - 1;
			/*
			pShmSessionInfo->tShmemMetadataMediaInfo.ahShmemHandle = pRTSPStreaming->ahShmemMetadataHandle[iTrackIndex];
			if(pShmSessionInfo->tShmemMetadataMediaInfo.ahShmemHandle == NULL)
			{
				printf("[%s]pShmSessionInfo->tShmemMetadataMediaInfo.ahShmemHandle(%p) == NULL, iTrackIndex %d\n", __FUNCTION__, 
					&pShmSessionInfo->tShmemMetadataMediaInfo.ahShmemHandle, iTrackIndex);
				syslog(LOG_ERR," [%s] pShmSessionInfo->tShmemMetadataMediaInfo.ahShmemHandle(%p) == NULL, iTrackIndex %d\n", __FUNCTION__, 
					&pShmSessionInfo->tShmemMetadataMediaInfo.ahShmemHandle, iTrackIndex);
			}
			*/
			//20130605 modified by Jimmy to support metadata event
			for ( i = 0; i < SHMEM_HANDLE_MAX_NUM; i++)
			{
				pShmSessionInfo->tShmemMetadataMediaInfo.ahShmemHandle[i] = pRTSPStreaming->ahShmemMetadataHandle[iTrackIndex][i];
				if(pShmSessionInfo->tShmemMetadataMediaInfo.ahShmemHandle[i] == NULL)
				{
					printf("[%s]pShmSessionInfo->tShmemMetadataMediaInfo.ahShmemHandle[%d](%p) == NULL, iTrackIndex %d\n", __FUNCTION__, 
						i, &pShmSessionInfo->tShmemMetadataMediaInfo.ahShmemHandle[i], iTrackIndex);
					syslog(LOG_ERR," [%s] pShmSessionInfo->tShmemMetadataMediaInfo.ahShmemHandle[%d](%p) == NULL, iTrackIndex %d\n", __FUNCTION__, 
						i, &pShmSessionInfo->tShmemMetadataMediaInfo.ahShmemHandle[i], iTrackIndex);
				}
			}
			pShmSessionInfo->tShmemMetadataMediaInfo.bFrameGenerated = TRUE;
			stRTPRTCPSessionParam.ptShmemMediaInfo = &pShmSessionInfo->tShmemMetadataMediaInfo;
#endif
			//stRTPRTCPSessionParam.iCodecIndex = pRTSPStreaming->stMulticast[i].iSDPIndex;
			printf("[%s]: Metadata of stream type %d for scalable multicast start\n", __FUNCTION__, pRTSPStreaming->stMulticast[iMulticastCount].iSDPIndex);
			stRTPRTCPSessionParam.iCodecIndex = pRTSPStreaming->fMetadataCallBack((DWORD) pRTSPStreaming->hParentMetadataHandle,
														MEDIA_CALLBACK_CHECK_CODEC_INDEX, 
														(void*)pRTSPStreaming->stMulticast[iMulticastCount].iSDPIndex);

#ifdef _SHARED_MEM
			//20101018 Add by danny for support multiple channel text on video
			if( pRTSPStreaming->stMulticast[iMulticastCount].iSDPIndex <= LIVE_STREAM_NUM)
			{
				stRTPRTCPSessionParam.iMultipleChannelChannelIndex = iTrackIndex + 1;
			}
#endif
			//20130327 modified by Jimmy to support different video/audio multicast addresses for ONVIF Test Tool 12.12
			//20170727 multicast 
			stRTPRTCPSessionParam.iSdpIndex = pRTSPStreaming->stMulticast[iMulticastCount].iSDPIndex;
			RTPRTCPChannel_AddMulticastSession(pRTSPStreaming->hRTPRTCPChannelMetadataHandle, &stRTPRTCPSessionParam, iMulticastCount + 1);
			//20111205 Modified by danny For UDP mode socket leak
			//20100714 Add by danny For Multicast parameters load dynamically
			printf("[%s]:Add Metadata multicast channel ID:%ul Socket:%d\n", __FUNCTION__, stRTPRTCPSessionParam.dwSessionID, *stRTPRTCPSessionParam.psktRTP);
			pRTSPStreaming->stMulticast[iMulticastCount].iAlreadyMulticastMetadata= 1;
		}
#endif

	}
    
    return S_OK;
}

SCODE RTSPStreaming_StartAlwaysMulticast(HANDLE hRTSPStreaming, int iMulticastIndex)
{
   	RTSPSTREAMING *pRTSPStreaming;

	if(!hRTSPStreaming)
		return S_FAIL;
	
	pRTSPStreaming=(RTSPSTREAMING *)hRTSPStreaming;

	//20130327 modified by Jimmy to support different video/audio multicast addresses for ONVIF Test Tool 12.12
	TelnetShell_DbgPrint("[%s]: Video of Streaming Type %d should start !!!\r\n", __FUNCTION__, pRTSPStreaming->stMulticast[iMulticastIndex-1].iSDPIndex);
	pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle,0,ccctStreamTypeVideoStart, pRTSPStreaming->stMulticast[iMulticastIndex-1].iSDPIndex);

	TelnetShell_DbgPrint("[%s]: Audio of Streaming Type %d should start !!!\r\n", __FUNCTION__, pRTSPStreaming->stMulticast[iMulticastIndex-1].iSDPIndex);
	pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle,0,ccctStreamTypeAudioStart, pRTSPStreaming->stMulticast[iMulticastIndex-1].iSDPIndex);

	//20120806 added by Jimmy for metadata
	TelnetShell_DbgPrint("[%s]: Metadata of Streaming Type %d should start !!!\r\n", __FUNCTION__, pRTSPStreaming->stMulticast[iMulticastIndex-1].iSDPIndex);
	pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle,0,ccctStreamTypeMetadataStart, pRTSPStreaming->stMulticast[iMulticastIndex-1].iSDPIndex);

	if( RTSPStreaming_AddMulticast(hRTSPStreaming, iMulticastIndex-1) != S_OK )
	{
		return S_FAIL;
	}
	
    return S_OK;
}

int RTSPStreaming_RemoveSpecificStreamMulticastClients(HANDLE hRTSPStreaming, int iMulticastIndex)
{
	RTSPSTREAMING	*pRTSPStreaming;
	int				i = 0;

	if(!hRTSPStreaming)
		return -1;

	pRTSPStreaming=(RTSPSTREAMING *)hRTSPStreaming;

	if( pRTSPStreaming->stMulticast[iMulticastIndex-1].iAlreadyMulticastAudio )
	{
		RTPRTCPChannel_RemoveMulticastSession(pRTSPStreaming->hRTPRTCPChannelAudioHandle,iMulticastIndex);
		pRTSPStreaming->stMulticast[iMulticastIndex-1].iAlreadyMulticastAudio = 0;
		printf("Stop audio multicast\r\n");
		
		if( pRTSPStreaming->stMulticast[iMulticastIndex-1].iAlwaysMulticast && 
			pRTSPStreaming->stMulticast[iMulticastIndex-1].iAlwaysMulticastAudio )
		{
			//20130327 modified by Jimmy to support different video/audio multicast addresses for ONVIF Test Tool 12.12
			TelnetShell_DbgPrint("[%s]: Audio of Streaming Type %d should stop !!!\r\n", __FUNCTION__, pRTSPStreaming->stMulticast[iMulticastIndex-1].iSDPIndex);
			pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle,0,ccctStreamTypeAudioStop, pRTSPStreaming->stMulticast[iMulticastIndex-1].iSDPIndex);
		}
	}
	if( pRTSPStreaming->stMulticast[iMulticastIndex-1].iAlreadyMulticastVideo )
	{
		RTPRTCPChannel_RemoveMulticastSession(pRTSPStreaming->hRTPRTCPChannelVideoHandle,iMulticastIndex);
		pRTSPStreaming->stMulticast[iMulticastIndex-1].iAlreadyMulticastVideo = 0;
		printf("Stop video multicast\r\n");

		if( pRTSPStreaming->stMulticast[iMulticastIndex-1].iAlwaysMulticast && 
			pRTSPStreaming->stMulticast[iMulticastIndex-1].iAlwaysMulticastVideo )
		{
			//20130327 modified by Jimmy to support different video/audio multicast addresses for ONVIF Test Tool 12.12
			TelnetShell_DbgPrint("[%s]: Video of Streaming Type %d should stop !!!\r\n", __FUNCTION__, pRTSPStreaming->stMulticast[iMulticastIndex-1].iSDPIndex);
			pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle,0,ccctStreamTypeVideoStop, pRTSPStreaming->stMulticast[iMulticastIndex-1].iSDPIndex);
		}
	}
	//20120806 added by Jimmy for metadata
	if( pRTSPStreaming->stMulticast[iMulticastIndex-1].iAlreadyMulticastMetadata)
	{
		RTPRTCPChannel_RemoveMulticastSession(pRTSPStreaming->hRTPRTCPChannelMetadataHandle,iMulticastIndex);
		pRTSPStreaming->stMulticast[iMulticastIndex-1].iAlreadyMulticastMetadata= 0;
		printf("Stop metadata multicast\r\n");

		if( pRTSPStreaming->stMulticast[iMulticastIndex-1].iAlwaysMulticast && 
			pRTSPStreaming->stMulticast[iMulticastIndex-1].iAlwaysMulticastMetadata )
		{
			//20130327 modified by Jimmy to support different video/audio multicast addresses for ONVIF Test Tool 12.12
			TelnetShell_DbgPrint("[%s]: Metadata of Streaming Type %d should stop !!!\r\n", __FUNCTION__, pRTSPStreaming->stMulticast[iMulticastIndex-1].iSDPIndex);
			pRTSPStreaming->fControlCallBack((DWORD)pRTSPStreaming->hParentControlHandle,0,ccctStreamTypeMetadataStop, pRTSPStreaming->stMulticast[iMulticastIndex-1].iSDPIndex);
		}
	}

	for( i = 0; i < MAX_CONNECT_NUM ; i++)
	{
		if( pRTSPStreaming->pstSessionList[i].dwSessionID != 0 &&  pRTSPStreaming->pstSessionList[i].iMulticast == iMulticastIndex)
		{
			RTSPStreaming_RemoveSession(hRTSPStreaming, pRTSPStreaming->pstSessionList[i].dwSessionID);
		}
	}

	return 0;
}
#endif

int RTSPStreaming_SetVideoParameters(HANDLE hRTSPStreaming,int iCodecIndex, TRTSPSTREAMING_VIDENCODING_PARAM *pstVideoEncodingParameter,DWORD dwFlag)
{
	RTSPSTREAMING *pRTSPStreaming;
	RTPRTCPCHANNEL_PARAM stRTPRTCPChannelParam;

	if(!hRTSPStreaming)
		return -1;
	
	if( iCodecIndex < 1 || iCodecIndex > MULTIPLE_STREAM_NUM )
		return -1;

	pRTSPStreaming=(RTSPSTREAMING *)hRTSPStreaming;
	
    OSSemaphore_Get(pRTSPStreaming->hMediaParamSemaphore,INFINITE);

	strncpy(pRTSPStreaming->tVideoEncodeParam[iCodecIndex-1].acTrackName,pstVideoEncodingParameter->acTrackName,RTSPSTREAMING_TRACK_NAME_LEN-1);
	//20140605 added by Charles for ONVIF track token
	if(pRTSPStreaming->tVideoEncodeParam[iCodecIndex-1].acTrackToken[0] == '\0')
    {
        memset(pRTSPStreaming->tVideoEncodeParam[iCodecIndex-1].acTrackToken, 0, RTSPSTREAMING_TRACK_NAME_LEN);
	    strncpy(pRTSPStreaming->tVideoEncodeParam[iCodecIndex-1].acTrackToken,pstVideoEncodingParameter->acTrackName,RTSPSTREAMING_TRACK_NAME_LEN-1);
    }   

	if( dwFlag & RTSPSTREAMING_VIDEO_CODECTYPE)
	{
		//20090819 Change of codec type should result in kicking clients
		if(pRTSPStreaming->tVideoEncodeParam[iCodecIndex-1].eVideoCodecType != pstVideoEncodingParameter->eVideoCodecType)
		{
			pRTSPStreaming->tVideoEncodeParam[iCodecIndex-1].eVideoCodecType = pstVideoEncodingParameter->eVideoCodecType;
			printf("Removing clients for stream %d due to codec change!\n", iCodecIndex);
			//20130315 added by Jimmy to log more information
			syslog(LOG_DEBUG, "Removing clients for stream %d due to codec change!\n", iCodecIndex);
			RTSPStreaming_RemoveSpecificStreamClients(hRTSPStreaming, iCodecIndex);
		}
		//20090723 Update codec type for always multicast to avoid incorrect codec type at start up
		if((pRTSPStreaming->stMulticast[iCodecIndex-1].iAlwaysMulticastVideo == 1) && (pRTSPStreaming->stMulticast[iCodecIndex-1].iAlwaysMulticast == 1))
		{
			RTPRTCPCOMPOSER_PARAM  stRTPRTCPComposerParam;

			memset(&stRTPRTCPComposerParam, 0, sizeof(RTPRTCPCOMPOSER_PARAM));
			if(pRTSPStreaming->tVideoEncodeParam[pRTSPStreaming->stMulticast[iCodecIndex-1].iSDPIndex-1].eVideoCodecType == mctMP4V)
			{
				stRTPRTCPComposerParam.iMediaType=RTSPSTREAMING_MPEG4_MEDIATYPE;
			}
			else if(pRTSPStreaming->tVideoEncodeParam[pRTSPStreaming->stMulticast[iCodecIndex-1].iSDPIndex-1].eVideoCodecType == mctJPEG)
			{
				stRTPRTCPComposerParam.iMediaType=RTSPSTREAMING_JPEG_MEDIATYPE;
			}
			else if(pRTSPStreaming->tVideoEncodeParam[pRTSPStreaming->stMulticast[iCodecIndex-1].iSDPIndex-1].eVideoCodecType == mctH264)
			{
				stRTPRTCPComposerParam.iMediaType=RTSPSTREAMING_H264_MEDIATYPE;
			}
            else if(pRTSPStreaming->tVideoEncodeParam[pRTSPStreaming->stMulticast[iCodecIndex-1].iSDPIndex-1].eVideoCodecType == mctH265)
			{
				stRTPRTCPComposerParam.iMediaType=RTSPSTREAMING_H265_MEDIATYPE;
			}
				
			RTPRTCPComposer_SetCodecType(pRTSPStreaming->stMulticast[iCodecIndex-1].hRTPRTCPVideoComposerHandle, &stRTPRTCPComposerParam);
			//printf("Updated Always multicast %d with codectype %d\n", iCodecIndex, stRTPRTCPComposerParam.iMediaType);
		}
	}

	if( dwFlag & RTSPSTREAMING_VIDEO_WIDTH)
		pRTSPStreaming->tVideoEncodeParam[iCodecIndex-1].iWidth = pstVideoEncodingParameter->iWidth;
	    
    if( dwFlag & RTSPSTREAMING_VIDEO_HEIGHT)	    
		pRTSPStreaming->tVideoEncodeParam[iCodecIndex-1].iHeight = pstVideoEncodingParameter->iHeight;
	    
    if( dwFlag & RTSPSTREAMING_VIDEO_DECODEBUFF )	    
		pRTSPStreaming->tVideoEncodeParam[iCodecIndex-1].iDecoderBufferSize = pstVideoEncodingParameter->iDecoderBufferSize;
	    
	if( dwFlag & RTSPSTREAMING_VIDEO_BITRATE )
		pRTSPStreaming->tVideoEncodeParam[iCodecIndex-1].iBitRate=pstVideoEncodingParameter->iBitRate;

	//20120116 Modify by danny for Update video clockrate for always multicast to avoid incorrect video clockrate at start up
    if( dwFlag & RTSPSTREAMING_VIDEO_CLOCKRATE )
    {
		pRTSPStreaming->tVideoEncodeParam[iCodecIndex-1].iClockRate=pstVideoEncodingParameter->iClockRate;
		
		if((pRTSPStreaming->stMulticast[iCodecIndex-1].iAlwaysMulticastVideo == 1) && (pRTSPStreaming->stMulticast[iCodecIndex-1].iAlwaysMulticast == 1))
		{
			RTPRTCPCOMPOSER_PARAM  stRTPRTCPComposerParam;

			memset(&stRTPRTCPComposerParam, 0, sizeof(RTPRTCPCOMPOSER_PARAM));
			stRTPRTCPComposerParam.iSampleFrequency = pRTSPStreaming->tVideoEncodeParam[iCodecIndex-1].iClockRate;

			RTPRTCPComposer_SetVideoClockRate(pRTSPStreaming->stMulticast[iCodecIndex-1].hRTPRTCPVideoComposerHandle, &stRTPRTCPComposerParam);
			printf("Updated Always multicast %d with video clockrate %d\n", iCodecIndex, stRTPRTCPComposerParam.iSampleFrequency);
		}
    }
	
    if( dwFlag & RTSPSTREAMING_VIDEO_MPEG4CI )	    
    {
    	//20111209 Modify by danny for support Detect Tampering Watermark
        if( pstVideoEncodingParameter->iMPEG4HeaderLen < MAX_MP4V_HEADER_SIZE &&		//CID:247, CHECKER:OVERRUN_STATIC
            pstVideoEncodingParameter->iMPEG4HeaderLen > 0 )
        {
			pRTSPStreaming->tVideoEncodeParam[iCodecIndex-1].iMPEG4HeaderLen = pstVideoEncodingParameter->iMPEG4HeaderLen;	
			memcpy(pRTSPStreaming->tVideoEncodeParam[iCodecIndex-1].acMPEG4Header,pstVideoEncodingParameter->acMPEG4Header,pstVideoEncodingParameter->iMPEG4HeaderLen);
	    }    
	    else
	    {
			DbgLog((dfCONSOLE|dfINTERNAL,"[RTSPStreaming]: Set video parameter failed length=%d \n",pRTSPStreaming->tVideoEncodeParam[iCodecIndex-1].iMPEG4HeaderLen));               	    		
            OSSemaphore_Post(pRTSPStreaming->hMediaParamSemaphore);
		    return -1;    
	    }	    
	}    
	
	if( dwFlag & RTSPSTREAMING_VIDEO_PROLEVE )
	{
		if(pstVideoEncodingParameter->eVideoCodecType == mctH264)
		{
			rtspstrcpy(pRTSPStreaming->tVideoEncodeParam[iCodecIndex-1].acProfileLevelID, pstVideoEncodingParameter->acProfileLevelID, sizeof(pRTSPStreaming->tVideoEncodeParam[iCodecIndex-1].acProfileLevelID));
			//Set packetization mode as well
			pRTSPStreaming->tVideoEncodeParam[iCodecIndex-1].iPacketizationMode = pstVideoEncodingParameter->iPacketizationMode;
		}
		else
		{
			pRTSPStreaming->tVideoEncodeParam[iCodecIndex-1].iProfileLevel = pstVideoEncodingParameter->iProfileLevel;
		}
	}

	if( dwFlag & RTSPSTREAMING_VIDEO_H264SPROP)
	{
		rtspstrcpy(pRTSPStreaming->tVideoEncodeParam[iCodecIndex-1].acSpropParamSet, pstVideoEncodingParameter->acSpropParamSet, sizeof(pRTSPStreaming->tVideoEncodeParam[iCodecIndex-1].acSpropParamSet));
	}

    if( dwFlag & RTSPSTREAMING_VIDEO_H265SPROP)
    {
        rtspstrcpy(pRTSPStreaming->tVideoEncodeParam[iCodecIndex-1].acSpropVPSParam, pstVideoEncodingParameter->acSpropVPSParam, sizeof(pRTSPStreaming->tVideoEncodeParam[iCodecIndex-1].acSpropVPSParam));
        rtspstrcpy(pRTSPStreaming->tVideoEncodeParam[iCodecIndex-1].acSpropSPSParam, pstVideoEncodingParameter->acSpropSPSParam, sizeof(pRTSPStreaming->tVideoEncodeParam[iCodecIndex-1].acSpropSPSParam));
        rtspstrcpy(pRTSPStreaming->tVideoEncodeParam[iCodecIndex-1].acSpropPPSParam, pstVideoEncodingParameter->acSpropPPSParam, sizeof(pRTSPStreaming->tVideoEncodeParam[iCodecIndex-1].acSpropPPSParam));
    }
	
	if( dwFlag & RTSPSTREAMING_VIDEO_SET_CI )
	{
		printf("Video CI is set correctly!\n");
		pRTSPStreaming->tVideoEncodeParam[iCodecIndex-1].iCIReady = 1;
	}
		
	stRTPRTCPChannelParam.iRTPRTCPMediaType=RTPRTCPCHANNEL_MEDIATYPE_VIDEO;
	//stRTPRTCPChannelParam.pbyMPEG4StartBitStream=(BYTE*)pRTSPStreaming->tVideoEncodeParam[iCodecIndex-1].acMPEG4Header;
	//stRTPRTCPChannelParam.iMPEG4StartBitStreamLength=pRTSPStreaming->tVideoEncodeParam[iCodecIndex-1].iMPEG4HeaderLen;
	
    OSSemaphore_Post(pRTSPStreaming->hMediaParamSemaphore);

	RTPRTCPChannel_SetParameters( pRTSPStreaming->hRTPRTCPChannelVideoHandle, &stRTPRTCPChannelParam);
		
	return 0;	
}	

int RTSPStreaming_SetAudioParameters(HANDLE hRTSPStreaming,int iCodecIndex ,TRTSPSTREAMING_AUDENCODING_PARAM *pstAudioEncodingParameter,DWORD dwFlag)
{
	RTSPSTREAMING *pRTSPStreaming;

	if(!hRTSPStreaming)
		return -1;

	if( iCodecIndex < 1 ||  iCodecIndex > MULTIPLE_STREAM_NUM )
		return -1;
	
	pRTSPStreaming=(RTSPSTREAMING *)hRTSPStreaming;
	
    OSSemaphore_Get(pRTSPStreaming->hMediaParamSemaphore,INFINITE);

	strncpy(pRTSPStreaming->tAudioEncodeParam[iCodecIndex-1].acTrackName,pstAudioEncodingParameter->acTrackName,RTSPSTREAMING_TRACK_NAME_LEN-1);
	//20140605 added by Charles for ONVIF track token
	if(pRTSPStreaming->tAudioEncodeParam[iCodecIndex-1].acTrackToken[0] == '\0')
    {
        memset(pRTSPStreaming->tAudioEncodeParam[iCodecIndex-1].acTrackToken, 0, RTSPSTREAMING_TRACK_NAME_LEN);
	    strncpy(pRTSPStreaming->tAudioEncodeParam[iCodecIndex-1].acTrackToken,pstAudioEncodingParameter->acTrackName,RTSPSTREAMING_TRACK_NAME_LEN-1);
    }   

	if( dwFlag & RTSPSTREAMING_AUDIO_CODECTYPE )
	{
		pRTSPStreaming->tAudioEncodeParam[iCodecIndex-1].iAudioCodecType=pstAudioEncodingParameter->iAudioCodecType;

		//20130322 added by Jimmy to fix wrong G711 RTP payload type with always multicast
		if((pRTSPStreaming->stMulticast[iCodecIndex-1].iAlreadyMulticastAudio == 1) && (pRTSPStreaming->stMulticast[iCodecIndex-1].iAlwaysMulticast == 1))
		{
			RTPRTCPCOMPOSER_PARAM  stRTPRTCPComposerParam;
		
			memset(&stRTPRTCPComposerParam, 0, sizeof(RTPRTCPCOMPOSER_PARAM));
#ifdef _G711_AUDIOIN
			if (pRTSPStreaming->tAudioEncodeParam[iCodecIndex-1].iAudioCodecType == ractG711u)
			{
				stRTPRTCPComposerParam.iMediaType = 0 ;
			}
			else if (pRTSPStreaming->tAudioEncodeParam[iCodecIndex-1].iAudioCodecType == ractG711a)
			{
				stRTPRTCPComposerParam.iMediaType = 8 ;
			}
			else
#endif
			stRTPRTCPComposerParam.iMediaType = RTSPSTREAMING_AMR_MEDIATYPE;

			RTPRTCPComposer_SetAudioCodecType(pRTSPStreaming->stMulticast[iCodecIndex-1].hRTPRTCPAudioComposerHandle, &stRTPRTCPComposerParam);
			//20130327 added by Jimmy to support different video/audio multicast addresses for ONVIF Test Tool 12.12
			RTPRTCPComposer_SetAudioCodecType(pRTSPStreaming->stMulticast[iCodecIndex-1].hRTPRTCPAudioComposerHandle, &stRTPRTCPComposerParam);
			printf("Updated Always multicast %d with audio codec type %d\n", iCodecIndex, stRTPRTCPComposerParam.iMediaType);
		}

	}
	    
    if( dwFlag & RTSPSTREAMING_AUDIO_BITRATE )	    
		pRTSPStreaming->tAudioEncodeParam[iCodecIndex-1].iBitRate=pstAudioEncodingParameter->iBitRate;

    if(	dwFlag & RTSPSTREAMING_AUDIO_CLOCKRATE )    
		pRTSPStreaming->tAudioEncodeParam[iCodecIndex-1].iClockRate=pstAudioEncodingParameter->iClockRate;
    
    if( dwFlag & RTSPSTREAMING_AUDIO_PACKETTIME )
		pRTSPStreaming->tAudioEncodeParam[iCodecIndex-1].iPacketTime= pstAudioEncodingParameter->iPacketTime;
        
    if( dwFlag & RTSPSTREAMING_AUDIO_OCTECTALIGN )   
		pRTSPStreaming->tAudioEncodeParam[iCodecIndex-1].iOctetAlign= pstAudioEncodingParameter->iOctetAlign;

    if( dwFlag & RTSPSTREAMING_AUDIO_AMRCRC )       
		pRTSPStreaming->tAudioEncodeParam[iCodecIndex-1].iAMRcrc = pstAudioEncodingParameter->iAMRcrc;
        
    if( dwFlag & RTSPSTREAMING_AUDIO_ROBUSTSORT )   
		pRTSPStreaming->tAudioEncodeParam[iCodecIndex-1].iRobustSorting=pstAudioEncodingParameter->iRobustSorting;
        
	if( dwFlag & RTSPSTREAMING_AUDIO_SET_CI )
		pRTSPStreaming->tAudioEncodeParam[iCodecIndex-1].iCIReady = 1;

	if( dwFlag & RTSPSTREAMING_AUDIO_PACKINGMODE )	
		pRTSPStreaming->tAudioEncodeParam[iCodecIndex-1].bIsBigEndian = pstAudioEncodingParameter->bIsBigEndian;

	pRTSPStreaming->tAudioEncodeParam[iCodecIndex-1].iM4ASpecConfLen = pstAudioEncodingParameter->iM4ASpecConfLen;	

	if( pstAudioEncodingParameter->iM4ASpecConfLen < 16 && pstAudioEncodingParameter->iM4ASpecConfLen >0 )
	{
		memcpy(pRTSPStreaming->tAudioEncodeParam[iCodecIndex-1].acM4ASpecConf, 
		pstAudioEncodingParameter->acM4ASpecConf,
		pstAudioEncodingParameter->iM4ASpecConfLen);
	}
	else
	{
		OSSemaphore_Post(pRTSPStreaming->hMediaParamSemaphore);
		return -1;
	}

	pRTSPStreaming->tAudioEncodeParam[iCodecIndex-1].iM4AProfileLevel = pstAudioEncodingParameter->iM4AProfileLevel;
	pRTSPStreaming->tAudioEncodeParam[iCodecIndex-1].iChanNum= pstAudioEncodingParameter->iChanNum;	

	if ((pRTSPStreaming->tAudioEncodeParam[iCodecIndex-1].iAudioCodecType == ractAAC4) &&
		(pRTSPStreaming->tAudioEncodeParam[iCodecIndex-1].iM4ASpecConfLen <= 0))
	{
		DbgLog((dfCONSOLE|dfINTERNAL,"[RTSPStreaming]: Set M4A parameter failed length=%d \n",pRTSPStreaming->tAudioEncodeParam[iCodecIndex-1].iM4ASpecConfLen ));   
	    OSSemaphore_Post(pRTSPStreaming->hMediaParamSemaphore);
		return -1;
	}	
    OSSemaphore_Post(pRTSPStreaming->hMediaParamSemaphore);
        
	return 0;	
}
//20120801 added by Jimmy for metadata
int RTSPStreaming_SetMetadataParameters(HANDLE hRTSPStreaming, int iCodecIndex, TRTSPSTREAMING_METADATAENCODING_PARAM *pstMetadataEncodingParameter, DWORD dwFlag)
{
	RTSPSTREAMING *pRTSPStreaming;

	if(!hRTSPStreaming)
		return -1;

	if( iCodecIndex < 1 ||  iCodecIndex > MULTIPLE_STREAM_NUM )
		return -1;
	
	pRTSPStreaming=(RTSPSTREAMING *)hRTSPStreaming;
	
    OSSemaphore_Get(pRTSPStreaming->hMediaParamSemaphore,INFINITE);

	strncpy(pRTSPStreaming->tMetadataEncodeParam[iCodecIndex-1].acTrackName,pstMetadataEncodingParameter->acTrackName,RTSPSTREAMING_TRACK_NAME_LEN-1);
	//20140605 added by Charles for ONVIF track token
	if(pRTSPStreaming->tMetadataEncodeParam[iCodecIndex-1].acTrackToken[0] == '\0')
    {
        memset(pRTSPStreaming->tMetadataEncodeParam[iCodecIndex-1].acTrackToken, 0, RTSPSTREAMING_TRACK_NAME_LEN);
	    strncpy(pRTSPStreaming->tMetadataEncodeParam[iCodecIndex-1].acTrackToken,pstMetadataEncodingParameter->acTrackName,RTSPSTREAMING_TRACK_NAME_LEN-1);
    }   

	if( dwFlag & RTSPSTREAMING_METADATA_CODECTYPE )
		pRTSPStreaming->tMetadataEncodeParam[iCodecIndex-1].iMetadataCodecType=pstMetadataEncodingParameter->iMetadataCodecType;
	    
    if( dwFlag & RTSPSTREAMING_METADATA_BITRATE )	    
		pRTSPStreaming->tMetadataEncodeParam[iCodecIndex-1].iBitRate=pstMetadataEncodingParameter->iBitRate;

    if(	dwFlag & RTSPSTREAMING_METADATA_CLOCKRATE )    
		pRTSPStreaming->tMetadataEncodeParam[iCodecIndex-1].iClockRate=pstMetadataEncodingParameter->iClockRate;
    
    if( dwFlag & RTSPSTREAMING_METADATA_PACKETTIME )
		pRTSPStreaming->tMetadataEncodeParam[iCodecIndex-1].iPacketTime= pstMetadataEncodingParameter->iPacketTime;
        
    if( dwFlag & RTSPSTREAMING_METADATA_OCTECTALIGN )   
		pRTSPStreaming->tMetadataEncodeParam[iCodecIndex-1].iOctetAlign= pstMetadataEncodingParameter->iOctetAlign;

    if( dwFlag & RTSPSTREAMING_METADATA_AMRCRC )       
		pRTSPStreaming->tMetadataEncodeParam[iCodecIndex-1].iAMRcrc = pstMetadataEncodingParameter->iAMRcrc;
        
    if( dwFlag & RTSPSTREAMING_METADATA_ROBUSTSORT )   
		pRTSPStreaming->tMetadataEncodeParam[iCodecIndex-1].iRobustSorting=pstMetadataEncodingParameter->iRobustSorting;
        
	if( dwFlag & RTSPSTREAMING_METADATA_SET_CI )
		pRTSPStreaming->tMetadataEncodeParam[iCodecIndex-1].iCIReady = 1;

	pRTSPStreaming->tMetadataEncodeParam[iCodecIndex-1].iM4ASpecConfLen = pstMetadataEncodingParameter->iM4ASpecConfLen;	

	if( pstMetadataEncodingParameter->iM4ASpecConfLen < 16 && pstMetadataEncodingParameter->iM4ASpecConfLen >0 )
	{
		memcpy(pRTSPStreaming->tMetadataEncodeParam[iCodecIndex-1].acM4ASpecConf, 
		pstMetadataEncodingParameter->acM4ASpecConf,
		pstMetadataEncodingParameter->iM4ASpecConfLen);
	}
	else
	{
		OSSemaphore_Post(pRTSPStreaming->hMediaParamSemaphore);
		return -1;
	}

	pRTSPStreaming->tMetadataEncodeParam[iCodecIndex-1].iM4AProfileLevel = pstMetadataEncodingParameter->iM4AProfileLevel;
	pRTSPStreaming->tMetadataEncodeParam[iCodecIndex-1].iChanNum= pstMetadataEncodingParameter->iChanNum;	

	if ((pRTSPStreaming->tMetadataEncodeParam[iCodecIndex-1].iMetadataCodecType == ractAAC4) &&
		(pRTSPStreaming->tMetadataEncodeParam[iCodecIndex-1].iM4ASpecConfLen <= 0))
	{
		DbgLog((dfCONSOLE|dfINTERNAL,"[RTSPStreaming]: Set M4A parameter failed length=%d \n",pRTSPStreaming->tMetadataEncodeParam[iCodecIndex-1].iM4ASpecConfLen ));   
	    OSSemaphore_Post(pRTSPStreaming->hMediaParamSemaphore);
		return -1;
	}	
    OSSemaphore_Post(pRTSPStreaming->hMediaParamSemaphore);
        
	return 0;	
}

//20120925 added by Jimmy for ONVIF backchannel
int RTSPStreaming_SetAudiobackParameters(HANDLE hRTSPStreaming,int iChannelIndex ,TRTSPSTREAMING_AUDDECODING_PARAM *pstAudioDecodingParameter)
{
	RTSPSTREAMING *pRTSPStreaming;

	if(!hRTSPStreaming)
		return -1;

	if( iChannelIndex < 1 ||  iChannelIndex > MULTIPLE_CHANNEL_NUM)
		return -1;
	
	pRTSPStreaming=(RTSPSTREAMING *)hRTSPStreaming;

	strncpy(pRTSPStreaming->tAudioDecodeParam[iChannelIndex-1].acTrackName,pstAudioDecodingParameter->acTrackName,RTSPSTREAMING_TRACK_NAME_LEN-1);
        
	return 0;	
}

//20140605 added by Charles for ONVIF track token
int RTSPStreaming_SetMODTrackTokenParameters(HANDLE hRTSPStreaming,int iCodecIndex, char *pcTrackToken)
{
	
	RTSPSTREAMING *pRTSPStreaming;
	char *pc = NULL;
	char *pcTemp = NULL;
	int	iStreamingMode;
	DWORD dwTrackTokenLen;
	
	if(!hRTSPStreaming)
		return -1;

	if( iCodecIndex <= LIVE_STREAM_NUM ||	iCodecIndex > MULTIPLE_STREAM_NUM )
		return -1;
	
	pRTSPStreaming = (RTSPSTREAMING *)hRTSPStreaming;
	
	OSSemaphore_Get(pRTSPStreaming->hMediaParamSemaphore,INFINITE);
	iStreamingMode = pRTSPStreaming->iRTSPStreamingMediaType[iCodecIndex-1];

	if(iStreamingMode & RTSPSTREAMING_MEDIATYPE_VIDEO)
	{		
		memset(pRTSPStreaming->tVideoEncodeParam[iCodecIndex-1].acTrackToken, 0, RTSPSTREAMING_TRACK_NAME_LEN);
		
		if(*pcTrackToken == '\0')
		{
			strncpy(pRTSPStreaming->tVideoEncodeParam[iCodecIndex-1].acTrackToken, pRTSPStreaming->tVideoEncodeParam[iCodecIndex-1].acTrackName, RTSPSTREAMING_TRACK_NAME_LEN-1);
		}
		else if((pc = strchr(pcTrackToken, 'v')) == NULL)
		{
			OSSemaphore_Post(pRTSPStreaming->hMediaParamSemaphore);
			return -1;
		}
		else
		{
			for( pcTemp = pc; *pcTemp != ':' && *pcTemp != '\0'; pcTemp++);
			dwTrackTokenLen = ((pcTemp-pc) < RTSPSTREAMING_TRACK_NAME_LEN) ? (pcTemp-pc) : (RTSPSTREAMING_TRACK_NAME_LEN-1);
			strncpy(pRTSPStreaming->tVideoEncodeParam[iCodecIndex-1].acTrackToken, pc, dwTrackTokenLen);
		}
		printf("[%s]Video TrackToken=%s\n", __FUNCTION__, pRTSPStreaming->tVideoEncodeParam[iCodecIndex-1].acTrackToken);
	}
	
	if(iStreamingMode & RTSPSTREAMING_MEDIATYPE_AUDIO)
	{		
		memset(pRTSPStreaming->tAudioEncodeParam[iCodecIndex-1].acTrackToken, 0, RTSPSTREAMING_TRACK_NAME_LEN);
		
		if(*pcTrackToken == '\0')
		{
			strncpy(pRTSPStreaming->tAudioEncodeParam[iCodecIndex-1].acTrackToken, pRTSPStreaming->tAudioEncodeParam[iCodecIndex-1].acTrackName, RTSPSTREAMING_TRACK_NAME_LEN-1);
		}
		else if((pc = strchr(pcTrackToken, 'a')) == NULL)
		{
			OSSemaphore_Post(pRTSPStreaming->hMediaParamSemaphore);
			return -1;
		}
		else
		{
			for( pcTemp = pc; *pcTemp != ':' && *pcTemp != '\0'; pcTemp++);
			dwTrackTokenLen = ((pcTemp-pc) < RTSPSTREAMING_TRACK_NAME_LEN) ? (pcTemp-pc) : (RTSPSTREAMING_TRACK_NAME_LEN-1);
			strncpy(pRTSPStreaming->tAudioEncodeParam[iCodecIndex-1].acTrackToken, pc, dwTrackTokenLen);
		}
		printf("[%s]Audio TrackToken=%s\n", __FUNCTION__, pRTSPStreaming->tAudioEncodeParam[iCodecIndex-1].acTrackToken);
	}
	
	if(iStreamingMode & RTSPSTREAMING_MEDIATYPE_METADATA)
	{		
		memset(pRTSPStreaming->tMetadataEncodeParam[iCodecIndex-1].acTrackToken, 0, RTSPSTREAMING_TRACK_NAME_LEN);
		
		if(*pcTrackToken == '\0')
		{
			strncpy(pRTSPStreaming->tMetadataEncodeParam[iCodecIndex-1].acTrackToken, pRTSPStreaming->tMetadataEncodeParam[iCodecIndex-1].acTrackName, RTSPSTREAMING_TRACK_NAME_LEN-1);
		}
		else if((pc = strchr(pcTrackToken, 'm')) == NULL)
		{
			OSSemaphore_Post(pRTSPStreaming->hMediaParamSemaphore);
			return -1;
		}
		else
		{
			for( pcTemp = pc; *pcTemp != ':' && *pcTemp != '\0'; pcTemp++);
			dwTrackTokenLen = ((pcTemp-pc) < RTSPSTREAMING_TRACK_NAME_LEN) ? (pcTemp-pc) : (RTSPSTREAMING_TRACK_NAME_LEN-1);
			strncpy(pRTSPStreaming->tMetadataEncodeParam[iCodecIndex-1].acTrackToken, pc, dwTrackTokenLen);
		}
		printf("[%s]Metadata TrackToken=%s\n", __FUNCTION__, pRTSPStreaming->tMetadataEncodeParam[iCodecIndex-1].acTrackToken);
	}
	OSSemaphore_Post(pRTSPStreaming->hMediaParamSemaphore);
			
	return 0;
}


int RTSPStreaming_AddAccessList(HANDLE hRTSPStreaming, unsigned long ulStartIP, unsigned long ulEndIP) // in network order
{
	RTSPSTREAMING *pRTSPStreaming;
	int iResult;

	if(!hRTSPStreaming)
		return -1;
	
	pRTSPStreaming=(RTSPSTREAMING *)hRTSPStreaming;
	
	if(!pRTSPStreaming->hIPAccessCheckHandle)
		return -1;	

	iResult=IPAccessCheck_AddAccessList(pRTSPStreaming->hIPAccessCheckHandle,ulStartIP,ulEndIP);
	return iResult;		
}


int RTSPStreaming_RemoveAccessList(HANDLE hRTSPStreaming, unsigned long ulStartIP, unsigned long ulEndIP) // in network order
{
	RTSPSTREAMING *pRTSPStreaming;
	int iResult;

	if(!hRTSPStreaming)
		return -1;
	
	pRTSPStreaming=(RTSPSTREAMING *)hRTSPStreaming;
	
	if(!pRTSPStreaming->hIPAccessCheckHandle)
		return -1;	

	iResult=IPAccessCheck_RemoveAccessList(pRTSPStreaming->hIPAccessCheckHandle,ulStartIP,ulEndIP);
		
	return iResult;			
}

// add by Yun, 2005/06/19
int RTSPStreaming_ClearAccessList(HANDLE hRTSPStreaming)
{
	RTSPSTREAMING *pRTSPStreaming;
	int iResult;

	if(!hRTSPStreaming)
		return -1;
	
	pRTSPStreaming=(RTSPSTREAMING *)hRTSPStreaming;
	
	if(!pRTSPStreaming->hIPAccessCheckHandle)
		return -1;	

	iResult=IPAccessCheck_ClearAccessList(pRTSPStreaming->hIPAccessCheckHandle);
		
	return iResult;			
}
	
int RTSPStreaming_AddDenyList(HANDLE hRTSPStreaming, unsigned long ulStartIP, unsigned long ulEndIP) // in network order
{
	RTSPSTREAMING *pRTSPStreaming;
	int iResult;

	if(!hRTSPStreaming)
		return -1;
	
	pRTSPStreaming=(RTSPSTREAMING *)hRTSPStreaming;
	
	if(!pRTSPStreaming->hIPAccessCheckHandle)
		return -1;	

	iResult=IPAccessCheck_AddDenyList(pRTSPStreaming->hIPAccessCheckHandle,ulStartIP,ulEndIP);
	
	return iResult;			
}

	
int RTSPStreaming_RemoveDenyList(HANDLE hRTSPStreaming, unsigned long ulStartIP, unsigned long ulEndIP) // in network order
{
	RTSPSTREAMING *pRTSPStreaming;
//	VIDENCODING_PARAM stVideoEncodingParam;
	int iResult;

	if(!hRTSPStreaming)
		return -1;
	
	pRTSPStreaming=(RTSPSTREAMING *)hRTSPStreaming;
	
	if(!pRTSPStreaming->hIPAccessCheckHandle)
		return -1;	

	iResult=IPAccessCheck_RemoveDenyList(pRTSPStreaming->hIPAccessCheckHandle,ulStartIP,ulEndIP);
		
	return iResult;			
}	


int RTSPStreaming_RemoveSession(HANDLE hRTSPStreaming, DWORD dwSessionID)
{
	RTSPSTREAMING *pRTSPStreaming;
//	int iResult;
	//HANDLE	hTemp;
	int	i;
	//RTPMEDIABUFFER	*pMediaBuffer;

#ifdef RTSPRTP_MULTICAST
	//20100720 Added by danny to fix Backchannel multicast session terminated, rtsp server has not stopped sending video/audio RTP/RTCP
	int iMulticastIndex = 0, iMulticastCurrentSessionNumber = 0;
#endif

	if(!hRTSPStreaming)
		return -1;
	
	pRTSPStreaming=(RTSPSTREAMING *)hRTSPStreaming;
	
    OSSemaphore_Get(pRTSPStreaming->ulSessionListSemaphore,INFINITE);
			            
	if(pRTSPStreaming->iSessionListNumber == 0 )
	{           
		OSSemaphore_Post(pRTSPStreaming->ulSessionListSemaphore);
		return -1;			
	}           
	else        
	{           
		for(i=0; i<pRTSPStreaming->iSessionListNumber; i++)
		{       
			if( pRTSPStreaming->pstSessionList[i].dwSessionID == dwSessionID )
			{   
				//20120806 modified by Jimmy for metadata
				if( pRTSPStreaming->pstSessionList[i].byMediaStatus & RTSPSTREAMING_MEDIATYPE_AUDIO)
				{
					RTPRTCPChannel_RemoveOneSession(pRTSPStreaming->hRTPRTCPChannelAudioHandle, dwSessionID);
#ifdef RTSPRTP_MULTICAST
					//20100720 Added by danny to fix Backchannel multicast session terminated, rtsp server has not stopped sending video/audio RTP/RTCP
					if( pRTSPStreaming->pstSessionList[i].iMulticast )
					{
						iMulticastIndex = pRTSPStreaming->pstSessionList[i].iMulticast;
						iMulticastCurrentSessionNumber = RTSPStreaming_GetMulticastCurrentSessionNumber(hRTSPStreaming, iMulticastIndex);
						printf("iMulticast %d CurrentSessionNumber %d\r\n", iMulticastIndex, iMulticastCurrentSessionNumber);
					}
					if( iMulticastIndex && iMulticastCurrentSessionNumber == 1 && 
						pRTSPStreaming->stMulticast[iMulticastIndex-1].iAlreadyMulticastAudio &&
						pRTSPStreaming->stMulticast[iMulticastIndex-1].iAlwaysMulticast == 0)
					{
						RTPRTCPChannel_RemoveMulticastSession(pRTSPStreaming->hRTPRTCPChannelAudioHandle,iMulticastIndex);
						pRTSPStreaming->stMulticast[iMulticastIndex-1].iAlreadyMulticastAudio = 0;
						printf("Stop audio multicast\r\n");
					}
#endif
					//20081113 Louis fix session removal
					//20120806 modified by Jimmy for metadata
					RTSPStreaming_RemoveSessionListBySessionID(pRTSPStreaming, dwSessionID, RTSPSTREAMING_MEDIATYPE_AUDIO);
				}
				//20120806 modified by Jimmy for metadata
				if( pRTSPStreaming->pstSessionList[i].byMediaStatus & RTSPSTREAMING_MEDIATYPE_VIDEO)
				{
					RTPRTCPChannel_RemoveOneSession(pRTSPStreaming->hRTPRTCPChannelVideoHandle, dwSessionID);
#ifdef RTSPRTP_MULTICAST
					//20100720 Added by danny to fix Backchannel multicast session terminated, rtsp server has not stopped sending video/audio RTP/RTCP
					if( pRTSPStreaming->pstSessionList[i].iMulticast )
					{
						iMulticastIndex = pRTSPStreaming->pstSessionList[i].iMulticast;
						iMulticastCurrentSessionNumber = RTSPStreaming_GetMulticastCurrentSessionNumber(hRTSPStreaming, iMulticastIndex);
						printf("iMulticast %d CurrentSessionNumber %d\r\n", iMulticastIndex, iMulticastCurrentSessionNumber);
					}
					if( iMulticastIndex && iMulticastCurrentSessionNumber == 1 &&
						pRTSPStreaming->stMulticast[iMulticastIndex-1].iAlreadyMulticastVideo &&
						pRTSPStreaming->stMulticast[iMulticastIndex-1].iAlwaysMulticast == 0)
					{
						RTPRTCPChannel_RemoveMulticastSession(pRTSPStreaming->hRTPRTCPChannelVideoHandle,iMulticastIndex);
						pRTSPStreaming->stMulticast[iMulticastIndex-1].iAlreadyMulticastVideo = 0;
						printf("Stop video multicast\r\n");
					}
#endif
					//20081113 Louis fix session removal
					//20120806 modified by Jimmy for metadata
					RTSPStreaming_RemoveSessionListBySessionID(pRTSPStreaming, dwSessionID, RTSPSTREAMING_MEDIATYPE_VIDEO);
				}

				//20120806 added by Jimmy for metadata
				if( pRTSPStreaming->pstSessionList[i].byMediaStatus & RTSPSTREAMING_MEDIATYPE_METADATA)
				{
					RTPRTCPChannel_RemoveOneSession(pRTSPStreaming->hRTPRTCPChannelMetadataHandle, dwSessionID);
#ifdef RTSPRTP_MULTICAST
					//20100720 Added by danny to fix Backchannel multicast session terminated, rtsp server has not stopped sending video/audio RTP/RTCP
					if( pRTSPStreaming->pstSessionList[i].iMulticast )
					{
						iMulticastIndex = pRTSPStreaming->pstSessionList[i].iMulticast;
						iMulticastCurrentSessionNumber = RTSPStreaming_GetMulticastCurrentSessionNumber(hRTSPStreaming, iMulticastIndex);
						printf("iMulticast %d CurrentSessionNumber %d\r\n", iMulticastIndex, iMulticastCurrentSessionNumber);
					}
					if( iMulticastIndex && iMulticastCurrentSessionNumber == 1 &&
						pRTSPStreaming->stMulticast[iMulticastIndex-1].iAlreadyMulticastMetadata &&
						pRTSPStreaming->stMulticast[iMulticastIndex-1].iAlwaysMulticast == 0)
					{
						RTPRTCPChannel_RemoveMulticastSession(pRTSPStreaming->hRTPRTCPChannelMetadataHandle,iMulticastIndex);
						pRTSPStreaming->stMulticast[iMulticastIndex-1].iAlreadyMulticastMetadata= 0;
						printf("Stop metadata multicast\r\n");
					}
#endif
					//20081113 Louis fix session removal
					RTSPStreaming_RemoveSessionListBySessionID(pRTSPStreaming, dwSessionID, RTSPSTREAMING_MEDIATYPE_METADATA);
				}

				printf("========Remove one session from API, %d remains=======\n",pRTSPStreaming->iSessionListNumber);

				RTSPServer_RemoveSessionFromAPI(pRTSPStreaming->hRTSPServerHandle,dwSessionID);
#ifdef _SIP
				SIPUA_RemoveDialog(pRTSPStreaming->hSIPUAHandle, dwSessionID);
#endif
				OSSemaphore_Post(pRTSPStreaming->ulSessionListSemaphore);
				return 0;
			}	
		}                          		        
	}           
	
	OSSemaphore_Post(pRTSPStreaming->ulSessionListSemaphore);
                      
	return -1;		
	
}	

#ifdef _SHARED_MEM
//20101208 Modified by danny For GUID format change
//20100105 Added For Seamless Recording
int RTSPStreaming_RemoveGUIDSessions(HANDLE hRTSPStreaming, char* pcSessionGUID)
{
	RTSPSTREAMING *pRTSPStreaming;
	int	i, iRet;

	if(!hRTSPStreaming)
		return -1;
	
	pRTSPStreaming=(RTSPSTREAMING *)hRTSPStreaming;

	printf("Removing sessions due to GUID %s remove\n", pcSessionGUID);
	//20130315 added by Jimmy to log more information
	syslog(LOG_DEBUG, "Removing sessions due to GUID %s remove\n", pcSessionGUID);
	for( i = 0; i < MAX_CONNECT_NUM ; i++ )
	{
		iRet = strncmp(pcSessionGUID, pRTSPStreaming->pstSessionList[i].acSeamlessRecordingGUID, RTSPS_Seamless_Recording_GUID_LENGTH);
		if( (pRTSPStreaming->pstSessionList[i].dwSessionID != 0) && (iRet == 0) )
		{
			if( RTSPStreaming_RemoveSession(hRTSPStreaming, pRTSPStreaming->pstSessionList[i].dwSessionID) != 0 )
			{
				printf("[%s]Error removing session with ID %u\n", __FUNCTION__, pRTSPStreaming->pstSessionList[i].dwSessionID);
			}
		}
	}

	if( RTSPServer_ResetGUID(pRTSPStreaming->hRTSPServerHandle, pcSessionGUID) != 0 )
	{
		printf("[%s]Error reset GUID %s\n", __FUNCTION__, pcSessionGUID);
		return -1;
	}

	return 0;
}
#endif

//20101018 Add by danny for support multiple channel text on video
int RTSPStreaming_SendLocation(HANDLE hRTSPStreaming, char* pcLocation, int iMultipleChannelChannelIndex)
{
   	RTSPSTREAMING *pRTSPStreaming;

	if(!hRTSPStreaming)
		return -1;
	
	pRTSPStreaming=(RTSPSTREAMING *)hRTSPStreaming;
#ifdef _SHARED_MEM
	//20120806 modified by Jimmy for metadata
	if( ( RTPRTCPChannel_SetLocation(pRTSPStreaming->hRTPRTCPChannelVideoHandle, pcLocation, iMultipleChannelChannelIndex) != S_OK ) ||
		( RTPRTCPChannel_SetLocation(pRTSPStreaming->hRTPRTCPChannelAudioHandle, pcLocation, iMultipleChannelChannelIndex) != S_OK ) ||
		( RTPRTCPChannel_SetLocation(pRTSPStreaming->hRTPRTCPChannelMetadataHandle, pcLocation, iMultipleChannelChannelIndex) != S_OK ))
        return -1;
    else
        return 0;
#else
	//20120806 modified by Jimmy for metadata
	if( ( RTPPacketizer_SetLocation(pRTSPStreaming->hVideoPacketizer, pcLocation) != S_OK ) ||
		( RTPPacketizer_SetLocation(pRTSPStreaming->hAudioPacketizer, pcLocation) != S_OK ) ||
		( RTPPacketizer_SetLocation(pRTSPStreaming->hMetadataPacketizer, pcLocation) != S_OK ))
        return -1;
    else
        return 0;
#endif

}

int RTSPStreaming_SetMediaStreamMode(HANDLE hRTSPStreaming,DWORD dwMediaStreamMode, int iCodecIndex)
{
   	RTSPSTREAMING *pRTSPStreaming;

	if(!hRTSPStreaming)
		return -1;
	
	pRTSPStreaming=(RTSPSTREAMING *)hRTSPStreaming;
	
	if( iCodecIndex < 1 || iCodecIndex > MULTIPLE_STREAM_NUM)
		return -2;

	//20120806 modified by Jimmy for metadata
	pRTSPStreaming->iRTSPStreamingMediaType[iCodecIndex-1] = dwMediaStreamMode;
	/*
	if( dwMediaStreamMode == RTSPSTREAMING_MEDIATYPE_AUDIOVIDEO )
		pRTSPStreaming->iRTSPStreamingMediaType[iCodecIndex-1] = RTSPSTREAMING_MEDIATYPE_AUDIOVIDEO;

	if( dwMediaStreamMode == RTSPSTREAMING_MEDIATYPE_AUDIOONLY )
		pRTSPStreaming->iRTSPStreamingMediaType[iCodecIndex-1] = RTSPSTREAMING_MEDIATYPE_AUDIOONLY;

	if( dwMediaStreamMode == RTSPSTREAMING_MEDIATYPE_VIDEOONLY )
		pRTSPStreaming->iRTSPStreamingMediaType[iCodecIndex-1] = RTSPSTREAMING_MEDIATYPE_VIDEOONLY;
	*/

    return 0;
}


void RTSPStreaming_RemoveRTPOverHTTPInfo(RTSPSTREAMING *pRTSPStreaming,int iIndex, int iCloseSocketFlag)
{
	if( iCloseSocketFlag == TRUE)
	{
		if( pRTSPStreaming->pstRTPOverHTTPInfo[iIndex].iRecvSock > 0 )
		{
			TelnetShell_DbgPrint("-------HTTP socket %d closed------------\r\n",pRTSPStreaming->pstRTPOverHTTPInfo[iIndex].iRecvSock);
			closesocket(pRTSPStreaming->pstRTPOverHTTPInfo[iIndex].iRecvSock);
		}

		if( pRTSPStreaming->pstRTPOverHTTPInfo[iIndex].iSendSock > 0 )
		{
			TelnetShell_DbgPrint("-------HTTP socket %d closed------------\r\n",pRTSPStreaming->pstRTPOverHTTPInfo[iIndex].iSendSock);
			closesocket(pRTSPStreaming->pstRTPOverHTTPInfo[iIndex].iSendSock);
		}
	}

    if( pRTSPStreaming->iRTPOverHTTPNumber > 1 )
    {
        memcpy((void*)(pRTSPStreaming->pstRTPOverHTTPInfo + iIndex )
	              ,(void*)(pRTSPStreaming->pstRTPOverHTTPInfo + pRTSPStreaming->iRTPOverHTTPNumber -1)
	              ,sizeof(TRTP_OVER_HTTPINFO));     
    }    
        
    memset((void*)(pRTSPStreaming->pstRTPOverHTTPInfo + pRTSPStreaming->iRTPOverHTTPNumber -1),0,sizeof(TRTP_OVER_HTTPINFO));                	                          
	pRTSPStreaming->pstRTPOverHTTPInfo[pRTSPStreaming->iRTPOverHTTPNumber-1].iRecvSock = -1;
	pRTSPStreaming->pstRTPOverHTTPInfo[pRTSPStreaming->iRTPOverHTTPNumber-1].iSendSock = -1;

    pRTSPStreaming->iRTPOverHTTPNumber --;                	                                                  
	TelnetShell_DbgPrint("-------HTTP socket number %d----------\r\n",pRTSPStreaming->iRTPOverHTTPNumber);

}

int RTSPStreaming_CheckIdleHTTPSocket(RTSPSTREAMING *pRTSPStreaming)
{
    int i;
    
    for( i=0 ; i< pRTSPStreaming->iMaximumSessionCount; i++)
    {
		if( pRTSPStreaming->pstRTPOverHTTPInfo[i].acSessionCookie[0] != 0 )
			pRTSPStreaming->pstRTPOverHTTPInfo[i].iCheckCount ++;

	    if( pRTSPStreaming->pstRTPOverHTTPInfo[i].iCheckCount > 2 )	        
	    {	
		    TelnetShell_DbgPrint("[RTSP Streaming]: GET idle socket removing\r\n");
            RTSPStreaming_RemoveRTPOverHTTPInfo(pRTSPStreaming,i,TRUE);                      
		}    
	}
	return 0;
}

int RTSPStreaming_AddRTPOverHTTPSock(HANDLE hRTSPStreaming,TStreamServer_ConnectionSettings *ptConnectionSetting)
{
   	RTSPSTREAMING	*pRTSPStreaming;
	THTTPCONNINFO	tHTTPConnInfo;
   	int i,iResult=-1;
   	struct sockaddr_storage peersock;		//20090413 for IPV4&V6
	struct sockaddr_in6	*ptv6addr =  NULL;
	struct sockaddr_in	*ptv4addr =  NULL;

	int socklen = sizeof(peersock);

	if(!hRTSPStreaming || !ptConnectionSetting || ptConnectionSetting->sckControl < 0)
		return -1;
	
	pRTSPStreaming=(RTSPSTREAMING *)hRTSPStreaming;

    //ShengFu bug fixed for no filtering if HTTP mode
	memset(&peersock, 0, sizeof(peersock));
	getpeername( ptConnectionSetting->sckControl, (struct sockaddr * ) & peersock, (unsigned int *)&socklen) ;
	if(peersock.ss_family == AF_INET6)
	{
		ptv6addr = (struct sockaddr_in6 *)&peersock;
		if (IN6_IS_ADDR_V4MAPPED(&ptv6addr->sin6_addr))
		{
			struct sockaddr_in	v4mapaddr;
			struct in6_addr in6addr;
				
			in6addr = ptv6addr->sin6_addr;
			memset(&v4mapaddr, 0, sizeof(v4mapaddr));

			printf( "Someone connecting from HTTP IPv4 %d.%d.%d.%d\n",
				in6addr.s6_addr[12],
				in6addr.s6_addr[13],
				in6addr.s6_addr[14],
				in6addr.s6_addr[15]);
			v4mapaddr.sin_addr.s_addr =  MAKEFOURCC(in6addr.s6_addr[12], in6addr.s6_addr[13], in6addr.s6_addr[14], in6addr.s6_addr[15]);
			
			iResult=IPAccessCheck_CheckIP(pRTSPStreaming->hIPAccessCheckHandle, v4mapaddr.sin_addr.s_addr);
		}
		else
		{
			char szPresentString[64]="";
			printf( "Someone connecting from HTTP IPV6 %s\n", 
				inet_ntop(AF_INET6, ptv6addr->sin6_addr.s6_addr, szPresentString, sizeof(szPresentString)));
			
			//Bypass IPAccessCheck for IPv6 for now
			iResult = 0;
		}
	}
	else if(peersock.ss_family == AF_INET)
	{
		ptv4addr = (struct sockaddr_in *)&peersock;
		iResult=IPAccessCheck_CheckIP(pRTSPStreaming->hIPAccessCheckHandle, ptv4addr->sin_addr.s_addr);
	}

	if( iResult != 0 )
	{
		closesocket(ptConnectionSetting->sckControl);
        RTSPStreaming_CheckIdleHTTPSocket(pRTSPStreaming);
		return -8;
	}

	memset((void*)&tHTTPConnInfo,0,sizeof(THTTPCONNINFO));

	//20140418 added by Charles for http port support rtsp describe command
	if(ptConnectionSetting->iHTTPMethod == SS_HTTPMOTHOD_DESCRIBE)
	{
		tHTTPConnInfo.iRecvSock = ptConnectionSetting->sckControl;
		tHTTPConnInfo.iSendSock = ptConnectionSetting->sckControl;
		if( ptConnectionSetting->dwRecvLength != 0 )
		{
			if(ptConnectionSetting->dwRecvLength > 0 &&
			   ptConnectionSetting->dwRecvLength < RTSP_HTTP_MESSAGE_LEN )
			{
			   memcpy(tHTTPConnInfo.acMessageBuffer, ptConnectionSetting->pcRecvBuffer, ptConnectionSetting->dwRecvLength);
			}
			else
			{
				TelnetShell_DbgPrint("[RTSP Streaming]: message too large in HTTP socket\r\n");
				closesocket(ptConnectionSetting->sckControl);
				return -1;
			}
		}
			
        iResult = RTSPServer_AddRTPOverHTTPSock(pRTSPStreaming->hRTSPServerHandle, &tHTTPConnInfo, RTSP_HTTP_ADD_DESCRIBE);
		if(iResult < 0)
		{
			closesocket(ptConnectionSetting->sckControl);
		}
		return iResult;
	}
		
	TelnetShell_DbgPrint("-------HTTP socket %d add into RTSP, session cookie:%s------------\r\n",ptConnectionSetting->sckControl,ptConnectionSetting->pszSessionCookie);
	//scan for exsiting session-cookie for pair socket
    if( pRTSPStreaming->iRTPOverHTTPNumber > 0 )
	{	    
    	for( i=0 ; i < pRTSPStreaming->iRTPOverHTTPNumber ; i++)
	    {
	       // pRTSPStreaming->pstRTPOverHTTPInfo[i].iCheckCount ++;
	        if( strcmp(pRTSPStreaming->pstRTPOverHTTPInfo[i].acSessionCookie,ptConnectionSetting->pszSessionCookie) == 0 )	    
	        {
                if( pRTSPStreaming->pstRTPOverHTTPInfo[i].iSendSock >= 0 
                    && pRTSPStreaming->pstRTPOverHTTPInfo[i].iRecvSock < 0 
                    && ptConnectionSetting->iHTTPMethod == SS_HTTPMOTHOD_POST )        
                {
					tHTTPConnInfo.iRecvSock = pRTSPStreaming->pstRTPOverHTTPInfo[i].iRecvSock = ptConnectionSetting->sckControl;
					tHTTPConnInfo.iSendSock = pRTSPStreaming->pstRTPOverHTTPInfo[i].iSendSock;
					//CID:63, CHECKER:BUFFER_SIZE_WARNING
					rtspstrcpy(tHTTPConnInfo.acSessionCookie,pRTSPStreaming->pstRTPOverHTTPInfo[i].acSessionCookie,RTSP_HTTP_COOKIE_LEN);

					if( ptConnectionSetting->dwRecvLength != 0 )
					{
						if(ptConnectionSetting->dwRecvLength > 0 &&
						   ptConnectionSetting->dwRecvLength < RTSP_HTTP_MESSAGE_LEN )
						{
						   memcpy(tHTTPConnInfo.acMessageBuffer,ptConnectionSetting->pcRecvBuffer,ptConnectionSetting->dwRecvLength);
						}
						else
						{
							TelnetShell_DbgPrint("[RTSP Streaming]: message too large in HTTP POST socket\r\n");
							RTSPStreaming_RemoveRTPOverHTTPInfo(pRTSPStreaming,i, TRUE);
							return -2;
						}
					}
						
	                iResult = RTSPServer_AddRTPOverHTTPSock(pRTSPStreaming->hRTSPServerHandle,&tHTTPConnInfo,RTSP_HTTP_ADD_PAIR);
#ifdef _LINUX
				//syslog(LOG_ERR, "[RTSPstreaming: RTP over HTTP added %d %d",tHTTPConnInfo.iSendSock,tHTTPConnInfo.iRecvSock);
#endif //_LINUX	                                                
                }    
                else if( pRTSPStreaming->pstRTPOverHTTPInfo[i].iSendSock < 0 
                    && pRTSPStreaming->pstRTPOverHTTPInfo[i].iRecvSock >= 0 
                    && ptConnectionSetting->iHTTPMethod == SS_HTTPMOTHOD_GET )        
                {
					tHTTPConnInfo.iRecvSock = pRTSPStreaming->pstRTPOverHTTPInfo[i].iRecvSock;
					tHTTPConnInfo.iSendSock = pRTSPStreaming->pstRTPOverHTTPInfo[i].iSendSock = ptConnectionSetting->sckControl;
					rtspstrcpy(tHTTPConnInfo.acSessionCookie,pRTSPStreaming->pstRTPOverHTTPInfo[i].acSessionCookie,RTSP_HTTP_COOKIE_LEN);

   	                iResult = RTSPServer_AddRTPOverHTTPSock(pRTSPStreaming->hRTSPServerHandle,&tHTTPConnInfo,RTSP_HTTP_ADD_PAIR);
//	                                                  ,ptConnectionSetting->sckControl
//	                                                  ,pRTSPStreaming->pstRTPOverHTTPInfo[i].iRecvSock);	              
	                                                  
#ifdef _LINUX
    				//syslog(LOG_ERR, "[RTSPstreaming: RTP over HTTP added %d %d",tHTTPConnInfo.iSendSock,tHTTPConnInfo.iRecvSock);
#endif //_LINUX	                                                
                }
                else
                    iResult = -2;	        
                      
                if( iResult == 0 )
					RTSPStreaming_RemoveRTPOverHTTPInfo(pRTSPStreaming,i,FALSE);                      
				else
					RTSPStreaming_RemoveRTPOverHTTPInfo(pRTSPStreaming,i,TRUE);                      
				
#ifdef _LINUX
    			//syslog(LOG_ERR, "[RTSPstreaming: Current unfinished RTPoverHTTP %d",pRTSPStreaming->iRTPOverHTTPNumber);
#endif //_LINUX	
                return iResult;
                
	        }// end of if session cookie matched 	    
	    
	        /*if( pRTSPStreaming->pstRTPOverHTTPInfo[i].iCheckCount > pRTSPStreaming->iMaximumSessionCount )	        
			{
				TelnetShell_DbgPrint("[RTSP Streaming]: GET idle socket removing\r\n");
                RTSPStreaming_RemoveRTPOverHTTPInfo(pRTSPStreaming,i,TRUE);                      
			}*/
        }// end of for loop check
	}
	
	iResult = -1;
	// if no match of session-cookie, add a new one into list
	if( pRTSPStreaming->iRTPOverHTTPNumber < pRTSPStreaming->iMaximumSessionCount )  
	{       
		if( ptConnectionSetting->iHTTPMethod == SS_HTTPMOTHOD_GET)
        {	        
			if( strlen(ptConnectionSetting->pszSessionCookie) < RTSP_HTTP_COOKIE_LEN )
			{             
				pRTSPStreaming->iRTPOverHTTPNumber ++; 
				rtspstrcpy(pRTSPStreaming->pstRTPOverHTTPInfo[pRTSPStreaming->iRTPOverHTTPNumber-1].acSessionCookie
						,ptConnectionSetting->pszSessionCookie, sizeof(pRTSPStreaming->pstRTPOverHTTPInfo[pRTSPStreaming->iRTPOverHTTPNumber-1].acSessionCookie));     

		        pRTSPStreaming->pstRTPOverHTTPInfo[pRTSPStreaming->iRTPOverHTTPNumber-1].iRecvSock = -1;
			    pRTSPStreaming->pstRTPOverHTTPInfo[pRTSPStreaming->iRTPOverHTTPNumber-1].iSendSock = ptConnectionSetting->sckControl; 
				iResult = 0;
			}          
			else
				iResult = -4;
        }
        else if( ptConnectionSetting->iHTTPMethod == SS_HTTPMOTHOD_POST)
        {
			//iResult = -8;
			TelnetShell_DbgPrint("!!!!!------POST before GET %d----------!!!!!!!!!\r\n",ptConnectionSetting->sckControl);

			// This is a behavior of QT player only
			// During streaming, QT will periodically send out POST with OPTION
			// Each time QT will disconnect TCP and reconnect for keeping alive
            //pRTSPStreaming->pstRTPOverHTTPInfo[pRTSPStreaming->iRTPOverHTTPNumber-1].iRecvSock = ptConnectionSetting->sckControl;
            //pRTSPStreaming->pstRTPOverHTTPInfo[pRTSPStreaming->iRTPOverHTTPNumber-1].iSendSock = -1;            
			
			tHTTPConnInfo.iRecvSock = ptConnectionSetting->sckControl;
			tHTTPConnInfo.iSendSock = -1;
			rtspstrcpy(tHTTPConnInfo.acSessionCookie,ptConnectionSetting->pszSessionCookie,RTSP_HTTP_COOKIE_LEN);

			if( ptConnectionSetting->dwRecvLength != 0 )
			{
				if(ptConnectionSetting->dwRecvLength > 0 &&
				   ptConnectionSetting->dwRecvLength < RTSP_HTTP_MESSAGE_LEN )
				{
				   memcpy(tHTTPConnInfo.acMessageBuffer,ptConnectionSetting->pcRecvBuffer,ptConnectionSetting->dwRecvLength);
				}
				else
				{
					closesocket(ptConnectionSetting->sckControl);
					return -2;
				}
			}

            iResult = RTSPServer_AddRTPOverHTTPSock(pRTSPStreaming->hRTSPServerHandle,&tHTTPConnInfo,RTSP_HTTP_ADD_SINGLE);
        }
		else
			iResult = -6;            
	}
	else
		iResult = -7;
      	
	if( iResult < 0 )
	{
		closesocket(ptConnectionSetting->sckControl);
	}

	RTSPStreaming_CheckIdleHTTPSocket(pRTSPStreaming);

	return iResult;
   
}

/*void RTSPStreaming_RemoveRTPOverHTTPInfo(RTSPSTREAMING *pRTSPStreaming,int iIndex)
{
    if( pRTSPStreaming->iRTPOverHTTPNumber > 1 )
    {
        memcpy((void*)(pRTSPStreaming->pstRTPOverHTTPInfo + iIndex )
	              ,(void*)(pRTSPStreaming->pstRTPOverHTTPInfo + pRTSPStreaming->iRTPOverHTTPNumber -1)
	              ,sizeof(TRTP_OVER_HTTPINFO));     
    }    
        
    memset((void*)(pRTSPStreaming->pstRTPOverHTTPInfo + pRTSPStreaming->iRTPOverHTTPNumber -1),0,sizeof(TRTP_OVER_HTTPINFO));                	                          
    pRTSPStreaming->iRTPOverHTTPNumber --;                	                                                  
}


int RTSPStreaming_AddRTPOverHTTPSock(HANDLE hRTSPStreaming,TStreamServer_ConnectionSettings *ptConnectionSetting)
{
   	RTSPSTREAMING	*pRTSPStreaming;
	THTTPCONNINFO	tHTTPConnInfo;
   	int i,iResult=-1;

	if(!hRTSPStreaming || !ptConnectionSetting || ptConnectionSetting->sckControl < 0)
		return -1;
	
	pRTSPStreaming=(RTSPSTREAMING *)hRTSPStreaming;

	memset((void*)&tHTTPConnInfo,0,sizeof(THTTPCONNINFO));

    if( pRTSPStreaming->iRTPOverHTTPNumber > 0 )
	{	    
    	for( i=0 ; i < pRTSPStreaming->iRTPOverHTTPNumber ; i++)
	    {
	        pRTSPStreaming->pstRTPOverHTTPInfo[i].iCheckCount ++;
	        if( strcmp(pRTSPStreaming->pstRTPOverHTTPInfo[i].acSessionCookie,ptConnectionSetting->pszSessionCookie) == 0 )	    
	        {
                if( pRTSPStreaming->pstRTPOverHTTPInfo[i].iSendSock >= 0 
                    && pRTSPStreaming->pstRTPOverHTTPInfo[i].iRecvSock < 0 
                    && ptConnectionSetting->iHTTPMethod == SS_HTTPMOTHOD_POST )        
                {
					tHTTPConnInfo.iRecvSock = ptConnectionSetting->sckControl;
					tHTTPConnInfo.iSendSock = pRTSPStreaming->pstRTPOverHTTPInfo[i].iSendSock;
					strncpy(tHTTPConnInfo.acSessionCookie,pRTSPStreaming->pstRTPOverHTTPInfo[i].acSessionCookie,RTPSTREAMING_SESSIONCOOKIE_SIZE);

					if( ptConnectionSetting->dwRecvLength != 0 )
					{
						if(ptConnectionSetting->dwRecvLength > 0 &&
						   ptConnectionSetting->dwRecvLength < RTSP_HTTP_MESSAGE_LEN )
						{
						   memcpy(tHTTPConnInfo.acMessageBuffer,ptConnectionSetting->pcRecvBuffer,ptConnectionSetting->dwRecvLength);
						}
						else
						{
							return -2;
						}
					}
						
	                iResult = RTSPServer_AddRTPOverHTTPSock(pRTSPStreaming->hRTSPServerHandle,&tHTTPConnInfo,RTSP_HTTP_ADD_PAIR);
//	                                                ,pRTSPStreaming->pstRTPOverHTTPInfo[i].iSendSock
//	                                                ,ptConnectionSetting->sckControl);	                
#ifdef _LINUX
				syslog(LOG_ERR, "[RTSPstreaming: RTP over HTTP added %d %d",tHTTPConnInfo.iSendSock,tHTTPConnInfo.iRecvSock);
#endif //_LINUX	                                                
                }    
                else if( pRTSPStreaming->pstRTPOverHTTPInfo[i].iSendSock < 0 
                    && pRTSPStreaming->pstRTPOverHTTPInfo[i].iRecvSock >= 0 
                    && ptConnectionSetting->iHTTPMethod == SS_HTTPMOTHOD_GET )        
                {
					tHTTPConnInfo.iRecvSock = pRTSPStreaming->pstRTPOverHTTPInfo[i].iRecvSock;
					tHTTPConnInfo.iSendSock = ptConnectionSetting->sckControl;
					strncpy(tHTTPConnInfo.acSessionCookie,pRTSPStreaming->pstRTPOverHTTPInfo[i].acSessionCookie,RTPSTREAMING_SESSIONCOOKIE_SIZE);

   	                iResult = RTSPServer_AddRTPOverHTTPSock(pRTSPStreaming->hRTSPServerHandle,&tHTTPConnInfo,RTSP_HTTP_ADD_PAIR);
//	                                                  ,ptConnectionSetting->sckControl
//	                                                  ,pRTSPStreaming->pstRTPOverHTTPInfo[i].iRecvSock);	              
	                                                  
#ifdef _LINUX
    				syslog(LOG_ERR, "[RTSPstreaming: RTP over HTTP added %d %d",tHTTPConnInfo.iSendSock,tHTTPConnInfo.iRecvSock);
#endif //_LINUX	                                                
                }
                else
                    iResult = -2;	        
                      
                RTSPStreaming_RemoveRTPOverHTTPInfo(pRTSPStreaming,i);                      
#ifdef _LINUX
    			syslog(LOG_ERR, "[RTSPstreaming: Current unfinished RTPoverHTTP %d",pRTSPStreaming->iRTPOverHTTPNumber);
#endif //_LINUX	
                return iResult;
                
	        }// end of if session cookie matched 	    
	    
	        if( pRTSPStreaming->pstRTPOverHTTPInfo[i].iCheckCount > pRTSPStreaming->iMaximumSessionCount )	        
                RTSPStreaming_RemoveRTPOverHTTPInfo(pRTSPStreaming,i);                      
        }// end of for loop check
	}
		
	if( pRTSPStreaming->iRTPOverHTTPNumber >= pRTSPStreaming->iMaximumSessionCount )
	    return -3;
	else    
	{
		if( ptConnectionSetting->iHTTPMethod == SS_HTTPMOTHOD_GET) //quick time behavior
	        pRTSPStreaming->iRTPOverHTTPNumber ++; 

        if( strlen(ptConnectionSetting->pszSessionCookie) < RTPSTREAMING_SESSIONCOOKIE_SIZE )
        {             
            strcpy(pRTSPStreaming->pstRTPOverHTTPInfo[pRTSPStreaming->iRTPOverHTTPNumber-1].acSessionCookie
                   ,ptConnectionSetting->pszSessionCookie);     
        }          
        else
            return -4;
        
        if( ptConnectionSetting->iHTTPMethod == SS_HTTPMOTHOD_POST)
        {
            pRTSPStreaming->pstRTPOverHTTPInfo[pRTSPStreaming->iRTPOverHTTPNumber-1].iRecvSock = ptConnectionSetting->sckControl;
            pRTSPStreaming->pstRTPOverHTTPInfo[pRTSPStreaming->iRTPOverHTTPNumber-1].iSendSock = -1;            
			// behavior of quicktime player only 
			tHTTPConnInfo.iRecvSock = ptConnectionSetting->sckControl;
			tHTTPConnInfo.iSendSock = -1;
			strncpy(tHTTPConnInfo.acSessionCookie,ptConnectionSetting->pszSessionCookie,RTPSTREAMING_SESSIONCOOKIE_SIZE);

			if( ptConnectionSetting->dwRecvLength != 0 )
			{
				if(ptConnectionSetting->dwRecvLength > 0 &&
				   ptConnectionSetting->dwRecvLength < RTSP_HTTP_MESSAGE_LEN )
				{
				   memcpy(tHTTPConnInfo.acMessageBuffer,ptConnectionSetting->pcRecvBuffer,ptConnectionSetting->dwRecvLength);
				}
				else
				{
					return -2;
				}
			}

            return RTSPServer_AddRTPOverHTTPSock(pRTSPStreaming->hRTSPServerHandle,&tHTTPConnInfo,RTSP_HTTP_ADD_SINGLE);

        }
        else if( ptConnectionSetting->iHTTPMethod == SS_HTTPMOTHOD_GET)
        {
            pRTSPStreaming->pstRTPOverHTTPInfo[pRTSPStreaming->iRTPOverHTTPNumber-1].iRecvSock = -1;
            pRTSPStreaming->pstRTPOverHTTPInfo[pRTSPStreaming->iRTPOverHTTPNumber-1].iSendSock = ptConnectionSetting->sckControl;            
        }
        else
            return -5;
            
	}
      	   
    return 0;
    
}*/

int RTSPStreaming_GetVersion(BYTE *byMajor, BYTE *byMinor, BYTE *byBuild, BYTE *byRevision)
{
	*byMajor = (BYTE)(RTSPSTREAMINGSERVER_VERSION & 0x000000FF);
	*byMinor = (BYTE)((RTSPSTREAMINGSERVER_VERSION & 0x0000FF00) >> 8);
	*byBuild = (BYTE)((RTSPSTREAMINGSERVER_VERSION & 0x00FF0000) >> 16);
	*byRevision = (BYTE)((RTSPSTREAMINGSERVER_VERSION & 0xFF000000) >> 24);
	
	return S_OK;
}

/*
//  This function has not implemeted and test
int RTSPStreaming_GetSessionInformation(HANDLE hRTSPStreaming, RTSPSTREAMING_SESSION ** ppstSession, int * piSessionCount)
{
	RTSPSTREAMING *pRTSPStreaming;
	unsigned long aulMsg[4];

	
//	return -1;
	
	if(!hRTSPStreaming)
		return -1;
	
	pRTSPStreaming=(RTSPSTREAMING *)hRTSPStreaming;
	

	return 0;	
}	
*/

// Add by Kate, 2005/08/26
HANDLE RTSPStreaming_GetIPAccessHandle(HANDLE hRTSPStreaming)
{
   	RTSPSTREAMING *pRTSPStreaming;

	if(!hRTSPStreaming)
		return NULL;
	
	pRTSPStreaming=(RTSPSTREAMING *)hRTSPStreaming;	
	
	return pRTSPStreaming->hIPAccessCheckHandle;
}

// Add by ShengFu, 2005/10/13
int RTSPStreaming_GetCurrentSessionNumber(HANDLE hRTSPStreaming)
{
   	RTSPSTREAMING *pRTSPStreaming;

	if(!hRTSPStreaming)
		return -1;
	
	pRTSPStreaming=(RTSPSTREAMING *)hRTSPStreaming;	
	
	return RTSPServer_GetCurrentSessionNumber(pRTSPStreaming->hRTSPServerHandle);

}

int RTSPStreaming_GetRTSPSessionInfo(HANDLE hRTSPStreaming,char* pSessionInfoBuf, int iLength)
{
   	RTSPSTREAMING   *pRTSPStreaming;
   	int             i;
    
	if(!hRTSPStreaming)
		return -1;
		
	pRTSPStreaming=(RTSPSTREAMING *)hRTSPStreaming;	

    memset((void*)pSessionInfoBuf,0,iLength);
        
    snprintf(pSessionInfoBuf, iLength - 1, "%d\r\n",pRTSPStreaming->iSessionListNumber);
    
    for( i=0 ; i< pRTSPStreaming->iSessionListNumber; i++ )       
    {
        snprintf(pSessionInfoBuf + strlen(pSessionInfoBuf), iLength - strlen(pSessionInfoBuf) - 1,"%s %d\r\n",(char*)inet_ntoa(*((struct in_addr*)&(pRTSPStreaming->pstSessionList[i].ulClientIP))),pRTSPStreaming->pstSessionList[i].dwSessionID);
        
        if( (iLength - strlen(pSessionInfoBuf) ) < 30 )
            return -1;
    }    
    
    return 0;
        
}
#ifdef _SHARED_MEM
SCODE RTSPCriticalSection_Initial(HANDLE *phObject)
{
    pthread_mutexattr_t muAttr;
	*phObject = (HANDLE)malloc(sizeof(pthread_mutex_t));

	if(!*phObject)	return OSE_MEMORY;

    if(pthread_mutexattr_init(&muAttr) != S_OK)   return S_FAIL;
    if(pthread_mutexattr_settype(&muAttr, PTHREAD_MUTEX_ERRORCHECK_NP) != S_OK)
    {
        return S_FAIL;
    }

    if(pthread_mutex_init((pthread_mutex_t *)(*phObject), &muAttr) != S_OK)
    {
        return S_FAIL;
    }
    pthread_mutexattr_destroy(&muAttr);
    return S_OK;
}
#endif
