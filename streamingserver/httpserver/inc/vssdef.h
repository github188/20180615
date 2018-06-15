/*
 *******************************************************************************
 * $Header: /RD_1/Project/PNX1300_PSOS/Farseer/common/src/vssdef.h 13    06/05/08 5:08p Weicheng $
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
 * $History: vssdef.h $
 * 
 * *****************  Version 13  *****************
 * User: Weicheng     Date: 06/05/08   Time: 5:08p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/common/src
 * modufy for telnet terminal
 * 
 * *****************  Version 12  *****************
 * User: Greg         Date: 06/04/06   Time: 9:56a
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/common/src
 * sync with SD
 * 
 * *****************  Version 11  *****************
 * User: Greg         Date: 05/11/25   Time: 11:16a
 * Updated in $/RD_1/Project/PNX1300_PSOS/Waternoose/common/src
 * add MAX_CONNECT_NUM ; EXTERNAL_SCRIPT_FILE
 * 
 * *****************  Version 10  *****************
 * User: Weicheng     Date: 05/09/16   Time: 12:05a
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/common/src
 * ADD VIDEO_SIZE_CCDVGA
 * 
 * *****************  Version 9  *****************
 * User: Kate         Date: 05/08/26   Time: 6:32p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/common/src
 * Modify SNMP versions definition
 * 
 * *****************  Version 8  *****************
 * User: Kate         Date: 05/08/25   Time: 4:10p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/common/src
 * Add SNMP related defintion
 * 
 * *****************  Version 7  *****************
 * User: Greg         Date: 05/08/19   Time: 7:23p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Waternoose/common/src
 * modify Video Size definition for VS2403
 * 
 * *****************  Version 6  *****************
 * User: Albus        Date: 05/08/19   Time: 9:25a
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/common/src
 * MAX_VGA_SIZE re-added
 * 
 * *****************  Version 5  *****************
 * User: Albus        Date: 05/08/18   Time: 10:35p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/common/src
 * add VGA size for CCD
 * 
 * *****************  Version 4  *****************
 * User: Kate         Date: 05/08/18   Time: 7:30p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/common/src
 * 1. Add definition for system alarm
 * 2. Modify the message body size of email for _STARTUP_SYSTEMLOG
 * 
 * *****************  Version 3  *****************
 * User: Kate         Date: 05/08/11   Time: 8:48p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/common/src
 * 1. the default admin for DYNA is "Admin"
 * 2. add definition about SNMP
 * 
 * *****************  Version 2  *****************
 * User: Greg         Date: 05/08/08   Time: 3:13p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Waternoose/common/src
 * VS2403 code sync by Greg
 * 
 * *****************  Version 1  *****************
 * User: Kate         Date: 05/07/25   Time: 4:13p
 * Created in $/RD_1/Project/PNX1300_PSOS/Farseer/COMMON/src
 * First checkin in new mars
 * 
 * *****************  Version 25  *****************
 * User: Yun          Date: 05/07/06   Time: 11:47a
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/common/src
 * Modify for _SPEED_DOME
 * 
 * *****************  Version 24  *****************
 * User: Yun          Date: 05/06/22   Time: 3:14p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/common/src
 * Modify PTZ_ENABLED definition
 * 
 * *****************  Version 23  *****************
 * User: Yun          Date: 05/06/22   Time: 1:22p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/common/src
 * Modify for MOXA VPort3310
 * 
 * *****************  Version 22  *****************
 * User: Hoz          Date: 05/06/21   Time: 5:33p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Waternoose/common/src
 * redifine camera command list
 * 
 * *****************  Version 21  *****************
 * User: Weicheng     Date: 05/05/26   Time: 2:26p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Waternoose/common/src
 * modify for _PCB_VS2403
 * 
 * *****************  Version 20  *****************
 * User: Hoz          Date: 05/05/25   Time: 5:47p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Waternoose/common/src
 * 
 * *****************  Version 19  *****************
 * User: Joe          Date: 05/05/24   Time: 11:29a
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/common/src
 * add EXT_PTZDRV2_DLL definitions
 * 
 * *****************  Version 18  *****************
 * User: Yun          Date: 05/02/01   Time: 8:11p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/common/src
 * Modify MAX_MAIL_BODY_SIZE from 864 to 1024
 * 
 * *****************  Version 17  *****************
 * User: Yun          Date: 05/01/28   Time: 2:30p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/common/src
 * 1. Add focus speed limitation
 * 2. Add user-defined home zoom position
 * 3. Add white balance related parameters
 * 
 * *****************  Version 16  *****************
 * User: Yun          Date: 04/12/06   Time: 4:48p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/common/src
 * Add ENABLE_SET_CAMERA_Z definition
 * 
 * *****************  Version 15  *****************
 * User: Yun          Date: 04/11/15   Time: 4:46p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/common/src
 * Modify MAX_CAMERA_AUTO_SPEED to 6 for Z
 * 
 * *****************  Version 14  *****************
 * User: Yun          Date: 04/11/10   Time: 2:17p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/common/src
 * 1. Add MAX_CAMERA_DWELLINGTIME
 * 2. Change the value of max account number, max preset number, max
 * patrol number and max preset name length
 * 
 * *****************  Version 13  *****************
 * User: Yun          Date: 04/11/05   Time: 2:39p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/common/src
 * Add _CUSTOM_SNAPSHOT_FILE_PREFIX
 * 
 * *****************  Version 12  *****************
 * User: Albus        Date: 04/11/02   Time: 7:45p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/common/src
 * PTZ module tested.
 * 
 * *****************  Version 11  *****************
 * User: Yun          Date: 04/11/01   Time: 7:52p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/common/src
 * Add MAX_CAMERA_IRIS_LEVEL and CAMERA_FOCUS_SLEEP_MSEC
 * 
 * *****************  Version 10  *****************
 * User: Yun          Date: 04/10/22   Time: 6:27p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/common/src
 * 1. Remove _BUILTIN_PTZ definition
 * 2. Add _BUILTIN_Z definition
 * 3. Add zoom related definitions
 * 
 * *****************  Version 9  *****************
 * User: Yun          Date: 04/10/22   Time: 1:53p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/common/src
 * Add _MULTIPLE_PREVEVENT_IMAGE definition
 * 
 * *****************  Version 8  *****************
 * User: Yun          Date: 04/10/18   Time: 10:02a
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/common/src
 * Add _BUILTIN_PTZ definition
 * 
 * *****************  Version 7  *****************
 * User: Yun          Date: 04/10/15   Time: 11:53a
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/common/src
 * Add  MAX_ACCESSLIST_NUM
 * 
 * *****************  Version 6  *****************
 * User: Yun          Date: 04/09/07   Time: 5:53p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/common/src
 * Change the video text length to 14
 * 
 * *****************  Version 5  *****************
 * User: Yun          Date: 04/06/26   Time: 1:14p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/common/src
 * Modify the account number to 22 for D-Link
 * (20 users + admin + demo)
 * 
 * *****************  Version 2  *****************
 * User: Yun          Date: 04/04/09   Time: 11:58a
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/common/src
 * Modify for Dlink, account number: 21, root name: admin
 * 
 * *****************  Version 15  *****************
 * User: Yun          Date: 04/03/19   Time: 11:04a
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/common/src
 * Add PTZ_ENABLED definition
 * 
 * *****************  Version 14  *****************
 * User: Yun          Date: 04/02/25   Time: 9:39a
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/common/src
 * Add camera command definition
 * 
 * *****************  Version 13  *****************
 * User: Yun          Date: 04/02/19   Time: 3:12p
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/common/src
 * 1. modify AUDIO_TRANSMODE_NONE
 * 2. add AUDIO_TRANSMODE_UNKNOWN
 * 
 * *****************  Version 12  *****************
 * User: Yun          Date: 04/01/08   Time: 11:49a
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/common/src
 * Define TXRate and KeyLength according to the wireless definition
 * 
 * *****************  Version 11  *****************
 * User: Yun          Date: 03/12/16   Time: 6:39p
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/common/src
 * Add camera speed level
 * 
 * *****************  Version 10  *****************
 * User: Yun          Date: 03/12/12   Time: 2:01p
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/common/src
 * Add definition of camera control
 * 
 * *****************  Version 9  *****************
 * User: Joe          Date: 03/12/04   Time: 10:57a
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/common/src
 * Add camera preset location definitions
 * 
 * *****************  Version 8  *****************
 * User: Joe          Date: 03/10/24   Time: 1:53p
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/common/src
 * Add default NTP server definition
 * 
 * *****************  Version 7  *****************
 * User: Joe          Date: 03/10/21   Time: 5:21p
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/common/src
 * Add wireless settings.
 * 
 * *****************  Version 6  *****************
 * User: Yun          Date: 03/10/07   Time: 1:30p
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/common/src
 * Add SINGLE_SNAPSHOT definition
 * 
 * *****************  Version 5  *****************
 * User: Joe          Date: 03/09/25   Time: 10:33a
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/common/src
 * Add server script definitions
 * 
 * *****************  Version 4  *****************
 * User: Joe          Date: 03/09/10   Time: 1:48p
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/common/src
 * Add definitions for mail & message
 * 
 * *****************  Version 3  *****************
 * User: Jason        Date: 03/08/28   Time: 11:08a
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/common/src
 * Add audio mode AUDIO_TRANSMODE_NONE
 * 
 * *****************  Version 2  *****************
 * User: Joe          Date: 03/08/28   Time: 9:55a
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/common/src
 * Add webpage selection definitions.
 * 
 * *****************  Version 1  *****************
 * User: Joe          Date: 03/07/29   Time: 1:40p
 * Created in $/rd_1/Project/TM1300_PSOS/FarSeer/common/src
 * Vivotek server script definitions
 *
 *******************************************************************************
 */

/*!
 *******************************************************************************
 * Copyright 2000-2003 Vivotek, Inc. All rights reserved.
 *
 * \file
 * vssdef.h
 *
 * \brief
 * error code & definitions for Vivotek server script compiler
 * The definitions should be consistent with farseerdef.h & netdef.h
 *
 * \date
 * 2003/05/30
 *
 * \author
 * Joe Wu
 *
 *
 *******************************************************************************
 */

#ifndef _VSS_DEF_H_
#define _VSS_DEF_H_

#include "netdef.h"

#define ERR_VSP_INVALID_EVENT   0x80000001
#define ERR_VSP_INVALID_VERSION 0x80000002

// definitions for server script
#ifndef S_OK
#define S_OK 0
#endif

#ifndef S_FAIL
#define S_FAIL (unsigned int)(-1)
#endif


//! invalid ID number (for counting from zero)
#define INVALID_NUM				0xFFFFFFFF

//! max length of caption text
#define MAX_CAPTION_LEN			14
//! max length of firmware version
#define MAX_VERSION_LEN			39
//! date format length "yyyy/mm/dd"
#define DATE_LEN				10
//! time format length "hh:mm:ss"
#define TIME_LEN				8
//! max account number including root

#if defined(_DLINK) || defined(_MOXA)
#define MAX_ACCOUNT_NUM			22
#else
#define MAX_ACCOUNT_NUM			21
#endif //_DLINK

//! max motion detection window numbers
#define MAX_MOTION_WIN_NUM      3
//! max length of preset location
#define MAX_PRESET_LOCATION_LEN	40
//! max preset location number
#define MAX_PRESET_LOCATION_NUM	20
//! max patrol location number
#define MAX_PATROL_LOCATION_NUM	40
//! max length for ptz driver name
#define	MAX_PTZDRVNAME_LEN		40
//! max external ptz drivers number
#define MAX_PTZDRV_NUM			20
//! max ptz driver file name len
#define MAX_PTZDRVFILENAME_LEN	40

#define MAX_CAMERA_DWELLINGTIME	255

#define MAX_CAMERA_PAN_SPEED	5
#define MIN_CAMERA_PAN_SPEED	-5
#define MAX_CAMERA_TILT_SPEED	5
#define MIN_CAMERA_TILT_SPEED	-5

#if defined(_BUILTIN_Z) || defined(_SPEED_DOME)
#define MAX_CAMERA_AUTO_SPEED	6
#define MAX_CAMERA_SPEED_LEVEL	6
#else
#define MAX_CAMERA_AUTO_SPEED	5
#define MAX_CAMERA_SPEED_LEVEL	5
#endif // _BUILITIN_Z
#define MIN_CAMERA_AUTO_SPEED	1

#define MAX_CAMERA_ZOOM_SPEED	5
#define MIN_CAMERA_ZOOM_SPEED	-5
#define MAX_CAMERA_FOCUS_SPEED	5
#define MIN_CAMERA_FOCUS_SPEED	(-5)
#define MIN_CAMERA_PT_X     (-104)
#define MAX_CAMERA_PT_X     (104)
#ifdef _BUILTIN_Z
#define MIN_CAMERA_PT_Y     (-51)
#define MAX_CAMERA_PT_Y     (95)
#else
#define MIN_CAMERA_PT_Y     (-15)
#define MAX_CAMERA_PT_Y     (28)
#endif
#define MIN_CAMERA_Z		(-5)
#define MAX_CAMERA_Z		(5)

#define USER_DEFINED_HOME_ZOOM_POSITION		39

#define ENABLE_SET_CAMERA_Z	(1<<16)

#define MAX_CAMERA_IRIS_LEVEL	8
#define CAMERA_FOCUS_SLEEP_MSEC	100
/*
#define CAMERA_CMD_LEFT 		1
#define CAMERA_CMD_RIGHT 		2
#define CAMERA_CMD_STOP			3
#define CAMERA_CMD_UP			4
#define CAMERA_CMD_AUTOPAN		5
#define CAMERA_CMD_AUTOPATROL	6
#define CAMERA_CMD_GOTO			7
#define CAMERA_CMD_DOWN			8
#define CAMERA_CMD_HOME			10
*/
#define CAMERA_CMD_LEFT 		1
#define CAMERA_CMD_RIGHT 		2
#define CAMERA_CMD_FOCUSNEAR	3
#define CAMERA_CMD_UP			4
#define CAMERA_CMD_HOME			5
#define CAMERA_CMD_FOCUSAUTO	6
#define CAMERA_CMD_FOCUSFAR 	7
#define CAMERA_CMD_DOWN			8
#define CAMERA_CMD_ZOOMIN		9
#define CAMERA_CMD_ZOOMOUT		10
#define CAMERA_CMD_STOP			11
#define CAMERA_CMD_AUTOPAN		12
#define CAMERA_CMD_AUTOPATROL	13
#define CAMERA_CMD_GOTO			14
#define CAMERA_CMD_RECALL		15
#define CAMERA_CMD_CRUISE		16
#define CAMERA_CMD_SEQUENCE		17
//! max parameter pair of HTTP request
#define MAX_HTTP_PARAM_PAIR_NUM	256

#ifdef _MULTIPLE_PREVEVENT_IMAGE

#if defined(_CUSTOM_SNAPSHOT_FILE_PREFIX) || (defined(_STARTUP_SYSTEMLOG) && defined(_WIRELESS))
//! max message size in mail content
#define	MAX_MAIL_BODY_SIZE		1024
#else
//! max message size in mail content
#define	MAX_MAIL_BODY_SIZE		384
#endif //_CUSTOM_SNAPSHOT_FILE_PREFIX

//! max attachments in mail
#define MAX_MAIL_ATTACHMENTS	8
#else

#ifdef _STARTUP_SYSTEMLOG
//! max message size in mail content
#define	MAX_MAIL_BODY_SIZE		1024
#else
//! max message size in mail content
#define	MAX_MAIL_BODY_SIZE		256
#endif // _STARTUP_SYSTEMLOG

//! max attachments in mail
#define MAX_MAIL_ATTACHMENTS	4
#endif // _MULTIPLE_PREVEVENT_IMAGE

//! max message size usnig tcp
#define MAX_MSG_SIZE			256

#ifdef _PCB_VS2403
//! max number of digital input
#define MAX_DI_NUM              4

//! max number of digital output
#define MAX_DO_NUM              4
#elif defined(_PCB_VPORT3310)
//! max number of digital input
#define MAX_DI_NUM              2

//! max number of digital output
#define MAX_DO_NUM              2
#elif defined(_SPEED_DOME)
#define MAX_DI_NUM              8
//! max number of digital output
#define MAX_DO_NUM              1
#else
//! max number of digital input
#define MAX_DI_NUM              1

//! max number of digital output
#define MAX_DO_NUM              1
#endif

//! file name for server page dll
#define SVR_PAGE_DLL			"svrpage.dll"

//! file name for server script dll
#define SVR_SCRIPT_DLL			"svrscript.dll"

// added by Joe 2004/06/07
//! file name for external ptz driver
#define EXT_PTZDRV_DLL			"extptzdrv.dll"
// added by Joe 2005/05/17
//! file name for second external ptz driver
#define EXT_PTZDRV2_DLL			"extptzdrv2.dll"

//! directory for place ptz driver
#define EXT_PTZDRV_PATH			"/flash/extptzdrv"

//! directories for place data that will be cleared when restore system
#define SETTING_PATH            "/flash/settings"

//! file name of configuration
#define CONFIG_INI              "config.ini"

//! file name of system log
#define SYSTEM_LOG				"system.log"

//! file name of external script file
#ifdef EXTERNAL_APP_SCRIPT
#define EXTERNAL_APP_SCRIPFILE  "script.txt"
#endif

//! default external file name
#define DEFAULT_SCRIPTFILE		"script.vssx"

//! file name prefix of snapshot
#define SNAP_PREFIX				"snap_"

//! file name of single snapshot
#define SINGLE_SNAPSHOT			"video.jpg"

//! file name of L3 flash program
#define L3_PROGRAM_NAME         "flash.bin"

// Added by Jeffrey 2006/04/11
#ifdef _TELNET_TERMINAL
#define TELNET_TERMINAL_UI_XML_FILE	"VPort3310_TelnetUI_v2.xml"
#endif // _TELNET_TERMINAL

#ifdef _DLINK
#define ROOT_NAME               "admin"
#elif defined(_DYNA)
#define ROOT_NAME				"Admin"
#else
//! root name
#define ROOT_NAME               "root"
#endif //_DLINK

//! default NTP server string
#define	USE_DEFAULT_NTP_SERVER	"skip to invoke default server"

//! definitions for configuration settings
#define NTP_UPDATE_ONE_HOUR		1
#define NTP_UPDATE_ONE_DAY		2
#define NTP_UPDATE_ONE_WEEK		3
#define NTP_UPDATE_ONE_MONTH	4


#define VIDEO_CODEC_MPEG4   0
#define VIDEO_CODEC_MJPEG   1
#define MAX_VIDEO_CODEC     1

#ifdef _WATERNOOSE
#define VIDEO_SIZE_HALF     1
#define VIDEO_SIZE_HALFx2   4
#define VIDEO_SIZE_NORMAL   2
#define VIDEO_SIZE_NORMALx2 5
#define VIDEO_SIZE_HALF_D1  6
#define VIDEO_SIZE_DOUBLE   3
#define MAX_VIDEO_SIZE      6
#else
#define VIDEO_SIZE_HALF     1
#define VIDEO_SIZE_HALFx2   2
#define VIDEO_SIZE_NORMAL   3
#define VIDEO_SIZE_NORMALx2 4
#define VIDEO_SIZE_DOUBLE   5
#ifdef _CCD_VGA
#define VIDEO_SIZE_CCDVGA   6
#define MAX_VIDEO_SIZE      6
#else
#define MAX_VIDEO_SIZE      5
#endif
#endif // _WATERNOOSE

#define MIN_VIDEO_SIZE      1

#define VIDEO_COLOR_MONO    0
#define VIDEO_COLOR_COLOR   1
#define MAX_VIDEO_COLOR     1

#define VIDEO_QUALITY_FIX_BITRATE   0
#define VIDEO_QUALITY_FIX_QUANT     1
#define MAX_VIDEO_QUALITY           1

#define	MIN_VIDEO_QUANT				1
#define	MAX_VIDEO_QUANT				5

#define MIN_VIDEO_BITRATE			32000
#define MAX_VIDEO_BITRATE			3000000

#define VIDEO_WB_AUTOWB             0
#define VIDEO_WB_FIXED_INDOOR       1
#define VIDEO_WB_FIXED_FLUORESCENT  2
#define VIDEO_WB_FIXED_OUTDOOR      3
#define MAX_VIDEO_WB                3

#define VIDEO_MODULATION_NTSC       0
#define VIDEO_MODULATION_PAL        1
#define VIDEO_MODULATION_AUTO       2
#define MAX_VIDEO_MODULATION        2

#define AUDIO_TRANSMODE_UNKNOWN		-1
#define AUDIO_TRANSMODE_FULLDUPLEX  0
#define AUDIO_TRANSMODE_HALFDUPLEX  1
#define AUDIO_TRANSMODE_TALK        2
#define AUDIO_TRANSMODE_LISTEN      3
#define AUDIO_TRANSMODE_NONE        4
#define MAX_AUDIO_TRANSMODE         4

#ifndef AUDIO_SOURCE_INTERNAL
#define AUDIO_SOURCE_INTERNAL		1
#endif
#ifndef AUDIO_SOURCE_EXTERNAL
#define AUDIO_SOURCE_EXTERNAL		0
#endif
#define MAX_AUDIO_SOURCE			1

#define AUDIO_OPMODE_TOGGLE         0
#define AUDIO_OPMODE_PRESS          1
#define MAX_AUDIO_OPMODE            1

#define WIRELESS_MODE_INFRASTRUCT	0
#define WIRELESS_MODE_ADHOC			1
#define MAX_WIRELESS_MODE			1

#define MIN_WIRELESS_CHANNEL		1
#define MAX_WIRELESS_CHANNEL		11

#define WIRELESS_TXRATE_1M			1
#define WIRELESS_TXRATE_2M			2
#define WIRELESS_TXRATE_5p5M		3
#define WIRELESS_TXRATE_11M			4
#define WIRELESS_TXRATE_22M			5

#if defined(_WIRELESS_802_11b_plus)
#define MAX_WIRELESS_TXRATE			5
#elif defined(_WIRELESS_802_11g)
#define MAX_WIRELESS_TXRATE			13
#endif

#define WIRELESS_PREAMBLE_LONG		0
#define WIRELESS_PREAMBLE_SHORT		1
#define MAX_WIRELESS_PREAMBLE		1

#define WIRELESS_AUTHMODE_OPEN		0
#define WIRELESS_AUTHMODE_SHARED	1
#define WIRELESS_AUTHMODE_AUTO		2
#define MAX_WIRELESS_AUTHMODE		2

#define WIRELESS_KEYLENGTH_64		1
#define WIRELESS_KEYLENGTH_128		2
#define WIRELESS_KEYLENGTH_256		3

#if defined(_WIRELESS_802_11b_plus)
#define MAX_WIRELESS_KEYLENGTH		3
#elif defined(_WIRELESS_802_11g)
#define MAX_WIRELESS_KEYLENGTH		2
#endif

#define WIRELESS_KEYFORMAT_HEX		0
#define WIRELESS_KEYFORMAT_ASCII	1
#define MAX_WIRELESS_KEYFORMAT		1

#define MAX_WIRELESS_KEY_SELECT		4

// added by Yun
#ifdef _SPEED_DOME
#define PTZ_ENABLED                 127 // add by greg 20050926

#else  //_SPEED_DOME
#if defined(_BUILTIN_PT) && defined(_BUILTIN_Z)
#define PTZ_ENABLED					63
#elif	defined(_BUILTIN_PT)
#define PTZ_ENABLED					15
#elif   defined(_EXTERNAL_CCD)
#ifdef _EXTERNAL_PTZ
#define PTZ_ENABLED					1
#else
#define PTZ_ENABLED					0
#endif // _EXTERNAL_PTZ
#else
#define PTZ_ENABLED					2
#endif
#endif // _SPEED_DOME

#define EXTPTZDRV_NONE				128
#define EXTPTZDRV_CUSTOM_CAMERA		129
#endif

#define MAX_ACCESSLIST_NUM			10

#define WHITEBALANCE_AUTOTRACKING	0
#define WHITEBALANCE_MANUALSET		1

#ifdef _UART
#define SHORTCUT_NUM				5
#define MAX_SPEEDLINK_COMMAND_LEN   61
#define MAX_SPEEDLINK_NAME_LEN      9
#define MAX_CUSTOM_COMMAND_LEN      61
#define MAX_CUSTOM_COMMAND_NUM      10
#define EXTPTZDRV_NONE				128
#define EXTPTZDRV_CUSTOM_CAMERA		129
#define EXTPTZDRV_GENERIC_COMMAND   127
#define SERIAL_NONE					128	
#define SERIAL 	   					0
#define SERIAL2    					1
#endif
#ifdef _ADD_PPPOE
#define MAX_PPPOE_NAME_LEN		63
#define MAX_PPPOE_PASS_LEN		63
#endif
#ifdef _IPACCESSCHECK
#define IPACCESSCHECK_MAX_IPLIST	20
#endif 

#ifdef _WATERNOOSE
#define VIDEO_QUAL_MEDIUM			1
#define VIDEO_QUAL_STANDARD			2
#define VIDEO_QUAL_GOOD				3
#define VIDEO_QUAL_DETAILED			4
#define VIDEO_QUAL_EXCELLENT		5
#define MAX_VIDEO_QUAL		        5
#define MIN_VIDEO_QUAL		        1
#define UI_IMAGE					1
#define UI_TEXT             	    2
#define GRAPHIC_HIDE				1
#define GRAPHIC_DEFAULT				2
#define GRAPHIC_URL					3
#endif //_WATERNOOSE

#define MAX_SNMP_COMMUN_LEN		14
#define MAX_SNMP_LEN			30

// The following definition must be sync with snmpagent.h
// -->
#define SNMP_VERSIONS_V1V2V3	1
#define SNMP_VERSIONS_V1V2		2
#define SNMP_VERSIONS_V3		3

#define SNMP_AUTH_NONE			0	//SNMPV3_AUTHTYPE_NoAuth
#define SNMP_AUTH_MD5			1	//SNMPV3_AUTHTYPE_MD5
#define SNMP_AUTH_SHA			2	//SNMPV3_AUTHTYPE_SHA
// <--

#define VSSEVENT_PARAMTYPE_NETWORK	1
#define VSSEVENT_PARAMTYPE_POWER	2

#define NETWORK_NONE				0
#define NETWORK_10M					1
#define NETWORK_100M				2

#define POWER_PWR1					0x01
#define POWER_PWR2					0x02

// network relative definitions
//! max allowed connections for streaming
#ifdef _WATERNOOSE
#define MAX_CONNECT_NUM         20
#else
#define MAX_CONNECT_NUM         10	// 6	//  
#endif
