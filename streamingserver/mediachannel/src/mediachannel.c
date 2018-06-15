/* *  Copyright (c) 2002 Vivotek Inc. All rights reserved. *
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
 *  Module name        :   RTP/RTCP Channel
 *  File name          :   Channel.c 
 *  File description   :   RTP/RTCP packet transmitter 
 *  Author             :   ShengFu
 *  Created at         :   2002/5/20 
 *  Note               :   
 *	$Log: /RD_1/Protocol/RTSP/Server/rtspstreamserver/mediachannel/src/mediachannel.c $
 * 
 * 2     06/05/18 3:26p Shengfu
 * update to version 1.4.0.0
 * 
 * 10    06/01/10 5:48p Shengfu
 * update to 1.3.0.6
 * 
 * 8     05/11/30 6:23p Shengfu
 * update to version 1.3.0.4
 * 
 * 7     05/11/03 11:57a Shengfu
 * update to version 1.3.0.3
 * 
 * 6     05/10/07 8:48a Shengfu
 * 
 * 5     05/09/27 1:14p Shengfu
 * update to version 1,3,0,1
 * 
 * 4     05/08/19 11:49a Shengfu
 * 
 * 1     05/08/19 11:28a Shengfu
 * 
 * 3     05/08/10 9:01a Shengfu
 * update rtspstreaming server which enable multicast
 * 
 * 13    05/07/13 2:25p Shengfu
 * update rtsp streaming server
 * 
 * 12    05/04/15 1:35p Shengfu
 * 1. multicast added, but disable
 * 2. RTP extension added
 * 
 * 9     05/03/04 5:25p Shengfu
 * 
 * 8     05/03/04 3:57p Shengfu
 * 
 * 7     05/03/02 1:32p Shengfu
 * 1. send out RTCP sender report once got receiver report from client
 * 
 * 6     05/02/01 2:47p Shengfu
 * 
 * 5     05/01/24 10:03a Shengfu
 * 
 * 4     05/01/18 3:09p Shengfu
 * 1. RTCP socket could be zero
 * 
 * 3     04/12/20 2:34p Shengfu
 * update to version 1.1.0.0
 * 
 * 2     04/09/16 4:33p Shengfu
 * 
 * 1     04/09/14 9:36a Shengfu
 * 
 * 1     04/09/14 9:16a Shengfu
 * 
 * 10    03/12/23 7:04p Shengfu
 * 
 * 8     03/12/05 5:27p Shengfu
 * update to version 0102j
 * 
 * 5     03/10/24 4:07p Shengfu
 * 
 * 8     03/08/27 3:51p Shengfu
 * 
 * 2     03/04/17 2:45p Shengfu
 * update to 0102b
 * 
 * 4     02/08/09 1:53p Simon
 * Don't need to send the I frame for the first video packet to reduce the
 * audio delay problem
 * 
 * 3     02/06/24 5:18p Shengfu
 * video_only modified ( send out SDP header every I frame )  
 */


#include <stdlib.h>
#include <string.h>
#include "rtsprtpcommon.h"
#include "mediachannel_local.h"

#ifdef _PSOS_TRIMEDIA        
#include "extio.h"			
#endif       

DWORD  THREADAPI ChannelSendLoop(DWORD dwParam);
void	ChannelRTCPSendLoop(void*	lpParam);
void    ChannelSendRTCPToEveryConnection(CHANNEL *pChannel);
void    ChannelReceiveRTCPReportFromEveryConnection(CHANNEL *pChannel);
void	ChannelReceiveRTCPReportFromFixedSocket(CHANNEL *pChannel);

/*unsigned long  Get_SessionID(RTPRTCPCHANNEL_SESSION* sess)
{
    unsigned long ulID;
    
    CriticalLock(&sess->csSemaph);
    ulID = sess->dwSessionID;
    CriticalUnlock(&sess->csSemaph);
    
    return ulID;
}


SOCKET Get_RTCPSocket(RTPRTCPCHANNEL_SESSION* sess)
{
    SOCKET rtcp_sock;
    
    CriticalLock(&sess->csSemaph);
    rtcp_sock = sess->sktRTCP;
    CriticalUnlock(&sess->csSemaph);
    
    return rtcp_sock;
}
*/
#ifdef _SHARED_MEM
SCODE RTPRTCPChannel_InitShmMediaInfo(RTPRTCPCHANNEL_SESSION *pSession, TShmemMediaInfo *pShmMediaInfo)
{
	//20130605 added by Jimmy to support metadata event
	int i;

	if(pShmMediaInfo == NULL)
	{
		return S_FAIL;
	}

	TShmemSessionInfo *pShmSessionInfo = (TShmemSessionInfo *)pShmMediaInfo->hParentHandle;

	pShmMediaInfo->bFrameGenerated = TRUE;
	pShmMediaInfo->iRemainingSize = 0;
#ifdef _METADATA_ENABLE
    if(pSession != NULL)	
	{
        //20140819 added by Charles for eventparser API
        if(pSession->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_METADATA)
        {
            EventParser_FreeBufClient(&pShmMediaInfo->ahShmemHandle[0], pShmMediaInfo->iShmemClientBufID);
        }
    }
#endif
	//20130605 modified by Jimmy to support metadata event
	for (i = 0; i < SHMEM_HANDLE_MAX_NUM; i++)
	{
		pShmMediaInfo->ahShmemHandle[i] = NULL;
		//printf("[%s]pShmMediaInfo->ahShmemHandle[%d](%p)\n", __FUNCTION__, i, &pShmMediaInfo->ahShmemHandle[i]);

		if(pShmMediaInfo->ahClientBuf[i] != NULL)
		{
			//20081009 release the media buffer
			SharedMem_ReleaseReadBuffer(pShmMediaInfo->ahClientBuf[i]);
			SharedMem_FreeBufClient(&pShmMediaInfo->ahClientBuf[i]);
			pShmMediaInfo->ahClientBuf[i] = NULL;
		}
	}

	ChannelShmResetMediaInfo(pShmMediaInfo);

	pShmMediaInfo->tStreamBuffer.dwBytesUsed = 0;
	pShmMediaInfo->tStreamBuffer.wPacketCount = 0;
	pShmMediaInfo->tStreamBuffer.dwCurrentPosition = 0;
	pShmMediaInfo->bCSAcquiredButNotSelected = FALSE;
	//20130603 added by Jimmy to follow RFC 3551, only set the marker bit in the first audio packet
	pShmMediaInfo->bFirstAudioPacketized = FALSE;
    //20140812 Added by Charles for mod no drop frame
    pShmMediaInfo->bMediaOnDemand = FALSE;
    //20140819 added by Charles for eventparser API
    pShmMediaInfo->bAnalytics = FALSE;
    pShmMediaInfo->bGetNewData = TRUE;
    memset(pShmMediaInfo->acVideoAnalyticsConfigToken, 0, RTSPSTREAMING_TOKEN_LENGTH);
    pShmMediaInfo->iShmemClientBufID = 0;
    pShmMediaInfo->iProcessIndex = 0;

	if(pSession != NULL)	//Multicast will not have session!
	{
		if(pSession->hTCPMuxCS != NULL)
		{
			OSCriticalSection_Leave(pSession->hTCPMuxCS);
			pShmSessionInfo->eCritSecStatus = eCSReleased;
		}
	}

	return S_OK;
}
#endif

void RTPRTCPChannel_InitSession(RTPRTCPCHANNEL_SESSION *pSession)
{
	//20110706 Modified by danny For TCP/HTTP mode socket not sync in multi thread
	if( pSession->iRTPStreamingType == RTP_OVER_UDP && pSession->psktRTP != NULL && *pSession->psktRTP > 0)
	{
		closesocket(*pSession->psktRTP);
		*pSession->psktRTP = -1;
	}
	if( pSession->iRTPStreamingType == RTP_OVER_UDP && pSession->psktRTCP != NULL && *pSession->psktRTCP > 0)
	{
		closesocket(*pSession->psktRTCP);
		*pSession->psktRTCP = -1;
	}
	
    pSession->dwSessionID = 0;
    pSession->iStartRTCPSReport = 0;
    pSession->iStatus = SESSION_IDLE;
	pSession->iRTPStreamingType= -1;
	pSession->iEmbeddedRTCPID = -1;
	pSession->iEmbeddedRTPID = -1;
	pSession->iVivotekClient = 0;
	pSession->iCodecIndex = 0;
	//20101018 Add by danny for support multiple channel text on video
	pSession->iMultipleChannelChannelIndex = 0;

    pSession->hRTPRTCPComposerHandle = NULL;
	
	memset((void*)&pSession->RTPNATAddr,0,sizeof(RTSP_SOCKADDR));
	memset((void*)&pSession->RTCPNATAddr,0,sizeof(RTSP_SOCKADDR));

	pSession->dwQTTimeoutInitial =0;

#ifdef _SHARED_MEM
	//First initialize the structure if exist
	if(pSession->ptShmemMediaInfo != NULL)
	{
		RTPRTCPChannel_InitShmMediaInfo(pSession, pSession->ptShmemMediaInfo);
	}
	//Set the pointer to NULL
	pSession->ptShmemMediaInfo = NULL;
	//Init RTCP remaining length
	pSession->iRTCPOffset = 0;
	pSession->iRTCPRemainingLength = 0;
#endif
    pSession->nChannelType = 0;
    //20141110 added by Charles for ONVIF Profile G
    pSession->iCSeq = 0;
    pSession->iCSeqUpdated = 0;
    pSession->idebug = 0;
	return;
}

SCODE  RTPRTCPChannel_CloseMulticast(CHANNEL *pChannel, int iMulticastIndex)
{
	//20110627 Add by danny for join/leave multicast group by session start/stop
	struct ip_mreq      ssdpMcastAddr;
	
	pChannel->fCallbackFunct(pChannel->hParentHandle
			,RTPRTCPCHANNEL_CALLBACKFLAG_MULTICAST_CLOSED
			,(void*)(iMulticastIndex+1),0);
	pChannel->stMulticast[iMulticastIndex].hRTPRTCPComposerHandle = NULL;
	pChannel->stMulticast[iMulticastIndex].iStatus = SESSION_IDLE;
	pChannel->stMulticast[iMulticastIndex].sktRTP = -1;

	//20110627 Add by danny for join/leave multicast group by session start/stop
	if( pChannel->stMulticast[iMulticastIndex].sktRTCP > 0 )
	{
		printf("[%s]Before close sktRTCP %d\n", __func__, pChannel->stMulticast[iMulticastIndex].sktRTCP);
		memset((void *)&ssdpMcastAddr, 0, sizeof(ssdpMcastAddr));
		ssdpMcastAddr.imr_interface.s_addr = INADDR_ANY;
		ssdpMcastAddr.imr_multiaddr.s_addr = pChannel->stMulticast[iMulticastIndex].ulMulticastAddress;
		if( setsockopt(pChannel->stMulticast[iMulticastIndex].sktRTCP, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char *)&ssdpMcastAddr, sizeof(ssdpMcastAddr)) != 0 )
		{
			TelnetShell_DbgPrint("Error in leave multicast group %d!!\r\n",WSAGetLastError());
		}
		pChannel->stMulticast[iMulticastIndex].ulMulticastAddress = 0;
		pChannel->stMulticast[iMulticastIndex].sktRTCP = -1;
	}
	
	//20110725 Add by danny For Multicast RTCP receive report keep alive
	pChannel->stMulticast[iMulticastIndex].iRRAlive = 0;
	pChannel->stMulticast[iMulticastIndex].dwRecvReportTime = 0;
	
	pChannel->stMulticast[iMulticastIndex].iVivotekClient = 0;
	//20101018 Add by danny for support multiple channel text on video
	pChannel->stMulticast[iMulticastIndex].iMultipleChannelChannelIndex = 0;
    //20141110 added by Charles for ONVIF Profile G
    pChannel->stMulticast[iMulticastIndex].iCSeq = 0;
    pChannel->stMulticast[iMulticastIndex].iCSeqUpdated = 0;
#ifdef _SHARED_MEM
#ifdef _METADATA_ENABLE
    //20140819 added by Charles for eventparser API
    if (pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_METADATA)
    {
        EventParser_FreeBufClient(&pChannel->stMulticast[iMulticastIndex].ptShmemMediaInfo->ahShmemHandle[0], pChannel->stMulticast[iMulticastIndex].ptShmemMediaInfo->iShmemClientBufID);
    }
#endif
	if(pChannel->stMulticast[iMulticastIndex].ptShmemMediaInfo != NULL)
	{
		RTPRTCPChannel_InitShmMediaInfo(NULL, pChannel->stMulticast[iMulticastIndex].ptShmemMediaInfo);
	}
#endif
	return S_OK;
}

HANDLE RTPRTCPChannel_Create( int iMaxSessionNumber, RTPRTCPCHANNEL_PARAM *pstRTPRTCPChannelParameter)
{
    ULONG ulResult1,ulResult2;
	RTPRTCPCHANNEL_CONNECTION   *pRTPSessionInfo;
	unsigned long ulMsg[4];
	TOSThreadInitOptions InitThreadOptions;
	int i;

    CHANNEL *pChannel=(CHANNEL *)malloc( sizeof(CHANNEL) );
    
    if(pChannel)
    {
        memset(pChannel,0,sizeof(CHANNEL) );

		pChannel->iUDPRTPSock = pstRTPRTCPChannelParameter->iUDPRTPSock;
		pChannel->iUDPRTCPSock = pstRTPRTCPChannelParameter->iUDPRTCPSock;

        pChannel->session= (RTPRTCPCHANNEL_SESSION*)malloc(iMaxSessionNumber*sizeof(RTPRTCPCHANNEL_SESSION));
        //CID:397 RESOURCE_LEAK
		if( pChannel->session == NULL )
		{
			free(pChannel);
			pChannel = NULL;
			return 0;
		}

        if(pstRTPRTCPChannelParameter->iRTPRTCPMediaType == RTPRTCPCHANNEL_MEDIATYPE_VIDEO )
        {
            /*pChannel->pcMp4EncodeParam = (char*)malloc(1500);//pstRTPRTCPChannelParameter->iMPEG4StartBitStreamLength+13);

			if( pChannel->pcMp4EncodeParam == NULL )
				return 0;
			
			pChannel->nMp4EncodeLength = pstRTPRTCPChannelParameter->iMPEG4StartBitStreamLength;
			memcpy(pChannel->pcMp4EncodeParam+12
			,pstRTPRTCPChannelParameter->pbyMPEG4StartBitStream
			,pstRTPRTCPChannelParameter->iMPEG4StartBitStreamLength);*/
		
        } //if video -> copy mp4 video decode parameter

		pChannel->pRTPTmpMediaBuffer = (RTPMEDIABUFFER*)malloc(sizeof(RTPMEDIABUFFER)+1500);

		 //CID:397 RESOURCE_LEAK
		if( pChannel->pRTPTmpMediaBuffer == 0 )
		{
			free(pChannel->session);
			pChannel->session = NULL;
			free(pChannel);
			pChannel = NULL;
			return 0;
		}
			
		memset((void*)(pChannel->pRTPTmpMediaBuffer),0,sizeof(RTPMEDIABUFFER)+1500);

        for(i=0;i<iMaxSessionNumber;i++)
		{
            memset((void*)(&pChannel->session[i]),0,sizeof(RTPRTCPCHANNEL_SESSION));
			RTPRTCPChannel_InitSession(pChannel->session+i);
            //pChannel->session[i].sktRTP = -1;
            //pChannel->session[i].sktRTCP = -1;
            //pChannel->session[i].hRTPRTCPComposerHandle = NULL;
		}

#ifdef RTSPRTP_MULTICAST
		//20130327 modified by Jimmy to support different video/audio multicast addresses for ONVIF Test Tool 12.12
		//20130904 modified by Charles for ondemand multicast
		for( i=0 ;i < (RTSP_MULTICASTNUMBER + RTSP_ONDEMAND_MULTICASTNUMBER) ; i++)
		{
			pChannel->stMulticast[i].hRTPRTCPComposerHandle = NULL;
			pChannel->stMulticast[i].sktRTCP = -1;
			pChannel->stMulticast[i].sktRTP = -1;
			pChannel->stMulticast[i].iStatus = SESSION_IDLE;
			pChannel->stMulticast[i].iVivotekClient = 0;
			//20101018 Add by danny for support multiple channel text on video
			pChannel->stMulticast[i].iMultipleChannelChannelIndex = 0;
            pChannel->stMulticast[i].iCSeq = 0;
            pChannel->stMulticast[i].iCSeqUpdated = 0;
		}
#endif

        pChannel->nChannelType = pstRTPRTCPChannelParameter->iRTPRTCPMediaType;
        pChannel->iMaxSession = iMaxSessionNumber;
        ulResult1=OSMsgQueue_Initial((HANDLE*)(&pChannel->qAddRemoveConnection), iMaxSessionNumber*4);
		ulResult2=OSMsgQueue_Initial((HANDLE*)(&pChannel->qSessionInfo), iMaxSessionNumber*2);

//		ulResult=q_create("QCha", 10, Q_LOCAL | Q_FIFO | Q_LIMIT, &pChannel->qAddRemoveConnection);
		
		if(ulResult1 != S_OK || ulResult2 != S_OK)
		{
			free(pChannel);
			return 0;
		}
		
		for(i=0;i<iMaxSessionNumber;i++)
		{
			pRTPSessionInfo = (RTPRTCPCHANNEL_CONNECTION*)malloc(sizeof(RTPRTCPCHANNEL_CONNECTION));

			if( pRTPSessionInfo != NULL )
			{
				memset(pRTPSessionInfo,0,sizeof(RTPRTCPCHANNEL_CONNECTION));
				ulMsg[0]=(unsigned long)pRTPSessionInfo; 
				OSMsgQueue_Send((HANDLE)pChannel->qSessionInfo, (DWORD*)ulMsg);
			}
		}

		pChannel->nAddRemoveonnectionCount=0;
		pChannel->nTotalConnectionCount=0;
				
		memset((void*)&InitThreadOptions,0,sizeof(TOSThreadInitOptions));
		InitThreadOptions.dwInstance = (DWORD)pChannel;
		InitThreadOptions.dwPriority = pstRTPRTCPChannelParameter->ulThreadPriority;
		InitThreadOptions.dwStackSize = 0;												//20090316 Let kernel handle the size
		InitThreadOptions.dwFlags = T_LOCAL;
		InitThreadOptions.dwMode  = T_PREEMPT|T_SUPV;
		InitThreadOptions.pThreadProc = ChannelSendLoop;
	
		if( OSThread_Initial(&(pChannel->hThread),&InitThreadOptions) != S_OK)
			return 0;

		return (HANDLE)pChannel;
	}
	else
		return 0;
}

int RTPRTCPChannel_SetParameters(HANDLE hRTPRTCPChannelHandle, RTPRTCPCHANNEL_PARAM *pstVideoEncodingParameter)
{
	CHANNEL *pChannel;

	if(!hRTPRTCPChannelHandle)
	{
		return CHANNEL_ERR_NOTOPEN;
    }		

	pChannel=(CHANNEL *)hRTPRTCPChannelHandle;

	if(!pstVideoEncodingParameter)
	{
		return CHANNEL_ERR_PARAM_NULL;
    }		

    if( pstVideoEncodingParameter->iRTPRTCPMediaType == RTPRTCPCHANNEL_MEDIATYPE_VIDEO )
    {
        /*pChannel->nMp4EncodeLength = pstVideoEncodingParameter->iMPEG4StartBitStreamLength;
        memcpy(pChannel->pcMp4EncodeParam+12
               ,pstVideoEncodingParameter->pbyMPEG4StartBitStream
               ,pstVideoEncodingParameter->iMPEG4StartBitStreamLength);*/
    }
               
     pChannel->nChannelType = pstVideoEncodingParameter->iRTPRTCPMediaType;
   
	return 0;
}


int RTPRTCPChannel_SetCallback(HANDLE hRTPRTCPChannelHandle, RTPRTCPCHANNELCALLBACK fCallback, HANDLE hParentHandle)
{
	CHANNEL *pChannel;
	if(!hRTPRTCPChannelHandle)
	{
		return CHANNEL_ERR_NOTOPEN;
    }		

	pChannel=(CHANNEL *)hRTPRTCPChannelHandle;

	if(!fCallback )
	{
		return CHANNEL_ERR_SETCALLBACK_NULL;
    }		

	pChannel->fCallbackFunct=fCallback;
	pChannel->hParentHandle=hParentHandle;

	return 0;
}


int RTPRTCPChannel_Start(HANDLE hRTPRTCPChannelHandle)
{
	CHANNEL *pChannel;
//	DWORD dwThreadID1,dwThreadID2;
//	HANDLE	hThread1,hThread2;


	if(!hRTPRTCPChannelHandle)
	{
		return CHANNEL_ERR_NOTOPEN;
    }		

	pChannel=(CHANNEL *)hRTPRTCPChannelHandle;

	pChannel->nChannelStatus=CHANNEL_RUNNING;		
    OSThread_Start(pChannel->hThread);
//	pSOSCreateThread("rtp ",1500,(void *)ChannelSendLoop,(void *)pChannel,ulRTPThreadPriority);

	return 0;
}


int RTPRTCPChannel_Stop(HANDLE hRTPRTCPChannelHandle)
{
	CHANNEL *pChannel;
//	DWORD dwThreadID;
	int nCount=0;

	if(!hRTPRTCPChannelHandle)
	{
		return CHANNEL_ERR_NOTOPEN;
    }		

	pChannel=(CHANNEL *)hRTPRTCPChannelHandle;

	pChannel->nChannelStatus=CHANNEL_STOPPING;		

	while(pChannel->nChannelStatus!=CHANNEL_STOPPED)
	{
		if(nCount<20)
		{			
			OSSleep_MSec(20);
			nCount ++;
        }			
		else
		{			
			return 1;
        }			
	}

	return 0;
}

#ifdef RTSPRTP_MULTICAST

int RTPRTCPChannel_RemoveMulticastSession(HANDLE hRTPRTCPChannelHandle,int iGroupIndex)
{
	CHANNEL			*pChannel;
	unsigned long	ulResult;
	unsigned long	aulMsgBuf[4];	
	int				iCounter=0;

	if(!hRTPRTCPChannelHandle)
	{
		return CHANNEL_ERR_NOTOPEN;
    }		

	pChannel=(CHANNEL *)hRTPRTCPChannelHandle;
	
	memset(aulMsgBuf,0,4*sizeof(unsigned long));	
	aulMsgBuf[0] = RTPRTCP_REMOVE_MULTICAST;
	aulMsgBuf[1] = iGroupIndex;
	
	while(1)
    {
	    ulResult=OSMsgQueue_Send((HANDLE)pChannel->qAddRemoveConnection, (DWORD*)aulMsgBuf);

		iCounter++;
		if( iCounter > 10)
			return -1;

	    if(ulResult != S_OK )
	    {
            TelnetShell_DbgPrint("[Channel] Remove multicast failed, %d\n",(int)ulResult);
		    OSSleep_MSec(200);		
	    }
	    else
	        break;
	}

	pChannel->nAddRemoveonnectionCount++;
	
	return 0;
}


#endif

int RTPRTCPChannel_RemoveOneSession(HANDLE hRTPRTCPChannelHandle, DWORD dwSessionID)
{
	CHANNEL *pChannel;
	unsigned long ulResult;
	unsigned long aulMsgBuf[4];	
	int iCounter=0;

	if(!hRTPRTCPChannelHandle)
	{
		return CHANNEL_ERR_NOTOPEN;
    }		

	pChannel=(CHANNEL *)hRTPRTCPChannelHandle;
	
	memset(aulMsgBuf,0,4*sizeof(unsigned long));	
	aulMsgBuf[0]=RTPRTCP_REMOVE_SESSION;
	aulMsgBuf[1]=(DWORD)dwSessionID;

    while(1)
    {
        iCounter++;
	    ulResult=OSMsgQueue_Send((HANDLE)pChannel->qAddRemoveConnection, (DWORD*)aulMsgBuf);

	    if(ulResult != S_OK )
	    {
#ifdef _LINUX
            //syslog(LOG_ERR,"[Channel] Remove One connect failed, %d\n",(int)ulResult);
#endif // _LINUX
            TelnetShell_DbgPrint("[Channel] Remove One connect failed, %d\n",(int)ulResult);
		    OSSleep_MSec(200);		
	    }
	    else
	        break;
	        
        if( iCounter > 10 )
            return -1;	        
	}

	pChannel->nAddRemoveonnectionCount++;
	
	return 0;
}

DWORD ChannelLoopRemoveOneConnect(HANDLE hChannel, DWORD dwSessionID)
{
	CHANNEL *pChannel;
	int i;
	
	if(!hChannel)
	{
		return CHANNEL_ERR_NOTOPEN;
    }		

	pChannel=(CHANNEL *)hChannel;
	
    for(i=0 ; i< pChannel->iMaxSession ; i++ )
    {
        if( pChannel->session[i].dwSessionID == dwSessionID )
        {
			            
            if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_VIDEO)
			{
                TelnetShell_DbgPrint("video channel removed:ID %lu socket %d\n",(long)dwSessionID, *pChannel->session[i].psktRTP);
			}
			//20120813 modified by Jimmy for metadata
			else if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_AUDIO)
			{
                TelnetShell_DbgPrint("audio channel removed:ID %lu socket %d\n",(long)dwSessionID, *pChannel->session[i].psktRTP);
			}
			//20120813 added by Jimmy for metadata
			else if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_METADATA)
			{
                TelnetShell_DbgPrint("metadata channel removed:ID %lu socket %d\n",(long)dwSessionID, *pChannel->session[i].psktRTP);
			}				
/*			if( pChannel->session[i].iRTPStreamingType == RTP_OVER_UDP)  
				closesocket(pChannel->session[i].sktRTP);

            pChannel->session[i].sktRTP = -1;
            pChannel->session[i].dwSessionID = 0; 
            pChannel->session[i].iStartRTCPSReport = 0;

			if( pChannel->session[i].iRTPStreamingType == RTP_OVER_UDP)
				closesocket(pChannel->session[i].sktRTCP);
            pChannel->session[i].sktRTCP = -1;

            pChannel->session[i].iStatus = SESSION_IDLE;
            pChannel->session[i].hRTPRTCPComposerHandle = NULL;*/
//			iStreamType = pChannel->session[i].iRTPStreamingType;
			RTPRTCPChannel_InitSession(&pChannel->session[i]);

			// callback for UDP port released ok
			//if( iStreamType == RTP_OVER_UDP) ShengFu 2006/08/03 bug fixed for session management error, call back both TCP and UDP mode			
			pChannel->fCallbackFunct(pChannel->hParentHandle, RTPRTCPCHANNEL_CALLBACKFLAG_REMOVE_OK, (void *)dwSessionID, 0);
 
            pChannel->nTotalConnectionCount -- ;
#ifdef _LINUX
            //syslog(LOG_ERR, "RTP channels remained %d\n",pChannel->nTotalConnectionCount);
#endif
            TelnetShell_DbgPrint("RTP channels remained %d\r\n",pChannel->nTotalConnectionCount);
				
            break;
        }	
	}

	return 0;

}

int RTPRTCPChannel_PauseOneSession(HANDLE hRTPRTCPChannelHandle, DWORD dwSessionID)
{
	CHANNEL			*pChannel;
	unsigned long	ulResult;
	unsigned long	aulMsgBuf[4];	
	int				iCount=0;


	if(!hRTPRTCPChannelHandle)
	{
		return CHANNEL_ERR_NOTOPEN;
    }		

	pChannel=(CHANNEL *)hRTPRTCPChannelHandle;
	
	memset(aulMsgBuf,0,4*sizeof(unsigned long));	
	aulMsgBuf[0]=RTPRTCP_PAUSE_SESSION;
	aulMsgBuf[1]=(DWORD)dwSessionID;

    while(1)
    {
	    ulResult=OSMsgQueue_Send((HANDLE)pChannel->qAddRemoveConnection, (DWORD*)aulMsgBuf);

		iCount ++;
	    if(ulResult != S_OK)
	    {
//		    DbgPrint(("[Channel] Pause One connect failed, %d\n",ulResult));
#ifdef _LINUX
            //syslog(LOG_ERR,"[Channel] Pause One connect failed, %d\n",(int)ulResult);
#endif // _LINUX
            DbgLog((dfCONSOLE|dfINTERNAL,"[Channel] Pause One connect failed, %d\n",ulResult));
		    OSSleep_MSec(200);	
	    }
	    else
	        break;

		if(iCount > 10)
			return -1;
    }
    
	pChannel->nAddRemoveonnectionCount++;
	
	return 0;
}

DWORD ChannelLoopPauseOneConnect(HANDLE hChannel, DWORD dwSessionID)
{
	CHANNEL *pChannel;
	int i;	

	if(!hChannel)
	{
		return CHANNEL_ERR_NOTOPEN;
    }		

	pChannel=(CHANNEL *)hChannel;
	
    for(i=0 ; i< pChannel->iMaxSession ; i++ )
    {
        if( pChannel->session[i].dwSessionID == dwSessionID )
        {			   
            pChannel->session[i].iStatus = SESSION_PAUSED;
	    	break;
        }	
	}

	return 0;

}

int RTPRTCPChannel_ResumeOneSession(HANDLE hRTPRTCPChannelHandle, DWORD dwSessionID)
{
	CHANNEL			*pChannel;
	unsigned long	ulResult;
	unsigned long	aulMsgBuf[4];	
	int				iCount=0;

	if(!hRTPRTCPChannelHandle)
	{
		return CHANNEL_ERR_NOTOPEN;
    }		

	pChannel=(CHANNEL *)hRTPRTCPChannelHandle;
	
	memset(aulMsgBuf,0,4*sizeof(unsigned long));	
	aulMsgBuf[0]=RTPRTSP_RESUME_SESSION;
	aulMsgBuf[1]=(DWORD)dwSessionID;

    while(1)
    {
	    ulResult=OSMsgQueue_Send((HANDLE)pChannel->qAddRemoveConnection, (DWORD*)aulMsgBuf);
		iCount ++;

	    if(ulResult)
	    {
#ifdef _LINUX
            //syslog(LOG_ERR,"[Channel] Resume One connect failed, %d\n",(int)ulResult);
#endif // _LINUX
            DbgLog((dfCONSOLE|dfINTERNAL,"[Channel] Resume One connect failed, %d\n",ulResult));
		    OSSleep_MSec(200);			
	    }
	    else
	        break;

		if( iCount > 10 )
			return -1;
    }

	pChannel->nAddRemoveonnectionCount++;
	
	return 0;
}

DWORD ChannelLoopResumeOneConnect(HANDLE hChannel, DWORD dwSessionID)
{
	CHANNEL *pChannel;
	int i;	

	if(!hChannel)
	{
		return CHANNEL_ERR_NOTOPEN;
    }		

	pChannel=(CHANNEL *)hChannel;
	
    for(i=0 ; i< pChannel->iMaxSession ; i++ )
    {
        if( pChannel->session[i].dwSessionID == dwSessionID )
        {			   
            pChannel->session[i].iStatus = SESSION_PLAYING;
	    	break;
        }	
	}

	return 0;

}

DWORD	ChannelLoopAddOneConnect(HANDLE hChannel, unsigned long *aulMsgBuf)
{
	CHANNEL *pChannel;
	RTPRTCPCHANNEL_CONNECTION *pRTPSessionInfo;
	int i;
	ULONG ulNonBlock,ulMsg[4];


	if(!hChannel)
	{
		return CHANNEL_ERR_NOTOPEN;
    }		

	pChannel=(CHANNEL *)hChannel;

	for(i=0; i< pChannel->iMaxSession ; i++ )
	{
        if( pChannel->session[i].dwSessionID == 0 )
        {
			pRTPSessionInfo = (RTPRTCPCHANNEL_CONNECTION*)aulMsgBuf[1];
			pChannel->session[i].dwSessionID = pRTPSessionInfo->dwSessionID;

#ifdef _SHARED_MEM
			pChannel->session[i].ptShmemMediaInfo = pRTPSessionInfo->ptShmemMediaInfo;
            //20140819 added by Charles for eventparser API, for eventparser to separate different shmem client buffer
            pChannel->session[i].ptShmemMediaInfo->iShmemClientBufID = i;
			//20101020 Add by danny for support seamless stream TCP/UDP timeout
			pChannel->session[i].bSeamlessStream = pRTPSessionInfo->bSeamlessStream;
#endif
			if( pRTPSessionInfo->iRTPStreamingType == RTP_OVER_UDP)
			{
				pChannel->session[i].iRTPStreamingType = RTP_OVER_UDP;
				//20111205 Modified by danny For UDP mode socket leak
				//20110706 Modified by danny For TCP/HTTP mode socket not sync in multi thread
				pChannel->session[i].psktRTP = pRTPSessionInfo->psktRTP;
				pChannel->session[i].psktRTCP = pRTPSessionInfo->psktRTCP;
				memcpy((void*)&pChannel->session[i].RTPNATAddr,(void*)&pRTPSessionInfo->RTPNATAddr,sizeof(RTSP_SOCKADDR));
				memcpy((void*)&pChannel->session[i].RTCPNATAddr,(void*)&pRTPSessionInfo->RTCPNATAddr,sizeof(RTSP_SOCKADDR));
			}
			else
			{
				pChannel->session[i].iRTPStreamingType = RTP_OVER_TCP;
				pChannel->session[i].iEmbeddedRTCPID = pRTPSessionInfo->iEmbeddedRTCPID;
				pChannel->session[i].iEmbeddedRTPID = pRTPSessionInfo->iEmbeddedRTPID;
				//20110706 Modified by danny For TCP/HTTP mode socket not sync in multi thread
				pChannel->session[i].psktRTP  = pRTPSessionInfo->psktRTSPSocket;
				pChannel->session[i].psktRTCP  = pRTPSessionInfo->psktRTSPSocket;
				//20160603 add by faber, set tcp mutex when session add
				pChannel->session[i].hTCPMuxCS = pRTPSessionInfo->hTCPMuxCS;
				printf("[%s]psktRTP %p(%d), psktRTCP %p(%d)\n", __func__, pChannel->session[i].psktRTP , *pChannel->session[i].psktRTP, pChannel->session[i].psktRTCP, *pChannel->session[i].psktRTCP);
			}

			pChannel->session[i].iStatus = SESSION_PLAYING;
    
            ulNonBlock=1;
            ioctlsocket(*pChannel->session[i].psktRTP,FIONBIO,&ulNonBlock);
            ioctlsocket(*pChannel->session[i].psktRTCP,FIONBIO,&ulNonBlock);
	    
			pChannel->session[i].iVivotekClient = pRTPSessionInfo->iVivotekClient;
			pChannel->session[i].iCodecIndex = pRTPSessionInfo->iCodecIndex;

			if(pRTPSessionInfo->iNotifySource)
			{
				printf("channel type = %d\n", pChannel->nChannelType );
				printf("sdp index = %d\n", pRTPSessionInfo->iSdpIndex);
				pChannel->fCallbackFunct(pChannel->hParentHandle,RTPRTCPCHANNEL_CALLBACKFLAG_SESSION_START,pRTPSessionInfo->iSdpIndex,0);
			}
			//20160621 add by Faber, we should guarantee the session is added before the force intra
			if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_VIDEO)
            {
            	pChannel->fCallbackFunct(pChannel->hParentHandle,RTPRTCPCHANNEL_CALLBACKFLAG_FORCE_I_FRAME,(void*)pChannel->session[i].iCodecIndex,0);
                printf("ChannelLoopAddOneConnect add session = %lu\n", pChannel->session[i].dwSessionID);
            }

			//20101018 Add by danny for support multiple channel text on video
			pChannel->session[i].iMultipleChannelChannelIndex = pRTPSessionInfo->iMultipleChannelChannelIndex;
            pChannel->session[i].nChannelType = pChannel->nChannelType;
            //20141110 added by Charles for ONVIF Profile G
            pChannel->session[i].iCSeq = pRTPSessionInfo->iCseq;
            pChannel->session[i].iCSeqUpdated = pRTPSessionInfo->iCseq;

            if( pChannel->session[i].iVivotekClient != 0 )
                TelnetShell_DbgPrint("--[Media Channel]RTPEX Player added--\r\n");
            					    
			pChannel->session[i].hRTPRTCPComposerHandle = (HANDLE)pRTPSessionInfo->hRTPRTCPComposerHandle;				
            pChannel->nTotalConnectionCount ++ ;
            //20141110 added by Charles for ONVIF Profile G
            //Support only one RTPExtension tag now, No Onvif RTPExtension for VivotekClient when playback
            if(pChannel->session[i].ptShmemMediaInfo->bMediaOnDemand && !pChannel->session[i].iVivotekClient)
            {
                //when playback, sendout RTP extension data defined by ONVIF Profile G
                RTPRTCPComposer_SetOnvifExtValidate(pChannel->session[i].hRTPRTCPComposerHandle);
            }
#ifdef _LINUX
            //syslog(LOG_ERR, "Add connection RTP %d\n",pChannel->nTotalConnectionCount);
#endif // _LINUX
			ulMsg[0] = aulMsgBuf[1];
			OSMsgQueue_Send((HANDLE)pChannel->qSessionInfo,(DWORD*)ulMsg);
            break;
      }
	}

	return 0;
}

#ifdef RTSPRTP_MULTICAST
DWORD	ChannelLoopAddMulticast(HANDLE hChannel, unsigned long *aulMsgBuf)
{
	CHANNEL *pChannel;
	RTPRTCPCHANNEL_CONNECTION *pRTPSessionInfo;
	ULONG ulNonBlock,ulMsg[4];
	//20110627 Add by danny for join/leave multicast group by session start/stop
	struct ip_mreq      ssdpMcastAddr;

	if(!hChannel)
	{
		return CHANNEL_ERR_NOTOPEN;
    }		

	pChannel=(CHANNEL *)hChannel;

	pRTPSessionInfo = (RTPRTCPCHANNEL_CONNECTION*)aulMsgBuf[1];
#ifdef _SHARED_MEM
	pChannel->stMulticast[aulMsgBuf[2]-1].ptShmemMediaInfo = pRTPSessionInfo->ptShmemMediaInfo;
    //20140819 added by Charles for eventparser API, for eventparser to separate different shmem client buffer
    pChannel->stMulticast[aulMsgBuf[2]-1].ptShmemMediaInfo->iShmemClientBufID = aulMsgBuf[2] + pChannel->iMaxSession;
#endif
	//20111205 Modified by danny For UDP mode socket leak
	pChannel->stMulticast[aulMsgBuf[2]-1].sktRTP   = *pRTPSessionInfo->psktRTP;
	pChannel->stMulticast[aulMsgBuf[2]-1].sktRTCP  = *pRTPSessionInfo->psktRTCP;

	//20110627 Add by danny for join/leave multicast group by session start/stop
	printf("[%s]After assigned sktRTCP %d\n", __func__, pChannel->stMulticast[aulMsgBuf[2]-1].sktRTCP);
	pChannel->stMulticast[aulMsgBuf[2]-1].ulMulticastAddress = pRTPSessionInfo->ulMulticastAddress;
	memset((void *)&ssdpMcastAddr, 0, sizeof(ssdpMcastAddr));
	ssdpMcastAddr.imr_interface.s_addr = INADDR_ANY;
	ssdpMcastAddr.imr_multiaddr.s_addr = pRTPSessionInfo->ulMulticastAddress;
	if( setsockopt(pChannel->stMulticast[aulMsgBuf[2]-1].sktRTCP, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&ssdpMcastAddr, sizeof(ssdpMcastAddr)) != 0 )
	{
		TelnetShell_DbgPrint("Error in joing multicast group %d!!\r\n",WSAGetLastError());
	}

	//20110725 Add by danny For Multicast RTCP receive report keep alive
	pChannel->stMulticast[aulMsgBuf[2]-1].iRRAlive = pRTPSessionInfo->iRRAlive;
	printf("[%s]After assigned iRRAlive %d\n", __func__, pChannel->stMulticast[aulMsgBuf[2]-1].iRRAlive);
	if(pChannel->stMulticast[aulMsgBuf[2]-1].iRRAlive == TRUE)
	{
		OSTick_GetMSec(&pChannel->stMulticast[aulMsgBuf[2]-1].dwRecvReportTime);
	}
	memset(&pChannel->stMulticast[aulMsgBuf[2]-1].RTCPDstAddr,0,sizeof(pChannel->stMulticast[aulMsgBuf[2]-1].RTCPDstAddr));
	pChannel->stMulticast[aulMsgBuf[2]-1].RTCPDstAddr.sin_family = AF_INET;
	pChannel->stMulticast[aulMsgBuf[2]-1].RTCPDstAddr.sin_addr.s_addr = pRTPSessionInfo->ulMulticastAddress;
	pChannel->stMulticast[aulMsgBuf[2]-1].RTCPDstAddr.sin_port = htons(pRTPSessionInfo->usMulticastRTCPPort);
	
	pChannel->stMulticast[aulMsgBuf[2]-1].iVivotekClient = pRTPSessionInfo->iVivotekClient;
	pChannel->stMulticast[aulMsgBuf[2]-1].iCodecIndex = pRTPSessionInfo->iCodecIndex;
	//20101018 Add by danny for support multiple channel text on video
	pChannel->stMulticast[aulMsgBuf[2]-1].iMultipleChannelChannelIndex = pRTPSessionInfo->iMultipleChannelChannelIndex;
    //20141110 added by Charles for ONVIF Profile G
    pChannel->stMulticast[aulMsgBuf[2]-1].iCSeq = pRTPSessionInfo->iCseq;
    pChannel->stMulticast[aulMsgBuf[2]-1].iCSeqUpdated = pRTPSessionInfo->iCseq;

	pChannel->stMulticast[aulMsgBuf[2]-1].iStatus = SESSION_PLAYING;
    //20160621 add by Faber, we should quanrantee the session is add into the list of channel before fource intra
    //because of multicast session is added is depand on SESSION_PLAYING, so we should force intra after session status changed
	//20170727 modify by Faber, multicast need to output start when session start
    if(pRTPSessionInfo->iNotifySource)
	{
		printf("channel type = %d\n", pChannel->nChannelType );
		printf("sdp index = %d\n", pRTPSessionInfo->iSdpIndex);
		pChannel->fCallbackFunct(pChannel->hParentHandle,RTPRTCPCHANNEL_CALLBACKFLAG_SESSION_START,pRTPSessionInfo->iSdpIndex,0);
	}
	if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_VIDEO)
    {
    	pChannel->fCallbackFunct(pChannel->hParentHandle,RTPRTCPCHANNEL_CALLBACKFLAG_FORCE_I_FRAME,(void*)pChannel->stMulticast[aulMsgBuf[2]-1].iCodecIndex,0);
    }

    ulNonBlock=1;
	ioctlsocket(pChannel->stMulticast[aulMsgBuf[2]-1].sktRTP,FIONBIO,&ulNonBlock);
	ioctlsocket(pChannel->stMulticast[aulMsgBuf[2]-1].sktRTCP,FIONBIO,&ulNonBlock);

	pChannel->stMulticast[aulMsgBuf[2]-1].hRTPRTCPComposerHandle = (HANDLE)pRTPSessionInfo->hRTPRTCPComposerHandle;				
	ulMsg[0] = aulMsgBuf[1];
	OSMsgQueue_Send((HANDLE)pChannel->qSessionInfo,(DWORD*)ulMsg);
	
	if( pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_VIDEO )
        printf("[Channel]: multicast video session added\r\n");
	//20120816 modified by Jimmy for metadata
    else if( pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_AUDIO)
        printf("[Channel]: multicast audio session added\r\n");
	//20120816 added by Jimmy for metadata
    else if( pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_METADATA)
		printf("[Channel]: multicast metadata session added\r\n");
    
	return 0;
}

DWORD ChannelLoopRemoveMulticast(HANDLE hChannel, int iIndex)
{
	CHANNEL *pChannel;

	if(!hChannel)
	{
		return CHANNEL_ERR_NOTOPEN;
    }		

	pChannel=(CHANNEL *)hChannel;
	            
	RTPRTCPChannel_CloseMulticast(pChannel, iIndex - 1);
	
	return 0;

}

//20100714 Added by danny For Multicast parameters load dynamically
DWORD RTPRTCPChannel_CheckMulticastAddAvailable(HANDLE hChannel, int iMulticastCount)
{
	CHANNEL *pChannel;

	if(!hChannel)
	{
		return CHANNEL_ERR_NOTOPEN;
    }		

	pChannel=(CHANNEL *)hChannel;
	printf("[%s]iMulticastCount %d, iStatus %d\n", __FUNCTION__, iMulticastCount, pChannel->stMulticast[iMulticastCount].iStatus);
	if( pChannel->stMulticast[iMulticastCount].iStatus == SESSION_IDLE )
	{
		return S_OK;
	}
	
	return S_FAIL;
}

int RTPRTCPChannel_AddMulticastSession(HANDLE hRTPRTCPChannelHandle, RTPRTCPCHANNEL_CONNECTION * pstRTPRTCPSession,int iGroupIndex)
{
    CHANNEL			*pChannel;
	ULONG			ulResult;
	unsigned long	aulMsgBuf[4];	
	int				iCount=0;
	
	if(!hRTPRTCPChannelHandle)
	{
		return CHANNEL_ERR_NOTOPEN;
    }
    
	pChannel=(CHANNEL *)hRTPRTCPChannelHandle;
	
    if(pstRTPRTCPSession)
    {
        memset(aulMsgBuf,0,4*sizeof(unsigned long));	

		ulResult = OSMsgQueue_Receive((HANDLE)pChannel->qSessionInfo, (DWORD*)aulMsgBuf,10);
		if( ulResult != 0 )
		{
			DbgLog((dfCONSOLE|dfINTERNAL,"[Channel] Add One connect failed(no Info buffer), %d\n",ulResult));
			return -1;
		}
		memcpy((void*)aulMsgBuf[0],(void*)pstRTPRTCPSession,sizeof(RTPRTCPCHANNEL_CONNECTION));

	    aulMsgBuf[1] = aulMsgBuf[0];
		aulMsgBuf[0] = RTPRTCP_ADD_MULTICAST;
		aulMsgBuf[2] = iGroupIndex; 
		aulMsgBuf[3] = 0;

    }
    else
        return CHANNEL_ERR_PARAM_NULL;

    while(1)
    {
	    ulResult=OSMsgQueue_Send((HANDLE)pChannel->qAddRemoveConnection, (DWORD*)aulMsgBuf);
	
		iCount ++;
	    if(ulResult)
	    {
            DbgLog((dfCONSOLE|dfINTERNAL,"[Channel] Add One connect failed, %d\n",ulResult));
            OSSleep_MSec(200);			
		}
		else
		{
            DbgLog((dfCONSOLE|dfINTERNAL,"[Channel] Add One connect OK ID=%lu\n",aulMsgBuf[0]));
		    break;
		}

		if( iCount > 10)
			return -1;
	}	    

	pChannel->nAddRemoveonnectionCount++;

	return 0;
}
#endif

int RTPRTCPChannel_AddOneSession(HANDLE hRTPRTCPChannelHandle, RTPRTCPCHANNEL_CONNECTION * pstRTPRTCPSession)
{
    CHANNEL *pChannel;
	ULONG ulResult;
	unsigned long aulMsgBuf[4];	
	int     iCounter =0;
	
	if(!hRTPRTCPChannelHandle)
	{
		return CHANNEL_ERR_NOTOPEN;
    }
    
	pChannel=(CHANNEL *)hRTPRTCPChannelHandle;
	
    if(pstRTPRTCPSession)
    {
        memset(aulMsgBuf,0,4*sizeof(unsigned long));	

		ulResult = OSMsgQueue_Receive((HANDLE)pChannel->qSessionInfo, (DWORD*)aulMsgBuf,10);
		if( ulResult != 0 )
		{
#ifdef _LINUX
			//syslog(LOG_ERR,"[Channel] Add One connect failed(no Info buffer), %d\n",(int)ulResult);
#endif // _LINUX
			TelnetShell_DbgPrint("[Channel] Add One connect failed(no Info buffer), %d\n",(int)ulResult);
			return -1;
		}
		memcpy((void*)aulMsgBuf[0],(void*)pstRTPRTCPSession,sizeof(RTPRTCPCHANNEL_CONNECTION));

	    aulMsgBuf[1] = aulMsgBuf[0];
		aulMsgBuf[0] = RTPRTCP_ADD_SESSION;
		aulMsgBuf[2] = 0;
		aulMsgBuf[3] = 0;

    }
    else
        return CHANNEL_ERR_PARAM_NULL;

    while(1)
    {
        iCounter++ ;        
	    ulResult=OSMsgQueue_Send((HANDLE)pChannel->qAddRemoveConnection, (DWORD*)aulMsgBuf);
	
	    if(ulResult)
	    {
//		    DbgPrint(("[Channel] Add One connect failed, %d\n",ulResult));
#ifdef _LINUX
			//syslog(LOG_ERR,"[Channel] Add One connect failed, %d\n",(int)ulResult);
#endif //_LINUX
            DbgLog((dfCONSOLE|dfINTERNAL,"[Channel] Add One connect failed, %d\n",ulResult));
            OSSleep_MSec(200);			
		}
		else
		{
            DbgLog((dfCONSOLE|dfINTERNAL,"[Channel] Add One connect OK ID=%lu\n",aulMsgBuf[0]));
		    break;
		}
		
		if( iCounter > 10 )
		    return -2;
	}	    

	pChannel->nAddRemoveonnectionCount++;

	return 0;
}



DWORD	ChannelLoopCheckOutsideConnectQueue(HANDLE hChannel)
{
	CHANNEL *pChannel;
	ULONG ulResult;
	unsigned long aulMsgBuf[4];	

	if(!hChannel)
	{
		return CHANNEL_ERR_NOTOPEN;
    }		

	pChannel=(CHANNEL *)hChannel;
	

	while(1)
	{
		memset(aulMsgBuf,0,4*sizeof(unsigned long));	
	    ulResult=OSMsgQueue_Receive((HANDLE)pChannel->qAddRemoveConnection,(DWORD*)aulMsgBuf,0);

        if(ulResult==S_OK)
        {
            pChannel->nAddRemoveonnectionCount--;
		
            if(aulMsgBuf[0] == RTPRTCP_REMOVE_SESSION)   //remove one session
            {
#ifdef _LINUX
                //syslog(LOG_ERR,"q-receive remove\n");
#endif //_LINUX
                ChannelLoopRemoveOneConnect(hChannel,(DWORD)aulMsgBuf[1]);                
            }
            else if( aulMsgBuf[0] == RTPRTCP_PAUSE_SESSION) // Pause one session
            {
            	ChannelLoopPauseOneConnect(hChannel,(DWORD)aulMsgBuf[1]);
        	}
        	else if( aulMsgBuf[0] == RTPRTSP_RESUME_SESSION) // Resume one session
        	{
        		ChannelLoopResumeOneConnect(hChannel,(DWORD)aulMsgBuf[1]);
        	}
            else if( aulMsgBuf[0] == RTPRTCP_ADD_SESSION) // Add one session        	
            {
#ifdef _LINUX
                //syslog(LOG_ERR,"q-receive add\n");
#endif //_LINUX
                ChannelLoopAddOneConnect(hChannel,(unsigned long*)aulMsgBuf);
            }
#ifdef RTSPRTP_MULTICAST
			else if( aulMsgBuf[0] == RTPRTCP_ADD_MULTICAST) // Add multicast        	
			{
                ChannelLoopAddMulticast(hChannel,(unsigned long*)aulMsgBuf);
			}
			else if( aulMsgBuf[0] == RTPRTCP_REMOVE_MULTICAST) // remove multicast        	
			{
                ChannelLoopRemoveMulticast(hChannel, aulMsgBuf[1]);
			}
#endif
        }
        else 
        {
//            printf("[Channel] Outside Connect Queue error or empty, %d",ulResult);
            return 0;
        }
        
	}

	return 0;
}


int RTPRTCPChannel_Release(HANDLE hRTPRTCPChannelHandle)
{
	CHANNEL			*pChannel;
	unsigned long	ulMsg[4];
	//DWORD			dwExitCode;

	if(!hRTPRTCPChannelHandle)
	{
		return CHANNEL_ERR_NOTOPEN;
    }

	pChannel=(CHANNEL *)hRTPRTCPChannelHandle;
	
/*	while(pChannel->nChannelStatus!=CHANNEL_STOPPED)
	{
		if(nCount<20)
		{
		    nCount ++;
			OSSleep_MSec(200);	
        }			
		else
		{
			return 1;
        }			
	}*/


	while( OSMsgQueue_Receive((HANDLE)pChannel->qSessionInfo, (DWORD*)ulMsg,0) == S_OK)
	{
		if( (void*)ulMsg[0] != NULL )
			free((void*)ulMsg[0]);
	}

    OSMsgQueue_Release((HANDLE*)(&pChannel->qAddRemoveConnection));
	OSMsgQueue_Release((HANDLE*)(&pChannel->qSessionInfo));

	free((void*)pChannel->pRTPTmpMediaBuffer );
	//free((void*)pChannel->pcMp4EncodeParam);
    free((void*)pChannel->session);	


	/*if( pChannel->nChannelStatus == CHANNEL_STOPPED )
		printf("======media channel stopped======\n");*/
	
	if( pChannel->hThread )
	{
		/*if (OSThread_WaitFor(pChannel->hThread, 5000, &dwExitCode) != S_OK)
		{
			OSThread_Terminate(pChannel->hThread);
		}*/
		OSThread_Release((HANDLE*)&pChannel->hThread);
	}

	memset(pChannel,0,sizeof(CHANNEL) );
	free((void*)pChannel);

	return 0;
}


int	RTPRTCPChannel_ComposeEmbeddedRTPInfo(char* pcBuffer, int iIdentifier, int iLength)
{
	char *pChar;
	short sLength;

	pChar = pcBuffer - 4;
	*pChar = '$';
	*(pChar+1) = (char)iIdentifier;
	sLength = htons(iLength);
	memcpy((void*)(pChar+2),(void*)(&sLength),2);
		
	return 0;
}

#ifdef RTSPRTP_MULTICAST
int MulticastMedia(RTPRTCPCHANNEL_MULTICAST *pMulticast, RTPMEDIABUFFER	*pMediaBuffer,CHANNEL *pChannel)
{

	int nSendSize=0;
	SOCKET sSocket;
	char *pcSendBuf;
	int nBufSize,iHeaderLen;
    int iCounter,nResult;
    //static int iForceI=0;
    struct timeval tv;
    fd_set fdsConnect;
    //char buffer[1500];
 
       
    if( pMulticast->hRTPRTCPComposerHandle == NULL)
    {     
		printf("[Channel]: RTPRTCP handle error (mulitcast)\r\n");			    
       return CHANNEL_ERR_NOTOPEN;
    }                      
     
	RTPRTCPComposer_RTPHeaderComposer(pMulticast->hRTPRTCPComposerHandle,pMediaBuffer);

	if( pMediaBuffer->dwExtensionLen > 0 )
		iHeaderLen = 12 + 4 + pMediaBuffer->dwExtensionLen;
	else
		iHeaderLen = 12 ;


	nBufSize=pMediaBuffer->dwBytesUsed;
	pcSendBuf=(char *)pMediaBuffer->pbDataStart;
	
	iCounter=0;

	while(nBufSize>0 )
	{		   	    
		sSocket=pMulticast->sktRTP;
        
        if( iCounter > 500 )
        {
			pMediaBuffer->pbDataStart += iHeaderLen;
			pMediaBuffer->dwBytesUsed -= iHeaderLen;

			printf("[Channel]: send out retry abort (multicast)\r\n");
		
            return -1;
        }
        
        if(sSocket<0)
        {        
			printf("[Channel]: Socket has closed (multicast)\r\n");            
			pMediaBuffer->pbDataStart += iHeaderLen;
			pMediaBuffer->dwBytesUsed -= iHeaderLen;

            return -1;
        }	
        
        FD_ZERO(&fdsConnect);
		FD_SET(sSocket, &fdsConnect);
	    
		tv.tv_sec = 1;
		tv.tv_usec = 0;

   		nResult=select(sSocket + 1,NULL ,&fdsConnect, NULL, &tv);
			
		if( nResult > 0 )
		{		    			
            nSendSize=send(sSocket, pcSendBuf,nBufSize, SEND_FLAGS);
			
            if(nSendSize < 0)
            {
#if defined (_WIN32_)
                if(WSAGetLastError() == WSAEWOULDBLOCK )
#elif defined(_PSOS_TRIMEDIA) || defined (_LINUX)          
                if (errno == EWOULDBLOCK )
#endif            
                {
                    OSSleep_MSec(20);	
#ifdef _PSOS_TRIMEDIA                    
                    TelnetShell_DbgPrint(" EWOULDBLOCK\r\n");
#endif                    
                    nSendSize=0;
                }
                else
                {
                    
                    printf("Multicast Outsend fail \r\n");
                    
					pMediaBuffer->pbDataStart += iHeaderLen;
		            pMediaBuffer->dwBytesUsed -= iHeaderLen;		            

				    return -1;
                }
            }
            else if( nSendSize==0)
            {
                OSSleep_MSec(20);	
            }
            
            nBufSize-=nSendSize;
            pcSendBuf+=nSendSize;
        }           
        iCounter++;        
	}
	
    RTPRTCPComposer_UpdateSenderReport(pMulticast->hRTPRTCPComposerHandle,pMediaBuffer->dwBytesUsed-iHeaderLen);      
 
	pMediaBuffer->pbDataStart += iHeaderLen;
	pMediaBuffer->dwBytesUsed -= iHeaderLen;
		
	return 0;
}
#endif

int OutSendMedia(RTPRTCPCHANNEL_SESSION *pSession, RTPMEDIABUFFER	*pMediaBuffer,char* pcMp4Param,int nMp4EncodeLength, int MediaType,CHANNEL *pChannel)
{

	int nSendSize=0;
	//20110706 Modified by danny For TCP/HTTP mode socket not sync in multi thread
	int sSocket=*pSession->psktRTP;
	char *pcSendBuf;
	int nBufSize,iHeaderLen;
    int iCounter,nResult;
    static int iForceI=0;
    struct timeval tv;
    fd_set fdsConnect;
    
    if( pSession->hRTPRTCPComposerHandle == NULL)
    {
#ifdef _PSOS_TRIMEDIA        
       TelnetShell_DbgPrint("[Channel] RTPRTCP handle error\r\n");			
#endif       
       return CHANNEL_ERR_NOTOPEN;
    }       
    
	// make sure the first packet of video is I frame
	//20130725 modified by Jimmy to avoid sending audio RTCP before first RTP packet
    if(pMediaBuffer->bIFrame == 1)
    {
        //mark this session is started!
        RTPRTCPComposer_SetValidate(pSession->hRTPRTCPComposerHandle);
    }

    if( RTPRTCPComposer_GetValidate(pSession->hRTPRTCPComposerHandle) == 0 
        && MediaType == RTPRTCPCHANNEL_MEDIATYPE_VIDEO )
    {
        if( iForceI%10 == 0)     
        {
			pChannel->fCallbackFunct(pChannel->hParentHandle,RTPRTCPCHANNEL_CALLBACKFLAG_FORCE_I_FRAME,(void*)pSession->iCodecIndex,0);            
        }     
        iForceI ++;
	    return 0;
	}    

	RTPRTCPComposer_RTPHeaderComposer(pSession->hRTPRTCPComposerHandle,pMediaBuffer);

    if( pSession->iRTPStreamingType == RTP_OVER_TCP )
	{
		RTPRTCPChannel_ComposeEmbeddedRTPInfo((char*)pMediaBuffer->pbDataStart, pSession->iEmbeddedRTPID, pMediaBuffer->dwBytesUsed);
		pMediaBuffer->dwBytesUsed = pMediaBuffer->dwBytesUsed + 4 ;
		pMediaBuffer->pbDataStart = pMediaBuffer->pbDataStart - 4 ;
	}

	if( pSession->iRTPStreamingType == RTP_OVER_TCP )
	{
		if( pMediaBuffer->dwExtensionLen > 0 )
			iHeaderLen = 12 + 4 + 4 + pMediaBuffer->dwExtensionLen;
		else
			iHeaderLen = 12 + 4;
	}
	else
	{
		if( pMediaBuffer->dwExtensionLen > 0 )
			iHeaderLen = 12 + 4 + pMediaBuffer->dwExtensionLen;
		else
			iHeaderLen = 12 ;
	}

	nBufSize=pMediaBuffer->dwBytesUsed;
	pcSendBuf=(char *)pMediaBuffer->pbDataStart;
	
	iCounter=0;

    if(pSession->iRTPStreamingType == RTP_OVER_TCP )
        OSCriticalSection_Enter(pSession->hTCPMuxCS);
	
	while(nBufSize>0 )
	{		   	
		//20110706 Modified by danny For TCP/HTTP mode socket not sync in multi thread
		if( pSession->psktRTP != NULL && *pSession->psktRTP > 0 )
			sSocket=*pSession->psktRTP;	
		else
			sSocket= pChannel->iUDPRTPSock;

        if( iCounter > 1000 )
        {
			if( pSession->iRTPStreamingType == RTP_OVER_TCP )
			{
				pMediaBuffer->pbDataStart += iHeaderLen;
				pMediaBuffer->dwBytesUsed -= iHeaderLen;
		        OSCriticalSection_Leave(pSession->hTCPMuxCS);

			}
			else
			{
				pMediaBuffer->pbDataStart += iHeaderLen;
				pMediaBuffer->dwBytesUsed -= iHeaderLen;
			}
#ifdef _LINUX
			//syslog(LOG_ERR, "send out retry abort\n");
#endif //_LINUX
			printf("media channel: send out retry abort!!\n");		
            return -1;
        }
        
/*        if(sSocket<0)
        {
#ifdef _PSOS_TRIMEDIA            
            TelnetShell_DbgPrint("[Channel] Socket has closed\r\n");
#endif            
			if( pSession->iRTPStreamingType == RTP_OVER_TCP )
			{
				pMediaBuffer->pbDataStart += iHeaderLen;
				pMediaBuffer->dwBytesUsed -= iHeaderLen;
		        OSCriticalSection_Leave(pSession->hTCPMuxCS);


			}
			else
			{
				pMediaBuffer->pbDataStart += iHeaderLen;
				pMediaBuffer->dwBytesUsed -= iHeaderLen;
			}
            return -1;
        }	*/
        
        FD_ZERO(&fdsConnect);
		FD_SET(sSocket, &fdsConnect);
	    
		// Moved by cchuang, 2005/07/25
		tv.tv_sec = 0;
		tv.tv_usec = 1000;

   		nResult = select(sSocket + 1,NULL ,&fdsConnect, NULL, &tv);
			
		if( nResult > 0 )
		{	
			//20110706 Modified by danny For TCP/HTTP mode socket not sync in multi thread
			if( pSession->psktRTP != NULL && *pSession->psktRTP > 0 )
				nSendSize = send(sSocket, pcSendBuf,nBufSize, SEND_FLAGS);
			else
				nSendSize = sendto(pChannel->iUDPRTPSock, pcSendBuf,nBufSize, SEND_FLAGS,(struct sockaddr *)&pSession->RTPNATAddr,sizeof(RTSP_SOCKADDR));
	
            if(nSendSize < 0)
            {

#if defined (_WIN32_)
                if(WSAGetLastError() == WSAEWOULDBLOCK )
#elif defined(_PSOS_TRIMEDIA) || defined(_LINUX)
                if (errno == EWOULDBLOCK )
#endif            
                {
                    OSSleep_MSec(20);	             
                    printf("EWOULDBLOCK\r\n");                 
                    nSendSize=0;
                }
				else if(errno == ECONNREFUSED && pSession->iRTPStreamingType == RTP_OVER_UDP)
				{
					//20090326 ICMP error should be ignored for UDP, this will enable quicktime protocol rolling
					pMediaBuffer->pbDataStart += iHeaderLen;
		            pMediaBuffer->dwBytesUsed -= iHeaderLen;
					//20090403 timeout mechanism
					if(pSession->dwQTTimeoutInitial == 0)
					{
						OSTick_GetMSec(&pSession->dwQTTimeoutInitial);
					}
					else
					{
						DWORD		dwNow = 0, dwOffset = 0;
						OSTick_GetMSec(&dwNow);
						dwOffset = rtspCheckTimeDifference(pSession->dwQTTimeoutInitial, dwNow);
						if(dwOffset > 20000)
						{
							return -1;
						}
					}
					return 0;
				}
                else
                {             
				    if( pSession->iRTPStreamingType != RTP_OVER_UDP )
				    {
	        			pMediaBuffer->pbDataStart += iHeaderLen;
			    		pMediaBuffer->dwBytesUsed -= iHeaderLen;
   				        OSCriticalSection_Leave(pSession->hTCPMuxCS);
				    }
				    else
				    {
					    pMediaBuffer->pbDataStart += iHeaderLen;
		                pMediaBuffer->dwBytesUsed -= iHeaderLen;		            
				    }

				    return -1;
                }
            }
            else if( nSendSize==0)
            {
                OSSleep_MSec(20);	
            }
                        
            nBufSize-=nSendSize;
            pcSendBuf+=nSendSize;
        }    		
		else if( nResult < 0 )
		{
			if( pSession->iRTPStreamingType == RTP_OVER_TCP )
			{
	        	pMediaBuffer->pbDataStart += iHeaderLen;
				pMediaBuffer->dwBytesUsed -= iHeaderLen;
   			    OSCriticalSection_Leave(pSession->hTCPMuxCS);
		    }
		    else
		    {
			    pMediaBuffer->pbDataStart += iHeaderLen;
                pMediaBuffer->dwBytesUsed -= iHeaderLen;		            
		    }

		    return -1;
		}
		
        iCounter++;        
	}
	
	if( pSession->iRTPStreamingType == RTP_OVER_TCP )
	{
        OSCriticalSection_Leave(pSession->hTCPMuxCS);
	    RTPRTCPComposer_UpdateSenderReport(pSession->hRTPRTCPComposerHandle,pMediaBuffer->dwBytesUsed-iHeaderLen);
	}    
	else
	    RTPRTCPComposer_UpdateSenderReport(pSession->hRTPRTCPComposerHandle,pMediaBuffer->dwBytesUsed-iHeaderLen);      
 
	if( pSession->iRTPStreamingType == RTP_OVER_TCP )
	{
		pMediaBuffer->pbDataStart += iHeaderLen;
		pMediaBuffer->dwBytesUsed -= iHeaderLen;
	}
	else
	{
		pMediaBuffer->pbDataStart += iHeaderLen;
		pMediaBuffer->dwBytesUsed -= iHeaderLen;
	}
		

	return 0;
}

//20110725 Add by danny For Multicast RTCP receive report keep alive
BOOL Check_Is_Timeout(DWORD dwFirstTime, DWORD dwTimeoutInMSec, DWORD *dwPassTime)
{
	// declarations
	DWORD	dwCurrentTime;
	
	OSTick_GetMSec(&dwCurrentTime);
	
	if(dwCurrentTime >= dwFirstTime)
	{
		if(dwPassTime != NULL)
		{
			*dwPassTime = (dwCurrentTime - dwFirstTime);
		}
		if((dwCurrentTime - dwFirstTime) > (dwTimeoutInMSec))
		{
			return TRUE;
		}
	}
	else
	{
		if(dwPassTime != NULL)
		{
			*dwPassTime = (dwFirstTime - dwCurrentTime);
		}
		if((dwFirstTime - dwCurrentTime) > (dwTimeoutInMSec))
		{
			return TRUE;
		}
	}
	
	return FALSE;
	
}

DWORD	ChannelSendMediaToEveryConnect(CHANNEL *pChannel,RTPMEDIABUFFER * pMediaBuffer )
{
	int nResult;	
    int i;
    DWORD dwSessionID;
	
	for(i=0 ; i <pChannel->iMaxSession ; i ++ )
	{
     	if(pChannel->session[i].dwSessionID != 0 &&
			pChannel->session[i].iCodecIndex == pMediaBuffer->dwStreamIndex &&
     		pChannel->session[i].iStatus != SESSION_PAUSED )
		{
			//send no RTP extension to none-vvtk player
			if( pChannel->session[i].iVivotekClient == 0 && pMediaBuffer->dwExtensionLen > 0 )
			{
				nResult=OutSendMedia(&pChannel->session[i], pChannel->pRTPTmpMediaBuffer,pChannel->pcMp4EncodeParam,pChannel->nMp4EncodeLength,pChannel->nChannelType,pChannel);
			}
			else
			{
				nResult=OutSendMedia(&pChannel->session[i], pMediaBuffer,pChannel->pcMp4EncodeParam,pChannel->nMp4EncodeLength,pChannel->nChannelType,pChannel);
			}	

			if(nResult==-1)
			{
				if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_VIDEO)
				{
#ifdef _PSOS_TRIMEDIA
					TelnetShell_DbgPrint("--video channel removed:ID %u socket %d--\r\n",pChannel->session[i].dwSessionID, *pChannel->session[i].psktRTP);
#endif                    
				}
				//20120816 modified by Jimmy for metadata
				else if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_AUDIO)
				{
#ifdef _PSOS_TRIMEDIA
					TelnetShell_DbgPrint("audio channel removed:ID %u socket %d\r\n",pChannel->session[i].dwSessionID, *pChannel->session[i].psktRTP);
#endif                   
				}
				//20120816 added by Jimmy for metadata
				else if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_METADATA)
				{
#ifdef _PSOS_TRIMEDIA
					TelnetShell_DbgPrint("metadata channel removed:ID %u socket %d\r\n",pChannel->session[i].dwSessionID, *pChannel->session[i].psktRTP);
#endif                   
				}

                dwSessionID=pChannel->session[i].dwSessionID;		  					   	   

				RTPRTCPChannel_InitSession(&pChannel->session[i]);

                pChannel->fCallbackFunct(pChannel->hParentHandle
		                       ,RTPRTCPCHANNEL_CALLBACKFLAG_SESSION_CLOSED
		  					   ,(void *)dwSessionID,0);

                pChannel->nTotalConnectionCount -- ;			

			}
		}

	}

	return 0;

}

#ifdef RTSPRTP_MULTICAST
int  ChannelMulticastRTCP(RTPRTCPCHANNEL_MULTICAST *pMulticast)
{
    char acRTCPBuffer[100];
    int iRTCPLen;

    int nSendSize=0;
    SOCKET sSocket;
    char *pcSendBuf;
    int nBufSize,nResult;
    int iCounter;
    struct timeval tv;
    fd_set fdsConnect;  
	HANDLE hRTPRTCPComposerHandle = pMulticast->hRTPRTCPComposerHandle;
 
    if( hRTPRTCPComposerHandle == NULL)
    {
        return CHANNEL_ERR_NOTOPEN;
    }        

	sSocket = pMulticast->sktRTCP;
    RTPRTCPComposer_CreateRTCPSenderReport(hRTPRTCPComposerHandle, acRTCPBuffer+4, &iRTCPLen);

	nBufSize=iRTCPLen;
	pcSendBuf=acRTCPBuffer+4;

    if( sSocket < 0 )
    {
        return -1;
    }      

    iCounter=0;
        
	while(nBufSize>0)
	{	
		if( iCounter > 3000 || sSocket<0)
		{
		    return -1;
        }      
			
		FD_ZERO(&fdsConnect);
		FD_SET(sSocket, &fdsConnect);
	    
		// Moved by cchuang, 2005/07/25
		tv.tv_sec = 0;
		tv.tv_usec = 0;

   		nResult=select(sSocket + 1, NULL,&fdsConnect, NULL, &tv);
			
		if( nResult > 0 )
		{
			//20110725 Add by danny For Multicast RTCP receive report keep alive
			//nSendSize=send(sSocket, pcSendBuf,nBufSize, SEND_FLAGS);
			nSendSize = sendto(sSocket, pcSendBuf, nBufSize, SEND_FLAGS, (struct sockaddr *)&pMulticast->RTCPDstAddr, sizeof(pMulticast->RTCPDstAddr));
			
		    if(nSendSize==-1)
		    {
#if defined (_WIN32_)
                if(WSAGetLastError() == WSAEWOULDBLOCK )
#elif defined(_PSOS_TRIMEDIA) || defined(_LINUX)
                if (errno == EWOULDBLOCK )
#endif
     		    {
                    OSSleep_MSec(10);
//                  DbgPrint((" EWOULDBLOCK \n"));
                    nSendSize=0;
       	    	}
       		    else
       		    {
#ifdef _PSOS_TRIMEDIA       		        
                    TelnetShell_DbgPrint("RTCP Outsend fail\r\n");
#endif                
                    return 1; // return 0 !! RTCP won't bind any port till RReport is received 
       		    }
		    }
		    else if(nSendSize==0)
                OSSleep_MSec(10);
                
            nBufSize-=nSendSize;
            pcSendBuf+=nSendSize;    
        }
		iCounter++;        
	}

	return 0;

}

int	ChannelRecvMulticastRTCP(RTPRTCPCHANNEL_MULTICAST *pMulticast)
{
    char acRTCPBuffer[1500];
    //int iRTCPLen;

    SOCKET sSocket;
    int nResult,nRecvBytes;
    struct timeval tv;
    fd_set fdsConnect;  
	HANDLE hRTPRTCPComposerHandle = pMulticast->hRTPRTCPComposerHandle;
 
    if( hRTPRTCPComposerHandle == NULL)
    {
        return CHANNEL_ERR_NOTOPEN;
    }        
    
	sSocket = pMulticast->sktRTCP;

    if( sSocket < 0 )
    {
        return -1;
    }      

	tv.tv_sec = 0;
	tv.tv_usec = 0;
    
    while(1)
    {    
	    FD_ZERO(&fdsConnect);
	    FD_SET(sSocket, &fdsConnect);
	    
   	    nResult=select(sSocket + 1,&fdsConnect,NULL, NULL, &tv);
			
	    if( nResult > 0 )
	    {
            nRecvBytes=recv(sSocket, acRTCPBuffer,1500, RECV_FLAGS);
            TelnetShell_DbgPrint("[Media Channel]Recv %d RRReort\r\n",nRecvBytes);
			//20110725 Add by danny For Multicast RTCP receive report keep alive
			if(pMulticast->iRRAlive == TRUE && nRecvBytes > 0 )
			{
				OSTick_GetMSec(&pMulticast->dwRecvReportTime);
			}
			//if( nRecvBytes < 1500 )  
            //    RTPRTCPComposer_ParseRTCPPacket(acRTCPBuffer,nRecvBytes,hRTPRTCPComposerHandle);
			if( nRecvBytes < 0 )
				break;
        }
        else if(nResult == 0 )
        {
            break;
        }
        else
        {
            return -1;
        }            
    }
    
    return 0;
}

#endif

DWORD GetMSecondsSinceRoot(void)
{
    static DWORD  g_dwInitSeconds = 0;
    static DWORD  g_dwInitMSeconds = 0;
    DWORD  dwSeconds,dwMSeconds,dwTimeDiff;
    unsigned long ulTime;
    
    if( g_dwInitSeconds == 0)
    {
        OSTime_GetTimer(&g_dwInitSeconds,&g_dwInitMSeconds);
        return 0;
    }
    
    OSTime_GetTimer(&dwSeconds,&dwMSeconds);
    dwTimeDiff = dwSeconds - g_dwInitSeconds;
    
   	if( dwMSeconds >= g_dwInitMSeconds )
  		ulTime=dwTimeDiff*1000+(dwMSeconds-g_dwInitMSeconds);
  	else
  		ulTime=dwTimeDiff*1000-(g_dwInitMSeconds-dwMSeconds);

    return ulTime;
    
}

DWORD  THREADAPI ChannelSendLoop(DWORD dwParam)
{
	CHANNEL *pChannel=(CHANNEL *)dwParam;
	DWORD dwNowTime,dwNowTimeMulti;
	DWORD dwPrevTime,dwPrevTimeMulti;
	int i = 0;
	HANDLE hChannel=(HANDLE)pChannel;
#ifndef _SHARED_MEM
	RTPMEDIABUFFER *pMediaBuffer=NULL,*pMSendBuff=NULL;
#endif

#ifdef _LINUX
	//syslog(LOG_ERR, "[ChannelSend] pid is %d\n", getpid());
	//syslog(LOG_INFO, "[ChannelSend] pid is %d\n", getpid());
#endif //_LINUX	
	dwPrevTime=dwPrevTimeMulti=GetMSecondsSinceRoot();

	while(pChannel->nChannelStatus == CHANNEL_RUNNING)
	{
    	ChannelLoopCheckOutsideConnectQueue(hChannel);
#ifndef _SHARED_MEM
		if( pChannel->fCallbackFunct(pChannel->hParentHandle
		                       ,RTPRTCPCHANNEL_CALLBACKFLAG_GET_MEDIABUF
		  					   ,(void *)100,&pMediaBuffer) != 0 )
		{	
			//printf("no media to send\n");
		    continue;
		}
#endif
#ifdef _SHARED_MEM
		//if(pChannel->nTotalConnectionCount > 0 )
		{
			int	iResult = 0;
		
			//Request buffer for each client
			ChannelShmemRequestBuffer(pChannel);
			//Compose buffer for each client
			ChannelShmemCutBuffer(pChannel);
			//Compose RTP header for each client
			ChannelShmemComposeHeader(pChannel);
			//Compose RTP header for multicast 
#ifdef RTSPRTP_MULTICAST
			ChannelShmemMulticastComposeHeader(pChannel);
#endif
			//Attemp to send for each client
			//20140812 modified for Genetec, send the RTCP before the first I-frame is sent
			ChannelSendRTCPToEveryConnection(pChannel); 
			ChannelShmemSend(pChannel);
		
			//RTCP Part, copied from former
			dwNowTime = GetMSecondsSinceRoot();

			if( dwPrevTime > dwNowTime || dwNowTime-dwPrevTime > 500)
			{
				dwPrevTime=dwNowTime;
	
	    		ChannelSendRTCPToEveryConnection(pChannel);
			    ChannelReceiveRTCPReportFromEveryConnection(pChannel);
				ChannelReceiveRTCPReportFromFixedSocket(pChannel);

				//20081001 Check for TCP connection timeout
				ChannelShmemCheckTimeout(pChannel);
		    }

			//Multicast RTCP
#ifdef RTSPRTP_MULTICAST
			dwNowTimeMulti = GetMSecondsSinceRoot();
			//20130327 modified by Jimmy to support different video/audio multicast addresses for ONVIF Test Tool 12.12
			//20130830 modified by Charles for ondemand multicast
			for( i=0 ; i< (RTSP_MULTICASTNUMBER + RTSP_ONDEMAND_MULTICASTNUMBER) ; i++)
			{	    
				if(pChannel->stMulticast[i].iStatus == SESSION_PLAYING)
				{
					if( dwPrevTimeMulti > dwNowTimeMulti || dwNowTimeMulti-dwPrevTimeMulti > 5000)
					{
						dwPrevTimeMulti=dwNowTimeMulti;		
	    				ChannelMulticastRTCP(&pChannel->stMulticast[i]);	    	
	    				ChannelRecvMulticastRTCP(&pChannel->stMulticast[i]);
					}

					//20110725 Add by danny For Multicast RTCP receive report keep alive
					if( pChannel->stMulticast[i].iRRAlive == TRUE && Check_Is_Timeout(pChannel->stMulticast[i].dwRecvReportTime, g_iMulticastTimeout*1000, NULL) )
					{
						if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_VIDEO)
						{
							TelnetShell_DbgPrint("Multicast %d video channel Timeout %d s, socket %d\n", i, g_iMulticastTimeout, pChannel->stMulticast[i].sktRTCP);
						}
						//20120816 modified by Jimmy for metadata
						else if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_AUDIO)
						{
							TelnetShell_DbgPrint("Multicast %d audio channel Timeout %d s, socket %d\n", i, g_iMulticastTimeout, pChannel->stMulticast[i].sktRTCP);
						}
						//20120816 added by Jimmy for metadata
						else if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_METADATA)
						{
							TelnetShell_DbgPrint("Multicast %d metadata channel Timeout %d s, socket %d\n", i, g_iMulticastTimeout, pChannel->stMulticast[i].sktRTCP);
						}

						RTPRTCPChannel_CloseMulticast(pChannel, i);
					}
				}
			}
#endif
			//Check for unfinished RTCP
			for(i=0 ; i <pChannel->iMaxSession ; i ++ )
			{
				if(pChannel->session[i].dwSessionID != 0 && 
				pChannel->session[i].iStatus != SESSION_PAUSED &&
				pChannel->session[i].iRTCPRemainingLength != 0)
				{
					iResult = SendRTCPSenderReport(&pChannel->session[i],pChannel->session[i].hRTPRTCPComposerHandle,pChannel);
					if(iResult == -1)
					{
						printf("RTCP Send remaining abnormal, session closed!\n");
						//20130315 added by Jimmy to log more information
						syslog(LOG_DEBUG, "RTCP Send remaining abnormal, session closed!\n");
						RTPRTCPChannel_CloseSession(pChannel, &pChannel->session[i]);
					}
				}
			}
		}
#else //#ifdef _SHARED_MEM

		
		// copy one media as none rtp extension for none-vvtk player
        if(pMediaBuffer->dwExtensionLen > 0)
		{		    
			memcpy(pChannel->pRTPTmpMediaBuffer,pMediaBuffer,sizeof(RTPMEDIABUFFER)+pMediaBuffer->dwBytesUsed);
			pChannel->pRTPTmpMediaBuffer->dwExtensionLen = 0;
			pChannel->pRTPTmpMediaBuffer->pbBufferStart=pChannel->pRTPTmpMediaBuffer->pbDataStart=(BYTE *)(pChannel->pRTPTmpMediaBuffer+1);	
		}

#ifdef RTSPRTP_MULTICAST
		//20130327 modified by Jimmy to support different video/audio multicast addresses for ONVIF Test Tool 12.12
		//20130830 modified by Charles for ondemand multicast
		for( i=0 ; i< RTSP_MULTICASTNUMBER + RTSP_AUDIO_EXTRA_MULTICASTNUMBER + RTSP_ONDEMAND_MULTICASTNUMBER) ; i++)
		{	    
			if( pChannel->stMulticast[i].iStatus == SESSION_PLAYING &&
				pChannel->stMulticast[i].iCodecIndex == pMediaBuffer->dwStreamIndex)
			{
               		    
			    if( pChannel->stMulticast[i].iVivotekClient == FALSE &&
			        //pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_VIDEO &&
				    pMediaBuffer->dwExtensionLen > 0)
			        pMSendBuff = pChannel->pRTPTmpMediaBuffer;			        		            
			    else
			        pMSendBuff = pMediaBuffer;	
			                        			                           
				if( MulticastMedia(&pChannel->stMulticast[i],pMSendBuff,pChannel) != 0 )
				{
					RTPRTCPChannel_CloseMulticast(pChannel, i);
				}

				dwNowTimeMulti=GetMSecondsSinceRoot();
				if( dwPrevTimeMulti > dwNowTimeMulti || dwNowTimeMulti-dwPrevTimeMulti>5000)
				{
					dwPrevTimeMulti=dwNowTimeMulti;		
	    			ChannelMulticastRTCP(&pChannel->stMulticast[i]);	    	
	    			ChannelRecvMulticastRTCP(&pChannel->stMulticast[i]);
				}	
			}		
		}
#endif
		if(pChannel->nTotalConnectionCount > 0 )
		{
	

			ChannelSendMediaToEveryConnect(pChannel,pMediaBuffer);

 			dwNowTime=GetMSecondsSinceRoot();
 
			if( dwPrevTime > dwNowTime || dwNowTime-dwPrevTime>500)
			{
				dwPrevTime=dwNowTime;
	
	    		ChannelSendRTCPToEveryConnection(pChannel);
			    ChannelReceiveRTCPReportFromEveryConnection(pChannel);

				ChannelReceiveRTCPReportFromFixedSocket(pChannel);
		    }	
			
		}	
	
        pMediaBuffer->pbDataStart = pMediaBuffer->pbBufferStart;
	    pMediaBuffer->dwBytesUsed = 0;
		pChannel->fCallbackFunct(pChannel->hParentHandle
			                    ,RTPRTCPCHANNEL_CALLBACKFLAG_SEND_EMPTYBUF
								,pMediaBuffer,0);
#endif
	}
#ifdef _SHARED_MEM
	ChannelShmReleaseBuffer(pChannel);
#endif

	pChannel->nChannelStatus = CHANNEL_STOPPED;

	//printf(" \n RTCP thread stop ");	

	return 0;
}


int SendRTCPSenderReport(RTPRTCPCHANNEL_SESSION *pSession,HANDLE hRTPRTCPComposerHandle,CHANNEL *pChannel)
{
    int iRTCPLen;

    int nSendSize = 0;
    int sSocket;
    char *pcSendBuf;
    int nBufSize,nResult;
    struct timeval tv;
    fd_set fdsConnect;  
	int	 iCounter = 0;
 
    if( pSession->hRTPRTCPComposerHandle == NULL)
    {
        return CHANNEL_ERR_NOTOPEN;
    }        

	//20110706 Modified by danny For TCP/HTTP mode socket not sync in multi thread
	if( pSession->psktRTCP != NULL && *pSession->psktRTCP > 0 )
		sSocket = *pSession->psktRTCP;
	else
		sSocket = pChannel->iUDPRTCPSock;

	if( sSocket < 0 )
    {
        return -1;
    }     

	if(pSession->iRTCPRemainingLength == 0)
	{
		if( pSession->iRTPStreamingType == RTP_OVER_TCP )
		{
			RTPRTCPComposer_CreateRTCPSenderReport(hRTPRTCPComposerHandle, pSession->acRTCPBuf+4, &iRTCPLen);
			RTPRTCPChannel_ComposeEmbeddedRTPInfo(pSession->acRTCPBuf+4, pSession->iEmbeddedRTCPID,iRTCPLen);	
			nBufSize = iRTCPLen + 4;
			pcSendBuf= pSession->acRTCPBuf;

			pSession->iRTCPRemainingLength = nBufSize;
			pSession->iRTCPOffset = 0;
		}
		else
		{
			RTPRTCPComposer_CreateRTCPSenderReport(hRTPRTCPComposerHandle, pSession->acRTCPBuf, &iRTCPLen);
			nBufSize=iRTCPLen;
			pcSendBuf=pSession->acRTCPBuf;

			pSession->iRTCPRemainingLength = nBufSize;
			pSession->iRTCPOffset = 0;
		}
	}
	else
	{
		nBufSize = pSession->iRTCPRemainingLength;
		pcSendBuf =	pSession->acRTCPBuf + pSession->iRTCPOffset;
	}	

	
   	//Get critical section for TCP
    if(pSession->iRTPStreamingType == RTP_OVER_TCP )
#ifdef _SHARED_MEM
	{
		if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_AUDIO)
		{
			if(RTPRTCPCs_TryEnter(pSession, eCSAudioRTCP) != S_OK)
			{
				return 1;
			}
			else
			{
				//printf("[%s] @@@RTPRTCPCs_TryEnter AudioRTCP %p ok@@@\n", __FUNCTION__, pSession->hTCPMuxCS);
			}
		}
		//20120816 modified by Jimmy for metadata
		else if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_VIDEO)
		{
			if(RTPRTCPCs_TryEnter(pSession, eCSVideoRTCP) != S_OK)
			{
				return 1;
			}
			else
			{
				//printf("[%s] @@@RTPRTCPCs_TryEnter VideoRTCP %p ok@@@\n", __FUNCTION__, pSession->hTCPMuxCS);
			}
		}
		//20120816 added by Jimmy for metadata
		else if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_METADATA)
		{
			if(RTPRTCPCs_TryEnter(pSession, eCSMetadataRTCP) != S_OK)
			{
				return 1;
			}
			else
			{
				//printf("[%s] @@@RTPRTCPCs_TryEnter MetadataRTCP %p ok@@@\n", __FUNCTION__, pSession->hTCPMuxCS);
			}
		}
		else
		{
			return 1;
		}

	}  
#else
        OSCriticalSection_Enter(pSession->hTCPMuxCS);
#endif
		
	
	/* 20080825 Loop marked out by Louis to use non-blocking mode */
	/* 20090116 Share-memory should use non-blocking while non-share memory should use blocking! */
#ifndef _SHARED_MEM
	while(nBufSize>0)
#endif
	{	
		if( iCounter > 3000 )	//Add back 20090116 for non-share memory blocking mode
		{
#ifdef _PSOS_TRIMEDIA		    
		    TelnetShell_DbgPrint("RTCP sendout abort\r\n");			
#endif		    
        	if( pSession->iRTPStreamingType == RTP_OVER_TCP )
	        {   
   		         // OSCriticalSection_Leave(pSession->hTCPMuxCS);
   		        printf("RTCP sendout abort\r\n");
   		        //Modify by Faber, prevent deadlock
	        	if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_AUDIO)
				{
					RTPRTCPCs_Release(pSession, eCSAudioRTCP);
					//printf("[%s] @@@RTPRTCPCs_Release AudioRTCP %p, iSendSize == -1@@@\n", __FUNCTION__, pSession->hTCPMuxCS);
				}
				//20120816 modified by Jimmy for metadata
				else if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_VIDEO)
				{
					RTPRTCPCs_Release(pSession, eCSVideoRTCP);
					//printf("[%s] @@@RTPRTCPCs_Release VideoRTCP %p, iSendSize == -1@@@\n", __FUNCTION__, pSession->hTCPMuxCS);
				}
				//20120816 added by Jimmy for metadata
				else if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_METADATA)
				{
					RTPRTCPCs_Release(pSession, eCSMetadataRTCP);
					//printf("[%s] @@@RTPRTCPCs_Release MetadataRTCP %p, iSendSize == -1@@@\n", __FUNCTION__, pSession->hTCPMuxCS);
				}
            }            
		    return -1;
        }      

		FD_ZERO(&fdsConnect);
		FD_SET(sSocket, &fdsConnect);
	    
		// Moved by cchuang, 2005/07/25
		tv.tv_sec = 0;
		tv.tv_usec = 1000;

   		nResult=select(sSocket + 1, NULL,&fdsConnect, NULL, &tv);
			
		if( nResult > 0 )
		{
			//20110706 Modified by danny For TCP/HTTP mode socket not sync in multi thread
			if( pSession->psktRTCP != NULL && *pSession->psktRTCP > 0 )
    			nSendSize=send(sSocket, pcSendBuf,nBufSize, SEND_FLAGS);
			else 
			{
				//TelnetShell_DbgPrint("--RTCP fixed sockey outsend---\r\n");
#ifdef _INET6
				if ( IN6_IS_ADDR_UNSPECIFIED(&pSession->RTCPNATAddr.sin6_addr)==0 )
#else
				if( pSession->RTCPNATAddr.sin_addr.s_addr != 0 )
#endif
				{
					nSendSize = sendto(pChannel->iUDPRTCPSock, pcSendBuf,nBufSize,SEND_FLAGS,(struct sockaddr *)&pSession->RTCPNATAddr,sizeof(RTSP_SOCKADDR));
					if( nSendSize < 0 )
						printf("error %d\r\n",WSAGetLastError());
				}
#ifndef _SHARED_MEM
				else
					break;
#endif
			}

			if(nSendSize == -1)
		    {
#if defined (_WIN32_)
                if(WSAGetLastError() == WSAEWOULDBLOCK )
#elif defined(_PSOS_TRIMEDIA) || defined(_LINUX)
                if (errno == EWOULDBLOCK )
#endif
     		    {
#ifndef _SHARED_MEM
					OSSleep_MSec(10);
#endif
                    nSendSize=0;
       	    	}
       		    else
       		    {             
       		    	printf("RTCP sendout abort\r\n");
					pSession->iRTCPRemainingLength = 0;
					pSession->iRTCPOffset = 0;

                   	if( pSession->iRTPStreamingType == RTP_OVER_TCP )
	                {   
#ifdef _SHARED_MEM
						if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_AUDIO)
						{
							RTPRTCPCs_Release(pSession, eCSAudioRTCP);
							//printf("[%s] @@@RTPRTCPCs_Release AudioRTCP %p, iSendSize == -1@@@\n", __FUNCTION__, pSession->hTCPMuxCS);
						}
						//20120816 modified by Jimmy for metadata
						else if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_VIDEO)
						{
							RTPRTCPCs_Release(pSession, eCSVideoRTCP);
							//printf("[%s] @@@RTPRTCPCs_Release VideoRTCP %p, iSendSize == -1@@@\n", __FUNCTION__, pSession->hTCPMuxCS);
						}
						//20120816 added by Jimmy for metadata
						else if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_METADATA)
						{
							RTPRTCPCs_Release(pSession, eCSMetadataRTCP);
							//printf("[%s] @@@RTPRTCPCs_Release MetadataRTCP %p, iSendSize == -1@@@\n", __FUNCTION__, pSession->hTCPMuxCS);
						}
						return -1; //if tcp send fail, we should disconnect the session
#else
						OSCriticalSection_Leave(pSession->hTCPMuxCS);
						return -1; //if tcp send fail, we should disconnect the session
#endif
                    }
                    
                    return 1;// Can not return -1 because NAT case will succeed when RReport is received
       		    }
		    }
		    else if( nSendSize == 0 )
			{	//20090116 Share-memory and non-share memory should take different actions!
#ifdef _SHARED_MEM
				if( pSession->iRTPStreamingType == RTP_OVER_TCP )
				{
					if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_AUDIO)
					{
						RTPRTCPCs_Release(pSession, eCSAudioRTCP);
						//printf("[%s] @@@RTPRTCPCs_Release AudioRTCP %p, iSendSize == 0@@@\n", __FUNCTION__, pSession->hTCPMuxCS);
					}
					//20120816 modified by Jimmy for metadata
					else if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_VIDEO)
					{
						RTPRTCPCs_Release(pSession, eCSVideoRTCP);
						//printf("[%s] @@@RTPRTCPCs_Release VideoRTCP %p, iSendSize == 0@@@\n", __FUNCTION__, pSession->hTCPMuxCS);
					}
					//20120816 added by Jimmy for metadata
					else if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_METADATA)
					{
						RTPRTCPCs_Release(pSession, eCSMetadataRTCP);
						//printf("[%s] @@@RTPRTCPCs_Release MetadataRTCP %p, iSendSize == 0@@@\n", __FUNCTION__, pSession->hTCPMuxCS);
					}

				}
				return 1;
#else
				OSSleep_MSec(10);
#endif
			}

            nBufSize-=nSendSize;
            pcSendBuf+=nSendSize;    
			pSession->iRTCPRemainingLength -= nSendSize;
			pSession->iRTCPOffset += nSendSize;
        }       
		iCounter++;	//20090116 Add back to do loop check for non-share memory
	}

	if(pSession->iRTCPRemainingLength == 0)
	{
		if(pSession->iRTPStreamingType == RTP_OVER_TCP )
		{
#ifdef _SHARED_MEM
			if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_AUDIO)
			{
				RTPRTCPCs_Release(pSession, eCSAudioRTCP);
				//printf("[%s] @@@RTPRTCPCs_Release AudioRTCP %p, iRTCPRemainingLength == 0@@@\n", __FUNCTION__, pSession->hTCPMuxCS);
			}
			//20120816 modified by Jimmy for metadata
			else if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_VIDEO)
			{
				RTPRTCPCs_Release(pSession, eCSVideoRTCP);
				//printf("[%s] @@@RTPRTCPCs_Release VideoRTCP %p, iRTCPRemainingLength == 0@@@\n", __FUNCTION__, pSession->hTCPMuxCS);
			}
			//20120816 added by Jimmy for metadata
			else if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_METADATA)
			{
				RTPRTCPCs_Release(pSession, eCSMetadataRTCP);
				//printf("[%s] @@@RTPRTCPCs_Release MetadataRTCP %p, iRTCPRemainingLength == 0@@@\n", __FUNCTION__, pSession->hTCPMuxCS);
			}
#else
			OSCriticalSection_Leave(pSession->hTCPMuxCS);
#endif
		}
		pSession->iRTCPOffset = 0;
		RTPRTCPComposer_IncreaseCountOfMissingReport(hRTPRTCPComposerHandle);
	}
	else
	{
		if(pSession->iRTPStreamingType == RTP_OVER_TCP )
		{
#ifdef _SHARED_MEM
			if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_AUDIO)
			{
				RTPRTCPCs_Release(pSession, eCSAudioRTCP);
				//printf("[%s] @@@RTPRTCPCs_Release AudioRTCP %p, iRTCPRemainingLength != 0@@@\n", __FUNCTION__, pSession->hTCPMuxCS);
			}
			//20120816 modified by Jimmy for metadata
			else if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_VIDEO)
			{
				RTPRTCPCs_Release(pSession, eCSVideoRTCP);
				//printf("[%s] @@@RTPRTCPCs_Release VideoRTCP %p, iRTCPRemainingLength != 0@@@\n", __FUNCTION__, pSession->hTCPMuxCS);
			}
			//20120816 added by Jimmy for metadata
			else if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_METADATA)
			{
				RTPRTCPCs_Release(pSession, eCSMetadataRTCP);
				//printf("[%s] @@@RTPRTCPCs_Release MetadataRTCP %p, iRTCPRemainingLength != 0@@@\n", __FUNCTION__, pSession->hTCPMuxCS);
			}

#else
			printf("RTCP blocking mode send error!\n");
			OSCriticalSection_Leave(pSession->hTCPMuxCS);
#endif
		}
	}

	return 0;
}

void  ChannelSendRTCPToEveryConnection(CHANNEL *pChannel)
{
    int i;
    unsigned long ulSessionID;
    int iResult;
    DWORD dwSessionID;
	
    for(i=0;i<pChannel->iMaxSession;i++)
    {
        ulSessionID= pChannel->session[i].dwSessionID;
        
        // return if video channel are not streaming yet
        //20130725 modified by Jimmy to avoid sending audio RTCP before first RTP packet
        if( RTPRTCPComposer_GetValidate(pChannel->session[i].hRTPRTCPComposerHandle) == 0 )
        {
	        return;
	    } 
     
        if( ulSessionID != 0 && pChannel->session[i].iStatus != SESSION_PAUSED)        
        {
     	   	
            if( RTPRTCPComposer_GetRTCPStartTime(pChannel->session[i].hRTPRTCPComposerHandle) == 0 )			
            {		 
				
				TelnetShell_DbgPrint("First time to send RTCP sender report\r\n");

                RTPRTCPComposer_SetRTCPStartTime(pChannel->session[i].hRTPRTCPComposerHandle);

                iResult=SendRTCPSenderReport(&pChannel->session[i],pChannel->session[i].hRTPRTCPComposerHandle,pChannel) ;	     

                if(iResult==-1)
                {
					if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_VIDEO)
					{
//						DbgPrint(("video channel removed:ID %lu socket %d\n",pChannel->session[i].dwSessionID,pChannel->session[i].sktRTP));	  					   
						DbgLog((dfCONSOLE|dfINTERNAL,"RTCP:video removed:ID %u socket %d\n",pChannel->session[i].dwSessionID, *pChannel->session[i].psktRTCP));
						//20130315 added by Jimmy to log more information
						syslog(LOG_DEBUG, "[ChannelSendRTCPToEveryConnection]:video removed:ID %u socket %d\n",pChannel->session[i].dwSessionID, *pChannel->session[i].psktRTCP);
					}
					//20120816 modified by Jimmy for metadata
					else if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_AUDIO)
					{
//						DbgPrint(("audio channel removed:ID %lu socket %d\n",pChannel->session[i].dwSessionID,pChannel->session[i].sktRTP));	  					   
						DbgLog((dfCONSOLE|dfINTERNAL,"RTCP:audio removed:ID %u socket %d\n",pChannel->session[i].dwSessionID, *pChannel->session[i].psktRTCP));
						//20130315 added by Jimmy to log more information
						syslog(LOG_DEBUG, "[ChannelSendRTCPToEveryConnection]:audio removed:ID %u socket %d\n",pChannel->session[i].dwSessionID, *pChannel->session[i].psktRTCP);
					}
					//20120816 added by Jimmy for metadata
					else if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_METADATA)
					{
//						DbgPrint(("metadata channel removed:ID %lu socket %d\n",pChannel->session[i].dwSessionID,pChannel->session[i].sktRTP));
						DbgLog((dfCONSOLE|dfINTERNAL,"RTCP:metadata removed:ID %u socket %d\n",pChannel->session[i].dwSessionID, *pChannel->session[i].psktRTCP));
						//20130315 added by Jimmy to log more information
						syslog(LOG_DEBUG, "[ChannelSendRTCPToEveryConnection]:metadata removed:ID %u socket %d\n",pChannel->session[i].dwSessionID, *pChannel->session[i].psktRTCP);
					}

                        
                    dwSessionID=pChannel->session[i].dwSessionID;

/*					if( pChannel->session[i].iRTPStreamingType == RTP_OVER_UDP)
						closesocket(pChannel->session[i].sktRTP);
                    pChannel->session[i].sktRTP = -1;		  	 

                    pChannel->session[i].dwSessionID = 0;
                    pChannel->session[i].iStartRTCPSReport = 0;

					if( pChannel->session[i].iRTPStreamingType == RTP_OVER_UDP)
	                    closesocket(pChannel->session[i].sktRTCP);

                    pChannel->session[i].sktRTCP = -1;
                    pChannel->session[i].hRTPRTCPComposerHandle =NULL;   */
					RTPRTCPChannel_InitSession(&pChannel->session[i]);


                    pChannel->fCallbackFunct(pChannel->hParentHandle
		                       ,RTPRTCPCHANNEL_CALLBACKFLAG_SESSION_CLOSED
		  					   ,(void *)dwSessionID,0);
                    pChannel->nTotalConnectionCount -- ;			
#ifdef _LINUX
            //syslog(LOG_ERR, "1906:Remove connection RTP %d\n",pChannel->nTotalConnectionCount);
#endif //_LINUX
                }

            }
            else            
            {

                if( RTPRTCPComposer_IsTimeToReport(pChannel->session[i].hRTPRTCPComposerHandle) ||
				    (pChannel->session[i].iStartRTCPSReport == 0))
                {
                    iResult=SendRTCPSenderReport(&pChannel->session[i],pChannel->session[i].hRTPRTCPComposerHandle,pChannel); 	   
                                        
                    if(iResult==-1)
                    {
						if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_VIDEO)
						{
//							DbgPrint(("video channel removed:ID %lu socket %d\n",pChannel->session[i].dwSessionID,pChannel->session[i].sktRTP));	  					   
							DbgLog((dfCONSOLE|dfINTERNAL,"RTCP:video removed:ID %u socket %d\n",pChannel->session[i].dwSessionID, *pChannel->session[i].psktRTCP));
							//20130315 added by Jimmy to log more information
							syslog(LOG_DEBUG, "[ChannelSendRTCPToEveryConnection]:video removed:ID %u socket %d\n",pChannel->session[i].dwSessionID, *pChannel->session[i].psktRTCP);
						}
						//20120816 modified by Jimmy for metadata
						else if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_AUDIO)
						{
//							DbgPrint(("audio channel removed:ID %lu socket %d\n",pChannel->session[i].dwSessionID,pChannel->session[i].sktRTP));	  					   
							DbgLog((dfCONSOLE|dfINTERNAL,"RTCP:audio removed:ID %u socket %d\n",pChannel->session[i].dwSessionID, *pChannel->session[i].psktRTCP));
							//20130315 added by Jimmy to log more information
							syslog(LOG_DEBUG, "[ChannelSendRTCPToEveryConnection]:audio removed:ID %u socket %d\n",pChannel->session[i].dwSessionID, *pChannel->session[i].psktRTCP);
						}
						//20120816 added by Jimmy for metadata
						else if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_METADATA)
						{
//							DbgPrint(("metadata channel removed:ID %lu socket %d\n",pChannel->session[i].dwSessionID,pChannel->session[i].sktRTP));
							DbgLog((dfCONSOLE|dfINTERNAL,"RTCP:metadata removed:ID %u socket %d\n",pChannel->session[i].dwSessionID, *pChannel->session[i].psktRTCP));
							//20130315 added by Jimmy to log more information
							syslog(LOG_DEBUG, "[ChannelSendRTCPToEveryConnection]:metadata removed:ID %u socket %d\n",pChannel->session[i].dwSessionID, *pChannel->session[i].psktRTCP);
						}						

                        dwSessionID=pChannel->session[i].dwSessionID;	

/*						if( pChannel->session[i].iRTPStreamingType == RTP_OVER_UDP)
							closesocket(pChannel->session[i].sktRTP);
                        pChannel->session[i].sktRTP = -1;	  	   
                        pChannel->session[i].dwSessionID = 0;
                        pChannel->session[i].iStartRTCPSReport = 0;

						if( pChannel->session[i].iRTPStreamingType == RTP_OVER_UDP)
							closesocket(pChannel->session[i].sktRTCP);

                        pChannel->session[i].sktRTCP = -1;
                        pChannel->session[i].hRTPRTCPComposerHandle = NULL; */
						RTPRTCPChannel_InitSession(&pChannel->session[i]);


                        pChannel->fCallbackFunct(pChannel->hParentHandle
		                       ,RTPRTCPCHANNEL_CALLBACKFLAG_SESSION_CLOSED
		  					   ,(void *)dwSessionID,0);
                        pChannel->nTotalConnectionCount -- ;			
#ifdef _LINUX
            //syslog(LOG_ERR, "1949:Remove connection RTP %d\n",pChannel->nTotalConnectionCount);
#endif //_LINUX
                    }	
					else if(iResult == 0 )
						pChannel->session[i].iStartRTCPSReport = 1;

                }  
            }
        }	  
    } 
}

//20101020 Add by danny for support seamless stream TCP/UDP timeout
int check_crash_session(HANDLE hRTPRTCPComposerHandle, BOOL bSeamlessStream)
{
	int iRRCount;

	if( bSeamlessStream == TRUE )
	{
		iRRCount = RTPRTCP_SeamlessStream_MissingRRCount;
	}
	else
	{
		iRRCount = RTPRTCP_MissingRRCount;
	}
	
    //if((RTPRTCPComposer_GetCountOfMissingReport(hRTPRTCPComposerHandle)) > 10)//4 )
    if((RTPRTCPComposer_GetCountOfMissingReport(hRTPRTCPComposerHandle)) > iRRCount)
    {
        printf("Time out client!\n");
//        DbgPrint(("[Channel] Time out client\n"));
        DbgLog((dfCONSOLE|dfINTERNAL,"[Channel] Time out client\n"));
        return 1;
    }	
    else
    {
        return 0;
    }        
}

void ChannelReceiveRTCPReportFromFixedSocket(CHANNEL *pChannel)
{
	int iReady,iLen,iRTCPLen,i;
    fd_set rset;
    char acRTCPBuffer[500];

    struct timeval timeout;
    RTSP_SOCKADDR saddr;
	DWORD dwSSRC;

	memset(acRTCPBuffer, 0, sizeof(acRTCPBuffer));
	while(1)
	{
		FD_ZERO(&rset);
		FD_SET(pChannel->iUDPRTCPSock, &rset);

	    timeout.tv_sec = 0;//1;
	    timeout.tv_usec = 0;//500;

	    iReady = select(pChannel->iUDPRTCPSock+1, &rset, NULL, NULL,&timeout);
		
		if( iReady > 0)
		{						
			//TelnetShell_DbgPrint("Scan Fixed UDP port for RTCP\r\n");

			memset((void*)&saddr,0, sizeof(saddr));
            iLen = sizeof(saddr);
			//CID:1103, CHECKER:TAINTED_SCALAR
            iRTCPLen = recvfrom(pChannel->iUDPRTCPSock,acRTCPBuffer,sizeof(acRTCPBuffer) - 1,RECV_FLAGS,(struct sockaddr *)&saddr,(unsigned int *)&iLen);

			if( iRTCPLen > 0 )
			{
				if( RTPRTCPComposer_ParseRTCPPacketBySSRC(acRTCPBuffer,iRTCPLen,&dwSSRC) == 0 )
				{
					for( i=0 ; i<pChannel->iMaxSession ; i++)
					{

						if( pChannel->session[i].hRTPRTCPComposerHandle != NULL && 
							dwSSRC == RTPRTCPComposer_GetSessionSSRC(pChannel->session[i].hRTPRTCPComposerHandle) )
						{
							// TelnetShell_DbgPrint("Got RReport from SSRC %lu\r\n",(long)dwSSRC);
							memcpy((void*)&pChannel->session[i].RTCPNATAddr,(void*)&saddr,sizeof(RTSP_SOCKADDR));
	                        RTPRTCPComposer_ResetCountOfMissingReport(pChannel->session[i].hRTPRTCPComposerHandle);							
							break;
						}
					}

					if( i == pChannel->iMaxSession)
						TelnetShell_DbgPrint("RReport with error SSRC %lu\r\n",(long)dwSSRC);
				}				 
			}
			else
			{
#ifdef _LINUX
				//syslog(LOG_ERR, "Fixed RTCP recv failed!!\r\n");
#endif //_LINUX													
#ifdef _PSOS_TRIMEDIA                            
				TelnetShell_DbgPrint("Fixed RTCP recv failed!!\r\n");                            
#endif  
				break;
			}
		}
		else
		{
			break;
		}

	}

	return;

}

void   ChannelReceiveRTCPReportFromEveryConnection(CHANNEL *pChannel)
{
    int i;
    int maxfd = 0 ;
    fd_set rset;
    int nready;
    char acRTCPBuffer[500];
    int  iRTCPLen,iLen;
    RTSP_SOCKADDR saddr;

    struct timeval timeout;
//    unsigned long ulSessionID;
    int rtcp_sock;
    DWORD dwSessionID;

    FD_ZERO(&rset);
	    
    for(i=0;i< pChannel->iMaxSession;i++)
    {
		//20131220 added by Charles to avoid sending audio RTCP before first RTP packet
        if( RTPRTCPComposer_GetValidate(pChannel->session[i].hRTPRTCPComposerHandle) == 0 )
        {
	        continue;
	    } 
		//20110706 Modified by danny For TCP/HTTP mode socket not sync in multi thread
		if( ( pChannel->session[i].dwSessionID) != 0 &&
        	  pChannel->session[i].iStatus != SESSION_PAUSED &&
        	  pChannel->session[i].iRTPStreamingType == RTP_OVER_UDP &&
			  pChannel->session[i].psktRTCP != NULL && 
			  *pChannel->session[i].psktRTCP > 0 )
        {
            rtcp_sock = *pChannel->session[i].psktRTCP;
	    
            if( rtcp_sock >= 0 )
            {
                FD_SET(rtcp_sock, &rset);
            }                
	     
            if(  rtcp_sock > (unsigned int)maxfd)
            {
                maxfd = rtcp_sock;
            }                
        } 
    }

	memset(acRTCPBuffer, 0, sizeof(acRTCPBuffer));
	// Moved by cchuang, 2005/07/25
    timeout.tv_sec = 0;//1;
    timeout.tv_usec = 0;//500;
    nready = select(maxfd+1, &rset, NULL, NULL,&timeout);

        for(i=0;i< pChannel->iMaxSession;i++)
        {
            if( ( pChannel->session[i].dwSessionID) != 0 &&
        	     pChannel->session[i].iStatus != SESSION_PAUSED &&
        	     pChannel->session[i].iRTPStreamingType == RTP_OVER_UDP )				 
            {
				//20110706 Modified by danny For TCP/HTTP mode socket not sync in multi thread
				rtcp_sock = *pChannel->session[i].psktRTCP;

                if( rtcp_sock >=0 && FD_ISSET(rtcp_sock,&rset) && nready >0 )
                {
                    memset((void*)&saddr,0, sizeof(saddr));
                    iLen = sizeof(saddr);
					//CID:1104, CHECKER:TAINTED_SCALAR
                    iRTCPLen = recvfrom(rtcp_sock,acRTCPBuffer,sizeof(acRTCPBuffer) - 1,RECV_FLAGS,(struct sockaddr *)&saddr,(unsigned int *)&iLen);
                        
                    if (  iRTCPLen <= 0 )
                    {
                        //20121004 added by Jimmy to fix that connection close because of unicast RTCP socket receiving EWOULDBLOCK
						if( errno == EWOULDBLOCK )
						{
							continue;
						}

						if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_VIDEO)
						{
							DbgLog((dfCONSOLE|dfINTERNAL,"video channel removed:ID %u socket %d\n",pChannel->session[i].dwSessionID, *pChannel->session[i].psktRTCP));
							//20130315 added by Jimmy to log more information
							syslog(LOG_DEBUG, "[ChannelReceiveRTCPReportFromEveryConnection]: video channel removed:ID %u socket %d, errno = %d\n",pChannel->session[i].dwSessionID, *pChannel->session[i].psktRTCP, errno);
						}
						//20120816 modified by Jimmy for metadata
						else if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_AUDIO)
						{
							DbgLog((dfCONSOLE|dfINTERNAL,"audio channel removed:ID %u socket %d\n",pChannel->session[i].dwSessionID, *pChannel->session[i].psktRTCP));
							//20130315 added by Jimmy to log more information
							syslog(LOG_DEBUG, "[ChannelReceiveRTCPReportFromEveryConnection]: audio channel removed:ID %u socket %d, errno = %d\n",pChannel->session[i].dwSessionID, *pChannel->session[i].psktRTCP, errno);
						}
						//20120816 added by Jimmy for metadata
						else if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_METADATA)
						{
							DbgLog((dfCONSOLE|dfINTERNAL,"metadata channel removed:ID %u socket %d\n",pChannel->session[i].dwSessionID, *pChannel->session[i].psktRTCP));
							//20130315 added by Jimmy to log more information
							syslog(LOG_DEBUG, "[ChannelReceiveRTCPReportFromEveryConnection]: metadata channel removed:ID %u socket %d, errno = %d\n",pChannel->session[i].dwSessionID, *pChannel->session[i].psktRTCP, errno);
						}

                    	
                        dwSessionID=pChannel->session[i].dwSessionID;

/*						if( pChannel->session[i].iRTPStreamingType == RTP_OVER_UDP)
							closesocket(pChannel->session[i].sktRTP);
                        pChannel->session[i].sktRTP = -1;	  	   
                        pChannel->session[i].dwSessionID = 0;
                        pChannel->session[i].iStartRTCPSReport = 0;

						if( pChannel->session[i].iRTPStreamingType == RTP_OVER_UDP)
							closesocket(pChannel->session[i].sktRTCP);

                        pChannel->session[i].sktRTCP = -1;
                        pChannel->session[i].hRTPRTCPComposerHandle = NULL;*/

						RTPRTCPChannel_InitSession(&pChannel->session[i]);

                        pChannel->fCallbackFunct(pChannel->hParentHandle
		                       ,RTPRTCPCHANNEL_CALLBACKFLAG_SESSION_CLOSED
		  					   ,(void *)dwSessionID,0);
            
                        pChannel->nTotalConnectionCount -- ;									
#ifdef _LINUX
            //syslog(LOG_ERR, "2065:Remove connection RTP %d\n",pChannel->nTotalConnectionCount);
#endif //_LINUX
                        
                        continue ;
                    }
                    else                    
                    {
                        //For RTCP sender report passing NAT 
                        if( pChannel->session[i].iStartRTCPSReport == 0 )
                        {                           
							//20110706 Modified by danny For TCP/HTTP mode socket not sync in multi thread
							if( connect(*pChannel->session[i].psktRTCP,(struct sockaddr* )&saddr,sizeof(saddr)) == 0)
                            {
#ifdef _PSOS_TRIMEDIA                            
                               TelnetShell_DbgPrint("Got RReport from %s port:%d\r\n",inet_ntoa(saddr.sin_addr),ntohs(saddr.sin_port));                            
#endif                                 
								printf("[%s] socket %d connect successfully!\n", __FUNCTION__, *pChannel->session[i].psktRTCP);
							   	SendRTCPSenderReport(&pChannel->session[i],pChannel->session[i].hRTPRTCPComposerHandle,pChannel) ;	     
                               	pChannel->session[i].iStartRTCPSReport = 1;
                            }                                
                        }    

                        RTPRTCPComposer_ParseRTCPPacket(acRTCPBuffer,iRTCPLen,pChannel->session[i].hRTPRTCPComposerHandle);
                        /*pRTPRecvReport.dwSessionID = pChannel->session[i].dwSessionID;			   
                        pRTPRecvReport.iPacketLossRate = RTPRTCPComposer_GetLostRate(pChannel->session[i].hRTPRTCPComposerHandle);	
                        pRTPRecvReport.iJitter = RTPRTCPComposer_GetJitter(pChannel->session[i].hRTPRTCPComposerHandle);	
                        pChannel->fCallbackFunct(pChannel->hParentHandle
		                       ,RTPRTCPCHANNEL_CALLBACKFLAG_RECEIVERREPORT
		  					   ,(void*)&pRTPRecvReport,0);*/
		  				printf("Got RTCP recv report %d bytes\r\n",iRTCPLen);	   
                        RTPRTCPComposer_ResetCountOfMissingReport(pChannel->session[i].hRTPRTCPComposerHandle);
                    }
	   
                }     

				//20101020 Add by danny for support seamless stream TCP/UDP timeout
                if( check_crash_session(pChannel->session[i].hRTPRTCPComposerHandle, pChannel->session[i].bSeamlessStream) )
                {
					if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_VIDEO)
					{
						TelnetShell_DbgPrint("video channel Timeout:ID %lu socket %d\n",(long)pChannel->session[i].dwSessionID, *pChannel->session[i].psktRTCP);
						//20130315 added by Jimmy to log more information
						syslog(LOG_DEBUG, "video channel Timeout:ID %lu socket %d\n",(long)pChannel->session[i].dwSessionID, *pChannel->session[i].psktRTCP);
					}
					//20120816 modified by Jimmy for metadata
					else if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_AUDIO)
					{
						TelnetShell_DbgPrint("audio channel Timeout:ID %lu socket %d\n",(long)pChannel->session[i].dwSessionID, *pChannel->session[i].psktRTCP);
						//20130315 added by Jimmy to log more information
						syslog(LOG_DEBUG, "audio channel Timeout:ID %lu socket %d\n",(long)pChannel->session[i].dwSessionID, *pChannel->session[i].psktRTCP);
					}
					//20120816 added by Jimmy for metadata
					else if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_METADATA)
					{
						TelnetShell_DbgPrint("metadata channel Timeout:ID %lu socket %d\n",(long)pChannel->session[i].dwSessionID, *pChannel->session[i].psktRTCP);
						//20130315 added by Jimmy to log more information
						syslog(LOG_DEBUG, "metadata channel Timeout:ID %lu socket %d\n",(long)pChannel->session[i].dwSessionID, *pChannel->session[i].psktRTCP);
					}

				    dwSessionID=pChannel->session[i].dwSessionID;

/*					if( pChannel->session[i].iRTPStreamingType == RTP_OVER_UDP)
						closesocket(pChannel->session[i].sktRTP);

                    pChannel->session[i].sktRTP = -1;	  	   
                    pChannel->session[i].dwSessionID = 0;
                    pChannel->session[i].iStartRTCPSReport = 0;

					if( pChannel->session[i].iRTPStreamingType == RTP_OVER_UDP)
						closesocket(pChannel->session[i].sktRTCP);

                    pChannel->session[i].sktRTCP = -1;
                    pChannel->session[i].hRTPRTCPComposerHandle = NULL;*/

					RTPRTCPChannel_InitSession(&pChannel->session[i]);

                    pChannel->fCallbackFunct(pChannel->hParentHandle
		                       ,RTPRTCPCHANNEL_CALLBACKFLAG_SESSION_CLOSED
		  					   ,(void *)dwSessionID,0);
            
                    pChannel->nTotalConnectionCount -- ;									
#ifdef _LINUX
            //syslog(LOG_ERR, "2130:Remove connection RTP %d\n",pChannel->nTotalConnectionCount);
#endif //_LINUX
                    

                }	  
            }
        }
   
}


void	ChannelRTCPSendLoop(void*	lpParam)
{
  CHANNEL *pChannel=(CHANNEL *)lpParam;
  
//  printf("RTCP task begin!!\n");
    while(pChannel->nChannelStatus == CHANNEL_RUNNING)
	{     
	  if( pChannel->nTotalConnectionCount <= 0 )
	  {
          OSSleep_MSec(300);
	      continue;
	  }  
      ChannelSendRTCPToEveryConnection(pChannel);
      ChannelReceiveRTCPReportFromEveryConnection(pChannel);
	}
	
	printf(" \n RTCP thread stop ");	

}

int RTPRTCPChannel_AddTCPMuxHandle(HANDLE hRTPRTCPChannelHandle,HANDLE hTCPMuxCS)
{
    int i;
    
    CHANNEL *pChannel=(CHANNEL *)hRTPRTCPChannelHandle;

    for(i=0;i< pChannel->iMaxSession;i++)
    {
        if( pChannel->session[i].hTCPMuxCS == NULL )
        {
              pChannel->session[i].hTCPMuxCS = hTCPMuxCS;
              return 0;
        }      
    }
    
    return -1;
}
#ifdef _SHARED_MEM
//20101018 Add by danny for support multiple channel text on video
SCODE RTPRTCPChannel_SetLocation(HANDLE hRTPRTCPChannelHandle, char* pcLocation, int iMultipleChannelChannelIndex)
{
	CHANNEL *pChannel=(CHANNEL *)hRTPRTCPChannelHandle;
	int i;

    if( hRTPRTCPChannelHandle == NULL || pcLocation == NULL)
        return S_FAIL;        
    
    if( strlen(pcLocation) > LOCATION_LEN ) return S_FAIL;
    
	//rtspstrcpy(pChannel->acLocation, pcLocation, sizeof(pChannel->acLocation));    
	if( iMultipleChannelChannelIndex == 0 )
	{
		for( i = 0; i < MULTIPLE_CHANNEL_NUM; i++ )
		{
			rtspstrcpy(pChannel->acLocation[i], pcLocation, sizeof(pChannel->acLocation[i]));
			printf("Location %d set at media channel: %s\r\n", i + 1, pcLocation);
		}
	}
	else
	{
		if( iMultipleChannelChannelIndex <= MULTIPLE_CHANNEL_NUM )
		{
			rtspstrcpy(pChannel->acLocation[iMultipleChannelChannelIndex-1], pcLocation, sizeof(pChannel->acLocation[iMultipleChannelChannelIndex-1]));
			printf("Location %d set at media channel: %s\r\n", iMultipleChannelChannelIndex, pcLocation);
		}	
	}
	
    return 0;
}
#endif
SCODE RTPRTCPChannel_CloseSession(CHANNEL *pChannel, RTPRTCPCHANNEL_SESSION *pSession)
{
	DWORD	dwSessionID = 0;

	if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_VIDEO)
	{  					   
		DbgLog((dfCONSOLE|dfINTERNAL,"RTCP:video removed:ID %u socket %d\n",pSession->dwSessionID, *pSession->psktRTP));
		//20130315 added by Jimmy to log more information
		syslog(LOG_DEBUG, "[RTPRTCPChannel_CloseSession]:video removed:ID %u socket %d\n",pSession->dwSessionID, *pSession->psktRTP);
	}
	//20120816 modified by Jimmy for metadata
	else if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_AUDIO)
	{  					   
		DbgLog((dfCONSOLE|dfINTERNAL,"RTCP:audio removed:ID %u socket %d\n",pSession->dwSessionID, *pSession->psktRTP));
		//20130315 added by Jimmy to log more information
		syslog(LOG_DEBUG, "[RTPRTCPChannel_CloseSession]:audio removed:ID %u socket %d\n",pSession->dwSessionID, *pSession->psktRTP);
	}
	//20120816 added by Jimmy for metadata
	else if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_METADATA)
	{  					   
		DbgLog((dfCONSOLE|dfINTERNAL,"RTCP:metadata removed:ID %u socket %d\n",pSession->dwSessionID, *pSession->psktRTP));
		//20130315 added by Jimmy to log more information
		syslog(LOG_DEBUG, "[RTPRTCPChannel_CloseSession]:metadata removed:ID %u socket %d\n",pSession->dwSessionID, *pSession->psktRTP);
	}

    dwSessionID=pSession->dwSessionID;
	RTPRTCPChannel_InitSession(pSession);

	pChannel->fCallbackFunct(pChannel->hParentHandle
			,RTPRTCPCHANNEL_CALLBACKFLAG_SESSION_CLOSED
			,(void *)dwSessionID,0);

    pChannel->nTotalConnectionCount -- ;	

	return S_OK;
}

SCODE RTPRTCPCs_GetLockStatus(RTPRTCPCHANNEL_SESSION *pSession, ECritSecStatus* eStatus)
{
	TShmemSessionInfo			*pShmSessionInfo = (TShmemSessionInfo *)pSession->ptShmemMediaInfo->hParentHandle;
	*eStatus = pShmSessionInfo->eCritSecStatus;
	return S_OK;
}

SCODE RTPRTCPCs_TryEnter(RTPRTCPCHANNEL_SESSION *pSession, ECritSecStatus eStatus)
{
	TShmemSessionInfo			*pShmSessionInfo = NULL;

	if(pSession == NULL)
	{
		return S_FAIL;
	}
	//CID:421 REVERSE_INULL
	pShmSessionInfo = (TShmemSessionInfo *)pSession->ptShmemMediaInfo->hParentHandle;

	if(pSession->iRTPStreamingType != RTP_OVER_TCP)
	{
		return S_OK;
	}

	if(pShmSessionInfo->eCritSecStatus == eStatus)
	{
		//No need to lock as critical section is already obtained!
		return S_OK;
	}
	else if(pShmSessionInfo->eCritSecStatus == eCSReleased)
	{
		//Attemp to lock
		if(OSCriticalSection_TryEnter(pSession->hTCPMuxCS) == S_OK)
		{
			//printf("[%s] @@@OSCriticalSection_TryEnter %p ok@@@\n", __FUNCTION__, pSession->hTCPMuxCS);
			pShmSessionInfo->eCritSecStatus = eStatus;
			//20120829 added by Jimmy for metadata
			if(eStatus == eCSVideoRTP || eStatus == eCSAudioRTP || eStatus == eCSMetadataRTP)
			{
				pSession->ptShmemMediaInfo->bCSAcquiredButNotSelected = TRUE;
			}
			return S_OK;
		}
		else
		{
			printf("Status %d blocked by %d\n", eStatus, pShmSessionInfo->eCritSecStatus);
			return S_FAIL;
		}
	}
	else
	{
		//Already locked by someone else
		printf("Status %d blocked by %d\n", eStatus, pShmSessionInfo->eCritSecStatus);
		return S_FAIL;
	}
}

SCODE RTPRTCPCs_Release(RTPRTCPCHANNEL_SESSION *pSession, ECritSecStatus eStatus)
{
	TShmemSessionInfo			*pShmSessionInfo = NULL;

	if(pSession == NULL)
	{
		return S_FAIL;
	}
	//CID:420 REVERSE_INULL
	pShmSessionInfo = (TShmemSessionInfo *)pSession->ptShmemMediaInfo->hParentHandle;

	if(pSession->iRTPStreamingType != RTP_OVER_TCP)
	{
		return S_OK;
	}

	if(pShmSessionInfo->eCritSecStatus == eStatus)
	{
		//201605030 Modify by Faber, prevent status change after release lock
		pShmSessionInfo->eCritSecStatus = eCSReleased;
		pSession->ptShmemMediaInfo->bCSAcquiredButNotSelected = FALSE;
		OSCriticalSection_Leave(pSession->hTCPMuxCS);
		return S_OK;
	}
	else
	{
		//Not a good idea to do unlock
		printf("Warning: error leaving critical section %d. Status :%d!\n", eStatus, pShmSessionInfo->eCritSecStatus);
		return S_FAIL;
	}

	return S_OK;
}

SCODE RTPRTCPChannel_SessionKeepAlive(HANDLE hRTPRTCPChannelHandle, DWORD dwSessionID)
{
	int		i = 0;
	CHANNEL *pChannel=(CHANNEL *)hRTPRTCPChannelHandle;

	if(dwSessionID == 0)
	{
		return S_FAIL;
	}
    
    for(i=0; i< pChannel->iMaxSession; i++)
    {
		if( pChannel->session[i].dwSessionID == dwSessionID )
        {
			//printf("Session ID %u receved keep-alive from RTSP OPTIONS\n", dwSessionID);
			RTPRTCPComposer_ResetCountOfMissingReport(pChannel->session[i].hRTPRTCPComposerHandle);
            return S_OK;
        }      
    }

	// printf("No corresponding Session ID %u to keep alive\n", dwSessionID);

	return S_FAIL;
}

//20141110 added by Charles for ONVIF Profile G
SCODE RTPRTCPChannel_SetPlayCseq(HANDLE hRTPRTCPChannelHandle, DWORD dwSessionID, int iMulticastIndex, int iCseq)
{
	int		i = 0;
	CHANNEL *pChannel = (CHANNEL *)hRTPRTCPChannelHandle;

    if(iMulticastIndex > 0)
    {
        pChannel->stMulticast[iMulticastIndex].iCSeq = iCseq;
        return S_OK;
    }
    else
    {
        for(i=0; i< pChannel->iMaxSession; i++)
        {
    		if( pChannel->session[i].dwSessionID == dwSessionID )
            {
    			pChannel->session[i].iCSeq = iCseq;
                return S_OK;
            }      
        }
    }

	return S_FAIL;
}

