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
 *  Module name        :   RTP/RTCP Composer
 *  File name          :   rtp.c 
 *  File description   :   RTP/RTCP composer  
 *  Author             :   ShengFu
 *  Created at         :   2002/5/20 
 *  Note               :   
 *	$Log: /RD_1/Protocol/RTSP/Server/rtspstreamserver/rtprtcp/src/rtprtcp.c $
 * 
 * 2     06/05/18 3:26p Shengfu
 * update to version 1.4.0.0
 * 
 * 5     05/11/30 6:23p Shengfu
 * update to version 1.3.0.4
 * 
 * 4     05/11/03 11:57a Shengfu
 * update to version 1.3.0.3
 * 
 * 3     05/08/19 11:49a Shengfu
 * 
 * 1     05/08/19 11:29a Shengfu
 * 
 * 2     05/08/10 9:01a Shengfu
 * update rtspstreaming server which enable multicast
 * 
 * 8     05/07/13 2:26p Shengfu
 * update rtsp streaming server
 * 
 * 7     05/05/13 4:36p Shengfu
 * update for new RTP extension
 * 
 * 6     05/04/15 1:35p Shengfu
 * 1. multicast added, but disable
 * 2. RTP extension added
 * 
 * 4     05/03/14 6:41p Shengfu
 * bug fixed for send out Sender Report immediately once 
 * timestamp jump more than 5 seconds
 * 
 * 3     05/02/18 10:08a Shengfu
 * 1. fix the bug that RTCP fail to send out if system clock changed
 *     (timestamp is incorrect if system clock changed)
 * 
 * 2     04/09/16 4:33p Shengfu
 * 
 * 1     04/09/14 9:38a Shengfu
 * 
 * 1     04/09/14 9:19a Shengfu
 * 
 * 8     03/12/05 5:27p Shengfu
 * update to version 0102j
 * 
 * 5     03/10/24 4:07p Shengfu
 * 
 * 11    03/08/27 3:51p Shengfu
 * 
 * 2     03/04/17 2:45p Shengfu
 * update to 0102b
 * 
 * 7     02/08/09 1:53p Simon
 * Set the marker bit of video header packet to 0. 
 * 
 * 6     02/07/29 3:26p Shengfu
 * erase CName DbgPrint
 * 
 * 5     02/07/29 2:21p Shengfu
 * erase CName DbgPrint
 * 
 * 4     02/07/22 3:06p Shengfu
 * ComposerHandle Reset modified - memory set 0 before set anything
 * 
 * 3     02/07/19 9:31a Shengfu
 * RTCP CName modify to NTT@ip address
 * 
 * 2     02/06/08 1:06p Simon
 * After code review, ShengFu revices this module.
 */
 


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rtprtcp_local.h"


#ifdef _LINUX
#include <sys/syslog.h>
#include <arpa/inet.h>
#endif // _LINUX


void RTPRTCPComposer_GetRealTime(UINT64_NTP* pNTP64)
{
    DWORD      dwSeconds,dwMSeconds;
/*    DWORD dDate, dTime, dTicks;
    time_t  theCurrentTime, minSec;
    struct tm  theTimeStruct;
   
    tm_get(&dDate, &dTime, &dTicks);
    theTimeStruct.tm_sec = (dTime&0xFF);
    theTimeStruct.tm_min = ((dTime>>8)&0xFF);
    theTimeStruct.tm_hour = (dTime>>16) ;
    theTimeStruct.tm_mday = (dDate&0xFF);
    theTimeStruct.tm_mon = ((dDate>>8)&0xFF) - 1;
    theTimeStruct.tm_year = (dDate>>16) - 1900;
    theTimeStruct.tm_isdst = -1;

    minSec=(dTicks*1000)/KC_TICKS2SEC ;

    theCurrentTime = mktime(&theTimeStruct);*/
    OSTime_GetTimer(&dwSeconds, &dwMSeconds);
	pNTP64->msdw = dwSeconds+2208988800ul;
	pNTP64->lsdw = dwMSeconds; 
   
    return ; 

}

BOOL RTPRTCPComposer_IsTimeToReport(HANDLE hRTPRTCPComposerHandle)
{
    double dDIFFTIME;
    UINT64_NTP U64DTime;
    rtpSession *pRTPSession;
    UINT64_NTP time;
  
//  Time = reduceNNTP(time);
    RTPRTCPComposer_GetRealTime(&time);

    pRTPSession = (rtpSession*)hRTPRTCPComposerHandle;
  
    if( pRTPSession->iForceSenderReport == 1 )
    {
        pRTPSession->iForceSenderReport = 0;
        return TRUE;
    }
  
    if( pRTPSession->rtcpItv.IndexTime.msdw == 0 )
    {
		pRTPSession->rtcpItv.IndexTime.msdw = time.msdw;
		pRTPSession->rtcpItv.IndexTime.lsdw = time.lsdw;
    }        

    U64DTime.msdw = time.msdw - pRTPSession->rtcpItv.IndexTime.msdw;
    
    if( time.lsdw > pRTPSession->rtcpItv.IndexTime.lsdw )
    {
        U64DTime.lsdw = (int)(time.lsdw - pRTPSession->rtcpItv.IndexTime.lsdw);
    }        
    else
    {
        U64DTime.msdw --;
        U64DTime.lsdw = 1000 + time.lsdw - pRTPSession->rtcpItv.IndexTime.lsdw;
    }        

    dDIFFTIME = (double)U64DTime.msdw + (double)U64DTime.lsdw/1000.;
  
    if( dDIFFTIME > 7. )
    {
            pRTPSession->rtcpItv.IndexTime.msdw = time.msdw;
            pRTPSession->rtcpItv.IndexTime.lsdw = time.lsdw;
            return TRUE; 
    }
    else 
        return FALSE;
        

/*    if( dDIFFTIME > 7. && dDIFFTIME < 500)
    {
        printf("interval time = %f \n",dDIFFTIME);
        if(dDIFFTIME > rtcp_interval(1,1,7500.0,FALSE,600
		                        ,&(pRTPSession->rtcpItv.avg_pck_size)
								,&(pRTPSession->rtcpItv.Initial)))
        {
            pRTPSession->rtcpItv.IndexTime.msdw = time.msdw;
            pRTPSession->rtcpItv.IndexTime.lsdw = time.lsdw;
            return TRUE; 
        }
        else
            return FALSE;
	
    }
    else 
        return FALSE;*/
  
  	 
}

void RTPRTCPComposer_GetNTPTime(UINT64_NTP* pNTP64 )
{
    DWORD      dwSeconds,dwMSeconds;
/*    DWORD dDate, dTime, dTicks;
    time_t  theCurrentTime, minSec;
    struct tm  theTimeStruct;
   
    tm_get(&dDate, &dTime, &dTicks);
    theTimeStruct.tm_sec = (dTime&0xFF);
    theTimeStruct.tm_min = ((dTime>>8)&0xFF);
    theTimeStruct.tm_hour = (dTime>>16) ;
    theTimeStruct.tm_mday = (dDate&0xFF);
    theTimeStruct.tm_mon = ((dDate>>8)&0xFF) - 1;
    theTimeStruct.tm_year = (dDate>>16) - 1900;
    theTimeStruct.tm_isdst = -1;

    minSec=(dTicks*1000)/KC_TICKS2SEC ;

    theCurrentTime = mktime(&theTimeStruct);*/
    
    OSTime_GetTimer(&dwSeconds, &dwMSeconds);
	pNTP64->msdw = dwSeconds+2208988800ul;  //SecondsFrom1900Till1970 = 2208988800;
	pNTP64->lsdw = dwMSeconds; 
	pNTP64->lsdw *= 4294967;   
    
    return;
 
 
/*    const  UINT32 from1900Till1970 = (UINT32)2208988800ul;
    UINT64_NTP nntpTime;
    struct _timeb timeptr ;
    
    _ftime( &timeptr );
    nntpTime.msdw = from1900Till1970 + timeptr.time;
    nntpTime.lsdw = (UINT32)timeptr.millitm;    
    
    return nntpTime;
*/ 
}


/*static void init_seq(rtpSource *s, UINT16 seq)
{
    s->base_seq       = seq;
    s->max_seq        = seq;
    s->bad_seq        = RTP_SEQ_MOD + 1;
    s->cycles         = 0;
    s->received       = 0;
    s->received_prior = 0;
    s->expected_prior = 0;
}*/

void RTPRTCPComposer_CreateRTCPSenderReport(HANDLE hRTPRTCPComposerHandle, char* pcBuffer, int *iLen)
{
    rtcpHeader head;
    rtpSession* pRTPSession;
    
    pRTPSession = (rtpSession*)hRTPRTCPComposerHandle;

    // Modified by Jeffrey 2007/03/30
    //makeHeader(&head,pRTPSession->Server.ssrc,0, RTCP_SR,28);
    RTPRTCP_MakeHeader(&head,pRTPSession->Server.ssrc, 0, RTCP_SR, 28);
    memcpy(pcBuffer,&(head),SIZEOF_RTCPHEADER/8);
    
    pRTPSession->SR.tRTP = pRTPSession->ulTimestamp;

	//20140107 modified by Charles to Convert local time to UTC time
	UINT64_NTP UTCTime;
	RTPRTCP_GetUtcTime(&UTCTime, pRTPSession->stNTPStartTime);
    pRTPSession->SR.tNNTP.msdw = UTCTime.msdw;  
	pRTPSession->SR.tNNTP.lsdw = ((UTCTime.lsdw<<22)/1000)*1024;
	//pRTPSession->SR.tNNTP.msdw = pRTPSession->stNTPStartTime.msdw;   
    //pRTPSession->SR.tNNTP.lsdw = ((pRTPSession->stNTPStartTime.lsdw<<22)/1000)*1024;

    memcpy(pcBuffer + 8,&(pRTPSession->SR), SIZEOF_SR/8);
  
    // Modified by Jeffrey 2007/03/30
    //ConvertHeader2l((UINT8*)pcBuffer + 8,0,5);
    RTPRTCP_ConvertHeader2l((UINT8*)pcBuffer + 8, 0, 5);

    // Modified by Jeffrey 2007/03/30
    //makeHeader(&head,pRTPSession->Server.ssrc,1, RTCP_SDES,(UINT16)((SIZEOF_RTCPHEADER + SIZEOF_SDES(pRTPSession->Server.CName)*8)/8));
    RTPRTCP_MakeHeader(&head,pRTPSession->Server.ssrc, 1, RTCP_SDES, (UINT16)((SIZEOF_RTCPHEADER + SIZEOF_SDES(pRTPSession->Server.CName)*8)/8));
    memcpy(pcBuffer + 28, (char *)&head, SIZEOF_RTCPHEADER/8); 
    memcpy(pcBuffer + 36, &(pRTPSession->Server.CName),SIZEOF_SDES(pRTPSession->Server.CName));
    
    *iLen = 36 + SIZEOF_SDES(pRTPSession->Server.CName); 

      
 /*   pRTPSession = (rtpSession*)hRTPRTCPComposerHandle;

    makeHeader(&head,pRTPSession->Server.ssrc,0, RTCP_SR,28);
    memcpy(pcBuffer,&(head),SIZEOF_RTCPHEADER/8);
    RTPRTCPComposer_GetRealTime(&(pRTPSession->SR.tNNTP));

    pRTPSession->SR.tRTP = (UINT32)(( (int)(pRTPSession->SR.tNNTP.msdw - pRTPSession->ulRTCPStartTime.msdw)*1000+(int)(pRTPSession->SR.tNNTP.lsdw - pRTPSession->ulRTCPStartTime.lsdw) )* (pRTPSession->ulSampleRate/1000));
    pRTPSession->SR.tRTP += pRTPSession->ulInitTimestamp;

    pRTPSession->SR.tNNTP.lsdw *= 4294967;

    memcpy(pcBuffer + 8,&(pRTPSession->SR), SIZEOF_SR/8);
  
    ConvertHeader2l(pcBuffer + 8,0,5);

    makeHeader(&head,pRTPSession->Server.ssrc,1, RTCP_SDES,(UINT16)((SIZEOF_RTCPHEADER + SIZEOF_SDES(pRTPSession->Server.CName)*8)/8));
    memcpy(pcBuffer + 28, (char *)&head, SIZEOF_RTCPHEADER/8); 
    memcpy(pcBuffer + 36, &(pRTPSession->Server.CName),SIZEOF_SDES(pRTPSession->Server.CName));
    
    *iLen = 36 + SIZEOF_SDES(pRTPSession->Server.CName); */
}

int RTPRTCPComposer_ParseRTCPPacketBySSRC(char* pcBuf,int iLen, DWORD* pdwSenderSSRC)
{
    rtcpHeader *head;
    char *pcCurrPtr = pcBuf, *pcDataPtr, *pcCompoundEnd;
    int iHdr_count, iHdr_len;
    rtcpType hdr_type;
    rtpSession tRTPSession;
    
    pcCompoundEnd = pcBuf + iLen;

	memset(&tRTPSession,0,sizeof(rtpSession));
    while (pcCurrPtr < pcCompoundEnd)
    {
        if ((pcCompoundEnd + 1 - pcCurrPtr) < 1)
        {
            return -1;//ERR_RTCP_ILLEGALPACKET;
        }

        head = (rtcpHeader*)(pcCurrPtr);

		// Modified by Jeffrey 2007/03/30
        RTPRTCP_ConvertHeader2l((UINT8*)pcCurrPtr, 0, 1);

        iHdr_count = RTPRTCP_BitFieldGet(head->bits, HEADER_RC, HDR_LEN_RC);
        hdr_type = (rtcpType)RTPRTCP_BitFieldGet(head->bits, HEADER_PT, HDR_LEN_PT);
        iHdr_len = sizeof(UINT32) * (RTPRTCP_BitFieldGet(head->bits, HEADER_len, HDR_LEN_len));
        
        if ((pcCompoundEnd - pcCurrPtr) < iHdr_len)
        {
            return -1;//ERR_RTCP_ILLEGALPACKET;
        }
        
        pcDataPtr = (char *)head + sizeof(UINT32);
        
        // Modified by Jeffrey 2007/03/30
        //rtcpProcessRTCPPacket(pcDataPtr, iHdr_len, hdr_type, iHdr_count,&tRTPSession);
        RTPRTCP_ProcessRTCPPacket(pcDataPtr, iHdr_len, hdr_type, iHdr_count, &tRTPSession);

        pcCurrPtr += iHdr_len + sizeof(UINT32);

		if( tRTPSession.RR.ssrc != 0 )
		{
			*pdwSenderSSRC = tRTPSession.RR.ssrc;
			return 0;
		}
    }


    return 0;
}


int RTPRTCPComposer_ParseRTCPPacket(char* pcBuf,int iLen, HANDLE hRTPRTCPComposerHandle)
{
    rtcpHeader *head;
    char *pcCurrPtr = pcBuf, *pcDataPtr, *pcCompoundEnd;
    int iHdr_count, iHdr_len;
    rtcpType hdr_type;
    rtpSession* pRTPSession;
    
    pRTPSession = (rtpSession*)hRTPRTCPComposerHandle;  

    pcCompoundEnd = pcBuf + iLen;

    while (pcCurrPtr < pcCompoundEnd)
    {
        if ((pcCompoundEnd + 1 - pcCurrPtr) < 1)
        {
            return -1;//ERR_RTCP_ILLEGALPACKET;
        }

        head = (rtcpHeader*)(pcCurrPtr);

		// Modified by Jeffrey 2007/03/30
        RTPRTCP_ConvertHeader2l((UINT8*)pcCurrPtr, 0, 1);

        iHdr_count = RTPRTCP_BitFieldGet(head->bits, HEADER_RC, HDR_LEN_RC);
        hdr_type  = (rtcpType)RTPRTCP_BitFieldGet(head->bits, HEADER_PT, HDR_LEN_PT);
        iHdr_len   = sizeof(UINT32) * (RTPRTCP_BitFieldGet(head->bits, HEADER_len, HDR_LEN_len));

        if ((pcCompoundEnd - pcCurrPtr) < iHdr_len)
        {
            return -1;//ERR_RTCP_ILLEGALPACKET;
        }
        
        pcDataPtr = (char *)head + sizeof(UINT32);
        
        // Modified by Jeffrey 2007/03/30
        //rtcpProcessRTCPPacket(pcDataPtr, iHdr_len, hdr_type, iHdr_count,pRTPSession);
        RTPRTCP_ProcessRTCPPacket(pcDataPtr, iHdr_len, hdr_type, iHdr_count, pRTPSession);

        pcCurrPtr += iHdr_len + sizeof(UINT32);
    }

    return 0;
}

/*
UINT64_NTP  Time2Seconds(DWORD dDate,DWORD dTime,DWORD dTicks)
{
    UINT64_NTP nntpTime;
    time_t  theCurrentTime, minSec;
    struct tm  theTimeStruct;
   
    theTimeStruct.tm_sec = (dTime&0xFF);
    theTimeStruct.tm_min = ((dTime>>8)&0xFF);
    theTimeStruct.tm_hour = (dTime>>16) ;
    theTimeStruct.tm_mday = (dDate&0xFF);
    theTimeStruct.tm_mon = ((dDate>>8)&0xFF) - 1;
    theTimeStruct.tm_year = (dDate>>16) - 1900;
    theTimeStruct.tm_isdst = -1;

    minSec=((dTicks*1000)/KC_TICKS2SEC) ;

    theCurrentTime = mktime(&theTimeStruct);

    nntpTime.msdw = theCurrentTime+2208988800;
    nntpTime.lsdw = minSec;   
    nntpTime.lsdw *= 4294967;            
    return nntpTime;
   
}*/

// Modified by Jeffrey 2007/03/30
void RTPRTCPComposer_MP4VideoHeaderComposer(void* hRTP, char* pcBuf,RTPMEDIABUFFER *pMBuf)
{
    INT32 *plHeader;
    rtpSession *ptSession = (rtpSession*)hRTP;
    
    if (ptSession->stNTPStartTime.msdw == 0)
    {
        RTPRTCP_Time2MinSeconds(&(ptSession->stNTPStartTime), pMBuf->ulSeconds, pMBuf->ulMSeconds);    	
    }  

    plHeader = (INT32 *)pcBuf;
    plHeader[0] = 0;
    plHeader[0] = RTPRTCP_BitFieldSet(plHeader[0], 2, 30, 2);	
    plHeader[0] = RTPRTCP_BitFieldSet(plHeader[0], 0, 23, 1);	// set marker be as 1
    plHeader[0] = RTPRTCP_BitFieldSet(plHeader[0], ptSession->iMediaType, 16, 7);	//payload type
    plHeader[0] = RTPRTCP_BitFieldSet(plHeader[0], ptSession->usSeq, 0, 16);	
    plHeader[1] = RTPRTCP_GetTimeStamp(pMBuf, ptSession);
    plHeader[2] = ptSession->Server.ssrc;
    ptSession->usSeq ++;
  	       
	RTPRTCP_ConvertHeader2l((UINT8*)plHeader, 0, 3);
}


int RTPRTCPComposer_RTPHeaderParse(PROTOCOL_MEDIABUFFER* pMediaBuff, DWORD *pdwSSRC)
{
	char* pcRecvData = pMediaBuff->pbBufferStart;
	DWORD *header = (DWORD*)pcRecvData;
	int   iCSRC, iEXProfile, iEXlen;
	
	/*if( pMediaBuff->dwBytesUsed > 2000 )
		return -1;
		
	memcpy(abyOper, pMediaBuff->pbBufferStart, pMediaBuff->dwBytesUsed);*/

	//pszLocName[0] = 0;

	RTPRTCP_ConvertHeader2h((UINT32 *)pcRecvData, 0, 3);
	iCSRC = RTPRTCP_BitFieldGet(header[0], 24, 4);
	RTPRTCP_ConvertHeader2h((UINT32 *)pcRecvData, 3, iCSRC);
	
	if ((12 + iCSRC*sizeof(DWORD)) > pMediaBuff->dwBufferLength)
	{
		return -1;
	}

	if (RTPRTCP_BitFieldGet(header[0], 30, 2) != 2)
	{
		return -1;
	}

    ((RTPHEADERINFO*)(pMediaBuff->pbHeaderInfoStart))->ulTimeStamp = header[1];			
    ((RTPHEADERINFO*)(pMediaBuff->pbHeaderInfoStart))->usSeqNumber = (UINT16)(RTPRTCP_BitFieldGet(header[0], 0, 16));
    ((RTPHEADERINFO*)(pMediaBuff->pbHeaderInfoStart))->iMarker = RTPRTCP_BitFieldGet(header[0], 23, 1);
	((RTPHEADERINFO*)(pMediaBuff->pbHeaderInfoStart))->iExtension = RTPRTCP_BitFieldGet(header[0], 28, 1);
	*pdwSSRC = header[2];

	if( ((RTPHEADERINFO*)(pMediaBuff->pbHeaderInfoStart))->iExtension == 0 )
	{		
		pMediaBuff->dwHeaderSize = (12 + iCSRC*sizeof(DWORD));
		pMediaBuff->pbDataStart = pMediaBuff->pbBufferStart + pMediaBuff->dwHeaderSize;
		pMediaBuff->dwBytesUsed = pMediaBuff->dwBytesUsed - pMediaBuff->dwHeaderSize;
//		pszLocName[0] = 1;  // this means no extenision for this packet
	}
	else
	{
		RTPRTCP_ConvertHeader2h((UINT32 *)pcRecvData, 3 + iCSRC, 1);
		iEXProfile = RTPRTCP_BitFieldGet(header[3 + iCSRC], 16, 16);
		iEXlen = RTPRTCP_BitFieldGet(header[3 + iCSRC], 0, 16) * 4;

		((RTPHEADERINFO*)(pMediaBuff->pbHeaderInfoStart))->iExtension = 0;
		pMediaBuff->dwHeaderSize = (12 + iCSRC * sizeof(DWORD)) + 4 + iEXlen;
		pMediaBuff->pbDataStart = pMediaBuff->pbBufferStart + pMediaBuff->dwHeaderSize;
		pMediaBuff->dwBytesUsed = pMediaBuff->dwBytesUsed - pMediaBuff->dwHeaderSize;
//		pszLocName[0] = 1;  // this means no extenision for this packet

		if (pMediaBuff->dwBytesUsed > pMediaBuff->dwBufferLength)
		{
			return -1;
		}
	}

	return 0;
}

// Modified by Jeffrey 2007/03/30
void RTPRTCPComposer_RTPHeaderComposer(HANDLE hRTP, RTPMEDIABUFFER *pBuf)
{
    INT32 *plHeader;
    rtpSession *ptSession = (rtpSession*)hRTP;

    if (ptSession->stNTPStartTime.msdw == 0)
    {
        RTPRTCP_Time2MinSeconds(&(ptSession->stNTPStartTime), pBuf->ulSeconds, pBuf->ulMSeconds);    	
    }

	if (pBuf->dwExtensionLen > 0)
	{
#ifdef _SHARED_MEM
		plHeader = (INT32*)pBuf->pbDataStart;
#else
		plHeader = (INT32*)((char*)pBuf->pbDataStart - 12 - 4 - pBuf->dwExtensionLen);
#endif
		plHeader[0] = 0;
		plHeader[0] = RTPRTCP_BitFieldSet(plHeader[0],2,30,2);
		if (pBuf->dwExtensionLen > 0)
		{
			plHeader[0] = RTPRTCP_BitFieldSet(plHeader[0], 1, 28, 1);
		}
		plHeader[0] = RTPRTCP_BitFieldSet(plHeader[0], pBuf->bMarker, 23, 1);	
		plHeader[0] = RTPRTCP_BitFieldSet(plHeader[0], ptSession->iMediaType, 16, 7);	//payload type
		plHeader[0] = RTPRTCP_BitFieldSet(plHeader[0], ptSession->usSeq, 0, 16);	
		plHeader[1] = RTPRTCP_GetTimeStamp(pBuf, ptSession);
		plHeader[2] = ptSession->Server.ssrc;
		if (pBuf->dwExtensionLen > 0)
		{
			plHeader[3] = RTPRTCP_BitFieldSet(plHeader[3], (pBuf->dwExtensionLen)/4, 0, 16);
            if(ptSession->bOnvifRTPExt)
            {   
                plHeader[3] = RTPRTCP_BitFieldSet(plHeader[3], 0xABAC, 16, 16);
            }
			else if (ptSession->iMediaType == RTSPSTREAMING_MPEG4_MEDIATYPE)
			{
				plHeader[3] = RTPRTCP_BitFieldSet(plHeader[3], 0x5282, 16, 16);
			}
			else if(ptSession->iMediaType == RTSPSTREAMING_JPEG_MEDIATYPE)
			{
				plHeader[3] = RTPRTCP_BitFieldSet(plHeader[3], 0xffd8, 16, 16);
			}
#ifndef _NO_NEED_H264_PROFILE_NUM
			else if(ptSession->iMediaType == RTSPSTREAMING_H264_MEDIATYPE)
			{
				plHeader[3] = RTPRTCP_BitFieldSet(plHeader[3], 0x7070, 16, 16);
			}
#endif
            else if(ptSession->iMediaType == RTSPSTREAMING_H265_MEDIATYPE)
			{
				plHeader[3] = RTPRTCP_BitFieldSet(plHeader[3], 0x8080, 16, 16); 
			}
			else
			{
				plHeader[3] = RTPRTCP_BitFieldSet(plHeader[3], 0x3897, 16, 16);
			}
		}
		ptSession->usSeq ++;
  	       
		//ConvertHeader2l((UINT8*)plHeader,0,(12 + pBuf->dwExtensionLen + 4 )/4);
		RTPRTCP_ConvertHeader2l((UINT8*)plHeader, 0, (12 + 4 )/4);
#ifndef _SHARED_MEM
		pBuf->dwBytesUsed = pBuf->dwBytesUsed + 12 + 4 + pBuf->dwExtensionLen;
		pBuf->pbDataStart = pBuf->pbDataStart - 12 - 4 - pBuf->dwExtensionLen;
#endif
	}
	else
	{
#ifdef _SHARED_MEM
		plHeader = (INT32*)pBuf->pbDataStart;
#else
		plHeader = (INT32*)((char*)pBuf->pbDataStart - 12);
#endif
		plHeader[0] = 0;
		plHeader[0] = RTPRTCP_BitFieldSet(plHeader[0], 2, 30, 2);	
		plHeader[0] = RTPRTCP_BitFieldSet(plHeader[0], pBuf->bMarker, 23, 1);	
		plHeader[0] = RTPRTCP_BitFieldSet(plHeader[0], ptSession->iMediaType, 16, 7);	//payload type
		plHeader[0] = RTPRTCP_BitFieldSet(plHeader[0], ptSession->usSeq, 0, 16);	
		plHeader[1] = RTPRTCP_GetTimeStamp(pBuf, ptSession);
		plHeader[2] = ptSession->Server.ssrc;
		ptSession->usSeq ++;
  	       
		RTPRTCP_ConvertHeader2l((UINT8*)plHeader, 0, 3);
#ifndef _SHARED_MEM
		pBuf->dwBytesUsed = pBuf->dwBytesUsed + 12;			
		pBuf->pbDataStart = pBuf->pbDataStart - 12;
#endif
	}
	//printf("[%s] ptSession->usSeq=%u\n", __FUNCTION__, ptSession->usSeq);
}

void RTP_Init(rtpSession* pRTPSession,char* pcCNAME)
{
    pRTPSession->Server.ssrc = 0;
    pRTPSession->usSeq = 0;

    pRTPSession->ulTimestamp = 0;
    pRTPSession->ulSampleRate = 0;

    pRTPSession->iForceSenderReport = 0;
    
    // Modified by Jeffrey 2007/03/30
    //setSDES(RTCP_SDES_CNAME,&(pRTPSession->Server.CName),pcCNAME,strlen(pcCNAME));
    RTPRTCP_SetSDES(RTCP_SDES_CNAME, &(pRTPSession->Server.CName), pcCNAME, strlen(pcCNAME));
}

rtpSession* RTP_Create(int media)
{
    rtpSession *pRTPSession;
    int         iSock,iLen;    
    struct sockaddr_in addr;
    char        acHostName[255];

    pRTPSession = (rtpSession*)malloc(sizeof(rtpSession));

	if( pRTPSession != NULL )
	    memset(pRTPSession,0,sizeof(rtpSession));
	else
		return NULL;
    
    iSock = socket(AF_INET,SOCK_DGRAM,0);
	//CID:169, CHECKER:NEGATIVE_RETURNS
	if(iSock < 0)
	{
		free(pRTPSession);
		return NULL;
	}
	//CID:1158, CHECKER:UNINIT
	iLen = sizeof(addr);
    getsockname(iSock,(struct sockaddr *)&addr,&iLen);
    rtspstrcpy(acHostName,"NTT@", sizeof(acHostName));
	rtspstrcat(acHostName, (char *) inet_ntoa(addr.sin_addr), sizeof(acHostName));
//    DbgPrint(("CNAME : %s\n",acHostName));
    closesocket(iSock);
    
    RTP_Init(pRTPSession,acHostName);

    return pRTPSession;

}


HANDLE RTPRTCPComposer_Create( )
{
   return (HANDLE) RTP_Create(0);    
}

int RTPRTCPComposer_Reset(HANDLE hRTPRTCPComposerHandle,RTPRTCPCOMPOSER_PARAM *pstVideoEncodingParameter)
{
    rtpSession *pRTPSession;
// Modified by Jeffrey 2007/03/14
#if defined(_PSOS_TRIMEDIA) || defined(_LINUX)
    int         iSock, iLen;
#endif
    struct sockaddr_in	addr;
    char				acHostName[255];
    
#ifdef _WIN32_ 
	struct  hostent  *phost;
	char    acText[64];
#endif
  
    if( !hRTPRTCPComposerHandle ) 
    { 
        return -1;
    }        
       
    pRTPSession = (rtpSession*)hRTPRTCPComposerHandle;
    memset(pRTPSession,0,sizeof(rtpSession));
	memset(&addr,0,sizeof(struct sockaddr_in));

#ifdef _WIN32_
	gethostname(acText, 64);
	phost = gethostbyname(acText);
	addr.sin_addr.S_un.S_addr = *(int*) phost->h_addr_list[0];
	rtspstrcpy(acHostName,"StreamingServer@", sizeof(acHostName));
	rtspstrcat(acHostName,inet_ntoa(addr.sin_addr), sizeof(acHostName));
	printf("CNAME=%s\n",acHostName);
#endif 

#if defined(_PSOS_TRIMEDIA) || defined(_LINUX)
    iSock = socket(AF_INET,SOCK_DGRAM,0);
	//CID:168, CHECKER:NEGATIVE_RETURNS
	if(iSock < 0)
	{
		printf("RTPRTCP composer socket allocation failed!\n");
		return -1;
	}
	//CID:1157, CHECKER:UNINIT
	iLen = sizeof(addr);
    getsockname(iSock,(struct sockaddr *)&addr,&iLen);
    rtspstrcpy(acHostName,"StreamingServer@", sizeof(acHostName));
	rtspstrcat(acHostName,(char *)inet_ntoa(addr.sin_addr), sizeof(acHostName));
//    DbgPrint(("CNAME : %s\n",acHostName));
    closesocket(iSock);
#endif
    
    RTP_Init(pRTPSession,acHostName);
         
    pRTPSession->Server.ssrc  = pstVideoEncodingParameter->dwSSRC ;
    pRTPSession->usSeq        = pstVideoEncodingParameter->wInitialSequenceNumber;
    pRTPSession->ulTimestamp  = pstVideoEncodingParameter->dwInitialTimeStamp;
    pRTPSession->ulSampleRate = pstVideoEncodingParameter->iSampleFrequency;
    pRTPSession->iMediaType   = pstVideoEncodingParameter->iMediaType;
    
    return 0;
}

int RTPRTCPComposer_SetCodecType(HANDLE hRTPRTCPComposerHandle,RTPRTCPCOMPOSER_PARAM *pstVideoEncodingParameter)
{
	rtpSession *pRTPSession;
    pRTPSession = (rtpSession*)hRTPRTCPComposerHandle;

	if(pstVideoEncodingParameter->iMediaType > 0)
	{
		pRTPSession->iMediaType   = pstVideoEncodingParameter->iMediaType;
	}

	return 0;
}

//20130322 added by Jimmy to fix wrong G711 RTP payload type with always multicast
int RTPRTCPComposer_SetAudioCodecType(HANDLE hRTPRTCPComposerHandle,RTPRTCPCOMPOSER_PARAM *pstAudioEncodingParameter)
{
	rtpSession *pRTPSession;
    pRTPSession = (rtpSession*)hRTPRTCPComposerHandle;

	if(pstAudioEncodingParameter->iMediaType >= 0)
	{
		pRTPSession->iMediaType   = pstAudioEncodingParameter->iMediaType;
	}

	return 0;
}


//20120116 Modify by danny for Update video clockrate for always multicast to avoid incorrect video clockrate at start up
int RTPRTCPComposer_SetVideoClockRate(HANDLE hRTPRTCPComposerHandle,RTPRTCPCOMPOSER_PARAM *pstVideoEncodingParameter)
{
	rtpSession *pRTPSession;
    pRTPSession = (rtpSession*)hRTPRTCPComposerHandle;

	if(pstVideoEncodingParameter->iSampleFrequency > 0)
	{
		pRTPSession->ulSampleRate   = pstVideoEncodingParameter->iSampleFrequency;
	}

	return 0;
}

int RTPRTCPComposer_Update(HANDLE hRTPRTCPComposerHandle,RTPRTCPCOMPOSER_PARAM *pstVideoEncodingParameter)
{
    rtpSession *pRTPSession;
     
    if( !hRTPRTCPComposerHandle ) 
    { 
        return -1;
    }        
        
    pRTPSession = (rtpSession*)hRTPRTCPComposerHandle;
            
    pstVideoEncodingParameter->dwSSRC = pRTPSession->Server.ssrc ;
    pstVideoEncodingParameter->wInitialSequenceNumber = pRTPSession->usSeq;
    pstVideoEncodingParameter->dwInitialTimeStamp = pRTPSession->ulTimestamp;
    
    return 0;
}
    
int RTPRTCPComposer_Close(HANDLE hRTPRTCPComposerHandle)
{
    rtpSession *pRTPSession;
  
    if(!hRTPRTCPComposerHandle)
    {
        return 0 ;
    }
    
    pRTPSession = (rtpSession*)hRTPRTCPComposerHandle;
  
    free(pRTPSession);
    
    return 0;
    
}

int RTPRTCPComposer_GetRTPSessionSize(void)
{
    return sizeof(rtpSession);   
    
}

unsigned long RTPRTCPComposer_GetRTPStartTime(HANDLE hRTPRTCPComposerHandle)
{
    rtpSession* pRTPSession;
  
    if(!hRTPRTCPComposerHandle)
    {
        return 0 ;
    }
    pRTPSession = (rtpSession*)hRTPRTCPComposerHandle;
  
    return pRTPSession->stNTPStartTime.msdw;
    
}

void RTPRTCPComposer_UpdateSenderReport(HANDLE hRTPRTCPComposerHandle, unsigned int uiSentBytes)
{
    rtpSession* pRTPSession;  
  
    if(!hRTPRTCPComposerHandle)
    {
        return ;
    }
  
    pRTPSession = (rtpSession*)hRTPRTCPComposerHandle;
  
    pRTPSession->SR.nBytes += uiSentBytes;
    pRTPSession->SR.nPackets ++;  
    
}

unsigned long RTPRTCPComposer_GetRTCPStartTime(HANDLE hRTPRTCPComposerHandle)
{
    rtpSession* pRTPSession;
  
    if(!hRTPRTCPComposerHandle)
    {
        return 0;
    }
  
    pRTPSession = (rtpSession*)hRTPRTCPComposerHandle;
  
    return pRTPSession->ulRTCPStartTime.msdw;
    
}

void RTPRTCPComposer_SetRTCPStartTime(HANDLE hRTPRTCPComposerHandle)
{
    rtpSession* pRTPSession;
    
    if(!hRTPRTCPComposerHandle)
    {
        return;
    }
  
    pRTPSession = (rtpSession*)hRTPRTCPComposerHandle;
    RTPRTCPComposer_GetRealTime(&(pRTPSession->ulRTCPStartTime));
    pRTPSession->ulInitTimestamp = pRTPSession->ulTimestamp;

//	RTPRTCPComposer_GetRealTime(&NTP64);
//    pRTPSession->ulRTCPStartTime = reduceNNTP(NTP64); 
    
}   

void RTPRTCPComposer_IncreaseCountOfMissingReport(HANDLE hRTPRTCPComposerHandle)
{
    rtpSession* pRTPSession;
    
    if(!hRTPRTCPComposerHandle)
    {
        return;
    }
  
    pRTPSession = (rtpSession*)hRTPRTCPComposerHandle;
    
    pRTPSession->RRCount ++;
}

void RTPRTCPComposer_ResetCountOfMissingReport(HANDLE hRTPRTCPComposerHandle)
{
    rtpSession* pRTPSession;
    
    if(!hRTPRTCPComposerHandle)
    {
        return;
    }
  
    pRTPSession = (rtpSession*)hRTPRTCPComposerHandle;
    
    pRTPSession->RRCount = 0;
}

int RTPRTCPComposer_GetCountOfMissingReport(HANDLE hRTPRTCPComposerHandle)
{
    rtpSession* pRTPSession;
    
    if(!hRTPRTCPComposerHandle)
    {
        return -1;
    }
  
    pRTPSession = (rtpSession*)hRTPRTCPComposerHandle;
    
    return pRTPSession->RRCount;
    
} 

int RTPRTCPComposer_GetLostRate(HANDLE hRTPRTCPComposerHandle)
{
    rtpSession* pRTPSession;
  
    if(!hRTPRTCPComposerHandle)
    {
        return -1 ;
    }
    
    pRTPSession = (rtpSession*)hRTPRTCPComposerHandle;
    
    return pRTPSession->RR.bfLost;
    
}   


DWORD RTPRTCPComposer_GetSessionSSRC(HANDLE hRTPRTCPComposerHandle)
{
    rtpSession* pRTPSession;

    if(!hRTPRTCPComposerHandle)
    {
        return 0;
    }
    pRTPSession = (rtpSession*)hRTPRTCPComposerHandle;
    
    return pRTPSession->Server.ssrc;

}

int RTPRTCPComposer_GetJitter(HANDLE hRTPRTCPComposerHandle)
{
    rtpSession* pRTPSession;
  
    if(!hRTPRTCPComposerHandle)
    {
        return -1 ;
    }
    pRTPSession = (rtpSession*)hRTPRTCPComposerHandle;
    
    return pRTPSession->RR.nJitter;
    
}

int RTPRTCPComposer_SetValidate(HANDLE hRTPRTCPComposerHandle)
{
    rtpSession* pRTPSession;
  
    if(!hRTPRTCPComposerHandle)
    {
        return -1 ;
    }
    pRTPSession = (rtpSession*)hRTPRTCPComposerHandle;
    
    pRTPSession->iInvalid = 1;
    
    return 0;
}

int RTPRTCPComposer_GetValidate(HANDLE hRTPRTCPComposerHandle)
{
    rtpSession* pRTPSession;
  
    if(!hRTPRTCPComposerHandle)
    {
        return -1 ;
    }
    pRTPSession = (rtpSession*)hRTPRTCPComposerHandle;
    
    return pRTPSession->iInvalid;

}

//20141110 added by Charles for ONVIF Profile G
void RTPRTCPComposer_SetOnvifExtValidate(HANDLE hRTPRTCPComposerHandle)
{
    rtpSession* pRTPSession;
  
    if(!hRTPRTCPComposerHandle)
    {
        return;
    }
    pRTPSession = (rtpSession*)hRTPRTCPComposerHandle;

    pRTPSession->bOnvifRTPExt = TRUE;

}

void RTPRTCPComposer_GetNTPTimeFromUnixLocalTime(unsigned long ullocal_msw, unsigned long ullocal_lsw, unsigned long *ulntp_msw, unsigned long *ulntp_lsw)    
{
    UINT64_NTP NTPTime;
    UINT64_NTP LocalTime;

    LocalTime.msdw = ullocal_msw + 2208988800ul; //Convert Unix Time to NTP Time
    LocalTime.lsdw = ullocal_lsw;
    
    //convert to UTC time from local time
    RTPRTCP_GetUtcTime(&NTPTime, LocalTime);
    *ulntp_msw = NTPTime.msdw;
    *ulntp_lsw = ((NTPTime.lsdw<<22)/1000)*1024;
    
}

