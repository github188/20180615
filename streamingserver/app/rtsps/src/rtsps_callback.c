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
 * rtsps_callback.c
 *
 * \brief
 * Callback functions for rtsp streaming server.
 *
 * \date
 * 2006/05/11
 *
 * \author
 * Rey Cheng
 *
 *
 *******************************************************************************
 */
#include <string.h>
#include <signal.h>
#include <sys/file.h>

#include "rtsprtpcommon.h"
#include "rtsps_callback.h"
#include "encrypt_md5.h"
#include "account_mgr_app.h"
//ShengFu 2006/07/12 remove Ubuffer thread
#include "message.h"

/*! The number of bits on which the AU-size field is encoded in the AU-header.
    This value MUST consist with the SizeLength field in SDP. */
#define RTPM4APACK_AU_SIZE_LENGTH	13
/*! The number of bits on which the AU-index field is encoded in the first
	AU-header. AND the number of bits on which the AU-Index-delta field is 
	encoded in any non-first AU-header.
    This value MUST consist with the IndexLength and IndexDeltaLength 
	fields in SDP. */
#define RTPM4APACK_AU_INDEX_LENGTH	3
#define MAX_AMR_FRAMES_PER_UBUFFER	16
#define	SHMEM_ASAP_THRESHOLD		2

const BYTE g_abyFT2Size[16] = {12, 13, 15, 17, 19, 20, 26, 31, 5, 0, 0, 0, 0, 0, 0, 0};

#define UserName "root"
#define Password ""

#define AUDIO_TAG_LEN_BYTES	4

//extern FILE* pTempFile;

#ifdef _G711_AUDIOIN
SCODE G711BitstreamPack(TBitstreamBuffer *pBitstreamBuf, TUBuffer *pUBuffer, int iIndex, int TUBufferSize)
{
	BYTE *pbyPointer;

	int iOffset = 0;
	DWORD dwAudioTagLen = 0;
	
	if( pUBuffer->dwUserDataSize == 0 )
	{
		iOffset = 0 ;
		dwAudioTagLen = 0;
	}
	else
	{
		iOffset = pBitstreamBuf->dwOffset = pUBuffer->dwUserDataSize - AUDIO_TAG_LEN_BYTES;
		dwAudioTagLen = AUDIO_TAG_LEN_BYTES ;
	}
	//20081215 Buffer check
	if (pUBuffer->dwSize > RTSPSTREAMING_AUDIOBUFFER_SIZE)
	{
		printf("Warning!! Audio data size exceeding upper limit!\n");
		pBitstreamBuf->dwBytesUsed = 0;
		return S_FAIL;
	}

	pBitstreamBuf->dwBytesUsed = pUBuffer->dwSize - TUBufferSize - dwAudioTagLen;

	pbyPointer = (BYTE *) pUBuffer;
	pbyPointer = pbyPointer + TUBufferSize + dwAudioTagLen;
	
	memcpy((void*)pBitstreamBuf->pbyBuffer, (void*) pbyPointer, pBitstreamBuf->dwBytesUsed);

	pBitstreamBuf->dwBytesUsed = pUBuffer->dwSize - TUBufferSize - pUBuffer->dwUserDataSize;

	pBitstreamBuf->dwStreamIndex = iIndex;
	// set time stamp and stream type
	pBitstreamBuf->dwSecond = pUBuffer->dwSec;
	pBitstreamBuf->dwMilliSecond = (pUBuffer->dwUSec / 1000);
	pBitstreamBuf->dwStreamType = mctG711U; // Useless?
	pBitstreamBuf->bChangeSetting = FALSE;
	pBitstreamBuf->tFrameType = MEDIADB_FRAME_INTRA;
	pBitstreamBuf->dwIsBoundary = pUBuffer->bIsBoundary;
	pBitstreamBuf->pdwPacketSize[0] = 1;
	pBitstreamBuf->pdwPacketSize[1] = pUBuffer->dwSize - TUBufferSize - pUBuffer->dwUserDataSize;
#ifdef _SHARED_MEM
	pBitstreamBuf->wPacketCount = 0;
	pBitstreamBuf->dwCurrentPosition = 0;
#endif
	return S_OK;
}
#endif

#ifdef _G726_AUDIOIN
SCODE G726BitstreamPack(TBitstreamBuffer *pBitstreamBuf, TUBuffer *pUBuffer, int iIndex, int TUBufferSize)
{
	BYTE *pbyPointer;

	int iOffset = 0;
	DWORD dwAudioTagLen = 0;
	
	if( pUBuffer->dwUserDataSize == 0 )
	{
		iOffset = 0 ;
		dwAudioTagLen = 0;
	}
	else
	{
		iOffset = pBitstreamBuf->dwOffset = pUBuffer->dwUserDataSize - AUDIO_TAG_LEN_BYTES;
		dwAudioTagLen = AUDIO_TAG_LEN_BYTES ;
	}
	//20081215 Buffer check
	if (pUBuffer->dwSize > RTSPSTREAMING_AUDIOBUFFER_SIZE)
	{
		printf("Warning!! Audio data size exceeding upper limit!\n");
		pBitstreamBuf->dwBytesUsed = 0;
		return S_FAIL;
	}

	pBitstreamBuf->dwBytesUsed = pUBuffer->dwSize - TUBufferSize - dwAudioTagLen;

	pbyPointer = (BYTE *) pUBuffer;
	pbyPointer = pbyPointer + TUBufferSize + dwAudioTagLen;
	
	memcpy((void*)pBitstreamBuf->pbyBuffer, (void*) pbyPointer, pBitstreamBuf->dwBytesUsed);

	pBitstreamBuf->dwBytesUsed = pUBuffer->dwSize - TUBufferSize - pUBuffer->dwUserDataSize;

	pBitstreamBuf->dwStreamIndex = iIndex;
	// set time stamp and stream type
	pBitstreamBuf->dwSecond = pUBuffer->dwSec;
	pBitstreamBuf->dwMilliSecond = (pUBuffer->dwUSec / 1000);
	pBitstreamBuf->dwStreamType = mctG726; // Useless?
	pBitstreamBuf->bChangeSetting = FALSE;
	pBitstreamBuf->tFrameType = MEDIADB_FRAME_INTRA;
	pBitstreamBuf->dwIsBoundary = pUBuffer->bIsBoundary;
	pBitstreamBuf->pdwPacketSize[0] = 1;
	pBitstreamBuf->pdwPacketSize[1] = pUBuffer->dwSize - TUBufferSize - pUBuffer->dwUserDataSize;
#ifdef _SHARED_MEM
	pBitstreamBuf->wPacketCount = 0;
	pBitstreamBuf->dwCurrentPosition = 0;
#endif
	return S_OK;
}
#endif

int M4ABitstreamPack(TBitstreamBuffer *pBitstreamBuf, TUBuffer *pUBuffer, int iIndex, int TUBufferSize)
{
	DWORD dwAUSize;
	BYTE *pbyPointer;

	int iOffset = 0;
	//pBitstreamBuf->dwOffset = 0;
	
	if( pUBuffer->dwUserDataSize == 0 )
	{
		printf("Warning!! User data size of audio UBuffer can NOT be zero!!\n");
		pBitstreamBuf->dwBytesUsed = 0;
		return S_FAIL;
	}
	//20081215 Buffer check
	if (pUBuffer->dwSize > RTSPSTREAMING_AUDIOBUFFER_SIZE)
	{
		printf("Warning!! Audio data size exceeding upper limit!\n");
		pBitstreamBuf->dwBytesUsed = 0;
		return S_FAIL;
	}
	
	iOffset = pBitstreamBuf->dwOffset = pUBuffer->dwUserDataSize - AUDIO_TAG_LEN_BYTES;
	
	pbyPointer = (BYTE *)pUBuffer;
	pbyPointer = pbyPointer + TUBufferSize + pUBuffer->dwUserDataSize;
	dwAUSize = (pUBuffer->dwSize - TUBufferSize - pUBuffer->dwUserDataSize) << 3;
	pBitstreamBuf->pbyBuffer[iOffset] = (RTPM4APACK_AU_SIZE_LENGTH + RTPM4APACK_AU_INDEX_LENGTH) >> 8;
	pBitstreamBuf->pbyBuffer[iOffset+1] = (RTPM4APACK_AU_SIZE_LENGTH + RTPM4APACK_AU_INDEX_LENGTH) & 0xFF;
	pBitstreamBuf->pbyBuffer[iOffset+2] = dwAUSize >> 8;
	pBitstreamBuf->pbyBuffer[iOffset+3] = dwAUSize & 0xFF;
	memcpy((pBitstreamBuf->pbyBuffer + iOffset + 4), pbyPointer, pUBuffer->dwSize - TUBufferSize - pUBuffer->dwUserDataSize);	
    memcpy((void*)pBitstreamBuf->pbyBuffer,(char*)pUBuffer + TUBufferSize + AUDIO_TAG_LEN_BYTES,pUBuffer->dwUserDataSize - AUDIO_TAG_LEN_BYTES);
		
	pBitstreamBuf->dwBytesUsed = 4 + pUBuffer->dwSize - TUBufferSize - pUBuffer->dwUserDataSize;

	pBitstreamBuf->dwStreamIndex = iIndex;
	// set time stamp and stream type
	pBitstreamBuf->dwSecond = pUBuffer->dwSec;
	pBitstreamBuf->dwMilliSecond = (pUBuffer->dwUSec / 1000);
	pBitstreamBuf->dwStreamType = mctAAC;
	pBitstreamBuf->bChangeSetting = FALSE;
	pBitstreamBuf->tFrameType = MEDIADB_FRAME_INTRA;
	pBitstreamBuf->dwIsBoundary = pUBuffer->bIsBoundary;
	pBitstreamBuf->pdwPacketSize[0] = 1;
	pBitstreamBuf->pdwPacketSize[1] = 4 + pUBuffer->dwSize- TUBufferSize - pUBuffer->dwUserDataSize;
#ifdef _SHARED_MEM
	pBitstreamBuf->wPacketCount = 0;
	pBitstreamBuf->dwCurrentPosition = 0;
#endif

	return S_OK;
}

SCODE AMRBitstreamPack(TBitstreamBuffer *pBitstreamBuf, TUBuffer *pUBuffer, int iIndex, int iFramesPerUBuffer, int TUBufferSize)
{
	int i;
	DWORD dwSize;
	BYTE *pbyPointer;
	BYTE *pbyBitStream;
	BYTE abyToC[MAX_AMR_FRAMES_PER_UBUFFER];
	int iOffset = 0;
	
	if( pUBuffer->dwUserDataSize == 0 )
	{
		printf("Warning!! User data size of audio UBuffer can NOT be zero!!\n");
		pBitstreamBuf->dwBytesUsed = 0;
		return S_FAIL;
	}
	//20081215 Buffer check
	if (pUBuffer->dwSize > RTSPSTREAMING_AUDIOBUFFER_SIZE)
	{
		printf("Warning!! Audio data size exceeding upper limit!\n");
		pBitstreamBuf->dwBytesUsed = 0;
		return S_FAIL;
	}

	iOffset = pBitstreamBuf->dwOffset = pUBuffer->dwUserDataSize - AUDIO_TAG_LEN_BYTES;
	
	//pbyPointer = (BYTE *)pUBuffer;
	pbyPointer = (BYTE *)pUBuffer + TUBufferSize + pUBuffer->dwUserDataSize;
	pbyBitStream = &pBitstreamBuf->pbyBuffer[iOffset];
	*pbyBitStream = 0xf0; //ShengFu modified 2006/08/02 0x60;
	pbyBitStream += (1 + iFramesPerUBuffer);

	for (i=0; i<(iFramesPerUBuffer-1); i++)
	{
		abyToC[i] = ( 0x80 | *pbyPointer);
		dwSize = g_abyFT2Size[*pbyPointer >> 3];
		pbyPointer++;
		memcpy(pbyBitStream, pbyPointer, dwSize);
		pbyBitStream += dwSize;
		pbyPointer += dwSize;
	}
	abyToC[iFramesPerUBuffer-1] = *pbyPointer;
	dwSize = g_abyFT2Size[*pbyPointer >> 3];
	pbyPointer++;
	memcpy(pbyBitStream, pbyPointer, dwSize);
	// copy ToC entries
	pbyBitStream = &pBitstreamBuf->pbyBuffer[iOffset];
	pbyBitStream++;
	memcpy(pbyBitStream, abyToC, iFramesPerUBuffer);

    memcpy((void*)pBitstreamBuf->pbyBuffer,(BYTE *)pUBuffer + TUBufferSize + AUDIO_TAG_LEN_BYTES,pUBuffer->dwUserDataSize - AUDIO_TAG_LEN_BYTES);
    
   /* printf("Size: %d %d %d\n",pUBuffer->dwSize,pUBuffer->dwSize - sizeof(TUBuffer) - pUBuffer->dwUserDataSize,pUBuffer->dwUserDataSize);
    
    for(i=0;i<32;i++)
        printf("%02x",pBitstreamBuf->pbyBuffer[i]);
    
    printf("\n");*/
    
	pBitstreamBuf->dwBytesUsed = 1 + pUBuffer->dwSize - TUBufferSize - pUBuffer->dwUserDataSize;
	pBitstreamBuf->dwStreamIndex = iIndex;
	// set time stamp and stream type
	pBitstreamBuf->dwSecond = pUBuffer->dwSec;
	pBitstreamBuf->dwMilliSecond = (pUBuffer->dwUSec / 1000);
	pBitstreamBuf->dwStreamType = mctGAMR;
	pBitstreamBuf->bChangeSetting = FALSE;
	pBitstreamBuf->tFrameType = MEDIADB_FRAME_INTRA;
	pBitstreamBuf->dwIsBoundary = pUBuffer->bIsBoundary;
	pBitstreamBuf->pdwPacketSize[0] = 1;
	pBitstreamBuf->pdwPacketSize[1] = 1 + pUBuffer->dwSize- TUBufferSize - pUBuffer->dwUserDataSize;
#ifdef _SHARED_MEM
	pBitstreamBuf->wPacketCount = 0;
	pBitstreamBuf->dwCurrentPosition = 0;
#endif
	return S_OK;
}

SCODE StreamSvrWriteFile(char *pzFilePathName, char* pWriteBuff, int iWriteLength)
{
    FILE *fptr;
    
    if( (fptr = fopen(pzFilePathName, "w")) != NULL )
    {
        if( (flock(fileno(fptr), LOCK_EX)) == 0)    	                
        {
            fwrite(pWriteBuff,sizeof(char),iWriteLength,fptr);                                                        
            flock(fileno(fptr), LOCK_UN);
        }
        else
        {
            printf("flock file %s failed %d\r\n", pzFilePathName, errno);
			syslog(LOG_ERR, "[%s]flock file %s failed %d\n", __FUNCTION__, pzFilePathName, errno);
			return S_FAIL;
        } 
        fclose(fptr);        	                
    }
    else
    {
		printf("open file %s failed %d\r\n", pzFilePathName, errno);
		syslog(LOG_ERR, "[%s]open file %s failed %d\n", __FUNCTION__, pzFilePathName, errno);
		return S_FAIL;
    }
	
    return S_OK;
}

SCODE StreamSvrAudioInCallback(DWORD dwInstance, DWORD dwCallbackType, void *pvCallbackData)
{
	static int iIndex = -1;
	TSTREAMSERVERINFO *pThis = (TSTREAMSERVERINFO *)dwInstance;
	int         i,iSDPSize=0;
	char        pcSDPBuffer[RTSPSTREAMING_SDP_MAXSIZE];
    int         TUBufferSize = sizeof(TUBuffer);

    if(dwCallbackType == MEDIA_CALLBACK_REQUEST_BUFFER)
    {
		TUBuffer *pUBuffer;
		iIndex++;
		//iIndex = iIndex%pThis->dwStreamNumber;
        //iIndex = iIndex%AUDIO_TRACK_NUMBER;
		
		if( pThis->iGotAudioFlag == FALSE)
		{
		    usleep(10);
		    return S_FAIL;   
		}

		/*if (iIndex == 0)
		{
			sem_wait(&pThis->semVideDataReady);
		}
	
		if (pThis->bVideUBufEmpty[iIndex] == TRUE)
		{
			if (iIndex+1 == pThis->dwStreamNumber)
			{
				sem_post(&pThis->semWriteVideData);
			}
			return S_FAIL;
		}
		if( pThis->tAudioSrcInfo[iIndex].iFdSock < 0 )
		{
		    return S_FAIL;
		} */  

        //printf("audio unix socket %d %d\n",iIndex,pThis->tAudioSrcInfo[iIndex].iFdSock);
		//if(GetUBuffer(pThis->tAudioSrcInfo[iIndex].iFdSock, pThis) == S_OK)
		if( GetAudioUBuffer(pThis,&iIndex) != S_OK )
		{            		    
		    return S_FAIL;
        }      
                
		// filled channel 
		pUBuffer = (TUBuffer *)pThis->pbyAudiUBuffer;
		
        /*printf("Size: %d %d %d\n",pUBuffer->dwSize,pUBuffer->dwSize - sizeof(TUBuffer) - pUBuffer->dwUserDataSize,pUBuffer->dwUserDataSize);
        
        for(i=0;i<32;i++)
            printf("%02x",pThis->pbyAudiUBuffer[sizeof(TUBuffer)+i]);
            
        printf("\n");*/
 		
		if (pUBuffer->dwDataType == FOURCC_CONF)
		{

			TRTSPSTREAMING_AUDENCODING_PARAM stRTSPAudioParam;
			memset(&stRTSPAudioParam, 0, sizeof(TRTSPSTREAMING_AUDENCODING_PARAM));
			// TODO : bitrate ???
			stRTSPAudioParam.iBitRate = 12800;
			
			stRTSPAudioParam.iPacketTime = 200;
			stRTSPAudioParam.iOctetAlign = 1;
			stRTSPAudioParam.iAMRcrc=0;
			stRTSPAudioParam.iRobustSorting=0;
			stRTSPAudioParam.bIsBigEndian = FALSE;
			//CID:1084 CHECKER: STRING_OVERFLOW
            strncpy(stRTSPAudioParam.acTrackName,pThis->tAudioSrcInfo[iIndex].acTrackName, sizeof(stRTSPAudioParam.acTrackName));
			stRTSPAudioParam.acTrackName[sizeof(stRTSPAudioParam.acTrackName) - 1] = 0;

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
    	            //CID:187, CHECKER: NEGATIVE_RETURNS
    	            if( pThis->tStreamInfo[i].iEnable == TRUE && iSDPSize > 0)
    	            	StreamSvrWriteFile(pThis->tStreamInfo[i].szSDPFullPathName,pcSDPBuffer, iSDPSize);
    	            else	
						remove(pThis->tStreamInfo[i].szSDPFullPathName);

	            }
	        }			
		}
		else if (pUBuffer->dwDataType == FOURCC_AAC4)
		{

			if( M4ABitstreamPack(pThis->pAudiBitstreamBuf, pUBuffer, pThis->tAudioSrcInfo[iIndex].iMediaIndex, TUBufferSize) != S_OK)
			{
				return S_FAIL;
			}
			
			*((TBitstreamBuffer **)pvCallbackData) = pThis->pAudiBitstreamBuf;
							
			//printf("Aframe index %d\r\n",pThis->pAudiBitstreamBuf->dwStreamIndex);

		}
		else if (pUBuffer->dwDataType == FOURCC_GAMR)
		{
			if (AMRBitstreamPack(pThis->pAudiBitstreamBuf, pUBuffer, pThis->tAudioSrcInfo[iIndex].iMediaIndex, pThis->tAudioSrcInfo[iIndex].iFramesPerUBuffer, TUBufferSize) != S_OK)
			{
				return S_FAIL;
			}
			*((TBitstreamBuffer **)pvCallbackData) = pThis->pAudiBitstreamBuf;
		}
#ifdef _G711_AUDIOIN
		else if (pUBuffer->dwDataType == FOURCC_G711)
		{
			if (G711BitstreamPack(pThis->pAudiBitstreamBuf, pUBuffer, pThis->tAudioSrcInfo[iIndex].iMediaIndex, TUBufferSize) != S_OK)
			{
				return S_FAIL;
			}
			*((TBitstreamBuffer **)pvCallbackData) = pThis->pAudiBitstreamBuf;
		}
#endif
#ifdef _G726_AUDIOIN
		else if (pUBuffer->dwDataType == FOURCC_G726)
		{
			if (G726BitstreamPack(pThis->pAudiBitstreamBuf, pUBuffer, pThis->tAudioSrcInfo[iIndex].iMediaIndex, TUBufferSize) != S_OK)
			{
				return S_FAIL;
			}
			*((TBitstreamBuffer **)pvCallbackData) = pThis->pAudiBitstreamBuf;
		}
#endif

		/*pThis->bAudiUBufEmpty[iIndex] = TRUE;
		if (iIndex+1 == pThis->dwStreamNumber)
		{
			sem_post(&pThis->semWriteAudiData);
		}*/

		return *((void **)pvCallbackData) != NULL ? S_OK : S_FAIL;
    }
    else if( dwCallbackType == MEDIA_CALLBACK_CHECK_CODEC_INDEX)
    {
        int iSDPIndex = (int)pvCallbackData;
        
        if( iSDPIndex > MULTIPLE_STREAM_NUM )
            return -1;
        
        return pThis->tAudioSrcInfo[pThis->tStreamInfo[iSDPIndex-1].iAudioSrcIndex-1].iMediaIndex;

		/*int i;
		for(i=0; i<AUDIO_TRACK_NUMBER; i++)
		{
		    if( strcmp(pThis->tAudioSrcInfo[i].acTrackName,(char*)pvCallbackData) == 0 )
		    {
		        return pThis->tAudioSrcInfo[i].iMediaIndex;
		    }
		}
		return -1;*/
	}	
    else if(dwCallbackType == MEDIA_CALLBACK_RELEASE_BUFFER)
    {
		return S_OK;
    }
    else // MEDIA_CALLBACK_FLUSH_BUFFER
    {
    	return S_OK;
    }
	return S_OK;
}

SCODE StreamSvrVideoCallback(DWORD dwInstance, DWORD dwCallbackType, void* pvCallbackData)
{
	static int iIndex = -1;
	TSTREAMSERVERINFO *pThis = (TSTREAMSERVERINFO *)dwInstance;
	int         i,iSDPSize=0;
	char        pcSDPBuffer[RTSPSTREAMING_SDP_MAXSIZE];

    if(dwCallbackType == MEDIA_CALLBACK_REQUEST_BUFFER)
    {
		TUBuffer *pUBuffer;		
		iIndex++;
		//iIndex = iIndex%pThis->dwStreamNumber;
		//iIndex = iIndex%VIDEO_TRACK_NUMBER;
		
		if( pThis->iGotVideoFlag == FALSE)
		{
		    usleep(10);
		    return S_FAIL;   
		}
		/*if (iIndex == 0)
		{
			sem_wait(&pThis->semVideDataReady);
		}
	
		if (pThis->bVideUBufEmpty[iIndex] == TRUE)
		{
			if (iIndex+1 == pThis->dwStreamNumber)
			{
				sem_post(&pThis->semWriteVideData);
			}
			return S_FAIL;
		}
		if( pThis->tVideoSrcInfo[iIndex].iFdSock < 0 )
		{
		    return S_FAIL;
		}*/    
		
		if(GetVideoUBuffer(pThis,&iIndex) != S_OK)
		{
		    return S_FAIL;
		}		
		
		pUBuffer = (TUBuffer *)pThis->pbyVideUBuffer;
		if (pUBuffer->dwDataType == FOURCC_CONF)
		{
		
			TRTSPSTREAMING_VIDENCODING_PARAM stRTSPVideoParam;
			TUBufferConfMP4V *pMP4VConfUBuffer;
		
			//200080926 initialize bitrate
			memset(&stRTSPVideoParam, 0, sizeof(TRTSPSTREAMING_VIDENCODING_PARAM));
			//20090326 must define codec type to avoid error!
			stRTSPVideoParam.eVideoCodecType = mctMP4V;
			pMP4VConfUBuffer = (TUBufferConfMP4V *)pThis->pbyVideUBuffer;
			stRTSPVideoParam.iProfileLevel = pMP4VConfUBuffer->dwProfileLevel;
			stRTSPVideoParam.iMPEG4HeaderLen = pMP4VConfUBuffer->dwSize - sizeof(TUBufferConfMP4V);
			memcpy(stRTSPVideoParam.acMPEG4Header, (BYTE *)pMP4VConfUBuffer+sizeof(TUBufferConfMP4V), stRTSPVideoParam.iMPEG4HeaderLen);
			stRTSPVideoParam.acMPEG4Header[stRTSPVideoParam.iMPEG4HeaderLen] = 0;
			stRTSPVideoParam.iClockRate = 30000;
			stRTSPVideoParam.iDecoderBufferSize = 76800;
			//CID:1085, CHECKER: STRING_OVERFLOW
			strncpy(stRTSPVideoParam.acTrackName,pThis->tVideoSrcInfo[iIndex].acTrackName, sizeof(stRTSPVideoParam.acTrackName));
			stRTSPVideoParam.acTrackName[sizeof(stRTSPVideoParam.acTrackName) - 1] = 0;
			
			/* Add by ShengFu: check which stream index this media source is mapped to */
            for (i=0; i<pThis->dwStreamNumber; i++)
	        {
	            if(  pThis->tStreamInfo[i].iVideoSrcIndex == iIndex + 1 )        
	            {
	                RTSPStreaming_SetVideoParameters(pThis->hRTSPServer, i+1, &stRTSPVideoParam, 
									RTSPSTREAMING_VIDEO_PROLEVE  | RTSPSTREAMING_VIDEO_BITRATE  | 
						            RTSPSTREAMING_VIDEO_CLOCKRATE| RTSPSTREAMING_VIDEO_MPEG4CI  | 
						            RTSPSTREAMING_VIDEO_WIDTH    | RTSPSTREAMING_VIDEO_HEIGHT   | 
						            RTSPSTREAMING_VIDEO_SET_CI   | RTSPSTREAMING_VIDEO_DECODEBUFF|
									RTSPSTREAMING_VIDEO_CODECTYPE);										//20090326 video codec type should be set
    	            printf("[APP]: %d video source is set to %d stream \r\n",iIndex + 1,i +1);

					//20120925 modified by Jimmy for ONVIF backchannel
					//20111124 modify by danny support Backchannel Multicast for ONVIF test tool 1.02.4
       	            iSDPSize = RTSPStreaming_ComposeAVSDP(pThis->hRTSPServer,i+1 ,(unsigned long)NULL , FALSE, pThis->tMulticastInfo[i].iAlwaysMulticast, pcSDPBuffer,RTSPSTREAMING_SDP_MAXSIZE, FALSE, REQUIRE_NONE);    	            
       	            //printf("\r\n%s\r\n",pcSDPBuffer);       	            
					//CID:188, CHECKER: NEGATIVE_RETURNS
       	            if( pThis->tStreamInfo[i].iEnable == TRUE && iSDPSize > 0)
       	            	StreamSvrWriteFile(pThis->tStreamInfo[i].szSDPFullPathName, pcSDPBuffer, iSDPSize);
					else
						remove(pThis->tStreamInfo[i].szSDPFullPathName);       	            	

	            }
	        }	
		}
		else if (pUBuffer->dwDataType == FOURCC_MP4V)
		{
		    int 			iOffset = 0;
		    unsigned short 	usTag,usLength;
		    DWORD			dwPacketInfo;
		    BYTE *pbyPointer = (BYTE *)pUBuffer + sizeof(TUBuffer);

			memcpy(&usTag, pbyPointer,2);
		    memcpy(&usLength, pbyPointer+2,2);
		    
	    	if( pUBuffer->dwUserDataSize == 0 )
	    	{
				printf("Warning!! User data size of video UBuffer can NOT be zero!!\n");
				return S_FAIL;
			}
		    
		    if( (usTag != TAG_MP4V_PACKETINFO) || 
		    	(usLength > (MAX_MP4V_PACKET_NUM + 1)*4 ) ||
		    	(usLength%4 != 0)	)
		    {
		    	*((void **)pvCallbackData) = NULL;
				return S_FAIL;
		    	//printf("Tag: %d Length:%d \n",usTag,usLength);		    	
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
					pThis->pVideBitstreamBuf->dwOffset = iOffset = pUBuffer->dwUserDataSize - 4 - usLength - 4;
				}
				else
				{
					pThis->pVideBitstreamBuf->dwOffset = iOffset = pUBuffer->dwUserDataSize - 4 - usLength - 4 - usIntelligentVideoLength - 4;	
				}
				
				pThis->pVideBitstreamBuf->dwIntelligentVideoLength = usIntelligentVideoLength;
				//printf("PacketInfo %d Userdata %d IVINFO %d\n", usLength, pThis->pVideBitstreamBuf->dwOffset, pThis->pVideBitstreamBuf->dwIntelligentVideoLength);

				pbyPointer = (BYTE *)pUBuffer;
				pbyPointer += sizeof(TUBuffer);
				memcpy(pThis->pVideBitstreamBuf->pbyBuffer, pbyPointer + 4 + usLength + 4, pUBuffer->dwSize- sizeof(TUBuffer) - 4 - usLength - 4);
				// set time stamp and stream type
				pThis->pVideBitstreamBuf->dwSecond = pUBuffer->dwSec;
				pThis->pVideBitstreamBuf->dwMilliSecond = (pUBuffer->dwUSec / 1000);
				pThis->pVideBitstreamBuf->dwStreamType = mctMP4V;
				pThis->pVideBitstreamBuf->bChangeSetting = FALSE;
				pThis->pVideBitstreamBuf->dwBytesUsed = pUBuffer->dwSize- sizeof(TUBuffer) - pUBuffer->dwUserDataSize;
				pThis->pVideBitstreamBuf->dwStreamIndex = pThis->tVideoSrcInfo[iIndex].iMediaIndex;
				if(pUBuffer->bIsSync == TRUE)
				{
					pThis->pVideBitstreamBuf->tFrameType = MEDIADB_FRAME_INTRA;
				}
				else
				{
					pThis->pVideBitstreamBuf->tFrameType = MEDIADB_FRAME_PRED;
				}
				
				pbyPointer = (BYTE *)pUBuffer + sizeof(TUBuffer);
				pbyPointer =  pbyPointer + 4;
				memcpy(&dwPacketInfo,pbyPointer,4);
				pThis->pVideBitstreamBuf->pdwPacketSize[0] = dwPacketInfo;
				
				//printf("%lu packet cut(",dwPacketInfo);
				for( i=1 ; i < (pThis->pVideBitstreamBuf->pdwPacketSize[0]+1) ; i++  )
				{					
					pbyPointer = pbyPointer + 4;
					memcpy(&dwPacketInfo,pbyPointer,4);
					pThis->pVideBitstreamBuf->pdwPacketSize[i] = dwPacketInfo;
					//printf("%lu,",pThis->pVideBitstreamBuf->pdwPacketSize[i]);
				}
				//printf(")\n");	
				pThis->pVideBitstreamBuf->dwIsBoundary = pUBuffer->bIsBoundary;
				*((TBitstreamBuffer **)pvCallbackData) = pThis->pVideBitstreamBuf;
			}
		}
		
		/*pThis->bVideUBufEmpty[iIndex] = TRUE;
		
		if (iIndex+1 == pThis->dwStreamNumber)
		{
			sem_post(&pThis->semWriteVideData);
		}*/

		return *((void **)pvCallbackData) != NULL ? S_OK : S_FAIL;
    }
    else if(dwCallbackType == MEDIA_CALLBACK_RELEASE_BUFFER)
    {
		// sem_post(&pThis->semWriteVideData);
    }
    else if( dwCallbackType == MEDIA_CALLBACK_CHECK_CODEC_INDEX)
    {
        int iSDPIndex = (int)pvCallbackData;
        
        if( iSDPIndex > MULTIPLE_STREAM_NUM )
            return -1;
        
        return pThis->tVideoSrcInfo[pThis->tStreamInfo[iSDPIndex-1].iVideoSrcIndex-1].iMediaIndex;
        
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
#if 0
    	int iCnt = 0;
    	// flush video bitstream buffer
    	iCnt = 0;
    	while((pBitstreamBuf = OSConstQueue_GetElementFromFilledQueue(((TPeonHandle *)dwInstance)->hVBSQueue, 0)) != NULL)
    	{
    		OSConstQueue_PutElementToEmptyQueue(((TPeonHandle *)dwInstance)->hVBSQueue, pBitstreamBuf);
			printf("RTSP : put element to empty queue\n");
    		iCnt++;
    	}    	    	
    	return S_OK;
#endif
    }
	
	return S_OK;
}
//20120801 added by Jimmy for metadata
SCODE StreamSvrMetadataCallback(DWORD dwInstance, DWORD dwCallbackType, void* pvCallbackData)
{
	//to be implement
	return S_OK;
}


SCODE StreamSvrCheckIfMediaTrackForMulticast(DWORD dwInstance,DWORD  dwSDPIndex,DWORD dwMediaType)
{
	TSTREAMSERVERINFO *pThis = (TSTREAMSERVERINFO *)dwInstance;
	int i;
    
    if( pThis->tMulticastInfo[dwSDPIndex-1].iAlwaysMulticast == TRUE)
        return S_OK;
        
    //for(i=0; i<MULTIPLE_STREAM_NUM; i++)
	//20130327 modified by Jimmy to support different video/audio multicast addresses for ONVIF Test Tool 12.12
    for(i=0; i<(RTSP_MULTICASTNUMBER); i++)
    {
        if( pThis->tMulticastInfo[i].iAlwaysMulticast == TRUE )
        {
			//20120830 modified by Jimmy for metadata
			if( dwMediaType & RTSPSTREAMING_MEDIATYPE_AUDIO )
            {
                if(pThis->tStreamInfo[pThis->tMulticastInfo[i].iSDPIndex-1].iAudioSrcIndex == pThis->tStreamInfo[dwSDPIndex-1].iAudioSrcIndex )
                    return S_OK;   
            }
            
            if( dwMediaType & RTSPSTREAMING_MEDIATYPE_VIDEO )
            {
                if(pThis->tStreamInfo[pThis->tMulticastInfo[i].iSDPIndex-1].iVideoSrcIndex == pThis->tStreamInfo[dwSDPIndex-1].iVideoSrcIndex )
                    return S_OK;   
            }

			if( dwMediaType & RTSPSTREAMING_MEDIATYPE_METADATA )
            {
                if(pThis->tStreamInfo[pThis->tMulticastInfo[i].iSDPIndex-1].iMetadataSrcIndex == pThis->tStreamInfo[dwSDPIndex-1].iMetadataSrcIndex )
                    return S_OK;   
            }
        }
    }
    
    return S_FAIL;
}

SCODE StreamSvrCtrlChCallback(DWORD dwInstance, DWORD dwConnectionID, DWORD dwCallbackType, DWORD dwCallbackData)
{
	CHAR				*pcLoc;
	TSTREAMSERVERINFO	*pThis = (TSTREAMSERVERINFO *)dwInstance;
    char				acTemp[100];
	//20100728 Added by danny For multiple channels videoin/audioin
	int 	iStreamIndex;

	if(dwCallbackType == ccctGetLocation)
	{
		pcLoc = (CHAR*) dwCallbackData;
		printf("[App] CID %u get location sting %s\r\n", dwConnectionID, pcLoc);
	}
	else if(dwCallbackType == ccctCreateMediaChannel)
	{
		printf("[App] CID %u ccctCreateMediaChannel\n", dwConnectionID);
	}
	else if(dwCallbackType == ccctReleaseMediaChannel)
	{
		printf("[App] CID %u ccctReleaseMediaChannel\n", dwConnectionID);
	}
	else if(dwCallbackType == ccctForceIntra)
	{
		//20100428 Added For Media on demand
		if(dwCallbackData <= LIVE_STREAM_NUM)
		{
			TSTREAMSERVERINFO *pThis = (TSTREAMSERVERINFO *)dwInstance;

			//20100728 Added by danny For multiple channels videoin/audioin
			iStreamIndex = StreamingServer_GetMultipleChannelStreamIndex((HANDLE)pThis, dwCallbackData);
			printf("Force Intra to video track %d\r\n", iStreamIndex);
			
			//Modified by Louis 20081119
    	    acTemp[1] = snprintf(acTemp+2, sizeof(acTemp) - 2, "<control id=\"0\" stream=\"%d\">%s", iStreamIndex, CONTROL_MSG_FORCEINTRA);    
    	    acTemp[0] = 0x02;
    	    write(pThis->tVideoSrcInfo[dwCallbackData-1].iFdFIFO,acTemp,acTemp[1]+2);
			printf("%d %s\r\n",acTemp[1],acTemp+2);
		}
	}
	else if(dwCallbackType == ccctPacketLoss)
	{
		printf("[App] CID %u report packet loss\r\n", dwConnectionID);
	}
	else if(dwCallbackType == ccctWaitHTTPSocket)
	{
//		printf("[App] CID %u receive waiting HTTP socket callback\r\n", dwConnectionID);
//		bWaitHTTPSocket = TRUE;
	}
	else if(dwCallbackType == ccctSocketError)
	{
		printf("[App] CID %u receive Socket Error\r\n", dwConnectionID);
	}
	else if(dwCallbackType == ccctClose)
	{
		printf("[App] CID %u closed \r\n", dwConnectionID);
	}
	else if(dwCallbackType == ccctTimeout)
	{
		printf("[App] CID %u timeout\r\n", dwConnectionID);
	}
	else if(dwCallbackType == ccctNeedAuthorization) //20161212
	{
		TAuthorInfo *ptAuthInfo = (TAuthorInfo*)dwCallbackData;
		//Modified by Louis 2008/01/18 for Web-Attraction
		if( ptAuthInfo->iAuthType == RTSPSTREAMING_AUTHENTICATION_DISABLE || pThis->bWebAttraction == TRUE || pThis->bFactoryMode == TRUE) 
		{
			return 0;
		}
		return -1;
	}
	//20161104 remove the workaround with root/blank
	else if(dwCallbackType == ccctAuthorization)
	{
		TAuthorInfo *ptAuthInfo = (TAuthorInfo*)dwCallbackData;
		int	iResult;
		int	i;
		//printf("---authentication---\r\n");	
		printf("%s %s\n",ptAuthInfo->acUserName,ptAuthInfo->acPasswd);
		//if the access name is alias name, authentication is must!
		for( i=0 ; i < MULTIPLE_STREAM_NUM ; i++ )
		{
			if( (ptAuthInfo->iAuthType == RTSPSTREAMING_AUTHENTICATION_DISABLE) &&
				(strcmp(ptAuthInfo->acAccessName,g_acAliasAccessName[i]) == 0 ) )
				ptAuthInfo->iAuthType = RTSPSTREAMING_AUTHENTICATION_BASIC;
		}
		
		
		if( ptAuthInfo->iAuthType == RTSPSTREAMING_AUTHENTICATION_BASIC )
		{			
			//printf("%s %s\n",ptAuthInfo->acUserName,ptAuthInfo->acPasswd);
			iResult = AM_Query(ptAuthInfo->acUserName
								,ptAuthInfo->acPasswd
								,AMA_USER_QUERY_PASSWD|AMA_USER_QUERY_BOOLEAN
								,&(pThis->tAMConf));

			if( iResult == 1 )
			{
				printf("authorization OK!!\n");
				return 0;
			}
			else
			{
				printf("authorization failed %d %s %s\n",iResult,ptAuthInfo->acUserName,ptAuthInfo->acPasswd);
				return -1;	
			}
		}
		
		if(ptAuthInfo->iAuthType == RTSPSTREAMING_AUTHENTICATION_DIGEST )
		{
			// int i;			
				
			printf("username: %s \n",ptAuthInfo->acUserName);
			iResult = AM_GetDigestPass(ptAuthInfo->acUserName
										,ptAuthInfo->acPasswd
										,&(pThis->tAMConf));
			printf("digest %s\n",ptAuthInfo->acPasswd);										

/*			printf("%d ",iResult);
			for( i=0 ; i<32 ; i++)
				printf("%02x",ptAuthInfo->acPasswd[i]);
			printf("\n");
			
			EncryptionUtl_MD5(ptAuthInfo->acPasswd,"dog",":","streaming_server",":","123",NULL);
										
			for( i=0 ; i<32 ; i++)
				printf("%02x",ptAuthInfo->acPasswd[i]);
			printf("\n");				*/
			
			//(1) if no authentication informaion is sent from client and no root password is set, no digest authentication (return 0)
			//(2) if athentication of specific user is needed and data base is hit, return 0
			//20161104 modify by Faber, we should verify the authenication even in root/blank
			if( iResult == 1 )
			{
				return 0;
			}
			else
				return -1;	
			/*memset((void*)(ptAuthInfo->acPasswd),0,SS_AUTH_PASS_LENGTH);
			EncryptionUtl_MD5(ptAuthInfo->acPasswd,UserName,":","streaming_server",":",Password,NULL);
			return 0;*/
		}
		else
			return -1;
	}
	else if( dwCallbackType == ccctRTSPSessionInfoUpdate)
	{
    	StreamSvrWriteFile(pThis->tRTSPInfo.szSessionInfoPath,(char*)dwCallbackData,strlen((char*)dwCallbackData));	        	       	        
	}
	else if( dwCallbackType == ccctAudioUploadSDPInfo)
	{
		TCtrlChCodecInfo 	*ptAudioSDPInfo = NULL;
		TUBufferConfG711 	*ptUBufferConfG711 = NULL;
		TUBuffer			*ptUBuffer = NULL;
		int 				iResult;
		
		if( (void*)dwCallbackData == NULL )
		{
			printf("Error in callback for audio SDP info!\n");
			return -1;
		}	
		
		ptUBuffer = (TUBuffer*)pThis->pbyAudioUploadBuff;
		ptUBuffer->dwDataType = FOURCC_CONF;
		ptUBuffer->dwFollowingDataType = FOURCC_G711;
		ptAudioSDPInfo = (TCtrlChCodecInfo*)dwCallbackData;				
		ptUBufferConfG711 = (TUBufferConfG711*)pThis->pbyAudioUploadBuff;		
		
		if( ptAudioSDPInfo->eCodecType == mctG711A )
		{
			ptUBufferConfG711->dwCompressionLaw = FOURCC_ALAW;			
			//printf("a-law = %lu\n",FOURCC_ALAW);
		}
		else // if( ptAudioSDPInfo->eCodecType == mctG711U )
		{
			ptUBufferConfG711->dwCompressionLaw = FOURCC_ULAW;
			//printf("u-law = %lu\n",FOURCC_ULAW);
		}

		//20101220 Added by Danny for mark G711 upstream
		ptUBufferConfG711->bUpStream = TRUE;
		
		ptUBufferConfG711->dwChannelNumber = ptAudioSDPInfo->wChannelNumber;
		
		//20100708 Added by Danny for optional channels output data
		ptUBufferConfG711->dwBitmapDWordLen = 1;
		ptUBufferConfG711->dwChannelBitmap = ptAudioSDPInfo->dwChannelBitmap;
		//printf("[%s] ptUBufferConfG711->dwChannelBitmap %u\n", __FUNCTION__, ptUBufferConfG711->dwChannelBitmap);
		
		iResult = writeClientSocket(pThis->tAudioDstInfo.iFdSock, pThis->pbyAudioUploadBuff, sizeof(TUBufferConfG711));
		if( iResult < 0 )
		{
			if ( connectClientSocket(pThis->tAudioDstInfo.iFdSock, pThis->tAudioDstInfo.acSockPathName) == S_OK)
			{
				iResult = writeClientSocket(pThis->tAudioDstInfo.iFdSock, pThis->pbyAudioUploadBuff, sizeof(TUBufferConfG711));
				if( iResult < 0 )
				{
					int iError = errno;
					printf("Send CI to Adec fail(%s)\n", strerror(iError));
#ifdef _LINUX					
					syslog(LOG_WARNING,"Send CI to Adec fail(%s)\n", strerror(iError));
#endif
				}
			}
			else
			{
				int iError = errno;
				printf("Connect to Adec fail(%s)\n", strerror(iError));
#ifdef _LINUX					
				syslog(LOG_WARNING,"Connect to Adec fail(%s)\n", strerror(iError));
#endif
			}
		}
							
		return 0;
	}
	else if( dwCallbackType == ccctRTPAudioDataUpload )
	{
		PROTOCOL_MEDIABUFFER *ptMediaBuffer=NULL ;
		int		iResult;		
		
		//static int iCount=0;		
		//iCount ++;
		
		if( ((void*)dwCallbackData == NULL) || 
		    (pThis->tAudioDstInfo.iFdSock < 0 ) )
		{
			printf("Error in callback\n");
			return 0;
		}
		
		ptMediaBuffer = (PROTOCOL_MEDIABUFFER *)dwCallbackData;
		
		((TUBuffer*)pThis->pbyAudioUploadBuff)->dwDataType = FOURCC_G711;
		((TUBuffer*)pThis->pbyAudioUploadBuff)->dwSize = ptMediaBuffer->dwBytesUsed + sizeof(TUBuffer);
		memcpy((void*)(pThis->pbyAudioUploadBuff + sizeof(TUBuffer)),ptMediaBuffer->pbDataStart,ptMediaBuffer->dwBytesUsed);
		
		iResult = writeClientSocket(pThis->tAudioDstInfo.iFdSock, pThis->pbyAudioUploadBuff, ((TUBuffer*)pThis->pbyAudioUploadBuff)->dwSize);
		if( iResult < 0 )
		{
			connectClientSocket(pThis->tAudioDstInfo.iFdSock, pThis->tAudioDstInfo.acSockPathName);
		}		
		
		/*if( pTempFile != NULL )
		{			
			fwrite(ptMediaBuffer->pbDataStart,sizeof(char),ptMediaBuffer->dwBytesUsed,pTempFile);
		}
		if( iCount > 500 && pTempFile != NULL )
		{								
			fclose(pTempFile);
			pTempFile = NULL;
			printf("audio dump OK %d\n",iCount);			
		}*/
		
		/*if( ((RTPHEADERINFO*)(ptMediaBuffer->pbHeaderInfoStart))->usSeqNumber%100 == 0 )
		{			
			printf("Size=%d Seq=%d TS=%lu\r\n"
						,ptMediaBuffer->dwBytesUsed
						,((RTPHEADERINFO*)(ptMediaBuffer->pbHeaderInfoStart))->usSeqNumber
						,((RTPHEADERINFO*)(ptMediaBuffer->pbHeaderInfoStart))->ulTimeStamp);
		}*/
	}
	else if(dwCallbackType == ccctStraemTypeVideoPause) //20160623 add by Faber, pause MOD before set parameter
	{
		int iStreamIndex = pThis->tStreamInfo[dwCallbackData-1].iVideoSrcIndex - LIVE_STREAM_NUM;
		if(iStreamIndex< 0)
			return -1;
		acTemp[0] = 0x02;
		acTemp[1] = snprintf(acTemp+2, sizeof(acTemp) - 2, "<control id=\"0\" stream=\"%d\"><video/>%s", iStreamIndex, CONTROL_MSG_PAUSE);
		write(pThis->tVideoSrcInfo[pThis->tStreamInfo[dwCallbackData-1].iVideoSrcIndex-1].iFdFIFO,acTemp,acTemp[1]+2);
		printf("%d %s\r\n",acTemp[1],acTemp+2);
	}
	else if(dwCallbackType == ccctStraemTypeAudioPause)
	{
		int iStreamIndex = pThis->tStreamInfo[dwCallbackData-1].iAudioSrcIndex - LIVE_STREAM_NUM;
		if(iStreamIndex< 0)
			return -1;
		acTemp[0] = 0x02;
		acTemp[1] = snprintf(acTemp+2, sizeof(acTemp) - 2, "<control id=\"0\" stream=\"%d\"><audio/>%s", iStreamIndex, CONTROL_MSG_PAUSE);
		write(pThis->tVideoSrcInfo[pThis->tStreamInfo[dwCallbackData-1].iAudioSrcIndex-1].iFdFIFO,acTemp,acTemp[1]+2);
		printf("%d %s\r\n",acTemp[1],acTemp+2);
	}
	else if(dwCallbackType == ccctStraemTypeVideoResume)
	{
		int iStreamIndex = pThis->tStreamInfo[dwCallbackData-1].iVideoSrcIndex - LIVE_STREAM_NUM;
		if(iStreamIndex< 0)
			return -1;
		acTemp[0] = 0x02;
		acTemp[1] = snprintf(acTemp+2, sizeof(acTemp) - 2, "<control id=\"0\" stream=\"%d\"><video/>%s", iStreamIndex, CONTROL_MSG_RESUME);
		write(pThis->tVideoSrcInfo[pThis->tStreamInfo[dwCallbackData-1].iVideoSrcIndex-1].iFdFIFO,acTemp,acTemp[1]+2);
		printf("%d %s\r\n",acTemp[1],acTemp+2);
	}
	else if(dwCallbackType == ccctStraemTypeAudioResume)
	{
		int iStreamIndex = pThis->tStreamInfo[dwCallbackData-1].iAudioSrcIndex - LIVE_STREAM_NUM;
		if(iStreamIndex< 0)
			return -1;
		acTemp[0] = 0x02;
		acTemp[1] = snprintf(acTemp+2, sizeof(acTemp) - 2, "<control id=\"0\" stream=\"%d\"><video/>%s", iStreamIndex, CONTROL_MSG_RESUME);
		write(pThis->tVideoSrcInfo[pThis->tStreamInfo[dwCallbackData-1].iAudioSrcIndex-1].iFdFIFO,acTemp,acTemp[1]+2);
		printf("%d %s\r\n",acTemp[1],acTemp+2);
	}
	else if(dwCallbackType == ccctStreamTypeVideoStart)
    { 
		if( dwCallbackData > MULTIPLE_STREAM_NUM || dwCallbackData <= 0 )
        {
            printf("Erorr!! SDP index messed up %d\r\n",dwCallbackData);
            return -1;        
        }
        
        if( pThis->tStreamInfo[dwCallbackData-1].iVideoSrcIndex != 0 )    
    	    pThis->tVideoSrcInfo[pThis->tStreamInfo[dwCallbackData-1].iVideoSrcIndex-1].iNumberOfWorkingSDP++;                	
    	
		printf("[%s]Video %d iNumberOfWorkingSDP %d\r\n", __FUNCTION__, pThis->tStreamInfo[dwCallbackData-1].iVideoSrcIndex-1, 
				pThis->tVideoSrcInfo[pThis->tStreamInfo[dwCallbackData-1].iVideoSrcIndex-1].iNumberOfWorkingSDP);
		if( pThis->tVideoSrcInfo[pThis->tStreamInfo[dwCallbackData-1].iVideoSrcIndex-1].iNumberOfWorkingSDP == 1 )
    	{
    	    printf("Video track %d should start\r\n",pThis->tStreamInfo[dwCallbackData-1].iVideoSrcIndex);    
#ifdef _SHARED_MEM
			//20100428 Added For Media on demand
			if(dwCallbackData > LIVE_STREAM_NUM)
			{
				//Self test for MOD server not finish
				acTemp[1] = snprintf(acTemp+2, sizeof(acTemp) - 2, "<control id=\"0\" stream=\"%d\"><video/>%s", pThis->tStreamInfo[dwCallbackData-1].iVideoSrcIndex - LIVE_STREAM_NUM, CONTROL_MSG_START);
				//acTemp[1] = snprintf(acTemp+2, sizeof(acTemp) - 2, "<control id=\"0\" stream=\"%d\">%s", pThis->tStreamInfo[dwCallbackData-1].iVideoSrcIndex, CONTROL_MSG_START);
			}
			else
#endif
			{    	
				//20100728 Added by danny For multiple channels videoin/audioin
				iStreamIndex = StreamingServer_GetMultipleChannelStreamIndex((HANDLE)pThis, pThis->tStreamInfo[dwCallbackData-1].iVideoSrcIndex);
				
				//Modified by Louis 20081119 to add stream ID
    	        acTemp[1] = snprintf(acTemp+2, sizeof(acTemp) - 2, "<control id=\"0\" stream=\"%d\">%s", iStreamIndex, CONTROL_MSG_START);
    	    }
			acTemp[0] = 0x02;
			write(pThis->tVideoSrcInfo[pThis->tStreamInfo[dwCallbackData-1].iVideoSrcIndex-1].iFdFIFO,acTemp,acTemp[1]+2);
			printf("%d %s\r\n",acTemp[1],acTemp+2);
    	}        
    }
    else if(dwCallbackType == ccctStreamTypeAudioStart)
    { 
		if( dwCallbackData > MULTIPLE_STREAM_NUM || dwCallbackData <= 0 )
        {
            printf("Erorr!! SDP index messed up %d\r\n",dwCallbackData);
            return -1;        
        }
        
        if( pThis->tStreamInfo[dwCallbackData-1].iAudioSrcIndex != 0 )    
    	    pThis->tAudioSrcInfo[pThis->tStreamInfo[dwCallbackData-1].iAudioSrcIndex-1].iNumberOfWorkingSDP++;                	

    	printf("[%s]Audio %d iNumberOfWorkingSDP %d\r\n", __FUNCTION__, pThis->tStreamInfo[dwCallbackData-1].iAudioSrcIndex-1, 
				pThis->tAudioSrcInfo[pThis->tStreamInfo[dwCallbackData-1].iAudioSrcIndex-1].iNumberOfWorkingSDP);
    	if( pThis->tAudioSrcInfo[pThis->tStreamInfo[dwCallbackData-1].iAudioSrcIndex-1].iNumberOfWorkingSDP == 1 )
    	{
    	    printf("Audio track %d should start\r\n",pThis->tStreamInfo[dwCallbackData-1].iAudioSrcIndex);
#ifdef _SHARED_MEM
			//20100428 Added For Media on demand
			if(dwCallbackData > LIVE_STREAM_NUM)
			{
				//Self test for MOD server not finish
				acTemp[1] = snprintf(acTemp+2, sizeof(acTemp) - 2,"<control id=\"0\" stream=\"%d\"><audio/>%s", pThis->tStreamInfo[dwCallbackData-1].iAudioSrcIndex - LIVE_AUDIO_STREAM_NUM, CONTROL_MSG_START);
				//acTemp[1] = snprintf(acTemp+2, sizeof(acTemp) - 2,"<control id=\"0\" stream=\"%d\">%s", pThis->tStreamInfo[dwCallbackData-1].iAudioSrcIndex, CONTROL_MSG_START);
			}
			else
#endif
			{
				//20100728 Added by danny For multiple channels videoin/audioin
				iStreamIndex = StreamingServer_GetMultipleChannelStreamIndex((HANDLE)pThis, pThis->tStreamInfo[dwCallbackData-1].iAudioSrcIndex);
				
				//Modified by Louis 20081119 to add stream ID
				acTemp[1] = snprintf(acTemp+2, sizeof(acTemp) - 2,"<control id=\"0\" stream=\"%d\">%s", iStreamIndex, CONTROL_MSG_START);
    	    }
			acTemp[0] = 0x02;
			write(pThis->tAudioSrcInfo[pThis->tStreamInfo[dwCallbackData-1].iAudioSrcIndex-1].iFdFIFO,acTemp,acTemp[1]+2);
			printf("%d %s\r\n",acTemp[1],acTemp+2);
        }
    }
	//20120821 added by Jimmy for metadata
    else if(dwCallbackType == ccctStreamTypeMetadataStart)
    { 
        if( dwCallbackData > MULTIPLE_STREAM_NUM || dwCallbackData <= 0 )
        {
            printf("Erorr!! SDP index messed up %d\r\n",dwCallbackData);
            return -1;        
        }
        
        if( pThis->tStreamInfo[dwCallbackData-1].iMetadataSrcIndex != 0 )    
            pThis->tMetadataSrcInfo[pThis->tStreamInfo[dwCallbackData-1].iMetadataSrcIndex-1].iNumberOfWorkingSDP++;                    

        printf("[%s]Metadata %d iNumberOfWorkingSDP %d\r\n", __FUNCTION__, pThis->tStreamInfo[dwCallbackData-1].iMetadataSrcIndex-1, 
                pThis->tMetadataSrcInfo[pThis->tStreamInfo[dwCallbackData-1].iMetadataSrcIndex-1].iNumberOfWorkingSDP);
        if( pThis->tMetadataSrcInfo[pThis->tStreamInfo[dwCallbackData-1].iMetadataSrcIndex-1].iNumberOfWorkingSDP == 1 )
        {
            printf("Metadata track %d should start\r\n",pThis->tStreamInfo[dwCallbackData-1].iMetadataSrcIndex);
#ifdef _SHARED_MEM
            //20100428 Added For Media on demand
            if(dwCallbackData > LIVE_STREAM_NUM)
            {
                //Self test for MOD server not finish
                acTemp[1] = snprintf(acTemp+2, sizeof(acTemp) - 2,"<control id=\"0\" stream=\"%d\"><metadata/>%s", pThis->tStreamInfo[dwCallbackData-1].iMetadataSrcIndex - LIVE_METADATA_STREAM_NUM, CONTROL_MSG_START);
                //acTemp[1] = snprintf(acTemp+2, sizeof(acTemp) - 2,"<control id=\"0\" stream=\"%d\">%s", pThis->tStreamInfo[dwCallbackData-1].iAudioSrcIndex, CONTROL_MSG_START);
            }
            else
#endif
            {
                //20100728 Added by danny For multiple channels videoin/audioin
                iStreamIndex = StreamingServer_GetMultipleChannelStreamIndex((HANDLE)pThis, pThis->tStreamInfo[dwCallbackData-1].iMetadataSrcIndex);
                
                //Modified by Louis 20081119 to add stream ID
                acTemp[1] = snprintf(acTemp+2, sizeof(acTemp) - 2,"<control id=\"0\" stream=\"%d\">%s", iStreamIndex, CONTROL_MSG_START);
            }
            acTemp[0] = 0x02;
            write(pThis->tMetadataSrcInfo[pThis->tStreamInfo[dwCallbackData-1].iMetadataSrcIndex-1].iFdFIFO,acTemp,acTemp[1]+2);
            printf("%d %s\r\n",acTemp[1],acTemp+2);
        }
    }
    else if(dwCallbackType == ccctStreamTypeVideoStop)
    {       
        if( dwCallbackData > MULTIPLE_STREAM_NUM || dwCallbackData <= 0 )
        {
            printf("Erorr!! SDP index messed up %d\r\n",dwCallbackData);
            return -1;        
        }
        
        if( pThis->tStreamInfo[dwCallbackData-1].iVideoSrcIndex != 0 )    
    	    pThis->tVideoSrcInfo[pThis->tStreamInfo[dwCallbackData-1].iVideoSrcIndex-1].iNumberOfWorkingSDP--;                	

		printf("[%s]Video %d iNumberOfWorkingSDP %d\r\n", __FUNCTION__, pThis->tStreamInfo[dwCallbackData-1].iVideoSrcIndex-1, 
				pThis->tVideoSrcInfo[pThis->tStreamInfo[dwCallbackData-1].iVideoSrcIndex-1].iNumberOfWorkingSDP);
		if( pThis->tVideoSrcInfo[pThis->tStreamInfo[dwCallbackData-1].iVideoSrcIndex-1].iNumberOfWorkingSDP == 0 )    	
    	{
    		//20120830 modified by Jimmy for metadata
    	    if( StreamSvrCheckIfMediaTrackForMulticast((DWORD)pThis,dwCallbackData,RTSPSTREAMING_MEDIATYPE_VIDEO) == S_OK)
    	    {
    	        //some stream still needs video to multicast!
    	        return S_OK;
    	    }
    	          	    
    	    printf("Video track %d should stop\r\n",pThis->tStreamInfo[dwCallbackData-1].iVideoSrcIndex);    
#ifdef _SHARED_MEM
			//20100428 Added For Media on demand
			if(dwCallbackData > LIVE_STREAM_NUM)
			{
				//Self test for MOD server not finish
				acTemp[1] = snprintf(acTemp+2, sizeof(acTemp) - 2,"<control id=\"0\" stream=\"%d\"><video/>%s", pThis->tStreamInfo[dwCallbackData-1].iVideoSrcIndex - LIVE_STREAM_NUM, CONTROL_MSG_STOP);
				//acTemp[1] = snprintf(acTemp+2, sizeof(acTemp) - 2,"<control id=\"0\" stream=\"%d\">%s", pThis->tStreamInfo[dwCallbackData-1].iVideoSrcIndex, CONTROL_MSG_STOP);
			}
			else
#endif
			{  	     
				//20100728 Added by danny For multiple channels videoin/audioin
				iStreamIndex = StreamingServer_GetMultipleChannelStreamIndex((HANDLE)pThis, pThis->tStreamInfo[dwCallbackData-1].iVideoSrcIndex);
				
				//Modified by Louis 20081119 to add stream ID
				acTemp[1] = snprintf(acTemp+2, sizeof(acTemp) - 2,"<control id=\"0\" stream=\"%d\">%s", iStreamIndex, CONTROL_MSG_STOP);
    	    }
			acTemp[0] = 0x02;

			int iSdpIndex = pThis->tStreamInfo[dwCallbackData-1].iVideoSrcIndex - 1;
			printf("video stop before write\n");
			OSCriticalSection_Enter(pThis->tVideoSrcInfo[iSdpIndex].hMediaSrcMutex);
        	
			write(pThis->tVideoSrcInfo[iSdpIndex].iFdFIFO,acTemp,acTemp[1]+2);
			//20170524
			if(dwCallbackData > LIVE_STREAM_NUM)
			{
				pThis->aiModStream[iSdpIndex - LIVE_STREAM_NUM] = 0;
				dump_mod_stream(pThis->aiModStream);
				printf("pThis->aiModStream[%d] stop\n", iSdpIndex - LIVE_STREAM_NUM );
			}
			printf("%d %s\r\n",acTemp[1],acTemp+2);
            printf("video stop after write\n");
            OSCriticalSection_Leave(pThis->tVideoSrcInfo[iSdpIndex].hMediaSrcMutex);
            
			
    	}           
    }
    else if(dwCallbackType == ccctStreamTypeAudioStop)
    {
        if( dwCallbackData > MULTIPLE_STREAM_NUM || dwCallbackData <= 0 )
        {
            printf("Erorr!! SDP index messed up %d\r\n",dwCallbackData);
            return -1;        
        }
        
        if( pThis->tStreamInfo[dwCallbackData-1].iAudioSrcIndex != 0 )    
    	    pThis->tAudioSrcInfo[pThis->tStreamInfo[dwCallbackData-1].iAudioSrcIndex-1].iNumberOfWorkingSDP--;                	
    	
		printf("[%s]Audio %d iNumberOfWorkingSDP %d\r\n", __FUNCTION__, pThis->tStreamInfo[dwCallbackData-1].iAudioSrcIndex-1, 
				pThis->tAudioSrcInfo[pThis->tStreamInfo[dwCallbackData-1].iAudioSrcIndex-1].iNumberOfWorkingSDP);
		if( pThis->tAudioSrcInfo[pThis->tStreamInfo[dwCallbackData-1].iAudioSrcIndex-1].iNumberOfWorkingSDP == 0 )
    	{
    		//20120830 modified by Jimmy for metadata
    	    if( StreamSvrCheckIfMediaTrackForMulticast((DWORD)pThis,dwCallbackData,RTSPSTREAMING_MEDIATYPE_AUDIO) == S_OK)
    	    {
    	        //some stream still needs audio to multicast!
    	        return S_OK;
    	    }
    	            	    
    	    printf("Audio track %d should stop\r\n",pThis->tStreamInfo[dwCallbackData-1].iAudioSrcIndex);
#ifdef _SHARED_MEM
			//20100428 Added For Media on demand
			if(dwCallbackData > LIVE_STREAM_NUM)
			{
				//Self test for MOD server not finish
				acTemp[1] = snprintf(acTemp+2, sizeof(acTemp) - 2,"<control id=\"0\" stream=\"%d\"><audio/>%s", pThis->tStreamInfo[dwCallbackData-1].iAudioSrcIndex - LIVE_AUDIO_STREAM_NUM, CONTROL_MSG_STOP);
				//acTemp[1] = snprintf(acTemp+2, sizeof(acTemp) - 2,"<control id=\"0\" stream=\"%d\">%s", pThis->tStreamInfo[dwCallbackData-1].iAudioSrcIndex, CONTROL_MSG_STOP);
			}
			else
#endif
			{
				//20100728 Added by danny For multiple channels videoin/audioin
				iStreamIndex = StreamingServer_GetMultipleChannelStreamIndex((HANDLE)pThis, pThis->tStreamInfo[dwCallbackData-1].iAudioSrcIndex);
				
				//Modified by Louis 20081119 to add stream ID
				acTemp[1] = snprintf(acTemp+2, sizeof(acTemp) - 2,"<control id=\"0\" stream=\"%d\">%s", iStreamIndex, CONTROL_MSG_STOP);
    	    }
			acTemp[0] = 0x02;
			
			OSCriticalSection_Enter(pThis->tAudioSrcInfo[pThis->tStreamInfo[dwCallbackData-1].iAudioSrcIndex-1].hMediaSrcMutex);
			printf("audio stop before write\n");
			write(pThis->tAudioSrcInfo[pThis->tStreamInfo[dwCallbackData-1].iAudioSrcIndex-1].iFdFIFO,acTemp,acTemp[1]+2);
			printf("audio stop after write\n");
			OSCriticalSection_Leave(pThis->tAudioSrcInfo[pThis->tStreamInfo[dwCallbackData-1].iAudioSrcIndex-1].hMediaSrcMutex);
            
			printf("%d %s\r\n",acTemp[1],acTemp+2);
    	}
    }
	//20120821 added by Jimmy for metadata
    else if(dwCallbackType == ccctStreamTypeMetadataStop)
    {
        if( dwCallbackData > MULTIPLE_STREAM_NUM || dwCallbackData <= 0 )
        {
            printf("Erorr!! SDP index messed up %d\r\n",dwCallbackData);
            return -1;        
        }
        
        if( pThis->tStreamInfo[dwCallbackData-1].iMetadataSrcIndex != 0 )    
            pThis->tMetadataSrcInfo[pThis->tStreamInfo[dwCallbackData-1].iMetadataSrcIndex-1].iNumberOfWorkingSDP--;                    
        
        printf("[%s]Metadata %d iNumberOfWorkingSDP %d\r\n", __FUNCTION__, pThis->tStreamInfo[dwCallbackData-1].iMetadataSrcIndex-1, 
                pThis->tMetadataSrcInfo[pThis->tStreamInfo[dwCallbackData-1].iMetadataSrcIndex-1].iNumberOfWorkingSDP);
        if( pThis->tMetadataSrcInfo[pThis->tStreamInfo[dwCallbackData-1].iMetadataSrcIndex-1].iNumberOfWorkingSDP == 0 )
        {           
            if( StreamSvrCheckIfMediaTrackForMulticast((DWORD)pThis,dwCallbackData,RTSPSTREAMING_MEDIATYPE_METADATA) == S_OK)
            {
                //some stream still needs audio to multicast!
                return S_OK;
            }
                            
            printf("Metadata track %d should stop\r\n",pThis->tStreamInfo[dwCallbackData-1].iMetadataSrcIndex);
#ifdef _SHARED_MEM
            //20100428 Added For Media on demand
            if(dwCallbackData > LIVE_STREAM_NUM)
            {
                //Self test for MOD server not finish
                acTemp[1] = snprintf(acTemp+2, sizeof(acTemp) - 2,"<control id=\"0\" stream=\"%d\"><metadata/>%s", pThis->tStreamInfo[dwCallbackData-1].iMetadataSrcIndex - LIVE_METADATA_STREAM_NUM, CONTROL_MSG_STOP);
                //acTemp[1] = snprintf(acTemp+2, sizeof(acTemp) - 2,"<control id=\"0\" stream=\"%d\">%s", pThis->tStreamInfo[dwCallbackData-1].iAudioSrcIndex, CONTROL_MSG_STOP);
            }
            else
#endif
            {
                //20100728 Added by danny For multiple channels videoin/audioin
                iStreamIndex = StreamingServer_GetMultipleChannelStreamIndex((HANDLE)pThis, pThis->tStreamInfo[dwCallbackData-1].iMetadataSrcIndex);
                
                //Modified by Louis 20081119 to add stream ID
                acTemp[1] = snprintf(acTemp+2, sizeof(acTemp) - 2,"<control id=\"0\" stream=\"%d\">%s", iStreamIndex, CONTROL_MSG_STOP);
            }
            acTemp[0] = 0x02;
            write(pThis->tMetadataSrcInfo[pThis->tStreamInfo[dwCallbackData-1].iMetadataSrcIndex-1].iFdFIFO,acTemp,acTemp[1]+2);
            printf("%d %s\r\n",acTemp[1],acTemp+2);
        }
    }
    else if( dwCallbackType == ccctForceCI)
    {
		TMultipleStreamCIInfo		*ptCIInfo = NULL;
		
		//20090324 Multiple Stream
		if(dwCallbackData != 0)
		{
			ptCIInfo = (TMultipleStreamCIInfo *)dwCallbackData;
		}
    	StreamingServer_SetMediaTrackParam((HANDLE)pThis, ptCIInfo);
    }
#ifdef _SHARED_MEM
	//20110915 Modify by danny for support Genetec MOD
	//20100105 Added For Seamless Recording
	else if( dwCallbackType ==ccctSetToConfiger )
	{
		char		*pzCmd = NULL;
		
		if(dwCallbackData != 0)
		{
			pzCmd = (char *)dwCallbackData;
		}
		
		return StreamingServer_SetToConfigure(pzCmd);
	}
	else if( dwCallbackType ==ccctRecoderStateUpdate )
	{
		char		*pzBuffer = NULL;
		
		if(dwCallbackData != 0)
		{
			pzBuffer = (char *)dwCallbackData;
		}
		
		return StreamingServer_SetRecoderState(pzBuffer);
	}
	//20100428 Added For Media on demand
	else if( dwCallbackType == ccctMODForceCI)
    {
		TMultipleStreamCIInfo		*ptCIInfo = NULL;
		
		if(dwCallbackData != 0)
		{
			ptCIInfo = (TMultipleStreamCIInfo *)dwCallbackData;
		}
    	return StreamingServer_SetMODMediaTrackParam((HANDLE)pThis, ptCIInfo);
    }
	else if( dwCallbackType == ccctSetMODControlInfo)
    {
		RTSPSERVER_MODREQUEST *pstRTSPServerMODRequest = NULL;
		
		if(dwCallbackData != 0)
		{
			pstRTSPServerMODRequest = (RTSPSERVER_MODREQUEST *)dwCallbackData;
		}
    	return StreamingServer_SetMODControlParam((HANDLE)pThis, pstRTSPServerMODRequest);
    }
#endif
	//20100604 Added by Danny for sessioninfo
	else if( dwCallbackType == ccctSIPUAessionInfoUpdate)
	{
    	return StreamSvrWriteFile(pThis->tSIPInfo.szSessionInfoPath, (char*)dwCallbackData, strlen((char*)dwCallbackData));	        	       	        
	}
	//20100728 Modifed by danny For multiple channels videoin/audioin
	else if( dwCallbackType == ccctGetMultipleChannelChannelIndex)
	{
    	return StreamingServer_GetMultipleChannelChannelIndex((HANDLE)pThis, (int)dwCallbackData);	        	       	        
	}
	//20110401 Added by danny For support RTSPServer thread watchdog
	else if( dwCallbackType == ccctRTSPServerKickWatchdog)
	{
		OSTime_GetTimer(&pThis->dwRTSPServerLastKick, NULL);
		
		return S_OK;     	       	        
	}
	else if(dwCallbackType == cccGetNemModStream) //20170524
	{
		int iModStreamIndex = 0;
		int *piModNewStreamNumber = (int*)dwCallbackData;
		for(iModStreamIndex = 0; iModStreamIndex < MOD_STREAM_NUM; iModStreamIndex++)
		{
			if(!pThis->aiModStream[iModStreamIndex])
			{
				*piModNewStreamNumber = iModStreamIndex;
				pThis->aiModStream[iModStreamIndex] = 1;
				dump_mod_stream(pThis->aiModStream);
				return S_OK;
			}
		}
		dump_mod_stream(pThis->aiModStream);
		return S_FAIL;
	}
	else if(dwCallbackType == cccReleaseModStream)
	{
		int piModIndex = (int*)dwCallbackData;
		piModIndex -= 1;
		if(piModIndex > LIVE_STREAM_NUM)
		{
			printf("pThis->aiModStream[%d] = 0\n", piModIndex - LIVE_STREAM_NUM);
			pThis->aiModStream[piModIndex - LIVE_STREAM_NUM ] = 0;	
			dump_mod_stream(pThis->aiModStream);
		}
		
		
		return S_OK;
	}
	else
	{
		printf("[App] CID %u Callback %u\r\n", dwConnectionID, dwCallbackType);
	}
	return S_OK;
}

void dump_mod_stream(int *aiModStream)
{
	printf("---------------- %s -----------------\n", __func__);
	int iModStream = 0;
	for(iModStream = 0; iModStream < MOD_STREAM_NUM; iModStream++)
	{
		printf("pThis->aiModStream[%d] = %d\n", iModStream, *(aiModStream + iModStream));
	}
	printf("-----------------------------------------\n");
}