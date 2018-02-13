#
# Copyright (C) 2018 GlobalLogic
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Include only for Renesas ones.
ifneq (,$(filter $(TARGET_PRODUCT), salvator ulcb kingfisher))
LOCAL_PATH:= $(call my-dir)

################################################################################
# Build BootControl HAL                                                        #
################################################################################
include $(CLEAR_VARS)

LOCAL_MODULE := android.hardware.boot@1.0-service.renesas
LOCAL_INIT_RC := android.hardware.boot@1.0-service.renesas.rc
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MODULE_TAGS := optional
LOCAL_PROPRIETARY_MODULE := true

LOCAL_CFLAGS = -D_FILE_OFFSET_BITS=64 \
	-D_POSIX_C_SOURCE=199309L \
	-Wa,--noexecstack \
	-Werror \
	-Wall \
	-Wextra \
	-Wformat=2 \
	-Wmissing-prototypes \
	-Wno-psabi \
	-Wno-unused-parameter \
	-ffunction-sections \
	-fstack-protector-strong \
	-g \
	-DAVB_ENABLE_DEBUG \
	-DAVB_COMPILATION

LOCAL_CPPFLAGS = -Wnon-virtual-dtor -fno-strict-aliasing
LOCAL_LDFLAGS = -Wl,--gc-sections -rdynamic

LOCAL_SRC_FILES := \
	service.cpp \
	bootcontrol_hal.cpp

LOCAL_STATIC_LIBRARIES := \
	libavb_user \
	libfs_mgr

LOCAL_SHARED_LIBRARIES := \
	libbase \
	liblog \
	libcutils \
	libhardware \
	libhidlbase \
	libhidltransport \
	libutils \
	android.hardware.boot@1.0

include $(BUILD_EXECUTABLE)
endif # Include only for Renesas ones.
