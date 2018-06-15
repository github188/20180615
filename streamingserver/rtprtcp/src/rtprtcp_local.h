

#ifndef __RTP_H
#define __RTP_H

#include <string.h>
#include "osisolate.h"
#include "common.h"
#include "typedef.h"
#include "sockdef.h"
#include "rtprtcp.h"
#include "timezonemmap.h"

#ifndef UINT8	// ui
typedef unsigned char  UINT8;
#endif

#ifndef UINT16	// ui
typedef unsigned short  UINT16;
#endif

#ifndef UINT32	// ui
typedef unsigned long  UINT32;
#endif

#ifndef INT32	// ui
typedef long  INT32;
#endif

//#define VIDEO                     1
//#define AUDIO                     0
#define MAXSDES                   255
#define MAXRTPSESSIONS            10
#define MAXRTPSESSIONMEMBERS      50
#define MAXRTCPPACKET             1470

#define MAXIPS                    20
#define MAX_DROPOUT               3000
#define MAX_MISORDER              100
#define MIN_SEQUENTIAL            2
#define RTP_SEQ_MOD               0x10000

/*This is workaround to avoid RTPtimestamp inaccurate when go to time in playback*/
#define MAX_FRAME_TIMEGAP           2  


/* RTCP header bit locations - see the standard */
#define HEADER_V                  30      /* version                       */
#define HEADER_P                  29      /* padding                       */
#define HEADER_RC                 24      /* reception report count        */
#define HEADER_PT                 16      /* packet type                   */
#define HEADER_len                0       /* packet length in 32-bit words */

/* RTCP header bit field lengths - see the standard */
#define HDR_LEN_V                 2       /* version                       */
#define HDR_LEN_P                 1       /* padding                       */
#define HDR_LEN_RC                5       /* reception report count        */
#define HDR_LEN_PT                8       /* packet type                   */
#define HDR_LEN_len               16      /* packet length in 32-bit words */

/* used to overcome byte-allignment issues */
#define SIZEOF_RTCPHEADER         32 * 2
#define SIZEOF_SR                 32 * 5
#define SIZEOF_RR                 32 * 6

#define SIZEOF_SDES(sdes)         (((sdes).length + 6) & 0xfc)

/* initial bit field value for RTCP headers: V=2,P=0,RC=0,PT=0,len=0 */
#define RTCP_HEADER_INIT          0x80000000


#define reduceNNTP(a) (((a).msdw<<16)+((a).lsdw>>16))

typedef struct 
{
   unsigned long msdw;
   unsigned long lsdw;
} UINT64_NTP; 


typedef enum {
   RTCP_SR   = 200,               /* sender report            */
   RTCP_RR   = 201,               /* receiver report          */
   RTCP_SDES = 202,               /* source description items */
   RTCP_BYE  = 203,               /* end of participation     */
   RTCP_APP  = 204                /* application specific     */
} rtcpType;


typedef enum {
   RTCP_SDES_END   = 0,
   RTCP_SDES_CNAME = 1,
   RTCP_SDES_NAME  = 2,
   RTCP_SDES_EMAIL = 3,
   RTCP_SDES_PHONE = 4,
   RTCP_SDES_LOC   = 5,
   RTCP_SDES_TOOL  = 6,
   RTCP_SDES_NOTE  = 7,
   RTCP_SDES_PRIV  = 8
} rtcpSDesType;



typedef struct
{
   UINT64_NTP  tNNTP;
   UINT32  tRTP;
        
   UINT32  nPackets;
   UINT32  nBytes;
} rtcpSR;

typedef struct
{
   UINT32  ssrc;
   UINT32  bfLost;      /* 8Bit fraction lost and 24 bit cumulative lost */
   UINT32  nExtMaxSeq;
   UINT32  nJitter;
   UINT32  tLSR;
   UINT32  tDLSR;
} rtcpRR;


typedef struct 
{
   UINT8  type;
   UINT8  length;
   char   value[MAXSDES + 1];     /* leave a place for an asciiz */
} rtcpSDES;

typedef struct {
   UINT16  max_seq;               /* highest seq. number seen */
   UINT32  cycles;                /* shifted count of seq. number cycles */
   UINT32  base_seq;              /* base seq number */
   UINT32  bad_seq;               /* last 'bad' seq number + 1 */
   UINT32  probation;             /* sequ. packets till source is valid */
   UINT32  received;              /* packets received */
   UINT32  expected_prior;        /* packet expected at last interval */
   UINT32  received_prior;        /* packet received at last interval */
   UINT32  transit;               /* relative trans time for prev pkt */
   UINT32  jitter;                /* estimated jitter */
   /* ... */
} rtpSource;

typedef struct
{
   UINT32    ssrc; 
   rtcpSDES  CName;
} rtcpInfo;

typedef struct
{
  UINT64_NTP IndexTime;
  int    avg_pck_size;
  int    Initial;
}rtcpInterval; 

typedef struct 
{
    int		       iInvalid;
	int            iMediaType;

	UINT32         ulTimestamp;     // timestamp
    UINT16         usSeq;           // seq No.

    UINT32         ulInitTimestamp; 
    int            iForceSenderReport;
    
    
	UINT64_NTP	   stNTPStartTime;// the time server start to send RTP packets
    UINT32         ulSampleRate;
	UINT64_NTP     ulRTCPStartTime; // the time server start to send RTCP packets
    rtcpSR         SR;            // all information of Sender Report
    rtcpRR         RR;            // all information of Receive Report
	rtcpInfo       Server;       
	rtcpInfo       Client;
	rtcpInterval   rtcpItv;

	int            RRCount;      // Count of Receiver report from client...used in timeout
	//20141110 added by Charles for ONVIF Profile G
	BOOL           bOnvifRTPExt;
    UINT32         ulIncrementTimestamp;     // timestamp Increment
} rtpSession;


typedef struct 
{
	BYTE    V;
	BYTE    P;
	BYTE    X;
	BYTE    CC; 
    BYTE    marker;
    BYTE    payloadType;
	UINT16   sequenceNumber;
	UINT32  timestamp;
    INT32   ssrc;

 /* total length of rtp header */
    int     len;

} rtpHeader;

typedef struct
{
   UINT32  bits; 
   UINT32  ssrc; 
} rtcpHeader;


#ifdef __cplusplus
extern "C" {
#endif

#define MAX_UDPSIZE 2048
#define RTP_PORTNUM_VIDEO "4588"
#define RTCP_PORTNUM_VIDEO "4589"
#define RTP_PORTNUM_AUDIO "4590"
#define RTCP_PORTNUM_AUDIO "4591"

// Added by Jeffrey 2007/03/30
UINT32 RTPRTCP_BitFieldGet(UINT32 uiValue, int iStartBit, int iBits);
UINT32 RTPRTCP_BitFieldSet(UINT32 uiValue, UINT32 uiBitField, int iStartBit, int iBits);
void RTPRTCP_SetSDES(rtcpSDesType eType, rtcpSDES* ptSdes, char *pcData, int iLength);
int RTPRTCP_ConvertHeader2h(UINT32 *plBuff, int iStartIndex, int iSize);
int RTPRTCP_ConvertHeader2l(UINT8 *piBuff, int iStartIndex, int iSize);
UINT32 RTPRTCP_GetTimeStamp(RTPMEDIABUFFER *ptBuf, rtpSession* ptRTPSession);
void RTPRTCP_MakeHeader(rtcpHeader* ptHeader, UINT32 uiSsrc, UINT8 uiCount, rtcpType eType, UINT16 uiDataLen);
int RTPRTCP_ProcessRTCPPacket(char* pcData, INT32 iDataLen, rtcpType eType, INT32 iReportCount, rtpSession *ptRTPSession);
void RTPRTCP_SetSDES(rtcpSDesType eType, rtcpSDES* ptSdes, char *pcData, int iLength);
void RTPRTCP_Time2MinSeconds(UINT64_NTP* pNTP64, unsigned long ulSeconds, unsigned long ulMSeconds);
//20140108 added by Charles to Convert local time to UTC time
void RTPRTCP_GetUtcTime(UINT64_NTP* pUTC, UINT64_NTP tLocal);



#ifdef __cplusplus
}
#endif




#endif  /* __RTP_H */

