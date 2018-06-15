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
 *  Module name         :   MediaBufferQueue.h
 *  Module description  :   Maintain the media buffer by FIFO character
 *  Author              :   Simon
 *  Created at          :   2002/04/22
 *  Revision            :   1.0
 ******************************************************************************
 *                        Revision history
 ******************************************************************************
 */

#ifndef _MEDIABUFFER_QUEUE_H_
#define _MEDIABUFFER_QUEUE_H_

#include "osisolate.h"
#include "typedef.h"

/* Global data */

#define MEDIABUFQUEUE_ERR_INVALID_HANDLE		-1
#define MEDIABUFQUEUE_ERR_DELETEQUEUE_FAILURE	-2
#define MEDIABUFQUEUE_ERR_MEDIABUF_NOTEMPTY		-3
#define MEDIABUFQUEUE_ERR_GETQUEUE_FAILURE		-4
#define MEDIABUFQUEUE_ERR_GETQUEUE_TIMEOUT		-5
#define MEDIABUFQUEUE_ERR_ADDQUEUE_FAILURE		-6
#define MEDIABUFQUEUE_ERR_QUEUEFULL				-7

/*!
 *******************************************************************************
 * \brief
 * Create the instance for Media Queue
 *
 * \param nMaximumBufferNum
 * \a (i) number of media buffers that should be created
 *
 * \retval Handle of media buffer queue
 *
 * \note
 * This function will create and initialize the media queue
 *
 **************************************************************************** */ 
HANDLE MediaBufQueue_Create(int nMaximumBufferNum);
/*!
***************************************************************************************************
* \brief
* Release the instance of Media Queue 
*
* \param hRTSPServerHandle 
* \a (i) the handle of Media buffer  
*
* \retval 0 
* Release the instance of Media Queue  success
*
* \retval 1
* Release the instance of Media Queue  failed
*
***************************************************************************************************/
int MediaBufQueue_Delete(HANDLE hMediaBufferQueue);
/*!
***************************************************************************************************
* \brief
* Get the number of media buffers in  Media Queue 
*
* \param hRTSPServerHandle 
* \a (i) the handle of Media buffer  
*
* \retval number of buffers in the media queue
*
***************************************************************************************************/
int MediaBufQueue_GetMediaBufCount(HANDLE hMediaBufferQueue);
/*!
***************************************************************************************************
* \brief
* Add media buffer to queue
*
* \param hRTSPServerHandle 
* \a (i) the handle of Media buffer 
*
* \param pvMediaBuffer 
* \a (i) pointer to media buffer
*
* \retval 0 
* Add media buffer to queue  success
*
* \retval 1
* Add media buffer to queue  failed
*
***************************************************************************************************/
int MediaBufQueue_AddMediaBuffer(HANDLE hMediaBufferQueue, void *pvMediaBuffer);
/*!
***************************************************************************************************
* \brief
* Get media buffer from Media Queue 
*
* \param hRTSPServerHandle 
* \a (i) the handle of Media buffer 
*
* \param nTimeOutInMilliSec 
* \a (i) number of milli-sec to wait for media buffer 
*
* \param ppvMediaBuffer 
* \a (i) pointer to receive the pointer of the media buffer 
*
* \retval 0 
* Get media buffer from queue success
*
* \retval 1
* Get media buffer from queue failed
*
***************************************************************************************************/
int MediaBufQueue_GetMediaBuffer(HANDLE hMediaBufferQueue, int nTimeOutInMilliSec, void ** ppvMediaBuffer);


//void BufferTest();	

#endif  //_MEDIABUFFER_QUEUE_H_




