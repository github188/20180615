/*
 *******************************************************************************
 * $Header$
 *
 *  Copyright (c) 2007-2010 Vivotek Inc. All rights reserved.
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
 * $History$
 * 
 *******************************************************************************
 */

/*!
 *******************************************************************************
 * Copyright 2007-2010 Vivotek, Inc. All rights reserved.
 *
 * \file
 * rtprtcp_utility.c
 *
 * \brief
 * Utility routines for RTP/RTCP implement file.
 * Moved from rtprtcp.c
 *
 * \date
 * 2007/03/30
 *
 * \author
 * Jeffrey Lee $Orginal draft by ShengFu$
 *
 *******************************************************************************
 */

#include "rtprtcp_local.h"

#define	W32Len(l)	((l + 3) / 4)  /* length in 32-bit words */

UINT32 RTPRTCP_BitFieldSet(UINT32 uiValue, UINT32 uiBitField, int iStartBit, int iBits)
{
    int	iMask = (1 << iBits) - 1;
    
    return (uiValue & ~(iMask << iStartBit)) + 
           ((uiBitField & iMask) << iStartBit);
}

UINT32 RTPRTCP_BitFieldGet(UINT32 uiValue, int iStartBit, int iBits)
{
    int iMask = (1 << iBits) - 1;

    return (uiValue >> iStartBit) & iMask; 
}

void RTPRTCP_SetSDES(rtcpSDesType eType, rtcpSDES* ptSdes, char *pcData, int iLength)
{
    ptSdes->type   = (unsigned char)eType;
    ptSdes->length = (unsigned char)iLength;

	if (iLength < 255)
	{
		memcpy(ptSdes->value, pcData, iLength);
		memset(ptSdes->value + iLength, 0, 4 - ((iLength + 2) % sizeof(UINT32)));
	}
}

int RTPRTCP_ConvertHeader2h(UINT32 *plBuff, int iStartIndex, int iSize)
{
    int i;
    
    for (i = iStartIndex; i < (iStartIndex + iSize); i++)
    {
        ((UINT32*)plBuff)[i] = ntohl(((UINT32*)plBuff)[i]);
    }        
 
    return (0);
}

int RTPRTCP_ConvertHeader2l(UINT8 *piBuff, int iStartIndex, int iSize)
{
    int i;
    
    for (i = iStartIndex; i < (iStartIndex + iSize); i++)
    {
	    ((UINT32*)piBuff)[i] = htonl(((UINT32*)piBuff)[i]);
    }	    

    return (0);
}

void RTPRTCP_Time2MinSeconds(UINT64_NTP* pNTP64, unsigned long ulSeconds, unsigned long ulMSeconds)
{
/*    time_t  theCurrentTime, minSec;
    struct tm  theTimeStruct;
   
    theTimeStruct.tm_sec = (dTime&0xFF);
    theTimeStruct.tm_min = ((dTime>>8)&0xFF);
    theTimeStruct.tm_hour = (dTime>>16) ;
    theTimeStruct.tm_mday = (dDate&0xFF);
    theTimeStruct.tm_mon = ((dDate>>8)&0xFF) - 1;
    theTimeStruct.tm_year = (dDate>>16) - 1900;
    theTimeStruct.tm_isdst = -1;

    minSec=((dTicks*1000)/KC_TICKS2SEC) ;

    theCurrentTime = mktime(&theTimeStruct);*/

	pNTP64->msdw = ulSeconds + 2208988800ul;
    pNTP64->lsdw = ulMSeconds;   
         
    return;
}

void RTPRTCP_MakeHeader(rtcpHeader* ptHeader, UINT32 uiSsrc, UINT8 uiCount, rtcpType eType, UINT16 uiDataLen)
{
    ptHeader->ssrc = uiSsrc;    
    ptHeader->bits = 0x80000000;
	ptHeader->bits = RTPRTCP_BitFieldSet(ptHeader->bits, uiCount, HEADER_RC, HDR_LEN_RC);
    ptHeader->bits = RTPRTCP_BitFieldSet(ptHeader->bits, eType, HEADER_PT, HDR_LEN_PT);        
    ptHeader->bits = RTPRTCP_BitFieldSet(ptHeader->bits, W32Len(uiDataLen) - 1, HEADER_len, HDR_LEN_len);
        
    RTPRTCP_ConvertHeader2h((UINT32 *)(ptHeader), 0, W32Len(SIZEOF_RTCPHEADER/8));

    return ;
}

int RTPRTCP_ProcessRTCPPacket(char* pcData, INT32 iDataLen, rtcpType eType, INT32 iReportCount, rtpSession *ptRTPSession)
{
    unsigned int uiScanned = 0;
    rtcpSDES *ptSDES;
    int i;
 
    if (iDataLen == 0)
    {
        return 0;
    }        

    /* process the information */
    switch (eType)
    {
        case RTCP_SR:
        {
            break;
        }     
		case RTCP_RR:   
        {
            RTPRTCP_ConvertHeader2l((UINT8*)pcData, 0, 1);
            ptRTPSession->Client.ssrc = *(UINT32 *)(pcData);
            uiScanned = sizeof(UINT32);

            RTPRTCP_ConvertHeader2l((UINT8*)pcData + uiScanned, 0, W32Len(sizeof(rtcpRR)));
            memcpy((void*)&ptRTPSession->RR, (void*)(pcData + uiScanned), sizeof(rtcpRR));
            break;
        }
        case RTCP_SDES: 
        {
            for (i = 0; i < iReportCount; i++)
            {
                RTPRTCP_ConvertHeader2l((UINT8*)pcData + uiScanned, 0, 1);
                ptRTPSession->Client.ssrc = *(UINT32 *)(pcData + uiScanned);

                ptSDES = (rtcpSDES *)(pcData + uiScanned + sizeof(ptRTPSession->Client.ssrc));

                switch(ptSDES->type)
                {
                    case RTCP_SDES_CNAME:
                    	memcpy(&(ptRTPSession->Client.CName), ptSDES, SIZEOF_SDES(*ptSDES));
                    	ptRTPSession->Client.CName.value[ptSDES->length] = 0;    
                    	break;
                    
/* known SDES types that are not handled:
                    case RTCP_SDES_END:
                    case RTCP_SDES_NAME:
                    case RTCP_SDES_EMAIL:
                    case RTCP_SDES_PHONE:
                    case RTCP_SDES_LOC:
                    case RTCP_SDES_TOOL:
                    case RTCP_SDES_NOTE:
                    case RTCP_SDES_PRIV:
                    	break;
*/
                }

                uiScanned += SIZEOF_SDES(*ptSDES) + sizeof(UINT32);
            }
            
            break;
        }           
        case RTCP_BYE:  
        {
            break;
        }
            
        case RTCP_APP: 
		{
		}	
            break;

		default :
			break;
    }

    return 0;
}

UINT32 RTPRTCP_GetTimeStamp(RTPMEDIABUFFER *ptBuf, rtpSession* ptRTPSession)
{
    UINT32 ulTimeStamp;
    UINT64_NTP nntpTime;
  
    RTPRTCP_Time2MinSeconds(&nntpTime, ptBuf->ulSeconds, ptBuf->ulMSeconds);    	

	if (((nntpTime.msdw - ptRTPSession->stNTPStartTime.msdw) < MAX_FRAME_TIMEGAP) && ((nntpTime.msdw - ptRTPSession->stNTPStartTime.msdw) >= 0))
	{
		ulTimeStamp = (UINT32)(( (int)(nntpTime.msdw - ptRTPSession->stNTPStartTime.msdw)*1000 + 
    	                         (int)(nntpTime.lsdw - ptRTPSession->stNTPStartTime.lsdw) )* 
    	                         (ptRTPSession->ulSampleRate/1000) );
        if(ulTimeStamp > 0)
        {
            ptRTPSession->ulIncrementTimestamp = ulTimeStamp;
        }
	}
	else
	{
        //use the old one
		ulTimeStamp = ptRTPSession->ulIncrementTimestamp;
	}

	if (ptRTPSession->ulSampleRate == 0)
	{
#ifdef _LINUX
		//CID:351, CHECKER:TOO_MANY_PRINTF_ARGS
		syslog(LOG_DEBUG, "SampleRate=%lu\n", ptRTPSession->ulSampleRate);
#endif
		return 0;
	}

    if( ulTimeStamp*1000/ptRTPSession->ulSampleRate > 5000 )
	{
#if 0 //def _LINUX
		syslog(LOG_DEBUG, "%s:%d:Force sender report, %u, %u", 
				__FILE__, __LINE__, 
				ulTimeStamp, ptRTPSession->ulSampleRate);
	    syslog(LOG_DEBUG, "%s:%d: %u, %u", __FILE__, __LINE__, 
                ptBuf->ulSeconds, ptBuf->ulMSeconds);
		syslog(LOG_DEBUG, "%u, %u", 
				nntpTime.msdw, nntpTime.lsdw );
		syslog(LOG_DEBUG, "%u, %u", 
				ptRTPSession->stNTPStartTime.msdw, 
				ptRTPSession->stNTPStartTime.lsdw);
#endif //_LINUX
		
        ptRTPSession->iForceSenderReport = 1;
	}
            
//	sprintf(acStart,"\n Get_TimeStampDiff=%d CurrentSec=%d CurrentMinSec=%d LastSec=%d LastMinSec=%d\n", 	ulTimeStamp,nntpTime.msdw,nntpTime.lsdw,prtp->stNTPStartTime.msdw,prtp->stNTPStartTime.lsdw);
//	ADD_LOG(acStart);  
  
    ulTimeStamp += ptRTPSession->ulTimestamp;
    ptRTPSession->stNTPStartTime.msdw=nntpTime.msdw;
    ptRTPSession->stNTPStartTime.lsdw=nntpTime.lsdw;
    ptRTPSession->ulTimestamp=ulTimeStamp;
    return ulTimeStamp;
}

//20140108 added by Charles to Convert local time to UTC time
void RTPRTCP_GetUtcTime(UINT64_NTP* pUTC, UINT64_NTP tLocal)
{
	HANDLE	hTZ;
	struct	timeval	tLocalTime;
	struct	timeval	tUTCTime;
	tLocalTime.tv_sec = tLocal.msdw;
	tLocalTime.tv_usec = 1000*(tLocal.lsdw);

	TimezoneMmap_Initial(&hTZ);

	if(TimezoneMmap_GetUtcTime(hTZ, &tLocalTime, &tUTCTime) == S_FAIL)
	{
		pUTC->msdw = tLocal.msdw;
		pUTC->lsdw = tLocal.lsdw;
		TimezoneMmap_Release(&hTZ); 
		return;
	}
	
	TimezoneMmap_Release(&hTZ);     
	
	pUTC->msdw = tUTCTime.tv_sec;
	pUTC->lsdw = tUTCTime.tv_usec/1000;
	return;

}

