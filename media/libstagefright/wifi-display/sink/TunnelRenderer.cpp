/*
 * Copyright 2012, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#define LOG_NDEBUG 0
// MStar Android Patch Begin
#define DUMP_FILE 0
// MStar Android Patch End
#define LOG_TAG "TunnelRenderer"
#include <utils/Log.h>

#include "TunnelRenderer.h"

#include "ATSParser.h"

#include <binder/IMemory.h>
#include <binder/IServiceManager.h>
#include <gui/SurfaceComposerClient.h>
#include <media/IMediaPlayerService.h>
#include <media/IStreamSource.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/AMessage.h>
#include <ui/DisplayInfo.h>

// MStar Android Patch Begin
#include <binder/MemoryDealer.h>
#include <media/mediaplayer.h>
#ifdef BUILD_MSTARTV
#include <mapi_hdcp2.h>
#endif
// MStar Android Patch End

namespace android {
// MStar Android Patch Begin
struct TunnelRenderer::PlayerClient : public BnMediaPlayerClient {
    PlayerClient(TunnelRenderer *owner) : mOwner(owner){}
// MStar Android Patch End

    virtual void notify(int msg, int ext1, int ext2, const Parcel *obj) {
        ALOGI("notify %d, %d, %d", msg, ext1, ext2);

        // MStar Android Patch Begin
        switch (msg) {
            case MEDIA_PREPARED:
            {
                sp<AMessage> queueBufferMsg =
                            new AMessage(TunnelRenderer::kPlayerPrepared, mOwner->id());
                queueBufferMsg->post();
                break;
            }
            case MEDIA_PLAYBACK_COMPLETE:
            {
                sp<AMessage> queueBufferMsg =
                            new AMessage(TunnelRenderer::kPlayerPlaybackComplete, mOwner->id());
                queueBufferMsg->post();
                break;
            }
            default:
                ALOGV(" notify : default do nothing");
                break;
        }
        // MStar Android Patch End
    }

protected:
    virtual ~PlayerClient() {}

private:
    // MStar Android Patch Begin
    TunnelRenderer *mOwner;
    // MStar Android Patch End
    DISALLOW_EVIL_CONSTRUCTORS(PlayerClient);
};

struct TunnelRenderer::StreamSource : public BnStreamSource {
    StreamSource(TunnelRenderer *owner);

    virtual void setListener(const sp<IStreamListener> &listener);
    virtual void setBuffers(const Vector<sp<IMemory> > &buffers);

    virtual void onBufferAvailable(size_t index);

    virtual uint32_t flags() const;

    void doSomeWork();

protected:
    virtual ~StreamSource();

private:
    mutable Mutex mLock;

    TunnelRenderer *mOwner;

    sp<IStreamListener> mListener;

    Vector<sp<IMemory> > mBuffers;
    List<size_t> mIndicesAvailable;

    size_t mNumDeqeued;

// MStar Android Patch Begin
#if (DUMP_FILE == 1)
    FILE *mdump_fp;
#endif
#ifdef BUILD_MSTARTV
    mapi_hdcp2 *pmapi_hdcp;
#endif
// MStar Android Patch End

    DISALLOW_EVIL_CONSTRUCTORS(StreamSource);
};

////////////////////////////////////////////////////////////////////////////////

TunnelRenderer::StreamSource::StreamSource(TunnelRenderer *owner)
    : mOwner(owner),
      mNumDeqeued(0) {
// MStar Android Patch Begin
#if (DUMP_FILE == 1)
    mdump_fp = fopen("/data/wfd.ts", "ab+");
    if(mdump_fp == NULL)
        ALOGI("open dump file error : %s", strerror(errno));
#endif

#ifdef BUILD_MSTARTV
    pmapi_hdcp = mapi_hdcp2::GetInstance();
    ALOGD("pmapi_hdcp: %p", pmapi_hdcp);
#endif
// MStar Android Patch End
}

TunnelRenderer::StreamSource::~StreamSource() {
}

void TunnelRenderer::StreamSource::setListener(
        const sp<IStreamListener> &listener) {
    mListener = listener;
}

void TunnelRenderer::StreamSource::setBuffers(
        const Vector<sp<IMemory> > &buffers) {
    mBuffers = buffers;
}

void TunnelRenderer::StreamSource::onBufferAvailable(size_t index) {
    CHECK_LT(index, mBuffers.size());

    {
        Mutex::Autolock autoLock(mLock);
        mIndicesAvailable.push_back(index);
    }
    doSomeWork();
}

uint32_t TunnelRenderer::StreamSource::flags() const {
    return kFlagAlignedVideoData;
}

void TunnelRenderer::StreamSource::doSomeWork() {
    Mutex::Autolock autoLock(mLock);
// MStar Android Patch Begin
    while (!mIndicesAvailable.empty()) {
        sp<ABuffer> srcBuffer = mOwner->dequeueBuffer();
        if (srcBuffer == NULL ) {
            break;
        }
#if (DUMP_FILE == 1)
        ALOGV("dequeue TS packet of size %d", srcBuffer->size());
#ifdef BUILD_MSTARTV
        unsigned char dstbuf[1024*2];
        memset(dstbuf, 0, 1024*2);

        pmapi_hdcp->mapi_HDCP2_decrypt((unsigned char*)srcBuffer->data(), srcBuffer->size(), (unsigned char*)&dstbuf[0], srcBuffer->size(), E_MAPI_TS);
        if(mdump_fp != NULL)
            fwrite(dstbuf, srcBuffer->size(), 1, mdump_fp);
    #else
        if (mdump_fp != NULL)
            fwrite(srcBuffer->data(), srcBuffer->size(), 1, mdump_fp);
#endif
#else
        ++mNumDeqeued;

        if (mNumDeqeued == 1) {
            ALOGI("fixing real time now.");
            sp<AMessage> extra = new AMessage;

            extra->setInt32(
                    IStreamListener::kKeyDiscontinuityMask,
                    ATSParser::DISCONTINUITY_ABSOLUTE_TIME);

            extra->setInt64("timeUs", ALooper::GetNowUs());

            mListener->issueCommand(
                    IStreamListener::DISCONTINUITY,
                    false /* synchronous */,
                    extra);
        }

        ALOGV("dequeue TS packet of size %d", srcBuffer->size());

        size_t index = *mIndicesAvailable.begin();
        mIndicesAvailable.erase(mIndicesAvailable.begin());

        sp<IMemory> mem = mBuffers.itemAt(index);
        CHECK_LE(srcBuffer->size(), mem->size());
        CHECK_EQ((srcBuffer->size() % 188), 0u);
#ifdef BUILD_MSTARTV
        pmapi_hdcp->mapi_HDCP2_decrypt((unsigned char*)srcBuffer->data(), srcBuffer->size(), (unsigned char*)mem->pointer(), srcBuffer->size(), E_MAPI_TS);
#else
        memcpy(mem->pointer(), srcBuffer->data(), srcBuffer->size());
#endif
        mListener->queueBuffer(index, srcBuffer->size());
    #endif
// MStar Android Patch End
    }
}

////////////////////////////////////////////////////////////////////////////////

TunnelRenderer::TunnelRenderer(
        const sp<AMessage> &notifyLost,
        const sp<IGraphicBufferProducer> &surfaceTex)
    : mNotifyLost(notifyLost),
      mSurfaceTex(surfaceTex),
      mTotalBytesQueued(0ll),
      mLastDequeuedExtSeqNo(-1),
      mFirstFailedAttemptUs(-1ll),
      mRequestedRetransmission(false) {
}

TunnelRenderer::~TunnelRenderer() {
    destroyPlayer();
}

void TunnelRenderer::queueBuffer(const sp<ABuffer> &buffer) {
      Mutex::Autolock autoLock(mLock);

    mTotalBytesQueued += buffer->size();

    if (mPackets.empty()) {
        mPackets.push_back(buffer);
        return;
    }
    //gettimeofday(&tv_start, NULL);

    int32_t newExtendedSeqNo = buffer->int32Data();

    List<sp<ABuffer> >::iterator firstIt = mPackets.begin();
    List<sp<ABuffer> >::iterator it = --mPackets.end();
   for (;;) {
        int32_t extendedSeqNo = (*it)->int32Data();

        if (extendedSeqNo == newExtendedSeqNo) {
            // Duplicate packet.
            return;
        }

        if (extendedSeqNo < newExtendedSeqNo) {
            // Insert new packet after the one at "it".
            mPackets.insert(++it, buffer);
            return;
        }

        if (it == firstIt) {
            // Insert new packet before the first existing one.
            mPackets.insert(it, buffer);
            return;
        }

        --it;
   }
}

sp<ABuffer> TunnelRenderer::dequeueBuffer() {
 Mutex::Autolock autoLock(mLock);

    sp<ABuffer> buffer;
    int32_t extSeqNo;
    while (!mPackets.empty()) {
        buffer = *mPackets.begin();
        extSeqNo = buffer->int32Data();

        if (mLastDequeuedExtSeqNo < 0 || extSeqNo > mLastDequeuedExtSeqNo) {
            break;
        }
        // MStar Android Patch Begin
        if(mLastDequeuedExtSeqNo-extSeqNo > 3000)//seq num diff too large
            break;
        // MStar Android Patch End
        // This is a retransmission of a packet we've already returned.

        mTotalBytesQueued -= buffer->size();
        buffer.clear();
        extSeqNo = -1;

        mPackets.erase(mPackets.begin());
    }

    if (mPackets.empty()) {
        if (mFirstFailedAttemptUs < 0ll) {
            mFirstFailedAttemptUs = ALooper::GetNowUs();
            mRequestedRetransmission = false;
        } else {
            ALOGV("no packets available for %.2f secs",
                    (ALooper::GetNowUs() - mFirstFailedAttemptUs) / 1E6);
        }

        return NULL;
    }

    if (mLastDequeuedExtSeqNo < 0 || extSeqNo == mLastDequeuedExtSeqNo + 1) {
        if (mRequestedRetransmission) {
            ALOGI("Recovered after requesting retransmission of %d",
                  extSeqNo);
        }

        mLastDequeuedExtSeqNo = extSeqNo;
        mFirstFailedAttemptUs = -1ll;
        mRequestedRetransmission = false;

        mPackets.erase(mPackets.begin());

        mTotalBytesQueued -= buffer->size();

        return buffer;
    }

// MStar Android Patch Begin
#if 0
    if (mFirstFailedAttemptUs < 0ll) {
        mFirstFailedAttemptUs = ALooper::GetNowUs();

        ALOGI("failed to get the correct packet the first time.");
        return NULL;
    }

    if (mFirstFailedAttemptUs + 50000ll > ALooper::GetNowUs()) {
        // We're willing to wait a little while to get the right packet.

        if (!mRequestedRetransmission) {
            ALOGI("requesting retransmission of seqNo %d",
                  (mLastDequeuedExtSeqNo + 1) & 0xffff);

            sp<AMessage> notify = mNotifyLost->dup();
            notify->setInt32("seqNo", (mLastDequeuedExtSeqNo + 1) & 0xffff);
            notify->post();

            mRequestedRetransmission = true;
        } else {
            ALOGI("still waiting for the correct packet to arrive.");
        }

        return NULL;
    }

    ALOGI("dropping packet. extSeqNo %d didn't arrive in time",
            mLastDequeuedExtSeqNo + 1);
#endif
// MStar Android Patch End

    // Permanent failure, we never received the packet.
    mLastDequeuedExtSeqNo = extSeqNo;
    mFirstFailedAttemptUs = -1ll;
    mRequestedRetransmission = false;

    mTotalBytesQueued -= buffer->size();

    mPackets.erase(mPackets.begin());

    return buffer;
}

void TunnelRenderer::onMessageReceived(const sp<AMessage> &msg) {
    switch (msg->what()) {
        case kWhatQueueBuffer:
        {
            sp<ABuffer> buffer;
            CHECK(msg->findBuffer("buffer", &buffer));

            queueBuffer(buffer);

            if (mStreamSource == NULL) {
                if (mTotalBytesQueued > 0ll) {
                    initPlayer();
                } else {
                    ALOGI("Have %lld bytes queued...", mTotalBytesQueued);
                }
            } else {
                mStreamSource->doSomeWork();
            }
            break;
        }
        // MStar Android Patch Begin
        case kPlayerPrepared:
        {
            ALOGI("TunnelRenderer::onMessageReceived kPlayerPrepared");
            startPlayer();
            break;
        }
        case kPlayerPlaybackComplete:
        {
            ALOGI("TunnelRenderer::onMessageReceived kPlayerPlaybackComplete");
            destroyPlayer();
            break;
        }
        // MStar Android Patch End
        default:
            TRESPASS();
    }
}

void TunnelRenderer::initPlayer() {
// MStar Android Patch Begin
#if (DUMP_FILE == 1)
    mStreamSource = new StreamSource(this);
    sp<MemoryDealer> mMemoryDealer;
      int kNumBuffers = 8;
      int kBufferSize = 188 * 10;

      Vector<sp<IMemory> > myBuffers;

      mMemoryDealer = new MemoryDealer(kNumBuffers * kBufferSize);
      for (size_t i = 0; i < kNumBuffers; ++i) {
          sp<IMemory> mem = mMemoryDealer->allocate(kBufferSize);
          CHECK(mem != NULL);

          myBuffers.push(mem);
      }
      mStreamSource->setBuffers(myBuffers);
    for (size_t i = 0; i < kNumBuffers; ++i) {
        mStreamSource->onBufferAvailable(i);
    }
#else
    ALOGI("TunnelRenderer::createSurface = 0x%p", mSurfaceTex.get());
    if (mSurfaceTex == NULL) {
        mComposerClient = new SurfaceComposerClient;
        CHECK_EQ(mComposerClient->initCheck(), (status_t)OK);

        DisplayInfo info;

        sp<IBinder> display = SurfaceComposerClient::getBuiltInDisplay(0);
        SurfaceComposerClient::getDisplayInfo(display, &info);
        ALOGI("TunnelRenderer::initPlayer info.w = %d", info.w);
        ALOGI("TunnelRenderer::initPlayer info.h = %d", info.h);

        ssize_t displayWidth = info.w;
        ssize_t displayHeight = info.h;
        // MStar Android Patch Begin
        mSurfaceControl =
            mComposerClient->createSurface(
                    String8("SurfaceView"),
                    displayWidth,
                    displayHeight,
                    PIXEL_FORMAT_RGBA_8888,
                    0x400);
        // MStar Android Patch End
        CHECK(mSurfaceControl != NULL);
        CHECK(mSurfaceControl->isValid());

        SurfaceComposerClient::openGlobalTransaction();
        CHECK_EQ(mSurfaceControl->setLayer(40000/*INT_MAX*/), (status_t)OK);
        CHECK_EQ(mSurfaceControl->show(), (status_t)OK);
        SurfaceComposerClient::closeGlobalTransaction();

        mSurface = mSurfaceControl->getSurface();
        CHECK(mSurface != NULL);
    }

    sp<IServiceManager> sm = defaultServiceManager();
    sp<IBinder> binder = sm->getService(String16("media.player"));
    Parcel request;
    sp<IMediaPlayerService> service = interface_cast<IMediaPlayerService>(binder);
    CHECK(service.get() != NULL);

    mStreamSource = new StreamSource(this);
    mPlayerClient = new PlayerClient(this);
    mPlayer = service->create(mPlayerClient, 0);
    CHECK(mPlayer != NULL);
    CHECK_EQ(mPlayer->setDataSource((sp<IStreamSource>)mStreamSource), (status_t)OK);
    request.writeInt32(1);
    mPlayer->setParameter(KEY_PARAMETER_SET_GAME_MODE, request);
    mPlayer->setParameter(KEY_PARAMETER_SET_LOW_LATENCY_MODE, request);

    sp<IGraphicBufferProducer> surftext;
    surftext = mSurfaceTex != NULL ? mSurfaceTex : mSurface->getIGraphicBufferProducer();
    //surftext->setSynchronousMode(false);
    mPlayer->setVideoSurfaceTexture(surftext);

    mPlayer->prepareAsync();
#endif
// MStar Android Patch End
}

// MStar Android Patch Begin
void TunnelRenderer::startPlayer() {
    mPlayer->start();
}
// MStar Android Patch End

void TunnelRenderer::destroyPlayer() {
    mStreamSource.clear();

    mPlayer->stop();
    mPlayer.clear();

    if (mSurfaceTex == NULL) {
        mSurface.clear();
        mSurfaceControl.clear();

        mComposerClient->dispose();
        mComposerClient.clear();
    }
}

}  // namespace android
