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

#ifndef BOOTCONTROL_HAL_H_
#define BOOTCONTROL_HAL_H_

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include <mutex>

#include <hidl/Status.h>
#include <hidl/MQDescriptor.h>

/* Contain structures for Virtual A/B update */
#include <bootloader_message/bootloader_message.h>

#include <android/hardware/boot/1.1/IBootControl.h>

namespace android {
namespace hardware {
namespace boot {
namespace V1_1 {
namespace renesas {

using ::android::hardware::boot::V1_0::BoolResult;
using ::android::hardware::boot::V1_0::CommandResult;
using ::android::hardware::boot::V1_1::IBootControl;
using ::android::hardware::boot::V1_1::MergeStatus;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_string;

class BootControl: public IBootControl {

public:
    BootControl();
    ~BootControl();

    /* Methods from ::android::hardware::boot::V1_0::IBootControl follow */
    Return<uint32_t> getNumberSlots() override;
    Return<uint32_t> getCurrentSlot() override;
    Return<void> markBootSuccessful(markBootSuccessful_cb _hidl_cb) override;
    Return<void> setActiveBootSlot(uint32_t slot, setActiveBootSlot_cb _hidl_cb)
            override;
    Return<void> setSlotAsUnbootable(uint32_t slot,
            setSlotAsUnbootable_cb _hidl_cb) override;
    Return<BoolResult> isSlotBootable(uint32_t slot) override;
    Return<BoolResult> isSlotMarkedSuccessful(uint32_t slot) override;
    Return<void> getSuffix(uint32_t slot, getSuffix_cb _hidl_cb) override;

    /* Methods from ::android::hardware::boot::V1_1::IBootControl follow. */
    Return<bool> setSnapshotMergeStatus(MergeStatus status) override;
    Return<MergeStatus> getSnapshotMergeStatus() override;
private:

    static const uint32_t AVB_AB_MAGIC_LEN = 4;

    /* Number of available slots (A/B) present on the device */
    static const uint32_t AVB_AB_MAX_SLOTS = 2;

    /* Magic for the A/B struct when serialized */
    const char* AVB_AB_MAGIC = "\0AB0";

    /* Versioning for the on-disk A/B metadata - keep in sync with avbtool */
    const uint32_t AVB_AB_MAJOR_VERSION = 1;

    /* Maximum values for slot data */
    const uint32_t AVB_AB_MAX_PRIORITY = 15;
    const uint32_t AVB_AB_MAX_TRIES_REMAINING = 7;

    /* AvbABData struct is stored in 2048 bytes offset
     * into the 'misc' partition
     */
    const off_t AVB_AB_METADATA_MISC_PARTITION_OFFSET = 2048;

    const char* AVB_AB_SLOT_SUFFIXIES[AVB_AB_MAX_SLOTS] = { "_a", "_b" };

    const char* AVB_AB_PROP_SLOT_SUFFIX = "ro.boot.slot_suffix";

    /* The path to the misc device */
    const char* AVB_AB_PROP_MISC_DEVICE =
            "/dev/block/platform/soc/ee140000.sd/by-name/misc";

    const uint32_t AVB_AB_ERROR_SLOT_INDEX = 0xABCDFFFF;

    /* Struct used for recording per-slot A/B metadata.
     *
     * When serialized, data is stored in network byte-order.
     */
    typedef struct AvbABSlotData {
        /* Slot priority. Valid values range from herr0 to AVB_AB_MAX_PRIORITY,
         * both inclusive with 1 being the lowest and AVB_AB_MAX_PRIORITY
         * being the highest. The special value 0 is used to indicate the
         * slot is unbootable.
         */
        uint8_t priority;

        /* Number of times left attempting to boot this slot ranging from 0
         * to AVB_AB_MAX_TRIES_REMAINING.
         */
        uint8_t tries_remaining;

        /* Non-zero if this slot has booted successfully, 0 otherwise. */
        uint8_t successful_boot;

        /* Reserved for future use. */
        uint8_t reserved[1];
    }__attribute__((packed)) AvbABSlotData;

    /* Struct used for recording A/B metadata.
     *
     * When serialized, data is stored in network byte-order.
     */
    typedef struct AvbABData {
        /* Magic number used for identification - see AVB_AB_MAGIC. */
        uint8_t magic[AVB_AB_MAGIC_LEN];

        /* Version of on-disk struct - see AVB_AB_{MAJOR,MINOR}_VERSION. */
        uint8_t version_major;
        uint8_t version_minor;

        /* Padding to ensure |slots| field start eight bytes in. */
        uint8_t reserved1[2];

        /* Per-slot metadata. */
        AvbABSlotData slots[2];

        /* Reserved for future use. */
        uint8_t reserved2[12];

        /* CRC32 of all 28 bytes preceding this field in LE format. */
        uint32_t crc32;
    }__attribute__((packed)) AvbABData;

    static_assert(sizeof(struct AvbABData) == 32,
            "struct AvbABData size changed, must be equal 32 bytes");

    /* Max supported version of Virtual A/B header */
    const uint32_t MAX_VIRTUAL_AB_MESSAGE_VERSION = 2;

    uint32_t GetCurrentSlotIndex(void);
    uint32_t SlotSuffixToIndex(const char* suffix);
    bool SlotIsBootable(AvbABSlotData* slot_data);
    void SlotNormalize(AvbABSlotData* slot_data);
    void SlotSetUnbootable(AvbABSlotData* slot_data);
    uint32_t CalculateAvbABDataCRC(const AvbABData* ab_data);
    uint32_t CRC32(const uint8_t* buf, size_t size);
    bool ReadFromFile(const char* filepath, size_t offset,
                void *buffer, size_t size);
    bool WriteToFile(const char* filepath, size_t offset,
                void* buffer, size_t size);
    bool LoadAvbABData(AvbABData* ab_data);
    bool UpdateAndSaveAvbABData(AvbABData* ab_data);
    bool ValidateAvbABData(const AvbABData* ab_data);

    void InitVirtualABMessage(misc_virtual_ab_message *virtual_ab_data);
    bool ValidateVirtualABMessage(misc_virtual_ab_message *virtual_ab_data);
    bool LoadVirtualABMessage(misc_virtual_ab_message* virtual_ab_data);
    bool SaveVirtualABMessage(misc_virtual_ab_message* virtual_ab_data);

    /* The slot where we are running from */
    uint32_t m_current_slot_index;

    /* Needed to implement thread protection for Vitrual A/B status */
    std::mutex m_merge_status_lock;
};

extern "C" IBootControl* HIDL_FETCH_IBootControl(const char* name);

} // namespace renesas
} // namespace V1_0
} // namespace boot
} // namespace hardware
} // namespace android

#endif /* BOOTCONTROL_HAL_H_ */
