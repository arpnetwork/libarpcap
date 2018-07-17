/*
 * Copyright 2018 ARP Network
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

#include "ScreenCapture.h"

#include <binder/ProcessState.h>

#include <gui/BufferQueue.h>
#include <gui/CpuConsumer.h>
#include <gui/ISurfaceComposer.h>
#include <gui/SurfaceComposerClient.h>

#include <ui/DisplayInfo.h>
#include <ui/Rect.h>

using android::BufferItem;
using android::BufferQueue;
using android::ConsumerBase;
using android::CpuConsumer;
using android::IBinder;
using android::IGraphicBufferProducer;
using android::IGraphicBufferConsumer;
using android::ISurfaceComposer;
using android::sp;
using android::status_t;
using android::String8;
using android::SurfaceComposerClient;

#define NAME                "ARP"
#define MAX_LOCKED_BUFFERS  3

#define SWAP(a, b) \
    do { \
        a ^= b; \
        b ^= a; \
        a ^= b; \
    } while(0)

namespace arp {

static status_t getMainDisplayInfo(android::DisplayInfo *info);

class FrameProxy : public ConsumerBase::FrameAvailableListener {
  public:
    FrameProxy(arp_callback cb) : mCallback(cb) {}

    virtual void onFrameAvailable(const BufferItem &item) {
        if (mCallback != nullptr) {
            mCallback(item.mFrameNumber, item.mTimestamp);
        }
    }

  private:
    arp_callback mCallback;
};

class ScreenCapture
{
  public:
    ScreenCapture();
    ~ScreenCapture();

    status_t createDisplay(
        uint32_t paddingTop, uint32_t paddingBottom,
        uint32_t width, uint32_t height, arp_callback cb);
    void destroyDisplay();
    status_t acquireFrameBuffer(ARPFrameBuffer *fb);
    void releaseFrameBuffer();

  private:
    CpuConsumer::LockedBuffer mBuffer;

    sp<IBinder> mDisplay;
    sp<IGraphicBufferProducer> mBufferProducer;
    sp<IGraphicBufferConsumer> mBufferConsumer;
    sp<CpuConsumer> mCpuConsumer;
    sp<FrameProxy> mFrameProxy;
};

ScreenCapture::ScreenCapture() {
    mBuffer.data = nullptr;
}

ScreenCapture::~ScreenCapture() {
    destroyDisplay();
}

status_t ScreenCapture::createDisplay(
    uint32_t paddingTop, uint32_t paddingBottom,
    uint32_t width, uint32_t height, arp_callback cb)
{
    destroyDisplay();

    status_t result = android::NO_ERROR;

    mFrameProxy = new FrameProxy(cb);

    android::DisplayInfo info;
    result = getMainDisplayInfo(&info);
    if (result != android::NO_ERROR) {
        return result;
    }

    uint32_t srcWidth = info.w;
    uint32_t srcHeight = info.h;
    uint32_t dstWidth = width;
    uint32_t dstHeight = height;

    if (info.orientation == DISPLAY_ORIENTATION_90 ||
        info.orientation == DISPLAY_ORIENTATION_270)
    {
        SWAP(srcWidth, srcHeight);
        SWAP(dstWidth, dstHeight);
    }

    android::Rect layerStackRect(0, paddingTop, srcWidth, srcHeight - paddingBottom);
    android::Rect visibleRect(dstWidth, dstHeight);

    // Always make sure we could initialize
    {
        sp<SurfaceComposerClient> sc = new SurfaceComposerClient();

        if ((result = sc->initCheck()) != android::NO_ERROR) {
            return result;
        }
    }

    String8 name(NAME);
    mDisplay = SurfaceComposerClient::createDisplay(name, true);
    BufferQueue::createBufferQueue(&mBufferProducer, &mBufferConsumer, false);
    mBufferConsumer->setDefaultBufferSize(dstWidth, dstHeight);
    mBufferConsumer->setDefaultBufferFormat(PIXEL_FORMAT_RGBA_8888);

    // CpuConsumer
    mCpuConsumer = new CpuConsumer(mBufferConsumer, MAX_LOCKED_BUFFERS, false);
    mCpuConsumer->setName(name);
    mCpuConsumer->setFrameAvailableListener(mFrameProxy);

    SurfaceComposerClient::openGlobalTransaction();
    SurfaceComposerClient::setDisplaySurface(mDisplay, mBufferProducer);
    SurfaceComposerClient::setDisplayProjection(
            mDisplay, DISPLAY_ORIENTATION_0, layerStackRect, visibleRect);
    SurfaceComposerClient::setDisplayLayerStack(mDisplay, 0); // default stack
    SurfaceComposerClient::closeGlobalTransaction();

    return android::NO_ERROR;
}

void ScreenCapture::destroyDisplay() {
    if (mDisplay != nullptr) {
        SurfaceComposerClient::destroyDisplay(mDisplay);

        // Releases frame buffer if locked.
        releaseFrameBuffer();

        mBufferProducer = nullptr;
        mBufferConsumer = nullptr;
        mCpuConsumer = nullptr;
        mDisplay = nullptr;
        mFrameProxy = nullptr;
    }
}

int ScreenCapture::acquireFrameBuffer(ARPFrameBuffer *fb) {
    status_t result = mCpuConsumer->lockNextBuffer(&mBuffer);
    if (result != android::NO_ERROR) {
        return result;
    }

    fb->data = mBuffer.data;
    fb->format = mBuffer.format;
    fb->width = mBuffer.width;
    fb->height = mBuffer.height;
    fb->stride = mBuffer.stride;
    fb->timestamp = mBuffer.timestamp;
    fb->frame_number = mBuffer.frameNumber;

    return android::NO_ERROR;
}

void ScreenCapture::releaseFrameBuffer() {
    if (mBuffer.data != nullptr) {
        mCpuConsumer->unlockBuffer(mBuffer);
        mBuffer.data = nullptr;
    }
}

status_t getMainDisplayInfo(android::DisplayInfo *info) {
    sp<IBinder> display = SurfaceComposerClient::getBuiltInDisplay(ISurfaceComposer::eDisplayIdMain);
    return SurfaceComposerClient::getDisplayInfo(display, info);
}

} // arp

static arp::ScreenCapture *sCap = nullptr;

void arpcap_init() {
    android::ProcessState::self()->startThreadPool();
}

void arpcap_fini() {
    arpcap_destroy();
}

int arpcap_get_display_info(ARPDisplayInfo *info) {
    if (info == nullptr) {
        return android::BAD_VALUE;
    }

    android::DisplayInfo displayInfo;
    status_t result = arp::getMainDisplayInfo(&displayInfo);
    if (result != android::NO_ERROR) {
        return result;
    }

    info->width = displayInfo.w;
    info->height = displayInfo.h;
    info->orientation = displayInfo.orientation;

    return android::NO_ERROR;
}

int arpcap_create(
    uint32_t paddingTop, uint32_t paddingBottom,
    uint32_t width, uint32_t height, arp_callback cb)
{
    if (sCap != nullptr) {
        return android::ALREADY_EXISTS;
    }

    if ((width == 0 || height == 0 || cb == nullptr) &&
        (paddingTop + paddingBottom < height))
    {
        return android::BAD_VALUE;
    }

    sCap = new arp::ScreenCapture();
    status_t res = sCap->createDisplay(paddingTop, paddingBottom, width, height, cb);
    if (res != android::NO_ERROR) {
        delete sCap;
        sCap = nullptr;
    }

    return res;
}

void arpcap_destroy() {
    if (sCap != nullptr) {
        sCap->destroyDisplay();
        delete sCap;
        sCap = nullptr;
    }
}

int arpcap_acquire_frame_buffer(ARPFrameBuffer *fb) {
    if (fb == nullptr) {
        return android::BAD_VALUE;
    }

    return sCap->acquireFrameBuffer(fb);
}

void arpcap_release_frame_buffer() {
    sCap->releaseFrameBuffer();
}
