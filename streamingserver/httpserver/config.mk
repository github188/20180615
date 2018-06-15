#---------------------------------------------------------------------
#  $Header:$
#  Copyright (c) 2000-2007 Vivotek Inc. All rights reserved.
#
#  +-----------------------------------------------------------------+
#  | THIS SOFTWARE IS FURNISHED UNDER A LICENSE AND MAY ONLY BE USED |
#  | AND COPIED IN ACCORDANCE WITH THE TERMS AND CONDITIONS OF SUCH  |
#  | A LICENSE AND WITH THE INCLUSION OF THE THIS COPY RIGHT NOTICE. |
#  | THIS SOFTWARE OR ANY OTHER COPIES OF THIS SOFTWARE MAY NOT BE   |
#  | PROVIDED OR OTHERWISE MADE AVAILABLE TO ANY OTHER PERSON. THE   |
#  | OWNERSHIP AND TITLE OF THIS SOFTWARE IS NOT TRANSFERRED.        |
#  |                                                                 |
#  | THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE WITHOUT   |
#  | ANY PRIOR NOTICE AND SHOULD NOT BE CONSTRUED AS A COMMITMENT BY |
#  | VIVOTEK INC.                                                    |
#  +-----------------------------------------------------------------+
#
#  Project name         : NONE
#  Module name          : config.mk
#  Module description   : configuration for makefile inclusion
#  Author               : Joe Wu
#  Created at           : 2006/08/02
#  $History:$
#
#---------------------------------------------------------------------

#---------------------------------------------------------------------
# Configurations
#---------------------------------------------------------------------

# general configuration
#---------------------------------------------------------------------
INCDIR	 	= ../inc 
TOOL_CONFIG = GenericLinux
LIBS		= $(common_lib) $(osisolate_lib) $(rtsprtpcommon_lib) $(rtpmediaqueue_lib)

# specific configuration
#---------------------------------------------------------------------
ifeq "$(BUILD_CONF)" "linux_release"
PLATFORM = _LINUX
SYSDEFS  = 
CFLAGS	 = -O3 -D_NDEBUG -D_DISABLE_TELNETSERVER -D_MULTICAST -D_MULTICAST_NUM=2
USRDEFS  = 
endif

#---------------------------------------------------------------------
ifeq "$(BUILD_CONF)" "linux_debug"
PLATFORM = _LINUX
SYSDEFS  = 
CFLAGS	 = -O3 -D_DEBUG -Wall -g -D_DISABLE_TELNETSERVER -D_MULTICAST -D_MULTICAST_NUM=2
USRDEFS  = 
endif

#---------------------------------------------------------------------
ifeq "$(BUILD_CONF)" "armlinux_release"
PLATFORM = _LINUX
TOOL_CONFIG	= ARMLinux
SYSDEFS  = 
CFLAGS	 = -O3 -D_NDEBUG -D_DISABLE_TELNETSERVER -D_MULTICAST -D_MULTICAST_NUM=2
USRDEFS  = 
endif

#---------------------------------------------------------------------
ifeq "$(BUILD_CONF)" "armlinux_debug"
PLATFORM = _LINUX
TOOL_CONFIG	= ARMLinux
SYSDEFS  = 
CFLAGS	 = -O0 -D_DEBUG -Wall -g -D_DISABLE_TELNETSERVER -D_MULTICAST -D_MULTICAST_NUM=2
USRDEFS  = 
endif

#---------------------------------------------------------------------
ifeq "$(BUILD_CONF)" "vcwin32_release"
PLATFORM	= _WIN32_
TOOL_CONFIG = VCWin32
CFLAGS	 	= -D_NDEBUG -D_WIN32_WINNT=0x0400
endif

#---------------------------------------------------------------------
# tool settings
#---------------------------------------------------------------------

include $(MAKEINC)/tools.mk

ifeq "$(TOOL_CONFIG)" "CONFIG_NAME"
# you can add your additional tools configuration here
endif
