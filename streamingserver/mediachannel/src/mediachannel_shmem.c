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

#include <stdlib.h>
#include <string.h>
#include "rtsprtpcommon.h"
#include "mediachannel_local.h"

#ifdef _PSOS_TRIMEDIA        
#include "extio.h"			
#endif  
void faber_debug(CHANNEL *pChannel);

#ifdef _SHARED_MEM
static int media_channel_idebug = 0;
unsigned long ChannelShmBitFieldSet(unsigned long uiValue, unsigned long uiBitField, int iStartBit, int iBits)
{
    int	iMask = (1 << iBits) - 1;
    
    return (uiValue & ~(iMask << iStartBit)) + 
           ((uiBitField & iMask) << iStartBit);
}

SCODE  ChannelShmReleaseBuffer(CHANNEL *pChannel)
{
	int		i = 0;
	//20130605 added by Jimmy to support metadata event
	int		j;

	for(i=0 ; i <pChannel->iMaxSession ; i ++ )
	{
		if(pChannel->session[i].dwSessionID != 0 && 
			pChannel->session[i].iStatus != SESSION_PAUSED)
		{
			if(pChannel->session[i].ptShmemMediaInfo != NULL)
			{
				//20130605 modified by Jimmy to support metadata event
				for(j = 0; j < SHMEM_HANDLE_MAX_NUM; j++)
				{
					if(pChannel->session[i].ptShmemMediaInfo->ahClientBuf[j] != NULL)
					{
						SharedMem_ReleaseReadBuffer(pChannel->session[i].ptShmemMediaInfo->ahClientBuf[j]);
						SharedMem_FreeBufClient(&pChannel->session[i].ptShmemMediaInfo->ahClientBuf[j]);
						pChannel->session[i].ptShmemMediaInfo->ahClientBuf[j] = NULL;
					}
				}
			}
		}
	}
	return S_OK;
}

SCODE  ChannelShmSelectError(CHANNEL *pChannel)
{
	int		i = 0, iSckOptLen = 0;
	char    acBuf[16];

	//Unicast
	for(i=0 ; i <pChannel->iMaxSession ; i ++ )
	{
		if(pChannel->session[i].dwSessionID != 0 && 
			pChannel->session[i].iStatus != SESSION_PAUSED)
		{
			RTPRTCPCHANNEL_SESSION		*pSession = &pChannel->session[i];
			int							sSocket = INVALID_SOCKET;

			memset( acBuf, 0, sizeof(acBuf) );
			iSckOptLen = sizeof(int);
			//20110706 Modified by danny For TCP/HTTP mode socket not sync in multi thread
			if( pSession->psktRTP != NULL && *pSession->psktRTP > 0 )
				{
					sSocket= *pSession->psktRTP;	
				}
				else
				{
					sSocket= pChannel->iUDPRTPSock;
				}
			//Check for errors
			if(getsockopt(sSocket, SOL_SOCKET, SO_RCVBUF, (void*)acBuf, (unsigned int *)&iSckOptLen) < 0)
			{
				//Close connection!
				printf("Session %d closed due to socket error!\n", pSession->dwSessionID);
				//20130315 added by Jimmy to log more information
				syslog(LOG_DEBUG, "Session %d closed due to socket error!, errno = %d\n", pSession->dwSessionID, errno);
				RTPRTCPChannel_CloseSession(pChannel, pSession);
			}
		}
	}
	//Multicast
	//20130327 modified by Jimmy to support different video/audio multicast addresses for ONVIF Test Tool 12.12
	//20130904 modified by Charles for ondemand multicast
	for( i=0 ; i< (RTSP_MULTICASTNUMBER + RTSP_ONDEMAND_MULTICASTNUMBER) ; i++)
	{	    
		if( pChannel->stMulticast[i].iStatus == SESSION_PLAYING)
		{
			RTPRTCPCHANNEL_MULTICAST	*pMulticast = &pChannel->stMulticast[i];
			int							sSocket = INVALID_SOCKET;

			sSocket = pMulticast->sktRTP;
			//Check for errors
			if(getsockopt(sSocket, SOL_SOCKET, SO_RCVBUF, (void*)acBuf, (unsigned int *)&iSckOptLen) < 0)
			{
				//Close connection!
				printf("Multicast %d closed due to socket error!\n", i);
				//20130315 added by Jimmy to log more information
				syslog(LOG_DEBUG, "Multicast %d closed due to socket error!, errno = %d\n", i, errno);
				RTPRTCPChannel_CloseMulticast(pChannel, i);
			}
		}
	}

	return S_OK;
}

SCODE ChannelShmemCheckTimeout(CHANNEL *pChannel)
{
	//20101020 Add by danny for support seamless stream TCP/UDP timeout
	DWORD	dwNow = 0, dwElasped = 0, dwTCPTimeOut;
	int		i = 0;

	//Acquire time
	OSTick_GetMSec(&dwNow);
	int iCheckFFF = 0;
	for(i=0 ; i <pChannel->iMaxSession ; i ++ )
	{
     	if(pChannel->session[i].dwSessionID != 0 &&
		   pChannel->session[i].iStatus != SESSION_PAUSED)
		{
			if(pChannel->session[i].ptShmemMediaInfo->tAggreMediaBuffer.dwBaseTime != 0)
		   {
			   //Calculate the time elasped
			   if(dwNow > pChannel->session[i].ptShmemMediaInfo->tAggreMediaBuffer.dwBaseTime)
			   {
					dwElasped = dwNow - pChannel->session[i].ptShmemMediaInfo->tAggreMediaBuffer.dwBaseTime;
			   }
			   else
			   {
			   		iCheckFFF = 1;
					dwElasped = 0xffffffff - pChannel->session[i].ptShmemMediaInfo->tAggreMediaBuffer.dwBaseTime + dwNow + 1;
			   }
			   //Timeout client if time elasped exceed defined value
			   //20101020 Add by danny for support seamless stream TCP/UDP timeout
			   if(pChannel->session[i].bSeamlessStream == TRUE)
			   {
			   	   dwTCPTimeOut = RTPRTCP_SeamlessStream_TCP_TIMEOUT;
			   }
			   else
			   {
			   	   dwTCPTimeOut = RTPRTCP_TCP_TIMEOUT;
			   }
			   if(dwElasped > 10000)
			   {
			   		pChannel->session[i].idebug = 1;
			   		media_channel_idebug = 1;
			   }
			   //if(dwElasped > RTPRTCP_TCP_TIMEOUT)
			   if(dwElasped > dwTCPTimeOut)
			   {
				   printf("Client Session ID %d timeout !\n", pChannel->session[i].dwSessionID);
				   printf("dwNow = %u, dwElasped = %u\n", dwNow, dwElasped);
				   printf("pChannel->session[i].ptShmemMediaInfo->tAggreMediaBuffer.dwBaseTime = %u, iCheckFFF = %d\n", pChannel->session[i].ptShmemMediaInfo->tAggreMediaBuffer.dwBaseTime, iCheckFFF);
				   //20130315 added by Jimmy to log more information
				   syslog(LOG_DEBUG, "Client Session ID %d timeout !\n", pChannel->session[i].dwSessionID);
				   RTPRTCPChannel_CloseSession(pChannel, &pChannel->session[i]);
			   }
		   }
		}
	}

	return S_OK;
}

//20101018 Add by danny for support multiple channel text on video
DWORD ChannelShmComposeRTPExt(CHANNEL *pChannel, BYTE *pbyOut, DWORD dwOutLen, BYTE *pbyDP, DWORD dwDPLen, DWORD dwIVLen, int iMultipleChannelChannelIndex)
{
	DWORD	dwActulOutLen, dwPadding, dwTmp;
	//DWORD	dwLocationLen = strlen(pChannel->acLocation);
	DWORD	dwLocationLen = 0;
	BYTE	*pbyUserData = NULL, *pbyIV = NULL;
	//20100628 danny, Modified for TLV issue
	int		iTLVRet;
	
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
	
	if(iMultipleChannelChannelIndex <= MULTIPLE_CHANNEL_NUM)
	{
		if(pChannel->nChannelType != RTPRTCPCHANNEL_MEDIATYPE_METADATA)
		{
			dwLocationLen = strlen(pChannel->acLocation[iMultipleChannelChannelIndex-1]);
		}
	}	
	//20100628 danny, Modified for TLV issue, 1 is tag size
	/*dwTmp = dwDPLen + (dwDPLen ? 2 : 0) + 
		dwLocationLen + (dwLocationLen ? 2 : 0) + dwIVLen + (dwIVLen ? 2 : 0);*/
	dwTmp = dwDPLen + (dwDPLen ? (1 + RtspRtpCommon_TLVStrlen(NULL, (int)dwDPLen)) : 0) + 
			dwLocationLen + (dwLocationLen ? (1 + RtspRtpCommon_TLVStrlen(NULL, (int)dwLocationLen)) : 0) + 
			dwIVLen + (dwIVLen ? (1 + RtspRtpCommon_TLVStrlen(NULL, (int)dwIVLen)) : 0);
	
	/* Nothing to be done */
	if (dwTmp == 0)	
	{
		//printf("no extension!\n");
		return 0;
	}
	//printf("[%s]dwTmp %d\n", __FUNCTION__, dwTmp);
	/* Check padding is needed. */
	dwActulOutLen = (dwTmp & 0x3) ? (((dwTmp >> 2) + 1) << 2) : dwTmp;
	//printf("[%s]dwActulOutLen %d\n", __FUNCTION__, dwActulOutLen);
	dwPadding = dwActulOutLen - dwTmp;
	/* If the output size is not enough, ignore the extension */
	if (dwActulOutLen > dwOutLen)
	{
		TelnetShell_DbgPrint("!!! extension overflow %d Userdata %d Intelligent %d\r\n", dwActulOutLen, dwDPLen, dwIVLen);
		syslog(LOG_WARNING, "RTP extension overflow, extension data ignored!\n");
		return 0;
	}
	/* feed the zero padding at start place */
	memset(pbyOut, 0, dwPadding);  
	pbyOut += dwPadding;
	/* feed the Location */
	if (dwLocationLen > 0 && dwLocationLen < LOCATION_LEN)
	{
		pbyOut[0] = eRTP_EX_LOCATION;
		//20100628 danny, Modified for TLV issue, 1 is tag size
		/*pbyOut[1] = (BYTE) dwLocationLen;
		pbyOut += 2;*/
		iTLVRet = RtspRtpCommon_TLVStrlen((char*)&pbyOut[1], dwLocationLen);
		pbyOut += (1 + iTLVRet);
		//memcpy (pbyOut, pChannel->acLocation, dwLocationLen);
		memcpy (pbyOut, pChannel->acLocation[iMultipleChannelChannelIndex-1], dwLocationLen);
		pbyOut += dwLocationLen;
	}
	//printf("[%s]dwLocationLen %d\n", __FUNCTION__, dwLocationLen);
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
		//20100628 danny, Modified for TLV issue, 1 is tag size
		/*pbyOut[1] = (BYTE) dwIVLen;
		pbyOut += 2;*/
		iTLVRet = RtspRtpCommon_TLVStrlen((char*)&pbyOut[1], dwIVLen);
		pbyOut += (1 + iTLVRet);
		memcpy (pbyOut, pbyIV, dwIVLen);
		pbyOut += dwIVLen;

	}
	//printf("[%s]dwIVLen %d\n", __FUNCTION__, dwIVLen);
	/* feed the DataPacket */
	if (dwDPLen)
	{
		pbyOut[0] = eRTP_EX_DATAPACKETHEADER;
		//20100628 danny, Modified for TLV issue, 1 is tag size
		/*pbyOut[1] = (BYTE) dwDPLen;
		pbyOut += 2;*/
		iTLVRet = RtspRtpCommon_TLVStrlen((char*)&pbyOut[1], dwDPLen);
		pbyOut += (1 + iTLVRet);
		memcpy (pbyOut, pbyUserData, dwDPLen);
		pbyOut += dwDPLen;
	}
	//printf("[%s]dwDPLen %d\n", __FUNCTION__, dwDPLen);
	return dwActulOutLen;
}

//20120618 danny, modified JPEG RTP header extension according to ONVIF spec and RFC2435
//20101108 danny, Modified for TLV issue
DWORD ChannelShmComposeJPEGRTPExt(BYTE *pbyOut, DWORD dwOutLen, BYTE *pbyIn, DWORD dwAppDataSize, BYTE *pbyStartOfFrameBDCT, DWORD dwStartOfFrameBDCTSize)
{
	DWORD	dwActualOutLen = 0, dwPadding = 0, dwExtensionLength = 0;
	//int		iTLVRet;

	//if(dwAppDataSize == 0 && dwStartOfFrameBDCTSize == 0)	//No application data and Start Of Frame Baseline DCT to be appended
	if(dwAppDataSize == 0)	//No application data to be appended
	{
		return 0;
	}

	//For now, the extension is application data only, no need for TLV ! 20090117
	//dwExtensionLength = dwAppDataSize + dwStartOfFrameBDCTSize;	//Append application data and Start Of Frame Baseline DCT
	dwExtensionLength = dwAppDataSize;								//Append application data
	//dwExtensionLength = dwAppDataSize + (dwAppDataSize ? (1 + RtspRtpCommon_TLVStrlen(NULL, (int)dwAppDataSize)) : 0);
	//printf("[%s]dwExtensionLength %d\n", __FUNCTION__, dwExtensionLength);
	
	//Check if padding is needed
	if(dwExtensionLength & 0x3)
	{
		dwActualOutLen = ((dwExtensionLength >> 2) + 1) << 2;
	}
	else
	{
		dwActualOutLen = dwExtensionLength;
	}
	dwPadding = dwActualOutLen - dwExtensionLength;
	/* If the output size is not enough, ignore the extension */
	if (dwActualOutLen > dwOutLen)
	{
		TelnetShell_DbgPrint("!!! JPEG RTP extension overflow %d\r\n", dwActualOutLen);
		return 0;
	}
	//Feed the padding with 0, however we must note that ONVIF spec requires 0xff padding
	memset(pbyOut, 255, dwPadding);
	pbyOut += dwPadding;
	//Feed in the application data now
	if(dwAppDataSize)
	{
		//pbyOut[0] = eRTP_EX_JPEGAPPDATA;
		/*pbyOut[1] = (BYTE) dwAppDataSize;
		pbyOut += 2;*/
		//iTLVRet = RtspRtpCommon_TLVStrlen((char*)&pbyOut[1], dwAppDataSize);
		//pbyOut += (1 + iTLVRet);
		memcpy(pbyOut, pbyIn, dwAppDataSize);
		pbyOut += dwAppDataSize;
	}

	//Feed in the Start Of Frame Baseline DCT now
	/*if(dwStartOfFrameBDCTSize)
	{
		memcpy(pbyOut, pbyStartOfFrameBDCT, dwStartOfFrameBDCTSize);
		pbyOut[10] = 0;
		pbyOut[13] = 1;
		pbyOut[16] = 2;
		pbyOut += dwStartOfFrameBDCTSize;
	}*/
	//printf("[%s]dwAppDataSize %d\n", __FUNCTION__, dwAppDataSize);
	return dwActualOutLen;
}

//20141110 added by Charles for ONVIF Profile G
DWORD ChannelShmComposeONVIFRTPExt(BYTE *pbyOut, DWORD dwOutLen, DWORD dwSecond, DWORD dwMilliSecond, BOOL bOnvifDbit, int iCSeqUpdated)
{
    unsigned long *plOutData = (unsigned long*)pbyOut;
    int iExtensionNum = 0;
    int i;
    unsigned long ulntp_msw, ulntp_lsw;

    RTPRTCPComposer_GetNTPTimeFromUnixLocalTime(dwSecond, dwMilliSecond, &ulntp_msw, &ulntp_lsw);
        
    plOutData[0] = ulntp_msw;
    iExtensionNum++;
    
    plOutData[1] = ulntp_lsw;
    iExtensionNum++;
    
    plOutData[2] = ChannelShmBitFieldSet(plOutData[2], 0xffff, 0, 16); //padding
    plOutData[2] = ChannelShmBitFieldSet(plOutData[2], iCSeqUpdated, 16, 8); //Cseq
    plOutData[2] = ChannelShmBitFieldSet(plOutData[2], 0, 24, 5); //mbz, must be zero
    if(bOnvifDbit) 
    {
        plOutData[2] = ChannelShmBitFieldSet(plOutData[2], 1, 29, 1); //D
    }
    else
    {
        plOutData[2] = ChannelShmBitFieldSet(plOutData[2], 0, 29, 1); //D
    }
    plOutData[2] = ChannelShmBitFieldSet(plOutData[2], 0, 30, 1); //E
    plOutData[2] = ChannelShmBitFieldSet(plOutData[2], 0, 31, 1); //C
    iExtensionNum++;

    for(i = 0; i < iExtensionNum; i++)
    {
        plOutData[i] = htonl(plOutData[i]);
    }

    return (iExtensionNum * sizeof(unsigned long));
}

//20150408 added by Charles ONVIF JPEG Extension
DWORD ChannelShmComposeOnvifJpegRTPExt(BYTE *pbyOut, DWORD dwOutLen, BYTE *pbyStartOfFrameBDCT, DWORD dwStartOfFrameBDCTSize, BOOL bComposeExtTag)
{
    DWORD   dwActualOutLen = 0, dwPadding = 0, dwExtensionLength = 0;

    if(dwStartOfFrameBDCTSize == 0)
    {
        return 0;
    }

    dwExtensionLength = dwStartOfFrameBDCTSize;
    //Check if padding is needed
	if(dwExtensionLength & 0x3)
	{
		dwActualOutLen = ((dwExtensionLength >> 2) + 1) << 2;
	}
	else
	{
		dwActualOutLen = dwExtensionLength;
	}

    if (dwActualOutLen > dwOutLen)
	{
		printf("[%s]ONVIF JPEG RTP extension overflow %d\r\n", __FUNCTION__, dwActualOutLen);
		return 0;
	}

    dwPadding = dwActualOutLen - dwExtensionLength;

    //if bComposeExtTag is set, compose FFD8 extension tag here!
    if(bComposeExtTag)
    {
        DWORD dwTempLength = dwActualOutLen >> 2;
        BYTE *pbyExtensionLength = (BYTE*)&dwTempLength;
        pbyOut[0] = 0xFF;
        pbyOut[1] = 0xD8;
        pbyOut[2] = pbyExtensionLength[1];
        pbyOut[3] = pbyExtensionLength[0];
        pbyOut += 4;
    }

    //Feed the SOF Marker
    memcpy(pbyOut, pbyStartOfFrameBDCT, dwStartOfFrameBDCTSize);
    //Feed the padding
    memset(pbyOut + dwStartOfFrameBDCTSize, 0xFF, dwPadding);
    
    return (dwActualOutLen + (bComposeExtTag ? 4: 0));
    
}

SCODE  ChannelShmComposeRFC2435Header(TBitstreamBuffer *ptBitStreamBuffer, BYTE *pbyOut, BOOL bFirstPacket)
{
	BYTE	*pbyFragment = NULL;


	//Fill in 1 byte Type-Specific 
	pbyOut[0] = 0;
	pbyOut++;

	//Fill in 3 bytes Fragment, this must be done in reverse order
	pbyFragment = (BYTE *)(&ptBitStreamBuffer->dwCurrentPosition);
	pbyOut[0] = pbyFragment[2];
	pbyOut[1] = pbyFragment[1];
	pbyOut[2] = pbyFragment[0];
	pbyOut += 3;

	//Fill in 1 bytes Type
	pbyOut[0] = ptBitStreamBuffer->tJPEGoverRTPInfo.byType;
	//20101210 Add by danny For support DRI header
	if (ptBitStreamBuffer->tJPEGoverRTPInfo.dwDRI)
	{
		pbyOut[0] += 64;
	}
	pbyOut++;

	//Fill in 1 bytes Q value, fixed for 128 right now
	pbyOut[0] = 128;
	pbyOut++;

    //20150408 modified by Charles for ONVIF JPEG Extension
    if(!ptBitStreamBuffer->tJPEGoverRTPInfo.bEnableOnvifJpegRTPExt)
    {
        //Fill in 1 bytes Width
    	pbyOut[0] = (BYTE)(ptBitStreamBuffer->tJPEGoverRTPInfo.wWidth/8);
    	pbyOut++;

    	//Fill in 1 bytes Height
    	pbyOut[0] = (BYTE)(ptBitStreamBuffer->tJPEGoverRTPInfo.wHeight/8);
    	pbyOut++;
    }
	else
    {
        pbyOut[0] = 0;
		pbyOut++;
		pbyOut[0] = 0;
		pbyOut++;		
    }   

	//20101210 Add by danny For support DRI header
	if (ptBitStreamBuffer->tJPEGoverRTPInfo.dwDRI)
	{
		BYTE	*pbyDRI = (BYTE *)(&ptBitStreamBuffer->tJPEGoverRTPInfo.dwDRI);
		pbyOut[0] = pbyDRI[1];
		pbyOut[1] = pbyDRI[0];
		pbyOut[2] = 0xff;
		pbyOut[3] = 0xff;
		pbyOut += 4;
	}
	
	//Fill in Quantization table if this is the first packet
	if(bFirstPacket)
	{
		// 1 bytes MBZ value
		pbyOut[0] = 0;
		pbyOut++;
		// 1 bytes Precision value
		pbyOut[0] = 0;
		pbyOut++;
		// 2 bytes length value, with current value fixed at 128 bytes
		pbyOut[0] = 0;
		pbyOut[1] = 128;
		pbyOut += 2;
		// Write the 128 bytes Quantization table
		memcpy(pbyOut, ptBitStreamBuffer->tJPEGoverRTPInfo.acQuantizationTable, 128);
		pbyOut += 128;
	}

	return S_OK;
}

SCODE ChannelShmComposeH264FU_A(TBitstreamBuffer *ptBitStreamBuffer, BYTE *pbyOut, WORD wPacketCount, BOOL bEndMarker)
{
	BYTE	byFUIndicator = 0, byFUHeader = 0;

    /*+---------------+
      |0|1|2|3|4|5|6|7|
      +-+-+-+-+-+-+-+-+
      |F|NRI|  Type   |
      +---------------+  FU Indicator, TYPE is 28 for FU-A, F & NRI are from NALU octet */

	
	byFUIndicator = ((ptBitStreamBuffer->tH264overRTPInfo.byNALOctet & 0xE0) | 0x1C);	//0xE0 = 11100000, 0x1C = 28
	memcpy(pbyOut, &byFUIndicator, 1);
	//printf("NALOCTET %02x FU indicator %02x\n", ptBitStreamBuffer->tH264overRTPInfo.byNALOctet, byFUIndicator);

   /* +---------------+
      |0|1|2|3|4|5|6|7|
      +-+-+-+-+-+-+-+-+
      |S|E|R|  Type   |
      +---------------+ FU Header, S marks start, E marks End, R is ignored, Type is NALU type*/

	byFUHeader = ptBitStreamBuffer->tH264overRTPInfo.byNALOctet & 0x1F;	//0x1F = 00011111

	if(wPacketCount == 1)
	{
		//First RTP packet of a fragmentation
		byFUHeader = (byFUHeader | (1 << 7));
	}
	else if(bEndMarker == 1)
	{
		//Last RTP packet of a fragmentation
		byFUHeader = (byFUHeader | (1 << 6));
	}
	else
	{
		//Not either, So the two bits can remain zero
	}

	memcpy(pbyOut + 1, &byFUHeader, 1);
	//printf("NALOCTET %02x FU header %02x\n", ptBitStreamBuffer->tH264overRTPInfo.byNALOctet, byFUHeader);

	return S_OK;
}

SCODE ChannelShmH264Info(TBitstreamBuffer *ptBitStreamBuffer, BYTE *pbyH264Stream, TH264overRTPInfo *ptH264Info, DWORD dwNALULength, DWORD *pdwStreamOffset, DWORD dwAvailableBuffer)
{
	DWORD dwNALULen = 0;

	memcpy(&dwNALULen, pbyH264Stream, 4);
	dwNALULen = ntohl(dwNALULen);
	//Size comparison check!
	if(dwNALULen != dwNALULength)
	{
		printf("NALU size mismatch! %u to %u\n", dwNALULen, dwNALULength);
		return S_FAIL;
	}

	//20090609 compute correctly to evaluate if this packet need to be fragmented
	if(dwNALULength > dwAvailableBuffer)
	{
		ptH264Info->ePacketFragMode = EFragmentationNALU;
		*pdwStreamOffset = 5;								//4 bytes start code (or length) + 1 bytes octet
	}
	else
	{
		ptH264Info->ePacketFragMode = ESingleNALU;
		*pdwStreamOffset = 4;								//4 bytes start code (or length) 
	}

	//NALU type and Octet
	memcpy(&ptH264Info->byNALOctet, pbyH264Stream + 4, 1);
	ptH264Info->byNALType = (ptH264Info->byNALOctet & 0x1f);

	//Intra frame
	if(ptH264Info->byNALType == 5)
	{
		ptBitStreamBuffer->tFrameType = MEDIADB_FRAME_INTRA;
	}
	else
	{
		ptBitStreamBuffer->tFrameType = MEDIADB_FRAME_PRED;
	}

	/*printf("NALU type %u size %u Head %02x %02x %02x %02x Tail %02x %02x %02x %02x\n",
			ptBitStreamBuffer->tFrameType, 
			dwNALULength,			
			*(pbyH264Stream + *pdwStreamOffset+ 0),
			*(pbyH264Stream + *pdwStreamOffset+ 1),
			*(pbyH264Stream + *pdwStreamOffset+ 2),
			*(pbyH264Stream + *pdwStreamOffset+ 3),			
			*(pbyH264Stream + dwNALULength + 0),
			*(pbyH264Stream + dwNALULength + 1),
			*(pbyH264Stream + dwNALULength + 2),
			*(pbyH264Stream + dwNALULength + 3));*/

	return S_OK;
}

SCODE ChannelShmComposeH265FU_A(TBitstreamBuffer *ptBitStreamBuffer, BYTE *pbyOut, WORD wPacketCount, BOOL bEndMarker)
{
     BYTE    byFUHeader = 0;
     unsigned short usPayloadHdr = 0;
    
       /* HEVC payload header
         0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |F|   Type    |  LayerId  | TID |
        +-------------+-----------------+

           Forbidden zero (F): 1 bit
           NAL unit type (Type): 6 bits
           NUH layer ID (LayerId): 6 bits
           NUH temporal ID plus 1 (TID): 3 bits */
     
     usPayloadHdr = ((ptBitStreamBuffer->tH265overRTPInfo.usNALHeader & 0x81FF) | 0x6200);  
     usPayloadHdr = htons(usPayloadHdr);
     memcpy(pbyOut, &usPayloadHdr, 2);
     
     /* +---------------+
          0 1 2 3 4 5 6 7
         +-+-+-+-+-+-+-+-+
         |S|E|  FuType   |
         +---------------+ FU Header, S marks start, E marks End, FuType is NALU type*/
    
     byFUHeader = ptBitStreamBuffer->tH265overRTPInfo.byNALType;
    
     if(wPacketCount == 1)
     {
         //First RTP packet of a fragmentation
         byFUHeader = (byFUHeader | (1 << 7));
     }
     else if(bEndMarker == 1)
     {
         //Last RTP packet of a fragmentation
         byFUHeader = (byFUHeader | (1 << 6));
     }
     else
     {
         //Not either, So the two bits can remain zero
     }
    
     memcpy(pbyOut + 2, &byFUHeader, 1);
     //printf("NALOCTET %02x FU header %02x\n", ptBitStreamBuffer->tH264overRTPInfo.byNALOctet, byFUHeader);
    
     return S_OK;
}

SCODE ChannelShmH265Info(TBitstreamBuffer *ptBitStreamBuffer, BYTE *pbyH265Stream, TH265overRTPInfo *ptH265Info, DWORD dwNALULength, DWORD *pdwStreamOffset, DWORD dwAvailableBuffer)
{
	DWORD dwNALULen = 0;

	memcpy(&dwNALULen, pbyH265Stream, 4);
	dwNALULen = ntohl(dwNALULen);
	//Size comparison check!
	if(dwNALULen != dwNALULength)
	{
		printf("NALU size mismatch! %u to %u\n", dwNALULen, dwNALULength);
		return S_FAIL;
	}

	//20090609 compute correctly to evaluate if this packet need to be fragmented
	if(dwNALULength > dwAvailableBuffer)
	{
		ptH265Info->ePacketFragMode = EH265FragmentationNALU;
		*pdwStreamOffset = 6;								//4bytes start code (or length) + 2 bytes payload header
	}
	else
	{
		ptH265Info->ePacketFragMode = EH265SingleNALU;
		*pdwStreamOffset = 4;								//4bytes start code (or length) 
	}

	//NALU type and Octet
	memcpy(&ptH265Info->usNALHeader, pbyH265Stream + 4, 2);
    ptH265Info->usNALHeader = ntohs(ptH265Info->usNALHeader);
	ptH265Info->byNALType = (ptH265Info->usNALHeader >> 9) & 0x3f;

	//Intra frame
	if(ptH265Info->byNALType == NAL_IDR_W_RADL || ptH265Info->byNALType == NAL_IDR_N_LP)
	{
		ptBitStreamBuffer->tFrameType = MEDIADB_FRAME_INTRA;
	}
	else
	{
		ptBitStreamBuffer->tFrameType = MEDIADB_FRAME_PRED;
	}

	/*printf("NALU type %u size %u Head %02x %02x %02x %02x Tail %02x %02x %02x %02x\n",
			ptBitStreamBuffer->tFrameType, 
			dwNALULength,			
			*(pbyH265Stream + *pdwStreamOffset+ 0),
			*(pbyH265Stream + *pdwStreamOffset+ 1),
			*(pbyH265Stream + *pdwStreamOffset+ 2),
			*(pbyH265Stream + *pdwStreamOffset+ 3),			
			*(pbyH265Stream + dwNALULength + 0),
			*(pbyH265Stream + dwNALULength + 1),
			*(pbyH265Stream + dwNALULength + 2),
			*(pbyH265Stream + dwNALULength + 3));*/

	return S_OK;

}

SCODE  ChannelShmClearAggregateBuffer(TAggregateMediaBuffer *ptAggreBuf)	//This function will clear the whole Aggregate buffer
{
	//int			i = 0;

	ptAggreBuf->dwTotalSize = 0;
	ptAggreBuf->dwTotalRemaining = 0;
	ptAggreBuf->dwBaseTime = 0;
	ptAggreBuf->iProcessRTPIndex = 0;

/*														 PS: We assume this is not needed
	for(i = 0; i < ptAggreBuf->iBufferNumber; i++)
	{
		ptAggreBuf->ptRTPBuffer[i].dwBytesUsed = 0;
		ptAggreBuf->ptRTPBuffer[i].dwExtensionLen = 0;
		ptAggreBuf->ptRTPBuffer[i].dwRemainingLength = 0;
		ptAggreBuf->ptRTPBuffer[i].dwBitStreamLength = 0;	
		ptAggreBuf->ptRTPBuffer[i].dwRTPExtraData = 0;
		ptAggreBuf->ptRTPBuffer[i].pbDataStart = (BYTE *)ptAggreBuf->ptRTPBuffer[i].acPadBuffer + 4;
	}
*/
	return S_OK;
}


SCODE  ChannelShmResetMediaInfo(TShmemMediaInfo *pMediaInfo)	//This function will discard the whole frame
{
	int	i = 0;

	pMediaInfo->iRemainingSize = 0;
	pMediaInfo->tStreamBuffer.dwBytesUsed = 0;
	pMediaInfo->tStreamBuffer.dwCurrentPosition = 0;
	pMediaInfo->tAggreMediaBuffer.dwTotalSize = 0;
	pMediaInfo->tAggreMediaBuffer.dwTotalRemaining = 0;
	pMediaInfo->tAggreMediaBuffer.dwBaseTime = 0;
	pMediaInfo->tAggreMediaBuffer.iProcessRTPIndex = 0;

	for(i = 0; i < pMediaInfo->tAggreMediaBuffer.iBufferNumber; i++)
	{
		pMediaInfo->tAggreMediaBuffer.ptRTPBuffer[i].dwBytesUsed = 0;
		pMediaInfo->tAggreMediaBuffer.ptRTPBuffer[i].dwExtensionLen = 0;
		pMediaInfo->tAggreMediaBuffer.ptRTPBuffer[i].dwRemainingLength = 0;
		pMediaInfo->tAggreMediaBuffer.ptRTPBuffer[i].dwBitStreamLength = 0;	
		pMediaInfo->tAggreMediaBuffer.ptRTPBuffer[i].dwRTPExtraData = 0;
		pMediaInfo->tAggreMediaBuffer.ptRTPBuffer[i].pbDataStart = (BYTE *)pMediaInfo->tAggreMediaBuffer.ptRTPBuffer[i].acPadBuffer + 4;
	}

	//20091228 Fixed TCP broken image, avoid release Shmem before send out completely
	//20130605 modified by Jimmy to support metadata event
	for(i = 0; i < SHMEM_HANDLE_MAX_NUM; i++)
	{
		SharedMem_ReleaseReadBuffer(pMediaInfo->ahClientBuf[i]);
	}
	
	return S_OK;
}

//20101018 Add by danny for support multiple channel text on video
SCODE  ChannelShmPacketize(CHANNEL *pChannel, TShmemMediaInfo *pMediaInfo, int iVivotekClient, int iMultipleChannelChannelIndex, int iCSeq, int *piCSeqUpdated)
{
    char*			pchBitBuffPtr;
    int				iBitLen = 0;
	BOOL			bFirstIPacket = FALSE;
	BYTE			abyRTPExt[RTP_EXTENSION - 4];
	DWORD			dwPacketNum = 0, dwRemPacketSize = 0, dwTmp = 0, dwExtLen = 0;
	RTPMEDIABUFFER* pRTPMediaBuff = NULL;
	//20081230 JPEG codec type
	TJPEGoverRTPInfo	*ptJPEGoverRTPInfo = &pMediaInfo->tStreamBuffer.tJPEGoverRTPInfo;
	DWORD				dwExtraRTPPayload = 0, dwStreamStartOffset = 0;
	//20091015 Writev modification
	int				iBufferCount = 0;

	//Check for media present first, if bitstream buffer is empty then return
	if(pMediaInfo->tStreamBuffer.dwBytesUsed == 0 || pMediaInfo->iRemainingSize == 0)
	{
		return S_FAIL;
	}

	//20091015 Writev modification
	pMediaInfo->tAggreMediaBuffer.dwTotalSize = 0;
	pMediaInfo->tAggreMediaBuffer.dwTotalRemaining = 0;
	while(iBufferCount < pMediaInfo->tAggreMediaBuffer.iBufferNumber && pMediaInfo->iRemainingSize > 0)
	{
		BYTE	*pbyHeaderBuffer = NULL, *pbyRTPExtraStart = NULL;

		pRTPMediaBuff = &(pMediaInfo->tAggreMediaBuffer.ptRTPBuffer[iBufferCount]);
		pchBitBuffPtr = (CHAR*) pMediaInfo->tStreamBuffer.pbyBuffer;
		iBitLen = pMediaInfo->tStreamBuffer.dwBytesUsed;
		dwPacketNum = pMediaInfo->tStreamBuffer.pdwPacketSize[0];

		//Fisrt Packet of I frame
		if( pMediaInfo->tStreamBuffer.tFrameType == MEDIADB_FRAME_INTRA && pMediaInfo->tStreamBuffer.wPacketCount == 0)
		{
			bFirstIPacket = TRUE;  
		}

		//Compose Extension if this is the first packet of the frame
		if(pMediaInfo->tStreamBuffer.wPacketCount == 0 && iVivotekClient == 1)
		{
			if(pMediaInfo->tStreamBuffer.dwStreamType == mctJPEG)
			{
			    if(ptJPEGoverRTPInfo->bEnableOnvifJpegRTPExt)
                {
                    //NOTE:we must put Onvif JPEG Extension before VVTK Application Data to pass Onvif test tool
                    dwExtLen += ChannelShmComposeOnvifJpegRTPExt(abyRTPExt + dwExtLen, sizeof(abyRTPExt) - dwExtLen, ptJPEGoverRTPInfo->pbyStartOfFrameBDCT, ptJPEGoverRTPInfo->dwStartOfFrameBDCTSize, FALSE);
                }    
				//20120618 danny, modified JPEG RTP header extension according to ONVIF spec and RFC2435
				dwExtLen += ChannelShmComposeJPEGRTPExt(abyRTPExt + dwExtLen, sizeof(abyRTPExt) - dwExtLen, ptJPEGoverRTPInfo->pbyApplicationData, ptJPEGoverRTPInfo->dwApplicationDataSize, ptJPEGoverRTPInfo->pbyStartOfFrameBDCT, ptJPEGoverRTPInfo->dwStartOfFrameBDCTSize);
                
			}
			else
			{
				//20101018 Add by danny for support multiple channel text on video
				dwExtLen = ChannelShmComposeRTPExt(pChannel, abyRTPExt, sizeof(abyRTPExt), (BYTE *)pchBitBuffPtr, pMediaInfo->tStreamBuffer.dwOffset, 
													pMediaInfo->tStreamBuffer.dwIntelligentVideoLength, iMultipleChannelChannelIndex);
			}
		}
        //20141110 added by Charles for ONVIF Profile G
        else if(pMediaInfo->tStreamBuffer.wPacketCount == 0 && pMediaInfo->bMediaOnDemand)
        {
            if(pMediaInfo->tStreamBuffer.bOnvifDbit)
            {
                *piCSeqUpdated = iCSeq;
            }
            dwExtLen = ChannelShmComposeONVIFRTPExt(abyRTPExt, sizeof(abyRTPExt), pMediaInfo->tStreamBuffer.dwSecond, pMediaInfo->tStreamBuffer.dwMilliSecond, pMediaInfo->tStreamBuffer.bOnvifDbit, *piCSeqUpdated);
        }
		else
		{
			dwExtLen = 0;
		}

        //20150408 added by Charles for ONVIF JPEG Extension
        if(pMediaInfo->tStreamBuffer.wPacketCount == 0 && pMediaInfo->tStreamBuffer.dwStreamType == mctJPEG && ptJPEGoverRTPInfo->bEnableOnvifJpegRTPExt && iVivotekClient == 0)
        {
            BOOL    bComposeExtTag = FALSE; 
            if(dwExtLen)
            {
                bComposeExtTag = TRUE;
            }
            dwExtLen += ChannelShmComposeOnvifJpegRTPExt(abyRTPExt + dwExtLen, sizeof(abyRTPExt) - dwExtLen, ptJPEGoverRTPInfo->pbyStartOfFrameBDCT, ptJPEGoverRTPInfo->dwStartOfFrameBDCTSize, bComposeExtTag);
        }

		//Check for remaining size to be cut from this Bitstreambuf
		if(pMediaInfo->iRemainingSize > 0)
		{
			if(dwPacketNum > 1 && pMediaInfo->tStreamBuffer.dwStreamType != mctH264 && pMediaInfo->tStreamBuffer.dwStreamType != mctH265)  //Video packet mode, notice that H.264 shares this field but not for video-packet mode
			{
				dwRemPacketSize = pMediaInfo->tStreamBuffer.pdwPacketSize[pMediaInfo->tStreamBuffer.wPacketCount + 1];
			}
			else
			{
				dwRemPacketSize = pMediaInfo->iRemainingSize;
			}
		}

		//Move start pointer according to IVA data and current position
		if(pMediaInfo->tStreamBuffer.dwIntelligentVideoLength == 0)
		{
			pchBitBuffPtr += (pMediaInfo->tStreamBuffer.dwOffset + pMediaInfo->tStreamBuffer.dwCurrentPosition) ;
		}
		else
		{
			pchBitBuffPtr += (pMediaInfo->tStreamBuffer.dwOffset + 4 + pMediaInfo->tStreamBuffer.dwIntelligentVideoLength + pMediaInfo->tStreamBuffer.dwCurrentPosition) ;
		}

		//H264 only
		if(pMediaInfo->tStreamBuffer.wPacketCount == 0 && pMediaInfo->tStreamBuffer.dwStreamType == mctH264)
		{
			//Parse for information
			if(ChannelShmH264Info(&pMediaInfo->tStreamBuffer, (BYTE *)pchBitBuffPtr, &pMediaInfo->tStreamBuffer.tH264overRTPInfo, pMediaInfo->tStreamBuffer.pdwPacketSize[pMediaInfo->tStreamBuffer.tH264overRTPInfo.iNALUIndex], &dwStreamStartOffset, pRTPMediaBuff->dwBufferLength - dwExtLen) != S_OK)
			{
				ChannelShmResetMediaInfo(pMediaInfo);
				return S_FAIL;
			}
			if(pMediaInfo->tStreamBuffer.tFrameType == MEDIADB_FRAME_INTRA)
			{
				bFirstIPacket = TRUE;
			}
		}

        //20150113 added by Charles for H.265
        //H265 only
        if(pMediaInfo->tStreamBuffer.wPacketCount == 0 && pMediaInfo->tStreamBuffer.dwStreamType == mctH265)
        {
            //Parse for information
			if(ChannelShmH265Info(&pMediaInfo->tStreamBuffer, (BYTE *)pchBitBuffPtr, &pMediaInfo->tStreamBuffer.tH265overRTPInfo, pMediaInfo->tStreamBuffer.pdwPacketSize[pMediaInfo->tStreamBuffer.tH265overRTPInfo.iNALUIndex], &dwStreamStartOffset, pRTPMediaBuff->dwBufferLength - dwExtLen) != S_OK)
			{
				ChannelShmResetMediaInfo(pMediaInfo);
				return S_FAIL;
			}
            if(pMediaInfo->tStreamBuffer.tFrameType == MEDIADB_FRAME_INTRA)
			{
				bFirstIPacket = TRUE;
			}
        }

		//20081230 Compute the extra RTP payload length for all codecs
		if(pMediaInfo->tStreamBuffer.dwStreamType == mctMP4V)
		{
			dwExtraRTPPayload = 0;
		}
		else if(pMediaInfo->tStreamBuffer.dwStreamType == mctJPEG)
		{
			if(pMediaInfo->tStreamBuffer.wPacketCount == 0) //First packet must include Quantization table
			{
				dwExtraRTPPayload = 8 + 132; 
			}
			else
			{
				dwExtraRTPPayload = 8;
			}

			//20101210 Add by danny For support DRI header
			if (ptJPEGoverRTPInfo->dwDRI)
			{
				dwExtraRTPPayload += 4;
			}
		}
		else if(pMediaInfo->tStreamBuffer.dwStreamType == mctH264)
		{
			if(pMediaInfo->tStreamBuffer.tH264overRTPInfo.ePacketFragMode == ESingleNALU)	//No extra payload header needed
			{
				dwExtraRTPPayload = 0;
			}
			else if(pMediaInfo->tStreamBuffer.tH264overRTPInfo.ePacketFragMode == EFragmentationNALU) //Need to add fragmentation unit
			{
				dwExtraRTPPayload = 2;
			}
		}
        else if(pMediaInfo->tStreamBuffer.dwStreamType == mctH265)
        {
            if(pMediaInfo->tStreamBuffer.tH265overRTPInfo.ePacketFragMode == EH265SingleNALU)	//No extra payload header needed
			{
				dwExtraRTPPayload = 0;
			}
			else if(pMediaInfo->tStreamBuffer.tH265overRTPInfo.ePacketFragMode == EH265FragmentationNALU) //Need to add fragmentation unit
			{
				dwExtraRTPPayload = 3;
			}
        }
        
		if(pMediaInfo->tStreamBuffer.wPacketCount == 0)
		{
			//Only the first RTP packet will put the RTPExtra in Aggregate buffer. It will not be counted as header length.
			pRTPMediaBuff->dwRTPExtraData = dwExtraRTPPayload + dwExtLen;
			pRTPMediaBuff->iHeaderLength = 0;
		}
		else
		{
			//For other packets the RTPExtra will be counted in the header length. It will be placed after the header!
			pRTPMediaBuff->dwRTPExtraData = dwExtLen;
			pRTPMediaBuff->iHeaderLength = dwExtraRTPPayload;
		}

		//Compute the possible length of data to be copied, 20081229 modified according to video codec type
		dwTmp = pRTPMediaBuff->dwBufferLength - dwExtLen - dwExtraRTPPayload;
		if (dwTmp > (dwRemPacketSize - dwStreamStartOffset))
		{
			dwTmp = dwRemPacketSize - dwStreamStartOffset;	//20090325
		}
		//printf("dwTmp is %d dwBufflen %d dwextlen %d stream offset %d, RTP extra %d\n", dwTmp, pRTPMediaBuff->dwBufferLength, dwExtLen, dwStreamStartOffset, dwExtraRTPPayload);	

		//Copy extension, now we will copy the aggregate buffer
		pRTPMediaBuff->dwExtensionLen = dwExtLen;
		pbyHeaderBuffer = pMediaInfo->tAggreMediaBuffer.pbyRTPExtraData;
		if (dwExtLen != 0)
		{
			memcpy(pbyHeaderBuffer, abyRTPExt, dwExtLen);
			pbyHeaderBuffer += dwExtLen;
		}	

		//Now we need to decide where to put the RTP extra data. For first packet of MJPEG it is placed at the pbyRTPextra in aggregate buffer
		//For other packet it is placed in acPadBuffer after the TCP embedded info & RTP header
		if(pMediaInfo->tStreamBuffer.wPacketCount == 0)
		{
			pbyRTPExtraStart = pbyHeaderBuffer;
		}
		else
		{
			if(pRTPMediaBuff->dwExtensionLen > 0)
			{
				pbyRTPExtraStart = pRTPMediaBuff->pbDataStart + 16;
			}
			else
			{
				pbyRTPExtraStart = pRTPMediaBuff->pbDataStart + 12;
			}
		}


		//20081229 JPEG must compose RFC 2435 header and Quantization-table
		if(pMediaInfo->tStreamBuffer.dwStreamType == mctJPEG)
		{
			//RFC 2435 header
			ChannelShmComposeRFC2435Header(&pMediaInfo->tStreamBuffer, pbyRTPExtraStart, bFirstIPacket);
		}

		//Copy data, we need to set the correct pointer and mark the length
		pRTPMediaBuff->pbBufferStart = (BYTE *)pchBitBuffPtr + dwStreamStartOffset;
		pRTPMediaBuff->dwBitStreamLength = dwTmp;

		//Update dwBytesUsed parameters in RTP, include header + extension + RTP extra + bitstream length, but missing TCP embed info
		if(dwExtLen > 0)
		{
			pRTPMediaBuff->dwBytesUsed = pRTPMediaBuff->dwBitStreamLength + pRTPMediaBuff->dwExtensionLen + dwExtraRTPPayload + 16;
		}
		else
		{
			pRTPMediaBuff->dwBytesUsed = pRTPMediaBuff->dwBitStreamLength + pRTPMediaBuff->dwExtensionLen + dwExtraRTPPayload + 12;
		}

		//Increment packet count / current position / remaining
		pMediaInfo->tStreamBuffer.wPacketCount++;
		pMediaInfo->tStreamBuffer.dwCurrentPosition += (dwTmp + dwStreamStartOffset);	//dwExtraRTPPayload must not be counted
		pRTPMediaBuff->dwRemainingLength = pRTPMediaBuff->dwBytesUsed;				
		pRTPMediaBuff->bHeaderComposed = FALSE;
		
		//Decrement on totoal remaining size of a frame
		pMediaInfo->iRemainingSize -= (dwTmp+ dwStreamStartOffset);						//Remaining size calculation involves only entropy data

		//Calculate total size for aggregate buffer
		pMediaInfo->tAggreMediaBuffer.dwTotalSize += pRTPMediaBuff->dwBytesUsed;	

		if(pMediaInfo->iRemainingSize < 0)
		{
			//printf("dwtmp %d streamoffset %d position %d remaining %d\n", dwTmp,dwStreamStartOffset, pMediaInfo->tStreamBuffer.dwCurrentPosition, pRTPMediaBuff->dwRemainingLength);
			printf("Panic! Incorrect packet cut %d\n", pMediaInfo->iRemainingSize);
			syslog(LOG_ERR, "[MediaChannel]: Incorrect Packet Cut %d\n", pMediaInfo->iRemainingSize);
			
			ChannelShmResetMediaInfo(pMediaInfo);
			return S_FAIL;
		}
		if(pMediaInfo->tStreamBuffer.dwCurrentPosition > pMediaInfo->tStreamBuffer.dwBytesUsed)
		{
			printf("Panic! Incorrect current position caused by cut: %d max %d\n", pMediaInfo->tStreamBuffer.dwCurrentPosition, pMediaInfo->tStreamBuffer.dwBytesUsed);
			syslog(LOG_ERR, "[MediaChannel]: Incorrect current position caused by cut: %d\n", pMediaInfo->tStreamBuffer.dwCurrentPosition);
			
			ChannelShmResetMediaInfo(pMediaInfo);
			return S_FAIL;
		}
		
		/*printf("Cut %u packets dwBytesUsed %u with bitstreamsize %u Extension %u RTPExtra data %u whole frame size %u Remaining is %u\n", 
			iBufferCount, 
			pRTPMediaBuff->dwBytesUsed, 
			pRTPMediaBuff->dwBitStreamLength, 
			dwExtLen,
			pRTPMediaBuff->dwRTPExtraData,
			pMediaInfo->tStreamBuffer.dwBytesUsed, 
			pMediaInfo->iRemainingSize);*/
		

		//20130603 added by Jimmy to follow RFC 3551, only set the marker bit in the first audio packet
		if (pMediaInfo->tStreamBuffer.dwStreamType == mctG711A ||
			pMediaInfo->tStreamBuffer.dwStreamType == mctG711U ||
			pMediaInfo->tStreamBuffer.dwStreamType == mctG726 ||
			pMediaInfo->tStreamBuffer.dwStreamType == mctGAMR)
		{
			if(pMediaInfo->bFirstAudioPacketized)
			{
				pRTPMediaBuff->bMarker = 0;
			}
			else
			{
				pMediaInfo->bFirstAudioPacketized = TRUE;
				pRTPMediaBuff->bMarker = 1;
			}
		}
		//Check for Boundary
		else if(pMediaInfo->iRemainingSize == 0)
		{
			pRTPMediaBuff->bMarker = 1;
			if(pMediaInfo->tStreamBuffer.dwStreamType == mctH264 && (pMediaInfo->tStreamBuffer.tH264overRTPInfo.iNALUIndex < pMediaInfo->tStreamBuffer.pdwPacketSize[0]))
			{
				//20120716 danny, modified H264 RTP marker bit according to RFC3984
				pRTPMediaBuff->bMarker = 0;
				//Proceed to next NALU!
				pMediaInfo->tStreamBuffer.tH264overRTPInfo.iNALUIndex++;
				//20090325 NALU length must be included in remaining length
				pMediaInfo->iRemainingSize = pMediaInfo->tStreamBuffer.pdwPacketSize[pMediaInfo->tStreamBuffer.tH264overRTPInfo.iNALUIndex] + 4;
				pMediaInfo->tStreamBuffer.wPacketCount = 0;
			}
            else if(pMediaInfo->tStreamBuffer.dwStreamType == mctH265 && (pMediaInfo->tStreamBuffer.tH265overRTPInfo.iNALUIndex < pMediaInfo->tStreamBuffer.pdwPacketSize[0]))
            {
				pRTPMediaBuff->bMarker = 0;
				//Proceed to next NALU!
				pMediaInfo->tStreamBuffer.tH265overRTPInfo.iNALUIndex++;
				//20090325 NALU length must be included in remaining length
				pMediaInfo->iRemainingSize = pMediaInfo->tStreamBuffer.pdwPacketSize[pMediaInfo->tStreamBuffer.tH265overRTPInfo.iNALUIndex] + 4;
				pMediaInfo->tStreamBuffer.wPacketCount = 0;
            }
			//20091228 Fixed TCP broken image, avoid release Shmem before send out completely 
			/*else
			{
				SharedMem_ReleaseReadBuffer(pMediaInfo->hClientBuf);
			}*/
		}
		else
		{
			pRTPMediaBuff->bMarker = 0;
		}

		//20090108 H.264 Fragmentation Header, it is placed here because we need to know if this is the first/last RTP packet
		if(pMediaInfo->tStreamBuffer.dwStreamType == mctH264 && pMediaInfo->tStreamBuffer.tH264overRTPInfo.ePacketFragMode == EFragmentationNALU)
		{
			ChannelShmComposeH264FU_A(&pMediaInfo->tStreamBuffer, pbyRTPExtraStart, pMediaInfo->tStreamBuffer.wPacketCount, pRTPMediaBuff->bMarker);
		}
        //20150113 added by Charles for H.265
        if(pMediaInfo->tStreamBuffer.dwStreamType == mctH265 && pMediaInfo->tStreamBuffer.tH265overRTPInfo.ePacketFragMode == EH265FragmentationNALU)
		{
			ChannelShmComposeH265FU_A(&pMediaInfo->tStreamBuffer, pbyRTPExtraStart, pMediaInfo->tStreamBuffer.wPacketCount, pRTPMediaBuff->bMarker);
		}

		//Check for first packet in I-frame
		if(bFirstIPacket)
		{
			pRTPMediaBuff->bIFrame = 1; 
			bFirstIPacket = FALSE;
		}
		else
		{
			pRTPMediaBuff->bIFrame = 0; 
		}
	               
		pRTPMediaBuff->ulSeconds = pMediaInfo->tStreamBuffer.dwSecond;
		pRTPMediaBuff->ulMSeconds= pMediaInfo->tStreamBuffer.dwMilliSecond;
		pRTPMediaBuff->dwStreamIndex = pMediaInfo->tStreamBuffer.dwStreamIndex;

		//Reset some settings for next round
		dwExtraRTPPayload = 0;
		dwStreamStartOffset = 0;
		dwRemPacketSize = 0;
		dwTmp = 0; 
		dwExtLen = 0;
		bFirstIPacket = FALSE;  
		iBufferCount++;
	}

	//20081001 TCP Timeout
	OSTick_GetMSec(&pMediaInfo->tAggreMediaBuffer.dwBaseTime);
	// printf("-------------------%s------------------%u\n", __func__, pRTPMediaBuff->ulSeconds);
	return S_OK;
}

SCODE  ChannelShmemRequestBuffer(CHANNEL *pChannel)
{
    int i;

	for(i=0 ; i <pChannel->iMaxSession ; i ++ )
	{
     	if(pChannel->session[i].dwSessionID != 0 &&
		   pChannel->session[i].iStatus != SESSION_PAUSED)
		{
			//20091228 Avoid TCP mode, callback to Shmem request buffer before send out completely
			if(pChannel->session[i].ptShmemMediaInfo->iRemainingSize == 0 && pChannel->session[i].ptShmemMediaInfo->bFrameGenerated
				&& pChannel->session[i].ptShmemMediaInfo->tAggreMediaBuffer.dwTotalRemaining == 0 )
			{
				if(pChannel->fCallbackFunct(pChannel->hParentHandle
		                      ,RTPRTCPCHANNEL_CALLBACKFLAG_SHMEM_GETBUF
		  				   ,(void *)pChannel->session[i].ptShmemMediaInfo, NULL) == 0 )
				{
					//printf("Bitstreambuf receivied size = %d!\n", pChannel->session[i].ptShmemMediaInfo->tStreamBuffer.dwBytesUsed);
					pChannel->session[i].ptShmemMediaInfo->dwTimeoutInitial = 0;
				}
				else	//20090403 record if there is no frame for a long time
				{
					if(pChannel->session[i].ptShmemMediaInfo->dwTimeoutInitial == 0)
					{
						OSTick_GetMSec(&pChannel->session[i].ptShmemMediaInfo->dwTimeoutInitial);
					}
					else
					{
						DWORD	dwNow = 0, dwOffset = 0;
						OSTick_GetMSec(&dwNow);
						dwOffset = rtspCheckTimeDifference(pChannel->session[i].ptShmemMediaInfo->dwTimeoutInitial, dwNow);
						if(dwOffset > 5000 && pChannel->nChannelType != RTPRTCPCHANNEL_MEDIATYPE_METADATA) //20160715 metadata connot get data is normal
						{
							printf("Share memory sessionID %u query fail for 5 seconds!\n",pChannel->session[i].dwSessionID);
							syslog(LOG_DEBUG, "Share memory sessionID %u query fail for 5 seconds!\n", pChannel->session[i].dwSessionID);
							pChannel->session[i].ptShmemMediaInfo->dwTimeoutInitial = 0;
						}
					}
				}	
			}
		}
	}
#ifdef RTSPRTP_MULTICAST
	//20130327 modified by Jimmy to support different video/audio multicast addresses for ONVIF Test Tool 12.12
	for( i=0 ; i< (RTSP_MULTICASTNUMBER + RTSP_ONDEMAND_MULTICASTNUMBER) ; i++)
	{	    
		if(pChannel->stMulticast[i].iStatus == SESSION_PLAYING)
		{
			//20091228 Avoid TCP mode, callback to Shmem request buffer before send out completely
			if(pChannel->stMulticast[i].ptShmemMediaInfo->iRemainingSize == 0 && pChannel->stMulticast[i].ptShmemMediaInfo->bFrameGenerated
				&& pChannel->stMulticast[i].ptShmemMediaInfo->tAggreMediaBuffer.dwTotalRemaining == 0 )
			{
				if(pChannel->fCallbackFunct(pChannel->hParentHandle
							,RTPRTCPCHANNEL_CALLBACKFLAG_SHMEM_GETBUF
	  					,(void *)pChannel->stMulticast[i].ptShmemMediaInfo, NULL) == 0 )
				{
					//printf("Bitstreambuf receivied size = %d!\n", pChannel->stMulticast[i].ptShmemMediaInfo->tStreamBuffer.dwBytesUsed);
					pChannel->stMulticast[i].ptShmemMediaInfo->dwTimeoutInitial = 0;
				}
				else	//20090403 record if there is no frame for a long time
				{
					if(pChannel->stMulticast[i].ptShmemMediaInfo->dwTimeoutInitial == 0)
					{
						OSTick_GetMSec(&pChannel->stMulticast[i].ptShmemMediaInfo->dwTimeoutInitial);
					}
					else
					{
						DWORD	dwNow = 0, dwOffset = 0;
						OSTick_GetMSec(&dwNow);
						dwOffset = rtspCheckTimeDifference(pChannel->stMulticast[i].ptShmemMediaInfo->dwTimeoutInitial, dwNow);
						if(dwOffset > 5000 && pChannel->nChannelType != RTPRTCPCHANNEL_MEDIATYPE_METADATA) //20160715 metadata connot get data is normal
						{
							// printf("Share memory multicastID %d query fail for 5 seconds!\n", i);
							syslog(LOG_DEBUG, "Share memory multicastID %d query fail for 5 seconds!\n", i);
							pChannel->stMulticast[i].ptShmemMediaInfo->dwTimeoutInitial = 0;
						}
					}
				}	
			}	
		}
	}
#endif
		return S_OK;
}

BOOL  ChannelShmemIsHeaderComposed(TAggregateMediaBuffer *pAggreBuffer)
{
	//We assume that all header will be composed together, so we only need to check the first RTP
	if(pAggreBuffer->ptRTPBuffer[0].bHeaderComposed)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

SCODE  ChannelShmemCutBuffer(CHANNEL *pChannel)
{
	int	i = 0;

	for(i=0 ; i <pChannel->iMaxSession ; i ++ )
	{
		if(pChannel->session[i].dwSessionID != 0)
		{
			TShmemMediaInfo	*ptShmemMediaInfo = pChannel->session[i].ptShmemMediaInfo;

			if(ptShmemMediaInfo->tAggreMediaBuffer.dwTotalRemaining == 0 &&
				ptShmemMediaInfo->iRemainingSize != 0 &&
				ptShmemMediaInfo->tStreamBuffer.dwBytesUsed != 0)
			{
				//20101018 Add by danny for support multiple channel text on video
				ChannelShmPacketize(pChannel, ptShmemMediaInfo, pChannel->session[i].iVivotekClient, pChannel->session[i].iMultipleChannelChannelIndex, pChannel->session[i].iCSeq, &pChannel->session[i].iCSeqUpdated);
			}
		}
	}
#ifdef RTSPRTP_MULTICAST
	//20130327 modified by Jimmy to support different video/audio multicast addresses for ONVIF Test Tool 12.12
	//20130904 modified by Charles for ondemand multicast
	for( i=0 ; i< (RTSP_MULTICASTNUMBER + RTSP_ONDEMAND_MULTICASTNUMBER) ; i++)
	{	    
		if(pChannel->stMulticast[i].iStatus == SESSION_PLAYING)
		{
			TShmemMediaInfo	*ptShmemMediaInfo = pChannel->stMulticast[i].ptShmemMediaInfo;

			if(ptShmemMediaInfo->tAggreMediaBuffer.dwTotalRemaining == 0 &&
				ptShmemMediaInfo->iRemainingSize != 0 &&
				ptShmemMediaInfo->tStreamBuffer.dwBytesUsed != 0)
			{
				//20101018 Add by danny for support multiple channel text on video
				ChannelShmPacketize(pChannel, ptShmemMediaInfo, pChannel->stMulticast[i].iVivotekClient, pChannel->stMulticast[i].iMultipleChannelChannelIndex, pChannel->stMulticast[i].iCSeq, &pChannel->stMulticast[i].iCSeqUpdated);
			}
		}
	}
#endif
	return S_OK;
}

#ifdef RTSPRTP_MULTICAST
SCODE  ChannelShmemMulticastComposeHeader(CHANNEL *pChannel)
{
	int	i = 0;
	//20130327 modified by Jimmy to support different video/audio multicast addresses for ONVIF Test Tool 12.12
	//20130904 modified by Charles for ondemand multicast
	for( i=0 ; i< (RTSP_MULTICASTNUMBER + RTSP_ONDEMAND_MULTICASTNUMBER) ; i++)
	{	    
		if(pChannel->stMulticast[i].iStatus == SESSION_PLAYING && !ChannelShmemIsHeaderComposed(&pChannel->stMulticast[i].ptShmemMediaInfo->tAggreMediaBuffer))
		{
			int		iCount = 0;

			for(iCount = 0; iCount < pChannel->stMulticast[i].ptShmemMediaInfo->tAggreMediaBuffer.iBufferNumber; iCount++)
			{
				RTPMEDIABUFFER *pMediaBuffer = &(pChannel->stMulticast[i].ptShmemMediaInfo->tAggreMediaBuffer.ptRTPBuffer[iCount]);
				HANDLE hRTPRTCPComposerHandle = pChannel->stMulticast[i].hRTPRTCPComposerHandle;

				if(pMediaBuffer->dwRemainingLength > 0 && (!pMediaBuffer->bHeaderComposed))
				{

					RTPRTCPComposer_RTPHeaderComposer(hRTPRTCPComposerHandle,pMediaBuffer);

					//Update the remaining length to be sent
					pMediaBuffer->dwRemainingLength = pMediaBuffer->dwBytesUsed;

					if( pMediaBuffer->dwExtensionLen > 0 )
						pMediaBuffer->iHeaderLength += (12 + 4);
					else
						pMediaBuffer->iHeaderLength += 12 ;
					
					//Set the marker
					pMediaBuffer->bHeaderComposed = TRUE;
				}
			}
			pChannel->stMulticast[i].ptShmemMediaInfo->tAggreMediaBuffer.dwTotalRemaining = pChannel->stMulticast[i].ptShmemMediaInfo->tAggreMediaBuffer.dwTotalSize;
		}
	}
	
	return S_OK;
}
#endif

SCODE  ChannelShmemComposeHeader(CHANNEL *pChannel)
{
	int i = 0, iForceI = 0;

	for(i=0 ; i <pChannel->iMaxSession ; i ++ )
	{
		if(pChannel->session[i].dwSessionID != 0 && 
			pChannel->session[i].iStatus != SESSION_PAUSED &&
			!ChannelShmemIsHeaderComposed(&pChannel->session[i].ptShmemMediaInfo->tAggreMediaBuffer))
		{
			int		iCount = 0;

			//20110711 Modified by danny For Avoid frame type not match CI
			int		iCodecIndex = pChannel->session[i].iCodecIndex;
			DWORD 	dwFrameType = pChannel->session[i].ptShmemMediaInfo->tStreamBuffer.dwStreamType;
			
			if( pChannel->fCallbackFunct(pChannel->hParentHandle, RTPRTCPCHANNEL_CALLBACKFLAG_CHECK_FRAMETYPE_MATCH_CI, (void*)iCodecIndex, (void*)dwFrameType) != S_OK )
			{
				continue;
			}
			
			for(iCount = 0; iCount < pChannel->session[i].ptShmemMediaInfo->tAggreMediaBuffer.iBufferNumber; iCount ++)
			{
				RTPMEDIABUFFER *pMediaBuffer = &(pChannel->session[i].ptShmemMediaInfo->tAggreMediaBuffer.ptRTPBuffer[iCount]);
				RTPRTCPCHANNEL_SESSION *pSession = &pChannel->session[i];

				if(pMediaBuffer->dwRemainingLength > 0 && (!pMediaBuffer->bHeaderComposed))
				{
					// make sure the first packet of video is I frame
					//20130725 modified by Jimmy to avoid sending audio RTCP before first RTP packet
					if(pMediaBuffer->bIFrame == 1)    
					{
						//mark this session is started!
						RTPRTCPComposer_SetValidate(pSession->hRTPRTCPComposerHandle);
					}

					// Do not send out P frame if no I frame is generated
					if( RTPRTCPComposer_GetValidate(pSession->hRTPRTCPComposerHandle) == 0 
						&& pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_VIDEO )
					{
						if( iForceI%10 == 0)     
						{
							pChannel->fCallbackFunct(pChannel->hParentHandle,RTPRTCPCHANNEL_CALLBACKFLAG_FORCE_I_FRAME,(void*)pSession->iCodecIndex,0);            
						}     
						iForceI ++;
						//20080918 Discard current frame and RTP packet
						ChannelShmResetMediaInfo(pSession->ptShmemMediaInfo);
						return 0;
					}

					RTPRTCPComposer_RTPHeaderComposer(pSession->hRTPRTCPComposerHandle,pMediaBuffer);

					if( pSession->iRTPStreamingType == RTP_OVER_TCP )
					{
						RTPRTCPChannel_ComposeEmbeddedRTPInfo((char*)pMediaBuffer->pbDataStart, pSession->iEmbeddedRTPID, pMediaBuffer->dwBytesUsed);
						pMediaBuffer->dwBytesUsed = pMediaBuffer->dwBytesUsed + 4 ;
						pMediaBuffer->pbDataStart = pMediaBuffer->pbDataStart - 4 ;
						//Writev aggregate buffer	
						pChannel->session[i].ptShmemMediaInfo->tAggreMediaBuffer.dwTotalSize += 4;
					}
					//Update the remaining length to be sent
					pMediaBuffer->dwRemainingLength = pMediaBuffer->dwBytesUsed;

					if( pSession->iRTPStreamingType == RTP_OVER_TCP )
					{
						if( pMediaBuffer->dwExtensionLen > 0 )
							pMediaBuffer->iHeaderLength += (12 + 4 + 4);
						else
							pMediaBuffer->iHeaderLength += (12 + 4);
					}
					else
					{
						if( pMediaBuffer->dwExtensionLen > 0 )
							pMediaBuffer->iHeaderLength += (12 + 4);
						else
							pMediaBuffer->iHeaderLength += 12 ;
					}
					//Set the marker
					pMediaBuffer->bHeaderComposed = TRUE;
				}
			}
			//Writev assign the total remaining size to be the total buffer size (Everything)
			pChannel->session[i].ptShmemMediaInfo->tAggreMediaBuffer.dwTotalRemaining = pChannel->session[i].ptShmemMediaInfo->tAggreMediaBuffer.dwTotalSize;
		}
	}

	return S_OK;
}

SCODE  ChannelShmemUDPFillIovStructure(struct iovec *ptIov, TShmemMediaInfo *ptMediaInfo, int *piIovNumber, int *piRTPNumber)
{
	int					iCount = 0, iRTPBufferNumber = 0, iIOVBufferNumber = 0;
	RTPMEDIABUFFER		*pRTPMediaBuff = NULL;


	iCount = ptMediaInfo->tAggreMediaBuffer.iProcessRTPIndex;
	//printf("[%s] ProcessRTPIndex = %d\n", __FUNCTION__, iCount);
	while(iCount < ptMediaInfo->tAggreMediaBuffer.iBufferNumber)		//FIXME: maybe we don't need to traverse all buffer, just non-empty ones
	{
		pRTPMediaBuff = &(ptMediaInfo->tAggreMediaBuffer.ptRTPBuffer[iCount]);
		
		if(pRTPMediaBuff->dwRemainingLength > 0)
		{
			iRTPBufferNumber++;

			//Fill in header
			ptIov[iIOVBufferNumber].iov_base =  pRTPMediaBuff->pbDataStart;
			ptIov[iIOVBufferNumber].iov_len =	pRTPMediaBuff->iHeaderLength;
			iIOVBufferNumber++;

			/*
			printf("Count %d Headerlength %d offset %d @ -%02x-%02x-%02x-%02x-\n", iCount, pRTPMediaBuff->iHeaderLength , iOffset, 
			*((BYTE *)ptIov[iIOVBufferNumber - 1].iov_base+0), 
			*((BYTE *)ptIov[iIOVBufferNumber - 1].iov_base+1), 
			*((BYTE *)ptIov[iIOVBufferNumber - 1].iov_base+2), 
			*((BYTE *)ptIov[iIOVBufferNumber - 1].iov_base+3));
			*/

			//Fill in the Extra data
			if(pRTPMediaBuff->dwRTPExtraData > 0)
			{
				ptIov[iIOVBufferNumber].iov_base =  ptMediaInfo->tAggreMediaBuffer.pbyRTPExtraData;
				ptIov[iIOVBufferNumber].iov_len =	pRTPMediaBuff->dwRTPExtraData;
				iIOVBufferNumber++;
			}

			//Fill in the bitstream
			ptIov[iIOVBufferNumber].iov_base =  pRTPMediaBuff->pbBufferStart;
			ptIov[iIOVBufferNumber].iov_len =	pRTPMediaBuff->dwBitStreamLength;
			iIOVBufferNumber++;
/*
			printf("Count %d IOV %d bitstream length %d Memaddress %p @-%02x-%02x-%02x-%02x-\n", iCount, iIOVBufferNumber - 1, 
			ptIov[iIOVBufferNumber - 1].iov_len, ptIov[iIOVBufferNumber - 1].iov_base, 
			*((BYTE *)ptIov[iIOVBufferNumber - 1].iov_base+0), 
			*((BYTE *)ptIov[iIOVBufferNumber - 1].iov_base+1), 
			*((BYTE *)ptIov[iIOVBufferNumber - 1].iov_base+2), 
			*((BYTE *)ptIov[iIOVBufferNumber - 1].iov_base+3));
*/

			//printf("A Remaining %u Bitstream %u Extra %u Offset %d\n", pRTPMediaBuff->dwRemainingLength, pRTPMediaBuff->dwBitStreamLength, pRTPMediaBuff->dwRTPExtraData, iOffset);
		
			//UDP will only write one RTP at a time
			break;
		}
		iCount++;
	}

	*piIovNumber = iIOVBufferNumber;
	*piRTPNumber = iRTPBufferNumber;
	
	//printf("Writev IOV with %d buffer %d RTP buffer %d total size\n", iIOVBufferNumber, iRTPBufferNumber, ptMediaInfo->tAggreMediaBuffer.dwTotalSize);

	return S_OK;
}

SCODE  ChannelShmemFillIovStructure(struct iovec *ptIov, TShmemMediaInfo *ptMediaInfo, int *piIovNumber, int *piRTPNumber)
{
	int					iCount = 0, iOffset = 0, iRTPBufferNumber = 0, iIOVBufferNumber = 0;
	RTPMEDIABUFFER		*pRTPMediaBuff = NULL;


	iCount = ptMediaInfo->tAggreMediaBuffer.iProcessRTPIndex;
	//printf("[%s] ProcessRTPIndex = %d\n", __FUNCTION__, iCount);
	while(iCount < ptMediaInfo->tAggreMediaBuffer.iBufferNumber)		//FIXME: maybe we don't need to traverse all buffer, just non-empty ones
	{
		pRTPMediaBuff = &(ptMediaInfo->tAggreMediaBuffer.ptRTPBuffer[iCount]);
		
		if(pRTPMediaBuff->dwRemainingLength > 0)
		{
			iRTPBufferNumber++;

			if((iOffset = pRTPMediaBuff->dwRemainingLength - (pRTPMediaBuff->dwBitStreamLength + pRTPMediaBuff->dwRTPExtraData)) > 0)
			{
				//We are only half way thorugh the header now
				//Fill in the rest of the header first
				ptIov[iIOVBufferNumber].iov_base =  pRTPMediaBuff->pbDataStart + (pRTPMediaBuff->iHeaderLength - iOffset);
				ptIov[iIOVBufferNumber].iov_len =	iOffset;
				iIOVBufferNumber++;

				/*
				printf("Count %d Headerlength %d offset %d @ -%02x-%02x-%02x-%02x-\n", iCount, pRTPMediaBuff->iHeaderLength , iOffset, 
				*((BYTE *)ptIov[iIOVBufferNumber - 1].iov_base+0), 
				*((BYTE *)ptIov[iIOVBufferNumber - 1].iov_base+1), 
				*((BYTE *)ptIov[iIOVBufferNumber - 1].iov_base+2), 
				*((BYTE *)ptIov[iIOVBufferNumber - 1].iov_base+3));
				*/

				//Fill in the Extra data if such data is present
				if(pRTPMediaBuff->dwRTPExtraData > 0)
				{
					ptIov[iIOVBufferNumber].iov_base =  ptMediaInfo->tAggreMediaBuffer.pbyRTPExtraData;
					ptIov[iIOVBufferNumber].iov_len =	pRTPMediaBuff->dwRTPExtraData;
					iIOVBufferNumber++;
					//printf("A dwRTPExtraData %u\n", pRTPMediaBuff->dwRTPExtraData);
				}

				//Fill in the bitstream
				ptIov[iIOVBufferNumber].iov_base =  pRTPMediaBuff->pbBufferStart;
				ptIov[iIOVBufferNumber].iov_len =	pRTPMediaBuff->dwBitStreamLength;
				iIOVBufferNumber++;
				//printf("A Remaining %u Bitstream %u Extra %u Offset %d\n", pRTPMediaBuff->dwRemainingLength, pRTPMediaBuff->dwBitStreamLength, pRTPMediaBuff->dwRTPExtraData, iOffset);
			}
			else if((iOffset = pRTPMediaBuff->dwRemainingLength - pRTPMediaBuff->dwBitStreamLength) > 0)
			{
				//We are only half way thorugh the extension and RTPextra now.
				//Fill in the rest of the extension
				ptIov[iIOVBufferNumber].iov_base =  ptMediaInfo->tAggreMediaBuffer.pbyRTPExtraData + (pRTPMediaBuff->dwRTPExtraData - iOffset);
				ptIov[iIOVBufferNumber].iov_len =	iOffset;
				iIOVBufferNumber++;
				//Fill in the bitstream
				ptIov[iIOVBufferNumber].iov_base =  pRTPMediaBuff->pbBufferStart;
				ptIov[iIOVBufferNumber].iov_len =	pRTPMediaBuff->dwBitStreamLength;
				iIOVBufferNumber++;
				//printf("B Remaining %d Bitstream %d Extra %d offset %d\n", pRTPMediaBuff->dwRemainingLength, pRTPMediaBuff->dwBitStreamLength, pRTPMediaBuff->dwRTPExtraData, iOffset);
			}
			else
			{
				//We are half way through the bitstream
				//Fill in the rest of bitstream
				ptIov[iIOVBufferNumber].iov_base =  pRTPMediaBuff->pbBufferStart + (pRTPMediaBuff->dwBitStreamLength - pRTPMediaBuff->dwRemainingLength);
				ptIov[iIOVBufferNumber].iov_len =	pRTPMediaBuff->dwRemainingLength;
				iIOVBufferNumber++;
				//printf("C Remaining %d Bitstream %d Extra %d\n", pRTPMediaBuff->dwRemainingLength, pRTPMediaBuff->dwBitStreamLength, pRTPMediaBuff->dwRTPExtraData);
			}
		}

		iCount++;
	}

	*piIovNumber = iIOVBufferNumber;
	*piRTPNumber = iRTPBufferNumber;
	
	//printf("Writev IOV with %d buffer %d RTP buffer %d total size\n", iIOVBufferNumber, iRTPBufferNumber, ptMediaInfo->tAggreMediaBuffer.dwTotalSize);

	return S_OK;
}

SCODE  ChannelShmemCalculateRemainingSize(TShmemMediaInfo *ptMediaInfo, int iSendSize)
{
	int					iCount = 0, iSent = iSendSize;
	RTPMEDIABUFFER		*pRTPMediaBuff = NULL;
	
	//Check the first RTP packet to see if extension is already sent !
	pRTPMediaBuff = &(ptMediaInfo->tAggreMediaBuffer.ptRTPBuffer[0]);
		
	if(pRTPMediaBuff->dwRTPExtraData > 0)
	{
		if(pRTPMediaBuff->dwRemainingLength > 0 && pRTPMediaBuff->dwRemainingLength < pRTPMediaBuff->dwBitStreamLength)
		{
			pRTPMediaBuff->dwRTPExtraData = 0;
		}
	}

	while(iSent > 0 && iCount < ptMediaInfo->tAggreMediaBuffer.iBufferNumber)
	{
		pRTPMediaBuff = &(ptMediaInfo->tAggreMediaBuffer.ptRTPBuffer[iCount]);

		if(pRTPMediaBuff->dwRemainingLength > iSent)
		{
			pRTPMediaBuff->dwRemainingLength = pRTPMediaBuff->dwRemainingLength - iSent;
			iSent = 0;
		}
		else
		{
			//This RTP packet is fully sent
			iSent = iSent - pRTPMediaBuff->dwRemainingLength;
			pRTPMediaBuff->dwRemainingLength = 0;
			pRTPMediaBuff->dwBytesUsed = 0;
			//Restore the RTP architecture
			pRTPMediaBuff->pbDataStart = (BYTE *)pRTPMediaBuff->acPadBuffer + 4;
		}

		iCount++;
	}
	
	ptMediaInfo->tAggreMediaBuffer.iProcessRTPIndex = iCount - 1;
	//printf("[%s] ProcessRTPIndex %d\n", __FUNCTION__, ptMediaInfo->tAggreMediaBuffer.iProcessRTPIndex);
	return S_OK;
}

void faber_debug(CHANNEL *pChannel)
{
	printf("%s\n", __func__);
}
SCODE  ChannelShmemSend(CHANNEL *pChannel)
{
	int					i = 0, iResult = 0, iMaxSck = 0;
	int					j = 0;
    fd_set				fdsConnect;
	TShmemSelectInfo	tSelectInfo;

	//Zero the fdset
	FD_ZERO(&fdsConnect);
	// faber_debug(pChannel);
	//Add socket to select
	for(i=0 ; i <pChannel->iMaxSession ; i ++ )
	{
		if(pChannel->session[i].dwSessionID != 0 && 
			pChannel->session[i].iStatus != SESSION_PAUSED)
		{
			RTPRTCPCHANNEL_SESSION		*pSession = &pChannel->session[i];
			int							sSocket = INVALID_SOCKET;

			if(pChannel->session[i].ptShmemMediaInfo->tAggreMediaBuffer.dwTotalRemaining > 0)
			{
				//20080905 check if critical section is cleared for TCP
				if( pSession->iRTPStreamingType == RTP_OVER_TCP )
				{
					if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_AUDIO)
					{
						if(RTPRTCPCs_TryEnter(pSession, eCSAudioRTP) != S_OK)
						{
							continue;
						}
						else
						{
							//printf("[%s] @@@RTPRTCPCs_TryEnter AudioRTP %p ok@@@\n", __FUNCTION__, pSession->hTCPMuxCS);
						}
					}
					//20120816 modified by Jimmy for metadata
					else if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_VIDEO)
					{
						if(RTPRTCPCs_TryEnter(pSession, eCSVideoRTP) != S_OK)
						{
							continue;
						}
						else
						{
							//printf("[%s] @@@RTPRTCPCs_TryEnter VideoRTP %p ok@@@\n", __FUNCTION__, pSession->hTCPMuxCS);
						}
					}
					//20120816 added by Jimmy for metadata
					else if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_METADATA)
					{
						if(RTPRTCPCs_TryEnter(pSession, eCSMetadataRTP) != S_OK)
						{
							continue;
						}
						else
						{
							//printf("[%s] @@@RTPRTCPCs_TryEnter MetadataRTP %p ok@@@\n", __FUNCTION__, pSession->hTCPMuxCS);
						}
					}
					else
					{
						continue;
					}

				}
				if(pSession->idebug)
				{
					printf("%s %d, pChannel->nChannelType = %d\n", __func__, __LINE__, pChannel->nChannelType);
				}
				//20110706 Modified by danny For TCP/HTTP mode socket not sync in multi thread
				if( pSession->psktRTP != NULL && *pSession->psktRTP > 0 )
				{
					sSocket= *pSession->psktRTP;
				}
				else
				{
					sSocket= pChannel->iUDPRTPSock;
				}

				FD_SET(sSocket, &fdsConnect);
			
				if(sSocket > iMaxSck)
				{
					iMaxSck = sSocket;
				}
			}
		}
	}
	//Add Multicast socket
#ifdef RTSPRTP_MULTICAST
	//20130327 modified by Jimmy to support different video/audio multicast addresses for ONVIF Test Tool 12.12
	//20130904 modified by Charles for ondemand multicast
	for( i=0 ; i< (RTSP_MULTICASTNUMBER + RTSP_ONDEMAND_MULTICASTNUMBER) ; i++)
	{	    
		if( pChannel->stMulticast[i].iStatus == SESSION_PLAYING )
		{
			RTPRTCPCHANNEL_MULTICAST	*pMulticast = &pChannel->stMulticast[i];
			int							sSocket = INVALID_SOCKET;

			if(pChannel->stMulticast[i].ptShmemMediaInfo->tAggreMediaBuffer.dwTotalRemaining > 0)
			{
				sSocket = pMulticast->sktRTP;

				if(sSocket > 0)
				{
					FD_SET(sSocket, &fdsConnect);
			
					if(sSocket > iMaxSck)
					{
						iMaxSck = sSocket;
					}
				}
			}
		}
	}
#endif
	//Callback to do unix socket select
	tSelectInfo.iMaxSck = iMaxSck;
	tSelectInfo.iResult = 0;
	tSelectInfo.pfdsWrite = &fdsConnect;
	memset(tSelectInfo.aiNewFrame, 0, sizeof(tSelectInfo.aiNewFrame));

	if(pChannel->fCallbackFunct(pChannel->hParentHandle
		                       ,RTPRTCPCHANNEL_CALLBACKFLAG_SHMEM_SELECT
		  					   ,(void *)&tSelectInfo, NULL) != S_OK)
	{
		printf("mediachannel.c select error: %s!\n",strerror(errno));
		ChannelShmSelectError(pChannel);
		return S_FAIL;
	}

	iResult = tSelectInfo.iResult;

	//Send if selected
	if(iResult > 0)
	{
		for(i=0 ; i <pChannel->iMaxSession ; i ++ )
		{
			if(pChannel->session[i].dwSessionID != 0 && 
				pChannel->session[i].iStatus != SESSION_PAUSED)
			{
				RTPRTCPCHANNEL_SESSION		*pSession = &pChannel->session[i];
				int							sSocket = INVALID_SOCKET;
				int							iIovBufferNumber = 0;
				int							iRTPBufferNumber = 0;
				if(media_channel_idebug)
				{
					printf("%s mediatype %d, %d\n", __func__, pChannel->nChannelType, __LINE__);
					printf("pChannel->session[%d].ptShmemMediaInfo->tAggreMediaBuffer.dwTotalRemaining = %u\n", i, pChannel->session[i].ptShmemMediaInfo->tAggreMediaBuffer.dwTotalRemaining);
				}
				if(pChannel->session[i].ptShmemMediaInfo->tAggreMediaBuffer.dwTotalRemaining > 0)
				{	
					//20110706 Modified by danny For TCP/HTTP mode socket not sync in multi thread
					if( pSession->psktRTP != NULL && *pSession->psktRTP > 0 )
					{
						sSocket= *pSession->psktRTP;	
					}
					else
					{
						sSocket= pChannel->iUDPRTPSock;
					}
					if(media_channel_idebug)
					{
						ECritSecStatus lockstatus = eCSAudioRTP;
						RTPRTCPCs_GetLockStatus(pSession, &lockstatus);

						printf("session %d FD_ISSET(sSocket) = %d, lockstatus = %d\n", i, FD_ISSET(sSocket, &fdsConnect), lockstatus);
						
					}
					if(FD_ISSET(sSocket, &fdsConnect))
					{
						int				iSendSize = 0;
						//20110822 Add by danny for fix alignment trap, if H264 frame size too large cause atIov overflow
						struct iovec	atIov[RTSPS_VIDEO_RTPBUFFER_NUM * RTP_MAX_IOV_COMPONENT];

						//20091109 while loop to send all UDP RTP
						while(TRUE)
						{
							//Assign Buffer for writev
							if( pSession->iRTPStreamingType == RTP_OVER_TCP )
							{
								ChannelShmemFillIovStructure(atIov, pChannel->session[i].ptShmemMediaInfo, &iIovBufferNumber, &iRTPBufferNumber);
							}
							else
							{
								ChannelShmemUDPFillIovStructure(atIov, pChannel->session[i].ptShmemMediaInfo, &iIovBufferNumber, &iRTPBufferNumber);
							}
							//pcSendBuf = (char *)(pMediaBuffer->pbDataStart + (pMediaBuffer->dwBytesUsed - pMediaBuffer->dwRemainingLength));
												 
							//Mark Critical Section is not newly obtained
							if( pSession->iRTPStreamingType == RTP_OVER_TCP )
							{
								pSession->ptShmemMediaInfo->bCSAcquiredButNotSelected = FALSE;
							}

							//Socket selected, attempt to send!
							//20110706 Modified by danny For TCP/HTTP mode socket not sync in multi thread
							if( pSession->psktRTP != NULL && *pSession->psktRTP > 0 )
							{
								//iSendSize = send(sSocket, pcSendBuf, iBufSize, SEND_FLAGS);
								//if(i == 0)
								{
									iSendSize = writev(sSocket, atIov, iIovBufferNumber);
									printf("send%d %d(time=%u)\n", i, iSendSize, pChannel->session[i].ptShmemMediaInfo->tStreamBuffer.dwSecond);
								}
							}
							else
							{
								/* BUGON: Sendto not processed yet...
								else
									iSendSize = sendto(pChannel->iUDPRTPSock, pcSendBuf, iBufSize, SEND_FLAGS, (struct sockaddr *)&pSession->RTPNATAddr,sizeof(RTSP_SOCKADDR));
								*/
								syslog(LOG_ERR, "[ChannelShmemSend]: pSession->sktRTP <= 0\n");
								//20110907 danny fixed mediachannel thread lock  
								if(pSession->iRTPStreamingType == RTP_OVER_TCP )
								{
									if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_AUDIO)
									{
										RTPRTCPCs_Release(pSession, eCSAudioRTP);
										//printf("[%s] @@@RTPRTCPCs_Release AudioRTP %p, iSendSize <= 0@@@\n", __FUNCTION__, pSession->hTCPMuxCS);
									}
									//20120816 modified by Jimmy for metadata
									else if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_VIDEO)
									{
										RTPRTCPCs_Release(pSession, eCSVideoRTP);
										//printf("[%s] @@@RTPRTCPCs_Release VideoRTP %p, iSendSize <= 0@@@\n", __FUNCTION__, pSession->hTCPMuxCS);
									}
									//20120816 added by Jimmy for metadata
									else if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_METADATA)
									{
										RTPRTCPCs_Release(pSession, eCSMetadataRTP);
										//printf("[%s] @@@RTPRTCPCs_Release MetadataRTP %p, iSendSize <= 0@@@\n", __FUNCTION__, pSession->hTCPMuxCS);
									}

								} 
								//Close abnormal session
								RTPRTCPChannel_CloseSession(pChannel, pSession);
								//20091109 change to next session
								break;
							}

							if(iSendSize <= 0)
							{
								if(errno == EWOULDBLOCK)
								{
									iSendSize = 0;
								}
								else if(errno == ECONNREFUSED && pSession->iRTPStreamingType == RTP_OVER_UDP)
								{
									//20090326 ICMP error should be ignored for UDP, this will enable quicktime protocol rolling
									//ChannelShmClearAggregateBuffer(&pChannel->session[i].ptShmemMediaInfo->tAggreMediaBuffer);
									//20091228 Fixed TCP broken image, Avoid release Shmem before send out completely
									ChannelShmResetMediaInfo(pChannel->session[i].ptShmemMediaInfo);
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
											//20130315 added by Jimmy to log more information
											syslog(LOG_DEBUG, "[ChannelShmemSend]: ECONNREFUSED\n");
											RTPRTCPChannel_CloseSession(pChannel, pSession);
										}
									}
									//20091109 change to next session
									break;
								}
								else
								{
									printf("RTP Send socket error %d %s!\n",errno, strerror(errno));
									//20130315 added by Jimmy to log more information
									syslog(LOG_DEBUG, "RTP Send socket error %d %s!\n",errno, strerror(errno));
									if(pSession->iRTPStreamingType == RTP_OVER_TCP )
									{
										if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_AUDIO)
										{
											RTPRTCPCs_Release(pSession, eCSAudioRTP);
											//printf("[%s] @@@RTPRTCPCs_Release AudioRTP %p, iSendSize <= 0@@@\n", __FUNCTION__, pSession->hTCPMuxCS);
										}
										//20120816 modified by Jimmy for metadata
										else if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_VIDEO)
										{
											RTPRTCPCs_Release(pSession, eCSVideoRTP);
											//printf("[%s] @@@RTPRTCPCs_Release VideoRTP %p, iSendSize <= 0@@@\n", __FUNCTION__, pSession->hTCPMuxCS);
										}
										//20120816 added by Jimmy for metadata
										else if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_METADATA)
										{
											RTPRTCPCs_Release(pSession, eCSMetadataRTP);
											//printf("[%s] @@@RTPRTCPCs_Release MetadataRTP %p, iSendSize <= 0@@@\n", __FUNCTION__, pSession->hTCPMuxCS);
										}

									} 
									//Close abnormal session
									RTPRTCPChannel_CloseSession(pChannel, pSession);
									//20091109 change to next session
									break;
								}
							}

							//Calculate the remaining size to be sent
							/*DEBUG*/
							if(pChannel->session[i].ptShmemMediaInfo->tAggreMediaBuffer.dwTotalRemaining < iSendSize)
							{
								printf("*****************RTSP send size overflow! %u to %d**********\n", pChannel->session[i].ptShmemMediaInfo->tAggreMediaBuffer.dwTotalRemaining, iSendSize);
								syslog(LOG_ALERT, "RTSP send size overflow! %u to %d\n", pChannel->session[i].ptShmemMediaInfo->tAggreMediaBuffer.dwTotalRemaining, iSendSize);
							}

							pChannel->session[i].ptShmemMediaInfo->tAggreMediaBuffer.dwTotalRemaining -= iSendSize;
							ChannelShmemCalculateRemainingSize(pChannel->session[i].ptShmemMediaInfo, iSendSize);

							//20091109 while loop to send all UDP RTP
							if(pSession->iRTPStreamingType == RTP_OVER_TCP)
							{
								break;
							}
							else
							{
								//Only exit if UDP finished sending everything in the aggregate buffer
								if(pChannel->session[i].ptShmemMediaInfo->tAggreMediaBuffer.dwTotalRemaining <= 0)
								{
									break;
								}
							}
#ifdef  _SEND_SLEEP
                            //20141002 Add sleep time to avoid busy loop when there is multiple rtsp connection in some platform
							usleep(1); 
#endif
						}// end of while(TRUE)

						/*
						if(pChannel->session[i].ptShmemMediaInfo)
							printf("Send size %d(%p) vs total %d(%p) remaining %d(%p)\n",
								iSendSize, 
								&iSendSize, 
								pChannel->session[i].ptShmemMediaInfo->tAggreMediaBuffer.dwTotalSize, 
								&pChannel->session[i].ptShmemMediaInfo->tAggreMediaBuffer.dwTotalSize, 
								pChannel->session[i].ptShmemMediaInfo->tAggreMediaBuffer.dwTotalRemaining, 
								&pChannel->session[i].ptShmemMediaInfo->tAggreMediaBuffer.dwTotalRemaining);
						*/
						
						if(pChannel->session[i].ptShmemMediaInfo != NULL && pChannel->session[i].ptShmemMediaInfo->tAggreMediaBuffer.dwTotalRemaining == 0)
						{	
							//20100630 Move here to update SR bytes correctly
							RTPRTCPComposer_UpdateSenderReport(pSession->hRTPRTCPComposerHandle,pChannel->session[i].ptShmemMediaInfo->tAggreMediaBuffer.dwTotalSize);

							//20081001 TCP timeout
							pChannel->session[i].ptShmemMediaInfo->tAggreMediaBuffer.dwBaseTime = 0;

							//All data is sent! restore RTP structure
							ChannelShmClearAggregateBuffer(&pChannel->session[i].ptShmemMediaInfo->tAggreMediaBuffer);
							
							//20091228 Avoid Frame size more than AggreMediaBuffer
							if ( pChannel->session[i].ptShmemMediaInfo->iRemainingSize == 0 )
							{
								//20100629 Move here to release TCP handle
								if( pSession->iRTPStreamingType == RTP_OVER_TCP )
								{
									if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_AUDIO)
									{
										RTPRTCPCs_Release(pSession, eCSAudioRTP);
										//printf("[%s] @@@RTPRTCPCs_Release AudioRTP %p, iRemainingSize == 0@@@\n", __FUNCTION__, pSession->hTCPMuxCS);
									}
									//20120816 modified by Jimmy for metadata
									else if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_VIDEO)
									{
										//printf("Releasing TCP_MUX because the whole frame has been sent!\n");
										RTPRTCPCs_Release(pSession, eCSVideoRTP);
										//printf("[%s] @@@RTPRTCPCs_Release VideoRTP %p, iRemainingSize == 0@@@\n", __FUNCTION__, pSession->hTCPMuxCS);
									}
									//20120816 added by Jimmy for metadata
									else if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_METADATA)
									{
										RTPRTCPCs_Release(pSession, eCSMetadataRTP);
										//printf("[%s] @@@RTPRTCPCs_Release MetadataRTP %p, iRemainingSize == 0@@@\n", __FUNCTION__, pSession->hTCPMuxCS);
									}

									//RTPRTCPComposer_UpdateSenderReport(pSession->hRTPRTCPComposerHandle,pChannel->session[i].ptShmemMediaInfo->tAggreMediaBuffer.dwTotalSize);
								}    
								else
								{
									//RTPRTCPComposer_UpdateSenderReport(pSession->hRTPRTCPComposerHandle,pChannel->session[i].ptShmemMediaInfo->tAggreMediaBuffer.dwTotalSize);     
								}

								//20130605 modified by Jimmy to support metadata event
								for(j = 0; j < SHMEM_HANDLE_MAX_NUM; j++)
								{
									SharedMem_ReleaseReadBuffer(pChannel->session[i].ptShmemMediaInfo->ahClientBuf[j]);
								}
							}
							else
							{
								//printf("Not released Shmem ReadBuffer, iRemainingsize=%d\n", pChannel->session[i].ptShmemMediaInfo->iRemainingSize);
								//syslog(LOG_DEBUG, "[ChannelShmemSend]: Not released Shmem ReadBuffer\n");
							}
						}
					}
					else	//If this socket is not selected
					{
						//printf("Socket %d of session %d not selected!\n", sSocket, pChannel->session[i].dwSessionID);
						//Return Critical Section if critical section is newly obtained!
						if(pSession->iRTPStreamingType == RTP_OVER_TCP && pSession->ptShmemMediaInfo->bCSAcquiredButNotSelected)
						{
							if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_AUDIO)
							{
								RTPRTCPCs_Release(pSession, eCSAudioRTP);
								//printf("[%s] @@@RTPRTCPCs_Release AudioRTP %p, this socket is not selected@@@\n", __FUNCTION__, pSession->hTCPMuxCS);
							}
							//20120816 modified by Jimmy for metadata
							else if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_VIDEO)
							{
								RTPRTCPCs_Release(pSession, eCSVideoRTP);
								//printf("[%s] @@@RTPRTCPCs_Release VideoRTP %p, this socket is not selected@@@\n", __FUNCTION__, pSession->hTCPMuxCS);
							}
							//20120816 added by Jimmy for metadata
							else if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_METADATA)
							{
								RTPRTCPCs_Release(pSession, eCSMetadataRTP);
								//printf("[%s] @@@RTPRTCPCs_Release MetadataRTP %p, this socket is not selected@@@\n", __FUNCTION__, pSession->hTCPMuxCS);
							}

						}
					}
				}//End of pMediaBuffer check
			}//End of session exist check
		}//End of for loop

		//Proceed to send multicast packet

#ifdef RTSPRTP_MULTICAST
		//20130327 modified by Jimmy to support different video/audio multicast addresses for ONVIF Test Tool 12.12
		//20130904 modified by Charles for ondemand multicast
		for( i=0 ; i< (RTSP_MULTICASTNUMBER + RTSP_ONDEMAND_MULTICASTNUMBER) ; i++)
		{	    
			if( pChannel->stMulticast[i].iStatus == SESSION_PLAYING )
			{
				RTPRTCPCHANNEL_MULTICAST	*pMulticast = &pChannel->stMulticast[i];
				int							sSocket = INVALID_SOCKET;
				int							iIovBufferNumber = 0;
				int							iRTPBufferNumber = 0;

				if(pMulticast->ptShmemMediaInfo->tAggreMediaBuffer.dwTotalRemaining > 0)
				{
					sSocket = pMulticast->sktRTP;

					if(FD_ISSET(sSocket, &fdsConnect))
					{
						int				iSendSize = 0;
						struct iovec	atIov[3];

						//20091109 while loop to send all UDP RTP
						while(TRUE)
						{
							ChannelShmemUDPFillIovStructure(atIov, pMulticast->ptShmemMediaInfo, &iIovBufferNumber, &iRTPBufferNumber);

							//Proceed to send
							iSendSize = writev(sSocket, atIov, iIovBufferNumber);
							//20160127 debug msg
							/* if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_AUDIO)
							{
								struct sockaddr_in addr;
								size_t addr_length = sizeof(struct sockaddr_in);

								char guest_ip[20];
							

								getpeername (sSocket, &addr, &addr_length);
								inet_ntop(AF_INET, &addr.sin_addr, guest_ip, sizeof(guest_ip));
								int tempPort = ntohs(addr.sin_port);
								printf("guest_ip = %s:%d, iSendSize = %d\n", guest_ip, tempPort, iSendSize);
							}*/
							if(iSendSize <= 0)
							{
								if(errno == EWOULDBLOCK)
								{
									iSendSize = 0;
								}
								else
								{
									printf("Multicast outsend failed!\n");	
									//Close abnormal multicast
									RTPRTCPChannel_CloseMulticast(pChannel, i);
									//20091109 change to next session
									break;
								}
							}
							//printf("Multicast sent %d bytes\n", iSendSize);
						
							//Calculate remaining size
							pMulticast->ptShmemMediaInfo->tAggreMediaBuffer.dwTotalRemaining -= iSendSize;
							ChannelShmemCalculateRemainingSize(pMulticast->ptShmemMediaInfo, iSendSize);
						
							//20091109 while loop to send all UDP RTP, Only exit if UDP finished sending everything in the aggregate buffer
							if(pMulticast->ptShmemMediaInfo->tAggreMediaBuffer.dwTotalRemaining <= 0)
							{
								break;
							}
						}// end of while(TRUE)
						
						if(pMulticast->ptShmemMediaInfo != NULL && pMulticast->ptShmemMediaInfo->tAggreMediaBuffer.dwTotalRemaining == 0)
						{
							//20081001 timeout
							pMulticast->ptShmemMediaInfo->tAggreMediaBuffer.dwBaseTime = 0;

							RTPRTCPComposer_UpdateSenderReport(pMulticast->hRTPRTCPComposerHandle, pMulticast->ptShmemMediaInfo->tAggreMediaBuffer.dwTotalSize);   

							//All data is sent! restore RTP structure
							ChannelShmClearAggregateBuffer(&pMulticast->ptShmemMediaInfo->tAggreMediaBuffer);
							
							//20091228 Avoid Frame size more than AggreMediaBuffer
							if ( pMulticast->ptShmemMediaInfo->iRemainingSize == 0 )
							{
								//20130605 modified by Jimmy to support metadata event
								for(j = 0; j < SHMEM_HANDLE_MAX_NUM; j++)
								{
									SharedMem_ReleaseReadBuffer(pMulticast->ptShmemMediaInfo->ahClientBuf[j]);
								}
							}
						}
					}
					else
					{
						//printf("Multicast socket not selected, unwritable?\n");
					}
				}
			}
		}
#endif 
	}
	else	//if Select returned 0, nothing is selected
	{
		//TCP should return critical section if newly obtained!
		for(i=0 ; i <pChannel->iMaxSession ; i ++ )
		{
			if(pChannel->session[i].dwSessionID != 0 && 
				pChannel->session[i].iStatus != SESSION_PAUSED)
			{
				RTPRTCPCHANNEL_SESSION		*pSession = &pChannel->session[i];

				if(pSession->iRTPStreamingType == RTP_OVER_TCP && pSession->ptShmemMediaInfo->bCSAcquiredButNotSelected)
				{
					if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_AUDIO)
					{
						RTPRTCPCs_Release(pSession, eCSAudioRTP);
						//printf("[%s] @@@RTPRTCPCs_Release AudioRTP %p, Select returned 0@@@\n", __FUNCTION__, pSession->hTCPMuxCS);
					}
					//20120817 modified by Jimmy for metadata
					else if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_VIDEO)
					{
						RTPRTCPCs_Release(pSession, eCSVideoRTP);
						//printf("[%s] @@@RTPRTCPCs_Release VideoRTP %p, Select returned 0@@@\n", __FUNCTION__, pSession->hTCPMuxCS);
					}
					//20120817 added by Jimmy for metadata
					else if(pChannel->nChannelType == RTPRTCPCHANNEL_MEDIATYPE_METADATA)
					{
						RTPRTCPCs_Release(pSession, eCSMetadataRTP);
						//printf("[%s] @@@RTPRTCPCs_Release MetadataRTP %p, Select returned 0@@@\n", __FUNCTION__, pSession->hTCPMuxCS);
					}

				}		
			}
		}
	}

	return S_OK;
}
#endif
