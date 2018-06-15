/*
 *  Copyright (c) 2004 Vivotek Inc. All rights reserved.
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
 *  Module name         :   RTPPacketizer.h
 *  Module description  :   Callback to ask for one audio/video frame and 
 *                          packetize into RTP media buffer format
 *                            
 *  Author              :   ShengFu
 *  Created at          :   2004/04/23
 *  Revision            :   1.0
 ******************************************************************************
 *                        Revision history
 ******************************************************************************
 */

#include <stdlib.h> 
#include <string.h>
#include "rtppacketizer.h"

#ifndef _WIN32_
	#ifndef _LINUX_X86
		#include "datapacketdef.h"
	#endif
#endif 
#ifdef _LINUX
#include <sys/syslog.h>
#endif // _LINUX
 
 
 typedef struct 
 {
    HANDLE hDataQueue;
    HANDLE hEmptyQueue;
    
    DWORD  dwThreadID;
	// Added by cchuang 2005/05/23
	BOOL   bTerminateThread;
    DWORD  dwStatus;
    RTPPACKETIZERCALLBACK fRTPPacketizerCallback;
    HANDLE              hParentHandle;
      
    char   acLocation[LOCATION_LEN];
	//modified 20080722 to avoid initialization error
    DWORD 	dwSSec[MAXIMUM_TRACK_NUMBER];
    DWORD 	dwSMSec[MAXIMUM_TRACK_NUMBER];
    DWORD	dwSMarker[MAXIMUM_TRACK_NUMBER];    
 }stRTPPACKETIZER;
 
#define STATUS_STOP       0
#define STATUS_RUNNING    1

DWORD THREADAPI RTPPacketizer_MainLoop(DWORD hRTPPacketizer);

SCODE RTPPacketizer_Create(HANDLE* phRTPPacketizer,stRTPPACKETIZERPARAM* pRTPPacketizerParam)
{   
    stRTPPACKETIZER* pRTPPacketizer;
    TOSThreadInitOptions ThreadOption;
    int					i;

    if( phRTPPacketizer == 0 || pRTPPacketizerParam == 0)
        return S_FAIL;

    pRTPPacketizer = (stRTPPACKETIZER*)malloc(sizeof(stRTPPACKETIZER));    

	if( pRTPPacketizer != NULL )
		memset((char*)pRTPPacketizer,0,sizeof(stRTPPACKETIZER));
	else
		return S_FAIL;
    
    pRTPPacketizer->hDataQueue = pRTPPacketizerParam->hRTPMediaDataQueue;
    pRTPPacketizer->hEmptyQueue = pRTPPacketizerParam->hRTPMediaEmptyQueue;
    pRTPPacketizer->dwStatus = STATUS_STOP;  
    //modified 20080722 to avoid initialization error
    for( i=0 ; i<MAXIMUM_TRACK_NUMBER ; i++ )
    	pRTPPacketizer->dwSMarker[i] = 1;  
	
    memset((char*)&ThreadOption,0,sizeof(TOSThreadInitOptions));
    ThreadOption.dwStackSize= 0x2000;
    ThreadOption.dwInstance = (DWORD)pRTPPacketizer;
    
	ThreadOption.dwPriority = pRTPPacketizerParam->dwThreadPriority;    
#if defined (_PSOS_TRIMEDIA)
	ThreadOption.dwFlags = T_LOCAL;
    ThreadOption.dwMode = T_PREEMPT|T_ISR;
#endif 
    ThreadOption.pThreadProc = RTPPacketizer_MainLoop;    

    if( OSThread_Initial((HANDLE*)&(pRTPPacketizer->dwThreadID), &ThreadOption) != S_OK )
        return S_FAIL;
    
    PRINTF("=====Packet Thread ID %lu\n",pRTPPacketizer->dwThreadID);
    *phRTPPacketizer = pRTPPacketizer;
    
    return S_OK;
}

SCODE RTPPacketizer_Release(HANDLE *phRTPPacketizer)
{
    stRTPPACKETIZER* pRTPPacketizer;
    DWORD dwExitCode;

    if( *phRTPPacketizer == 0 )
        return S_FAIL;

    pRTPPacketizer = (stRTPPACKETIZER*)*phRTPPacketizer;
    
    /*if( pRTPPacketizer->dwStatus == STATUS_STOP )
    {
    	PRINTF("=====packetizer stopped %lu======= \n",pRTPPacketizer);    	
    } */ 
           
	if( pRTPPacketizer->dwThreadID != 0 )
	{
		/*if (OSThread_WaitFor((void*)pRTPPacketizer->dwThreadID, 5000, &dwExitCode) != S_OK)
		{
			PRINTF("kill thread 2\n");
			OSThread_Terminate((void*)pRTPPacketizer->dwThreadID);
			PRINTF("kill thread 3\n");
		}*/
		
		//PRINTF("before release thread of packetizer %lu\n",pRTPPacketizer->dwThreadID);
		OSThread_Release((HANDLE*)&(pRTPPacketizer->dwThreadID));		
		//PRINTF("after release thread of packetizer\n");
	}
	
	pRTPPacketizer->hDataQueue = NULL;
    pRTPPacketizer->hEmptyQueue = NULL;
 		
    if( pRTPPacketizer != 0 )
    {
    	//PRINTF("before free packetizer %lu\n",pRTPPacketizer);
    	free(pRTPPacketizer);   
    	//PRINTF("after free packetizer\n");
        *phRTPPacketizer = NULL;        
        return S_OK;
    }
    else
        return S_FAIL;
}

/*
	ret: the actual output length in pbyOut
 */
//2007/11/14 modified by Louis for Intelligent video
DWORD RTPPacketizer_ComposeRTPExt(stRTPPACKETIZER* pRTPPacketizer, 
		BYTE *pbyOut, DWORD dwOutLen, BYTE *pbyDP, DWORD dwDPLen, DWORD dwIVLen)
{
	DWORD	dwActulOutLen, dwPadding, dwTmp;
	DWORD	dwLocationLen = strlen(pRTPPacketizer->acLocation);
	BYTE	*pbyUserData = NULL, *pbyIV = NULL;

	if(dwIVLen != 0)
	{
		pbyIV = pbyDP;
		pbyUserData = pbyDP + dwIVLen + 4;
	}
	else
	{
		pbyUserData = pbyDP;
		pbyIV = NULL;
	}

	dwTmp = dwDPLen + (dwDPLen ? 2 : 0) + 
		dwLocationLen + (dwLocationLen ? 2 : 0) + dwIVLen + (dwIVLen ? 2 : 0);
	/* Nothing to be done */
	if (dwTmp == 0)	
	{
		PRINTF("no extension!\n");
		return 0;
	}
	/* Check padding is needed. */
	dwActulOutLen = (dwTmp & 0x3) ? (((dwTmp >> 2) + 1) << 2) : dwTmp;
	dwPadding = dwActulOutLen - dwTmp;
	/* If the output size is not enough, ignore the extension */
	if (dwActulOutLen > dwOutLen)
	{
		TelnetShell_DbgPrint("!!! extension overflow %d\r\n", dwActulOutLen);
		return 0;
	}
	/* feed the zero padding at start place */
	memset(pbyOut, 0, dwPadding);  
	pbyOut += dwPadding;
	/* feed the Location */
	if (dwLocationLen > 0 && dwLocationLen < LOCATION_LEN)
	{
		pbyOut[0] = eRTP_EX_LOCATION;
		pbyOut[1] = (BYTE) dwLocationLen;
		pbyOut += 2;
		memcpy (pbyOut, pRTPPacketizer->acLocation, dwLocationLen);
		pbyOut += dwLocationLen;
	}
	/* feed the Intelligen Video */
	if (dwIVLen)
	{
		//Added for Ben's Debug
		int i = 0;
		printf("IV data: ");
		for (i = 0; i < dwIVLen; i++)
		{
			printf("%02x-", pbyIV[i]);
		}
		printf("\n");

		pbyOut[0] = eRTP_EX_IVAEXTENINFO;
		pbyOut[1] = (BYTE) dwIVLen;
		pbyOut += 2;
		memcpy (pbyOut, pbyIV, dwIVLen);
		pbyOut += dwIVLen;

	}
	
	/* feed the DataPacket */
	if (dwDPLen)
	{
		pbyOut[0] = eRTP_EX_DATAPACKETHEADER;
		pbyOut[1] = (BYTE) dwDPLen;
		pbyOut += 2;
		memcpy (pbyOut, pbyUserData, dwDPLen);
		pbyOut += dwDPLen;
	}

	return dwActulOutLen;
}

SCODE RTPPacketizer_Cut(stRTPPACKETIZER* pRTPPacketizer, TBitstreamBuffer* pBitStreamBuffer)
{
#if defined (_WIN32_)	
    DWORD dwSec, dwMSec;//dwMediaType;
#endif
    
    char* pchBitBuffPtr;
    int   iBitLen;
    RTPMEDIABUFFER* pRTPMediaBuff;
    int iFirstIPacket = 0,iResult;
    int i;
	DWORD	dwPacketNum, dwRemPacketSize, dwTmp, dwExtLen=0;
	BYTE	abyRTPExt[RTP_EXTENSION - 4];
//    TMediaDataPacketInfo 	tDataPacketInfo;
//#ifdef _LINUX
//    static DWORD dwSSec=0, dwSMSec=0, dwSMarker=1;
//#endif    
    if( pBitStreamBuffer->dwBytesUsed <= 0 )
        return S_FAIL;

#if defined (_WIN32_) || defined (_LINUX_X86)
    OSTime_GetTimer(&dwSec, &dwMSec);
#endif    
    
#ifndef _WIN32_
    //get rid of mp4 proprietary header 
#ifndef _LINUX_X86
    //Shengfu 2006/06/28 remove 
    //dwMediaType = DPPAR_GetMediaType(pBitStreamBuffer->dwStreamType);
#endif
#endif

    pchBitBuffPtr = (CHAR*) pBitStreamBuffer->pbyBuffer;
    iBitLen = pBitStreamBuffer->dwBytesUsed;

    if( pBitStreamBuffer->tFrameType == MEDIADB_FRAME_INTRA )
        iFirstIPacket = 1;    

#if defined (_WIN32_) || defined(_LINUX_X86)

	i=0;
    while( iBitLen > 0 )
    {
        pRTPMediaBuff = NULL;
        iResult=MediaBufQueue_GetMediaBuffer(pRTPPacketizer->hEmptyQueue,100,(void **)&(pRTPMediaBuff));
	
        if( iResult==0 && pRTPMediaBuff != NULL )
        {
			i++;

			pRTPMediaBuff->dwExtensionLen = 0;
			if( i== 1)
			{
				pRTPMediaBuff->dwExtensionLen = pBitStreamBuffer->dwOffset;
				memcpy(pRTPMediaBuff->pbBufferStart-pRTPMediaBuff->dwExtensionLen
				   ,pBitStreamBuffer->pbUserData,pRTPMediaBuff->dwExtensionLen);
			}

            if( (DWORD)iBitLen > pRTPMediaBuff->dwBufferLength )
            {   
                memcpy(pRTPMediaBuff->pbBufferStart,pchBitBuffPtr,pRTPMediaBuff->dwBufferLength);
                pRTPMediaBuff->dwBytesUsed = pRTPMediaBuff->dwBufferLength;
                pRTPMediaBuff->bMarker = 0;
            }
            else
            {
                memcpy(pRTPMediaBuff->pbBufferStart,pchBitBuffPtr,iBitLen);
                pRTPMediaBuff->dwBytesUsed = iBitLen;    
                pRTPMediaBuff->bMarker = 1;
            }

            if( iFirstIPacket == 1 )
            {
                pRTPMediaBuff->bIFrame = 1; 
                iFirstIPacket =0;
            }
            else
                pRTPMediaBuff->bIFrame = 0; 

			pRTPMediaBuff->ulSeconds = dwSec;
			pRTPMediaBuff->ulMSeconds = dwMSec;
			pRTPMediaBuff->dwStreamIndex = pBitStreamBuffer->dwStreamIndex;

            pchBitBuffPtr = pchBitBuffPtr + pRTPMediaBuff->dwBufferLength;
            iBitLen = iBitLen - pRTPMediaBuff->dwBufferLength;
            
            if( MediaBufQueue_AddMediaBuffer(pRTPPacketizer->hDataQueue,pRTPMediaBuff) != 0 )
                DbgLog((dfCONSOLE|dfINTERNAL,"[packetizer]enqueue failed\n"));         
        }
    }
/*	i=0;
    while( iBitLen > 0 )
    {
        pRTPMediaBuff = NULL;
        iResult=MediaBufQueue_GetMediaBuffer(pRTPPacketizer->hEmptyQueue,100,(void **)&(pRTPMediaBuff));
        
        if( iResult==0 && pRTPMediaBuff != NULL )
        {
			i++;

			pRTPMediaBuff->dwExtensionLen = 0;
			if( i== 1)
			{
				pRTPMediaBuff->dwExtensionLen = pBitStreamBuffer->dwOffset;
				//memcpy(pRTPMediaBuff->pbBufferStart-pRTPMediaBuff->dwExtensionLen
				//   ,pBitStreamBuffer->pbUserData,pRTPMediaBuff->dwExtensionLen);
				//compose extension of data packet header 
				memcpy(pRTPMediaBuff->pbBufferStart-pRTPMediaBuff->dwExtensionLen
				   ,pchBitBuffPtr,pRTPMediaBuff->dwExtensionLen);
				pchBitBuffPtr += pRTPMediaBuff->dwExtensionLen;
			}

            if( (DWORD)iBitLen > pRTPMediaBuff->dwBufferLength )
            {   
                memcpy(pRTPMediaBuff->pbBufferStart,pchBitBuffPtr,pRTPMediaBuff->dwBufferLength);
                pRTPMediaBuff->dwBytesUsed = pRTPMediaBuff->dwBufferLength;
                pRTPMediaBuff->bMarker = 0;
            }
            else
            {
                memcpy(pRTPMediaBuff->pbBufferStart,pchBitBuffPtr,iBitLen);
                pRTPMediaBuff->dwBytesUsed = iBitLen;
                pRTPMediaBuff->bMarker = 1;
            }

		    TelnetShell_DbgPrint("packetizer %d\r\n",iBitLen);

            if( iFirstIPacket == 1 )
            {
                pRTPMediaBuff->bIFrame = 1; 
                iFirstIPacket =0;
            }
            else
                pRTPMediaBuff->bIFrame = 0; 

			pRTPMediaBuff->ulSeconds = pBitStreamBuffer->dwSecond;
			pRTPMediaBuff->ulMSeconds= pBitStreamBuffer->dwMilliSecond;
            
            pchBitBuffPtr = pchBitBuffPtr + pRTPMediaBuff->dwBufferLength;
            iBitLen = iBitLen - pRTPMediaBuff->dwBufferLength;
            
            if( MediaBufQueue_AddMediaBuffer(pRTPPacketizer->hDataQueue,pRTPMediaBuff) != 0 )
                DbgLog((dfCONSOLE|dfINTERNAL,"[packetizer]enqueue failed\n"));         
        }
    }*/
#else    
	i = 0;
	dwRemPacketSize = 0;
	dwPacketNum = pBitStreamBuffer->pdwPacketSize[0];
		 
	// only when timestamp changed( different frame), RTP extension is made 	
	if( ( ( ( pRTPPacketizer->dwSSec[pBitStreamBuffer->dwStreamIndex] != pBitStreamBuffer->dwSecond ) ||
          ( pRTPPacketizer->dwSMSec[pBitStreamBuffer->dwStreamIndex] != pBitStreamBuffer->dwMilliSecond) ) ) && (pRTPPacketizer->dwSMarker[pBitStreamBuffer->dwStreamIndex] == 1) )
    {
		dwExtLen = RTPPacketizer_ComposeRTPExt(pRTPPacketizer, abyRTPExt, 
				sizeof(abyRTPExt), (BYTE *) pchBitBuffPtr, pBitStreamBuffer->dwOffset, pBitStreamBuffer->dwIntelligentVideoLength);
		pRTPPacketizer->dwSSec[pBitStreamBuffer->dwStreamIndex] = pBitStreamBuffer->dwSecond;
        pRTPPacketizer->dwSMSec[pBitStreamBuffer->dwStreamIndex] = pBitStreamBuffer->dwMilliSecond;			
	}
	else
	{
		//PRINTF("different Ubuffer the same frame\n");
		dwExtLen = 0;			
	}		
		
	//PRINTF("packetizer offset:%d %d\n",pBitStreamBuffer->dwOffset,dwExtLen);
	//Modified by Louis for Intelligent Video! 2007/12/24 for correct data start!
	if(pBitStreamBuffer->dwIntelligentVideoLength == 0)
	{
		pchBitBuffPtr += (pBitStreamBuffer->dwOffset) ;
	}
	else
	{
		pchBitBuffPtr += (pBitStreamBuffer->dwOffset + 4 + pBitStreamBuffer->dwIntelligentVideoLength) ;
	}


	while (1)
    {
		if (dwRemPacketSize == 0)
		{
			if (i >= dwPacketNum)	break;

			i ++;
			dwRemPacketSize = pBitStreamBuffer->pdwPacketSize[i];
		}
        pRTPMediaBuff = NULL;
        iResult = MediaBufQueue_GetMediaBuffer(pRTPPacketizer->hEmptyQueue,100,(void **)&(pRTPMediaBuff));

#ifdef _LINUX        
        if (!(iResult == 0 && pRTPMediaBuff != NULL))
		{
			//syslog(LOG_DEBUG, "%s:%d: [packetizer]dequeue failed", __FILE__, __LINE__);
		}
#endif //_LINUX
        if (iResult == 0 && pRTPMediaBuff != NULL)
        {
//          PRINTF("%s:%d: i = %d, size %d, to %p from %p (%5d, %5d)\n", 
//				__FILE__, __LINE__, i, 
//				*(pBitStreamBuffer->pdwPacketSize+i), 
//				pRTPMediaBuff, pchBitBuffPtr, 
//				pBitStreamBuffer->dwSecond, 
//				pBitStreamBuffer->dwMilliSecond);

			/* Feed the RTP extension legnth field */
			pRTPMediaBuff->dwExtensionLen = dwExtLen;
			if (dwExtLen != 0)
			{
				memcpy(pRTPMediaBuff->pbBufferStart - dwExtLen, 
						abyRTPExt, dwExtLen);
			}	

			/* Maybe the packet is too big to exceed one MTU after add
			   RTP extension, trying to limited the output if the extension
			   is used. */
			dwTmp = pRTPMediaBuff->dwBufferLength - dwExtLen;
			
			if (dwTmp > dwRemPacketSize)
				dwTmp = dwRemPacketSize;					
								
        	memcpy(pRTPMediaBuff->pbBufferStart, pchBitBuffPtr, dwTmp);
        	pRTPMediaBuff->dwBytesUsed = dwTmp;
            pchBitBuffPtr = pchBitBuffPtr + pRTPMediaBuff->dwBytesUsed;
			dwRemPacketSize -= pRTPMediaBuff->dwBytesUsed; 
			dwExtLen = 0;

            if ((i == dwPacketNum) && (dwRemPacketSize == 0))
            {   
            	// Media type audio => 1, video => 0    
            	if( pBitStreamBuffer->dwIsBoundary == TRUE )
            	{
            		pRTPMediaBuff->bMarker = 1;//!dwMediaType;            	
            		
            		pRTPPacketizer->dwSMarker[pBitStreamBuffer->dwStreamIndex] = 1;
            	}
            }
            else
            {
                pRTPMediaBuff->bMarker = 0;
                pRTPPacketizer->dwSMarker[pBitStreamBuffer->dwStreamIndex] = 0;
            }

            if( iFirstIPacket == 1 )
            {
                pRTPMediaBuff->bIFrame = 1; 
                iFirstIPacket =0;
            }
            else
                pRTPMediaBuff->bIFrame = 0; 

			pRTPMediaBuff->ulSeconds = pBitStreamBuffer->dwSecond;
			pRTPMediaBuff->ulMSeconds= pBitStreamBuffer->dwMilliSecond;
			pRTPMediaBuff->dwStreamIndex = pBitStreamBuffer->dwStreamIndex;

            //syslog(LOG_DEBUG, "%s:%d: %u, %u, %d", 
			//		__FILE__, __LINE__, 
			//		pBitStreamBuffer->dwSecond, 
			//		pBitStreamBuffer->dwMilliSecond, 
			//		!dwMediaType);

            
            if( MediaBufQueue_AddMediaBuffer(pRTPPacketizer->hDataQueue,pRTPMediaBuff) != 0 )
			{
#ifdef _LINUX
				//syslog(LOG_DEBUG, "%s:%d: [packetizer]enqueue failed", __FILE__, __LINE__);
#endif //_LINUX
			}
        }
    }
#endif
	return S_OK;
}


DWORD THREADAPI RTPPacketizer_MainLoop(DWORD hRTPPacketizer)
{
    stRTPPACKETIZER*    pRTPPacketizer;
    TBitstreamBuffer*   pBitStreamBuffer = NULL;
    int                 iResult;
    
    pRTPPacketizer = (stRTPPACKETIZER*)hRTPPacketizer;

#ifdef _LINUX
	//syslog(LOG_INFO, "[RTP Packetizer] pid is %d\n", getpid());
#endif //_LINUX

	// modified by cchuang 2005/05/23
    while (!pRTPPacketizer->bTerminateThread)
    {
        pBitStreamBuffer = NULL;
        // request for media frame
        iResult = pRTPPacketizer->fRTPPacketizerCallback(pRTPPacketizer->hParentHandle
                                               , RTPPACKETIZER_CALLBACK_REQUESTBUFFER
                                               , (void*) 100
                                               , (void*)&(pBitStreamBuffer));
                                                     
        if( iResult == S_OK && pBitStreamBuffer != NULL )
        {
            RTPPacketizer_Cut(pRTPPacketizer,pBitStreamBuffer);   
            pRTPPacketizer->fRTPPacketizerCallback(pRTPPacketizer->hParentHandle
                                               , RTPPACKETIZER_CALLBACK_RETURNBUFFER
                                               , (void*) 100
                                               , (void*)(&pBitStreamBuffer));
        }
    }
    pRTPPacketizer->dwStatus = STATUS_STOP;
	return S_OK;
}

SCODE RTPPacketizer_Start(HANDLE hRTPPacketizer)
{
    stRTPPACKETIZER* pRTPPacketizer;
    
    if( hRTPPacketizer == NULL )
        return S_FAIL;        

    pRTPPacketizer = (stRTPPACKETIZER*)hRTPPacketizer;
	// Added by cchuang 2005/05/23
	pRTPPacketizer->bTerminateThread = FALSE;
    pRTPPacketizer->dwStatus = STATUS_RUNNING;
    
    if( OSThread_Start((HANDLE)pRTPPacketizer->dwThreadID) != S_OK)
        return S_FAIL;

    return S_OK;
}

SCODE RTPPacketizer_STOP(HANDLE hRTPPacketizer)
{
    stRTPPACKETIZER* pRTPPacketizer;
    int i=0;
 
    if( hRTPPacketizer == NULL )
        return S_FAIL;        

    pRTPPacketizer = (stRTPPACKETIZER*)hRTPPacketizer;

	
	// modified by cchuang, 2005/05/23
    pRTPPacketizer->bTerminateThread = TRUE;
    while (pRTPPacketizer->dwStatus != STATUS_STOP)
    {		
        //pRTPPacketizer->dwStatus = STATUS_STOP;
        OSSleep_MSec(20);
        //OSSleep_Sec(1);
        i++;
        if( i > 20 )
        {        	
        	break;
        }
    }
    
    return S_OK;
}

SCODE RTPPacketizer_SetCallBakck(HANDLE hRTPPacketizer, RTPPACKETIZERCALLBACK fCallback, HANDLE hParentHandle)
{
    stRTPPACKETIZER* pRTPPacketizer;

    if( hRTPPacketizer == NULL || fCallback == NULL)
        return S_FAIL;        
 
    pRTPPacketizer = (stRTPPACKETIZER*)hRTPPacketizer;
    pRTPPacketizer->fRTPPacketizerCallback = fCallback;
    pRTPPacketizer->hParentHandle = hParentHandle;
    
    return S_OK;
}
 
SCODE RTPPacketizer_SetLocation(HANDLE hRTPPacketizer, char* pcLocation)
{
    stRTPPACKETIZER* pRTPPacketizer;

    TelnetShell_DbgPrint("Location: %s\r\n",pcLocation);

    if( hRTPPacketizer == NULL || pcLocation == NULL)
        return S_FAIL;        
    
    if( strlen(pcLocation) > LOCATION_LEN ) return S_FAIL;
    
    pRTPPacketizer = (stRTPPACKETIZER*)hRTPPacketizer;
    rtspstrcpy(pRTPPacketizer->acLocation,pcLocation, sizeof(pRTPPacketizer->acLocation));    

    return 0;
}

