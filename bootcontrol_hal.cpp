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

#include <errno.h>
#include <string.h>
#include <exception>

#include <utils/Log.h>
#include <cutils/properties.h>

#include "bootcontrol_hal.h"

namespace android {
namespace hardware {
namespace boot {
namespace V1_0 {
namespace renesas {

BootControl::BootControl()
    : m_ops(nullptr), m_is_init(false) {
    ALOGD("Create BootControl");
    m_ops = avb_ops_user_new();
    if (m_ops == nullptr) {
        ALOGE("Unable to allocate AvbOps instance.");
    } else {
        m_is_init = true; //init done
    }
};

BootControl::~BootControl() {
    ALOGD("Destroy BootControl");
    if (m_ops) {
        avb_ops_user_free(m_ops);
    }
};

//Returns the number of available slots
Return<uint32_t> BootControl::getNumberSlots() {
    return NUM_SLOTS;
};

//Returns the slot number of that the current boot is booted from
Return<uint32_t> BootControl::getCurrentSlot() {
    char propbuf[PROPERTY_VALUE_MAX] = {0};

    property_get("ro.boot.slot_suffix", propbuf, "");
    if (strcmp(propbuf, "_a") == 0) {
        return 0;
    } else if (strcmp(propbuf, "_b") == 0) {
        return 1;
    } else {
        ALOGE("Unexpected slot suffix.");
    }
    return 0;
};

//Marks the current slot as having booted successfully
Return<void> BootControl::markBootSuccessful(markBootSuccessful_cb _hidl_cb) {
    int ret = -EIO;

    if (m_is_init) {
        uint32_t currSlot = getCurrentSlot();
        if (avb_ab_mark_slot_successful(m_ops->ab_ops, currSlot) ==
            AVB_IO_RESULT_OK) {
            ret = 0;
        }
    } else {
        ALOGE("AvbOps instance was not allocated.");
        ret = -ENOMEM;
    }

    struct CommandResult cr;
    cr.success = (ret == 0);
    cr.errMsg = strerror(-ret);
    _hidl_cb(cr);
    return Void();
};

//Marks the slot passed in parameter as the active boot slot
Return<void> BootControl::setActiveBootSlot(uint32_t slot,
        setActiveBootSlot_cb _hidl_cb) {
    int ret = -EIO;

    if (m_is_init) {
        if (slot < getNumberSlots()) {
            if (avb_ab_mark_slot_active(m_ops->ab_ops, slot) ==
                AVB_IO_RESULT_OK) {
                ret = 0;
            }
        }
    } else {
        ALOGE("AvbOps instance was not allocated.");
        ret = -ENOMEM;
    }

    struct CommandResult cr;
    cr.success = (ret == 0);
    cr.errMsg = strerror(-ret);
    _hidl_cb(cr);
    return Void();
};

//Marks the slot passed in parameter as an unbootable
Return<void> BootControl::setSlotAsUnbootable(uint32_t slot,
        setSlotAsUnbootable_cb _hidl_cb) {
    int ret = -EIO;

    if (m_is_init) {
        if (slot < getNumberSlots()) {
            if (avb_ab_mark_slot_unbootable(m_ops->ab_ops, slot) ==
                AVB_IO_RESULT_OK) {
                ret = 0;
            }
        }
    } else {
        ALOGE("AvbOps instance was not allocated.");
        ret = -ENOMEM;
    }

    struct CommandResult cr;
    cr.success = (ret == 0);
    cr.errMsg = strerror(-ret);
    _hidl_cb(cr);
    return Void();
};

//Returns if the slot passed in parameter is bootable
Return<BoolResult> BootControl::isSlotBootable(uint32_t slot) {
    if (!m_is_init) {
        ALOGE("AvbOps instance was not allocated.");
        return BoolResult::INVALID_SLOT;
    }
    if (slot >= getNumberSlots()) {
        //The slot does not exist
        return BoolResult::INVALID_SLOT;
    }

    AvbABData ab_data;
    bool is_bootable = false;

    if (avb_ab_data_read(m_ops->ab_ops, &ab_data) != AVB_IO_RESULT_OK) {
        return BoolResult::INVALID_SLOT;
    }

    is_bootable = (ab_data.slots[slot].priority > 0) &&
        (ab_data.slots[slot].successful_boot ||
        (ab_data.slots[slot].tries_remaining > 0));

    //Returns TRUE if the slot is bootable, FALSE if it's not
    return is_bootable ? BoolResult::TRUE : BoolResult::FALSE;
};

//Returns if the slot passed in parameter has been marked as successful
//using markBootSuccessful()
Return<BoolResult> BootControl::isSlotMarkedSuccessful(uint32_t slot) {
    if (!m_is_init) {
        ALOGE("AvbOps instance was not allocated.");
        return BoolResult::INVALID_SLOT;
    }
    if (slot >= getNumberSlots()) {
        //The slot does not exist
        return BoolResult::INVALID_SLOT;
    }

    AvbABData ab_data;
    bool is_marked_successful = false;

    if (avb_ab_data_read(m_ops->ab_ops, &ab_data) != AVB_IO_RESULT_OK) {
        return BoolResult::INVALID_SLOT;
    }

    is_marked_successful = ab_data.slots[slot].successful_boot;

    //Returns TRUE if slot has been marked as successful, FALSE if it has not
    return is_marked_successful ? BoolResult::TRUE : BoolResult::FALSE;
};

//Returns the string suffix used by partitions
Return<void> BootControl::getSuffix(uint32_t slot, getSuffix_cb _hidl_cb) {
    static const char* suffix[2] = {"_a", "_b"};
    //Returns the empty string "" if slot does not match an existing slot.
    hidl_string ans("");

    if (slot < getNumberSlots()) {
        ans = suffix[slot];
    }
    _hidl_cb(ans);
    return Void();
};

}  // namespace renesas
}  // namespace V1_0
}  // namespace boot
}  // namespace hardware
}  // namespace android
