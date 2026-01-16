#-----------------------------------------------------------------------------
# Copyright (c) 2025 Lattice Semiconductor Corporation
#
# SPDX-License-Identifier: UNLICENSED
#
#-----------------------------------------------------------------------------

#-----------------------------------------------------------------------------
# include Makefile.vars
#-----------------------------------------------------------------------------
VARS_FILE :=  $(HUB_DIR)/Makefile.vars
include $(VARS_FILE)

#-----------------------------------------------------------------------------
# Phony targets
#-----------------------------------------------------------------------------
.PHONY: all setup build build_lib build_app build_py build_drivers package \
		clean dist_clean clean_lib clean_app clean_py clean_drivers \
		run_minimal_app run_memcheck run_minimal_py run_streaming_app \
		run_streaming_py

#-----------------------------------------------------------------------------
# targets
#-----------------------------------------------------------------------------
all: build

# TBD-DPN: This works for ARM Linux; once support is added for other OSes
# this will be revisited
setup:
	@if [ -e ${HUB_EXT_INSTALLS_DIR} ]; then \
echo "--- HUB externals already set up! ---"; \
else \
echo "--- Setting up HUB externals ---"; \
$(MAKE_EXECUTABLE) $(SCRIPTS_DIR)/setup_hub.sh; \
$(SCRIPTS_DIR)/setup_hub.sh ${HUB_EXTERNAL_DIR} ${HUB_EXT_INSTALLS_DIR}; \
fi

build: build_lib build_app build_py build_drivers

build_lib: setup $(VARS_FILE)
	$(MAKE) -C $(HUB_LIB_DIR)

build_app: build_lib
	$(MAKE) -C $(HUB_APP_DIR)

build_py: build_lib
	$(MAKE) -C $(HUB_PY_DIR)

build_drivers: 
	$(MAKE) -C $(HUB_DRIVERS_DIR)

$(HUB_PKG_DIR):
	$(MKDIR) $(HUB_PKG_DIR)

package: build_lib build_app $(HUB_PKG_DIR)
	@$(MAKE) -C $(HUB_LIB_DIR) package
	@$(MAKE) -C $(HUB_APP_DIR) package
	@$(MAKE) -C $(HUB_PY_DIR) package
	@$(MAKE) -C $(HUB_DRIVERS_DIR) package
	@$(COPY) $(HUB_EXAMPLES_DIR) $(HUB_PKG_DIR)
	@$(MAKE_EXECUTABLE) ${SCRIPTS_DIR}/package_hub.sh
	$(SCRIPTS_DIR)/package_hub.sh $(HUB_DEB_PKG_NAME) 	\
			$(realpath $(HUB_INC_DIR)/hub_version.h) 	\
			$(ARCH) $(HUB_PKG_DIR)
	
clean_lib:
	$(MAKE) -C $(HUB_LIB_DIR) clean

clean_app:
	$(MAKE) -C $(HUB_APP_DIR) clean

clean_img_ops:
	$(RM) $(OUTPUT_DIR)/*.bmp

clean_py:
	$(MAKE) -C $(HUB_PY_DIR) clean

clean_package:
	$(RM) $(HUB_PKG_DIR)

clean_drivers:
	$(MAKE) -C $(HUB_DRIVERS_DIR) clean

# Remove HUB dependencies install
dist_clean:
	$(RM) $(HUB_EXT_INSTALLS_DIR)

run_minimal_app: build_app
	$(MAKE) -C $(HUB_APP_DIR) run_minimal_app

run_streaming_app: build_app
	$(MAKE) -C $(HUB_APP_DIR) run_streaming_app

run_img_ops_app: build_app
	$(MAKE) -C $(HUB_APP_DIR) run_img_ops_app

run_streaming_py: build_app
	$(MAKE) -C $(HUB_APP_DIR) run_streaming_py

run_minimal_py: build_app
	$(MAKE) -C $(HUB_APP_DIR) run_minimal_py

run_memcheck: build_app
	$(MAKE) -C $(HUB_APP_DIR) run_memcheck

enable_lsc219_cam:
	$(MAKE) -C $(HUB_DRIVERS_DIR) enable_lsc219_cam

clean: clean_lib clean_app clean_py clean_drivers clean_package clean_img_ops
