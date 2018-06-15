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
 * rtsps_ubuffer.h
 *
 * \brief
 * UBuffer reader for rtsp streaming server. (header file for rtsps_ubuffer.c)
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

#ifndef _RTSPS_UBUFFER_H_
#define _RTSPS_UBUFFER_H_

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "rtsps_local.h"

SCODE GetVideoUBuffer(TSTREAMSERVERINFO *pThis,int *piMediaTrackIndex);
SCODE GetAudioUBuffer(TSTREAMSERVERINFO *pThis,int *piMediaTrackIndex);
SCODE initClientSocket(int *piFd);
SCODE connectClientSocket(int fdOut, const char *szSckName);
int writeClientSocket(int fdOut, BYTE *abUBuffer, DWORD dwWriteSize);
int create_unix_socket(const char *path);

//DWORD VideUBufReader(DWORD dwInstance);
//DWORD AudiUBufReader(DWORD dwInstance);

#endif // _RTSPS_UBUFFER_H_
