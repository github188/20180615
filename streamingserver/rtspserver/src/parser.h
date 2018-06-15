#ifndef _PARSE_H_
#define _PARSE_H_


#ifdef __cplusplus
extern "C"
{
#endif

#include "rtsp_server_local.h"

#define	RTSP_DEFAULT_PORT 554
#define HDRBUFLEN         2048
#define TOKENLEN          100

//20130924 added by Charles for ondemand multicast
#define	ONDEMAND_MULTICAST_LEN 30

#define DESCRIBE_TKN    "DESCRIBE"
#define SETUP_TKN       "SETUP"
#define PLAY_TKN        "PLAY"
#define PAUSE_TKN       "PAUSE"
#define TEARDOWN_TKN    "TEARDOWN"
#define OPTIONS_TKN		"OPTIONS"
#define SETPARAM_TKN    "SET_PARAMETER"
//20101006 Added by danny for support GET_PARAMETER command for keep-alive in RTSP signaling
#define GETPARAM_TKN    "GET_PARAMETER"

/* method codes */
#define RTSP_SETUP_METHOD     0
#define RTSP_PLAY_METHOD      1
#define RTSP_PAUSE_METHOD     2
#define RTSP_TEARDOWN_METHOD  3
#define RTSP_DESCRIBE_METHOD  4
#define RTSP_OPTIONS_METHOD   5
#define RTSP_SETPARAM_METHOD  6
//20101006 Added by danny for support GET_PARAMETER command for keep-alive in RTSP signaling
#define RTSP_GETPARAM_METHOD  7

/*
 * method response codes.  These are 100 greater than their
 * associated method values.  This allows for simplified
 * creation of event codes that get used in event_handler()
 */
#define RTSP_SETUP_RESPONSE      100
#define RTSP_GET_RESPONSE        101
#define RTSP_REDIRECT_RESPONSE   102
#define RTSP_PLAY_RESPONSE       103
#define RTSP_PAUSE_RESPONSE      104
#define RTSP_SESSION_RESPONSE    105
#define RTSP_HELLO_RESPONSE      106
#define RTSP_RECORD_RESPONSE     107
#define RTSP_CLOSE_RESPONSE      108
#define RTSP_GET_PARAM_RESPONSE  109
#define RTSP_SET_PARAM_RESPONSE  110
#define RTSP_EXTENSION_RESPONSE  111

#define AUTH_USER_LENGTH			65
#define	AUTH_NONCE_LENGTH			50
#define AUTH_RESPONCE_LENGTH		200 //user name 64, password 64, base 64 encode
//20131111 added by Charles for digest authentication fail when using ffmpeg
#define	AUTH_URI_LENGTH			RTSP_URL_LEN

#ifdef _SHARED_MEM
/* 20100402 Media on Demand */
#define RTSPMOD_STIME_KEYWORD			"stime"
#define RTSPMOD_ETIME_KEYWORD			"etime"
#define RTSPMOD_LOCTIME_KEYWORD			"loctime"
#define RTSPMOD_LENGTH_KEYWORD			"length"
#define RTSPMOD_FILE_KEYWORD			"file"
#define RTSPMOD_LOC_KEYWORD				"loc"
#define RTSPMOD_MODE_KEYWORD			"mode"
#define RTSPMOD_NORMAL_MODE_KEYWORD		"normal"
#define RTSPMOD_DOWNLOAD_MODE_KEYWORD	"download"
#define RTSPMOD_SYNC_MODE_KEYWORD		"sync"

#define RTSPMOD_LOCTIME_LENGTH			2				//0 or 1
#define RTSPMOD_LENGTH_LENGTH			12				//Positive integer
#endif

typedef struct
{
	int	 iAuthMode;
	char acUserName[AUTH_USER_LENGTH];
	char acNonce[AUTH_NONCE_LENGTH];
	char acResponse[AUTH_RESPONCE_LENGTH];
	char acNonceCount[9];
	char acCNonce[AUTH_NONCE_LENGTH];
	char acURI[AUTH_URI_LENGTH];
}TRAWAUTHORINFO;

int   RTSPServer_GetCSeq(RTSP_CLIENT* pClient);
char* RTSPServer_GetState( int iCode );
#ifdef _SHARED_MEM
/* 20100428 Added For Media on demand */
char* RTSPServer_GetMODHeaderField( int iCode );
#endif
void  RTSPServer_RemoveMessage( int len, RTSP_CLIENT* pClient);
void  RTSPServer_DiscardMessage( RTSP_CLIENT* pClient );
int   RTSPServer_IsValidMethod( char *pcFirstLine,RTSP_CLIENT *pClient,RTSP_SERVER *pServer);
int   RTSPServer_GetMessageLen( int *piHdrLen, int *piBodyLen, RTSP_CLIENT* pClient);
int   RTSPServer_GetString( char *pcMsgBuffer, int iMsgHeaderLen, char *pcToken, char *pcSep, char* pcStringLine, int iBufLength);
void  RTSPServer_StrToLower( char *pcSource, int iLen );
int   RTSPServer_GetClientRTPPort(char* pcBuf,unsigned short *pusRTPPort,unsigned short *pusRTCPPort);
int   RTSPServer_GetNumber( char *pcMsgBuffer, int iMsgHeaderLen, char *pcToken, char *pcSep, unsigned long *pulNumber, long *plNumber );
int   RTSPServer_GetInterleavedID(char *pcMsgBuffer,int *piRTPID, int *piRTCPID);
int	  RTSPServer_GetAuthorInfo(char *pcMsgBuffer,TRAWAUTHORINFO *ptRawAuthorInfo);
/* Modified for Shmem*/
int RTSPServer_ParseURL(const char *pcURL, RTSP_CLIENT *pClient, unsigned short *pusPort, EParseMethod eMethod);
int RTSPServer_ParseOndemandMulticastInfo(RTSP_CLIENT *pClient, int iMediaType, bool *pbWithOnDemandMulticast);
int CheckIfRepeatMulticastInfo(RTSP_CLIENT *pClient, RTSP_SERVER *pServer);


int RTSPServer_ParseExtraInfo(RTSP_CLIENT* pClient);
#ifdef _SHARED_MEM
int RTSPServer_ParseMODInfo(RTSP_CLIENT* pClient);
#endif

#ifdef _cplusplus
}
#endif

#endif  /* _PARSE_H_ */

