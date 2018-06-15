
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
 *  File name          :   mediachannel_shmem
 *  File description   :   media channel for share memory
 *  Author             :   Louis
 *  Created at         :   2008/10/08 
 *  Note               :   
 **************************************************************************/

#include <string.h>
#include <signal.h>
#include <sys/file.h>

#include "rtsps_callback.h"
#include "encrypt_md5.h"
#include "encrypt_base64.h"
#include "rtsps_ubuffer.h"
//20120830 added by Jimmy for metadata
/*#ifdef _METADATA_ENABLE
#include "ivametadata_dec_onvif.h"
#endif*/

#define	SHMEM_ASAP_THRESHOLD		2


//20130605 added by Jimmy to support metadata event
/*#ifdef _METADATA_ENABLE
EMetadataStreamTag aMetadataTag[] = {
	METADATASTREAMTAG_VIDEOANALYTICS
#ifdef _METADATA_EVENT_ENABLE
	, METADATASTREAMTAG_EVENT
#endif
};
#endif*/

//20080925 Unix socket check
SCODE StreamSvr_CheckUnixSocket(TSTREAMSERVERINFO *pThis)
{
	int		i = 0, iSckOptLen = 0;
	char    acBuf[16];

	for( i=0; i< VIDEO_TRACK_NUMBER; i++ )
	{
		memset( acBuf, 0, sizeof(acBuf));
		iSckOptLen = sizeof(int);

		if(getsockopt(pThis->tVideoSrcInfo[i].iFdSock, SOL_SOCKET, SO_RCVBUF, (void*)acBuf, (unsigned int *)&iSckOptLen) < 0)
		{
			//Socket Error, proceed to reconnect
			close(pThis->tVideoSrcInfo[i].iFdSock);
			pThis->tVideoSrcInfo[i].iFdSock = -1;

			if(CfgParser_GetUnixDomainSocket(&pThis->tVideoSrcInfo[i].acSockPathName, &pThis->tVideoSrcInfo[i].iFdSock) != S_OK)
			{
				printf("Warning! Reconnect unix domain socket %s failed!\n", pThis->tVideoSrcInfo[i].acSockPathName);
				syslog(LOG_ERR, "[APP] Reconnect unix domain socket %s failed!\n", pThis->tVideoSrcInfo[i].acSockPathName);
			}
			else
			{
				printf("Reconnect unix domain socket %s to socket %d!\n", pThis->tVideoSrcInfo[i].acSockPathName, pThis->tVideoSrcInfo[i].iFdSock);
				syslog(LOG_ERR, "Reconnect unix domain socket %s to socket %d!\n", pThis->tVideoSrcInfo[i].acSockPathName, pThis->tVideoSrcInfo[i].iFdSock);
			}
		}
	}
	for( i=0; i< AUDIO_TRACK_NUMBER; i++ )
	{
		memset( acBuf, 0, sizeof(acBuf));
		iSckOptLen = sizeof(int);

		if(getsockopt(pThis->tAudioSrcInfo[i].iFdSock, SOL_SOCKET, SO_RCVBUF, (void*)acBuf, (unsigned int *)&iSckOptLen) < 0)
		{
			//Socket Error, proceed to reconnect
			close(pThis->tAudioSrcInfo[i].iFdSock);
			pThis->tAudioSrcInfo[i].iFdSock = -1;

			if(CfgParser_GetUnixDomainSocket(&pThis->tAudioSrcInfo[i].acSockPathName, &pThis->tAudioSrcInfo[i].iFdSock) != S_OK)
			{
				printf("Warning! Reconnect unix domain socket %s failed!\n", pThis->tAudioSrcInfo[i].acSockPathName);
				syslog(LOG_ERR, "[APP] Reconnect unix domain socket %s failed!\n", pThis->tAudioSrcInfo[i].acSockPathName);
			}
			else
			{
				printf("Reconnect unix domain socket %s to socket %d!\n", pThis->tAudioSrcInfo[i].acSockPathName, pThis->tAudioSrcInfo[i].iFdSock);
				syslog(LOG_ERR, "Reconnect unix domain socket %s to socket %d!\n", pThis->tAudioSrcInfo[i].acSockPathName, pThis->tAudioSrcInfo[i].iFdSock);
			}
		}
	}
	//20120830 added by Jimmy for metadata
	for( i=0; i< METADATA_TRACK_NUMBER; i++ )
	{
		memset( acBuf, 0, sizeof(acBuf));
		iSckOptLen = sizeof(int);

		if(getsockopt(pThis->tMetadataSrcInfo[i].iFdSock, SOL_SOCKET, SO_RCVBUF, (void*)acBuf, (unsigned int *)&iSckOptLen) < 0)
		{
			//Socket Error, proceed to reconnect
			close(pThis->tMetadataSrcInfo[i].iFdSock);
			pThis->tMetadataSrcInfo[i].iFdSock = -1;

			if(CfgParser_GetUnixDomainSocket(&pThis->tMetadataSrcInfo[i].acSockPathName, &pThis->tMetadataSrcInfo[i].iFdSock) != S_OK)
			{
				printf("Warning! Reconnect unix domain socket %s failed!\n", pThis->tMetadataSrcInfo[i].acSockPathName);
				syslog(LOG_ERR, "[APP] Reconnect unix domain socket %s failed!\n", pThis->tMetadataSrcInfo[i].acSockPathName);
			}
			else
			{
				printf("Reconnect unix domain socket %s to socket %d!\n", pThis->tMetadataSrcInfo[i].acSockPathName, pThis->tMetadataSrcInfo[i].iFdSock);
				syslog(LOG_ERR, "Reconnect unix domain socket %s to socket %d!\n", pThis->tMetadataSrcInfo[i].acSockPathName, pThis->tMetadataSrcInfo[i].iFdSock);
			}
		}
	}

	return S_OK;
}

#ifdef _SHARED_MEM

//20120801 added by Jimmy for metadata
SCODE StreamSvrShmemMetadataCallback(DWORD dwInstance, DWORD dwCallbackType, void *pvCallbackData)
{
	static int iIndex = -1;
	TSTREAMSERVERINFO *pThis = (TSTREAMSERVERINFO *)dwInstance;
	//int         iSDPSize=0;
	//char        pcSDPBuffer[RTSPSTREAMING_SDP_MAXSIZE];

	if(dwCallbackType == MEDIA_CALLBACK_SHM_SELECT_SOCKET)
	{
		TShmemSelectInfo	*pSelectInfo = (TShmemSelectInfo *)pvCallbackData;
		TUBuffer			*pUBuffer = NULL;		
		int					i = 0, iMaxSck = 0, iResult = 0;
		fd_set				fdsRead;
		struct timeval		timeout;

		if( pThis->iGotMetadataFlag == FALSE)
		{
		    usleep(10);
		    return S_FAIL;   
		}
		
		//Init Select
		timeout.tv_sec = 0;
		//20140612 modified by Charles to avoid busy loop in some platform with high resolution timer
		//20150424 modified again for Rossini
		timeout.tv_usec = 100000;//10000;//100;
		
		FD_ZERO(&fdsRead);
		
		iMaxSck = pSelectInfo->iMaxSck;

		//Add read socket 
		for( i=0 ; i<METADATA_TRACK_NUMBER; i++)
		{
			if( pThis->tMetadataSrcInfo[i].iFdSock > 0 )
				FD_SET(pThis->tMetadataSrcInfo[i].iFdSock, &fdsRead);
				
			if( pThis->tMetadataSrcInfo[i].iFdSock > iMaxSck )
				iMaxSck = pThis->tMetadataSrcInfo[i].iFdSock ;  
		}

		//Select
		iResult = select(iMaxSck + 1, &fdsRead, pSelectInfo->pfdsWrite, NULL, &timeout);
		pSelectInfo->iResult = iResult;

		//Read in result
		if(iResult > 0)
		{
			for( iIndex = 0 ; iIndex < METADATA_TRACK_NUMBER; iIndex ++)
			{
				if(pThis->tMetadataSrcInfo[iIndex].iFdSock > 0 && FD_ISSET(pThis->tMetadataSrcInfo[iIndex].iFdSock, &fdsRead))
				{
					if(read(pThis->tMetadataSrcInfo[iIndex].iFdSock, pThis->pbyMetadataUBuffer, RTSPS_METADATA_UBUFFER_HEADERSIZE) <= 0 )
					{
						printf("Unix socket %d of metadata index %d read error: errno %d\n", pThis->tMetadataSrcInfo[iIndex].iFdSock, iIndex, errno);
						//20080925 unix socket check
						StreamSvr_CheckUnixSocket(pThis);
 		    			continue;
					}
					else
					{
						pUBuffer = (TUBuffer *)pThis->pbyMetadataUBuffer;
						if (pUBuffer->dwDataType == FOURCC_CONF)
						{	
							TRTSPSTREAMING_METADATAENCODING_PARAM stRTSPMetadataParam;
							memset(&stRTSPMetadataParam, 0, sizeof(TRTSPSTREAMING_METADATAENCODING_PARAM));

							stRTSPMetadataParam.iBitRate = 12800;
							stRTSPMetadataParam.iPacketTime = 200;
							stRTSPMetadataParam.iOctetAlign = 1;
							stRTSPMetadataParam.iAMRcrc=0;
							stRTSPMetadataParam.iRobustSorting=0;

							rtspstrcpy(stRTSPMetadataParam.acTrackName, pThis->tMetadataSrcInfo[iIndex].acTrackName, sizeof(stRTSPMetadataParam.acTrackName));
						}
						else if (pUBuffer->dwDataType == FOURCC_MIVA)
						{
							//update the marker
							pSelectInfo->aiNewFrame[iIndex] = 1;
						}
					}
				}
			}
		}

		if(iResult < 0)
		{
			//20080925 unix socket check
			StreamSvr_CheckUnixSocket(pThis);
			return S_FAIL;
		}

		return S_OK;	
	}
	else if(dwCallbackType == MEDIA_CALLBACK_SHM_REQUEST_BUFFER)
	{
		TShmemSessionInfo	*pShmInfo = (TShmemSessionInfo *)pvCallbackData;

		//Set Event parser opt
		TEPSetOption	    tEPSetOpt;
		if(pShmInfo->tShmemMetadataMediaInfo.acVideoAnalyticsConfigToken[0] == '\0')
		{
			tEPSetOpt.szSceneTag = NULL;
		}
		else
		{
			tEPSetOpt.szSceneTag = pShmInfo->tShmemMetadataMediaInfo.acVideoAnalyticsConfigToken;
		}
		tEPSetOpt.iBufClientID = pShmInfo->tShmemMetadataMediaInfo.iShmemClientBufID;
		EventParser_SetOption(&pShmInfo->tShmemMetadataMediaInfo.ahShmemHandle[0], &tEPSetOpt);

		//get metadata via event parser
		if(pShmInfo->tShmemMetadataMediaInfo.bGetNewData)
		{
			pShmInfo->tShmemMetadataMediaInfo.bGetNewData = FALSE;
			pShmInfo->tShmemMetadataMediaInfo.iProcessIndex = 0;

			if (EventParser_GetDataWithExtension(&pShmInfo->tShmemMetadataMediaInfo.ahShmemHandle[0], &pShmInfo->tGetEventInfo) != S_OK)
			{
				EventParser_ReleaseDataBuf(&pShmInfo->tShmemMetadataMediaInfo.ahShmemHandle[0]);
				pShmInfo->tShmemMetadataMediaInfo.bGetNewData = TRUE;
				return S_FAIL;
			}
		}

		if(pShmInfo->tGetEventInfo.uiTotalEvents == 0)
		{
			EventParser_ReleaseDataBuf(&pShmInfo->tShmemMetadataMediaInfo.ahShmemHandle[0]);
			pShmInfo->tShmemMetadataMediaInfo.bGetNewData = TRUE;
			return S_FAIL;
		}
		// printf("pShmInfo->tGetEventInfo.uiTotalEvents = %u\n", pShmInfo->tGetEventInfo.uiTotalEvents);



		pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.dwBytesUsed = 0;

		//read metadata
		TUBuffer* pUbuffer = (TUBuffer*)pShmInfo->tGetEventInfo.ptEPData[pShmInfo->tShmemMetadataMediaInfo.iProcessIndex].read_memory;
		char*  pbyPointer = (char *)pShmInfo->tGetEventInfo.ptEPData[pShmInfo->tShmemMetadataMediaInfo.iProcessIndex].read_memory;

		DWORD dwResult = 0;
		//copy userdata, 4 means TAG_XML_EXTENINFO + sizeof(userdata)
		unsigned int uTlvBufferLen = pUbuffer->dwUserDataSize - 4;
		memcpy(pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.pbyBuffer, pbyPointer + sizeof(TUBuffer) + 4, pUbuffer->dwUserDataSize - 4);
		dwResult += uTlvBufferLen;

		//write onvif header
		dwResult += snprintf((char *)pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.pbyBuffer + dwResult,
								   RTSPSTREAMING_METADATAXML_SIZE - dwResult,
								   "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
								   "<tt:MetadataStream xmlns:tt=\"http://www.onvif.org/ver10/schema\" xmlns:wsnt=\"http://docs.oasis-open.org/wsn/b-2\">");


		int iProcessIndex = pShmInfo->tShmemMetadataMediaInfo.iProcessIndex;
		pbyPointer += (sizeof(TUBuffer) + pUbuffer->dwUserDataSize);
		//printf("sizeof ubuffer + userdatasize = %u + %u\n", sizeof(TUBuffer), pUbuffer->dwUserDataSize);
//		pbyPointer += 72;
//		printf("content = %s\n", pbyPointer);

		//compose the start tag
		if(pShmInfo->tGetEventInfo.ptEPData[iProcessIndex].uiDatafmt == DATA_FMT_ONVIFANALYTICS)
		{
			if(!pShmInfo->tShmemMetadataMediaInfo.bAnalytics)
			{
				EventParser_ReleaseDataBuf(&pShmInfo->tShmemMetadataMediaInfo.ahShmemHandle[0]);
				pShmInfo->tShmemMetadataMediaInfo.bGetNewData = TRUE;
				return S_FAIL;
			}
			dwResult += snprintf((char *)pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.pbyBuffer + dwResult,
							   RTSPSTREAMING_METADATAXML_SIZE - dwResult, "<tt:VideoAnalytics>");
		}
		else if(0) //eventparser not yet support PTZ Data Format
		{
			dwResult += snprintf((char *)pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.pbyBuffer + dwResult,
							   RTSPSTREAMING_METADATAXML_SIZE - dwResult, "<tt:PTZ>");
		}
		else if(pShmInfo->tGetEventInfo.ptEPData[iProcessIndex].uiDatafmt == DATA_FMT_ONVIFNOTIFY)
		{
			dwResult += snprintf((char *)pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.pbyBuffer + dwResult,
							   RTSPSTREAMING_METADATAXML_SIZE - dwResult, "<tt:Event>");
		}
		else if(0) //eventparser not yet support EXTENSTION Data Format
		{
			dwResult += snprintf((char *)pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.pbyBuffer + dwResult,
							   RTSPSTREAMING_METADATAXML_SIZE - dwResult, "<tt:Extension>");
		}

		dwResult += snprintf((char *)pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.pbyBuffer + dwResult,
								       RTSPSTREAMING_METADATAXML_SIZE - dwResult, "%s", pbyPointer);

		//compose the end tag
		if(pShmInfo->tGetEventInfo.ptEPData[iProcessIndex].uiDatafmt == DATA_FMT_ONVIFANALYTICS)
		{
			dwResult += snprintf((char *)pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.pbyBuffer + dwResult,
							   RTSPSTREAMING_METADATAXML_SIZE - dwResult, "</tt:VideoAnalytics>");
		}
		else if(0)
		{
			dwResult += snprintf((char *)pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.pbyBuffer + dwResult,
							   RTSPSTREAMING_METADATAXML_SIZE - dwResult, "</tt:PTZ>");
		}
		else if(pShmInfo->tGetEventInfo.ptEPData[iProcessIndex].uiDatafmt == DATA_FMT_ONVIFNOTIFY)
		{
			dwResult += snprintf((char *)pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.pbyBuffer + dwResult,
							   RTSPSTREAMING_METADATAXML_SIZE - dwResult, "</tt:Event>");
		}
		else if(0)
		{
			dwResult += snprintf((char *)pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.pbyBuffer + dwResult,
							   RTSPSTREAMING_METADATAXML_SIZE - dwResult, "</tt:Extension>");
		}

		dwResult += snprintf((char *)pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.pbyBuffer + dwResult,
								   RTSPSTREAMING_METADATAXML_SIZE - dwResult,
								   "</tt:MetadataStream>");
		// printf("content = %s\n", (char *)pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.pbyBuffer + pUbuffer->dwUserDataSize - 4);
		//parser ubuffer
		{
			pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.dwSecond = pUbuffer->dwSec;
			pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.dwMilliSecond = (pUbuffer->dwUSec / 1000);
			//Remaining size calculation involves only entropy data 
			pShmInfo->tShmemMetadataMediaInfo.iRemainingSize = dwResult - uTlvBufferLen; 

		}
		pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.dwBytesUsed = dwResult - uTlvBufferLen;
		pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.dwStreamIndex = pThis->tMetadataSrcInfo[0].iMediaIndex;
		pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.dwOffset = uTlvBufferLen; //start from tlv application data
		pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.tFrameType = MEDIADB_FRAME_INTRA;
		pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.pdwPacketSize[0] = 1;
		pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.pdwPacketSize[1] = pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.dwBytesUsed;
		pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.wPacketCount = 0;
				pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.dwCurrentPosition = 0;

		pShmInfo->tGetEventInfo.uiTotalEvents--;
		pShmInfo->tShmemMetadataMediaInfo.iProcessIndex++;
		if(pShmInfo->tGetEventInfo.uiTotalEvents == 0)
		{
			pShmInfo->tShmemMetadataMediaInfo.bGetNewData = TRUE;
		EventParser_ReleaseDataBuf(&pShmInfo->tShmemMetadataMediaInfo.ahShmemHandle[0]);
	}
		
	}
#ifdef _METADATA_ENABLE
	//20130605 modified by Jimmy to support metadata event
	//20140819 modified by Charles for eventparser API
	else if(dwCallbackType == MEDIA_CALLBACK_SHM_REQUEST_BUFFER_OLD)
	{
		TShmemSessionInfo	*pShmInfo = (TShmemSessionInfo *)pvCallbackData;
		//TUBuffer			*pUBuffer = NULL;
		//TUBuffer			*pFirstAvailableUBuffer = NULL;
		//BYTE				*pcSharedUBuffer = NULL;   
		//BOOL                 bHasMetadata = FALSE;     
		//DWORD				dwSharedSize = 0;
		//TSMemReadParam		tSMRParam;
		//unsigned short 		usTag = 0,usLength = 0;
		//char *pcSize = NULL;
        //BOOL                bMediaOnDemand = pShmInfo->tShmemMetadataMediaInfo.bMediaOnDemand;
		BYTE                *pbyPointer = NULL ;
		DWORD               dwMetadataSize = 0, dwResult = 0;
		int	                i;
        BOOL                bHasWrittenXMLOpeningTag = FALSE;  
        BOOL                bHasAvailableMetadata = FALSE;
        TEPSetOption	    tEPSetOpt;
        struct timeval		tTimestamp;
        
        memset(&tEPSetOpt, 0, sizeof(TEPSetOption));
		pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.dwBytesUsed = 0;
		pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.dwOffset = 0;
        
        if(pShmInfo->tShmemMetadataMediaInfo.acVideoAnalyticsConfigToken[0] == '\0')
        {
            tEPSetOpt.szSceneTag = NULL;
        }
        else
        {
            tEPSetOpt.szSceneTag = pShmInfo->tShmemMetadataMediaInfo.acVideoAnalyticsConfigToken;
        }
        
        tEPSetOpt.iBufClientID = pShmInfo->tShmemMetadataMediaInfo.iShmemClientBufID;
        EventParser_SetOption(&pShmInfo->tShmemMetadataMediaInfo.ahShmemHandle[0], &tEPSetOpt);
        
        if(pShmInfo->tShmemMetadataMediaInfo.bGetNewData)
        {
            pShmInfo->tShmemMetadataMediaInfo.bGetNewData = FALSE;
            pShmInfo->tShmemMetadataMediaInfo.iProcessIndex = 0;

            if (EventParser_GetData(&pShmInfo->tShmemMetadataMediaInfo.ahShmemHandle[0], &pShmInfo->tGetEventInfo) != S_OK)
            {
                EventParser_ReleaseDataBuf(&pShmInfo->tShmemMetadataMediaInfo.ahShmemHandle[0]);
                pShmInfo->tShmemMetadataMediaInfo.bGetNewData = TRUE;
                return S_FAIL;
            }
        }

        /*for( i = 0; i < tGetEventInfo.uiTotalEvents; i++) 
        {
            printf("=====Metadata from eventparser=====\n");
            printf("\tSize: %d, fmt: %d, msg: %s\n", tGetEventInfo.ptEPData[i].uiTlvSize
												   , tGetEventInfo.ptEPData[i].uiDatafmt
												   , (char *)tGetEventInfo.ptEPData[i].read_memory);

        }*/
        if(!bHasWrittenXMLOpeningTag)
		{
		    dwResult = snprintf((char *)pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.pbyBuffer + pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.dwOffset,
						   RTSPSTREAMING_METADATAXML_SIZE - pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.dwOffset,
						   "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
						   "<tt:MetaDataStream xmlns:tt=\"http://www.onvif.org/ver10/schema\" xmlns:wsnt=\"http://docs.oasis-open.org/wsn/b-2\">");
			pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.dwBytesUsed += dwResult;
			//*(pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.pbyBuffer + pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.dwOffset + pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.dwBytesUsed) = '\0';
			
            bHasWrittenXMLOpeningTag = TRUE;
		}
        
        /*Start to compose metadata, Tag: VIDEOANALYTICS, PTZ, EVENT, EXTENSTION*/
        for(; pShmInfo->tShmemMetadataMediaInfo.iProcessIndex < pShmInfo->tGetEventInfo.uiTotalEvents; pShmInfo->tShmemMetadataMediaInfo.iProcessIndex++)
        {
            pbyPointer = NULL;
			dwMetadataSize = 0;
			dwResult = 0;
            i = pShmInfo->tShmemMetadataMediaInfo.iProcessIndex;

            pbyPointer = (char *)pShmInfo->tGetEventInfo.ptEPData[i].read_memory;
            dwMetadataSize = pShmInfo->tGetEventInfo.ptEPData[i].uiTlvSize;

            if(dwMetadataSize == 0)
            {
                continue;
            }
           
            //compose the start tag
            if(pShmInfo->tGetEventInfo.ptEPData[i].uiDatafmt == DATA_FMT_ONVIFANALYTICS) 
            {
                if(!pShmInfo->tShmemMetadataMediaInfo.bAnalytics)
                {
                    continue;
                }
                dwResult = snprintf((char *)pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.pbyBuffer + strlen(pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.pbyBuffer),
    							   RTSPSTREAMING_METADATAXML_SIZE - strlen(pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.pbyBuffer), "<tt:VideoAnalytics>"); 
            }
            else if(0) //eventparser not yet support PTZ Data Format
            {
                dwResult = snprintf((char *)pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.pbyBuffer + strlen(pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.pbyBuffer),
    							   RTSPSTREAMING_METADATAXML_SIZE - strlen(pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.pbyBuffer), "<tt:PTZ>"); 
            }
            else if(pShmInfo->tGetEventInfo.ptEPData[i].uiDatafmt == DATA_FMT_ONVIFNOTIFY)
            {
                dwResult = snprintf((char *)pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.pbyBuffer + strlen(pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.pbyBuffer),
    							   RTSPSTREAMING_METADATAXML_SIZE - strlen(pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.pbyBuffer), "<tt:Event>"); 
            }
            else if(0) //eventparser not yet support EXTENSTION Data Format
            {
                dwResult = snprintf((char *)pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.pbyBuffer + strlen(pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.pbyBuffer),
    							   RTSPSTREAMING_METADATAXML_SIZE - strlen(pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.pbyBuffer), "<tt:Extension>"); 
            }
            else
            {
                continue;
            }
            
            pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.dwBytesUsed += dwResult;
                
            //no need to convert format by rtsp now
            /*if ( IVAMetadata_TLV2MetadataStream(aMetadataTag[iMetadataTagIndex],
												pbyPointer,
												dwMetadataSize,
												pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.pbyBuffer + pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.dwOffset + pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.dwBytesUsed,
												RTSPSTREAMING_METADATAXML_SIZE - pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.dwOffset - pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.dwBytesUsed - 1,
												&dwResult) != S_OK )
    			{
    				printf("IVAMetadata_TLV2Xml failed\n");
    				if(bHasWrittenXMLOpeningTag)
    				{
    					pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.dwBytesUsed = 0;
    					pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.dwOffset = 0;
    					bHasWrittenXMLOpeningTag = FALSE;
    				}

    				continue;
    			}*/
    			
            dwResult = snprintf((char *)pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.pbyBuffer + strlen(pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.pbyBuffer),
						       RTSPSTREAMING_METADATAXML_SIZE - strlen(pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.pbyBuffer), "%s", pbyPointer); 
            bHasAvailableMetadata = TRUE;
            pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.dwBytesUsed += dwResult;
    		
            //compose the end tag
            if(pShmInfo->tGetEventInfo.ptEPData[i].uiDatafmt == DATA_FMT_ONVIFANALYTICS) 
            {
                dwResult = snprintf((char *)pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.pbyBuffer + strlen(pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.pbyBuffer),
    							   RTSPSTREAMING_METADATAXML_SIZE - strlen(pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.pbyBuffer), "</tt:VideoAnalytics>"); 
            }
            else if(0)
            {
                dwResult = snprintf((char *)pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.pbyBuffer + strlen(pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.pbyBuffer),
    							   RTSPSTREAMING_METADATAXML_SIZE - strlen(pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.pbyBuffer), "</tt:PTZ>"); 
            }
            else if(pShmInfo->tGetEventInfo.ptEPData[i].uiDatafmt == DATA_FMT_ONVIFNOTIFY)
            {
                dwResult = snprintf((char *)pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.pbyBuffer + strlen(pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.pbyBuffer),
    							   RTSPSTREAMING_METADATAXML_SIZE - strlen(pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.pbyBuffer), "</tt:Event>"); 
            }
            else if(0)
            {
                dwResult = snprintf((char *)pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.pbyBuffer + strlen(pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.pbyBuffer),
    							   RTSPSTREAMING_METADATAXML_SIZE - strlen(pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.pbyBuffer), "</tt:Extension>"); 
            }
            
            pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.dwBytesUsed += dwResult;
            pShmInfo->tShmemMetadataMediaInfo.iProcessIndex ++;
            break; //process only one tag

        }

        if(pShmInfo->tShmemMetadataMediaInfo.iProcessIndex >= pShmInfo->tGetEventInfo.uiTotalEvents)
        {
            pShmInfo->tShmemMetadataMediaInfo.bGetNewData = TRUE;
            //20160107 remove by faber, we should release databuf whether the data we got or not
            //if(pShmInfo->tGetEventInfo.uiTotalEvents != 0) 
            {
                EventParser_ReleaseDataBuf(&pShmInfo->tShmemMetadataMediaInfo.ahShmemHandle[0]);
            }
        }
        
        if(!bHasAvailableMetadata)
        {
            pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.dwBytesUsed = 0;
            return S_FAIL;
        }

        dwResult = snprintf((char *)pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.pbyBuffer + strlen(pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.pbyBuffer),
						   RTSPSTREAMING_METADATAXML_SIZE - strlen(pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.pbyBuffer),
						   "</tt:MetaDataStream>");
		pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.dwBytesUsed += dwResult;
        //20140812 Added by Charles, Fix RTSP crush, when metadata size is larger than buffer size
        if(pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.dwBytesUsed > (RTSPSTREAMING_METADATAXML_SIZE - pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.dwOffset - 1))
        {
            syslog(LOG_ERR, "Metadata size exceeding upper limit!\n");
            printf("====Warning!! Metadata size exceeding upper limit!====\n");
            pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.dwBytesUsed = 0;
            return S_FAIL;
        }
        
        gettimeofday(&tTimestamp, NULL);
        pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.dwSecond = tTimestamp.tv_sec;
		pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.dwMilliSecond = (tTimestamp.tv_usec / 1000);
		pShmInfo->tShmemMetadataMediaInfo.iRemainingSize = pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.dwBytesUsed;
		pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.dwStreamIndex = pThis->tMetadataSrcInfo[0].iMediaIndex;
		pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.dwStreamType = mctMIVA;
		pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.bChangeSetting = FALSE;
		pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.tFrameType = MEDIADB_FRAME_INTRA;
		pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.pdwPacketSize[0] = 1;
		pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.pdwPacketSize[1] = pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.dwBytesUsed;
		pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.wPacketCount = 0;
		pShmInfo->tShmemMetadataMediaInfo.tStreamBuffer.dwCurrentPosition = 0;

		return S_OK;
	}
#endif
	else if( dwCallbackType == MEDIA_CALLBACK_CHECK_CODEC_INDEX)
    {
        int iSDPIndex = (int)pvCallbackData;
        
        if( iSDPIndex > MULTIPLE_STREAM_NUM )
            return -1;
        
        return pThis->tMetadataSrcInfo[pThis->tStreamInfo[iSDPIndex-1].iMetadataSrcIndex-1].iMediaIndex;
        
		/*int i;
		
		for(i=0; i<VIDEO_TRACK_NUMBER; i++)
		{
		    if( strcmp(pThis->tVideoSrcInfo[i].acTrackName,(char*)pvCallbackData) == 0 )
		    {
		        return pThis->tVideoSrcInfo[i].iMediaIndex;
		    }
		}
		return -1;*/
	}
    else // MEDIA_CALLBACK_FLUSH_BUFFER
    {
    	return S_OK;
    }
	
	return S_OK;

}


SCODE StreamSvrShmemAudioCallback(DWORD dwInstance, DWORD dwCallbackType, void *pvCallbackData)
{
	static int iIndex = -1;
	TSTREAMSERVERINFO *pThis = (TSTREAMSERVERINFO *)dwInstance;
	int         iSDPSize=0;
	char        pcSDPBuffer[RTSPSTREAMING_SDP_MAXSIZE];

	if(dwCallbackType == MEDIA_CALLBACK_SHM_SELECT_SOCKET)
	{
		TShmemSelectInfo	*pSelectInfo = (TShmemSelectInfo *)pvCallbackData;
		TUBuffer			*pUBuffer = NULL;		
		int					i = 0, iMaxSck = 0, iResult = 0;
		fd_set				fdsRead;
		struct timeval		timeout;

		if( pThis->iGotAudioFlag == FALSE)
		{
		    usleep(10);
		    return S_FAIL;   
		}
		
		//Init Select
		timeout.tv_sec = 0;
		//20140612 modified by Charles to avoid busy loop in some platform with high resolution timer
		//20150424 modified again for Rossini
		timeout.tv_usec = 100000;//10000;//100;
		
		FD_ZERO(&fdsRead);
		
		iMaxSck = pSelectInfo->iMaxSck;

		//Add read socket 
		for( i=0 ; i<AUDIO_TRACK_NUMBER; i++)
		{
			if( pThis->tAudioSrcInfo[i].iFdSock > 0 )
				FD_SET(pThis->tAudioSrcInfo[i].iFdSock, &fdsRead);
				
			if( pThis->tAudioSrcInfo[i].iFdSock > iMaxSck )
				iMaxSck = pThis->tAudioSrcInfo[i].iFdSock ;  
		}

		//Select
		iResult = select(iMaxSck + 1, &fdsRead, pSelectInfo->pfdsWrite, NULL, &timeout);
		pSelectInfo->iResult = iResult;

		//Read in result
		if(iResult > 0)
		{
			for( iIndex = 0 ; iIndex < AUDIO_TRACK_NUMBER; iIndex ++)
			{
				if(pThis->tAudioSrcInfo[iIndex].iFdSock > 0 && FD_ISSET(pThis->tAudioSrcInfo[iIndex].iFdSock, &fdsRead))
				{
					if(read(pThis->tAudioSrcInfo[iIndex].iFdSock, pThis->pbyAudiUBuffer, RTSPS_AUDIO_UBUFFER_HEADERSIZE) <= 0 )
					{
						printf("Unix socket %d of audio index %d read error: errno %d\n", pThis->tAudioSrcInfo[iIndex].iFdSock, iIndex, errno);
						//20080925 unix socket check
						StreamSvr_CheckUnixSocket(pThis);
 		    			continue;
					}
					else
					{
						pUBuffer = (TUBuffer *)pThis->pbyAudiUBuffer;
						if (pUBuffer->dwDataType == FOURCC_CONF)
						{	
							TRTSPSTREAMING_AUDENCODING_PARAM stRTSPAudioParam;
							memset(&stRTSPAudioParam, 0, sizeof(TRTSPSTREAMING_AUDENCODING_PARAM));

							stRTSPAudioParam.iBitRate = 12800;
							stRTSPAudioParam.iPacketTime = 200;
							stRTSPAudioParam.iOctetAlign = 1;
							stRTSPAudioParam.iAMRcrc=0;
							stRTSPAudioParam.iRobustSorting=0;
							stRTSPAudioParam.bIsBigEndian = FALSE;

							rtspstrcpy(stRTSPAudioParam.acTrackName,pThis->tAudioSrcInfo[iIndex].acTrackName, sizeof(stRTSPAudioParam.acTrackName));

							if (pUBuffer->dwFollowingDataType == FOURCC_AAC4)
							{
								TUBufferConfAAC4 *pAAC4ConfUBuffer;
								pAAC4ConfUBuffer = (TUBufferConfAAC4 *)pThis->pbyAudiUBuffer;

								stRTSPAudioParam.iM4AProfileLevel = pAAC4ConfUBuffer->dwProfileLevel;
								stRTSPAudioParam.iM4ASpecConfLen = pAAC4ConfUBuffer->dwSize - sizeof(TUBufferConfAAC4);
								memcpy(stRTSPAudioParam.acM4ASpecConf, (BYTE *)pAAC4ConfUBuffer+sizeof(TUBufferConfAAC4), stRTSPAudioParam.iM4ASpecConfLen);
								stRTSPAudioParam.iAudioCodecType = ractAAC4;
								stRTSPAudioParam.iChanNum = pAAC4ConfUBuffer->dwChannelNumber;
								stRTSPAudioParam.iClockRate = pAAC4ConfUBuffer->dwSampleRate;
								/*printf("dwChannelNumber %u, dwSampleRate %u, dwProfileLevel %u\n",
									pAAC4ConfUBuffer->dwChannelNumber, pAAC4ConfUBuffer->dwSampleRate, pAAC4ConfUBuffer->dwProfileLevel);*/
							}
							else if (pUBuffer->dwFollowingDataType == FOURCC_GAMR)
							{
								TUBufferConfGAMR *pGAMRConfUBuffer;
								pGAMRConfUBuffer = (TUBufferConfGAMR *)pThis->pbyAudiUBuffer;

								stRTSPAudioParam.iAudioCodecType = ractGAMR;
								stRTSPAudioParam.iChanNum= 1;
								stRTSPAudioParam.iClockRate = pGAMRConfUBuffer->dwSampleRate;

								pThis->tAudioSrcInfo[iIndex].iFramesPerUBuffer = pGAMRConfUBuffer->byFramesPerSample;
							}
#ifdef _G726_AUDIOIN
							else if (pUBuffer->dwFollowingDataType == FOURCC_G726)
							{
								TUBufferConfG726 *pG726ConfUBuffer;
								pG726ConfUBuffer = (TUBufferConfG726 *)pThis->pbyAudiUBuffer;

								stRTSPAudioParam.iAudioCodecType = ractG726;
								stRTSPAudioParam.iChanNum= pG726ConfUBuffer->dwChannelNumber;
								stRTSPAudioParam.iClockRate = 8000;
								stRTSPAudioParam.iBitRate = pG726ConfUBuffer->dwBitRate;
								stRTSPAudioParam.bIsBigEndian = pG726ConfUBuffer->bIsBigEndian;
							}
#endif
#ifdef _G711_AUDIOIN
							else // if (pUBuffer->dwFollowingDataType == FOURCC_G711)
							{
								TUBufferConfG711 *pG711ConfUBuffer;
								pG711ConfUBuffer = (TUBufferConfG711 *)pThis->pbyAudiUBuffer;

								stRTSPAudioParam.iAudioCodecType = ( pG711ConfUBuffer->dwCompressionLaw ==  FOURCC_ULAW ) ? ractG711u : ractG711a;
								stRTSPAudioParam.iChanNum= pG711ConfUBuffer->dwChannelNumber;
								stRTSPAudioParam.iClockRate = 8000;
							}
#endif 
							/* Add by ShengFu: check which stream index this media source is mapped to */
							for (i=0; i<pThis->dwStreamNumber; i++)
							{
								if(  pThis->tStreamInfo[i].iAudioSrcIndex == iIndex + 1 )        
								{
									RTSPStreaming_SetAudioParameters(pThis->hRTSPServer, i+1, &stRTSPAudioParam, 
													RTSPSTREAMING_AUDIO_BITRATE|RTSPSTREAMING_AUDIO_CLOCKRATE|
													RTSPSTREAMING_AUDIO_PACKETTIME|RTSPSTREAMING_AUDIO_OCTECTALIGN|
													RTSPSTREAMING_AUDIO_AMRCRC|RTSPSTREAMING_AUDIO_ROBUSTSORT|
													RTSPSTREAMING_AUDIO_SET_CI|RTSPSTREAMING_AUDIO_CODECTYPE|
													RTSPSTREAMING_AUDIO_PACKINGMODE);
    								printf("[APP]: %d audio source is set to %d stream \r\n",iIndex + 1,i +1);    									

    								//20120925 modified by Jimmy for ONVIF backchannel
    								//20111124 modify by danny support Backchannel Multicast for ONVIF test tool 1.02.4
    								iSDPSize = RTSPStreaming_ComposeAVSDP(pThis->hRTSPServer,i+1 ,(unsigned long)NULL , FALSE, pThis->tMulticastInfo[i].iAlwaysMulticast, pcSDPBuffer,RTSPSTREAMING_SDP_MAXSIZE, FALSE, REQUIRE_NONE);
				    	            
    								if( pThis->tStreamInfo[i].iEnable == TRUE)
    	            					StreamSvrWriteFile(pThis->tStreamInfo[i].szSDPFullPathName,pcSDPBuffer, iSDPSize);
    								else	
										remove(pThis->tStreamInfo[i].szSDPFullPathName);

								}
							}			
						}
						else if (pUBuffer->dwDataType == FOURCC_AAC4)
						{
							//update the marker
							pSelectInfo->aiNewFrame[iIndex] = 1;
						}
						else if (pUBuffer->dwDataType == FOURCC_GAMR)
						{
							//update the marker
							pSelectInfo->aiNewFrame[iIndex] = 1;
						}
#ifdef _G711_AUDIOIN
						else if(pUBuffer->dwDataType == FOURCC_G711)
						{
							//update the marker
							pSelectInfo->aiNewFrame[iIndex] = 1;
						}
#endif
#ifdef _G726_AUDIOIN
						else if(pUBuffer->dwDataType == FOURCC_G726)
						{
							//update the marker
							pSelectInfo->aiNewFrame[iIndex] = 1;
						}
#endif

					}
				}
			}
		}

		if(iResult < 0)
		{
			//20080925 unix socket check
			printf("audio result = %d\n, iResult", iResult);
			StreamSvr_CheckUnixSocket(pThis);
			return S_FAIL;
		}

		return S_OK;	
	}				
	else if(dwCallbackType == MEDIA_CALLBACK_SHM_REQUEST_BUFFER)
	{
		TShmemSessionInfo	*pShmInfo = (TShmemSessionInfo *)pvCallbackData;
		TUBuffer			*pUBuffer = NULL;
		BYTE				*pcSharedUBuffer = NULL ;
		DWORD				dwSharedSize = 0;
		TSMemReadParam		tSMRParam;
        BOOL                bMediaOnDemand = pShmInfo->tShmemAudioMediaInfo.bMediaOnDemand;
        int                 TUBufferSize;

        if(bMediaOnDemand)
        {
            TUBufferSize = sizeof(TUBufferConfMOD);
        }
        else
        {
            TUBufferSize = sizeof(TUBuffer);
        }
        
		memset(&tSMRParam, 0, sizeof(TSMemReadParam));

		//tSMRParam.dwAccessMode = pShmInfo->dwAccessMode;
		if(pShmInfo->eHSMode == eHSASAP)
		{
			tSMRParam.dwAccessMode = SHRFLAG_ACCESS_CACHE;
            //20140812 Added by Charles for mod no drop frame
            if( bMediaOnDemand == TRUE )
            {                
                tSMRParam.dwAccessMode |= SHRFLAG_NO_DROP_FRAME;
            }
			pShmInfo->tShmemAudioMediaInfo.iASAPCount++;
			if(pShmInfo->tShmemAudioMediaInfo.iASAPCount > SHMEM_ASAP_THRESHOLD)
			{
				pShmInfo->tShmemAudioMediaInfo.iASAPCount = 0;
				pShmInfo->tShmemAudioMediaInfo.bFrameGenerated = FALSE;
			}
		}
		else if(pShmInfo->eHSMode == eHSAdaptiveRecording || pShmInfo->eHSMode == eHSHistory)
		{
			tSMRParam.dwAccessMode = (SHRFLAG_NO_OVERTAKE | SHRFLAG_ACCESS_CACHE);
            //20140812 Added by Charles for mod no drop frame
            if( bMediaOnDemand == TRUE )
            {                
                tSMRParam.dwAccessMode |= SHRFLAG_NO_DROP_FRAME;
            }
            else
            {
                tSMRParam.dwAccessMode |= SHRFLAG_KEEP_SYNC;
            }
		}
		else
		{
			//Default to eHSLiveStreaming
			tSMRParam.dwAccessMode = (SHRFLAG_ACCESS_CACHE);
            //20140812 Added by Charles for mod no drop frame
            if( bMediaOnDemand == TRUE )
            {                
                tSMRParam.dwAccessMode |= SHRFLAG_NO_DROP_FRAME;
            }
            else
            {
                tSMRParam.dwAccessMode |= SHRFLAG_KEEP_SYNC;
            }
		}

		if(pShmInfo->dwRefTime == 0)
		{
			OSTick_GetMSec(&pShmInfo->dwRefTime);
		}
		//tSMRParam.dwRefTimeTick = pShmInfo->dwRefTime;
		tSMRParam.dwRefTimeTick = 0;
		tSMRParam.dwPastMSec = pShmInfo->dwBypasyMSec;

		//printf("Callback to Audio request %d %d %d\n", pShmInfo->dwBypasyMSec, pShmInfo->dwProtectedDelta, pShmInfo->eHSMode);
		//20130605 modified by Jimmy to support metadata event
		if (SharedMem_QueryReadBuffer(pShmInfo->tShmemAudioMediaInfo.ahShmemHandle[0], &pShmInfo->tShmemAudioMediaInfo.ahClientBuf[0], &pcSharedUBuffer, &dwSharedSize, &tSMRParam) == S_OK)
		{
			pUBuffer = (TUBuffer *) pcSharedUBuffer;
			//20081201 check buffer
			if((pUBuffer->dwSize > dwSharedSize) || (dwSharedSize > (pUBuffer->dwSize + 4)))
			{
				printf("Warning!! Audio Ubuffer size conflict %u <--> %u\n", dwSharedSize, pUBuffer->dwSize);
				syslog(LOG_ERR, "Warning!! Ubuffer size conflict %u <--> %u\n", dwSharedSize, pUBuffer->dwSize);
				SharedMem_ReleaseReadBuffer(pShmInfo->tShmemAudioMediaInfo.ahClientBuf[0]);
				return S_FAIL;
			}
		}
		else
		{
			pShmInfo->tShmemAudioMediaInfo.iASAPCount = 0;
			pShmInfo->tShmemAudioMediaInfo.bFrameGenerated = FALSE;
			return S_FAIL;
		}
        //20141110 added by Charles for ONVIF Profile G
        if(bMediaOnDemand)
        {
            pShmInfo->tShmemAudioMediaInfo.tStreamBuffer.bOnvifDbit = ((TUBufferConfMOD *)pcSharedUBuffer)->bOnvifDbit;
        }

		//FIXME: iIndex may not be 0 
		iIndex = 0;

		if (pUBuffer->dwDataType == FOURCC_AAC4)
		{
			if( M4ABitstreamPack(&pShmInfo->tShmemAudioMediaInfo.tStreamBuffer, pUBuffer, pThis->tAudioSrcInfo[iIndex].iMediaIndex, TUBufferSize) != S_OK)
			{
				//20130605 modified by Jimmy to support metadata event
				SharedMem_ReleaseReadBuffer(pShmInfo->tShmemAudioMediaInfo.ahClientBuf[0]);
				return S_FAIL;
			}

		}
		else if (pUBuffer->dwDataType == FOURCC_GAMR)
		{
			if (AMRBitstreamPack(&pShmInfo->tShmemAudioMediaInfo.tStreamBuffer, pUBuffer, pThis->tAudioSrcInfo[iIndex].iMediaIndex, pThis->tAudioSrcInfo[iIndex].iFramesPerUBuffer, TUBufferSize) != S_OK)
			{
				//20130605 modified by Jimmy to support metadata event
				SharedMem_ReleaseReadBuffer(pShmInfo->tShmemAudioMediaInfo.ahClientBuf[0]);
				return S_FAIL;
			}
		}
#ifdef _G711_AUDIOIN
		else if (pUBuffer->dwDataType == FOURCC_G711)
		{
			if (G711BitstreamPack(&pShmInfo->tShmemAudioMediaInfo.tStreamBuffer, pUBuffer, pThis->tAudioSrcInfo[iIndex].iMediaIndex, TUBufferSize) != S_OK)
			{
				//20130605 modified by Jimmy to support metadata event
				SharedMem_ReleaseReadBuffer(pShmInfo->tShmemAudioMediaInfo.ahClientBuf[0]);
				return S_FAIL;
			}
		}
#endif
#ifdef _G726_AUDIOIN
		else if (pUBuffer->dwDataType == FOURCC_G726)
		{
			if (G726BitstreamPack(&pShmInfo->tShmemAudioMediaInfo.tStreamBuffer, pUBuffer, pThis->tAudioSrcInfo[iIndex].iMediaIndex, TUBufferSize) != S_OK)
			{
				//20130605 modified by Jimmy to support metadata event
				SharedMem_ReleaseReadBuffer(pShmInfo->tShmemAudioMediaInfo.ahClientBuf[0]);
				return S_FAIL;
			}
		}
#endif

		pShmInfo->tShmemAudioMediaInfo.iRemainingSize = pShmInfo->tShmemAudioMediaInfo.tStreamBuffer.dwBytesUsed;

		return S_OK;
	}
    else // MEDIA_CALLBACK_FLUSH_BUFFER
    {
    	return S_OK;
    }
	return S_OK;
}

SCODE StreamSvrShmemVideoCallback(DWORD dwInstance, DWORD dwCallbackType, void* pvCallbackData)
{
	static int iIndex = -1;
	TSTREAMSERVERINFO *pThis = (TSTREAMSERVERINFO *)dwInstance;
	int         i,iSDPSize=0, iSize = 0;
	char        pcSDPBuffer[RTSPSTREAMING_SDP_MAXSIZE];

	if(dwCallbackType == MEDIA_CALLBACK_SHM_SELECT_SOCKET)
	{
		TShmemSelectInfo	*pSelectInfo = (TShmemSelectInfo *)pvCallbackData;
		TUBuffer			*pUBuffer = NULL;		
		int					i = 0, iMaxSck = 0, iResult = 0;
		fd_set				fdsRead;
		struct timeval		timeout;


		if( pThis->iGotVideoFlag == FALSE)
		{
		    usleep(10);
		    return S_FAIL;   
		}

		//Init Select
		timeout.tv_sec = 0;
		//20140612 modified by Charles to avoid busy loop in some platform with high resolution timer
		//20150424 modified again for Rossini
		//NOTE:select timeout defined here may decide the polling interval of message queue
		//So, we should not set this value too large. Otherwise, Mediachannel thread won't get message from other threads immediately
		//On the other hand, we should not set this value too small, Otherwise, busy waiting will occur when process is idle
		//Now, 100ms maybe a suitable value!
		timeout.tv_usec = 100000;//10000;//100;
		
		FD_ZERO(&fdsRead);
		
		iMaxSck = pSelectInfo->iMaxSck;

		//Add read socket 
		for( i=0 ; i<VIDEO_TRACK_NUMBER; i++)
		{
		if( pThis->tVideoSrcInfo[i].iFdSock > 0 )
			FD_SET(pThis->tVideoSrcInfo[i].iFdSock, &fdsRead);
			
		if( pThis->tVideoSrcInfo[i].iFdSock > iMaxSck )
			iMaxSck = pThis->tVideoSrcInfo[i].iFdSock ;  
		}

		//Select
		iResult = select(iMaxSck + 1, &fdsRead, pSelectInfo->pfdsWrite, NULL, &timeout);
		pSelectInfo->iResult = iResult;

		//Read in result
		if(iResult > 0)
		{
			for( iIndex = 0 ; iIndex < VIDEO_TRACK_NUMBER; iIndex ++)
			{
				if(pThis->tVideoSrcInfo[iIndex].iFdSock > 0 && FD_ISSET(pThis->tVideoSrcInfo[iIndex].iFdSock, &fdsRead))
				{
					if((iSize = read(pThis->tVideoSrcInfo[iIndex].iFdSock, pThis->pbyVideUBuffer, RTSPS_VIDEO_UBUFFER_HEADERSIZE)) <= 0 )
					{
						printf("Unix socket %d of video index %d read error: errno %d\n",pThis->tVideoSrcInfo[iIndex].iFdSock, iIndex, errno);
						//20080925 unix socket check
						StreamSvr_CheckUnixSocket(pThis);
 		    			continue;
					}
					else
					{
						pUBuffer = (TUBuffer *)pThis->pbyVideUBuffer;

						if (pUBuffer->dwDataType == FOURCC_CONF)
						{
							TRTSPSTREAMING_VIDENCODING_PARAM stRTSPVideoParam;
							TUBufferConfMP4V *pMP4VConfUBuffer;
							TUBufferConfJPEG *pJPEGConfUBuffer;
							TUBufferConfH264 *pH264ConfUBuffer;
                            TUBufferConfH265 *pH265ConfUBuffer;
							
							//200081231 initialize bitrate
							memset(&stRTSPVideoParam, 0, sizeof(TRTSPSTREAMING_VIDENCODING_PARAM));

							if(pUBuffer->dwFollowingDataType == FOURCC_MP4V)
							{
								stRTSPVideoParam.eVideoCodecType = mctMP4V;

								pMP4VConfUBuffer = (TUBufferConfMP4V *)pThis->pbyVideUBuffer;

								//20100428 Added For Media on demand
								stRTSPVideoParam.dwFileLength = pMP4VConfUBuffer->dwFileLength;
								stRTSPVideoParam.iProfileLevel = pMP4VConfUBuffer->dwProfileLevel;
								stRTSPVideoParam.iMPEG4HeaderLen = pMP4VConfUBuffer->dwSize - sizeof(TUBufferConfMP4V);
								memcpy(stRTSPVideoParam.acMPEG4Header, (BYTE *)pMP4VConfUBuffer+sizeof(TUBufferConfMP4V), stRTSPVideoParam.iMPEG4HeaderLen);
								stRTSPVideoParam.acMPEG4Header[stRTSPVideoParam.iMPEG4HeaderLen] = 0;
								stRTSPVideoParam.iClockRate = 30000;
								stRTSPVideoParam.iDecoderBufferSize = 76800;
								rtspstrcpy(stRTSPVideoParam.acTrackName,pThis->tVideoSrcInfo[iIndex].acTrackName, sizeof(stRTSPVideoParam.acTrackName));
	 
								/* Add by ShengFu: check which stream index this media source is mapped to */
								for (i=0; i<pThis->dwStreamNumber; i++)
								{
									if(  pThis->tStreamInfo[i].iVideoSrcIndex == iIndex + 1 )        
									{
										RTSPStreaming_SetVideoParameters(pThis->hRTSPServer, i+1, &stRTSPVideoParam, 
														RTSPSTREAMING_VIDEO_PROLEVE  | RTSPSTREAMING_VIDEO_BITRATE  | 
														RTSPSTREAMING_VIDEO_CLOCKRATE| RTSPSTREAMING_VIDEO_MPEG4CI  | 
														RTSPSTREAMING_VIDEO_WIDTH    | RTSPSTREAMING_VIDEO_HEIGHT   | 
														RTSPSTREAMING_VIDEO_SET_CI   | RTSPSTREAMING_VIDEO_DECODEBUFF |
														RTSPSTREAMING_VIDEO_CODECTYPE);
    									printf("[APP]: %d video source(MPEG4) is set to %d stream \r\n",iIndex + 1,i +1);

										//20120925 modified by Jimmy for ONVIF backchannel
										//20111124 modify by danny support Backchannel Multicast for ONVIF test tool 1.02.4
       									iSDPSize = RTSPStreaming_ComposeAVSDP(pThis->hRTSPServer,i+1 ,(unsigned long)NULL , FALSE, pThis->tMulticastInfo[i].iAlwaysMulticast, pcSDPBuffer,RTSPSTREAMING_SDP_MAXSIZE, FALSE, REQUIRE_NONE);    	            
       									//printf("\r\n%s\r\n",pcSDPBuffer);       	            
       									if( pThis->tStreamInfo[i].iEnable == TRUE )
       	            						StreamSvrWriteFile(pThis->tStreamInfo[i].szSDPFullPathName, pcSDPBuffer, iSDPSize);
										else
											remove(pThis->tStreamInfo[i].szSDPFullPathName);       	            	
									}
								}	
							}
							else if(pUBuffer->dwFollowingDataType == FOURCC_JPEG)
							{
								stRTSPVideoParam.eVideoCodecType = mctJPEG;

								pJPEGConfUBuffer = (TUBufferConfJPEG *)pThis->pbyVideUBuffer;

								//20100428 Added For Media on demand
								stRTSPVideoParam.dwFileLength = pJPEGConfUBuffer->dwFileLength;
								stRTSPVideoParam.iWidth = pJPEGConfUBuffer->dwWidth;
								stRTSPVideoParam.iHeight = pJPEGConfUBuffer->dwHeight;
								stRTSPVideoParam.iMPEG4HeaderLen = pJPEGConfUBuffer->dwHeaderLen;
								stRTSPVideoParam.dwJPEGComponentNum = pJPEGConfUBuffer->dwNumComponents;
								stRTSPVideoParam.iClockRate = 90000;
								rtspstrcpy(stRTSPVideoParam.acTrackName,pThis->tVideoSrcInfo[iIndex].acTrackName, sizeof(stRTSPVideoParam.acTrackName));

								for (i=0; i<pThis->dwStreamNumber; i++)
								{
									if(  pThis->tStreamInfo[i].iVideoSrcIndex == iIndex + 1 )        
									{
										RTSPStreaming_SetVideoParameters(pThis->hRTSPServer, i+1, &stRTSPVideoParam, 
														RTSPSTREAMING_VIDEO_CLOCKRATE| RTSPSTREAMING_VIDEO_SET_CI  | 
														RTSPSTREAMING_VIDEO_WIDTH    | RTSPSTREAMING_VIDEO_HEIGHT  |
														RTSPSTREAMING_VIDEO_CODECTYPE);
    									printf("[APP]: %d video source(JPEG) is set to %d stream \r\n",iIndex + 1,i +1);

										//20120925 modified by Jimmy for ONVIF backchannel
										//20111124 modify by danny support Backchannel Multicast for ONVIF test tool 1.02.4
       									iSDPSize = RTSPStreaming_ComposeAVSDP(pThis->hRTSPServer,i+1 ,(unsigned long)NULL , FALSE, pThis->tMulticastInfo[i].iAlwaysMulticast, pcSDPBuffer,RTSPSTREAMING_SDP_MAXSIZE, FALSE, REQUIRE_NONE);    	            
       									//printf("\r\n%s\r\n",pcSDPBuffer);       	            
       									if( pThis->tStreamInfo[i].iEnable == TRUE )
       	            						StreamSvrWriteFile(pThis->tStreamInfo[i].szSDPFullPathName, pcSDPBuffer, iSDPSize);
										else
											remove(pThis->tStreamInfo[i].szSDPFullPathName);       	            	
									}
								}	
							}
							else if(pUBuffer->dwFollowingDataType == FOURCC_H264)
							{
								pH264ConfUBuffer = (TUBufferConfH264 *)pThis->pbyVideUBuffer;

								//20100428 Added For Media on demand
								stRTSPVideoParam.dwFileLength = pH264ConfUBuffer->dwFileLength;
								stRTSPVideoParam.iPacketizationMode = 1; //Fixed for non-interleaved
								stRTSPVideoParam.eVideoCodecType = mctH264;
								stRTSPVideoParam.iProfileLevel = 0; //Irrelevant
								stRTSPVideoParam.iBitRate = 0;
								stRTSPVideoParam.iClockRate = 90000;
								stRTSPVideoParam.iMPEG4HeaderLen = 0;
								stRTSPVideoParam.iWidth = pH264ConfUBuffer->dwWidth;	
								stRTSPVideoParam.iHeight = pH264ConfUBuffer->dwHeight; 

								//Extract Information from H.264
								H264ExtractNaluInfo(pThis->pbyVideUBuffer, &stRTSPVideoParam);

								//Prepare profile level
								snprintf(stRTSPVideoParam.acProfileLevelID, sizeof(stRTSPVideoParam.acProfileLevelID), "%02x%02x%02x", 
										pH264ConfUBuffer->abyProfileLevelId[0], 
										pH264ConfUBuffer->abyProfileLevelId[1], 
										pH264ConfUBuffer->abyProfileLevelId[2]);
								stRTSPVideoParam.acProfileLevelID[sizeof(stRTSPVideoParam.acProfileLevelID) - 1] = 0;

								rtspstrcpy(stRTSPVideoParam.acTrackName,pThis->tVideoSrcInfo[iIndex].acTrackName, sizeof(stRTSPVideoParam.acTrackName));

								for (i=0; i<pThis->dwStreamNumber; i++)
								{
									if(pThis->tStreamInfo[i].iVideoSrcIndex == iIndex + 1 )        
									{
										RTSPStreaming_SetVideoParameters(pThis->hRTSPServer, i+1, &stRTSPVideoParam, 
															RTSPSTREAMING_VIDEO_PROLEVE  | RTSPSTREAMING_VIDEO_BITRATE  | 
															RTSPSTREAMING_VIDEO_CLOCKRATE| RTSPSTREAMING_VIDEO_H264SPROP  | 
															RTSPSTREAMING_VIDEO_WIDTH    | RTSPSTREAMING_VIDEO_HEIGHT   | 
															RTSPSTREAMING_VIDEO_SET_CI   | RTSPSTREAMING_VIDEO_CODECTYPE );
    									printf("[APP]: %d video source(H264) is set to %d stream \r\n",iIndex + 1,i +1);

										//20120925 modified by Jimmy for ONVIF backchannel
										//20111124 modify by danny support Backchannel Multicast for ONVIF test tool 1.02.4
       									iSDPSize = RTSPStreaming_ComposeAVSDP(pThis->hRTSPServer,i+1 ,(unsigned long)NULL , FALSE, pThis->tMulticastInfo[i].iAlwaysMulticast, pcSDPBuffer,RTSPSTREAMING_SDP_MAXSIZE, FALSE, REQUIRE_NONE);    	            
       									//printf("\r\n%s\r\n",pcSDPBuffer);       	            
       									if( pThis->tStreamInfo[i].iEnable == TRUE )
       	            						StreamSvrWriteFile(pThis->tStreamInfo[i].szSDPFullPathName, pcSDPBuffer, iSDPSize);
										else
											remove(pThis->tStreamInfo[i].szSDPFullPathName);       	            	
									}
								}	
							}
                            //20150113 added by Charles for H.265
                            else if(pUBuffer->dwFollowingDataType == FOURCC_H265)
                            {
                                pH265ConfUBuffer = (TUBufferConfH265 *)pThis->pbyVideUBuffer;

								//20100428 Added For Media on demand
								stRTSPVideoParam.dwFileLength = pH265ConfUBuffer->dwFileLength;
								stRTSPVideoParam.eVideoCodecType = mctH265;
								//stRTSPVideoParam.iProfileLevel = 0; //Irrelevant
								stRTSPVideoParam.iBitRate = 0;
								stRTSPVideoParam.iClockRate = 90000;
								stRTSPVideoParam.iMPEG4HeaderLen = 0;
								stRTSPVideoParam.iWidth = pH265ConfUBuffer->dwWidth;	
								stRTSPVideoParam.iHeight = pH265ConfUBuffer->dwHeight;

								//Extract NAL Information from H.265
								H265ExtractNaluInfo(pThis->pbyVideUBuffer, &stRTSPVideoParam);

								rtspstrcpy(stRTSPVideoParam.acTrackName, pThis->tVideoSrcInfo[iIndex].acTrackName, sizeof(stRTSPVideoParam.acTrackName));

								for (i=0; i<pThis->dwStreamNumber; i++)
								{
									if(pThis->tStreamInfo[i].iVideoSrcIndex == iIndex + 1 )        
									{
										RTSPStreaming_SetVideoParameters(pThis->hRTSPServer, i+1, &stRTSPVideoParam, 
															/*RTSPSTREAMING_VIDEO_PROLEVE  |*/ RTSPSTREAMING_VIDEO_BITRATE  | 
															RTSPSTREAMING_VIDEO_CLOCKRATE| RTSPSTREAMING_VIDEO_H265SPROP  | 
															RTSPSTREAMING_VIDEO_WIDTH    | RTSPSTREAMING_VIDEO_HEIGHT   | 
															RTSPSTREAMING_VIDEO_SET_CI   | RTSPSTREAMING_VIDEO_CODECTYPE);
    									printf("[APP]: %d video source(H265) is set to %d stream \r\n",iIndex + 1,i +1);

       									iSDPSize = RTSPStreaming_ComposeAVSDP(pThis->hRTSPServer,i+1 ,(unsigned long)NULL , FALSE, pThis->tMulticastInfo[i].iAlwaysMulticast, pcSDPBuffer,RTSPSTREAMING_SDP_MAXSIZE, FALSE, REQUIRE_NONE);    	            
       									//printf("\r\n%s\r\n",pcSDPBuffer);       	            
       									if( pThis->tStreamInfo[i].iEnable == TRUE )
       	            						StreamSvrWriteFile(pThis->tStreamInfo[i].szSDPFullPathName, pcSDPBuffer, iSDPSize);
										else
											remove(pThis->tStreamInfo[i].szSDPFullPathName);       	            	
									}
								}	
                            }
						}
						else if (pUBuffer->dwDataType == FOURCC_JPEG)
						{
							//update the marker
							pSelectInfo->aiNewFrame[iIndex] = 1;
						}
						else if (pUBuffer->dwDataType == FOURCC_MP4V)
						{
							//update the marker
							pSelectInfo->aiNewFrame[iIndex] = 1;
						}
						else if (pUBuffer->dwDataType == FOURCC_H264)
						{
							//update the marker
							pSelectInfo->aiNewFrame[iIndex] = 1;
						}
                        //20150113 added by Charles for H.265
                        else if (pUBuffer->dwDataType == FOURCC_H265)
						{
							//update the marker
							pSelectInfo->aiNewFrame[iIndex] = 1;
						}
						//20100428 Added For Media on demand
						else if (pUBuffer->dwDataType == FOURCC_MOD)
						{
							TUBufferConfMOD  *pMODConfUBuffer;
							pMODConfUBuffer = (TUBufferConfMOD *)pThis->pbyVideUBuffer;
							/*int i;
							char* p=(char*)pMODConfUBuffer;
							for ( i = 0 ; i < sizeof(TUBufferConfMOD) ; i++ )
							{
								printf("%d ", *p);
								p++;
							}*/
							printf("[%s]Receive stream %d FOURCC_MOD, bIsFromMODActively %d\n", __FUNCTION__, pMODConfUBuffer->dwStreamID + LIVE_STREAM_NUM, pMODConfUBuffer->bIsFromMODActively);
							if (pMODConfUBuffer->dwStreamID > 0 && pMODConfUBuffer->dwStreamID + LIVE_STREAM_NUM <= MULTIPLE_STREAM_NUM)
							{
								RTSPStreaming_UpdateMODControlParameters(pThis->hRTSPServer, pMODConfUBuffer);
							}
							else
							{
								printf("[%s]Receive unknown pMODConfUBuffer->dwStreamID %d FOURCC_MOD\n", __FUNCTION__, pMODConfUBuffer->dwStreamID);
							}
						}
					}
				}
			}
		}
		
		if(iResult < 0)
		{
			StreamSvr_CheckUnixSocket(pThis);
			return S_FAIL;
		}

		return S_OK;		
	}
	else if(dwCallbackType == MEDIA_CALLBACK_SHM_REQUEST_BUFFER)
	{
		TShmemSessionInfo	*pShmInfo = (TShmemSessionInfo *)pvCallbackData;
		TUBuffer			*pUBuffer = NULL;
		BYTE				*pbyPointer = NULL;
		BYTE				*pcSharedUBuffer = NULL ;
		DWORD				dwSharedSize = 0;
		TSMemReadParam		tSMRParam;
		unsigned short 		usTag = 0,usLength = 0;
		DWORD				dwPacketInfo = 0;
        BOOL                bMediaOnDemand = pShmInfo->tShmemVideoMediaInfo.bMediaOnDemand;
        int                 TUBufferSize;
		//Test TCP broken image
		//memset(pShmInfo->acUBuffer, 0, sizeof(pShmInfo->acUBuffer));
		//Self test for MOD server not finish
		/*FILE *fp;
		int iReadNum = 0;*/
        //20141110 added by Charles for ONVIF Profile G
        if(bMediaOnDemand)
        {
            TUBufferSize = sizeof(TUBufferConfMOD);
        }
        else
        {
            TUBufferSize = sizeof(TUBuffer);
        }
        
		memset(&tSMRParam, 0, sizeof(TSMemReadParam));

		tSMRParam.dwPastMSec = pShmInfo->dwBypasyMSec;
		if(pShmInfo->dwRefTime == 0)
		{
			OSTick_GetMSec(&pShmInfo->dwRefTime);
		}

		tSMRParam.dwRefTimeTick = 0;
		if(pShmInfo->eHSMode == eHSASAP)
		{
			tSMRParam.dwAccessMode = (SHRFLAG_FIRST_MARKFRAME |  SHRFLAG_ACCESS_CACHE);
            //20140812 Added by Charles for mod no drop frame
            if( bMediaOnDemand == TRUE )
            {                
                tSMRParam.dwAccessMode |= SHRFLAG_NO_DROP_FRAME;
            }
			pShmInfo->tShmemVideoMediaInfo.iASAPCount++;
			if(pShmInfo->tShmemVideoMediaInfo.iASAPCount > SHMEM_ASAP_THRESHOLD)
			{
				pShmInfo->tShmemVideoMediaInfo.iASAPCount = 0;
				pShmInfo->tShmemVideoMediaInfo.bFrameGenerated = FALSE;
			}
		}
		else if(pShmInfo->eHSMode == eHSAdaptiveRecording)
		{
			//20100812 Modified For Client Side Frame Rate Control
			tSMRParam.dwAccessMode = (SHRFLAG_FIRST_MARKFRAME | SHRFLAG_NO_OVERTAKE | SHRFLAG_ACCESS_CACHE | 
									  SHRFLAG_ADAPTIVE_MARKFRAMEONLY_MODE | SHRFLAG_ADAPTIVE_FRAMERATELIMIT_MODE);
            //20140812 Added by Charles for mod no drop frame
            if( bMediaOnDemand == TRUE )
            {                
                tSMRParam.dwAccessMode |= SHRFLAG_NO_DROP_FRAME;
            }
            else
            {
                tSMRParam.dwAccessMode |= SHRFLAG_JUMP_MARKFRAME;
            }
			tSMRParam.dwFrameLimitMSec = pShmInfo->iFrameIntervalMSec;
		}
		else if (pShmInfo->eHSMode == eHSHistory)
		{
			tSMRParam.dwAccessMode = (SHRFLAG_FIRST_MARKFRAME | SHRFLAG_NO_OVERTAKE | SHRFLAG_ACCESS_CACHE);
            //20140812 Added by Charles for mod no drop frame
            if( bMediaOnDemand == TRUE )
            {                
                tSMRParam.dwAccessMode |= SHRFLAG_NO_DROP_FRAME;
            }
            else
            {
                tSMRParam.dwAccessMode |= SHRFLAG_JUMP_MARKFRAME;
            }
			//20100812 Add For Client Side Frame Rate Control
			if( pShmInfo->iFrameIntervalMSec >= 0 )
			{
				tSMRParam.dwAccessMode |= SHRFLAG_FRAMERATE_LIMIT;
				tSMRParam.dwFrameLimitMSec = pShmInfo->iFrameIntervalMSec;
			}
		}
		else
		{
			//Default to eHSLiveStreaming
			tSMRParam.dwAccessMode = (SHRFLAG_FIRST_MARKFRAME | SHRFLAG_ACCESS_CACHE );
            //20140812 Added by Charles for mod no drop frame
            if( bMediaOnDemand == TRUE )
            {                
                tSMRParam.dwAccessMode |= SHRFLAG_NO_DROP_FRAME;
            }
            else
            {
                tSMRParam.dwAccessMode |= SHRFLAG_JUMP_MARKFRAME;
            }
			//20100812 Add For Client Side Frame Rate Control
			if( pShmInfo->iFrameIntervalMSec >= 0 )
			{
				tSMRParam.dwAccessMode |= SHRFLAG_FRAMERATE_LIMIT;
				tSMRParam.dwFrameLimitMSec = pShmInfo->iFrameIntervalMSec;
			}
		}
		//added by neil 10/12/30
		if(pShmInfo->eSVCMode != eSVCNull)
		{
			if(pShmInfo->eSVCMode == eSVCMarkFrameLevel1)
			{
				//printf("set the level to 1\n");
				tSMRParam.dwAccessMode |= SHRFLAG_SVC_MARKFRAME_AND_LEVEL1_MODE;
			}
			else if(pShmInfo->eSVCMode == eSVCMarkFrameLevel2)
			{
				//printf("set the level to 2\n");
				tSMRParam.dwAccessMode |= SHRFLAG_SVC_MARKFRAME_AND_LEVEL2_MODE;
			}
			else if(pShmInfo->eSVCMode == eSVCMarkFrameLevel3)
			{
				//printf("set the level to 3\n");
				tSMRParam.dwAccessMode |= SHRFLAG_SVC_MARKFRAME_AND_LEVEL3_MODE;
			}
			else if(pShmInfo->eSVCMode == eSVCMarkFrameLevel4)
			{
				//printf("set the level to 4\n");
				tSMRParam.dwAccessMode |= SHRFLAG_SVC_MARKFRAME_AND_LEVEL4_MODE;
			}
			else if(pShmInfo->eSVCMode == eSVCMarkFrameLevel5)
			{
				//printf("set the level to 5\n");
				tSMRParam.dwAccessMode |= SHRFLAG_SVC_MARKFRAME_AND_LEVEL5_MODE;
			}
			else if(pShmInfo->eSVCMode == eSVCMarkFrameLevel6)
			{
				//printf("set the level to 6\n");
				tSMRParam.dwAccessMode |= SHRFLAG_SVC_MARKFRAME_AND_LEVEL6_MODE;
			}
			else if(pShmInfo->eSVCMode == eSVCMarkFrameLevel7)
			{
				//printf("set the level to 7\n");
				tSMRParam.dwAccessMode |= SHRFLAG_SVC_MARKFRAME_AND_LEVEL7_MODE;
			}
			else if(pShmInfo->eSVCMode == eSVCMarkFrameOnly)
			{
				tSMRParam.dwAccessMode |= SHRFLAG_ONLY_MARKFRAME;
			}		
		}

		//added by neil 11/01/14
		if(pShmInfo->bForceFrameInterval)
		{
			tSMRParam.dwAccessMode |= SHRFLAG_FRAMERATE_LIMIT ;
            tSMRParam.dwFrameLimitMSec = pShmInfo->dwFrameInterval;
		}
		//printf("Callback to Video request %d %d %d %d\n", pShmInfo->dwBypasyMSec, pShmInfo->dwProtectedDelta, pShmInfo->eHSMode, tSMRParam.dwFrameLimitMSec);

		//20130605 modified by Jimmy to support metadata event
		if (SharedMem_QueryReadBuffer(pShmInfo->tShmemVideoMediaInfo.ahShmemHandle[0], &pShmInfo->tShmemVideoMediaInfo.ahClientBuf[0], &pcSharedUBuffer, &dwSharedSize, &tSMRParam) == S_OK)
		{
		//Self test for MOD server not finish
		/*if ((fp = fopen("/mnt/ramdisk/mjpeg.06.m4v", "r")) != NULL )
		{
			iReadNum = fread(pShmInfo->acUBuffer, sizeof(char), sizeof(pShmInfo->acUBuffer), fp);
			fclose(fp);
			printf("pShmInfo->acUBuffer=%p, iReadNum=%d\n", pShmInfo->acUBuffer, iReadNum);*/
			
			pbyPointer = pcSharedUBuffer + TUBufferSize;
			pUBuffer = (TUBuffer *) pcSharedUBuffer;
            
			//Test TCP broken image
			/*pbyPointer = (BYTE *) pShmInfo->acUBuffer + sizeof(TUBuffer);
			pUBuffer = (TUBuffer *) pShmInfo->acUBuffer;
			printf("pShmInfo->acUBuffer=%p, sizeof(pShmInfo->acUBuffer)=%d, pcSharedUBuffer=%p, pUBuffer->dwSize=%d\n",
					pShmInfo->acUBuffer,
					sizeof(pShmInfo->acUBuffer),
					pcSharedUBuffer,
					dwSharedSize);
			memcpy(pShmInfo->acUBuffer, pcSharedUBuffer, dwSharedSize);
			pShmInfo->acUBuffer[pUBuffer->dwSize] = '\0';*/
			
			//Shmem add padding to avoid four byte alignment
			if((pUBuffer->dwSize > dwSharedSize) || (dwSharedSize > (pUBuffer->dwSize + 4)))
			{
				printf("Warning!! Ubuffer size conflict %u <--> %u\n", dwSharedSize, pUBuffer->dwSize);
				syslog(LOG_ERR, "Warning!! Ubuffer size conflict %u <--> %u\n", dwSharedSize, pUBuffer->dwSize);
				SharedMem_ReleaseReadBuffer(pShmInfo->tShmemVideoMediaInfo.ahClientBuf[0]);	//20081201 release buffer
				return S_FAIL;
			}

			//printf("GetVideoUBuffer: dwSharedSize=%d, pUBuffer->dwSize=%d(%s), pUBuffer->dwUserDataSize=%d\n", dwSharedSize, pUBuffer->dwSize, (pUBuffer->bIsSync) ? "I" : "P", pUBuffer->dwUserDataSize);
			//printf("pUBuffer->dwSec = %u, pUBuffer->dwMSec = %u\r\n", pUBuffer->dwSec, pUBuffer->dwUSec / 1000);
		}
		else
		{
			pShmInfo->tShmemVideoMediaInfo.iASAPCount = 0;
			pShmInfo->tShmemVideoMediaInfo.bFrameGenerated = FALSE;
			return S_FAIL;
		}

		memcpy(&usTag, pbyPointer,2);
		memcpy(&usLength, pbyPointer+2,2);
        //20141110 added by Charles for ONVIF Profile G
        if(bMediaOnDemand)
        {
            pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.bOnvifDbit = ((TUBufferConfMOD *)pcSharedUBuffer)->bOnvifDbit;
        }
		    
	    if( pUBuffer->dwUserDataSize == 0 || !(pUBuffer->dwDataType != FOURCC_MP4V || pUBuffer->dwDataType != FOURCC_JPEG || pUBuffer->dwDataType != FOURCC_H264 ))
		{
			printf("Warning!! User data size is zero or datatype is %c%c%c%c\n",
				*((char *)&(pUBuffer->dwDataType)+0),
				*((char *)&(pUBuffer->dwDataType)+1),
				*((char *)&(pUBuffer->dwDataType)+2),
				*((char *)&(pUBuffer->dwDataType)+3)
				);
			syslog(LOG_ERR, "Warning!! User data size is zero or datatype is %c%c%c%c\n",
				*((char *)&(pUBuffer->dwDataType)+0),
				*((char *)&(pUBuffer->dwDataType)+1),
				*((char *)&(pUBuffer->dwDataType)+2),
				*((char *)&(pUBuffer->dwDataType)+3)
				);

			//20130605 modified by Jimmy to support metadata event
			SharedMem_ReleaseReadBuffer(pShmInfo->tShmemVideoMediaInfo.ahClientBuf[0]);
			return S_FAIL;
		}
		//20081226 multiple video codec support
		if(pUBuffer->dwDataType == FOURCC_MP4V)
		{
			if( (usTag != TAG_MP4V_PACKETINFO) || 
				(usLength > (MAX_MP4V_PACKET_NUM + 1)*4 ) ||
				(usLength%4 != 0)	)
			{
				//20130605 modified by Jimmy to support metadata event
				SharedMem_ReleaseReadBuffer(pShmInfo->tShmemVideoMediaInfo.ahClientBuf[0]);
				return S_FAIL;		    	
			}
			else
			{		    	 		    
				unsigned short		usIntelligentVideoTag, usIntelligentVideoLength;
				//Add by Louis 2007/11/14 for intelligent video
				pbyPointer = pbyPointer + usLength + 4;
				memcpy(&usIntelligentVideoTag, pbyPointer,2);
				memcpy(&usIntelligentVideoLength, pbyPointer+2,2);
				if(usIntelligentVideoTag != TAG_MP4V_IVAEXTENINFO)
				{
					//printf("NO Intelligent Video!\n");
					usIntelligentVideoLength = 0;
					pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.dwOffset = pUBuffer->dwUserDataSize - 4 - usLength - 4;
				}
				else
				{
					pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.dwOffset  = pUBuffer->dwUserDataSize - 4 - usLength - 4 - usIntelligentVideoLength - 4;	
				}
		
				pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.dwIntelligentVideoLength = usIntelligentVideoLength;
				//printf("PacketInfo %d Userdata %d IVINFO %d\n", usLength, pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.dwOffset, pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.dwIntelligentVideoLength);

				pbyPointer = (BYTE *)pUBuffer;
				pbyPointer += TUBufferSize;
				//memcpy(pThis->pVideBitstreamBuf->pbyBuffer, pbyPointer + 4 + usLength + 4, pUBuffer->dwSize- sizeof(TUBuffer) - 4 - usLength - 4);
				//20080814 no need to copy the memory
				pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.pbyBuffer = pbyPointer + 4 + usLength + 4;
				// set time stamp and stream type
				pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.dwSecond = pUBuffer->dwSec;
				pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.dwMilliSecond = (pUBuffer->dwUSec / 1000);
				pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.dwStreamType = mctMP4V;
				pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.bChangeSetting = FALSE;
				pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.dwBytesUsed = pUBuffer->dwSize- TUBufferSize - pUBuffer->dwUserDataSize;
				pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.dwStreamIndex = pThis->tVideoSrcInfo[iIndex].iMediaIndex;
				if(pUBuffer->bIsSync == TRUE)
				{
					pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.tFrameType = MEDIADB_FRAME_INTRA;
				}
				else
				{
					pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.tFrameType = MEDIADB_FRAME_PRED;
				}
					
				pbyPointer = (BYTE *)pUBuffer + TUBufferSize;
				pbyPointer =  pbyPointer + 4;
				memcpy(&dwPacketInfo,pbyPointer,4);

				//printf("%u packet cut\n",dwPacketInfo);
				pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.pdwPacketSize[0] = dwPacketInfo;
					
				for( i=1 ; i < (pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.pdwPacketSize[0]+1) ; i++  )
				{					
					pbyPointer = pbyPointer + 4;
					memcpy(&dwPacketInfo,pbyPointer,4);
					pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.pdwPacketSize[i] = dwPacketInfo;
					//printf("pdwPacketSize[%d]=%u\n", i, pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.pdwPacketSize[i]);
				}
		
				pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.dwIsBoundary = pUBuffer->bIsBoundary;
				pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.wPacketCount = 0;
				pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.dwCurrentPosition = 0;
				pShmInfo->tShmemVideoMediaInfo.iRemainingSize = pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.dwBytesUsed;
				//20081013 Safety check
				if(pShmInfo->tShmemVideoMediaInfo.iRemainingSize <= 0)
				{
					printf("Warning!! Frame Size is incorrect after DWORD-to-INT conversion %d\n", pShmInfo->tShmemVideoMediaInfo.iRemainingSize);
					syslog(LOG_ERR, "Warning!! Frame Size is incorrect after DWORD-to-INT conversion %d\n", pShmInfo->tShmemVideoMediaInfo.iRemainingSize);
					//20130605 modified by Jimmy to support metadata event					
					SharedMem_ReleaseReadBuffer(pShmInfo->tShmemVideoMediaInfo.ahClientBuf[0]);
					return S_FAIL;
				}
			}
		}
		else if(pUBuffer->dwDataType == FOURCC_JPEG)
		{
			DWORD		dwJPEGFrameSize = 0;
			//pbyPointer will now point to start of JPEG frame (FFD8)
			//Userdata can be safely ignored as everything can be parsed in JPEG header
			pbyPointer = (BYTE *)pUBuffer + TUBufferSize + pUBuffer->dwUserDataSize;
			dwJPEGFrameSize = pUBuffer->dwSize- TUBufferSize - pUBuffer->dwUserDataSize;

			pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.dwBufSize = dwJPEGFrameSize;
			pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.tFrameType = MEDIADB_FRAME_INTRA;
			pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.dwSecond = pUBuffer->dwSec;
			pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.dwMilliSecond = (pUBuffer->dwUSec / 1000);
			pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.dwStreamType = mctJPEG;
			pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.pdwPacketSize[0] = 1;
			pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.dwOffset = 0;
			pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.dwIntelligentVideoLength = 0;
			pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.bChangeSetting = FALSE;
			pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.dwStreamIndex = pThis->tVideoSrcInfo[iIndex].iMediaIndex;
			pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.dwIsBoundary = pUBuffer->bIsBoundary;
			pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.wPacketCount = 0;
			pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.dwCurrentPosition = 0;
			//Parse JPEG frame
			if(JPEGBitStreamParse(&pShmInfo->tShmemVideoMediaInfo.tStreamBuffer, pbyPointer, dwJPEGFrameSize) != S_OK)
			{
				//20130605 modified by Jimmy to support metadata event
				SharedMem_ReleaseReadBuffer(pShmInfo->tShmemVideoMediaInfo.ahClientBuf[0]);
				return S_FAIL;
			}
			//Assign Remaining Size
			pShmInfo->tShmemVideoMediaInfo.iRemainingSize = pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.dwBytesUsed;
			if(pShmInfo->tShmemVideoMediaInfo.iRemainingSize <= 0)
			{
				printf("Warning!! Frame Size is incorrect after DWORD-to-INT conversion %d\n", pShmInfo->tShmemVideoMediaInfo.iRemainingSize);
				syslog(LOG_ERR, "Warning!! Frame Size is incorrect after DWORD-to-INT conversion %d\n", pShmInfo->tShmemVideoMediaInfo.iRemainingSize);
				//20130605 modified by Jimmy to support metadata event
				SharedMem_ReleaseReadBuffer(pShmInfo->tShmemVideoMediaInfo.ahClientBuf[0]);
				return S_FAIL;
			}
		}
		else if(pUBuffer->dwDataType == FOURCC_H264)
		{
			DWORD	dwOffset = 0, dwPacketInfoLength = 0;
			//Test TCP broken image
			/*BYTE	*pbyBitStreamStart = NULL;
			FILE *fp;
			int iWriteNum = 0;*/
			
			pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.dwOffset = 0;
			pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.dwIntelligentVideoLength = 0;
			//Parse the tag
			while(dwOffset < pUBuffer->dwUserDataSize)
			{
				if(usTag == TAG_H264_NALINFO)
				{
				    /*  _______________________________________________________________________________
					 *  | 0x0042 |  132   |   NAL_Count     |     NALBytes[16]    |    NALTypes[16]    |
					 *  |________|________|_________________|_____________________|_______(no use)_____|
					 *    2bytes   2bytes       4bytes         4bytes*16=64bytes     4bytes*16=64bytes  */
					DWORD	dwNALUCount = 0, dwNALULength = 0;
					BYTE	*pbyH264Info = pbyPointer + 4;

					dwPacketInfoLength = usLength;
					memcpy(&dwNALUCount, pbyH264Info, 4);
					pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.pdwPacketSize[0] = dwNALUCount;
					for( i = 1; i < (dwNALUCount + 1); i++)
					{
						pbyH264Info += 4;
						memcpy(&dwNALULength, pbyH264Info, 4);
						pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.pdwPacketSize[i] = dwNALULength;
						//printf("Parsed %u frame with %dst size %u\n", dwNALUCount, i, pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.pdwPacketSize[i]);
					}
				}
				else if(usTag == TAG_H264_EXTENINFO)
				{
					if(pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.dwIntelligentVideoLength == 0)	//No intelligent video
					{
//						printf("h264 extension = %u\n", pUBuffer->dwUserDataSize - 4 - dwPacketInfoLength - 4);
						pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.dwOffset = pUBuffer->dwUserDataSize - 4 - dwPacketInfoLength - 4;
					}
				}
				else if(usTag == TAG_MP4V_IVAEXTENINFO)
				{
					//If intelligent video is present, it should always come before TAG_MP4V_EXTENINFO
					pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.dwIntelligentVideoLength = usLength;
					pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.dwOffset  = pUBuffer->dwUserDataSize - 4 - dwPacketInfoLength - 4 - usLength - 4;
				}
				else
				{
					//Ignore the Flag
					printf("Extension Tag Type %u ignored!\n", usTag);
				}
				dwOffset = dwOffset + usLength + 4;
				pbyPointer = pbyPointer + usLength + 4;
				memcpy(&usTag, pbyPointer,2);
				memcpy(&usLength, pbyPointer+2,2);
			}

			//Assign the necessary parameter
			pbyPointer = (BYTE *)pUBuffer + TUBufferSize;
			//Test TCP broken image
			//pbyBitStreamStart = (BYTE *)pUBuffer + sizeof(TUBuffer) + pUBuffer->dwUserDataSize;
			
			//extesion length
			if(pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.dwOffset == 0)
			{	//Start of bitstream should be the H.264 stream (including 4 bytes packet length)
				pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.pbyBuffer = pbyPointer + 4 + dwPacketInfoLength;
			}
			else
			{	//Start of bitstream should be the extension data
				pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.pbyBuffer = pbyPointer + 4 + dwPacketInfoLength + 4;
			}
			pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.dwSecond = pUBuffer->dwSec;
			pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.dwMilliSecond = (pUBuffer->dwUSec / 1000);
			pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.dwStreamType = mctH264;
			pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.bChangeSetting = FALSE;
			pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.dwBytesUsed = pUBuffer->dwSize- TUBufferSize - pUBuffer->dwUserDataSize;
			pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.dwStreamIndex = pThis->tVideoSrcInfo[iIndex].iMediaIndex;
			//Test TCP broken image
			/*if ((fp = fopen("/mnt/ramdisk/test.h264", "a+")) == NULL )
			{
				printf("[%s] Open file /mnt/ramdisk/test.m4v fail\n", __FUNCTION__);
			}
			else
			{
				if(pUBuffer->bIsSync == TRUE)
				{
					pbyBitStreamStart[3]  = 0x01;
					pbyBitStreamStart[16] = 0x01;
					pbyBitStreamStart[21] = 0x00;
					pbyBitStreamStart[22] = 0x00;
					pbyBitStreamStart[23] = 0x00;
					pbyBitStreamStart[24] = 0x01;
				}
				else
				{
					pbyBitStreamStart[0]  = 0x00;
					pbyBitStreamStart[1]  = 0x00;
					pbyBitStreamStart[2]  = 0x00;
					pbyBitStreamStart[3]  = 0x01;
				}
				iWriteNum = fwrite(pbyBitStreamStart, sizeof(char), pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.dwBytesUsed, fp);
				if (iWriteNum != pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.dwBytesUsed)
				{
					printf("[%s] Write %d vs total bitstream size %d\n", __FUNCTION__, iWriteNum, pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.dwBytesUsed);
				}
			}
			fclose(fp);*/
			
			if(pUBuffer->bIsSync == TRUE)
			{
				pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.tFrameType = MEDIADB_FRAME_INTRA;
				//printf("[%s] I-frame, UBuffer Counter=%d\n", __FUNCTION__, pUBuffer->dwSeqNo);
				//20110309 Change SVC mode after iFrame
				if(pShmInfo->eSVCTempMode != eSVCInvalid)
				{
					printf("*****original SVC mode = %d, new SVC mode = %d***\n", pShmInfo->eSVCMode, pShmInfo->eSVCTempMode);
					pShmInfo->eSVCMode = pShmInfo->eSVCTempMode;
					pShmInfo->eSVCTempMode = eSVCInvalid;
				}
			}
			else
			{
				pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.tFrameType = MEDIADB_FRAME_PRED;
				//printf("[%s] P-frame, UBuffer Counter=%d\n", __FUNCTION__, pUBuffer->dwSeqNo);
			}
			pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.dwIsBoundary = pUBuffer->bIsBoundary;
			pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.wPacketCount = 0;
			pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.dwCurrentPosition = 0;
			pShmInfo->tShmemVideoMediaInfo.iRemainingSize = pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.pdwPacketSize[1] + 4;
			//H.264 Parameter initialize
			pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.tH264overRTPInfo.iNALUIndex = 1;

			if(pShmInfo->tShmemVideoMediaInfo.iRemainingSize <= 0)
			{
				printf("Warning!! Frame Size is incorrect after DWORD-to-INT conversion %d\n", pShmInfo->tShmemVideoMediaInfo.iRemainingSize);
				syslog(LOG_ERR, "Warning!! Frame Size is incorrect after DWORD-to-INT conversion %d\n", pShmInfo->tShmemVideoMediaInfo.iRemainingSize);
				//20130605 modified by Jimmy to support metadata event
				SharedMem_ReleaseReadBuffer(pShmInfo->tShmemVideoMediaInfo.ahClientBuf[0]);
				return S_FAIL;
			}
		}
        else if(pUBuffer->dwDataType == FOURCC_H265)
        {
            DWORD   dwOffset = 0, dwPacketInfoLength = 0;
            pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.dwOffset = 0;
            pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.dwIntelligentVideoLength = 0;
            //Parse the tag
            while(dwOffset < pUBuffer->dwUserDataSize)
            {
                if(usTag == TAG_H265_NALINFO)
                {
                    /*  _______________________________________________________________________________
                     *  | 0x0052 |  132   |   NAL_Count     |     NALBytes[16]    |    NALTypes[16]    |
                     *  |________|________|_________________|_____________________|_______(no use)_____|
                     *    2bytes   2bytes       4bytes         4bytes*16=64bytes     4bytes*16=64bytes  */
                    DWORD   dwNALUCount = 0, dwNALULength = 0;
                    BYTE    *pbyH265Info = pbyPointer + 4;

                    dwPacketInfoLength = usLength;
                    memcpy(&dwNALUCount, pbyH265Info, 4);
                    pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.pdwPacketSize[0] = dwNALUCount;
                    for( i = 1; i < (dwNALUCount + 1); i++)
                    {
                        pbyH265Info += 4;
                        memcpy(&dwNALULength, pbyH265Info, 4);
                        pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.pdwPacketSize[i] = dwNALULength;
                        //printf("Parsed %u frame with %dst size %u\n", dwNALUCount, i, pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.pdwPacketSize[i]);
                    }
                }
                else if(usTag == TAG_H265_EXTENINFO)
                {
                    if(pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.dwIntelligentVideoLength == 0)  //No intelligent video
                    {
                        pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.dwOffset = pUBuffer->dwUserDataSize - 4 - dwPacketInfoLength - 4;
                    }
                }
                else if(usTag == TAG_MP4V_IVAEXTENINFO)
                {
                    //If intelligent video is present, it should always come before TAG_MP4V_EXTENINFO
                    pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.dwIntelligentVideoLength = usLength;
                    pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.dwOffset  = pUBuffer->dwUserDataSize - 4 - dwPacketInfoLength - 4 - usLength - 4;
                }
                else
                {
                    //Ignore the Flag
                    printf("Extension Tag Type %u ignored!\n", usTag);
                }
                dwOffset = dwOffset + usLength + 4;
                pbyPointer = pbyPointer + usLength + 4;
                memcpy(&usTag, pbyPointer,2);
                memcpy(&usLength, pbyPointer+2,2);
            }

            //Assign the necessary parameter
            pbyPointer = (BYTE *)pUBuffer + TUBufferSize;
            
            //extesion length
            if(pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.dwOffset == 0)
            {   //Start of bitstream should be the H.265 stream (including 4 bytes packet length)
                pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.pbyBuffer = pbyPointer + 4 + dwPacketInfoLength;
            }
            else
            {   //Start of bitstream should be the extension data
                pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.pbyBuffer = pbyPointer + 4 + dwPacketInfoLength + 4;
            }
            pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.dwSecond = pUBuffer->dwSec;
            pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.dwMilliSecond = (pUBuffer->dwUSec / 1000);
            pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.dwStreamType = mctH265;
            pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.bChangeSetting = FALSE;
            pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.dwBytesUsed = pUBuffer->dwSize- TUBufferSize - pUBuffer->dwUserDataSize;
            pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.dwStreamIndex = pThis->tVideoSrcInfo[iIndex].iMediaIndex;
            
            if(pUBuffer->bIsSync == TRUE)
            {
                pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.tFrameType = MEDIADB_FRAME_INTRA;
                //printf("[%s] I-frame, UBuffer Counter=%d\n", __FUNCTION__, pUBuffer->dwSeqNo);
                //20110309 Change SVC mode after iFrame
                if(pShmInfo->eSVCTempMode != eSVCInvalid)
                {
                    printf("*****original SVC mode = %d, new SVC mode = %d***\n", pShmInfo->eSVCMode, pShmInfo->eSVCTempMode);
                    pShmInfo->eSVCMode = pShmInfo->eSVCTempMode;
                    pShmInfo->eSVCTempMode = eSVCInvalid;
                }
            }
            else
            {
                pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.tFrameType = MEDIADB_FRAME_PRED;
                //printf("[%s] P-frame, UBuffer Counter=%d\n", __FUNCTION__, pUBuffer->dwSeqNo);
            }
            pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.dwIsBoundary = pUBuffer->bIsBoundary;
            pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.wPacketCount = 0;
            pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.dwCurrentPosition = 0;
            pShmInfo->tShmemVideoMediaInfo.iRemainingSize = pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.pdwPacketSize[1] + 4;
            //H.265 Parameter initialize
            pShmInfo->tShmemVideoMediaInfo.tStreamBuffer.tH265overRTPInfo.iNALUIndex = 1;

            if(pShmInfo->tShmemVideoMediaInfo.iRemainingSize <= 0)
            {
                printf("Warning!! Frame Size is incorrect after DWORD-to-INT conversion %d\n", pShmInfo->tShmemVideoMediaInfo.iRemainingSize);
                syslog(LOG_ERR, "Warning!! Frame Size is incorrect after DWORD-to-INT conversion %d\n", pShmInfo->tShmemVideoMediaInfo.iRemainingSize);
                //20130605 modified by Jimmy to support metadata event
                SharedMem_ReleaseReadBuffer(pShmInfo->tShmemVideoMediaInfo.ahClientBuf[0]);
                return S_FAIL;
            }
        }

		return S_OK;
	}
	else if(dwCallbackType == MEDIA_CALLBACK_SHM_CHECKBUFFER)
	{
		DWORD	dwDuration = 0;
		HANDLE	hSharedMem = (HANDLE)pvCallbackData;

		if(SharedMem_CheckTimeshift(hSharedMem, &dwDuration) != S_OK)
		{
			printf("Time-shift not supported in this stream!\n");
			return 0xffffffff;
		}
		return dwDuration;
	}
    else // MEDIA_CALLBACK_FLUSH_BUFFER
    {
    	return S_OK;
    }
	
	return S_OK;
}

SCODE JPEGBitStreamParse(TBitstreamBuffer *pBitstreamBuf, BYTE *pJPEGFrame, DWORD dwFrameSize)
{
	DWORD				dwOffset = 0;
	BYTE				*pbyCurrent = NULL;
	TJPEGoverRTPInfo	*ptJPEGInfo;

	//NULL Check
	if(pBitstreamBuf == NULL || pJPEGFrame == NULL || dwFrameSize == 0)
	{
		return S_FAIL;
	}
	memset(&pBitstreamBuf->tJPEGoverRTPInfo, 0, sizeof(TJPEGoverRTPInfo));
	ptJPEGInfo = &pBitstreamBuf->tJPEGoverRTPInfo;

	//Check for JPEG SOI
	if(*pJPEGFrame != 0xff || *(pJPEGFrame + 1) != 0xd8)
	{
		return S_FAIL;
	}
	dwOffset += 2;

	while(dwOffset < dwFrameSize)
	{
		WORD	wValueLength = 0;

		pbyCurrent = pJPEGFrame + dwOffset;
		wValueLength = 0;

		if(*pbyCurrent != 0xff)
		{
			printf("Incorrect JPEG Marker\n");
			return S_FAIL;
		}
		//Parse according to different JPEG Marker
		//printf("JPEG Marker %02x %02x %02x %02x\n", *(pbyCurrent + 0), *(pbyCurrent + 1), *(pbyCurrent + 2), *(pbyCurrent + 3));
		memcpy(&wValueLength, pbyCurrent + 2, 2);
		wValueLength = htons(wValueLength);

		if(*(pbyCurrent + 1) == 0xe2)			//Application Data that is to be used as RTP extension
		{
			//20120618 danny, modified JPEG RTP header extension according to ONVIF spec and RFC2435
			ptJPEGInfo->pbyApplicationData = pbyCurrent;//pbyCurrent + 4;
			ptJPEGInfo->dwApplicationDataSize = wValueLength + 2;//wValueLength - 2;
			dwOffset = dwOffset + wValueLength + 2;

			//printf("Parsed Application data %x %x %x %x, %u bytes\n", *ptJPEGInfo->pbyApplicationData, *(ptJPEGInfo->pbyApplicationData+1), *(ptJPEGInfo->pbyApplicationData+2), *(ptJPEGInfo->pbyApplicationData+3), ptJPEGInfo->dwApplicationDataSize);
		}
		else if(*(pbyCurrent + 1) == 0xe0)		//JFIF information that should be ignored
		{
			dwOffset = dwOffset + wValueLength + 2;
		}	
		else if(*(pbyCurrent + 1) == 0xe1)		//EXIF information that should be ignored
		{
			dwOffset = dwOffset + wValueLength + 2;
		}
		else if(*(pbyCurrent + 1) == 0xdb && wValueLength == 67) //Single Quantization Table
		{
			int i;
			int iTableOffset = (*(pbyCurrent + 4) == 0) ? 0 : 64;
			for(i = 0; i < 64; i++)
			{
				memcpy(ptJPEGInfo->acQuantizationTable + iTableOffset + i, pbyCurrent + i + 5, 1);
			}
			dwOffset += wValueLength + 2;
		}
		else if(*(pbyCurrent + 1) == 0xdb)		//Quantization Table
		{
			int		i = 0;
            //20150408 modified by Charles for DQT Length Check
            //NOTE:JPEG over RTP should carry two Quantization Table
			if(wValueLength != 132 && wValueLength != 197 && wValueLength != 262) 
			{
				printf("Error quantization table length!\n");
				return S_FAIL;
			}
			for(i = 0; i < 64; i++)
			{
				memcpy(ptJPEGInfo->acQuantizationTable + i, pbyCurrent + i + 5, 1);
			}
			for(i = 0; i < 64; i++)
			{
				memcpy(ptJPEGInfo->acQuantizationTable + i + 64, pbyCurrent + i + 5 + 64 + 1, 1);
			}
			dwOffset = dwOffset + wValueLength + 2;
		}
		else if(*(pbyCurrent + 1) == 0xc0)		//BaseLine DCT to determine pixels and ratios
		{
			BYTE	bySamplingFactor = 0, byVerticalSamplingFactor = 0;
			WORD	wHeight = 0, wWidth = 0;

			//Copy the vertical & horizontal length
			memcpy(&wHeight, pbyCurrent + 5, 2);
			ptJPEGInfo->wHeight = htons(wHeight);
			memcpy(&wWidth, pbyCurrent + 7, 2);
			ptJPEGInfo->wWidth = htons(wWidth);
            //20150408 added by Charles for ONVIF JPEG Extension
            if(ptJPEGInfo->wHeight > 2040 || ptJPEGInfo->wWidth > 2040)
            {
                ptJPEGInfo->bEnableOnvifJpegRTPExt = TRUE;
            }
            else
            {
                ptJPEGInfo->bEnableOnvifJpegRTPExt = FALSE;
            }
			//Check the first component sampling factor
			memcpy(&bySamplingFactor, pbyCurrent + 11, 1);
			byVerticalSamplingFactor = (bySamplingFactor & 0xf);
			if(byVerticalSamplingFactor == 2)
			{
				//4:2:0 format
				ptJPEGInfo->byType = 1;
			}
			else if(byVerticalSamplingFactor == 1)
			{
				//4:2:2 format
				ptJPEGInfo->byType = 0;
			}
			else
			{
				printf("incorrect vertical sampling factor %d Sampling factor %d\n", byVerticalSamplingFactor, bySamplingFactor);
				return S_FAIL;
			}
			//20120618 danny, modified JPEG RTP header extension according to ONVIF spec and RFC2435
			ptJPEGInfo->pbyStartOfFrameBDCT = pbyCurrent;
			ptJPEGInfo->dwStartOfFrameBDCTSize = wValueLength + 2;
			dwOffset = dwOffset + wValueLength + 2;
			//printf("JPEG with type %d Width %u height %u\n", ptJPEGInfo->byType, ptJPEGInfo->wWidth, ptJPEGInfo->wHeight);
		}
		else if(*(pbyCurrent + 1) == 0xc4)		//Define Huffman Table, skipped
		{
			dwOffset = dwOffset + wValueLength + 2;
		}
		else if(*(pbyCurrent + 1) == 0xda)		//Start of scan marker
		{
			dwOffset = dwOffset + wValueLength + 2;
			pBitstreamBuf->pbyBuffer = pbyCurrent + wValueLength + 2;
			pBitstreamBuf->dwBytesUsed = dwFrameSize - dwOffset;
			//According to RFC 2435 the entropy data cannot exceed 2^24 bytes
			if(pBitstreamBuf->dwBytesUsed > 0xffffff)
			{
				printf("Entropy data exceed 2^24 maximum size!\n");
				return S_FAIL;
			}
			//printf("Entropy data size %u calculated.\n", pBitstreamBuf->dwBytesUsed);
			break;
		}
		//20101210 Add by danny For support DRI header
		else if(*(pbyCurrent + 1) == 0xdd)		//DRI marker segment
		{
			WORD	wDRI = 0;
			dwOffset = dwOffset + wValueLength + 2;
			
			memcpy(&wDRI, pbyCurrent + 4, 2);
			ptJPEGInfo->dwDRI = htons(wDRI);
		}
		else if(*(pbyCurrent + 1) == 0xff) //we should skip 0xFF, it's not a tag
		{
			dwOffset++;
		}
		else
		{
			//unknown header, let's skip it
			dwOffset = dwOffset + wValueLength + 2;
		}
	}
	//Confirm that most parameters are acquired
	if(pBitstreamBuf->pbyBuffer == NULL || ptJPEGInfo->wHeight == 0 || ptJPEGInfo->wWidth == 0)
	{
		return S_FAIL;
	}
	//Make sure that end of file is Indeed FFD9
	if(*(pBitstreamBuf->pbyBuffer + pBitstreamBuf->dwBytesUsed -2) != 0xff ||
	   *(pBitstreamBuf->pbyBuffer + pBitstreamBuf->dwBytesUsed -1) != 0xd9)
	{
		printf("JPEG EOI incorrect!\n");
		return S_FAIL;
	}

	return S_OK;
}

SCODE H264ExtractNaluInfo(BYTE *pbyVideUBuffer, TRTSPSTREAMING_VIDENCODING_PARAM *pRTSPVideoParam)
{
	int		i = 0, iSPSLength = -1, iPPSLength = -1;
	BYTE	*pbyNalu = NULL, *pbySPS = NULL, *pbyPPS = NULL;
	char	acSpropTemp[RTSPSTREAMING_MAX_SPROP_PARAM];

	pbyNalu	= pbyVideUBuffer + sizeof(TUBufferConfH264);

	//Parse NALU for Sprop-parameter-set
	for(i = 0; i< 2; i++)
	{
		unsigned short	usNALULength = 0;
		BYTE			byNALUOctet = 0;
		BYTE			byNALUType = 0;
		//printf("four octet %02x %02x %02x %02x\n", pbyNalu[0], 	 pbyNalu[1], pbyNalu[2], pbyNalu[3]);						
		memcpy(&usNALULength, pbyNalu, 2);
		memcpy(&byNALUOctet, pbyNalu + 2, 1);
		byNALUType = (byNALUOctet & 0x1f);
		usNALULength = ntohs(usNALULength);
		printf("Parsed NALU type %u Length %u\n", byNALUType, usNALULength);

		if(byNALUType == 7)	//Sequence Parameter Set
		{
			pbySPS = pbyNalu + 2;
			iSPSLength = usNALULength;
		}
		else if(byNALUType == 8) //Picture Parameter Set
		{
			pbyPPS = pbyNalu + 2;
			iPPSLength = usNALULength;
		}

		pbyNalu += (usNALULength + 2);
	}

	if(iSPSLength >0 && iPPSLength > 0 && pbySPS != NULL && pbyPPS != NULL)
	{
		//Use SPS & PPS to construct Sprop-parameter-set
		memset(pRTSPVideoParam->acSpropParamSet, 0, sizeof(pRTSPVideoParam->acSpropParamSet));
		//SPS 
		memset(acSpropTemp, 0, sizeof(acSpropTemp));
		EncryptionUtl_Base64_EncodeData((char *)pbySPS, (unsigned short)iSPSLength, acSpropTemp, sizeof(acSpropTemp));
		snprintf(pRTSPVideoParam->acSpropParamSet, sizeof(pRTSPVideoParam->acSpropParamSet) - 1, "%s,", acSpropTemp);
		//PPS
		memset(acSpropTemp, 0, sizeof(acSpropTemp));
		EncryptionUtl_Base64_EncodeData((char *)pbyPPS, (unsigned short)iPPSLength, acSpropTemp, sizeof(acSpropTemp));
		snprintf(pRTSPVideoParam->acSpropParamSet + strlen(pRTSPVideoParam->acSpropParamSet), sizeof(pRTSPVideoParam->acSpropParamSet) - strlen(pRTSPVideoParam->acSpropParamSet) - 1, "%s", acSpropTemp);
	}
	else
	{
		return S_FAIL;
	}

	return S_OK;
}
//20150113 added by Charles for H.265
void H265ExtractNaluInfo(BYTE *pbyVideUBuffer, TRTSPSTREAMING_VIDENCODING_PARAM *pRTSPVideoParam)
{
    int i, j;
    BYTE	*pbyNALU = NULL;
    char    *pcSpropParam = NULL;
    BYTE    byArraysNum = 0;
    unsigned short  usNALUNum = 0;
    unsigned short	usNALULength = 0;
    BYTE            byNALUOctet = 0;
    EH265NALUnitType    eH265NALUnitType = 0;
    char	        acSpropTemp[RTSPSTREAMING_MAX_SPROP_PARAM];
    BYTE *pbyDecSpecInfo = ((TUBufferConfH265 *)pbyVideUBuffer)->abyDecSpecInfo + sizeof(TH265DecConfRec);

    memcpy(&byArraysNum, pbyDecSpecInfo, 1);
    pbyDecSpecInfo += 1;
    
    for(i = 0; i < byArraysNum; i++)
    {
        memcpy(&byNALUOctet, pbyDecSpecInfo, 1);
        eH265NALUnitType = (byNALUOctet & 0x3f);
        memcpy(&usNALUNum, pbyDecSpecInfo + 1, 2);
        usNALUNum = ntohs(usNALUNum);
        pbyDecSpecInfo += 3;
        
        if(eH265NALUnitType == NAL_VPS)
        {
            pcSpropParam = pRTSPVideoParam->acSpropVPSParam;
        }
        else if(eH265NALUnitType == NAL_SPS)
        {
            pcSpropParam = pRTSPVideoParam->acSpropSPSParam;
        }
        else if(eH265NALUnitType == NAL_PPS)
        {
            pcSpropParam = pRTSPVideoParam->acSpropPPSParam;
        }
        
        for(j = 0; j < usNALUNum; j++)
        {
            memcpy(&usNALULength, pbyDecSpecInfo, 2);
            usNALULength = ntohs(usNALULength);
            pbyNALU = pbyDecSpecInfo + 2;
            
            memset(acSpropTemp, 0, sizeof(acSpropTemp));
	        EncryptionUtl_Base64_EncodeData((char *)pbyNALU, (unsigned short)usNALULength, acSpropTemp, sizeof(acSpropTemp));
            if(j == (usNALUNum - 1))
            {
                snprintf(pcSpropParam, RTSPSTREAMING_MAX_SPROP_PARAM, "%s", acSpropTemp);
            }
            else
            {
                snprintf(pcSpropParam, RTSPSTREAMING_MAX_SPROP_PARAM, "%s ,", acSpropTemp);
            }
           
            pbyDecSpecInfo += (2 + usNALULength);
            
        }
    }
}

#endif //_SHARED_MEM
