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
 *  Module name        :   RTSP Server
 *  File name          :   rtsp_server.c 
 *  File description   :   RTSP API and main loop 
 *  Author             :   ShengFu
 *  Created at         :   2002/4/24 
 *  Note               :   
 *	$Log: /RD_1/Protocol/RTSP/Server/rtspstreamserver/rtspserver/src/rtspserver.c $
 * 
 * 3     06/06/21 5:16p Shengfu
 * 
 * 2     06/05/18 3:26p Shengfu
 * update to version 1.4.0.0
 * 
 * 10    06/01/23 4:21p Shengfu
 * upgrade to ver 1.3.0.7
 * 
 * 9     06/01/10 5:48p Shengfu
 * update to 1.3.0.6
 * 
 * 7     05/11/30 6:23p Shengfu
 * update to version 1.3.0.4
 * 
 * 6     05/11/03 11:57a Shengfu
 * update to version 1.3.0.3
 * 
 * 5     05/10/07 8:48a Shengfu
 * 
 * 4     05/09/27 1:14p Shengfu
 * update to version 1,3,0,1
 * 
 * 3     05/08/19 11:49a Shengfu
 * 
 * 1     05/08/19 11:30a Shengfu
 * 
 * 2     05/08/10 9:01a Shengfu
 * update rtspstreaming server which enable multicast
 * 
 * 6     05/07/13 2:26p Shengfu
 * update rtsp streaming server
 * 
 * 5     05/04/15 1:35p Shengfu
 * 1. multicast added, but disable
 * 2. RTP extension added
 * 
 * 3     05/01/24 10:02a Shengfu
 * 
 * 2     04/12/20 2:34p Shengfu
 * update to version 1.1.0.0
 * 
 * 1     04/09/14 9:39a Shengfu
 * 
 * 1     04/09/14 9:21a Shengfu
 * 
 * 2     03/04/17 2:45p Shengfu
 * update to 0102b
 * 
 * 7     02/08/09 1:54p Simon
 * initial the socket value
 * 
 * 6     02/07/04 11:07a Simon
 * 1. Fix initial bug (abnormal close unallocate socket)
 * 2. Change SDP request callback method
 * 
 * 5     02/07/01 2:01p Shengfu
 * fujitsu server compliant
 * 
 * 4     02/06/24 5:18p Shengfu
 * video_only modified 
 */

#ifdef _LINUX
#include <sys/syslog.h>
#endif // _LINUX

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "rtsp_server_local.h"

#ifdef _PSOS_TRIMEDIA        
#include "extio.h"			
#endif       

PROTOCOL_MEDIABUFFER *g_ptMediaBuffer=NULL;
#define MEDIABUFFER_HEADERSIZE	12
#define MEDIABUFFER_SIZE		1460

int RTSPServer_UpdateClientSessionInfo(RTSP_SERVER *pRTSPServer);
DWORD THREADAPI RTSPServer_MainServerLoop(DWORD hRTSPServerHandle);
int RTSPServer_CheckTimeoutForAudioback(HANDLE hRTSPServerHandle);

static int MediaBuffer_NewMediaBuffer(PROTOCOL_MEDIABUFFER** ppMediaBuffer,unsigned int uiHeaderInfoSize, unsigned int uiBufferSize,unsigned int uiHeaderSize)
{
	if( ppMediaBuffer == NULL ) 
		return -2; 

	*ppMediaBuffer = (PROTOCOL_MEDIABUFFER*)malloc(sizeof(PROTOCOL_MEDIABUFFER)+uiBufferSize + uiHeaderSize + uiHeaderInfoSize);

	if( *ppMediaBuffer == NULL ) { 
		return -1; 
	}

#if 0
	printf("Header Len = %d, HeaderInfo len = %d, BufferSize = %d, PROTOCOL_MEDIABUFFER Size = %d\n", 
			uiHeaderSize, uiHeaderInfoSize, uiBufferSize, sizeof(PROTOCOL_MEDIABUFFER));
#endif // 0 
	memset(*ppMediaBuffer,0,sizeof(PROTOCOL_MEDIABUFFER) + uiBufferSize + uiHeaderSize + uiHeaderInfoSize); 
	(*ppMediaBuffer)->pbDataStart = (*ppMediaBuffer)->pbBufferStart = (BYTE*)((BYTE*)((*ppMediaBuffer)+1) + uiHeaderInfoSize); 
	(*ppMediaBuffer)->dwBufferLength = uiBufferSize + uiHeaderSize; 
	(*ppMediaBuffer)->dwHeaderSize = uiHeaderSize; 
	(*ppMediaBuffer)->dwDataBufferSize = uiBufferSize; 
	(*ppMediaBuffer)->pbHeaderInfoStart = (BYTE*)((*ppMediaBuffer)+1); 
	(*ppMediaBuffer)->dwHeaderInfoSize = uiHeaderInfoSize;
	return 0;
}


static int MediaBuffer_ReleaseMediaBuffer(PROTOCOL_MEDIABUFFER** ppMediaBuffer)
{ 
	if (ppMediaBuffer == NULL) 
		return -1;

	if (*ppMediaBuffer != NULL ) 
		free(*ppMediaBuffer); 
	
	*ppMediaBuffer = NULL;
	return 0;
}

SCODE RTSPServer_SetQosToSocket(int	iSockFd, TQosInfo *ptQosInfo, EQosMediaType eMediaType)
{
	int		iOpt = 0, iResult = 0;

	if(iSockFd <= 0)
	{
		return S_FAIL;
	}
	
	if(ptQosInfo->iCosEnabled)
	{
		if(eMediaType == eQosVideoType || eMediaType == eQosMuxedType)		//Video
		{
			iOpt = ptQosInfo->iCosVideoPriority;
			iResult = setsockopt(iSockFd, SOL_SOCKET, SO_PRIORITY, &iOpt, sizeof(iOpt)); 
		}
		else if(eMediaType == eQosAudioType)  //Audio
		{
			iOpt = ptQosInfo->iCosAudioPriority;
			iResult = setsockopt(iSockFd, SOL_SOCKET, SO_PRIORITY, &iOpt, sizeof(iOpt)); 
		}
		//20120726 added by Jimmy for metadata
		else if(eMediaType == eQosMetadataType)  //Metadata
		{
			iOpt = ptQosInfo->iCosMetadataPriority;
			iResult = setsockopt(iSockFd, SOL_SOCKET, SO_PRIORITY, &iOpt, sizeof(iOpt)); 
		}

	}
	if(ptQosInfo->iDscpEnabled)
	{
		if(eMediaType == eQosVideoType || eMediaType == eQosMuxedType)	//Video
		{
			iOpt = ptQosInfo->iDscpVideoPriority << 2;
			iResult = setsockopt(iSockFd, SOL_IP, IP_TOS, &iOpt, sizeof(iOpt));  
		}
		else if(eMediaType == eQosAudioType)  //Audio
		{
			iOpt = ptQosInfo->iDscpAudioPriority << 2;
			iResult = setsockopt(iSockFd, SOL_IP, IP_TOS, &iOpt, sizeof(iOpt));  
		}
		//20120726 added by Jimmy for metadata
		else if(eMediaType == eQosMetadataType)  //Metadata
		{
			iOpt = ptQosInfo->iDscpMetadataPriority << 2;
			iResult = setsockopt(iSockFd, SOL_IP, IP_TOS, &iOpt, sizeof(iOpt));  
		}

	}

	if(iResult < 0)
	{
		printf("Set QOS parameter fail for Sock %d\n", iSockFd);
		return S_FAIL;
	}
	//printf("Set QOS parameter %d for Sock %d(%d)\n", iOpt, iSockFd, eMediaType);
	return S_OK;
}

SCODE RTSPServer_UpdateQosParameters(HANDLE hRTSPServerHandle, TQosInfo *ptQosInfo)
{    
	RTSP_SERVER *pRTSPServer;
	int			i = 0;

    if( hRTSPServerHandle == NULL || ptQosInfo == NULL)
    {
        return S_FAIL ;
    }
    pRTSPServer = (RTSP_SERVER*)hRTSPServerHandle;

	//Update the internal QOS structure
	memcpy(&pRTSPServer->tQosInfo, ptQosInfo, sizeof(TQosInfo));

	//Apply the QOS to each running client
	for(i=0;i<pRTSPServer->iMaxSessionNumber;i++)
    {
		if(pRTSPServer->pClient[i].ulSessionID > 0)
		{
			//RTSP signaling socket
			if(pRTSPServer->pClient[i].iRTPStreamingMode != RTP_OVER_HTTP)
			{
				RTSPServer_SetQosToSocket(pRTSPServer->pClient[i].iSendSockfd, ptQosInfo, eQosMuxedType);
			}
			//Video sockets
			RTSPServer_SetQosToSocket(pRTSPServer->pClient[i].rtp_sock[0][0], ptQosInfo, eQosVideoType);
			RTSPServer_SetQosToSocket(pRTSPServer->pClient[i].rtp_sock[0][1], ptQosInfo, eQosVideoType);
			//Audio sockets
			RTSPServer_SetQosToSocket(pRTSPServer->pClient[i].rtp_sock[1][0], ptQosInfo, eQosAudioType);
			RTSPServer_SetQosToSocket(pRTSPServer->pClient[i].rtp_sock[1][1], ptQosInfo, eQosAudioType);
			//20120726 added by Jimmy for metadata
			//Metadata sockets
#ifdef _METADATA_ENABLE
			RTSPServer_SetQosToSocket(pRTSPServer->pClient[i].rtp_sock[2][0], ptQosInfo, eQosMetadataType);
			RTSPServer_SetQosToSocket(pRTSPServer->pClient[i].rtp_sock[2][1], ptQosInfo, eQosMetadataType);
#endif

		}
    }	

	return S_OK;
}

#ifdef _SHARED_MEM
/* 20100105 Added For Seamless Recording */
SCODE RTSPServer_UpdateSeamlessRecordingParameters(HANDLE hRTSPServerHandle, TSeamlessRecordingInfo *ptSeamlessRecordingInfo)
{    
	RTSP_SERVER *pRTSPServer;
	int iSessionCount = 0;
	
    if( hRTSPServerHandle == NULL || ptSeamlessRecordingInfo == NULL)
    {
        return S_FAIL ;
    }
    pRTSPServer = (RTSP_SERVER*)hRTSPServerHandle;
	
	//Update the internal Seamless Recording structure
	memcpy(&pRTSPServer->tSeamlessRecordingInfo, ptSeamlessRecordingInfo, sizeof(TSeamlessRecordingInfo));
	printf("[%s] pRTSPServer->tSeamlessRecordingInfo.iSeamlessDiskMode=%d, pRTSPServer->tSeamlessRecordingInfo.iSeamlessMaxConnection=%d, pRTSPServer->tSeamlessRecordingInfo.iSeamlessStreamNumber=%d, pRTSPServer->tSeamlessRecordingInfo.iRecordingEnable=%d\n",
					__FUNCTION__,
					pRTSPServer->tSeamlessRecordingInfo.iSeamlessDiskMode,
					pRTSPServer->tSeamlessRecordingInfo.iSeamlessMaxConnection,
					pRTSPServer->tSeamlessRecordingInfo.iSeamlessStreamNumber, 
					pRTSPServer->tSeamlessRecordingInfo.iRecordingEnable);

	for(iSessionCount = 0; iSessionCount < pRTSPServer->tSeamlessRecordingInfo.iSeamlessMaxConnection; iSessionCount++)
	{
		printf("[%s] guid%d_id=%s\n", __FUNCTION__, iSessionCount, pRTSPServer->tSeamlessRecordingInfo.tGUIDListInfo[iSessionCount].acGUID);
		printf("[%s] guid%d_number=%d\n", __FUNCTION__, iSessionCount, pRTSPServer->tSeamlessRecordingInfo.tGUIDListInfo[iSessionCount].iNumber);
		printf("[%s] guid%d_underrecording=%d\n", __FUNCTION__, iSessionCount, pRTSPServer->tSeamlessRecordingInfo.tGUIDListInfo[iSessionCount].iUnderRecording);
	}
	printf("[%s] Seamless Recording current connection number=%d\n", __FUNCTION__, pRTSPServer->tSeamlessRecordingInfo.iSeamlessConnectionNumber);
	
	return S_OK;
}

SCODE RTSPServer_UpdateDisconnectGUIDListInfo(RTSP_SERVER *pRTSPServer, RTSP_CLIENT *pClient)
{
	int iSessionCount;
	BOOL bNormalDisconnected;
	
	iSessionCount = pClient->iSeamlessRecordingSession;
	bNormalDisconnected = pClient->bNormalDisconnected;
	
	pRTSPServer->tSeamlessRecordingInfo.tGUIDListInfo[iSessionCount].iNumber--;
	pRTSPServer->tSeamlessRecordingInfo.iSeamlessConnectionNumber--;
	if( pRTSPServer->tSeamlessRecordingInfo.tGUIDListInfo[iSessionCount].iNumber == 0 )
	{
		if( bNormalDisconnected == TRUE )
		{
			printf("[%s] Normal disconnected for guid=%s last connection\n", __FUNCTION__, 
															pRTSPServer->tSeamlessRecordingInfo.tGUIDListInfo[iSessionCount].acGUID);
			memset(&pRTSPServer->tSeamlessRecordingInfo.tGUIDListInfo[iSessionCount], 0, sizeof(TGUIDListInfo));
		}
		else
		{
			printf("[%s] Abnormal disconnected for guid=%s last connection\n", __FUNCTION__, 
															pRTSPServer->tSeamlessRecordingInfo.tGUIDListInfo[iSessionCount].acGUID);
			pRTSPServer->tSeamlessRecordingInfo.tGUIDListInfo[iSessionCount].iUnderRecording = 1;
			pRTSPServer->tSeamlessRecordingInfo.iSeamlessConnectionNumber++;
			if( pRTSPServer->tSeamlessRecordingInfo.iRecordingEnable == 0 )
			{
				pRTSPServer->tSeamlessRecordingInfo.iRecordingEnable = 1;
				printf("[%s] Start recording!\n", __FUNCTION__);

				if( pRTSPServer->fcallback(pRTSPServer->hParentHandle
							,RTSPSERVER_CALLBACKFLAG_UPDATE_RECODERSTATE
							,(void*)&pRTSPServer->tSeamlessRecordingInfo, (void*)&pClient->tLastFrameInfo) != S_OK )
				{
					DbgLog((dfCONSOLE|dfINTERNAL,"RTSPSERVER_CALLBACKFLAG_UPDATE_RECODERSTATE failed!\n"));
					printf("[%s] RTSPSERVER_CALLBACKFLAG_UPDATE_RECODERSTATE failed!\n", __FUNCTION__);
				}
			}
		}
	}

	if( pRTSPServer->fcallback(pRTSPServer->hParentHandle
							,RTSPSERVER_CALLBACKFLAG_UPDATE_GUIDLISTINFO
							,(void*)&pRTSPServer->tSeamlessRecordingInfo,(void*)iSessionCount) != S_OK )
	{
		DbgLog((dfCONSOLE|dfINTERNAL,"RTSPSERVER_CALLBACKFLAG_UPDATE_GUIDLISTINFO failed!\n"));
		printf("[%s] RTSPSERVER_CALLBACKFLAG_UPDATE_GUIDLISTINFO failed!\n", __FUNCTION__);
	}
	
	return S_OK;
}

SCODE RTSPServer_SetLastFrameTimeForAVStream(HANDLE hRTSPServerHandle, HANDLE hShmemSessionInfo, unsigned long ulSessionID)
{
	RTSP_SERVER *pRTSPServer;
	TShmemSessionInfo *pShmSessionInfo;
	int i;

    if( hRTSPServerHandle == NULL )
    {
        return S_FAIL ;
    }
    
    pRTSPServer = (RTSP_SERVER*)hRTSPServerHandle;
	pShmSessionInfo = (TShmemSessionInfo*)hShmemSessionInfo;

	for( i=0; i<pRTSPServer->iMaxSessionNumber; i++)
    {
		if( (pRTSPServer->pClient[i].ulSessionID == ulSessionID) && (pRTSPServer->pClient[i].bSeamlessStream == TRUE) )
		{
			if( pRTSPServer->pClient[i].acMediaType[0][0] != 0 )
			{
				pRTSPServer->pClient[i].tLastFrameInfo.ulLastFrameTimeSec[0] = pShmSessionInfo->tShmemVideoMediaInfo.tStreamBuffer.dwSecond;
				pRTSPServer->pClient[i].tLastFrameInfo.ulLastFrameTimeMSec[0] = pShmSessionInfo->tShmemVideoMediaInfo.tStreamBuffer.dwMilliSecond;
				//printf("[%s] MediaChannel set LastFrameTime OK!\n", __FUNCTION__);
			}
			if( pRTSPServer->pClient[i].acMediaType[1][0] != 0 )
			{
				pRTSPServer->pClient[i].tLastFrameInfo.ulLastFrameTimeSec[1] = pShmSessionInfo->tShmemAudioMediaInfo.tStreamBuffer.dwSecond;
				pRTSPServer->pClient[i].tLastFrameInfo.ulLastFrameTimeMSec[1] = pShmSessionInfo->tShmemAudioMediaInfo.tStreamBuffer.dwMilliSecond;
			}
			//20120726 added by Jimmy for metadata
#ifdef _METADATA_ENABLE
			if( pRTSPServer->pClient[i].acMediaType[2][0] != 0 )
			{
				pRTSPServer->pClient[i].tLastFrameInfo.ulLastFrameTimeSec[2] = pShmSessionInfo->tShmemMetadataMediaInfo.tStreamBuffer.dwSecond;
				pRTSPServer->pClient[i].tLastFrameInfo.ulLastFrameTimeMSec[2] = pShmSessionInfo->tShmemMetadataMediaInfo.tStreamBuffer.dwMilliSecond;
			}
#endif
			break;
		}
	}

	/*if( i == pRTSPServer->iMaxSessionNumber )
    {
		printf("[%s] Set LastFrameTime failed, no sessionid %lu match!\n", __FUNCTION__, ulSessionID);
		return S_FAIL ;
    }*/
	
	return S_OK;
}
#endif

int RTSPServer_GetManagedSessionNumber(HANDLE hRTSPServerHandle)
{
    RTSP_SERVER *pRTSPServer;
	int i, iManagedSessionNumber;

    if( hRTSPServerHandle == NULL )
    {
        return -1 ;
    }
    
    pRTSPServer = (RTSP_SERVER*)hRTSPServerHandle;

	iManagedSessionNumber = 0;

	for(i = 0; i < pRTSPServer->iMaxSessionNumber; i++)
	{
		if(pRTSPServer->pClient[i].bManagedbySessionMgr == TRUE)
		{
			iManagedSessionNumber++;
		}
	}

	return iManagedSessionNumber;
}

void RTSPServer_InitClient(RTSP_CLIENT *pClient)
{
    int				j;
#ifdef _SESSION_MGR
	//20080621 Session manager
	TConnInfo		tConnInfo;
#endif

    if( pClient->iStatus == SETUP_STATE )
    {
    	//20120723 modified by Jimmy for metadata
		for(j=0;j<MEDIA_TYPE_NUMBER;j++)
		{
			if(pClient->rtp_sock[j][0]>=0)
			{
				closesocket(pClient->rtp_sock[j][0]);
				//20111205 Modified by danny For UDP mode socket leak
				pClient->rtp_sock[j][0] = -1;
			}
			
			if(pClient->rtp_sock[j][1]>=0)
			{	
				closesocket(pClient->rtp_sock[j][1]);
				pClient->rtp_sock[j][1] = -1;
			}

		}
		/*
    	if(pClient->rtp_sock[0][0]>=0)
    	{
        	closesocket(pClient->rtp_sock[0][0]);
        	//20111205 Modified by danny For UDP mode socket leak
			pClient->rtp_sock[0][0] = -1;
        }
        
        if(pClient->rtp_sock[0][1]>=0)
        {	
        	closesocket(pClient->rtp_sock[0][1]);
			pClient->rtp_sock[0][1] = -1;
        }
        
        if(pClient->rtp_sock[1][0]>=0)
        {	
        	closesocket(pClient->rtp_sock[1][0]);
			pClient->rtp_sock[1][0] = -1;
        }
       
        if(pClient->rtp_sock[1][1]>=0)
        {	
        	closesocket(pClient->rtp_sock[1][1]);
			pClient->rtp_sock[1][1] = -1;
        }
		*/
        
    }
        
    if( pClient->iRecvSockfd != -1 || pClient->iSendSockfd != -1)
    {
        //2005/05/31 add yb ShengFu       
        if( pClient->iRecvSockfd == pClient->iSendSockfd )
            closesocket(pClient->iSendSockfd);
        else
		{
			if( pClient->iRecvSockfd >= 0 )
			{
				closesocket(pClient->iRecvSockfd);
				TelnetShell_DbgPrint("---HTTP mode %d socket closed\r\n",pClient->iRecvSockfd);
			}

			if( pClient->iSendSockfd >= 0 ) 
			{
				closesocket(pClient->iSendSockfd);            
				TelnetShell_DbgPrint("---HTTP mode %d socket closed\r\n",pClient->iSendSockfd);
			}
		}
		 pClient->iRecvSockfd = -1;			/* -1 indicates available entry */
    	 pClient->iSendSockfd = -1;			/* -1 indicates available entry */
            
#ifdef _LINUX
        //syslog(LOG_ERR, "close RTSP socket %d %d\n", pClient->iRecvSockfd,pClient->iSendSockfd);
#endif //_LINUX
	    pClient->parent->iCurrentSessionNumber --;
#ifdef _SESSION_MGR
		//20110907 danny fixed iCurrentSessionNumber not sync with SessionMgr  
		//20080621 for session manager, 20080708 modified again
		//if(pClient->bManagedbySessionMgr)
		{
			tConnInfo.eStreamType = eConnectTypeRTSP;
			tConnInfo.eEncoderMode = eetNULL;
			tConnInfo.eConnMgr = eRemoveConn;
			tConnInfo.tProfile.iID = pClient->iSDPIndex;
			//20100623 danny, Modified for fix sessioninfo corrupted issue
			/*
				20120906 modifeid by Jimmy to fix sessioninfo not concsistent issue:
				The seesion number in sessioninfo must be concsistent with the managed session number( pClient->bManagedbySessionMgr==TRUE ), not iCurrentSessionNumber.
			*/
			//tConnInfo.iConnectNum  = pClient->parent->iCurrentSessionNumber;
			pClient->bManagedbySessionMgr = FALSE;
			tConnInfo.iConnectNum  = RTSPServer_GetManagedSessionNumber(pClient->parent);

			//if(!SessionMgr_CheckConnSt(&tConnInfo))
			if(!SessionMgr_UpdateConnSt(&tConnInfo, pClient->parent->hSessionMgrHandle))
			{
				syslog(LOG_ERR, "Removing client %lu from session manager error %d, server currentSessionNumber %d, managedSessionNumber %d\n",
					pClient->ulSessionID, tConnInfo.tProfile.eStatus, pClient->parent->iCurrentSessionNumber, RTSPServer_GetManagedSessionNumber(pClient->parent));
			}
		}
#endif
#ifdef RTSPRTP_MULTICAST
		
		if( pClient->ulSessionID != 0 )
		{
			if( pClient->iMulticast > 0)
			{
				pClient->parent->iCurrMulticastNumber[pClient->iMulticast-1] --;
				TelnetShell_DbgPrint("Multicast %d number = %d\r\n",pClient->iMulticast, pClient->parent->iCurrMulticastNumber[pClient->iMulticast-1]);
			}	
			else
			{
				pClient->parent->iCurrUnicastNumber --;
				TelnetShell_DbgPrint("unicast number = %d\r\n",pClient->parent->iCurrUnicastNumber);
			}
		}
#endif
    }

	//20080708 modified again for Phase III
	/*
		20120906 modifeid by Jimmy to fix sessioninfo not concsistent issue:
		The seesion number in sessioninfo must be concsistent with the managed session number( pClient->bManagedbySessionMgr==TRUE ), not iCurrentSessionNumber.
	*/
	//pClient->bManagedbySessionMgr = FALSE;
    
    pClient->iTimeOut = 0;
    pClient->iCSeq = -1;
    pClient->ulClientAddress = 0;
#ifdef _INET6
	memset(&pClient->tClientSckAddr, 0, sizeof(pClient->tClientSckAddr)) ;
#endif
	//20100917 Modified by danny for fix assign LastFrameTime before mediachannel thread set
    //pClient->ulSessionID = 0 ;   
#ifdef WISE_SPOT_AUTHENTICATE
	pClient->ulWiseSpotID = 0;
#endif
    pClient->iPlayerType = 0;
    pClient->hTCPMuxCS = 0;
	pClient->dwBaseMSec = 0;
	//20100917 Modified by danny for fix assign LastFrameTime before mediachannel thread set
	//RTSPServer_UpdateClientSessionInfo(pClient->parent);

    //20120723 modified by Jimmy for metadata
    for(j=0;j<MEDIA_TYPE_NUMBER;j++)
    {     
        pClient->usPlayFlag[j] = 0;
        pClient->usPort[j][0] = 0;
	    pClient->usPort[j][1] = 0;
	    //20111205 Modified by danny For UDP mode socket leak
		//pClient->rtp_sock[j][0]=-1;
   		//pClient->rtp_sock[j][1]=-1;
		//20100917 Modified by danny for fix assign LastFrameTime before mediachannel thread set
		//memset(pClient->acMediaType[j],0,40);
		pClient->usInitSequence[j]=0;
	    pClient->ulInitTimestamp[j]=0;
	    pClient->ulSSRC[j]=0;
		pClient->iEmbeddedRTPID[j] = -1;
		pClient->iEmbeddedRTCPID[j] = -1;
	    memset((void*)(&(pClient->NATRTPAddr[j])),0,sizeof(RTSP_SOCKADDR));
    }

    pClient->iRecvSize = 0;
    pClient->iUnfinishedRTSPSize = 0;
    
	pClient->iSymRTP = 0;
#ifndef _SHARED_MEM
	//20091116 support connected UDP
	pClient->iFixedUDPSourcePort = 0;
#endif
	pClient->iRTPStreamingMode = 0;
    pClient->iStatus = INIT_STATE;
	pClient->iVivotekClient = 0;

	//20120925 added by Jimmy for ONVIF backchannel
	pClient->iRequire = REQUIRE_NONE;

//20130902 modified by Charles for ondemand multicast
#ifdef RTSPRTP_MULTICAST
	pClient->iMulticast = 0;
    pClient->bNewMulticast = TRUE;
    memset(&(pClient->tOndemandMulticastInfo),0,sizeof(MULTICASTINFO));

#endif
     
    memset(pClient->acRecvBuffer,0,MAX_RECVBUFF);
    memset(pClient->acBase64Buffer,0,MAX_RECVBUFF);
    
    memset(pClient->acServerName,0,RTSP_URL_LEN - RTSP_URL_ACCESSNAME_LEN);
    memset(pClient->acObjURL,0,RTSP_URL_LEN);
    memset(pClient->acObject,0,RTSP_URL_ACCESSNAME_LEN);
	memset(pClient->acSessionCookie,0,RTSP_HTTP_COOKIE_LEN);
	//20080611 added for client management
	memset(pClient->acUserName, 0, RTSP_USERNAME_LENGTH);
	//20081121 for URL authentication
	memset(pClient->acPassWord, 0, RTSP_PASSWORD_LENGTH);
	pClient->bURLAuthenticated = FALSE;

#ifdef _SHARED_MEM
	pClient->eHSMode = eHSLiveStreaming;
	//added by neil 11/04/13 
	pClient->eSVCMode = eSVCNull;
	pClient->dwBypasyMSec = 0;
	pClient->dwDescribeMSec = 0;
	pClient->dwMinSftMSec = 0;
	memset(pClient->acShmemReturnString, 0, RTSPPARSER_REPLY_STRING_LENGTH);
	//20100812 Added For Client Side Frame Rate Control
	pClient->iFrameIntervalMSec = -1;
#endif

	pClient->iOrigSDPIndex = 0;
#ifdef _SESSION_MGR
	memset(&pClient->tSessMgrInfo, 0, sizeof(TSessMgrStreamInfo));
#endif

#ifdef _SHARED_MEM
	//20100105 Added For Seamless Recording
	if( pClient->bSeamlessStream == TRUE )
	{
		if( pClient->iSeamlessRecordingSession >= 0 )
		{
			RTSPServer_UpdateDisconnectGUIDListInfo(pClient->parent, pClient);
		}
		pClient->bSeamlessStream = FALSE;
	}
	memset(pClient->acSeamlessRecordingGUID, 0, RTSPS_Seamless_Recording_GUID_LENGTH);
	pClient->bNormalDisconnected = FALSE;
	pClient->iSeamlessRecordingSession = -1;
	memset(&pClient->tLastFrameInfo, 0, sizeof(TLastFrameInfo));
	
	//20100402 Media on demand
	if( pClient->bMediaOnDemand == TRUE )
	{
		pClient->bMediaOnDemand = FALSE;
		if( pClient->parent->iMODConnectionNumber > 0 )
		{
			pClient->parent->iMODConnectionNumber--;
		}
		// pClient->parent->bMODStreamEnable[pClient->iSDPIndex - LIVE_STREAM_NUM - 1] = FALSE;
		printf("[%s] Media on demand Connection Number=%d\n", __FUNCTION__, pClient->parent->iMODConnectionNumber);
	}
	memset(&pClient->tMODInfo, 0, sizeof(TMODInfo));
#endif	

	//20120925 added by Jimmy for ONVIF backchannel
	printf("[%s] pClient->iChannelIndex = %d, pClient->ulSessionID = %lu\n",__FUNCTION__,pClient->iChannelIndex,pClient->ulSessionID);
	if( pClient->iChannelIndex > 0)
	{
			printf("[%s] pClient->ulAudiobackSessionID[pClient->iChannelIndex-1] = %lu\n",__FUNCTION__,pClient->parent->ulAudiobackSessionID[pClient->iChannelIndex-1]);
	}

	if( pClient->bAudioback == TRUE )
	{
		if( pClient->parent->iAudiobackSock[pClient->iChannelIndex-1][0] >= 0 )
		{
			closesocket( pClient->parent->iAudiobackSock[pClient->iChannelIndex-1][0] );
			pClient->parent->iAudiobackSock[pClient->iChannelIndex-1][0] = -1;
		}
		if( pClient->parent->iAudiobackSock[pClient->iChannelIndex-1][1] >= 0 )
		{
			closesocket( pClient->parent->iAudiobackSock[pClient->iChannelIndex-1][1] );
			pClient->parent->iAudiobackSock[pClient->iChannelIndex-1][1] = -1;
		}
		pClient->parent->ulAudiobackSessionID[pClient->iChannelIndex-1] = 0;
		pClient->parent->dwLastRecvTimeofAudioback[pClient->iChannelIndex-1] = 0;
		pClient->bAudioback = FALSE;
	}

	pClient->iSDPIndex = 0;
	//20120925 added by Jimmy for ONVIF backchannel
	pClient->iChannelIndex = 0;
	//20100917 Modified by danny for fix assign LastFrameTime before mediachannel thread set
	pClient->ulSessionID = 0;
	//20120723 modified by Jimmy for metadata
	for(j=0;j<MEDIA_TYPE_NUMBER;j++)
    {
    	memset(pClient->acMediaType[j],0,40);
	}
	RTSPServer_UpdateClientSessionInfo(pClient->parent);
	
//    DbgLog((dfCONSOLE|dfINTERNAL,"Init...Current Connection = %d\n",pClient->parent->iCurrentSessionNumber));
#ifdef _PSOS_TRIMEDIA
    TelnetShell_DbgPrint("Init...Current Connection = %d\r\n",pClient->parent->iCurrentSessionNumber);
#else
    printf("Init...Current Connection = %d\r\n",pClient->parent->iCurrentSessionNumber); 
	#ifdef _LINUX    
		syslog(LOG_DEBUG,"Init...Current Connection = %d\n",pClient->parent->iCurrentSessionNumber);     
	#endif	
#endif
	//20151111 initial pause flag
	pClient->iIsPause = 0;
}

void RTSPServer_CheckIdleClient(RTSP_CLIENT* pClient,RTSP_SERVER *pRTSPServer)
{
	DWORD dwMilliSecond,dwDiffMSecond;

	OSTick_GetMSec(&dwMilliSecond);
	dwDiffMSecond = dwMilliSecond - pClient->dwBaseMSec;
   
    if( (pClient->iStatus == INIT_STATE || pClient->iStatus == SETUP_STATE)
	  && dwDiffMSecond > CHECKIDLECLIENT_TIMEOUT) 
    {
        TelnetShell_DbgPrint("Client time out! force to disconnect...\n");
#ifdef _LINUX
		syslog(LOG_DEBUG,"Client time out! force to disconnect\n");
#endif //_LINUX

        RTSPServer_InitClient(pClient);
    }

	if( (pClient->iStatus == TEARDOWN_STATE) && (dwDiffMSecond > 5000 ) )
    {
        if( !RTSPServer_SendReply(200, 0,pClient,pRTSPServer) )
        {    
            TelnetShell_DbgPrint(("Timeout and Teardown 200 OK\r\n"));    
            pClient->parent = pRTSPServer;
            RTSPServer_InitClient(pClient);
        }
        else
            TelnetShell_DbgPrint(("Timeout and Reply Teardown 200 OK failed\r\n"));    
    }

}

int RTSPServer_SetCallback(HANDLE hRTSPServerHandle, RTSPSERVERCALLBACK fCallback, HANDLE hParentHandle)
{
    RTSP_SERVER *pRTSPServer;

    if( hRTSPServerHandle == NULL || fCallback == NULL)
    {
        return -1 ;
    }
    
    pRTSPServer = (RTSP_SERVER*)hRTSPServerHandle;

    pRTSPServer->fcallback = fCallback;
    pRTSPServer->hParentHandle=hParentHandle; 
  
    return 0 ;
}

#ifdef _SESSION_MGR
/* 20100623 danny, Added for fix sessioninfo corrupted issue */
int RTSPServer_AddSessionMgrHandle(HANDLE hRTSPServerHandle, HANDLE hSessionMgrHandle)
{
    RTSP_SERVER *pRTSPServer;

    if( hRTSPServerHandle == NULL)
    {
        return -1 ;
    }
    
    pRTSPServer = (RTSP_SERVER*)hRTSPServerHandle;

    pRTSPServer->hSessionMgrHandle = hSessionMgrHandle; 
  
    return 0 ;
}
#endif

int	RTSPServer_CheckPrivateIPAddress(unsigned long ulIPAddress)
{
	unsigned long ulTempIP = htonl(ulIPAddress);

	
    if( ((ulTempIP)&0xff000000) == 0x0a000000)	// 10.0.0.0 ~ 10.255.255.255
		return TRUE;

	if( ((ulTempIP)&0xfff00000) == 0xac100000)	// 172.16.0.0 ~ 172.31.255.255
		return TRUE;

	if( ((ulTempIP)&0xffff0000) == 0xc0a80000)	// 192.168.0.0 ~ 192.168.255.255
		return TRUE;
	
	return FALSE;

}


HANDLE RTSPServer_Create( int iMaxSessionNumber, RTSPSERVER_PARAM *pstRTSPServerParameter)
{
    RTSP_SERVER		*pRTSPServer;
    int i=0;
	DWORD			adwMsgBuf[4];
    TOSThreadInitOptions  OSThreadOption;

    iMaxSessionNumber ++;
    pRTSPServer = (RTSP_SERVER *)calloc(sizeof(RTSP_SERVER),1);

	if( pRTSPServer == NULL )
		return NULL;

	pRTSPServer->pClient = (RTSP_CLIENT *)malloc(iMaxSessionNumber*sizeof(RTSP_CLIENT));

	if( pRTSPServer->pClient == NULL )
	{
		free(pRTSPServer);
		return NULL;
	}
	if(	MediaBuffer_NewMediaBuffer(&g_ptMediaBuffer,sizeof(RTPHEADERINFO),MEDIABUFFER_SIZE,MEDIABUFFER_HEADERSIZE) < 0 )
	{
		free(pRTSPServer->pClient);
		free(pRTSPServer);
		return NULL;
	}
	
    pRTSPServer->iMaxSessionNumber= iMaxSessionNumber;
    pRTSPServer->iCurrentSessionNumber= 0;//iMaxSessionNumber;
	//printf("pRTSPServer->iMaxSessionNumber=%d\n", pRTSPServer->iMaxSessionNumber);
#ifdef _SHARED_MEM
	if(pstRTSPServerParameter->dwProtectedDelta == 0 || pstRTSPServerParameter->dwProtectedDelta > MAX_PROTECTED_DELTA)
	{
		pRTSPServer->dwProtectedDelta = DEFAULT_PROTECTED_DELTA;
	}
	else
	{
		pRTSPServer->dwProtectedDelta = pstRTSPServerParameter->dwProtectedDelta;
	}
#endif
    
#ifdef WISE_SPOT_AUTHENTICATE
	pRTSPServer->dwAuthenticateIP = 0;
#endif

#ifdef RTSPRTP_MULTICAST
	pRTSPServer->iMaxUnicastNumber = iMaxSessionNumber/2;
	pRTSPServer->iCurrUnicastNumber = 0;
#endif

    for( i=0;i<iMaxSessionNumber;i++)
    {
		pRTSPServer->pClient[i].hTCPMuxCS = NULL;
        pRTSPServer->pClient[i].parent = pRTSPServer;
        pRTSPServer->pClient[i].iStatus = INIT_STATE;  
		pRTSPServer->pClient[i].iMulticast = 0;
#ifdef _SHARED_MEM
		//20100105 Added For Seamless Recording
		pRTSPServer->pClient[i].bSeamlessStream = FALSE;
#endif
		//20120925 added by Jimmy for ONVIF backchannel
		pRTSPServer->pClient[i].bAudioback = FALSE;
    }

    //20120925 added by Jimmy for ONVIF backchannel
    for( i=0;i<MULTIPLE_CHANNEL_NUM;i++)
    {
        pRTSPServer->iAudiobackSock[i][0] = -1;
        pRTSPServer->iAudiobackSock[i][1] = -1;
        pRTSPServer->ulAudiobackSessionID[i] = 0;
		pRTSPServer->dwLastRecvTimeofAudioback[i] = 0;
    }

	
#ifndef _SHARED_MEM
	//20091116 support connected UDP    
    if( RTSPServer_CheckPrivateIPAddress(pstRTSPServerParameter->ulIP) == TRUE ) 	
	{
		pRTSPServer->iBehindNAT = TRUE;
		printf("Server is behind NAT\r\n!");
	}
	else
	{
		pRTSPServer->iBehindNAT = FALSE;
    	printf("Server is NOT behind NAT\r\n!");
	}
#endif

    memcpy(&pRTSPServer->rtsp_param,pstRTSPServerParameter,sizeof(RTSPSERVER_PARAM));
#ifdef RTSPRTP_MULTICAST
	for( i=0; i < RTSP_MULTICASTNUMBER ; i ++)
	{
		//20110630 Add by danny For Multicast enable/disable
		pRTSPServer->iMulticastEnable[i] = pstRTSPServerParameter->iMulticastEnable[i];
		pRTSPServer->ulMulticastAddress[i] = pstRTSPServerParameter->ulMulticastAddress[i];
		//20160127 add by Faber
		pRTSPServer->ulMulticastAudioAddress[i] = pstRTSPServerParameter->ulMulticastAudioAddress[i];
		pRTSPServer->ulMulticastMetadataAddress[i] = pstRTSPServerParameter->ulMulticastMetadataAddress[i];
		pRTSPServer->usMulticastVideoPort[i] = pstRTSPServerParameter->usMulticastVideoPort[i];
		pRTSPServer->usMulticastAudioPort[i] = pstRTSPServerParameter->usMulticastAudioPort[i];
		//20120726 added by Jimmy for metadata
		pRTSPServer->usMulticastMetadataPort[i] = pstRTSPServerParameter->usMulticastMetadataPort[i];
		pRTSPServer->usTTL[i] = pstRTSPServerParameter->usTTL[i];
        pRTSPServer->iRTPExtension[i] = pstRTSPServerParameter->iRTPExtension[i];
        pRTSPServer->iMulticastSDPIndex[i] = pstRTSPServerParameter->iMulticastSDPIndex[i];

		pRTSPServer->iMaxMulticastNumber[i] = (iMaxSessionNumber - pRTSPServer->iMaxUnicastNumber)/RTSP_MULTICASTNUMBER ;
		pRTSPServer->iCurrMulticastNumber[i] = 0;
        TelnetShell_DbgPrint("MCur %d MMax %d\r\n",pRTSPServer->iCurrMulticastNumber[i],pRTSPServer->iMaxMulticastNumber[i]);
		
	}
    //20130904 added by Charles for ondemand multicast
    for( i=0; i < RTSP_MULTICASTNUMBER ; i ++)
    {
		//20110630 Add by danny For Multicast enable/disable
		pRTSPServer->iMulticastEnable[i + RTSP_MULTICASTNUMBER] = pstRTSPServerParameter->iMulticastEnable[i];
		pRTSPServer->iMaxMulticastNumber[i + RTSP_MULTICASTNUMBER] = (iMaxSessionNumber - pRTSPServer->iMaxUnicastNumber)/RTSP_MULTICASTNUMBER ;
		pRTSPServer->iCurrMulticastNumber[i + RTSP_MULTICASTNUMBER] = 0;
        TelnetShell_DbgPrint("MCur %d MMax %d\r\n",pRTSPServer->iCurrMulticastNumber[i],pRTSPServer->iMaxMulticastNumber[i]);
    }

    
#endif

    pRTSPServer->usSendSize = 0;
    memset(pRTSPServer->acSendBuffer,0,MAX_SNDBUFF);
  
    if( OSMsgQueue_Initial((HANDLE*)&pRTSPServer->ulDeleteQueue,pRTSPServer->iMaxSessionNumber*2) != S_OK )    
    {
        return NULL; 
    }
    
	if( OSMsgQueue_Initial((HANDLE*)&pRTSPServer->hHttpInfoBuffQueue,pRTSPServer->iMaxSessionNumber) != S_OK )    
    {
        return NULL; 
    }
	
	if( OSMsgQueue_Initial((HANDLE*)&pRTSPServer->hTeardownOKQueue,pRTSPServer->iMaxSessionNumber) != S_OK )    
    {
        return NULL; 
    }

	for(i=0;i<pRTSPServer->iMaxSessionNumber;i++)
	{
		memset(adwMsgBuf,0,4*sizeof(DWORD));
		adwMsgBuf[0]= (DWORD)malloc(sizeof(THTTPCONNINFO));
		if( adwMsgBuf[0] != 0 )
			OSMsgQueue_Send(pRTSPServer->hHttpInfoBuffQueue,(DWORD*)adwMsgBuf);
	}

    if( OSMsgQueue_Initial((HANDLE*)&pRTSPServer->hHttpSockQueue,pRTSPServer->iMaxSessionNumber) != S_OK )    
    {
        return NULL; 
    }
	
#ifdef _SHARED_MEM
	//20100105 Added For Seamless Recording
	if( OSMsgQueue_Initial((HANDLE*)&pRTSPServer->hResetGUIDQueue,pRTSPServer->iMaxSessionNumber) != S_OK )    
    {
        return NULL; 
    }
#endif

	//20081121 for URL authentication
	pRTSPServer->iURLAuthEnabled = pstRTSPServerParameter->iURLAuthEnabled;

	memset((char*)&OSThreadOption,0,sizeof(TOSThreadInitOptions));
    OSThreadOption.dwStackSize= 0;										//20090316 Let kernel manage the size
    OSThreadOption.dwInstance = (DWORD)pRTSPServer;

#if defined (_PSOS_TRIMEDIA)    
	OSThreadOption.dwPriority = pstRTSPServerParameter->ulThreadPriority;    
    OSThreadOption.dwFlags = T_LOCAL;
    OSThreadOption.dwMode = T_PREEMPT|T_ISR;
#endif    
    OSThreadOption.pThreadProc = RTSPServer_MainServerLoop;
    

	if( OSThread_Initial((HANDLE*)&pRTSPServer->ulTaskID,&OSThreadOption) != S_OK)
		return NULL;

    return pRTSPServer;
}

int RTSPServer_Close(HANDLE hRTSPServerHandle)
{
   	RTSP_SERVER		*pRTSPServer;
	unsigned long	ulMessage[4];
	// DWORD dwExitCode;

    pRTSPServer = (RTSP_SERVER*)hRTSPServerHandle;
//    q_delete(pRTSPServer->ulDeleteQueue);
    OSMsgQueue_Release((HANDLE*)&pRTSPServer->ulDeleteQueue);
    OSMsgQueue_Release((HANDLE*)&pRTSPServer->hHttpSockQueue);
    OSMsgQueue_Release((HANDLE*)&pRTSPServer->hTeardownOKQueue);
	
	while( OSMsgQueue_Receive(pRTSPServer->hHttpInfoBuffQueue,(DWORD*)ulMessage, 0) == S_OK )
	{
		if( ulMessage[0] != 0 )
			free((void*)ulMessage[0]);
	}

    OSMsgQueue_Release((HANDLE*)&pRTSPServer->hHttpInfoBuffQueue);
	
#ifdef _SHARED_MEM
	//20100105 Added For Seamless Recording
	OSMsgQueue_Release((HANDLE*)&pRTSPServer->hResetGUIDQueue);
#endif

	/*if (pRTSPServer->iRunning == 0)
		printf("====rtsp thread stopped=====\n");*/
		
	if( pRTSPServer->ulTaskID )
	{
		/*printf("kill thread 1\n");
		if (OSThread_WaitFor((void*)pRTSPServer->ulTaskID, 5000, &dwExitCode) != S_OK)
		{
			printf("kill thread 2\n");
			OSThread_Terminate((void*)pRTSPServer->ulTaskID);
			printf("kill thread 3\n");
		}
		printf("kill thread 4\n");*/
		OSThread_Release((HANDLE*)&pRTSPServer->ulTaskID);
	
	}
		
	//for(i=0; i< pRTSPServer->iMaxSessionNumber-1 ; i ++)
	//	OSCriticalSection_Release((HANDLE*)&(pRTSPServer->pClient[i].hTCPMuxCS));

	MediaBuffer_ReleaseMediaBuffer(&g_ptMediaBuffer);
    free(pRTSPServer->pClient);
    free(pRTSPServer);
    
	//printf("free handle\n");
	
    return 0;
}

int RTSPServer_RemoveSession(HANDLE hRTSPServerHandle, DWORD dwSessionID)
{
    RTSP_SERVER *pRTSPServer;
    unsigned long ulMsgBuf[4];
	int			iRet;

	if( hRTSPServerHandle == NULL )
		return -1;

    pRTSPServer = (RTSP_SERVER *)hRTSPServerHandle;
	
    ulMsgBuf[0] = dwSessionID;
    iRet = OSMsgQueue_Send((HANDLE)pRTSPServer->ulDeleteQueue,(DWORD*)ulMsgBuf);
#ifdef  _LINUX
	if( iRet != 0 )
	{
		syslog(LOG_ALERT,"Remove RTSP session failed! One zombie!!!!\n");
		printf("!!!!!!!!!!!!!!!!!!!!!!!!One Zombie Client!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
	}
	else
	{
		printf("!!!!!!!!!!!!!!!!!!!!!!!!Remove One RTSP session!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
	}

#endif
	return iRet;
 
}

int RTSPServer_RemoveSessionFromAPI(HANDLE hRTSPServerHandle, DWORD dwSessionID)
{
    RTSP_SERVER *pRTSPServer;
    unsigned long ulMsgBuf[4];
	int			iRet;
#ifdef _SHARED_MEM
	int			i;
#endif

	if( hRTSPServerHandle == NULL )
		return -1;

    pRTSPServer = (RTSP_SERVER *)hRTSPServerHandle;
	
#ifdef _SHARED_MEM
	for(i=0 ; i<pRTSPServer->iMaxSessionNumber ; i++)
   	{
		if( (pRTSPServer->pClient[i].ulSessionID == dwSessionID) && (pRTSPServer->pClient[i].bSeamlessStream == TRUE) )
        {
			pRTSPServer->pClient[i].bNormalDisconnected = TRUE;
		}
	}
#endif

    ulMsgBuf[0] = dwSessionID;
    iRet = OSMsgQueue_Send((HANDLE)pRTSPServer->ulDeleteQueue,(DWORD*)ulMsgBuf);
#ifdef  _LINUX
	if( iRet != 0 )
	{
		syslog(LOG_ALERT,"Remove RTSP session failed! One zombie!!!!\n");
		printf("!!!!!!!!!!!!!!!!!!!!!!!!One Zombie Client!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
	}
	else
	{
		printf("!!!!!!!!!!!!!!!!!!!!!!!!Remove One RTSP session!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
	}

#endif
	return iRet;
 
}

#ifdef _SHARED_MEM
//20101208 Modified by danny For GUID format change
//20100105 Added For Seamless Recording
int RTSPServer_ResetGUID(HANDLE hRTSPServerHandle, char* pcSessionGUID)
{
    RTSP_SERVER *pRTSPServer;
    unsigned long ulMsgBuf[4];
	int			iRet;

	if( hRTSPServerHandle == NULL )
		return -1;

    pRTSPServer = (RTSP_SERVER *)hRTSPServerHandle;

    ulMsgBuf[0] = (unsigned long)pcSessionGUID;
    iRet = OSMsgQueue_Send((HANDLE)pRTSPServer->hResetGUIDQueue,(DWORD*)ulMsgBuf);
#ifdef  _LINUX
	if( iRet != 0 )
	{
		syslog(LOG_ALERT,"Send GUID to ResetGUIDQueue failed!\n");
		printf("Send GUID %s to ResetGUIDQueue failed!\n", pcSessionGUID);
	}
	else
	{
		printf("Send GUID %s to ResetGUIDQueue OK!\n", pcSessionGUID);
	}

#endif
	return iRet;
 
}
#endif

void RTSPServer_TeardownSessionOK(HANDLE hRTSPServerHandle, DWORD dwSessionID)
{
    RTSP_SERVER *pRTSPServer;
    unsigned long ulMsgBuf[4];

    if( hRTSPServerHandle == NULL )
        return;
    
    pRTSPServer = (RTSP_SERVER *)hRTSPServerHandle;
    ulMsgBuf[0] = dwSessionID;
    OSMsgQueue_Send(pRTSPServer->hTeardownOKQueue,(DWORD*)ulMsgBuf);
    
    return;
    
}

void RTSPServer_CheckTeardownOKQueue(RTSP_SERVER *pRTSPServer)
{
    unsigned long ulMsgBuf[4];
    unsigned long ulSessionID;
    int i;
	//20120724 added by Jimmy For metadata
	int j;

    while( OSMsgQueue_Receive(pRTSPServer->hTeardownOKQueue,(DWORD*)ulMsgBuf,0) == S_OK)
    {
        ulSessionID = ulMsgBuf[0];
        for(i=0 ; i<pRTSPServer->iMaxSessionNumber ; i++)
        {
            if( pRTSPServer->pClient[i].ulSessionID == ulSessionID )
            {
				//20120724 added by Jimmy For metadata
				//make sure video and audio port are all released
				/*if( pRTSPServer->pClient[i].usPort[1][0] == 0 )
					pRTSPServer->pClient[i].usPort[0][0] = 0;			
				else
					pRTSPServer->pClient[i].usPort[1][0] = 0;*/
				for( j = MEDIA_TYPE_NUMBER - 1; j >= 0; j-- )
				{
					if( pRTSPServer->pClient[i].usPort[j][0] != 0 )
						{
							pRTSPServer->pClient[i].usPort[j][0] = 0;
							break;
						}
				}


				if( pRTSPServer->pClient[i].usPort[0][0] == 0 )
				{
					if( !RTSPServer_SendReply(200, 0,&pRTSPServer->pClient[i],pRTSPServer) )
					{    
						printf("Teardown 200 OK\r\n");    
						pRTSPServer->pClient[i].parent = pRTSPServer;
						RTSPServer_InitClient(&pRTSPServer->pClient[i]);
					}
					else
						printf("Reply Teardown 200 OK failed\r\n");    
				}
            } 
        } 
    }

}


void RTSPServer_CheckDeleteQueue(RTSP_SERVER *pRTSPServer)
{
    unsigned long ulMsgBuf[4];
    unsigned long ulSessionID;
    int i;

    while( OSMsgQueue_Receive((HANDLE)pRTSPServer->ulDeleteQueue,(DWORD*)ulMsgBuf,0) == S_OK )
    {
        ulSessionID = ulMsgBuf[0];

        for(i=0 ; i<pRTSPServer->iMaxSessionNumber ; i++)
        {
            if( pRTSPServer->pClient[i].ulSessionID == ulSessionID )
            {
                printf("RTSP session %lu %d removed %d\r\n",ulSessionID,pRTSPServer->pClient[i].iRecvSockfd,pRTSPServer->pClient[i].iSendSockfd);
				//20130315 added by Jimmy to log more information
				syslog(LOG_DEBUG, "RTSP session %lu %d removed %d\r\n",ulSessionID,pRTSPServer->pClient[i].iRecvSockfd,pRTSPServer->pClient[i].iSendSockfd);
                //2005/05/31closesocket(pRTSPServer->pClient[i].iSockfd);
                pRTSPServer->pClient[i].parent = pRTSPServer;
                RTSPServer_InitClient(&pRTSPServer->pClient[i]);
                break;
            } 
        } 
    }

}

//20110401 Added by danny For support RTSPServer thread watchdog
void RTSPServer_KickWatchDog(RTSP_SERVER *pRTSPServer)
{
    if( pRTSPServer->fcallback(pRTSPServer->hParentHandle, RTSPSERVER_CALLBACKFLAG_KICK_WATCHDOG, 0, 0) != S_OK )
	{
		DbgLog((dfCONSOLE|dfINTERNAL,"RTSPSERVER_CALLBACKFLAG_KICK_WATCHDOG failed!\n"));
		printf("[%s] RTSPSERVER_CALLBACKFLAG_KICK_WATCHDOG failed!\n", __FUNCTION__);
    }
}

#ifdef _SHARED_MEM
//20101208 Modified by danny For GUID format change
//20100105 Added For Seamless Recording
void RTSPServer_CheckResetGUIDQueue(RTSP_SERVER *pRTSPServer)
{
    unsigned long ulMsgBuf[4];
    //unsigned long ulSessionGUID;
	char		  acSeamlessRecordingGUID[RTSPS_Seamless_Recording_GUID_LENGTH];
	int i, iRet;

    while( OSMsgQueue_Receive((HANDLE)pRTSPServer->hResetGUIDQueue,(DWORD*)ulMsgBuf,0) == S_OK )
    {
        //ulSessionGUID = ulMsgBuf[0];
		if( ulMsgBuf[0] != 0 )
		{
			memcpy((void*)acSeamlessRecordingGUID ,(void*)ulMsgBuf[0], sizeof(acSeamlessRecordingGUID));
		}
		printf("[%s] Reset GUID %s\n", __FUNCTION__, acSeamlessRecordingGUID);
        for(i=0 ; i<pRTSPServer->tSeamlessRecordingInfo.iSeamlessMaxConnection ; i++)
        {
			iRet = rtspstrncasecmp(acSeamlessRecordingGUID, pRTSPServer->tSeamlessRecordingInfo.tGUIDListInfo[i].acGUID, RTSPS_Seamless_Recording_GUID_LENGTH);
			if( iRet == 0 )
            {
				pRTSPServer->tSeamlessRecordingInfo.iSeamlessConnectionNumber-=pRTSPServer->tSeamlessRecordingInfo.tGUIDListInfo[i].iUnderRecording;
				memset(&pRTSPServer->tSeamlessRecordingInfo.tGUIDListInfo[i], 0, sizeof(TGUIDListInfo));
				printf("[%s] Reset GUID %s OK!\n", __FUNCTION__, acSeamlessRecordingGUID);

				RTSPServer_CheckSeamlessAllGUIDsBack(pRTSPServer);
				if( pRTSPServer->fcallback(pRTSPServer->hParentHandle
									,RTSPSERVER_CALLBACKFLAG_UPDATE_GUIDLISTINFO
									,(void*)&pRTSPServer->tSeamlessRecordingInfo,(void*)i) != S_OK )
				{
					DbgLog((dfCONSOLE|dfINTERNAL,"RTSPSERVER_CALLBACKFLAG_UPDATE_GUIDLISTINFO failed!\n"));
					printf("[%s] RTSPSERVER_CALLBACKFLAG_UPDATE_GUIDLISTINFO failed!\n", __FUNCTION__);
    			}
				
                break;
            } 
        } 
    }

}
#endif

void  RTSPServer_CheckHttpSockQueue(RTSP_SERVER *pRTSPServer)
{
    unsigned long ulMsgBuf[4];
    int i,iLen=0;
    THTTPCONNINFO	*ptHttpConnInfo;

    while( OSMsgQueue_Receive((HANDLE)pRTSPServer->hHttpSockQueue,(DWORD*)ulMsgBuf,0) == S_OK )
    {
		ptHttpConnInfo = (THTTPCONNINFO	*)ulMsgBuf[0];
		TelnetShell_DbgPrint("-----HTTP recv sock %d send sock %d add into RTSP\r\n----",ptHttpConnInfo->iRecvSock,ptHttpConnInfo->iSendSock);
		//20140418 modified by Charles for http port support rtsp describe command
		if( ulMsgBuf[1] == RTSP_HTTP_ADD_PAIR || ulMsgBuf[1] == RTSP_HTTP_ADD_DESCRIBE)
		{
			for (i = 0; i < pRTSPServer->iMaxSessionNumber; i++)
			{
				if( (pRTSPServer->pClient[i].iRecvSockfd < 0 ) && (pRTSPServer->pClient[i].iSendSockfd < 0 ) )
				{
					iLen = strlen(ptHttpConnInfo->acMessageBuffer);
										
					if( iLen != 0 )
					{
						if(  iLen < RTSP_HTTP_MESSAGE_LEN && iLen > 0 )
						{
							memcpy(pRTSPServer->pClient[i].acRecvBuffer,ptHttpConnInfo->acMessageBuffer,iLen);
							pRTSPServer->pClient[i].iRecvSize = iLen;
						}
						else
						{
							closesocket(ptHttpConnInfo->iRecvSock);
							closesocket(ptHttpConnInfo->iSendSock);
#ifdef _LINUX
							syslog(LOG_ALERT,"!!!Error RTSP message from HTTP server!!!\n");
#endif
							TelnetShell_DbgPrint("[RTSPServer] Error RTSP message from HTTP server\r\n");
							break;
						}
					}

					pRTSPServer->pClient[i].iRecvSockfd = ptHttpConnInfo->iRecvSock;	
					pRTSPServer->pClient[i].iSendSockfd = ptHttpConnInfo->iSendSock;	
					//20140418 modified by Charles for http port support rtsp describe command
					if(ulMsgBuf[1] == RTSP_HTTP_ADD_PAIR)
					{
						pRTSPServer->pClient[i].iRTPStreamingMode = RTP_OVER_HTTP;
						strncpy(pRTSPServer->pClient[i].acSessionCookie,ptHttpConnInfo->acSessionCookie,RTSP_HTTP_COOKIE_LEN-1);
					}
					
					OSTick_GetMSec(&pRTSPServer->pClient[i].dwBaseMSec);
					pRTSPServer->iCurrentSessionNumber ++;

					//20080623 Parse port from HTTP connection!
					{
						int						iLen = 0;
						struct sockaddr_storage tClient_addr;
						struct sockaddr_in      *ptClient4_addr=NULL;
						struct sockaddr_in6     *ptClient6_addr=NULL;
						struct sockaddr_in	v4mapaddr;	
						memset(&v4mapaddr, 0, sizeof(v4mapaddr));

						iLen = sizeof(tClient_addr);
						if (getpeername(pRTSPServer->pClient[i].iRecvSockfd,(struct sockaddr *)&tClient_addr, (unsigned int *)&iLen) == S_OK )
    					{
							if (tClient_addr.ss_family == AF_INET)
							{
								ptClient4_addr = (struct sockaddr_in*)&tClient_addr;
								pRTSPServer->pClient[i].wClientPort = ntohs(ptClient4_addr->sin_port);
							}
							else if(tClient_addr.ss_family == AF_INET6)
							{
								ptClient6_addr = (struct sockaddr_in6*)&tClient_addr;
								pRTSPServer->pClient[i].wClientPort = ntohs(ptClient6_addr->sin6_port);
							}
							else
							{
								printf("[RTSP server]: Error parsing port for RTSP over HTTP connection!\r\n");
								pRTSPServer->pClient[i].wClientPort = 0;
							}
							//20140423 added by Charles for http port support rtsp describe command
#ifdef _INET6
							if (IN6_IS_ADDR_V4MAPPED(&((struct sockaddr_in6*)&tClient_addr)->sin6_addr))
							{
								struct in6_addr in6addr;
								in6addr = ((struct sockaddr_in6*)&tClient_addr)->sin6_addr;

								printf( "Someone connecting from HTTP IPv4 %d.%d.%d.%d\n",
									in6addr.s6_addr[12],
									in6addr.s6_addr[13],
									in6addr.s6_addr[14],
									in6addr.s6_addr[15]);
								v4mapaddr.sin_addr.s_addr =  MAKEFOURCC(in6addr.s6_addr[12], in6addr.s6_addr[13], in6addr.s6_addr[14], in6addr.s6_addr[15]);								
							}
							else
							{
								char szPresentString[64]="";
								printf( "Someone connecting from HTTP IPV6 %s\n", 
									inet_ntop(AF_INET6, &((struct sockaddr_in6*)&tClient_addr)->sin6_addr, szPresentString, sizeof(szPresentString)));
							}
#endif
						}
						else
						{
							printf("[RTSP server]: Error parsing port for RTSP over HTTP connection!\r\n");
							pRTSPServer->pClient[i].wClientPort = 0;
						}

						if(ulMsgBuf[1] == RTSP_HTTP_ADD_DESCRIBE)
						{
							/* 20090225 QOS */
							RTSPServer_SetQosToSocket(pRTSPServer->pClient[i].iSendSockfd, &pRTSPServer->tQosInfo, eQosMuxedType);
							int iVal=1;
							setsockopt(pRTSPServer->pClient[i].iRecvSockfd,IPPROTO_TCP,TCP_NODELAY,(char *)&iVal,sizeof(int) ); 			   
							
							unsigned long ulNonBlock=1;
							ioctlsocket(pRTSPServer->pClient[i].iRecvSockfd,FIONBIO,&ulNonBlock);
#ifdef _INET6
							pRTSPServer->pClient[i].ulClientAddress = v4mapaddr.sin_addr.s_addr;
							memcpy( &pRTSPServer->pClient[i].tClientSckAddr, (struct sockaddr_in6*)&tClient_addr, sizeof(RTSP_SOCKADDR));
#else
							pRTSPServer->pClient[i].ulClientAddress = ((struct sockaddr_in*)&tClient_addr)->sin_addr.s_addr;
#endif
						}
					}

					if( pRTSPServer->iCurrentSessionNumber == pRTSPServer->iMaxSessionNumber )
					{
						pRTSPServer->pClient[i].iTimeOut = -10;
						printf("[RTSP server]: Internal limit max RTP over HTTP connection reached\r\n");
					}

#ifdef _LINUX
					//syslog(LOG_ERR,"[RTSPServer] RTP over HTTP added %d %d Curr%d\n",(int)ulMsgBuf[0],(int)ulMsgBuf[1],pRTSPServer->iCurrentSessionNumber);
#endif //_LINUX                
					TelnetShell_DbgPrint("[RTSPServer] RTP over HTTP added send %d recv %d Curr%d\n",pRTSPServer->pClient[i].iSendSockfd,pRTSPServer->pClient[i].iRecvSockfd,pRTSPServer->iCurrentSessionNumber);
					break;
				}
			}

			if( i == pRTSPServer->iMaxSessionNumber ) // exceed max session number
			{
				closesocket(ptHttpConnInfo->iRecvSock);
				closesocket(ptHttpConnInfo->iSendSock);
				TelnetShell_DbgPrint("[RTSPServer] Exceed Max RTP over HTTP send %d recv %d Curr%d\n",pRTSPServer->pClient[i].iSendSockfd,pRTSPServer->pClient[i].iRecvSockfd,pRTSPServer->iCurrentSessionNumber);
			}
		}
		else  //QT periodically send POST with OPTION for keeping alive
		{
			for (i = 0; i < pRTSPServer->iMaxSessionNumber; i++)
			{
				if (pRTSPServer->pClient[i].iRTPStreamingMode == RTP_OVER_HTTP)		
				{
					if( strcmp(pRTSPServer->pClient[i].acSessionCookie,ptHttpConnInfo->acSessionCookie) == 0 )
					{
						//20091204 Avoid VitaminCtrl normal-post socket assign too fast before iRecvSockfd not close
						if (pRTSPServer->pClient[i].iRecvSockfd != -1)
						{
							closesocket(pRTSPServer->pClient[i].iRecvSockfd);
							printf("Normal POST socket %d closed\r\n", pRTSPServer->pClient[i].iRecvSockfd);
							pRTSPServer->pClient[i].iRecvSockfd = -1;
						}
						pRTSPServer->pClient[i].iRecvSockfd = ptHttpConnInfo->iRecvSock;	
						//pRTSPServer->pClient[i].iTimeOut = 0;
						OSTick_GetMSec(&pRTSPServer->pClient[i].dwBaseMSec);

						iLen = strlen(ptHttpConnInfo->acMessageBuffer);

						//HTTP server might already receive some RTSP msg
						if( iLen != 0 )
						{
							if( iLen < RTSP_HTTP_MESSAGE_LEN && iLen > 0 )
							{
								memcpy(pRTSPServer->pClient[i].acRecvBuffer,ptHttpConnInfo->acMessageBuffer,iLen);
								pRTSPServer->pClient[i].iRecvSize = iLen;
							}
							else
							{
								closesocket(ptHttpConnInfo->iRecvSock);
								TelnetShell_DbgPrint("[RTSPServer] Error RTSP message from HTTP server (POST only)\r\n");
							}
						}
						printf("POST socket added!!!\r\n");
						break;
					}
				}
			}

			if( i == pRTSPServer->iMaxSessionNumber ) // exceed max session number
			{
				closesocket(ptHttpConnInfo->iRecvSock);
				TelnetShell_DbgPrint("[RTSPServer] Quicktime Wrong Session Cookie for incoming POST %d\r\n",ptHttpConnInfo->iRecvSock);
			}	
		}

		OSMsgQueue_Send((HANDLE)pRTSPServer->hHttpInfoBuffQueue,(DWORD*)ulMsgBuf);
    }

    return;
}    


int RTSPServer_SessionStart(RTSP_CLIENT *pClient,RTSPSERVER_SESSIONINFORMATION *pSessionInfo)
{
	RTSP_SERVER *pRTSPServer = pClient->parent;
	
    if( pRTSPServer != NULL && pRTSPServer->fcallback != NULL)
    {
		RTSPServer_UpdateClientSessionInfo(pRTSPServer);

        if( pSessionInfo->iRTPStreamingType == RTP_OVER_TCP )
            TelnetShell_DbgPrint("RTP over RTSP session %lu Add %d\r\n",(long)pSessionInfo->dwSessionID, *pSessionInfo->psktRTSPSocket);
        else 
		{
#ifdef RTSPRTP_MULTICAST
			if( pSessionInfo->iMulticast)
				TelnetShell_DbgPrint("RTP over UDP session multicast %lu Add\r\n",(long)pSessionInfo->dwSessionID);
			else
#endif
				TelnetShell_DbgPrint("RTP over UDP session unicast %lu Add %d\r\n",(long)pSessionInfo->dwSessionID,pSessionInfo->iVivotekClient);
		}

        //20120925 added by Jimmy for ONVIF backchannel
        //2005/05/31 modified by ShengFu
        if( (pClient->acMediaType[0][0] == 0) && (pClient->bAudioback == TRUE) )
        {
            TelnetShell_DbgPrint("RTSP Session Start OK. Audioback only\r\n");
            return 0;
        }
        else if( pRTSPServer->fcallback(pRTSPServer->hParentHandle
	                ,RTSPSERVER_CALLBACKFLAG_SESSION_START
                    ,(void*)pSessionInfo,(void*)&pClient->hTCPMuxCS) != 0)
            return -1;
        else
		{
			TelnetShell_DbgPrint("RTSP Session Start OK\r\n");
            return 0;                        
		}		
    }                    
    
	return -1;
}

int RTSPServer_SessionStop(RTSP_SERVER *pRTSPServer,DWORD dwSessionID, int MulticastIndex)
{

    if( pRTSPServer != NULL && pRTSPServer->fcallback != NULL)
    {
#ifdef RTSPRTP_MULTICAST
		if( MulticastIndex >0 )
		{
			//We should close ondemand multicast here!
			if( pRTSPServer->iCurrMulticastNumber[MulticastIndex-1] == 1 && MulticastIndex > (RTSP_MULTICASTNUMBER))
			{
				MULTICASTINFO	tMulticastInfo; 
				memset(&tMulticastInfo, 0, sizeof(MULTICASTINFO));
				RTSPServer_UpdateMulticastParameters(pRTSPServer, MulticastIndex-1, &tMulticastInfo);
	            //close ondemand multicast socket
				pRTSPServer->fcallback(pRTSPServer->hParentHandle, RTSPSERVER_CALLBACKFLAG_CLOSE_MULTICASTSOCKET, (void *)MulticastIndex, 0);
			}
			
			//Modified by Louis 2007/11/16 to fix multicast bug!
			if( pRTSPServer->iCurrMulticastNumber[MulticastIndex-1] == 1 ) 
			{
				return pRTSPServer->fcallback(pRTSPServer->hParentHandle, RTSPSERVER_CALLBACKFLAG_SESSION_STOP, (void*)dwSessionID, (void*)MulticastIndex);
			}
			else if(pRTSPServer->iCurrMulticastNumber[MulticastIndex-1] > 1)
			{
				return pRTSPServer->fcallback(pRTSPServer->hParentHandle, RTSPSERVER_CALLBACKFLAG_SESSION_REMOVE_BACKCHANNEL, (void*)dwSessionID, (void*)MulticastIndex);
			}
		}
		else
#endif		
		{
			return pRTSPServer->fcallback(pRTSPServer->hParentHandle
	                ,RTSPSERVER_CALLBACKFLAG_SESSION_STOP
                    ,(void*)dwSessionID,0);
		}
    }     
                   
    return -1;
}

int RTSPServer_SessionPause(RTSP_CLIENT *pClient,DWORD dwSessionID)
{
	//20151111
	pClient->iIsPause = 1;
	RTSP_SERVER* pRTSPServer = pClient->parent;
    if( pRTSPServer != NULL && pRTSPServer->fcallback != NULL)
    {
        return pRTSPServer->fcallback(pRTSPServer->hParentHandle
	                ,RTSPSERVER_CALLBACKFLAG_SESSION_PAUSE
                    ,(void*)dwSessionID,0);
    }     
                   
    return -1;
}

int RTSPServer_SessionResume(RTSP_CLIENT *pClient,DWORD dwSessionID)
{
	//20151111
	pClient->iIsPause = 0;
	RTSP_SERVER* pRTSPServer = pClient->parent;
	
    if( pRTSPServer != NULL && pRTSPServer->fcallback != NULL)
    {
        return pRTSPServer->fcallback(pRTSPServer->hParentHandle
	                ,RTSPSERVER_CALLBACKFLAG_SESSION_RESUME
                    ,(void*)dwSessionID,0);
    }     
                   
    return -1;
}

int RTSPServer_UpdateClientSessionInfo(RTSP_SERVER *pRTSPServer)
{
	//20080611 Modified from 1000 to 2000 Louis
	//20110127 Modified from 2000 to 3000 Danny
	char 	acSessionInfoBuf[3000];
	int		i = 0;
#ifdef _SESSION_MGR	
	int		iCount = 0;
#endif
	//CID:436, CHECKER:REVERSE_INULL
	if(pRTSPServer == NULL)
	{
		return -1;
	}
#ifdef _SESSION_MGR	
	//Whole function re-write by Louis 20080611 for connection status
	memset((void*)acSessionInfoBuf,0,sizeof(acSessionInfoBuf));
	snprintf(acSessionInfoBuf, sizeof(acSessionInfoBuf) - 1, "<connectstatus>\n");
	/*
		20120906 modifeid by Jimmy to fix sessioninfo not concsistent issue:
		The seesion number in sessioninfo must be concsistent with the managed session number( pClient->bManagedbySessionMgr==TRUE ), not iCurrentSessionNumber.
	*/
	snprintf(acSessionInfoBuf + strlen(acSessionInfoBuf), sizeof(acSessionInfoBuf) - strlen(acSessionInfoBuf) - 1,"<totalconnection>%d</totalconnection>\n",RTSPServer_GetManagedSessionNumber(pRTSPServer));
	//snprintf(acSessionInfoBuf + strlen(acSessionInfoBuf), sizeof(acSessionInfoBuf) - strlen(acSessionInfoBuf) - 1,"<totalconnection>%d</totalconnection>\n",pRTSPServer->iCurrentSessionNumber);
    
	for(i=0; i<pRTSPServer->iMaxSessionNumber; i++)
	{
		if( pRTSPServer->pClient[i].ulSessionID != 0 )
		{
			//Write Initial identifier
			iCount++;
			snprintf(acSessionInfoBuf + strlen(acSessionInfoBuf), sizeof(acSessionInfoBuf) - strlen(acSessionInfoBuf) - 1,"<i%d>\n", iCount);

			//Write IP Address
#ifdef _INET6
			if (pRTSPServer->pClient[i].ulClientAddress == 0)
			{
				char szPresentString[64]="";
				snprintf( acSessionInfoBuf + strlen(acSessionInfoBuf), sizeof(acSessionInfoBuf) - strlen(acSessionInfoBuf) - 1,
					"<ipaddr>%s</ipaddr>\n", 
					inet_ntop(AF_INET6, &pRTSPServer->pClient[i].tClientSckAddr.sin6_addr, szPresentString, sizeof(szPresentString)));
			}
			else
#endif		
			{
			snprintf(acSessionInfoBuf + strlen(acSessionInfoBuf), sizeof(acSessionInfoBuf) - strlen(acSessionInfoBuf) - 1,"<ipaddr>%s</ipaddr>\n", (char*)inet_ntoa(*((struct in_addr*)&(pRTSPServer->pClient[i].ulClientAddress))));
			}
			//Write Port
			snprintf(acSessionInfoBuf + strlen(acSessionInfoBuf), sizeof(acSessionInfoBuf) - strlen(acSessionInfoBuf) - 1,"<port>%d</port>\n", pRTSPServer->pClient[i].wClientPort);
			//Write Session ID
			snprintf(acSessionInfoBuf + strlen(acSessionInfoBuf), sizeof(acSessionInfoBuf) - strlen(acSessionInfoBuf) - 1,"<session>%lu</session>\n", pRTSPServer->pClient[i].ulSessionID);
			//Write Stream
			snprintf(acSessionInfoBuf + strlen(acSessionInfoBuf), sizeof(acSessionInfoBuf) - strlen(acSessionInfoBuf) - 1,"<stream>%d</stream>\n", pRTSPServer->pClient[i].iSDPIndex);
			//Write User ID
			if(pRTSPServer->pClient[i].acUserName[0] == 0)
			{
				//No Authentication
				snprintf(acSessionInfoBuf + strlen(acSessionInfoBuf), sizeof(acSessionInfoBuf) - strlen(acSessionInfoBuf) - 1,"<uid>Authentication Disabled</uid>\n");
			}
			else
			{
				snprintf(acSessionInfoBuf + strlen(acSessionInfoBuf), sizeof(acSessionInfoBuf) - strlen(acSessionInfoBuf) - 1,"<uid>%s</uid>\n", pRTSPServer->pClient[i].acUserName);
			}
			//Write Start Time
			snprintf(acSessionInfoBuf + strlen(acSessionInfoBuf), sizeof(acSessionInfoBuf) - strlen(acSessionInfoBuf) - 1,"<starttime>%u</starttime>\n", pRTSPServer->pClient[i].dwBaseMSec);
			//Write ending identifier
			snprintf(acSessionInfoBuf + strlen(acSessionInfoBuf), sizeof(acSessionInfoBuf) - strlen(acSessionInfoBuf) - 1,"</i%d>\n", iCount);
		}
        
        if( (sizeof(acSessionInfoBuf) - strlen(acSessionInfoBuf) ) < 80 )
        {
        	printf("[RTSP server]: buff is too small for client session information\r\n");
            return -1;
        }
	}
	//Add XML Tail
	snprintf(acSessionInfoBuf + strlen(acSessionInfoBuf), sizeof(acSessionInfoBuf) - strlen(acSessionInfoBuf) - 1,"</connectstatus>\n");
	
	if( pRTSPServer != NULL && pRTSPServer->fcallback != NULL)
    {
        return pRTSPServer->fcallback(pRTSPServer->hParentHandle
	                ,RTSPSERVER_CALLBACKFLAG_UPDATE_SESSIONINFO
                    ,(void*)acSessionInfoBuf,0);
    }     
#else

	memset((void*)acSessionInfoBuf,0,sizeof(acSessionInfoBuf));
	snprintf(acSessionInfoBuf, sizeof(acSessionInfoBuf) - 1, "%d\n", pRTSPServer->iCurrentSessionNumber);
   
	for(i=0; i<pRTSPServer->iMaxSessionNumber; i++)
	{
		if( pRTSPServer->pClient[i].ulSessionID != 0 )
		{
#ifdef _INET6
			if (pRTSPServer->pClient[i].ulClientAddress == 0)
			{
				char szPresentString[64]="";

				snprintf( acSessionInfoBuf + strlen(acSessionInfoBuf), sizeof(acSessionInfoBuf) - strlen(acSessionInfoBuf) - 1,
					"%s  %lu\n", 
					inet_ntop(AF_INET6, &pRTSPServer->pClient[i].tClientSckAddr.sin6_addr, szPresentString, sizeof(szPresentString)),
					pRTSPServer->pClient[i].ulSessionID
					);
			}
			else
#endif
			{
			snprintf(acSessionInfoBuf + strlen(acSessionInfoBuf), sizeof(acSessionInfoBuf) - strlen(acSessionInfoBuf) - 1,"%s %lu\n",(char*)inet_ntoa(*((struct in_addr*)&(pRTSPServer->pClient[i].ulClientAddress))),pRTSPServer->pClient[i].ulSessionID);
			}
		}
        
        if( (sizeof(acSessionInfoBuf) - strlen(acSessionInfoBuf) ) < 30 )
        {
        	printf("[RTSP server]: buff is too small for client session information\r\n");
            return -1;
        }
	}
	if( pRTSPServer != NULL && pRTSPServer->fcallback != NULL)
    {
        return pRTSPServer->fcallback(pRTSPServer->hParentHandle
	                ,RTSPSERVER_CALLBACKFLAG_UPDATE_SESSIONINFO
                    ,(void*)acSessionInfoBuf,0);
    }     
#endif    
                   
    return -1;
	
}

int RTSPServer_UpdateRTPSession(RTSP_SERVER *pRTSPServer,RTSPSERVER_SESSIONINFORMATION *pSessionInfo)
{

    if( pRTSPServer != NULL && pRTSPServer->fcallback != NULL)
    {
        return pRTSPServer->fcallback(pRTSPServer->hParentHandle
	                ,RTSPSERVER_CALLBACKFLAG_SESSION_RTPUPDATE
                    ,(void*)pSessionInfo,0);
    }     
                   
    return -1;
}

//int RTSPServer_GetSDP(RTSP_SERVER *pRTSPServer,char *pcSDPName,char *pcSDPBuffer)
int RTSPServer_GetSDP(RTSP_SERVER *pRTSPServer,RTSPSERVER_SDPREQUEST *pstSDPRequest)
{
    if( pRTSPServer != NULL && pRTSPServer->fcallback != NULL)
    {
        return pRTSPServer->fcallback(pRTSPServer->hParentHandle
	                ,RTSPSERVER_CALLBACKFLAG_SDP_REQUEST
                    ,(void*)pstSDPRequest,0);
    }                    
    
    return -1;	
}
//20090312 multiple stream 
int	RTSPServer_ComposeSDP(RTSP_SERVER *pRTSPServer,RTSPSERVER_SDPREQUEST *pstSDPRequest)
{
    if( pRTSPServer != NULL && pRTSPServer->fcallback != NULL)
    {
		return pRTSPServer->fcallback(pRTSPServer->hParentHandle
	                ,RTSPSERVER_CALLBACKFLAG_COMPOSE_SDP
                    ,(void*)pstSDPRequest,0);
    }                    
    
    return -1;	
}
int	RTSPServer_ResetCI(RTSP_SERVER *pRTSPServer, int iSDPIndex)
{
    if( pRTSPServer != NULL && pRTSPServer->fcallback != NULL)
    {
        return pRTSPServer->fcallback(pRTSPServer->hParentHandle
	                ,RTSPSERVER_CALLBACKFLAG_RESET_CI
                    ,(void*)iSDPIndex,0);
    }                    
    
    return -1;	
}

int RTSPServer_IPCheck(RTSP_SERVER *pRTSPServer,RTSPSERVER_CLIENTIP *pClientIP)
{
  
    if( pRTSPServer != NULL && pRTSPServer->fcallback != NULL)
    {
        return pRTSPServer->fcallback(pRTSPServer->hParentHandle
	               ,RTSPSERVER_CALLBACKFLAG_ACCESSIP_CHECK
                   ,(void*)pClientIP,0);	
    }                   		  

    return -1;
}

int RTSPServer_Authentication(RTSP_SERVER *pRTSPServer,struct sockaddr_in cliaddr)
{
    RTSPSERVER_CLIENTIP ClientIP;

    ClientIP.ulIP = cliaddr.sin_addr.s_addr;
    ClientIP.usPort = cliaddr.sin_port;

    return RTSPServer_IPCheck(pRTSPServer,&ClientIP);

}


/*void RTSPServer_SymmetricRTPPacketParseSSRC(char* pcBuff,DWORD *pdwSSRC)
{
 
	DWORD *header=(DWORD*)pcBuff;

	ConvertHeader2h((DWORD*)pcBuff,0,3);
	//pClient->ulInitTimestamp[i]=header[2];
    //pClient->usInitSequence[i] =(UINT16)(bitfieldGet(header[0],0,16));
	*pdwSSRC=header[1];
    TelnetShell_DbgPrint("Symmetric RTP SSRC = %lu\r\n",header[1]);    
 
	return;
}


void RTSPServer_MatchSSRCandSockAddr(RTSP_SERVER *pRTSPServer,DWORD dwSSRC,struct sockaddr_in *psaddr)
{
	int i,j;

	for( i=0 ; i<pRTSPServer->iMaxSessionNumber ; i++)
	{
		if( pRTSPServer->pClient[i].ulSessionID != 0 )
		{
			for( j=0; j<2 ; j++)
			{
				if( pRTSPServer->pClient[i].ulSSRC[j] == dwSSRC )
				{
					memcpy((void*)&pRTSPServer->pClient[i].NATRTPAddr[j],(void*)psaddr,sizeof(struct sockaddr_in));
					pRTSPServer->pClient[i].iSymRTP --;
					break;
				}
			}		
		}	
	}
}*/

int RTSPServer_MatchSessionIDandSockAddr(RTSP_SERVER *pRTSPServer,DWORD dwSessionID,RTSP_SOCKADDR *psaddr,int iFlag)
{
	int i;
	//int iVideo;
	//20120809 added by Jimmy for metadata
	int j,iTrackMediaType;

	for( i=0 ; i<pRTSPServer->iMaxSessionNumber ; i++)
	{
		if( pRTSPServer->pClient[i].ulSessionID == dwSessionID )
		{
			//20120809 modified by Jimmy for metadata
			/*
			iVideo = pRTSPServer->fcallback(pRTSPServer->hParentHandle
								,RTSPSERVER_CALLBACKFLAG_CHECK_VIDEO_TRACK
								,(void*)pRTSPServer->pClient[i].iSDPIndex,pRTSPServer->pClient[i].acMediaType[0]);	

			if( iFlag == 0 ) //video
			{

				//if( strcmp(pRTSPServer->pClient[i].acMediaType[0],RTSP_VIDEO_ACCESS_NAME1) == 0 )
				if( iVideo == TRUE )
				{
					memcpy((void*)&pRTSPServer->pClient[i].NATRTPAddr[0],(void*)psaddr,sizeof(RTSP_SOCKADDR));
					pRTSPServer->pClient[i].iSymRTP --;
				}
				else
				{
					memcpy((void*)&pRTSPServer->pClient[i].NATRTPAddr[1],(void*)psaddr,sizeof(RTSP_SOCKADDR));
					pRTSPServer->pClient[i].iSymRTP --;							
				}
			}
			else //audio
			{
				//if( strcmp(pRTSPServer->pClient[i].acMediaType[0],RTSP_VIDEO_ACCESS_NAME1) != 0)
				if( iVideo == FALSE )
				{
					memcpy((void*)&pRTSPServer->pClient[i].NATRTPAddr[0],(void*)psaddr,sizeof(RTSP_SOCKADDR));
					pRTSPServer->pClient[i].iSymRTP --;
				}
				else
				{
					memcpy((void*)&pRTSPServer->pClient[i].NATRTPAddr[1],(void*)psaddr,sizeof(RTSP_SOCKADDR));
					pRTSPServer->pClient[i].iSymRTP --;							
				}
				
			}
			*/
			for ( j = 0; j < MEDIA_TYPE_NUMBER; j++)
			{
				if( pRTSPServer->pClient[i].acMediaType[j][0] == 0 )
					break;

				iTrackMediaType = pRTSPServer->fcallback(pRTSPServer->hParentHandle
														,RTSPSERVER_CALLBACKFLAG_CHECK_TRACK_MEDIATYPE
														,(void*)pRTSPServer->pClient[i].iSDPIndex,pRTSPServer->pClient[i].acMediaType[j]);
			
				if (( (iFlag == 0) && (iTrackMediaType == RTSPSERVER_MEDIATYPE_VIDEO) ) ||
					( (iFlag == 1) && (iTrackMediaType == RTSPSERVER_MEDIATYPE_AUDIO) ) ||
					( (iFlag == 2) && (iTrackMediaType == RTSPSERVER_MEDIATYPE_METADATA) ))
				{
					memcpy((void*)&pRTSPServer->pClient[i].NATRTPAddr[j],(void*)psaddr,sizeof(RTSP_SOCKADDR));
					pRTSPServer->pClient[i].iSymRTP --;
					break;
				}			
			}
			return 0;
		}	
	}

	return -1;
}


//ShengFh Symetric RTP 
//2012 modified by Jimmy for metadata
char acProbeACK[][18]={{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                         0xf7,0x86,0x00,0x00,0x2b,0x4f,0x4b,0x20,0x55,0x73},                      
                        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                         0xf3,0xe4,0x00,0x00,0xe3,0x14,0x00,0x00,0x00,0x50},
                         {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xf8,0x33,
                         0x00,0x00,0xe3,0x14,0x00,0x00,0x77,0x88}};

int RTSPServer_HandleProbeRTPPacket(RTSP_CLIENT *pClient)
{
    fd_set       ReadSet;
//  	struct sockaddr_in	cliaddr;    
    struct timeval timeout;
    int iReady,i,iSocket,iLen,iNATAddrLen;
    char acRecvBuffer[70];
    
    FD_ZERO(&ReadSet);

	//20120723 modified by Jimmy for metadata
	/*
    for(i=0;i<2;i++)
    {
        if( pClient->rtp_sock[i][0] >= 0 )
            FD_SET(pClient->rtp_sock[i][0], &ReadSet);            
    }   
     
    if( pClient->rtp_sock[0][0] > pClient->rtp_sock[1][0])         
 		 iSocket =  pClient->rtp_sock[0][0];
	else                   
        iSocket =  pClient->rtp_sock[1][0];
	*/
    
	if( pClient->rtp_sock[0][0] < 0 )
	{
		printf("[%s] pClient->rtp_sock[0][0] < 0\n", __FUNCTION__);
		return 0;
	}

 	//20120723 modified by Jimmy for metadata
	iSocket = -1;
	for(i=0;i<MEDIA_TYPE_NUMBER;i++)
	{
		if( pClient->rtp_sock[i][0] >= 0 )
		{
			FD_SET(pClient->rtp_sock[i][0], &ReadSet);
			if(pClient->rtp_sock[i][0] > iSocket)
				iSocket = pClient->rtp_sock[i][0];
		}
	}

	
	// Moved by cchuang, 2005/07/25
    timeout.tv_sec = 0;
	//20120523 Modify by danny For IPv6 UDP connect fail with errno 22
    timeout.tv_usec = 10000;//100;//0;
   
    iReady = select(iSocket+1, &ReadSet, NULL, NULL,&timeout);
    printf("select socket %d\n",iSocket);
    if( iReady >  0 )
    {
        //20120723 modified by Jimmy for metadata
        for(i=0;i<MEDIA_TYPE_NUMBER;i++)
        {
			if (pClient->rtp_sock[i][0] < 0)
				continue;

            if (FD_ISSET(pClient->rtp_sock[i][0], &ReadSet) )    
            {
                iLen = 0;
	            memset(acRecvBuffer,0,70);

			    iNATAddrLen = sizeof(pClient->NATRTPAddr[i]);

    			iLen = recvfrom(pClient->rtp_sock[i][0], acRecvBuffer,sizeof(acRecvBuffer)
				                 , RECV_FLAGS, (struct sockaddr*)&pClient->NATRTPAddr[i], (unsigned int *)&iNATAddrLen );				                				                 
	    		if( iLen > 0 ) 
		    	{
		    	    TelnetShell_DbgPrint("Got probe packet !!\r\n");    
#ifndef _INET6
#ifdef _WIN32
					printf("\nRecv from %s\n", inet_ntoa(pClient->NATRTPAddr[i].sin_addr));    			
#endif
#endif      
                    if(  pClient->iPlayerType != REAL_MEDIA_PLAYER )
                    {
       		            iLen = sendto(pClient->rtp_sock[i][0],acProbeACK[i],18, SEND_FLAGS,
			            (struct sockaddr*)&pClient->NATRTPAddr[i], iNATAddrLen);
			            if( iLen != 18 )
			            {
			                TelnetShell_DbgPrint("ACK probe packet failed %d\r\n",iLen);
			            }    
			            else
			            {
						    pClient->iSymRTP --;
			                TelnetShell_DbgPrint("ACK probe packet OK\r\n");    
			            }    
			        }    
			        else
                    {                           
                        //iLen = sendto(pClient->rtp_sock[i][0],acRecvBuffer,iLen, 0,
			            //(struct sockaddr*)&pClient->NATAddr[i],sizeof(struct sockaddr));

                        //RTSPServer_ProbeRTPPacketParse(acRecvBuffer,pClient,i);
                        pClient->iSymRTP --;
                    }                    
    		    }  
    		}    
        }
    } 
    else if( iReady ==  0 )
    {
        TelnetShell_DbgPrint("no probe packet...\r\n");
    }
	else
	{
#ifdef _WIN32_
		printf("select error %d\n",WSAGetLastError());
#endif
	}
	
    return 0;             
}    
//ShengFu Symetric RTP 

void RTSPServer_HandleSymmetricRTPbyFixedUDPSocket(RTSP_SERVER *pRTSPServer,fd_set *pReadSet)
{
	//20120723 modified by Jimmy for metadata
	int i,iSymmSocket[MEDIA_TYPE_NUMBER],iRecvSize,iLen;
    RTSP_SOCKADDR saddr;
	DWORD dwSessionID;


	if( pRTSPServer == NULL )
		return ;

	iSymmSocket[0] = pRTSPServer->rtsp_param.iUDPRTPVSock;
	iSymmSocket[1] = pRTSPServer->rtsp_param.iUDPRTPASock;
	//20120723 added by Jimmy for metadata
#ifdef _METADATA_ENABLE
	iSymmSocket[2] = pRTSPServer->rtsp_param.iUDPRTPMSock;
#endif

	//20120723 modified by Jimmy for metadata
	for( i=0; i<MEDIA_TYPE_NUMBER ;i++)
	{
		if( FD_ISSET( iSymmSocket[i],pReadSet) )
		{
			memset((void*)&saddr,0, sizeof(saddr));
			iLen = sizeof(saddr);
			iRecvSize = recvfrom(iSymmSocket[i],g_ptMediaBuffer->pbBufferStart,g_ptMediaBuffer->dwDataBufferSize,RECV_FLAGS,(struct sockaddr *)&saddr,(unsigned int *)&iLen);
			
			if( iRecvSize == 4 )
			{
				TelnetShell_DbgPrint("[%s] iRecvSize == 4\r\n", __FUNCTION__);
				memcpy((void*)&dwSessionID,(void*)g_ptMediaBuffer->pbBufferStart,sizeof(DWORD));

				if( RTSPServer_MatchSessionIDandSockAddr(pRTSPServer,dwSessionID,&saddr,i) == 0 )
				{
					if( (iLen = sendto(iSymmSocket[i],g_ptMediaBuffer->pbBufferStart,4,SEND_FLAGS,(struct sockaddr *)&saddr,sizeof(saddr))) == 4 )
					{
					TelnetShell_DbgPrint("Got Symmetric VVTK RTP pakcets %lu\r\n",(long)dwSessionID);						
					}
				}
				else
				{
					TelnetShell_DbgPrint("[%s] RTSPServer_MatchSessionIDandSockAddr() != 0\r\n", __FUNCTION__);
				}
			}
			else if( iRecvSize > 0 && iRecvSize != 4)
			{
				BOOL bIsUnknownPacket = TRUE;
#ifdef	_SIP_TWO_WAY_AUDIO
				DWORD dwSSRC = 0;

				g_ptMediaBuffer->dwBytesUsed = iRecvSize;
				
				if (pRTSPServer->ulSessionIDofUpStreamAudio == 0)
				{
					// No Upload Stream
				}
				else if (RTPRTCPComposer_RTPHeaderParse(g_ptMediaBuffer, &dwSSRC) == S_OK)
				{
#ifdef _DEBUG
					if (((RTPHEADERINFO*)(g_ptMediaBuffer->pbHeaderInfoStart))->usSeqNumber%1000 == 0)
					{
						printf("Size=%d SSRC=%lu Seq=%d TS=%lu\r\n",g_ptMediaBuffer->dwBytesUsed,dwSSRC,((RTPHEADERINFO*)(g_ptMediaBuffer->pbHeaderInfoStart))->usSeqNumber,((RTPHEADERINFO*)(g_ptMediaBuffer->pbHeaderInfoStart))->ulTimeStamp);
					}
#endif
					
					if (pRTSPServer->fcallback != NULL )
					{
#ifdef _SIP_VVTKONLY
						if ((dwSSRC == pRTSPServer->ulSSRCofUpStreamAudio) && (pRTSPServer->ulSSRCofUpStreamAudio != 0))
#else
						if ((pRTSPServer->ulSSRCofUpStreamAudio == SIP_2WAYAUDIO_OTHERVENDOR_SSRC) || 
							((dwSSRC == pRTSPServer->ulSSRCofUpStreamAudio) && (pRTSPServer->ulSSRCofUpStreamAudio != 0)))
#endif //_SIP_VVTKONLY
						{						
							pRTSPServer->fcallback(pRTSPServer->hParentHandle
								,RTSPSERVER_CALLBACKFLAG_UPLOAD_AUDIODATA
								,(void*)g_ptMediaBuffer,0);
							RTSPServer_SetLastRecvTimeForUpStreamAudioNow(pRTSPServer);
							bIsUnknownPacket = FALSE;
						}
#ifdef _DEBUG
						else
						{
							//Recv RTP with mis-matched SSRC
							printf("SSRCofUpStreamAudio=%lu RecvSSRC=%lu\n", pRTSPServer->ulSSRCofUpStreamAudio, dwSSRC);
						}
#endif
					}
				}
#endif //_SIP_TWO_WAY_AUDIO
				
				if (bIsUnknownPacket && i == 0)
				{
#ifdef _INET6
                    //printf("unknown data received from fixed Video Socket\r\n");
#else
					//printf("unknown data received from fixed Video Socket from %s:%d\r\n", inet_ntoa(saddr.sin_addr), ntohs(saddr.sin_port));
#endif
				}
				else if (bIsUnknownPacket)
				{
#ifdef _INET6
					//printf("unknown data received from fixed Audio Socket\r\n");
#else
					//printf("unknown data received from fixed Audio Socket from %s:%d\r\n", inet_ntoa(saddr.sin_addr), ntohs(saddr.sin_port));
#endif
				}
				break;
			}
			else
			{
				TelnetShell_DbgPrint("Symmetric Socket recv failed\r\n");
#ifdef _WIN32_
				printf("%d",WSAGetLastError());
#endif
				
			}			
		}
	}

	return;

}


void RTSPServer_HandleAcceptSocket(RTSP_SERVER *pRTSPServer,int iListenFD, fd_set *pReadSet)
{
#ifdef _INET6
	struct sockaddr_in	v4mapaddr;
	BOOL bComeFromIPv6 = FALSE;
#endif
	RTSP_SOCKADDR cliaddr;

    int				iClientLen;
	SOCKET			iConnFD;
    unsigned long	ulNonBlock;
	int				i,iVal;

	memset(&cliaddr,0, sizeof(cliaddr));
		
	if (FD_ISSET(iListenFD, pReadSet))/* new client connection */
	{	                          
		printf("Someone Connected\n");
        iClientLen = sizeof(cliaddr);
        iConnFD = accept(iListenFD, (struct sockaddr *) &cliaddr, (unsigned int *)&iClientLen);
		//CID:186, CHECKER:NEGATIVE_RETURNS
		if(iConnFD < 0)
		{
			printf("New client accept error %d\n",errno);
			return;
		}
#ifdef _LINUX
#ifdef _INET6
		memset(&v4mapaddr, 0, sizeof(v4mapaddr));

		if (IN6_IS_ADDR_V4MAPPED(&cliaddr.sin6_addr))
		{
			struct in6_addr in6addr = cliaddr.sin6_addr;

			printf( "Someone connecting from IPv4 %d.%d.%d.%d\n",
				in6addr.s6_addr[12],
				in6addr.s6_addr[13],
				in6addr.s6_addr[14],
				in6addr.s6_addr[15]);
			v4mapaddr.sin_addr.s_addr =  MAKEFOURCC(in6addr.s6_addr[12], in6addr.s6_addr[13], in6addr.s6_addr[14], in6addr.s6_addr[15]);
			printf("Accpet IP = %s Socket = %d\n",(char*)inet_ntoa(v4mapaddr.sin_addr),iConnFD);
		}
		else
		{
			char szPresentString[64]="";
			printf( "Someone connecting from %s\n", 
				inet_ntop(AF_INET6, &cliaddr.sin6_addr, szPresentString, sizeof(szPresentString)));

			bComeFromIPv6 = TRUE;
		}
#endif
#endif

		ulNonBlock=1;
		ioctlsocket(iConnFD,FIONBIO,&ulNonBlock);
		iVal=1;
		setsockopt(iConnFD,IPPROTO_TCP,TCP_NODELAY,(char *)&iVal,sizeof(int) );                

        if( (pRTSPServer->iCurrentSessionNumber >= pRTSPServer->iMaxSessionNumber)
#ifdef _INET6
			|| ((bComeFromIPv6 == FALSE)&&(RTSPServer_Authentication(pRTSPServer,v4mapaddr) != 0 ))
#else
            || (RTSPServer_Authentication(pRTSPServer,cliaddr) != 0 )
#endif
			)
        {
#ifdef _LINUX
            //syslog(LOG_ERR,"Reject IP!!\n");
#endif
			TelnetShell_DbgPrint("[RTSP server]: Reject IP!!\n");
            closesocket(iConnFD);			
        }   
        else	
        {
#ifdef _LINUX
#ifndef _INET6
			printf("Accpet IP = %s Socket = %d\n",(char*)inet_ntoa(cliaddr.sin_addr),iConnFD);
#endif
			//syslog(LOG_ERR, "Accpet IP = %s Socket = %d\n",(char*)inet_ntoa(cliaddr.sin_addr),iConnFD);
#endif
                            
            for (i = 0; i < pRTSPServer->iMaxSessionNumber; i++)
            {
                if (( pRTSPServer->pClient[i].iRecvSockfd < 0 ) && (pRTSPServer->pClient[i].iSendSockfd < 0 ) )
                {
                    pRTSPServer->pClient[i].iRecvSockfd = iConnFD;	/* save descriptor */
                    pRTSPServer->pClient[i].iSendSockfd = iConnFD;	/* save descriptor */
					/* 20090225 QOS */
					RTSPServer_SetQosToSocket(pRTSPServer->pClient[i].iSendSockfd, &pRTSPServer->tQosInfo, eQosMuxedType);
                    OSTick_GetMSec(&pRTSPServer->pClient[i].dwBaseMSec);
                    iVal=1;
                    setsockopt(pRTSPServer->pClient[i].iRecvSockfd,IPPROTO_TCP,TCP_NODELAY,(char *)&iVal,sizeof(int) );                
                    
                    ulNonBlock=1;
                    ioctlsocket(pRTSPServer->pClient[i].iRecvSockfd,FIONBIO,&ulNonBlock);
#ifdef _INET6
					pRTSPServer->pClient[i].ulClientAddress = v4mapaddr.sin_addr.s_addr;
					memcpy( &pRTSPServer->pClient[i].tClientSckAddr, &cliaddr, sizeof(cliaddr));
					//20080620 for recording client port
					pRTSPServer->pClient[i].wClientPort = ntohs(cliaddr.sin6_port);
#else
                    pRTSPServer->pClient[i].ulClientAddress = cliaddr.sin_addr.s_addr;
					//20080620 for recording client port
					pRTSPServer->pClient[i].wClientPort = ntohs(cliaddr.sin_port);
#endif
					//20160603 add by faber, specific client index
					pRTSPServer->pClient[i].iClientIndex = i;

					pRTSPServer->iCurrentSessionNumber ++;
					printf("!!!!RTSP session added %d!!!!\n",pRTSPServer->iCurrentSessionNumber);
					printf("clinet index = %d\n", pRTSPServer->pClient[i].iClientIndex);
#ifndef _INET6
                    TelnetShell_DbgPrint("Accpet IP= %s Socket= %d, Current clients= %d\r\n",(char*)inet_ntoa(cliaddr.sin_addr),iConnFD,pRTSPServer->iCurrentSessionNumber);
#endif
                    break;
                }
            }
            
            if( i == pRTSPServer->iMaxSessionNumber )
            {
#ifdef _LINUX
                //syslog(LOG_ERR,"Panic! not exceed max connection but no more room\r\n");
#endif
                TelnetShell_DbgPrint("Panic! not exceed max connection but no more room\r\n");
                closesocket(iConnFD);
            }

            if( pRTSPServer->iCurrentSessionNumber == pRTSPServer->iMaxSessionNumber )
            {
#ifdef _LINUX
                //syslog(LOG_ERR,"too many clients!!\n");
#endif
                pRTSPServer->pClient[i].iTimeOut = -10;
            }
        }
    }
}

void RTSPServer_HandleActiveClient(RTSP_SERVER *pRTSPServer)
{
	int i;

	for(i = 0; i < pRTSPServer->iMaxSessionNumber; i++)      
	{	
		if( pRTSPServer->pClient[i].iStatus != PLAY_STATE )
		{
			if( ( pRTSPServer->pClient[i].iRecvSockfd >= 0 ) ||
				(pRTSPServer->pClient[i].iRTPStreamingMode == RTP_OVER_HTTP)  )
		    {
				RTSPServer_CheckIdleClient(&pRTSPServer->pClient[i],pRTSPServer);      
			}
		}
		RTSPServer_MessageHandler(&pRTSPServer->pClient[i],pRTSPServer);
	}
}

//20120925 added by Jimmy for ONVIF backchannel
void RTSPServer_HandleAudioback(RTSP_SERVER *pRTSPServer,fd_set *pReadSet)
{
	int i,iRecvSize,iLen;
    RTSP_SOCKADDR saddr;
	DWORD dwSSRC;

	for( i = 0; i < MULTIPLE_CHANNEL_NUM; i++ )
	{
		if( pRTSPServer->iAudiobackSock[i][0] < 0)
			continue;

		if( FD_ISSET(pRTSPServer->iAudiobackSock[i][0], pReadSet) )
		{
			memset((void*)&saddr,0, sizeof(saddr));
			iLen = sizeof(saddr);
			iRecvSize = recvfrom(pRTSPServer->iAudiobackSock[i][0],g_ptMediaBuffer->pbBufferStart,g_ptMediaBuffer->dwDataBufferSize,RECV_FLAGS,(struct sockaddr *)&saddr,(unsigned int *)&iLen);

			if( iRecvSize < 0)
			{
				printf("[%s]Error: %s\n", __FUNCTION__, strerror(errno));
			}
			else if( iRecvSize != 0 )
			{
				//printf("[%s] Received %d bytes\n", __FUNCTION__, iRecvSize);

				dwSSRC = 0;
				g_ptMediaBuffer->dwBytesUsed = iRecvSize;

				if(RTPRTCPComposer_RTPHeaderParse(g_ptMediaBuffer, &dwSSRC) == S_OK)
				{
					OSTime_GetTimer(&pRTSPServer->dwLastRecvTimeofAudioback[i], NULL);
					//printf("[%s] g_ptMediaBuffer->dwBytesUsed = %d\n", __FUNCTION__, g_ptMediaBuffer->dwBytesUsed);

					pRTSPServer->fcallback(pRTSPServer->hParentHandle
										,RTSPSERVER_CALLBACKFLAG_UPDATE_AUDIODATA_SDPINFO
										,(void*)(i + 1),0);

					pRTSPServer->fcallback(pRTSPServer->hParentHandle
										,RTSPSERVER_CALLBACKFLAG_UPLOAD_AUDIODATA
										,(void*)g_ptMediaBuffer,0);
				}
			}
		}
	}
}

SCODE RTSPServer_CheckSocketStatus(int	iSocket)
{
	int					iResult = 0;
	fd_set				fdsRead;
	struct timeval		timeout;
	char				acReadBuffer[100];
	
	//Make sure the socket is valid
	if(iSocket == INVALID_SOCKET)
	{
		return S_FAIL;
	}

	//Prepare for select
	FD_ZERO(&fdsRead);
	timeout.tv_sec = 0;
	timeout.tv_usec = 1000;
	FD_SET(iSocket, &fdsRead);

	iResult = select(iSocket + 1, &fdsRead, NULL, NULL, &timeout);

	if(iResult > 0)
	{
		if(read(iSocket, acReadBuffer, sizeof(acReadBuffer)) <= 0)
		{
			//Socket is closed by peer
			printf("Socket %d closed by peer!\n", iSocket);
			return S_FAIL;
		}
	}
	else if(iResult == 0)
	{
		//It is likely there is nothing to read in, so the socket is normal
		return S_OK;
	}
	else if(iResult < 0)
	{
		//The socket is corrupt
		printf("Socket %d abnormal!\n", iSocket);
		return S_FAIL;
	}

	return S_OK;
}

void RTSPServer_HandleRTSPSignaling(RTSP_SERVER *pRTSPServer, fd_set	*pReadSet)
{
	int	i,iSockFD,iRecvSize;

    for(i = 0; i < pRTSPServer->iMaxSessionNumber; i++)      
    {		                            
        if( (iSockFD = pRTSPServer->pClient[i].iRecvSockfd) < 0)
        {
			continue; 
        }				

		if (FD_ISSET(iSockFD, pReadSet))
		{
			iRecvSize = recv(iSockFD,pRTSPServer->pClient[i].acRecvBuffer + pRTSPServer->pClient[i].iRecvSize
				            ,MAX_RECVBUFF-pRTSPServer->pClient[i].iRecvSize,RECV_FLAGS) ;
		                
            if( ( iRecvSize > MAX_RECVBUFF)|| (iRecvSize <= 0 ) )
			{
				printf("RTSP socket %d error, code: %d!\n",iSockFD, errno);
				//20130315 added by Jimmy to log more information
				syslog(LOG_DEBUG, "RTSP socket %d error, code: %d!\n",iSockFD, errno);
#ifdef _LINUX
				//syslog(LOG_ERR,"RTSP socket error %d!\r\n",iSockFD);
#endif //_LINUX
#ifdef _PSOS_TRIMEDIA        
				LED2_ON();			
#endif       

				if( pRTSPServer->pClient[i].iRTPStreamingMode != RTP_OVER_HTTP  )
				{
					if( pRTSPServer->pClient[i].ulSessionID != 0 )
						RTSPServer_SessionStop(pRTSPServer,pRTSPServer->pClient[i].ulSessionID,pRTSPServer->pClient[i].iMulticast);

					RTSPServer_InitClient(&pRTSPServer->pClient[i]);
				}
				else if( pRTSPServer->pClient[i].iRTPStreamingMode == RTP_OVER_HTTP )
				{
					// QT player in HTTP mode will disconnect TCP and then send POST with OPTION
					closesocket(iSockFD);
					pRTSPServer->pClient[i].iRecvSockfd = -1;
					//20081008 also check GET socket to ensure connections are not closed
					if(RTSPServer_CheckSocketStatus(pRTSPServer->pClient[i].iSendSockfd) != S_OK)
					{
						TelnetShell_DbgPrint("HTTP socket pair closed, connection should close\r\n");
						RTSPServer_SessionStop(pRTSPServer,pRTSPServer->pClient[i].ulSessionID,pRTSPServer->pClient[i].iMulticast);
						RTSPServer_InitClient(&pRTSPServer->pClient[i]);				
					}
					else
					{
						TelnetShell_DbgPrint("----QT POST socket %d closed----\r\n",iSockFD);
					}
				}
			}
			else
			{	
				pRTSPServer->pClient[i].iRecvSize = pRTSPServer->pClient[i].iRecvSize + iRecvSize;			
				pRTSPServer->pClient[i].acRecvBuffer[pRTSPServer->pClient[i].iRecvSize]=0;
   			}	
        }

        //RTSPServer_MessageHandler(&pRTSPServer->pClient[i],pRTSPServer);

	} // end of for loop of all sessions

}

void RTSPServer_HandleInvalidSocket(RTSP_SERVER *pRTSPServer)
{
	int i;
	int	iValue,iValueLen;

	for(i = 0; i < pRTSPServer->iMaxSessionNumber; i++)      
	{	
		if( pRTSPServer->pClient[i].iRecvSockfd >= 0 )
		{
			//CID:1177, CHECKER:UNINIT
			iValueLen = sizeof(iValue);
			if( getsockopt(pRTSPServer->pClient[i].iRecvSockfd,SOL_SOCKET,SO_RCVBUF,&iValue,(unsigned int *)&iValueLen) != 0)
			{
				RTSPServer_InitClient(&pRTSPServer->pClient[i]);   
			}
		}
	}
}

int RTSPServer_SelectAddSockets(RTSP_SERVER *pRTSPServer,int iListenFD,	fd_set	*pReadSet)
{
	int		iMaxfd,i;
    struct timeval timeout;
	
    FD_ZERO(pReadSet);
	FD_SET(iListenFD, pReadSet);
	//20130403 modified by Jimmy to fix high CPU usage issue when the metadata build option is not enbaled
	if(pRTSPServer->rtsp_param.iUDPRTPVSock >= 0)
		FD_SET(pRTSPServer->rtsp_param.iUDPRTPVSock, pReadSet);
	if(pRTSPServer->rtsp_param.iUDPRTPASock >= 0)
		FD_SET(pRTSPServer->rtsp_param.iUDPRTPASock, pReadSet);
	//20120723 added by Jimmy for metadata
	if(pRTSPServer->rtsp_param.iUDPRTPMSock >= 0)
		FD_SET(pRTSPServer->rtsp_param.iUDPRTPMSock, pReadSet);

	iMaxfd = iListenFD;

	if( pRTSPServer->rtsp_param.iUDPRTPVSock > iMaxfd)
		iMaxfd = pRTSPServer->rtsp_param.iUDPRTPVSock;

	if( pRTSPServer->rtsp_param.iUDPRTPASock > iMaxfd)
		iMaxfd = pRTSPServer->rtsp_param.iUDPRTPASock;

	//20120723 added by Jimmy for metadata
	if( pRTSPServer->rtsp_param.iUDPRTPMSock > iMaxfd)
		iMaxfd = pRTSPServer->rtsp_param.iUDPRTPMSock;

	//20120925 added by Jimmy for ONVIF backchannel
	for( i = 0; i < MULTIPLE_CHANNEL_NUM; i++ )
	{
		if( (pRTSPServer->dwLastRecvTimeofAudioback[i] != 0) && (pRTSPServer->iAudiobackSock[i][0] >= 0) )
		{
			FD_SET(pRTSPServer->iAudiobackSock[i][0], pReadSet);
            if( pRTSPServer->iAudiobackSock[i][0] > iMaxfd )
            {
				iMaxfd = pRTSPServer->iAudiobackSock[i][0];
            }
		}
	}

    for( i=0 ; i< pRTSPServer->iMaxSessionNumber ; i++ )
    {
		if( pRTSPServer->pClient[i].iRecvSockfd >= 0 )
        {			
			FD_SET(pRTSPServer->pClient[i].iRecvSockfd, pReadSet);
            if(  pRTSPServer->pClient[i].iRecvSockfd > iMaxfd)
            {
				iMaxfd = pRTSPServer->pClient[i].iRecvSockfd ;
            }      

			//ShengFu Symetric RTP                             
            if( pRTSPServer->pClient[i].iSymRTP > 0 )
            {
				if( pRTSPServer->pClient[i].iStatus != PLAY_STATE)
				{
					printf("Symetric RTP !!!!\n");
					RTSPServer_HandleProbeRTPPacket(&(pRTSPServer->pClient[i]));                
				}		
             }                
             //ShengFu Symetric RTP
        } 
	}

	timeout.tv_sec = 0;
	timeout.tv_usec = 100*1000;

    return select(iMaxfd+1, pReadSet, NULL, NULL,&timeout);

}

DWORD THREADAPI RTSPServer_MainServerLoop(DWORD hRTSPServerHandle)
{
    int		i,j;
    SOCKET  iListenFD;
    int		iReady;
	
    fd_set       ReadSet;
    RTSP_SERVER *pRTSPServer;
    unsigned long ulNonBlock;
    int iVal,iCount;
		
#ifdef _INET6
    struct in6_addr in6any_addr = IN6ADDR_ANY_INIT;
#endif
	RTSP_SOCKADDR servaddr;
		
#ifdef _LINUX
	//syslog(LOG_ERR,"[RTSPServer] pid is %d\n", getpid());
	printf("[RTSPServer] pid is %d\n", getpid());
#endif //_LINUX

    pRTSPServer = (RTSP_SERVER*)hRTSPServerHandle;

#ifdef _INET6
	iListenFD = socket(AF_INET6, SOCK_STREAM, 0);
#else
    iListenFD = socket(AF_INET, SOCK_STREAM, 0);
#endif
	//CID:190, CHECKER: NEGATIVE_RETURNS
	if(iListenFD < 0)
	{
		syslog(LOG_ALERT,"open socket in rtspserver failed\n");
		return -1;
	}
	/* Reuse the socket */
#ifdef _LINUX
	iVal = 1;
	setsockopt(iListenFD, SOL_SOCKET, SO_REUSEADDR, &iVal, sizeof(iVal));  
#endif // _LINUX

#ifdef _INET6
	memset(&servaddr,0, sizeof(struct sockaddr_in6));
    servaddr.sin6_family = AF_INET6;
	servaddr.sin6_addr = in6any_addr;
    //memcpy(&server_sockaddr.sin6_addr, &in6any_addr, sizeof (in6any_addr));
    servaddr.sin6_port = htons(pRTSPServer->rtsp_param.rtsp_port);
    fprintf(stderr, "[IPv6] socket created!\n");
#else
    memset(&servaddr,0, sizeof(struct sockaddr_in));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port        = htons(pRTSPServer->rtsp_param.rtsp_port);
#endif

	printf("RTSP port %d\n",pRTSPServer->rtsp_param.rtsp_port);
	
	iCount = 0;
	while(1)
	{
		iCount++; 
		
    	if (bind(iListenFD, (struct sockaddr *) &servaddr, 
				sizeof(servaddr)) != 0)
		{
			OSSleep_MSec(100);
		}
		else
			break;
		
		if( iCount > 30 )
		{
#ifdef _LINUX
			syslog(LOG_ALERT,"bind rtsp port %d failed\n",pRTSPServer->rtsp_param.rtsp_port);
#endif			
			return -1;
		}
	}
	
    ulNonBlock=1;
    iVal = ioctlsocket(iListenFD,FIONBIO,&ulNonBlock);
	
    iVal = listen(iListenFD, LISTENQ);

	
    for(i=0;i<pRTSPServer->iMaxSessionNumber;i++)
    {
        pRTSPServer->pClient[i].parent = pRTSPServer;
        pRTSPServer->pClient[i].iStatus = INIT_STATE;
        pRTSPServer->pClient[i].iRecvSockfd	=	-1;
        pRTSPServer->pClient[i].iSendSockfd	=	-1;

        //20120723 modified by Jimmy for metadata
        for(j=0;j<MEDIA_TYPE_NUMBER;j++)
        {
            pRTSPServer->pClient[i].rtp_sock[j][0] = -1;
            pRTSPServer->pClient[i].rtp_sock[j][1] = -1;            
        }
        /*
        pRTSPServer->pClient[i].rtp_sock[0][0] = -1;
        pRTSPServer->pClient[i].rtp_sock[0][1] = -1;
        pRTSPServer->pClient[i].rtp_sock[1][0] = -1;
        pRTSPServer->pClient[i].rtp_sock[1][1] = -1;
        */

        RTSPServer_InitClient(&pRTSPServer->pClient[i]);
    }	
    
    pRTSPServer->iCurrentSessionNumber = 0;
 
	TelnetShell_DbgPrint("[RTSP server]: RTSP Server Start!!\r\n");

    for( ; ; )
    {
		if (pRTSPServer->iTerminateThread == 1)
			break;

		iReady = RTSPServer_SelectAddSockets(pRTSPServer,iListenFD,&ReadSet);
		if( iReady > 0 )
		{
			RTSPServer_HandleAcceptSocket(pRTSPServer,iListenFD,&ReadSet);
			RTSPServer_HandleSymmetricRTPbyFixedUDPSocket(pRTSPServer,&ReadSet);
			//20120925 added by Jimmy for ONVIF backchannel
			RTSPServer_HandleAudioback(pRTSPServer,&ReadSet);
			RTSPServer_HandleRTSPSignaling(pRTSPServer,&ReadSet);
		}
		else if( iReady < 0)
		{
#ifdef _LINUX
			//20080926 quick exit
			if (pRTSPServer->iTerminateThread == 1)
			{
				break;
			}

			printf("!!!![RTSPServer]: Select Sockets error %s!!!!!!\r\n",strerror(errno));
			//20130315 added by Jimmy to log more information
			syslog(LOG_DEBUG, "!!!![RTSPServer]: Select Sockets error %s!!!!!!\r\n",strerror(errno));
#endif
			RTSPServer_HandleInvalidSocket(pRTSPServer);
		}

#ifdef	_SIP_TWO_WAY_AUDIO
		RTSPServer_CheckTimeoutForUpStreamAudio(pRTSPServer);
#endif

		//20120925 added by Jimmy for ONVIF backchannel
		RTSPServer_CheckTimeoutForAudioback(pRTSPServer);

		RTSPServer_HandleActiveClient(pRTSPServer);

		RTSPServer_CheckDeleteQueue(pRTSPServer);
        RTSPServer_CheckHttpSockQueue(pRTSPServer);
		RTSPServer_CheckTeardownOKQueue(pRTSPServer);
#ifdef _SHARED_MEM
		//20100105 Added For Seamless Recording
		RTSPServer_CheckResetGUIDQueue(pRTSPServer);
#endif
		//20110401 Added by danny For support RTSPServer thread watchdog
		RTSPServer_KickWatchDog(pRTSPServer);

    }// end of main RTSP server while loop

	pRTSPServer->iRunning = 0;

    return 0;
}

int RTSPServer_Start(HANDLE hRTSPServerHandle)
{
    RTSP_SERVER* pRTSPServer;

	if( hRTSPServerHandle == NULL )
		return -1;

    pRTSPServer = (RTSP_SERVER*)hRTSPServerHandle;
	pRTSPServer->iTerminateThread = 0;
	pRTSPServer->iRunning = 1;
	
    OSThread_Start((HANDLE)pRTSPServer->ulTaskID);
  
    return 0;
}

int RTSPServer_Stop(HANDLE hRTSPServerHandle)
{
    RTSP_SERVER* 	pRTSPServer;
    int				iCount=0;

	if( hRTSPServerHandle == NULL )
		return -1;

    pRTSPServer = (RTSP_SERVER*)hRTSPServerHandle;
	pRTSPServer->iTerminateThread = 1;
	while (1)
	{
		iCount++;
		if( iCount > 20 )
			return -1;
			
		if (pRTSPServer->iRunning == 0)
			break;
			
       	OSSleep_MSec(20);       	
	}

	return 0;
}


int RTSPServer_SetParameters(HANDLE hRTSPServerHandle, RTSPSERVER_PARAM *pstVideoEncodingParameter)
{
    RTSP_SERVER *pRTSPServer;

    if( hRTSPServerHandle == NULL )
        return -1;

    pRTSPServer = (RTSP_SERVER*)hRTSPServerHandle;
    memcpy(&pRTSPServer->rtsp_param,pstVideoEncodingParameter,sizeof(RTSPSERVER_PARAM));

    return 0;
}

/*int RTSPServer_SetMediaStreamMode(HANDLE hRTSPServerHandle, DWORD dwMediaStreamMode)
{
    RTSP_SERVER *pRTSPServer;

    if( hRTSPServerHandle == NULL )
        return -1;
        
    pRTSPServer = (RTSP_SERVER*)hRTSPServerHandle;
    pRTSPServer->rtsp_param.StreamingMode = dwMediaStreamMode ;

    return 0;
}*/
int RTSPServer_AddRTPOverHTTPSock(HANDLE hRTSPServerHandle, THTTPCONNINFO *ptHTTPConnInfo,DWORD dwFlag)
{
    RTSP_SERVER *pRTSPServer;
    unsigned long ulMessage[4],ulCounter=0;

    if( hRTSPServerHandle == NULL )
        return -1;
                    
    pRTSPServer = (RTSP_SERVER*)hRTSPServerHandle;
		
	if( OSMsgQueue_Receive(pRTSPServer->hHttpInfoBuffQueue,(DWORD*)ulMessage, 0) != S_OK )
	{
		return -3;
	}

	if( ulMessage[0] != 0 )
	{
		memcpy((void*)ulMessage[0] ,(void*)ptHTTPConnInfo,sizeof(THTTPCONNINFO));
		ulMessage[1] = dwFlag;
	}
	else
		return -4;  

    while(1)
    {
        ulCounter++;
        if( OSMsgQueue_Send((HANDLE)pRTSPServer->hHttpSockQueue,(DWORD*)ulMessage) != S_OK)
        {
  			OSSleep_MSec(100);
        }
        else
            break;
        
        if( ulCounter > 6 )                
            return -2;
    }
    
    return 0;
}    

int RTSPServer_AddTCPMuxHandle(HANDLE hRTSPServerHandle, HANDLE hTCPMuxCS)
{
    RTSP_SERVER *pRTSPServer;
    int i;

    if( hRTSPServerHandle == NULL )
    {
        return -1 ;
    }
    
    pRTSPServer = (RTSP_SERVER*)hRTSPServerHandle;

    for (i = 0; i < pRTSPServer->iMaxSessionNumber; i++)
    {
        if (pRTSPServer->pClient[i].hTCPMuxCS == NULL)
        {
            pRTSPServer->pClient[i].hTCPMuxCS = hTCPMuxCS;
            return 0;            
        }
    }
    
    return -1;
}

#ifdef RTSPRTP_MULTICAST
//20100720 Added by danny to fix Backchannel multicast session terminated, rtsp server has not stopped sending video/audio RTP/RTCP
int RTSPServer_GetMulticastCurrentSessionNumber(HANDLE hRTSPServerHandle, int iMulticastindex)
{
    RTSP_SERVER *pRTSPServer;

    if( hRTSPServerHandle == NULL )
    {
        return -1 ;
    }
    
    pRTSPServer = (RTSP_SERVER*)hRTSPServerHandle;

	return pRTSPServer->iCurrMulticastNumber[iMulticastindex - 1];
}

//20100714 Added by danny For Multicast parameters load dynamically
SCODE RTSPServer_UpdateMulticastParameters(HANDLE hRTSPServerHandle, int iMulticastCount, MULTICASTINFO *ptMulticastInfo)
{    
	RTSP_SERVER *pRTSPServer;
	
    if( hRTSPServerHandle == NULL || ptMulticastInfo == NULL)
    {
        return S_FAIL ;
    }
    pRTSPServer = (RTSP_SERVER*)hRTSPServerHandle;

	//20110630 Add by danny For Multicast enable/disable
	pRTSPServer->iMulticastEnable[iMulticastCount] = ptMulticastInfo->iEnable;
	pRTSPServer->ulMulticastAddress[iMulticastCount] = ptMulticastInfo->ulMulticastAddress;
	//20150127 Add by Faber, for separate audio/metadata addresss
	pRTSPServer->ulMulticastAudioAddress[iMulticastCount] = ptMulticastInfo->ulMulticastAudioAddress;
	pRTSPServer->ulMulticastMetadataAddress[iMulticastCount] = ptMulticastInfo->ulMulticastMetadataAddress;
	pRTSPServer->usMulticastVideoPort[iMulticastCount] = ptMulticastInfo->usMulticastVideoPort;
	pRTSPServer->usMulticastAudioPort[iMulticastCount] = ptMulticastInfo->usMulticastAudioPort;
	//20120726 added by Jimmy for metadata
	pRTSPServer->usMulticastMetadataPort[iMulticastCount] = ptMulticastInfo->usMulticastMetadataPort;
	pRTSPServer->usTTL[iMulticastCount] = ptMulticastInfo->usTTL;
	pRTSPServer->iRTPExtension[iMulticastCount] = ptMulticastInfo->iRTPExtension;
	pRTSPServer->iMulticastSDPIndex[iMulticastCount] = ptMulticastInfo->iSDPIndex;

	/*printf("[%s]\n%d\n%d\n%d\n%lu\n%d\n%d\n", __FUNCTION__,
					pRTSPServer->iMulticastEnable[iMulticastCount],
					pRTSPServer->usMulticastVideoPort[iMulticastCount],
					pRTSPServer->usMulticastAudioPort[iMulticastCount],
					pRTSPServer->ulMulticastAddress[iMulticastCount],
					pRTSPServer->usTTL[iMulticastCount],
					pRTSPServer->iRTPExtension[iMulticastCount]);*/
	
	return S_OK;
}
#endif

int RTSPServer_GetCurrentSessionNumber(HANDLE hRTSPServerHandle)
{
    RTSP_SERVER *pRTSPServer;

    if( hRTSPServerHandle == NULL )
    {
        return -1 ;
    }
    
    pRTSPServer = (RTSP_SERVER*)hRTSPServerHandle;

	return pRTSPServer->iCurrentSessionNumber;
}

int	RTSPServer_SetAuthenticationType(HANDLE hRTSPServerHandle,int iAuthType)
{
    RTSP_SERVER *pRTSPServer;

    if( hRTSPServerHandle == NULL )
    {
        return -1 ;
    }

    pRTSPServer = (RTSP_SERVER*)hRTSPServerHandle;

	pRTSPServer->iAuthenticationType = iAuthType;

	return 0;
}

#ifdef WISE_SPOT_AUTHENTICATE
int RTSPServer_AddWiseSpotAuthenticateIP(HANDLE hRTSPServerHandle, DWORD dwIP)
{
    RTSP_SERVER *pRTSPServer;

    if( hRTSPServerHandle == NULL )
    {
        return -1 ;
    }
    
    pRTSPServer = (RTSP_SERVER*)hRTSPServerHandle;

	pRTSPServer->dwAuthenticateIP = dwIP;
}
#endif

#ifdef	_SIP_TWO_WAY_AUDIO
int RTSPServer_SetSSRCForUpStreamAudio(HANDLE hRTSPServerHandle,unsigned long ulSSRC)
{
    RTSP_SERVER *pRTSPServer;

    if( hRTSPServerHandle == NULL )
    {
        return -1 ;
    }
    
    pRTSPServer = (RTSP_SERVER*)hRTSPServerHandle;

	pRTSPServer->ulSSRCofUpStreamAudio = ulSSRC;
	printf("[RTSPServer] Set SSRC %d %d\r\n", (DWORD) ulSSRC, (DWORD) pRTSPServer->ulSSRCofUpStreamAudio);
	return 0;
}

int RTSPServer_SetSessionIDForUpStreamAudio(HANDLE hRTSPServerHandle,unsigned long ulSessionID)
{
    RTSP_SERVER *pRTSPServer;

    if( hRTSPServerHandle == NULL )
    {
        return -1 ;
    }
    
    pRTSPServer = (RTSP_SERVER*)hRTSPServerHandle;

	pRTSPServer->ulSessionIDofUpStreamAudio = ulSessionID;
	return 0;
}

int RTSPServer_SetLastRecvTimeForUpStreamAudio(HANDLE hRTSPServerHandle, DWORD dwLastRecvSec)
{
	RTSP_SERVER *pRTSPServer;

    if( hRTSPServerHandle == NULL )
    {
        return -1 ;
    }
    
    pRTSPServer = (RTSP_SERVER*)hRTSPServerHandle;

	pRTSPServer->dwLastRecvTimeofUpStreamAudio = dwLastRecvSec;
	return 0;
}

int RTSPServer_SetLastRecvTimeForUpStreamAudioNow(HANDLE hRTSPServerHandle)
{
	RTSP_SERVER *pRTSPServer;

    if( hRTSPServerHandle == NULL )
    {
        return -1 ;
    }
    
    pRTSPServer = (RTSP_SERVER*)hRTSPServerHandle;
	OSTime_GetTimer(&pRTSPServer->dwLastRecvTimeofUpStreamAudio, NULL);
	return 0;
}

int RTSPServer_CheckTimeoutForUpStreamAudio(HANDLE hRTSPServerHandle)
{
	DWORD dwSec;
	RTSP_SERVER *pRTSPServer;

    if( hRTSPServerHandle == NULL )
    {
        return -1 ;
    }
	
    pRTSPServer = (RTSP_SERVER*)hRTSPServerHandle;

	if (pRTSPServer->ulSessionIDofUpStreamAudio != 0)
	{
		OSTime_GetTimer(&dwSec, NULL);
		if ((dwSec - pRTSPServer->dwLastRecvTimeofUpStreamAudio) > 10)
		{
			pRTSPServer->dwSuccessiveTimeout++;
		}
		else
		{
			pRTSPServer->dwSuccessiveTimeout = 0;
		}

		//Successive three times to match timeout condition avoid systemtime rollback misjudge!
		if (pRTSPServer->dwSuccessiveTimeout > 2)
		{
			pRTSPServer->fcallback(pRTSPServer->hParentHandle
				,RTSPSERVER_CALLBACKFLAG_UPLOAD_AUDIODATA_TIMEOUT
				,(void*)pRTSPServer->ulSessionIDofUpStreamAudio,0);
			pRTSPServer->ulSessionIDofUpStreamAudio = 0;
			pRTSPServer->ulSSRCofUpStreamAudio = 0;
			pRTSPServer->dwLastRecvTimeofUpStreamAudio = 0;
			pRTSPServer->dwSuccessiveTimeout = 0;
		}
	}

	return 0;
}

//20120925 added by Jimmy for ONVIF backchannel
int RTSPServer_CheckTimeoutForAudioback(HANDLE hRTSPServerHandle)
{
	int i,j;
	DWORD dwSec;
	RTSP_SERVER *pRTSPServer;

    if( hRTSPServerHandle == NULL )
    {
        return -1 ;
    }
	
    pRTSPServer = (RTSP_SERVER*)hRTSPServerHandle;

	for( i = 0; i < MULTIPLE_CHANNEL_NUM; i++ )
	{
		if(pRTSPServer->dwLastRecvTimeofAudioback[i] != 0)
		{
			OSTime_GetTimer(&dwSec, NULL);
			if( (dwSec - pRTSPServer->dwLastRecvTimeofAudioback[i]) > RTSPSERVER_AUDIOBACK_TIMEOUT)
			{
				for( j = 0; j < pRTSPServer->iMaxSessionNumber; j++)
				{
					if( (pRTSPServer->pClient[j].ulSessionID != 0) && (pRTSPServer->ulAudiobackSessionID[i] == pRTSPServer->pClient[j].ulSessionID) )
					{
						printf("[%s] Audioback%d is timeout, SessionID = %lu\n",__FUNCTION__, i, pRTSPServer->pClient[j].ulSessionID);
						if(pRTSPServer->pClient[j].acMediaType[0][0] == '\0')
						{
							RTSPServer_InitClient(&pRTSPServer->pClient[j]);
						}
						else
						{
							RTSPServer_SessionStop(pRTSPServer,pRTSPServer->pClient[j].ulSessionID,pRTSPServer->pClient[j].iMulticast);
						}
					}
				}
			}
		}
	}

	return 0;
}

#endif



