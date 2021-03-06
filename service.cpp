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
using android::status_t;
using android::OK;

int main(int /* argc */, char * /* argv */ []) {

    fprintf(stderr, "Loading BootControl HAL...\n");

    android::sp<IBootControl> bootcontrol = new (std::nothrow) BootControl();
    if (bootcontrol == nullptr) {
        fprintf(stderr, "Could not allocate BootControl.\n");
        return -ENOMEM;
    }

    configureRpcThreadpool(1, true);

    status_t status = bootcontrol->registerAsService();
    if (status != android::OK) {
        fprintf(stderr, "Failed to register BootControl HAL\n");
        CHECK_NE(status, OK);
    }

    joinRpcThreadpool();
}
