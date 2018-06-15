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
 * rtsps_callback.h
 *
 * \brief
 * Callback functions for rtsp streaming server. (header file for rtsps_callback.c)
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

#ifndef _RTSPS_CALLBACK_H_
#define _RTSPS_CALLBACK_H_

#include "rtsps_local.h"
#include "rtsps_ubuffer.h"

SCODE StreamSvrAudioInCallback(DWORD dwInstance, DWORD dwCallbackType, void *pvCallbackData);
SCODE StreamSvrVideoCallback(DWORD dwInstance, DWORD dwCallbackType, void* pvCallbackData);
//20120801 added by Jimmy for metadata
SCODE StreamSvrMetadataCallback(DWORD dwInstance, DWORD dwCallbackType, void* pvCallbackData);
SCODE StreamSvrCtrlChCallback(DWORD dwInstance, DWORD dwConnectionID, DWORD dwCallbackType, DWORD dwCallbackData);

#ifdef _SHARED_MEM
/* Callback function for share memory*/
SCODE StreamSvrShmemVideoCallback(DWORD dwInstance, DWORD dwCallbackType, void* pvCallbackData);
SCODE StreamSvrShmemAudioCallback(DWORD dwInstance, DWORD dwCallbackType, void *pvCallbackData);
//20120801 added by Jimmy for metadata
SCODE StreamSvrShmemMetadataCallback(DWORD dwInstance, DWORD dwCallbackType, void *pvCallbackData);
#endif

/* for share memory layout*/
SCODE H264ExtractNaluInfo(BYTE *pbyVideUBuffer, TRTSPSTREAMING_VIDENCODING_PARAM *pRTSPVideoParam);
void H265ExtractNaluInfo(BYTE *pbyVideUBuffer, TRTSPSTREAMING_VIDENCODING_PARAM *pRTSPVideoParam);
SCODE JPEGBitStreamParse(TBitstreamBuffer *pBitstreamBuf, BYTE *pJPEGFrame, DWORD dwFrameSize);
SCODE G711BitstreamPack(TBitstreamBuffer *pBitstreamBuf, TUBuffer *pUBuffer, int iIndex, int TUBufferSize);
SCODE G726BitstreamPack(TBitstreamBuffer *pBitstreamBuf, TUBuffer *pUBuffer, int iIndex, int TUBufferSize);
int   M4ABitstreamPack(TBitstreamBuffer *pBitstreamBuf, TUBuffer *pUBuffer, int iIndex, int TUBufferSize);
SCODE AMRBitstreamPack(TBitstreamBuffer *pBitstreamBuf, TUBuffer *pUBuffer, int iIndex, int iFramesPerUBuffer, int TUBufferSize);
SCODE StreamSvrWriteFile(char *pzFilePathName, char* pWriteBuff, int iWriteLength);

#endif // _RTSPS_CALLBACK_H_
