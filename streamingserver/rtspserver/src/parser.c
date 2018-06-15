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
 *  File name          :   parser.c 
 *  File description   :   RTSP message parser 
 *  Author             :   ShengFu
 *  Created at         :   2002/4/24 
 *  Note               :   
 *	$Log: /RD_1/Protocol/RTSP/Server/rtspstreamserver/rtspserver/src/parser.c $
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
 * 6     05/09/27 1:14p Shengfu
 * update to version 1,3,0,1
 * 
 * 4     05/08/26 10:02a Shengfu
 * 
 * 3     05/08/19 11:49a Shengfu
 * 
 * 1     05/08/19 11:30a Shengfu
 * 
 * 2     05/08/10 9:01a Shengfu
 * update rtspstreaming server which enable multicast
 * 
 * 4     05/07/13 2:26p Shengfu
 * update rtsp streaming server
 * 
 * 3     05/04/15 1:35p Shengfu
 * 1. multicast added, but disable
 * 2. RTP extension added
 * 
 * 2     04/12/20 2:34p Shengfu
 * update to version 1.1.0.0
 * 
 * 1     04/09/14 9:39a Shengfu
 * 
 * 1     04/09/14 9:20a Shengfu
 * 
 * 2     03/04/17 2:45p Shengfu
 * update to 0102b
 * 
 * 5     02/07/01 2:01p Shengfu
 * fujitsu server compliant
 * 
 * 4     02/06/24 5:18p Shengfu
 * video_only modified
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "parser.h"
#include "rtsp_server_local.h"
#include "streamserver.h"   //20130904 added by Charles for ondemand multicast

/*WAVEFMTX PCM_hdr;*/

//20101006 Added by danny for support GET_PARAMETER command for keep-alive in RTSP signaling
TKN g_Methods [] =
{
	{DESCRIBE_TKN, RTSP_DESCRIBE_METHOD},
	{PLAY_TKN, RTSP_PLAY_METHOD},
	{PAUSE_TKN, RTSP_PAUSE_METHOD},
	{SETUP_TKN, RTSP_SETUP_METHOD},
	{TEARDOWN_TKN,RTSP_TEARDOWN_METHOD},
	{OPTIONS_TKN,RTSP_OPTIONS_METHOD},
	{SETPARAM_TKN,RTSP_SETPARAM_METHOD},
	{GETPARAM_TKN,RTSP_GETPARAM_METHOD},
	{0, -1}
};

TKN g_Status [] =
{
   {"Continue", 100},
   {"OK", 200},
   {"Created", 201},
   {"Low on Storage Space",250},
   {"Multiple Choices", 300},
   {"Moved Permanently", 301},
   {"Moved Temporarily", 302},
   {"See Other",303},
   {"User Proxy",304},
   {"Bad Request", 400},
   {"Unauthorized", 401},
   {"Payment Required", 402},
   {"Forbidden", 403},
   {"Not Found", 404},
   {"Method Not Allowed", 405},
   {"Not Acceptable", 406},
   {"Proxy Authentication Required", 407},
   {"Request Time-out", 408},
   {"Gone", 410},
   {"Length Required", 411},
   {"Precondition Failed", 412},
   {"Request Entity Too Large", 413},
   {"Request-URI Too Large", 414},
   {"Unsupported Media Type", 415},
   {"Parameter Not Understood", 451},
   {"Conference Not Found", 452},
   {"Not Enough Bandwidth", 453},
   {"Session Not Found", 454},
   {"Method Not Valid In This State", 455},
   {"Header Field Not Valid for Resource", 456},
   {"Invalid Range", 457},
   {"Parameter Is Read-Only", 458},
   {"Aggregate operation not allowed",459},
   {"Only aggregate operation allowed",460},
   {"Unsupported transport",461},
   {"Destination unreachable",462},
   {"Internal Server Error", 500},
   {"Not Implemented", 501},
   {"Bad Gateway", 502},
   {"Service Unavailable", 503},
   {"Gateway Time-out", 504},
   {"RTSP Version Not Supported", 505},
   {"Option not supported", 551},
   {0, -1}
};

char  g_acInvld_method[] = "Invalid Method";

/* message header keywords */
const char  g_acContentLength[] = "Content-Length";
const char  g_acsAccept[] = "Accept";
const char  g_acsAllow[] = "Allow";
const char  g_acsBlocksize[] = "Blocksize";
const char  g_acsContentType[] = "Content-Type";
const char  g_acsDate[] = "Date";
const char  g_acsTransport[] = "Transport";
const char  g_acsSeq[] = "CSeq";
const char  g_acsSession[] = "Session";
const char  g_acsRtpInfo[] = "RTP-Info";

#ifdef _SHARED_MEM
//20110915 Modify by danny for support Genetec MOD
//20100105 Added For Media on demand
//20141110 modified by Charles for ONVIF Profile G
TKN g_MODHeaderField [] =
{
   {"Speed", MOD_PLAYSPEED},
   {"Scale", MOD_PLAYSCALE},
   {"Range", MOD_RANGE},
   {"Rate-Control", MOD_RATECONTROL},
   {"Immediate", MOD_IMMEDIATE},
   {0, -1}
};

char  g_acInvld_headerfield[] = "Invalid Header Field";
#endif

int RTSPServer_ParseURL(const char *pcURL, RTSP_CLIENT *pClient, unsigned short *pusPort, EParseMethod eMethod)
{
    /* expects format '[rtsp://server[:port/]]filename?ExtraInfo */
    char  acIP[RTSP_URL_LEN],acURI[RTSP_URL_LEN-RTSP_URL_ACCESSNAME_LEN];
    char  *pcPortStr,*pURI;
    int   iValidURL = 0;
    char  *pcToken;
    int   iHasPort = 0, iRet = 0;
	int	  i = 0, j = 0;

#ifdef _INET6
	char acAnotherServerName[RTSP_URL_LEN-RTSP_URL_ACCESSNAME_LEN]="";
	unsigned short usAnotherPort = 0;
#endif
  
	memset(acIP,0,RTSP_URL_LEN);
	memset(acURI,0,RTSP_URL_LEN-RTSP_URL_ACCESSNAME_LEN);

    if(strncmp(pcURL,"rtsp://",7) == 0 || strncmp(pcURL,"http://",7) == 0) //20090821 Modified for ONVIF message
    {
		strncpy(acIP, pcURL+7,RTSP_URL_LEN-8);
		
		//20090821 Modified for ONVIF message
		if(eMethod == eDescribeMethod || eMethod == ePlayMethod)
		{
			pURI = strchr(acIP,'/');
		}	
		else
		{
			pURI = strrchr(acIP,'/');
		}

		//20070919 YenChun Fixed if no access name
		if (pURI && *(pURI+1) != '\0' )
		{
			strncpy(acURI,pURI+1,RTSP_URL_LEN-RTSP_URL_ACCESSNAME_LEN-1);
		}

#ifdef _INET6
		if (*acIP == '[')
		{
			char* pcRightbracket = NULL;

			pcRightbracket = strchr(acIP+1, ']');
			if (pcRightbracket != NULL)
			{
				DWORD  dwTmpServerLen = (DWORD) ( pcRightbracket - acIP + 1) ;
				DWORD dwServerLen = (dwTmpServerLen < (RTSP_URL_LEN-RTSP_URL_ACCESSNAME_LEN-1)) ? dwTmpServerLen : RTSP_URL_LEN-RTSP_URL_ACCESSNAME_LEN-1 ;
				// literal IPv6 address
				if (pcRightbracket[1] == ':') 
				{
					//port
					usAnotherPort = (unsigned short)atol(&pcRightbracket[2]);
				}
				else
				{
					usAnotherPort = RTSP_DEFAULT_PORT;
				}
				strncpy(acAnotherServerName, acIP, dwServerLen);
			}
			else
			{
				// Invaild URL
			}
		}
#endif
        
        if (strchr(acIP, ':'))
        {
            iHasPort = 1;
        }

        pcToken = strtok(acIP, " :/\t\n");

        if(pcToken)
        {
			pClient->acServerName[RTSP_URL_LEN-RTSP_URL_ACCESSNAME_LEN-1] = 0;
            strncpy(pClient->acServerName, pcToken,RTSP_URL_LEN-RTSP_URL_ACCESSNAME_LEN-1);

            if(iHasPort)
            {
                pcPortStr = strtok(NULL, " /\t\n");

                if(pcPortStr)
                {
                    *pusPort = (unsigned short)atol(pcPortStr);
                }
                else
                {
                    *pusPort = RTSP_DEFAULT_PORT;
                }
            }
            else
            {
                *pusPort = RTSP_DEFAULT_PORT;
            }

			/* Parse for file Name */
			for( i = 0; acURI[i] != '\0' && acURI[i] != '/' && acURI[i] != '?'; i++);
			strncpy(pClient->acObject, acURI, i);
			pClient->acObject[i] = 0;

			/*Collect the remaining data */
			for( j = i + 1; acURI[j] != '\0'; j++);
			strncpy(pClient->acExtraInfo, acURI + i + 1, j);
			pClient->acExtraInfo[j] = 0;
			if(eMethod == eDescribeMethod)
			{
#ifdef _SHARED_MEM
				/* 20100402 Parse for MOD if MOD keyword is matched */
				if(strncmp(pClient->acObject, RTSPMOD_SDP_KEYWORD, strlen(RTSPMOD_SDP_KEYWORD)) == 0)
				{
					//20110915 Modify by danny for support Genetec MOD
					//if(pClient->acExtraInfo[0] != 0)
					{
						pClient->bMediaOnDemand = TRUE;
					}
					/*else
					{
						return -1;
					}*/
					/*if((iRet = RTSPServer_ParseMODInfo(pClient)) != 0)
					{
						if(iRet == -1)
						{
							iValidURL = 0;
						}
						else
						{
							iValidURL = iRet;	//Pass back special cases
						}
						return iValidURL;
					}*/
				}
				else
#endif
				{
					if((iRet = RTSPServer_ParseExtraInfo(pClient)) != 0)
					{
						//20081218 for 415 Unsupported Media Type
						if(iRet == -1)
						{
							iValidURL = 0;
						}
						else
						{
							iValidURL = iRet;	//Pass back special cases
						}
						return iValidURL;
					}
				}
			}
			/* Moved here for Shmem */
			iValidURL = 1;
        }
    }
    else
    {
        *pusPort = RTSP_DEFAULT_PORT;
		pcToken = NULL;
		pcToken = strchr(pcURL,'/');
		if( pcToken != NULL)
			pcToken++;
		else
			pcToken = (char*)pcURL;

		memset(pClient->acObject,0,RTSP_URL_ACCESSNAME_LEN);	
		strncpy(pClient->acObject, pcToken,RTSP_URL_ACCESSNAME_LEN-1);
        iValidURL = 1;
    }

#ifdef _INET6
	if (usAnotherPort)
	{
		*pusPort = usAnotherPort;
	}
	if (acAnotherServerName[0] != '\0')
	{
		strncpy(pClient->acServerName, acAnotherServerName,RTSP_URL_LEN-RTSP_URL_ACCESSNAME_LEN-1);
	}
#endif
	printf("Parsed Server %s port %u filename %s info %s\n", pClient->acServerName, *pusPort, pClient->acObject, pClient->acExtraInfo);
    return iValidURL;
}

//20160406 Add by Faber, for checking Multicast IP and port
SCODE RTSPServer_CheckMulticastIPPortIsUsed(RTSP_CLIENT *pClient, const unsigned long ulMulticastAddress, const int iPort)
{
	SCODE sRet = S_OK;
	RTSP_SERVER* pRtspServer = pClient->parent;

	int iMulticastIndex = 0;
	int iMulticastTotal = RTSP_MULTICASTNUMBER + RTSP_ONDEMAND_MULTICASTNUMBER;
	for(; iMulticastIndex < iMulticastTotal; iMulticastIndex++)
	{

		if( pRtspServer->ulMulticastAddress[iMulticastIndex] && (ulMulticastAddress == pRtspServer->ulMulticastAddress[iMulticastIndex]))
		{
			if(iPort)
			{
				if(iPort == pRtspServer->usMulticastVideoPort[iMulticastIndex])
				{
					return S_FAIL;
				}
				if(iPort == pRtspServer->usMulticastAudioPort[iMulticastIndex])
				{
					return S_FAIL;
				}
				if(iPort == pRtspServer->usMulticastMetadataPort[iMulticastIndex])
				{
					return S_FAIL;
				}
			}

		}

		if(pRtspServer->ulMulticastAudioAddress[iMulticastIndex] && (ulMulticastAddress == pRtspServer->ulMulticastAudioAddress[iMulticastIndex]))
		{
			if(iPort)
			{
				if(iPort == pRtspServer->usMulticastVideoPort[iMulticastIndex])
				{
					return S_FAIL;
				}
				if(iPort == pRtspServer->usMulticastAudioPort[iMulticastIndex])
				{
					return S_FAIL;
				}
				if(iPort == pRtspServer->usMulticastMetadataPort[iMulticastIndex])
				{
					return S_FAIL;
				}
			}
		}

		if(pRtspServer->ulMulticastMetadataAddress[iMulticastIndex] && (ulMulticastAddress == pRtspServer->ulMulticastMetadataAddress[iMulticastIndex]))
		{
			if(iPort)
			{
				if(iPort == pRtspServer->usMulticastVideoPort[iMulticastIndex])
				{
					return S_FAIL;
				}
				if(iPort == pRtspServer->usMulticastAudioPort[iMulticastIndex])
				{
					return S_FAIL;
				}
				if(iPort == pRtspServer->usMulticastMetadataPort[iMulticastIndex])
				{
					return S_FAIL;
				}
			}
		}
	}
	return sRet;
}

//20130830 added by Charles for ondemand multicast
int RTSPServer_ParseOndemandMulticastInfo(RTSP_CLIENT *pClient, int iMediaType, bool *pbWithOnDemandMulticast)
{
	char	*pchar = NULL;
	char	*pchar1 = NULL;
	char	*pchar2 = NULL;
	char	acTempAllPort[ONDEMAND_MULTICAST_LEN];	
	char	acTempMulticastAddress[ONDEMAND_MULTICAST_LEN];
	char	acTempTTL[ONDEMAND_MULTICAST_LEN];
	char	acTempRTPPort[ONDEMAND_MULTICAST_LEN];
	unsigned long	uTempMulticastAddress;
	unsigned long	uTempRTPPort = 0;

	*pbWithOnDemandMulticast = FALSE;
	if(((pchar = strstr(pClient->acRecvBuffer,"destination=")) != NULL) && ((pchar1 = strstr(pClient->acRecvBuffer,"port=")) != NULL))
	{
		if((sscanf(pchar, "destination=%29[^;\t\n\r]", acTempMulticastAddress) == 1) && (sscanf(pchar1, "port=%29[^;\t\n\r]", acTempAllPort) == 1))
		{
			uTempMulticastAddress = inet_addr(acTempMulticastAddress);
			if(uTempMulticastAddress == 0 || uTempMulticastAddress == -1)
			{
				return -1;
			}
			if(sscanf(acTempAllPort, "%29[^-]", acTempRTPPort) == 1)
			{
				if((uTempRTPPort = atoi(acTempRTPPort)) == 0)
				{
					return -1;
				}
			}

			if(RTSPServer_CheckMulticastIPPortIsUsed(pClient, uTempMulticastAddress, uTempRTPPort) != S_OK)
			{
				printf("multicast address is in used\n");
				//return -1;
			}
			switch(iMediaType)
			{
			case SETUP_VIDEO:
			{
				pClient->tOndemandMulticastInfo.ulMulticastAddress = uTempMulticastAddress;
				pClient->tOndemandMulticastInfo.usMulticastVideoPort = uTempRTPPort;
				break;
			}
			case SETUP_AUDIO:
			{
				pClient->tOndemandMulticastInfo.ulMulticastAudioAddress = uTempMulticastAddress;
				pClient->tOndemandMulticastInfo.usMulticastAudioPort = uTempRTPPort;
				break;
			}
			case SETUP_METADATA:
			{
				pClient->tOndemandMulticastInfo.ulMulticastMetadataAddress = uTempMulticastAddress;
				pClient->tOndemandMulticastInfo.usMulticastMetadataPort = uTempRTPPort;
				break;
			}
			default :
				break;
			}

			if((pchar2 = strstr(pClient->acRecvBuffer,"ttl")) != NULL)
			{
				if(sscanf(pchar2, "ttl=%29[^;\t\n\r]", acTempTTL) == 1)
				{
					pClient->tOndemandMulticastInfo.usTTL = atoi(acTempTTL);
				}
				else
				{
					pClient->tOndemandMulticastInfo.usTTL = pClient->parent->usTTL[pClient->iSDPIndex-1];
				}
				
			}
			else
			{
				pClient->tOndemandMulticastInfo.usTTL = pClient->parent->usTTL[pClient->iSDPIndex-1];
			}

			printf("parse multicast info success!\n");
			*pbWithOnDemandMulticast = TRUE;
		}

	}

	return 0;
}

//20130830 added by Charles for ondemand multicast
int CheckIfRepeatMulticastInfo(RTSP_CLIENT *pClient, RTSP_SERVER* pServer)
{
    int     i;
    for(i=0 ; i<RTSP_MULTICASTNUMBER+RTSP_ONDEMAND_MULTICASTNUMBER; i++)
    {
        if(pClient->tOndemandMulticastInfo.ulMulticastAddress == pServer->ulMulticastAddress[i])
        {

			if(i < RTSP_MULTICASTNUMBER || i >= RTSP_MULTICASTNUMBER)
			{
				if (pServer->usMulticastVideoPort[i] == pClient->tOndemandMulticastInfo.usMulticastVideoPort
					|| pServer->usMulticastVideoPort[i] == pClient->tOndemandMulticastInfo.usMulticastAudioPort
#ifdef _METADATA_ENABLE
					|| pServer->usMulticastVideoPort[i] == pClient->tOndemandMulticastInfo.usMulticastMetadataPort
#endif
					)
				{
					return -1;
				}
			}
			
			if (pServer->usMulticastAudioPort[i] == pClient->tOndemandMulticastInfo.usMulticastVideoPort
				|| pServer->usMulticastAudioPort[i] == pClient->tOndemandMulticastInfo.usMulticastAudioPort
#ifdef _METADATA_ENABLE
				|| pServer->usMulticastAudioPort[i] == pClient->tOndemandMulticastInfo.usMulticastMetadataPort
#endif
				)
			{
				return -1;
			}
#ifdef _METADATA_ENABLE
			if(pServer->usMulticastMetadataPort[i] == pClient->tOndemandMulticastInfo.usMulticastVideoPort
				|| pServer->usMulticastMetadataPort[i] == pClient->tOndemandMulticastInfo.usMulticastAudioPort
				|| pServer->usMulticastMetadataPort[i] == pClient->tOndemandMulticastInfo.usMulticastMetadataPort)
			{
				return -1;
			}
#endif

/*#else
			if(i < RTSP_MULTICASTNUMBER || i >= RTSP_MULTICASTNUMBER+RTSP_AUDIO_EXTRA_MULTICASTNUMBER)
			{
				if(pServer->usMulticastVideoPort[i] == pClient->tMulticastInfo.usMulticastVideoPort
					|| pServer->usMulticastVideoPort[i] == pClient->tMulticastInfo.usMulticastAudioPort)
				{
					return -1;
				}
			}
			if(pServer->usMulticastAudioPort[i] == pClient->tMulticastInfo.usMulticastVideoPort
				|| pServer->usMulticastAudioPort[i] == pClient->tMulticastInfo.usMulticastAudioPort)
			{
				return -1;
			}
#endif*/
	    }
    }
	
    return 0;

}


#ifdef _SHARED_MEM
//20100105 Added For Media on demand
int RTSPServer_ParseMODInfo(RTSP_CLIENT* pClient)
{
	int			i = 0, iTemp = 0;
	char		*pcToken = NULL, *pcBuf = NULL;
	char		acDescriptor[RTSPPARSER_DESCRIPTOR_LENGTH], acLocTime[RTSPMOD_LOCTIME_LENGTH], acMediaLength[RTSPMOD_LENGTH_LENGTH];

	//Check for pointer
	if(pClient == NULL)
	{
		return -1;
	}

	//initialize
	memset(acDescriptor, 0, RTSPPARSER_DESCRIPTOR_LENGTH);
	memset(acLocTime, 0, RTSPMOD_LOCTIME_LENGTH);
	memset(acMediaLength, 0, RTSPMOD_LENGTH_LENGTH);

	pcToken = strtok(pClient->acExtraInfo, "&");

	while(TRUE)
	{
		for(i = 0; pcToken[i] != 0 && pcToken[i] != '='; i++);
		memset(acDescriptor, 0, sizeof(acDescriptor));
		strncpy(acDescriptor, pcToken, i);
		acDescriptor[RTSPPARSER_DESCRIPTOR_LENGTH - 1] = 0;

		pcBuf = pcToken + i + 1;

		if(!rtspstrncasecmp(acDescriptor, RTSPMOD_STIME_KEYWORD, sizeof(RTSPMOD_STIME_KEYWORD)))
		{
			strncpy(pClient->tMODInfo.acStime, pcBuf, RTSPMOD_STIME_LENGTH - 1);
			pClient->tMODInfo.acStime[RTSPMOD_STIME_LENGTH - 1] = 0;
			printf("Parsed Start Time %s\n", pClient->tMODInfo.acStime);
		}
		else if(!rtspstrncasecmp(acDescriptor, RTSPMOD_ETIME_KEYWORD, sizeof(RTSPMOD_ETIME_KEYWORD)))
		{
			strncpy(pClient->tMODInfo.acEtime, pcBuf, RTSPMOD_ETIME_LENGTH - 1);
			pClient->tMODInfo.acEtime[RTSPMOD_ETIME_LENGTH - 1] = 0;
			printf("Parsed End Time %s\n", pClient->tMODInfo.acEtime);
		}	
		else if(!rtspstrncasecmp(acDescriptor, RTSPMOD_LOCTIME_KEYWORD, sizeof(RTSPMOD_LOCTIME_KEYWORD)))
		{
			strncpy(acLocTime, pcBuf, RTSPMOD_LOCTIME_LENGTH - 1);
			acLocTime[RTSPMOD_LOCTIME_LENGTH - 1] = 0;
			
			if(strncmp(acLocTime, "0",  RTSPMOD_LOCTIME_LENGTH - 1) == 0)
			{
				pClient->tMODInfo.bLocTime = FALSE;
				printf("Parsed local time mode is FALSE\n");
			}
			else if(strncmp(acLocTime, "1",  RTSPMOD_LOCTIME_LENGTH - 1) == 0)
			{
				pClient->tMODInfo.bLocTime = TRUE;
				printf("Parsed local time mode is TRUE\n");
			}
			else
			{
				printf("Invalid local time %s specified in MOD\n", acLocTime);
				return -1;
			}			
		}
		else if(!rtspstrncasecmp(acDescriptor, RTSPMOD_LENGTH_KEYWORD, sizeof(RTSPMOD_LENGTH_KEYWORD)))
		{
			strncpy(acMediaLength, pcBuf, RTSPMOD_LENGTH_LENGTH - 1);
			acMediaLength[RTSPMOD_LENGTH_LENGTH - 1] = 0;
				
			if((iTemp = atoi(acMediaLength)) <= 0)
			{
				printf("Invalid media length %s in MOD\n", acMediaLength);
			}
			else
			{
				pClient->tMODInfo.dwLength = (DWORD)iTemp;
				printf("Parsed media length %d\n", pClient->tMODInfo.dwLength);
			}
		}
		else if(!rtspstrncasecmp(acDescriptor, RTSPMOD_FILE_KEYWORD, sizeof(RTSPMOD_FILE_KEYWORD)))
		{
			strncpy(pClient->tMODInfo.acFile, pcBuf, RTSPMOD_FILE_LENGTH - 1);
			pClient->tMODInfo.acFile[RTSPMOD_FILE_LENGTH - 1] = 0;
			printf("Parsed file name is %s\n", pClient->tMODInfo.acFile);
		}	
		else if(!rtspstrncasecmp(acDescriptor, RTSPMOD_LOC_KEYWORD, sizeof(RTSPMOD_LOC_KEYWORD)))
		{
			strncpy(pClient->tMODInfo.acLoc, pcBuf, RTSPMOD_LOC_LENGTH - 1);
			pClient->tMODInfo.acLoc[RTSPMOD_LOC_LENGTH - 1] = 0;
			printf("Parsed location is %s\n", pClient->tMODInfo.acLoc);
		}
		else if(!rtspstrncasecmp(acDescriptor, RTSPMOD_MODE_KEYWORD, sizeof(RTSPMOD_MODE_KEYWORD)))
		{
			if(!rtspstrncasecmp(pcBuf, RTSPMOD_NORMAL_MODE_KEYWORD, sizeof(RTSPMOD_NORMAL_MODE_KEYWORD)))
			{
				pClient->tMODInfo.eMODMode = eMOD_Normal;
			}
			else if(!rtspstrncasecmp(pcBuf, RTSPMOD_DOWNLOAD_MODE_KEYWORD, sizeof(RTSPMOD_DOWNLOAD_MODE_KEYWORD)))
			{
				pClient->tMODInfo.eMODMode = eMOD_Download;
			}
			else if(!rtspstrncasecmp(pcBuf, RTSPMOD_SYNC_MODE_KEYWORD, sizeof(RTSPMOD_SYNC_MODE_KEYWORD)))
			{
				pClient->tMODInfo.eMODMode = eMOD_Sync;
			}
			else
			{
				printf("Incorrect MOD mode: %s!\n", pcBuf);
				return -1;
			}

			printf("Parsed MOD mode is %d\n", pClient->tMODInfo.eMODMode);
		}
		else
		{
			printf("Unparsed descriptor \"%s\"\n", acDescriptor);
		}

		pcToken = strtok(NULL, "&");
		if(pcToken == NULL)
		{
			break;
		}
	}

	if(pClient->tMODInfo.acStime[0] == 0 && pClient->tMODInfo.acEtime[0] == 0 && pClient->tMODInfo.acFile[0] == 0)
	{
		printf("MOD need to specify start-time or end-time or file name!\n");
		return -1;
	}
	else
	{
		pClient->bMediaOnDemand = TRUE;
	}

	return 0;
}
#endif

int RTSPServer_ParseExtraInfo(RTSP_CLIENT* pClient)
{
	int			i = 0;
	char		*pcToken = NULL, *pcBuf = NULL;
	char		acDescriptor[RTSPPARSER_DESCRIPTOR_LENGTH], acBypast[RTSPPARSER_BYPAST_LENGTH], acHSMode[RTSPPARSER_MODE_LENGTH];
#ifdef _SHARED_MEM
	int			iTemp = 0, iRefTimeMinute = -1, iRefTimeSecond = -1;
	bool		bShmForceCheck = FALSE;
	//20100105 Added For Seamless Recording
	unsigned long	ulTemp = 0;
#endif

	//Check for pointer
	if(pClient == NULL)
	{
		return -1;
	}

	if(pClient->acExtraInfo[0] == 0)
	{
#ifdef _SHARED_MEM
		pClient->dwBypasyMSec = 0;
		pClient->eHSMode = eHSLiveStreaming;
		//added by neil 10/12/30
		pClient->eSVCMode = eSVCNull;
		pClient->bForceFrameInterval = FALSE;
		pClient->dwFrameInterval = 0;
#endif
		return 0;
	}

	//Initialize
	memset(acDescriptor, 0, sizeof(acDescriptor));
	memset(acBypast, 0, sizeof(acBypast));
	memset(acHSMode, 0, sizeof(acHSMode));
	//20081002 checking initialize
#ifdef _SHARED_MEM
	memset(pClient->acShmemReturnString, 0, RTSPPARSER_REPLY_STRING_LENGTH);
	pClient->dwBypasyMSec = 0xffffffff;
	pClient->eHSMode = eHSModeMax;
	pClient->dwMinSftMSec = 0xffffffff;
	pClient->dwDescribeMSec = 0;
#endif

	pcToken = strtok(pClient->acExtraInfo, "&");

	while(TRUE)
	{
		for(i = 0; pcToken[i] != 0 && pcToken[i] != '='; i++);
		memset(acDescriptor, 0, sizeof(acDescriptor));
		strncpy(acDescriptor, pcToken, i);
		acDescriptor[RTSPPARSER_DESCRIPTOR_LENGTH - 1] = 0;
#ifdef _SHARED_MEM	
		iTemp = 0;
#endif
		pcBuf = pcToken + i + 1;
		
		if(!rtspstrncasecmp(acDescriptor, RTSPPARSER_USERNAME_KEYWORD, sizeof(RTSPPARSER_USERNAME_KEYWORD)))
		{
			strncpy(pClient->acUserName, pcBuf, RTSP_USERNAME_LENGTH -1);
			pClient->acUserName[RTSP_USERNAME_LENGTH - 1] = 0;
		}
		else if(!rtspstrncasecmp(acDescriptor, RTSPPARSER_PASSWORD_KEYWORD, sizeof(RTSPPARSER_PASSWORD_KEYWORD)))
		{
			EncryptionUtl_Base64_DecodeString(pcBuf, pClient->acPassWord, RTSP_PASSWORD_LENGTH);
			pClient->acPassWord[RTSP_PASSWORD_LENGTH - 1] = 0;
		}	
#ifdef _SESSION_MGR
		//20090309 Multiple Stream
		else if(!rtspstrncasecmp(acDescriptor, RTSPPARSER_CODEC_KEYWORD, sizeof(RTSPPARSER_CODEC_KEYWORD)))
		{
			strncpy(pClient->tSessMgrInfo.acCodecType, pcBuf, 15);
			pClient->tSessMgrInfo.acCodecType[15] = 0;
			//printf("Parsed codec type %s\n", pClient->tSessMgrInfo.acCodecType);
		}
		else if(!rtspstrncasecmp(acDescriptor, RTSPPARSER_RESOLUTION_KEYWORD, sizeof(RTSPPARSER_RESOLUTION_KEYWORD)))
		{
			strncpy(pClient->tSessMgrInfo.acResolution, pcBuf, 15);
			pClient->tSessMgrInfo.acResolution[15] = 0;
			//printf("Parsed resolution %s\n", pClient->tSessMgrInfo.acResolution);
		}
		else if(!rtspstrncasecmp(acDescriptor, RTSPPARSER_MP4_INTRA_KEYWORD, sizeof(RTSPPARSER_MP4_INTRA_KEYWORD)))
		{
			strncpy(pClient->tSessMgrInfo.tMP4.acIntraPeriod, pcBuf, 7);
			pClient->tSessMgrInfo.tMP4.acIntraPeriod[7] = 0;
		}
		else if(!rtspstrncasecmp(acDescriptor, RTSPPARSER_MP4_RATEMODE_KEYWORD, sizeof(RTSPPARSER_MP4_RATEMODE_KEYWORD)))
		{
			strncpy(pClient->tSessMgrInfo.tMP4.acRateControlMode, pcBuf, 7);
			pClient->tSessMgrInfo.tMP4.acRateControlMode[7] = 0;
		}
		else if(!rtspstrncasecmp(acDescriptor, RTSPPARSER_MP4_QUANT_KEYWORD, sizeof(RTSPPARSER_MP4_QUANT_KEYWORD)))
		{
			strncpy(pClient->tSessMgrInfo.tMP4.acQuant, pcBuf, 3);
			pClient->tSessMgrInfo.tMP4.acQuant[3] = 0;
		}
		else if(!rtspstrncasecmp(acDescriptor, RTSPPARSER_MP4_QVALUE_KEYWORD, sizeof(RTSPPARSER_MP4_QVALUE_KEYWORD)))
		{
			strncpy(pClient->tSessMgrInfo.tMP4.acQvalue, pcBuf, 3);
			pClient->tSessMgrInfo.tMP4.acQvalue[3] = 0;
		}
		else if(!rtspstrncasecmp(acDescriptor, RTSPPARSER_MP4_BITRATE_KEYWORD, sizeof(RTSPPARSER_MP4_BITRATE_KEYWORD)))
		{
			strncpy(pClient->tSessMgrInfo.tMP4.acBitRate, pcBuf, 15);
			pClient->tSessMgrInfo.tMP4.acBitRate[15] = 0;
		}
		else if(!rtspstrncasecmp(acDescriptor, RTSPPARSER_MP4_FRAMERATE_KEYWORD, sizeof(RTSPPARSER_MP4_FRAMERATE_KEYWORD)))
		{
			strncpy(pClient->tSessMgrInfo.tMP4.acMaxFrame, pcBuf, 3);
			pClient->tSessMgrInfo.tMP4.acMaxFrame[3] = 0;
		}
		else if(!rtspstrncasecmp(acDescriptor, RTSPPARSER_MJPEG_QUANT_KEYWORD, sizeof(RTSPPARSER_MJPEG_QUANT_KEYWORD)))
		{
			strncpy(pClient->tSessMgrInfo.tMjpeg.acQuant, pcBuf, 3);
			pClient->tSessMgrInfo.tMjpeg.acQuant[3] = 0;
		}
		else if(!rtspstrncasecmp(acDescriptor, RTSPPARSER_MJPEG_QVALUE_KEYWORD, sizeof(RTSPPARSER_MJPEG_QVALUE_KEYWORD)))
		{
			strncpy(pClient->tSessMgrInfo.tMjpeg.acQvalue, pcBuf, 3);
			pClient->tSessMgrInfo.tMjpeg.acQvalue[3] = 0;
		}
		else if(!rtspstrncasecmp(acDescriptor, RTSPPARSER_MJPEG_FRAMERATE_KEYWORD, sizeof(RTSPPARSER_MJPEG_FRAMERATE_KEYWORD)))
		{
			strncpy(pClient->tSessMgrInfo.tMjpeg.acMaxFrame, pcBuf, 3);
			pClient->tSessMgrInfo.tMjpeg.acMaxFrame[3] = 0;
		}
		//20100222 Added For Liveany Support H.264
		else if(!rtspstrncasecmp(acDescriptor, RTSPPARSER_H264_INTRA_KEYWORD, sizeof(RTSPPARSER_H264_INTRA_KEYWORD)))
		{
			strncpy(pClient->tSessMgrInfo.tH264.acIntraPeriod, pcBuf, 7);
			pClient->tSessMgrInfo.tH264.acIntraPeriod[7] = 0;
		}
		else if(!rtspstrncasecmp(acDescriptor, RTSPPARSER_H264_RATEMODE_KEYWORD, sizeof(RTSPPARSER_H264_RATEMODE_KEYWORD)))
		{
			strncpy(pClient->tSessMgrInfo.tH264.acRateControlMode, pcBuf, 7);
			pClient->tSessMgrInfo.tH264.acRateControlMode[7] = 0;
		}
		else if(!rtspstrncasecmp(acDescriptor, RTSPPARSER_H264_QUANT_KEYWORD, sizeof(RTSPPARSER_H264_QUANT_KEYWORD)))
		{
			strncpy(pClient->tSessMgrInfo.tH264.acQuant, pcBuf, 3);
			pClient->tSessMgrInfo.tH264.acQuant[3] = 0;
		}
		else if(!rtspstrncasecmp(acDescriptor, RTSPPARSER_H264_QVALUE_KEYWORD, sizeof(RTSPPARSER_H264_QVALUE_KEYWORD)))
		{
			strncpy(pClient->tSessMgrInfo.tH264.acQvalue, pcBuf, 3);
			pClient->tSessMgrInfo.tH264.acQvalue[3] = 0;
		}
		else if(!rtspstrncasecmp(acDescriptor, RTSPPARSER_H264_BITRATE_KEYWORD, sizeof(RTSPPARSER_H264_BITRATE_KEYWORD)))
		{
			strncpy(pClient->tSessMgrInfo.tH264.acBitRate, pcBuf, 15);
			pClient->tSessMgrInfo.tH264.acBitRate[15] = 0;
		}
		else if(!rtspstrncasecmp(acDescriptor, RTSPPARSER_H264_FRAMERATE_KEYWORD, sizeof(RTSPPARSER_H264_FRAMERATE_KEYWORD)))
		{
			strncpy(pClient->tSessMgrInfo.tH264.acMaxFrame, pcBuf, 3);
			pClient->tSessMgrInfo.tH264.acMaxFrame[3] = 0;
		}
#endif
#ifdef _SHARED_MEM			
		else if(!rtspstrncasecmp(acDescriptor, RTSPPARSER_MAXSFT_KEYWORD, sizeof(RTSPPARSER_MAXSFT_KEYWORD)))
		{
			if((iTemp = atoi(pcBuf)) <= 0)
			{
				printf("Incorrect history streaming bypast number!\n");
				return -1;
			}
			else
			{
				//printf("HS bypast is %d seconds\n", iTemp);
				pClient->dwBypasyMSec = iTemp * 1000;
			}
		}
		//added by neil 2011/01/14 
		else if(!rtspstrncasecmp(acDescriptor, RTSPPARSER_SVC_FRAMEINTERVAL_KEYWORK, sizeof(RTSPPARSER_SVC_FRAMEINTERVAL_KEYWORK)))
		{
			int iFrameInterval = 0;
            iFrameInterval = atoi(pcBuf);
            if(iFrameInterval <= 0 || iFrameInterval > 60000)
            {
				return -1;
            }
            else
            {
                pClient->bForceFrameInterval = TRUE;
                pClient->dwFrameInterval = iFrameInterval;
            }

		}
		//added by neil 2010/12/30
		else if(!rtspstrncasecmp(acDescriptor, RTSPPARSER_SVCMODE_KEYWORD, sizeof(RTSPPARSER_SVCMODE_KEYWORD)))
		{
			if(!rtspstrncasecmp(pcBuf, RTSPPARSER_SVC_MODE_MARKFRAME_LEVEL1_KEYWORD, sizeof(RTSPPARSER_SVC_MODE_MARKFRAME_LEVEL1_KEYWORD)))
			{
				pClient->eSVCMode = eSVCMarkFrameLevel1;
				printf("***************I + Layer 0****************\n");
			}
			else if(!rtspstrncasecmp(pcBuf, RTSPPARSER_SVC_MODE_MARKFRAME_LEVEL2_KEYWORD, sizeof(RTSPPARSER_SVC_MODE_MARKFRAME_LEVEL2_KEYWORD)))
			{
				pClient->eSVCMode = eSVCMarkFrameLevel2;
				printf("***************I + Layer 0 + Layer 1****************\n");
			}
			else if(!rtspstrncasecmp(pcBuf, RTSPPARSER_SVC_MODE_MARKFRAME_LEVEL3_KEYWORD, sizeof(RTSPPARSER_SVC_MODE_MARKFRAME_LEVEL3_KEYWORD)))
			{
				pClient->eSVCMode = eSVCMarkFrameLevel3;
				printf("***************I + Layer 0 + Layer 1 + Layer 2****************\n");
			}
			else if(!rtspstrncasecmp(pcBuf, RTSPPARSER_SVC_MODE_MARKFRAME_LEVEL4_KEYWORD, sizeof(RTSPPARSER_SVC_MODE_MARKFRAME_LEVEL4_KEYWORD)))
			{
				pClient->eSVCMode = eSVCMarkFrameLevel4;
				printf("***************I + Layer 0 + Layer 1 + Layer 2 + Layer 3****************\n");
			}
			else if(!rtspstrncasecmp(pcBuf, RTSPPARSER_SVC_MODE_MARKFRAME_LEVEL5_KEYWORD, sizeof(RTSPPARSER_SVC_MODE_MARKFRAME_LEVEL5_KEYWORD)))
			{
				pClient->eSVCMode = eSVCMarkFrameLevel5;
				printf("***************I + Layer 0 + Layer 1 + Layer 2 + Layer 3 + Layer4****************\n");
			}
			else if(!rtspstrncasecmp(pcBuf, RTSPPARSER_SVC_MODE_MARKFRAME_LEVEL6_KEYWORD, sizeof(RTSPPARSER_SVC_MODE_MARKFRAME_LEVEL6_KEYWORD)))
			{
				pClient->eSVCMode = eSVCMarkFrameLevel6;
				printf("***************I + Layer 0 + Layer 1 + Layer 2 + Layer 3 + Layer 4 + Layer 5****************\n");
			}
			else if(!rtspstrncasecmp(pcBuf, RTSPPARSER_SVC_MODE_MARKFRAME_LEVEL7_KEYWORD, sizeof(RTSPPARSER_SVC_MODE_MARKFRAME_LEVEL7_KEYWORD)))
			{
				pClient->eSVCMode = eSVCMarkFrameLevel7;
				printf("***************I + Layer 0 + Layer 1 + Layer 2 + Layer 3 + Layer 4 + Layer 5 + Layer6****************\n");
			}
			else if(!rtspstrncasecmp(pcBuf, RTSPPARSER_SVC_MODE_MARKFRAME_ONLY_KEYWORD, sizeof(RTSPPARSER_SVC_MODE_MARKFRAME_ONLY_KEYWORD)))
			{
				pClient->eSVCMode = eSVCMarkFrameOnly;
				printf("***************I only****************\n");
			}
			else
			{
				printf("Incorrect svc mode!\n");
				return -1;
			}

		}
		else if(!rtspstrncasecmp(acDescriptor, RTSPPARSER_TSMODE_KEYWORD, sizeof(RTSPPARSER_TSMODE_KEYWORD)))
		{
			if(!rtspstrncasecmp(pcBuf, RTSPPARSER_NORMAL_MODE_KEYWORD, sizeof(RTSPPARSER_NORMAL_MODE_KEYWORD)))
			{
				pClient->eHSMode = eHSHistory;
			}
			else if(!rtspstrncasecmp(pcBuf, RTSPPARSER_ADAPTIVE_MODE_KEYWORD, sizeof(RTSPPARSER_ADAPTIVE_MODE_KEYWORD)))
			{
				pClient->eHSMode = eHSAdaptiveRecording;
			}
			else
			{
				printf("Incorrect history streaming mode!\n");
				return -1;
			}
		}
		else if(!rtspstrncasecmp(acDescriptor, RTSPPARSER_CHECK_KEYWORD, sizeof(RTSPPARSER_CHECK_KEYWORD)))
		{
			bShmForceCheck = TRUE;
		}
		else if(!rtspstrncasecmp(acDescriptor, RTSPPARSER_MINSFT_KEYWORD, sizeof(RTSPPARSER_MINSFT_KEYWORD)))
		{
			if((iTemp = atoi(pcBuf)) <= 0)
			{
				printf("Incorrect history streaming minsft number!\n");
				return -1;
			}
			else
			{
				//printf("Minsft is %d seconds\n", iTemp);
				pClient->dwMinSftMSec = iTemp * 1000;
			}
		}
		else if(!rtspstrncasecmp(acDescriptor, RTSPPARSER_REFTIME_KEYWORD, sizeof(RTSPPARSER_REFTIME_KEYWORD)))
		{
			char	*ptMinute = NULL, *ptSecond = NULL;
			
			ptMinute = strchr(pcBuf, ':');
			if(ptMinute == NULL)
			{
				printf("Incorrect reference time structure!\n");
				return -1;
			}

			*ptMinute = '\0';
			iRefTimeMinute = atol(pcBuf);

			if((iRefTimeMinute <= 0 && pcBuf[0] != '0') ||
				iRefTimeMinute < 0 ||
				iRefTimeMinute > 60)
			{
				printf("Incorrect reference minute structure %s!\n", pcBuf);
				return -1;
			}
			
			ptSecond = ptMinute + 1;
			iRefTimeSecond = atol(ptSecond);

			if((iRefTimeSecond <= 0 && ptSecond[0] != '0') ||
				iRefTimeSecond < 0 ||
				iRefTimeSecond > 60)
			{
				printf("Incorrect reference second structure %s!\n", ptSecond);
				return -1;
			}
			printf("Parsed Timeshift reference time is %d minutes %d seconds\n", iRefTimeMinute, iRefTimeSecond);
		}
		//20100812 Added For Client Side Frame Rate Control
		else if( !rtspstrncasecmp(acDescriptor, RTSPPARSER_FRAME_INTERVAL, sizeof(RTSPPARSER_FRAME_INTERVAL)) )
		{
			if( (!rtspstrncasecmp(pcBuf, "-", sizeof("-") - 1)) )
			{
				printf("[%s] Incorrect Frame Interval!\n", __FUNCTION__);
				return -1;
			}
			printf("[%s] pcBuf=%s\n", __FUNCTION__, pcBuf);
			errno = 0;
			ulTemp = strtoul(pcBuf, NULL, 10);
			printf("[%s] ulTemp=%lu\n", __FUNCTION__, ulTemp);
			if( (ulTemp == UNSIGNED_INT_NUMBER) && (errno == ERANGE) )
			{
				printf("[%s] Incorrect Frame Interval, errno=%d(%s)!\n", __FUNCTION__, errno, strerror(errno));
				return -1;
			}
			else if( (ulTemp < 1) || (ulTemp > UNSIGNED_SHORT_NUMBER) )
			{
				printf("[%s] Not support Frame Interval %lu\n", __FUNCTION__, ulTemp);
				return -1;
			}
			else
			{
				pClient->iFrameIntervalMSec = ulTemp;
			}
		}
		//20100105 Added For Seamless Recording
		else if( !rtspstrncasecmp(acDescriptor, RTSPPARSER_SEAMLESS_RECORDING_GUID, sizeof(RTSPPARSER_SEAMLESS_RECORDING_GUID)) )
		{
			//20101208 Modified by danny For GUID format change
			//if( (!rtspstrncasecmp(pcBuf, "-", sizeof("-") - 1)) || (pcBuf[0] == 0) )
			if( pcBuf[0] == 0 )
			{
				printf("[%s] Incorrect Seamless Recording GUID format!\n", __FUNCTION__);
				return -1;
			}
			printf("[%s] pcBuf=%s\n", __FUNCTION__, pcBuf);
			/*errno = 0;
			ulTemp = strtoul(pcBuf, NULL, 10);
			printf("[%s] ulTemp=%lu\n", __FUNCTION__, ulTemp);
			if( (ulTemp == UNSIGNED_INT_NUMBER) && (errno == ERANGE) )
			{
				printf("[%s] Incorrect Seamless Recording GUID, errno=%d(%s)!\n", __FUNCTION__, errno, strerror(errno));
				return -1;
			}
			else
			{*/
				strncpy(pClient->acSeamlessRecordingGUID, pcBuf, RTSPS_Seamless_Recording_GUID_LENGTH - 1);
				pClient->acSeamlessRecordingGUID[RTSPS_Seamless_Recording_GUID_LENGTH - 1] = 0;
				pClient->bSeamlessStream = TRUE;
				printf("[%s] Seamless Recording GUID=%s\n", __FUNCTION__, pClient->acSeamlessRecordingGUID);
			//}
		}
#endif
		else
		{
			printf("Unparsed descriptor \"%s\"\n", acDescriptor);
		}

		pcToken = strtok(NULL, "&");
		if(pcToken == NULL)
		{
			break;
		}
	}

	//Do authentication if username and password is not empty
	if(pClient->acUserName[0] != 0)
	{
		TAuthorInfo     tAuthorInfo;
		int				iRet = 0;

		rtspstrcpy(tAuthorInfo.acAccessName, pClient->acObject, sizeof(tAuthorInfo.acAccessName));
		rtspstrcpy(tAuthorInfo.acUserName, pClient->acUserName, sizeof(tAuthorInfo.acUserName));
		rtspstrcpy(tAuthorInfo.acPasswd, pClient->acPassWord, sizeof(tAuthorInfo.acPasswd));
		tAuthorInfo.iAuthType = RTSPSTREAMING_AUTHENTICATION_BASIC;

		iRet = pClient->parent->fcallback(pClient->parent->hParentHandle
			                ,RTSPSERVER_CALLBACKFLAG_AUTHORIZATION
				            ,&tAuthorInfo,0);
		if(iRet == 0)
		{
			printf("URL authentication passed!\n");
			pClient->bURLAuthenticated = TRUE;
		}
	}


#ifdef _SHARED_MEM
	if(pClient->dwBypasyMSec == 0xffffffff)
	{
		pClient->dwBypasyMSec = 0;
	}
	if(pClient->eHSMode == eHSModeMax)
	{
		pClient->eHSMode = eHSHistory;
	}
	if(pClient->dwMinSftMSec == 0xffffffff)
	{
		pClient->dwMinSftMSec = 0;
	}
	if(iRefTimeMinute == -1 || iRefTimeSecond == -1)
	{
		OSTick_GetMSec(&pClient->dwDescribeMSec);
	}
	else	//Adjustment must be made
	{
		int	iStartTime = 0, iEndTime = 0, iOffset = 0;
		time_t timep;
		struct tm *p;
    
		// receive current time which is the end time
		time(&timep);
		p=gmtime(&timep);
		iEndTime = p->tm_min * 60 + p->tm_sec;

		// calculate start time
		iStartTime = iRefTimeMinute * 60 + iRefTimeSecond;

		// calculate offset
		if(iEndTime >= iStartTime)
		{
			iOffset = iEndTime - iStartTime;
		}
		else
		{
			iOffset = 60 * 60 - iStartTime + iEndTime ;
		}
		printf("Playback time-shift adjusted by %d seconds\n", iOffset);
		//Add offset
		if(iOffset > 0)
		{
			pClient->dwBypasyMSec = pClient->dwBypasyMSec + iOffset * 1000;
			pClient->dwMinSftMSec = pClient->dwMinSftMSec + iOffset * 1000;
		}
	}

	{
		DWORD	dwTSRemain = 0;
		int		iRet = 0;

		//Callback to check how much timeshift is available
		iRet = pClient->parent->fcallback(pClient->parent->hParentHandle
			                ,RTSPSERVER_CALLBACKFLAG_SHMEM_QUERYTIMESHIFT
							,pClient->acObject, &dwTSRemain);

		if(iRet != S_OK)
		{
			printf("%s timeshift error\n", __func__);
		}
		//printf("Remaining Timeshift range is %d MilliSeconds!\n", dwTSRemain);
		//If timeshift available is less than minsft than we should return 415 Media Not Supported
		if(bShmForceCheck && (dwTSRemain < pClient->dwMinSftMSec))
		{
			snprintf(pClient->acShmemReturnString, RTSPPARSER_REPLY_STRING_LENGTH - 1, "Server: RTSP Server (timeshift=%d)", dwTSRemain/1000);
			return -2;
		}
		else if(bShmForceCheck && dwTSRemain == 0xffffffff)
		{
			//Timeshift is not supported and forcechk is enabled, should return 415
			snprintf(pClient->acShmemReturnString, RTSPPARSER_REPLY_STRING_LENGTH - 1, "Server: RTSP Server (timeshift=%d)", -1);
			return -2;
		}
		else if(dwTSRemain == 0xffffffff)
		{
			//Timeshift is not supported, continue to stream live!
			snprintf(pClient->acShmemReturnString, RTSPPARSER_REPLY_STRING_LENGTH - 1, "Server: RTSP Server (timeshift=%d)", -1);
		}
		else 
		{
			snprintf(pClient->acShmemReturnString, RTSPPARSER_REPLY_STRING_LENGTH - 1, "Server: RTSP Server (timeshift=%d)", dwTSRemain/1000);
		}
	}
#endif

	return 0;
}

int RTSPServer_GetCSeq(RTSP_CLIENT* pClient)
{
    char  *pcStartPtr;//, *pcEndPtr;
    //char  acSeq[15];
    unsigned int iCseq,iLen,iRet;

    pcStartPtr = strstr(pClient->acRecvBuffer,"cseq");

    if(pcStartPtr == NULL )
    {
        pcStartPtr = strstr(pClient->acRecvBuffer,"CSeq");
        
        if( pcStartPtr == NULL )
        {
            return -1;
        }
    }
    
    iLen = strspn(pcStartPtr + strlen("CSeq"),": \t");

	iRet= sscanf(pcStartPtr+strlen("CSeq")+iLen,"%d",&iCseq) ;

	if( iRet != 1 )
		return -1;
	else
		return iCseq;

/*    pcEndPtr= strchr(pcStartPtr,'\n');
    strncpy(acSeq,pcStartPtr,pcEndPtr - pcStartPtr);
    pcStartPtr = strtok(acSeq," :");
    pcStartPtr = strtok(NULL,"\0\r");
    iCseq = atoi(pcStartPtr);
    
    return iCseq;	*/

}

#ifdef _SHARED_MEM
//20100105 Added For Media on demand
char* RTSPServer_GetMODHeaderField( int iCode )
{
    TKN   *pTKN;
   
    for ( pTKN = g_MODHeaderField; pTKN->code != -1; pTKN++ )
    {  
        if ( pTKN->code == iCode )
        {
            return pTKN->token ;
        }
    }

    DbgLog((dfCONSOLE|dfINTERNAL,"??? Invalid MOD Header Field Code used." ));
    return g_acInvld_headerfield ;
}
#endif

char* RTSPServer_GetState( int iCode )
{
    TKN   *pTKN;
   
    for ( pTKN = g_Status; pTKN->code != -1; pTKN++ )
    {  
        if ( pTKN->code == iCode )
        {
            return pTKN->token ;
        }
    }

    DbgLog((dfCONSOLE|dfINTERNAL,"??? Invalid Error Code used." ));
    return g_acInvld_method ;
}

int RTSPServer_GetMessageLen( int *piHdrLen, int *piBodyLen, RTSP_CLIENT* pClient)
{
    int   iMsgEnd;     /* end of message found */
    int   iHasMsgBody;      /* message body exists */
    int   iTerminator;      /* terminator count */
    int   iWhiteSpace;      /* white space */
    int   iTotalMsgLen;      /* total message length including any message body */
    int   iMsgBoduLen;      /* message body length */
	int	  iTmpHeaderLength = 0; /* header length by \r\n\r\n */
    char  c;       /* character */
    char  *p;

    iMsgEnd = iHasMsgBody = iTotalMsgLen = iMsgBoduLen = 0;
    while ( iTotalMsgLen <= pClient->iRecvSize )
    {
      /* look for eol. */
        iTotalMsgLen += strcspn( &pClient->acRecvBuffer [iTotalMsgLen], "\r\n" );
        if ( iTotalMsgLen > pClient->iRecvSize )
        {
            break;
        }   
		//ptemp = pClient->acRecvBuffer + iTotalMsgLen;
		//iLeft = strlen(pClient->acRecvBuffer);
      /*
       * work through terminaters and then check if it is the
       * end of the message header.
       */
        iTerminator = iWhiteSpace = 0;
        
        while ( !iMsgEnd && ((iTotalMsgLen + iTerminator + iWhiteSpace) < pClient->iRecvSize) )
        {
            c = pClient->acRecvBuffer [iTotalMsgLen + iTerminator + iWhiteSpace];
            if ( c == '\r' || c == '\n' )
            {
                iTerminator++;
            }
            else if ( (iTerminator < 3) && ((c == ' ') || (c == '\t')) )
            {
                iWhiteSpace++; /* white space between lf & cr is sloppy, but tolerated. */
            }   
            else
            {
                break;
            }                
        }
      /*
       * cr,lf pair only counts as one end of line terminator.
       * Double line feeds are tolerated as end marker to the message header
       * section of the message.  This is in keeping with RFC 2068,
       * section 19.3 Tolerant Applications.
       * Otherwise, CRLF is the legal end-of-line marker for all HTTP/1.1
       * protocol compatible message elements.
       */
        if ( (iTerminator > 2) || ((iTerminator == 2) && (pClient->acRecvBuffer [iTotalMsgLen] == pClient->acRecvBuffer [iTotalMsgLen + 1])) )
        {
            iMsgEnd = 1;          /* must be the end of the message header */
			iTmpHeaderLength = iTotalMsgLen + iTerminator + iWhiteSpace;
        }
        
        iTotalMsgLen += iTerminator + iWhiteSpace;

        if ( iMsgEnd )
        {
            iTotalMsgLen += iMsgBoduLen;      /* add in the message body length */
            break;         /* all done finding the end of the message. */
        }
        
        if ( iTotalMsgLen >= pClient->iRecvSize )
        {
            break;
        }
      /*
       * check first token in each line to determine if there is
       * a message body.
       */
        if ( !iHasMsgBody )     /* content length token not yet encountered. */
        {
            if ( !rtspstrncasecmp( &pClient->acRecvBuffer [iTotalMsgLen],"Content-Length",
                                                  sizeof(g_acContentLength)-1 ) )
            {
                iHasMsgBody = 1;     /* there is a message body. */
                iTotalMsgLen += sizeof(g_acContentLength)-1;
                
                while ( iTotalMsgLen < pClient->iRecvSize )
                {
                    c = pClient->acRecvBuffer [iTotalMsgLen];
                    
                    if ( (c == ':') || (c == ' ') )
                    {
                        iTotalMsgLen++;
                    }                        
                    else
                    {
                        break;
                    }                        
                }

                if ( sscanf( &pClient->acRecvBuffer [iTotalMsgLen], "%d", &iMsgBoduLen ) != 1 )
                {
                    DbgLog((dfCONSOLE|dfINTERNAL,"ALERT: invalid ContentLength encountered in message."));
                    return 1;
                }
				else if (iMsgBoduLen < 0 || iMsgBoduLen > pClient->iRecvSize)
				{
					iMsgBoduLen = 0;
					DbgLog((dfCONSOLE|dfINTERNAL,"ALERT: invalid ContentLength encountered in message."));
                    return 1;
				}
            }
        }
    }
	/* 20081231 Body length check , Header + body should not exceed totoal received message size */
	if(iTmpHeaderLength + iMsgBoduLen > pClient->iRecvSize)
	{
		iMsgBoduLen = 0;
		DbgLog((dfCONSOLE|dfINTERNAL,"ALERT: buffer did not contain the entire RTSP message or content-length is invalid"));
        return 1;
	}

    if ( iTotalMsgLen > pClient->iRecvSize )
    {
        DbgLog((dfCONSOLE|dfINTERNAL,"PANIC: buffer did not contain the entire RTSP message." ));
        return 1;
    }

   /*
    * go through any trailing nulls.  Some servers send null terminated strings
    * following the body part of the message.  It is probably not strictly
    * legal when the null byte is not included in the Content-Length count.
    * However, it is tolerated here.
    */

    *piHdrLen = iTotalMsgLen - iMsgBoduLen;
    for ( iTerminator = pClient->iRecvSize - iTotalMsgLen, p = &pClient->acRecvBuffer [iTotalMsgLen];
         iTerminator && (*p == '\0'); p++, iMsgBoduLen++, iTerminator-- );
         
    *piBodyLen = iMsgBoduLen;
    
    return 0;
}

void RTSPServer_RemoveMessage( int len, RTSP_CLIENT* pClient)
{
   pClient->iRecvSize -= len;
   pClient->iUnfinishedRTSPSize = pClient->iRecvSize;

   if( pClient->iUnfinishedRTSPSize != 0 )
		printf("unfinished RTSP message %d\r\n",pClient->iUnfinishedRTSPSize);
   
   if ( pClient->iRecvSize > 0 && pClient->iRecvSize < MAX_RECVBUFF && len )    /* discard the message from the in_buffer. */
	{
		 memmove( pClient->acRecvBuffer, &pClient->acRecvBuffer[len], pClient->iRecvSize ); 
		 pClient->iUnfinishedRTSPSize = 0;
	}

//    memset(pClient->acRecvBuffer,0,MAX_RECVBUFF);
//    pClient->iRecvSize = 0;
}

void RTSPServer_DiscardMessage( RTSP_CLIENT* pClient )
{
    int   iHeaderLen;      /* message header length */
    int   iMsgBodyLen;      /* message body length */
   
    if ( RTSPServer_GetMessageLen( &iHeaderLen, &iMsgBodyLen, pClient ) == 0 )
    {
        RTSPServer_RemoveMessage( iHeaderLen + iMsgBodyLen,pClient );    /* now discard the entire message. */
    }
}

void RTSPServer_StrToLower( char *pcSource, int iLen )
{
   while ( iLen )
   {
      *pcSource = tolower( *pcSource );
      pcSource++;
      iLen--;
   }
}

int RTSPServer_GetAuthorInfo(char *pcMsgBuffer,TRAWAUTHORINFO *ptRawAuthorInfo)
{
    char *pcChar = NULL,*pcEnd = NULL,*pcTemp=NULL ;
    int	iAuthenType;
    pcChar = strstr(pcMsgBuffer,"Authorization");
    
    if( pcChar == NULL )
        return -1;
    else
    {
		/*if( iAuthenType == RTSP_AUTH_BASIC )
			pcChar = strstr(pcChar+13,"Basic");
		
		if( iAuthenType == RTSP_AUTH_DIGEST )
			pcChar = strstr(pcChar+13,"Digest");*/
		if( (pcTemp = strstr(pcChar+13,"Basic")) != NULL )
		{
			iAuthenType = ptRawAuthorInfo->iAuthMode = RTSP_AUTH_BASIC;
		}
		else if( (pcTemp = strstr(pcChar+13,"Digest")) != NULL )
		{
			iAuthenType = ptRawAuthorInfo->iAuthMode = RTSP_AUTH_DIGEST;
		}
		else
		{
			return -1;	       
		}

		pcChar = pcTemp;
            		                
			if(iAuthenType == RTSP_AUTH_BASIC )
			{
				pcChar = pcChar + strlen("Basic") + 1;      
				pcEnd = strchr(pcChar,'\r');

				if( pcEnd == NULL)
				{
					if(( pcEnd = strchr(pcChar,'\n')) == NULL )
						return -1;             
				}   

				if( pcEnd-pcChar < AUTH_RESPONCE_LENGTH )
				{
					memcpy(ptRawAuthorInfo->acResponse,pcChar,pcEnd-pcChar);
					ptRawAuthorInfo->acResponse[pcEnd-pcChar] = 0;
					return 0;
				}
				else
					return -1;
			}
			else if( iAuthenType == RTSP_AUTH_DIGEST )
			{
				if( ( pcTemp = strstr(pcChar,"username=\"") )== NULL )
					return -1;
				else
				{
					pcTemp = pcTemp + strlen("username=\""); 
					pcEnd = strchr(pcTemp,'"');

					if( pcEnd == NULL )
						return -1;
					else
					{	
						if( pcEnd-pcTemp < AUTH_USER_LENGTH )
						{
							memcpy(ptRawAuthorInfo->acUserName,pcTemp,pcEnd-pcTemp);
							ptRawAuthorInfo->acUserName[pcEnd-pcTemp] = 0;
						}
						else
							return -1;
					}
				}

				if( ( pcTemp = strstr(pcChar,"nonce=\"") )== NULL )
					return -1;
				else
				{
					pcTemp = pcTemp + strlen("nonce=\"") ; //skip '=' and '"'
					pcEnd = strchr(pcTemp,'"');
					if( pcEnd == NULL )
						return -1;
					else
					{
						if( pcEnd-pcTemp < AUTH_NONCE_LENGTH )
						{
							memcpy(ptRawAuthorInfo->acNonce,pcTemp,pcEnd-pcTemp);
							ptRawAuthorInfo->acNonce[pcEnd-pcTemp] = 0;
						}
						else
							return -1;
					}

				}
				
				//20140221 modified by Charles to follow the method of RTSP digest auth defined in RFC2069
				/*if( ( pcTemp = strstr(pcChar,"nc=") )== NULL )
					return -1;
				else
				{
					pcTemp = pcTemp + strlen("nc="); 
					pcEnd = strchr(pcTemp,',');
					
					//20131111 modified by Charles for digest authentication fail when using ffmpeg
					if( pcEnd == NULL )
					{
						if((pcEnd = strchr(pcTemp,'\r')) == NULL) //if it is the last one!
						{
							return -1;
						}
					}
					//else
					//{
						if( pcEnd-pcTemp < 9 )
						{
							memcpy(ptRawAuthorInfo->acNonceCount,pcTemp,pcEnd-pcTemp);
							ptRawAuthorInfo->acNonceCount[pcEnd-pcTemp] = 0;
						}
						else
							return -1;
					//}
				}*/

				/*if( ( pcTemp = strstr(pcChar,"cnonce=\"") )== NULL )
					return -1;
				else
				{
					pcTemp = pcTemp + strlen("cnonce=\"");
					pcEnd = strchr(pcTemp,'"');

					if( pcEnd == NULL )
						return -1;
					else
					{
						if( pcEnd-pcTemp < AUTH_NONCE_LENGTH )
						{
							memcpy(ptRawAuthorInfo->acCNonce,pcTemp,pcEnd-pcTemp);
							ptRawAuthorInfo->acCNonce[pcEnd-pcTemp] = 0;
						}
						else
							return -1;
					}
				}*/

				if( ( pcTemp = strstr(pcChar,"response=\"") )== NULL )
					return -1;
				else
				{
					pcTemp = pcTemp + strlen("response=\"");
					pcEnd = strchr(pcTemp,'"');

					if( pcEnd == NULL )
						return -1;
					else
					{
						if( pcEnd-pcTemp < AUTH_RESPONCE_LENGTH )
						{
							memcpy(ptRawAuthorInfo->acResponse,pcTemp,pcEnd-pcTemp);
							ptRawAuthorInfo->acResponse[pcEnd-pcTemp] = 0;
						}
						else
							return -1;
					}
				}
				//20131111 added by Charles for digest authentication fail when using ffmpeg
				if( ( pcTemp = strstr(pcChar,"uri=\"") )== NULL )
					return -1;
				else
				{
					pcTemp = pcTemp + strlen("uri=\""); 
					pcEnd = strchr(pcTemp,'"');

					if( pcEnd == NULL )
						return -1;
					else
					{	
						if( pcEnd-pcTemp < AUTH_URI_LENGTH )
						{
							memcpy(ptRawAuthorInfo->acURI,pcTemp,pcEnd-pcTemp);
							ptRawAuthorInfo->acURI[pcEnd-pcTemp] = 0;
						}
						else
							return -1;
					}
				}			

				return 0;							
			}
			else
			{
				return -1;
			}
        }                            
}
//20090204 Add max buffer length to avoid buffer overrun
int RTSPServer_GetString( char *pcMsgBuffer, int iMsgHeaderLen, char *pcToken, char *pcSep, char* pcStringLine, int iBufLength)
{
    char     acBuffer[MAX_RECVBUFF];
    char     acTokenBuf[TOKENLEN];  /* temporary buffer for protected string constants. */
    int      iTokenLen, iStringLineLength = 0;          /* token length */
    int      n=0;
    char     *pc = NULL,*pcEnd;

 
    iTokenLen = strlen( pcToken );

	if( iMsgHeaderLen < MAX_RECVBUFF )
		strncpy(acBuffer, pcMsgBuffer, iMsgHeaderLen );   /* create a null terminated string */
	else
		return 1;

    acBuffer[iMsgHeaderLen] = '\0';           /* make sure the string is terminated */
    rtspstrcpy( acTokenBuf, pcToken, sizeof(acTokenBuf) );          /* create a temporary copy. */
 
    RTSPServer_StrToLower( acBuffer, iMsgHeaderLen );
    RTSPServer_StrToLower( acTokenBuf, iTokenLen );
    
   /* now see if the header setting token is present in the message header */
    if ( (pc = strstr( acBuffer, acTokenBuf )) )
    {
        if ( (n = strspn( pc + iTokenLen, pcSep )) || (!strlen(pcSep)))
        {
            pc += iTokenLen + n;
//            ulIndex = pc - acBuffer;
//            pc = acBuffer + ulIndex;
        }

	    pcEnd = strchr(pc,'\n');
		//20090204 Buffer length check to avoid buffer overrun
		if(	(pcEnd-pc) > (iBufLength - 1))
		{
			iStringLineLength = iBufLength - 1;
			syslog(LOG_INFO, "RTSP signaling extension is too long %d bytes!\n", pcEnd-pc);
		}
		else
		{
			iStringLineLength = pcEnd-pc;
		}

	    memcpy(pcStringLine,pc, iStringLineLength);
		pcStringLine[iStringLineLength] = 0;

		return 0;
    }
	else   
	{       
        return 1 ;
    }                
}

int RTSPServer_GetClientRTPPort(char* pcBuf,unsigned short *pusRTPPort,unsigned short *pusRTCPPort)
{
    char *pcStart,*pcEnd;
    char acBuf[MAX_LINE_LEN];

    pcStart = strstr(pcBuf,"client_port");
    
    if( pcStart == NULL )
    {
        return 1;
    }
         
    pcEnd = strchr(pcStart,'\n');
  
	if( pcEnd-pcStart < MAX_LINE_LEN )
	{
	   memcpy(acBuf,pcStart,pcEnd-pcStart);
	   acBuf[pcEnd-pcStart] = 0;
	}
	else
		return 2;

    pcStart = strtok(acBuf," =");

    pcStart = strtok(NULL," -;");

    *pusRTPPort = atoi(pcStart);
    pcStart = strtok(NULL," \n");
    
    if( pcStart != NULL )
    {
        *pusRTCPPort = atoi(pcStart);
    }
    
    if( *pusRTPPort == 0 )
    {
        return 1;
    }        
    else 
    {
        return 0;
    }        
}

int   RTSPServer_GetInterleavedID(char *pcMsgBuffer,int *piRTPID, int *piRTCPID)
{
	char *pChar;

	if( pcMsgBuffer == NULL )
		return -1;

	if( (pChar = strstr(pcMsgBuffer,"interleaved=")) == NULL )
	{
		return -1;
	}
	else
	{
		pChar = pChar + 12;
		if(!sscanf( pChar, "%d-%d",piRTPID,piRTCPID ) )
			return -1;
		else
			return 0;
	}
}

int RTSPServer_GetNumber( char *pcMsgBuffer, int iMsgHeaderLen, char *pcToken, char *pcSep, unsigned long *pulNumber, long *plNumber )
{
    char     acBuffer [MAX_RECVBUFF];
    char     acToken [TOKENLEN];  /* temporary buffer for protected string constants. */
    int      iTokenLen;          /* token length */
    int      iRet;
    unsigned long   ulIndex;
    char     *pcPointer;

   /* prepare the strings for comparison */
    iTokenLen = strlen( pcToken );

	memset(acBuffer,0,MAX_RECVBUFF);
	if( iMsgHeaderLen < MAX_RECVBUFF)
		strncpy( acBuffer, pcMsgBuffer, iMsgHeaderLen );   
	else
		return 1;

    rtspstrcpy( acToken, pcToken, sizeof(acToken) );          
   
    RTSPServer_StrToLower( acBuffer, iMsgHeaderLen );
    RTSPServer_StrToLower( acToken, iTokenLen );

   /* now see if the header setting token is present in the message header */
    if ( (pcPointer = strstr( acBuffer, acToken )) )
    {
        if ( (iRet = strspn( pcPointer + iTokenLen, pcSep )) )
        {
            pcPointer += iTokenLen + iRet;
            
            if ( pulNumber )
            {
                iRet = sscanf( pcPointer, "%u", (unsigned int*)pulNumber );
				
            }
            
            if ( iRet && plNumber )
            {
                iRet = sscanf( pcPointer, "%ld", plNumber );
            }
            
            if ( iRet == 1 )
            {
                ulIndex = pcPointer - acBuffer;
                pcPointer = pcMsgBuffer + ulIndex;
            }
            else
            {
                pcPointer = 0;     /* just ignore bad value. */
            }    
        }
        else
        {
            pcPointer = 0;
        }   
    }  
    else
    {
        pcPointer = 0;            /* there was not a valid separater. */
    }
    
    if( pcPointer == 0 )
    {
        return 1;
    }       
    else
    {
        return 0;
    }        
    
}


int RTSPServer_IsValidMethod( char *pcFirstLine,RTSP_CLIENT *pClient,RTSP_SERVER *pServer)
{
    TKN            *TKNMethod;
    char           acMethod [32];
    char           acObject [256];
    char           acVer [32];
    int            iRet;          /* parameter count */

   /*
    * Check for a valid method token and convert
    * it to a switch code.
    * Returns -2 if there are not 4 valid tokens in the request line.
    * Returns -1 if the Method token is invalid.
    * Otherwise, the method code is returned.
    */
    *acMethod = *acObject = '\0';

   /* parse first line of message header as if it were a request message */
    iRet = sscanf( pcFirstLine, " %31s %255s %31s", acMethod, acObject, acVer);

    if ( iRet != 3 )
    {
        RTSPServer_DiscardMessage(pClient); /* remove remainder of message from in_buffer */
        return( -2 );
    }

    for ( TKNMethod = g_Methods; TKNMethod->code != -1; TKNMethod++ )
    {
        if ( !strcmp( TKNMethod->token, acMethod ) )
        {
            break;
        }
    }
    
    if ( TKNMethod->code == -1 )
    {
        RTSPServer_DiscardMessage(pClient); /* remove remainder of message from in_buffer */
        RTSPServer_SendReply( 400, 0, pClient,pServer );   /* Bad Request */
    }

    return( TKNMethod->code );
}





