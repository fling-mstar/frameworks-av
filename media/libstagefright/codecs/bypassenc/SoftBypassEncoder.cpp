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
#define LOG_TAG "SoftBypassEncoder"
#include <utils/Log.h>

#include "OMX_Video.h"

#include <HardwareAPI.h>
#include <MetadataBufferType.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/Utils.h>
#include <ui/Rect.h>
#include <ui/GraphicBufferMapper.h>

#include "SoftBypassEncoder.h"

namespace android {

template<class T>
static void InitOMXParams(T *params) {
    params->nSize = sizeof(T);
    // MStar Android Patch Begin
    params->nVersion.s.nVersionMajor = OMX_VERSION_MAJOR;
    params->nVersion.s.nVersionMinor = OMX_VERSION_MINOR;
    params->nVersion.s.nRevision = OMX_VERSION_REVISION;
    params->nVersion.s.nStep = OMX_VERSION_STEP;
    // MStar Android Patch End
}

SoftBypassEncoder::SoftBypassEncoder(
            const char *name,
            const OMX_CALLBACKTYPE *callbacks,
            OMX_PTR appData,
            OMX_COMPONENTTYPE **component)
    : SimpleSoftOMXComponent(name, callbacks, appData, component),
      mVideoWidth(1280),
      mVideoHeight(720),
      mVideoFrameRate(30),
      mVideoBitRate(192000),
      mVideoColorFormat(OMX_COLOR_FormatEncodedAVC),
      mStoreMetaDataInBuffers(false),
      mIDRFrameRefreshIntervalInSec(1),
      mIsPrependCSD(0),
      mNumInputFrames(-1),
      mPrevTimestampUs(-1),
      mStarted(false),
      mSawInputEOS(false),
      mSignalledError(false),
      mInputFrameData(NULL),
      mSliceGroup(NULL) {

    initPorts();
    ALOGI("Construct SoftBypassEncoder");
}

SoftBypassEncoder::~SoftBypassEncoder() {
    ALOGV("Destruct SoftBypassEncoder");
    releaseEncoder();
    List<BufferInfo *> &outQueue = getPortQueue(1);
    List<BufferInfo *> &inQueue = getPortQueue(0);
    CHECK(outQueue.empty());
    CHECK(inQueue.empty());
    BypassEncDeInitialize();
}

OMX_ERRORTYPE SoftBypassEncoder::initEncoder() {
    CHECK(!mStarted);

    mEncParams.width = mVideoWidth;
    mEncParams.height = mVideoHeight;
    mEncParams.bitrate = mVideoBitRate;
    mEncParams.frame_rate = mVideoFrameRate;
    mEncParams.key_interval = (mIDRFrameRefreshIntervalInSec <= 0 ? 1 : mIDRFrameRefreshIntervalInSec*mVideoFrameRate) - 1;

    mNumInputFrames = -1;  // 1st two buffers contain SPS and PPS
    mSpsPpsHeaderReceived = false;
    mReadyForNextFrame = true;
    mIsIDRFrame = false;
    mStarted = true;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE SoftBypassEncoder::releaseEncoder() {
    if (!mStarted) {
        return OMX_ErrorNone;
    }

    releaseOutputBuffers();

    delete mInputFrameData;
    mInputFrameData = NULL;

    delete mSliceGroup;
    mSliceGroup = NULL;

    mStarted = false;

    return OMX_ErrorNone;
}

void SoftBypassEncoder::releaseOutputBuffers() {
    for (size_t i = 0; i < mOutputBuffers.size(); ++i) {
        MediaBuffer *buffer = mOutputBuffers.editItemAt(i);
        buffer->setObserver(NULL);
        buffer->release();
    }
    mOutputBuffers.clear();
}

void SoftBypassEncoder::initPorts() {
    OMX_PARAM_PORTDEFINITIONTYPE def;
    InitOMXParams(&def);

    const size_t kInputBufferSize = 1024;

    const size_t kOutputBufferSize = 1280*720;

    def.nPortIndex = 0;
    def.eDir = OMX_DirInput;
    def.nBufferCountMin = kNumBuffers;
    def.nBufferCountActual = def.nBufferCountMin;
    def.nBufferSize = kInputBufferSize;
    def.bEnabled = OMX_TRUE;
    def.bPopulated = OMX_FALSE;
    def.eDomain = OMX_PortDomainVideo;
    def.bBuffersContiguous = OMX_FALSE;
    def.nBufferAlignment = 1;

    def.format.video.cMIMEType = const_cast<char *>("bypass/video");
    def.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
    def.format.video.eColorFormat = OMX_COLOR_FormatEncodedAVC;
    def.format.video.xFramerate = (mVideoFrameRate << 16);  // Q16 format
    def.format.video.nBitrate = mVideoBitRate;
    def.format.video.nFrameWidth = mVideoWidth;
    def.format.video.nFrameHeight = mVideoHeight;
    def.format.video.nStride = mVideoWidth;
    def.format.video.nSliceHeight = mVideoHeight;

    addPort(def);

    def.nPortIndex = 1;
    def.eDir = OMX_DirOutput;
    def.nBufferCountMin = kNumBuffers;
    def.nBufferCountActual = def.nBufferCountMin;
    def.nBufferSize = kOutputBufferSize;
    def.bEnabled = OMX_TRUE;
    def.bPopulated = OMX_FALSE;
    def.eDomain = OMX_PortDomainVideo;
    def.bBuffersContiguous = OMX_FALSE;
    def.nBufferAlignment = 2;

    def.format.video.cMIMEType = const_cast<char *>("video/avc");
    def.format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;
    def.format.video.eColorFormat = OMX_COLOR_FormatUnused;
    def.format.video.xFramerate = (0 << 16);  // Q16 format
    def.format.video.nBitrate = mVideoBitRate;
    def.format.video.nFrameWidth = mVideoWidth;
    def.format.video.nFrameHeight = mVideoHeight;
    def.format.video.nStride = mVideoWidth;
    def.format.video.nSliceHeight = mVideoHeight;

    addPort(def);
}

OMX_ERRORTYPE SoftBypassEncoder::internalGetParameter(
        OMX_INDEXTYPE index, OMX_PTR params) {

    switch (index) {
        case OMX_IndexParamVideoErrorCorrection:
        {
            return OMX_ErrorNotImplemented;
        }

        case OMX_IndexParamVideoBitrate:
        {
            OMX_VIDEO_PARAM_BITRATETYPE *bitRate =
                (OMX_VIDEO_PARAM_BITRATETYPE *) params;

            if (bitRate->nPortIndex != 1) {
                return OMX_ErrorUndefined;
            }

            bitRate->eControlRate = OMX_Video_ControlRateVariable;
            bitRate->nTargetBitrate = mVideoBitRate;
            return OMX_ErrorNone;
        }

        case OMX_IndexParamVideoPortFormat:
        {
            OMX_VIDEO_PARAM_PORTFORMATTYPE *formatParams =
                (OMX_VIDEO_PARAM_PORTFORMATTYPE *)params;

            if (formatParams->nPortIndex > 1) {
                return OMX_ErrorUndefined;
            }

            if (formatParams->nIndex > 2) {
                return OMX_ErrorNoMore;
            }

            if (formatParams->nPortIndex == 0) {
                formatParams->eCompressionFormat = OMX_VIDEO_CodingUnused;
                if (formatParams->nIndex == 0) {
                    formatParams->eColorFormat = OMX_COLOR_FormatEncodedAVC;
                } else {
                    formatParams->eColorFormat = OMX_COLOR_FormatEncodedAVC;
                }
            } else {
                formatParams->eCompressionFormat = OMX_VIDEO_CodingAVC;
                formatParams->eColorFormat = OMX_COLOR_FormatUnused;
            }

            return OMX_ErrorNone;
        }

        case OMX_IndexParamVideoAvc:
        {
            OMX_VIDEO_PARAM_AVCTYPE *avcParams =
                (OMX_VIDEO_PARAM_AVCTYPE *)params;

            if (avcParams->nPortIndex != 1) {
                return OMX_ErrorUndefined;
            }

            avcParams->eProfile = OMX_VIDEO_AVCProfileBaseline;
            avcParams->eLevel = OMX_VIDEO_AVCLevel2;
            avcParams->nRefFrames = 1;
            avcParams->nBFrames = 0;
            avcParams->bUseHadamard = OMX_TRUE;
            avcParams->nAllowedPictureTypes =
                    (OMX_VIDEO_PictureTypeI | OMX_VIDEO_PictureTypeP);
            avcParams->nRefIdx10ActiveMinus1 = 0;
            avcParams->nRefIdx11ActiveMinus1 = 0;
            avcParams->bWeightedPPrediction = OMX_FALSE;
            avcParams->bEntropyCodingCABAC = OMX_FALSE;
            avcParams->bconstIpred = OMX_FALSE;
            avcParams->bDirect8x8Inference = OMX_FALSE;
            avcParams->bDirectSpatialTemporal = OMX_FALSE;
            avcParams->nCabacInitIdc = 0;
            return OMX_ErrorNone;
        }

        case OMX_IndexParamVideoProfileLevelQuerySupported:
        {
            OMX_VIDEO_PARAM_PROFILELEVELTYPE *profileLevel =
                (OMX_VIDEO_PARAM_PROFILELEVELTYPE *)params;

            if (profileLevel->nPortIndex != 1) {
                return OMX_ErrorUndefined;
            }

            if (profileLevel->nProfileIndex > 0) {
                return OMX_ErrorNoMore;
            }

            profileLevel->eProfile = OMX_VIDEO_AVCProfileBaseline;
            profileLevel->eLevel = OMX_VIDEO_AVCLevel2;

            return OMX_ErrorNone;
        }

        default:
            return SimpleSoftOMXComponent::internalGetParameter(index, params);
    }
}

OMX_ERRORTYPE SoftBypassEncoder::internalSetParameter(
        OMX_INDEXTYPE index, const OMX_PTR params) {
    int32_t indexFull = index;

    switch (indexFull) {
        case OMX_IndexParamVideoErrorCorrection:
        {
            return OMX_ErrorNotImplemented;
        }

        case OMX_IndexParamVideoBitrate:
        {
            OMX_VIDEO_PARAM_BITRATETYPE *bitRate =
                (OMX_VIDEO_PARAM_BITRATETYPE *) params;

            if (bitRate->nPortIndex != 1 ||
                bitRate->eControlRate != OMX_Video_ControlRateVariable) {
                return OMX_ErrorUndefined;
            }

            mVideoBitRate = bitRate->nTargetBitrate;
            return OMX_ErrorNone;
        }

        case OMX_IndexParamPortDefinition:
        {
            OMX_PARAM_PORTDEFINITIONTYPE *def =
                (OMX_PARAM_PORTDEFINITIONTYPE *)params;
            if (def->nPortIndex > 1) {
                return OMX_ErrorUndefined;
            }

            if (def->nPortIndex == 0) {
                if (def->format.video.eCompressionFormat != OMX_VIDEO_CodingUnused ||
                    (def->format.video.eColorFormat != OMX_COLOR_FormatEncodedAVC)) {
                    return OMX_ErrorUndefined;
                }
            } else {
                if (def->format.video.eCompressionFormat != OMX_VIDEO_CodingAVC ||
                    (def->format.video.eColorFormat != OMX_COLOR_FormatUnused)) {
                    return OMX_ErrorUndefined;
                }
            }

            OMX_ERRORTYPE err = SimpleSoftOMXComponent::internalSetParameter(index, params);
            if (OMX_ErrorNone != err) {
                return err;
            }

            if (def->nPortIndex == 0) {
                mVideoWidth = def->format.video.nFrameWidth;
                mVideoHeight = def->format.video.nFrameHeight;
                mVideoFrameRate = def->format.video.xFramerate >> 16;
                mVideoColorFormat = def->format.video.eColorFormat;
            } else {
                mVideoBitRate = def->format.video.nBitrate;
            }

            return OMX_ErrorNone;
        }

        case OMX_IndexParamStandardComponentRole:
        {
            const OMX_PARAM_COMPONENTROLETYPE *roleParams =
                (const OMX_PARAM_COMPONENTROLETYPE *)params;

            if (strncmp((const char *)roleParams->cRole,
                        "video_encoder.bypass",
                        OMX_MAX_STRINGNAME_SIZE - 1)) {
                return OMX_ErrorUndefined;
            }

            return OMX_ErrorNone;
        }

        case OMX_IndexParamVideoPortFormat:
        {
            const OMX_VIDEO_PARAM_PORTFORMATTYPE *formatParams =
                (const OMX_VIDEO_PARAM_PORTFORMATTYPE *)params;

            if (formatParams->nPortIndex > 1) {
                return OMX_ErrorUndefined;
            }

            if (formatParams->nIndex > 2) {
                return OMX_ErrorNoMore;
            }

            if (formatParams->nPortIndex == 0) {
                if (formatParams->eCompressionFormat != OMX_VIDEO_CodingUnused ||
                    ((formatParams->nIndex == 0 &&
                      formatParams->eColorFormat != OMX_COLOR_FormatEncodedAVC) ||
                    (formatParams->nIndex == 1 &&
                     formatParams->eColorFormat != OMX_COLOR_FormatEncodedAVC) ||
                    (formatParams->nIndex == 2 &&
                     formatParams->eColorFormat != OMX_COLOR_FormatAndroidOpaque) )) {
                    return OMX_ErrorUndefined;
                }
                mVideoColorFormat = formatParams->eColorFormat;
            } else {
                if (formatParams->eCompressionFormat != OMX_VIDEO_CodingAVC ||
                    formatParams->eColorFormat != OMX_COLOR_FormatUnused) {
                    return OMX_ErrorUndefined;
                }
            }

            return OMX_ErrorNone;
        }

        case OMX_IndexParamVideoAvc:
        {
            OMX_VIDEO_PARAM_AVCTYPE *avcType =
                (OMX_VIDEO_PARAM_AVCTYPE *)params;

            if (avcType->nPortIndex != 1) {
                return OMX_ErrorUndefined;
            }

            // PV's AVC encoder only supports baseline profile
            if (avcType->eProfile != OMX_VIDEO_AVCProfileBaseline ||
                avcType->nRefFrames != 1 ||
                avcType->nBFrames != 0 ||
                avcType->bUseHadamard != OMX_TRUE ||
                (avcType->nAllowedPictureTypes & OMX_VIDEO_PictureTypeB) != 0 ||
                avcType->nRefIdx10ActiveMinus1 != 0 ||
                avcType->nRefIdx11ActiveMinus1 != 0 ||
                avcType->bWeightedPPrediction != OMX_FALSE ||
                avcType->bEntropyCodingCABAC != OMX_FALSE ||
                avcType->bconstIpred != OMX_FALSE ||
                avcType->bDirect8x8Inference != OMX_FALSE ||
                avcType->bDirectSpatialTemporal != OMX_FALSE ||
                avcType->nCabacInitIdc != 0) {
                return OMX_ErrorUndefined;
            }

            return OMX_ErrorNone;
        }

        case kStoreMetaDataExtensionIndex:
        {
            StoreMetaDataInBuffersParams *storeParams =
                    (StoreMetaDataInBuffersParams*)params;
            if (storeParams->nPortIndex != 0) {
                ALOGE("%s: StoreMetadataInBuffersParams.nPortIndex not zero!",
                        __FUNCTION__);
                return OMX_ErrorUndefined;
            }

            mStoreMetaDataInBuffers = storeParams->bStoreMetaData;
            ALOGV("StoreMetaDataInBuffers set to: %s",
                    mStoreMetaDataInBuffers ? " true" : "false");

            if (mStoreMetaDataInBuffers) {
                mVideoColorFormat == OMX_COLOR_FormatYUV420SemiPlanar;
                if (mInputFrameData == NULL) {
                    mInputFrameData =
                            (uint8_t *) malloc((mVideoWidth * mVideoHeight * 3 ) >> 1);
                }
            }

            return OMX_ErrorNone;
        }

        default:
            return SimpleSoftOMXComponent::internalSetParameter(index, params);
    }
}

void SoftBypassEncoder::onQueueFilled(OMX_U32 portIndex) {
    if (mSignalledError || mSawInputEOS) {
        return;
    }

    if (!mStarted) {
        if (OMX_ErrorNone != initEncoder()) {
            return;
        }
    }

    List<BufferInfo *> &inQueue = getPortQueue(0);
    List<BufferInfo *> &outQueue = getPortQueue(1);

    while (!mSawInputEOS && !inQueue.empty() && !outQueue.empty()) {
        BufferInfo *inInfo = *inQueue.begin();
        OMX_BUFFERHEADERTYPE *inHeader = inInfo->mHeader;
        BufferInfo *outInfo = *outQueue.begin();
        OMX_BUFFERHEADERTYPE *outHeader = outInfo->mHeader;

        outHeader->nTimeStamp = 0;
        outHeader->nFlags = 0;
        outHeader->nOffset = 0;
        outHeader->nFilledLen = 0;
        outHeader->nOffset = 0;

        uint8_t *outPtr = (uint8_t *) outHeader->pBuffer;
        uint32_t dataLength = outHeader->nAllocLen;
        MS_Bypass_FrameInfo_t * frmInfo = (MS_Bypass_FrameInfo_t *) inInfo->mHeader->pBuffer;

        if (!mSpsPpsHeaderReceived && mNumInputFrames < 0) {
            // get MJPG_H264 byte bistream
            BypassEncInitialize(frmInfo, &mEncParams, NULL, NULL);
        }

        int32_t type = BYPASSENC_FRAMETYPE_DUMMY;
        uint32_t sps_len = 0;    // used to get data for SPS
        BypassEnc_Status encoderStatus = BYPASSENC_SUCCESS;

        encoderStatus = BypassEncodeNAL(frmInfo, outPtr, &dataLength, &type);
        // Combine SPS and PPS and place them in the very first output buffer
        // SPS and PPS are separated by start code 0x00000001
        // Assume that we have exactly one SPS and exactly one PPS.
        if (!mSpsPpsHeaderReceived && mNumInputFrames <= 0) {
            switch (type) {
                case BYPASSENC_FRAMETYPE_I:
                    ++mNumInputFrames;
                    outHeader->nFlags = OMX_BUFFERFLAG_CODECCONFIG;
                    mSpsPpsHeaderReceived = true;
                    BypassEncConfigGet(outPtr, dataLength, &sps_len);
                    outHeader->nFilledLen = sps_len;
                    outQueue.erase(outQueue.begin());
                    outInfo->mOwnedByUs = false;
                    notifyFillBufferDone(outHeader);
                    return;
                default:
                    ALOGE("get next frame");
                    inQueue.erase(inQueue.begin());
                    inInfo->mOwnedByUs = false;
                    notifyEmptyBufferDone(inHeader);
                    break;
            }
            if(!mSpsPpsHeaderReceived)
                continue;
        }

        // Save the input buffer info so that it can be
        // passed to an output buffer
        InputBufferInfo info;
        info.mTimeUs = inHeader->nTimeStamp;
        info.mFlags = inHeader->nFlags;
        mInputBufferInfoVec.push(info);
        mPrevTimestampUs = inHeader->nTimeStamp;
        if ((BYPASSENC_FRAMETYPE_I == type) && mIsPrependCSD) {
            // prepend CSD for every I frame
            outHeader->nFlags |= OMX_BUFFERFLAG_SYNCFRAME;
        }else if (BYPASSENC_FRAMETYPE_I == type) {
            BypassEncConfigGet(outPtr, dataLength, &sps_len);
            outHeader->nFlags |= OMX_BUFFERFLAG_SYNCFRAME;
            dataLength -= sps_len;
            memcpy(outPtr, outPtr+sps_len, dataLength);
        }
        if (inHeader->nFlags & OMX_BUFFERFLAG_EOS) {
            mSawInputEOS = true;
        }

        inQueue.erase(inQueue.begin());
        inInfo->mOwnedByUs = false;
        //releaseGrallocData(srcBuffer);
        notifyEmptyBufferDone(inHeader);

        outQueue.erase(outQueue.begin());
        CHECK(!mInputBufferInfoVec.empty());
        InputBufferInfo *inputBufInfo = mInputBufferInfoVec.begin();
        outHeader->nTimeStamp = inputBufInfo->mTimeUs;
        outHeader->nFlags |= (inputBufInfo->mFlags | OMX_BUFFERFLAG_ENDOFFRAME);
        if (mSawInputEOS) {
            outHeader->nFlags |= OMX_BUFFERFLAG_EOS;
        }
        outHeader->nFilledLen = dataLength;
        outInfo->mOwnedByUs = false;
        notifyFillBufferDone(outHeader);
        mInputBufferInfoVec.erase(mInputBufferInfoVec.begin());
    }
}

void SoftBypassEncoder::signalBufferReturned(MediaBuffer *buffer) {
    ALOGV("signalBufferReturned: %p", buffer);
}

OMX_ERRORTYPE SoftBypassEncoder::setConfig(
        OMX_INDEXTYPE index, const OMX_PTR params) {

    OMX_ERRORTYPE          ret = OMX_ErrorNone;

    switch(index) {
        case OMX_IndexConfigVideoBitrate:
        {
            OMX_VIDEO_CONFIG_BITRATETYPE* pVideoConfigBitrate = (OMX_VIDEO_CONFIG_BITRATETYPE *)params;
            OMX_U32                       portIndex = pVideoConfigBitrate->nPortIndex;

            if ((portIndex != 1)) {
                ret = OMX_ErrorBadPortIndex;
                ALOGE("[%] invalid port index: %d", index, portIndex);
            } else{
                BypassEncSettingsUpdate(SETTING_UPDATE_BITRATE, pVideoConfigBitrate->nEncodeBitrate);
            }
            ret = OMX_ErrorNone;
        }
            break;

        case OMX_IndexConfigVideoFramerate:
        {
            OMX_CONFIG_FRAMERATETYPE* pVideoFrameRate = (OMX_CONFIG_FRAMERATETYPE *)params;
            OMX_U32                       portIndex = pVideoFrameRate->nPortIndex;

            if ((portIndex != 1)) {
                ret = OMX_ErrorBadPortIndex;
                ALOGE("[%x] invalid port index: %d", index, portIndex);
            } else{
                BypassEncSettingsUpdate(SETTING_UPDATE_FRAMERATE, pVideoFrameRate->xEncodeFramerate);
            }
            ret = OMX_ErrorNone;
        }
            break;

        case OMX_IndexConfigVideoIntraVOPRefresh:
        {
            OMX_CONFIG_INTRAREFRESHVOPTYPE* pVideoIVOPRefresh = (OMX_CONFIG_INTRAREFRESHVOPTYPE *)params;
            OMX_U32                       portIndex = pVideoIVOPRefresh->nPortIndex;

            if ((portIndex != 1)) {
                ret = OMX_ErrorBadPortIndex;
                ALOGE("[%x] invalid port index: %d", index, portIndex);
            } else if (pVideoIVOPRefresh->IntraRefreshVOP){
                BypassEncSettingsUpdate(SETTING_UPDATE_INTRA, 0);
            }
            ret = OMX_ErrorNone;
        }
            break;

        case OMX_IndexMstarPrependCSD:
        {
            mIsPrependCSD = *((OMX_U32*)params);

            ALOGE("OMX_IndexMstarPrependCSD? %x", mIsPrependCSD);
            ret = OMX_ErrorNone;
        }
            break;

        default:
            ret = SoftOMXComponent::setConfig(index, params);
            break;
    }
    return ret;
}
OMX_ERRORTYPE SoftBypassEncoder::getExtensionIndex(
        const char *name, OMX_INDEXTYPE *index) {
    if (!strcmp(name, "OMX.google.android.index.storeMetaDataInBuffers")) {
        *(int32_t*)index = kStoreMetaDataExtensionIndex;
        return OMX_ErrorNone;
    }
    return OMX_ErrorUndefined;
}

uint8_t *SoftBypassEncoder::extractGrallocData(void *data, buffer_handle_t *buffer) {
    OMX_U32 type = *(OMX_U32*)data;
    status_t res;
    if (type != kMetadataBufferTypeGrallocSource) {
        ALOGE("Data passed in with metadata mode does not have type "
                "kMetadataBufferTypeGrallocSource (%d), has type %ld instead",
                kMetadataBufferTypeGrallocSource, type);
        return NULL;
    }
    buffer_handle_t imgBuffer = *(buffer_handle_t*)((uint8_t*)data + 4);

    const Rect rect(mVideoWidth, mVideoHeight);
    uint8_t *img;
    res = GraphicBufferMapper::get().lock(imgBuffer,
            GRALLOC_USAGE_HW_VIDEO_ENCODER,
            rect, (void**)&img);
    if (res != OK) {
        ALOGE("%s: Unable to lock image buffer %p for access", __FUNCTION__,
                imgBuffer);
        return NULL;
    }

    *buffer = imgBuffer;
    return img;
}

void SoftBypassEncoder::releaseGrallocData(buffer_handle_t buffer) {
    if (mStoreMetaDataInBuffers) {
        GraphicBufferMapper::get().unlock(buffer);
    }
}

}  // namespace android

android::SoftOMXComponent *createSoftOMXComponent(
        const char *name, const OMX_CALLBACKTYPE *callbacks,
        OMX_PTR appData, OMX_COMPONENTTYPE **component) {
    return new android::SoftBypassEncoder(name, callbacks, appData, component);
}
