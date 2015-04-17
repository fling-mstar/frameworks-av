//<MStar Software>
//******************************************************************************
// MStar Software
// Copyright (c) 2010 - 2012 MStar Semiconductor, Inc. All rights reserved.
// All software, firmware and related documentation herein ("MStar Software") are
// intellectual property of MStar Semiconductor, Inc. ("MStar") and protected by
// law, including, but not limited to, copyright law and international treaties.
// Any use, modification, reproduction, retransmission, or republication of all
// or part of MStar Software is expressly prohibited, unless prior written
// permission has been granted by MStar.
//
// By accessing, browsing and/or using MStar Software, you acknowledge that you
// have read, understood, and agree, to be bound by below terms ("Terms") and to
// comply with all applicable laws and regulations:
//
// 1. MStar shall retain any and all right, ownership and interest to MStar
//    Software and any modification/derivatives thereof.
//    No right, ownership, or interest to MStar Software and any
//    modification/derivatives thereof is transferred to you under Terms.
//
// 2. You understand that MStar Software might include, incorporate or be
//    supplied together with third party's software and the use of MStar
//    Software may require additional licenses from third parties.
//    Therefore, you hereby agree it is your sole responsibility to separately
//    obtain any and all third party right and license necessary for your use of
//    such third party's software.
//
// 3. MStar Software and any modification/derivatives thereof shall be deemed as
//    MStar's confidential information and you agree to keep MStar's
//    confidential information in strictest confidence and not disclose to any
//    third party.
//
// 4. MStar Software is provided on an "AS IS" basis without warranties of any
//    kind. Any warranties are hereby expressly disclaimed by MStar, including
//    without limitation, any warranties of merchantability, non-infringement of
//    intellectual property rights, fitness for a particular purpose, error free
//    and in conformity with any international standard.  You agree to waive any
//    claim against MStar for any loss, damage, cost or expense that you may
//    incur related to your use of MStar Software.
//    In no event shall MStar be liable for any direct, indirect, incidental or
//    consequential damages, including without limitation, lost of profit or
//    revenues, lost or damage of data, and unauthorized system use.
//    You agree that this Section 4 shall still apply without being affected
//    even if MStar Software has been modified by MStar in accordance with your
//    request or instruction for your use, except otherwise agreed by both
//    parties in writing.
//
// 5. If requested, MStar may from time to time provide technical supports or
//    services in relation with MStar Software to you for your use of
//    MStar Software in conjunction with your or your customer's product
//    ("Services").
//    You understand and agree that, except otherwise agreed by both parties in
//    writing, Services are provided on an "AS IS" basis and the warranty
//    disclaimer set forth in Section 4 above shall apply.
//
// 6. Nothing contained herein shall be construed as by implication, estoppels
//    or otherwise:
//    (a) conferring any license or right to use MStar name, trademark, service
//        mark, symbol or any other identification;
//    (b) obligating MStar or any of its affiliates to furnish any person,
//        including without limitation, you and your customers, any assistance
//        of any kind whatsoever, or any information; or
//    (c) conferring any license or right under any intellectual property right.
//
// 7. These terms shall be governed by and construed in accordance with the laws
//    of Taiwan, R.O.C., excluding its conflict of law rules.
//    Any and all dispute arising out hereof or related hereto shall be finally
//    settled by arbitration referred to the Chinese Arbitration Association,
//    Taipei in accordance with the ROC Arbitration Law and the Arbitration
//    Rules of the Association by three (3) arbitrators appointed in accordance
//    with the said Rules.
//    The place of arbitration shall be in Taipei, Taiwan and the language shall
//    be English.
//    The arbitration award shall be final and binding to both parties.
//
//******************************************************************************
//<MStar Software>

#define LOG_TAG "VEcaptureWriter"
#include <utils/Log.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <media/stagefright/VECaptureWriter.h>
#include <media/stagefright/MediaBuffer.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaSource.h>
#include <media/stagefright/Utils.h>
#include <binder/IServiceManager.h>

namespace android {
static volatile int32_t gLogLevel = 0;
#define ALOG1(...) ALOGD_IF(gLogLevel >= 1, __VA_ARGS__);

class VECaptureWriter::Track {
public:
    Track(VECaptureWriter *owner, const sp<MediaSource> &source);
    ~Track();

    status_t start(MetaData *params);
    status_t stop();
    status_t pause();
    bool reachedEOS();
    void setWriteFunc(void *cookie,
                      ssize_t (*write)(void *cookie, const void *data, size_t size));

private:
    VECaptureWriter *mOwner;
    sp<MetaData> mMeta;
    sp<MediaSource> mSource;
    volatile bool mDone;
    volatile bool mPaused;
    volatile bool mResumed;

    pthread_t mThread;
    bool mReachedEOS;

    void *mTWriteCookie;
    ssize_t (*mTWriteFunc)(void *cookie, const void *data, size_t size);

    static void *ThreadWrapper(void *me);
    void threadEntry();

    Track(const Track &);
    Track &operator=(const Track &);
};

VECaptureWriter::VECaptureWriter(int frameRate, void *cookie,
        ssize_t (*write)(void *cookie, const void *data, size_t size))
:mFrameRate(frameRate),
mPaused(false),
   mStarted(false),
   mWriteCookie(cookie),
   mWriteFunc(write) {
}

VECaptureWriter::~VECaptureWriter() {
    stop();

    while (!mTracks.empty()) {
        List<Track *>::iterator it = mTracks.begin();
        delete *it;
        (*it) = NULL;
        mTracks.erase(it);
    }
    mTracks.clear();
}

status_t VECaptureWriter::addSource(const sp<MediaSource> &source) {
    Track *track = new Track(this, source);
    track->setWriteFunc(mWriteCookie, mWriteFunc);
    mTracks.push_back(track);

    return OK;
}

status_t VECaptureWriter::startTracks(MetaData *params) {
    ALOG1("startTracks ->");

    for (List<Track *>::iterator it = mTracks.begin();
        it != mTracks.end(); ++it) {
        status_t err = (*it)->start(params);

        if (err != OK) {
            for (List<Track *>::iterator it2 = mTracks.begin();
                it2 != it; ++it2) {
                (*it2)->stop();
            }

            return err;
        }
    }
    ALOG1("startTracks <-");

    return OK;
}

status_t VECaptureWriter::start(MetaData *param) {
    ALOG1("start ->");

    if (mStarted) {
        if (mPaused) {
            mPaused = false;
            return startTracks(param);
        }
        return OK;
    }

    //status_t err = startWriterThread();
    //if (err != OK) {
    //    return err;
    //}

    status_t err = startTracks(param);
    if (err != OK) {
        return err;
    }

    mStarted = true;

    if (mFrameRate > 0)
        mDelayUs =  (int64_t)1000000 / (int64_t)mFrameRate;
    else
        mDelayUs =  (int64_t)1000000 / (int64_t)15;

    ALOG1("start <-");

    return OK;
}

status_t VECaptureWriter::pause() {
    mPaused = true;
    status_t err = OK;
    for (List<Track *>::iterator it = mTracks.begin();
        it != mTracks.end(); ++it) {
        status_t status = (*it)->pause();
        if (status != OK) {
            err = status;
        }
    }
    return err;
}

void VECaptureWriter::stopWriterThread() {
    ALOG1("stopWriterThread");

    {
        Mutex::Autolock autolock(mLock);

        mDone = true;
    }

    //void *dummy;
    //pthread_join(mThread, &dummy);
}

status_t VECaptureWriter::stop() {
    status_t err = OK;

    for (List<Track *>::iterator it = mTracks.begin();
        it != mTracks.end(); ++it) {
        status_t status = (*it)->stop();
        if (err == OK && status != OK) {
            err = status;
        }
    }

    stopWriterThread();

    mStarted = false;
    return err;
}

bool VECaptureWriter::reachedEOS() {
    bool allDone = true;
    for (List<Track *>::iterator it = mTracks.begin();
        it != mTracks.end(); ++it) {
        if (!(*it)->reachedEOS()) {
            allDone = false;
            break;
        }
    }

    return allDone;
}

void VECaptureWriter::lock() {
    mLock.lock();
}

void VECaptureWriter::unlock() {
    mLock.unlock();
}

////////////////////////////////////////////////////////////////////////////////

VECaptureWriter::Track::Track(
                             VECaptureWriter *owner, const sp<MediaSource> &source)
: mOwner(owner),
mMeta(source->getFormat()),
mSource(source),
mDone(false),
mPaused(false),
mResumed(false),
mReachedEOS(false) {
}

VECaptureWriter::Track::~Track() {
    stop();
}

void *VECaptureWriter::ThreadWrapper(void *me) {
    ALOGV("ThreadWrapper: %p", me);
    VECaptureWriter *writer = static_cast<VECaptureWriter *>(me);
    writer->threadFunc();
    return NULL;
}

void VECaptureWriter::threadFunc() {
    ALOG1("threadFunc");

    prctl(PR_SET_NAME, (unsigned long)"VECaptureWriter", 0, 0, 0);
    while (!mDone) {
        {
            Mutex::Autolock autolock(mLock);
//            mChunkReadyCondition.wait(mLock);
//            CHECK_EQ(writeOneChunk(), OK);
        }
    }

    {
        // Write ALL samples
        Mutex::Autolock autolock(mLock);
//        writeChunks();
    }
}

status_t VECaptureWriter::startWriterThread() {
    ALOG1("startWriterThread");

    mDone = false;
    for (List<Track *>::iterator it = mTracks.begin();
        it != mTracks.end(); ++it) {
//        ChunkInfo info;
//        info.mTrack = *it;
//        mChunkInfos.push_back(info);
    }

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_create(&mThread, &attr, ThreadWrapper, this);
    pthread_attr_destroy(&attr);
    return OK;
}

status_t VECaptureWriter::Track::pause() {
    mPaused = true;
    return OK;
}

status_t VECaptureWriter::Track::start(MetaData *params) {
    ALOG1("Track start -> mDone = %d, mPaused = %d", mDone, mPaused);

    if (!mDone && mPaused) {
        mPaused = false;
        mResumed = true;
        return OK;
    }

    int64_t startTimeUs;
    if (params == NULL || !params->findInt64(kKeyTime, &startTimeUs)) {
        startTimeUs = 0;
    }

    sp<MetaData> meta = new MetaData;
    meta->setInt64(kKeyTime, startTimeUs);
    status_t err = mSource->start();
    if (err != OK) {
        mDone = mReachedEOS = true;
        return err;
    }

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    mDone = false;
    mReachedEOS = false;

    pthread_create(&mThread, &attr, ThreadWrapper, this);
    pthread_attr_destroy(&attr);

    ALOG1("Track start <-");

    return OK;
}

status_t VECaptureWriter::Track::stop() {
    ALOG1("stop");

    if (mDone) {
        return OK;
    }

    mDone = true;

    void *dummy;
    pthread_join(mThread, &dummy);

    status_t err = (status_t) dummy;

    {
        status_t status = mSource->stop();
        if (err == OK && status != OK && status != ERROR_END_OF_STREAM) {
            err = status;
        }
    }

    return err;
}

bool VECaptureWriter::Track::reachedEOS() {
    return mReachedEOS;
}

void VECaptureWriter::Track::setWriteFunc(void *cookie,
                  ssize_t (*write)(void *cookie, const void *data, size_t size)) {
    mTWriteCookie = cookie;
    mTWriteFunc = write;
}

void *VECaptureWriter::Track::ThreadWrapper(void *me) {
    Track *track = static_cast<Track *>(me);

    track->threadEntry();
    return NULL;
}

void VECaptureWriter::Track::threadEntry() {
    sp<MetaData> meta = mSource->getFormat();
    const char *mime;
    meta->findCString(kKeyMIMEType, &mime);
    bool is_mpeg4 = !strcasecmp(mime, MEDIA_MIMETYPE_VIDEO_MPEG4);
    bool is_avc = !strcasecmp(mime, MEDIA_MIMETYPE_VIDEO_AVC);
    //struct timeval start, end;

    ALOG1("Track::threadEntry start ....");

    //gettimeofday(&start, NULL);

    MediaBuffer *buffer;
    while (!mDone) {
        if (mSource->read(&buffer) != OK)
            // OMXCodec::read change to timeout return (when using camerasource)
            // reason: when media recorder start with no frame send to OMXCodec (with camerasource),
            // media recorder can not stop always (because mSource->read can not return)
            continue;

        if (buffer->range_length() == 0) {
            buffer->release();
            buffer = NULL;

            continue;
        }

        int64_t timestampUs;
        buffer->meta_data()->findInt64(kKeyTime, &timestampUs);

        ALOG1("read frame done: timestampUs = %lld us", timestampUs);
#if 0
        gettimeofday(&end, NULL);

        int64_t diff;
        diff = ((int64_t)end.tv_sec * 1000000 + end.tv_usec) - ((int64_t)start.tv_sec * 1000000 + start.tv_usec);
        if (diff < mOwner->mDelayUs) {
            //To Do: check necessary of below
            usleep((mOwner->mDelayUs - diff));
        }
        gettimeofday(&start, NULL);
#endif

        if(mTWriteFunc)
            (*mTWriteFunc)(mTWriteCookie, buffer->data(), buffer->range_length());
        buffer->release();
        buffer = NULL;
        ALOG1("send frame done");
    }

    mReachedEOS = true;
}

}  // namespace android
