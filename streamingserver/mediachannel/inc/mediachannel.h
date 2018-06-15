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
 *  Module name         :   RTPRTCPChannelAPI.h
 *  Module description  :   Handle media transmission of RTP/RTCP channel to client site
 *  Author              :   Simon
 *  Created at          :   2002/4/24
 *  Revision            :   1.0
 ******************************************************************************
 *                        Revision history
 ******************************************************************************
 *
 * $Log: /RD_1/Protocol/RTSP/Server/rtspstreamserver/mediachannel/src/mediachannel.h $
 * 
 * 2     06/05/18 3:26p Shengfu
 * update to version 1.4.0.0
 * 
 * 6     06/01/10 5:48p Shengfu
 * update to 1.3.0.6
 * 
 * 5     05/11/03 11:57a Shengfu
 * update to version 1.3.0.3
 * 
 * 4     05/09/27 1:14p Shengfu
 * update to version 1,3,0,1
 * 
 * 3     05/08/19 11:49a Shengfu
 * 
 * 1     05/08/19 11:28a Shengfu
 * 
 * 2     05/08/10 9:01a Shengfu
 * update rtspstreaming server which enable multicast
 * 
 * 5     05/04/15 1:35p Shengfu
 * 1. multicast added, but disable
 * 2. RTP extension added
 * 
 * 3     04/12/20 2:34p Shengfu
 * update to version 1.1.0.0
 * 
 * 2     04/09/16 4:33p Shengfu
 * 
 * 1     04/09/14 9:36a Shengfu
 * 
 * 1     04/09/14 9:16a Shengfu
 * 
 * 8     03/12/05 5:27p Shengfu
 * update to version 0102j
 * 
 * 5     03/10/24 4:07p Shengfu
 * 
 * 6     03/08/27 3:51p Shengfu
 * 
 * 2     03/04/17 2:44p Shengfu
 * update to 0102b
 * 
 * 2     02/06/08 1:09p Simon
 * Add doc code
 */

#ifndef	_RTPRTCPCHANNELAPI_H_
#define _RTPRTCPCHANNELAPI_H_

#include "osisolate.h"
#include "typedef.h"
#include "common.h"
#include "sockdef.h"
#include "rtsprtpcommon.h"
#include "streamserver.h"

/*! Type of Audio & Video for Media Channel */
#define RTPRTCPCHANNEL_MEDIATYPE_AUDIO		1
#define RTPRTCPCHANNEL_MEDIATYPE_VIDEO		2
//20120801 added by Jimmy for metadata
#define RTPRTCPCHANNEL_MEDIATYPE_METADATA		3

#define TCP_REQUEST_CS    1
#define TCP_RELEASE_CS    2

/*! This is the RTPRTCP intial parameters */
typedef struct
{
	/*! Media Channel Type (Must be audio or video)*/
	int iRTPRTCPMediaType;
	/*! Pointer to the head of bit stream*/
	BYTE* pbyMPEG4StartBitStream;
	/*! length of bit stream*/
	int iMPEG4StartBitStreamLength;
	/*! timeout value of RTCP*/
	int iRTCPTimeOut;
	/*! Priority of media channel thread*/
	unsigned long ulThreadPriority;
	/*! UDP mode RTSP server behind NAT sockets /Socket for RTP*/
	int iUDPRTPSock;
	/*! UDP mode RTSP server behind NAT sockets /Socket for RTCP*/
	int	iUDPRTCPSock;
	
} RTPRTCPCHANNEL_PARAM;

/*! This structure detail the session information for each connection */
typedef struct
{
	/*! Session ID*/
	DWORD	dwSessionID;
	//20111205 Modified by danny For UDP mode socket leak
	/*! socket to send RTP*/
	SOCKET	*psktRTP;
	/*! socket to send RTCP*/
	SOCKET	*psktRTCP;
	/*! RTPRTCP composer handle*/
	HANDLE	hRTPRTCPComposerHandle;	
    /*! RTP-over-UDP or RTP-over-TCP or RTP-over-HTTP*/
    int		iRTPStreamingType;
    /*! RTP ID*/
	int		iEmbeddedRTPID;
    /*! RTCP ID*/
	int		iEmbeddedRTCPID;
    /*! RTSP socket for RTP-over-TCP mode*/
	//20110706 Modified by danny For TCP/HTTP mode socket not sync in multi thread
	SOCKET	*psktRTSPSocket;
	/*! 1 means vivotek client*/
	int		iVivotekClient;
	/*! Codec Index*/
	int		iCodecIndex;
	//20101018 Add by danny for support multiple channel text on video
	/*! MultipleChannel Channel Index*/
	int		iMultipleChannelChannelIndex;
    //20141110 added by Charles for ONVIF Profile G
     /*! Cseq Index*/
    int     iCseq;
	/*! Address for client RTP when Server in NAT*/
   	RTSP_SOCKADDR RTPNATAddr;
	/*! Address for client when Server in NAT*/
	RTSP_SOCKADDR RTCPNATAddr;

#ifdef _SHARED_MEM
	//20101020 Add by danny for support seamless stream TCP/UDP timeout
	bool				bSeamlessStream;
	/*! Shmem media info */
	TShmemMediaInfo	  *ptShmemMediaInfo;
#endif

#ifdef RTSPRTP_MULTICAST
	//20110627 Add by danny for join/leave multicast group by session start/stop
	unsigned long	ulMulticastAddress;
	unsigned short	usMulticastRTCPPort;
	//20110725 Add by danny For Multicast RTCP receive report keep alive
	int             iRRAlive;
#endif
	//20160601 add by faber, assign each mutex in a session
	HANDLE hTCPMuxCS;
	//20170517 add by faber, notify source with output start in media channel
	int iNotifySource;
	int iSdpIndex;
} RTPRTCPCHANNEL_CONNECTION;

typedef struct
{
	/*! Session ID in Receiver Report*/
	DWORD  dwSessionID;	
	/*! Packet loss rate*/
	DWORD  iPacketLossRate;
	/*! Jitter rate*/
	DWORD  iJitter;	

} RTPRTCPCHANNEL_RECEIVERREPORT;
	
#define RTPRTCPCHANNEL_CALLBACKFLAG_GET_MEDIABUF			1
#define RTPRTCPCHANNEL_CALLBACKFLAG_SEND_EMPTYBUF			2
#define RTPRTCPCHANNEL_CALLBACKFLAG_SESSION_TIMEOUT			3
#define RTPRTCPCHANNEL_CALLBACKFLAG_SESSION_CLOSED			4
#define RTPRTCPCHANNEL_CALLBACKFLAG_RECEIVERREPORT			5
#define RTPRTCPCHANNEL_CALLBACKFLAG_FORCE_I_FRAME			6
#define RTPRTCPCHANNEL_CALLBACKFLAG_MULTICAST_CLOSED		7 
#define RTPRTCPCHANNEL_CALLBACKFLAG_REMOVE_OK				8
#define RTPRTCPCHANNEL_CALLBACKFLAG_SHMEM_GETBUF			9
#define RTPRTCPCHANNEL_CALLBACKFLAG_SHMEM_SELECT			10
///20110711 Modified by danny For Avoid frame type not match CI
#define RTPRTCPCHANNEL_CALLBACKFLAG_CHECK_FRAMETYPE_MATCH_CI		11

#define RTPRTCPCHANNEL_CALLBACKFLAG_SESSION_START			12
/*! Callback Function Prototype */
/*!
********************************************************************************************************
* \brief
* CallBack Function of media channel to Control Module
* \param hParentHandle
* \a (i) handle of control module which created media channel
*
* \param uMsgFlag
* \a (i) the flag used to identify which action should take to the callback function
* 
* \param pvParam1
* \a (i/o) the first parameter use to callback to control module
* 
* \param pvParam1
* \a (i/o) the second parameter use to callback to control module
*
* \note
in uMsgFlag case : RTPRTCPCHANNEL_CALLBACKFLAG_GET_MEDIABUF
    pvParam1: get DATA buffer from control component including the timeout mechanism in milliSecond unit. 
              int datatype. The negative value is meaning infinite timeout.  
    pvParam2: put the DATA buffer pointer.     
              MEDIABUFFER ** datatype. 
    return value: 0 is OK, negative value means timeout or error          


in uMsgFlag case : RTPRTCPCHANNEL_CALLBACKFLAG_SEND_EMPTYBUF
    pvParam1: send EMPTY buffer to control component. 
              MEDIABUFFER * datatype.  
    pvParam2: not used
    return value: 0 is OK, negative value means error          


in uMsgFlag case : RTPRTCPCHANNEL_CALLBACKFLAG_SESSION_TIMEOUT
    pvParam1: send timeout session ID to control component. 
              DWORD datatype.  
    pvParam2: not used
    return value: 0 is OK, negative value means error          


in uMsgFlag case : RTPRTCPCHANNEL_CALLBACKFLAG_SESSION_CLOSED
    pvParam1: send closed session ID to control component.(socket is closed by RTP/RTCP Channel component) 
              DWORD datatype.  
    pvParam2: not used
    return value: 0 is OK, negative value means error          
    

in uMsgFlag case : RTPRTCPCHANNEL_CALLBACKFLAG_RECEIVERREPORT
    pvParam1: send the receiver report to control component.
              RTPRTCPCHANNEL_RECEIVERREPORT * datatype.  
    pvParam2: not used
    return value: 0 is OK, negative value means error          

in uMsgFlag case : RTPRTCPCHANNEL_CALLBACKFLAG_FORCE_I_FRAME
    pvParam1: Callback to force I framce transmit.
              Param 1 must specify which stream to callback to. 
    pvParam2: not used
    return value: 0 is OK, negative value means error   

in uMsgFlag case : RTPRTCPCHANNEL_CALLBACKFLAG_MULTICAST_CLOSED
    pvParam1: Callback to notify that errors occurred and multicast must be closed.
              Param 1 must specify which stream to callback to.
    pvParam2: not used
    return value: 0 is OK, negative value means error   

in uMsgFlag case : RTPRTCPCHANNEL_CALLBACKFLAG_REMOVE_OK
    pvParam1: notify that channel is removed.
              Param 1 is Session ID of removed session.
    pvParam2: not used
    return value: 0 is OK, negative value means error   

in uMsgFlag case : RTPRTCPCHANNEL_CALLBACKFLAG_SHMEM_GETBUF
    pvParam1: pointer to the share memory buffer
    pvParam2: not used
    return value: 0 is OK, negative value means error   
    
in uMsgFlag case : RTPRTCPCHANNEL_CALLBACKFLAG_SHMEM_CHECKUNIXSOCKET
    pvParam1: Timeout in milliseconds
    pvParam2: not used
    return value: 0 is OK, negative value means error   
*/
typedef int (*RTPRTCPCHANNELCALLBACK)(HANDLE hParentHandle, UINT uMsgFlag, void * pvParam1, void * pvParam2);

/*!
 *******************************************************************************
 * \brief
 * Create the instance for media channel
 *
 * \param iMaxSessionNumber
 * \a (i) Maximum number of sessions allowed
 *
 * \param pstRTPRTCPChannelParameter
 * \a (i) pointer to RTPRTCPCHANNEL_PARAM structure
 *
 * \retval Handle of media channel
 * Create the instance ok.
 * \retval NULL
 * Create the instance failed. 
 *
 * \note
 * This function will create and initialize the media channel
 *
 *
 **************************************************************************** */
HANDLE RTPRTCPChannel_Create( int iMaxSessionNumber, RTPRTCPCHANNEL_PARAM *pstRTPRTCPChannelParameter);
/*!
 *******************************************************************************
 * \brief
 * Start the media channel
 *
 * \param hRTPRTCPChannelHandle
 * \a (i) Handle to the media channel
 *
 * \retval 0
 * Start the media channel ok.
 * \retval other
 * Start the media channel failed. 
 *
 * \note
 * This function will start the media channel
 *
 **************************************************************************** */
int RTPRTCPChannel_Start(HANDLE hRTPRTCPChannelHandle);
/*!
 *******************************************************************************
 * \brief
 * Stop the media channel
 *
 * \param hRTPRTCPChannelHandle
 * \a (i) Handle to the media channel
 *
 * \retval 0
 * Stop the media channel ok.
 * \retval other
 * Stop the media channel failed. 
 *
 * \note
 * This function will Stop the media channel
 *
 **************************************************************************** */
int RTPRTCPChannel_Stop(HANDLE hRTPRTCPChannelHandle);
/*!
 *******************************************************************************
 * \brief
 * Release the media channel
 *
 * \param hRTPRTCPChannelHandle
 * \a (i) Handle to the media channel
 *
 * \retval 0
 * Release the media channel ok.
 * \retval other
 * Release the media channel failed. 
 *
 * \note
 * This function will Release the media channel & resources
 *
 **************************************************************************** */
int RTPRTCPChannel_Release(HANDLE hRTPRTCPChannelHandle);
/*!
 *******************************************************************************
 * \brief
 * Set callback for Media channel
 *
 * \param hRTPRTCPChannelHandle
 * \a (i) Handle to the media channel
 *
 * \param fCallback
 * \a (i) Callback function of media channel
 *
 * \param hParentHandle
 * \a (i) Handle for callback to pass
 *
 * \retval 0
 * Set callback ok.
 * \retval other
 * Set callback failed. 
 *
 * \note
 * This function will Set the callback function, be sure to call it before using media channel
 *
 **************************************************************************** */
int RTPRTCPChannel_SetCallback(HANDLE hRTPRTCPChannelHandle, RTPRTCPCHANNELCALLBACK fCallback, HANDLE hParentHandle);
/*!
 *******************************************************************************
 * \brief
 * Set parameters for Media channel
 *
 * \param hRTPRTCPChannelHandle
 * \a (i) Handle to the media channel
 *
 * \param pstVideoEncodingParameter
 * \a (i) pointer to the RTPRTCPCHANNEL_PARAM structure
 *
 * \retval 0
 * Set parameter ok.
 * \retval other
 * Set parameter failed. 
 *
 * \note
 * This function will set the parameters for media channel.
 *
 **************************************************************************** */
int RTPRTCPChannel_SetParameters(HANDLE hRTPRTCPChannelHandle, RTPRTCPCHANNEL_PARAM *pstVideoEncodingParameter);
/*!
 *******************************************************************************
 * \brief
 * Add one session for Media channel
 *
 * \param hRTPRTCPChannelHandle
 * \a (i) Handle to the media channel
 *
 * \param pstRTPRTCPSession
 * \a (i) pointer to the RTPRTCPCHANNEL_CONNECTION structure which contains session information
 *
 * \retval 0
 * Add session ok.
 * \retval other
 * Add session failed. 
 *
 * \note
 * This function will add one session to media channel.
 *
 **************************************************************************** */
int RTPRTCPChannel_AddOneSession(HANDLE hRTPRTCPChannelHandle, RTPRTCPCHANNEL_CONNECTION * pstRTPRTCPSession);
/*!
 *******************************************************************************
 * \brief
 * Pause one session for Media channel
 *
 * \param hRTPRTCPChannelHandle
 * \a (i) Handle to the media channel
 *
 * \param dwSessionID
 * \a (i) session ID of the specified session
 *
 * \retval 0
 * Pause session ok.
 * \retval other
 * Pause session failed. 
 *
 * \note
 * This function will Pause one session to media channel.
 *
 **************************************************************************** */
int RTPRTCPChannel_PauseOneSession(HANDLE hRTPRTCPChannelHandle,DWORD dwSessionID);
/*!
 *******************************************************************************
 * \brief
 * Resume one session for Media channel
 *
 * \param hRTPRTCPChannelHandle
 * \a (i) Handle to the media channel
 *
 * \param dwSessionID
 * \a (i) session ID of the specified session
 *
 * \retval 0
 * Resume session ok.
 * \retval other
 * Resume session failed. 
 *
 * \note
 * This function will Resume one session to media channel.
 *
 **************************************************************************** */
int RTPRTCPChannel_ResumeOneSession(HANDLE hRTPRTCPChannelHandle,DWORD dwSessionID);
/*!
 *******************************************************************************
 * \brief
 * Remove one session from media channel
 *
 * \param hRTPRTCPChannelHandle
 * \a (i) Handle to the media channel
 *
 * \param dwSessionID
 * \a (i) session ID of the specified session
 *
 * \retval 0
 * Remove session ok.
 * \retval other
 * Remove session failed. 
 *
 * \note
 * This function will Remove one session from media channel.
 *
 **************************************************************************** */
int RTPRTCPChannel_RemoveOneSession(HANDLE hRTPRTCPChannelHandle, DWORD dwSessionID);
/*!
 *******************************************************************************
 * \brief
 * Add Critical Section Handle for media channel
 *
 * \param hRTPRTCPChannelHandle
 * \a (i) Handle to the media channel
 *
 * \param hTCPMuxCS
 * \a (i) Handle to Critical Section
 *
 * \retval 0
 * Add Critical Section Handle ok.
 * \retval other
 * Add Critical Section Handle failed. 
 *
 * \note
 * This function will Remove one session from media channel.
 *
 *****************************************************************************/
int RTPRTCPChannel_AddTCPMuxHandle(HANDLE hRTPRTCPChannelHandle,HANDLE hTCPMuxCS);

#ifdef RTSPRTP_MULTICAST
/*!
 *******************************************************************************
 * \brief
 * Add one session for multicast media channel
 *
 * \param hRTPRTCPChannelHandle
 * \a (i) Handle to the media channel
 *
 * \param pstRTPRTCPSession
 * \a (i) pointer to the RTPRTCPCHANNEL_CONNECTION structure which contains session information
 *
 * \param iGroupIndex
 * \a (i) Stream number index (1 or 2 when Dual stream)
 *
 * \retval 0
 * Add session ok.
 * \retval other
 * Add session failed. 
 *
 * \note
 * This function will add one session to media channel.
 *
 **************************************************************************** */
int RTPRTCPChannel_AddMulticastSession(HANDLE hRTPRTCPChannelHandle, RTPRTCPCHANNEL_CONNECTION * pstRTPRTCPSession,int iGroupIndex);
/*!
 *******************************************************************************
 * \brief
 * Remove multicast session for Media channel
 *
 * \param hRTPRTCPChannelHandle
 * \a (i) Handle to the media channel
 *
 * \param iGroupIndex
 * \a (i) stream index number
 *
 * \retval 0
 * Remove session ok.
 * \retval other
 * Remove session failed. 
 *
 * \note
 * This function will Remove multicast session to media channel. Make sure there are no more clients for this multicast stream.
 *
 **************************************************************************** */
int RTPRTCPChannel_RemoveMulticastSession(HANDLE hRTPRTCPChannelHandle,int iGroupIndex);
#endif

/*!
 *******************************************************************************
 * \brief
 * 20100319 Callback from RTSP Server to keep-alive the session
 *
 * \param hRTPRTCPChannelHandle
 * \a (i) Handle to the media channel
 *
 * \param SessionID
 * \a (i) Session ID of the client
 *
 * \retval S_OK
 * Keep alive ok.
 * \retval S_FAIL
 * Keep alive failed. 
 *
 * \note
 * This function serves as a keep-alive, useful for client that does not send Receiver Report.
 *
 **************************************************************************** */
SCODE RTPRTCPChannel_SessionKeepAlive(HANDLE hRTPRTCPChannelHandle, DWORD dwSessionID);

#ifdef _SHARED_MEM
//20101018 Add by danny for support multiple channel text on video
/* Added 20080815 to support Shmem architecture */
SCODE RTPRTCPChannel_SetLocation(HANDLE hRTPRTCPChannelHandle, char* pcLocation, int iMultipleChannelChannelIndex);
#endif

#ifdef RTSPRTP_MULTICAST
//20100714 Added by danny For Multicast parameters load dynamically
DWORD RTPRTCPChannel_CheckMulticastAddAvailable(HANDLE hChannel, int iMulticastCount);
#endif

//20141110 added by Charles for ONVIF Profile G
SCODE RTPRTCPChannel_SetPlayCseq(HANDLE hRTPRTCPChannelHandle, DWORD dwSessionID, int iMulticastIndex, int iCseq);

#endif  //_RTPRTCPCHANNELAPI_H_


