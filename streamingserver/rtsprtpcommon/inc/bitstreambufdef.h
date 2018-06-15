/*
 *******************************************************************************
 * $Header: /RD_1/Project/PNX1300_PSOS/Farseer/common/src/bitstreambufdef.h 4     05/09/27 2:27p Shengfu $
 *
 *  Copyright (c) 2000-2003 Vivotek Inc. All rights reserved.
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
 * $History: bitstreambufdef.h $
 * 
 * *****************  Version 4  *****************
 * User: Shengfu      Date: 05/09/27   Time: 2:27p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/common/src
 * 
 * *****************  Version 3  *****************
 * User: Kate         Date: 05/08/29   Time: 10:21p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/common/src
 * Add bSignal for _DIAGNOSIS
 * 
 * *****************  Version 2  *****************
 * User: Weicheng     Date: 05/07/30   Time: 8:20p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Waternoose/common/src
 * code sync (video request)
 * 
 * *****************  Version 1  *****************
 * User: Kate         Date: 05/07/25   Time: 4:13p
 * Created in $/RD_1/Project/PNX1300_PSOS/Farseer/COMMON/src
 * First checkin in new mars
 * 
 * *****************  Version 7  *****************
 * User: Yun          Date: 05/07/14   Time: 2:31p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/common/src
 * Add bChangeSetting for rtsp streaming
 * 
 * *****************  Version 6  *****************
 * User: Weicheng     Date: 05/06/02   Time: 2:27p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Waternoose/common/src
 * 
 * *****************  Version 5  *****************
 * User: Weicheng     Date: 05/05/27   Time: 7:01p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Waternoose/common/src
 * modify for _VIDEO_REQUEST_INFORMATION_CONTROL
 * 
 * *****************  Version 4  *****************
 * User: Weicheng     Date: 05/05/19   Time: 10:42a
 * Updated in $/RD_1/Project/PNX1300_PSOS/Waternoose/common/src
 * modify for Waternoose
 * 
 * *****************  Version 3  *****************
 * User: Shengfu      Date: 05/04/18   Time: 12:02p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/common/src
 * RTSP RTP extension added
 * 
 * *****************  Version 1  *****************
 * User: Shengfu      Date: 05/02/02   Time: 3:53p
 * Created in $/RD_1/Protocol/Module/RTSP6K_SERVER/include
 * 
 * *****************  Version 2  *****************
 * User: Yun          Date: 03/08/07   Time: 4:31p
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/common/src
 * Add dwBytesUsed and tFrameType field
 * 
 * *****************  Version 1  *****************
 * User: Yun          Date: 03/07/28   Time: 2:59p
 * Created in $/rd_1/Project/TM1300_PSOS/FarSeer/common/src
 *
 *******************************************************************************
 */

/*!
 *******************************************************************************
 * Copyright 2000-2003 Vivotek, Inc. All rights reserved.
 *
 * \file
 * bitstreambufdef.h
 *
 * \brief
 * header file of bitstream buffer structure
 *
 * \date
 * 2003/04/29
 *
 * \author
 * May Hsu
 *
 *******************************************************************************
 */
/* =========================================================================================== */
#ifndef __BITSTREAMBUFDEF_H__
#define __BITSTREAMBUFDEF_H__

#include "typedef.h"
#include "mediatypedef.h"
#ifdef _VIDEO_REQUEST_INFORMATION_CONTROL
#include "videocomm.h"
#endif


typedef struct
{
	WORD		wWidth;
	WORD		wHeight;
	//20101210 Add by danny For support DRI header
	DWORD		dwDRI;
	BYTE		*pbyApplicationData;
	DWORD		dwApplicationDataSize;
	BYTE		byType;
  	char		acQuantizationTable[128];
	//20120618 danny, modified JPEG RTP header extension according to ONVIF spec and RFC2435
	BYTE		*pbyStartOfFrameBDCT;
	DWORD		dwStartOfFrameBDCTSize;
    //20150408 added by Charles for ONVIF JPEG Extension
    BOOL        bEnableOnvifJpegRTPExt;

} TJPEGoverRTPInfo;

typedef enum
{
	ESingleNALU = 0,
	EFragmentationNALU = 1,
	EAggregationNALU = 2		//Currently unsupported

} EH264PacketFragMode;

typedef enum
{
	EH265SingleNALU = 0,
	EH265FragmentationNALU = 1,
	EH265AggregationNALU = 2,		//Currently unsupported
	EH265PACINALU = 3              //Currently unsupported 

} EH265PacketFragMode;

typedef struct
{
	EH264PacketFragMode		ePacketFragMode;
	BYTE					byNALOctet;			//This stores Forbidden bit, NRI unit, NAL type
	BYTE					byNALType;			//NAL type
	int						iNALUIndex;			//Indicate NALU index in Ubuffer (which can contain multiple NALUs)

} TH264overRTPInfo;

typedef struct
{
	EH265PacketFragMode		ePacketFragMode;
	unsigned short			usNALHeader;	    //This stores Forbidden bit, NALU type, LayerID, temporalID
	BYTE					byNALType;			//NAL type
	int						iNALUIndex;		//Indicate NALU index in Ubuffer (which can contain multiple NALUs)

} TH265overRTPInfo;

#if defined (_WIN32_) || defined (_LINUX_X86)

typedef struct
{
	//! pointer of buffer storing bitstream
	BYTE				*pbyBuffer;
	//! total size of bistream buffer
	DWORD				dwBufSize;
	//! a length of used media data size
	DWORD				dwBytesUsed;
	//! the frame type in the bitstream buffer
	TMediaDBFrameType	tFrameType;

	BYTE				*pbUserData;
	DWORD				dwOffset;
	DWORD               dwStreamIndex;

} TBitstreamBuffer;

#else
//! a data structure about media bitstream buffer
typedef struct
{
	//! pointer of buffer storing bitstream
	BYTE				*pbyBuffer;
	//! total size of bistream buffer
	DWORD				dwBufSize;
	//! a length of used media data size
	DWORD				dwBytesUsed;
	//! the frame type in the bitstream buffer
	TMediaDBFrameType	tFrameType;

	DWORD				dwSecond;
	DWORD				dwMilliSecond;
	DWORD				dwStreamType;
	DWORD				*pdwPacketSize;
	//DWORD				pdwPacketSize[41];
	DWORD				dwOffset;	
	DWORD				dwIntelligentVideoLength;	   //added by Louis for Intelligent Video 2007/11/14
	BOOL				bChangeSetting;
	DWORD               dwStreamIndex;
	DWORD				dwIsBoundary;
    //20141110 added by Charles for ONVIF Profile G
    BOOL                bOnvifDbit;
#ifdef _SHARED_MEM
	WORD				wPacketCount;
	DWORD				dwCurrentPosition;
	TJPEGoverRTPInfo	tJPEGoverRTPInfo;
	TH264overRTPInfo	tH264overRTPInfo;
    TH265overRTPInfo	tH265overRTPInfo;
#endif 
#ifdef _DIAGNOSIS
	BOOL				bSignal;
#endif // _DIAGNOSIS
#ifdef _VIDEO_REQUEST_INFORMATION_CONTROL
    EVideoFormat    	vFormat;
    DWORD               dwChNum;
    DWORD               dwQuant;
    HANDLE              hVRICtrl;
    DWORD				dwOffset; // VIVO_TAG
#endif
} TBitstreamBuffer;

#endif 
#endif //__BITSTREAMBUFDEF_H__
