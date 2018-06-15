/*
 *  Copyright (c) 2004 Vivotek Inc. All rights reserved.
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
 *  Module name         :   RTPPacketizer.h
 *  Module description  :   Callback to ask for one audio/video frame and 
 *                          packetize into RTP media buffer format
 *                            
 *  Author              :   ShengFu
 *  Created at          :   2004/04/23
 *  Revision            :   1.0
 ******************************************************************************
 *                        Revision history
 ******************************************************************************
 */
#ifndef _RTPPACKETIZER_H_
#define _RTPPACKETIZER_H_
 
#include "osisolate.h"
#include "common.h"
#include "typedef.h"
#include "bitstreambufdef.h"
#include "rtpmediabuf.h"
#include "rtpmediaqueue.h"
#include "streamserver.h"
 
/*! Structure for Packetizer Initialization parameters*/
 typedef struct
 {
	/*! Data Queue Handle*/
    HANDLE	hRTPMediaDataQueue;
	/*! Empty Queue Handle*/
    HANDLE	hRTPMediaEmptyQueue;
	/*! Thread Priority for Packetizer*/
	DWORD	dwThreadPriority;
 }stRTPPACKETIZERPARAM;


#define RTPPACKETIZER_CALLBACK_REQUESTBUFFER  1
#define RTPPACKETIZER_CALLBACK_RETURNBUFFER   2
/*!
********************************************************************************************************
* \brief
* CallBack Function of packetizer to Control Module
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
* callback type RTPPACKETIZER_CALLBACK_REQUESTBUFFER : request a encoded bit stream buffer 
* callback param1 : timeout value (milliseconds) for request buffer 
* callback param2 : address of the pointer of the encoded bit stream buffer
* 
* callback type RTPPACKETIZER_CALLBACK_RETURNBUFFER : return a bit stream buffer 
* callback param1 : timeout value (milliseconds) for return buffer 
* callback param2 : the pointer of the bit stream buffer
*
********************************************************************************/
typedef DWORD (*RTPPACKETIZERCALLBACK)(HANDLE hParentHandle, UINT uMsgFlag, void * pvParam1, void * pvParam2);

/*!
 *******************************************************************************
 * \brief
 * Create the instance for Packetizer
 *
 * \param phRTPPacketizer
 * \a (i) pointer to receive handle of new created instance
 *
 * \param pRTPPacketizerParam
 * \a (i) pointer to stRTPPACKETIZERPARAM structure which contains initial information for packetizer
 *
 * \retval S_OK
 * Create the instance ok.
 * \retval other
 * Create the instance failed. 
 *
 * \note
 * This function will create and initialize the packetizer
 *
 **************************************************************************** */ 
SCODE RTPPacketizer_Create(HANDLE* phRTPPacketizer,stRTPPACKETIZERPARAM* pRTPPacketizerParam);
/*!
 *******************************************************************************
 * \brief
 * Release the instance of Packetizer
 *
 * \param phRTPPacketizer
 * \a (i) Handle to the packetizer instance
 *
 * \retval S_OK
 * Release the instance ok.
 * \retval other
 * Release the instance failed. 
 *
 * \note
 * This function will release the packetizer resource
 *
 **************************************************************************** */ 
SCODE RTPPacketizer_Release(HANDLE* phRTPPacketizer); 
/*!
 *******************************************************************************
 * \brief
 * Start the instance of Packetizer
 *
 * \param phRTPPacketizer
 * \a (i) Handle to the packetizer instance
 *
 * \retval S_OK
 * Start the instance ok.
 * \retval other
 * Start the instance failed. 
 *
 * \note
 * This function will Start the packetizer
 *
 **************************************************************************** */
SCODE RTPPacketizer_Start(HANDLE hRTPPacketizer);
/*!
 *******************************************************************************
 * \brief
 * Stop the instance of Packetizer
 *
 * \param phRTPPacketizer
 * \a (i) Handle to the packetizer instance
 *
 * \retval S_OK
 * Stop the instance ok.
 * \retval other
 * Stop the instance failed. 
 *
 * \note
 * This function will Stop the packetizer
 *
 **************************************************************************** */
SCODE RTPPacketizer_STOP(HANDLE hRTPPacketizer);
/*!
 *******************************************************************************
 * \brief
 * Set callback function for packetizer
 *
 * \param phRTPPacketizer
 * \a (i) Handle to the packetizer instance
 *
 * \param fCallback
 * \a (i) callback function name
 *
 * \param hParentHandle
 * \a (i) handle that is to be passed back by callback function
 *
 * \retval S_OK
 * Set the callback function ok.
 * \retval other
 * Set the callback function failed. 
 *
 * \note
 * This function will set the callback function. Be sure to call this before any other operation.
 *
 **************************************************************************** */
SCODE RTPPacketizer_SetCallBakck(HANDLE hRTPPacketizerHandle, RTPPACKETIZERCALLBACK fCallback, HANDLE hParentHandle);
 /*!
 *******************************************************************************
 * \brief
 * Set the location for Packetizer
 *
 * \param phRTPPacketizer
 * \a (i) Handle to the packetizer instance
 *
 * \param pcLocation
 * \a (i) pointer to the string that contains new location 
 *
 * \retval S_OK
 * Set location ok.
 * \retval other
 * Set location failed. 
 *
 **************************************************************************** */
SCODE RTPPacketizer_SetLocation(HANDLE hRTPPacketizer, char* pcLocation); 
 
#endif  //_RTPPACKETIZER_H
