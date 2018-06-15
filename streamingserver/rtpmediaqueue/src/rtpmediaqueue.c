/*
 *  Copyright (c) 2000 Vivotek Inc. All rights reserved.
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
 *  Module name         :   MediaBufferQueue.c
 *  Module description  :   Maintain the media buffer by FIFO character
 *  Author              :   Simon
 *  Created at          :   2002/04/22
 *  Revision            :   1.0
 ******************************************************************************
 *                        Revision history
 ******************************************************************************
 */

#include <stdlib.h>
#include <string.h>
#include "rtpmediaqueue.h"

typedef struct tagMEDIABUFFERQUEUE
{
	unsigned long ulMediaBufferQueue; 
	int	   nMaximumMediaBufferNumber;
	int    nMediaBufferCount;
	
}MEDIABUFFERQUEUE;


HANDLE MediaBufQueue_Create(int nMaximumBufferNum)
{
	unsigned long ulResult;
	MEDIABUFFERQUEUE *pMediaBufferQueue= (MEDIABUFFERQUEUE *)malloc(sizeof(MEDIABUFFERQUEUE));

	if(!pMediaBufferQueue)
		return (HANDLE)NULL;
	//CID:398 RESOURCE LEAK
	if(nMaximumBufferNum<=0)
	{
		free(pMediaBufferQueue);
		pMediaBufferQueue = NULL;
		return (HANDLE)NULL;
	}
		
	pMediaBufferQueue->nMaximumMediaBufferNumber=nMaximumBufferNum;
	pMediaBufferQueue->nMediaBufferCount=0;
	pMediaBufferQueue->ulMediaBufferQueue=0;

    ulResult=OSMsgQueue_Initial((HANDLE*)&pMediaBufferQueue->ulMediaBufferQueue, nMaximumBufferNum);	
    //PRINTF("ulMediaBufferQueue = %lu\n",pMediaBufferQueue->ulMediaBufferQueue);
//	ulResult=q_create("MBuf", nMaximumBufferNum, Q_LOCAL | Q_FIFO | Q_LIMIT, &pMediaBufferQueue->ulMediaBufferQueue);
	
	if(ulResult)
	{
		free(pMediaBufferQueue);
		return (HANDLE)NULL;
	}
	
	return (HANDLE)pMediaBufferQueue;
}




int MediaBufQueue_Delete(HANDLE hMediaBufferQueue)
{
	MEDIABUFFERQUEUE *pMediaBufferQueue;
	unsigned long ulResult;
	
	if(!hMediaBufferQueue)
		return MEDIABUFQUEUE_ERR_INVALID_HANDLE; //-1;
	
	pMediaBufferQueue=(MEDIABUFFERQUEUE *)hMediaBufferQueue;	
	
/*	if(pMediaBufferQueue->nMediaBufferCount>=0)
	{
		return MEDIABUFQUEUE_ERR_MEDIABUF_NOTEMPTY; //-1;
	}
	else*/
	{
//		ulResult=q_delete(pMediaBufferQueue->ulMediaBufferQueue);
//PRINTF("before ulMediaBufferQueue %lu\n",pMediaBufferQueue->ulMediaBufferQueue);

        ulResult=OSMsgQueue_Release((HANDLE*)&pMediaBufferQueue->ulMediaBufferQueue);
//PRINTF("after ulMediaBufferQueue\n");
        
		if(ulResult)
		{
			return MEDIABUFQUEUE_ERR_DELETEQUEUE_FAILURE;
		}
		
		pMediaBufferQueue->ulMediaBufferQueue=0;
		free(pMediaBufferQueue);
		return 0;
	}
}



int MediaBufQueue_GetMediaBufCount(HANDLE hMediaBufferQueue)
{
	MEDIABUFFERQUEUE *pMediaBufferQueue;
	
	if(!hMediaBufferQueue)
		return MEDIABUFQUEUE_ERR_INVALID_HANDLE; //-1;
	
	pMediaBufferQueue=(MEDIABUFFERQUEUE *)hMediaBufferQueue;	
	
	return pMediaBufferQueue->nMediaBufferCount;

}


int MediaBufQueue_AddMediaBuffer(HANDLE hMediaBufferQueue, void *pvMediaBuffer)
{
	MEDIABUFFERQUEUE *pMediaBufferQueue;
	unsigned long aulMsgBuf[4];
	SCODE			 sRet;
	
	if(!hMediaBufferQueue)
		return MEDIABUFQUEUE_ERR_INVALID_HANDLE; 
	
	pMediaBufferQueue=(MEDIABUFFERQUEUE *)hMediaBufferQueue;	
	
//	if(pMediaBufferQueue->nMediaBufferCount >= pMediaBufferQueue->nMaximumMediaBufferNumber)
//		return MEDIABUFQUEUE_ERR_QUEUEFULL; 
	
	memset(aulMsgBuf,0,4*sizeof(unsigned long));	
	aulMsgBuf[0]=(unsigned long)pvMediaBuffer;

	pMediaBufferQueue->nMediaBufferCount++;

    sRet=OSMsgQueue_Send((HANDLE)pMediaBufferQueue->ulMediaBufferQueue,(DWORD*)aulMsgBuf);	
//	ulResult=q_send(pMediaBufferQueue->ulMediaBufferQueue,aulMsgBuf);
	
	if(sRet != S_OK)
	{
		pMediaBufferQueue->nMediaBufferCount--;
		return MEDIABUFQUEUE_ERR_ADDQUEUE_FAILURE;  
	}
	else
	{
		return 0;			
	}
}	



int MediaBufQueue_GetMediaBuffer(HANDLE hMediaBufferQueue, int nTimeOutInMilliSec, void ** ppvMediaBuffer)
{
	MEDIABUFFERQUEUE *pMediaBufferQueue;
	unsigned long aulMsgBuf[4];
	unsigned long ulResult;
	
	if(!hMediaBufferQueue)
		return MEDIABUFQUEUE_ERR_INVALID_HANDLE; //-1;
	
	pMediaBufferQueue=(MEDIABUFFERQUEUE *)hMediaBufferQueue;	


	if( nTimeOutInMilliSec == 0 )
	{
//		ulResult=q_receive(pMediaBufferQueue->ulMediaBufferQueue,Q_NOWAIT,0,aulMsgBuf);			
        ulResult=OSMsgQueue_Receive((HANDLE)pMediaBufferQueue->ulMediaBufferQueue,(DWORD*)aulMsgBuf,0);		
	}
	else if( nTimeOutInMilliSec < 0 )
	{
//		ulResult=q_receive(pMediaBufferQueue->ulMediaBufferQueue,Q_WAIT,0,aulMsgBuf);			
        ulResult=OSMsgQueue_Receive((HANDLE)pMediaBufferQueue->ulMediaBufferQueue,(DWORD*)aulMsgBuf,INFINITE);		
	}
	else
	{
//		ulTimeOutInTick=(nTimeOutInMilliSec*KC_TICKS2SEC)/1000;
//		ulResult=q_receive(pMediaBufferQueue->ulMediaBufferQueue,Q_WAIT,ulTimeOutInTick,aulMsgBuf);			
        ulResult=OSMsgQueue_Receive((HANDLE)pMediaBufferQueue->ulMediaBufferQueue,(DWORD*)aulMsgBuf,nTimeOutInMilliSec);		
	}
		
	if(ulResult==0)
	{
		if(aulMsgBuf[0])
		{
			*ppvMediaBuffer=(void *)aulMsgBuf[0];
		}
		else
		{
			*ppvMediaBuffer=0;
		}
		pMediaBufferQueue->nMediaBufferCount--;

		return 0;
	}
/*	else if(ulResult==0x01)
	{
		*ppvMediaBuffer=0;
		return MEDIABUFQUEUE_ERR_GETQUEUE_TIMEOUT;
	}*/
	else
	{
		*ppvMediaBuffer=0;
		return MEDIABUFQUEUE_ERR_GETQUEUE_FAILURE;
	}


}	

//---------------------------------------------------------------
/*
#include <stdio.h>
#include "mediabuf.h"

MEDIABUFFER * TestAllocateMediaBuffer(int nBufferSize)
{

	MEDIABUFFER *pNextBuffer=NULL;

	int nSize=sizeof(MEDIABUFFER);
	pNextBuffer=(MEDIABUFFER *)malloc(nSize+nBufferSize);
	memset(pNextBuffer,0,nSize+nBufferSize);
	if(pNextBuffer)
	{	
		pNextBuffer->pbBufferStart=pNextBuffer->pbDataStart=(BYTE *)(pNextBuffer+1);
		pNextBuffer->dwBufferLength=nBufferSize;
	} 
	return pNextBuffer;
}



void BufferTest()
{
	MEDIABUFFER *pMediaBuffer;
	HANDLE	hBufferQueue;
	int nResult,i;
	void * pvTemp;
	
	hBufferQueue=MediaBufQueue_Create(5);
	PRINTF(" \n Add buffer hBufferQueue=%d  ",hBufferQueue);
	
	
	for(i=0;i<6;i++)
	{
		pMediaBuffer=TestAllocateMediaBuffer(512);

		nResult=MediaBufQueue_AddMediaBuffer(hBufferQueue, (void *)pMediaBuffer);
		PRINTF(" \n Add buffer result=%d Buffer=%d ",nResult,pMediaBuffer);
	}
	
	tm_wkafter(100);
	
	nResult=MediaBufQueue_GetMediaBufCount(hBufferQueue);
	PRINTF(" \n Count=%d ",nResult);

	
	for(i=0;i<20;i++)
	{
		pMediaBuffer=NULL;
		
		nResult=MediaBufQueue_GetMediaBuffer(hBufferQueue, 1000, (void **)(&pMediaBuffer));
		PRINTF(" \n get buffer result=%d Buffer=%d ",nResult,pMediaBuffer);
	
		nResult=MediaBufQueue_GetMediaBufCount(hBufferQueue);
		PRINTF(" \n Count=%d ",nResult);
		
		nResult=MediaBufQueue_AddMediaBuffer(hBufferQueue, (void *)pMediaBuffer);
		PRINTF(" \n Add buffer result=%d Buffer=%d ",nResult,pMediaBuffer);
		
	}
	
	
	return;
}*/
