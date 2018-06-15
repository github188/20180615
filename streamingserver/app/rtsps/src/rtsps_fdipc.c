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
 * rtsps_fdipc.c
 *
 * \brief
 * Exchange file descriptor between http server and rtsp server.
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

#include <sys/stat.h>
#include <string.h>
#include "rtspstreamingserver.h"
#include "fdipc.h"
#include "rtsps_fdipc.h"
#include "message.h"

//20110401 Added by danny For support RTSPServer thread watchdog
//20140605 Modified by Charles to let swatchdog restart the server when RTSPServer thread is lock
SCODE CheckRTSPServerThreadAlive(HANDLE hObject)
{
	TSTREAMSERVERINFO *pThis;
	DWORD			  dwSec;
	//char 			  szCmd[64];
	//int			  iLength;

	pThis = (TSTREAMSERVERINFO *) hObject;
	DWORD dwRTSPServerLastKickTemp = pThis->dwRTSPServerLastKick;
	OSTime_GetTimer(&dwSec, NULL);
	
	if ( (dwSec > dwRTSPServerLastKickTemp) && ((dwSec - dwRTSPServerLastKickTemp) > RTSPSERVER_NOKICK_TIMEOUT) && (pThis->bRTSPServerRestarting == FALSE) )
	{
		pThis->bRTSPServerRestarting = TRUE;
		//iLength = snprintf(szCmd, sizeof(szCmd) - 1, "set Value 99 0 0\nnetwork_rtsp_restart=1");
		//szCmd[iLength] = '\0';
		//printf("[%s] szCmd=%s\n",__FUNCTION__, szCmd);
		//StreamingServer_SetToConfigure(szCmd);
		printf("RTSPServer thread lock %d(s), restart RTSP server\n", RTSPSERVER_NOKICK_TIMEOUT);
		syslog(LOG_ERR, "RTSPServer thread lock %d(s), restart RTSP server!\n", RTSPSERVER_NOKICK_TIMEOUT);
		return S_FAIL;
	}

	return S_OK;
}

SCODE RTSPSSetupFdIPCSocket(HANDLE hObject)
{
	TSTREAMSERVERINFO *pThis;
	int iNonBlocking = 1;

	pThis = (TSTREAMSERVERINFO *) hObject;

	remove(pThis->tRTSPInfo.szHTTPfdipcSock);
	if ((pThis->iSDPSocket = fdipc_server_socket(pThis->tRTSPInfo.szHTTPfdipcSock, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)) == -1)
	{
		printf("create fdipc server fail\n");
		return S_FAIL;
	}
	//YenChun 20111208, move out from fdipc_server_socket()
	if (ioctl(pThis->iSDPSocket, FIONBIO, &iNonBlocking) != 0)
	{
		printf("ioctl() socket error %s\n", strerror(errno));
		close(pThis->iSDPSocket);
		pThis->iSDPSocket = -1;
		return S_FAIL;
	}

	return S_OK;
}

#define QUERY_STRING_HEADER_INDEX	2
#define METHOD_HEADER_INDEX			3

SCODE RTSPHandleRTSPOverHTTPHandleMethod(TStreamServer_ConnectionSettings *ptSSConnSettings, const char* szMethod, const char* szQueryString)
{
	if(strstr(szMethod, "GET"))
	{
		/* GET */
		ptSSConnSettings->iHTTPMethod = SS_HTTPMOTHOD_GET;
		ptSSConnSettings->dwRecvLength = 0;
		printf("HTTP GET message received\n");
	}
	else if(strstr(szMethod, "POST"))
	{
		char tmpbuf[GENERAL_BUFFER_LENGTH];
		rtspstrcpy(tmpbuf, szQueryString, sizeof(tmpbuf));
		/* POST */
		ptSSConnSettings->iHTTPMethod = SS_HTTPMOTHOD_POST;
		
		if (strstr(szMethod, "POST1"))
		{
			/* get describe info */
			char *tok = strtok(tmpbuf, "&");
			snprintf(ptSSConnSettings->pcRecvBuffer, sizeof(ptSSConnSettings->pcRecvBuffer), "%s", tok);
			ptSSConnSettings->dwRecvLength = strlen(ptSSConnSettings->pcRecvBuffer);
			printf("ptSSConnSettings->dwRecvLength = %u\n", ptSSConnSettings->dwRecvLength);
			printf("ptSSConnSettings->pcRecvBuffer = %s\n", ptSSConnSettings->pcRecvBuffer);
		}
	}
	else if(strstr(szMethod, "&DESCRIBE"))
	{
		/* DESCRIBE */
		ptSSConnSettings->iHTTPMethod = SS_HTTPMOTHOD_DESCRIBE;

		int iStringLength = strlen(szQueryString);
		strncpy(ptSSConnSettings->pcRecvBuffer, szQueryString, iStringLength);
		ptSSConnSettings->pcRecvBuffer[iStringLength] = '\0';
		ptSSConnSettings->dwRecvLength = iStringLength;
		printf("DESCRIBE command with length of %d\n",ptSSConnSettings->dwRecvLength);
	}
	else
	{
		printf("invalid method from BOA!\n");
		
		return S_FAIL;
	}
	return S_OK;
}

SCODE RTSPHandleRTSPOverHttpSocketParseCookie(char* szSessionCookie, const char* szQueryString)
{
	printf("%s szQueryString = %s\n", __func__, szQueryString);

	char szTmpQueryString[GENERAL_BUFFER_LENGTH];
	memcpy(szTmpQueryString, szQueryString, GENERAL_BUFFER_LENGTH);


	char * tok = strstr(szTmpQueryString, "http_x_sessioncookie");
	if(tok == NULL)
	{
		printf("http_x_sessioncookie not found in Boa Fdipc message!\n");
		return S_FAIL;
	}

	tok = (char *)strtok(tok, "=");
	tok = (char *)strtok(NULL, "&");
	snprintf(szSessionCookie, sizeof(szSessionCookie), "%s", tok);
	return S_OK;
}

void RTSPHandleRTSPOverHTTPSocket(TSTREAMSERVERINFO *pThis,	TStreamServer_ConnectionSettings *ptSSConnSettings)
{
	int iRet;
	int iClientSck;
	char buf[GENERAL_BUFFER_LENGTH];
	int iReadSz;
	
		
	iReadSz = fdipc_recv(pThis->iSDPSocket, (unsigned char *)buf, GENERAL_BUFFER_LENGTH, &iClientSck);
		
    if( iReadSz > 0 )
    	printf("Got Something from BOA %d!!\r\n",iReadSz);
    else
    {
        return;
    }  
    char *pQueryString = NULL;
    int iSplitPos = 0;
    int iIovIndex = 0;
    while(iSplitPos < iReadSz)
    {
		iIovIndex++;
		switch(iIovIndex)
		{
			case QUERY_STRING_HEADER_INDEX:
			{
				/* find http_x_sessioncookie */
				pQueryString = buf + iSplitPos;
				break;
			}
			case METHOD_HEADER_INDEX:
			{
				if(RTSPHandleRTSPOverHTTPHandleMethod(ptSSConnSettings, buf + iSplitPos, pQueryString) != S_OK)
				{
					closesocket(iClientSck);
					return ; //FIXME without error handle?
				}
				break;
			}
			default:
				break;
		}

		iSplitPos += strlen(&buf[iSplitPos]) + 1;
    }

    char szSessionCookie[GENERAL_BUFFER_LENGTH];
    memset(szSessionCookie, 0, GENERAL_BUFFER_LENGTH);
    if(ptSSConnSettings->iHTTPMethod != SS_HTTPMOTHOD_DESCRIBE)
    {
    	if(RTSPHandleRTSPOverHttpSocketParseCookie(szSessionCookie, pQueryString) != S_OK)
    	{

    		printf("%s without session cookie pQueryString = %s\n", __func__, pQueryString);
    		closesocket(iClientSck);
    		return ; //FIXME without error handle?
    	}
    }
    

	/* replace '\0' with ' ' */
	// for (i=0; i<iReadSz; i++)
	// {
	// 	if (buf[i] == '\0')
	// 	{
	// 		buf[i] = ' ';
	// 	}
	// }
	// printf("buf = \n%s\n------------------------end------------------\n", buf);
	//20140418 modified by Charles for http port support rtsp describe command


	
	/* rtp over http */
	ptSSConnSettings->sckControl = iClientSck;
	ptSSConnSettings->dwPrivilege = 143;
	ptSSConnSettings->dwConnectionID = 0;
	ptSSConnSettings->pszSessionCookie = szSessionCookie;
	
	if ( (iRet = RTSPStreaming_AddRTPOverHTTPSock(pThis->hRTSPServer, ptSSConnSettings)) != S_OK)
	{
		printf("RTSPStreaming_AddRTPOverHTTPSock fail, iRet=%d\n", iRet);
		closesocket(iClientSck);
	}
//#endif
	return ;
}

int ReadAndParseControlMessage(int iFD,int iLength,char *pcBuffer)
{
	int	iSelect, iResult, iReceived, iExpected;
	fd_set  SockfdInSelect;
  	struct timeval  tTime;
  	char *pcStart=NULL, *pcEnd=NULL;
  	
  	iReceived = 0;
  	iExpected = iLength;
  	
  	while(1)
  	{
		FD_ZERO( &SockfdInSelect );
  		FD_SET( iFD, &SockfdInSelect);

		tTime.tv_sec = 0 ;
		tTime.tv_usec = 100 ;

		iSelect = select( iFD + 1, &SockfdInSelect, NULL, NULL, &tTime);

		if( iSelect > 0 )
		{
			//printf("select something again!\n");
			iResult = read(iFD,pcBuffer+iReceived,iExpected-iReceived); 
			if(iResult < 0 )
			{
				printf("Error of receiving Control FIFO %s\n",strerror(errno));
				break;
			}
			iReceived = iReceived + iResult;
			if( iReceived == iExpected )
			{
				printf("%d bytes received\n",iReceived);
				pcStart = strstr(pcBuffer,"<location>");
				
				if( pcStart != NULL ) 
				{
					pcStart = pcStart + strlen("<location>");
					pcEnd = strstr(pcBuffer,"</location>");
					
					if( pcEnd != NULL )
					{
						memcpy(pcBuffer,pcStart,pcEnd-pcStart);
						pcBuffer[pcEnd-pcStart] = 0;
						return 0;
					}					
					else
					{
						break;
					}
				}
				else
					break;
			}
		}
	}
	return -1;
}

SCODE	InitializeSWatchDog(HANDLE hRTSPS)
{
	TSTREAMSERVERINFO		*pThis;
	TMessageInfo			tMsgInfo;
	TMessageUtilOption      tMsgUtilInfo;
	char					acBuffer[WATCHDOG_BUFFER_SIZE];
	char					*pcValue = NULL;
	int						iBufferLength = 0;
	SCODE					scResult;

	pThis = (TSTREAMSERVERINFO *)hRTSPS;
	//20090713, We should check the watchlist files
	pcValue = XMLSParser_ReadContent(WATCHDOG_WATCHLIST, "/swatchdog/rtsps/id");
	if(pcValue != NULL)
	{
		pThis->iSWatchDogID = atoi(pcValue);
		if(pThis->iSWatchDogID <= 0 || pThis->iSWatchDogID > WATCHDOG_MAX_ID)
		{
			return S_FAIL;
		}
	}
	else
	{
		return S_FAIL;
	}

	memset(&tMsgInfo, 0, sizeof(tMsgInfo));
	memset(acBuffer, 0 , WATCHDOG_BUFFER_SIZE);
	tMsgInfo.iIndex = pThis->iSWatchDogID;
	tMsgInfo.iType = cmEnrollWatchDog;
	iBufferLength = WATCHDOG_BUFFER_SIZE - 1;

    scResult = Message_Compose_Control(acBuffer, &iBufferLength, 1, &tMsgInfo);
    //printf("\tcompose control result = %d, length = %d\n", scResult, iBufferLength);
    //printf("\t%s\n", acBuffer+2);

	tMsgUtilInfo.iControlType = 0;
	sprintf(tMsgUtilInfo.szSocketPath, "%s", WATCHDOG_SVR_SCK);
	tMsgUtilInfo.iBufLenOut = iBufferLength;
	memcpy(tMsgUtilInfo.pcBuffer, acBuffer, iBufferLength);

	scResult = Message_Util_SendbySocket(&tMsgUtilInfo);

	if(scResult == S_OK)
	{
		pThis->bSWatchDogEnabled = TRUE;
		printf("SWatchDog Initialized with ID %d\n", pThis->iSWatchDogID);
		return S_OK;
	}
	else
	{
		printf("Swatchdog initialize failed!\n");
		return S_FAIL;
	}
}

SCODE   KickSWatchDog(HANDLE hRTSPS)
{
	TSTREAMSERVERINFO		*pThis;
	TMessageInfo			tMsgInfo;
	TMessageUtilOption      tMsgUtilInfo;
	char					acBuffer[WATCHDOG_BUFFER_SIZE];
	int						iBufferLength = 0;
	SCODE					scResult;

	pThis = (TSTREAMSERVERINFO *)hRTSPS;

	if(pThis->bSWatchDogEnabled)
	{
		DWORD		dwNow = 0, dwElasped = 0;

		OSTick_GetMSec(&dwNow);
		dwElasped = rtspCheckTimeDifference(pThis->dwSwatchDogLastKick, dwNow);
		if(dwElasped > 10000)
		{
			//Reset the timer
			pThis->dwSwatchDogLastKick = dwNow;
			//Send the kick message
			memset(&tMsgInfo, 0, sizeof(tMsgInfo));
			memset(acBuffer, 0 , WATCHDOG_BUFFER_SIZE);
			tMsgInfo.iIndex = pThis->iSWatchDogID;
			tMsgInfo.iType = cmKickWatchDog;
			iBufferLength = WATCHDOG_BUFFER_SIZE - 1;

			scResult = Message_Compose_Control(acBuffer, &iBufferLength, 1, &tMsgInfo);
			//printf("\tcompose control result = %d, length = %d\n", scResult, iBufferLength);
			//printf("\t%s\n", acBuffer+2);

			tMsgUtilInfo.iControlType = 0;
			sprintf(tMsgUtilInfo.szSocketPath, "%s", WATCHDOG_SVR_SCK);
			tMsgUtilInfo.iBufLenOut = iBufferLength;
			memcpy(tMsgUtilInfo.pcBuffer, acBuffer, iBufferLength);

			scResult = Message_Util_SendbySocket(&tMsgUtilInfo);

			if(scResult == S_OK)
			{
				return S_OK;
			}
			else
			{
				printf("Kick software watchdog failed!\n");
				return S_FAIL;
			}
		}
		else
		{
			//It is not yet time to kick watchdog
			return S_OK;
		}
	}
	else
	{
		return S_FAIL;  //SWatchdog is not enabled
	}
}

SCODE	ReleaseSWatchDog(HANDLE hRTSPS)
{
	TSTREAMSERVERINFO		*pThis;
	TMessageInfo			tMsgInfo;
	TMessageUtilOption      tMsgUtilInfo;
	char					acBuffer[WATCHDOG_BUFFER_SIZE];
	int						iBufferLength = 0;
	SCODE					scResult;

	pThis = (TSTREAMSERVERINFO *)hRTSPS;

	if(pThis->bSWatchDogEnabled)
	{
		memset(&tMsgInfo, 0, sizeof(tMsgInfo));
		memset(acBuffer, 0 , WATCHDOG_BUFFER_SIZE);
		tMsgInfo.iIndex = pThis->iSWatchDogID;
		tMsgInfo.iType = cmUnloadWatchDog;
		iBufferLength = WATCHDOG_BUFFER_SIZE - 1;

		scResult = Message_Compose_Control(acBuffer, &iBufferLength, 1, &tMsgInfo);
		//printf("\tcompose control result = %d, length = %d\n", scResult, iBufferLength);
		//printf("\t%s\n", acBuffer+2);

		tMsgUtilInfo.iControlType = 0;
		sprintf(tMsgUtilInfo.szSocketPath, "%s", WATCHDOG_SVR_SCK);
		tMsgUtilInfo.iBufLenOut = iBufferLength;
		memcpy(tMsgUtilInfo.pcBuffer, acBuffer, iBufferLength);

		scResult = Message_Util_SendbySocket(&tMsgUtilInfo);

		if(scResult == S_OK)
		{
			pThis->bSWatchDogEnabled = FALSE;
			return S_OK;
		}
		else
		{
			printf("Swatchdog release failed!\n");
			return S_FAIL;
		}
	}
	else
	{
		return S_FAIL;	//SWatchdog is not enabled
	}
}

//20080829 thread eliminated and transfer to function
SCODE RTPOverHttpSocketExchanger(HANDLE hRTSPS)
{
	TSTREAMSERVERINFO					*pThis;
  	fd_set								SockfdInSelect;
	//20080619 modified by Louis
  	int				i = 0, iSelect = 0,iResult = 0, iType = 0, iOffset = 0, iNumber = RTSPS_MESSAGE_NUMBER;		
	DWORD			dwLength = 0, dwSessionID = 0;
	TMessageInfo	*ptMsgInfo;
	//20080619 modified from 2-->1024 by Louis
	char			acBuffer[2048];
	char			acLocation[LOCATION_LEN];
	int				iMaxSck = 0;
	//20090626 SWatchDog
	struct timeval	tv;
	//20101018 Add by danny for support multiple channel text on video
	int				iMultipleChannelChannelIndex = 0;

	pThis = (TSTREAMSERVERINFO *)hRTSPS;
	ptMsgInfo = pThis->ptMsgInfo;

	memset(acLocation, 0, LOCATION_LEN);
	memset(acBuffer, 0, sizeof(acBuffer));	//20081003 reset buffer
	memset(pThis->tSSConnSettings.pcRecvBuffer, 0, GENERAL_BUFFER_LENGTH);

	if(!pThis->bRunning)
	{
		usleep(10);
		return S_FAIL;
	}
	
	//while (pThis->bRunning)
	{
		iMaxSck = 0;
		FD_ZERO( &SockfdInSelect );

		//20080829 add HTTP socket here!
		if(pThis->iSDPSocket > 0)
		{
			FD_SET( pThis->iSDPSocket, &SockfdInSelect);
			if(pThis->iSDPSocket > iMaxSck)
			{
				iMaxSck = pThis->iSDPSocket;
			}
		}
		//Add control Fifo socket	
		if( pThis->tRTSPInfo.iControlFIFO > 0 )
		{
			FD_SET( pThis->tRTSPInfo.iControlFIFO, &SockfdInSelect);
			if(pThis->tRTSPInfo.iControlFIFO > iMaxSck)
			{
				iMaxSck = pThis->tRTSPInfo.iControlFIFO;
			}
		}	
  		else
  		{
			if(access(pThis->tRTSPInfo.szContorlipcFIFO, F_OK) == -1)
			{
				int	iRes;

				iRes = mkfifo(pThis->tRTSPInfo.szContorlipcFIFO, 0777);
				if (iRes != 0)
				{
					printf("Could not create fifo %s\n", pThis->tRTSPInfo.szContorlipcFIFO);
					usleep(10);
					return S_FAIL;
				}
			}  

  			if ( (pThis->tRTSPInfo.iControlFIFO = open(pThis->tRTSPInfo.szContorlipcFIFO, O_RDONLY | O_NONBLOCK)) < 0 )
   			{
    			printf("reopen control FIFO fail, errno=%d\n", errno);
    			usleep(10);
    			return S_FAIL;				
			}
			else
			{
				//20080619 added by Louis 
				FD_SET(pThis->tRTSPInfo.iControlFIFO, &SockfdInSelect);
			}
  		}

		//20090626 SWatchDog
		tv.tv_sec = WATCHDOG_KICK_INTERVAL;
		tv.tv_usec = 0;
  		iSelect = select( iMaxSck + 1, &SockfdInSelect, NULL, NULL, &tv);

		if (iSelect == -1 && errno == EINTR)
  		{
			/* a signal was caught */
		}
  		else if ( iSelect < 0 )
  		{
  			printf("rtsps_fdipc.c: select error, %s\n",strerror(errno));
  		}
  		else if ( iSelect > 0 )
  		{
			if(FD_ISSET(pThis->iSDPSocket, &SockfdInSelect))
			{
				RTSPHandleRTSPOverHTTPSocket(pThis,&pThis->tSSConnSettings);
			}
  			
  			if (FD_ISSET(pThis->tRTSPInfo.iControlFIFO, &SockfdInSelect))
  			{  		
				//CID:1013, CHECKER:STRING_NULL
				iResult = read(pThis->tRTSPInfo.iControlFIFO, acBuffer, sizeof(acBuffer) - 1);
				acBuffer[sizeof(acBuffer) - 1] = 0;
				if( iResult < 0 )
				{
					printf("Error of receiving Control FIFO: %s\n",strerror(errno));
					close(pThis->tRTSPInfo.iControlFIFO);
					pThis->tRTSPInfo.iControlFIFO = -1;
 				 	return S_FAIL;
				}
				else if( iResult == 0)
				{
					close(pThis->tRTSPInfo.iControlFIFO);
					pThis->tRTSPInfo.iControlFIFO = -1;
					return S_FAIL;
				}

				printf("Reads:%s\n", acBuffer);
				//Initialize for message parsing
				iNumber = RTSPS_MESSAGE_NUMBER;

				if(Message_GetTypeLength(acBuffer, sizeof(acBuffer), &iType, &iOffset, &dwLength) != S_OK)
				{
					printf("Error parsing FIFO commands!\n");
					return S_FAIL;
				}
				if(iType == mtData)
				{
					//20101018 Add by danny for support multiple channel text on video
					iResult = Message_GetDataAttribute(pThis->pXMLHandle, acBuffer + iOffset, dwLength);
					if(iResult >= 0)
					{
						iMultipleChannelChannelIndex = iResult;
					}
					else
					{
						iMultipleChannelChannelIndex = 0;
					}
					//printf("[%s]Channel %d\n", __FUNCTION__, iMultipleChannelChannelIndex);
					
					if(Message_GetDataType(pThis->pXMLHandle, acBuffer + iOffset, dwLength) == dmLocation)
					{
						memset(acLocation, 0, LOCATION_LEN);
						if(Message_Parse_SingleData(pThis->pXMLHandle, acBuffer + iOffset, dwLength, acLocation, LOCATION_LEN) == S_OK)
						{
							//20101018 Add by danny for support multiple channel text on video
							RTSPStreaming_SendLocation(pThis->hRTSPServer, acLocation, iMultipleChannelChannelIndex);
						}
					}
				}
				else if(iType == mtControl)
				{			
					if(Message_Parse_Control(pThis->pXMLHandle, acBuffer + iOffset, dwLength, &iNumber, ptMsgInfo) == S_OK)
					{
						/*Expected input is <control id="0"><session id="SESSIONID">terminate</session></control> or
											<control id="0"><session guid="GUID">terminate</session></control>*/
						for (i=0; i<iNumber; i++)
						{
							if(ptMsgInfo[i].iType == cmSessionControl && ptMsgInfo[i].aiValue[0] == ccTerminate)
							{
								char	acSessionID[32], *pcStart = NULL;
								int		i = 0;
#ifdef _SHARED_MEM
								//20101208 Modified by danny For GUID format change
								//20100105 Added For Seamless Recording
								char	acSessionGUID[RTSPS_Seamless_Recording_GUID_LENGTH];
								//DWORD	dwSessionGUID = 0;
								
								memset(acSessionGUID, 0, sizeof(acSessionGUID));
#endif
								memset(acSessionID, 0, sizeof(acSessionID));
								//20081224 We have to parse ourself because DWORD is unsigned
								if( (pcStart = strstr(acBuffer, "session id=")) != NULL )
								{
									pcStart += 12;
									for(i = 0; pcStart[i] != 0 && pcStart[i] != '\"'; i++);
									strncpy(acSessionID, pcStart, i);

									dwSessionID = (DWORD)strtoul(acSessionID, NULL, 10);
									printf("Stop client with sessionID %u\n", dwSessionID);
									//20130315 added by Jimmy to log more information
									syslog(LOG_DEBUG, "Stop client with sessionID %u\n", dwSessionID);
									if(RTSPStreaming_RemoveSession(pThis->hRTSPServer, dwSessionID) != 0)
									{
										printf("Error removing session with ID %u\n", dwSessionID);
									}

									return 0;
								}
#ifdef _SHARED_MEM
								else if( (pcStart = strstr(acBuffer, "session guid=")) != NULL )
								{
									pcStart += 14;
									for(i = 0; pcStart[i] != 0 && pcStart[i] != '\"'; i++);
									strncpy(acSessionGUID, pcStart, i);

									//20101208 Modified by danny For GUID format change
									/*dwSessionGUID = (DWORD)strtoul(acSessionGUID, NULL, 10);
									printf("Stop client with GUID %u\n", dwSessionGUID);
									if(RTSPStreaming_RemoveGUIDSessions(pThis->hRTSPServer, dwSessionGUID) != 0)*/
									printf("Stop client with GUID %s\n", acSessionGUID);
									//20130315 added by Jimmy to log more information
									syslog(LOG_DEBUG, "Stop client with GUID %s\n", acSessionGUID);
									if(RTSPStreaming_RemoveGUIDSessions(pThis->hRTSPServer, acSessionGUID) != 0)
									{
										printf("Error removing session with GUID %s\n", acSessionGUID);
									}
									
									return 0;
								}
#endif
								else
								{
									printf("Error parsing FIFO commands!\n");
									return S_FAIL;
								}
								
							}
							//20091214 danny add, Venc send re-establish unix socket fifo command, cmKickWatchDog is used Unintendedly
							else if(ptMsgInfo[i].iType == cmKickWatchDog)
							{						
								int		iNum = 0;
								
								printf("ptMsgInfo[i].iIndex=%d\n", ptMsgInfo[i].iIndex);
								
								if (ptMsgInfo[i].iIndex >= 0)
								{
									iNum = ptMsgInfo[i].iIndex;
									
									close(pThis->tVideoSrcInfo[iNum].iFdSock);
									pThis->tVideoSrcInfo[iNum].iFdSock = -1;

									if(CfgParser_GetUnixDomainSocket(&pThis->tVideoSrcInfo[iNum].acSockPathName, &pThis->tVideoSrcInfo[iNum].iFdSock) != S_OK)
									{
										printf("[%s]Warning! Reconnect unix domain socket %s failed!\n", __FUNCTION__, pThis->tVideoSrcInfo[iNum].acSockPathName);
										syslog(LOG_ERR, "[%s]Reconnect unix domain socket %s failed!\n", __FUNCTION__, pThis->tVideoSrcInfo[iNum].acSockPathName);
									}
									else
									{
										printf("[%s]Reconnect unix domain socket %s to socket %d!\n", __FUNCTION__, pThis->tVideoSrcInfo[iNum].acSockPathName, pThis->tVideoSrcInfo[iNum].iFdSock);
										syslog(LOG_ERR, "[%s]Reconnect unix domain socket %s to socket %d!\n", __FUNCTION__, pThis->tVideoSrcInfo[iNum].acSockPathName, pThis->tVideoSrcInfo[iNum].iFdSock);
									}
								}
								else
								{
									printf("[%s]Invalid stream index!\n", __FUNCTION__);
									syslog(LOG_ERR, "[%s]Invalid stream index!\n", __FUNCTION__);
								}
							}
						}
					}
				}
				else
				{
					printf("Unsupported FIFO command type!\n");
					return S_FAIL;
				}
  			}
  		}
	}

	return 0;
}
