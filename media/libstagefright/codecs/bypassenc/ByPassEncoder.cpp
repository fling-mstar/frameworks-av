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

//#define LOG_NDEBUG 0
#define LOG_TAG "ByPassEncoder"
#include <utils/Log.h>

#include "ByPassEncoder.h"
//#include "api_bypass_frmformat.h"
#include "OMX_Video.h"

#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/MediaBufferGroup.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/Utils.h>

#include "AitUVC.h"
namespace android {
//#define MST_BYPASS_ENC_DUMP

static int findIDRStartCode(
        const uint8_t *data, size_t length) {

    ALOGV("findNextStartCode: %p %d", data, length);

    int bytesLeft = length;
    bool find = false;
    while (bytesLeft > 4  && !find) {
        if (!memcmp("\x00\x00\x00\x01", &data[length - bytesLeft], 4) && (0x5 == (data[length - bytesLeft+4] & 0x1f))) {
            find = true;
        } else {
            --bytesLeft;
        }
    }
    if (!find) {
        return -1;
    } else {
        return length - bytesLeft;
    }
}

ByPassEncoder::ByPassEncoder(
        const sp<MediaSource>& source,
        const sp<MetaData>& meta)
    : mSource(source),
      mMeta(meta),
      mKeyFrameInterval(0),
      mNumInputFrames(-1),
      mNextModTimeUs(0),
      mPrevTimestampUs(-1),
      mStarted(false),
      mIsConfigCodec(false),
      mInputBuffer(NULL),
      mOutputBuffer(NULL),
      mSPSBuffer(NULL),
      mEncodedBuffer(NULL) {

    ALOGV("Construct software ByPassEncoder");

    mInitCheck = initCheck(meta); 
}

ByPassEncoder::~ByPassEncoder() {
    ALOGV("Destruct software ByPassEncoder");
    if (mStarted) {
        stop();
    }
    
    if (mSPSBuffer) {
        free(mSPSBuffer);
        mSPSBuffer = NULL;
    }

    if (mEncodedBuffer) {
        free(mEncodedBuffer);
        mEncodedBuffer = NULL;
    }
    
}

status_t ByPassEncoder::initCheck(const sp<MetaData>& meta) {
    ALOGV("initCheck");
    CHECK(meta->findInt32(kKeyWidth, &mVideoWidth));
    CHECK(meta->findInt32(kKeyHeight, &mVideoHeight));
    CHECK(meta->findInt32(kKeyFrameRate, &mVideoFrameRate));
    CHECK(meta->findInt32(kKeyBitRate, &mVideoBitRate));
    CHECK(meta->findInt32(kKeyIFramesInterval, &mKeyFrameInterval));
    mKeyFrameInterval = (mKeyFrameInterval <= 0 ? 1 : mKeyFrameInterval*mVideoFrameRate) - 1;

    CHECK(meta->findInt32(kKeyColorFormat, &mVideoColorFormat));
    if (mVideoColorFormat != OMX_COLOR_FormatEncodedAVC) {
            ALOGE("Color format %d is not supported", mVideoColorFormat);
            return BAD_VALUE;
    }

    // XXX: Remove this restriction
    if (mVideoWidth % 16 != 0 || mVideoHeight % 16 != 0) {
        ALOGE("Video frame size %dx%d must be a multiple of 16",
            mVideoWidth, mVideoHeight);
        return BAD_VALUE;
    }

    // Need to know which role the encoder is in.
    // XXX: Set the mode proper for other types of applications
    //      like streaming or video conference
    const char *mime;
    CHECK(meta->findCString(kKeyMIMEType, &mime));

    CHECK(!strcmp(mime, MEDIA_MIMETYPE_VIDEO_BYPASS));
    //CHECK(!strcmp(mime, MEDIA_MIMETYPE_VIDEO_MPEG4) ||
    //      !strcmp(mime, MEDIA_MIMETYPE_VIDEO_H263));

    mFormat = new MetaData;
    mFormat->setInt32(kKeyWidth, mVideoWidth);
    mFormat->setInt32(kKeyHeight, mVideoHeight);
    mFormat->setInt32(kKeyBitRate, mVideoBitRate);
    mFormat->setInt32(kKeySampleRate, mVideoFrameRate);
    mFormat->setInt32(kKeyColorFormat, mVideoColorFormat);

    //mFormat->setCString(kKeyMIMEType, mime);

    /* hard code temporarily, SubMIME should be obtained from VTProtocol */
    const char *subMime;
    CHECK(meta->findCString(kKeySubMIMEType, &subMime));
    mFormat->setCString(kKeyMIMEType, subMime);
    mFormat->setCString(kKeySubMIMEType, subMime);

    mFormat->setCString(kKeyDecoderComponent, "ByPassEncoder");
    //mFormat->setCString(kKeyDecoderComponent, "M4vH263Encoder");

    // SPS/PPS data for H.264
    nSPSDataSize = 256;  // 256 should be enough to contain all information
    mSPSBuffer = (uint8_t *)malloc(nSPSDataSize);
    CHECK(mSPSBuffer);

    // H.264 byte stream extracted from 3-rd party API
    nEncodedBufSize = mVideoWidth*mVideoHeight;
    mEncodedBuffer = (uint8_t *)malloc(nEncodedBufSize);  // encoded size should be less than the size
    CHECK(mEncodedBuffer);
    
    return OK;
}

status_t ByPassEncoder::start(MetaData *params) {
    ALOGV("start");
    if (mInitCheck != OK) {
        return mInitCheck;
    }

    if (mStarted) {
        ALOGW("Call start() when encoder already started");
        return OK;
    }

    mSource->start(params);
    mNumInputFrames = -1;  // 1st frame contains codec specific data
    mStarted = true;

    return OK;
}

status_t ByPassEncoder::stop() {
    ALOGV("stop");
    if (!mStarted) {
        ALOGW("Call stop() when encoder has not started");
        return OK;
    }

    mSource->stop();
    mStarted = false;

    return OK;
}

sp<MetaData> ByPassEncoder::getFormat() {
    ALOGE("getFormat");
    return mFormat;
}

status_t ByPassEncoder::read(
        MediaBuffer **out, const ReadOptions *options) {

    int             streamType = AIT_H264_IFRAME;
    uint32_t    h264StreamSize = 0;
    uint8_t                 *encodedBuffer;
    
#if defined(MST_BYPASS_ENC_DUMP)    
    FILE *file = NULL;
#endif

    if(!mOutputBuffer) {
        // Ready for accepting an input video frame
        if (OK != mSource->read(&mInputBuffer, options)) {
            ALOGE("Failed to read from data source");
            return OK;
        }

        // get MJPG_H264 byte bistream
        MS_Bypass_FrameInfo_t * frmInfo = (MS_Bypass_FrameInfo_t *) mInputBuffer->data();
        if(!memcmp(MSTR_BYPASS_ENC, &frmInfo->starcode, sizeof(frmInfo->starcode))) {
            streamType = AitH264Demuxer_MJPG_H264((unsigned char*)frmInfo->pBypassBuf, frmInfo->nByPassSize, 
                mEncodedBuffer, nEncodedBufSize, &h264StreamSize);   
            encodedBuffer = mEncodedBuffer;
        } else {
            streamType = frmInfo->nStreamType;
            encodedBuffer = frmInfo->pBypassBuf;
            h264StreamSize = frmInfo->nByPassSize;
       }
        // start code
        if(AIT_H264_DUMMY_FRAME == streamType) {
            ALOGW("waiting for incoming encoded camera video frames\n");
            *out = new MediaBuffer(encodedBuffer, 0);
        } else {
            uint8_t *bitstream = (uint8_t *)encodedBuffer;
            int64_t timestampUs = 0;
            mInputBuffer->meta_data()->findInt64(kKeyTime, &timestampUs);
                        
            *out = new MediaBuffer(encodedBuffer, h264StreamSize);
            if ((mNumInputFrames < 0) && !memcmp("\x00\x00\x00\x01\x67", bitstream, 5)) {
                int avcc_len = findIDRStartCode(bitstream, h264StreamSize);

                ALOGI("AVCC Len: %d\n", avcc_len);
                (*out)->meta_data()->setInt64(kKeyTime, 0);
                (*out)->meta_data()->setInt32(kKeyIsSyncFrame, false);
                (*out)->meta_data()->setInt32(kKeyIsCodecConfig, 1);
                (*out)->set_range(0, avcc_len);
                mIsConfigCodec = 1;
                ALOGI("config codec info\n");

                mOutputBuffer = new MediaBuffer((void *)(encodedBuffer+avcc_len), h264StreamSize-avcc_len);
                mOutputBuffer->meta_data()->setInt32(kKeyIsSyncFrame, true);
                mOutputBuffer->meta_data()->setInt64(kKeyTime, timestampUs);
                ALOGI("Extract I Frame in SPS/PPS stream: %x, size: %x\n", mOutputBuffer->data(), mOutputBuffer->size());

#if defined(MST_BYPASS_ENC_DUMP)
                file = fopen("/var/tmp/media/local_h264.bin", "wb");
#endif
            } else if (!memcmp("\x00\x00\x00\x01\x67", bitstream, 5)) {
                int avcc_len = findIDRStartCode(bitstream, h264StreamSize);
                ALOGV("I-Frame--AVCC Len: %d\n", avcc_len);
                
                (*out)->meta_data()->setInt32(kKeyIsSyncFrame, true);
                (*out)->meta_data()->setInt64(kKeyTime, timestampUs); 
                (*out)->set_range(avcc_len, h264StreamSize-avcc_len);

#if defined(MST_BYPASS_ENC_DUMP)                
                file = fopen("/var/tmp/media/local_h264.bin", "ab");
#endif                
            } else if (mIsConfigCodec) {
                (*out)->meta_data()->setInt32(kKeyIsSyncFrame, false);
                (*out)->meta_data()->setInt64(kKeyTime, timestampUs);
                
#if defined(MST_BYPASS_ENC_DUMP)                
                file = fopen("/var/tmp/media/local_h264.bin", "ab");
#endif                
            } else {
                mNumInputFrames--;
                ALOGI("SkypeXU_SetIFrame result: %x\n", SkypeXU_SetIFrame((SkypeXUHandle) frmInfo->mXuHandle));
                (*out)->set_range(0, 0);
            }
#if defined(MST_BYPASS_ENC_DUMP)
            if (file != NULL) {
                fwrite((unsigned char *)encodedBuffer, h264StreamSize, 1, file);
                fclose(file);
            }
#endif            
        }
        mNumInputFrames ++;
        if(1 == mNumInputFrames) {
                ALOGI("SkypeXU_SetFrameRate: %d, result: %x\n", mVideoFrameRate, SkypeXU_SetFrameRate((SkypeXUHandle) frmInfo->mXuHandle, mVideoFrameRate));
                ALOGI("SkypeXU_SetBitrate: %d, result: %x\n", mVideoBitRate/1000, SkypeXU_SetBitrate((SkypeXUHandle) frmInfo->mXuHandle, mVideoBitRate/1000));   
                ALOGI("AitXU_SetPFrameCount: %d, result: %x\n", mKeyFrameInterval, AitXU_SetPFrameCount(((SkypeXUHandle) frmInfo->mXuHandle)->aitxu, mKeyFrameInterval));    
        }
        mInputBuffer->release();
        mInputBuffer = NULL;           
    } else {
        *out = mOutputBuffer;
        ALOGE("config I frame\n");    
    }
    
    (*out)->setObserver(this);
    (*out)->add_ref();
    return OK;
}

void ByPassEncoder::signalBufferReturned(MediaBuffer *buffer) {
    if(buffer == mOutputBuffer) {
        ALOGW("mOutputBuffer is signalBufferReturned, clear\n");
        mOutputBuffer = NULL;
    }
}

}  // namespace android
