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
#  Module name          : module1
#  Module description   : module1 sample makefile
#  Author               : Joe Wu
#  Created at           : 2006/08/02
#  $History:$
#
#---------------------------------------------------------------------

# environment settings
include $(MAKEINC)/default.mk
include $(MAKEINC)/tools.mk
#include $(MAKEINC)/apps.mk

APPS	= app/rtsps

# Targets
all:
	@for M in $(APPS); do \
		if make -C $$M -s; then \
		true; \
		else exit; fi; done
	
clean:
	@for M in $(APPS); do \
		if make -C $$M clean -s; then \
		true; \
		else exit; fi; done

cleanall:
	@for M in $(APPS); do \
		if make -C $$M cleanall -s; then \
		true; \
		else exit; fi; done
	
install:
	@for M in $(APPS); do \
		if make -C $$M install -s; then \
		true; \
		else exit; fi; done
