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
#  Module name          : ostest
#  Module description   : os testing program
#  Author               : Joe Wu
#  Created at           : 2006/08/03
#  $History:$
#
#---------------------------------------------------------------------

# environment settings
include $(MAKEINC)/default.mk
include $(MAKEINC)/version.mk

include $(CONFIGFILE)
include $(MAKEINC)/apps.mk
include $(MAKEINC)/filepp.mk
# install variables
package_init = init.d-rtsps
package_init_dir = /etc/init.d
package_sbin_dir = /usr/sbin
host_package_sbin_dir=${ROOTFSDIR}$(package_sbin_dir)
host_package_init_dir=${ROOTFSDIR}${package_init_dir}
INSTALLED_APP := $(host_package_sbin_dir)/$(MODNAME).$(EXESUFFIX)
NEW_APP := $(host_package_sbin_dir)/$(MODNAME)

# Targets
all: $(LIBS) $(CUSTOMLIBS) app pp_replace

install:
	#echo $(APPTARGET)
	$(call version_check)
	install src/${package_init} ${host_package_init_dir}/rtsps
	install $(APPTARGET) $(host_package_sbin_dir)
	$(MV) $(INSTALLED_APP) $(NEW_APP)
	$(STRIP) $(NEW_APP)
	$(CP) -rf $(PP_DIR)/scr/rootfs/usr/bin/* ${ROOTFSDIR}/usr/bin/
#ifeq "$(BUILD_CONF)" "armlinux_debug"
#	cp tmp/*.xml $(BUILD_ROOT)/armlinux_debug/$(soc_platform)/app/rtsps/bin
#endif

#ifeq "$(BUILD_CONF)" "armlinux_release"
#	cp tmp/*.xml $(BUILD_ROOT)/armlinux_release/$(soc_platform)/app/rtsps/bin
#endif
	
# post configuration operations
#----------------------------------------------------------------------
ifeq "$(BUILD_CONF)" "linux_release"

osisolate:
	@$(ECHO) Check \<$@\> ...
	@$(MAKE) -C $(PROJ_ROOT)/$@ BUILD_ROOT=$(BUILD_ROOT) BUILD_CONF=$(BUILD_CONF) -s
endif

#----------------------------------------------------------------------
ifeq "$(BUILD_CONF)" "linux_debug"
$(CUSTOMLIBS): 
	@$(ECHO) Checking \<$@\> ...
	@$(MAKE) -C $(PROJ_ROOT)/$@ BUILD_ROOT=$(BUILD_ROOT) CONFIGFILE=$(CONFIGFILE) -s
	
endif

