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
#define LOG_TAG "RMExtractor"
#include <utils/Log.h>

#include "include/avc_utils.h"
#include "include/RMExtractor.h"

#include <binder/ProcessState.h>
#include <media/stagefright/foundation/hexdump.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/DataSource.h>
#include <media/stagefright/MediaBuffer.h>
#include <media/stagefright/MediaBufferGroup.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/Utils.h>
#include <OMX_Audio.h>

#define MFP_DBG_RM2(x)   //(x)
#define MFP_DBG_RM3(x)   //x
#define MIN_HEADER_SIZE  13

//--- Defines the same as sample code ---
/* Audio codec defines */
#define RA_FORMAT_ID            0x2E7261FD  // RealAudio Stream
#define RA_NO_INTERLEAVER       0x496E7430  // 'Int0' (no interleaver)
#define RA_INTERLEAVER_SIPR     0x73697072  // "sipr"
#define RA_INTERLEAVER_GENR     0x67656E72  // "genr", interleaver used for ra8lbr and ra8hbr codecs
#define RA_INTERLEAVER_VBRS     0x76627273  // "vbrs", Simple VBR interleaver
#define RA_INTERLEAVER_VBRF     0x76627266  // "vbrf", Simple VBR interleaver (with possibly fragmenting)

/* Video codec defines */
#define RM_MEDIA_AUDIO          0x4155444FL // 'AUDO'
#define RM_MEDIA_VIDEO          0x5649444FL // 'VIDO'
#define RM_RVTRVIDEO_ID         0x52565452  // 'RVTR' (for rv20 codec)
#define RM_RVTR_RV30_ID         0x52565432  // 'RVT2' (for rv30 codec)
#define RM_RV20VIDEO_ID         0x52563230  // 'RV20'
#define RM_RV30VIDEO_ID         0x52563330  // 'RV30'
#define RM_RV40VIDEO_ID         0x52563430  // 'RV40'
#define RM_RVG2VIDEO_ID         0x52564732  // 'RVG2' (raw TCK format)
#define RM_RV89COMBO_ID         0x54524F4D  // 'TROM' (raw TCK format)

//#define RV20_MAJOR_BITSTREAM_VERSION  2

#define RV30_MAJOR_BITSTREAM_VERSION  3
//#define RV30_BITSTREAM_VERSION        2
//#define RV30_PRODUCT_RELEASE          0
//#define RV30_FRONTEND_VERSION         2

#define RV40_MAJOR_BITSTREAM_VERSION  4
//#define RV40_BITSTREAM_VERSION        0
//#define RV40_PRODUCT_RELEASE          2
//#define RV40_FRONTEND_VERSION         0

#define RAW_BITSTREAM_MINOR_VERSION   128

/*
#define RAW_RVG2_MAJOR_VERSION        2
#define RAW_RVG2_BITSTREAM_VERSION    RAW_BITSTREAM_MINOR_VERSION
#define RAW_RVG2_PRODUCT_RELEASE      0
#define RAW_RVG2_FRONTEND_VERSION     0
#define RAW_RV8_MAJOR_VERSION         3
#define RAW_RV8_BITSTREAM_VERSION     RAW_BITSTREAM_MINOR_VERSION
#define RAW_RV8_PRODUCT_RELEASE       0
#define RAW_RV8_FRONTEND_VERSION      0
#define RAW_RV9_MAJOR_VERSION         4
#define RAW_RV9_BITSTREAM_VERSION     RAW_BITSTREAM_MINOR_VERSION
#define RAW_RV9_PRODUCT_RELEASE       0
#define RAW_RV9_FRONTEND_VERSION      0
*/

/* definitions for decoding opaque data in bitstream header */
#define RV40_SPO_FLAG_UNRESTRICTEDMV           0x00000001    // ANNEX D
#define RV40_SPO_FLAG_EXTENDMVRANGE            0x00000002    // IMPLIES NEW VLC TABLES
#define RV40_SPO_FLAG_ADVMOTIONPRED            0x00000004    // ANNEX F
#define RV40_SPO_FLAG_ADVINTRA                 0x00000008    // ANNEX I
#define RV40_SPO_FLAG_INLOOPDEBLOCK            0x00000010    // ANNEX J
#define RV40_SPO_FLAG_SLICEMODE                0x00000020    // ANNEX K
#define RV40_SPO_FLAG_SLICESHAPE               0x00000040    // 0: free running; 1: rect
#define RV40_SPO_FLAG_SLICEORDER               0x00000080    // 0: sequential; 1: arbitrary
#define RV40_SPO_FLAG_REFPICTSELECTION         0x00000100    // ANNEX N
#define RV40_SPO_FLAG_INDEPENDSEGMENT          0x00000200    // ANNEX R
#define RV40_SPO_FLAG_ALTVLCTAB                0x00000400    // ANNEX S
#define RV40_SPO_FLAG_MODCHROMAQUANT           0x00000800    // ANNEX T
#define RV40_SPO_FLAG_BFRAMES                  0x00001000    // SETS DECODE PHASE
#define RV40_SPO_BITS_DEBLOCK_STRENGTH         0x0000e000    // deblocking strength
#define RV40_SPO_BITS_NUMRESAMPLE_IMAGES       0x00070000    // max of 8 RPR images sizes
#define RV40_SPO_FLAG_FRUFLAG                  0x00080000    // FRU BOOL: if 1 then OFF;
#define RV40_SPO_FLAG_FLIP_FLIP_INTL           0x00100000    // FLIP-FLOP interlacing;
#define RV40_SPO_FLAG_INTERLACE                0x00200000    // de-interlacing prefilter has been applied;
#define RV40_SPO_FLAG_MULTIPASS                0x00400000    // encoded with multipass;
#define RV40_SPO_FLAG_INV_TELECINE             0x00800000    // inverse-telecine prefilter has been applied;
#define RV40_SPO_FLAG_VBR_ENCODE               0x01000000    // encoded using VBR;
#define RV40_SPO_BITS_DEBLOCK_SHIFT            13
#define RV40_SPO_BITS_NUMRESAMPLE_IMAGES_SHIFT 16

#define RA_CODEC_ID_SIPR        0x73697072        // sipr
#define RA_CODEC_ID_COOK       0x636F6F6B        // cook
#define RA_CODEC_ID_ATRC       0x61747263        // atrc
#define RA_CODEC_ID_RAAC       0x72616163        // raac
#define RA_CODEC_ID_RACP       0x72616370        // racp

#define RM_RELIABLE_FLAG     0x0001
#define RM_KEYFRAME_FLAG    0x0002

#define RM_READSIZE 4096

#define DATA_PACKET_HEAD_VERSION0 12
#define DATA_PACKET_HEAD_VERSION1 13

namespace android {

struct RMExtractor::RMSource : public MediaSource {
    RMSource(const sp<RMExtractor> &extractor, size_t trackIndex);

    virtual status_t start(MetaData *params);
    virtual status_t stop();

    virtual sp<MetaData> getFormat();

    virtual status_t read(
                         MediaBuffer **buffer, const ReadOptions *options);

protected:
    virtual ~RMSource();

private:
    int64_t seek(Vector<IndexInfo> Index,int64_t s64IndexTime);
    sp<RMExtractor> mExtractor;
    size_t mTrackIndex;
    const RMExtractor::Track &mTrack;
    MediaBufferGroup *mBufferGroup;
    // size_t mSampleIndex;
    List<MediaBuffer *> mPendingFrames;

    DISALLOW_EVIL_CONSTRUCTORS(RMSource);
};

int64_t RMExtractor::RMSource::seek(Vector<IndexInfo> Index,int64_t s64IndexTime)
{
    int32_t s32left = 0;
    int32_t s32right = Index.size() - 1;
    int32_t s32TrackIndex =0;
    IndexInfo *pidxInf = NULL;
    while (s32left <= s32right) {
        int s32middle = (s32left + s32right) / 2;
        pidxInf = &Index.editItemAt( s32middle );
        if ( pidxInf->mPTS == s64IndexTime) {
            return pidxInf->mOffset;
        }
        if ( s64IndexTime > pidxInf->mPTS ) {
            s32left = s32middle + 1;
        } else {
            s32right = s32middle -1;
        }
    }
    if ( (uint32_t)s32left > Index.size() - 1) {
        pidxInf = &Index.editItemAt( Index.size() - 1 );
    }
    return pidxInf->mOffset;

}

RMExtractor::RMSource::RMSource(
                               const sp<RMExtractor> &extractor, size_t trackIndex)
: mExtractor(extractor),
mTrackIndex(trackIndex),
mTrack(mExtractor->mTracks.itemAt(trackIndex)),
mBufferGroup(NULL) {

}

RMExtractor::RMSource::~RMSource() {
    if (mBufferGroup) {
        stop();
    }
}

status_t RMExtractor::RMSource::start(MetaData *params) {
    uint32_t u32BlockSize = mExtractor->mRMContext.AudioTracks[mExtractor->mTrackMapId[mTrackIndex] ].audioSuperBlockSize;
    uint32_t u32MaxPacketSize = mExtractor->mRMContext.mMaxPacketSize;
    uint32_t u32BufferSize = 0;
    CHECK(!mBufferGroup);

    mBufferGroup = new MediaBufferGroup;

    if ( u32BlockSize > u32MaxPacketSize ) {
        u32BufferSize = u32BlockSize;
    } else {
        u32BufferSize = u32MaxPacketSize;
    }
    mBufferGroup->add_buffer(new MediaBuffer(u32BufferSize));//the buffer to set read data
    mBufferGroup->add_buffer(new MediaBuffer(u32BufferSize));//the buffer to set interleave data
    // mBufferGroup->add_buffer(new MediaBuffer(mTrack.mMaxSampleSize));
    // mSampleIndex = 0;

    //  const char *mime;
    //  CHECK(mTrack.mMeta->findCString(kKeyMIMEType, &mime));

    return OK;
}

status_t RMExtractor::RMSource::stop() {
    CHECK(mBufferGroup);

    delete mBufferGroup;
    mBufferGroup = NULL;

    return OK;
}

sp<MetaData> RMExtractor::RMSource::getFormat() {
    return mTrack.mMeta;
}

status_t RMExtractor::RMSource::read(
                                    MediaBuffer **buffer, const ReadOptions *options) {
    CHECK(mBufferGroup);
    *buffer = NULL;

    MediaBuffer *pOut = NULL;
    MediaBuffer *pInterLeave = NULL;
    uint16_t u16ObjVer;
    uint16_t u16Length;
    uint16_t u16StreamID;
    uint32_t u32Timestamp;
    uint16_t u16Offset = 0;
    uint8_t u8Flags = 0; // Key frame flag
    uint8_t u8PktHeader[13];
    uint8_t u8StreamType;
    uint8_t u8Header[14] = {0};
    uint8_t u8FrameType;
    uint8_t u8BrokenByUs = 0;
    uint8_t u8SizeFlag;
    uint32_t u32HdrLen_Var = 0;
    uint32_t u32FlushSize = 0;
    uint32_t u32RangeSize = 0;
    PacketInfo stPkt;
    uint32_t u32ValidSize = 0;
    uint8_t u8HeadSize = 0;
    const char *mime;
    int64_t seekTimeUs;
    ReadOptions::SeekMode mode;

    if ( options != NULL && options->getSeekTo(&seekTimeUs, &mode) ) {
        if ( seekTimeUs > 0) {
            bool bSeekFound = false;
            int64_t  s64IndexTime = seekTimeUs/1000;
            if (mExtractor->mRMContext.mFoundIndex != true) {
                return ERROR_MALFORMED;
            }
            //if seektime larger than maximum value in index, than set the last index offset
            uint32_t u32LastPts = ((mExtractor->mIndex[mTrackIndex]).itemAt( mExtractor->mIndex[mTrackIndex].size()-1 )).mPTS;
            off64_t offLastOffset = ((mExtractor->mIndex[mTrackIndex]).itemAt( mExtractor->mIndex[mTrackIndex].size()-1 )).mOffset;
            if ( s64IndexTime >= (int64_t)u32LastPts ) {
                mExtractor->mCurReadPos = offLastOffset;
            } else {
                mExtractor->mCurReadPos = seek(mExtractor->mIndex[mTrackIndex],s64IndexTime);
            }
        }
    }

    if (mExtractor->mCurReadPos >= mExtractor->mRMContext.PKT_endPosition) {
        ALOGW("readPos = %lld", mExtractor->mCurReadPos);
        ALOGW("PKEend = %lld", mExtractor->mRMContext.PKT_endPosition);
        return ERROR_END_OF_STREAM;
    }

    mTrack.mMeta->findCString(kKeyMIMEType, &mime);

    // If invalid data packet happens, try to resync to valid packet.
    if (mExtractor->mRMContext.u8Sync) {
        uint32_t u32i,u32j;
        bool bFound = false;
        uint8_t u8Buffer[RM_READSIZE];
        off64_t Offsize = 0;
        uint32_t u32ScanRange = mExtractor->mDataSource->readAt(mExtractor->mCurReadPos, u8Buffer, RM_READSIZE) ;

        if ( u32ScanRange < MIN_HEADER_SIZE ) {// min packet header size
            ALOGW("u32ScanRange = %d", u32ScanRange);
            return ERROR_END_OF_STREAM;
        }
        u32j = 0;
        while (u32j < u32ScanRange - MIN_HEADER_SIZE ) {
            int s32Offsize = 0;
            u16ObjVer = ((uint16_t)u8Buffer[u32j] << 8) | (uint16_t)u8Buffer[u32j+1];
            if (u16ObjVer != 0 && u16ObjVer != 1) {
                s32Offsize += 2;
            } else {
                if ((u8Buffer[u32j+12] != 0) && (u8Buffer[u32j+12] != 2)) {
                    s32Offsize += 2;
                } else if (u8Buffer[u32j+4] != 0) {
                    s32Offsize += 2;
                } else {
                    // If garbage data is all zero, skip them.
                    if ((u8Buffer[u32j] == 0) && (u8Buffer[u32j+1] == 0) && (u8Buffer[u32j+2] == 0) && (u8Buffer[u32j+3] == 0)) {
                        s32Offsize += 2;
                    } else {
                        bFound = true;
                        u32j += s32Offsize;
                        break;
                    }
                }
            }
            u32j += s32Offsize;
        }

        if (bFound) {
            mExtractor->mCurReadPos += u32j;
            mExtractor->mRMContext.u8Sync = 0;// if find vaild sync, set sync to 0
            MFP_DBG_RM3(ALOGD("Found = %x", bFound));
            MFP_DBG_RM3(ALOGD("mCurReadPos = %lld", mExtractor->mCurReadPos));
        } else {
            mExtractor->mCurReadPos += u32ScanRange - 11;
            MFP_DBG_RM3(ALOGD("Found = %x", bFound));
            MFP_DBG_RM3(ALOGD("mCurReadPos = %lld", mExtractor->mCurReadPos));
            return ERROR_MALFORMED;
        }
    }

    // Get Data Packet Header
    // prefetch first 12 bytes
    ssize_t n = mExtractor->mDataSource->readAt(mExtractor->mCurReadPos, u8PktHeader, sizeof(u8PktHeader));
    if ( n != sizeof(u8PktHeader) ) {
        ALOGE("Get Data Packet Header Error");
        return ERROR_END_OF_STREAM;
    }
    // Check object version
    u16ObjVer = U16_AT(u8PktHeader);
    u16Length = U16_AT((u8PktHeader+2));

    MFP_DBG_RM3(ALOGD("u16Length = %x",u16Length));
    if (( (u16ObjVer != 0) && (u16ObjVer != 1)) || (u16Length < 12)) {
        //cann't find the sync, need to resync
        if ( mExtractor->mCurReadPos >= mExtractor->mRMContext.PKT_endPosition) {
            return ERROR_END_OF_STREAM;
        }
        mExtractor->mRMContext.u8Sync = 1;
        ALOGW("*Rm Data Packet Header Error*");
        return ERROR_MALFORMED;
    }

    u16StreamID = U16_AT((u8PktHeader+4));
    u32Timestamp = U32_AT((u8PktHeader+6));
    MFP_DBG_RM3( ALOGD("u16StreamID = %d", u16StreamID) );
    MFP_DBG_RM3( ALOGD("u32Timestamp = %d", u32Timestamp) );
    if (u16ObjVer == 0) {
        u16Offset = DATA_PACKET_HEAD_VERSION0;//if u16ObjVer equals 12, the header size is 12
        u8Flags = u8Header[11];
        u8HeadSize = DATA_PACKET_HEAD_VERSION0;
    } else if (u16ObjVer == 1) {
        u16Offset = DATA_PACKET_HEAD_VERSION1;//if u16ObjVer equals 13, the header size is 13
        u8HeadSize = DATA_PACKET_HEAD_VERSION1;
    }

    off64_t off64TotalSize;
    if ( ERROR_UNSUPPORTED == mExtractor->mDataSource->getSize(&off64TotalSize)  ) {
        ALOGE("*Get totalSize Error*");
        return ERROR_IO;
    }
    if ( (off64TotalSize - mExtractor->mCurReadPos) < (off64_t)(u16Length + 4) ) {
        ALOGW("leftSize = %lld", off64TotalSize - mExtractor->mCurReadPos);
        return ERROR_END_OF_STREAM;
    }

    if (mTrack.mKind == Track::VIDEO) {

        u32FlushSize = u16Length;
        u32ValidSize = u16Length;

        if ( mExtractor->mDataSource->readAt(mExtractor->mCurReadPos+u16Offset, u8Header, 1) != 1 ) {
            ALOGE("*VIDEO::Read u8Header Error*");
            return ERROR_END_OF_STREAM;
        }
        u8FrameType = u8Header[0] >> 6;
        if ( (0 == u8FrameType) || (2 == u8FrameType)) {
            if ( mExtractor->mDataSource->readAt(mExtractor->mCurReadPos+u16Offset+2, u8Header, 1) != 1 ) {
                ALOGE("*VIDEO::Read u8Header Error*");
                return ERROR_END_OF_STREAM;
            }
            u8BrokenByUs = u8Header[0] >> 7;
        }
        //gbIsBroken
        if ( 1 == u8BrokenByUs ) {
            u8SizeFlag = (u8Header[0] >> 6) & 0x01;
            if ( 1 == u8SizeFlag) {
                u32HdrLen_Var += 2;
            } else {
                u32HdrLen_Var += 4;
            }

            if ( mExtractor->mDataSource->readAt(mExtractor->mCurReadPos+u16Offset+ 2 + u32HdrLen_Var, u8Header, 1) != 1 ) {
                ALOGE("*VIDEO::Read u8Header Error*");
                ALOGE("mCurReadPos = %lld", mExtractor->mCurReadPos+u16Offset+ 2 + u32HdrLen_Var);
                return ERROR_END_OF_STREAM;
            }
            u8SizeFlag = (u8Header[0] >> 6) & 0x01;
            if (u8SizeFlag == 1) {
                u32HdrLen_Var += 2;
            } else {
                u32HdrLen_Var += 4;
            }

            u32HdrLen_Var += 3;
            u16Offset += (uint16_t)u32HdrLen_Var;
            u32ValidSize -=  u16Offset;
            mExtractor->mCurReadPos += u16Offset;
            MFP_DBG_RM3(ALOGD("u32HdrLen_Var = %d", u32HdrLen_Var));
            MFP_DBG_RM3(ALOGD("u16Offset = %d", u16Offset));
            MFP_DBG_RM3(ALOGD("u32ValidSize = %d", u32ValidSize));
            MFP_DBG_RM3(ALOGD("mCurReadPos = %lld", mExtractor->mCurReadPos));
        }
    } else if ( mTrack.mKind== Track::AUDIO ) {

        uint8_t u8AudioTrackIndex = mExtractor->mTrackMapId[mTrackIndex];

        // Audio don't need data header, flush it

        u16Length -= u16Offset;
        u32FlushSize = u16Length;
        mExtractor->mCurReadPos += u16Offset;
        u32ValidSize = u16Length;
        MFP_DBG_RM3(ALOGD("u16Length = %d", u16Length));
        MFP_DBG_RM3(ALOGD("u16Offset = %d", u16Offset));
        MFP_DBG_RM3(ALOGD("mCurReadPos = %lld", mExtractor->mCurReadPos));
        if ( !strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_AAC) ) {
            uint32_t u32TotalSize = 0;
            uint32_t u32Idx = 0;
            uint32_t u32FrameCnt;
            uint32_t u32FrameSize;
            uint32_t i;
            uint8_t u8FrameCnt[2];

            if ( 2 != mExtractor->mDataSource->readAt(mExtractor->mCurReadPos + u32Idx , u8FrameCnt, 2) ) {
                ALOGE("*AAC::Read Error*");
                return ERROR_END_OF_STREAM;
            }
            u32FrameCnt = ((uint32_t)u8FrameCnt[0]<< 8) | (uint32_t)u8FrameCnt[1];
            u32FrameCnt = (u32FrameCnt + 7) >> 3;
            u32Idx += 2;

            //Sanity check to make sure that the AU header size is
            // greater than 0 and a multiple of 2 bytes
            if (u32FrameCnt && !(u32FrameCnt & 1)) {
                u32FrameCnt = u32FrameCnt >> 1;
                for (i = 0; i < u32FrameCnt; i++) {
                    uint8_t u8FrameSize [2];
                    if ( 2 != mExtractor->mDataSource->readAt(mExtractor->mCurReadPos + u32Idx , u8FrameSize, 2) ) {
                        ALOGE("*AAC::Read Error*");
                        return ERROR_END_OF_STREAM;
                    }
                    u32FrameSize = ((uint32_t)u8FrameSize[0] << 8) | u8FrameSize[1];
                    u32TotalSize += u32FrameSize;
                    u32Idx += 2;
                }

                mExtractor->mCurReadPos += (2 + 2 * u32FrameCnt);
                u32ValidSize = (int32_t)u32TotalSize;
                u32FlushSize = u16Length;
                MFP_DBG_RM3(ALOGD("u32ValidSize = %d", u32ValidSize));
                MFP_DBG_RM3(ALOGD("mCurReadPos = %lld", mExtractor->mCurReadPos));
            }
        }
    } else {
        u32ValidSize = u16Length;
        u32FlushSize = u16Length;
    }

    if ( 4 != mExtractor->mDataSource->readAt(mExtractor->mCurReadPos + u32FlushSize , u8Header, 4) ) {
        ALOGE("*Read error*");
        return ERROR_END_OF_STREAM;
    }
    u16ObjVer = (uint16_t)(((uint16_t)u8Header[0] << 8) | (uint32_t)u8Header[1]);

    status_t err = mBufferGroup->acquire_buffer(&pOut);
    if (err != OK) {
        ALOGE("*Acquire Buffer error*");
        return err;
    }

    if (u8Flags == RM_KEYFRAME_FLAG) {
        pOut->meta_data()->setInt32(kKeyIsSyncFrame, 1);
    }
    u32RangeSize = u32ValidSize;

    if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_COOK)) {
        uint16_t u16audioSuperBlockSize = mExtractor->mRMContext.AudioTracks[mExtractor->mTrackMapId[mTrackIndex] ].audioSuperBlockSize;
        uint8_t u8audioFramesPerBlock = mExtractor->mRMContext.AudioTracks[mExtractor->mTrackMapId[mTrackIndex] ].audioFramesPerBlock;
        uint16_t u16audioFrameSize = mExtractor->mRMContext.AudioTracks[mExtractor->mTrackMapId[mTrackIndex] ].audioFrameSize;
        uint32_t u32BlockSize = u8audioFramesPerBlock * u16audioFrameSize;
        int s32BlockNum =  u16audioSuperBlockSize / u8audioFramesPerBlock / u16audioFrameSize;
        off64_t offtmp = mExtractor->mCurReadPos;

        status_t err = mBufferGroup->acquire_buffer(&pInterLeave);
        if (err != OK) {
            ALOGE("Acquire buffer Error");
            err = ERROR_IO;
            goto fail;
        }
        mExtractor->mCurReadPos = mExtractor->mCurReadPos - u8HeadSize;
        // pick up each block in superBlock
        for (int i = 0; i < s32BlockNum ; i++ ) {
            err = mExtractor->readSample();// read block to find packet info

            if ( err != OK) {
                ALOGE("Read Sample Error");
                goto fail;
            }
            //find the frame in packet the real location, and fit them in buffer
            PacketInfo* pkt = &mExtractor->mPkt;
            mExtractor->mCurReadPos = pkt->mLen + mExtractor->mCurReadPos;
            if ( (int32_t)u32BlockSize != mExtractor->mDataSource->readAt(pkt->mPktOffset , pInterLeave->data(),u32BlockSize)) {
                ALOGE("Read Block Error");
                err = ERROR_IO;
                goto fail;
            }
            for (int j = 0 ; j < u8audioFramesPerBlock ; j++) {
                int s32destFrame = mExtractor->mRMContext.GENRPattern[j+i*u8audioFramesPerBlock];
                uint8_t* dest = (uint8_t*)pOut->data() + s32destFrame * u16audioFrameSize;
                uint8_t* src = (uint8_t*)pInterLeave->data() + j*u16audioFrameSize;
                if ( (uint32_t)(s32destFrame * u16audioFrameSize) > pOut->size()) {
                    ALOGE("InterLeave copy memory error");
                    err = ERROR_IO;
                    goto fail;
                }
                memcpy((void*)dest,(void*)(src),u16audioFrameSize);
            }
        }

        pInterLeave->release();
        pInterLeave = NULL;
        u32RangeSize = u16audioSuperBlockSize;
        u32ValidSize = 0;
    } else if ((int64_t)u32ValidSize != mExtractor->mDataSource->readAt(mExtractor->mCurReadPos,pOut->data(),u32ValidSize)) {
        err = ERROR_END_OF_STREAM;
        goto fail;
    }

    mExtractor->mCurReadPos += u32ValidSize;
    pOut->set_range(0, u32RangeSize);
    pOut->meta_data()->setInt64(kKeyTime, u32Timestamp*1000LL);

    *buffer = pOut;
    return OK;
    fail:
    if ( pOut != NULL ) {
        pOut->release();
        pOut = NULL;
    }
    if ( pInterLeave != NULL) {
        pInterLeave->release();
        pInterLeave = NULL;
    }
    return err;
}

status_t RMExtractor::readSample() {
    uint8_t header[14];
    ssize_t n = mDataSource->readAt(mCurReadPos, header, 14);
    uint16_t offset = 0, i = 0;
    uint16_t objVer = 0;
    uint16_t length = 0;
    uint16_t streamID = 0;
    uint32_t timestamp =0;
    //  uint8_t frameType = 0;
    //  uint8_t brokenByUs = 0;
    // uint8_t sizeFlag = 0;
    //  uint32_t hdrLen_Var = 0;
    uint8_t flags = 0; // Key frame flag
    //  uint8_t streamType;

    // Check object version
    objVer = ((uint16_t)header[0] << 8) | (uint16_t)header[1];
    length = ((uint16_t)header[2] << 8) | (uint16_t)header[3];
    if ((objVer != 0 && objVer != 1) || (length < 12)) {
        // to do, error concealment
        return ERROR_MALFORMED;
    }

    // Get packet length, stream number, timestamp
    streamID = ((uint16_t)header[4] << 8) | (uint16_t)header[5];
    timestamp = ((uint32_t)header[6] << 24) | ((uint32_t)header[7] << 16) | ((uint32_t)header[8] << 8) | (uint32_t)header[9];


    MFP_DBG_RM2(ALOGW("u16StreamNB = %d\n", streamID));

    if (objVer == 0) {
        offset = 12;
        flags = header[11];
    } else if (objVer == 1) {
        // asm_rule = (uint32_t)d[10]<<8 | (uint32_t)d[11]

        // asm_flags
        offset = 13;
    }

    mPkt.mPktOffset = mCurReadPos + offset;
    mPkt.mPTS = timestamp;
    mPkt.mStreamID = streamID;
    mPkt.mLen = length;
    MFP_DBG_RM3(ALOGD("mPktOffset %lld",mPkt.mPktOffset));
    MFP_DBG_RM3(ALOGD("mLen %d",length));
    MFP_DBG_RM3(ALOGD("offset %d",offset));
    MFP_DBG_RM3(ALOGD("mCurReadPos %lld",mCurReadPos));
    //stPkt.u32Size = length;
    //stPkt.u8Stream_Index = (uint8_t)streamID;
    return OK;
}

EN_RM_StreamType RMExtractor::CheckPhysical(off64_t offset, size_t size) {
    uint32_t tag;
    sp<ABuffer> buffer = new ABuffer(size);
    ssize_t n = mDataSource->readAt(offset, buffer->data(), buffer->size());

    const uint8_t *data = buffer->data();

    tag = U32_AT(&data[0]);

    if (tag == FOURCC('a', 'u', 'd', 'i')) {
        //tag = msAPI_VDPlayer_BMReadBytes_AutoLoad(4);
        tag = U32_AT(&data[4]);
        if (tag == FOURCC('o', '/', 'x', '-')) {
            //tag = msAPI_VDPlayer_BMReadBytes_AutoLoad(4);
            tag = U32_AT(&data[8]);
            if (tag == FOURCC('p', 'n', '-', 'r')) {
                //tag = msAPI_VDPlayer_BMReadBytes_AutoLoad(4);
                tag = U32_AT(&data[12]);
                if (tag == FOURCC('e', 'a', 'l', 'a')) {
                    //tag = msAPI_VDPlayer_BMReadBytes_AutoLoad(4);
                    tag = U32_AT(&data[16]);
                    if (tag == FOURCC('u', 'd', 'i', 'o')) {
                        ALOGW("audio/x-pn-realaudio\n");
                        //msAPI_RMFP_SetAVStreamArg(u8StreamIndex, RM_STREAM_ARG_STREAM_TYPE, RM_AUDIO_SINGLE_STREAM);
                        return  RM_AUDIO_SINGLE_STREAM;
                    }
                }
            }
        }
    } else if (tag == FOURCC('v', 'i', 'd', 'e')) {
        //tag = msAPI_VDPlayer_BMReadBytes_AutoLoad(4);
        tag = U32_AT(&data[4]);
        if (tag == FOURCC('o', '/', 'x', '-')) {
            //tag = msAPI_VDPlayer_BMReadBytes_AutoLoad(4);
            tag = U32_AT(&data[8]);
            if (tag == FOURCC('p', 'n', '-', 'r')) {
                //tag = msAPI_VDPlayer_BMReadBytes_AutoLoad(4);
                tag = U32_AT(&data[12]);
                if (tag == FOURCC('e', 'a', 'l', 'v')) {
                    //tag = msAPI_VDPlayer_BMReadBytes_AutoLoad(4);
                    tag = U32_AT(&data[16]);
                    if (tag == FOURCC('i', 'd', 'e', 'o')) {
                        ALOGW("video/x-pn-realvideo\n");
                        //msAPI_RMFP_SetAVStreamArg(u8StreamIndex, RM_STREAM_ARG_STREAM_TYPE, RM_VIDEO_SINGLE_STREAM);
                        return  RM_VIDEO_SINGLE_STREAM;
                    }
                }
            }
        }
    }
    return RM_UNDEFINED_STREAM;
}

EN_RM_StreamType RMExtractor::CheckMultiPhysical(off64_t offset, size_t size) {
    uint32_t tag;
    sp<ABuffer> buffer = new ABuffer(size);
    ssize_t n = mDataSource->readAt(offset, buffer->data(), buffer->size());

    const uint8_t *data = buffer->data();

    tag = U32_AT(&data[0]);

    if (tag == FOURCC('a', 'u', 'd', 'i')) {    // "audio/x-pn-multirate-realaudio"
        //tag = msAPI_VDPlayer_BMReadBytes_AutoLoad(4);
        tag = U32_AT(&data[4]);
        if (tag == FOURCC('o', '/', 'x', '-')) {
            //tag = msAPI_VDPlayer_BMReadBytes_AutoLoad(4);
            tag = U32_AT(&data[8]);
            if (tag == FOURCC('p', 'n', '-', 'm')) {
                //tag = msAPI_VDPlayer_BMReadBytes_AutoLoad(4);
                tag = U32_AT(&data[12]);
                if (tag == FOURCC('u', 'l', 't', 'i')) {
                    //tag = msAPI_VDPlayer_BMReadBytes_AutoLoad(4);
                    tag = U32_AT(&data[16]);
                    if (tag == FOURCC('r', 'a', 't', 'e')) {
                        //tag = msAPI_VDPlayer_BMReadBytes_AutoLoad(4);
                        tag = U32_AT(&data[20]);
                        if (tag == FOURCC('-', 'r', 'e', 'a')) {
                            //tag = msAPI_VDPlayer_BMReadBytes_AutoLoad(4);
                            tag = U32_AT(&data[24]);
                            if (tag == FOURCC('l', 'a', 'u', 'd')) {
                                //tag = msAPI_VDPlayer_BMReadBytes_AutoLoad(2);
                                tag = U16_AT(&data[28]);
                                if (tag == FOURCC(0, 0, 'i', 'o')) {
                                    ALOGW("audio/x-pn-multirate-realaudio\n");
                                    //msAPI_RMFP_SetAVStreamArg(u8StreamIndex, RM_STREAM_ARG_STREAM_TYPE, RM_AUDIO_MULTI_STREAM);
                                    return  RM_AUDIO_MULTI_STREAM;
                                }
                            }
                        }
                    }
                }
            }
        }
    } else if (tag == FOURCC('v', 'i', 'd', 'e')) {    // "video/x-pn-multirate-realvideo"
        //tag = msAPI_VDPlayer_BMReadBytes_AutoLoad(4);
        tag = U32_AT(&data[4]);
        if (tag == FOURCC('o', '/', 'x', '-')) {
            //tag = msAPI_VDPlayer_BMReadBytes_AutoLoad(4);
            tag = U32_AT(&data[8]);
            if (tag == FOURCC('p', 'n', '-', 'm')) {
                //tag = msAPI_VDPlayer_BMReadBytes_AutoLoad(4);
                tag = U32_AT(&data[12]);
                if (tag == FOURCC('u', 'l', 't', 'i')) {
                    //tag = msAPI_VDPlayer_BMReadBytes_AutoLoad(4);
                    tag = U32_AT(&data[16]);
                    if (tag == FOURCC('r', 'a', 't', 'e')) {
                        //tag = msAPI_VDPlayer_BMReadBytes_AutoLoad(4);
                        tag = U32_AT(&data[20]);
                        if (tag == FOURCC('-', 'r', 'e', 'a')) {
                            //tag = msAPI_VDPlayer_BMReadBytes_AutoLoad(4);
                            tag = U32_AT(&data[24]);
                            if (tag == FOURCC('l', 'v', 'i', 'd')) {
                                //tag = msAPI_VDPlayer_BMReadBytes_AutoLoad(2);
                                tag = U16_AT(&data[28]);
                                if (tag == FOURCC(0, 0, 'e', 'o')) {
                                    ALOGW("video/x-pn-multirate-realvideo\n");
                                    //msAPI_RMFP_SetAVStreamArg(u8StreamIndex, RM_STREAM_ARG_STREAM_TYPE, RM_VIDEO_MULTI_STREAM);
                                    return RM_VIDEO_MULTI_STREAM;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return RM_UNDEFINED_STREAM;
}

EN_RM_StreamType RMExtractor::CheckLogical(off64_t offset, size_t size) {
    uint32_t tag;
    sp<ABuffer> buffer = new ABuffer(size);
    ssize_t n = mDataSource->readAt(offset, buffer->data(), buffer->size());

    const uint8_t *data = buffer->data();

    //uint32_t tag = msAPI_VDPlayer_BMReadBytes_AutoLoad(4);
    tag = U32_AT(&data[0]);

    if (tag == FOURCC('l', 'o', 'g', 'i')) { // "logical-audio/x-pn-multirate-realaudio"
        //tag = msAPI_VDPlayer_BMReadBytes_AutoLoad(4);
        tag = U32_AT(&data[4]);
        if (tag == FOURCC('c', 'a', 'l', '-')) {
            //tag = msAPI_VDPlayer_BMReadBytes_AutoLoad(4);
            tag = U32_AT(&data[8]);
            if (tag == FOURCC('a', 'u', 'd', 'i')) {
                //tag = msAPI_VDPlayer_BMReadBytes_AutoLoad(4);
                tag = U32_AT(&data[12]);
                if (tag == FOURCC('o', '/', 'x', '-')) {
                    //tag = msAPI_VDPlayer_BMReadBytes_AutoLoad(4);
                    tag = U32_AT(&data[16]);
                    if (tag == FOURCC('p', 'n', '-', 'm')) {
                        //tag = msAPI_VDPlayer_BMReadBytes_AutoLoad(4);
                        tag = U32_AT(&data[20]);
                        if (tag == FOURCC('u', 'l', 't', 'i')) {
                            //tag = msAPI_VDPlayer_BMReadBytes_AutoLoad(4);
                            tag = U32_AT(&data[24]);
                            if (tag == FOURCC('r', 'a', 't', 'e')) {
                                //tag = msAPI_VDPlayer_BMReadBytes_AutoLoad(4);
                                tag = U32_AT(&data[28]);
                                if (tag == FOURCC('-', 'r', 'e', 'a')) {
                                    //tag = msAPI_VDPlayer_BMReadBytes_AutoLoad(4);
                                    tag = U32_AT(&data[32]);
                                    if (tag == FOURCC('l', 'a', 'u', 'd')) {
                                        //tag = msAPI_VDPlayer_BMReadBytes_AutoLoad(2);
                                        tag = U16_AT(&data[36]);
                                        if (tag == FOURCC(0, 0, 'i', 'o')) {
                                            ALOGW("logical-audio/x-pn-multirate-realaudio\n");
                                            //msAPI_RMFP_SetAVStreamArg(u8StreamIndex, RM_STREAM_ARG_STREAM_TYPE, RM_AUDIO_LOGICAL_STREAM);
                                            return RM_AUDIO_LOGICAL_STREAM;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            } else if (tag == FOURCC('v', 'i', 'd', 'e')) {    // "logical-video/x-pn-multirate-realvideo"
                //tag = msAPI_VDPlayer_BMReadBytes_AutoLoad(4);
                tag = U32_AT(&data[12]);
                if (tag == FOURCC('o', '/', 'x', '-')) {
                    //tag = msAPI_VDPlayer_BMReadBytes_AutoLoad(4);
                    tag = U32_AT(&data[16]);
                    if (tag == FOURCC('p', 'n', '-', 'm')) {
                        //tag = msAPI_VDPlayer_BMReadBytes_AutoLoad(4);
                        tag = U32_AT(&data[20]);
                        if (tag == FOURCC('u', 'l', 't', 'i')) {
                            //tag = msAPI_VDPlayer_BMReadBytes_AutoLoad(4);
                            tag = U32_AT(&data[24]);
                            if (tag == FOURCC('r', 'a', 't', 'e')) {
                                //tag = msAPI_VDPlayer_BMReadBytes_AutoLoad(4);
                                tag = U32_AT(&data[28]);
                                if (tag == FOURCC('-', 'r', 'e', 'a')) {
                                    //tag = msAPI_VDPlayer_BMReadBytes_AutoLoad(4);
                                    tag = U32_AT(&data[32]);
                                    if (tag == FOURCC('l', 'v', 'i', 'd')) {
                                        //tag = msAPI_VDPlayer_BMReadBytes_AutoLoad(2);
                                        tag = U16_AT(&data[36]);
                                        if (tag == FOURCC(0, 0, 'e', 'o')) {
                                            ALOGW("logical-video/x-pn-multirate-realvideo\n");
                                            //msAPI_RMFP_SetAVStreamArg(u8StreamIndex, RM_STREAM_ARG_STREAM_TYPE, RM_VIDEO_LOGICAL_STREAM);
                                            return  RM_VIDEO_LOGICAL_STREAM;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    //msAPI_VDPlayer_BMFlushBytes(u32size); // flush not read bits
    ALOGW("undefined logical stream\n");
    return RM_UNDEFINED_STREAM;
}

uint32_t RMExtractor::mc_pop(uint32_t x) {
    uint32_t n = 0;

    while (x) {
        ++n;
        x &= (x - 1); // repeatedly clear rightmost 1-bit

    }
    return n;
}

status_t RMExtractor::ra8lbr_Opq(off64_t offset, size_t size, uint32_t channelTemp) {
    off64_t tmpOffset = 0;
    uint32_t tag, major = 0, minor = 0, channelMask = 0, channelMaskTemp = 0, sampleChannel = 0, temp = 0;
    sp<ABuffer> buffer = new ABuffer(size);
    ssize_t n = mDataSource->readAt(offset, buffer->data(), buffer->size());
    uint8_t numCodecs = 0;
    const uint8_t *data = buffer->data();

    uint8_t audioTrackIndex = mRMContext.nbAudioTracks;

    while (channelTemp > mc_pop(channelMask)) {
        // major version
        major = data[tmpOffset];
        ALOGV("major = %d\n", major);
        tmpOffset += 1;

        // reserved
        tag = U16_AT(&data[tmpOffset]);
        ALOGV("reserved = %d\n", tag);
        tmpOffset += 2;

        // minor version
        minor = data[tmpOffset];
        ALOGV("minor = %d\n", minor);
        tmpOffset += 1;

        // num_samples
        tag = U16_AT(&data[tmpOffset]);
        if (sampleChannel == 0) {
            sampleChannel = tag ;
        }
        mRMContext.AudioTracks[audioTrackIndex].channelInfo[numCodecs].numOfSamples = tag;
        ALOGV("num_samples = %d\n", tag);
        tmpOffset += 2;

        // num_regions
        tag = U16_AT(&data[tmpOffset]);
        mRMContext.AudioTracks[audioTrackIndex].channelInfo[numCodecs].numOfRegions = tag;
        mRMContext.AudioTracks[audioTrackIndex].LBR.numOfRegions[numCodecs] = tag;

        ALOGV("num_regions = %d\n", tag);
        tmpOffset += 2;

        if ((major >= 1 && minor >= 3) || (channelTemp > 2)) {
            // delay
            tag = U32_AT(&data[tmpOffset]);
            tmpOffset += 4;
            ALOGV("delay = %d\n", tag);

            // cplStart
            tag = U16_AT(&data[tmpOffset]);
            mRMContext.AudioTracks[audioTrackIndex].channelInfo[numCodecs].CPLStart = tag;
            mRMContext.AudioTracks[audioTrackIndex].LBR.CPLStart[numCodecs] = tag;
            ALOGV("cplStart = %d\n", tag);
            tmpOffset += 2;

            // cplQbits
            tag = U16_AT(&data[tmpOffset]);
            mRMContext.AudioTracks[audioTrackIndex].channelInfo[numCodecs].CPLQBits = tag;
            mRMContext.AudioTracks[audioTrackIndex].LBR.CPLQBits[numCodecs] = tag;
            ALOGV("cplQbits = %d\n", tag);
            tmpOffset += 2;
        }


        if ((major >= 2 && minor == 0) || (channelTemp > 2)) {
            // channel_mask
            tag = U32_AT(&data[tmpOffset]);
            channelMaskTemp = tag ;
            ALOGV("channel_mask = %d\n", tag);
            tmpOffset += 4;
        }

        if (channelTemp == 1)
            channelMaskTemp = 0x00004;
        else if (channelTemp == 2)
            channelMaskTemp = 0x00003;

        channelMask |= channelMaskTemp;
        mRMContext.AudioTracks[audioTrackIndex].LBR.channelMask[numCodecs] = (uint16_t)mc_pop(channelMaskTemp);
        temp = mRMContext.AudioTracks[audioTrackIndex].LBR.channelMask[numCodecs];
        ALOGV("RM_LBR_NUM_CHANNEL(0) %d = %d\n", numCodecs, mc_pop(channelMaskTemp));
        numCodecs++;
    }

    temp = mRMContext.AudioTracks[audioTrackIndex].LBR.channelMask[0];
    mRMContext.AudioTracks[audioTrackIndex].LBR.sampleChannel = (sampleChannel / temp);
    ALOGV("RM_LBR_SAMPLE_CHANNEL = %d\n", mRMContext.AudioTracks[audioTrackIndex].LBR.sampleChannel);
    tag = mRMContext.AudioTracks[audioTrackIndex].samplerate;
    mRMContext.AudioTracks[audioTrackIndex].LBR.sampleRate = tag;
    ALOGV("RM_LBR_SAMPLE_RATE = %d\n", mRMContext.AudioTracks[audioTrackIndex].LBR.sampleRate);
    mRMContext.AudioTracks[audioTrackIndex].LBR.numOfCodecID = numCodecs;
    return 0;
}

status_t RMExtractor::GetAudioFmt5(off64_t offset, size_t size) {
    off64_t tmpOffset = 0;
    uint32_t tag, factor, blockSize, frameSize, numSBF, codecID, channelTemp;
    uint8_t audioTrackIndex = mRMContext.nbAudioTracks;


    sp<ABuffer> buffer = new ABuffer(size);
    ssize_t n = mDataSource->readAt(offset, buffer->data(), buffer->size());

    const uint8_t *data = buffer->data();

    // flush reserved, version, reversion, header bytes, flavor, granularity
    tmpOffset += (10 + 2 * 4 + 4);

    // total_bytes
    tag = U32_AT(&data[tmpOffset]);
    ALOGV("Radio data size = %d\n", tag);
    tmpOffset += 4;

    // bytes_per_minute
    tag = U32_AT(&data[tmpOffset]);
    ALOGV("Bytes per minute = %d\n", tag);
    tmpOffset += 4;

    // bytes_per_minute2
    tag = U32_AT(&data[tmpOffset]);
    ALOGV("Bytes per minute2 = %d\n", tag);
    tmpOffset += 4;

    // interleave_factor
    factor = U16_AT(&data[tmpOffset]);
    mRMContext.AudioTracks[audioTrackIndex].factor = factor;
    ALOGV("Interleave factor = %d\n", factor);
    tmpOffset += 2;

    // interleave_block_size
    blockSize = U16_AT(&data[tmpOffset]);
    mRMContext.AudioTracks[audioTrackIndex].blockSize = blockSize;
    ALOGV("Interleave block size = %d\n", blockSize);
    tmpOffset += 2;

    // codec_frame_size
    frameSize = U16_AT(&data[tmpOffset]);
    mRMContext.AudioTracks[audioTrackIndex].frameSize = frameSize;
    mRMContext.AudioTracks[audioTrackIndex].LBR.frameSize[0] = frameSize;

    ALOGV("Codec frame size = %d\n", frameSize);
    tmpOffset += 2;

    // user_data
    tag = U32_AT(&data[tmpOffset]);
    ALOGV("user_data = %d\n", tag);
    tmpOffset += 4;

    // sample_rate
    tag = U32_AT(&data[tmpOffset]);
    tag >>= 16;
    mRMContext.AudioTracks[audioTrackIndex].samplerate = tag;
    ALOGV("Sample rate = %d\n", tag);
    tmpOffset += 4;

    // actual_sample_rate
    tag = U32_AT(&data[tmpOffset]);
    tag >>= 16;
    ALOGV("Actual sample rate = %d\n", tag);
    tmpOffset += 4;

    // sample_size
    tag = U16_AT(&data[tmpOffset]);
    mRMContext.AudioTracks[audioTrackIndex].samplesize = tag;
    ALOGV("Sample size = %d\n", tag);
    tmpOffset += 2;

    // num_channels
    //tag = msAPI_VDPlayer_BMReadBytes_AutoLoad(2);
    tag = U16_AT(&data[tmpOffset]);
    channelTemp = tag ;
    mRMContext.AudioTracks[audioTrackIndex].channel = tag;
    ALOGV("Number of channels = %d\n", tag);
    tmpOffset += 2;

    // interleaver_id
    tag = U32_AT(&data[tmpOffset]);
    ALOGV("Interleaver id = %d\n", tag);
    tmpOffset += 4;

    // codec_id
    codecID = U32_AT(&data[tmpOffset]);
    ALOGV("Codec id = 0x%x\n", codecID);
    tmpOffset += 4;

#if 0
    switch (codecID) {
    case RA_CODEC_ID_SIPR:
        {
            ALOGV("RA_CODEC_ID_SIPR\n");
            //tag = E_VDP_CODEC_ID_SIPR;
        }
        break;
    case RA_CODEC_ID_COOK:
        {
            ALOGV("RA_CODEC_ID_COOK\n");
            //  mRMContext.mMeta->setCString(kKeyMIMEType, );
            //tag = E_VDP_CODEC_ID_COOK;
        }
        break;
    case RA_CODEC_ID_ATRC:
        {
            ALOGV("RA_CODEC_ID_ATRC\n");
            //tag = E_VDP_CODEC_ID_ATRC;
        }
        break;
    case RA_CODEC_ID_RAAC:
        {
            ALOGV("RA_CODEC_ID_RAAC\n");
            //tag = E_VDP_CODEC_ID_RAAC;
        }
        break;
    case RA_CODEC_ID_RACP:
        {
            ALOGV("RA_CODEC_ID_RACP\n");
            //tag = E_VDP_CODEC_ID_RACP;
            //tag = E_VDP_CODEC_ID_RAAC;
        }
        break;
    default:
        {
            ALOGV("default\n");
            //tag = E_VDP_CODEC_ID_NONE;
        }
        break;
    }
#endif
    ALOGV("Codec id = 0x%d\n", codecID);
    mRMContext.AudioTracks[audioTrackIndex].codecID = codecID;

    // is_interleaved_flag
    tag = data[tmpOffset];
    ALOGV("is_interleaved_flag = %d\n", tag);
    tmpOffset += 1;

    // can_copy_flag
    tag = data[tmpOffset];
    ALOGV("can_copy_flag = %d\n", tag);
    tmpOffset += 1;

    // stream_type
    tag = data[tmpOffset];
    mRMContext.AudioTracks[audioTrackIndex].type = tag;
    ALOGV("stream_type = 0x%x\n", tag);
    tmpOffset += 1;

    // has_interleave_pattern_flag
    tag = data[tmpOffset];
    ALOGV("has_interleave_pattern_flag = 0x%x\n", tag);
    tmpOffset += 1;

    if (tag != 0) {
        numSBF = (factor * blockSize) / frameSize;
        // we don't handle specific interleave pattern noe
        tmpOffset += numSBF;
    }

    // codec_opaque_data_size
    tag = U32_AT(&data[tmpOffset]);
    ALOGV("codec_opaque_data_size = %d\n", tag);
    tmpOffset += 4;

    switch (codecID) {
    case FOURCC('c', 'o', 'o', 'k'):     // lbr
        ra8lbr_Opq(offset + tmpOffset, tag, channelTemp);
        tag = 0;
        break;
    case FOURCC('a', 't', 'r', 'c'):    // hbr
        ALOGW("Codec id = atrc\n");
        break;
    case FOURCC('r', 'a', 'a', 'c'):    // AAC
        ALOGW("Codec id = raac\n");
        break;
    case FOURCC('r', 'a', 'c', 'p'):    // AAC
        ALOGW("Codec id = racp\n");
        break;
    case FOURCC('s', 'i', 'p', 'r'):    // RealAudio Voice
        ALOGW("Codec id = sipr\n");
        break;
    default:
        ALOGW("Codec id = unknow \n");
        break;
    }
    return OK;
}

status_t RMExtractor::getAudioHeader(off64_t offset, size_t size) {
    uint32_t tag;
    sp<ABuffer> buffer = new ABuffer(size);
    ssize_t n = mDataSource->readAt(offset, buffer->data(), buffer->size());

    const uint8_t *data = buffer->data();

    // header version
    tag = U16_AT(&data[0]);

    ALOGV("Audio header version = %d\n", tag);

    if (tag == 5) {
        GetAudioFmt5(offset + 2, size - 2);
    }

    return OK;
}

status_t RMExtractor::getAudioOpq(off64_t offset, size_t size) {
    off64_t tmpOffset = 0;
    uint32_t tag, headerSize, i;
    sp<ABuffer> buffer = new ABuffer(size);
    ssize_t n = mDataSource->readAt(offset, buffer->data(), buffer->size());

    const uint8_t *data = buffer->data();

    // single or multiple
    tag = U32_AT(&data[0]);
    tmpOffset += 4;

    ALOGV("single_or_multiple = 0x%x\n", tag);

    if (tag == FOURCC('M', 'L', 'T', 'I')) {  // 0x4D4C5449
        // number of rules
        tag = U16_AT(&data[4]);
        ALOGV("Number of rules = %d\n", tag);
        tmpOffset += 2;

        // flush rule to substream map
        tmpOffset += (tag * 2);

        // number of substreams
        tag = U16_AT(&data[tmpOffset]);
        ALOGV("Number of substreams = %d\n", tag);
        tmpOffset += 2;
        for (i = 0; i < tag; i++) {
            // substream header size
            headerSize = U32_AT(&data[tmpOffset]);
            tmpOffset += 4;

            //flush real_audio_id == "0x2E7261FD"
            tmpOffset += 4;
            getAudioHeader(offset + tmpOffset, headerSize);
            tmpOffset += (headerSize - 4);
        }
    } else if (tag == 0x2E7261FD) { // single
        size -= 4;
        getAudioHeader(offset + tmpOffset, size);
    }
    return OK;
}

status_t RMExtractor::getVideoCodecOpq(off64_t offset, size_t size, uint32_t codecID, uint32_t width, uint32_t height) {
    off64_t tmpOffset = 0;
    uint32_t tag, i = 0, SPOExtra = 0, dimWidth = 0, dimHeight = 0;
    uint32_t encodeSize = 0, largestPels = 0, largestLines = 0, numResampleImgSize = 0;
    uint8_t majorVersion = 0, minorVersion = 0;

    sp<ABuffer> buffer = new ABuffer(size);
    ssize_t n = mDataSource->readAt(offset, buffer->data(), buffer->size());

    const uint8_t *data = buffer->data();

    switch (codecID) {
    case RM_RV30VIDEO_ID:
        // u32ECCMask = 0x20;
        break;
    case RM_RV40VIDEO_ID:
        // u32ECCMask = 0x80;
        break;
    case RM_RV89COMBO_ID:
        // u32ECCMask = 0;
        break;
    default:
        ALOGW("video codec not supported\n");
        return OK;    // codec not supported
    }

    if (size > 0) {
        // SPO_extra_flags
        SPOExtra = U32_AT(&data[tmpOffset]);
        ALOGV("SPO_extra_flags = %d\n", SPOExtra);
        tmpOffset += 4;

        //bitstream_major_verion, bitstream_minor_version, bitstream_release_version, reserved, frontend_version
        tag = U32_AT(&data[tmpOffset]);
        // bitstream_major_version
        majorVersion = (uint8_t)((tag >> 28) & 0xF);
        // bitstream_minor_version
        minorVersion = (uint8_t)((tag >> 20) & 0xFF);
        tmpOffset += 4;

        // Decode extra opaque data
        if (!(minorVersion & RAW_BITSTREAM_MINOR_VERSION)) {
            if (majorVersion == RV30_MAJOR_BITSTREAM_VERSION) { //u8MajorVersion == RV20_MAJOR_BITSTREAM_VERSION  // we don't support RV20
                // RV8
                // Reference Picture Resampling sizes
                numResampleImgSize = (SPOExtra & RV40_SPO_BITS_NUMRESAMPLE_IMAGES) >> RV40_SPO_BITS_NUMRESAMPLE_IMAGES_SHIFT;
                ALOGV("RPR size = %d\n", numResampleImgSize);

                for (i = 1; i < numResampleImgSize + 1; i++) {
                    // need a array to get dimension
                    tag = data[tmpOffset];
                    dimWidth = (tag << 2);
                    mRMContext.streamRPR.dimInfo[i].dimWidth = dimWidth;
                    ALOGV("Resample width = %d\n", dimWidth);
                    tmpOffset += 1;

                    tag = data[tmpOffset];
                    dimHeight = (tag << 2);
                    mRMContext.streamRPR.dimInfo[i].dimHeight= dimHeight;
                    ALOGV("Resample height = %d\n", dimHeight);
                    tmpOffset += 1;
                }
                mRMContext.streamRPR.numResampleImgSize = numResampleImgSize + 1;
            } else if (majorVersion == RV40_MAJOR_BITSTREAM_VERSION) {
                mRMContext.streamRPR.numResampleImgSize = 1;
                // RV9 largest encoded dimensions
                if (size >= 12) {
                    // encode size
                    encodeSize = U32_AT(&data[tmpOffset]);
                    ALOGV("Video encode size = %d\n", encodeSize);
                    tmpOffset += 4;
                }
            }
        }
    }

    // Set largest encoded dimensions
    largestPels = ((encodeSize >> 14) & 0x3FFFC);
    if (largestPels == 0)
        largestPels= width;
    ALOGV("LargestPels = %d\n", largestPels);
    largestLines = ((encodeSize << 2) & 0x3FFFC);
    if (largestLines == 0)
        largestLines = height;
    ALOGV("LargestLines = %d\n", largestLines);

    return OK;
}

status_t RMExtractor::getVideoHeader(off64_t offset, size_t size) {
    off64_t tmpOffset = 0;
    uint32_t tag, codecID, width, height, fmt;

    sp<ABuffer> buffer = new ABuffer(size);
    ssize_t n = mDataSource->readAt(offset, buffer->data(), buffer->size());

    const uint8_t *data = buffer->data();
    uint8_t videoTrackIndex = mRMContext.nbVideoTracks;

    // media object format, 4cc
    fmt = U32_AT(&data[tmpOffset]);
    ALOGV("Media object format = 0x%x\n", fmt);
    tmpOffset += 4;

    // codec id, 4cc
    codecID = U32_AT(&data[tmpOffset]);
    if (codecID == RM_RVTR_RV30_ID)    //  Fix up, the same as sample code
        codecID = RM_RV30VIDEO_ID;
    tmpOffset += 4;

#if 0
    switch (codecID) {
    case RM_RV30VIDEO_ID:
        //tag = E_VDP_CODEC_ID_RV30;
        break;
    case RM_RV40VIDEO_ID:
        //tag = E_VDP_CODEC_ID_RV40;
        break;
    default:
        //tag = E_VDP_CODEC_ID_NONE;
        break;
    }
#endif
    mRMContext.VideoTracks[videoTrackIndex].codecID = codecID;
    ALOGV("Codec ID = 0x%x\n", codecID);

    // frame width
    width = U16_AT(&data[tmpOffset]);
    mRMContext.VideoTracks[videoTrackIndex].width = width;
    ALOGV("Frame width = %d\n", width);
    tmpOffset += 2;

    // frame height
    height = U16_AT(&data[tmpOffset]);
    mRMContext.VideoTracks[videoTrackIndex].height = height;
    ALOGV("Frame height = %d\n", height);
    tmpOffset += 2;

    // bit count
    tag = U16_AT(&data[tmpOffset]);
    ALOGV("Bit count = %d\n", tag);
    tmpOffset += 2;

    // pad width, number of columns of padding on right to get 16 x 16 block
    tag = U16_AT(&data[tmpOffset]);
    ALOGV("Pad width= %d\n", tag);
    tmpOffset += 2;

    // pad height, number of rows of padding on bottom to get 16 x 16 block
    tag = U16_AT(&data[tmpOffset]);
    ALOGV("Pad height = %d\n", tag);
    tmpOffset += 2;

    if (fmt == 0x5649444F) { //VIDO
        // frame rate, fps
        tag = U32_AT(&data[tmpOffset]);
        mRMContext.VideoTracks[videoTrackIndex].frameRate = tag;
        mRMContext.VideoTracks[videoTrackIndex].frameRateBase = 0x10000;
        ALOGV("Frame rate = %d\n", tag);
        tmpOffset += 4;
    }

    ALOGV("Video codec opaque data size = %d\n", (uint32_t)(size - tmpOffset));
    getVideoCodecOpq(offset + tmpOffset, size - tmpOffset, codecID, width, height);

    return OK;
}

status_t RMExtractor::getVideoOpq(off64_t offset, size_t size) {
    off64_t tmpOffset = 0;
    uint32_t tag, headerLen, i;

    sp<ABuffer> buffer = new ABuffer(size);
    ssize_t n = mDataSource->readAt(offset, buffer->data(), buffer->size());

    const uint8_t *data = buffer->data();

    // single_len_or_mult_id
    headerLen = U32_AT(&data[tmpOffset]);
    tmpOffset += 4;
    ALOGV("single_len_or_mult_id = 0x%x\n", headerLen);

    if (headerLen == 0x4D4C5449) {        // "MLTI"
        // number of rules
        tag = U16_AT(&data[tmpOffset]);
        ALOGV("Number of rules = %d\n", tag);
        tmpOffset += 2;

        // flush rule to substream map
        tmpOffset += tag * 2;

        // number of substreams
        tag = U16_AT(&data[tmpOffset]);
        ALOGV("Number of substreams = %d\n", tag);
        tmpOffset += 2;

        for (i = 0; i < tag; i++) {
            // header length
            headerLen = U32_AT(&data[tmpOffset]);
            tmpOffset += 4;
            // header length // note: sample code is different with spec
            headerLen = U32_AT(&data[tmpOffset]);
            tmpOffset += 4;
            getVideoHeader(offset + tmpOffset, headerLen - 4);
        }
    } else {     // single
        // note: sample code is different with spec
        getVideoHeader(offset + tmpOffset, headerLen - 4);
    }
    return OK;
}

int32_t RMExtractor::Deinterleave_Genr(uint16_t interleaveFactor, uint16_t interleaveBlockSize, uint16_t frameSize)
{
    // Number of frames per superblock
    uint16_t numFrames = interleaveFactor * (interleaveBlockSize / frameSize);
    uint16_t framesPerBlock = interleaveBlockSize / frameSize;
    uint16_t count = 0;
    uint16_t blockIndx = 0;
    uint16_t frameIndx = 0;
    uint16_t tmpIdx = 0;
    bool even = true;
    uint16_t i = 0;

    mRMContext.numFrames = numFrames;
    if (interleaveFactor == 1) {
        for (i = 0; i < numFrames; i++) {
            mRMContext.GENRPattern[i] = i;
        }
    } else {
        while (count < numFrames) {
            tmpIdx = blockIndx * framesPerBlock + frameIndx;
            if (tmpIdx >= numFrames) {
                ALOGW("Deinterleave genr error\n");
                return -1;
            }

            mRMContext.GENRPattern[tmpIdx] = count;

            count++;
            blockIndx += 2;
            if (blockIndx >= interleaveFactor) {
                if (even) {
                    even = false;
                    blockIndx = 1;
                } else {
                    even = true;
                    blockIndx = 0;
                    frameIndx++;
                }
            }
        }

        count = 0;
        ALOGV("GENR:\n");
        while (count < numFrames) {
            ALOGV("%d: %d\n", count, mRMContext.GENRPattern[count]);
            count++;
        }
    }

    return 0;
}

status_t RMExtractor::ParserPROP(off64_t offset, size_t size) {
    uint32_t tag;

    sp<ABuffer> buffer = new ABuffer(size);
    ssize_t n = mDataSource->readAt(offset, buffer->data(), buffer->size());

    const uint8_t *data = buffer->data();

    tag = U16_AT(&data[8]); // object version

    if (tag == 0) {
        // max bitrate
        tag = U32_AT(&data[10]);
        ALOGV("Max bitrate = %d\n", tag);

        // avg bitrate
        tag = U32_AT(&data[14]);
        ALOGV("Avg bitrate = %d\n", tag);

        // max packet size
        tag = U32_AT(&data[18]);
        mRMContext.mMaxPacketSize = tag;
        ALOGV("Max packet size = %d\n", tag);

        // avg packet size
        tag = U32_AT(&data[22]);
        mRMContext.mAvgPacketSize = tag;
        ALOGV("Avg packet size = %d\n", tag);

        // number of packets
        tag = U32_AT(&data[26]);
        ALOGV("Number of packets = %d\n", tag);

        // duration
        tag = U32_AT(&data[30]);
        if (tag > mRMContext.VideoTracks[0].duration)
            mRMContext.VideoTracks[0].duration = tag;
        mRMContext.AudioTracks[0].duration = tag;
        ALOGV("Duration = %d\n", tag);

        // preroll
        tag = U32_AT(&data[34]);
        ALOGV("Preroll = %d\n", tag);

        // index offset
        tag = U32_AT(&data[38]);
        mRMContext.indexOffset = tag;
        ALOGV("Index offset = %d\n", tag);

        // data offset
        tag = U32_AT(&data[42]);
        mRMContext.PKT_startPosition = tag;
        ALOGV("Data offset = %d\n", tag);

        // number of streams
        tag = U16_AT(&data[46]);
        ALOGV("Number of streams = %d\n", tag);

        // flags
        tag = U16_AT(&data[48]);
        ALOGV("flags = %d\n", tag);

    }
    return OK;
}


status_t RMExtractor::ParserMDPR(off64_t offset, size_t size) {
    off64_t tmpOffset = 0;
    uint32_t tag, streamID, bitrate, duration;
    uint16_t interleaveFactor = 0;
    uint16_t interleaveBlockSize = 0;
    uint16_t codecFrameSize = 0;
    uint8_t streamIndex = 0;
    uint8_t videoTrackIndex = 0;
    uint8_t audioTrackIndex = 0;
    EN_RM_StreamType streamType = RM_UNDEFINED_STREAM;
    //EN_STREAM_TYPE enStreamType = STREAM_OTHER;

    streamIndex = mRMContext.nbStreams;
    videoTrackIndex =  mRMContext.nbVideoTracks;
    audioTrackIndex = mRMContext.nbAudioTracks;


    sp<ABuffer> buffer = new ABuffer(size);
    ssize_t n = mDataSource->readAt(offset, buffer->data(), buffer->size());

    const uint8_t *data = buffer->data();

    tag = U16_AT(&data[8]); // object version

    if (tag == 0) {
        // stream number (stream ID)
        streamID = U16_AT(&data[10]);
        ALOGV("Stream number = %d\n", streamID);

        // max bitrate
        tag = U32_AT(&data[12]);
        ALOGV("Max bitrate = %d\n", tag);

        // avg bitrate
        bitrate = U32_AT(&data[16]);
        ALOGV("Avg bitrate = %d\n", bitrate);

        // max packet size
        tag = U32_AT(&data[20]);
        ALOGV("Max packet size = %d\n", tag);

        // avg packet size
        tag = U32_AT(&data[24]);
        ALOGV("Avg packet size = %d\n", tag);

        // start time
        tag = U32_AT(&data[28]);
        ALOGV("Start time = %d\n", tag);

        // preroll
        tag = U32_AT(&data[32]);
        ALOGV("Preroll = %d\n", tag);

        // duration
        duration = U32_AT(&data[36]);
        ALOGV("Duration = %d\n", duration);

        // stream name size
        tag = data[40];
        ALOGV("Stream name size = %d\n", tag);

        // stream name
        tmpOffset = 41 + tag;

        // mime type size
        tag = data[tmpOffset];
        ALOGV("Mime type size = %d\n", tag);

        tmpOffset++;

        switch (tag) {
        case 20: // "audio/x-pn-realaudio" or "video/x-pn-realvideo"
            streamType = CheckPhysical(offset + tmpOffset, tag);
            break;
        case 30: // "audio/x-pn-multirate-realaudio" or "video/x-pn-multirate-realvideo"
            streamType = CheckMultiPhysical(offset + tmpOffset, tag);
            break;
        case 38: // "logical-audio/x-pn-multirate-realaudio" or "logical-video/x-pn-multirate-realvideo"
            streamType = CheckLogical(offset + tmpOffset, tag);
            break;
        default: // // not a useful mime type
            ALOGW("Unknow stream\n");
            break;
        }
        tmpOffset += tag;

        tag = U32_AT(&data[tmpOffset]); // type specific Length
        ALOGV("Type specific length = %d\n", tag);

        tmpOffset += 4;

        switch (streamType) {
        case RM_AUDIO_SINGLE_STREAM:
            {
                mRMContext.AudioTracks[audioTrackIndex].bitrate = bitrate;
                mRMContext.AudioTracks[audioTrackIndex].streamID = streamID;
                getAudioOpq(offset + tmpOffset, tag);
                tmpOffset += tag;
                ALOGV("Audio stream number = %d\n", streamID);
                if (mRMContext.AudioTracks[audioTrackIndex].codecID == RA_CODEC_ID_COOK) { // LBR
                    ALOGV("Audio Codec : LBR\n");

                    interleaveFactor = (uint16_t)mRMContext.AudioTracks[audioTrackIndex].factor;
                    ALOGV("Interleave factor = %d\n", interleaveFactor);

                    interleaveBlockSize = (uint16_t)mRMContext.AudioTracks[audioTrackIndex].blockSize;
                    ALOGV("Interleave block size = %d\n", interleaveBlockSize);

                    codecFrameSize = (uint16_t)mRMContext.AudioTracks[audioTrackIndex].frameSize;
                    ALOGV("Codec frame size = %d\n", codecFrameSize);

                    mRMContext.AudioTracks[audioTrackIndex].audioFramesPerBlock = (uint8_t)(interleaveBlockSize / codecFrameSize);
                    ALOGV("u8FramesPerBlock = %d\n", mRMContext.AudioTracks[audioTrackIndex].audioFramesPerBlock);
                    mRMContext.AudioTracks[audioTrackIndex].audioFrameSize = codecFrameSize;
                    ALOGV("u16CodecFrameSize = %d\n", mRMContext.AudioTracks[audioTrackIndex].audioFrameSize);

                    Deinterleave_Genr(interleaveFactor, interleaveBlockSize, codecFrameSize);
                    mRMContext.AudioTracks[audioTrackIndex].audioSuperBlockSize = mRMContext.AudioTracks[audioTrackIndex].audioFrameSize * mRMContext.numFrames;
                    ALOGV("SuperBlockSize = %d\n", mRMContext.AudioTracks[audioTrackIndex].audioSuperBlockSize);
                    mRMContext.nbAudioTracks++;
                } else if (mRMContext.AudioTracks[audioTrackIndex].codecID == RA_CODEC_ID_RAAC) {    // AAC-LC
                    mRMContext.nbAudioTracks++;
                    ALOGV("Audio Codec : RAAC\n");
                } else if (mRMContext.AudioTracks[audioTrackIndex].codecID == RA_CODEC_ID_RACP) {    // HE-AAC
                    mRMContext.nbAudioTracks++;
                    ALOGV("Audio Codec : RACP\n");
                } else {
                    ALOGW("Unsupport Audio Codec\n");
                    audioCodecSupported = false;
                    goto GetMDPR_End;
                }
            }
            break;
        case RM_VIDEO_SINGLE_STREAM:
            {
                mRMContext.VideoTracks[videoTrackIndex].streamID = streamID;
                mRMContext.VideoTracks[videoTrackIndex].duration = duration;

                getVideoOpq(offset + tmpOffset, tag);

                if ((mRMContext.VideoTracks[videoTrackIndex].codecID  == RM_RV30VIDEO_ID) ||
                    (mRMContext.VideoTracks[videoTrackIndex].codecID  == RM_RV40VIDEO_ID)) {
                    ALOGV("Video Codec : RV30 (RV8)/ RV40 (RV9)\n");
                    mRMContext.streamRPR.dimInfo[0].dimWidth = mRMContext.VideoTracks[videoTrackIndex].width;
                    mRMContext.streamRPR.dimInfo[0].dimHeight = mRMContext.VideoTracks[videoTrackIndex].height;
                    mRMContext.nbVideoTracks++;
                } else {
                    ALOGW("Unsupport Video Codec\n");
                    goto GetMDPR_End;
                }
            }
            break;
        case RM_AUDIO_MULTI_STREAM:
        case RM_VIDEO_MULTI_STREAM:
        case RM_AUDIO_LOGICAL_STREAM:
        case RM_VIDEO_LOGICAL_STREAM:
        case RM_UNDEFINED_STREAM:
        default:
            // flush not read bits
            tmpOffset += tag;
            goto GetMDPR_End;
        }
        mRMContext.nbStreams++;
    }
    GetMDPR_End:
    return OK;
}

status_t RMExtractor::ParserDATA(off64_t offset, size_t size) {
    off64_t tmpOffset = 0;
    uint32_t tag;

    sp<ABuffer> buffer = new ABuffer(size);
    ssize_t n = mDataSource->readAt(offset, buffer->data(), buffer->size());

    const uint8_t *data = buffer->data();

    // Object_id & ChunkSize have read outside.
    tmpOffset += 8;

    // object_version
    tag = U16_AT(&data[tmpOffset]);
    tmpOffset += 2;

    if (tag == 0) {
        // num_packets
        tag = U32_AT(&data[tmpOffset]);
        tmpOffset += 4;
        ALOGV("Number of packets %d\n", tag);

        // next_data_header
        tag = U32_AT(&data[tmpOffset]);
        tmpOffset += 4;
        ALOGV("Next Data Header %d\n", tag);

        mRMContext.PKT_startPosition += tmpOffset;
        ALOGV("Reset PKT StartPosition %x\n", (uint32_t)mRMContext.PKT_startPosition);
    }
    return OK;
}

status_t RMExtractor::parseIndex()
{
    off64_t tmpOffset = 0;
    off64_t fileSize;
    uint32_t idx;
    uint32_t tag;
    uint32_t numIndices = 0;
    uint32_t nextIndexHeader= 0;
    uint32_t size = 0;
    int32_t prePktCnt = 0;
    uint16_t streamNum;
    uint8_t tmp[10];
    uint8_t u8StreamCnt = mRMContext.nbStreams;

    mDataSource->getSize(&fileSize);

    if (mRMContext.indexOffset > fileSize) {
        ALOGW("file size < index offset! no index info\n");
        return -1;
    }
    while (1) {

        ssize_t n = mDataSource->readAt(mRMContext.indexOffset + tmpOffset, tmp, 10);

        if (n < 10) {
            return(n < 0) ? n : (ssize_t)ERROR_MALFORMED;
        }

        //-------------------------------------
        // header
        //-------------------------------------
        //flush id

        // size
        size = U32_AT(&tmp[4]);
        if (size == 0) {
            goto NOT_SUPPORT;
        }

        // version
        tag = U16_AT(&tmp[8]);

        if (tag == 0) {
            off64_t tmpOffset1 = 0;
            prePktCnt = 0;
            sp<ABuffer> buffer = new ABuffer(size);
            ssize_t n = mDataSource->readAt(mRMContext.indexOffset + tmpOffset + 10, buffer->data(), buffer->size());
            const uint8_t *data = buffer->data();

            numIndices = U32_AT(&data[tmpOffset1]);
            tmpOffset1 += 4;
            streamNum = U16_AT(&data[tmpOffset1]);
            tmpOffset1 += 2;
            nextIndexHeader = U32_AT(&data[tmpOffset1]);
            tmpOffset1 += 4;

            ALOGV("[INDEX] numIndices=%d\n", numIndices);
            ALOGV("[INDEX] streamNum=%d\n", streamNum);
            ALOGV("[INDEX] nextIndexHeader=0x%08d\n", nextIndexHeader);

            if (numIndices == 0)
                goto NOT_SUPPORT;

            if ( (streamNum < mRMContext.nbStreams) && (u8StreamCnt > 0) ) {
                ALOGV("[INDEX] Video Index Table\n");

                for (idx = 0; idx < numIndices; idx++) {
                    // version
                    tag = U16_AT(&data[tmpOffset1]);//(uint16_t)msAPI_VDPlayer_BMReadBytes_AutoLoad(2);
                    tmpOffset1 += 2;

                    if (tag == 0) {
                        mIndex[streamNum].push();
                        IndexInfo *idxInf = &mIndex[streamNum].editItemAt(mIndex[streamNum].size() - 1);

                        idxInf->mPTS = U32_AT(&data[tmpOffset1]);
                        tmpOffset1 += 4;
                        idxInf->mOffset = U32_AT(&data[tmpOffset1]);
                        tmpOffset1 += 4;
                        tag = U32_AT(&data[tmpOffset1]); // packet count
                        tmpOffset1 += 4;
                        ALOGV("record: pts=%d offset=%d\n", idxInf->mPTS , (uint32_t)idxInf->mOffset);
                        if (prePktCnt <= (int32_t)tag) {
                            prePktCnt = (int32_t)tag;
                        } else {
                            ALOGW("Index packet count error !! PrePktCnt = %d, CurPktCnt = %d\n", prePktCnt, tag);
                            goto NOT_SUPPORT;
                        }
                    } else {
                        goto NOT_SUPPORT;
                    }
                }

                u8StreamCnt -- ;
                if ( 0 == u8StreamCnt) {
                    mRMContext.mFoundIndex = true;
                    goto END;
                }
            }
        } else {
            goto NOT_SUPPORT;
        }

        // end of index section
        if (nextIndexHeader == 0) {
            break;
        }
        tmpOffset += size;

    }

    END:
    return OK;

    NOT_SUPPORT:
    ALOGW("Unsupported index object type\n");
    return BAD_VALUE;
}

status_t RMExtractor::parseHeaders() {
    uint8_t tmp[18];
    ssize_t n = mDataSource->readAt(0, tmp, 18);
    uint32_t tag, size, offset = 0;
    uint32_t headCount = 0;
    bool done = false;

    if (n < 18) {
        return(n < 0) ? n : (ssize_t)ERROR_MALFORMED;
    }

    tag = U32_AT(tmp); // .RMP
    size = U32_AT(&tmp[4]); // size

    tag = U16_AT(&tmp[8]); // version

    offset += 10;
    if (tag == 0 || tag == 1) {
        tag = U32_AT(&tmp[10]); // file version
        headCount = U32_AT(&tmp[14]); // header count
        offset += 8;
    }


    for (uint8_t i = 0; (i < headCount) && (!done); i++) {
        mDataSource->readAt(offset, tmp, 8);

        if (n < 8) {
            return(n < 0) ? n : (ssize_t)ERROR_MALFORMED;
        }

        tag = U32_AT(tmp); // object ID
        size = U32_AT(&tmp[4]); // Object size

        switch (tag) {
        case FOURCC('P', 'R', 'O', 'P'):    // Properties Header
            {
                ALOGV("--- Properties Header ---\n");
                ALOGV("Object ID = PROP\n");
                ALOGV("Chunk size = %d\n", size);
                ParserPROP(offset, size);
            }
            break;
        case FOURCC('M', 'D', 'P', 'R'):    // Media Properties Header
            {
                ALOGV("--- Media Properties Header ---\n");
                ALOGV("Object ID = MDPR\n");
                ParserMDPR(offset, size);
            }
            break;
        case FOURCC('D', 'A', 'T', 'A'):    // Data Chunk Header
            {
                ALOGV("--- Data Chunk Header ---\n");
                ALOGV("Object ID = DATA\n");

                mRMContext.PKT_endPosition = mRMContext.PKT_startPosition +  size;
                ParserDATA(offset, 20);
                done = true;
            }
            break;
        case FOURCC('C', 'O', 'N', 'T'):    // Content Description Header
            ALOGV("--- Content Description ---\n");
            ALOGV("Object ID = CONT\n");
        default:
            /* skip tag */
            ALOGW("*Unknown Header*\n");
            if ((size < 8) ||
                ((offset + size) > mRMContext.PKT_startPosition)) {
                ALOGW("[Error] bad header - jump to data section\n");
                done = true;
                break;
            }
            break;
        }
        offset += size;
    }

    ALOGV("ReadHeader finished\n");

    // Calculate Nb_Frames from duration, FrameRate and FrameRateBase.
    if (mRMContext.VideoTracks[0].frameRateBase != 0) {
        mRMContext.VideoTracks[0].nb_Frames = (uint32_t)(((uint64_t)mRMContext.VideoTracks[0].duration *
                                                          (uint64_t)mRMContext.VideoTracks[0].frameRate) / ((uint64_t)mRMContext.VideoTracks[0].frameRateBase * (uint64_t)1000));
    }
    ALOGV("nb_frame=%d\n", mRMContext.VideoTracks[0].nb_Frames);

    ALOGV("ReadHeader summary\n");
    ALOGV("Total Stream = %d\n", mRMContext.nbStreams);
    ALOGV("Total Video Stream = %d\n", mRMContext.nbVideoTracks);
    ALOGV("Total Audio Stream = %d\n", mRMContext.nbAudioTracks);

#if 0
    ALOGV("AudioTrackIDMap:\n"));
    for (i = 0; i < g_pstDemuxer->pstBitsContent->u8Nb_AudioTracks; i++) {
        ALOGV("Audio Track %d -- Stream Index %d\n", i, g_pstDemuxer->pstBitsContent->u8AudioTrackIDMap[i]));
    }
#endif
    /* check if find data section */
    if (!done) {
        return(ssize_t)ERROR_MALFORMED;
    }

    //gbIsBroken = FALSE;
    return OK;
}

RMExtractor::RMExtractor(const sp<DataSource> &dataSource)
: mDataSource(dataSource) {

    memset((size_t*)&mRMContext, 0, sizeof(ST_CONTAINER_RM));
    mInitCheck = parseHeaders();
    mInitCheck = AddTracks();

    if (mInitCheck != OK) {
        return;
    }
    if ((parseIndex() != OK) || ( mRMContext.mFoundIndex != true)) {
        for (int i =0 ; i < MAX_TOTAL_TRACKS ; i++) {
            mIndex[i].clear();
        }
    }
    mCurReadPos = mRMContext.PKT_startPosition;
}

status_t RMExtractor::AddTracks() {
    uint8_t i;

    // add video track
    for (i = 0; i < mRMContext.nbVideoTracks; i++) {
        ALOGV("codec id: %x\n", mRMContext.VideoTracks[i].codecID);
        sp<MetaData> meta = new MetaData;
        meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_VIDEO_RV);
        meta->setInt32(kKeyWidth, mRMContext.VideoTracks[i].width);
        meta->setInt32(kKeyHeight, mRMContext.VideoTracks[i].height);
        meta->setInt32(kKeyFrameRate, mRMContext.VideoTracks[i].frameRate);
        meta->setInt64(kKeyDuration, (uint64_t)1000 * mRMContext.VideoTracks[0].duration);
        mTracks.push();
        mTrackMapId[mTracks.size() - 1] = i;
        Track *track = &mTracks.editItemAt(mTracks.size() - 1);
        track->mKind = Track::VIDEO;
        track->mMeta = meta;
    }

    // add audio track
    for (i = 0; i < mRMContext.nbAudioTracks; i++) {
        ALOGV("codec id: %x\n", mRMContext.AudioTracks[i].codecID);
        sp<MetaData> meta = new MetaData;
        if (mRMContext.AudioTracks[i].codecID == RA_CODEC_ID_COOK)
            meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_COOK);
        else if (mRMContext.AudioTracks[i].codecID == RA_CODEC_ID_RAAC ||
                 mRMContext.AudioTracks[i].codecID == RA_CODEC_ID_RACP)
            meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_AAC);

        meta->setInt32(kKeySampleRate, mRMContext.AudioTracks[i].samplerate);
        meta->setInt32(kKeyChannelCount, mRMContext.AudioTracks[i].channel);
        meta->setInt64(kKeyDuration, (uint64_t)1000 * mRMContext.AudioTracks[0].duration);

        mTracks.push();
        mTrackMapId[mTracks.size() - 1] = i;
        Track *track = &mTracks.editItemAt(mTracks.size() - 1);
        track->mKind = Track::AUDIO;
        track->mMeta = meta;

        OMX_AUDIO_PARAM_RATYPE stProfileType;
        if (mRMContext.AudioTracks[i].codecID == RA_CODEC_ID_COOK) {
            //add etra data

            stProfileType.nNumCodecs = mRMContext.AudioTracks[i].LBR.numOfCodecID;
            stProfileType.nSamplePerFrame = mRMContext.AudioTracks[i].LBR.sampleChannel;
            stProfileType.nSamplingRate = mRMContext.AudioTracks[i].samplerate;
            stProfileType.nFramesPerBlock = mRMContext.AudioTracks[i].audioFramesPerBlock;
            stProfileType.eFormat = OMX_AUDIO_RA8;
            MFP_DBG_RM3(ALOGD("stProfileType.nNumCodecs = %d",stProfileType.nNumCodecs));
            MFP_DBG_RM3(ALOGD("stProfileType.nSamplePerFrame = %d",stProfileType.nSamplePerFrame));
            MFP_DBG_RM3(ALOGD("stProfileType.nSamplingRate = %d",stProfileType.nSamplingRate));

            if (mRMContext.AudioTracks[i].LBR.numOfCodecID!=1) {
                uint8_t u8PktHeader[13];
                off64_t offSize = mRMContext.PKT_startPosition;
                ssize_t n = mDataSource->readAt(offSize, u8PktHeader, sizeof(u8PktHeader));
                if ( n != sizeof(u8PktHeader) ) {
                    ALOGE("Get Data Packet Header Error");
                    return ERROR_END_OF_STREAM;
                }
                uint16_t u16ObjVer = U16_AT(u8PktHeader);
                uint16_t u16Length = U16_AT((u8PktHeader+2));
                MFP_DBG_RM3(ALOGD("u16Length = %x",u16Length));
                if ((u16ObjVer != 0 && u16ObjVer != 1) || (u16Length < 12)) {
                    return ERROR_MALFORMED;
                }

                uint32_t u32FrameSize = 0;
                uint32_t u32NumCodec = 0 ;
                uint32_t u32NumTemp = 0 ;
                uint32_t u32Frame[5] = {0, 0, 0, 0, 0};

                u32NumCodec = mRMContext.AudioTracks[i].LBR.numOfCodecID;
                u32FrameSize = mRMContext.AudioTracks[i].audioFrameSize;

                u32Frame[0] = u32FrameSize ;
                mRMContext.AudioTracks[i].LBR.frameSize[0] = u32FrameSize;

                for (u32NumTemp = 1; u32NumCodec > u32NumTemp; u32NumTemp++) {
                    uint8_t u8FrameSize;
                    if ( 1 != mDataSource->readAt(offSize + u16Length - u32NumCodec + u32NumTemp, &u8FrameSize, 1) ) {
                        return ERROR_END_OF_STREAM;
                    }
                    u32FrameSize = (uint32_t)u8FrameSize;
                    u32Frame[u32NumTemp] = u32FrameSize << 1;
                    u32Frame[0] -= (u32Frame[u32NumTemp] + 1);
                }

                for (u32NumTemp = 0; u32NumCodec > u32NumTemp; u32NumTemp++) {
                    mRMContext.AudioTracks[i].LBR.frameSize[u32NumTemp] = u32Frame[u32NumTemp];
                    MFP_DBG_RM3(ALOGD("u32Frame[u32NumTemp]%d %d",u32NumTemp,u32Frame[u32NumTemp]));
                }
                mRMContext.AudioTracks[i].LBR.isFrameSizeReady = true;
            }


            for (int j=0;j<mRMContext.AudioTracks[i].LBR.numOfCodecID;j++) {
                stProfileType.nChannels[j] = mRMContext.AudioTracks[i].LBR.channelMask[j];
                stProfileType.nNumRegions[j] = mRMContext.AudioTracks[i].channelInfo[j].numOfRegions;
                stProfileType.nCouplingStartRegion[j] = mRMContext.AudioTracks[i].channelInfo[j].CPLStart;
                stProfileType.nCouplingQuantBits[j] = mRMContext.AudioTracks[i].channelInfo[j].CPLQBits;
                stProfileType.nBitsPerFrame[j] = mRMContext.AudioTracks[i].LBR.frameSize[j];
                MFP_DBG_RM3(AALOGD("nBitsPerFrame[%d] = %d", j , stProfileType.nBitsPerFrame[j]));
                MFP_DBG_RM3(ALOGD("stProfileType.nChannels[%d] = %d ", j,stProfileType.nChannels[j] ));
                MFP_DBG_RM3(ALOGD("stProfileType.nCouplingStartRegion[%d] = %d", j,stProfileType.nCouplingStartRegion[j]));
                MFP_DBG_RM3(ALOGD("stProfileType.nCouplingQuantBits[%d] = %d", j,stProfileType.nCouplingQuantBits[j]));
                MFP_DBG_RM3(ALOGD("stProfileType.nBitsPerFrame[%d] = %d", j,stProfileType.nBitsPerFrame[j]));
            }
            meta->setData(kKeyMsRaProfile, 0, &stProfileType, sizeof(stProfileType));
        }
    }
    return OK;
}

RMExtractor::~RMExtractor() {
}

size_t RMExtractor::countTracks() {
    return mRMContext.nbStreams;
}

sp<MediaSource> RMExtractor::getTrack(size_t index) {
    return index < mRMContext.nbStreams ? new RMSource(this, index) : NULL;
}

sp<MetaData> RMExtractor::getTrackMetaData(
                                          size_t index, uint32_t flags) {
    return index < mTracks.size() ? mTracks.editItemAt(index).mMeta : NULL;
}

sp<MetaData> RMExtractor::getMetaData() {
    sp<MetaData> meta = new MetaData;

    if (mInitCheck == OK) {
        meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_CONTAINER_RM);
    }

    return meta;
}

bool SniffRM(
            const sp<DataSource> &source, String8 *mimeType, float *confidence,
            sp<AMessage> *) {
    char tmp[12];
    if (source->readAt(0, tmp, 12) < 12) {
        return false;
    }

    if (!memcmp(tmp, ".RMF", 4)) {
        mimeType->setTo(MEDIA_MIMETYPE_CONTAINER_RM);

        // Just a tad over the mp3 extractor's confidence, since
        // these .RM files may contain .mp3 content that otherwise would
        // mistakenly lead to us identifying the entire file as a .mp3 file.
        *confidence = 0.21;

        return true;
    }

    return false;
}

}  // namespace android
