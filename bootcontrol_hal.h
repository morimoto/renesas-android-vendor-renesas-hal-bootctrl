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

#include <hidl/Status.h>
#include <hidl/MQDescriptor.h>
#include <libavb_user/libavb_user.h>
#include <android/hardware/boot/1.0/IBootControl.h>

namespace android {
namespace hardware {
namespace boot {
namespace V1_0 {
namespace renesas {

using ::android::hardware::boot::V1_0::BoolResult;
using ::android::hardware::boot::V1_0::CommandResult;
using ::android::hardware::boot::V1_0::IBootControl;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_string;

class BootControl: public IBootControl {

public:
    BootControl();
    ~BootControl();

    // Methods from ::android::hardware::boot::V1_0::IBootControl follow.
    Return<uint32_t> getNumberSlots() override;
    Return<uint32_t> getCurrentSlot() override;
    Return<void> markBootSuccessful(markBootSuccessful_cb _hidl_cb) override;
    Return<void> setActiveBootSlot(uint32_t slot,
        setActiveBootSlot_cb _hidl_cb) override;
    Return<void> setSlotAsUnbootable(uint32_t slot,
        setSlotAsUnbootable_cb _hidl_cb) override;
    Return<BoolResult> isSlotBootable(uint32_t slot) override;
    Return<BoolResult> isSlotMarkedSuccessful(uint32_t slot) override;
    Return<void> getSuffix(uint32_t slot, getSuffix_cb _hidl_cb) override;

private:
    const uint32_t NUM_SLOTS = 2; ///< number of available slots (A/B)
    AvbOps* m_ops; ///< high-level operations structure
    bool m_is_init; ///< initialization flag
};

} // namespace renesas
} // namespace V1_0
} // namespace boot
} // namespace hardware
} // namespace android

#endif /* BOOTCONTROL_HAL_H_ */
