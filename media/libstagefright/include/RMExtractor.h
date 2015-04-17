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
#ifndef RM_EXTRACTOR_H_

#define RM_EXTRACTOR_H_

#include <media/stagefright/foundation/ABase.h>
#include <media/stagefright/MediaExtractor.h>
#include <media/stagefright/MediaSource.h>
#include <utils/Vector.h>

typedef enum {
    RM_UNDEFINED_STREAM = 0,
    RM_AUDIO_SINGLE_STREAM,        // physical single rate
    RM_VIDEO_SINGLE_STREAM,        // physical single rate
    RM_AUDIO_MULTI_STREAM,         // physical multi rate
    RM_VIDEO_MULTI_STREAM,         // physical multi rate
    RM_AUDIO_LOGICAL_STREAM,       // logical stream
    RM_VIDEO_LOGICAL_STREAM        // logical stream
} EN_RM_StreamType;

namespace android {

#define GENR_PATTERN_SIZE           (128)
#define MAX_VIDEO_TRACKS            (4)
#define MAX_AUDIO_TRACKS            (10)
#define MAX_SUBTITLE_TRACKS         (16)
#define MAX_TOTAL_TRACKS            (MAX_AUDIO_TRACKS + MAX_VIDEO_TRACKS + MAX_SUBTITLE_TRACKS)

struct RMExtractor : public MediaExtractor {
    RMExtractor(const sp<DataSource> &dataSource);

    virtual size_t countTracks();

    virtual sp<MediaSource> getTrack(size_t index);

    virtual sp<MetaData> getTrackMetaData(
                                         size_t index, uint32_t flags);

    virtual sp<MetaData> getMetaData();

protected:
    virtual ~RMExtractor();

private:
    struct RMSource;


    sp<DataSource> mDataSource;
    status_t mInitCheck;
    uint8_t mTrackMapId[MAX_TOTAL_TRACKS + 1];

    struct Track {
        sp<MetaData> mMeta;
        enum Kind {
            AUDIO,
            VIDEO,
            OTHER
        } mKind;

    };

    struct IndexInfo {
        uint64_t mOffset;
        uint32_t mPTS;
    };

    struct PacketInfo {
        uint64_t mPktOffset;
        uint32_t mLen;
        uint32_t mPTS;
        uint8_t  mStreamID;
    };

    typedef struct {
        struct {
            // Local
            uint32_t streamID;
            uint32_t bitrate;
            uint32_t factor;
            uint32_t blockSize;
            uint32_t frameSize;
            uint32_t samplerate;
            uint32_t samplesize;
            uint32_t type;
            uint32_t codecID;
            uint32_t duration;

            uint8_t audioFramesPerBlock;
            uint8_t channel;
            uint16_t audioFrameSize;
            uint16_t audioSuperBlockSize;
            struct {
                uint32_t numOfSamples;
                uint32_t numOfRegions;
                uint32_t CPLStart;
                uint32_t CPLQBits;
            } channelInfo[5];

            struct {
                uint8_t isFrameSizeReady;
                uint8_t numOfCodecID;
                uint32_t sampleChannel;
                uint32_t sampleRate;
                uint16_t channelMask[5];
                uint16_t numOfRegions[5];
                uint32_t CPLStart[5];
                uint16_t CPLQBits[5];
                uint32_t frameSize[5];
            } LBR;
        } AudioTracks[MAX_AUDIO_TRACKS];

        struct {
            uint32_t streamID;
            uint32_t width;
            uint32_t height;
            uint32_t frameRate;
            uint32_t frameRateBase;
            uint32_t start;
            uint32_t nb_Frames;
            uint32_t duration;
            uint32_t codecID;
        } VideoTracks[MAX_VIDEO_TRACKS];

        uint32_t decodeWidth;
        uint32_t decodeHeight;

#if  0// no subtitle RM case, now disable
        struct {
            uint32_t width;
            uint32_t height;
            uint32_t language;
        } SubtitleTracks[MAX_SUBTITLE_TRACKS];
#endif

        // Local
        off64_t PKT_endPosition;
        off64_t PKT_startPosition;
        uint32_t indexOffset;
        uint16_t numFrames;
        uint16_t GENRPattern[GENR_PATTERN_SIZE];

        // If found invalid data packet, try resync to next valid packet.
        uint8_t u8Sync;

        struct {
            uint32_t numResampleImgSize;
            struct {
                uint32_t dimWidth;
                uint32_t dimHeight;
            } dimInfo[8];
        } streamRPR;

        uint32_t mMaxPacketSize;
        uint32_t mAvgPacketSize;
        uint8_t nbAudioTracks;
        uint8_t nbVideoTracks;
        uint8_t nbStreams;
        bool mFoundIndex;
    } ST_CONTAINER_RM, *PST_CONTAINER_RM;
    ST_CONTAINER_RM mRMContext;
    Vector<Track> mTracks;
    Vector<IndexInfo> mIndex[MAX_TOTAL_TRACKS ];
    off64_t mCurReadPos;
    PacketInfo mPkt;

    status_t parseHeaders();
    status_t ParserPROP(off64_t offset, size_t size);
    status_t ParserMDPR(off64_t offset, size_t size);
    status_t ParserDATA(off64_t offset, size_t size);
    EN_RM_StreamType CheckPhysical(off64_t offset, size_t size);
    EN_RM_StreamType CheckMultiPhysical(off64_t offset, size_t size);
    EN_RM_StreamType CheckLogical(off64_t offset, size_t size);
    uint32_t mc_pop(uint32_t x);
    status_t ra8lbr_Opq(off64_t offset, size_t size, uint32_t channelTemp);
    status_t GetAudioFmt5(off64_t offset, size_t size);
    status_t getAudioHeader(off64_t offset, size_t size);
    status_t getVideoCodecOpq(off64_t offset, size_t size, uint32_t codecID, uint32_t width, uint32_t height);
    status_t getVideoHeader(off64_t offset, size_t size);
    status_t getVideoOpq(off64_t offset, size_t size);
    status_t getAudioOpq(off64_t offset, size_t size);
    int32_t Deinterleave_Genr(uint16_t interleaveFactor, uint16_t interleaveBlockSize, uint16_t frameSize);
    status_t AddTracks();
    status_t parseIndex();
    status_t readSample();

    DISALLOW_EVIL_CONSTRUCTORS(RMExtractor);
};

class String8;
struct AMessage;

bool SniffRM(
            const sp<DataSource> &source, String8 *mimeType, float *confidence,
            sp<AMessage> *);

} // namespace android

#endif  // RM_EXTRACTOR_H_
