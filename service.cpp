/*
 * Copyright (C) 2018 GlobalLogic
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "BootControlHAL"

#include <new>
#include <utils/Log.h>
#include <hidl/HidlTransportSupport.h>
#include <hidl/LegacySupport.h>
#include <android-base/logging.h>

#include "bootcontrol_hal.h"

using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;
using android::hardware::boot::V1_0::IBootControl;
using android::hardware::boot::V1_0::renesas::BootControl;

const uint32_t MAX_THREADS = 1;

int main(int /* argc */, char * /* argv */ []) {

    ALOGI("Loading BootControl HAL...");
    android::sp<IBootControl> bootcontrol = new (std::nothrow) BootControl();
    if (bootcontrol == nullptr) {
        ALOGE("Could not allocate BootControl.");
        return 1;
    }

    configureRpcThreadpool(MAX_THREADS, true);

    CHECK_EQ(bootcontrol->registerAsService(), android::NO_ERROR)
        << "Failed to register BootControl HAL";

    joinRpcThreadpool();

    ALOGI("BootControl HAL is terminating...");
    return 0;
}
