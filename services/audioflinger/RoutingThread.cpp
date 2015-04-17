/*
**
** Copyright 2007, The Android Open Source Project
** Copyright (C) 2013 Broadcom Corporation
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#define LOG_TAG "RoutingThread"

#define LOG_NDEBUG 0

#include <cutils/compiler.h>
#include <media/AudioParameter.h>
#include <private/media/AudioTrackShared.h>
#include <binder/IPCThreadState.h>
#include "AudioFlinger.h"

#ifndef MAX
#define MAX(a, b)       (((a) > (b))?(a):(b))
#endif

#define ROUTING_BUFFER_SIZE 1024*2 // 960*4 20 ms 48kHz stereo

namespace android {

bool AudioFlinger::RoutingThread::InitializeInternal(AudioFlinger *pAudioFlinger)
{
    status_t lStatus;
    IAudioFlinger::track_flags_t track_flags = IAudioFlinger::TRACK_DEFAULT;

    // Get the process Id
    pid_t tid = getpid();

    // keep RT lock until we are fully started to avoid
    // setParameter to be called before routing thread is ready
    AutoMutex _l(mRtLock);

    // Get the playback thread associated with the output
    PlaybackThread *pb_thread = pAudioFlinger->checkPlaybackThread_l(mOutput);
    if (pb_thread == NULL)
    {
        ALOGE("thread for output io %d not found ", mOutput);
        return false;
    }

    // Get the record thread associated with the input
    RecordThread *input_thread = pAudioFlinger->checkRecordThread_l(mInput);
    if (input_thread == NULL)
    {
        ALOGE("thread for input io %d not found ", mInput);
        return false;
    }

    // Register the client
    sp<AudioFlinger::Client> client = pAudioFlinger->registerPid_l(tid);

    // Create the Output Track -------------------------------------------------

    // Ensure that buffer depth covers at least audio hardware latency
    uint32_t minBufCount = pb_thread->latency() /
                          ((1000 * pb_thread->frameCount())/ pb_thread->sampleRate());

    mOutputFormat = pb_thread->format();
    mOutputChannelCount = pb_thread->channelCount();

    ALOGD("pb_thread: %p minBufCount %d, out latency %d, pid %d, fmt %x, chcnt %d",
           pb_thread, minBufCount, pb_thread->latency(), tid, mOutputFormat,
           mOutputChannelCount);

    // Set the frame size for the output
    if (audio_is_linear_pcm(mOutputFormat)) {
        mOutputFrameSize = pb_thread->channelCount() *
                           audio_bytes_per_sample(mOutputFormat);
        mOutputFrameSizeAF = pb_thread->channelCount() * sizeof(int16_t);
    } else {
        ALOG_ASSERT("non pcm routing not supported yet");
        mOutputFrameSize = sizeof(uint8_t);
        mOutputFrameSizeAF = sizeof(uint8_t);
        return false;
    }

    // use minimal framecount for low latency (we are tightly synced with
    // record thread and don't need too much buffering to route data */

    // AUDIO_SINK_TUNING ??????????????
    minBufCount = 1;

    size_t minFrameCount = pb_thread->frameCount()* minBufCount;

    ALOGD("minFrameCount: %d, afFrameCount=%d, minBufCount=%d, sampleRate=%d, afSampleRate=%d"
          ", afLatency=%d", minFrameCount, pb_thread->frameCount(), minBufCount,
           pb_thread->sampleRate(), pb_thread->sampleRate(), pb_thread->latency());

    size_t frameCount = minFrameCount;

    ALOGV("RoutingThread::InitializeInternal :: creating output track, frameCount %d", frameCount);

    sp<PlaybackThread::Track> output_track = pb_thread->createTrack_l(client,
                                            AUDIO_STREAM_MUSIC,
                                            pb_thread->sampleRate(),
                                            pb_thread->format(),
                                            pb_thread->channelMask(),
                                            frameCount,
                                            NULL,
                                            0 /*lSessionId*/,
                                            &track_flags,
                                            tid,
                                            IPCThreadState::self()->getCallingUid(),
                                            &lStatus);

    if ((output_track == 0) || (lStatus != NO_ERROR))
    {
        ALOGE("AudioFlinger could not create output_track, status: %d", lStatus);
        return false;
    }

    // acquire strong reference immediately to avoid timing
    // issues if connection toggles quickly
    mOutputTrack      = output_track;
    mOutputFrameCount = frameCount;
    mOutputThread     = pb_thread;

    ALOGD("mOutput latency : %d",
        pb_thread->mOutput->stream->get_latency(pb_thread->mOutput->stream));

    sp<IMemory> iMem = output_track->getCblk();
    if (iMem == 0) {
        ALOGE("Could not get control block");
        return NO_INIT;
    }

    mOutputCblkMemory = iMem;
    audio_track_cblk_t* cblk = static_cast<audio_track_cblk_t*>(iMem->pointer());
    size_t temp = cblk->frameCount_;
    if (temp < frameCount || (frameCount == 0 && temp == 0)) {
        // In current design, AudioTrack client checks and ensures frame count validity before
        // passing it to AudioFlinger so AudioFlinger should not return a different value except
        // for fast track as it uses a special method of assigning frame count.
        ALOGW("Requested frameCount %u but received frameCount %u", frameCount, temp);
    }

    frameCount        = temp;
    mOutputFrameCount = frameCount;

    mOutputBuffers = (char*)cblk + sizeof(audio_track_cblk_t);

    mOutputProxy = new AudioTrackClientProxy(cblk,
                                             mOutputBuffers,
                                             frameCount,
                                             mOutputFrameSizeAF);

    mOutputProxy->setSampleRate(pb_thread->sampleRate());

    // Create the Input Track --------------------------------------------------

    minFrameCount = pAudioFlinger->getInputBufferSize(input_thread->sampleRate(), input_thread->format(),
                                       input_thread->channelMask());

    // AUDIO_SINK_TUNING
    minFrameCount /= 4; //488

    mInputChannelCount = input_thread->channelCount();
    mInputFormat       = input_thread->format();
    mInputStream       = input_thread->stream();

    if (audio_is_linear_pcm(mInputFormat)) {
        mInputFrameSize = mInputChannelCount *
                          audio_bytes_per_sample(mInputFormat);
    } else {
        ALOG_ASSERT("non pcm routing not supported yet");
        mInputFrameSize = sizeof(uint8_t);
        return false;
    }

    ALOGV("RoutingThread::InitializeInternal :: creating input track, minFrameCount %d", minFrameCount);

    sp<RecordThread::RecordTrack>
        input_track = input_thread->createRecordTrack_l(client,
                                                        input_thread->sampleRate(),
                                                        input_thread->format(),
                                                        input_thread->channelMask(),
                                                        minFrameCount,
                                                        0,
                                                        IPCThreadState::self()->getCallingUid(),
                                                        &track_flags,
                                                        -1,
                                                        &lStatus);

    if (lStatus != NO_ERROR) {
        // remove local strong reference to Client before deleting the RecordTrack so
        // that the Client destructor is called by the TrackBase destructor with mLock held
        client.clear();
        input_track.clear();
        ALOGE("lStatus %d", lStatus);
        return 0;
    }

    // aquire strong reference immediately to avoid timing
    // issues if connection toggles quickly
    mInputTrack = input_track;
    mInputFrameCount = minFrameCount;

    // store cblk for record track
    mInputCblkMemory = input_track->getCblk();
    if (mInputCblkMemory == 0) {
        lStatus = NO_MEMORY;
        return 0;
    }

    cblk = static_cast<audio_track_cblk_t*>(mInputCblkMemory->pointer());

    // no shared buffer for record track
    mInputBuffers = (char*)cblk + sizeof(audio_track_cblk_t);

    ALOGD("create record proxy frameCount %d, fr sz %d", mInputFrameCount,
           mInputFrameSize);

    mInputProxy = new AudioRecordClientProxy(cblk,
                                             (char*)mInputBuffers,
                                             mInputFrameCount,
                                             mInputFrameSize);
    threadLoopRun();

    return true;
}

AudioFlinger::RoutingThread *AudioFlinger::RoutingThread::Initialize(
    AudioFlinger *pAudioFlinger, audio_io_handle_t input, audio_io_handle_t output,
    audio_io_handle_t *rt_io_handle)
{
    RoutingThread *pRoutingThread = new RoutingThread(input, output);

    if (pRoutingThread != NULL) {
        if (!pRoutingThread->InitializeInternal(pAudioFlinger)) {
            pRoutingThread = NULL;
        }
        else {
            *rt_io_handle = input;
        }
    }

    return pRoutingThread;
}


// Enables routing data between input and output internally in audioflinger to
// minimize latency.
AudioFlinger::RoutingThread::RoutingThread(audio_io_handle_t input, audio_io_handle_t output)
    :   Thread(false /*canCallJava*/),
        mInput(input),
        mInputTrack(NULL),
        mInputActive(false),
        mOutput(output),
        mOutputTrack(NULL),
        mOutputActive(false)
{
    ALOGD("new RoutingThread %p", this);
    mRoutingThreadStarted = false;
    mRoutingThreadRunning = false;
    mRoutingThreadCommandPending = RT_CMD_NONE;
}

AudioFlinger::RoutingThread::~RoutingThread()
{
    // If not done already, exit the loopThread
    threadLoopExit();

    // wait for routing thread loop to exit
    this->requestExit();
    this->requestExitAndWait();

    ALOGD("~RoutingThread %p destructor complete.", this);
}

bool AudioFlinger::RoutingThread::isReady(void)
{
    return mRoutingThreadRunning;
}

status_t AudioFlinger::RoutingThread::setParameters(const String8& keyValuePairs)
{
    AudioParameter param = AudioParameter(keyValuePairs);
    String8 a2dpInputEnabled;

    status_t status = INVALID_OPERATION;

    ALOGD("RoutingThread::setParameters() %s", keyValuePairs.string());
    Mutex::Autolock _l(mRtLock);

    if (param.get(String8("A2dpInputEnable"), a2dpInputEnabled) == NO_ERROR) {
        bool isEnabled = a2dpInputEnabled == "true";

        status = NO_ERROR;

        if (isEnabled && mRoutingThreadStarted) {
            return status;
        } else if (isEnabled && !mRoutingThreadStarted) {
            // we are not started, set pending start command
            mRoutingThreadCommandPending = RT_CMD_START;
        } else if (!isEnabled && mRoutingThreadStarted) {
            // we are not stopped, set pending stop command
            mRoutingThreadCommandPending = RT_CMD_STOP;
        } else if (!isEnabled && !mRoutingThreadStarted) {
            return status;
        }

        mInputActive = isEnabled;
        mOutputActive = isEnabled;

        // wake up routing thread
        mRtCond.signal();
    }
    ALOGD("RoutingThread::setParameters() %s done", keyValuePairs.string());

    return status;
}

// called with mRtLock locked
status_t AudioFlinger::RoutingThread::startRouting(void)
{
    status_t lStatus;

    sp<IMemory> mCblkMemory;
    MixerThread *output_thread;

    ALOGD("%s Enter...", __FUNCTION__);

    mCblkMemory = mInputTrack->getCblk();

    ALOGD("%s :: got cblk for mInputTrack", __FUNCTION__);

    if (mCblkMemory == 0) {
        return NO_MEMORY;
    }

    // initialize input buffer control block
    mInputCblk = static_cast<audio_track_cblk_t*>(mCblkMemory->pointer());

    mCblkMemory = mOutputTrack->getCblk();

    ALOGD("%s :: got cblk for mOutputTrack", __FUNCTION__);

    if (mCblkMemory == 0) {
        return NO_MEMORY;
    }

    ALOGD("%s :: initialize output buffer control block", __FUNCTION__);

    // initialize output buffer control block
    mOutputCblk = static_cast<audio_track_cblk_t*>(mCblkMemory->pointer());

    mRoutingThreadStarted = true;

    ALOGD("%s :: add output track to output thread. Flags: 0x%x", __FUNCTION__, mOutputCblk->mFlags);

    // MR2 FIXME -- utilize thread priority change in void AudioTrack::start() ?

    ALOGD("%s :: internally start output track", __FUNCTION__);

    // internally start output track
    lStatus = mOutputTrack->start(AudioSystem::SYNC_EVENT_NONE, 0);
    ALOGD("%s :: started output track status %d Flags: 0x%x", __FUNCTION__, lStatus, mOutputCblk->mFlags);
    if (lStatus != NO_ERROR)
        return lStatus;

    ALOGD("%s :: internally start input track", __FUNCTION__);

    // internally start input track
    lStatus = mInputTrack->start(AudioSystem::SYNC_EVENT_NONE, 0);
    ALOGD("%s :: started record track status %d", __FUNCTION__, lStatus);

    return lStatus;
}

// called with mRtLock locked
void AudioFlinger::RoutingThread::stopRouting(void)
{
    ALOGD("stopRouting");

    if (mRoutingThreadStarted == false)
        return;

    mRoutingThreadStarted = false;

    // internally stop input track
//    mInputProxy->interrupt();
    mInputTrack->stop();
    ALOGD("stopRouting :: stopped input track");

    // internally stop output track
    mOutputTrack->stop();
    ALOGD("stopRouting :: stopped output track");
}

bool AudioFlinger::RoutingThread::threadLoop()
{
    uint8_t *mRoutingBuffer;

    ALOGD("RoutingThread::threadLoop starting");

    // allocate internal buffer to route data
    mRoutingBuffer = new uint8_t[ROUTING_BUFFER_SIZE];

    if (mRoutingBuffer == NULL)
        return 0;

    // Inform stack that the routing thead is up and running.
    mInputStream->set_parameters(mInputStream, "RoutingThreadReady=true");

    ALOGD("RoutingThread::threadLoop enter loop");


    while (mRoutingThreadRunning)
    {
        mRtLock.lock();
        if (mRoutingThreadCommandPending)
        {
            switch (mRoutingThreadCommandPending)
            {
                case RT_CMD_START:
                    if (startRouting() != NO_ERROR)
                    {
                        mRtLock.unlock();
                        goto exitRoutingThread;
                    }
                    break;

                case RT_CMD_STOP:
                    stopRouting();
                    break;

                default :
                    ALOGE("unknown routing thread command (%d)",
                          mRoutingThreadCommandPending);
                    break;
            }
            mRoutingThreadCommandPending = RT_CMD_NONE;
        }

        mRtLock.unlock();

        if (mRoutingThreadStarted == false)
        {
            AutoMutex lock(mRtLock);
            ALOGD("routing thread standby");
            mRtCond.wait(mRtLock);
            ALOGD("routing thread woke up");
            continue;
        }

       int ret = read(mRoutingBuffer, ROUTING_BUFFER_SIZE);
        if (ret > 0)
        {
            ALOGV("RoutingThread::threadLoop : [ROUTE DATA] %d frames input ---> output",
                   ret/frameSize());

            ret = write(mRoutingBuffer, ret);
        }

        // FIXME - dj -Currently just clear the Invalid flag or no output will be heard.
        int oldFlags = android_atomic_and(~CBLK_INVALID, &mOutputCblk->mFlags);
        ALOGV("threadLoop:oldFlags: 0x%x", oldFlags);

    }

exitRoutingThread :

    // make sure all tracks are stopped and removed
    stopRouting();

    // output and inputs are closed from apm

    delete[] mRoutingBuffer;

    ALOGD("========= Now exiting routing thread ===========");

    return false;

}

void AudioFlinger::RoutingThread::threadLoopRun()
{
    // NOTE: mRtLock already held when this is called.
    if (mRoutingThreadRunning == false) {
        ALOGD("RoutingThread::threadLoopRun Starting thread");
        mRoutingThreadRunning = true;

        run("RoutingThread", ANDROID_PRIORITY_AUDIO);
    }
}

void AudioFlinger::RoutingThread::threadLoopExit()
{
    mRtLock.lock();

    if (mRoutingThreadRunning == true) {
        ALOGD("RoutingThread::threadLoopExit signalling thread to exit");
        mRoutingThreadCommandPending = RT_CMD_STOP;
        mRoutingThreadRunning = false;
        mRtLock.unlock();
        mRtCond.signal();
    }
    else {
        mRtLock.unlock();
    }
}

// AudioRecord::obtainBuffer equivalent

status_t AudioFlinger::RoutingThread::obtainBuffer(Buffer* audioBuffer, const struct timespec *requested,
        struct timespec *elapsed, size_t *nonContig)
{
    // previous and new IAudioRecord sequence numbers are used to detect track re-creation
#if 0
    uint32_t oldSequence = 0;
    uint32_t newSequence;
#endif

    Proxy::Buffer buffer;
    status_t status = NO_ERROR;

    static const int32_t kMaxTries = 5;
    int32_t tryCounter = kMaxTries;

    do {
        // obtainBuffer() is called with mutex unlocked, so keep extra references to these fields to
        // keep them from going away if another thread re-creates the track during obtainBuffer()
        sp<AudioRecordClientProxy> proxy;
        sp<IMemory> iMem;
        {
            // start of lock scope
            AutoMutex lock(mLock);

#if 0
            newSequence = mSequence;
            // did previous obtainBuffer() fail due to media server death or voluntary invalidation?
            if (status == DEAD_OBJECT) {
                // re-create track, unless someone else has already done so
                if (newSequence == oldSequence) {
                    status = restoreRecord_l("obtainBuffer");
                    if (status != NO_ERROR) {
                        break;
                    }
                }
            }
            oldSequence = newSequence;
#endif
            // Keep the extra references
            proxy = mInputProxy;
            iMem  = mInputTrack->getCblk();

            // Non-blocking if track is stopped
#if 0
            if (!mActive) {
                requested = &ClientProxy::kNonBlocking;
            }
#endif

        }   // end of lock scope

        buffer.mFrameCount = audioBuffer->frameCount;
        // FIXME starts the requested timeout and elapsed over from scratch
        status = proxy->obtainBuffer(&buffer, requested, elapsed);

        // FIXME - dj - need to add condition to jump out on thread stop

    } while ((status == DEAD_OBJECT) && (tryCounter-- > 0));

    audioBuffer->frameCount = buffer.mFrameCount;
    audioBuffer->size       = buffer.mFrameCount * frameSize();
    audioBuffer->raw        = buffer.mRaw;
    if (nonContig != NULL) {
        *nonContig = buffer.mNonContig;
    }

//    ALOGV("[%s] exit status: %d frameCount: %d", __FUNCTION__, status, audioBuffer->frameCount);

    return status;
}

void AudioFlinger::RoutingThread::releaseBuffer(Buffer* audioBuffer)
{
    size_t stepCount = audioBuffer->size / frameSize();
//    ALOGV("RoutingThread::releaseBuffer %d frames", stepCount);

    if (stepCount == 0) {
        return;
    }

    Proxy::Buffer buffer;
    buffer.mFrameCount = stepCount;
    buffer.mRaw        = audioBuffer->raw;

    AutoMutex lock(mLock);
    mInputProxy->releaseBuffer(&buffer);

    return ;
}

size_t AudioFlinger::RoutingThread::frameSize() const
{
    ALOG_ASSERT(mOutputFormat == mInputFormat, "multiformat routing not supported");
    ALOG_ASSERT(mOutputChannelCount == mInputChannelCount, "Input to Output Channel Count difference not supported");

    if (audio_is_linear_pcm(mOutputFormat)) {
        return mOutputChannelCount*audio_bytes_per_sample(mOutputFormat);
    } else {
        return sizeof(uint8_t);
    }
}

status_t AudioFlinger::RoutingThread::read(void* buffer, size_t userSize)
{
    size_t mFrameSize = frameSize();
    const struct timespec TenMsPeriod = {0 /*tv_sec*/, 10000000 /*tv_nsec*/};

    ALOGV("RoutingThread::read Enter %p: %d bytes", this, userSize);

    if (ssize_t(userSize) < 0 || (buffer == NULL && userSize != 0)) {
        // sanity-check. user is most-likely passing an error code, and it would
        // make the return value ambiguous (actualSize vs error).
        ALOGE("AudioRecord::read(buffer=%p, size=%u (%d)", buffer, userSize, userSize);
        return BAD_VALUE;
    }

    ssize_t read = 0;
    Buffer audioBuffer;

    while (userSize >= mFrameSize) {
        audioBuffer.frameCount = userSize / mFrameSize;

        // Put a 10ms timeout on this obtainBuffer call
        status_t err = obtainBuffer(&audioBuffer, &TenMsPeriod);

        if (err < 0) {
            if (read > 0) {
                break;
            }

            ALOGV("RoutingThread::read Exit %p: err: %d", this, err);

            return ssize_t(err);
        }

        size_t bytesRead = audioBuffer.size;
        memcpy(buffer, audioBuffer.i8, bytesRead);
        buffer = ((char *) buffer) + bytesRead;
        userSize -= bytesRead;
        read += bytesRead;

        releaseBuffer(&audioBuffer);
    }

    ALOGV("RoutingThread::read Exit %p: read %d bytes", this, read);

    return read;

}

// AudioTrack::obtainBuffer equivalent code
status_t AudioFlinger::RoutingThread::obtainBufferOut(Buffer* audioBuffer, const struct timespec *requested,
        struct timespec *elapsed, size_t *nonContig)
{
#if 0
    // previous and new IAudioTrack sequence numbers are used to detect track re-creation
    uint32_t oldSequence = 0;
    uint32_t newSequence;
#endif

//    ALOGV("[%s] Enter status: frameCount: %d", __FUNCTION__, audioBuffer->frameCount);

    Proxy::Buffer buffer;
    status_t status = NO_ERROR;

    static const int32_t kMaxTries = 5;
    int32_t tryCounter = kMaxTries;

    do {
        // obtainBuffer() is called with mutex unlocked, so keep extra references to these fields to
        // keep them from going away if another thread re-creates the track during obtainBuffer()
        sp<AudioTrackClientProxy> proxy;
        sp<IMemory> iMem;

        {   // start of lock scope
            AutoMutex lock(mLock);

#if 0
            newSequence = mSequence;
            // did previous obtainBuffer() fail due to media server death or voluntary invalidation?
            if (status == DEAD_OBJECT) {
                // re-create track, unless someone else has already done so
                if (newSequence == oldSequence) {
                    status = restoreTrack_l("obtainBuffer");
                    if (status != NO_ERROR) {
                        buffer.mFrameCount = 0;
                        buffer.mRaw = NULL;
                        buffer.mNonContig = 0;
                        break;
                    }
                }
            }
            oldSequence = newSequence;
#endif

            // Keep the extra references
            proxy = mOutputProxy;
            iMem = mOutputTrack->getCblk();

#if 0
            if (mState == STATE_STOPPING) {
                status = -EINTR;
                buffer.mFrameCount = 0;
                buffer.mRaw = NULL;
                buffer.mNonContig = 0;
                break;
            }
#endif
            // Non-blocking if track is stopped or paused
#if 0
            if (mState != STATE_ACTIVE) {
#endif
            // FIXME - dj - Need to add a timeout???
//                requested = &ClientProxy::kNonBlocking;
#if 0
            }
#endif

        }   // end of lock scope

        buffer.mFrameCount = audioBuffer->frameCount;
        // FIXME starts the requested timeout and elapsed over from scratch
        status = proxy->obtainBuffer(&buffer, requested, elapsed);

    } while ((status == DEAD_OBJECT) && (tryCounter-- > 0));

    audioBuffer->frameCount = buffer.mFrameCount;
    audioBuffer->size = buffer.mFrameCount * frameSize();
    audioBuffer->raw = buffer.mRaw;
    if (nonContig != NULL) {
        *nonContig = buffer.mNonContig;
    }

//    ALOGV("[%s] exit status: %d frameCount: %d", __FUNCTION__, status, audioBuffer->frameCount);

    return status;
}


void AudioFlinger::RoutingThread::releaseBufferOut(Buffer* audioBuffer)
{
#if 0
    if (mTransfer == TRANSFER_SHARED) {
        return;
    }
#endif

    size_t stepCount = audioBuffer->size / frameSize();
    if (stepCount == 0) {
        return;
    }

    Proxy::Buffer buffer;
    buffer.mFrameCount = stepCount;
    buffer.mRaw = audioBuffer->raw;

    AutoMutex lock(mLock);
    mOutputProxy->releaseBuffer(&buffer);

    int oldFlags = android_atomic_and(~CBLK_DISABLED, &mOutputCblk->mFlags);

    if (oldFlags & CBLK_DISABLED) {
        status_t lStatus = mOutputTrack->start(AudioSystem::SYNC_EVENT_NONE, 0);
        ALOGD("%s :: restarting disabled output track status %d Flags: 0x%x", __FUNCTION__, lStatus, oldFlags);
    }

}

// AudioTrack::write equivalent
ssize_t AudioFlinger::RoutingThread::write(const void* buffer, size_t userSize)
{
    ALOGV("RoutingThread::write %p: %d bytes", this, userSize);

    if (ssize_t(userSize) < 0 || (buffer == NULL && userSize != 0)) {
        // Sanity-check: user is most-likely passing an error code, and it would
        // make the return value ambiguous (actualSize vs error).
        ALOGE("AudioTrack::write(buffer=%p, size=%u (%d)", buffer, userSize, userSize);
        return BAD_VALUE;
    }

    size_t written = 0;
    Buffer audioBuffer;
    size_t mFrameSize = frameSize();

    while (userSize >= mFrameSize) {
        audioBuffer.frameCount = userSize / mFrameSize;

        status_t err = obtainBufferOut(&audioBuffer, &ClientProxy::kForever);
        if (err < 0) {
            if (written > 0) {
                break;
            }

            ALOGV("RoutingThread::write %p: err: %d size: %d", this, err, audioBuffer.size);
            return ssize_t(err);
        }

        size_t toWrite;
#if 0
        if (mFormat == AUDIO_FORMAT_PCM_8_BIT && !(mFlags & AUDIO_OUTPUT_FLAG_DIRECT)) {
            // Divide capacity by 2 to take expansion into account
            toWrite = audioBuffer.size >> 1;
            memcpy_to_i16_from_u8(audioBuffer.i16, (const uint8_t *) buffer, toWrite);
        } else {
#endif
            toWrite = audioBuffer.size;
            memcpy(audioBuffer.i8, buffer, toWrite);
//        }

        buffer = ((const char *) buffer) + toWrite;
        userSize -= toWrite;
        written += toWrite;

        releaseBufferOut(&audioBuffer);
    }

    return written;

    ALOGV("RoutingThread::write %p: %d written", this, written);

}

}

