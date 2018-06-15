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
 *  File name          :   msg_handler.c 
 *  File description   :   RTSP message hanlder 
 *  Author             :   ShengFu
 *  Created at         :   2002/4/24 
 *  Note               :   
 *	$Log: /RD_1/Protocol/RTSP/Server/rtspstreamserver/rtspserver/src/msg_handler.c $
 * 
 * 3     06/06/21 5:16p Shengfu
 * 
 * 2     06/05/18 3:26p Shengfu
 * update to version 1.4.0.0
 * 
 * 13    06/01/23 4:21p Shengfu
 * upgrade to ver 1.3.0.7
 * 
 * 12    06/01/10 5:48p Shengfu
 * update to 1.3.0.6
 * 
 * 10    05/11/30 6:23p Shengfu
 * update to version 1.3.0.4
 * 
 * 9     05/11/03 11:57a Shengfu
 * update to version 1.3.0.3
 * 
 * 8     05/10/07 8:48a Shengfu
 * 
 * 7     05/09/27 1:14p Shengfu
 * update to version 1,3,0,1
 * 
 * 5     05/08/26 10:02a Shengfu
 * 
 * 4     05/08/19 11:49a Shengfu
 * 
 * 1     05/08/19 11:30a Shengfu
 * 
 * 3     05/08/10 9:01a Shengfu
 * update rtspstreaming server which enable multicast
 * 
 * 8     05/07/13 2:26p Shengfu
 * update rtsp streaming server
 * 
 * 7     05/04/15 1:35p Shengfu
 * 1. multicast added, but disable
 * 2. RTP extension added
 * 
 * 5     05/02/01 2:47p Shengfu
 * 
 * 4     05/01/24 10:02a Shengfu
 * 
 * 3     05/01/18 3:11p Shengfu
 * 1. ignore RTCP receiver report in RTP over RTSP mode
 * 2. if RTSP receiver any illegal message, release the client
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
 * 9     02/07/29 3:25p Shengfu
 * system log simplified
 * 
 * 8     02/07/29 2:22p Shengfu
 * system log simplified
 * 
 * 7     02/07/19 9:32a Shengfu
 * modify both video/audio the same SSRC
 * 
 * 6     02/07/04 11:07a Simon
 * 1. Fix OutSendMessage bug
 * 2. Change SDP request callback method
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
#include <time.h>

#include "../../rtspstreamingserver/inc/rtspstreamingserver.h"
#include "rtspserver.h"
#include "rtsp_server_local.h"
#include "parser.h"
#include "account_mgr_app.h"
/* Added by cchuang, 2004/11/22 */
#ifdef _LINUX
#define SEND_FLAGS  MSG_NOSIGNAL
#else
#define SEND_FLAGS  0
#endif

char* ConvertWeekday(WORD wWeekday)
{
    switch(wWeekday)
    {
        case 1:
            return "Mon";                
        case 2:
            return "Tue";
        case 3:
            return "Wed";
        case 4:
            return "Thu";
        case 5:
            return "Fri";
        case 6:
            return "Sat";
        case 0:
            return "Sun";        
        default:
            return "XXX";    
    }    
    
}

char* ConvertMonth(WORD wMonth)
{
    switch(wMonth)
    {
        case 1:
            return "Jan";
        case 2:
            return "Feb";
        case 3:
            return "Mar";
        case 4:
            return "Apr";
        case 5:
            return "May";
        case 6:
            return "Jun";
        case 7:
            return "Jul";        
        case 8:
            return "Aug";                
        case 9:
            return "Sep";
        case 10:
            return "Oct";
        case 11:
            return "Nov";
        case 12:
            return "Dec";
        default:
            return "XXX";    
    }    
    
}


unsigned long RTSPServer_GetRandomULong(void)
{
   	unsigned long ulValue=0,ulTemp;

    OSTick_GetCycle(&ulTemp,&ulValue);
    
    return ulValue;
}

unsigned long RTSPServer_GetSessionID(void)
{
	unsigned long ulValue=0;
	
	while(ulValue==0 || ulValue==1 || ulValue==2)
	{
        ulValue = RTSPServer_GetRandomULong();
    }	
    return  ulValue;
}

unsigned short RTSPServer_GetInitSequence(void)
{
    unsigned long ulValue=0,ulTemp;
    
    OSTick_GetCycle(&ulTemp,&ulValue);
    ulValue = ulValue%0xffff;
    return (short)ulValue;
}

unsigned long RTSPServer_GetInitTimestamp(void)
{
    return RTSPServer_GetRandomULong();
}

unsigned long RTSPServer_GetSSRC(void)
{
    return RTSPServer_GetRandomULong();
}

//20120925 modified by Jimmy for ONVIF backchannel
int RTSPServer_GetServerRTPport(int iFlag, unsigned short usPort[2], RTSP_CLIENT *pClient, int iMediaType)
{
    RTSP_SOCKADDR addr1;
    RTSP_SOCKADDR addr2;
     
	int iInput = 0;
	int iIndex=0;
#ifdef _SHARED_MEM
	//20091116 support connected UDP
	int 	iOpt;
#else
	int	n=0;
#endif

#ifdef _INET6
	int iSocketAF = AF_INET6;
#else
	int iSocketAF = AF_INET;
#endif

    int *psktRTP;
	int *psktRTCP;

    if( iMediaType == SETUP_AUDIOBACK)
    {
        psktRTP = &pClient->parent->iAudiobackSock[iFlag][0];
        psktRTCP = &pClient->parent->iAudiobackSock[iFlag][1];
    }
    else
    {
        psktRTP = &pClient->rtp_sock[iFlag][0];
        psktRTCP = &pClient->rtp_sock[iFlag][1];
    }
		


	
    
    if( (*psktRTP=socket(iSocketAF,SOCK_DGRAM,0)) <= 0 )
    {
        return -1;
    }        
    
    if( (*psktRTCP=socket(iSocketAF,SOCK_DGRAM,0)) <= 0 )
    {
        return -1;
    }
	
#ifdef _SHARED_MEM
	//20091116 support connected UDP
	iOpt = 1;
	if( (setsockopt(*psktRTP, SOL_SOCKET, SO_REUSEADDR, (char *) &iOpt, sizeof(iOpt))) !=0 )
	{
		
		printf("[%s] Could not set SO_REUSEADDR: %s\n", __FUNCTION__, strerror(errno));
		return S_FAIL;
	}
	
	if( (setsockopt(*psktRTCP, SOL_SOCKET, SO_REUSEADDR, (char *) &iOpt, sizeof(iOpt))) !=0 )
	{
		
		printf("[%s] Could not set SO_REUSEADDR: %s\n", __FUNCTION__, strerror(errno));
		return S_FAIL;
	}
#endif

	memset(&addr1,0,sizeof(addr1));
	//CID:1179, CHECKER:UNINIT
	iInput = sizeof(addr1);
    getsockname(*psktRTP,(struct sockaddr *)&addr1,(unsigned int *)&iInput);//sizeof(struct sockaddr_in));
#ifdef _INET6
	addr1.sin6_family = AF_INET6;
#else
    addr1.sin_family = AF_INET;
#endif

    memset(&addr2,0,sizeof(addr2));
    getsockname(*psktRTCP,(struct sockaddr *)&addr2,(unsigned int *)&iInput);//sizeof(struct sockaddr_in));
 #ifdef _INET6
	addr2.sin6_family = AF_INET6;
#else
    addr2.sin_family = AF_INET;
#endif

 
    while(iIndex<2)
    {  
#ifdef _SHARED_MEM
		//20091116 support connected UDP
		//20091230 Fixed Audio only port disorder issue
		if (iMediaType == SETUP_VIDEO) //Video port assign
		{
			usPort[0] = pClient->parent->rtsp_param.usRTPVPort;
        	usPort[1] = usPort[0] + 1;
		}
		if ( (iMediaType == SETUP_AUDIO) || (iMediaType == SETUP_AUDIOBACK) ) //Audio port assign
		{
			usPort[0] = pClient->parent->rtsp_param.usRTPAPort;
        	usPort[1] = usPort[0] + 1;
		}
		//20120726 added by Jimmy for metadata
		if (iMediaType == SETUP_METADATA) //Metadata port assign
		{
			usPort[0] = pClient->parent->rtsp_param.usRTPMPort;
        	usPort[1] = usPort[0] + 1;
		}

#else
		usPort[0] = RTP_PORT + n;
        usPort[1] = usPort[0] + 1;
#endif

#ifdef _INET6
		addr1.sin6_port = htons(usPort[0]);
        addr2.sin6_port = htons(usPort[1]);
#else
        addr1.sin_port = htons(usPort[0]);
        addr2.sin_port = htons(usPort[1]);
#endif
        
        if(!bind(*psktRTP,(struct sockaddr *) &addr1,sizeof(addr1)))
        {
            iIndex ++;
        }
#ifdef _SHARED_MEM
		else
		{
			printf("[%s] Could not bind *psktRTP: %s\n", __FUNCTION__, strerror(errno));
			return -1;
		}
#endif
		if(!bind(*psktRTCP,(struct sockaddr *) &addr2,sizeof(addr2)))
        {
            iIndex ++;
        }
#ifdef _SHARED_MEM
		else
		{
			printf("[%s] Could not bind *psktRTCP: %s\n", __FUNCTION__, strerror(errno));
			return -1;
		}
#endif

#ifndef _SHARED_MEM
		//20091116 support connected UDP       
        n = n + 2 ;
        
        if( n > 200 )
        {
            return -1;
        }
#endif
    }

	printf("Server Side RTP-RTCP Port %d-%d\r\n",usPort[0],usPort[1]);

    return 0;
}

#ifdef RTSPRTP_MULTICAST
void RTSPServer_FillMulticastSessison(RTSPSERVER_SESSIONINFORMATION *pSession_info, RTSP_CLIENT* pClient)
{
    int i;
	//int iIndex ;

	//20120724 modified by Jimmy for metadata
	/*
	if( pClient->acMediaType[1][0] == 0 )
	 	iIndex = 1;
	else
 		iIndex = 2;
	*/

    for( i =0 ; i< MEDIA_TYPE_NUMBER ; i++ )
    {
        //20120724 added by Jimmy for metadata
        if( pClient->acMediaType[i][0] == 0 )
            break;
        pSession_info->dwInitialTimeStamp[i] = pClient->ulInitTimestamp[i];
        pSession_info->wInitialSequenceNumber[i] = pClient->usInitSequence[i];
        pSession_info->dwSSRC[i]= pClient->ulSSRC[i];
		pSession_info->dwSessionID = pClient->ulSessionID;
		//CID:64, CHECKER:BUFFER_SIZE_WARNING
        rtspstrcpy(pSession_info->cMediaName[i],pClient->acMediaType[i], sizeof(pSession_info->cMediaName[i]));
    }

    //2006/09/21 ShengFu bug fixed for multiple stream backchannel multicast
	/*if( pClient->iVivotekClient )
		pSession_info->iMulticast = RTSP_MULTICASTNUMBER;
	else
		pSession_info->iMulticast = RTSP_MULTICASTNUMBER -1;*/
		
	pSession_info->iRTPStreamingType = RTP_OVER_UDP;
    pSession_info->iRTCPTimeOut = 10;
	pSession_info->iVivotekClient = pClient->iVivotekClient;
    //20130904 modified by Charles for ondemand multicast
	pSession_info->iMulticast = pClient->iMulticast;
    pSession_info->iSDPIndex =  pClient->iSDPIndex;
	
	pSession_info->ulClientIP = pClient->ulClientAddress;
    //20141110 added by Charles for ONVIF Profile G
    pSession_info->iCseq = pClient->iCSeq;
	printf("Multicast of stream type %d is required from RTSP\r\n",pClient->iSDPIndex);
}

//20130924 added by Charles for ondemand multicast
SCODE RTSPServer_AddOnDemandMulticast(RTSP_CLIENT *pClient, RTSP_SERVER *pServer)
{
	int	i;
	
	for( i=0 ; i < RTSP_MULTICASTNUMBER; i++)
	{
        if ( pClient->parent->ulMulticastAddress[i] == pClient->tOndemandMulticastInfo.ulMulticastAddress
			&& (pClient->tOndemandMulticastInfo.usMulticastVideoPort==0 ? TRUE :(pClient->parent->usMulticastVideoPort[i] == pClient->tOndemandMulticastInfo.usMulticastVideoPort))
			&& (pClient->tOndemandMulticastInfo.usMulticastAudioPort==0 ? TRUE :(pClient->parent->usMulticastAudioPort[i] == pClient->tOndemandMulticastInfo.usMulticastAudioPort))
#ifdef _METADATA_ENABLE
			&& (pClient->tOndemandMulticastInfo.usMulticastMetadataPort==0 ? TRUE :(pClient->parent->usMulticastMetadataPort[i] == pClient->tOndemandMulticastInfo.usMulticastMetadataPort))
#endif
			)
		{
			if(pClient->parent->iMulticastSDPIndex[i] == pClient->iSDPIndex)
			{
				pClient->iMulticast = pClient->iSDPIndex;
				pClient->parent->iCurrMulticastNumber[pClient->iMulticast-1] ++;	    
				TelnetShell_DbgPrint("current %d max %d\r\n",pClient->parent->iCurrMulticastNumber[pClient->iMulticast-1],pClient->parent->iMaxMulticastNumber[pClient->iMulticast-1]);
            	return S_OK;
			}
			else
			{
				break; //handle by CheckIfRepeatMulticastInfo
			}
		}
	}
	for( i=0 ; i < RTSP_ONDEMAND_MULTICASTNUMBER; i++)
	{
		if ( pClient->parent->ulMulticastAddress[i + RTSP_MULTICASTNUMBER] == pClient->tOndemandMulticastInfo.ulMulticastAddress
			 && (pClient->tOndemandMulticastInfo.usMulticastVideoPort==0 ? TRUE :(pClient->parent->usMulticastVideoPort[i +RTSP_MULTICASTNUMBER] == pClient->tOndemandMulticastInfo.usMulticastVideoPort))
			 && (pClient->tOndemandMulticastInfo.usMulticastAudioPort==0 ? TRUE :(pClient->parent->usMulticastAudioPort[i +RTSP_MULTICASTNUMBER] == pClient->tOndemandMulticastInfo.usMulticastAudioPort))
#ifdef _METADATA_ENABLE
			 && (pClient->tOndemandMulticastInfo.usMulticastMetadataPort==0 ? TRUE :(pClient->parent->usMulticastMetadataPort[i+RTSP_MULTICASTNUMBER] == pClient->tOndemandMulticastInfo.usMulticastMetadataPort))
#endif
			)
		{
			if(pClient->parent->iMulticastSDPIndex[i + RTSP_MULTICASTNUMBER] == pClient->iSDPIndex)
			{
			 	//pClient->iMulticast = pClient->iSDPIndex + RTSP_MULTICASTNUMBER + RTSP_AUDIO_EXTRA_MULTICASTNUMBER;
			 	pClient->iMulticast = i+1 + RTSP_MULTICASTNUMBER;
				pClient->parent->iCurrMulticastNumber[pClient->iMulticast-1] ++;	    
				TelnetShell_DbgPrint("current %d max %d\r\n",pClient->parent->iCurrMulticastNumber[pClient->iMulticast-1],pClient->parent->iMaxMulticastNumber[pClient->iMulticast-1]);
	           	return S_OK;
			}
			else
			{
				break; //handle by CheckIfRepeatMulticastInfo
			}
		}
	}
	for( i=0 ; i < RTSP_ONDEMAND_MULTICASTNUMBER; i++)
	{
		if(pClient->parent->ulMulticastAddress[i + RTSP_MULTICASTNUMBER] == 0)
		{
			break;						 
		}
	}
	if(i == RTSP_ONDEMAND_MULTICASTNUMBER) //No Available ondemand multicast
	{
		pClient->parent->iCurrMulticastNumber[pClient->iMulticast-1] ++; //workaround to avoid iCurrMulticastNumber to be wrong
		RTSPServer_DiscardMessage(pClient); 
		RTSPServer_SendReply(503, 0,pClient,pServer);
		RTSPServer_InitClient(pClient);
		return S_FAIL;		
	}
		
    //start to add new multicast
    //removed by faber, prevent omnicast retry
    // if(CheckIfRepeatMulticastInfo(pClient, pServer) != 0)
    // {
    // 	pClient->parent->iCurrMulticastNumber[pClient->iMulticast-1] ++; //workaround to avoid iCurrMulticastNumber to be wrong
    //     RTSPServer_DiscardMessage(pClient); 
    //     RTSPServer_SendReply(400, 0,pClient,pServer);
    //     RTSPServer_InitClient(pClient);
    //     return S_FAIL;       
    // }
	
	//create new multicast group!!
	//pClient->iMulticast = pClient->iSDPIndex + RTSP_MULTICASTNUMBER + RTSP_AUDIO_EXTRA_MULTICASTNUMBER;
	pClient->iMulticast = i+1 + RTSP_MULTICASTNUMBER;
	pClient->parent->iCurrMulticastNumber[pClient->iMulticast-1] ++;	    
	TelnetShell_DbgPrint("current %d max %d\r\n",pClient->parent->iCurrMulticastNumber[pClient->iMulticast-1],pClient->parent->iMaxMulticastNumber[pClient->iMulticast-1]);
   	pClient->tOndemandMulticastInfo.iEnable = 1;
   	pClient->tOndemandMulticastInfo.iAlwaysMulticast = 0;
   	pClient->tOndemandMulticastInfo.iSDPIndex = pClient->iSDPIndex;
   	pClient->tOndemandMulticastInfo.iRRAlive =  0;  //if we need to sync with config file, need to modify here
   	//pClient->tMulticastInfo.usTTL= pClient->parent->usTTL[(pClient->iSDPIndex)-1];
   	pClient->tOndemandMulticastInfo.iRTPExtension= pClient->parent->iRTPExtension[(pClient->iSDPIndex)-1];

	unsigned short usTempPort;
	//workaround to avoid create multicast socket failed
	if(pClient->tOndemandMulticastInfo.usMulticastVideoPort != 0)
	{
		usTempPort = pClient->tOndemandMulticastInfo.usMulticastVideoPort;
	}
	else if(pClient->tOndemandMulticastInfo.usMulticastAudioPort != 0)
	{
		usTempPort = pClient->tOndemandMulticastInfo.usMulticastAudioPort;
	}
#ifdef _METADATA_ENABLE				
	else if(pClient->tOndemandMulticastInfo.usMulticastMetadataPort != 0)
	{
		usTempPort = pClient->tOndemandMulticastInfo.usMulticastMetadataPort;
	}
#endif	
	else
	{
		return S_FAIL;
	}

	if(pClient->tOndemandMulticastInfo.usMulticastVideoPort == 0)
	{
		pClient->tOndemandMulticastInfo.usMulticastVideoPort = usTempPort;
	}
	if(pClient->tOndemandMulticastInfo.usMulticastAudioPort == 0)
	{
		pClient->tOndemandMulticastInfo.usMulticastAudioPort = usTempPort;
	}
#ifdef _METADATA_ENABLE				
	if(pClient->tOndemandMulticastInfo.usMulticastMetadataPort == 0)
	{
		pClient->tOndemandMulticastInfo.usMulticastMetadataPort = usTempPort;
	}
#endif	
	
   	RTSPStreaming_SetMulticastParameters(pClient->parent->hParentHandle, (pClient->iMulticast)-1, &(pClient->tOndemandMulticastInfo));

	return S_OK;
	
}



#endif

void RTSPServer_FillSessisonInfo(RTSPSERVER_SESSIONINFORMATION *pSession_info, RTSP_CLIENT* pClient)
{
    int i;
	//int iIndex ;
	RTSP_SOCKADDR sockaddr;

    //20120724 modified by Jimmy for metadata
	/*
	if( pClient->acMediaType[1][0] == 0 )
		iIndex = 1;
	else
		iIndex = 2;
 	*/

    for( i =0 ; i< MEDIA_TYPE_NUMBER ; i++ )
    {
		//20120724 added by Jimmy for metadata
		if( pClient->acMediaType[i][0] == 0 )
			break;

		//20100105 Added For Seamless Recording
		if( pClient->bSeamlessStream == TRUE )
		{
			//20101208 Modified by danny For GUID format change
			//pSession_info->dwSessionGUID = strtoul(pClient->acSeamlessRecordingGUID, NULL, 10);
			strncpy(pSession_info->acSeamlessRecordingGUID, pClient->acSeamlessRecordingGUID, RTSPS_Seamless_Recording_GUID_LENGTH - 1);
			pSession_info->acSeamlessRecordingGUID[RTSPS_Seamless_Recording_GUID_LENGTH - 1] = 0;
			//20101020 Add by danny for support seamless stream TCP/UDP timeout
			pSession_info->bSeamlessStream = TRUE;
		}
        //20140812 Added by Charles for mod no drop frame
        pSession_info->bMediaOnDemand = pClient->bMediaOnDemand;
        //20141110 added by Charles for ONVIF Profile G
        pSession_info->iCseq = pClient->iCSeq;
	
		pSession_info->dwInitialTimeStamp[i] = pClient->ulInitTimestamp[i];
        pSession_info->dwSessionID = pClient->ulSessionID;
        pSession_info->wInitialSequenceNumber[i] = pClient->usInitSequence[i];
        pSession_info->dwSSRC[i]= pClient->ulSSRC[i];

		pSession_info->eHSMode = pClient->eHSMode;
		//added by neil 10/12/30
		pSession_info->eSVCMode = pClient->eSVCMode;
		printf("***********SVC mode = %d**************\n", pSession_info->eSVCMode);
		//added by neil 11/01/14
		pSession_info->bForceFrameInterval = pClient->bForceFrameInterval;
		pSession_info->dwFrameInterval = pClient->dwFrameInterval;
        printf("*************frame interval = %d****************\n", pSession_info->dwFrameInterval);
		pSession_info->dwBypasyMSec = pClient->dwBypasyMSec;
		pSession_info->dwProtectedDelta = pClient->parent->dwProtectedDelta;
		//20100812 Added For Client Side Frame Rate Control
		pSession_info->iFrameIntervalMSec = pClient->iFrameIntervalMSec;

		if( pClient->iRTPStreamingMode == RTP_OVER_UDP )
		{
			pSession_info->iRTPStreamingType = RTP_OVER_UDP;
			
#ifdef _INET6
				if(IN6_IS_ADDR_UNSPECIFIED(&pClient->NATRTPAddr[i].sin6_addr)==0)
#else
				if( pClient->NATRTPAddr[i].sin_addr.s_addr != 0 )
#endif
				{            
					if( connect(pClient->rtp_sock[i][0],(struct sockaddr *)&pClient->NATRTPAddr[i],sizeof(pClient->NATRTPAddr[i])) == 0 )
					{
#ifdef _INET6
						printf("---NAT traverse port %d---\n",ntohs(pClient->NATRTPAddr[i].sin6_port));
						printf("[%s] socket %d connect successfully!\n", __FUNCTION__, pClient->rtp_sock[i][0]);
#else
						printf("---NAT traverse port %d---\n",ntohs(pClient->NATRTPAddr[i].sin_port));
						printf("[%s] socket %d connect successfully!\n", __FUNCTION__, pClient->rtp_sock[i][0]);
#endif
					}
					else    
					{
						printf("---NAT traverse failed--\n");
					}
				}
				else 
				{
					if( pClient->iSymRTP > 0 )
					{
						memset(&sockaddr,0,sizeof(sockaddr));
#ifdef _INET6
						sockaddr.sin6_family = AF_INET6;
						memcpy( &sockaddr.sin6_addr, &pClient->tClientSckAddr.sin6_addr, sizeof(struct in6_addr));
						sockaddr.sin6_port = htons(pClient->usPort[i][0]);
#else
						sockaddr.sin_family = AF_INET;
						sockaddr.sin_addr.s_addr =pClient->ulClientAddress;
						sockaddr.sin_port = htons(pClient->usPort[i][0]);
#endif
						//CID:91, CHECKER:CHECKED_RETURNS
						if(connect(pClient->rtp_sock[i][0],(struct sockaddr *)&sockaddr,sizeof(sockaddr)) < 0)
						{
							printf("FillSessionInfo socket %d connect failed!\n", pClient->rtp_sock[i][0]);
						}
						else
						{
							printf("[%s] socket %d connect successfully!\n", __FUNCTION__, pClient->rtp_sock[i][0]);
						}
						//pClient->iSymRTP --;
					}
				}

				//ShengFu Symetric RTP 
				//20111205 Modified by danny For UDP mode socket leak

				pSession_info->psktRTP[i] = &pClient->rtp_sock[i][0];
				printf("pSession_info->psktRTP[%d] = %d\n", i, *pSession_info->psktRTP[i]);
				pSession_info->psktRTCP[i] = &pClient->rtp_sock[i][1];
#ifndef _SHARED_MEM
			}
#endif
		}
		else
		{	
			pSession_info->iRTPStreamingType = RTP_OVER_TCP;
			pSession_info->iEmbeddedRTPID[i] = pClient->iEmbeddedRTPID[i];
			pSession_info->iEmbeddedRTCPID[i] = pClient->iEmbeddedRTCPID[i];
			//20110706 Modified by danny For TCP/HTTP mode socket not sync in multi thread
			pSession_info->psktRTSPSocket = &pClient->iSendSockfd;
			printf("[%s]psktRTSPSocket %p(%d)\n", __func__, pSession_info->psktRTSPSocket, *pSession_info->psktRTSPSocket);
			
			//20160603 add by faber, pass client index
			pSession_info->iClientIndex = pClient->iClientIndex;
			
			//ShengFu bug fixed for no peer IP while HTTP mode
			if( pClient->iRTPStreamingMode == RTP_OVER_HTTP )
			{
				RTSP_SOCKADDR peersock;
				int socklen = sizeof(peersock);

				//20110706 Modified by danny For TCP/HTTP mode socket not sync in multi thread 
				getpeername( *pSession_info->psktRTSPSocket, (struct sockaddr * ) & peersock, (unsigned int *)&socklen) ;
#ifdef _INET6
				if (IN6_IS_ADDR_V4MAPPED(&peersock.sin6_addr))
				{
					struct sockaddr_in	v4mapaddr;
					struct in6_addr in6addr = peersock.sin6_addr;

					memset(&v4mapaddr, 0, sizeof(v4mapaddr));
					v4mapaddr.sin_addr.s_addr =  MAKEFOURCC(in6addr.s6_addr[12], in6addr.s6_addr[13], in6addr.s6_addr[14], in6addr.s6_addr[15]);
					
					pClient->ulClientAddress = v4mapaddr.sin_addr.s_addr;
				}
				memcpy(&pClient->tClientSckAddr, &peersock, sizeof(RTSP_SOCKADDR));
#else
				pClient->ulClientAddress = peersock.sin_addr.s_addr;
#endif
			}
		}
		//shengfu 2006/08/17 
        //pSession_info->ulClientIP = ntohl(pClient->ulClientAddress);
        pSession_info->ulClientIP = pClient->ulClientAddress;
#ifdef _INET6
		memcpy(&pSession_info->tClientSckAddr, &pClient->tClientSckAddr, sizeof(RTSP_SOCKADDR));
#endif
		//CID:65, CHECKER:BUFFER_SIZE_WARNING
        rtspstrcpy(pSession_info->cMediaName[i],pClient->acMediaType[i], sizeof(pSession_info->cMediaName[i]));
        pSession_info->iRTCPTimeOut = 10;
		pSession_info->iVivotekClient = pClient->iVivotekClient;
		pSession_info->iSDPIndex = pClient->iSDPIndex;
    }
}

unsigned long RTSPServer_CheckIDCollision(RTSP_CLIENT* pClient, unsigned long ulID)
{
  RTSP_SERVER *pServer;
  int i;
  unsigned long ulSID;
  
    ulSID = ulID;
    pServer = pClient->parent;

    for( i=0 ; i<pServer->iMaxSessionNumber ; i++ )
    {
        if(pServer->pClient[i].iRecvSockfd > 0 )
        {
            if( pServer->pClient[i].ulSessionID == ulSID )
            {
                ulSID = RTSPServer_GetSessionID();
                i = -1; //20100617 danny, fix sessionid duplicate issue
            }
        }  
    }
  
    return ulSID;
}

#ifdef _SHARED_MEM
//20100428 Added For Media on demand
SCODE RTSPServer_CheckMediaOnDemandAvailable(int *iSDPindex, RTSP_CLIENT *pClient, RTSP_SERVER *pServer)
{
	int i;
	
	if( pServer->iMODConnectionNumber >= MOD_STREAM_NUM )
	{
		printf("[%s] Media on demand Connection Number=%d is Full!\n", __FUNCTION__, pServer->iMODConnectionNumber);
		pClient->bMediaOnDemand = FALSE;
		return S_FAIL;
	}
	pServer->iMODConnectionNumber++;
	printf("[%s] Media on demand Connection Number=%d\n", __FUNCTION__, pServer->iMODConnectionNumber);
	
	int iModNewIndex = 0;
	if( pServer->fcallback(pServer->hParentHandle
							,RTSPSERVER_CALLBACKFLAG_GET_NEW_MOD_STREAM
							,(void*)&iModNewIndex, 0) != S_OK )
	{
		return S_FAIL;
	}

	*iSDPindex += iModNewIndex;
	printf("new sdp = %d, mod index = %d\n", *iSDPindex, iModNewIndex);
	return S_OK;
}

//20100105 Added For Seamless Recording
SCODE RTSPServer_CheckSeamlessRecordingAvailable(char *pcSeamlessRecordingGUID, RTSP_CLIENT *pClient, RTSP_SERVER *pServer)
{
	int i = 0, iRet = 0;
	
	if( pServer->tSeamlessRecordingInfo.iSeamlessDiskMode == 0 )
	{
		printf("[%s] Disk Mode not support!\n", __FUNCTION__);
		return S_FAIL;
	}

	if( pClient->iSDPIndex != pServer->tSeamlessRecordingInfo.iSeamlessStreamNumber )
	{
		printf("[%s] Stream %d not support Seamless Recording!\n", __FUNCTION__, pClient->iSDPIndex);
		return S_FAIL;
	}
	
	for( i = 0; i < pServer->tSeamlessRecordingInfo.iSeamlessMaxConnection; i++ )
	{
		iRet = rtspstrncasecmp(pcSeamlessRecordingGUID, pServer->tSeamlessRecordingInfo.tGUIDListInfo[i].acGUID, RTSPS_Seamless_Recording_GUID_LENGTH);
		//Write guid down for guid is exist
		if( iRet == 0 )
		{
			pServer->tSeamlessRecordingInfo.tGUIDListInfo[i].iNumber++;
			if( pServer->tSeamlessRecordingInfo.tGUIDListInfo[i].iUnderRecording == 1 )
			{
				pServer->tSeamlessRecordingInfo.tGUIDListInfo[i].iUnderRecording = 0;
				printf("[%s] Guid%d back!\n", __FUNCTION__, i);
			}
			else
			{
				pServer->tSeamlessRecordingInfo.iSeamlessConnectionNumber++;
			}
			pClient->iSeamlessRecordingSession = i;
			printf("[%s] Add one to guid%d=%s, number=%d\n", __FUNCTION__, i, pServer->tSeamlessRecordingInfo.tGUIDListInfo[i].acGUID, 
																			pServer->tSeamlessRecordingInfo.tGUIDListInfo[i].iNumber);
			break;
		}
	}
	
	if( pServer->tSeamlessRecordingInfo.iSeamlessConnectionNumber > pServer->tSeamlessRecordingInfo.iSeamlessMaxConnection )
	{
		pClient->iSeamlessRecordingSession = -1;
		pServer->tSeamlessRecordingInfo.tGUIDListInfo[i].iNumber--;
		pServer->tSeamlessRecordingInfo.iSeamlessConnectionNumber--;
		printf("[%s] Seamless Connection Number=%d is Full!\n", __FUNCTION__, pServer->tSeamlessRecordingInfo.iSeamlessConnectionNumber);
		return S_FAIL;
	}

	//Write guid down for new guid
	if( iRet != 0 )
	{
		if( pServer->tSeamlessRecordingInfo.iSeamlessConnectionNumber < pServer->tSeamlessRecordingInfo.iSeamlessMaxConnection )
		{
			for( i = 0; i < pServer->tSeamlessRecordingInfo.iSeamlessMaxConnection; i++ )
			{
				if( pServer->tSeamlessRecordingInfo.tGUIDListInfo[i].acGUID[0] == 0 )
				{
					strncpy(pServer->tSeamlessRecordingInfo.tGUIDListInfo[i].acGUID, pcSeamlessRecordingGUID, RTSPS_Seamless_Recording_GUID_LENGTH - 1);
					pServer->tSeamlessRecordingInfo.tGUIDListInfo[i].iNumber++;
					pServer->tSeamlessRecordingInfo.iSeamlessConnectionNumber++;
					pClient->iSeamlessRecordingSession = i;
					printf("[%s] Add new guid%d=%s, number=%d\n", __FUNCTION__, i, pServer->tSeamlessRecordingInfo.tGUIDListInfo[i].acGUID, 
																			pServer->tSeamlessRecordingInfo.tGUIDListInfo[i].iNumber);
					return S_OK;
				}
			}
		}
		else
		{
			printf("[%s] Seamless Connection guid Count=%d is Full!\n", __FUNCTION__, pServer->tSeamlessRecordingInfo.iSeamlessConnectionNumber);
			return S_FAIL;
		}
	}

	return S_OK;
}

SCODE RTSPServer_CheckSeamlessAllGUIDsBack(RTSP_SERVER *pServer)
{
	int i = 0;
	
	if( pServer->tSeamlessRecordingInfo.iRecordingEnable == 1 )
	{
		printf("[%s] Now recording is enable!\n", __FUNCTION__);
		for( i = 0; i < pServer->tSeamlessRecordingInfo.iSeamlessMaxConnection; i++ )
		{
			if( pServer->tSeamlessRecordingInfo.tGUIDListInfo[i].iUnderRecording == 1 )
			{
				printf("[%s] Guid%d=%s not back yet, number=%d\n", __FUNCTION__, i, pServer->tSeamlessRecordingInfo.tGUIDListInfo[i].acGUID, 
																				pServer->tSeamlessRecordingInfo.tGUIDListInfo[i].iNumber);
				return S_OK;
			}
		}
		pServer->tSeamlessRecordingInfo.iRecordingEnable = 0;
		printf("[%s] All guids back, Stop recording!\n", __FUNCTION__);

		if( pServer->fcallback(pServer->hParentHandle
							,RTSPSERVER_CALLBACKFLAG_UPDATE_RECODERSTATE
							,(void*)&pServer->tSeamlessRecordingInfo,0) != S_OK )
		{
			DbgLog((dfCONSOLE|dfINTERNAL,"RTSPSERVER_CALLBACKFLAG_UPDATE_RECODERSTATE failed!\n"));
			printf("[%s] RTSPSERVER_CALLBACKFLAG_UPDATE_RECODERSTATE failed!\n", __FUNCTION__);
		}
	}

	return S_OK;
}
#endif

int RTSPServer_OutSendMessage(char* pcBuff, RTSP_CLIENT* pClient)
{
	//struct timeval timeout;
	//fd_set        write_set;
	//int nSelectRes;

	char *pcSendBuf;
	int nBufSize ;
	int nSendSize=0;
	int iResendCount;
	int iReturnValue;

	pcSendBuf = pcBuff;
	nBufSize = strlen(pcBuff);

	printf("%s\n",pcBuff);   
	iResendCount=0;
	iReturnValue=0;
	
	//20100920 Modified by danny for fix Taipei city project HTTP/TCP mode start code bug 
	if( (pClient->iRTPStreamingMode == RTP_OVER_TCP || pClient->iRTPStreamingMode == RTP_OVER_HTTP) &&
		pClient->hTCPMuxCS != 0 )
	{
		OSCriticalSection_Enter(pClient->hTCPMuxCS);
		//printf("[%s] @@@OSCriticalSection_Enter RTSP %p ok@@@\n", __FUNCTION__, pClient->hTCPMuxCS);
	}               
  	while(nBufSize>0)
	{	
		/*FD_ZERO(&write_set);
  		FD_SET(pClient->iSendSockfd, &write_set);
		
		timeout.tv_sec = 0;
		timeout.tv_usec = 200*1000;
    
		nSelectRes = select(pClient->iSendSockfd + 1, NULL,&write_set, NULL,&timeout);*/
		
		iResendCount++;
		
		if( iResendCount > 40 )
		{
			printf("time out ! send fail\n");
			//20100920 Modified by danny for fix Taipei city project HTTP/TCP mode start code bug
	      	if( (pClient->iRTPStreamingMode == RTP_OVER_TCP || pClient->iRTPStreamingMode == RTP_OVER_HTTP) &&
  				pClient->hTCPMuxCS != 0 )
	      	{
      	        OSCriticalSection_Leave(pClient->hTCPMuxCS);
				//printf("[%s] @@@OSCriticalSection_Leave RTSP %p, send time out@@@\n", __FUNCTION__, pClient->hTCPMuxCS);
	      	}
            return -1;
        }		    
		
		//if (nSelectRes > 0 )
		{
			nSendSize=send(pClient->iSendSockfd, pcSendBuf,nBufSize, SEND_FLAGS);
			
			if( nSendSize < 0 )
			{
#if defined (_WIN32_)
				if (WSAGetLastError() == WSAEWOULDBLOCK )
#elif defined(_PSOS_TRIMEDIA) || defined(_LINUX)
				if ( errno == EWOULDBLOCK )
#endif 
        		{
           			OSSleep_MSec(100);
//       				DbgPrint((" EWOULDBLOCK \n"));
       				nSendSize=0;
        		}
        		else
        		{
//       				DbgPrint(("[RTSPServer]Outsend fail \n"));
					iReturnValue= -1;
					//20100920 Modified by danny for fix Taipei city project HTTP/TCP mode start code bug 
				  	if( (pClient->iRTPStreamingMode == RTP_OVER_TCP || pClient->iRTPStreamingMode == RTP_OVER_HTTP) &&
  						pClient->hTCPMuxCS != 0 )
				  	{
  	          	        OSCriticalSection_Leave(pClient->hTCPMuxCS);
						//printf("[%s] @@@OSCriticalSection_Leave RTSP %p, send fail@@@\n", __FUNCTION__, pClient->hTCPMuxCS);
				  	}
                      /*pClient->parent->fcallback(pClient->parent->hParentHandle
	                    ,RTSPSERVER_CALLBACKFLAG_SESSION_TCP_CS
                        ,pClient->ulSessionID,TCP_RELEASE_CS);*/
  
					return iReturnValue;
         		}
			}
			else if(nSendSize==0)
			{
				OSSleep_MSec(10);		
			}
			else
			{			
		        nBufSize    -= nSendSize;
		        pcSendBuf   += nSendSize;
		        iReturnValue+= nSendSize;   
		    }
		}
		/*else if( nSelectRes <= 0 )
		{			
			int	iValue,iValueLen;

			printf("send select %d timeout/fail!\n",pClient->iSendSockfd);
			if( getsockopt(pClient->iSendSockfd,SOL_SOCKET,SO_RCVBUF,&iValue,&iValueLen) != 0)
			{
				return -1;   
			}
			
		}*/
	
	}
	//20100920 Modified by danny for fix Taipei city project HTTP/TCP mode start code bug 
  	if( (pClient->iRTPStreamingMode == RTP_OVER_TCP || pClient->iRTPStreamingMode == RTP_OVER_HTTP) &&
  		pClient->hTCPMuxCS != 0 )
  	{
        OSCriticalSection_Leave(pClient->hTCPMuxCS);
		//printf("[%s] @@@OSCriticalSection_Leave RTSP %p, send ok@@@\n", __FUNCTION__, pClient->hTCPMuxCS);
  	}
 /*       pClient->parent->fcallback(pClient->parent->hParentHandle
	                ,RTSPSERVER_CALLBACKFLAG_SESSION_TCP_CS
                    ,pClient->ulSessionID,TCP_RELEASE_CS);*/
  	printf("send done\n");
    return iReturnValue;
}


void RTSPServer_AddCSeq(char* pcBuffer,int iSeq)
{
    sprintf( pcBuffer + strlen( pcBuffer ), "CSeq: %d\r\n", iSeq );
}

void RTSPServer_AddEndMessage(char *pcBuffer)
{
    strcat( pcBuffer + strlen( pcBuffer ), "\r\n");
}

void RTSPServer_CleanBuffer(RTSP_CLIENT* pClient)
{
    memset(pClient->acServerName,0,sizeof(pClient->acServerName));
    memset(pClient->acObjURL,0,sizeof(pClient->acObjURL));
    memset(pClient->acObject,0,sizeof(pClient->acObject));
}

/********* Send out RTSP response ************************/

int RTSPServer_SendReply( int iErr, char *addon, RTSP_CLIENT *pClient,RTSP_SERVER* pServer)
{
    char    *pcBuffer;
    int     iRet;
	DWORD	dwSec;
	DWORD	dwMSec;

    pcBuffer = pClient->parent->acSendBuffer;
    memset(pcBuffer,0,MAX_SNDBUFF);
    snprintf( pcBuffer, MAX_SNDBUFF - 1, "%s %d %s\r\n", "RTSP/1.0", iErr,RTSPServer_GetState( iErr ));
    RTSPServer_AddCSeq(pcBuffer,pClient->iCSeq);
    
    if( pClient->ulSessionID != 0 )
   	    //20121005 added by Jimmy. A session identifier MUST be at least eight octets long according to RFC 2326, section 3.4
   	    snprintf(pcBuffer + strlen(pcBuffer), MAX_SNDBUFF - strlen(pcBuffer) - 1, "Session: %08lu\r\n", pClient->ulSessionID);   
	
	if( iErr == 401 )
	{
		if( pClient->parent->iAuthenticationType != RTSP_AUTH_DIGEST )
		{
   			snprintf(pcBuffer + strlen(pcBuffer), MAX_SNDBUFF - strlen(pcBuffer) - 1, "WWW-Authenticate: Basic realm=\"%s\"\r\n", RTSPSERVER_REALM);		
		}
		else
		{
			char acTemp[60];

			//20140221 modified by Charles to follow the method of RTSP digest auth defined in RFC2069
			//if(pClient->parent->acAuthenNonce[0] == 0 )
			{
				memset(acTemp,0,60);
				OSTime_GetTimer(&dwSec, &dwMSec);
#ifdef _INET6
				if (pClient->ulClientAddress == 0)
				{
					snprintf(acTemp, sizeof(acTemp) - 1,"%02x%02x%02x%02x%08x%08x",(unsigned int)pClient->tClientSckAddr.sin6_addr.s6_addr[12],(unsigned int)pClient->tClientSckAddr.sin6_addr.s6_addr[13],(unsigned int)pClient->tClientSckAddr.sin6_addr.s6_addr[14],(unsigned int)pClient->tClientSckAddr.sin6_addr.s6_addr[15] ,dwSec,dwMSec);
				}
				else
#endif
				{
				    snprintf(acTemp, sizeof(acTemp) - strlen(acTemp) - 1, "%08x%08x%08x",(unsigned int)pClient->ulClientAddress,dwSec,dwMSec);
				}
				//20160818 Modify by Faber, each client has specify nonce
				EncryptionUtl_MD5(pClient->acAuthenNonce,acTemp,NULL);
			}
			//20140221 modified by Charles to follow the method of RTSP digest auth defined in RFC2069
			//20160818 Modify by Faber, each client has specify nonce
			snprintf(pcBuffer + strlen(pcBuffer), MAX_SNDBUFF - strlen(pcBuffer) - 1, "WWW-Authenticate: Digest realm=\"%s\",nonce=\"%s\"\r\n", RTSPSERVER_REALM, pClient->acAuthenNonce);   
			//snprintf(pcBuffer + strlen(pcBuffer), MAX_SNDBUFF - strlen(pcBuffer) - 1, "WWW-Authenticate: Digest qop=\"auth\",realm=\"%s\",nonce=\"%s\"\r\n", RTSPSERVER_REALM, pClient->parent->acAuthenNonce);   
			//sprintf(pcBuffer + strlen(pcBuffer), "WWW-Authenticate: Digest realm=\"RTSP server\",nonce=\"%s\"\r\n",pClient->parent->acAuthenNonce);   
		}
	}
#ifdef _SHARED_MEM
	if(iErr == 415 && pClient->acShmemReturnString[0] != 0)
	{
		snprintf(pcBuffer + strlen(pcBuffer), MAX_SNDBUFF - strlen(pcBuffer) - 1, "%s\r\n", pClient->acShmemReturnString);
	}
#endif
    RTSPServer_AddEndMessage(pcBuffer);

    iRet = RTSPServer_OutSendMessage(pcBuffer,pClient);

    if( iRet == strlen(pcBuffer) )
    {
        return 0;
    }        
    else
    {
        if( pClient->ulSessionID != 0 )
        {
			RTSPServer_SessionStop(pClient->parent, pClient->ulSessionID,pClient->iMulticast);
        }              
    
        //2005/05/31closesocket(pClient->iSockfd); 
        pClient->parent = pServer;
        RTSPServer_InitClient(pClient);
        printf("Client Dead! disconnect..\n");
        return -1;
    }        

//    output(pcBuffer,pClient);

}

int RTSPServer_SendDescribeReply(RTSP_CLIENT *pClient,RTSP_SERVER* pServer)
{
    char   *pcRecvMesg;		/* get reply message buffer pointer */
//  char sdp[]= "v=0\r\no=WebEye-M 1019758856 1019758856 IN IP4 0.0.0.0\r\ns=live\r\nc=IN IP4 0.0.0.0\r\nt=0 0\r\nm=video 0 RTP/AVP 100\r\na=rtpmap:100 MP4V-ES/90000\r\na=fmtp:100 profile-level-id=3;config=000001B001000001B58913000001000000012000C488BA9850584121443F\r\na=control:rtsp://192.168.0.63:554/live\r\n";
//  char sdp[]="v=0\r\no=StreamingServer 3229052693 1019111111000 IN IP4 192.168.0.63\r\ns=\Live_a.mov\r\nu=http:///\r\ne=admin@\r\nc=IN IP4 0.0.0.0\r\nt=0 0\r\na=control:*\r\na=range:npt=0-70.51800\r\nm=audio 0 RTP/AVP 101\r\nb=AS:345\r\na=rtpmap:101 PCMU/22050/2\r\na=control:track-ID=2\r\na=range:npt=0.000000-70.518005\r\n";
//  char sdp[]="v=0\r\no=WebEye-M 1019758856 1019758856 IN IP4 0.0.0.0\r\ns=Live_a\r\nc=IN IP4 0.0.0.0\r\nt=0 0\r\nm=audio 0 RTP/AVP 101\r\nb=AS:64\r\na=rtpmap:101 PCMU/22050/2\r\na=control:rtsp://192.168.0.63:554/Live_a\r\n";
//  char sdp[]="v=0\r\no=- 4363603081003270144 4363603081003270144 IN IP4 192.168.0.63\r\ns=Live#1\r\nt=0 0\r\na=charset:Shift_JIS\r\na=control:rtsp://192.168.0.63/live\r\nm=video 0 RTP/AVP 100\r\nc=IN IP4 0.0.0.0\r\nb=AS:128\r\na=rtpmap:100 MP4V-ES/90000\r\na=control:rtsp://192.168.0.63/live/live.m4v\r\na=fmtp:100 profile-level-id=8;config=000001B008000001B58913000001000000012100844007A85020F0A21F\r\nm=audio 0 RTP/AVP 101\r\nc=IN IP4 0.0.0.0\r\nb=AS:353\r\na=rtpmap:101 PCMU/22050/2\r\na=control:rtsp://192.168.0.63/live/live.pcm\r\n";
    char acSDP[RTSPSTREAMING_SDP_MAXSIZE]; 
    int iResult;
    int iRet;
    RTSPSERVER_SDPREQUEST	stSDPRequest;    
    TOSDateTimeInfo			tDateInfo;
#ifdef _SESSION_MGR
	TConnInfo				tConnInfo;
#endif

    pcRecvMesg = pClient->parent->acSendBuffer;
    memset(pcRecvMesg,0,MAX_SNDBUFF);
    memset(acSDP,0,1000);
	memset(&stSDPRequest,0,sizeof(RTSPSERVER_SDPREQUEST));
    
    snprintf(pcRecvMesg, MAX_SNDBUFF - 1, "%s %d %s\r\n", "RTSP/1.0", 200,RTSPServer_GetState(200));
    RTSPServer_AddCSeq(pcRecvMesg,pClient->iCSeq);
    OSTime_GetDateTime(&tDateInfo);
    snprintf(pcRecvMesg + strlen(pcRecvMesg), MAX_SNDBUFF - strlen(pcRecvMesg) - 1, "Date: %s, %d %s %d %d:%d:%d GMT\r\n",ConvertWeekday(tDateInfo.wWeekDay)
                                                                             ,tDateInfo.wMonthDay
                                                                             ,ConvertMonth(tDateInfo.wMonth)
                                                                             ,tDateInfo.wYear
                                                                             ,tDateInfo.wHour
                                                                             ,tDateInfo.wMinute
                                                                             ,tDateInfo.wSecond);
	
	//sprintf(pcRecvMesg + strlen(pcRecvMesg), "Content-base: %s/\r\n", pClient->acObjURL);
	if( pClient->iPlayerType == QUICKTIME_PLAYER )
	{
		snprintf(pcRecvMesg + strlen(pcRecvMesg), MAX_SNDBUFF - strlen(pcRecvMesg) - 1, "Content-Base: %s/\r\n", pClient->acObjURL);
	}
	else
	{
		if( pClient->parent->rtsp_param.rtsp_port != 554 )
		{
			snprintf(pcRecvMesg + strlen(pcRecvMesg), MAX_SNDBUFF - strlen(pcRecvMesg) - 1, "Content-Base: rtsp://%s:%d/%s/\r\n", pClient->acServerName,pClient->parent->rtsp_param.rtsp_port,pClient->acObject);
		}
		else
		{
			snprintf(pcRecvMesg + strlen(pcRecvMesg), MAX_SNDBUFF - strlen(pcRecvMesg) - 1, "Content-Base: rtsp://%s/%s/\r\n", pClient->acServerName,pClient->acObject);	
		}
	}
	
    strncat(pcRecvMesg, "Content-Type: application/sdp\r\n", MAX_SNDBUFF - strlen(pcRecvMesg) - 1);
	
#ifdef _SHARED_MEM
	if(pClient->acShmemReturnString[0] != 0)
	{
		snprintf(pcRecvMesg + strlen(pcRecvMesg), MAX_SNDBUFF - strlen(pcRecvMesg) - 1, "%s\r\n", pClient->acShmemReturnString);
	}
#endif
	stSDPRequest.pcDescribe=pClient->acObject;
	stSDPRequest.ulIP=pClient->ulClientAddress;
	stSDPRequest.usPort=0;
	stSDPRequest.iVivotekClient = pClient->iVivotekClient;
	
	//20111124 modify by danny support Backchannel Multicast for ONVIF test tool 1.02.4
	stSDPRequest.iPlayerType = pClient->iPlayerType;
	
	stSDPRequest.pSDPBuffer = acSDP;
	stSDPRequest.iSDPBuffLen = RTSPSTREAMING_SDP_MAXSIZE;
	//Get SDPindex
	iResult=RTSPServer_GetSDP(pClient->parent, &stSDPRequest);
    if ( iResult < 0)
    {
        printf("Can't find the corresponding access name from config file. %d\n",iResult);
        RTSPServer_SendReply(404, 0,pClient,pServer);
        RTSPServer_CleanBuffer(pClient);
        return -1;
    }
	
#ifdef _SHARED_MEM
	//20100428 Added For Media on demand
	if( pClient->bMediaOnDemand == TRUE )
	{
		printf("[%s] MOD stream original SDPindex=%d\n", __FUNCTION__, stSDPRequest.iSDPindex);
		if( (iRet = RTSPServer_CheckMediaOnDemandAvailable(&stSDPRequest.iSDPindex, pClient, pServer)) != S_OK )
    	{
			DbgLog((dfCONSOLE|dfINTERNAL,"Media on demand Service Unavailable!\n"));
			RTSPServer_DiscardMessage(pClient);
        	RTSPServer_SendReply(503,0,pClient,pServer);
			pClient->parent = pServer;
        	RTSPServer_InitClient(pClient);
        	return (1);
    	}
		stSDPRequest.pcExtraInfo=pClient->acExtraInfo;
		printf("[%s] MOD stream assigned SDPindex=%d\n", __FUNCTION__, stSDPRequest.iSDPindex);
	}
#endif

	pClient->iSDPIndex = stSDPRequest.iSDPindex;
	pClient->iOrigSDPIndex = stSDPRequest.iSDPindex;				//20090317 for multiple stream

	//20120925 added by Jimmy for ONVIF backchannel
	pClient->iChannelIndex= pServer->fcallback(pServer->hParentHandle, RTSPSERVER_CALLBACKFLAG_GET_CHANNEL_INDEX, (void*)pClient->iSDPIndex, 0);

#ifdef _SHARED_MEM
	//20100105 Added For Seamless Recording
	if( pClient->bSeamlessStream == TRUE )
	{
		printf("[%s] Stream for Seamless Recording, guid=%s\n", __FUNCTION__, pClient->acSeamlessRecordingGUID);
		if( (iRet = RTSPServer_CheckSeamlessRecordingAvailable(pClient->acSeamlessRecordingGUID, pClient, pServer)) != S_OK )
    	{
			DbgLog((dfCONSOLE|dfINTERNAL,"Seamless Recording Service Unavailable!\n"));
			RTSPServer_DiscardMessage(pClient);
        	RTSPServer_SendReply(503,0,pClient,pServer);
			pClient->parent = pServer;
        	RTSPServer_InitClient(pClient);
        	return (1);
    	}
		
		RTSPServer_CheckSeamlessAllGUIDsBack(pServer);
		if( pServer->fcallback(pServer->hParentHandle
							,RTSPSERVER_CALLBACKFLAG_UPDATE_GUIDLISTINFO
							,(void*)&pServer->tSeamlessRecordingInfo,(void*)pClient->iSeamlessRecordingSession) != S_OK )
		{
			DbgLog((dfCONSOLE|dfINTERNAL,"RTSPSERVER_CALLBACKFLAG_UPDATE_GUIDLISTINFO failed!\n"));
			printf("[%s] RTSPSERVER_CALLBACKFLAG_UPDATE_GUIDLISTINFO failed!\n", __FUNCTION__);
			RTSPServer_DiscardMessage(pClient);
        	RTSPServer_SendReply(503,0,pClient,pServer);
			pClient->parent = pServer;
        	RTSPServer_InitClient(pClient);
        	return (1);
    	}
	}
#endif
	
	//20090312 Multiple stream
#ifdef _SESSION_MGR
	if(!pClient->bManagedbySessionMgr)
	{
		memset(&tConnInfo, 0 , sizeof(TConnInfo));
		tConnInfo.eStreamType = eConnectTypeRTSP;
		tConnInfo.eEncoderMode = eetNULL;
		tConnInfo.eConnMgr = eAddConn;
		memcpy(&tConnInfo.tProfile.tInfo, &pClient->tSessMgrInfo, sizeof(TSessMgrStreamInfo));
		tConnInfo.tProfile.iID = pClient->iSDPIndex;
		//20100623 danny, Modified for fix sessioninfo corrupted issue
		/*
			20120906 modifeid by Jimmy to fix sessioninfo not concsistent issue:
			The seesion number in sessioninfo must be concsistent with the managed session number( pClient->bManagedbySessionMgr==TRUE ), not iCurrentSessionNumber.
		*/
		//tConnInfo.iConnectNum  = pServer->iCurrentSessionNumber;
		tConnInfo.iConnectNum = RTSPServer_GetManagedSessionNumber(pClient->parent) + 1;
		
		//if(!SessionMgr_CheckConnSt(&tConnInfo))
		if(!SessionMgr_UpdateConnSt(&tConnInfo, pServer->hSessionMgrHandle))
		{
			syslog(LOG_ERR, "Add client %lu to session manager error %d, server currentSessionNumber %d, managedSessionNumber %d\n", 
				pClient->ulSessionID, tConnInfo.tProfile.eStatus, pServer->iCurrentSessionNumber, RTSPServer_GetManagedSessionNumber(pClient->parent));
			//20091130 add 503 Service Unavailable is returned when connection limit is reached
			if (tConnInfo.tProfile.eStatus == 17)
			{
				pServer->fcallback(pServer->hParentHandle, RTSPSERVER_CALLBACKFLAG_RELEASE_MOD_STREAM, (void*)pClient->iSDPIndex, 0);
				RTSPServer_SendReply(503, 0,pClient,pServer);
			}
			else
			{
				pServer->fcallback(pServer->hParentHandle, RTSPSERVER_CALLBACKFLAG_RELEASE_MOD_STREAM, (void*)pClient->iSDPIndex, 0);
				RTSPServer_SendReply(400, 0,pClient,pServer);
			}
			RTSPServer_CleanBuffer(pClient);
			return -1;
		}
		else
		{
			pClient->bManagedbySessionMgr = TRUE;	
			printf("Original SDP %d Profile manager returned SDP %d Status %d\n", pClient->iSDPIndex, tConnInfo.tProfile.iID, tConnInfo.tProfile.eStatus);
			//Assign back the real SDP index
			pClient->iSDPIndex = tConnInfo.tProfile.iID;
			//Reset CI if needed (session manager indicate different CI needed)
			if(tConnInfo.tProfile.eStatus == esmsUpdateCI)
			{
				RTSPServer_ResetCI(pClient->parent, pClient->iSDPIndex);
				stSDPRequest.bResetCI = TRUE;
				if(tConnInfo.tProfile.tInfo.acResolution[0] != 0)
				{
					rtspstrcpy(stSDPRequest.acResolution, tConnInfo.tProfile.tInfo.acResolution, sizeof(stSDPRequest.acResolution));
				}
				if(tConnInfo.tProfile.tInfo.acCodecType[0] != 0)
				{
					rtspstrcpy(stSDPRequest.acCodecType, tConnInfo.tProfile.tInfo.acCodecType, sizeof(stSDPRequest.acCodecType));
				}
			}
		}
	}
#endif

	//Proceed to compose SDP 20090312
	iResult = 0;
	stSDPRequest.iSDPindex = pClient->iSDPIndex;
	//20120925 added by Jimmy for ONVIF backchannel
	stSDPRequest.iRequire = pClient->iRequire;
	iResult = RTSPServer_ComposeSDP(pClient->parent, &stSDPRequest);

    if ( iResult < 0)
    {
        printf("Can't find the cooreponding SDP file to the DESCRIBE request. %d\n",iResult);
#ifdef _SHARED_MEM
		//20100428 Added For Media on demand
		if ( (pClient->bMediaOnDemand == TRUE) && (stSDPRequest.eMODRunCode & MOD_ERR_SERVER) )
		{
			if(stSDPRequest.eMODRunCode & MOD_ERR_SERVER_SEARCH_TIMEDOUT)
			{
				pServer->fcallback(pServer->hParentHandle, RTSPSERVER_CALLBACKFLAG_RELEASE_MOD_STREAM, (void*)pClient->iSDPIndex, 0);
				RTSPServer_SendReply(408, 0,pClient,pServer);
			}
			else
			{
				pServer->fcallback(pServer->hParentHandle, RTSPSERVER_CALLBACKFLAG_RELEASE_MOD_STREAM, (void*)pClient->iSDPIndex, 0);
				RTSPServer_SendReply(500, 0,pClient,pServer);
			}
		}
		else
#endif
		{
			pServer->fcallback(pServer->hParentHandle, RTSPSERVER_CALLBACKFLAG_RELEASE_MOD_STREAM, (void*)pClient->iSDPIndex, 0);
			RTSPServer_SendReply(404, 0,pClient,pServer);
		}
        RTSPServer_CleanBuffer(pClient);
        return -1;
    }

	snprintf(pcRecvMesg + strlen(pcRecvMesg), MAX_SNDBUFF - strlen(pcRecvMesg) - 1, "Content-Length: %d\r\n",stSDPRequest.iSDPBuffLen);
    RTSPServer_AddEndMessage(pcRecvMesg);
 
    // concatenate SDP part
    strncat(pcRecvMesg,acSDP, MAX_SNDBUFF - strlen(pcRecvMesg) - 1);

    iRet = RTSPServer_OutSendMessage(pcRecvMesg,pClient);

    if( iRet == strlen(pcRecvMesg) )
    {
//        DbgPrint(("DESCRIBE response sent.\n"));
        return 0;
    }        
    else
    {
        //2005/05/31closesocket(pClient->iSockfd); 
        pClient->parent = pServer;
        RTSPServer_InitClient(pClient);
//        DbgPrint(("Client Dead! disconnect..\n"));
        return -1;
    }     
}

#ifdef RTSPRTP_MULTICAST
int RTSPServer_SendMulticastSetupReply(RTSP_CLIENT *pClient,unsigned long ulSessionID,int iMediaType)
{
    char    *pcBuffer;
    int      iRet;

    //20150127 Add by Faber, for separate video/audio/metadata
    struct sockaddr_in TempVAddr;
	struct sockaddr_in TempAAddr;
	struct sockaddr_in TempMAddr;

	//20120726 modified by Jimmy for metadata
	unsigned short usRTPVideoPort,usRTPAudioPort,usRTPMetadataPort,usTTL;

    pcBuffer = pClient->parent->acSendBuffer;
    memset(pcBuffer,0,MAX_SNDBUFF);
    snprintf(pcBuffer, MAX_SNDBUFF - 1, "%s %d %s\r\n", "RTSP/1.0", 200,RTSPServer_GetState(200));
    RTSPServer_AddCSeq(pcBuffer,pClient->iCSeq);
    //20121005 added by Jimmy. A session identifier MUST be at least eight octets long according to RFC 2326, section 3.4
    snprintf(pcBuffer + strlen(pcBuffer), MAX_SNDBUFF - strlen(pcBuffer) - 1, "Session: %08lu;timeout=%d\r\n", ulSessionID, g_iMulticastTimeout);   


	//20130924 modified by Charles for ondemand multicast
#ifdef _WIN32_  
	if(pClient->bNewMulticast == TRUE)
	{
		TempAddr.sin_addr.S_un.S_addr = pClient->tMulticastInfo.ulMulticastAddress;
	}
	else
	{
		TempAddr.sin_addr.S_un.S_addr = pClient->parent->ulMulticastAddress[pClient->iMulticast-1];
	}

#else
	if(pClient->bNewMulticast == TRUE)
	{
		TempVAddr.sin_addr.s_addr = pClient->tOndemandMulticastInfo.ulMulticastAddress;
		TempAAddr.sin_addr.s_addr = pClient->tOndemandMulticastInfo.ulMulticastAudioAddress;
		TempMAddr.sin_addr.s_addr = pClient->tOndemandMulticastInfo.ulMulticastMetadataAddress;
	}
	else
	{
		TempVAddr.sin_addr.s_addr = pClient->parent->ulMulticastAddress[pClient->iMulticast-1];
		TempAAddr.sin_addr.s_addr = pClient->parent->ulMulticastAudioAddress[pClient->iMulticast-1];
		TempMAddr.sin_addr.s_addr = pClient->parent->ulMulticastMetadataAddress[pClient->iMulticast-1];
	}

#endif		
		
	if(pClient->bNewMulticast == TRUE)
	{
		usRTPVideoPort = pClient->tOndemandMulticastInfo.usMulticastVideoPort;
		usRTPAudioPort = pClient->tOndemandMulticastInfo.usMulticastAudioPort;
		usRTPMetadataPort = pClient->tOndemandMulticastInfo.usMulticastMetadataPort;
		usTTL = pClient->tOndemandMulticastInfo.usTTL;
	}
	else
	{
		usRTPVideoPort = pClient->parent->usMulticastVideoPort[pClient->iMulticast-1];
		usRTPAudioPort = pClient->parent->usMulticastAudioPort[pClient->iMulticast-1];
		//20120726 added by Jimmy for metadata
		usRTPMetadataPort = pClient->parent->usMulticastMetadataPort[pClient->iMulticast-1];
		usTTL = pClient->parent->usTTL[pClient->iMulticast-1];

	}
	

	//20120726 modified by Jimmy for metadata
	if( iMediaType == SETUP_VIDEO )
		snprintf(pcBuffer + strlen(pcBuffer) , MAX_SNDBUFF - strlen(pcBuffer) - 1, "Transport: RTP/AVP;multicast;destination=%s;port=%d-%d;ttl=%d\r\n",(char*)inet_ntoa(TempVAddr.sin_addr),usRTPVideoPort,usRTPVideoPort+1,usTTL);
	else if( iMediaType == SETUP_AUDIO)
		snprintf(pcBuffer + strlen(pcBuffer) , MAX_SNDBUFF - strlen(pcBuffer) - 1, "Transport: RTP/AVP;multicast;destination=%s;port=%d-%d;ttl=%d\r\n",(char*)inet_ntoa(TempAAddr.sin_addr),usRTPAudioPort,usRTPAudioPort+1,usTTL);
	else if( iMediaType == SETUP_METADATA)
		snprintf(pcBuffer + strlen(pcBuffer) , MAX_SNDBUFF - strlen(pcBuffer) - 1, "Transport: RTP/AVP;multicast;destination=%s;port=%d-%d;ttl=%d\r\n",(char*)inet_ntoa(TempMAddr.sin_addr),usRTPMetadataPort,usRTPMetadataPort+1,usTTL);


    RTSPServer_AddEndMessage(pcBuffer);
	iRet = RTSPServer_OutSendMessage(pcBuffer,pClient);

    if( iRet == strlen(pcBuffer) )
    {
        return 0;
    }        
    else
    {
        //2005/05/31closesocket(pClient->iSockfd); 
        RTSPServer_InitClient(pClient);
        printf("Client Dead! disconnect..\n");
        return -1;
    }     
    
}
#endif

int RTSPServer_SendSetupReply(RTSP_CLIENT *pClient,unsigned short usRTPPort,unsigned short usRTCPPort,unsigned short ausServerPort[2],RTSP_SERVER* pServer,unsigned long ulSessionID)
{
    char    *pcBuffer;
    int      iRet;
    TOSDateTimeInfo tDateInfo;


    pcBuffer = pClient->parent->acSendBuffer;
    memset(pcBuffer,0,MAX_SNDBUFF);
    snprintf(pcBuffer, MAX_SNDBUFF - 1, "%s %d %s\r\n", "RTSP/1.0", 200,RTSPServer_GetState(200));
    RTSPServer_AddCSeq(pcBuffer,pClient->iCSeq);
    
    snprintf(pcBuffer + strlen(pcBuffer), MAX_SNDBUFF - strlen(pcBuffer) - 1, "Session: %08lu;timeout=%d\r\n", ulSessionID, CHECKIDLECLIENT_TIMEOUT/1000);   

    if( pClient->iRTPStreamingMode == RTP_OVER_UDP)
	{
		snprintf(pcBuffer + strlen(pcBuffer) , MAX_SNDBUFF - strlen(pcBuffer) - 1, "Transport: RTP/AVP;unicast;client_port=%d-%d;server_port=%d-%d\r\n",usRTPPort,usRTCPPort,ausServerPort[0],ausServerPort[1]);
	}
	else
	{
		snprintf(pcBuffer + strlen(pcBuffer) , MAX_SNDBUFF - strlen(pcBuffer) - 1, "Transport: RTP/AVP/TCP;interleaved=%d-%d;unicast;mode=play\r\n",usRTPPort,usRTCPPort);
	}
	
    OSTime_GetDateTime(&tDateInfo);
    snprintf(pcBuffer + strlen(pcBuffer), MAX_SNDBUFF - strlen(pcBuffer) - 1,"Date: %s, %d %s %d %d:%d:%d GMT\r\n",ConvertWeekday(tDateInfo.wWeekDay)
                                                                             ,tDateInfo.wMonthDay
                                                                             ,ConvertMonth(tDateInfo.wMonth)
                                                                             ,tDateInfo.wYear
                                                                             ,tDateInfo.wHour
                                                                             ,tDateInfo.wMinute
                                                                             ,tDateInfo.wSecond);

    //20121005 added by Jimmy. A session identifier MUST be at least eight octets long according to RFC 2326, section 3.4
    
    //sprintf(pcBuffer + strlen(pcBuffer), "Reconnect: true\r\n");
    //sprintf(pcBuffer + strlen(pcBuffer), "RDTFeatureLevel: 3\r\n");
    //sprintf(pcBuffer + strlen(pcBuffer), "RealChallenge3: c508fc2279c6a82f33a422a3181701094f213d09,sdr=cf7a3210\r\n");

	//ShengFh Symetric RTP     
	if( pClient->iSymRTP >0 )
        strncat(pcBuffer,"Server: PVSS\r\n", MAX_SNDBUFF - strlen(pcBuffer) - 1);


	

    RTSPServer_AddEndMessage(pcBuffer);

    iRet = RTSPServer_OutSendMessage(pcBuffer,pClient);

	//TelnetShell_DbgPrint("%s\r\n",pcBuffer);
    if( iRet == strlen(pcBuffer) )
    {
//        printf("SETUP response sent.\n");
        return 0;
    }        
    else
    {
        pClient->parent = pServer;
        //2205/05/31 add by ShengFu
        pClient->iStatus = SETUP_STATE;
        RTSPServer_InitClient(pClient);
        printf("Client Dead! disconnect..\n");
        return -1;
    }     
    
}



int RTSPServer_SendPlayReply(RTSP_CLIENT* pClient,RTSP_SERVER* pServer)
{
    char *pcBuffer;
    int  iRet;
    TOSDateTimeInfo tDateInfo;
#ifdef _SHARED_MEM
	RTSPSERVER_MODREQUEST	stMODRequest;
	char *ptr = NULL;
	//20110915 Modify by danny for support Genetec MOD
	int  i;
#endif

    pcBuffer = pClient->parent->acSendBuffer;
    memset(pcBuffer,0,MAX_SNDBUFF);

#ifdef _SHARED_MEM
	//20100428 Added For Media on demand
	memset(&stMODRequest,0,sizeof(RTSPSERVER_MODREQUEST));
#endif

    snprintf(pcBuffer, MAX_SNDBUFF - 1, "%s %d %s\r\n", "RTSP/1.0", 200,RTSPServer_GetState(200));
    RTSPServer_AddCSeq(pcBuffer,pClient->iCSeq);
    OSTime_GetDateTime(&tDateInfo);
    snprintf(pcBuffer + strlen(pcBuffer), MAX_SNDBUFF - strlen(pcBuffer) - 1,"Date: %s, %d %s %d %d:%d:%d GMT\r\n",ConvertWeekday(tDateInfo.wWeekDay)
                                                                             ,tDateInfo.wMonthDay
                                                                             ,ConvertMonth(tDateInfo.wMonth)
                                                                             ,tDateInfo.wYear
                                                                             ,tDateInfo.wHour
                                                                             ,tDateInfo.wMinute
                                                                             ,tDateInfo.wSecond);

//20110725 Add by danny For Multicast RTCP receive report keep alive
#ifdef RTSPRTP_MULTICAST
	if( pClient->iMulticast > 0 )
	{
		//20121005 added by Jimmy. A session identifier MUST be at least eight octets long according to RFC 2326, section 3.4
		snprintf(pcBuffer + strlen(pcBuffer), MAX_SNDBUFF - strlen(pcBuffer) - 1,"Session: %08lu;timeout=%d\r\n",pClient->ulSessionID, g_iMulticastTimeout);
	}
	else
#endif
	{
		//20121005 added by Jimmy. A session identifier MUST be at least eight octets long according to RFC 2326, section 3.4
		snprintf(pcBuffer + strlen(pcBuffer), MAX_SNDBUFF - strlen(pcBuffer) - 1,"Session: %08lu;timeout=%d\r\n",pClient->ulSessionID, CHECKIDLECLIENT_TIMEOUT/1000);
	}
	
	//ShengFh Symetric RTP     
	if( pClient->iSymRTP >0 )
        strncat(pcBuffer,"Server: PVSS\r\n", MAX_SNDBUFF - strlen(pcBuffer) - 1);


	//!!!For Sony Ericsson handset, public IP is required for RTP-Info reply
/*	sprintf(pcBuffer + strlen(pcBuffer), "RTP-Info: url=rtsp://211.74.11.222/live.sdp/%s;seq=%d;rtptime=%lu;ssrc=%lu"
	                                //,pClient->acObjURL
					                ,pClient->acMediaType[0]
	                                ,pClient->usInitSequence[0]
						            ,pClient->ulInitTimestamp[0]
									,pClient->ulSSRC[0]);

    if(pClient->acMediaType[1][0] != 0 ) 
    {
        sprintf(pcBuffer + strlen(pcBuffer), ",url=rtsp://211.74.11.222/live.sdp/%s;seq=%d;rtptime=%lu;ssrc=%lu\r\n"
                    		        //,pClient->acObjURL
	                      	        ,pClient->acMediaType[1]
				                    ,pClient->usInitSequence[1]
	                  				,pClient->ulInitTimestamp[1]
									,pClient->ulSSRC[1]);
    }
    else
    {
        strcat(pcBuffer,"\r\n");
    }*/

	snprintf(pcBuffer + strlen(pcBuffer), MAX_SNDBUFF - strlen(pcBuffer) - 1, "RTP-Info: url=%s/%s;seq=%d;rtptime=%lu"
									,pClient->acObjURL
					                ,pClient->acMediaType[0]
	                                ,pClient->usInitSequence[0]
						            ,pClient->ulInitTimestamp[0]);

	if(pClient->iVivotekClient == 1)
		snprintf(pcBuffer + strlen(pcBuffer), MAX_SNDBUFF - strlen(pcBuffer) - 1,";ssrc=%lu",pClient->ulSSRC[0]);


    if(pClient->acMediaType[1][0] != 0 ) 
    {
		snprintf(pcBuffer + strlen(pcBuffer), MAX_SNDBUFF - strlen(pcBuffer) - 1, ",url=%s/%s;seq=%d;rtptime=%lu"
									,pClient->acObjURL
	                      	        ,pClient->acMediaType[1]
				                    ,pClient->usInitSequence[1]
	                  				,pClient->ulInitTimestamp[1]);

		if(pClient->iVivotekClient == 1)
			snprintf(pcBuffer + strlen(pcBuffer), MAX_SNDBUFF - strlen(pcBuffer) - 1,";ssrc=%lu",pClient->ulSSRC[1]);
    }

    //20120724 added by Jimmy for metadata
#ifdef _METADATA_ENABLE
    if(pClient->acMediaType[2][0] != 0 ) 
    {
		snprintf(pcBuffer + strlen(pcBuffer), MAX_SNDBUFF - strlen(pcBuffer) - 1, ",url=%s/%s;seq=%d;rtptime=%lu"
									,pClient->acObjURL
	                      	        ,pClient->acMediaType[2]
				                    ,pClient->usInitSequence[2]
	                  				,pClient->ulInitTimestamp[2]);

		if(pClient->iVivotekClient == 1)
			snprintf(pcBuffer + strlen(pcBuffer), MAX_SNDBUFF - strlen(pcBuffer) - 1,";ssrc=%lu",pClient->ulSSRC[2]);
    }
#endif


    strncat(pcBuffer,"\r\n", strlen("\r\n"));

	//20110915 Modify by danny for support Genetec MOD
	if ( pClient->bMediaOnDemand == FALSE )
	{
		snprintf(pcBuffer + strlen(pcBuffer), MAX_SNDBUFF - strlen(pcBuffer) - 1,"Range: npt=0-\r\n");
	}
	// else
	// {
	// 	snprintf(pcBuffer + strlen(pcBuffer), MAX_SNDBUFF - strlen(pcBuffer), "Range: %s\r\n", pClient->tMODInfo.acMODSetCommandValue[MOD_RANGE]);
	// }
	snprintf(pcBuffer + strlen(pcBuffer), MAX_SNDBUFF - strlen(pcBuffer) - 1,"RTCP-Interval: 250\r\n");

#ifdef _SHARED_MEM
	//20100428 Added For Media on demand
	if ( pClient->bMediaOnDemand == TRUE )
	{
		//20151109 stop mod, add by faber
		if(!pClient->iIsPause)
		{
			printf("Start to setting MOD, now stop it...\n");
			pServer->fcallback(pServer->hParentHandle
									, RTSPSERVER_CALLBACKFLAG_SET_MOD_STOP
									, (void*)&(pClient->iSDPIndex), 0);
		}
		//20110915 Modify by danny for support Genetec MOD
		for (i = 0; i < MOD_COMMAND_TYPE_LAST; i++)
		{
			if ( pClient->tMODInfo.eMODSetCommandType[i] == 0 )
			{
				continue;
			}
            //20141110 added by Charles for ONVIF Profile G
            if((pClient->iPlayerType == GENETEC_PLAYER) && (i == MOD_RANGE))
            {
                pClient->tMODInfo.eMODSetCommandType[MOD_IMMEDIATE] = MOD_IMMEDIATE;
                rtspstrcpy(pClient->tMODInfo.acMODSetCommandValue[MOD_IMMEDIATE], "yes", sizeof(pClient->tMODInfo.acMODSetCommandValue[MOD_IMMEDIATE]));
            }
			
			if((pClient->iPlayerType == ONVIF_TEST_TOOL) && (i == MOD_RATECONTROL))
			{
				//NOTE: Translate Rate-Control to Scale or Speed according to User-Agent
				//Translate to Scale only when client is onvif test tool!!!
				stMODRequest.bIsRateControl2Scale = TRUE;
			}
			else
			{
				stMODRequest.bIsRateControl2Scale = FALSE;
			}
			
			stMODRequest.eMODSetCommandType = pClient->tMODInfo.eMODSetCommandType[i];
			pClient->tMODInfo.eMODSetCommandType[i] = 0;
			rtspstrcpy(stMODRequest.acMODSetCommandValue, pClient->tMODInfo.acMODSetCommandValue[i], sizeof(stMODRequest.acMODSetCommandValue));
			memset(pClient->tMODInfo.acMODSetCommandValue[i], 0, sizeof(pClient->tMODInfo.acMODSetCommandValue[i]));
			ptr = strstr(stMODRequest.acMODSetCommandValue, "\r");
			if( ptr != NULL)
			{
				*ptr = '\0';
			}
			stMODRequest.iSDPIndex = pClient->iSDPIndex;

			/*if( stMODRequest.eMODSetCommandType == MOD_RESUME )
			{
				pServer->fcallback(pServer->hParentHandle, RTSPSERVER_CALLBACKFLAG_SET_MODCONTROLINFO, (void*)&stMODRequest, (void*)RTSPMOD_REQUEST_NOWAIT);
			}
			else*/
			{
				if( pServer->fcallback(pServer->hParentHandle
								,RTSPSERVER_CALLBACKFLAG_SET_MODCONTROLINFO
								,(void*)&stMODRequest,0) != S_OK )
				{
					DbgLog((dfCONSOLE|dfINTERNAL,"RTSPSERVER_CALLBACKFLAG_SET_MODCONTROLINFO failed!\n"));
					printf("[%s] RTSPSERVER_CALLBACKFLAG_SET_MODCONTROLINFO failed!\n", __FUNCTION__);
					RTSPServer_DiscardMessage(pClient);
        			RTSPServer_SendReply(503,0,pClient,pServer);
					pClient->parent = pServer;
                    //20141110 modified by Charles for ONVIF Profile G
                    //Don't close socket when MOD PLAY Queue is Full
                    if(!stMODRequest.bIsPlayQueueFull)
                    {
                        RTSPServer_InitClient(pClient);
                    }
        			return (1);
    			}
			
				if ( stMODRequest.eMODSetCommandType == stMODRequest.eMODReturnCommandType )
				{
					if( stMODRequest.eMODSetCommandType != MOD_PAUSE && stMODRequest.eMODSetCommandType != MOD_RESUME )
					{
						snprintf(pcBuffer + strlen(pcBuffer), MAX_SNDBUFF - strlen(pcBuffer) - 1,"%s: %s\r\n", 
								RTSPServer_GetMODHeaderField(stMODRequest.eMODReturnCommandType), stMODRequest.acMODReturnCommandValue);
					}
				}
				else
				{
					printf("[%s] eMODSetCommandType %d and eMODReturnCommandType %d mismatch!\n", __FUNCTION__, stMODRequest.eMODSetCommandType, stMODRequest.eMODReturnCommandType);
				}
			}
		}
	}
#endif

    RTSPServer_AddEndMessage(pcBuffer);

    iRet = RTSPServer_OutSendMessage(pcBuffer,pClient);
	
	//TelnetShell_DbgPrint("%s\r\n",pcBuffer);
    if( iRet == strlen(pcBuffer) )
    {
//        printf("PLAY response sent.\n");
    	if(pClient->bMediaOnDemand && !pClient->iIsPause) // start MOD, add by faber
		{
			printf("Setting done, start MOD\n");
			pServer->fcallback(pServer->hParentHandle
									, RTSPSERVER_CALLBACKFLAG_SET_MOD_START
									, (void*)&(pClient->iSDPIndex), 0);
		}
        return 0;
    }        
    else
    {
		RTSPServer_SessionStop(pClient->parent, pClient->ulSessionID,pClient->iMulticast);
        //2005/05/31closesocket(pClient->iSockfd); 
        pClient->parent = pServer;
        RTSPServer_InitClient(pClient);
        printf("Client Dead! disconnect..\n");
        return -1;
    }   
    
	
	return 0;
}

int RTSPServer_SendSetParameterReply(RTSP_CLIENT* pClient,RTSP_SERVER* pServer)
{                
    char *pcBuffer;
    int  iRet;
    TOSDateTimeInfo tDateInfo;    
    
    pcBuffer = pClient->parent->acSendBuffer;
    memset(pcBuffer,0,MAX_SNDBUFF);

    snprintf(pcBuffer, MAX_SNDBUFF - 1, "%s %d %s\r\n", "RTSP/1.0", 200,RTSPServer_GetState(200));
    RTSPServer_AddCSeq(pcBuffer,pClient->iCSeq);
    OSTime_GetDateTime(&tDateInfo);
    snprintf(pcBuffer + strlen(pcBuffer), MAX_SNDBUFF - strlen(pcBuffer) - 1,"Date: %s, %d %s %d %d:%d:%d GMT\r\n",ConvertWeekday(tDateInfo.wWeekDay)
                                                                             ,tDateInfo.wMonthDay
                                                                             ,ConvertMonth(tDateInfo.wMonth)
                                                                             ,tDateInfo.wYear
                                                                             ,tDateInfo.wHour
                                                                             ,tDateInfo.wMinute
                                                                             ,tDateInfo.wSecond);

    //20121005 added by Jimmy. A session identifier MUST be at least eight octets long according to RFC 2326, section 3.4
    snprintf(pcBuffer + strlen(pcBuffer), MAX_SNDBUFF - strlen(pcBuffer) - 1,"Session: %08lu;timeout=%d\r\n",pClient->ulSessionID, CHECKIDLECLIENT_TIMEOUT/1000);
    RTSPServer_AddEndMessage(pcBuffer);

	iRet = RTSPServer_OutSendMessage(pcBuffer,pClient);

    RTSPServer_DiscardMessage(pClient);

    if( iRet == strlen(pcBuffer) )
    {
        return 0;
    }        
    else
    {
		RTSPServer_SessionStop(pClient->parent, pClient->ulSessionID,pClient->iMulticast);
        //2005/05/31closesocket(pClient->iSockfd); 
        pClient->parent = pServer;
        RTSPServer_InitClient(pClient);
        printf("Client Dead! disconnect..\n");
        return -1;
    }   				
}

//20101006 Added by danny for support GET_PARAMETER command for keep-alive in RTSP signaling
int RTSPServer_SendGetParameterReply(RTSP_CLIENT* pClient,RTSP_SERVER* pServer)
{                
    char *pcBuffer;
    int  iRet;
    TOSDateTimeInfo tDateInfo;    
    
    pcBuffer = pClient->parent->acSendBuffer;
    memset(pcBuffer,0,MAX_SNDBUFF);

    snprintf(pcBuffer, MAX_SNDBUFF - 1, "%s %d %s\r\n", "RTSP/1.0", 200,RTSPServer_GetState(200));
    RTSPServer_AddCSeq(pcBuffer,pClient->iCSeq);
    OSTime_GetDateTime(&tDateInfo);
    snprintf(pcBuffer + strlen(pcBuffer), MAX_SNDBUFF - strlen(pcBuffer) - 1,"Date: %s, %d %s %d %d:%d:%d GMT\r\n",ConvertWeekday(tDateInfo.wWeekDay)
                                                                             ,tDateInfo.wMonthDay
                                                                             ,ConvertMonth(tDateInfo.wMonth)
                                                                             ,tDateInfo.wYear
                                                                             ,tDateInfo.wHour
                                                                             ,tDateInfo.wMinute
                                                                             ,tDateInfo.wSecond);

    //20121005 added by Jimmy. A session identifier MUST be at least eight octets long according to RFC 2326, section 3.4
    snprintf(pcBuffer + strlen(pcBuffer), MAX_SNDBUFF - strlen(pcBuffer) - 1,"Session: %08lu;timeout=%d\r\n",pClient->ulSessionID, CHECKIDLECLIENT_TIMEOUT/1000);
    RTSPServer_AddEndMessage(pcBuffer);

	iRet = RTSPServer_OutSendMessage(pcBuffer,pClient);

    RTSPServer_DiscardMessage(pClient);

    if( iRet == strlen(pcBuffer) )
    {
        return 0;
    }        
    else
    {
		RTSPServer_SessionStop(pClient->parent, pClient->ulSessionID,pClient->iMulticast);
        //2005/05/31closesocket(pClient->iSockfd); 
        pClient->parent = pServer;
        RTSPServer_InitClient(pClient);
        printf("Client Dead! disconnect..\n");
        return -1;
    }   				
}

int RTSPServer_SendOptionsReply(RTSP_CLIENT* pClient,RTSP_SERVER* pServer)
{
    char *pcBuffer;
    int  iRet;
    TOSDateTimeInfo tDateInfo;
    
    pcBuffer = pClient->parent->acSendBuffer;
    memset(pcBuffer,0,MAX_SNDBUFF);

    snprintf(pcBuffer, MAX_SNDBUFF - 1, "%s %d %s\r\n", "RTSP/1.0", 200,RTSPServer_GetState(200));
    RTSPServer_AddCSeq(pcBuffer,pClient->iCSeq);
    
    OSTime_GetDateTime(&tDateInfo);
    snprintf(pcBuffer + strlen(pcBuffer), MAX_SNDBUFF - strlen(pcBuffer) - 1,"Date: %s, %d %s %d %d:%d:%d GMT\r\n",ConvertWeekday(tDateInfo.wWeekDay)
                                                                             ,tDateInfo.wMonthDay
                                                                             ,ConvertMonth(tDateInfo.wMonth)
                                                                             ,tDateInfo.wYear
                                                                             ,tDateInfo.wHour
                                                                             ,tDateInfo.wMinute
                                                                             ,tDateInfo.wSecond);
    //sprintf(pcBuffer + strlen(pcBuffer),"Server: Helix Mobile Server Version 10.0.5.1621 (linux-2.2-libc6-i586-server) (RealServer compatible)\r\n");
    snprintf(pcBuffer + strlen(pcBuffer), MAX_SNDBUFF - strlen(pcBuffer) - 1,"Public: OPTIONS, DESCRIBE, PLAY, SETUP, GET_PARAMETER, SET_PARAMETER, TEARDOWN\r\n");
    //sprintf(pcBuffer + strlen(pcBuffer),"RealChallenge1: 2a9a2f0f62fcd8b63ff99ca2a8b5eb65\r\n");
    //sprintf(pcBuffer + strlen(pcBuffer),"StatsMask: 3\r\n");
    RTSPServer_AddEndMessage(pcBuffer);

	iRet = RTSPServer_OutSendMessage(pcBuffer,pClient);


    //TelnetShell_DbgPrint("%s\r\n",pcBuffer);
    

    RTSPServer_DiscardMessage(pClient);

    if( iRet == strlen(pcBuffer) )
    {
        return 0;
    }        
    else
    {
		RTSPServer_SessionStop(pClient->parent, pClient->ulSessionID,pClient->iMulticast);
        //2005/05/31closesocket(pClient->iSockfd); 
        pClient->parent = pServer;
        RTSPServer_InitClient(pClient);
        printf("Client Dead! disconnect..\n");
        return -1;
    }   

}


#ifdef WISE_SPOT_AUTHENTICATE

int RTSPServer_HandleWiseSpotAuthenticate(RTSP_CLIENT *pClient)
{
	unsigned long ulID;

	//For WiseSpot authentication
	if( pClient->ulClientAddress != pClient->parent->dwAuthenticateIP )
	{
		if( strstr(pClient->acExtraInfo,"session_id=") == 0 ) 
			return -1;

#ifdef _LINUX
		ulID = atoll(pClient->acExtraInfo + strlen("session_id=")); 
#else
		ulID = atol(pClient->acExtraInfo + strlen("session_id=")); 
#endif
		if( ulID != 0 )		
		{
			if( ((((ulID&0xff000000) >> 24) + ( (ulID&0x00ff0000) >> 16) +
				 ((ulID&0x0000ff00) >> 8)  + (ulID&0x000000ff) )&0x000000ff) == 0 )
			{
				pClient->ulWiseSpotID = ulID;
				return 0;
			}
			else
				return -1;
		}
		else
			return -1;
	}
	else
	{
		if( (pClient->acExtraInfo[0] != 0) && (strstr(pClient->acExtraInfo,"session_id=") != 0 ) )
		{
			pClient->ulWiseSpotID = atol(pClient->acExtraInfo + strlen("session_id="));
		}
		return 0;
	}

}


#endif


int	RTSPServer_HandleAuthentication(RTSP_CLIENT *pClient)
{
	int	iRet,iDecodeSize;
	TRAWAUTHORINFO	tRawAuthorInfo;
    TAuthorInfo     tAuthorInfo;
    char            acDecode[256];
	char			*pChar;

	memset((void*)&tRawAuthorInfo,0,sizeof(TRAWAUTHORINFO));
	memset((void*)&tAuthorInfo,0,sizeof(TAuthorInfo));

	rtspstrcpy(tAuthorInfo.acAccessName,pClient->acObject, sizeof(tAuthorInfo.acAccessName));

	iRet = -1;    
    //ShengFu 2005/08/26 RTSP Authorization
    tAuthorInfo.iAuthType = pClient->parent->iAuthenticationType;

    iRet = pClient->parent->fcallback(pClient->parent->hParentHandle
								,RTSPSERVER_CALLBACKFLAG_CHECK_NEED_AUTHORIZATOIN
								,&tAuthorInfo,0);

    if(iRet == 0)
    {
    	printf("no need to auth....\n");
    	return iRet;
    }

	if( RTSPServer_GetAuthorInfo(pClient->acRecvBuffer,&tRawAuthorInfo) == 0 )
    {
    	// if authnetication is turned on and client send wrong auth mode
    	if( ( tAuthorInfo.iAuthType != RTSP_AUTH_DISABLE) &&
    	    ( tRawAuthorInfo.iAuthMode != tAuthorInfo.iAuthType ) )
    	{
        	return iRet;
    	}
    		
		if( tRawAuthorInfo.iAuthMode == RTSP_AUTH_BASIC )
		{
			iDecodeSize = EncryptionUtl_Base64_DecodeString(tRawAuthorInfo.acResponse,acDecode,256);
			//CID:189, CHECKER: NEGATIVE RETURNS
			if(iDecodeSize > 0)
			{
				acDecode[iDecodeSize] = 0;   
				pChar = strchr(acDecode,':');
				if( pChar != NULL )
				{
					if( pChar-acDecode < 128 &&	acDecode+iDecodeSize-pChar-1 < 128 )
					{
						memcpy(tAuthorInfo.acUserName,acDecode,pChar-acDecode);
						tAuthorInfo.acUserName[pChar-acDecode] = 0;
						//20080611 added for client management
						rtspstrcpy(pClient->acUserName, tAuthorInfo.acUserName, sizeof(pClient->acUserName));

						memcpy(tAuthorInfo.acPasswd,pChar+1,acDecode+iDecodeSize-pChar-1);            
						tAuthorInfo.acPasswd[acDecode+iDecodeSize-pChar-1] = 0;
						//tAuthorInfo.iAuthType = RTSP_AUTH_BASIC;
	//			        TelnetShell_DbgPrint("User:Passwd %s %d:%s %d \r\n",tAuthorInfo.acUserName,strlen(tAuthorInfo.acUserName),tAuthorInfo.acPasswd,strlen(tAuthorInfo.acPasswd));	    
						iRet = pClient->parent->fcallback(pClient->parent->hParentHandle
								,RTSPSERVER_CALLBACKFLAG_AUTHORIZATION
								,&tAuthorInfo,0);
					}
				}    
			}
			//20130903 Charles Fix basic authentication common bug 
			/*else
			{
				iRet = 0;
			}*/
		}
		else if( tRawAuthorInfo.iAuthMode == RTSP_AUTH_DIGEST)//Digest Authentication
		{
			rtspstrcpy(tAuthorInfo.acUserName,tRawAuthorInfo.acUserName, sizeof(tAuthorInfo.acUserName));
			//20080611 added for client management
			rtspstrcpy(pClient->acUserName, tAuthorInfo.acUserName, sizeof(pClient->acUserName));
			//20160818 Modify by Faber, each client has specify nonce
			if( (iRet = strcmp(pClient->acAuthenNonce,tRawAuthorInfo.acNonce)) == 0)
			{
				//tAuthorInfo.iAuthType = RTSP_AUTH_DIGEST;
				iRet = pClient->parent->fcallback(pClient->parent->hParentHandle
			            ,RTSPSERVER_CALLBACKFLAG_AUTHORIZATION
			            ,&tAuthorInfo,0);
				if( iRet == 0 )
				{				
					char	acHash[33],acResponse[33];	
	
					memset(acHash,0,33);
					memset(acResponse,0,33);

					//20131111 modified by Charles for digest authentication fail when using ffmpeg
					//pChar = strrchr(pClient->acObjURL,'/');		
					pChar = tRawAuthorInfo.acURI;
					
					EncryptionUtl_MD5(acHash,"DESCRIBE",":",pChar,NULL);
					//20160818 Modify by Faber, each client has specify nonce
printf("Using URL:%s Pass:%s Nonce:%s NCount:%s CNonce:%s HA2:%s\n", pChar, tAuthorInfo.acPasswd, pClient->acAuthenNonce, tRawAuthorInfo.acNonceCount,tRawAuthorInfo.acCNonce, acHash);

					//20140221 modified by Charles to follow the method of RTSP digest auth defined in RFC2069
					/*EncryptionUtl_MD5(acResponse,tAuthorInfo.acPasswd,":"
						          ,pClient->parent->acAuthenNonce,":"
								  ,tRawAuthorInfo.acNonceCount,":"
								  ,tRawAuthorInfo.acCNonce,":"
								  ,"auth",":"
								  ,acHash,NULL);*/
				  //20160818 Modify by Faber, each client has specify nonce
					EncryptionUtl_MD5(acResponse,tAuthorInfo.acPasswd,":"
						          ,pClient->acAuthenNonce,":"
								  ,acHash,NULL);

printf("Compare %s to %s\n", acResponse, tRawAuthorInfo.acResponse);

					if( memcmp(acResponse,tRawAuthorInfo.acResponse,32) == 0 )
						iRet = S_OK;
					else
						iRet = -1;
				}
			}
			else
			{
				iRet = -1;
			}
		}
		else	//no authentication
		{	
			iRet = S_OK;
		}
    }
    else
    {
    	//20161212 we should require authorization
        return -1;
	}

	return iRet;
}


/****** Handle coming RTSP request ****************************/

int RTSPServer_HandleDescribeRequest(char *pcBuffer,RTSP_CLIENT *pClient,RTSP_SERVER* pServer)
{	
	//CID:1180, CHECKER:UNINIT
    unsigned short usRTSPPort = 0;
    char            acStringLine[MAX_LINE_LEN];
    int             iHeaderLen;		    /* message header length */
    int             iMsgBodyLen,iRet;		/* message body length */
/*	char			*pChar;
	TRAWAUTHORINFO	tRawAuthorInfo;
    TAuthorInfo     tAuthorInfo;

	memset((void*)&tRawAuthorInfo,0,sizeof(TRAWAUTHORINFO));
	memset((void*)&tAuthorInfo,0,sizeof(TAuthorInfo));*/

    printf("DESCRIBE request received.\n");
    TelnetShell_DbgPrint("%s\r\n",pClient->acRecvBuffer);
    //printf("pcBuffer=%s\r\n",pcBuffer);   
    if (!sscanf(pcBuffer, " %*s %254s ", pClient->acObjURL))
    {
//        DbgPrint(("DESCRIBE request is missing object (path/file) parameter.\n"));
        RTSPServer_DiscardMessage(pClient);	
        RTSPServer_SendReply(400, 0,pClient,pServer);		
        RTSPServer_CleanBuffer(pClient);
        return (1);
    }
	//printf("pClient->acObjURL=%s\r\n",pClient->acObjURL);
	/* Modified for Shmem */
	if ((iRet = RTSPServer_ParseURL(pClient->acObjURL, pClient, &usRTSPPort, eDescribeMethod)) != 1)
    {
        RTSPServer_DiscardMessage(pClient);	
		//20081218 for 415 Unsupported Media Type
		if(iRet == -2)
		{
			RTSPServer_SendReply(415, 0, pClient, pServer);
		}
		else
		{
			RTSPServer_SendReply(400, 0, pClient, pServer);
		}
        RTSPServer_CleanBuffer(pClient);
        return (1);
    }
	
#ifdef  RTSP_AUTHORIZATION
	iRet = S_OK;
	//Modified 20081121 for URL authentication
	if( !(pClient->iRTPStreamingMode == RTP_OVER_HTTP || (pClient->bURLAuthenticated && pClient->parent->iURLAuthEnabled)))
	{
		iRet = RTSPServer_HandleAuthentication(pClient);
	}

    if( iRet != S_OK )
    {
        RTSPServer_DiscardMessage(pClient);	
        RTSPServer_SendReply(401, 0,pClient,pServer);
        RTSPServer_CleanBuffer(pClient);
        return (1);
    }
	
#endif
	memset((void*)pClient->acAuthenNonce,0,NONCE_LENGTH);
    RTSPServer_GetMessageLen(&iHeaderLen, &iMsgBodyLen,pClient);	/* set header and message body length */ 
    if( (iRet= RTSPServer_GetString(pClient->acRecvBuffer,iHeaderLen, "User-Agent", ": \t",acStringLine, sizeof(acStringLine))) == 0 )
    {
#ifndef _WIN32_
#ifndef _LINUX_X86
        if(rtspstrncasecmp(acStringLine,"RTPExPlayer",11) == 0)
        {
            TelnetShell_DbgPrint("RTPEX player!\r\n");
			pClient->iVivotekClient = 1;
		}	
		else
			pClient->iVivotekClient = 0;
#endif
#endif
		if( (rtspstrncasecmp(acStringLine,"qts ",3) == 0) ||
			(rtspstrncasecmp(acStringLine,"QuickTime",strlen("QuickTime")) == 0) )
		{
			pClient->iPlayerType = QUICKTIME_PLAYER;
		}

		//20111124 modify by danny support Backchannel Multicast for ONVIF test tool 1.02.4
		if(rtspstrncasecmp(acStringLine,"ONVIF Filter",12) == 0)
		{
			pClient->iPlayerType = ONVIF_TEST_TOOL;
		}
        //20141110 added by Charles for ONVIF Profile G
        if(rtspstrncasecmp(acStringLine,"OmnicastRTSPClient",18) == 0)
		{
			pClient->iPlayerType = GENETEC_PLAYER;
		}
    }

    
    //if (!RTSPServer_ParseURL(pClient->acObjURL, pClient->acServerName, &usRTSPPort, pClient->acObject))
	/*if (!RTSPServer_ParseURL(pClient->acObjURL, pClient->acServerName, &usRTSPPort, pClient->acObject, pClient->acExtraInfo))
    {
//        DbgPrint(("Mangled URL in DESCRIBE.\n"));
        RTSPServer_DiscardMessage(pClient);	
        RTSPServer_SendReply(400, 0,pClient,pServer);
        RTSPServer_CleanBuffer(pClient);
        return (1);
    }*/

#ifdef WISE_SPOT_AUTHENTICATE
	if( RTSPServer_HandleWiseSpotAuthenticate(pClient) != 0 )
	{
	    pClient->parent = pServer;
        RTSPServer_InitClient(pClient);
        return(1);
	}
#endif

    RTSPServer_DiscardMessage(pClient);		
  /* disable path outside current directory. */
    if (strstr(pClient->acObject, "../"))
    {
//        DbgPrint(("DESCRIBE request specified an object parameter with a path "
//        "that is not allowed. '../' not permitted in path.\n"));
        /* unsupported media type */
        RTSPServer_SendReply(403, 0,pClient,pServer);
        RTSPServer_CleanBuffer(pClient);
        return (1);
    }

  /* reach the limit of client No. reject and disconnect*/
    if( pClient->iTimeOut < 0 )
    {
        /*DbgLog((dfCONSOLE|dfINTERNAL,"Not enough bandwidth!\n"));
        RTSPServer_SendReply(453,0,pClient,pServer);*/
		//20091207 modify return code from 453 to 503 Service Unavailable is returned when connection limit is reached
		DbgLog((dfCONSOLE|dfINTERNAL,"Service Unavailable!\n"));
        RTSPServer_SendReply(503,0,pClient,pServer);
        RTSPServer_CleanBuffer(pClient);
        //2005/05/31closesocket(pClient->iSockfd);
        pClient->parent = pServer;
        RTSPServer_InitClient(pClient);
        return(1);
    }

    if( RTSPServer_SendDescribeReply(pClient,pServer) != 0 )
    {
        DbgLog((dfCONSOLE|dfINTERNAL,"Describe Reply Sent out Failed!\n"));
        return 1;
    }

    TelnetShell_DbgPrint("SDP sendout \r\n");

#ifdef RTSP_DEBUG_LOG    
    ADD_LOG("DESCRIBE reply sent.\n");
#endif
    
    //pClient->iTimeOut = 0;
	OSTick_GetMSec(&pClient->dwBaseMSec);

    return (0);
}

int RTSPServer_HandleSetupRequest(char *pcFirstLine,RTSP_CLIENT *pClient,RTSP_SERVER* pServer)
{
    int             iRet,i, iIndex;		
	//20120725 added by Jimmy for metadata
	int             j;
    char            acStringLine[MAX_LINE_LEN];
    int             iHeaderLen;		    /* message header length */
    int             iMsgBodyLen;		/* message body length */
    unsigned short  usRTPPort=0,usRTCPPort=0,usRTSPPort;
    unsigned short  ausServerPort[2];
    unsigned long   ulSessionID;
    RTSP_SOCKADDR sockaddr;
	int				iMediaType = 0;
	int iStreamMode;
	//int iVideo;
	//20120809 added by Jimmy for metadata
	int iTrackMediaType;

	char *pchar2; 
	//20130924 added by Charles for ondemand multicast
	bool  bWithOnDemandMulticast = FALSE;  //to decide if client demand multicast info

    printf("SETUP request received.\n");
    TelnetShell_DbgPrint("%s\r\n", pcFirstLine);
 //   TelnetShell_DbgPrint("%s\r\n",pClient->acRecvBuffer);
        

    if (!sscanf(pcFirstLine, " %*s %254s ", pClient->acObjURL))
    {
        DbgLog((dfCONSOLE|dfINTERNAL,"SETUP request is missing object (path/file) parameter.\n"));
        RTSPServer_DiscardMessage(pClient);	
        RTSPServer_SendReply(400, 0,pClient,pServer);		
        RTSPServer_InitClient(pClient);
        return -1;
    }
	/* Modified for Shmem */
	//CID:220, CHECKER: NO_EFFECT
    if (!RTSPServer_ParseURL(pClient->acObjURL, pClient, &usRTSPPort, eSetupMethod) && pClient->acObject[0] == 0)
    {
//      DbgPrint(("Mangled URL in SETUP.\n"));
        RTSPServer_DiscardMessage(pClient);	
        RTSPServer_SendReply(400, 0,pClient,pServer);		
        RTSPServer_InitClient(pClient);
        return -1;
    }

	//20120925 added by Jimmy for ONVIF backchannel
	if( (pClient->iRequire == REQUIRE_ONVIF_BACKCHANNEL) && (pClient->bMediaOnDemand != TRUE) )
	{
		pchar2 = strrchr(pClient->acObject,'/');
		printf("pClient->acObject = %s\n", pClient->acObject);

		if( pchar2 != NULL )
		{
			iTrackMediaType = pClient->parent->fcallback(pClient->parent->hParentHandle
										,RTSPSERVER_CALLBACKFLAG_CHECK_TRACK_MEDIATYPE
										,(void*)pClient->iSDPIndex,pchar2+1);
		}
		else
		{
			iTrackMediaType = pClient->parent->fcallback(pClient->parent->hParentHandle
										,RTSPSERVER_CALLBACKFLAG_CHECK_TRACK_MEDIATYPE
										,(void*)pClient->iSDPIndex,pClient->acObject);
		}

		if( iTrackMediaType == RTSPSERVER_MEDIATYPE_AUDIOBACK)
		{
			iMediaType = SETUP_AUDIOBACK;
		}
	}
	
	//20120925 modified by Jimmy for ONVIF backchannel
	//20120724 modified by Jimmy for metadata
	for( i=0 ; (iMediaType != SETUP_AUDIOBACK) && (i < MEDIA_TYPE_NUMBER) ; i++)
	{
		if( pClient->acMediaType[i][1] == 0 )
		{
			pchar2 = strrchr(pClient->acObject,'/');
		
			if( pchar2 != NULL )
				rtspstrcpy(pClient->acMediaType[i],pchar2+1, sizeof(pClient->acMediaType[i]));
			else	
				rtspstrcpy(pClient->acMediaType[i],pClient->acObject, sizeof(pClient->acMediaType[i]));	

			//20120809 modified by Jimmy for metadata
			/*
			iVideo = pClient->parent->fcallback(pClient->parent->hParentHandle
										,RTSPSERVER_CALLBACKFLAG_CHECK_VIDEO_TRACK
										,(void*)pClient->iSDPIndex,pClient->acMediaType[i]);	

			iStreamMode = pClient->parent->fcallback(pClient->parent->hParentHandle
										,RTSPSERVER_CALLBACKFLAG_CHECK_STREAM_MODE
										,(void*)pClient->iSDPIndex,0);

			if( (iVideo == TRUE) && (iStreamMode != RTSPSERVER_MEDIATYPE_AUDIOONLY ))			
				iMediaType = SETUP_VIDEO;
			else if( (iVideo == FALSE) && (iStreamMode != RTSPSERVER_MEDIATYPE_VIDEOONLY ))					
				iMediaType = SETUP_AUDIO;
			else
			{
				RTSPServer_DiscardMessage(pClient);	
				RTSPServer_SendReply(400, 0,pClient,pServer);		
				RTSPServer_InitClient(pClient);
				return -1;
			}
			*/

			iTrackMediaType = pClient->parent->fcallback(pClient->parent->hParentHandle
										,RTSPSERVER_CALLBACKFLAG_CHECK_TRACK_MEDIATYPE
										,(void*)pClient->iSDPIndex,pClient->acMediaType[i]);	

			iStreamMode = pClient->parent->fcallback(pClient->parent->hParentHandle
										,RTSPSERVER_CALLBACKFLAG_CHECK_STREAM_MODE
										,(void*)pClient->iSDPIndex,0);

			if( (iTrackMediaType == RTSPSERVER_MEDIATYPE_VIDEO) && (iStreamMode & RTSPSERVER_MEDIATYPE_VIDEO))
				iMediaType = SETUP_VIDEO;
			else if( (iTrackMediaType == RTSPSERVER_MEDIATYPE_AUDIO) && (iStreamMode & RTSPSERVER_MEDIATYPE_AUDIO))					
				iMediaType = SETUP_AUDIO;
			else if( (iTrackMediaType == RTSPSERVER_MEDIATYPE_METADATA) && (iStreamMode & RTSPSERVER_MEDIATYPE_METADATA))					
				iMediaType = SETUP_METADATA;
			else
			{
				RTSPServer_DiscardMessage(pClient);	
				RTSPServer_SendReply(400, 0,pClient,pServer);		
				RTSPServer_InitClient(pClient);
				return -1;
			}


			break;
		}
	}
	//printf("[%s] pClient->iSDPIndex=%d, iStreamMode=%d, iMediaType=%d\n", __FUNCTION__, pClient->iSDPIndex, iStreamMode, iMediaType);
    RTSPServer_GetMessageLen(&iHeaderLen, &iMsgBodyLen,pClient);	/* set header and message body length */
    
    iRet= RTSPServer_GetString(pClient->acRecvBuffer,iHeaderLen, "Transport", ": \t", acStringLine, sizeof(acStringLine));
    
    if ( iRet != 0 )
    {
        DbgLog((dfCONSOLE|dfINTERNAL,"ALERT: Missing Transport settings.\n"));
        RTSPServer_RemoveMessage(iHeaderLen + iMsgBodyLen,pClient);	/* remove remainder of message from in_buffer */
        RTSPServer_SendReply(461,0,pClient,pServer);
        RTSPServer_InitClient(pClient);
        return -1;
    }

    if (rtspstrncasecmp(acStringLine, "rtp/avp", 7))
    {
        DbgLog((dfCONSOLE|dfINTERNAL,"ALERT: Missing Transport settings.\n"));
        RTSPServer_RemoveMessage(iHeaderLen + iMsgBodyLen,pClient);	/* remove remainder of message from in_buffer */
        /* return "not acceptable" status */
        RTSPServer_SendReply(461,0,pClient,pServer);
        RTSPServer_InitClient(pClient);
        return -1;   
    }

	if(!rtspstrncasecmp(acStringLine, "rtp/avp/tcp",11))
    {
		printf("RTP over RTSP!!\n");
		//20120925 modified by Jimmy for ONVIF backchannel
		if( (iMediaType == SETUP_AUDIOBACK) || (pClient->iRTPStreamingMode == RTP_OVER_UDP) )
		{
			DbgLog((dfCONSOLE|dfINTERNAL,"ALERT: Wrong Transport settings for UDP.\n"));
			RTSPServer_RemoveMessage(iHeaderLen + iMsgBodyLen,pClient);	/* remove remainder of message from in_buffer */
			/* return "not acceptable" status */
			RTSPServer_SendReply(461,0,pClient,pServer);
			RTSPServer_InitClient(pClient);
			return -1; 
		}

#ifdef RTSPRTP_MULTICAST
		/*if( pClient->parent->iCurrUnicastNumber >= pClient->parent->iMaxUnicastNumber )
		{
			DbgLog((dfCONSOLE|dfINTERNAL,"Not enough bandwidth!\n"));
			RTSPServer_SendReply(453,0,pClient,pServer);
			RTSPServer_CleanBuffer(pClient);
			//2005/05/31closesocket(pClient->iSockfd);
			pClient->parent = pServer;
			RTSPServer_InitClient(pClient);
			return -1;
		}
		else*/
		{
		    if( pClient->ulSessionID == 0)
			    pClient->parent->iCurrUnicastNumber++;
		}	
#endif
		if( pClient->iRTPStreamingMode != RTP_OVER_HTTP )
			pClient->iRTPStreamingMode = RTP_OVER_TCP;
		
		if( pClient->ulSessionID == 0 )
		{
			if( RTSPServer_GetInterleavedID(pClient->acRecvBuffer,pClient->iEmbeddedRTPID,pClient->iEmbeddedRTCPID) != 0 )
			{
			
				pClient->iEmbeddedRTPID[0] =usRTPPort= 1;
				pClient->iEmbeddedRTCPID[0]=usRTCPPort = 2;
			}
			else //ShengFu bug fixed for wrong channel ID if client assigned it
			{
				 usRTPPort= pClient->iEmbeddedRTPID[0];
				 usRTCPPort = pClient->iEmbeddedRTCPID[0];
			}
		}
		else
		{
			//20120725 modified by Jimmy for metadata
			for( i = 1 ; i < MEDIA_TYPE_NUMBER ; i++ )
			{
				if( (pClient->iEmbeddedRTPID[i] == -1) && (pClient->iEmbeddedRTCPID[i] == -1) )
				{
					if( RTSPServer_GetInterleavedID(pClient->acRecvBuffer, &pClient->iEmbeddedRTPID[i], &pClient->iEmbeddedRTCPID[i]) != 0 )
					{
						pClient->iEmbeddedRTPID[i] =usRTPPort= pClient->iEmbeddedRTPID[i-1] + 2;
						pClient->iEmbeddedRTCPID[i] =usRTCPPort= pClient->iEmbeddedRTCPID[i-1] + 2;
					}
					else//ShengFu bug fixed for wrong channel ID if client assigned it			
					{

						//20120725 modified by Jimmy for metadata
						for(j = 0; j < i; j++)
						{
							if( (pClient->iEmbeddedRTPID[i] == pClient->iEmbeddedRTPID[j]) ||
								(pClient->iEmbeddedRTCPID[i] == pClient->iEmbeddedRTCPID[j]) ||
								(pClient->iEmbeddedRTCPID[i] == pClient->iEmbeddedRTPID[j]) ||
								(pClient->iEmbeddedRTPID[i] == pClient->iEmbeddedRTCPID[j]) )
							{
								printf("Interleaved ID conflict!!! pClient->iEmbeddedRTPID[%d]=%d, pClient->iEmbeddedRTCPID[%d]=%d, pClient->iEmbeddedRTPID[%d]=%d, pClient->iEmbeddedRTCPID[%d]=%d\r\n",
										j, pClient->iEmbeddedRTPID[j], j, pClient->iEmbeddedRTCPID[j], i, pClient->iEmbeddedRTPID[i], i, pClient->iEmbeddedRTCPID[i]);
					
								RTSPServer_SendReply(461,0,pClient,pServer);
								RTSPServer_CleanBuffer(pClient);
								return -1; 
							}
						}
						usRTPPort= pClient->iEmbeddedRTPID[i];
						usRTCPPort = pClient->iEmbeddedRTCPID[i];
					}	
					break;
				}
			}
		}
	}
	else // for RTP over UDP mode
	{
		if( pClient->iRTPStreamingMode == RTP_OVER_TCP || pClient->iRTPStreamingMode == RTP_OVER_HTTP)
		{
			DbgLog((dfCONSOLE|dfINTERNAL,"ALERT: Wrong Transport settings for UDP.\n"));
			RTSPServer_RemoveMessage(iHeaderLen + iMsgBodyLen,pClient);	/* remove remainder of message from in_buffer */
			/* return "not acceptable" status */
			RTSPServer_SendReply(461,0,pClient,pServer);
			RTSPServer_InitClient(pClient);
			return -1; 
		}

		pClient->iRTPStreamingMode = RTP_OVER_UDP;
		
		//ShengFh Symetric RTP       
        RTSPServer_GetMessageLen(&iHeaderLen, &iMsgBodyLen,pClient);	/* set header and message body length */ 
        //20120925 modified by Jimmy for ONVIF backchannel
        if( (iMediaType != SETUP_AUDIOBACK) && ((iRet= RTSPServer_GetString(pClient->acRecvBuffer,iHeaderLen, "User-Agent", ": \t",acStringLine, sizeof(acStringLine))) == 0) )
        {
            if( (rtspstrncasecmp(acStringLine,"PVPlayer",8) == 0 )  ||
                (rtspstrncasecmp(acStringLine,"RTPExPlayer",11) == 0 ) ||
                (rtspstrncasecmp(acStringLine,"RealMedia Player",16) == 0 ))
            {
			    pClient->iSymRTP ++;

			    if(rtspstrncasecmp(acStringLine,"RealMedia Player ",17) == 0)
			        pClient->iPlayerType = REAL_MEDIA_PLAYER;
				else if(rtspstrncasecmp(acStringLine,"PVPlayer",8) == 0)
			        pClient->iPlayerType = PACKET_VIDEO_PLAYER;

				if( (rtspstrncasecmp(acStringLine,"RealMedia Player/mc",19) == 0 ) ||
					(rtspstrncasecmp(acStringLine,"RTPExPlayer",11) == 0 )	)
				{
#ifndef _SHARED_MEM
					//20091116 support connected UDP
					pClient->iFixedUDPSourcePort =1;
#endif
					TelnetShell_DbgPrint("UDP Fixed Port is used!\r\n");
				}
		    }	

			if( rtspstrncasecmp(acStringLine,"EricssonMobilePlatformsClient",29) == 0)
			{
			    pClient->iSymRTP ++;
#ifndef _SHARED_MEM
				//20091116 support connected UDP
				pClient->iFixedUDPSourcePort = 1;
#endif
				TelnetShell_DbgPrint("UDP Fixed Port is used!\r\n");
			}

        }
	    //ShengFh Symetric RTP  
#ifndef _SHARED_MEM
		//20091116 support connected UDP
        //turn on/off NAT solution 
		if( pClient->parent->iBehindNAT == TRUE )
		{
			pClient->iSymRTP ++;
			pClient->iFixedUDPSourcePort = 1;
		}
		else
			pClient->iFixedUDPSourcePort = 0;
#endif

#ifdef RTSPRTP_MULTICAST
		//check if multicast
		if( strstr(pClient->acRecvBuffer,"multicast") == NULL )
		{
			/*if( pClient->parent->iCurrUnicastNumber >= pClient->parent->iMaxUnicastNumber )
			{
				DbgLog((dfCONSOLE|dfINTERNAL,"Not enough bandwidth!\n"));
				RTSPServer_SendReply(453,0,pClient,pServer);
				RTSPServer_CleanBuffer(pClient);
				//2005/05/31closesocket(pClient->iSockfd);
				pClient->parent = pServer;
				RTSPServer_InitClient(pClient);
				return -1;
			}
			else*/
			{
    		    if( pClient->ulSessionID == 0)
    				pClient->parent->iCurrUnicastNumber++;
			}	
#endif
			if( RTSPServer_GetClientRTPPort(pClient->acRecvBuffer,&usRTPPort,&usRTCPPort) != 0 )
			{
				DbgLog((dfCONSOLE|dfINTERNAL,"ALERT: SETUP request missing port number setting."));
				RTSPServer_RemoveMessage(iHeaderLen + iMsgBodyLen,pClient);	/* remove remainder of message from in_buffer */
				RTSPServer_SendReply(461,0,pClient,pServer);
				RTSPServer_InitClient(pClient);
				return -1;
			}

			if( usRTCPPort == 0 )
			{
				usRTCPPort = usRTPPort + 1;
			}

			//connect UDP port to client side if not a symmetric RTP client
			memset(&sockaddr,0,sizeof(sockaddr));
#ifdef _INET6
			sockaddr.sin6_family = AF_INET6;
			memcpy(&sockaddr.sin6_addr, &pClient->tClientSckAddr.sin6_addr, sizeof(struct in6_addr));
#else
			sockaddr.sin_family = AF_INET;
			sockaddr.sin_addr.s_addr =pClient->ulClientAddress;
#endif
			//20120925 modified by Jimmy for ONVIF backchannel
			//20120724 modified by Jimmy for metadata
			for( i = 0 ; (iMediaType != SETUP_AUDIOBACK) && (i < MEDIA_TYPE_NUMBER) ; i++ )
			{
				if( pClient->usPort[i][0] == 0 )
				{ 
					pClient->usPort[i][0] = usRTPPort;
					pClient->usPort[i][1] = usRTCPPort;
					//printf("[%s] pClient->usPort[%d][0]=%d, pClient->usPort[%d][1]=%d!\n", __FUNCTION__, i, pClient->usPort[i][0], i, pClient->usPort[i][1]);
#ifndef _SHARED_MEM
					//20091116 support connected UDP
					if( pClient->iFixedUDPSourcePort == 0 )
					{
#endif
						if( RTSPServer_GetServerRTPport(i, ausServerPort, pClient, iMediaType) != 0 )
						{
							TelnetShell_DbgPrint("[RTSP server]: RTP sockets create error.\n");
							RTSPServer_RemoveMessage(iHeaderLen + iMsgBodyLen,pClient);	
							RTSPServer_SendReply(500,0,pClient,pServer);
							RTSPServer_InitClient(pClient);
							return -1;   
						}            

						//20091230 Fixed Audio only QoS value disorder issue
						if ( ausServerPort[0] == pClient->parent->rtsp_param.usRTPVPort )
						{
							RTSPServer_SetQosToSocket(pClient->rtp_sock[i][0], &pServer->tQosInfo, eQosVideoType);
							RTSPServer_SetQosToSocket(pClient->rtp_sock[i][1], &pServer->tQosInfo, eQosVideoType);
						}
						if ( ausServerPort[0] == pClient->parent->rtsp_param.usRTPAPort )
						{
							/* 20090225 QOS */
							RTSPServer_SetQosToSocket(pClient->rtp_sock[i][0], &pServer->tQosInfo, eQosAudioType);
							RTSPServer_SetQosToSocket(pClient->rtp_sock[i][1], &pServer->tQosInfo, eQosAudioType);
						}
						//20120726 added by Jimmy for metadata
						if ( ausServerPort[0] == pClient->parent->rtsp_param.usRTPMPort )
						{
							/* 20090225 QOS */
							RTSPServer_SetQosToSocket(pClient->rtp_sock[i][0], &pServer->tQosInfo, eQosMetadataType);
							RTSPServer_SetQosToSocket(pClient->rtp_sock[i][1], &pServer->tQosInfo, eQosMetadataType);
						}

#ifndef _SHARED_MEM
					}
#endif

					if(pClient->iSymRTP == 0)
					{
#ifdef _INET6
						sockaddr.sin6_port = htons(usRTPPort);
						//CID:66, CHECKER:UNCHECKED_CALL
						if(connect(pClient->rtp_sock[i][0],(struct sockaddr *)&sockaddr,sizeof(sockaddr)) < 0)
						{
							printf("Setup Request socket %d connect failed!\n", pClient->rtp_sock[i][0]);
						}
						else
						{
							printf("[%s] socket6 %d connect successfully!\n", __FUNCTION__, pClient->rtp_sock[i][0]);
						}
						sockaddr.sin6_port = htons(usRTCPPort);
						if(connect(pClient->rtp_sock[i][1],(struct sockaddr *)&sockaddr,sizeof(sockaddr)) < 0)
						{
							printf("Setup Request socket %d connect failed!\n", pClient->rtp_sock[i][1]);
						}
						else
						{
							printf("[%s] socket6 %d connect successfully!\n", __FUNCTION__, pClient->rtp_sock[i][1]);
						}
#else
						sockaddr.sin_port = htons(usRTPPort);
						if(connect(pClient->rtp_sock[i][0],(struct sockaddr *)&sockaddr,sizeof(sockaddr)) < 0)
						{
							printf("Setup Request socket %d connect failed!\n", pClient->rtp_sock[i][0]);
						}
						else
						{
							printf("[%s] socket %d connect successfully!\n", __FUNCTION__, pClient->rtp_sock[i][0]);
						}
						sockaddr.sin_port = htons(usRTCPPort);
						if(connect(pClient->rtp_sock[i][1],(struct sockaddr *)&sockaddr,sizeof(sockaddr)) < 0)
						{
							printf("Setup Request socket %d connect failed!\n", pClient->rtp_sock[i][1]);
						}
						else
						{
							printf("[%s] socket %d connect successfully!\n", __FUNCTION__, pClient->rtp_sock[i][1]);
						}
#endif
					}   
					break;
				}		
			}

			//20120925 added by Jimmy for ONVIF backchannel
			if( iMediaType == SETUP_AUDIOBACK )
			{

				if( pClient->parent->ulAudiobackSessionID[pClient->iChannelIndex-1] == 0 )
				{
					if( RTSPServer_GetServerRTPport(pClient->iChannelIndex-1, ausServerPort, pClient, iMediaType) != 0 )
					{
						TelnetShell_DbgPrint("[RTSP server]: RTP sockets create error.\n");
						RTSPServer_RemoveMessage(iHeaderLen + iMsgBodyLen,pClient); 
						RTSPServer_SendReply(500,0,pClient,pServer);
						RTSPServer_InitClient(pClient);
						return -1;	 
					}
					printf("[%s]usRTPPort = %d, usRTCPPort = %d\n",__FUNCTION__,usRTPPort,usRTCPPort);
#ifdef _INET6
					sockaddr.sin6_port = htons(usRTPPort);
					//CID:66, CHECKER:UNCHECKED_CALL
					if(connect(pClient->parent->iAudiobackSock[pClient->iChannelIndex-1][0],(struct sockaddr *)&sockaddr,sizeof(sockaddr)) < 0)
					{
						printf("Setup Request socket %d connect failed!\n", pClient->parent->iAudiobackSock[pClient->iChannelIndex-1][0]);
					}
					else
					{
						printf("[%s] socket %d connect successfully!\n", __FUNCTION__, pClient->parent->iAudiobackSock[pClient->iChannelIndex-1][0]);
					}
					sockaddr.sin6_port = htons(usRTCPPort);
					if(connect(pClient->parent->iAudiobackSock[pClient->iChannelIndex-1][1],(struct sockaddr *)&sockaddr,sizeof(sockaddr)) < 0)
					{
						printf("Setup Request socket %d connect failed!\n", pClient->parent->iAudiobackSock[pClient->iChannelIndex-1][1]);
					}
					else
					{
						printf("[%s] socket %d connect successfully!\n", __FUNCTION__, pClient->parent->iAudiobackSock[pClient->iChannelIndex-1][1]);
					}
#else
					sockaddr.sin_port = htons(usRTPPort);
					if(connect(pClient->parent->iAudiobackSock[pClient->iChannelIndex-1][0],(struct sockaddr *)&sockaddr,sizeof(sockaddr)) < 0)
					{
						printf("Setup Request socket %d connect failed!\n", pClient->parent->iAudiobackSock[pClient->iChannelIndex-1][0]);
					}
					else
					{
						printf("[%s] socket %d connect successfully!\n", __FUNCTION__, pClient->parent->iAudiobackSock[pClient->iChannelIndex-1][0]);
					}
					sockaddr.sin_port = htons(usRTCPPort);
					if(connect(pClient->parent->iAudiobackSock[pClient->iChannelIndex-1][1],(struct sockaddr *)&sockaddr,sizeof(sockaddr)) < 0)
					{
						printf("Setup Request socket %d connect failed!\n", pClient->parent->iAudiobackSock[pClient->iChannelIndex-1][1]);
					}
					else
					{
						printf("[%s] socket %d connect successfully!\n", __FUNCTION__, pClient->parent->iAudiobackSock[pClient->iChannelIndex-1][1]);
					}
#endif

					pClient->bAudioback = TRUE;

				}
				else
				{
					RTSPServer_DiscardMessage(pClient); 
					RTSPServer_SendReply(503, 0,pClient,pServer);
					RTSPServer_InitClient(pClient);
					return -1;
				}
			}

		   /* if( pClient->usPort[0][0] == 0 )
		    { 
			    pClient->usPort[0][0] = usRTPPort;
				pClient->usPort[0][1] = usRTCPPort;
        
				if( pClient->iFixedUDPSourcePort == 0 )
				{
					if( RTSPServer_GetServerRTPport(0,ausServerPort,pClient) != 0 )
					{
						DbgLog((dfCONSOLE|dfINTERNAL,"ALERT: RTP sockets create error.\n"));
						RTSPServer_RemoveMessage(iHeaderLen + iMsgBodyLen,pClient);	
						RTSPServer_SendReply(500,0,pClient,pServer);
						RTSPServer_CleanBuffer(pClient);
						return -1;   
					}            
				}

				if(pClient->iSymRTP == 0)
				{
					sockaddr.sin_port = htons(usRTPPort);
					connect(pClient->rtp_sock[0][0],(struct sockaddr *)&sockaddr,sizeof(sockaddr));
					sockaddr.sin_port = htons(usRTCPPort);
					connect(pClient->rtp_sock[0][1],(struct sockaddr *)&sockaddr,sizeof(sockaddr));
				}   
			}
			else
			{	 
				pClient->usPort[1][0] = usRTPPort;
				pClient->usPort[1][1] = usRTCPPort;
        
				if( pClient->iFixedUDPSourcePort == 0 )
				{
					if( RTSPServer_GetServerRTPport(1,ausServerPort,pClient) != 0)
					{
						DbgLog((dfCONSOLE|dfINTERNAL,"ALERT: RTP sockets create error.\n"));
						RTSPServer_RemoveMessage(iHeaderLen + iMsgBodyLen,pClient);	
						RTSPServer_SendReply(500,0,pClient,pServer);
						RTSPServer_CleanBuffer(pClient);
						return -1;   
					}
				}

				if(pClient->iSymRTP == 0)
				{
					sockaddr.sin_port = htons(usRTPPort);
					connect(pClient->rtp_sock[1][0],(struct sockaddr *)&sockaddr,sizeof(sockaddr));	
					sockaddr.sin_port = htons(usRTCPPort);
					connect(pClient->rtp_sock[1][1],(struct sockaddr *)&sockaddr,sizeof(sockaddr));
				}                   
		    }*/
#ifdef RTSPRTP_MULTICAST
		}
		else // if multicast
		{
			//20130830 added by Charles for ondemand multicast
			if(RTSPServer_ParseOndemandMulticastInfo(pClient, iMediaType, &bWithOnDemandMulticast) == -1)
			{
				RTSPServer_DiscardMessage(pClient); 
				RTSPServer_SendReply(400, 0,pClient,pClient->parent);
				RTSPServer_InitClient(pClient);
				return -1;

			}			

            
			//20120925 modified by Jimmy for ONVIF backchannel
			//20110630 Add by danny For Multicast enable/disable
			if( (iMediaType == SETUP_AUDIOBACK) || (pClient->parent->ulMulticastAddress[pClient->iSDPIndex - 1] == 0) || pClient->parent->iMulticastEnable[pClient->iSDPIndex - 1] == 0)
			{
				TelnetShell_DbgPrint("[RTSP server]: Multicast disable!\n");
				RTSPServer_SendReply(503,0,pClient,pServer);
				RTSPServer_CleanBuffer(pClient);
				pClient->parent = pServer;
				RTSPServer_InitClient(pClient);
				return -1;
			}
		
			TelnetShell_DbgPrint("Multicast setup from client\r\n");
					
			//20130924 modified by Charles for ondemand multicast
			if(pClient->bNewMulticast == TRUE)
			{
				if(bWithOnDemandMulticast == FALSE)
				{
					//without demand multicast info
					for( i=0 ; i<RTSP_MULTICASTNUMBER ; i++)
					{
						if( pClient->parent->iMulticastSDPIndex[i] == pClient->iSDPIndex)  
						{
							pClient->iMulticast = pClient->iSDPIndex;  //add to the original multicast group
							pClient->bNewMulticast = FALSE; 

							iIndex = pClient->iMulticast-1;
							pClient->parent->iCurrMulticastNumber[iIndex] ++;		
							TelnetShell_DbgPrint("current %d max %d\r\n",pClient->parent->iCurrMulticastNumber[iIndex],pClient->parent->iMaxMulticastNumber[iIndex]);

						}
					}
				}
				else
				{
					//with demand multicast info
					pClient->iMulticast = pClient->iSDPIndex;  //assign temp multicast group for send reply, maybe change later
					//RTSPServer_UpdateMulticastParameters(pClient->parent, (pClient->iSDPIndex+RTSP_MULTICASTNUMBER+RTSP_AUDIO_EXTRA_MULTICASTNUMBER-1), &(pClient->tMulticastInfo));
				}
			}
			
			

			/*if( pClient->iVivotekClient )
			{
			    // find the 1st multicast group with RTP extension
			    for( i=0 ; i<RTSP_MULTICASTNUMBER ; i ++)
			    {
			        if( pClient->parent->iRTPExtension[i] == TRUE )
			            break;
			    }
			    if( i == RTSP_MULTICASTNUMBER )
			        i = 0;
			        				
				{
	    		    if( pClient->ulSessionID == 0)
					    pClient->parent->iCurrMulticastNumber[i] ++;
					    
					pClient->iMulticast = i+1;
				}
			}
			else				
			{
			    // find the 1st multicast group without RTP extension
			    for( i=0 ; i<RTSP_MULTICASTNUMBER ; i ++)
			    {
			        if( pClient->parent->iRTPExtension[i] == FALSE )
			            break;
			    }
			    if( i == RTSP_MULTICASTNUMBER )
			        i = 0;
			    				
				{
	    		    if( pClient->ulSessionID == 0)
	    		    {
    					pClient->parent->iCurrMulticastNumber[i] ++;	
					    TelnetShell_DbgPrint("current %d max %d\r\n",pClient->parent->iCurrMulticastNumber[i],pClient->parent->iMaxMulticastNumber[i]);
    				}	
    					
					pClient->iMulticast = i+1;
				}				
			}*/
		}
#endif
	}

	if( pClient->ulSessionID == 0 )
    {
        pClient->ulSessionID = ulSessionID = RTSPServer_CheckIDCollision(pClient,RTSPServer_GetSessionID());
    }
    else
    {
		if ( RTSPServer_GetNumber(pClient->acRecvBuffer, iHeaderLen, "Session", ": \t",&ulSessionID, 0) != 0 )  
        {
            DbgLog((dfCONSOLE|dfINTERNAL,"ALERT:missing session ID in 2nd SETUP"));
            RTSPServer_RemoveMessage(iMsgBodyLen + iHeaderLen,pClient);
            RTSPServer_SendReply(459, 0,pClient,pServer);
            RTSPServer_InitClient(pClient);
            return -1;
        }        
        
        if( pClient->ulSessionID != ulSessionID )
        {
            DbgLog((dfCONSOLE|dfINTERNAL,"ALERT:session not found in 2nd SETUP %u ",ulSessionID));
            RTSPServer_RemoveMessage(iMsgBodyLen + iHeaderLen,pClient);
            RTSPServer_SendReply(454, 0,pClient,pServer);
            RTSPServer_InitClient(pClient);
            return -1;
        }
    }

    //20120925 added by Jimmy for ONVIF backchannel
    if( pClient->bAudioback == TRUE)
    {
        pClient->parent->ulAudiobackSessionID[pClient->iChannelIndex-1] = pClient->ulSessionID;
    }

    RTSPServer_RemoveMessage(iHeaderLen + iMsgBodyLen,pClient);		/* remove remainder of message from in_buffer */
    
#ifdef RTSPRTP_MULTICAST
	//20120925 modified by Jimmy for ONVIF backchannel
	if( (iMediaType!=SETUP_AUDIOBACK) && (pClient->iMulticast > 0) )
	{
		//pClient->iTimeOut = 0;
		OSTick_GetMSec(&pClient->dwBaseMSec);

		if( (RTSPServer_SendMulticastSetupReply(pClient,pClient->ulSessionID,iMediaType)) != 0 )
			return 1;
	    else
   			return 0;		
	}
	else
#endif
	{
		//pClient->iTimeOut = 0;
		OSTick_GetMSec(&pClient->dwBaseMSec);

#ifndef _SHARED_MEM
		//20091116 support connected UDP
		if( pClient->iFixedUDPSourcePort == 1)
		{
#endif
			//20120726 modified by Jimmy for metadata
			if( iMediaType == SETUP_VIDEO )
			{
				ausServerPort[0] = pClient->parent->rtsp_param.usRTPVPort;
				ausServerPort[1] = pClient->parent->rtsp_param.usRTCPVPort;
			}
			//20120925 modified by Jimmy for ONVIF backchannel
			else if( (iMediaType == SETUP_AUDIO) || (iMediaType == SETUP_AUDIOBACK) )
			{
				ausServerPort[0] = pClient->parent->rtsp_param.usRTPAPort;
				ausServerPort[1] = pClient->parent->rtsp_param.usRTCPAPort;
			}
			else if( iMediaType == SETUP_METADATA)
			{
				ausServerPort[0] = pClient->parent->rtsp_param.usRTPMPort;
				ausServerPort[1] = pClient->parent->rtsp_param.usRTCPMPort;
			}

#ifndef _SHARED_MEM
		}
#endif

		if( (RTSPServer_SendSetupReply(pClient,usRTPPort,usRTCPPort,ausServerPort,pServer,pClient->ulSessionID)) != 0 )
			return 1;
	    else
		{        
#ifdef RTSP_DEBUG_LOG        
			ADD_LOG("SetUp reply sent.\n");
#endif        
        
			return 0;
		}
	}
}

int RTSPServer_HandlePlayRequest(char *b,RTSP_CLIENT *pClient,RTSP_SERVER* pServer)
{
    int             iHeaderLen;		/* message header length */
    int             iMsgBodyLen;	/* message body length */
    int             i;
    unsigned long   ulSessionID;
    unsigned short  usRTSPPort;
#ifdef _SHARED_MEM
	//20110915 Modify by danny for support Genetec MOD
	//char			acTemp[64];
#endif
    RTSPSERVER_SESSIONINFORMATION stSessionInfo;
    memset(&stSessionInfo, 0, sizeof(RTSPSERVER_SESSIONINFORMATION));
    
    RTSPServer_GetMessageLen(&iHeaderLen, &iMsgBodyLen,pClient);

	TelnetShell_DbgPrint("%s\r\n",b);
    if (!sscanf(b, "%*s %254s",pClient->acObjURL))
    {
        DbgLog((dfCONSOLE|dfINTERNAL,"PLAY request is missing object (path/file) parameter.\n"));
        RTSPServer_RemoveMessage(iHeaderLen + iMsgBodyLen,pClient);	
        RTSPServer_SendReply(400, 0,pClient,pServer);	
        RTSPServer_InitClient(pClient);
        return -1;
    }
    
    //if the last char is '/', get rid of it    
    if( pClient->acObjURL[strlen(pClient->acObjURL)-1] == '/' )
        pClient->acObjURL[strlen(pClient->acObjURL)-1] = 0;

    /* Modified for Shmem */
    if (!RTSPServer_ParseURL(pClient->acObjURL, pClient, &usRTSPPort, ePlayMethod)) 
    {
        RTSPServer_DiscardMessage(pClient);		
        RTSPServer_SendReply(400, 0,pClient,pServer);		
        RTSPServer_InitClient(pClient);
        return -1;
    }

	if( pClient->parent->fcallback(pClient->parent->hParentHandle
								,RTSPSERVER_CALLBACKFLAG_CHECK_ACCESSNAME
								,(void*)pClient->acObject,(void*)pClient->iOrigSDPIndex) != 0 )
	{
		TelnetShell_DbgPrint("Mangled URL in PLAY %s\r\n", pClient->acObject);        
        RTSPServer_SendReply(400, 0,pClient,pServer);		
        RTSPServer_InitClient(pClient);
        return -1;
	}

	if ( RTSPServer_GetNumber(pClient->acRecvBuffer, iHeaderLen, "Session", ": \t",&ulSessionID, 0) != 0 )  
    {
        DbgLog((dfCONSOLE|dfINTERNAL,"ALERT: PLAY request missing Session setting. %u ",ulSessionID));
        RTSPServer_RemoveMessage(iMsgBodyLen + iHeaderLen,pClient);
        RTSPServer_SendReply(454, 0,pClient,pServer);
        RTSPServer_InitClient(pClient);
        return -1;
    }
   
    if( ulSessionID != pClient->ulSessionID )
    {
        DbgLog((dfCONSOLE|dfINTERNAL,"ALERT:session not found in PLAY %u ",ulSessionID));
        RTSPServer_RemoveMessage(iMsgBodyLen + iHeaderLen,pClient);
        RTSPServer_SendReply(454, 0,pClient,pServer);
        RTSPServer_InitClient(pClient);
        return -1;       
    }

#ifdef _SHARED_MEM
	//20100428 Added For Media on demand
	if ( pClient->bMediaOnDemand == TRUE )
	{
		//20110915 Modify by danny for support Genetec MOD
		if(RTSPServer_GetString(pClient->acRecvBuffer, iHeaderLen, "Scale", ": \t", 
								pClient->tMODInfo.acMODSetCommandValue[MOD_PLAYSCALE], sizeof(pClient->tMODInfo.acMODSetCommandValue[MOD_PLAYSCALE])) == 0 )  
		{
			pClient->tMODInfo.eMODSetCommandType[MOD_PLAYSCALE] = MOD_PLAYSCALE;
		}

		if(RTSPServer_GetString(pClient->acRecvBuffer, iHeaderLen, "Speed", ": \t", 
								pClient->tMODInfo.acMODSetCommandValue[MOD_PLAYSPEED], sizeof(pClient->tMODInfo.acMODSetCommandValue[MOD_PLAYSPEED])) == 0 )  
		{
			pClient->tMODInfo.eMODSetCommandType[MOD_PLAYSPEED] = MOD_PLAYSPEED;
		}

		if(RTSPServer_GetString(pClient->acRecvBuffer, iHeaderLen, "Range", ": \t", 
								pClient->tMODInfo.acMODSetCommandValue[MOD_RANGE], sizeof(pClient->tMODInfo.acMODSetCommandValue[MOD_RANGE])) == 0 )  
		{
			pClient->tMODInfo.eMODSetCommandType[MOD_RANGE] = MOD_RANGE;
		}
        //20141110 added by Charles for ONVIF Profile G
        if(RTSPServer_GetString(pClient->acRecvBuffer, iHeaderLen, "Rate-Control", ": \t", 
								pClient->tMODInfo.acMODSetCommandValue[MOD_RATECONTROL], sizeof(pClient->tMODInfo.acMODSetCommandValue[MOD_RATECONTROL])) == 0 )  
		{
			pClient->tMODInfo.eMODSetCommandType[MOD_RATECONTROL] = MOD_RATECONTROL;
		}

        if(RTSPServer_GetString(pClient->acRecvBuffer, iHeaderLen, "Immediate", ": \t", 
								pClient->tMODInfo.acMODSetCommandValue[MOD_IMMEDIATE], sizeof(pClient->tMODInfo.acMODSetCommandValue[MOD_IMMEDIATE])) == 0 )  
		{
			pClient->tMODInfo.eMODSetCommandType[MOD_IMMEDIATE] = MOD_IMMEDIATE;
		}
		//20151207 added by faber, support onvif header field of frame, and now we workaround with play scale
		char acFrameType[255];
		memset(acFrameType, 0, sizeof(acFrameType));
		if(RTSPServer_GetString(pClient->acRecvBuffer, iHeaderLen, "Frames", ": \t", acFrameType, sizeof(acFrameType) ) == 0)  
		{
			if(strstr(acFrameType, "intra") ) //20160623 Modify by Faber, for new interface syncframeonly
			{
				printf("intra frame\n");
				pClient->tMODInfo.eMODSetCommandType[MOD_SYNCFRAMEONLY] = MOD_SYNCFRAMEONLY;
			}
		}
	}
#endif

	//20120725 added by Jimmy for metadata
	for(i=0;i<MEDIA_TYPE_NUMBER;i++)
	{
		if(pClient->acMediaType[i][0] != 0)
		{
			if(pClient->ulInitTimestamp[i] == 0 && pClient->usInitSequence[i] == 0 )		
			{
				pClient->ulInitTimestamp[i] = 0;
				pClient->usInitSequence[i] = 0;
			}	
			if( pClient->ulSSRC[i] == 0 ) 
				pClient->ulSSRC[i] = RTSPServer_GetSSRC();
		}
	}

	//20120925 added by Jimmy for ONVIF backchannel
	if( pClient->bAudioback == TRUE )
	{
		OSTime_GetTimer(&pClient->parent->dwLastRecvTimeofAudioback[pClient->iChannelIndex-1], NULL);
	}

/*
    if( pClient->acMediaType[1][0] == 0 )
    {
//        pClient->ulInitTimestamp[0] = RTSPServer_GetInitTimestamp();
//        pClient->usInitSequence[0] = RTSPServer_GetInitSequence();
        if(pClient->ulInitTimestamp[0] == 0 && pClient->usInitSequence[0] == 0 )        
        {
		    pClient->ulInitTimestamp[0] = 0;
            pClient->usInitSequence[0] = 0;
        } 
          
        if(  pClient->ulSSRC[0] == 0 ) 
            pClient->ulSSRC[0] = RTSPServer_GetSSRC();                
    }
    else
    {
        for(i=0;i<2;i++)
        {
//            pClient->ulInitTimestamp[i] = RTSPServer_GetInitTimestamp();
//            pClient->usInitSequence[i] = RTSPServer_GetInitSequence();
            if(pClient->ulInitTimestamp[i] == 0 && pClient->usInitSequence[i] == 0 )        
            {
    			pClient->ulInitTimestamp[i] = 0;
            	pClient->usInitSequence[i] = 0;
            }	
        	if( pClient->ulSSRC[i] == 0 ) 
                pClient->ulSSRC[i] = RTSPServer_GetSSRC();
        }
    }
*/
    //20141110 added by Charles for ONVIF Profile G
    if(pClient->bMediaOnDemand && pClient->iStatus == PLAY_STATE)
    {
        stSessionInfo.iMulticast= pClient->iMulticast;
        stSessionInfo.dwSessionID = pClient->ulSessionID;
        stSessionInfo.iCseq = pClient->iCSeq;
        pClient->parent->fcallback(pClient->parent->hParentHandle, RTSPSERVER_CALLBACKFLAG_SET_PLAY_CSEQ, (void*)&stSessionInfo, 0);    
    }
    
	printf("%s\r\n",pClient->acRecvBuffer);

	//printf("recv len %d %d bytes left\r\n",pClient->iRecvSize,strlen(pClient->acRecvBuffer));

    RTSPServer_DiscardMessage(pClient);		
	//printf("recv len %d bytes left\r\n",pClient->iRecvSize);

  
    if( pClient->iPlayerType == REAL_MEDIA_PLAYER )
    {
        //OSSleep_MSec(200);
        //RTSPServer_HandleProbeRTPPacket(pClient);
    }
	
    if( ( RTSPServer_SendPlayReply(pClient,pServer) ) != 0 )
    {
        return -1;
    }
#ifdef _SHARED_MEM
	//20081219 for time-shift play time adjust!
	if(pClient->dwDescribeMSec != 0)
	{
		DWORD	dwNow = 0, dwOffset = 0;

		OSTick_GetMSec(&dwNow);
		dwOffset = rtspCheckTimeDifference(pClient->dwDescribeMSec, dwNow);
		if(dwOffset < RTSPPARSER_MAX_OFFSET_ADJUST * 1000)
		{
			pClient->dwBaseMSec += dwOffset;
		}
	}
#endif
#ifdef  RTSP_DEBUG_LOG
    ADD_LOG("Play reply sent.\n");        
#endif    
           
    return 0;
}

int RTSPServer_HandlePauseRequest(RTSP_CLIENT *pClient,RTSP_SERVER* pServer)
{
    unsigned long   ulSessionID;
    int             iHeaderLen;		/* message header length */
    int             iMsgBodyLen;	/* message body length */
#ifdef _SHARED_MEM
	//20100428 Added For Media on demand
	RTSPSERVER_MODREQUEST	stMODRequest;
	int				iLength = 0;
	memset(&stMODRequest,0,sizeof(RTSPSERVER_MODREQUEST));
#endif

    printf("Got PAUSE command!\n");
    RTSPServer_GetMessageLen(&iHeaderLen, &iMsgBodyLen,pClient);
    
	if ( RTSPServer_GetNumber(pClient->acRecvBuffer, iHeaderLen, "Session", ": \t",&ulSessionID, 0) != 0 )  
    {
        DbgLog((dfCONSOLE|dfINTERNAL,"ALERT: PAUSE request missing Session setting. %u ",ulSessionID));
        RTSPServer_RemoveMessage(iMsgBodyLen + iHeaderLen,pClient);
        RTSPServer_SendReply(454, 0,pClient,pServer);
        RTSPServer_CleanBuffer(pClient);
        return -1;
    }

    if( ulSessionID != pClient->ulSessionID )
    {
        DbgLog((dfCONSOLE|dfINTERNAL,"ALERT:session not found in PAUSE %u ",ulSessionID));
        RTSPServer_RemoveMessage(iMsgBodyLen + iHeaderLen,pClient);
        RTSPServer_SendReply(454, 0,pClient,pServer);
        RTSPServer_CleanBuffer(pClient);
        return -1;                
    }

#ifdef _SHARED_MEM
	//20100428 Added For Media on demand
	if ( pClient->bMediaOnDemand == TRUE )
	{	
		stMODRequest.eMODSetCommandType = MOD_PAUSE;
		stMODRequest.iSDPIndex = pClient->iSDPIndex;
		iLength = snprintf(stMODRequest.acMODSetCommandValue, sizeof(stMODRequest.acMODSetCommandValue) - 1, "1");
		stMODRequest.acMODSetCommandValue[iLength] = '\0';
		//pServer->fcallback(pServer->hParentHandle, RTSPSERVER_CALLBACKFLAG_SET_MODCONTROLINFO, (void*)&stMODRequest, (void*)RTSPMOD_REQUEST_NOWAIT);
		pServer->fcallback(pServer->hParentHandle, RTSPSERVER_CALLBACKFLAG_SET_MODCONTROLINFO, (void*)&stMODRequest, 0);
	}
#endif

    if( !RTSPServer_SendReply(200, 0,pClient,pServer) )
    {
        DbgLog((dfCONSOLE|dfINTERNAL,"PAUSE response sent.\n"));  
    }
	else	//20090407 return fail to ensure client does not enter pause state
	{
		return -1;
	}
    
    RTSPServer_DiscardMessage(pClient);		
        
    return 0;

}

int RTSPServer_HandleResumeRequest(RTSP_CLIENT *pClient,RTSP_SERVER* pServer)
{
    unsigned long   ulSessionID;
    int             iHeaderLen;		/* message header length */
    int             iMsgBodyLen;	/* message body length */
	RTSPSERVER_SESSIONINFORMATION stSessionInfo;
    //int				iVideo;
	//20120809 added by Jimmy for metadata
    int				iTrackMediaType, i;
#ifdef _SHARED_MEM
	//20100428 Added For Media on demand
	int				iLength = 0;
	//20110915 Modify by danny for support Genetec MOD
	//char			acTemp[64];
#endif

    RTSPServer_GetMessageLen(&iHeaderLen, &iMsgBodyLen,pClient);
    
	if ( RTSPServer_GetNumber(pClient->acRecvBuffer, iHeaderLen, "Session", ": \t",&ulSessionID, 0) != 0 )  
    {
        DbgLog((dfCONSOLE|dfINTERNAL,"ALERT: Resume request missing Session setting. %u ",ulSessionID));
        RTSPServer_RemoveMessage(iMsgBodyLen + iHeaderLen,pClient);
        RTSPServer_SendReply(454, 0,pClient,pServer);
        RTSPServer_CleanBuffer(pClient);
        return -1;
    }

    if( ulSessionID != pClient->ulSessionID )
    {
        DbgLog((dfCONSOLE|dfINTERNAL,"ALERT:session not found in PLAY %u ",ulSessionID));
        RTSPServer_RemoveMessage(iMsgBodyLen + iHeaderLen,pClient);
        RTSPServer_SendReply(454, 0,pClient,pServer);
        RTSPServer_CleanBuffer(pClient);
        return -1;                
    }
    
    stSessionInfo.dwSessionID = ulSessionID;
    
    //acquire the timestamp and seq# before PAUSE
    RTSPServer_UpdateRTPSession(pServer,&stSessionInfo);

    //20141110 added by Charles for ONVIF Profile G
    if(pClient->bMediaOnDemand)
    {
        stSessionInfo.iMulticast = pClient->iMulticast;
        stSessionInfo.iCseq = pClient->iCSeq;
        pClient->parent->fcallback(pClient->parent->hParentHandle, RTSPSERVER_CALLBACKFLAG_SET_PLAY_CSEQ, (void*)&stSessionInfo, 0);	
    }

	//20120801 modified by Jimmy for metadata
    /*
	iVideo = pClient->parent->fcallback(pClient->parent->hParentHandle
										,RTSPSERVER_CALLBACKFLAG_CHECK_VIDEO_TRACK
										,(void*)pClient->iSDPIndex,pClient->acMediaType[0]);	

    //if( strcmp((char*)&(pClient->acMediaType[0]),RTSP_VIDEO_ACCESS_NAME1) == 0 )
	if( iVideo == TRUE )
    {
	    pClient->ulInitTimestamp[1] = stSessionInfo.dwInitialTimeStamp[1];	
    	pClient->usInitSequence[1] = stSessionInfo.wInitialSequenceNumber[1];
    	pClient->ulInitTimestamp[0] = stSessionInfo.dwInitialTimeStamp[0];	
    	pClient->usInitSequence[0] = stSessionInfo.wInitialSequenceNumber[0];
    }
    else
    {
    	pClient->ulInitTimestamp[0] = stSessionInfo.dwInitialTimeStamp[1];	
    	pClient->usInitSequence[0] = stSessionInfo.wInitialSequenceNumber[1];
    	pClient->ulInitTimestamp[1] = stSessionInfo.dwInitialTimeStamp[0];	
    	pClient->usInitSequence[1] = stSessionInfo.wInitialSequenceNumber[0];
    }
    */

	for ( i = 0; i < MEDIA_TYPE_NUMBER; i++)
	{
		if( pClient->acMediaType[i][0] == 0 )
			break;

		iTrackMediaType = pClient->parent->fcallback(pClient->parent->hParentHandle
													,RTSPSERVER_CALLBACKFLAG_CHECK_TRACK_MEDIATYPE
													,(void*)pClient->iSDPIndex,pClient->acMediaType[i]);

		if( iTrackMediaType == RTSPSERVER_MEDIATYPE_VIDEO)
		{
			pClient->ulInitTimestamp[i] = stSessionInfo.dwInitialTimeStamp[0];	
			pClient->usInitSequence[i] = stSessionInfo.wInitialSequenceNumber[0];
		}
		else if( iTrackMediaType == RTSPSERVER_MEDIATYPE_AUDIO)
		{
			pClient->ulInitTimestamp[i] = stSessionInfo.dwInitialTimeStamp[1];	
			pClient->usInitSequence[i] = stSessionInfo.wInitialSequenceNumber[1];
		}
#ifdef _METADATA_ENABLE
		else if( iTrackMediaType == RTSPSERVER_MEDIATYPE_METADATA)
		{
			pClient->ulInitTimestamp[i] = stSessionInfo.dwInitialTimeStamp[2];
			pClient->usInitSequence[i] = stSessionInfo.wInitialSequenceNumber[2];
		}
#endif

	}	

#ifdef _SHARED_MEM
    //20100428 Added For Media on demand
	if ( pClient->bMediaOnDemand == TRUE )
	{
		//20110915 Modify by danny for support Genetec MOD
		if(RTSPServer_GetString(pClient->acRecvBuffer, iHeaderLen, "Scale", ": \t", 
								pClient->tMODInfo.acMODSetCommandValue[MOD_PLAYSCALE], sizeof(pClient->tMODInfo.acMODSetCommandValue[MOD_PLAYSCALE])) == 0 )  
		{
			pClient->tMODInfo.eMODSetCommandType[MOD_PLAYSCALE] = MOD_PLAYSCALE;
		}

		if(RTSPServer_GetString(pClient->acRecvBuffer, iHeaderLen, "Speed", ": \t", 
								pClient->tMODInfo.acMODSetCommandValue[MOD_PLAYSPEED], sizeof(pClient->tMODInfo.acMODSetCommandValue[MOD_PLAYSPEED])) == 0 )  
		{
			pClient->tMODInfo.eMODSetCommandType[MOD_PLAYSPEED] = MOD_PLAYSPEED;
		}

		if(RTSPServer_GetString(pClient->acRecvBuffer, iHeaderLen, "Range", ": \t", 
								pClient->tMODInfo.acMODSetCommandValue[MOD_RANGE], sizeof(pClient->tMODInfo.acMODSetCommandValue[MOD_RANGE])) == 0 )  
		{
			pClient->tMODInfo.eMODSetCommandType[MOD_RANGE] = MOD_RANGE;
		}
        //20141110 added by Charles for ONVIF Profile G
        if(RTSPServer_GetString(pClient->acRecvBuffer, iHeaderLen, "Rate-Control", ": \t", 
								pClient->tMODInfo.acMODSetCommandValue[MOD_RATECONTROL], sizeof(pClient->tMODInfo.acMODSetCommandValue[MOD_RATECONTROL])) == 0 )  
		{
			pClient->tMODInfo.eMODSetCommandType[MOD_RATECONTROL] = MOD_RATECONTROL;
		}

        if(RTSPServer_GetString(pClient->acRecvBuffer, iHeaderLen, "Immediate", ": \t", 
								pClient->tMODInfo.acMODSetCommandValue[MOD_IMMEDIATE], sizeof(pClient->tMODInfo.acMODSetCommandValue[MOD_IMMEDIATE])) == 0 )  
		{
			pClient->tMODInfo.eMODSetCommandType[MOD_IMMEDIATE] = MOD_IMMEDIATE;
		}

		pClient->tMODInfo.eMODSetCommandType[MOD_RESUME] = MOD_RESUME;
		iLength = snprintf(pClient->tMODInfo.acMODSetCommandValue[MOD_RESUME], sizeof(pClient->tMODInfo.acMODSetCommandValue[MOD_RESUME]) - 1, "0");
		pClient->tMODInfo.acMODSetCommandValue[MOD_RESUME][iLength] = '\0';
	}
#endif

    if( ( RTSPServer_SendPlayReply(pClient,pServer) ) != 0 )
    {	
    	return -1;  
    }
    DbgLog((dfCONSOLE|dfINTERNAL,"PLAY response sent.\n"));
    RTSPServer_DiscardMessage(pClient);		
            
    return 0;

}

void RTSPServer_HandleOptionsRequest(RTSP_CLIENT *pClient)
{
	int		iHeaderLen,iMsgBodyLen,iRet;
	char	acStringLine[64], *pcLoc = NULL;

	RTSPServer_GetMessageLen(&iHeaderLen, &iMsgBodyLen, pClient);	/* set header and message body length */ 

	/* 20100319 OPTION is used as keep-alive */
	//20131105 added by Charles for live555 proxyserver keepalive
	OSTick_GetMSec(&pClient->dwBaseMSec); 
	
	if(pClient->iStatus == PLAY_STATE)
	{
		pClient->parent->fcallback(pClient->parent->hParentHandle ,RTSPSERVER_CALLBACKFLAG_KEEP_ALIVE ,(void*)pClient->iSDPIndex, (void *)pClient->ulSessionID);	
	}

    if( (iRet= RTSPServer_GetString(pClient->acRecvBuffer,iHeaderLen, "Supported", ": \t",acStringLine, sizeof(acStringLine))) == 0 )
	{
		if( strstr(acStringLine,"force-i-frame") != NULL )
		{
			pClient->parent->fcallback(pClient->parent->hParentHandle
										,RTSPSERVER_CALLBACKFLAG_FORCE_I_FRAME
										,(void*)pClient->iSDPIndex,0);	
		}

		if( (pcLoc = strstr(acStringLine, "markframelevel")) != NULL)
		{

		    /* SVC level mapping
			Level 0 : I frame only
			Level 1 : eSVCMarkFrameLayer0
			Level 2 : eSVCMarkFrameLayer1
							   .
							   .
			Level 8 : All frames, set eSVCNull as value */
							  
			if(strncmp(acStringLine, RTSPPARSER_SVC_MODE_MARKFRAME_ONLY_KEYWORD, strlen(RTSPPARSER_SVC_MODE_MARKFRAME_ONLY_KEYWORD)) == 0)
			{
				pClient->eSVCMode = eSVCMarkFrameOnly;
			}
			else if(strncmp(acStringLine, RTSPPARSER_SVC_MODE_MARKFRAME_LEVEL1_KEYWORD, strlen(RTSPPARSER_SVC_MODE_MARKFRAME_LEVEL1_KEYWORD)) == 0)
			{
				pClient->eSVCMode = eSVCMarkFrameLevel1;
			}			
			else if(strncmp(acStringLine, RTSPPARSER_SVC_MODE_MARKFRAME_LEVEL2_KEYWORD, strlen(RTSPPARSER_SVC_MODE_MARKFRAME_LEVEL2_KEYWORD)) == 0)
			{
				pClient->eSVCMode = eSVCMarkFrameLevel2;
			}	
			else if(strncmp(acStringLine, RTSPPARSER_SVC_MODE_MARKFRAME_LEVEL3_KEYWORD, strlen(RTSPPARSER_SVC_MODE_MARKFRAME_LEVEL3_KEYWORD)) == 0)
			{
				pClient->eSVCMode = eSVCMarkFrameLevel3;
			}	
			else if(strncmp(acStringLine, RTSPPARSER_SVC_MODE_MARKFRAME_LEVEL4_KEYWORD, strlen(RTSPPARSER_SVC_MODE_MARKFRAME_LEVEL4_KEYWORD)) == 0)
			{
				pClient->eSVCMode = eSVCMarkFrameLevel4;
			}	
			else if(strncmp(acStringLine, RTSPPARSER_SVC_MODE_MARKFRAME_LEVEL5_KEYWORD, strlen(RTSPPARSER_SVC_MODE_MARKFRAME_LEVEL5_KEYWORD)) == 0)
			{
				pClient->eSVCMode = eSVCMarkFrameLevel5;
			}	
			else if(strncmp(acStringLine, RTSPPARSER_SVC_MODE_MARKFRAME_LEVEL6_KEYWORD, strlen(RTSPPARSER_SVC_MODE_MARKFRAME_LEVEL6_KEYWORD)) == 0)
			{
				pClient->eSVCMode = eSVCMarkFrameLevel6;
			}	
			else if(strncmp(acStringLine, RTSPPARSER_SVC_MODE_MARKFRAME_LEVEL7_KEYWORD, strlen(RTSPPARSER_SVC_MODE_MARKFRAME_LEVEL7_KEYWORD)) == 0)
			{
				pClient->eSVCMode = eSVCMarkFrameLevel7;
			}	
			else if(strncmp(acStringLine, RTSPPARSER_SVC_MODE_MARKFRAME_LAYERALL_KEYWORD, strlen(RTSPPARSER_SVC_MODE_MARKFRAME_LAYERALL_KEYWORD)) == 0)
			{
				pClient->eSVCMode = eSVCNull;
			}	
	
			//printf("SVC level %d\n", pClient->eSVCMode);
			pClient->parent->fcallback(pClient->parent->hParentHandle, RTSPSERVER_CALLBACKFLAG_SET_SVCLEVEL, (void*)pClient->ulSessionID, (void*)pClient->eSVCMode);	
		}
	}

	return;
}

void RTSPServer_HandleSetParameterRequest(RTSP_CLIENT *pClient)
{
	int		iHeaderLen,iMsgBodyLen,iRet;
	char	acStringLine[50];

	RTSPServer_GetMessageLen(&iHeaderLen, &iMsgBodyLen, pClient);	/* set header and message body length */ 

    if( (iRet= RTSPServer_GetString(pClient->acRecvBuffer + iHeaderLen, iMsgBodyLen, "sendkeyframe", ": \t",acStringLine, sizeof(acStringLine))) == 0 )
	{	
		if( strstr(acStringLine,"true") != 0 )
		{
			TelnetShell_DbgPrint("SET_PARAMETER Force I recevied for SDPIndex %d\r\n", pClient->iSDPIndex);

			pClient->parent->fcallback(pClient->parent->hParentHandle
										,RTSPSERVER_CALLBACKFLAG_FORCE_I_FRAME
										,(void*)pClient->iSDPIndex,0);	
		}
	}

	return;
}

//20101006 Added by danny for support GET_PARAMETER command for keep-alive in RTSP signaling
void RTSPServer_HandleGetParameterRequest(RTSP_CLIENT *pClient)
{
	int		iHeaderLen,iMsgBodyLen;

	RTSPServer_GetMessageLen(&iHeaderLen, &iMsgBodyLen, pClient);	/* set header and message body length */ 

	/* GET_PARAMETER is used as keep-alive */
	if(pClient->iStatus == PLAY_STATE)
	{
		pClient->parent->fcallback(pClient->parent->hParentHandle ,RTSPSERVER_CALLBACKFLAG_KEEP_ALIVE ,(void*)pClient->iSDPIndex, (void *)pClient->ulSessionID);	
	}

	return;
}

int RTSPServer_HandleTeardownRequest(RTSP_CLIENT *pClient,RTSP_SERVER* pServer)
{
    unsigned long   ulSessionID;
    int             iHeaderLen;		/* message header length */
    int             iMsgBodyLen;	/* message body length */
    
    RTSPServer_GetMessageLen(&iHeaderLen, &iMsgBodyLen,pClient);
    
	if ( RTSPServer_GetNumber(pClient->acRecvBuffer, iHeaderLen, "Session", ": \t",&ulSessionID, 0) != 0 )  
    {
        DbgLog((dfCONSOLE|dfINTERNAL,"ALERT: PLAY request missing Session setting. %u ",ulSessionID));
        RTSPServer_RemoveMessage(iMsgBodyLen + iHeaderLen,pClient);
        RTSPServer_SendReply(454, 0,pClient,pServer);
        RTSPServer_CleanBuffer(pClient);
        return -1;
    }

    if( ulSessionID != pClient->ulSessionID )
    {
        DbgLog((dfCONSOLE|dfINTERNAL,"ALERT:session not found in teardown %u ",ulSessionID));
        RTSPServer_RemoveMessage(iMsgBodyLen + iHeaderLen,pClient);
        RTSPServer_SendReply(454, 0,pClient,pServer);
        RTSPServer_CleanBuffer(pClient);
        return -1;                
    }

#ifdef _SHARED_MEM
	//20100105 Added For Seamless Recording
	if( pClient->bSeamlessStream == TRUE )
	{
		pClient->bNormalDisconnected = TRUE;
	}
#endif
	
	if( pClient->iRTPStreamingMode == RTP_OVER_UDP && 
		pClient->iMulticast == 0 )
	{
		// callback to release port instead of 200 OK
		if( RTSPServer_SessionStop(pClient->parent, pClient->ulSessionID,pClient->iMulticast) != 0 )	 
		{
			RTSPServer_SendReply(200, 0,pClient,pServer);
	 		RTSPServer_DiscardMessage(pClient);		
			pClient->parent = pServer;
			RTSPServer_InitClient(pClient);
			TelnetShell_DbgPrint("Reply Teardown 200 OK in SessionStop failure\r\n");
		}
		else
		{
			pClient->iStatus = TEARDOWN_STATE;        
			//pClient->iTimeOut = 0;
			OSTick_GetMSec(&pClient->dwBaseMSec);
			RTSPServer_RemoveMessage(iHeaderLen + iMsgBodyLen,pClient);		
		}
	}
	else
	{
		// reply 200 OK whithout port releasing
		printf("====TEARDOWN the session!========\n");

		if( !RTSPServer_SendReply(200, 0,pClient,pServer) )
		{
			printf("TEARDOWN response sent.\n");  
		}
    
	    RTSPServer_DiscardMessage(pClient);		

		//2007/11/05 modified by Louis
		if(RTSPServer_SessionStop(pClient->parent, pClient->ulSessionID,pClient->iMulticast) != 0)
		{
			printf("SessionStop Callback from RTSP server failed!\n");
		} 
        
		pClient->parent = pServer;
		printf("%d\n", __LINE__);
		// RTSPServer_InitClient(pClient);
        
	}

    return 0;

}


/*int RTSPServer_HandleTeardownRequest(RTSP_CLIENT *pClient,RTSP_SERVER* pServer)
{


    RTSPServer_DiscardMessage(pClient);		
    
    if( !RTSPServer_SendReply(200, 0,pClient,pServer) )
    {
        RTSPServer_SessionStop(pClient->parent, pClient->ulSessionID);	 
        closesocket(pClient->iSockfd);
        pClient->parent = pServer;
        RTSPServer_InitClient(pClient);
    }

    return (1);

}
*/
/***************************************
 *       server state machine
 **************************************/
void RTSPServer_HandleEvent(int iEvent, char *pcFirstLine, RTSP_CLIENT* pClient,RTSP_SERVER* pServer)
{
    //static char  *sStateErrMsg = "State error: event %d in state %d\n";
    RTSPSERVER_SESSIONINFORMATION session_info;

    memset((void*)&session_info,0,sizeof(session_info));
  /*
   * All state transitions are made here except when the last stream packet
   * is sent during a PLAY.  That transition is located in stream_event().
   */
    switch (pClient->iStatus)
    {
        case INIT_STATE:
        {
            switch (iEvent)
            {
                case RTSP_DESCRIBE_METHOD:
                    RTSPServer_HandleDescribeRequest(pcFirstLine,pClient,pServer);
                    break;

                case RTSP_SETUP_METHOD:
                    if(RTSPServer_HandleSetupRequest(pcFirstLine,pClient,pServer) == 0)
                    {
                         pClient->iStatus = SETUP_STATE;
                    } 
                    break;

				case RTSP_OPTIONS_METHOD:
					RTSPServer_HandleOptionsRequest(pClient);
					RTSPServer_SendOptionsReply(pClient,pServer);                    
					break;
					
                case RTSP_SETPARAM_METHOD:
					RTSPServer_SendSetParameterReply(pClient,pServer);                    
					break;

				//20101006 Added by danny for support GET_PARAMETER command for keep-alive in RTSP signaling
				case RTSP_GETPARAM_METHOD:
					RTSPServer_SendGetParameterReply(pClient,pServer);                    
					break;

                case RTSP_PLAY_METHOD:	/* method not valid this state. */
                case RTSP_PAUSE_METHOD:
                case RTSP_TEARDOWN_METHOD:
					RTSPServer_DiscardMessage(pClient);
                    RTSPServer_SendReply(455, "Accept: DESCRIBE\n",pClient,pServer);
                    break;

                default:
                    //DbgLog((dfCONSOLE|dfINTERNAL,sStateErrMsg, iEvent, pClient->iStatus));
                    //RTSPServer_SendReply(200, "Accept: OPTIONS\n",client);
                    RTSPServer_DiscardMessage(pClient);		/* remove remainder of message from in_buffer */
                    RTSPServer_SendReply(501, "Accept: DESCRIBE SETUP\n",pClient,pServer);
                    break;
            }
            break;
        }/* INIT state */
		
        case SETUP_STATE:
        {
            switch (iEvent)
            {
                case RTSP_PLAY_METHOD:
                    if( (RTSPServer_HandlePlayRequest(pcFirstLine,pClient,pServer)) == 0 )
                    {
                        pClient->iStatus = PLAY_STATE;
#ifdef RTSPRTP_MULTICAST
						//20130924 modified by Charles for ondemand multicast
						if( pClient->iMulticast > 0 )
                        {	
	                        if(pClient->bNewMulticast == TRUE)
							{
								if(RTSPServer_AddOnDemandMulticast(pClient, pServer) == S_FAIL)
								{
									break;
								}
								
							}
                        	
							RTSPServer_FillMulticastSessison(&session_info, pClient);
                        }                  
						else
#endif						
						RTSPServer_FillSessisonInfo(&session_info, pClient);
                        
                        //2005/05/31 modified by ShengFu
                        if( RTSPServer_SessionStart(pClient, &session_info) != 0 )
                        {
                            pClient->iStatus = SETUP_STATE;
                            RTSPServer_InitClient(pClient);
                        }
                    }
                    break;

                case RTSP_TEARDOWN_METHOD:
                    RTSPServer_HandleTeardownRequest(pClient,pServer);
                    break;

				case RTSP_OPTIONS_METHOD:
					RTSPServer_HandleOptionsRequest(pClient);
					RTSPServer_SendOptionsReply(pClient,pServer);                    
					break;
					
                case RTSP_SETPARAM_METHOD:
					RTSPServer_SendSetParameterReply(pClient,pServer);                    
					break;

				//20101006 Added by danny for support GET_PARAMETER command for keep-alive in RTSP signaling
				case RTSP_GETPARAM_METHOD:
					RTSPServer_SendGetParameterReply(pClient,pServer);                    
					break;

                case RTSP_DESCRIBE_METHOD:
                case RTSP_SETUP_METHOD:	
                    if(RTSPServer_HandleSetupRequest(pcFirstLine,pClient,pServer) == 0)
                    {
                         pClient->iStatus = SETUP_STATE;
                    } 
                    break;
                default:
                    //DbgLog((dfCONSOLE|dfINTERNAL,sStateErrMsg, iEvent, pClient->iStatus));
                    RTSPServer_DiscardMessage(pClient);		/* remove remainder of message from in_buffer */
                    RTSPServer_SendReply(501, "Accept: PLAY, TEARDOWN\n",pClient,pServer);
                    break;
            }
            break;
        }/* SETUP state */

        case PLAY_STATE:
        {
            switch (iEvent)
            {
                case RTSP_PLAY_METHOD:
                    RTSPServer_HandlePlayRequest(pcFirstLine,pClient,pServer);
                    break;
                    
				case RTSP_PAUSE_METHOD:
                    if( (RTSPServer_HandlePauseRequest(pClient,pServer)) == 0 )
                    {
                        pClient->iStatus = PAUSE_STATE;
                        RTSPServer_SessionPause(pClient, pClient->ulSessionID);           
                    }
                    break;                    

                case RTSP_TEARDOWN_METHOD:
                    RTSPServer_HandleTeardownRequest(pClient,pServer);                    
                    break;

				case RTSP_OPTIONS_METHOD:
					RTSPServer_HandleOptionsRequest(pClient);
					RTSPServer_SendOptionsReply(pClient,pServer);                    
					break;

                case RTSP_SETPARAM_METHOD:
					RTSPServer_HandleSetParameterRequest(pClient);
					RTSPServer_SendSetParameterReply(pClient,pServer);                    
					break;

				//20101006 Added by danny for support GET_PARAMETER command for keep-alive in RTSP signaling
				case RTSP_GETPARAM_METHOD:
					RTSPServer_HandleGetParameterRequest(pClient);
					RTSPServer_SendGetParameterReply(pClient,pServer);                    
					break;

                case RTSP_DESCRIBE_METHOD:
                case RTSP_SETUP_METHOD:	
                	RTSPServer_DiscardMessage(pClient);
                    RTSPServer_SendReply(455, "Accept: PLAY PAUSE TEARDOWN\n",pClient,pServer);
                    break;
                default:
                    //DbgLog((dfCONSOLE|dfINTERNAL,sStateErrMsg, iEvent, pClient->iStatus));
                    RTSPServer_DiscardMessage(pClient);		/* remove remainder of message from in_buffer */
                    RTSPServer_SendReply(501, "Accept: PAUSE, PLAY, TEARDOWN\n",pClient,pServer);
                    break;
            }
            break;
        }/* PLAY state */
        
        case PAUSE_STATE:
        {
            switch (iEvent)
            {
                case RTSP_PLAY_METHOD:
                    if( (RTSPServer_HandleResumeRequest(pClient,pServer)) == 0 )
                    {
                        pClient->iStatus = PLAY_STATE;
                        RTSPServer_SessionResume(pClient, pClient->ulSessionID);          
                    }
                    break;
                    				
                case RTSP_TEARDOWN_METHOD:
                    RTSPServer_HandleTeardownRequest(pClient,pServer);                    
                    break;

				case RTSP_OPTIONS_METHOD:
					RTSPServer_HandleOptionsRequest(pClient);
					RTSPServer_SendOptionsReply(pClient,pServer);                    
					break;

                case RTSP_SETPARAM_METHOD:
					RTSPServer_SendSetParameterReply(pClient,pServer);                    
					break;

				//20101006 Added by danny for support GET_PARAMETER command for keep-alive in RTSP signaling
				case RTSP_GETPARAM_METHOD:
					RTSPServer_SendGetParameterReply(pClient,pServer);                    
					break;

                case RTSP_DESCRIBE_METHOD:
                case RTSP_SETUP_METHOD:	
                	RTSPServer_DiscardMessage(pClient);
                    RTSPServer_SendReply(455, "Accept: PLAY TEARDOWN\n",pClient,pServer);
                    break;
                default:
                    //DbgLog((dfCONSOLE|dfINTERNAL,sStateErrMsg, iEvent, pClient->iStatus));
                    RTSPServer_DiscardMessage(pClient);		/* remove remainder of message from in_buffer */
                    RTSPServer_SendReply(501, "Accept: PAUSE, PLAY, TEARDOWN\n",pClient,pServer);
                    break;
            }
            break;
        }/* PLAY state */

        default:			/* invalid/unexpected current state. */
        {
            DbgLog((dfCONSOLE|dfINTERNAL,"What the?\n"));
            DbgLog((dfCONSOLE|dfINTERNAL,"State error: unknown state=%d, event code=%d\n",
                    pClient->iStatus, iEvent));
            RTSPServer_SendReply(501, "Accept: PAUSE, PLAY, TEARDOWN\n",pClient,pServer);
            RTSPServer_DiscardMessage(pClient);		/* remove remainder of message from in_buffer */
        }
        break;
    }				/* end of current state switch */
}

int RTSPServer_FullMsgRecv( RTSP_CLIENT* pClient )
{
    int   iMsgHeaderEnd;    /* end of message header found */
    int   iHasMsgBody;      /* message body exists */
    int   iTerminator;      /* terminator count */
    int   iWhiteSpace;      /* white space */
    int   iTotalMsgLen;     /* total message length including any message body */
    int   iMsgBodyLen,iExpectLen;      /* message body length */
    char  c;       /* character */
	short sDataLength;
	int   iCounter=0;

   /*
    * return 0 if a full RTSP message is NOT present in the acRecvBuffer yet.
    * return 1 if a full RTSP message is present in the acRecvBuffer and is
    * ready to be handled.
    * terminate on really ugly cases.
    */
    iMsgHeaderEnd = iHasMsgBody = iTotalMsgLen = iMsgBodyLen = 0;
   
    iCounter=0;
    while ( iTotalMsgLen <= pClient->iRecvSize )
    {
        iCounter ++;
        if( iCounter > 1000 )
        {
            TelnetShell_DbgPrint("Panic ! client is crazy!!\r\n");
            //20081008 remember to close session
			if(pClient->ulSessionID != 0)
			{
				RTSPServer_SessionStop(pClient->parent, pClient->ulSessionID, pClient->iMulticast);
			}
            RTSPServer_InitClient(pClient);            
        }
		// RTCP receiver report if RTCP is interleaved 
		if( pClient->acRecvBuffer[iTotalMsgLen] == '$' )
		{
   			sDataLength = *((short*)(pClient->acRecvBuffer+2));		
			iExpectLen = ntohs(sDataLength) + 4;

			if( pClient->iRecvSize >= iExpectLen )
			{										
				pClient->iRecvSize -= iExpectLen;
				memmove(pClient->acRecvBuffer , pClient->acRecvBuffer + iExpectLen , pClient->iRecvSize);				
				pClient->acRecvBuffer[pClient->iRecvSize] = 0;
			}
			return 0;
		}	

      /* look for eol. */
        iTotalMsgLen += strcspn( &pClient->acRecvBuffer[iTotalMsgLen], "\r\n" );
        
        if ( iTotalMsgLen > pClient->iRecvSize )
        {
            return( 0 );      /* haven't received the entire message yet. */
        }
      /*
       * work through terminaters and then check if it is the
       * end of the message header.
       */
        iTerminator = iWhiteSpace = 0;
      
        while ( !iMsgHeaderEnd && ((iTotalMsgLen + iTerminator + iWhiteSpace) < pClient->iRecvSize) )
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
            iMsgHeaderEnd = 1;      /* must be the end of the message header */
        }
        iTotalMsgLen += iTerminator + iWhiteSpace;

        if ( iMsgHeaderEnd )
        {
            iTotalMsgLen += iMsgBodyLen;   /* add in the message body length, if collected earlier */
            if ( iTotalMsgLen <= pClient->iRecvSize )
            {
                break;   /* all done finding the end of the message. */
            }       
        }

        if( iTotalMsgLen >= pClient->iRecvSize )
        {
            return( 0 );      /* haven't received the entire message yet. */
        }   
      /*
       * check first token in each line to determine if there is
       * a message body.
       */
        if( !iHasMsgBody )     /* content length token not yet encountered. */
        {
            if ( !rtspstrncasecmp( &pClient->acRecvBuffer [iTotalMsgLen], "Content-Length", strlen("Content-Length") ) )
            {
                iHasMsgBody = 1;     /* there is a message body. */
                iTotalMsgLen += strlen("Content-Length");
                
                while ( iTotalMsgLen < pClient->iRecvSize )
                {
                    c = pClient->acRecvBuffer[iTotalMsgLen];
                    
                    if ( (c == ':') || (c == ' ') )
                    {
                        iTotalMsgLen++;
                    }    
                    else
                    {
                        break;
                    }    
                }

                if ( sscanf( &pClient->acRecvBuffer [iTotalMsgLen], "%d", &iMsgBodyLen ) != 1 )
                {
                    DbgLog((dfCONSOLE|dfINTERNAL,"PANIC: invalid ContentLength encountered in message."));
                    return 0;
                }
            }
        }
    }

   return( 1 );
}

int RTSPServer_ReadMsg( char *pcFirstLine, int iMaxBufLen , RTSP_CLIENT *pClient )
{
    unsigned short     usLen;                /* line length */

   /*
    * returns 0 if a full RTSP message is not yet in the in_buffer.
    * returns 1 if an RTSP message is ready to be handled.
    * terminates if the in_buffer becomes full or 1st line of
    * message > buflen.
    */

    if (!pClient->iRecvSize)
    {
        return( 0 );
    }

    if ( !RTSPServer_FullMsgRecv(pClient) )
    {
        return( 0 );
    }
    
   /* transfer bytes from first line of message header for parsing. */
    usLen = strcspn( pClient->acRecvBuffer, "\r\n" );

    if (!usLen)
    {
        RTSPServer_RemoveMessage(2,pClient);
        return RTSPServer_ReadMsg(pcFirstLine, iMaxBufLen,pClient);
    }

    if (usLen > pClient->iRecvSize)
    {
        DbgLog((dfCONSOLE|dfINTERNAL,"PANIC:  message header in buffer is invalid.\n" ));
        pClient->iRecvSize = 0;
        memset(pClient->acRecvBuffer,0,MAX_RECVBUFF);
        return 0 ;
    }

    if ( usLen >= iMaxBufLen || usLen >= MAX_LINE_LEN)
    {
        DbgLog((dfCONSOLE|dfINTERNAL,"PANIC:  message header in buffer is too large.\n" ));
        pClient->iRecvSize = 0;
        memset(pClient->acRecvBuffer,0,MAX_RECVBUFF);
        return 0 ;
    }
      
    memcpy( pcFirstLine, pClient->acRecvBuffer, usLen );
    pcFirstLine[usLen] = '\0';   /* terminate it for further parsing */
   
    return( 1 );      /* a full message is ready to be handled */
}


void RTSPServer_MessageHandler(RTSP_CLIENT* pClient,RTSP_SERVER *pServer)
{
    char     acFirstLine[MAX_LINE_LEN];
    //20120925 added by Jimmy for ONVIF backchannel
    char     acStringLine[MAX_LINE_LEN];
    int      iRet,iHeaderLen,iMsgBodyLen;
    int      iMethodCode,iDecodeSize=0;
    int      iSeq;

    if( pClient->iRTPStreamingMode == RTP_OVER_HTTP && pClient->iRecvSize > 0)
    {
		memcpy(pClient->acBase64Buffer
                   ,pClient->acRecvBuffer + pClient->iUnfinishedRTSPSize
                   ,pClient->iRecvSize - pClient->iUnfinishedRTSPSize);
		pClient->acBase64Buffer[pClient->iRecvSize - pClient->iUnfinishedRTSPSize] =0;
        iDecodeSize = EncryptionUtl_Base64_DecodeString(pClient->acBase64Buffer,pClient->acRecvBuffer + pClient->iUnfinishedRTSPSize,MAX_RECVBUFF - pClient->iUnfinishedRTSPSize);

        pClient->iRecvSize = iDecodeSize + pClient->iUnfinishedRTSPSize ;
        pClient->acRecvBuffer[pClient->iRecvSize] = 0;
    }
	
    while( RTSPServer_ReadMsg( acFirstLine, MAX_LINE_LEN, pClient ) )
    {                
        iMethodCode = RTSPServer_IsValidMethod( acFirstLine,pClient,pServer);

        if ( iMethodCode < 0 )
        {
            if ( iMethodCode == -1 )
            {
                DbgLog((dfCONSOLE|dfINTERNAL,"Method requested was invalid Method \n"));
		        pClient->acRecvBuffer[0] = 0;              
				pClient->iRecvSize = 0;
				pClient->iUnfinishedRTSPSize = 0;
            }
            else if ( iMethodCode == -2 )
            {
                DbgLog((dfCONSOLE|dfINTERNAL,"Bad request line encountered\n" ));
            }
            return;
        }
        
        iSeq = RTSPServer_GetCSeq(pClient);
     
/*        if( iSeq == -1 || ( ( iSeq - pClient->iCSeq !=1 )&& ( pClient->iCSeq != -1) ))
        {
            RTSPServer_SendReply(456,0,pClient,pServer);
            RTSPServer_CleanBuffer(pClient);
            RTSPServer_DiscardMessage(pClient);
            return ;
        } 
        else
*/        {
            pClient->iCSeq = iSeq;
        }                  
        
        //20120925 added by Jimmy for ONVIF backchannel
        pClient->iRequire = REQUIRE_NONE;
        RTSPServer_GetMessageLen(&iHeaderLen, &iMsgBodyLen,pClient);
        iRet= RTSPServer_GetString(pClient->acRecvBuffer,iHeaderLen, "Require", ": \t", acStringLine, sizeof(acStringLine));
		
        if (iRet == 0)
        {
		    if (!rtspstrncasecmp(acStringLine, "www.onvif.org/ver20/backchannel", 31))
		    {
                pClient->iRequire = REQUIRE_ONVIF_BACKCHANNEL;
		    }
        }

        RTSPServer_HandleEvent( iMethodCode, acFirstLine, pClient,pServer );
   }
}


