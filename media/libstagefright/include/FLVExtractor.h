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

#ifndef FLV_EXTRACTOR_H_

#define FLV_EXTRACTOR_H_

#include <media/stagefright/foundation/ABase.h>
#include <media/stagefright/MediaExtractor.h>
#include <media/stagefright/MediaSource.h>
#include <utils/Vector.h>


namespace android {

#define ENABLE_FLV_ONMETADATA_PARSE 0


struct FLVExtractor : public MediaExtractor {
    FLVExtractor(const sp<DataSource> &dataSource);

    virtual size_t countTracks();

    virtual sp<MediaSource> getTrack(size_t index);

    virtual sp<MetaData> getTrackMetaData(
            size_t index, uint32_t flags);

    virtual sp<MetaData> getMetaData();

protected:
    virtual ~FLVExtractor();

private:
    struct FLVSource;


    sp<DataSource> mDataSource;
    status_t mInitCheck;

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

    typedef struct
    {
        struct
        {
            uint32_t bitrate;
            uint32_t factor;
            uint32_t blockSize;
            uint32_t blockAlign;
            uint32_t scale;
            uint32_t frameSize;
            uint32_t samplerate;
            uint32_t samplesize;
            uint32_t type;
            uint32_t codecID;
            uint32_t objectTypeID;

            uint8_t audioFramesPerBlock;
            uint8_t channel;
            uint8_t bitsPerSample;
            uint16_t audioFrameSize;
            uint16_t audioSuperBlockSize;

        } AudioTracks[1];

        struct
        {
            uint32_t width;
            uint32_t height;
            uint32_t type;
            uint32_t frameRate;
            uint32_t frameRateBase;
            uint32_t nb_Frames;
            uint32_t duration;
            uint32_t codecID;
            const void *csd;
            size_t csdSize;

        } VideoTracks[1];

        off64_t mPKT_startPosition;
        uint32_t mMaxPacketSize;
        uint8_t nbAudioTracks;
        uint8_t nbVideoTracks;
        uint8_t nbStreams;
        bool mFoundIndex;
    } ST_CONTAINER_FLV, *PST_CONTAINER_FLV;

    ST_CONTAINER_FLV mFLVContext;
    Vector<Track> mTracks;
    Vector<IndexInfo> mIndex;
    off64_t mCurReadPos;
    PacketInfo mPkt;
    status_t readAVCConfigurationRecord(off64_t offset, uint32_t size);
    uint32_t index2SampleRate(uint32_t sampleRateIdx);
    void readAACConfigurationRecord(uint32_t extraData);
    void AddTracks();
    status_t parseHeaders();
    uint16_t getBits(uint8_t *buffer, uint16_t totbitoffset, uint32_t *info, uint16_t bufferLen, uint16_t numbits);
    uint32_t readSyntaxElement_FLC(uint16_t readBits, uint8_t* buffer, uint16_t bufferLen, uint16_t* bitLen, bool bUnsige);
    uint16_t getVLCSymbol(uint8_t *buffer, uint16_t totbitoffset, uint32_t *info, uint16_t bufferLen);
    uint32_t readSyntaxElement_VLC(uint8_t* buffer, uint16_t bufferLen, uint16_t* bitLen, bool unsige);
    uint16_t EBSPtoRBSP(uint8_t *pSrc, uint8_t *pdestBuf, uint16_t end_bytepos);
    bool parseH264SPS(uint8_t* pAddr, uint16_t dataLen, uint8_t nalSize, bool startCode);

#if ENABLE_FLV_ONMETADATA_PARSE
    double int2Double(uint64_t v);
    int32_t amfGetString(off64_t offset, uint8_t *pBuffer, uint32_t buffSize, uint32_t *pCount);
    int32_t amfParseObject(off64_t offset, uint8_t *pKey, uint32_t *pCount, uint32_t maxPosition, uint32_t depth);
#endif
    DISALLOW_EVIL_CONSTRUCTORS(FLVExtractor);
};

class String8;
struct AMessage;

bool SniffFLV(
        const sp<DataSource> &source, String8 *mimeType, float *confidence,
        sp<AMessage> *);

}  // namespace android

#endif  // FLV_EXTRACTOR_H_
