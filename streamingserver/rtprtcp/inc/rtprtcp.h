/*
 *  Copyright (c) 2002 Vivotek Inc. All rights reserved.
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
 *  Module name         :   RTPRTCPComposerAPI.h
 *  Module description  :   compose the RTP/RTCP header
 *  Author              :   ShengFu
 *  Created at          :   2002/5/24
 *  Revision            :   1.1
 ******************************************************************************
 *                        Revision history
 ******************************************************************************
 */

#ifndef _RTPRTCP_H_
#define _RTPRTCP_H_

#include "typedef.h"
#include "rtpmediabuf.h"
#include "rtsprtpcommon.h"
#include "mediabuffer.h"
#include "streamserver.h"


typedef struct
{
	int		iMediaType;
	int		iSampleFrequency;
    DWORD	dwSessionID;
    DWORD   dwInitialTimeStamp;
    WORD	wInitialSequenceNumber;
	DWORD	dwSSRC;
	
} RTPRTCPCOMPOSER_PARAM;



HANDLE RTPRTCPComposer_Create(void);
int RTPRTCPComposer_Reset(HANDLE hRTPRTCPComposerHandle,RTPRTCPCOMPOSER_PARAM *pstVideoEncodingParameter);
int RTPRTCPComposer_Close(HANDLE hRTPRTCPComposerHandle);
int RTPRTCPComposer_SetCodecType(HANDLE hRTPRTCPComposerHandle,RTPRTCPCOMPOSER_PARAM *pstVideoEncodingParameter);
//20130322 added by Jimmy to fix wrong G711 RTP payload type with always multicast
int RTPRTCPComposer_SetAudioCodecType(HANDLE hRTPRTCPComposerHandle,RTPRTCPCOMPOSER_PARAM *pstAudioEncodingParameter);
//20120116 Modify by danny for Update video clockrate for always multicast to avoid incorrect video clockrate at start up
int RTPRTCPComposer_SetVideoClockRate(HANDLE hRTPRTCPComposerHandle,RTPRTCPCOMPOSER_PARAM *pstVideoEncodingParameter);

unsigned long RTPRTCPComposer_GetRTPStartTime(HANDLE hRTPRTCPComposer);
unsigned long RTPRTCPComposer_GetRTCPStartTime(HANDLE hRTPRTCPComposer);

void RTPRTCPComposer_UpdateSenderReport(HANDLE hRTPRTCPComposerHandle, unsigned int uiSentBytes);
void RTPRTCPComposer_RTPHeaderComposer(HANDLE hRTPRTCPComposerHandle, RTPMEDIABUFFER *buf);
int  RTPRTCPComposer_RTPHeaderParse(PROTOCOL_MEDIABUFFER* pMediaBuff, DWORD *pdwSSRC);

void RTPRTCPComposer_MP4VideoHeaderComposer(HANDLE hRTPRTCPComposerHandle, char* buf,RTPMEDIABUFFER *mbuf);
void RTPRTCPComposer_CreateRTCPSenderReport(HANDLE hRTPRTCPComposerHandle, char* buffer, int *len);
BOOL RTPRTCPComposer_IsTimeToReport(HANDLE hRTPRTCPComposerHandle);
void RTPRTCPComposer_IncreaseCountOfMissingReport(HANDLE hRTPRTCPComposerHandle);
void RTPRTCPComposer_ResetCountOfMissingReport(HANDLE hRTPRTCPComposerHandle);
int  RTPRTCPComposer_GetCountOfMissingReport(HANDLE hRTPRTCPComposerHandle);
int  RTPRTCPComposer_ParseRTCPPacket(char* buf,int len, HANDLE hRTPRTCPComposerHandle);
int  RTPRTCPComposer_GetLostRate(HANDLE hRTPRTCPComposerHandle);
int  RTPRTCPComposer_GetJitter(HANDLE hRTPRTCPComposerHandle);
int  RTPRTCPComposer_SetValidate(HANDLE hRTPRTCPComposerHandle);
int  RTPRTCPComposer_GetValidate(HANDLE hRTPRTCPComposerHandle);
void RTPRTCPComposer_SetRTCPStartTime(HANDLE hRTPRTCPComposerHandle);
int RTPRTCPComposer_Update(HANDLE hRTPRTCPComposerHandle,RTPRTCPCOMPOSER_PARAM *pstVideoEncodingParameter);

int RTPRTCPComposer_ParseRTCPPacketBySSRC(char* pcBuf,int iLen, DWORD* pdwSenderSSRC);
DWORD RTPRTCPComposer_GetSessionSSRC(HANDLE hRTPRTCPComposerHandle);
//20141110 modified by Charles for ONVIF Profile G
void RTPRTCPComposer_SetOnvifExtValidate(HANDLE hRTPRTCPComposerHandle);
void RTPRTCPComposer_GetNTPTimeFromUnixLocalTime(unsigned long ullocal_msw, unsigned long ullocal_lsw, unsigned long *ulntp_msw, unsigned long *ulntp_lsw);








#endif  //_RTPRTCPCOMPOSERAPI_H_




