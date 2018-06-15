
#ifndef _MEDIABUF_H_
#define _MEDIABUF_H_

#include <typedef.h>
#include "rtsprtpcommon.h"

typedef struct tagRTPMEDIABUFFER {
	/*! address of the Head of Header*/
	BYTE	* pbDataStart;	
	/*! number of bytes actually used in buffer*/
	DWORD	dwBytesUsed;	
	/*! Address of the head of bitstream*/	
	BYTE	* pbBufferStart;
	/*! length, in bytes, of buffer*/
	DWORD	dwBufferLength;
	/*! ( Video : 1=End of current frame, 0=otherwise)*/
	BOOL	bMarker;	    
	/*! 1=first buffer of I frame, 0=otherwise*/			          
	BOOL	bIFrame;        			
	/*! Timstamp in uSec*/
	unsigned long ulSeconds;
	/*! Timestamp in MSec*/
	unsigned long ulMSeconds;
    /*! Stream Index*/
	DWORD	dwStreamIndex;	
    /*! Length of Extension*/
	DWORD   dwExtensionLen;
#ifdef _SHARED_MEM
	/*! Amount of data remaining to be sent */
	DWORD	dwRemainingLength;
	/*! Marker if this packet has been in sent loop for more than one time*/
	BOOL	bHeaderComposed;
	/*! Calculate the header length offset for RTP restoring */
	int		iHeaderLength;
	/*! Amount of bitstream data for writev*/
	DWORD	dwBitStreamLength;
	/*! Size of the extraData (in Aggregate buffer)*/
	DWORD	dwRTPExtraData;
	/*! 12 bytes for RTSP header, 4 bytes for extension header, 4 byte for TCP embedded info, 8 bytes for possible RTP extra (RFC2435 or FU-A for H264)*/
	char    acPadBuffer[12 + 4 + 4 + 8 + 1]; 
#else
	/*! 4 bytes for extension header, 4 byte for TCP embedded info*/
	char    acPadBuffer[4+12+RTP_EXTENSION]; 
#endif


} RTPMEDIABUFFER;


#endif //_MEDIABUF_H_


