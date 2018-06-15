/*
 *******************************************************************************
 * $Header: /RD_1/Protocol/RTSP/Server/rtspstreamserver/rtsprtpcommon/src/rtsprtpcommon.c 2     06/05/18 3:26p Shengfu $
 *
 *  Copyright (c) 2000-2002 Vivotek Inc. All rights reserved.
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
 * $History: rtsprtpcommon.c $
 * 
 * *****************  Version 2  *****************
 * User: Shengfu      Date: 06/05/18   Time: 3:26p
 * Updated in $/RD_1/Protocol/RTSP/Server/rtspstreamserver/rtsprtpcommon/src
 * update to version 1.4.0.0
 * 
 * *****************  Version 3  *****************
 * User: Shengfu      Date: 05/08/19   Time: 11:49a
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/rtspstreamserver/rtsprtpcommon/src
 * 
 * *****************  Version 1  *****************
 * User: Shengfu      Date: 05/08/19   Time: 11:29a
 * Created in $/RD_1/Protocol/RTSP/Server/RTSPSTREAMSERVER/rtsprtpcommon/src
 * 
 * *****************  Version 2  *****************
 * User: Shengfu      Date: 05/08/10   Time: 9:01a
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/rtspstreamserver/rtsprtpcommon/src
 * update rtspstreaming server which enable multicast
 * 
 * *****************  Version 1  *****************
 * User: Shengfu      Date: 05/04/15   Time: 1:39p
 * Created in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/rtspstreamserver/RTSPRTPCOMMON/src
 * 
 * *****************  Version 1  *****************
 * User: Shengfu      Date: 05/03/31   Time: 01:58p
 * Created in $/RD_1/Protocol/Module/RTSP6K_SERVER/include
 * 
 *********************************************************************/
#include "rtsprtpcommon.h"

char *rtspstrcpy(char *acDst, const char *pcSrc, unsigned int uiSize)
{
	memset(acDst, 0, uiSize);
	return strncpy(acDst, pcSrc, uiSize - 1);
}

char *rtspstrcat(char *acDst, const char *pcSrc, unsigned int uiSize)
{
	strncat(acDst, pcSrc, uiSize - strlen(acDst) - 1);
	acDst[uiSize - 1] = 0;	
	return acDst;
}

#ifdef _SHARED_MEM
//20100428 Added For Media on demand
int RtspRtpCommon_TLVStrlen(char *qptr, int iMsgLength)
{	
	if (qptr != NULL)
	{
		if (iMsgLength >0 && iMsgLength <= 127)    
		{		
			*qptr++ = iMsgLength;
			//printf("[%s]return 1\n", __FUNCTION__);
			return 1;	
		}	
		else if (iMsgLength > 127 && iMsgLength <= 255)	
		{    	
			*qptr++ = 0x81;		
			*qptr++ = iMsgLength;
			//printf("[%s]return 2\n", __FUNCTION__);
			return 2;	
		}	
		else if (iMsgLength > 255 && iMsgLength <= 65535)	
		{		
			*qptr++ = 0x82;		
			*qptr++ = iMsgLength / 256;		
			*qptr++ = iMsgLength % 256;
			//printf("[%s]return 3\n", __FUNCTION__);
			return 3;	
		}	
		else	
		{		
			printf("[%s]Message length %d is not correct!\n", __FUNCTION__, iMsgLength);		
			return -1;	
		}
	}
	else
	{
		if (iMsgLength >0 && iMsgLength <= 127)    
		{		
			//printf("[%s]return 1\n", __FUNCTION__);
			return 1;	
		}	
		else if (iMsgLength > 127 && iMsgLength <= 255)	
		{    	
			//printf("[%s]return 2\n", __FUNCTION__);
			return 2;	
		}	
		else if (iMsgLength > 255 && iMsgLength <= 65535)	
		{
			//printf("[%s]return 3\n", __FUNCTION__);
			return 3;	
		}	
		else	
		{		
			printf("[%s]Message length %d is not correct!\n", __FUNCTION__, iMsgLength);		
			return -1;	
		}
	}
	
	return 0;
}
#endif
