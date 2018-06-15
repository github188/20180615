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
 * rtsps_fdipc.h
 *
 * \brief
 * Exchange file descriptor between http server and rtsp server. (header file for rtsps_fdipc.c)
 *
 * \date
 * 2006/05/11
 *
 * \author
 * Rey Cheng
 *
 *
 *******************************************************************************
 */

#ifndef _RTSPS_FDIPC_H_
#define _RTSPS_FDIPC_H_

#include <string.h>
#include "fdipc.h"
#include "xmlsparser.h"
#include "rtsps_local.h"

#define WATCHDOG_SVR_SCK		"/var/run/swatchdog/swatchdog.sck"
#define WATCHDOG_WATCHLIST		"/etc/conf.d/config_watchlist.xml"
#define WATCHDOG_BUFFER_SIZE	128
#define WATCHDOG_KICK_INTERVAL	15
#define WATCHDOG_MAX_ID			1024

//20110401 Added by danny For support RTSPServer thread watchdog
#define RTSPSERVER_NOKICK_TIMEOUT	180 //300

/* 20090115 Swatchdog */
SCODE	InitializeSWatchDog(HANDLE hRTSPS);
SCODE   KickSWatchDog(HANDLE hRTSPS);
SCODE	ReleaseSWatchDog(HANDLE hRTSPS);

SCODE RTPOverHttpSocketExchanger(HANDLE hRTSPS);
SCODE RTSPSSetupFdIPCSocket(HANDLE hObject);

//20110401 Added by danny For support RTSPServer thread watchdog
SCODE CheckRTSPServerThreadAlive(HANDLE hObject);

#endif // _RTSPS_FDIPC_H_
