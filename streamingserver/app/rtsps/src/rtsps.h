/*
 *******************************************************************************
 * $Header: $
 *
 *  Copyright (c) 2000-2006 Vivotek Inc. All rights reserved.
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
 * $History: $
 * 
 *******************************************************************************
 */

/*!
 *******************************************************************************
 * Copyright 2000-2006 Vivotek, Inc. All rights reserved.
 *
 * \file
 * rtsps.h
 *
 * \brief
 * rtsp streaming server for Kilrogg.
 *
 * \date
 * 2006/04/21
 *
 * \author
 * Rey Cheng
 *
 *
 *******************************************************************************
 */

#ifndef _RTSPS_H_
#define _RTSPS_H_

#include <semaphore.h>
#include "typedef.h"
#include "rtsprtpcommon.h"
#ifndef _SHARED_MEM
#include "rtpmediaqueue.h"
#include "rtppacketizer.h"
#endif
#include "rtprtcp.h"
#include "mediachannel.h"
#include "rtspserver.h"
#include "rtspstreamingserver.h"
#include "datapacketdef.h"
#include "ubuffer.h"
#include "account_mgr_app.h"
#include "expat.h"
#include "message.h"

#ifdef _ANTI_COUNTERFEIT
#include "aclib.h"
#endif

#if 0
// --------------------- function brief ----------------------------------------
SCODE StreamingServer_Initial(HANDLE *phObject, TRTSPSInitOptions *pInitOpts, char* pzConfigFile, char* pzAccessFile, char* pzQosFile);
SCODE StreamingServer_Start(HANDLE hObject);
SCODE StreamingServer_Stop(HANDLE hObject);
SCODE StreamingServer_Release(HANDLE *phObject);
#endif

/*! FOUR_CC Version code of your \b RTSPS instance. */
#define RTSPS_VERSION   MAKEFOURCC( 1, 3, 6, 3 )
#define RTSPS_VERSION_STRING    "1.3.6.3"
#define RTSPS_MODIFY_DATETIME   "Last modified at 2016/11/04 10:00:00"

#define	RTSP_SYSLOG_ID_STRING		"[RTSP SERVER]"
#define RTSPS_PID_FILE				"/var/run/rtsps.pid"

#ifdef _SHARED_MEM
//20100105 Added For Seamless Recording
#define EVENTMGR_SVR_SCK		"/var/run/eventmgr/emsocket"
#endif

//20101123 Added by danny For support advanced system log 
#define SYSLOG_CONF		"/etc/syslog.conf"
#define ACCESS_LOG		"messages_access"

/*! RTSPS initial options */
typedef struct rtsps_initial_options
{
	/*! This is a version control parameter. 
	    Set this value as \b RTSPS_VERSION.*/
	DWORD			dwVersion;
	/*! RTSP streaming server port, default : 554 */
	DWORD			dwRTSPPort;
	/*! RTSP streaming server ip address */
	const char		szIPAddr[20];
	/*! RTSP streaming server subnet mask */
	const char		szSubnetMask[20];
	/*! RTSP streaming server stream number */
	DWORD			dwStreamNumber;
	/*! RTSP streaming server access names */
	char			szAccessName[MULTIPLE_STREAM_NUM][255];
} TRTSPSInitOptions;

/*!
******************************************************************************
* \brief
* Create handle of RTSPS object
*
* \param phObject
* \a (o) pointer to receive the handle of the RTSPS object
*
* \retval S_OK
* Create object ok
*
* \retval S_FAIL
* Create object failed
*
* \remark
* A RTSPS object shall be initialized before using it.
*
* \see RTSPStreamingServer_Release
*
******************************************************************************
*/
SCODE StreamingServer_Initial(HANDLE *phObject, TRTSPSInitOptions *pInitOpts, char* pzConfigFile, char* pzAccessFile, char* pzQosFile);

/*!
******************************************************************************
* \brief
* Start the operation of rtsp streaming server
*
* \param hObject
* Handle of the RTSPS object
*
* \retval S_OK
* Start rtsp streaming ok
*
* \retval S_FAIL
* Start rtsp streaming failed
*
* \remark
* Be sure to call RTSPStreamingServer_Initial() before calling this.
*
* \see RTSPStreamingServer_Stop
*
******************************************************************************
*/
SCODE StreamingServer_Start(HANDLE hObject);

/*!
******************************************************************************
* \brief
* Stop the operation of rtsp streaming server
*
* \param hObject
* Handle of the RTSPS object
*
* \retval S_OK
* Stop rtsp streaming ok
*
* \retval S_FAIL
* Stop rtsp streaming failed
*
* \see RTSPStreamingServer_Start
*
******************************************************************************
*/
SCODE StreamingServer_Stop(HANDLE hObject);

/*!
******************************************************************************
* \brief
* Release the resources of a RTSPS object
*
* \param phObject
* \a (i/o) pointer to the handle of the RTSPS object
*
* \retval S_OK
* Release object ok
*
* \retval S_FAIL
* Release object failed
*
* \see RTSPStreamingServer_Start
*
******************************************************************************
*/
SCODE StreamingServer_Release(HANDLE *phObject);

#ifdef _SHARED_MEM
//20100428 Added For Media on demand
/*!
******************************************************************************
* \brief
* Send CONTROL_MSG_FORCECI to MOD Media source control fifo
*
* \param hObject
* \a (i) handle of the RTSPS object
*
* \retval S_OK
* Send CONTROL_MSG_FORCECI to MOD Media source control fifo ok
*
* \retval S_FAIL
* Send CONTROL_MSG_FORCECI to MOD Media source control fifo failed
*
******************************************************************************
*/
SCODE StreamingServer_SetMODMediaTrackParam(HANDLE hObject, TMultipleStreamCIInfo *ptCIInfo);
#endif

/*!
******************************************************************************
* \brief
* Send CONTROL_MSG_FORCECI to Media source control fifo, then send CONTROL_MSG_START to AlwaysMulticast Media source control fifo
*
* \param hObject
* \a (i) handle of the RTSPS object
*
* \retval S_OK
* Send CONTROL_MSG to Media source control fifo ok
*
* \retval S_FAIL
* Send CONTROL_MSG to Media source control fifo failed
*
******************************************************************************
*/
SCODE StreamingServer_SetMediaTrackParam(HANDLE hObject, TMultipleStreamCIInfo *ptCIInfo);
/*!
******************************************************************************
* \brief
* Parse and load the account manager information
*
* \param hObject
* \a (i)  handle of the RTSPS object
*
*
******************************************************************************
*/
void StreamingServer_AccountManagerParse(HANDLE hObject);
/*!
******************************************************************************
* \brief
* Update RTSP server dynamic parameters
*
* \param hRTSPS
* \a (i) pointer to the handle of the RTSPS object
*
* \param pzConfigFile
* \a (i) string which contains the config information
*
* \retval 0
* Update RTSP server dynamic parameters ok
*
* \retval others
* Update RTSP server dynamic parameters failed
*
******************************************************************************
*/
int	StreamingServer_UpdateDynamicPamater(HANDLE hRTSPS, char* pzConfigFile);

//20090224 QOS
int	StreamingServer_UpdateQosParameters(HANDLE hRTSPS, char* pzQosFile);

#ifdef _SHARED_MEM
//20100105 Added For Seamless Recording
int	StreamingServer_UpdateSeamlessRecordingParameters(HANDLE hRTSPS);
SCODE StreamingServer_SetToConfigure(char* pzCmd);
SCODE StreamingServer_SetRecoderState(char* pzCmd);

//20100428 Added For Media on demand
SCODE StreamingServer_SetMODControlParam(HANDLE hObject, RTSPSERVER_MODREQUEST *pstRTSPServerMODRequest);
char* StreamingServer_GetMODCommand( int iCode );
#endif

#ifdef RTSPRTP_MULTICAST
//20100714 Added bu danny For Multicast parameters load dynamically
int StreamingServer_UpdateMulticastParameters(HANDLE hRTSPS, char* pzConfigFile);
#endif

//20101123 Added by danny For support advanced system log 
BOOL StreamingServer_IsAdvLogSupport();

#endif // _RTSPS_H_
