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

#ifndef ASF_EXTRACTOR_H_

#define ASF_EXTRACTOR_H_

#include <media/stagefright/foundation/ABase.h>
#include <media/stagefright/MediaExtractor.h>
#include <media/stagefright/MediaSource.h>
#include <utils/Vector.h>
#include <utils/String8.h>


#define MAX_STREAM_NUM 10
#define ENABLE_WMDRMPD 0
#define ENABLE_PLAYREADY 0

#define MAX_ASF_VIDEO_TRACKS            (4)
#define MAX_ASF_AUDIO_TRACKS            (6)

typedef enum {
    E_GUID_UNKNOWN = 0,

    E_GUID_HEADER,
    E_GUID_DATA,
    E_GUID_INDEX,
    E_GUID_SIMPLE_INDEX,

    E_GUID_FILE_PROPERTIES,
    E_GUID_STREAM_PROPERTIES,
    E_GUID_CONTENT_DESCRIPTION,
    E_GUID_CONTENT_ENCRYPTION,
    E_GUID_EXTENDED_CONTENT_ENCRYPTION,
    E_GUID_DIGITAL_SIGNATURE,
    E_GUID_SCRIPT_COMMAND,
    E_GUID_HEADER_EXTENSION,
    E_GUID_MARKER,
    E_GUID_CODEC_LIST,
    E_GUID_STREAM_BITRATE_PROPERTIES,
    E_GUID_PADDING,
    E_GUID_EXTENDED_CONTENT_DESCRIPTION,

    E_GUID_METADATA,
    E_GUID_LANGUAGE_LIST,
    E_GUID_EXTENDED_STREAM_PROPERTIES,
    E_GUID_ADVANCED_MUTUAL_EXCLUSION,
    E_GUID_STREAM_PRIORITIZATION,

    E_GUID_STREAM_TYPE_AUDIO,
    E_GUID_STREAM_TYPE_VIDEO,
    E_GUID_STREAM_TYPE_COMMAND,
    E_GUID_STREAM_TYPE_EXTENDED,
    E_GUID_STREAM_TYPE_EXTENDED_AUDIO,

    E_GUID_NETFLIX_INDEX,

#if 1 // (ENABLE_PLAYREADY == 1)
    E_GUID_PROTECTED_SYSTEM_IDENTIFIER,
    E_GUID_CONTENT_PROTECTION_SYSTEM_MICROSOFT_PLAYREADY,
#endif
} ASF_GUID_TYPE;

typedef enum {
    CTRL_AV_HANDLER = 0,
    CTRL_SUBTITLE_HANDLER,
#if ENABLE_AUDIO_HANDLER
    CTRL_AUDIO_HANDLER,
#endif
    CTRL_MAX_HANDLER,
} EN_VDP_CTRL_HANDLER;

typedef struct {
    uint32_t u32V1;
    uint8_t u8V2[2];
    uint8_t u8V3[2];
    uint8_t u8V4[8];
} ST_ASF_GUID;

#define ST_ASF_OBJECT_COMMON \
    ST_ASF_GUID stGUID; \
    uint64_t u64Size; \
    uint64_t u64DataLen; \
    ASF_GUID_TYPE eType; \

typedef struct {
    ST_ASF_OBJECT_COMMON
} ST_ASF_OBJECT;

typedef struct {
    ST_ASF_OBJECT_COMMON
    uint16_t u16SubObjects;
    uint8_t u8Reserved1; /* 0x01, but could be safely ignored */
    uint8_t u8Reserved2; /* 0x02, if not must failed to source the contain */
} ST_ASF_OBJECT_HEADER;

typedef struct {
    ST_ASF_OBJECT_COMMON
    ST_ASF_GUID stReserved1;
    uint16_t u16Reserved2;
} ST_ASF_HEADEREXT;

typedef struct {
    ST_ASF_OBJECT_COMMON
    ST_ASF_GUID stFileID;
    uint64_t u64TotalDataPackets;
    uint64_t u64PKT_EndPosition;
    uint16_t u16Reserved;
} ST_ASF_OBJECT_DATA;

typedef struct {
    ST_ASF_OBJECT_COMMON
    ST_ASF_GUID stFileID;
    uint64_t u64EntryTimeInterval;
    uint32_t u32MaxPacketCount;
    uint32_t u32EntryCount;
} ST_ASF_OBJECT_SIMPLEINDEX;

typedef struct {
    ST_ASF_OBJECT_COMMON
    uint32_t u32EntryTimeInterval;
    uint16_t u16SpecifiersCount;
    uint32_t u32BlocksCount;
} ST_ASF_OBJECT_INDEX;

/* bitmapinfoheader fields specified in Microsoft documentation:
http://msdn2.microsoft.com/en-us/library/ms532290.aspx */
typedef struct {
    uint32_t u32BiSize;
    uint32_t u32BiWidth;
    uint32_t u32BiHeight;
    uint16_t u32BiPlanes;
    uint16_t u32BiBitCount;
    uint32_t u32BiCompression;
    uint32_t u32BiSizeImage;
    uint32_t u32BiXPelPerMeter;
    uint32_t u32BiYPelPerMeter;
    uint32_t u32BiClrUsed;
    uint32_t u32BiClrImportant;
} ST_ASF_BITMAPINFOHEADER;

typedef struct {
    uint64_t u64StartTime;
    uint64_t u64EndTime;
    uint32_t u32DataBitrate;
    uint32_t u32BufferSize;
    uint32_t u32InitBufFullness;
    uint32_t u32DataBitrate2;
    uint32_t u32BufferSize2;
    uint32_t u32InitBufFullness2;
    uint32_t u32MaxObjSize;
    uint32_t u32Flags;
    uint16_t u16StreamNumber;
    uint16_t u16LangIndex;
    uint64_t u64AvgTimePerFrame;
    uint16_t u16StreamNameCount;
    uint16_t u16NumPayloadExt;
} ST_ASF_STREAM_EXTEND;

typedef struct {
    ST_ASF_GUID stFileID;
    uint64_t u64FileSize;
    uint64_t u64CreationDate;
    uint64_t u64DataPacketsCount;
    uint64_t u64PlayDuration;
    uint64_t u64SendDuration;
    uint64_t u64Preroll;
    uint64_t u64MaxBitrate;
    uint32_t u32PacketSize;
    bool bIsBroadcast;
    bool bIsSeekable;
} ST_ASF_FILE;

#if (ENABLE_WMDRMPD == 1)
typedef struct {
    uint8_t* pu8DRMAddr;
    uint8_t* pu8DRMAddrStart;
    uint32_t u32Tag;
    uint32_t u32TagCount;
    uint8_t u8Encrypted;
} ST_ASF_DRM;
#endif

#if (ENABLE_PLAYREADY == 1)
typedef struct {
    uint8_t  u8Length;
    uint8_t  u8Last15[15];
} ST_AES_PREV_RESIDUE;
#endif  //ENABLE_PLAYREADY

/* waveformatex fields specified in Microsoft documentation:
http://msdn2.microsoft.com/en-us/library/ms713497.aspx */
typedef struct {
    uint16_t u16FormatTag;
    uint16_t u16Channels;
    uint32_t u32SamplesPerSec;
    uint32_t u32AvgBytesPerSec;
    uint16_t u16BlockAlign;
    uint16_t u16BitsPerSample;
    uint16_t u16CbSize;
    uint16_t u16EncodeOpt;
    uint32_t u32ChannelMask;
    uint32_t u32SamplePerBlock;
    uint16_t u16AdvanceEncodeOpt;
} ST_WAVEFORMATEX, *PST_WAVEFORMATEX;

typedef struct {
    uint8_t u8Encrypted;
    uint8_t u8StreamID;
    uint8_t u8WMADRCStreamID;
    bool bWMADRCExist;
    uint32_t u32WMADRCPeakReference;
    uint32_t u32WMADRCPeakTarget;
    uint32_t u32WMADRCAverageReference;
    uint32_t u32WMADRCAverageTarget;
} ST_ASF_STREAM_EXTEND_STORE;

typedef struct {
    uint8_t u8StreamID;
    uint64_t u64AvgTimePerFrame;
} ST_ASF_FRAME_RATE_STORE;

typedef struct {
    bool bIsMultiplePayload;
    bool bIsMultipleCompressedPayload;
    bool bHasKeyVideoFrameStart;
    bool bHasKeyAudioFrameStart;
    bool bCheckVStartCode;
    uint32_t MultipleCompressedPayloadLen;
    uint32_t MultipleCompressedPayloadLenParsed;
    uint32_t u32PaddingLen;
    uint32_t u32LastMediaObjectNum[MAX_STREAM_NUM];
    uint16_t u16PayloadCounter;
} ASF_CTRL_TYPE, *PASF_CTRL_TYPE;

typedef struct {
    uint8_t u8ECLen; /* error correction data length */

    uint32_t u32Length; /* length of this packet, usually constant per stream */
    uint32_t u32SendTime; /* send time of this packet in milliseconds */
    uint16_t u16Duration; /* duration of this packet in milliseconds */

    uint16_t u16PayloadCount; /* number of payloads contained in this packet */
    uint32_t u32PayloadDataLen; /* length of the raw payload data of this packet */
    uint8_t u8PacketProperty;
    uint32_t u32PayloadLenType;
} ASF_PACKET_TYPE, *PASF_PACKET_TYPE;

typedef struct {
    uint8_t u8StreamNumber; /* the stream number this payload belongs to */
    uint8_t u8KeyFrame; /* a flag indicating if this payload contains a key frame  */

    uint32_t u32MediaObjectNum; /* number of media object this payload is part of */
    uint32_t u32MediaObjectOffset; /* byte offset from beginning of media object */
    uint32_t u32MediaObjectSize; /* size of media object */

    uint32_t u32ReplicatedLen; /* length of some replicated data of a media object... */
    uint32_t u32DataLen; /* length of the actual payload data */
    uint32_t u32PTS; /* presentation time of this payload */
    uint32_t u8PTSDelta;
    uint64_t u64IV; /* for playready AES decrypt */
} ASF_PAYLOAD_TYPE, *PASF_PAYLOAD_TYPE;

typedef struct {
#if (ENABLE_WMDRMPD == 1)
    ST_ASF_DRM stDRM;
#endif
    ST_ASF_STREAM_EXTEND_STORE _stHeaderExtStore[MAX_STREAM_NUM];
    ST_ASF_FRAME_RATE_STORE stHeaderFrameRateStore[MAX_STREAM_NUM];
    uint8_t u8StreamExtCount;
#if (ENABLE_PLAYREADY == 1)
    BOOL bIsPlayReady;
#endif
} ST_CONTEXT_ASF, *PST_CONTEXT_ASF;

typedef struct {
    const char*  mime;
    int32_t     u32Tag;
} CodecTag;


namespace android {

struct ASFExtractor : public MediaExtractor {
    ASFExtractor(const sp<DataSource> &dataSource);

    virtual size_t countTracks();

    virtual sp<MediaSource> getTrack(size_t index);

    virtual sp<MetaData> getTrackMetaData(size_t index, uint32_t flags);

    virtual sp<MetaData> getMetaData();

protected:
    virtual ~ASFExtractor();

private:
    struct ASFSource;


    sp<DataSource> mDataSource;
    status_t mInitCheck;

    ST_ASF_FILE  _stFile;
    ST_ASF_FILE* g_pFile;
    ST_ASF_OBJECT_HEADER _stHeader;
    ST_ASF_OBJECT_HEADER* g_pHeader;
    ST_ASF_OBJECT_DATA _stData;
    ST_ASF_OBJECT_DATA* g_pData;
    ASF_PACKET_TYPE _stPacket[CTRL_MAX_HANDLER];
    ASF_PAYLOAD_TYPE _stPayload[CTRL_MAX_HANDLER];
    ASF_CTRL_TYPE _stASFCtrl[CTRL_MAX_HANDLER];
    ST_CONTEXT_ASF _stASF_Gbl;
    PASF_CTRL_TYPE g_pASFCtrl;
    PASF_PACKET_TYPE g_pASFPacket;
    PASF_PAYLOAD_TYPE g_pASFPayload;

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
            uint32_t duration;
            uint8_t u8StreamID;

            uint8_t audioFramesPerBlock;
            uint8_t channel;
            uint8_t bitsPerSample;
            uint16_t audioFrameSize;
            uint16_t audioSuperBlockSize;
            uint16_t u16EncodeOpt;
            uint16_t u16BitsPerSample;
            uint32_t u32ChannelMask;
            String8 *StrTitle;
            String8 *StrAuthor;
            String8 *StrAlbumTitle;
            String8 *StrYear;
            String8 *StrGenre;

        } AudioTracks[MAX_ASF_AUDIO_TRACKS];

        struct {
            uint32_t width;
            uint32_t height;
            uint32_t type;
            uint32_t frameRate;
            uint32_t frameRateBase;
            uint32_t nb_Frames;
            uint32_t duration;
            uint32_t codecID;
            uint8_t  streamID;
        } VideoTracks[MAX_ASF_VIDEO_TRACKS];

        off64_t mPKT_startPosition;
        uint32_t mMaxPacketSize;
        uint8_t nbAudioTracks;
        uint8_t nbVideoTracks;
        uint8_t nbStreams;
        bool mFoundIndex;
    } ST_CONTAINER_ASF, *PST_CONTAINER_ASF;

    ST_CONTAINER_ASF mASFContext;
    Vector<Track> mTracks;
    Vector<IndexInfo> mIndex;
    off64_t mCurReadPos;
    PacketInfo mPkt;
    void AddTracks();
    status_t parseHeaders();
    int matchGUID(const ST_ASF_GUID *pstGUID1, const ST_ASF_GUID *pstGUID2);
    ASF_GUID_TYPE getObjectType(const ST_ASF_GUID *pstGUID);
    ASF_GUID_TYPE getStreamType(const ST_ASF_GUID *pstGUID);
    ASF_GUID_TYPE getType(const ST_ASF_GUID *pstGUID);
    void getGUID(off64_t offset, ST_ASF_GUID *pstGUID);
    void parseObject(off64_t offset, ST_ASF_OBJECT *pstObj);
    status_t parseStreamProperties(off64_t offset, uint64_t u64ObjectSize);
    status_t parseFileProperties(off64_t offset, uint64_t u64ObjectSize);
    status_t parseExtendedStreamProperties(off64_t offset, uint64_t u64ObjectSize);
    status_t parseMetaData(off64_t offset, uint64_t u64ObjectSize);
    status_t parseHeaderExtension(off64_t offset, uint64_t u64ObjectSize);
    status_t parseContentDescription(off64_t offset, uint64_t u64ObjectSize);
    status_t parseExtendedContentDescription(off64_t offset, uint64_t u64ObjectSize);
    int32_t readWaveFormatEx(off64_t offset, PST_WAVEFORMATEX pstWaveFormatEx, uint32_t u32Length);
    const char* videoCodecTag2Mime(const CodecTag *pstTags, uint32_t u32Tag);
    const char* audioCodecTag2Mime(const CodecTag *pstTags, uint32_t u32Tag);
    status_t readPacket();
    DISALLOW_EVIL_CONSTRUCTORS(ASFExtractor);
};

class String8;
struct AMessage;

bool SniffASF(const sp<DataSource> &source, String8 *mimeType, float *confidence,
              sp<AMessage> *);

} // namespace android

#endif  // ASF_EXTRACTOR_H_
