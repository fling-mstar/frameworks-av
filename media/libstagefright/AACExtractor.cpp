/*
 * Copyright (C) 2011 The Android Open Source Project
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
#define LOG_TAG "AACExtractor"
#include <utils/Log.h>

// MStar Android Patch Begin
#include "include/ID3.h"
// MStar Android Patch End
#include "include/AACExtractor.h"
#include "include/avc_utils.h"

#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/DataSource.h>
#include <media/stagefright/MediaBufferGroup.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MediaSource.h>
#include <media/stagefright/MetaData.h>
#include <utils/String8.h>

namespace android {

class AACSource : public MediaSource {
public:
    AACSource(const sp<DataSource> &source,
              const sp<MetaData> &meta,
              const Vector<uint64_t> &offset_vector,
              int64_t frame_duration_us);

    virtual status_t start(MetaData *params = NULL);
    virtual status_t stop();

    virtual sp<MetaData> getFormat();

    virtual status_t read(
            MediaBuffer **buffer, const ReadOptions *options = NULL);

protected:
    virtual ~AACSource();

private:
    static const size_t kMaxFrameSize;
    sp<DataSource> mDataSource;
    sp<MetaData> mMeta;

    off64_t mOffset;
    int64_t mCurrentTimeUs;
    bool mStarted;
    MediaBufferGroup *mGroup;

    Vector<uint64_t> mOffsetVector;
    int64_t mFrameDurationUs;

    AACSource(const AACSource &);
    AACSource &operator=(const AACSource &);
};

////////////////////////////////////////////////////////////////////////////////

// Returns the sample rate based on the sampling frequency index
uint32_t get_sample_rate(const uint8_t sf_index)
{
    static const uint32_t sample_rates[] =
    {
        96000, 88200, 64000, 48000, 44100, 32000,
        24000, 22050, 16000, 12000, 11025, 8000
    };

    if (sf_index < sizeof(sample_rates) / sizeof(sample_rates[0])) {
        return sample_rates[sf_index];
    }

    return 0;
}

// Returns the frame length in bytes as described in an ADTS header starting at the given offset,
//     or 0 if the size can't be read due to an error in the header or a read failure.
// The returned value is the AAC frame size with the ADTS header length (regardless of
//     the presence of the CRC).
// If headerSize is non-NULL, it will be used to return the size of the header of this ADTS frame.
static size_t getAdtsFrameLength(const sp<DataSource> &source, off64_t offset, size_t* headerSize) {

    const size_t kAdtsHeaderLengthNoCrc = 7;
    const size_t kAdtsHeaderLengthWithCrc = 9;

    size_t frameSize = 0;

    uint8_t syncword[2];
    if (source->readAt(offset, &syncword, 2) != 2) {
        return 0;
    }
    if ((syncword[0] != 0xff) || ((syncword[1] & 0xf6) != 0xf0)) {
        return 0;
    }

    uint8_t protectionAbsent;
    if (source->readAt(offset + 1, &protectionAbsent, 1) < 1) {
        return 0;
    }
    protectionAbsent &= 0x1;

    uint8_t header[3];
    if (source->readAt(offset + 3, &header, 3) < 3) {
        return 0;
    }

    frameSize = (header[0] & 0x3) << 11 | header[1] << 3 | header[2] >> 5;

    // protectionAbsent is 0 if there is CRC
    size_t headSize = protectionAbsent ? kAdtsHeaderLengthNoCrc : kAdtsHeaderLengthWithCrc;
    if (headSize > frameSize) {
        return 0;
    }
    if (headerSize != NULL) {
        *headerSize = headSize;
    }

    return frameSize;
}

// MStar Android Patch Begin
static uint32_t SkipIDTag (
        const sp<DataSource> &source) {
    uint8_t header[10];
    uint32_t u32ID3v2Size = 0;

    if (source->readAt(0, &header, 3) != 3) {
        return 0;
    }

    if ((header[0] == 'I') && (header[1] == 'D') && (header[2] == '3')) {
        if (source->readAt(6, &header, 4) != 4) {
            return 0;
        }
        u32ID3v2Size = (uint32_t)(header[0] & 0x7F);
        u32ID3v2Size <<= 7;
        u32ID3v2Size |= (uint32_t)(header[1] & 0x7F);
        u32ID3v2Size <<= 7;
        u32ID3v2Size |= (uint32_t)(header[2] & 0x7F);
        u32ID3v2Size <<= 7;
        u32ID3v2Size |= (uint32_t)(header[3] & 0x7F);
        u32ID3v2Size += 10;
    }

    return u32ID3v2Size;
}
// MStar Android Patch End

AACExtractor::AACExtractor(
        const sp<DataSource> &source, const sp<AMessage> &_meta)
    : mDataSource(source),
      mInitCheck(NO_INIT),
      mFrameDurationUs(0) {
    sp<AMessage> meta = _meta;

    if (meta == NULL) {
        String8 mimeType;
        float confidence;
        sp<AMessage> _meta;

        // MStar Android Patch Begin
        if (!SniffAAC(mDataSource, &mimeType, &confidence, NULL)) {
        // MStar Android Patch End
            return;
        }
    }


    uint8_t profile, sf_index, channel, header[2];
    // MStar Android Patch Begin
    uint32_t u32IDTagSize = SkipIDTag(source);
    if (mDataSource->readAt(2 + u32IDTagSize, &header, 2) < 2) {
    // MStar Android Patch End
        return;
    }

    profile = (header[0] >> 6) & 0x3;
    sf_index = (header[0] >> 2) & 0xf;
    uint32_t sr = get_sample_rate(sf_index);
    if (sr == 0) {
        return;
    }
    channel = (header[0] & 0x1) << 2 | (header[1] >> 6);

    mMeta = MakeAACCodecSpecificData(profile, sf_index, channel);

    // MStar Android Patch Begin
    off64_t offset = 0;
    off64_t streamSize, numFrames = 0;
    size_t frameSize = 0;
    int64_t duration = 0;

    offset += u32IDTagSize;
    if (mDataSource->getSize(&streamSize) == OK) {
         while (offset < streamSize) {
            if ((frameSize = getAdtsFrameLength(source, offset, NULL)) == 0) {
                break;
            }

            mOffsetVector.push(offset);

            offset += frameSize;
            numFrames ++;
        }

        if (numFrames == 0)
            return;

        // Round up and get the duration
        mFrameDurationUs = (1024 * 1000000ll + (sr - 1)) / sr;
        duration = numFrames * mFrameDurationUs;
        mMeta->setInt64(kKeyDuration, duration);
    }
    // MStar Android Patch End

    mInitCheck = OK;
}

AACExtractor::~AACExtractor() {
}

sp<MetaData> AACExtractor::getMetaData() {
    sp<MetaData> meta = new MetaData;

    if (mInitCheck != OK) {
        return meta;
    }

    meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_AAC_ADTS);

    // MStar Android Patch Begin
    ID3 id3(mDataSource);

    if (!id3.isValid()) {
        return meta;
    }

    struct Map {
        int key;
        const char *tag1;
        const char *tag2;
    };
    static const Map kMap[] = {
        { kKeyAlbum, "TALB", "TAL" },
        { kKeyArtist, "TPE1", "TP1" },
        { kKeyAlbumArtist, "TPE2", "TP2" },
        { kKeyComposer, "TCOM", "TCM" },
        { kKeyGenre, "TCON", "TCO" },
        { kKeyTitle, "TIT2", "TT2" },
        { kKeyYear, "TYE", "TYER" },
        { kKeyAuthor, "TXT", "TEXT" },
        { kKeyCDTrackNumber, "TRK", "TRCK" },
        { kKeyDiscNumber, "TPA", "TPOS" },
        { kKeyCompilation, "TCP", "TCMP" },
    };
    static const size_t kNumMapEntries = sizeof(kMap) / sizeof(kMap[0]);

    for (size_t i = 0; i < kNumMapEntries; ++i) {
        ID3::Iterator *it = new ID3::Iterator(id3, kMap[i].tag1);
        if (it->done()) {
            delete it;
            it = new ID3::Iterator(id3, kMap[i].tag2);
        }

        if (it->done()) {
            delete it;
            continue;
        }

        String8 s;
        it->getString(&s);
        delete it;
        //ALOGD("ID3:%s,%s",kMap[i].tag1, s.string());
        meta->setCString(kMap[i].key, s);
    }

    size_t dataSize;
    String8 mime;
    const void *data = id3.getAlbumArt(&dataSize, &mime);

    if (data) {
        meta->setData(kKeyAlbumArt, MetaData::TYPE_NONE, data, dataSize);
        meta->setCString(kKeyAlbumArtMIME, mime.string());
    }
    // MStar Android Patch End

    return meta;
}

size_t AACExtractor::countTracks() {
    return mInitCheck == OK ? 1 : 0;
}

sp<MediaSource> AACExtractor::getTrack(size_t index) {
    if (mInitCheck != OK || index != 0) {
        return NULL;
    }

    return new AACSource(mDataSource, mMeta, mOffsetVector, mFrameDurationUs);
}

sp<MetaData> AACExtractor::getTrackMetaData(size_t index, uint32_t flags) {
    if (mInitCheck != OK || index != 0) {
        return NULL;
    }

    return mMeta;
}

////////////////////////////////////////////////////////////////////////////////

// 8192 = 2^13, 13bit AAC frame size (in bytes)
const size_t AACSource::kMaxFrameSize = 8192;

AACSource::AACSource(
        const sp<DataSource> &source, const sp<MetaData> &meta,
        const Vector<uint64_t> &offset_vector,
        int64_t frame_duration_us)
    : mDataSource(source),
      mMeta(meta),
      mOffset(0),
      mCurrentTimeUs(0),
      mStarted(false),
      mGroup(NULL),
      mOffsetVector(offset_vector),
      mFrameDurationUs(frame_duration_us) {
}

AACSource::~AACSource() {
    if (mStarted) {
        stop();
    }
}

status_t AACSource::start(MetaData *params) {
    CHECK(!mStarted);

    // MStar Android Patch Begin
    mOffset = SkipIDTag(mDataSource);
    // MStar Android Patch End

    mCurrentTimeUs = 0;
    mGroup = new MediaBufferGroup;
    mGroup->add_buffer(new MediaBuffer(kMaxFrameSize));
    mStarted = true;

    return OK;
}

status_t AACSource::stop() {
    CHECK(mStarted);

    delete mGroup;
    mGroup = NULL;

    mStarted = false;
    return OK;
}

sp<MetaData> AACSource::getFormat() {
    return mMeta;
}

status_t AACSource::read(
        MediaBuffer **out, const ReadOptions *options) {
    *out = NULL;

    int64_t seekTimeUs;
    ReadOptions::SeekMode mode;
    if (options && options->getSeekTo(&seekTimeUs, &mode)) {
        if (mFrameDurationUs > 0) {
            int64_t seekFrame = seekTimeUs / mFrameDurationUs;
            mCurrentTimeUs = seekFrame * mFrameDurationUs;

            mOffset = mOffsetVector.itemAt(seekFrame);
        }
    }

    size_t frameSize, frameSizeWithoutHeader, headerSize;
    if ((frameSize = getAdtsFrameLength(mDataSource, mOffset, &headerSize)) == 0) {
        return ERROR_END_OF_STREAM;
    }

    MediaBuffer *buffer;
    status_t err = mGroup->acquire_buffer(&buffer);
    if (err != OK) {
        return err;
    }

    frameSizeWithoutHeader = frameSize - headerSize;
    if (mDataSource->readAt(mOffset + headerSize, buffer->data(),
                frameSizeWithoutHeader) != (ssize_t)frameSizeWithoutHeader) {
        // MStar Android Patch Begin
        ALOGD("ERROR_IO");
        //***mark for let the softaac detect the data ,if it is wrong****
        //buffer->release();
        //buffer = NULL;

        //return ERROR_IO;
        // MStar Android Patch End
    }

    buffer->set_range(0, frameSizeWithoutHeader);
    buffer->meta_data()->setInt64(kKeyTime, mCurrentTimeUs);
    buffer->meta_data()->setInt32(kKeyIsSyncFrame, 1);

    mOffset += frameSize;
    mCurrentTimeUs += mFrameDurationUs;

    *out = buffer;
    return OK;
}

////////////////////////////////////////////////////////////////////////////////

// MStar Android Patch Begin
bool SniffAAC(
        const sp<DataSource> &source, String8 *mimeType, float *confidence,
        sp<AMessage> *meta) {
    char header[20];
    uint32_t u32IDTagSize;
    bool bIsAAC = false;

    u32IDTagSize = SkipIDTag(source);

    if (source->readAt(u32IDTagSize, &header, 20) != 20) {
        return false;
    }

    // AAC
    //ftyp3gp,ftypmp42,ftypisom,ftypM4V,ftypM4A still go mpeg4extractor, here only jump the id3tag
    if (!strncasecmp("ftypisom", header+4, 8)) {
        if (strstr(header, ".mp4")) {
            ALOGI("Content is  MP4 \n");
            return false;
        } else {
            ALOGI("Content is AAC M4A \n");
            bIsAAC = true;
        }
    } else if (!strncasecmp("ftypM4V", header+4, 7)) {
        ALOGI("Content is AAC M4V\n");
        bIsAAC = true;
    } else if (!strncasecmp("ftypmp41", header+4, 8)) {
        ALOGI("Content is AAC mp41\n");
        bIsAAC = true;
    } else if (!strncasecmp("ftypmp42", header+4, 8)) {
        ALOGI("Content is AAC mp42\n");
        bIsAAC = true;
    } else if (!strncasecmp("ftyp3gp6", header+4, 8)) {
        ALOGI("Content is AAC 3gp6\n");
        bIsAAC = true;
    } else if (!strncasecmp("ftyp3gp4", header+4, 8)) {
        ALOGI("Content is AAC 3gp4\n");
        bIsAAC = true;
    } else if (!strncasecmp("ftypmmp4", header+4, 8)) {
        ALOGI("Content is AAC mmp4\n");
        bIsAAC = true;
    } else if (!strncasecmp("ftypM4A", header+4, 7)) { // bacause:ftyp3gp6,ftyp3gp4,ftypM4A,ftypM4V...
        // M4A
        ALOGI("Content is AAC M4A\n");
        bIsAAC = true;
    } else if ((header[0] == 'A') && (header[1] == 'D') && (header[2] == 'I') && (header[3] == 'F')) {
        // ADIF
        ALOGI("Content is AAC ADIF\n");
        bIsAAC = true;
    } else if (((unsigned char)header[0] == 0xFF) && (((unsigned char)header[1] & 0xF6) == 0xF0)) {
        // ADTS
        ALOGI("Content is AAC ADTS\n");
        *mimeType = MEDIA_MIMETYPE_AUDIO_AAC_ADTS;
        *confidence = 0.2;

        *meta = new AMessage;
        (*meta)->setInt64("offset", u32IDTagSize);
        return true;
    }

    if (bIsAAC == true) {
        *mimeType = MEDIA_MIMETYPE_AUDIO_AAC;
        *confidence = 0.2;

        *meta = new AMessage;
        (*meta)->setInt64("offset", u32IDTagSize);

        return true;
    }

    return false;
}
// MStar Android Patch End

}  // namespace android
