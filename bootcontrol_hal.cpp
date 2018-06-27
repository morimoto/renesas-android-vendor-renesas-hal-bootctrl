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
#include <endian.h>
#include <fcntl.h>

#include <string>

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android-base/stringprintf.h>
#include <android-base/unique_fd.h>
#include <hardware/boot_control.h>
#include <hardware/hardware.h>

#include <utils/Log.h>
#include <cutils/properties.h>

#include "bootcontrol_hal.h"

namespace android {
namespace hardware {
namespace boot {
namespace V1_0 {
namespace renesas {

BootControl::BootControl()
    : m_current_slot_index(GetCurrentSlotIndex()) {
    ALOGD("Create BootControl");

    if (m_current_slot_index == AVB_AB_ERROR_SLOT_INDEX) {
        ALOGE("Unable to initialize BootControl");
    }
};

BootControl::~BootControl() {
    ALOGD("Destroy BootControl");
};

uint32_t BootControl::GetCurrentSlotIndex(void) {
    /* Initialize the current_slot from the read-only property */
    std::string suffix_prop = android::base::GetProperty(
            AVB_AB_PROP_SLOT_SUFFIX, "");

    return SlotSuffixToIndex(suffix_prop.c_str());
};

/* Return the index of the slot suffix passed
 * or AVB_AB_ERROR_SLOT_INDEX if not a valid slot suffix
 */
uint32_t BootControl::SlotSuffixToIndex(const char* suffix) {
    for (uint32_t slot = 0; slot < AVB_AB_MAX_SLOTS; ++slot) {
        if (!strncmp(AVB_AB_SLOT_SUFFIXIES[slot], suffix, 2)) {
            return slot;
        }
    }
    return AVB_AB_ERROR_SLOT_INDEX;
};

bool BootControl::SlotIsBootable(AvbABSlotData* slot_data) {
    return (slot_data->priority > 0)
            && (slot_data->successful_boot
                    || (slot_data->tries_remaining > 0));
};

/* Ensure all unbootable and/or illegal states are marked as the
 * canonical 'unbootable' state, e.g. priority=0, tries_remaining=0,
 * and successful_boot=0
 */
void BootControl::SlotNormalize(AvbABSlotData* slot_data) {
    if (slot_data->priority > 0) {
        if (slot_data->tries_remaining == 0 && !slot_data->successful_boot) {
            /* We've exhausted all tries -> unbootable */
            SlotSetUnbootable(slot_data);
        }
        if (slot_data->tries_remaining > 0 && slot_data->successful_boot) {
            /* Illegal state - avb_ab_mark_slot_successful() will clear
             * tries_remaining when setting successful_boot
             */
            SlotSetUnbootable(slot_data);
        }
    } else {
        SlotSetUnbootable(slot_data);
    }
};

void BootControl::SlotSetUnbootable(AvbABSlotData* slot_data) {
    slot_data->priority = 0;
    slot_data->tries_remaining = 0;
    slot_data->successful_boot = 0;
};

bool BootControl::ValidateAvbABData(const AvbABData* ab_data) {
    /* Ensure magic is correct */
    if (strncmp(reinterpret_cast<const char*>(ab_data->magic), AVB_AB_MAGIC,
            AVB_AB_MAGIC_LEN)) {
        ALOGE("Magic number is incorrect");
        return false;
    }

    /* Ensure we don't attempt to access any fields if the major version
     * is not supported
     */
    if (ab_data->version_major > AVB_AB_MAJOR_VERSION) {
        ALOGE("No support for given major version");
        return false;
    }

    /* Bail if CRC32 doesn't match */
    if (ab_data->crc32 != CalculateAvbABDataCRC(ab_data)) {
        ALOGE("CRC32 does not match");
        return false;
    }

    return true;
};

uint32_t BootControl::CRC32(const uint8_t* buf, size_t size) {
    static uint32_t crc_table[256];

    /* Compute the CRC-32 table only once */
    if (!crc_table[1]) {
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t crc = i;
            for (uint32_t j = 0; j < 8; ++j) {
                uint32_t mask = -(crc & 1);
                crc = (crc >> 1) ^ (0xEDB88320 & mask);
            }
            crc_table[i] = crc;
        }
    }

    uint32_t ret = -1;
    for (size_t i = 0; i < size; ++i) {
        ret = (ret >> 8) ^ crc_table[(ret ^ buf[i]) & 0xFF];
    }

    return ~ret;
};

/* Return the little-endian representation of the CRC-32 */
uint32_t BootControl::CalculateAvbABDataCRC(const AvbABData* ab_data) {
    return htole32(CRC32(reinterpret_cast<const uint8_t*>(ab_data),
            offsetof(AvbABData, crc32)));
};

bool BootControl::LoadAvbABData(const char* misc_device, AvbABData* ab_data) {
    android::base::unique_fd fd(open(misc_device, O_RDONLY));
    if (fd.get() == -1) {
        ALOGE("Failed to open '/mics' partition");
        return false;
    }
    if (lseek(fd, AVB_AB_METADATA_MISC_PARTITION_OFFSET, SEEK_SET)
            != AVB_AB_METADATA_MISC_PARTITION_OFFSET) {
        ALOGE("Failed to seek '/mics' partition");
        return false;
    }
    if (!android::base::ReadFully(fd.get(), ab_data, sizeof(AvbABData))) {
        ALOGE("Failed to read '/mics' partition");
        return false;
    }

    if (!ValidateAvbABData(ab_data)) {
        return false;
    }
#if 0
    for (uint32_t slot = 0; slot < AVB_AB_MAX_SLOTS; ++slot) {
        SlotNormalize(&ab_data->slots[slot]);
    }
#endif
    return true;
};

bool BootControl::UpdateAndSaveAvbABData(const char* misc_device,
        AvbABData* ab_data) {
    ab_data->crc32 = CalculateAvbABDataCRC(ab_data);

    android::base::unique_fd fd(open(misc_device, O_WRONLY | O_SYNC));
    if (fd.get() == -1) {
        ALOGE("Failed to open '/mics' partition");
        return false;
    }
    if (lseek(fd.get(), AVB_AB_METADATA_MISC_PARTITION_OFFSET, SEEK_SET)
            != AVB_AB_METADATA_MISC_PARTITION_OFFSET) {
        ALOGE("Failed to seek '/mics' partition");
        return false;
    }
    if (!android::base::WriteFully(fd.get(), ab_data, sizeof(AvbABData))) {
        ALOGE("Failed to write '/mics' partition");
        return false;
    }

    return true;
};

/* Returns the number of available slots */
Return<uint32_t> BootControl::getNumberSlots() {
    return AVB_AB_MAX_SLOTS;
};

/* Returns the slot number of that the current boot is booted from */
Return<uint32_t> BootControl::getCurrentSlot() {
    return m_current_slot_index;
};

/* Marks the current slot as having booted successfully */
Return<void> BootControl::markBootSuccessful(markBootSuccessful_cb _hidl_cb) {
    int ret = -EIO;

    if (m_current_slot_index != AVB_AB_ERROR_SLOT_INDEX) {

        AvbABData ab_data;
        if (LoadAvbABData(AVB_AB_PROP_MISC_DEVICE, &ab_data)) {

            if (SlotIsBootable(&ab_data.slots[m_current_slot_index])) {

                ab_data.slots[m_current_slot_index].successful_boot = 1;
                ab_data.slots[m_current_slot_index].tries_remaining = 0;

                if (UpdateAndSaveAvbABData(AVB_AB_PROP_MISC_DEVICE, &ab_data)) {
                    ret = 0;
                }

            } else {
                ALOGE("Cannot mark unbootable slot as successful");
            }
        }
    }

    struct CommandResult cr;
    cr.success = (ret == 0);
    cr.errMsg = strerror(-ret);
    _hidl_cb(cr);
    return Void();
};

/* Marks the slot passed in parameter as the active boot slot */
Return<void> BootControl::setActiveBootSlot(uint32_t slot,
        setActiveBootSlot_cb _hidl_cb) {
    int ret = -EINVAL;

    if (slot < getNumberSlots()) {
        ret = -EIO;
        AvbABData ab_data;
        if (LoadAvbABData(AVB_AB_PROP_MISC_DEVICE, &ab_data)) {

            /* Make requested slot top priority, unsuccessful,
             * and with max tries
             */
            ab_data.slots[slot].priority = AVB_AB_MAX_PRIORITY;
            ab_data.slots[slot].tries_remaining =
                    AVB_AB_MAX_TRIES_REMAINING;
            ab_data.slots[slot].successful_boot = 0;

            /* Ensure other slot doesn't have as high a priority. */
            uint32_t other_slot = 1 - slot;
            if (ab_data.slots[other_slot].priority == AVB_AB_MAX_PRIORITY) {
                ab_data.slots[other_slot].priority = AVB_AB_MAX_PRIORITY - 1;
            }

            if (UpdateAndSaveAvbABData(AVB_AB_PROP_MISC_DEVICE, &ab_data)) {
                ret = 0;
            }
        }
    }

    struct CommandResult cr;
    cr.success = (ret == 0);
    cr.errMsg = strerror(-ret);
    _hidl_cb(cr);
    return Void();
};

/* Marks the slot passed in parameter as an unbootable */
Return<void> BootControl::setSlotAsUnbootable(uint32_t slot,
        setSlotAsUnbootable_cb _hidl_cb) {
    int ret = -EINVAL;

    if (slot < getNumberSlots()) {
        ret = -EIO;
        AvbABData ab_data;
        if (LoadAvbABData(AVB_AB_PROP_MISC_DEVICE, &ab_data)) {

            SlotSetUnbootable(&ab_data.slots[slot]);

            if (UpdateAndSaveAvbABData(AVB_AB_PROP_MISC_DEVICE, &ab_data)) {
                ret = 0;
            }
        }
    }

    struct CommandResult cr;
    cr.success = (ret == 0);
    cr.errMsg = strerror(-ret);
    _hidl_cb(cr);
    return Void();
};

/* Returns if the slot passed in parameter is bootable */
Return<BoolResult> BootControl::isSlotBootable(uint32_t slot) {

    if (slot >= getNumberSlots()) {
        return BoolResult::INVALID_SLOT;
    }

    AvbABData ab_data;

    if (!LoadAvbABData(AVB_AB_PROP_MISC_DEVICE, &ab_data)) {
        return BoolResult::INVALID_SLOT;
    }

    /* Returns TRUE if the slot is bootable, FALSE if it's not */
    return SlotIsBootable(&ab_data.slots[slot]) ?
        BoolResult::TRUE : BoolResult::FALSE;
};

/* Returns if the slot passed in parameter has been marked as successful
 * using markBootSuccessful()
 */
Return<BoolResult> BootControl::isSlotMarkedSuccessful(uint32_t slot) {

    if (slot >= getNumberSlots()) {
        return BoolResult::INVALID_SLOT;
    }

    AvbABData ab_data;
    bool is_marked_successful = false;

    if (!LoadAvbABData(AVB_AB_PROP_MISC_DEVICE, &ab_data)) {
        return BoolResult::INVALID_SLOT;
    }

    is_marked_successful = ab_data.slots[slot].successful_boot;

    /* Returns TRUE if slot has been marked as successful, FALSE if it has not */
    return is_marked_successful ? BoolResult::TRUE : BoolResult::FALSE;
};

/* Returns the string suffix used by partitions */
Return<void> BootControl::getSuffix(uint32_t slot, getSuffix_cb _hidl_cb) {

    /* Returns the empty string "" if slot does not match an existing slot */
    hidl_string ans("");

    if (slot < getNumberSlots()) {
        ans = AVB_AB_SLOT_SUFFIXIES[slot];
    }

    _hidl_cb(ans);
    return Void();
};

}  // namespace renesas
}  // namespace V1_0
}  // namespace boot
}  // namespace hardware
}  // namespace android
