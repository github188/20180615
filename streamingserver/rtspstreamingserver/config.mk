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
CFLAGS	 	= -Wall -D_DISABLE_TELNETSERVER -D_MULTICAST -D_AAC_AUDIO_IN
USRDEFS  	= 
LIBS		= $(common_lib) $(expat_lib)  $(osisolate_lib) $(xmlsparser_lib) $(rtsprtpcommon_lib) $(rtpmediaqueue_lib) $(rtprtcp_lib) $(rtppacketizer_lib) $(mediachannel_lib) $(sipua_lib) $(sdpdecoder_lib) $(rtspserver_lib) $(ipfilter_lib) $(encrypt_vivostrcodec_lib) $(encrypt_base64_lib) $(encrypt_sha1_lib) $(encrypt_md5_lib) $(xmlprocessor_lib) $(onvifbasic_lib) $(account_lib) $(message_lib)

MODELNAME    = $(subst -, ,$(PRODUCTVER))
USRDEFS     += _$(word 1,$(MODELNAME))
USRDEFS     += _$(word 2,$(MODELNAME))

# Multiple channel configuration
#---------------------------------------------------------------------
ifneq "$(MULTIPLE_CHANNEL_TOTAL)" ""
CFLAGS      += -D_MULTIPLE_CHANNEL_TOTAL=$(MULTIPLE_CHANNEL_TOTAL)
else
CFLAGS      += -D_MULTIPLE_CHANNEL_TOTAL=1
endif

# Multiple stream configuration
#---------------------------------------------------------------------
ifneq "$(MULTIPLE_STREAM_TOTAL)" ""
CFLAGS		+= -D_MULTIPLE_STREAM_TOTAL=$(MULTIPLE_STREAM_TOTAL)
else
CFLAGS		+= -D_MULTIPLE_STREAM_TOTAL=2
endif

# MOD stream configuration
#---------------------------------------------------------------------
ifneq "$(MOD_STREAM_TOTAL)" ""
CFLAGS      += -D_MOD_STREAM_TOTAL=$(MOD_STREAM_TOTAL)
else
CFLAGS      += -D_MOD_STREAM_TOTAL=0
endif

# session manager configuration
#---------------------------------------------------------------------
ifeq "$(SESSMGR_ENABLE)" "yes"
LIBS        += network/sessionmgr
CFLAGS      += -D_SESSION_MGR
endif

# share memory architecture
#---------------------------------------------------------------------
ifeq "$(SHMEM_ENABLE)" "yes"
USRDEFS	 	+= _SHARED_MEM
endif
#---------------------------------------------------------------------

# ipv6 configuration
#---------------------------------------------------------------------
ifeq "$(IPV6_ENABLE)" "yes"
CFLAGS	 	+= -D_IPV6_ENABLE
endif

# sip-2-way audio configuration
#---------------------------------------------------------------------
ifeq "$(SIP_ENABLE)" "yes"
CFLAGS	 	+= -D_SIP_ENABLE
endif

# metadata streaming configuration
#---------------------------------------------------------------------
ifeq "$(METADATA_ENABLE)" "yes"
CFLAGS      += -D_METADATA_ENABLE
LIBS        += $(eventparser_lib)
endif

# metadata event configuration
#---------------------------------------------------------------------
ifeq "$(METADATA_EVENT_ENABLE)" "yes"
CFLAGS	 	+= -D_METADATA_EVENT_ENABLE
endif

# specific configuration
#---------------------------------------------------------------------
ifeq "$(BUILD_CONF)" "linux_release"
PLATFORM = _LINUX
SYSDEFS  = 
CFLAGS	 += -O3 -D_NDEBUG
endif

#---------------------------------------------------------------------
ifeq "$(BUILD_CONF)" "linux_debug"
PLATFORM = _LINUX
SYSDEFS  = 
CFLAGS	 += -O0 -D_DEBUG -g
endif

#---------------------------------------------------------------------
ifeq "$(BUILD_CONF)" "armlinux_release"
PLATFORM = _LINUX
TOOL_CONFIG	= ARMLinux
SYSDEFS  = 
CFLAGS	 += -O3 -D_NDEBUG -fno-strict-aliasing
endif

#---------------------------------------------------------------------
ifeq "$(BUILD_CONF)" "armlinux_debug"
PLATFORM = _LINUX
TOOL_CONFIG	= ARMLinux
SYSDEFS  = 
CFLAGS	 += -O0 -D_DEBUG -g
endif

#---------------------------------------------------------------------
ifeq "$(BUILD_CONF)" "vcwin32_release"
PLATFORM	= _WIN32_
TOOL_CONFIG = VCWin32
CFLAGS	 	+= -D_NDEBUG -D_WIN32_WINNT=0x0400
endif

#---------------------------------------------------------------------
# tool settings
#---------------------------------------------------------------------

include $(MAKEINC)/tools.mk

ifeq "$(TOOL_CONFIG)" "CONFIG_NAME"
# you can add your additional tools configuration here
endif
