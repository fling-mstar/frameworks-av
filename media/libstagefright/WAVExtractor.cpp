/*
 * Copyright (C) 2009 The Android Open Source Project
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

//#define LOG_NDEBUG 0
#define LOG_TAG "WAVExtractor"
#include <utils/Log.h>

#include "include/WAVExtractor.h"

#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/DataSource.h>
#include <media/stagefright/MediaBufferGroup.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MediaSource.h>
#include <media/stagefright/MetaData.h>
#include <utils/String8.h>
#include <cutils/bitops.h>
// MStar Android Patch Begin
#include <OMX_Audio.h>
// MStar Android Patch End

#define CHANNEL_MASK_USE_CHANNEL_ORDER 0

namespace android {

enum {
    WAVE_FORMAT_PCM        = 0x0001,
    // MStar Android Patch Begin
    WAVE_FORMAT_M_ADPCM    = 0x0002,
    WAVE_FORMAT_IEEE_FLOAT = 0x0003,
    // MStar Android Patch End
    WAVE_FORMAT_ALAW       = 0x0006,
    WAVE_FORMAT_MULAW      = 0x0007,
    WAVE_FORMAT_MSGSM      = 0x0031,
    // MStar Android Patch Begin
    WAVE_FORMAT_IMA_ADPCM  = 0x0011,
    WAVE_FORMAT_MPEG       = 0x0050,
    WAVE_FORMAT_MPEGLAYER3 = 0x0055,
    // MStar Android Patch End

    WAVE_FORMAT_EXTENSIBLE = 0xFFFE
};

// MStar Android Patch Begin
#define DTS_MAX_FRAMESIZE 16384
#define DTS_READ_HEAD_SIZE 20
#define ALAW_ULAW_READ_SIZE 2048

//DTS sample rates table
const uint32_t u32DTSSampleRateTable[] =
{
    0, 8000, 16000, 32000, 0, 0, 11025, 22050, 44100, 0, 0,
    12000, 24000, 48000, 96000, 192000
};

//DTS bit rates table
const uint32_t u32DTSBitRateTable[] =
{
    32000, 56000, 64000, 96000, 112000, 128000,
    192000, 224000, 256000, 320000, 384000,
    448000, 512000, 576000, 640000, 768000,
    896000, 1024000, 1152000, 1280000, 1344000,
    1408000, 1411200, 1472000, 1536000, 1920000,
    2048000, 3072000, 3840000, 1/*open*/, 2/*variable*/, 3/*lossless*/
};

//DTS channel num table
const int32_t u32DTSChannelTable[]=
{
    1,2,2,2,
    2,3,3,4,
    4,5,6,6,
    6,7,8,8
};

static bool DTSRsync(uint8_t* pDtsBuffer,off64_t off64BufferSize,off64_t &off64BeginPosition,uint32_t &u32FrameSize,
                     uint32_t &u32SampleRateIndex,uint32_t &u32BitRateIndex,uint32_t &u32ChannelNum,bool &bAudioDTS)
{
    uint16_t u16DTSBitsPerSample = 0;
    bool bLittleEndian;
    uint16_t u16FrameHdr[4] = { 0 };

#define MAX_USE_SIZE 11

    if (off64BeginPosition+MAX_USE_SIZE > off64BufferSize) {
        return false;
    }
    //14bit sync header 0x1FFFE800+0x07F
    //little endian case
    if ((pDtsBuffer[off64BeginPosition] == 0xFF) && (pDtsBuffer[off64BeginPosition+1] == 0x1F) &&
        (pDtsBuffer[off64BeginPosition+2] == 0x00) && (pDtsBuffer[off64BeginPosition+3] == 0xE8) &&
         ((pDtsBuffer[off64BeginPosition+4]&0xF0) == 0xF0) && (pDtsBuffer[off64BeginPosition+5] == 0x07) ) {
        u16FrameHdr[0] = (uint16_t)(pDtsBuffer[off64BeginPosition+4] | pDtsBuffer[off64BeginPosition+5] << 8) & 0x3FFF;
        u16FrameHdr[1] = (uint16_t)(pDtsBuffer[off64BeginPosition+6] | pDtsBuffer[off64BeginPosition+7] << 8) & 0x3FFF;
        u16FrameHdr[2] = (uint16_t)(pDtsBuffer[off64BeginPosition+8] | pDtsBuffer[off64BeginPosition+9] << 8) & 0x3FFF;
        u16FrameHdr[3] = (uint16_t)(pDtsBuffer[off64BeginPosition+10] | pDtsBuffer[off64BeginPosition+11] << 8) & 0x3FFF;

        u16DTSBitsPerSample = 14;
        bLittleEndian = true;
    }
    //14bit sync header 0x1FFFE800+0x07F
    //big endian case
    else if ((pDtsBuffer[off64BeginPosition] == 0x1F) && (pDtsBuffer[off64BeginPosition+1] == 0xFF) &&
             (pDtsBuffer[off64BeginPosition+2] == 0xE8) && (pDtsBuffer[off64BeginPosition+3] == 0x00) &&
             (pDtsBuffer[off64BeginPosition+4] == 0x07) && ((pDtsBuffer[off64BeginPosition+5]&0xF0) == 0xF0) ) {
        u16FrameHdr[0] = (uint16_t)(pDtsBuffer[off64BeginPosition+4] << 8 | pDtsBuffer[off64BeginPosition+5]) & 0x3FFF;
        u16FrameHdr[1] = (uint16_t)(pDtsBuffer[off64BeginPosition+6] << 8 | pDtsBuffer[off64BeginPosition+7]) & 0x3FFF;
        u16FrameHdr[2] = (uint16_t)(pDtsBuffer[off64BeginPosition+8] << 8 | pDtsBuffer[off64BeginPosition+9]) & 0x3FFF;
        u16FrameHdr[3] = (uint16_t)(pDtsBuffer[off64BeginPosition+10] << 8 | pDtsBuffer[off64BeginPosition+11]) & 0x3FFF;

        u16DTSBitsPerSample = 14;
        bLittleEndian = false;

    }
    //16bit sync header 0x7FFE8001+0x3F
    //little endian case
    else if ((pDtsBuffer[off64BeginPosition] == 0xFE) && (pDtsBuffer[off64BeginPosition+1] == 0x7F) &&
             (pDtsBuffer[off64BeginPosition+2] == 0x01) && (pDtsBuffer[off64BeginPosition+3] == 0x80) && (pDtsBuffer[off64BeginPosition+5]>>2 == 0x3F) ) {
        u16FrameHdr[0] = (uint16_t)(pDtsBuffer[off64BeginPosition+4] | pDtsBuffer[off64BeginPosition+5] << 8);
        u16FrameHdr[1] = (uint16_t)(pDtsBuffer[off64BeginPosition+6] | pDtsBuffer[off64BeginPosition+7] << 8);
        u16FrameHdr[2] = (uint16_t)(pDtsBuffer[off64BeginPosition+8] | pDtsBuffer[off64BeginPosition+9] << 8);

        u16DTSBitsPerSample = 16;
        bLittleEndian = true;
    }
    //16bit sync header 0x7FFE8001+0x3F
    //big endian case
    else if ((pDtsBuffer[off64BeginPosition] == 0x7F) && (pDtsBuffer[off64BeginPosition+1] == 0xFE) &&
             (pDtsBuffer[off64BeginPosition+2] == 0x80) && (pDtsBuffer[off64BeginPosition+3] == 0x01) && (pDtsBuffer[off64BeginPosition+4]>>2 == 0x3F) ) {
        u16FrameHdr[0] = (uint16_t)(pDtsBuffer[off64BeginPosition+4] << 8 | pDtsBuffer[off64BeginPosition+5]);
        u16FrameHdr[1] = (uint16_t)(pDtsBuffer[off64BeginPosition+6] << 8 | pDtsBuffer[off64BeginPosition+7]);
        u16FrameHdr[2] = (uint16_t)(pDtsBuffer[off64BeginPosition+8] << 8 | pDtsBuffer[off64BeginPosition+9]);

        bLittleEndian = false;
        u16DTSBitsPerSample = 16;
    }

    if (14 == u16DTSBitsPerSample) {
        u32FrameSize = (uint32_t)((u16FrameHdr[1] & 0x3FF) << 4 |(u16FrameHdr[2] & 0x3C00) >> 10) + 1;
        u32ChannelNum = (uint32_t)((u16FrameHdr[2] & 0x3F0) >> 4) ;
        u32SampleRateIndex = (uint32_t)(u16FrameHdr[2] & 0xF);
        u32BitRateIndex = (uint32_t)((u16FrameHdr[3] & 0x3E00)>> 9);

        if (u32SampleRateIndex > 15 ) {
            ALOGV("Invalid DTS sample rate index:0x%lx\n",u32SampleRateIndex);
            return false;
        }
        if ( u32BitRateIndex > 31 ) {
            ALOGV("Invalid DTS bitrate index:0x%lx\n",u32SampleRateIndex);
            return false;
        }
        bAudioDTS = true;
        return true;
    }

    if (16 == u16DTSBitsPerSample) {
        u32FrameSize = (uint32_t)((u16FrameHdr[0] & 3) << 12 |(u16FrameHdr[1] & 0xFFF0) >> 4) + 1;
        u32ChannelNum = (uint32_t)((u16FrameHdr[1] & 0xF) << 2 | (u16FrameHdr[2] & 0xC000) >> 14);
        u32SampleRateIndex = (uint32_t)((u16FrameHdr[2] & 0x3C00)>> 10);
        u32BitRateIndex = (uint32_t)((u16FrameHdr[2] & 0x3E0)>> 5);

        if (u32SampleRateIndex > 15) {
            ALOGV("Invalid DTS sample rate index:0x%lx\n",u32SampleRateIndex);
            return false;
        }
        if (u32BitRateIndex > 31) {
            ALOGV("Invalid DTS bitrate index:0x%lx\n",u32SampleRateIndex);
            return false;
        }
        bAudioDTS = true;
        return true;
    }
    bAudioDTS = false;
    return false;
}

static bool FindSync(const sp<DataSource> &spDataSource,off64_t off64BeginPos,uint32_t &s32NextSync)
{
    uint32_t u32TmpFrameSize;
    uint32_t u32TmpSampleRateIndex,u32TmpBitRateIndex;
    bool bTmpAudioDTS,bDTSVBR,bTmpLittleEndian;
    uint16_t u16TmpDTSBitsPerSample;
    uint32_t u32ChannelNumIndex;
    uint8_t pTmpBuffer[DTS_MAX_FRAMESIZE];

    ssize_t s32ReadSize = spDataSource->readAt(off64BeginPos, pTmpBuffer, DTS_MAX_FRAMESIZE);
    if (0 > s32ReadSize) {
        return false;
    }

    for (ssize_t i=0;i<s32ReadSize;i++) {
        off64_t off64NextFrame = i;
        if (DTSRsync(pTmpBuffer,off64_t(s32ReadSize),off64NextFrame,u32TmpFrameSize,
                      u32TmpSampleRateIndex,u32TmpBitRateIndex,u32ChannelNumIndex,bTmpAudioDTS)) {
            s32NextSync = i;
            return true;
        }
    }
    return false;
}
// MStar Android Patch End

static const char* WAVEEXT_SUBFORMAT = "\x00\x00\x00\x00\x10\x00\x80\x00\x00\xAA\x00\x38\x9B\x71";


static uint32_t U32_LE_AT(const uint8_t *ptr) {
    return ptr[3] << 24 | ptr[2] << 16 | ptr[1] << 8 | ptr[0];
}

static uint16_t U16_LE_AT(const uint8_t *ptr) {
    return ptr[1] << 8 | ptr[0];
}

struct WAVSource : public MediaSource {
    // MStar Android Patch Begin
    WAVSource(
            const sp<DataSource> &dataSource,
            const sp<MetaData> &meta,
            uint16_t waveFormat,
            int32_t bitsPerSample,
            off64_t offset, size_t size,
            uint32_t u32FrameSize,
            uint32_t u32BlockSize);
    // MStar Android Patch End

    virtual status_t start(MetaData *params = NULL);
    virtual status_t stop();
    virtual sp<MetaData> getFormat();

    virtual status_t read(
            MediaBuffer **buffer, const ReadOptions *options = NULL);

protected:
    virtual ~WAVSource();

private:
    static const size_t kMaxFrameSize;

    sp<DataSource> mDataSource;
    sp<MetaData> mMeta;
    uint16_t mWaveFormat;
    int32_t mSampleRate;
    int32_t mNumChannels;
    int32_t mBitsPerSample;
    off64_t mOffset;
    size_t mSize;
    bool mStarted;
    MediaBufferGroup *mGroup;
    off64_t mCurrentPos;
    // MStar Android Patch Begin
    uint32_t m_u32FrameSize;
    uint32_t m_u32BlockSize;
    // MStar Android Patch End

    WAVSource(const WAVSource &);
    WAVSource &operator=(const WAVSource &);
};

WAVExtractor::WAVExtractor(const sp<DataSource> &source)
    : mDataSource(source),
      mValidFormat(false),
      mChannelMask(CHANNEL_MASK_USE_CHANNEL_ORDER) {
    mInitCheck = init();
}

WAVExtractor::~WAVExtractor() {
}

sp<MetaData> WAVExtractor::getMetaData() {
    sp<MetaData> meta = new MetaData;

    if (mInitCheck != OK) {
        return meta;
    }

    meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_CONTAINER_WAV);

    return meta;
}

size_t WAVExtractor::countTracks() {
    return mInitCheck == OK ? 1 : 0;
}

sp<MediaSource> WAVExtractor::getTrack(size_t index) {
    if (mInitCheck != OK || index > 0) {
        return NULL;
    }

    // MStar Android Patch Begin
    return new WAVSource(
            mDataSource, mTrackMeta,
            mWaveFormat, mBitsPerSample, mDataOffset, mDataSize, m_u32FrameSize, m_u32BlockSize);
    // MStar Android Patch End
}

sp<MetaData> WAVExtractor::getTrackMetaData(
        size_t index, uint32_t flags) {
    if (mInitCheck != OK || index > 0) {
        return NULL;
    }

    return mTrackMeta;
}

status_t WAVExtractor::init() {
    uint8_t header[12];
    if (mDataSource->readAt(
                0, header, sizeof(header)) < (ssize_t)sizeof(header)) {
        return NO_INIT;
    }

    if (memcmp(header, "RIFF", 4) || memcmp(&header[8], "WAVE", 4)) {
        return NO_INIT;
    }

    size_t totalSize = U32_LE_AT(&header[4]);

    off64_t offset = 12;
    size_t remainingSize = totalSize;
    while (remainingSize >= 8) {
        uint8_t chunkHeader[8];
        if (mDataSource->readAt(offset, chunkHeader, 8) < 8) {
            return NO_INIT;
        }

        remainingSize -= 8;
        offset += 8;

        uint32_t chunkSize = U32_LE_AT(&chunkHeader[4]);

        if (chunkSize > remainingSize) {
            return NO_INIT;
        }

        if (!memcmp(chunkHeader, "fmt ", 4)) {
            if (chunkSize < 16) {
                return NO_INIT;
            }

            uint8_t formatSpec[40];
            if (mDataSource->readAt(offset, formatSpec, 2) < 2) {
                return NO_INIT;
            }

            mWaveFormat = U16_LE_AT(formatSpec);
            // MStar Android Patch Begin
            if (mWaveFormat != WAVE_FORMAT_PCM
                && mWaveFormat != WAVE_FORMAT_ALAW
                && mWaveFormat != WAVE_FORMAT_MULAW
                && mWaveFormat != WAVE_FORMAT_MSGSM
                && mWaveFormat != WAVE_FORMAT_EXTENSIBLE
                && mWaveFormat != WAVE_FORMAT_M_ADPCM
                && mWaveFormat != WAVE_FORMAT_IMA_ADPCM) {
                return ERROR_UNSUPPORTED;
            }
            // MStar Android Patch End

            uint8_t fmtSize = 16;
            if (mWaveFormat == WAVE_FORMAT_EXTENSIBLE) {
                fmtSize = 40;
            }
            if (mDataSource->readAt(offset, formatSpec, fmtSize) < fmtSize) {
                return NO_INIT;
            }

            mNumChannels = U16_LE_AT(&formatSpec[2]);
            if (mWaveFormat != WAVE_FORMAT_EXTENSIBLE) {
                if (mNumChannels != 1 && mNumChannels != 2) {
                    ALOGW("More than 2 channels (%d) in non-WAVE_EXT, unknown channel mask",
                            mNumChannels);
                }
            } else {
                if (mNumChannels < 1 && mNumChannels > 8) {
                    return ERROR_UNSUPPORTED;
                }
            }

            mSampleRate = U32_LE_AT(&formatSpec[4]);

            if (mSampleRate == 0) {
                return ERROR_MALFORMED;
            }

            // MStar Android Patch Begin
            m_s32BitRate = U32_LE_AT(&formatSpec[8])*8;

            if (m_s32BitRate <= 0) {
                return ERROR_UNSUPPORTED;
            }

            m_u32BlockSize = U16_LE_AT(&formatSpec[12]);
            if ((0 == m_u32BlockSize) &&
                ((mWaveFormat == WAVE_FORMAT_M_ADPCM) ||
                  (mWaveFormat == WAVE_FORMAT_IMA_ADPCM))) {
                return ERROR_MALFORMED;
            }
            // MStar Android Patch End

            mBitsPerSample = U16_LE_AT(&formatSpec[14]);

            if (mWaveFormat == WAVE_FORMAT_PCM
                    || mWaveFormat == WAVE_FORMAT_EXTENSIBLE) {
                // MStar Android Patch Begin
                if (mBitsPerSample != 8 && mBitsPerSample != 16
                    && mBitsPerSample != 24 && mBitsPerSample != 32) {
                // MStar Android Patch End
                    return ERROR_UNSUPPORTED;
                }
            } else if (mWaveFormat == WAVE_FORMAT_MSGSM) {
                if (mBitsPerSample != 0) {
                    return ERROR_UNSUPPORTED;
                }
            // MStar Android Patch Begin
            } else if (mWaveFormat == WAVE_FORMAT_MULAW || mWaveFormat == WAVE_FORMAT_ALAW) {
                if (mBitsPerSample != 8) {
                    return ERROR_UNSUPPORTED;
                }
            }
            // MStar Android Patch End

            if (mWaveFormat == WAVE_FORMAT_EXTENSIBLE) {
                uint16_t validBitsPerSample = U16_LE_AT(&formatSpec[18]);
                if (validBitsPerSample != mBitsPerSample) {
                    if (validBitsPerSample != 0) {
                        ALOGE("validBits(%d) != bitsPerSample(%d) are not supported",
                                validBitsPerSample, mBitsPerSample);
                        return ERROR_UNSUPPORTED;
                    } else {
                        // we only support valitBitsPerSample == bitsPerSample but some WAV_EXT
                        // writers don't correctly set the valid bits value, and leave it at 0.
                        ALOGW("WAVE_EXT has 0 valid bits per sample, ignoring");
                    }
                }

                mChannelMask = U32_LE_AT(&formatSpec[20]);
                ALOGV("numChannels=%d channelMask=0x%x", mNumChannels, mChannelMask);
                if ((mChannelMask >> 18) != 0) {
                    ALOGE("invalid channel mask 0x%x", mChannelMask);
                    return ERROR_MALFORMED;
                }

                if ((mChannelMask != CHANNEL_MASK_USE_CHANNEL_ORDER)
                        && (popcount(mChannelMask) != mNumChannels)) {
                    ALOGE("invalid number of channels (%d) in channel mask (0x%x)",
                            popcount(mChannelMask), mChannelMask);
                    return ERROR_MALFORMED;
                }

                // In a WAVE_EXT header, the first two bytes of the GUID stored at byte 24 contain
                // the sample format, using the same definitions as a regular WAV header
                mWaveFormat = U16_LE_AT(&formatSpec[24]);
                if (mWaveFormat != WAVE_FORMAT_PCM
                        && mWaveFormat != WAVE_FORMAT_ALAW
                        && mWaveFormat != WAVE_FORMAT_MULAW) {
                    return ERROR_UNSUPPORTED;
                }
                if (memcmp(&formatSpec[26], WAVEEXT_SUBFORMAT, 14)) {
                    ALOGE("unsupported GUID");
                    return ERROR_UNSUPPORTED;
                }
            }

            mValidFormat = true;
        } else if (!memcmp(chunkHeader, "data", 4)) {
            if (mValidFormat) {
                mDataOffset = offset;
                mDataSize = chunkSize;

                mTrackMeta = new MetaData;

                switch (mWaveFormat) {
                    case WAVE_FORMAT_PCM:
                        mTrackMeta->setCString(
                                kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_RAW);
                        break;
                    case WAVE_FORMAT_ALAW:
                        mTrackMeta->setCString(
                                kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_G711_ALAW);
                        break;
                    case WAVE_FORMAT_MSGSM:
                        mTrackMeta->setCString(
                                kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_MSGSM);
                        break;

                    // MStar Android Patch Begin
                    case WAVE_FORMAT_M_ADPCM:
                    case WAVE_FORMAT_IMA_ADPCM:
                        mTrackMeta->setCString(
                                kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_ADPCM);
                        break;
                    case WAVE_FORMAT_MULAW:
                        mTrackMeta->setCString(
                                kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_G711_MLAW);
                        break;
                    default:
                        ALOGV("Unsupported");
                        return ERROR_UNSUPPORTED;
                    // MStar Android Patch End
                }

                mTrackMeta->setInt32(kKeyChannelCount, mNumChannels);
                mTrackMeta->setInt32(kKeyChannelMask, mChannelMask);
                mTrackMeta->setInt32(kKeySampleRate, mSampleRate);

                // MStar Android Patch Begin
                mTrackMeta->setInt32(kKeyBitRate, m_s32BitRate);

                int64_t durationUs;
                if (mWaveFormat == WAVE_FORMAT_PCM) {
                    durationUs = 8000000LL * mDataSize / (mNumChannels * mBitsPerSample) / mSampleRate;
                } else if (mWaveFormat == WAVE_FORMAT_MSGSM) {
                      // 65 bytes decode to 320 8kHz samples
                    durationUs = 1000000LL * (mDataSize / 65 * 320) / 8000;
                } else {
                    durationUs = 8000000LL * totalSize / m_s32BitRate;
                }

                mTrackMeta->setInt64(kKeyDuration, durationUs);

                //adpcm alaw mulaw add extra data
                if ((mWaveFormat == WAVE_FORMAT_M_ADPCM)   ||
                    (mWaveFormat ==WAVE_FORMAT_IMA_ADPCM) ||
                    (mWaveFormat ==WAVE_FORMAT_ALAW) ||
                    (mWaveFormat ==WAVE_FORMAT_MULAW)) {
                    OMX_AUDIO_PARAM_ADPCMTYPE stAdpcm;
                    stAdpcm.nBitsPerSample = mBitsPerSample;
                    stAdpcm.nBlockSize = m_u32BlockSize;
                    stAdpcm.nChannels = mNumChannels;
                    stAdpcm.nSamplePerBlock = m_u32BlockSize*8/mBitsPerSample;
                    stAdpcm.nSampleRate = mSampleRate;

                    if (mWaveFormat == WAVE_FORMAT_M_ADPCM) {
                        stAdpcm.eType = OMX_AUDIO_ADPCMTypeMS;
                    } else if (mWaveFormat ==WAVE_FORMAT_IMA_ADPCM) {
                        stAdpcm.eType = OMX_AUDIO_ADPCMTypeDVI;
                    } else if (mWaveFormat ==WAVE_FORMAT_ALAW) {
                        stAdpcm.eType =  OMX_AUDIO_ADPCMPTypeALAW;
                    } else {
                        stAdpcm.eType = OMX_AUDIO_ADPCMPTypeULAW;
                    }
                    mTrackMeta->setData(kKeyMsAdpcmProfile, 0, &stAdpcm, sizeof(OMX_AUDIO_PARAM_ADPCMTYPE));
                }

                //Begin to find the DTS synchronized head
                if (mWaveFormat == WAVE_FORMAT_PCM) {
                    uint32_t u32FrameSize;
                    uint32_t u32SampleRateIndex, u32BitRateIndex;
                    bool bAudioDTS = false;
                    uint32_t u32ChannelNumIndex;
                    off64_t off64dtsOffset = mDataOffset;

                    ssize_t s32ReadSize;
                    if (totalSize > 0x1000) {
                        s32ReadSize = 0x1000;
                    } else {
                        s32ReadSize = totalSize;
                    }

                    uint8_t pDtsBuffer[4096];
                    if (mDataSource->readAt(0, pDtsBuffer, s32ReadSize) < s32ReadSize) {
                        return NO_INIT;
                    }
                    remainingSize -= s32ReadSize;
                    while (1) {
                        if (off64dtsOffset >= (s32ReadSize-DTS_READ_HEAD_SIZE)) {
                            break;
                        }

                        if (DTSRsync(pDtsBuffer,(off64_t)s32ReadSize,off64dtsOffset,u32FrameSize,
                                      u32SampleRateIndex,u32BitRateIndex,u32ChannelNumIndex,bAudioDTS)) {
                            uint32_t u32TmpFrameSize;
                            uint32_t u32TmpBitRateIndex,u32TmpSampleRateIndex;
                            bool bTmpAudioDTS;
                            uint32_t u32ChannelNumIndex;
                            uint16_t u16TmpDTSBitsPerSample;
                            off64_t off64tmp = off64dtsOffset+u32FrameSize;
                            uint8_t au8NextSync[DTS_READ_HEAD_SIZE];

                            m_u32FrameSize = u32FrameSize;

                            if (sizeof(au8NextSync) !=  mDataSource->readAt(off64tmp,au8NextSync,sizeof(au8NextSync))) {
                                return ERROR_END_OF_STREAM;
                            }

                            off64_t off64NextSync = 0;
                            if (!DTSRsync(au8NextSync,off64_t(DTS_READ_HEAD_SIZE),off64NextSync,u32TmpFrameSize,
                                          u32TmpSampleRateIndex,u32TmpBitRateIndex,u32ChannelNumIndex,bTmpAudioDTS)) {
                                if (FindSync(mDataSource,off64dtsOffset+4,u32FrameSize)) {
                                    m_u32FrameSize = u32FrameSize+4;
                                }

                            }
                            break;
                        }
                        off64dtsOffset++;

                    }
                    //if find the DTS sync,it will check the correctness of  the frameSize.
                    //if not correct, find the next sync to compute the frameSize.
                    if (bAudioDTS) {
                        mDataOffset = off64dtsOffset;
                        mTrackMeta->setCString(
                                              kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_DTS);
                        uint32_t u32SamplesPerSec = u32DTSSampleRateTable[u32SampleRateIndex];
                        bool bDTSVBR;
                        if ((u32DTSBitRateTable[u32BitRateIndex] == 2) || (u32DTSBitRateTable[u32BitRateIndex] == 3)) {
                            bDTSVBR = true;
                        }
                        //ALOGD("u32DTSBitRateTable[%d] = %d",u32BitRateIndex,u32DTSBitRateTable[u32BitRateIndex]);
                        if ((u32DTSBitRateTable[u32BitRateIndex] != 1)&&(u32DTSBitRateTable[u32BitRateIndex] != 2)&&(u32DTSBitRateTable[u32BitRateIndex] != 3)) {
                            int32_t u32AvgBytesPerSec = u32DTSBitRateTable[u32BitRateIndex]>>3;
                            if (u32AvgBytesPerSec !=0) {
                                mTrackMeta->setInt32(kKeySampleRate,u32SamplesPerSec);
                                mTrackMeta->setInt64(kKeyDuration,8000000LL*mDataSize/u32DTSBitRateTable[u32BitRateIndex]);
                                mTrackMeta->setInt32(kKeyBitRate,u32DTSBitRateTable[u32BitRateIndex]);
                                if (u32ChannelNumIndex < 16) {
                                    mTrackMeta->setInt32(kKeyChannelCount,u32DTSChannelTable[u32ChannelNumIndex]);
                                }
                            }
                        }
                    }
                }
                // MStar Android Patch End

                return OK;
            }
        }

        offset += chunkSize;
    }

    return NO_INIT;
}

const size_t WAVSource::kMaxFrameSize = 32768;

// MStar Android Patch Begin
WAVSource::WAVSource(
        const sp<DataSource> &dataSource,
        const sp<MetaData> &meta,
        uint16_t waveFormat,
        int32_t bitsPerSample,
        off64_t offset,size_t size,
        uint32_t u32FrameSize,
        uint32_t u32BlockSize)
    : mDataSource(dataSource),
      mMeta(meta),
      mWaveFormat(waveFormat),
      mSampleRate(0),
      mNumChannels(0),
      mBitsPerSample(bitsPerSample),
      mOffset(offset),
      mSize(size),
      m_u32FrameSize(u32FrameSize),
      m_u32BlockSize(u32BlockSize),
      mStarted(false),
      mGroup(NULL) {
// MStar Android Patch End
    CHECK(mMeta->findInt32(kKeySampleRate, &mSampleRate));
    CHECK(mMeta->findInt32(kKeyChannelCount, &mNumChannels));

    mMeta->setInt32(kKeyMaxInputSize, kMaxFrameSize);
}

WAVSource::~WAVSource() {
    if (mStarted) {
        stop();
    }
}

// MStar Android Patch Begin
status_t WAVSource::start(MetaData *params) {
    ALOGV("WAVSource::start");

    if ( true == mStarted) {
        return OK;
    }

    mGroup = new MediaBufferGroup;
    const char *mime;
    if (!mMeta->findCString(kKeyMIMEType,&mime)) {
        return ERROR_MALFORMED;
    }

    if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_DTS)) {
        mGroup->add_buffer(new MediaBuffer(DTS_MAX_FRAMESIZE));

    } else {
        mGroup->add_buffer(new MediaBuffer(kMaxFrameSize*2)); // enlarge buffer to hold 24/32b PCM

        if (mBitsPerSample == 8) {
            // As a temporary buffer for 8->16 bit conversion.
            mGroup->add_buffer(new MediaBuffer(kMaxFrameSize));
        }
    }

    mCurrentPos = mOffset;

    mStarted = true;

    return OK;
}
// MStar Android Patch End

status_t WAVSource::stop() {
    ALOGV("WAVSource::stop");

    // MStar Android Patch Begin
    if (false == mStarted) {
        return OK;
    }
    // MStar Android Patch End

    delete mGroup;
    mGroup = NULL;

    mStarted = false;

    return OK;
}

sp<MetaData> WAVSource::getFormat() {
    ALOGV("WAVSource::getFormat");

    return mMeta;
}

// MStar Android Patch Begin
status_t WAVSource::read(
        MediaBuffer **out, const ReadOptions *options) {
    *out = NULL;

    int64_t seekTimeUs;
    ReadOptions::SeekMode mode;
    const char *mime;

    int32_t s32BitRate;
    if (!mMeta->findInt32(kKeyBitRate,&s32BitRate)) {
        return ERROR_END_OF_STREAM;
    }
    if (s32BitRate <= 0) {
        ALOGE("The BitRate %d error",s32BitRate);
        return ERROR_MALFORMED;
    }

    if (!mMeta->findCString(kKeyMIMEType,&mime)) {
        return ERROR_END_OF_STREAM;
    }
    if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_DTS)) {
        if ((options != NULL) && (options->getSeekTo(&seekTimeUs, &mode))) {
            mCurrentPos = seekTimeUs * s32BitRate / 8000000LL + mOffset;
            uint32_t s32offSize;
            if (FindSync(mDataSource, mCurrentPos,s32offSize)) {
                mCurrentPos += s32offSize;
            }
        }

        MediaBuffer *pBuffer;
        status_t stErr = mGroup->acquire_buffer(&pBuffer);
        if (stErr != OK) {
            return stErr;
        }

        ssize_t sizeN = mDataSource->readAt(mCurrentPos, pBuffer->data(),
                                            m_u32FrameSize);

        if ((sizeN<=0) || ((uint32_t)sizeN != m_u32FrameSize)) {
            pBuffer->release();
            pBuffer = NULL;

            return ERROR_END_OF_STREAM;
        }

        pBuffer->set_range(0, m_u32FrameSize);
        //ALOGD("CurTime =  %lld",8000000LL*(mCurrentPos-mOffset)/s32BitRate);
        //ALOGD("mFrameSize =  %d, mCurrentPos =  %lld",m_u32FrameSize,mCurrentPos);
        pBuffer->meta_data()->setInt64(kKeyTime, 8000000LL*(mCurrentPos-mOffset)/s32BitRate);
        mCurrentPos += m_u32FrameSize;
        pBuffer->meta_data()->setInt32(kKeyIsSyncFrame, 1);
        *out = pBuffer;
        return OK;
    }

    if (options != NULL && options->getSeekTo(&seekTimeUs, &mode)) {
        int64_t pos;
        if (mWaveFormat == WAVE_FORMAT_PCM) {
            pos = (seekTimeUs * mSampleRate) / 8000000 * mNumChannels * (mBitsPerSample );
        } else if (mWaveFormat == WAVE_FORMAT_MSGSM) {
             // 65 bytes decode to 320 8kHz samples
            int64_t samplenumber = (seekTimeUs * mSampleRate) / 1000000;
            int64_t framenumber = samplenumber / 320;
            pos = framenumber * 65;
        } else {
            pos = 8000000LL * mCurrentPos /s32BitRate;
            if ( (mWaveFormat == WAVE_FORMAT_M_ADPCM) ||
                 (mWaveFormat == WAVE_FORMAT_IMA_ADPCM) ||
                 (mWaveFormat == WAVE_FORMAT_MULAW) ||
                 (mWaveFormat == WAVE_FORMAT_ALAW)) {
                pos = ( pos / m_u32BlockSize ) * m_u32BlockSize;
            }
        }
        if (pos > mSize) {
            pos = mSize;
        }
        mCurrentPos = pos + mOffset;
    }

    MediaBuffer *buffer;
    status_t err = mGroup->acquire_buffer(&buffer);
    if (err != OK) {
        return err;
    }

    // make sure that maxBytesToRead is multiple of 3, in 24-bit case
    size_t maxBytesToRead =
        mBitsPerSample == 8 ? kMaxFrameSize / 2 : 
        (mBitsPerSample == 24 ? 3*(kMaxFrameSize/3): kMaxFrameSize);

    if (mBitsPerSample == 24) {
        maxBytesToRead = (kMaxFrameSize*3) / 2;
    }
    if (mBitsPerSample == 32) {
        maxBytesToRead =  kMaxFrameSize*2;
    }

    size_t maxBytesAvailable =
        (mCurrentPos - mOffset >= (off64_t)mSize)
            ? 0 : mSize - (mCurrentPos - mOffset);

    if (maxBytesToRead > maxBytesAvailable) {
        maxBytesToRead = maxBytesAvailable;
    }

    if (mWaveFormat == WAVE_FORMAT_MSGSM) {
        // Microsoft packs 2 frames into 65 bytes, rather than using separate 33-byte frames,
        // so read multiples of 65, and use smaller buffers to account for ~10:1 expansion ratio
        if (maxBytesToRead > 1024) {
            maxBytesToRead = 1024;
        }
        maxBytesToRead = (maxBytesToRead / 65) * 65;
    } else if ((mWaveFormat == WAVE_FORMAT_M_ADPCM) ||
        (mWaveFormat == WAVE_FORMAT_IMA_ADPCM)) {
        if (maxBytesToRead > (10 * m_u32BlockSize)) {
            maxBytesToRead = 10 * m_u32BlockSize;
        }
    } else if ((mWaveFormat == WAVE_FORMAT_MULAW) ||
               (mWaveFormat == WAVE_FORMAT_ALAW)) {
        if (maxBytesToRead > ALAW_ULAW_READ_SIZE) {
            maxBytesToRead = ALAW_ULAW_READ_SIZE;
        }
    }

    ssize_t n = mDataSource->readAt(
            mCurrentPos, buffer->data(),
            maxBytesToRead);

    if (n <= 0) {
        buffer->release();
        buffer = NULL;

        return ERROR_END_OF_STREAM;
    }

    buffer->set_range(0, n);

    if (mWaveFormat == WAVE_FORMAT_PCM || mWaveFormat == WAVE_FORMAT_EXTENSIBLE) {
        if (mBitsPerSample == 8) {
            // Convert 8-bit unsigned samples to 16-bit signed.

            MediaBuffer *tmp;
            CHECK_EQ(mGroup->acquire_buffer(&tmp), (status_t)OK);

            // The new buffer holds the sample number of samples, but each
            // one is 2 bytes wide.
            tmp->set_range(0, 2 * n);

            int16_t *dst = (int16_t *)tmp->data();
            const uint8_t *src = (const uint8_t *)buffer->data();
            ssize_t numBytes = n;

            while (numBytes-- > 0) {
                *dst++ = ((int16_t)(*src) - 128) * 256;
                ++src;
            }

            buffer->release();
            buffer = tmp;
        } else if (mBitsPerSample == 24) {
            // Convert 24-bit signed samples to 16-bit signed.

            const uint8_t *src =
                (const uint8_t *)buffer->data() + buffer->range_offset();
            int16_t *dst = (int16_t *)src;

            size_t numSamples = buffer->range_length() / 3;
            for (size_t i = 0; i < numSamples; ++i) {
                int32_t x = (int32_t)(src[0] | src[1] << 8 | src[2] << 16);
                x = (x << 8) >> 8;  // sign extension

                x = x >> 8;
                *dst++ = (int16_t)x;
                src += 3;
            }

            buffer->set_range(buffer->range_offset(), 2 * numSamples);
        }
        else if (mBitsPerSample == 32) {
            // Convert 32-bit signed samples to 16-bit signed.

            const uint8_t *src = (const uint8_t *)buffer->data() + buffer->range_offset();
            int16_t *dst = (int16_t *)src;

            size_t numSamples = buffer->range_length() / 4;
            for (size_t i = 0; i < numSamples; ++i) {
                int32_t x = (int32_t)(src[0] | src[1] << 8 | src[2] << 16 | src[3] << 24);
                x = x >> 16;
                *dst++ = (int16_t)x;
                src += 4;
            }

            buffer->set_range(buffer->range_offset(), 2 * numSamples);
        }
    }

    int64_t timeStampUs = 0;

    if (mWaveFormat == WAVE_FORMAT_MSGSM) {
        timeStampUs = 1000000LL * (mCurrentPos - mOffset) * 320 / 65 / mSampleRate;
    } else if (mWaveFormat == WAVE_FORMAT_PCM) {
        timeStampUs = 8000000LL * (mCurrentPos - mOffset) / (mNumChannels * mBitsPerSample) / mSampleRate;
    } else {
        timeStampUs = 8000000LL *  (mCurrentPos - mOffset) / s32BitRate;
    }

    buffer->meta_data()->setInt64(kKeyTime, timeStampUs);

    buffer->meta_data()->setInt32(kKeyIsSyncFrame, 1);
    mCurrentPos += n;

    *out = buffer;

    return OK;
}
// MStar Android Patch End

////////////////////////////////////////////////////////////////////////////////

bool SniffWAV(
        const sp<DataSource> &source, String8 *mimeType, float *confidence,
        sp<AMessage> *) {
    char header[12];
    if (source->readAt(0, header, sizeof(header)) < (ssize_t)sizeof(header)) {
        return false;
    }

    if (memcmp(header, "RIFF", 4) || memcmp(&header[8], "WAVE", 4)) {
        return false;
    }

    sp<MediaExtractor> extractor = new WAVExtractor(source);
    if (extractor->countTracks() == 0) {
        return false;
    }

    *mimeType = MEDIA_MIMETYPE_CONTAINER_WAV;
    *confidence = 0.3f;

    return true;
}

}  // namespace android
