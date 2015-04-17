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
#define LOG_TAG "FLVExtractor"
#include <utils/Log.h>
#include "math.h"
#include "include/avc_utils.h"
#include "include/FLVExtractor.h"

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


#define FLV_DBG(x)   (x)
#define AVC_DBG(x)   (x)
//------------------------------------------------------------------------------
// Type and Structure Declaration
//------------------------------------------------------------------------------
typedef enum
{
    E_FLV_FLAGS_VIDEO = 1,
    E_FLV_FLAGS_AUDIO = 4,
    E_FLV_FLAGS_DATA  = 8,
} EN_FLV_TYPE_FLAGS;

typedef enum
{
    E_FLV_TAG_TYPE_AUDIO = 8,
    E_FLV_TAG_TYPE_VIDEO = 9,
    E_FLV_TAG_TYPE_DATA  = 0x12,
} EN_FLV_TAG_TYPE;


typedef enum
{
    E_FLV_DATA_TYPE_NUMBER     = 0,
    E_FLV_DATA_TYPE_BOOLEAN    = 1,
    E_FLV_DATA_TYPE_STRING     = 2,
    E_FLV_DATA_TYPE_OBJECT     = 3,
    E_FLV_DATA_TYPE_MOVIECLIP  = 4,
    E_FLV_DATA_TYPE_NULL       = 5,
    E_FLV_DATA_TYPE_UNDEFINED  = 6,
    E_FLV_DATA_TYPE_REFERENCE  = 7,
    E_FLV_DATA_TYPE_ECMA       = 8,
    E_FLV_DATA_TYPE_STRICT     = 10,
    E_FLV_DATA_TYPE_DATA       = 11,
    E_FLV_DATA_TYPE_LONGSTRING = 12,
}EN_FLV_DATA_TYPE;

#define VIDEO_TYPE_JPEG              1
#define VIDEO_TYPE_SORENSON_H263     2
#define VIDEO_TYPE_SCREEN_VIDEO      3
#define VIDEO_TYPE_ON2_VP6           4
#define VIDEO_TYPE_ON2_VP6_AlPHA     5
#define VIDEO_TYPE_SCREEN_VIDEO_V2   6
#define VIDEO_TYPE_AVC               7

#define AUDIO_TYPE_PCM               0
#define AUDIO_TYPE_ADPCM             1
#define AUDIO_TYPE_MP3               2
#define AUDIO_TYPE_LPCM              3
#define AUDIO_TYPE_NELLY_16K         4
#define AUDIO_TYPE_NELLY_8K          5
#define AUDIO_TYPE_NELLY             6
#define AUDIO_TYPE_ALAW              7
#define AUDIO_TYPE_ULAW              8
#define AUDIO_TYPE_RESERVED          9
#define AUDIO_TYPE_AAC               10
#define AUDIO_TYPE_SPEEX             11
#define AUDIO_TYPE_MP3_8K            14
#define AUDIO_TYPE_DEVICE_SPECIFIED  15

//------------------------------------------------------------------------------
// Macros
//------------------------------------------------------------------------------
#define MACRO_FLV_READHEADER_RANGE      0x100000
#define AMF_END_OF_OBJECT               0x09


#define H264_RBSP_BUF_LEN               204
#define SEQ_HEADER_BUF_SIZE             (0x1000UL)      // 4K

//------------------------------------------------------------------------------
// Local Variables
//------------------------------------------------------------------------------
static const uint32_t MFP_AUDIO_SAMPLE_RATE_TABLE[] =
{
96000, 88200, 64000, 48000, 44100, 32000,
24000, 22050, 16000, 12000, 11025, 8000,
7350,
44100, 48000, 32000,
};


namespace android {

struct FLVExtractor::FLVSource : public MediaSource {
    FLVSource(const sp<FLVExtractor> &extractor, size_t trackIndex);

    virtual status_t start(MetaData *params);
    virtual status_t stop();

    virtual sp<MetaData> getFormat();

    virtual status_t read(
            MediaBuffer **buffer, const ReadOptions *options);

protected:
    virtual ~FLVSource();

private:
    sp<FLVExtractor> mExtractor;
    size_t mTrackIndex;
    const FLVExtractor::Track &mTrack;
    MediaBufferGroup *mBufferGroup;
   // size_t mSampleIndex;
    List<MediaBuffer *> mPendingFrames;

    DISALLOW_EVIL_CONSTRUCTORS(FLVSource);
};

FLVExtractor::FLVSource::FLVSource(
        const sp<FLVExtractor> &extractor, size_t trackIndex)
    : mExtractor(extractor),
      mTrackIndex(trackIndex),
      mTrack(mExtractor->mTracks.itemAt(trackIndex)),
      mBufferGroup(NULL) {

     //sp<MetaData> meta = mExtractor->mTracks.itemAt(index).mMeta;
}

FLVExtractor::FLVSource::~FLVSource() {
    if (mBufferGroup) {
        stop();
    }
}

status_t FLVExtractor::FLVSource::start(MetaData *params) {
    CHECK(!mBufferGroup);

    mBufferGroup = new MediaBufferGroup;

   // mBufferGroup->add_buffer(new MediaBuffer(mExtractor->mFLVContext.mMaxPacketSize));
   // mBufferGroup->add_buffer(new MediaBuffer(mTrack.mMaxSampleSize));
   // mSampleIndex = 0;

  //  const char *mime;
  //  CHECK(mTrack.mMeta->findCString(kKeyMIMEType, &mime));

    return OK;
}

status_t FLVExtractor::FLVSource::stop() {
    CHECK(mBufferGroup);

    delete mBufferGroup;
    mBufferGroup = NULL;

    return OK;
}

sp<MetaData> FLVExtractor::FLVSource::getFormat() {
    return mTrack.mMeta;
}

status_t FLVExtractor::FLVSource::read(
        MediaBuffer **buffer, const ReadOptions *options) {
    CHECK(mBufferGroup);

    *buffer = NULL;

    return OK;
}

#if ENABLE_FLV_ONMETADATA_PARSE

double FLVExtractor::int2Double(uint64_t v)
{
    if(v+v > 0xFFEULL<<52)
        return 0;
    return ldexp((double)(((v&((1LL<<52)-1)) + (1LL<<52)) * (v>>63|1)), (int32_t)(v>>52&0x7FF)-1075);
}

int32_t FLVExtractor::amfGetString(off64_t offset, uint8_t *pBuffer, uint32_t buffSize, uint32_t *pCount)
{
    off64_t tmpOffset = 0;
    uint32_t length;
    uint32_t idx;


    if (*pCount < 2)
        return -1;

    sp<ABuffer> buffer = new ABuffer(*pCount);
    ssize_t n = mDataSource->readAt(offset, buffer->data(), buffer->size());

    if (n < (ssize_t)(*pCount)) {
        return n < 0 ? (status_t)n : ERROR_MALFORMED;
    }
    const uint8_t *data = buffer->data();

    length = U16_AT(&data[tmpOffset]);
    tmpOffset += 2;
    *pCount -= 2;

    if (*pCount < length)
        return -1;

    if(length >= buffSize) {
       // url_fskip(ioc, length);
       tmpOffset += length;
        *pCount -= length;
        return -1;
    }
    for (idx = 0; idx < length; idx++)
    {
        *(pBuffer + idx) = (uint8_t)data[tmpOffset];
        tmpOffset++;
        *pCount -= 1;
    }
    pBuffer[length] = '\0';
    return length;
}

int32_t FLVExtractor::amfParseObject(off64_t offset, uint8_t *pKey, uint32_t *pCount, uint32_t maxPosition, uint32_t depth)
{
    off64_t tmpOffset = 0;
    uint64_t tag;
    EN_FLV_DATA_TYPE eType;
    uint8_t string[256];
    double dbl_val = 0;

    if (*pCount < 1)
        return -1;

    sp<ABuffer> buffer = new ABuffer(*pCount);
    ssize_t n = mDataSource->readAt(offset, buffer->data(), buffer->size());

    if (n < (ssize_t)(*pCount)) {
        return n < 0 ? (status_t)n : ERROR_MALFORMED;
    }
    const uint8_t *data = buffer->data();

    eType = (EN_FLV_DATA_TYPE)data[tmpOffset];//msAPI_VDPlayer_BMReadBytes_AutoLoad(1);
    tmpOffset += 1;
    *pCount -= 1;

    switch(eType)
    {
        case E_FLV_DATA_TYPE_NUMBER:
            if (*pCount < 8)
                return -1;
            tag = U32_AT(&data[tmpOffset]);//msAPI_VDPlayer_BMReadBytes_AutoLoad(4);
            tmpOffset += 4;
            tag <<= 32;
            tag |= U32_AT(&data[tmpOffset]);//msAPI_VDPlayer_BMReadBytes_AutoLoad(4);
            tmpOffset += 4;
            dbl_val = int2Double(tag);
            *pCount -= 8;
            break;
        case E_FLV_DATA_TYPE_BOOLEAN:
            if (*pCount < 1)
                return -1;
            dbl_val = data[tmpOffset];//msAPI_VDPlayer_BMReadBytes_AutoLoad(1);
            tmpOffset++;
            *pCount -= 1;
            break;
        case E_FLV_DATA_TYPE_STRING:
            if(amfGetString(offset + tmpOffset, string, sizeof(string), pCount) < 0)
                return -1;
            break;
        case E_FLV_DATA_TYPE_OBJECT:
            {
                while((offset + tmpOffset) < (maxPosition - 2))
                {
                    if(amfGetString(offset + tmpOffset, string, sizeof(string), pCount) < 0)
                        return -1;

                    if(amfParseObject(offset + tmpOffset, string, pCount, maxPosition, depth + 1) < 0)
                        return -1; //if we couldn't skip, bomb out.
                }
                if (*pCount < 1)
                    return -1;
                *pCount -= 1;
                tmpOffset++;
                if(data[tmpOffset - 1] != AMF_END_OF_OBJECT)
                    return -1;
            }
            break;

        case E_FLV_DATA_TYPE_ECMA:
            if (*pCount < 4)
                return -1;
            //msAPI_VDPlayer_BMFlushBytes(4); //skip 32-bit max array index
            tmpOffset += 4;
            *pCount -= 4;
            while((offset + tmpOffset) <( maxPosition - 2)
                && amfGetString(offset + tmpOffset, string, sizeof(string), pCount) > 0)
            {
                //this is the only case in which we would want a nested parse to not skip over the object
                if(amfParseObject(offset + tmpOffset, string, pCount, maxPosition, depth + 1) < 0)
                    return -1;
            }
            if (*pCount < 1)
                return -1;
            *pCount -= 1;
            tmpOffset++;
            if(data[tmpOffset - 1] != AMF_END_OF_OBJECT)
                return -1;
            break;
        case E_FLV_DATA_TYPE_STRICT:
            {
                uint32_t arrayLen, idx;

                arrayLen = U32_AT(&data[tmpOffset]);//msAPI_VDPlayer_BMReadBytes_AutoLoad(4);
                tmpOffset += 4;
                *pCount -= 4;
                for(idx = 0; (idx < arrayLen) && ((offset + tmpOffset) < (maxPosition - 1)); idx++)
                {
                    if(amfParseObject(offset + tmpOffset, pKey, pCount, maxPosition, depth + 1) < 0)
                    {
                        return -1; //if we couldn't skip, bomb out.
                    }
                }
            }
            break;
        default:
            break;
    }
   // ALOGW("key: %s, depth: %d\n", pKey, depth);
    if(depth == 3 && pKey) // process filepositions&times
    {
        // to do.................. index parser
        #if 0
        uint32_t tmpIdxAddr = g_pstDemuxer->pstBitsContent->Container.stFLV.u32TmpIdxBufAddr;
        uint32_t count;

        // address & PTS both use 32 bits
        if (strcmp((const char*)pKey, "filepositions") == 0)
        {
            count = g_pstDemuxer->pstBitsContent->Container.stFLV.u32VideoIdxCnt;
            *((uint32_t*)tmpIdxAddr + (count << 1)) = (uint32_t)dbl_val;
            g_pstDemuxer->pstBitsContent->Container.stFLV.u32VideoIdxCnt++;
      //      ALOGW("pos: %d\n", (uint32_t)dbl_val);

        }
        else if (strcmp((const char*)pKey, "times") == 0)
        {
            count = g_pstDemuxer->pstBitsContent->Container.stFLV.u32AudioIdxCnt;
            *((uint32_t*)tmpIdxAddr + (count << 1) + 1) = (uint32_t)(dbl_val * 1000);
            g_pstDemuxer->pstBitsContent->Container.stFLV.u32AudioIdxCnt++;
       //     ALOGW("time: %d\n", *((uint32_t*)tmpIdxAddr + (count << 1) + 1));
        }
        #endif
    }
    else if (depth == 1 && pKey)
    {
        if (strcmp((const char*)pKey, "duration") == 0)
        {
            mFLVContext.VideoTracks[0].duration = (uint32_t)(dbl_val * 1000);
        }
    }
    return 0;
}
#endif // ENABLE_ONMETADATA_PARSE

uint16_t FLVExtractor::getBits(uint8_t *buffer, uint16_t totbitoffset, uint32_t *info, uint16_t bufferLen, uint16_t numbits)
{
    uint32_t inf;
    uint8_t bitoffset  = (totbitoffset & 0x07); // bit from start of byte
    uint16_t byteoffset = (totbitoffset >> 3);       // byte from start of buffer
    uint16_t bitcounter = numbits;
    uint8_t *curbyte;
    if ((byteoffset) + ((numbits + bitoffset)>> 3)  > bufferLen)
        return 0;

    curbyte = &(buffer[byteoffset]);
    bitoffset = 7 - bitoffset;
    inf=0;

    while (numbits--)
    {
        inf <<=1;
        inf |= ((*curbyte)>> (bitoffset--)) & 0x01;
        curbyte   += (bitoffset >> 3) & 0x01;
        //curbyte   -= (bitoffset >> 3);
        bitoffset &= 0x07;
        //curbyte   += (bitoffset == 7);
    }

    *info = inf;
    return bitcounter;           // return absolute offset in bit from start of frame

}


uint32_t FLVExtractor::readSyntaxElement_FLC(uint16_t u16ReadBits, uint8_t* buffer, uint16_t bufferLen, uint16_t* bitLen, bool bUnsige)
{
    uint16_t readbitLen = 0;
    uint32_t codeword = 0;
    readbitLen = getBits(buffer, *bitLen, &codeword, bufferLen, u16ReadBits);
    FLV_DBG(ALOGW("BitLen=%d, Readbits=%d, Codeword=%d.\n",*bitLen,readbitLen,codeword));
    *bitLen += readbitLen;
    if(!bUnsige)
    {
        /// codeword change to s32 format
    }
    return codeword;
}


uint16_t FLVExtractor::getVLCSymbol(uint8_t *buffer, uint16_t totbitoffset, uint32_t *info, uint16_t bufferLen)
{
    uint32_t inf;
    uint16_t byteoffset = (totbitoffset >> 3);         // byte from start of buffer
    uint8_t  bitoffset  = (uint8_t)(7 - (totbitoffset & 0x07)); // bit from start of byte
    uint16_t  bitcounter = 1;
    uint16_t  len        = 0;
    uint8_t *cur_byte = &(buffer[byteoffset]);
    uint8_t  ctr_bit    = ((*cur_byte) >> (bitoffset)) & 0x01;  // control bit for current bit posision
    while (ctr_bit == 0)
    {                 // find leading 1 bit
        len++;
        bitcounter++;
        bitoffset--;
        bitoffset &= 0x07;
        cur_byte  += (bitoffset == 7);
        byteoffset+= (bitoffset == 7);
        ctr_bit    = ((*cur_byte) >> (bitoffset)) & 0x01;
    }

    if (byteoffset + ((len + 7) >> 3) > bufferLen)
        return 0;

    // make infoword
    inf = 0;                          // shortest possible code is 1, then info is always 0
    while (len--)
    {
        bitoffset --;
        bitoffset &= 0x07;
        cur_byte  += (bitoffset == 7);
        bitcounter++;
        inf <<= 1;
        inf |= ((*cur_byte) >> (bitoffset)) & 0x01;
    }

    *info = inf;
    return bitcounter;           // return absolute offset in bit from start of frame
}

uint32_t FLVExtractor::readSyntaxElement_VLC(uint8_t* buffer, uint16_t bufferLen, uint16_t* bitLen, bool bUnsige)
{
    uint16_t readbitLen = 0;
    uint32_t codeword = 0;
    readbitLen = getVLCSymbol(buffer, *bitLen, &codeword, bufferLen);
    codeword = (1 << (readbitLen >> 1)) + codeword - 1;
    //TS_DBG(printf("BitLen=%d, Readbits=%d, Codeword=%d.\n",*bitLen,readbitLen,codeword));
    *bitLen += readbitLen;
    if(!bUnsige)
    {
        uint32_t u32codewordtmp = codeword;
        codeword = (u32codewordtmp + 1) >> 1;
        if((u32codewordtmp & 0x01) == 0)
            codeword = (-1*codeword);
    }
    return codeword;
}


uint16_t FLVExtractor::EBSPtoRBSP(uint8_t *pSrc, uint8_t *pdestBuf, uint16_t end_bytepos)
{
    uint16_t i = 0, j = 0, count = 0;

    for(i = 0; i < end_bytepos; i++)
    {
        /// Check 0x000000, 0x000001 or 0x000002, those shall not occur at any byte-aligned.
        /// ISO 14496-10 Page:49 , 7.4.1 emulation_prevention_three_byte information.
        if(count == 2 && pSrc[i] < 0x03)
            return j;
        if(count == 2 && pSrc[i] == 0x03)
        {
            if((i < end_bytepos-1) && (pSrc[i+1] > 0x03))
                return j;
            if(i == end_bytepos-1)
                return j;

            i++;
            count = 0;
        }
        pdestBuf[j] = pSrc[i];
        if(pSrc[i] == 0x00)
            count++;
        else
            count = 0;
        j++;
        if (j > H264_RBSP_BUF_LEN - 1)
            return j;
    }
    return j;
}


bool FLVExtractor::parseH264SPS(uint8_t* pu8Addr, uint16_t u16DataLen, uint8_t u8NalSize, bool bStartCode)
{
    uint8_t pu8tmp[H264_RBSP_BUF_LEN] = {0};
    uint16_t u16DataPos = 0;

    while(u16DataLen >= 5)
    {
        if( ( (pu8Addr[u16DataPos] == 0x0 && pu8Addr[u16DataPos+1] == 0x0 &&
            pu8Addr[u16DataPos+2] == 0x1 && (pu8Addr[u16DataPos+3] &0x1F) == 0x7) ||
            (bStartCode == false) ) && u16DataLen > 16)
        {
            uint16_t u16BitLen = 0;//8*5;
            uint8_t u8Profile_idc = 0, u8Level_idc = 0, u8pic_order_cnt_type = 0;
            uint32_t u32frame_width_in_mbs = 0, u32frame_height_in_mbs = 0, u32FrameRate = 0;
            uint8_t u8frame_mbs_only_flag = 0, u8aspect_ratio_idc = 0, u8direct_8x8_inference_flag = 0;//, u8mb_adaptive_frame_field_flag = 0;
            uint16_t u16num_ref_frames = 0;
            uint32_t VUIMaxDecFrameBuf = 0;

            /// EBSP to RBSP
            EBSPtoRBSP(&pu8Addr[u16DataPos+u8NalSize+1],pu8tmp,(u16DataLen-u8NalSize-1));
            u8Profile_idc = (uint8_t)readSyntaxElement_FLC(8,pu8tmp,u16DataLen,&u16BitLen,true);
            /// Constrant_set0~4_flag
            readSyntaxElement_FLC(4,pu8tmp,u16DataLen,&u16BitLen,true);
            /// Reserved_zero_4bits
            readSyntaxElement_FLC(4,pu8tmp,u16DataLen,&u16BitLen,true);
            u8Level_idc = (uint8_t)readSyntaxElement_FLC(8,pu8tmp,u16DataLen,&u16BitLen,true);
            if((u8Level_idc != 10) && (u8Level_idc != 11) && (u8Level_idc != 12) && (u8Level_idc != 13) && (u8Level_idc != 20) && (u8Level_idc != 21) &&
               (u8Level_idc != 22) && (u8Level_idc != 30) && (u8Level_idc != 31) && (u8Level_idc != 32) && (u8Level_idc != 40) && (u8Level_idc != 41) &&
               (u8Level_idc != 42) && (u8Level_idc != 50) && (u8Level_idc != 51))
            {
                ALOGW("ParseH264SPS Failed, Level idc = %d is illegal !!\n",u8Level_idc);
                return false;
            }
            /// Seq_parameter_set_id
            readSyntaxElement_VLC(pu8tmp,u16DataLen,&u16BitLen,true);
            if(u8Profile_idc == 100 || u8Profile_idc == 110 || u8Profile_idc == 122 || u8Profile_idc == 244 ||
                u8Profile_idc == 44 || u8Profile_idc == 83  || u8Profile_idc == 86 )
            {
                uint8_t u8Chroma_format_idc = (uint8_t) readSyntaxElement_VLC(pu8tmp,u16DataLen,&u16BitLen,true);
                //TS_DBG(printf("Chroma = %bu.\n",u8Chroma_format_idc));
                if(u8Chroma_format_idc == 3)
                {
                    /// residue_transform_flag
                    readSyntaxElement_FLC(1,pu8tmp,u16DataLen,&u16BitLen,true);
                }

                readSyntaxElement_VLC(pu8tmp,u16DataLen,&u16BitLen,true); ///bit_depth_luma
                readSyntaxElement_VLC(pu8tmp,u16DataLen,&u16BitLen,true); ///bit_depth_chroma
                readSyntaxElement_FLC(1,pu8tmp,u16DataLen,&u16BitLen,true); ///qpprime_y_zero_transform_bypass_flag
                if(readSyntaxElement_FLC(1,pu8tmp,u16DataLen,&u16BitLen,true)) ///seq_scaling_matrix_present_flag
                {
                    uint8_t i;
                    for (i=0;i<8;i++)
                    {
                        if (readSyntaxElement_FLC(1,pu8tmp,u16DataLen,&u16BitLen,true))//presented_flag[i]
                        {
                            // pass scaling_lists
                            int32_t lastScale = 8;
                            int32_t nextScale = 8;
                            uint32_t maxnum = i<6?16:64;
                            uint32_t j = 0;
                            for(j = 0; j < maxnum; j++ )
                            {
                                if( nextScale != 0 )
                                {
                                    int32_t delta_scale  = (int32_t)readSyntaxElement_VLC(pu8tmp,u16DataLen,&u16BitLen,false);
                                    nextScale = ( lastScale + delta_scale + 256 ) & 0xff;
                                }
                                lastScale = ( nextScale == 0 ) ? lastScale : nextScale;;
                            }
                        }
                    }
                }
            }
            else if((u8Profile_idc != 66) && (u8Profile_idc != 77) && (u8Profile_idc != 88))
            {
                ALOGW("ParseH264SPS Failed, Profile idc = %d is illegal !!\n",u8Profile_idc);
                return false;
            }
            /// log2_max_frame_num
            readSyntaxElement_VLC(pu8tmp,u16DataLen,&u16BitLen,true);
            u8pic_order_cnt_type = (uint8_t)readSyntaxElement_VLC(pu8tmp,u16DataLen,&u16BitLen,true);
            if (u8pic_order_cnt_type == 0)
            {
                //uint8_t u8log2_max_pic_order_cnt_lsb = (uint8_t)
                readSyntaxElement_VLC(pu8tmp,u16DataLen,&u16BitLen,true);
            }
            else if (u8pic_order_cnt_type == 1)
            {
                uint8_t u8num_ref_frames_in_pic_order_cnt_cycle = 0, i = 0;
                /// delta_pic_order_always_zero_flag
                readSyntaxElement_FLC(1,pu8tmp,u16DataLen,&u16BitLen,true);
                /// offset_for_non_ref_pic
                readSyntaxElement_VLC(pu8tmp,u16DataLen,&u16BitLen,false);
                // offset_for_top_to_bottom_field
                readSyntaxElement_VLC(pu8tmp,u16DataLen,&u16BitLen,false);
                u8num_ref_frames_in_pic_order_cnt_cycle = (uint8_t)readSyntaxElement_VLC(pu8tmp,u16DataLen,&u16BitLen,true);
                // get offsets
                for (i=0; i<u8num_ref_frames_in_pic_order_cnt_cycle; i++)
                {
                    readSyntaxElement_VLC(pu8tmp,u16DataLen,&u16BitLen,false);
                }
            }
            /// num_ref_frames
            u16num_ref_frames = (uint16_t)readSyntaxElement_VLC(pu8tmp,u16DataLen,&u16BitLen,true);
            /// gaps_in_frame_num_value_allowed_flag
            readSyntaxElement_FLC(1,pu8tmp,u16DataLen,&u16BitLen,true);

            /// picture width in MBs (bitstream contains value - 1)
            u32frame_width_in_mbs = readSyntaxElement_VLC(pu8tmp,u16DataLen,&u16BitLen,true) + 1;
            /// picture height in MBs (bitstream contains value - 1)
            u32frame_height_in_mbs = readSyntaxElement_VLC(pu8tmp,u16DataLen,&u16BitLen,true) + 1;

            u8frame_mbs_only_flag = (uint8_t)readSyntaxElement_FLC(1,pu8tmp,u16DataLen,&u16BitLen,true);
            //(printf("Frame MBs W = %d, H = %d.\n",u32frame_width_in_mbs,u32frame_height_in_mbs));

            if (u8frame_mbs_only_flag == 0)
            {
                readSyntaxElement_FLC(1,pu8tmp,u16DataLen,&u16BitLen,true);//mb_adaptive_frame_field_flag
                /*
                if (g_pstDemuxer->enFileFormat == EN_VDP_FILE_FORMAT_MKV)
                {
                    if (!u8mb_adaptive_frame_field_flag)
                    {
                        g_pstDemuxer->pstBitsContent->Container.stMKV.VideoTracks[0].u32FrameRateBase <<= 1;
                    }
                }
                */
            }
            u8direct_8x8_inference_flag = (uint8_t)readSyntaxElement_FLC(1,pu8tmp,u16DataLen,&u16BitLen,true);//direct_8x8_inference_flag

            if (readSyntaxElement_FLC(1,pu8tmp,u16DataLen,&u16BitLen,true))//frame_cropping_flag
            {
                /// frame_cropping_rect_left_offset
                readSyntaxElement_VLC(pu8tmp,u16DataLen,&u16BitLen,true);
                /// frame_cropping_rect_right_offset
                readSyntaxElement_VLC(pu8tmp,u16DataLen,&u16BitLen,true);
                /// frame_cropping_rect_top_offset
                readSyntaxElement_VLC(pu8tmp,u16DataLen,&u16BitLen,true);
                // frame_cropping_rect_bottom_offset
                readSyntaxElement_VLC(pu8tmp,u16DataLen,&u16BitLen,true);
                //TS_DBG(printf("Cropping Right = %lu, Btm = %lu.\n",u32frame_cropping_rect_right_offset,u32frame_cropping_rect_bottom_offset));
            }

            if (readSyntaxElement_FLC(1,pu8tmp,u16DataLen,&u16BitLen,true)) ///vui_parameters_present_flag
            {
                uint8_t nal_hrd_parameters_present_flag = 0, vcl_hrd_parameters_present_flag = 0;
                // uint8_t u8aspect_ratio_info_present_flag = (uint8_t)_MApp_TS_ReadSyntaxElement_FLC(1,pu8tmp,u16DataLen,&u16BitLen,true);
                if( readSyntaxElement_FLC(1,pu8tmp,u16DataLen,&u16BitLen,true) )
                {
                    u8aspect_ratio_idc = (uint8_t)readSyntaxElement_FLC(8,pu8tmp,u16DataLen,&u16BitLen,true);
                    //TS_DBG(printf("Aspect = %bu.\n",u8aspect_ratio_idc));
                    if( u8aspect_ratio_idc  ==  255)
                    {
                        /// sar_width
                        //pProgramList->u16AspectRatioX = (uint16_t)
                        readSyntaxElement_FLC(16,pu8tmp,u16DataLen,&u16BitLen,true);
                        /// sar_height
                        //pProgramList->u16AspectRatioY = (uint16_t)
                        readSyntaxElement_FLC(16,pu8tmp,u16DataLen,&u16BitLen,true);
                    }
                }
                /// overscan_info_present_flag
                if(readSyntaxElement_FLC(1,pu8tmp,u16DataLen,&u16BitLen,true))
                {
                    /// overscan_appropriate_flag
                    readSyntaxElement_FLC(1,pu8tmp,u16DataLen,&u16BitLen,true);
                }
                /// video_signal_type_present_flag
                if(readSyntaxElement_FLC(1,pu8tmp,u16DataLen,&u16BitLen,true))
                {
                    /// video_format
                    readSyntaxElement_FLC(3,pu8tmp,u16DataLen,&u16BitLen,true);
                    /// video_full_range_flag
                    readSyntaxElement_FLC(1,pu8tmp,u16DataLen,&u16BitLen,true);
                    /// colour_description_present_flag
                    if( readSyntaxElement_FLC(1,pu8tmp,u16DataLen,&u16BitLen,true) )
                    {
                        /// colour_primaries
                        readSyntaxElement_FLC(8,pu8tmp,u16DataLen,&u16BitLen,true);
                        /// transfer_characteristics
                        readSyntaxElement_FLC(8,pu8tmp,u16DataLen,&u16BitLen,true);
                        /// matrix_coefficients
                        readSyntaxElement_FLC(8,pu8tmp,u16DataLen,&u16BitLen,true);
                    }
                }
                /// chroma_loc_info_present_flag
                if( readSyntaxElement_FLC(1,pu8tmp,u16DataLen,&u16BitLen,true) ) {
                    /// chroma_sample_loc_type_top_field
                    readSyntaxElement_VLC(pu8tmp,u16DataLen,&u16BitLen,true);
                    /// chroma_sample_loc_type_bottom_field
                    readSyntaxElement_VLC(pu8tmp,u16DataLen,&u16BitLen,true);
                }
                /// timing_info_present_flag
                if( readSyntaxElement_FLC(1,pu8tmp,u16DataLen,&u16BitLen,true) ) {
                    uint32_t u32num_units_in_tick = readSyntaxElement_FLC(32,pu8tmp,u16DataLen,&u16BitLen,true);
                    uint32_t u32time_scale = readSyntaxElement_FLC(32,pu8tmp,u16DataLen,&u16BitLen,true);
                    uint8_t u8fixed_frame_rate_flag = (uint8_t)readSyntaxElement_FLC(1,pu8tmp,u16DataLen,&u16BitLen,true);

                    if(u32num_units_in_tick)
                    {
                        u32FrameRate = (uint32_t)(1000*u32time_scale/u32num_units_in_tick/(1 + u8fixed_frame_rate_flag));
                    }
                    else
                    {
                        u32FrameRate = (uint32_t)(1000*u32time_scale/(1 + u8fixed_frame_rate_flag));
                    }
                    if(u32FrameRate > 60000)
                    {
                        u32FrameRate >>= 1;
                    }
                    else if(u32FrameRate < 23970)
                    {
                        u32FrameRate = 30000;
                    }
                    //TS_DBG(printf("Timing info = %lu, %lu, %bu.\n",u32num_units_in_tick,u32time_scale,u8fixed_frame_rate_flag));
                }

                /// nal_hrd_parameters_present_flag
                nal_hrd_parameters_present_flag = (uint8_t)readSyntaxElement_FLC(1,pu8tmp,u16DataLen,&u16BitLen,true);
                //TS_DBG(printf("nal_hrd_parameters_present_flag = %d.\n",nal_hrd_parameters_present_flag));
                if(nal_hrd_parameters_present_flag)
                {
                    uint32_t u32cpb_cnt_minus1 = 0;
                    uint32_t u32SchedSeelIdx = 0;
                    u32cpb_cnt_minus1 = (uint32_t)readSyntaxElement_VLC(pu8tmp,u16DataLen,&u16BitLen,true);
                    readSyntaxElement_FLC(4,pu8tmp,u16DataLen,&u16BitLen,true);
                    readSyntaxElement_FLC(4,pu8tmp,u16DataLen,&u16BitLen,true);
                    for(u32SchedSeelIdx = 0; u32SchedSeelIdx<=u32cpb_cnt_minus1;u32SchedSeelIdx++ )
                    {
                        readSyntaxElement_VLC(pu8tmp,u16DataLen,&u16BitLen,true);
                        readSyntaxElement_VLC(pu8tmp,u16DataLen,&u16BitLen,true);
                        readSyntaxElement_FLC(1,pu8tmp,u16DataLen,&u16BitLen,true);
                    }
                    readSyntaxElement_FLC(5,pu8tmp,u16DataLen,&u16BitLen,true);
                    readSyntaxElement_FLC(5,pu8tmp,u16DataLen,&u16BitLen,true);
                    readSyntaxElement_FLC(5,pu8tmp,u16DataLen,&u16BitLen,true);
                    readSyntaxElement_FLC(5,pu8tmp,u16DataLen,&u16BitLen,true);
                }
                /// vcl_hrd_parameters_present_flag
                vcl_hrd_parameters_present_flag = (uint8_t)readSyntaxElement_FLC(1,pu8tmp,u16DataLen,&u16BitLen,true);
                if(vcl_hrd_parameters_present_flag)
                {
                    uint32_t u32cpb_cnt_minus1 = 0;
                    uint32_t u32SchedSeelIdx = 0;
                    u32cpb_cnt_minus1 = (uint32_t)readSyntaxElement_VLC(pu8tmp,u16DataLen,&u16BitLen,true);
                    readSyntaxElement_FLC(4,pu8tmp,u16DataLen,&u16BitLen,true);
                    readSyntaxElement_FLC(4,pu8tmp,u16DataLen,&u16BitLen,true);
                    for(u32SchedSeelIdx = 0; u32SchedSeelIdx<=u32cpb_cnt_minus1;u32SchedSeelIdx++ )
                    {
                        readSyntaxElement_VLC(pu8tmp,u16DataLen,&u16BitLen,true);
                        readSyntaxElement_VLC(pu8tmp,u16DataLen,&u16BitLen,true);
                        readSyntaxElement_FLC(1,pu8tmp,u16DataLen,&u16BitLen,true);
                    }
                    readSyntaxElement_FLC(5,pu8tmp,u16DataLen,&u16BitLen,true);
                    readSyntaxElement_FLC(5,pu8tmp,u16DataLen,&u16BitLen,true);
                    readSyntaxElement_FLC(5,pu8tmp,u16DataLen,&u16BitLen,true);
                    readSyntaxElement_FLC(5,pu8tmp,u16DataLen,&u16BitLen,true);
                }
                if(nal_hrd_parameters_present_flag || vcl_hrd_parameters_present_flag)
                {
                    readSyntaxElement_FLC(1,pu8tmp,u16DataLen,&u16BitLen,true);
                }
                readSyntaxElement_FLC(1,pu8tmp,u16DataLen,&u16BitLen,true);
                nal_hrd_parameters_present_flag = (uint8_t)readSyntaxElement_FLC(1,pu8tmp,u16DataLen,&u16BitLen,true);
                //TS_DBG(printf("bitstream_restriction_flag = %d, u16DataLen = %d, u16BitLen = %d.\n",nal_hrd_parameters_present_flag,u16DataLen,u16BitLen));
                /// bitstream_restriction_flag
                if(nal_hrd_parameters_present_flag)
                {
                    readSyntaxElement_FLC(1,pu8tmp,u16DataLen,&u16BitLen,true);
                    readSyntaxElement_VLC(pu8tmp,u16DataLen,&u16BitLen,true);
                    readSyntaxElement_VLC(pu8tmp,u16DataLen,&u16BitLen,true);
                    readSyntaxElement_VLC(pu8tmp,u16DataLen,&u16BitLen,true);
                    readSyntaxElement_VLC(pu8tmp,u16DataLen,&u16BitLen,true);
                    readSyntaxElement_VLC(pu8tmp,u16DataLen,&u16BitLen,true);
                    VUIMaxDecFrameBuf = readSyntaxElement_VLC(pu8tmp,u16DataLen,&u16BitLen,true);
                    //(printf("VUIMaxDecFrameBuf = %d, u16BitLen = %d.\n",VUIMaxDecFrameBuf,u16BitLen));
                }

            }

            #if 0
            pstVIDmemUsage->enID = E_VDP_CODEC_ID_H264;
            pstVIDmemUsage->u16Width = (uint16_t)(u32frame_width_in_mbs*16);
            pstVIDmemUsage->u16Height = (uint16_t)(16*u32frame_height_in_mbs*(2 - u8frame_mbs_only_flag));
            pstVIDmemUsage->u8Level_idc = u8Level_idc;
            pstVIDmemUsage->u8Profile_idc = u8Profile_idc;
            pstVIDmemUsage->u8DecFrameNum = (uint8_t)VUIMaxDecFrameBuf;
            pstVIDmemUsage->u8NumRefFrames = (uint8_t)u16num_ref_frames;
            pstVIDmemUsage->u8FrameMbsOnlyFlag = u8frame_mbs_only_flag;
            pstVIDmemUsage->u8Direct8x8InferenceFlag = u8direct_8x8_inference_flag;
            #endif
            mFLVContext.VideoTracks[0].width = u32frame_width_in_mbs*16;
            mFLVContext.VideoTracks[0].height = 16*u32frame_height_in_mbs*(2 - u8frame_mbs_only_flag);
            mFLVContext.VideoTracks[0].frameRate = u32FrameRate;
            ALOGW("Width: %d, Height: %d, Framerate: %d\n", mFLVContext.VideoTracks[0].width, mFLVContext.VideoTracks[0].height,
                mFLVContext.VideoTracks[0].frameRate);

            return true;
        }
        else
        {
            u16DataPos++;
            u16DataLen--;
        }
    }

    return false;
}

status_t FLVExtractor::readAVCConfigurationRecord(off64_t offset, uint32_t size) {
    off64_t tmpOffset = 0;
    uint32_t length;
    uint32_t headerInfoSize = 0, headerInfoSizeExt = 0;
    uint32_t idx;
    uint8_t nalLenSizeUnit, numSPS;
    uint8_t *pNal;

    sp<ABuffer> buffer = new ABuffer(size);
    ssize_t n = mDataSource->readAt(offset, buffer->data(), buffer->size());

    if (n < (ssize_t)size) {
        return n < 0 ? (status_t)n : ERROR_MALFORMED;
    }
    const uint8_t *data = buffer->data();


    sp<ABuffer> seqBuffer = new ABuffer(SEQ_HEADER_BUF_SIZE);
    uint8_t *seqData = seqBuffer->data();


    for (uint8_t i = 0; i < 16; i++)
        ALOGW("%x ", data[i]);
    ALOGW("\n");

    // flush configuration version, AVCProfileIndication, Profile_comatibility, AVCLevelIndication
    tmpOffset += 4;

    // lengthSizeMinusOne
    nalLenSizeUnit = (data[tmpOffset] & 0x03) + 1;
    tmpOffset += 1;
    AVC_DBG(ALOGW("Nal length size unit: %d\n", nalLenSizeUnit));

    // SPS number
    numSPS = data[tmpOffset] & 0x1F;
    tmpOffset += 1;
    AVC_DBG(ALOGW("SPS number: %d\n", numSPS));

    if (numSPS == 0)
    {
        ALOGW("sps number is zero!!!!!\n");
        return -1;
    }
    for (uint8_t i = 0; i < numSPS; i++) {
        //SPS Length
        length = U16_AT(&data[tmpOffset]);
        AVC_DBG(ALOGW("SPS length: %d\n", length));
        tmpOffset += 2;
        //msAPI_VDPlayer_Demuxer_WriteSeqHeader(u32Length, TRUE);

        idx = headerInfoSize;
        pNal = seqData + headerInfoSizeExt;

        if (nalLenSizeUnit == 1)
        {
            *(pNal + idx) = (uint8_t)length;
            pNal ++;
            headerInfoSizeExt++;
        }
        else if (nalLenSizeUnit == 2)
        {
            *(pNal + idx) = (uint8_t)(length >> 8);
            *(pNal + idx + 1) = (uint8_t)(length & 0xFF);
            pNal += 2;
            headerInfoSizeExt += 2;
        }
        else
        {
            if (nalLenSizeUnit == 4)
            {
                *(pNal + idx++) = 0x00;
            }
            *(pNal + idx++) = 0x00;
            *(pNal + idx++) = 0x00;
            *(pNal + idx++) = 0x01;
        }

        while(length--)
        {
            // SPS data
            *(pNal + idx++) = data[tmpOffset];
            tmpOffset++;
        }
        headerInfoSize = idx;
    }
    parseH264SPS(seqData, headerInfoSize+headerInfoSizeExt, nalLenSizeUnit, false);

#if 0
    sp<MetaData> meta = MakeAVCCodecSpecificData(buffer);

    if (meta == NULL) {
        ALOGE("Unable to extract AVC codec specific data");
        return ERROR_MALFORMED;
    }
#endif

    return OK;
}

uint32_t FLVExtractor::index2SampleRate(uint32_t sampleRateIdx) {
    uint32_t size;
    size = sizeof(MFP_AUDIO_SAMPLE_RATE_TABLE)/sizeof(uint32_t);

    if (sampleRateIdx >= size)
        return 0;
    return MFP_AUDIO_SAMPLE_RATE_TABLE[sampleRateIdx];
}

void FLVExtractor::readAACConfigurationRecord(uint32_t extraData) {
    uint32_t sampleRateIdx;

    mFLVContext.AudioTracks[0].objectTypeID = (extraData >> 27) & 0x1F;
    extraData <<= 5;
    if (mFLVContext.AudioTracks[0].objectTypeID == 31)
    {
        mFLVContext.AudioTracks[0].objectTypeID = 32 + ((extraData >> 26) & 0x2F);
        extraData <<= 6;
    }

    sampleRateIdx = (extraData >> 28) & 0xF;
    extraData <<= 4;
    if (sampleRateIdx == 0xF)
    {
        ALOGW("u32Audio_Extradata don't enough!!\n");
        mFLVContext.AudioTracks[0].samplerate = (extraData >> 8) & 0xFFFFFF;
        extraData <<= 24;
    }
    else
    {
        if ((mFLVContext.AudioTracks[0].objectTypeID == 29) && (sampleRateIdx < 3))
        {
            sampleRateIdx += 13;
        }

        sampleRateIdx = index2SampleRate(sampleRateIdx);
        if (sampleRateIdx != 0)
            mFLVContext.AudioTracks[0].samplerate = sampleRateIdx;
    }

    mFLVContext.AudioTracks[0].channel = (extraData >> 28) & 0xf;   // channels
    if (mFLVContext.AudioTracks[0].channel >= 7)
        mFLVContext.AudioTracks[0].channel = 8;
    return ;
}

status_t FLVExtractor::parseHeaders() {
    off64_t tmpOffset = 0;
    off64_t fileSize;
    off64_t baseOffset;
    uint32_t tag, dataOffset, dataSize, dts = 0;
    uint32_t firstFrameDTS = 0, previousDTS = 0;
    uint8_t tmp[18];
    uint8_t flag = 0, tagType = 0, dataobj = 0, videoIdx = 0;
    bool bAVCHeader = false;
    ssize_t n = mDataSource->readAt(0, tmp, 18);


    mDataSource->getSize(&fileSize);

    if (n < 18) {
        return (n < 0) ? n : (ssize_t)ERROR_MALFORMED;
    }

    // flush FLV, Version
    tmpOffset += 4;

    //
    flag = tmp[tmpOffset];
    if (flag == 0)
        flag = E_FLV_FLAGS_VIDEO | E_FLV_FLAGS_AUDIO;
    FLV_DBG(ALOGW("has video/audio: %x\n", flag));
    tmpOffset += 1;

    dataOffset = U32_AT(&tmp[tmpOffset]);
    FLV_DBG(ALOGW("data offset: %x\n", (uint32_t)dataOffset));
    mFLVContext.mPKT_startPosition = dataOffset;
    baseOffset = dataOffset;

    sp<ABuffer> buffer = new ABuffer(MACRO_FLV_READHEADER_RANGE);

    n = mDataSource->readAt(baseOffset, buffer->data(), buffer->size());
    const uint8_t *data = buffer->data();
    tmpOffset = 0;

    while (tmpOffset < n) {
        // tag size
        tmpOffset += 4;

        tag = U32_AT(&data[tmpOffset]);
        tagType = (tag >> 24);
        dataSize = (tag & 0xFFFFFF);
        FLV_DBG(ALOGW("tag: %x, data size: %x\n", tagType, dataSize));
        tmpOffset += 4;

        dts = U32_AT(&data[tmpOffset]);
        FLV_DBG(ALOGW("ori DTS: %x\n", dts));
        dts = ((dts & 0xFF) << 24) | (dts & 0xFFFFFF);
        FLV_DBG(ALOGW("real DTS: %x\n", dts));
        tmpOffset += 4;

        if (dataSize <= 1)
        {
            // flush stream id, remain size
            tmpOffset += (3 + dataSize);
            continue;
        }

        tag = U32_AT(&data[tmpOffset]);
        FLV_DBG(ALOGW("stream id(always 0) + data: %x\n", tag));
        dataobj = tag & 0xFF;
        tmpOffset += 4;

        switch(tagType) {
            case E_FLV_TAG_TYPE_VIDEO:
                switch(dataobj&0xF)
                {
                    case VIDEO_TYPE_SORENSON_H263:
                        mFLVContext.VideoTracks[0].codecID = VIDEO_TYPE_SORENSON_H263;
                        break;
                    case VIDEO_TYPE_AVC:
                        mFLVContext.VideoTracks[0].codecID = VIDEO_TYPE_AVC;
                        break;
                    case VIDEO_TYPE_ON2_VP6:
                        mFLVContext.VideoTracks[0].codecID = VIDEO_TYPE_ON2_VP6;
                        break;
                    case VIDEO_TYPE_ON2_VP6_AlPHA:
                        mFLVContext.VideoTracks[0].codecID = VIDEO_TYPE_ON2_VP6_AlPHA;
                        break;
                    default:
                        FLV_DBG(ALOGW("un-support Video Codec %x\n", (dataobj&0xF)));
                       return -1;
                        break;
                }
                FLV_DBG(ALOGW("video codec id: %d\n", mFLVContext.VideoTracks[0].codecID));
                if (mFLVContext.VideoTracks[0].codecID == VIDEO_TYPE_AVC)
                {
                    if (data[tmpOffset] == 0)
                    {
                        if (!bAVCHeader)
                        {
                            // tmpOffset + 4: flush AVC packet type, Composition time
                            if (readAVCConfigurationRecord(baseOffset + tmpOffset + 4, dataSize - 5) == OK)
                            {
                                bAVCHeader = true;
                            }
                        }
                        break;
                    }
                }

                if ((flag & E_FLV_FLAGS_VIDEO) == 0)
                    break;

                if (mFLVContext.nbVideoTracks != 1)
                {
                    firstFrameDTS = dts;
                    previousDTS= dts;
                    mFLVContext.nbVideoTracks = 1;
                    mFLVContext.nbStreams++;
                }
                else
                {
                    videoIdx++;
                    if ((dts - previousDTS) > 500)
                    {
                        if (videoIdx < 5)
                        {
                            previousDTS = dts;
                            break;
                        }

                        previousDTS = dts - 33;
                    }

                    if ((dts - previousDTS) != 0)
                        mFLVContext.VideoTracks[0].frameRate = 1000000 / (dts - previousDTS);
                    mFLVContext.VideoTracks[0].frameRateBase = 1000;
                    FLV_DBG(ALOGW("video Frame Rate: %d\n", mFLVContext.VideoTracks[0].frameRate));

                    mFLVContext.VideoTracks[0].duration = 0xFFFFFFFF;
                    flag &= ~E_FLV_FLAGS_VIDEO;
                }
                break;

            case E_FLV_TAG_TYPE_AUDIO:

                if ((flag & E_FLV_FLAGS_AUDIO) == 0)
                    break;

                if (dataobj& 0x1)
                    mFLVContext.AudioTracks[0].channel = 2;
                else
                    mFLVContext.AudioTracks[0].channel = 1;

                if (dataobj & 0x2)
                    mFLVContext.AudioTracks[0].bitsPerSample = 16;
                else
                    mFLVContext.AudioTracks[0].bitsPerSample = 8;

                mFLVContext.AudioTracks[0].samplerate = 44100 >> (3 - ((dataobj >> 2) & 0x3));

                FLV_DBG(ALOGW("channel: %d, bits/sample: %d, sample rate: %d\n",
                    mFLVContext.AudioTracks[0].channel, mFLVContext.AudioTracks[0].bitsPerSample,
                    mFLVContext.AudioTracks[0].samplerate));

                switch(dataobj >> 4)
                {
                    case AUDIO_TYPE_PCM: // LPCM, platform endian
                        mFLVContext.AudioTracks[0].codecID = AUDIO_TYPE_PCM;
                        break;
                    case AUDIO_TYPE_ADPCM: // ADPCM
                        mFLVContext.AudioTracks[0].codecID = AUDIO_TYPE_ADPCM;
                        break;
                    case AUDIO_TYPE_MP3: // MP3
                        mFLVContext.AudioTracks[0].codecID = AUDIO_TYPE_MP3;
                        mFLVContext.AudioTracks[0].scale = 1152;
                        mFLVContext.AudioTracks[0].blockAlign = 1152;
                        mFLVContext.AudioTracks[0].samplesize = 0;
                        break;
                    case AUDIO_TYPE_LPCM: // LPCM, little endian
                        mFLVContext.AudioTracks[0].codecID = AUDIO_TYPE_LPCM;
                        break;
                    case AUDIO_TYPE_AAC: // AAC
                        mFLVContext.AudioTracks[0].codecID = AUDIO_TYPE_AAC;
                        mFLVContext.AudioTracks[0].channel = 2;
                        mFLVContext.AudioTracks[0].samplerate = 44100;
                        break;
                    default:
                        ALOGW("non support Audio Codec %d\n", (dataobj >> 4));
                        return -1;
                        break;
                }
                FLV_DBG(ALOGW("Audio codec id: %d\n", mFLVContext.AudioTracks[0].codecID));
#if 1
                if (mFLVContext.AudioTracks[0].codecID == AUDIO_TYPE_AAC)
                {
                    uint32_t length = 0;
                    uint32_t extraData = 0;


                    if (data[tmpOffset] != 0)
                        break;

                    // flush AAC packet type
                    // later tmpOffset need plus 1

                    length = dataSize- 2;
                    if(length <= 4)
                    {
                        uint8_t idx;

                        for(idx = 0; idx < length; idx++)
                        {
                            extraData = (extraData << 8) | data[tmpOffset + idx + 1];
                        }
                        //extraData= msAPI_VDPlayer_BMReadBytes_AutoLoad((uint8_t)length);
                        extraData <<= (4 - length) * 8;
                        //dataSize = 1;
                    }
                    else
                    {
                        extraData = U32_AT(&data[tmpOffset + 1]);
                        //dataSize -= 5;
                    }

                    readAACConfigurationRecord(extraData);
                }
#endif //
                mFLVContext.nbAudioTracks++;
                mFLVContext.nbStreams++;

                flag &= ~E_FLV_FLAGS_AUDIO;
                break;

            case E_FLV_TAG_TYPE_DATA:
                #if ENABLE_FLV_ONMETADATA_PARSE
                {
                    uint32_t metaSize, tmpRec;
                    EN_FLV_DATA_TYPE eType;
                    uint8_t  array[11]; // for Storing string "onMetaData"

                    eType = (EN_FLV_DATA_TYPE)dataobj;
                    if (eType != E_FLV_DATA_TYPE_STRING)
                        break;

                    metaSize = dataobj - 1;
                    tmpRec = metaSize;
                    if(amfGetString(baseOffset + tmpOffset, array, sizeof(array), &metaSize) < 0
                        || strcmp((const char*)array, "onMetaData"))
                    {
                        //dataSize = metaSize + 1;
                        break;
                    }
                    tmpRec = tmpRec - metaSize;
                    //parse the second object (we want a mixed array)
                    amfParseObject(baseOffset + tmpOffset + tmpRec, array, &metaSize, dataOffset, 0);
                    //dataSize = metaSize + 1;
                }
                #endif //ENABLE_FLV_ONMETADATA_PARSE
                break;
            default:
                break;
        }
        tmpOffset += (dataSize - 1);
        if (flag == 0)
            break;
    }
    return OK;
}

FLVExtractor::FLVExtractor(const sp<DataSource> &dataSource)
    : mDataSource(dataSource) {

    memset((uint8_t*)&mFLVContext, 0, sizeof(ST_CONTAINER_FLV));
    mInitCheck = parseHeaders();

    if (mInitCheck != OK) {
        //mTracks.clear();
    }
    AddTracks();
}

void FLVExtractor::AddTracks() {
    uint8_t i;

    // add video track
    if (mFLVContext.nbVideoTracks == 1)
    {
        ALOGW("add video codec id: %x\n", mFLVContext.VideoTracks[0].codecID);
        sp<MetaData> meta = new MetaData;
        if (mFLVContext.VideoTracks[0].codecID == VIDEO_TYPE_SORENSON_H263)
            meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_VIDEO_H263);
        else if (mFLVContext.VideoTracks[0].codecID == VIDEO_TYPE_AVC)
            meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_VIDEO_AVC);
        else if (mFLVContext.VideoTracks[0].codecID == VIDEO_TYPE_ON2_VP6 ||
            mFLVContext.VideoTracks[0].codecID == VIDEO_TYPE_ON2_VP6_AlPHA)
            meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_VIDEO_VP6);

        meta->setInt32(kKeyWidth, mFLVContext.VideoTracks[0].width);
        meta->setInt32(kKeyHeight, mFLVContext.VideoTracks[0].height);
        meta->setInt32(kKeyFrameRate, mFLVContext.VideoTracks[0].frameRate);
        meta->setInt64(kKeyDuration, (uint64_t)1000 * mFLVContext.VideoTracks[0].duration);
        mTracks.push();
        Track *track = &mTracks.editItemAt(mTracks.size() - 1);
        track->mKind = Track::VIDEO;
        track->mMeta = meta;
    }
    else // no support audio only now
    {
        mFLVContext.nbVideoTracks = 0;
        mFLVContext.nbAudioTracks = 0;
        mFLVContext.nbStreams= 0;
    }

    // add audio track
    if (mFLVContext.nbAudioTracks == 1)
    {
        ALOGW("add audio codec id: %x\n", mFLVContext.AudioTracks[0].codecID);
        sp<MetaData> meta = new MetaData;
        if (mFLVContext.AudioTracks[0].codecID == AUDIO_TYPE_PCM)
            meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_PCM);
        else if (mFLVContext.AudioTracks[0].codecID == AUDIO_TYPE_ADPCM)
            meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_ADPCM);
        else if (mFLVContext.AudioTracks[0].codecID == AUDIO_TYPE_MP3)
            meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_MPEG);
        else if (mFLVContext.AudioTracks[0].codecID == AUDIO_TYPE_LPCM)
            meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_LPCM);
        else if (mFLVContext.AudioTracks[0].codecID == AUDIO_TYPE_AAC)
            meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_AAC);

        meta->setInt32(kKeySampleRate, mFLVContext.AudioTracks[0].samplerate);
        meta->setInt32(kKeyChannelCount, mFLVContext.AudioTracks[0].channel);
        mTracks.push();
        Track *track = &mTracks.editItemAt(mTracks.size() - 1);
        track->mKind = Track::AUDIO;
        track->mMeta = meta;
    }
}

FLVExtractor::~FLVExtractor() {
}

size_t FLVExtractor::countTracks() {
    return mFLVContext.nbStreams;
}

sp<MediaSource> FLVExtractor::getTrack(size_t index) {
    return index < mFLVContext.nbStreams ? new FLVSource(this, index) : NULL;
}

sp<MetaData> FLVExtractor::getTrackMetaData(
        size_t index, uint32_t flags) {
    return index < mTracks.size() ? mTracks.editItemAt(index).mMeta : NULL;
}

sp<MetaData> FLVExtractor::getMetaData() {
    sp<MetaData> meta = new MetaData;

    if (mInitCheck == OK) {
        meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_CONTAINER_FLV);
    }

    return meta;
}

bool SniffFLV(
        const sp<DataSource> &source, String8 *mimeType, float *confidence,
        sp<AMessage> *) {
    char tmp[12];
    if (source->readAt(0, tmp, 12) < 12) {
        return false;
    }

    if (!memcmp(tmp, "FLV", 3)) {
        mimeType->setTo(MEDIA_MIMETYPE_CONTAINER_FLV);

        // Just a tad over the mp3 extractor's confidence, since
        // these .RM files may contain .mp3 content that otherwise would
        // mistakenly lead to us identifying the entire file as a .mp3 file.
        *confidence = 0.21;

        return true;
    }

    return false;
}

}  // namespace android
