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
 * rtsps_ubuffer.c
 *
 * \brief
 * UBuffer reader for rtsp streaming server.
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

#include <sys/types.h>
#include <sys/stat.h>
#include "rtsps_ubuffer.h"


int create_unix_socket(const char *path)
{
	int fd;
	struct sockaddr_un sunx;

	if (path[0] == '\0')
		return -1;

	(void) unlink(path);

	memset(&sunx, 0, sizeof(sunx));
	sunx.sun_family = AF_UNIX;
	//CID:11, CHECKER:BUFFER_SIZE_WARNING
	(void) strncpy(sunx.sun_path, path, sizeof(sunx.sun_path) - 1);
	fd = socket(AF_UNIX, SOCK_DGRAM, 0);
	//CID:156, CHECKER:NEGATIVE_RETURNS
	if( fd < 0)
	{
		return -1;
	}
	if (bind(fd, (struct sockaddr *) &sunx, sizeof(sunx.sun_family)+strlen(sunx.sun_path)) < 0 || chmod(path, 0666) < 0) 
	{
		printf("cannot create unix socket %s (%d).%s\n", path, errno, strerror(errno));
		close(fd);
		return -1;
	}
	return fd;
}

SCODE GetVideoUBuffer(TSTREAMSERVERINFO *pThis,int *piMediaTrackIndex)
{
	fd_set	readfds;
	struct  timeval timeout;
	int     iResult;
	int		i,iMaxSock = 0;
	static int	iPreviousSelect = -1;
	
	/* select */
	timeout.tv_sec = 0;
	timeout.tv_usec = 100;  
		
	FD_ZERO(&readfds);
	for( i=0 ; i<VIDEO_TRACK_NUMBER; i++)
	{
		if( pThis->tVideoSrcInfo[i].iFdSock > 0 )
			FD_SET(pThis->tVideoSrcInfo[i].iFdSock, &readfds);
			
		if( pThis->tVideoSrcInfo[i].iFdSock > iMaxSock )
			iMaxSock = pThis->tVideoSrcInfo[i].iFdSock ;  
	}
	
	iResult = select(iMaxSock + 1, &readfds, NULL, NULL, &timeout);

	//20091118 handle select error
	if( iResult < 0 )
	{
		printf("Select returned -1\n");
		syslog(LOG_ERR, "[RTSP_APP] select return -1!!\n");
		StreamSvr_CheckUnixSocket(pThis);
		return S_FAIL;
	}
	else if (iResult == 0)	/* Not selected */
	{
		return S_FAIL;
	}

	// avoid each time we always select to read the same socket
	if( iPreviousSelect == VIDEO_TRACK_NUMBER - 1 )
		iPreviousSelect = -1;
		 
	for( i=0; i<VIDEO_TRACK_NUMBER; i++)
	{
		if( pThis->tVideoSrcInfo[i].iFdSock < 0 )
			continue;
			
		if( (FD_ISSET(pThis->tVideoSrcInfo[i].iFdSock,&readfds)) &&
		    ( (iResult == 1 )||(i > iPreviousSelect) ))
		{
			if( i > iPreviousSelect )
				iPreviousSelect = i;
				
			if( read(pThis->tVideoSrcInfo[i].iFdSock, pThis->pbyVideUBuffer, VIDEO_UBUFFER_SIZE) <= 0 )
			{
 		    	return S_FAIL;
			}   
			else
			{
				*piMediaTrackIndex= i;
				return S_OK;
			}
		}
	}			
	
	return S_FAIL;
}

SCODE GetAudioUBuffer(TSTREAMSERVERINFO *pThis,int *piMediaTrackIndex)
{
	fd_set	readfds;
	struct  timeval timeout;
	int     iResult,iSize=0;
	int		i,iMaxSock = 0;
	static int	iPreviousSelect = -1;
	
	/* select */
	timeout.tv_sec = 0;
	timeout.tv_usec = 100; 
		
	FD_ZERO(&readfds);
	for( i=0 ; i<AUDIO_TRACK_NUMBER; i++)
	{
		if( pThis->tAudioSrcInfo[i].iFdSock > 0 )
			FD_SET(pThis->tAudioSrcInfo[i].iFdSock, &readfds);
			
		if( pThis->tAudioSrcInfo[i].iFdSock > iMaxSock )
			iMaxSock = pThis->tAudioSrcInfo[i].iFdSock ;  
	}
	
	iResult = select(iMaxSock + 1, &readfds, NULL, NULL, &timeout);
	
	//20091118 handle select error
	if( iResult < 0 )
	{
		StreamSvr_CheckUnixSocket(pThis);
		return S_FAIL;
	}
	else if (iResult == 0)	/* Not selected */
	{
		return S_FAIL;
	}
		
	// avoid each time we always select to read the same socket
	if( iPreviousSelect == AUDIO_TRACK_NUMBER - 1 )
		iPreviousSelect = -1;
		 
	for( i=0; i<AUDIO_TRACK_NUMBER; i++)
	{
		if( pThis->tAudioSrcInfo[i].iFdSock < 0 )
			continue;
			
		if( (FD_ISSET(pThis->tAudioSrcInfo[i].iFdSock,&readfds)) &&
		    ( (iResult == 1 )||(i > iPreviousSelect) ))
		{
			if( i > iPreviousSelect )
				iPreviousSelect = i;
				
			if( (iSize = read(pThis->tAudioSrcInfo[i].iFdSock, pThis->pbyAudiUBuffer, AUDIO_UBUFFER_SIZE)) <= 0 )
			{
 		    	return S_FAIL;
			}   
			else
			{
				//printf("%d bytes from audio socket\n",iSize);
				*piMediaTrackIndex = i;
				return S_OK;
			}
		}
	}			
	
	return S_FAIL;	
}

SCODE initClientSocket(int *piFd)
{
	int	sck;

	/* Create the unix socket */
	sck = socket(AF_UNIX, SOCK_DGRAM, 0);
	
	if( sck > 0)
	{
		fcntl(sck, F_SETFL, O_NONBLOCK);
		*piFd = sck;
		return S_OK;
	}
	else
		return S_FAIL;
}

SCODE connectClientSocket(int fdOut, const char *szSckName)
{
	struct	sockaddr_un	sunx;
	int iRetry ;

	if( fdOut < 0)
		return S_FAIL;

	printf("connect path = %s \n",szSckName);
	memset(&sunx, 0, sizeof(sunx));
	sunx.sun_family = AF_UNIX;
	//(void) strncpy(sunx.sun_path, szSckName, strlen(szSckName));
	//CID:1026, CHECKER:STRING_OVERFLOW
	strncpy(sunx.sun_path, szSckName, sizeof(sunx.sun_path) - 1);
	for( iRetry = 0; iRetry < 1 ;iRetry++)
	{
		if (connect(fdOut, (struct sockaddr *)&sunx, 
			sizeof(sunx.sun_family) + strlen(sunx.sun_path)) < 0)
		{
			//fprintf(stderr, "%s:%d: socket connect failed. Retry %d time(s)\n", __FILE__, __LINE__, iRetry+1);
			//usleep(500000);
		}
		else
		{
			return S_OK;
		}

	}
	perror("UNIX socket connect failed!");
	return S_FAIL;
}

int writeClientSocket(int fdOut, BYTE *abUBuffer, DWORD dwWriteSize)
{
	int iRet;
	int iErrno;
	
	iRet = write(fdOut, abUBuffer, dwWriteSize);
	
	if( iRet < 0 )
	{
		iErrno = errno;
		printf("write socket error,%s\n" , strerror(errno));
		if( iErrno == EWOULDBLOCK)
			return 1;
		else
			return -1;
	}
	return 0;
}


/*
DWORD VideUBufReader(DWORD dwInstance)
{
	int i;
	TRTSPSInfo *pThis;

	pThis = (TRTSPSInfo *) dwInstance;

	while (pThis->bRunning)
	{
		BOOL bTestFlag = TRUE;
		
		sem_wait(&pThis->semWriteVideData);
		for (i=0; i<pThis->dwStreamNumber; i++)
		{
			if (pThis->bVideUBufEmpty[i] == TRUE)
			{
				if (GetUBuffer(pThis->tVideoSrcInfo[i].iFdSock, pThis) == S_OK)
				{
					read(pThis->tVideoSrcInfo[i].iFdSock, pThis->pbyVideUBuffer[i], VIDEO_UBUFFER_SIZE);
					pThis->bVideUBufEmpty[i] = FALSE;
				}
			}
		}
		for (i=0; i<pThis->dwStreamNumber; i++)
		{
			bTestFlag &= pThis->bVideUBufEmpty[i];
		}
		if (bTestFlag == TRUE)
		{			
			sem_post(&pThis->semWriteVideData);
		}
		else
		{
			sem_post(&pThis->semVideDataReady);
		}
	}	
	
	return 0;
}


DWORD AudiUBufReader(DWORD dwInstance)
{
	int i;
	TRTSPSInfo *pThis;

	pThis = (TRTSPSInfo *) dwInstance;

	while (pThis->bRunning)
	{
		BOOL bTestFlag = TRUE;
		// waiting for StreamSvrVideoCallback's MEDIA_CALLBACK_RELEASE_BUFFER 
		// then we can read new ubuffer from socket 
		sem_wait(&pThis->semWriteAudiData);
		for (i=0; i<pThis->dwStreamNumber; i++)
		{
			if (pThis->bAudiUBufEmpty[i] == TRUE)
			{
				if (GetUBuffer(pThis->tAudioSrcInfo[i].iFdSock, pThis) == S_OK)
				{
					read(pThis->tAudioSrcInfo[i].iFdSock, pThis->pbyAudiUBuffer[i], AUDIO_UBUFFER_SIZE);
					pThis->bAudiUBufEmpty[i] = FALSE;
				}
			}
		}
		for (i=0; i<pThis->dwStreamNumber; i++)
		{
			bTestFlag &= pThis->bAudiUBufEmpty[i];
		}
		if (bTestFlag == TRUE)
		{
			// read nothing 
			sem_post(&pThis->semWriteAudiData);
		}
		else
		{
			sem_post(&pThis->semAudiDataReady);
		}
	}	

	return 0;
}
*/
