/*
 *******************************************************************************
 * $Header: /RD_1/Project/PNX1300_PSOS/Farseer/COMMON/src/netdef.h 1     05/07/25 4:13p Kate $
 *
 *  Copyright (c) 2000-2003 Vivotek Inc. All rights reserved.
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
 * $History: netdef.h $
 * 
 * *****************  Version 1  *****************
 * User: Kate         Date: 05/07/25   Time: 4:13p
 * Created in $/RD_1/Project/PNX1300_PSOS/Farseer/COMMON/src
 * First checkin in new mars
 * 
 * *****************  Version 6  *****************
 * User: Joe          Date: 04/09/15   Time: 2:41p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/common/src
 * Add MAX_ACCESS_NAME_LEN
 * 
 * *****************  Version 5  *****************
 * User: Yun          Date: 04/09/07   Time: 5:53p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/common/src
 * Change MAX_PASS_LEN to 14
 * 
 * *****************  Version 4  *****************
 * User: Yun          Date: 04/07/06   Time: 9:49p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/common/src
 * Add MAX_DDNS_HOST_LEN and MAX_DDNS_PASS_LEN
 * 
 * *****************  Version 2  *****************
 * User: Jason        Date: 03/08/05   Time: 11:23a
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/common/src
 * Add NETWORK_PACKET_SIZE definition
 * 
 * *****************  Version 1  *****************
 * User: Joe          Date: 03/05/30   Time: 10:46a
 * Created in $/rd_1/Project/TM1300_PSOS/FarSeer/common/src
 * Constant definitions for some network parameters
 *
 *******************************************************************************
 */

/*!
 *******************************************************************************
 * Copyright 2000-2003 Vivotek, Inc. All rights reserved.
 *
 * \file
 * netdef.h
 *
 * \brief
 * some common definitions for network
 *
 * \date
 * 2003/03/21
 *
 * \author
 * Joe Wu
 *
 *
 *******************************************************************************
 */

#ifndef _NET_DEF_H_
#define _NET_DEF_H_

// http, ftp, file system relative definitions

//! max length of ip address
#define MAX_IPADDR_LEN          15

//! max length of domain name
#define MAX_DOMAIN_NAME_LEN		63

//! max length of file & url path
#define MAX_PATH_LEN            255

//! max length of device name
#define MAX_DEVPATH_LEN			15

//! max length of user name
#define MAX_NAME_LEN            16

//! max length of user password
#define MAX_PASS_LEN            14

#define MAX_NETAP_PASS_LEN		15

//! max length of host name
#define MAX_HOST_LEN			40

//! length of mac address
#define MAC_ADDR_LEN			12

//! max length of extra information in http
#define MAX_EXTRAINFO_LEN		8192

//! The ethernet packet size
#define NETWORK_PACKET_SIZE     1460

#define MAX_SMTP_NAME_LEN		63

#define MAX_SMTP_EMAIL_LEN		80

#define MAX_FTP_FOLDER_LEN		40

#define MAX_DDNS_HOST_LEN		127

#define MAX_DDNS_PASS_LEN		20

//! for RTSP access name
#define MAX_ACCESS_NAME_LEN		20

#endif

