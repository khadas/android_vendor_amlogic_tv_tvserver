/*
 * Copyright (C) 2016 The Android Open Source Project
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
 *  @author   Tellen Yu
 *  @version  1.0
 *  @date     2018/1/15
 *  @par function description:
 *  - 1 droidlogic tvserver daemon, hwbiner implematation
 */

#define LOG_TAG "tvserver"
#define LOG_TV_TAG "HIDLServer"

#include <inttypes.h>
#include <string>
#include <binder/Parcel.h>
#include <cutils/properties.h>
#include <android/hidl/allocator/1.0/IAllocator.h>
#include <android/hidl/memory/1.0/IMemory.h>
#include <hidlmemory/mapping.h>

#include "CTvLog.h"
#include "DroidTvServer.h"

namespace vendor {
namespace amlogic {
namespace hardware {
namespace tvserver {
namespace V1_0 {
namespace implementation {

using ::android::hidl::allocator::V1_0::IAllocator;
using ::android::hidl::memory::V1_0::IMemory;

static int nClient = 0;
DroidTvServer::DroidTvServer() : mDeathRecipient(new DeathRecipient(this)) {
    mTvServiceIntf = new DroidTvServiceIntf();
    mTvServiceIntf->setListener(this);
}

DroidTvServer::~DroidTvServer() {
    delete mTvServiceIntf;
}

void DroidTvServer::onEvent(const TvHidlParcel &hidlParcel) {
    int clientSize = mClients.size();

    ALOGI("onEvent event:%d, client size:%d", hidlParcel.msgType, clientSize);

#if 0
    sp<IAllocator> ashmemAllocator = IAllocator::getService("ashmem");
    if (ashmemAllocator == nullptr) {
        ALOGE("can not get ashmem service");
        return;
    }

    size_t size = p.dataSize();
    android::hardware::hidl_memory hidlMemory;
    auto res = ashmemAllocator->allocate(size, [&](bool success, const android::hardware::hidl_memory& memory) {
                if (!success) {
                    ALOGE("ashmem allocate size:%d fail", size);
                }
                hidlMemory = memory;
            });

    if (!res.isOk()) {
        ALOGE("ashmem allocate result fail");
        return;
    }

    sp<IMemory> memory = android::hardware::mapMemory(hidlMemory);
    void* data = memory->getPointer();
    memory->update();
    // update memory however you wish after calling update and before calling commit
    memcpy(data, p.data(), size);
    memory->commit();
#endif
    for (int i = 0; i < clientSize; i++) {
        if (mClients[i] != nullptr) {
            ALOGI("%s, client cookie:%d notifyCallback", __FUNCTION__, i);
            mClients[i]->notifyCallback(hidlParcel);
        }
    }
}

Return<void> DroidTvServer::lock() {
    return Void();
}

Return<void> DroidTvServer::unlock() {
    return Void();
}

Return<void> DroidTvServer::disconnect() {
    return Void();
}

Return<int32_t> DroidTvServer::processCmd(int32_t type, int32_t size) {
    #if 0
    Parcel p;

    sp<IMemory> memory = android::hardware::mapMemory(parcelMem);
    void* data = memory->getPointer();
    //memory->update();
    // update memory however you wish after calling update and before calling commit
    p.write(data, size);
    int ret = mTvServiceIntf->processCmd(p);
    //memory->commit();
    return ret;
    #endif
    return 0;
}

Return<int32_t> DroidTvServer::startTv() {
    return mTvServiceIntf->startTv();
}

Return<int32_t> DroidTvServer::stopTv() {
    return mTvServiceIntf->stopTv();
}

Return<int32_t> DroidTvServer::switchInputSrc(int32_t inputSrc) {
    return mTvServiceIntf->switchInputSrc(inputSrc);
}

Return<int32_t> DroidTvServer::getInputSrcConnectStatus(int32_t inputSrc) {
    return mTvServiceIntf->getInputSrcConnectStatus(inputSrc);
}

Return<int32_t> DroidTvServer::getCurrentInputSrc() {
    return mTvServiceIntf->getCurrentInputSrc();
}

Return<int32_t> DroidTvServer::getHdmiAvHotplugStatus() {
    return mTvServiceIntf->getHdmiAvHotplugStatus();
}

Return<void> DroidTvServer::getSupportInputDevices(getSupportInputDevices_cb _hidl_cb) {
    std::string devices = mTvServiceIntf->getSupportInputDevices();
    _hidl_cb(0/*don't use*/, devices);
    return Void();
}

Return<void> DroidTvServer::getCurSignalInfo(getCurSignalInfo_cb _hidl_cb) {
    SignalInfo info;
    mTvServiceIntf->getCurSignalInfo(info.fmt, info.transFmt, info.status, info.frameRate);

    _hidl_cb(info);
    return Void();
}

Return<int32_t> DroidTvServer::setMiscCfg(const hidl_string& key, const hidl_string& val) {
    return mTvServiceIntf->setMiscCfg(key, val);
}

Return<void> DroidTvServer::getMiscCfg(const hidl_string& key, const hidl_string& def, getMiscCfg_cb _hidl_cb) {
    std::string cfg = mTvServiceIntf->getMiscCfg(key, def);

    ALOGI("%s, key:%s def:%s, cfg:%s", __FUNCTION__, key.c_str(), def.c_str(), cfg.c_str());
    _hidl_cb(cfg);
    return Void();
}

Return<int32_t> DroidTvServer::getHdmiPorts() {
    return mTvServiceIntf->getHdmiPorts();
}

Return<int32_t> DroidTvServer::isDviSIgnal() {
    return mTvServiceIntf->getHdmiPorts();
}

Return<int32_t> DroidTvServer::isVgaTimingInHdmi() {
    return mTvServiceIntf->getHdmiPorts();
}

Return<int32_t> DroidTvServer::setHdmiEdidVersion(int32_t port_id, int32_t ver) {
    return mTvServiceIntf->setHdmiEdidVersion(port_id, ver);
}

Return<int32_t> DroidTvServer::getHdmiEdidVersion(int32_t port_id) {
    return mTvServiceIntf->getHdmiEdidVersion(port_id);
}

Return<int32_t> DroidTvServer::saveHdmiEdidVersion(int32_t port_id, int32_t ver) {
    return mTvServiceIntf->saveHdmiEdidVersion(port_id, ver);
}

Return<int32_t> DroidTvServer::handleGPIO(const hidl_string& key, int32_t is_out, int32_t edge) {
    return mTvServiceIntf->handleGPIO(key, is_out, edge);
}

Return<void> DroidTvServer::setCallback(const sp<ITvServerCallback>& callback, ConnectType type) {
    if ((int)type > (int)ConnectType::TYPE_TOTAL - 1) {
        ALOGE("%s don't support type:%d", __FUNCTION__, (int)type);
        return Void();
    }

    if (callback != nullptr) {
        if (mClients[nClient] != nullptr) {
            ALOGW("%s this type:%s had a callback, don't cover it", __FUNCTION__, getConnectTypeStr(type));
            //mClients[(int)type]->unlinkToDeath(mDeathRecipient);
        }
        else {
            mClients[nClient] = callback;
            Return<bool> linkResult = callback->linkToDeath(mDeathRecipient, nClient);
            bool linkSuccess = linkResult.isOk() ? static_cast<bool>(linkResult) : false;
            if (!linkSuccess) {
                ALOGW("Couldn't link death recipient for type: %s, client: %d", getConnectTypeStr(type), nClient);
            }
            ALOGI("%s client type:%s, client size:%d, nClient:%d", __FUNCTION__, getConnectTypeStr(type), (int)mClients.size(), nClient);
            nClient++;
        }

    }

    if (!mListenerStarted) {
        mTvServiceIntf->startListener();
        mListenerStarted= true;
    }

    return Void();
}

const char* DroidTvServer::getConnectTypeStr(ConnectType type) {
    switch (type) {
        case ConnectType::TYPE_HAL:
            return "HAL";
        case ConnectType::TYPE_EXTEND:
            return "EXTEND";
        default:
            return "unknown type";
    }
}

void DroidTvServer::handleServiceDeath(uint32_t client) {
    ALOGI("tvserver daemon client:%d died", client);
    mClients[client].clear();
    nClient--;
}

DroidTvServer::DeathRecipient::DeathRecipient(sp<DroidTvServer> server)
        : droidTvServer(server) {}

void DroidTvServer::DeathRecipient::serviceDied(
        uint64_t cookie,
        const wp<::android::hidl::base::V1_0::IBase>& /*who*/) {
    ALOGE("droid tvserver daemon a client died cookie:%d", (int)cookie);

    uint32_t client = static_cast<uint32_t>(cookie);
    droidTvServer->handleServiceDeath(client);
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace tvserver
}  // namespace hardware
}  // namespace amlogic
}  // namespace vendor
