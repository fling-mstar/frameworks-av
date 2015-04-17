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
#define LOG_TAG "ASFExtractor"
#include <utils/Log.h>
#include "math.h"
#include "ctype.h"
#include "include/ASFExtractor.h"

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


#define ASF_DBG(x)   (x)
//------------------------------------------------------------------------------
// Type and Structure Declaration
//------------------------------------------------------------------------------
#define U8_MAX 0xFF
#define U32_MAX 0xFFFFFFFFUL
#define S32_MAX 0xFFFFFFFFL
#define U64_MAX 0xFFFFFFFFFFFFFFFFULL


#define AUDIO_CODEC_WMAV1 0x160
#define AUDIO_CODEC_WMAV2 0x161
#define AUDIO_CODEC_WMAV3 0x162

#define MKTAG(a,b,c,d)      ((uint32_t)a | ((uint32_t)b << 8) | ((uint32_t)c << 16) | ((uint32_t)d << 24))
#define ASF_MAX_PAYLOAD_SIZE           0x10000  

/* top level object guids */
static const ST_ASF_GUID stGUID_ASF_Header =
{0x75B22630, {0x8E, 0x66}, {0xCF, 0x11}, {0xA6, 0xD9, 0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C}};

static const ST_ASF_GUID stGUID_ASF_Data =
{0x75B22636, {0x8E, 0x66}, {0xCF, 0x11}, {0xA6, 0xD9, 0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C}};

static const ST_ASF_GUID stGUID_ASF_Index =
{0xD6E229D3, {0xDA, 0x35}, {0xD1, 0x11}, {0x90, 0x34, 0x00, 0xA0, 0xC9, 0x03, 0x49, 0xBE}};

static const ST_ASF_GUID stGUID_ASF_SimpleIndex =
{0x33000890, {0xB1, 0xE5}, {0xCF, 0x11}, {0x89, 0xF4, 0x00, 0xA0, 0xC9, 0x03, 0x49, 0xCB}};

/* header level object guids */
static const ST_ASF_GUID stGUID_File_Property =
{0x8cabdca1, {0x47, 0xa9}, {0xcf, 0x11}, {0x8E, 0xe4, 0x00, 0xC0, 0x0C, 0x20, 0x53, 0x65}};

static const ST_ASF_GUID stGUID_Stream_Property =
{0xB7DC0791, {0xB7, 0xA9}, {0xCF, 0x11}, {0x8E, 0xE6, 0x00, 0xC0, 0x0C, 0x20, 0x53, 0x65}};

static const ST_ASF_GUID stGUID_Content_Description =
{0x75B22633, {0x8E, 0x66}, {0xCF, 0x11}, {0xA6, 0xD9, 0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C}};

static const ST_ASF_GUID stGUID_Content_Encryption =
{0x2211B3FB, {0x23, 0xBD}, {0xD2, 0x11}, {0xB4, 0xB7, 0x00, 0xA0, 0xC9, 0x55, 0xFC, 0x6E}};

static const ST_ASF_GUID stGUID_Extended_Content_Encryption =
{0x298AE614, {0x22, 0x26}, {0x17, 0x4C}, {0xB9, 0x35, 0xDA, 0xE0, 0x7E, 0xE9, 0x28, 0x9C}};

static const ST_ASF_GUID stGUID_Digital_Signature =
{0x2211B3FC, {0x23, 0xBD}, {0xD2, 0x11}, {0xB4, 0xB7, 0x00, 0xA0, 0xC9, 0x55, 0xFC, 0x6E}};

static const ST_ASF_GUID stGUID_Script_Command =
{0x1EFB1A30, {0x62, 0x0B}, {0xD0, 0x11}, {0xA3, 0x9B, 0x00, 0xA0, 0xC9, 0x03, 0x48, 0xF6}};

static const ST_ASF_GUID stGUID_Header_Extension =
{0x5FBF03B5, {0x2E, 0xA9}, {0xCF, 0x11}, {0x8E, 0xE3, 0x00, 0xC0, 0x0C, 0x20, 0x53, 0x65}};

static const ST_ASF_GUID stGUID_Marker =
{0xF487CD01, {0x51, 0xA9}, {0xCF, 0x11}, {0x8E, 0xE6, 0x00, 0xC0, 0x0C, 0x20, 0x53, 0x65}};

static const ST_ASF_GUID stGUID_Codec_List =
{0x86D15240, {0x1D, 0x31}, {0xD0, 0x11}, {0xA3, 0xA4, 0x00, 0xA0, 0xC9, 0x03, 0x48, 0xF6}};

#if 1 //(ENABLE_PLAYREADY == 1)
static const ST_ASF_GUID stGUID_Protection_System_Identifier =
{0x9A04F079, {0x40, 0x98}, {0x86, 0x42}, {0xAB, 0x92, 0xE6, 0x5B, 0xE0, 0x88, 0x5F, 0x95}};

static const ST_ASF_GUID stGUID_Content_Protection_System_Microsoft_PlayReady =
{0xF4637010, {0xC3, 0x03}, {0xCD, 0x42}, {0xB9, 0x32, 0xB4, 0x6A, 0xDF, 0xF3, 0xA6, 0xA5}};
#endif

/* TODO */
static const ST_ASF_GUID stGUID_Stream_Bitrate_Property =
{0x7BF875CE, {0x8D, 0x46}, {0xD1, 0x11}, {0x8D, 0x82, 0x00, 0x60, 0x97, 0xC9, 0xA2, 0xB2}};

static const ST_ASF_GUID stGUID_Padding =
{0x1806D474, {0xDF, 0xCA}, {0x09, 0x45}, {0xA4, 0xBA, 0x9A, 0xAB, 0xCB, 0x96, 0xAA, 0xE8}};

static const ST_ASF_GUID stGUID_Extended_Content_Description =
{0xD2D0A440, {0x07, 0xE3}, {0xD2, 0x11}, {0x97, 0xF0, 0x00, 0xA0, 0xC9, 0x5E, 0xA8, 0x50}};

/* header extension level object guids */
static const ST_ASF_GUID stGUID_Metadata =
{0xC5F8CBEA, {0xAF, 0x5B}, {0x77, 0x48}, {0x84, 0x67, 0xAA, 0x8C, 0x44, 0xFA, 0x4C, 0xCA}};

/* TODO */
static const ST_ASF_GUID stGUID_Language_List =
{0x7C4346A9, {0xE0, 0xEF}, {0xFC, 0x4B}, {0xB2, 0x29, 0x39, 0x3E, 0xDE, 0x41, 0x5C, 0x85}};

static const ST_ASF_GUID stGUID_Extended_Stream_Property =
{0x14E6A5CB, {0x72, 0xC6}, {0x32, 0x43}, {0x83, 0x99, 0xA9, 0x69, 0x52, 0x06, 0x5B, 0x5A}};

static const ST_ASF_GUID stGUID_Advanced_Mutual_Exclusion =
{0xA08649CF, {0x75, 0x47}, {0x70, 0x46}, {0x8A, 0x16, 0x6E, 0x35, 0x35, 0x75, 0x66, 0xCD}};

static const ST_ASF_GUID stGUID_Stream_Prioritization =
{0xD4FED15B, {0xD3, 0x88}, {0x4F, 0x45}, {0x81, 0xF0, 0xED, 0x5C, 0x45, 0x99, 0x9E, 0x24}};

/* stream type guids */
static const ST_ASF_GUID stGUID_Stream_Type_Audio =
{0xF8699E40, {0x4D, 0x5B}, {0xCF, 0x11}, {0xA8, 0xFD, 0x00, 0x80, 0x5F, 0x5C, 0x44, 0x2B}};

static const ST_ASF_GUID stGUID_Stream_Type_Video =
{0xbc19efc0, {0x4D, 0x5B}, {0xCF, 0x11}, {0xA8, 0xFD, 0x00, 0x80, 0x5F, 0x5C, 0x44, 0x2B}};

static const ST_ASF_GUID stGUID_Stream_Type_Command =
{0x59DACFC0, {0xE6, 0x59}, {0xD0, 0x11}, {0xA3, 0xAC, 0x00, 0xA0, 0xC9, 0x03, 0x48, 0xF6}};

static const ST_ASF_GUID stGUID_Stream_Type_Extended =
{0x3AFB65E2, {0xEF, 0x47}, {0xF2, 0x40}, {0xAC, 0x2C, 0x70, 0xA9, 0x0D, 0x71, 0xD3, 0x43}};

static const ST_ASF_GUID stGUID_Stream_Type_Extended_Audio =
{0x31178C9D, {0xE1, 0x03}, {0x28, 0x45}, {0xB5, 0x82, 0x3D, 0xF9, 0xDB, 0x22, 0xF5, 0x03}};

static const ST_ASF_GUID stGUID_Netflix_Index =
{0x7E1760AE, {0xCE, 0xC6}, {0x3E, 0x4F}, {0xAD, 0x26, 0x2D, 0x4B, 0xC4, 0x76, 0x8B, 0x52}};




namespace android {

const CodecTag MFP_WAVEFORMATEX_AUDIO_TAGS[] =
{
    { MEDIA_MIMETYPE_AUDIO_MPEG_LAYER_II, 0x50 },
    { MEDIA_MIMETYPE_AUDIO_MPEG, 0x55 },
    { MEDIA_MIMETYPE_AUDIO_AC3, 0x2000 },
    { MEDIA_MIMETYPE_AUDIO_LPCM, 0x01 },
    { MEDIA_MIMETYPE_AUDIO_PCM_u8, 0x01 }, /* must come after s16le in this list */
    { MEDIA_MIMETYPE_AUDIO_PCM_alaw, 0x06 },
    { MEDIA_MIMETYPE_AUDIO_PCM_ulaw, 0x07 },
    { MEDIA_MIMETYPE_AUDIO_ADPCM, 0x02 },
    { MEDIA_MIMETYPE_AUDIO_ADPCM, 0x11 },
    { MEDIA_MIMETYPE_AUDIO_ADPCM, 0x45 },
    { MEDIA_MIMETYPE_AUDIO_ADPCM, 0x61 },  /* rogue format number */
    { MEDIA_MIMETYPE_AUDIO_ADPCM, 0x62 },  /* rogue format number */
    { MEDIA_MIMETYPE_AUDIO_AAC_ADTS, 0xFF },
    { MEDIA_MIMETYPE_AUDIO_DTS, 0x2001 },
    { MEDIA_MIMETYPE_AUDIO_WMA, 0x160 },
    { MEDIA_MIMETYPE_AUDIO_WMA, 0x161 },
    { MEDIA_MIMETYPE_AUDIO_WMA, 0x162 },

    // Currently Un-supported
//    { E_VDP_CODEC_ID_VORBIS, ('V'<<8)+'o' }, //HACK/FIXME, does vorbis in WAV/AVI have an (in)official id?
    { MEDIA_MIMETYPE_AUDIO_UNKNOW, 0 },
};

const CodecTag MFP_ASF_VIDEO_TAGS[] =
{
    { MEDIA_MIMETYPE_VIDEO_DIVX4, MKTAG('D', 'I', 'V', 'X') },
    { MEDIA_MIMETYPE_VIDEO_DIVX, MKTAG('D', 'X', '5', '0') },
    { MEDIA_MIMETYPE_VIDEO_MPEG4, MKTAG('X', 'V', 'I', 'D') },
    { MEDIA_MIMETYPE_VIDEO_MPEG4, MKTAG('M', 'P', '4', 'S') },
    { MEDIA_MIMETYPE_VIDEO_MPEG2, MKTAG('m', 'p', 'g', '2') },
    { MEDIA_MIMETYPE_VIDEO_DIVX3, MKTAG('M', 'P', '4', '3') },
    { MEDIA_MIMETYPE_VIDEO_WMV3, MKTAG('W', 'M', 'V', '3') },
    { MEDIA_MIMETYPE_VIDEO_VC1, MKTAG('W', 'V', 'C', '1') },

    // Currently Un-supported
    //    { E_VDP_CODEC_ID_H263, MKTAG('H', '2', '6', '3') },
    { MEDIA_MIMETYPE_VIDEO_DIVX3, MKTAG('D', 'I', 'V', '3') },
    { MEDIA_MIMETYPE_VIDEO_AVC, MKTAG('H', '2', '6', '4') },
    { MEDIA_MIMETYPE_VIDEO_MJPG, MKTAG('M', 'J', 'P', 'G') },
    //    { E_VDP_CODEC_ID_MJPEG, MKTAG('M', 'J', 'P', 'G') },
    //    { E_VDP_CODEC_ID_MJPEG, MKTAG('L', 'J', 'P', 'G') },
    //    { E_VDP_CODEC_ID_MJPEG, MKTAG('J', 'P', 'G', 'L') },
    //    { E_VDP_CODEC_ID_H263P, MKTAG('H', '2', '6', '3') },
    //    { E_VDP_CODEC_ID_H263I, MKTAG('I', '2', '6', '3') },
    //    { E_VDP_CODEC_ID_H263P, MKTAG('U', '2', '6', '3') },
    //    { E_VDP_CODEC_ID_H263P, MKTAG('v', 'i', 'v', '1') },
    //    { E_VDP_CODEC_ID_MPEG4, MKTAG('M', '4', 'S', '2') },
    //    { E_VDP_CODEC_ID_MSMPEG4V3, MKTAG('D', 'I', 'V', '3') },
    //    { E_VDP_CODEC_ID_MSMPEG4V3, MKTAG('M', 'P', '4', '3') },
    //    { E_VDP_CODEC_ID_MSMPEG4V3, MKTAG('M', 'P', 'G', '3') },
    //    { E_VDP_CODEC_ID_MSMPEG4V3, MKTAG('D', 'I', 'V', '5') },
    //    { E_VDP_CODEC_ID_MSMPEG4V3, MKTAG('D', 'I', 'V', '6') },
    //    { E_VDP_CODEC_ID_MSMPEG4V3, MKTAG('D', 'I', 'V', '4') },
    //    { E_VDP_CODEC_ID_MSMPEG4V3, MKTAG('A', 'P', '4', '1') },
    //    { E_VDP_CODEC_ID_MSMPEG4V3, MKTAG('C', 'O', 'L', '1') },
    //    { E_VDP_CODEC_ID_MSMPEG4V3, MKTAG('C', 'O', 'L', '0') },
    //    { E_VDP_CODEC_ID_MSMPEG4V2, MKTAG('M', 'P', '4', '2') },
    //    { E_VDP_CODEC_ID_MSMPEG4V2, MKTAG('D', 'I', 'V', '2') },
    //    { E_VDP_CODEC_ID_MSMPEG4V1, MKTAG('M', 'P', 'G', '4') },
    //    { E_VDP_CODEC_ID_MPEG4, MKTAG(0x04, 0, 0, 0) },
    //    { E_VDP_CODEC_ID_MPEG4, MKTAG('D', 'I', 'V', '1') },
    //    { E_VDP_CODEC_ID_MPEG4, MKTAG('B', 'L', 'Z', '0') },
    //    { E_VDP_CODEC_ID_MPEG4, MKTAG('m', 'p', '4', 'v') },
    //    { E_VDP_CODEC_ID_MPEG4, MKTAG('U', 'M', 'P', '4') },
    //    { E_VDP_CODEC_ID_MPEG1VIDEO, MKTAG('m', 'p', 'e', 'g') },
    //    { E_VDP_CODEC_ID_MPEG1VIDEO, MKTAG('m', 'p', 'g', '1') },
    //    { E_VDP_CODEC_ID_MPEG1VIDEO, MKTAG('P', 'I', 'M', '1') },
    //    { E_VDP_CODEC_ID_MPEG1VIDEO, MKTAG('V', 'C', 'R', '2') },
    //    { E_VDP_CODEC_ID_MPEG1VIDEO, 0x10000001 },
    //    { E_VDP_CODEC_ID_MPEG2VIDEO, 0x10000002 },
    //    { E_VDP_CODEC_ID_WMV1, MKTAG('W', 'M', 'V', '1') },
    //    { E_VDP_CODEC_ID_WMV2, MKTAG('W', 'M', 'V', '2') },
    //    { E_VDP_CODEC_ID_DVVIDEO, MKTAG('d', 'v', 's', 'd') },
    //    { E_VDP_CODEC_ID_DVVIDEO, MKTAG('d', 'v', 'h', 'd') },
    //    { E_VDP_CODEC_ID_DVVIDEO, MKTAG('d', 'v', 's', 'l') },
    //    { E_VDP_CODEC_ID_DVVIDEO, MKTAG('d', 'v', '2', '5') },
    //    { E_VDP_CODEC_ID_LJPEG, MKTAG('L', 'J', 'P', 'G') },
    //    { E_VDP_CODEC_ID_HUFFYUV, MKTAG('H', 'F', 'Y', 'U') },
    //    { E_VDP_CODEC_ID_CYUV, MKTAG('C', 'Y', 'U', 'V') },
    //    { E_VDP_CODEC_ID_RAWVIDEO, MKTAG('Y', '4', '2', '2') },
    //    { E_VDP_CODEC_ID_RAWVIDEO, MKTAG('I', '4', '2', '0') },
    //    { E_VDP_CODEC_ID_INDEO3, MKTAG('I', 'V', '3', '1') },
    //    { E_VDP_CODEC_ID_INDEO3, MKTAG('I', 'V', '3', '2') },
    //    { E_VDP_CODEC_ID_VP3, MKTAG('V', 'P', '3', '1') },
    //    { E_VDP_CODEC_ID_ASV1, MKTAG('A', 'S', 'V', '1') },
    //    { E_VDP_CODEC_ID_ASV2, MKTAG('A', 'S', 'V', '2') },
    //    { E_VDP_CODEC_ID_VCR1, MKTAG('V', 'C', 'R', '1') },
    //    { E_VDP_CODEC_ID_FFV1, MKTAG('F', 'F', 'V', '1') },
    //    { E_VDP_CODEC_ID_XAN_WC4, MKTAG('X', 'x', 'a', 'n') },
    //    { E_VDP_CODEC_ID_MSRLE, MKTAG('m', 'r', 'l', 'e') },
    //    { E_VDP_CODEC_ID_MSRLE, MKTAG(0x1, 0x0, 0x0, 0x0) },
    //    { E_VDP_CODEC_ID_MSVIDEO1, MKTAG('M', 'S', 'V', 'C') },
    //    { E_VDP_CODEC_ID_MSVIDEO1, MKTAG('m', 's', 'v', 'c') },
    //    { E_VDP_CODEC_ID_MSVIDEO1, MKTAG('C', 'R', 'A', 'M') },
    //    { E_VDP_CODEC_ID_MSVIDEO1, MKTAG('c', 'r', 'a', 'm') },
    //    { E_VDP_CODEC_ID_MSVIDEO1, MKTAG('W', 'H', 'A', 'M') },
    //    { E_VDP_CODEC_ID_MSVIDEO1, MKTAG('w', 'h', 'a', 'm') },
    //    { E_VDP_CODEC_ID_CINEPAK, MKTAG('c', 'v', 'i', 'd') },
    //    { E_VDP_CODEC_ID_TRUEMOTION1, MKTAG('D', 'U', 'C', 'K') },
    //    { E_VDP_CODEC_ID_MSZH, MKTAG('M', 'S', 'Z', 'H') },
    //    { E_VDP_CODEC_ID_ZLIB, MKTAG('Z', 'L', 'I', 'B') },
    //    { E_VDP_CODEC_ID_4XM, MKTAG('4', 'X', 'M', 'V') },
    //    { E_VDP_CODEC_ID_FLV1, MKTAG('F', 'L', 'V', '1') },
    //    { E_VDP_CODEC_ID_SVQ1, MKTAG('s', 'v', 'q', '1') },

    { MEDIA_MIMETYPE_VIDEO_UNKNOW, 0 },
};

struct ASFExtractor::ASFSource : public MediaSource {
    ASFSource(const sp<DataSource> &source, const sp<ASFExtractor> &extractor, size_t trackIndex);

    virtual status_t start(MetaData *params);
    virtual status_t stop();

    virtual sp<MetaData> getFormat();

    virtual status_t read(MediaBuffer **buffer, const ReadOptions *options);

protected:
    virtual ~ASFSource();

private:
    
    status_t ParseASFPacketHeader(const sp<DataSource> &source, off64_t offset, size_t* pu32packetSize, size_t* pu32packetOffset, int32_t* ps32payloadNum);
    status_t ParseASFPayloadHeader(const sp<DataSource> &source, off64_t offset, size_t* pu32payloadSize, size_t* pu32payloadOffset, int64_t *ps64Pts);
    sp<DataSource> mDataSource;
    sp<ASFExtractor> mExtractor;
    size_t mTrackIndex;
    const ASFExtractor::Track &mTrack;
    MediaBufferGroup *mBufferGroup;
    // size_t mSampleIndex;
    List<MediaBuffer *> mPendingFrames;
    off64_t mOffset;
    int64_t mCurrentTimeUs;
    uint32_t mu32CurrentPacket;
    int32_t ms32PayloadLeft;
    bool mbIsMultiPayloads;
    uint8_t mu8SequenceLType;
    uint8_t mu8PaddingLType;
    uint8_t mu8PacketLType;
    uint32_t mu32PacketSendTime;

    uint8_t mu8ReplicatedDLType;
    uint8_t mu8OffseIMOLType;
    uint8_t mu8MediaONLType;
    uint8_t mu8StreamNLType;
    uint8_t mu8PayloadLType;
    uint8_t mu8CurPayloadStreamNumber;

    DISALLOW_EVIL_CONSTRUCTORS(ASFSource);
};

ASFExtractor::ASFSource::ASFSource(const sp<DataSource> &source, const sp<ASFExtractor> &extractor, size_t trackIndex)
: mDataSource(source),
mExtractor(extractor),
mTrackIndex(trackIndex),
mTrack(mExtractor->mTracks.itemAt(trackIndex)),
mBufferGroup(NULL) {
    //sp<MetaData> meta = mExtractor->mTracks.itemAt(index).mMeta;
}

ASFExtractor::ASFSource::~ASFSource() {
    if (mBufferGroup) {
        stop();
    }
}

status_t ASFExtractor::ASFSource::start(MetaData *params) {
    CHECK(!mBufferGroup);
    const size_t kMaxFrameSize = ASF_MAX_PAYLOAD_SIZE;

    mBufferGroup = new MediaBufferGroup;

    mBufferGroup->add_buffer(new MediaBuffer(kMaxFrameSize));
    // mSampleIndex = 0;

    //  const char *mime;
    //  CHECK(mTrack.mMeta->findCString(kKeyMIMEType, &mime));
    if(mExtractor->g_pASFPayload == NULL)
    {
        ALOGD("malloc asf payload.");
        mExtractor->g_pASFPayload = (PASF_PAYLOAD_TYPE)malloc(sizeof(ASF_PAYLOAD_TYPE));
        if(mExtractor->g_pASFPayload == NULL)
        {
            delete mBufferGroup;
            mBufferGroup = NULL;
        }
    }
    ms32PayloadLeft = 0;
    mu32CurrentPacket = 0;
    mOffset = mExtractor->g_pHeader->u64Size + 0x32; //Jump Data Object Header(0x32)

    return OK;
}

status_t ASFExtractor::ASFSource::stop() {
    CHECK(mBufferGroup);

    delete mBufferGroup;
    mBufferGroup = NULL;
    if(mExtractor->g_pASFPayload != NULL)
    {
        free(mExtractor->g_pASFPayload);
        mExtractor->g_pASFPayload = NULL;
    }

    return OK;
}

sp<MetaData> ASFExtractor::ASFSource::getFormat() {
    return mTrack.mMeta;
}
status_t ASFExtractor::ASFSource::ParseASFPacketHeader(const sp<DataSource> &source, off64_t offset, 
    size_t* pu32packetSize, size_t* pu32packetOffset, int32_t* ps32payloadNum)
{
    #define ASF_PACKET_HEADER_MAX_SIZE   32
    uint8_t u8Buf[ASF_PACKET_HEADER_MAX_SIZE];
    uint32_t i = 0;
    uint32_t u32ErrorCLength;

    memset(u8Buf, 0 , sizeof(u8Buf));
    if (source->readAt(offset, u8Buf,ASF_PACKET_HEADER_MAX_SIZE) != ASF_PACKET_HEADER_MAX_SIZE) 
    {
            
        ALOGE("ERROR_IO");  
        return ERROR_IO;
    }
    // Error Correction Flags
    //Check error packets.
    if(u8Buf[0] != 0x82)
    {
        uint8_t u8SkipPacketLimit = 10;
        while( (u8Buf[0] != 0x82) && (u8SkipPacketLimit > 0))
        {        
            i += mExtractor->g_pFile->u32PacketSize;
            if(source->readAt(offset+i, u8Buf, 1) != 1)
            {            
                ALOGE("ERROR_IO");  
                return ERROR_IO;
            }

            u8SkipPacketLimit--;
        }

        u32ErrorCLength = u8Buf[0] & 0x0F;

        if((u32ErrorCLength != 0) && (u32ErrorCLength != 2) )
        {
            ALOGE("can not find valid packet in next 10 packet data. stop play.\n");
            return 0xFFFFFFFF;
        }
        if(source->readAt(offset+i, u8Buf, ASF_PACKET_HEADER_MAX_SIZE) != ASF_PACKET_HEADER_MAX_SIZE)
        {            
            ALOGD("ERROR_IO");  
            return ERROR_IO;
        }
        i = 0;
    }

    if(u8Buf[0] & 0x80)
    {
        u32ErrorCLength = u8Buf[0] & 0x0F;
        i += u32ErrorCLength;
        i++;
    }


    mbIsMultiPayloads = u8Buf[i] & 0x01;
    mu8SequenceLType = (u8Buf[i] & 0x06) >> 1;
    mu8PaddingLType = (u8Buf[i] & 0x18) >> 3;
    mu8PacketLType = (u8Buf[i] & 0x60) >> 5;
    //ALOGD("##### Function:[%s] Line:[%d] bIsMultiPayloads:[%u], u8SequenceLType:[%u], u8PaddingLType:[%u], u8PacketLType:[%u]!! #####\n",__FUNCTION__,__LINE__,mbIsMultiPayloads,mu8SequenceLType,mu8PaddingLType,mu8PacketLType);
    i++;  //Jump Length Type Flags

    mu8ReplicatedDLType = u8Buf[i] & 0x03;
    mu8OffseIMOLType = (u8Buf[i] & 0x0C) >> 2;
    mu8MediaONLType = (u8Buf[i] & 0x30) >> 4;
    mu8StreamNLType = (u8Buf[i] & 0xC0) >> 6;
    //ALOGD("##### Function:[%s] Line:[%d] u8ReplicatedDLType:[%u], u8OffseIMOLType:[%u], u8MediaONLType:[%u], u8StreamNLType:[%u]!! #####\n",__FUNCTION__,__LINE__,mu8ReplicatedDLType,mu8OffseIMOLType,mu8MediaONLType,mu8StreamNLType);
    i++;  //Jump Property Flags

    if(mu8PacketLType)
    {        
        if ((1 << (mu8PacketLType - 1)))
        {
            memcpy(pu32packetSize, &u8Buf[i], (1 << (mu8PacketLType - 1)));
        }
        i += (1 << (mu8PacketLType - 1));
    }
    else
    {
        *pu32packetSize = mExtractor->g_pFile->u32PacketSize;
    }
    //ALOGD("##### File:[%s] Function:[%s] Line:[%d]##### u32PacketSize=[0x%lx]\n",__FILE__,__FUNCTION__,__LINE__,*pu32packetSize);

    if(mu8SequenceLType)
    {
        i += (1 << (mu8SequenceLType - 1));
    }

    if(mu8PaddingLType)
    {
        i += (1 << (mu8PaddingLType - 1));
    }

    memcpy(&mu32PacketSendTime, &u8Buf[i], 4);
    i += 4;    //Jump Send Time
    i += 2;    //Jump Duration

    //------Payload data----------//
    if(mbIsMultiPayloads)
    {
        *ps32payloadNum = u8Buf[i] & 0x3F;
        mu8PayloadLType = (u8Buf[i] & 0xC0) >> 6;
        i += 1;    //Jump Payload Flags
    }
    else
    {
        *ps32payloadNum = 1;
    }
    *pu32packetOffset = i;
    return OK;
}



status_t ASFExtractor::ASFSource::ParseASFPayloadHeader(const sp<DataSource> &source, off64_t offset, size_t* pu32payloadSize, size_t* pu32payloadOffset, int64_t *ps64PtsUs)
{    
    #define ASF_PAYLOAD_HEADER_MAX_SIZE   32
    uint8_t u8Buf[ASF_PAYLOAD_HEADER_MAX_SIZE];
    uint32_t i = 0,u32ReplicatedDLength = 0, u32PTS = 0;
    uint8_t u8PresentationTimeDelta = 0;
    
    if (source->readAt(offset, u8Buf,
                    ASF_PAYLOAD_HEADER_MAX_SIZE) != (ssize_t)ASF_PAYLOAD_HEADER_MAX_SIZE) 
    {
            
        ALOGD("ERROR_IO");  
        return ERROR_IO;
    }

    mu8CurPayloadStreamNumber = u8Buf[i];
    i += 1;    //Jump Stream Number

    if(mu8MediaONLType)
    {
        i += (1 << (mu8MediaONLType - 1));
    }
    if(mu8OffseIMOLType)
    {
        i += (1 << (mu8OffseIMOLType - 1));
    }
    if(mu8ReplicatedDLType)
    {
        memcpy(&u32ReplicatedDLength, &u8Buf[i], (1 << (mu8ReplicatedDLType - 1)));
        i += (1 << (mu8ReplicatedDLType - 1));
    }

    if(u32ReplicatedDLength > mExtractor->g_pFile->u32PacketSize)
    {
        ALOGE("u32ReplicatedDLength: %d is abnormal.\n", u32ReplicatedDLength);
        return ERROR_MALFORMED;
    }

    if(u32ReplicatedDLength == 0x1)  //Compressed payload data
    {
        memcpy(&u8PresentationTimeDelta, &u8Buf[i], 1);
        i++;  //Jump Presentation Time Delta

        //Sub-Payload Data
        memcpy(pu32payloadSize, &u8Buf[i], 1);
        //ALOGD("Case 1: %ld.\n", *pu32payloadSize);
        i++; //Jump Sub-Payload Data Length
        *ps64PtsUs = (int64_t)mu32PacketSendTime;
    }
    else  //non-compressed payload data
    {
        if(u32ReplicatedDLength < ASF_PAYLOAD_HEADER_MAX_SIZE)
        {
            // Tricky or follow spec.
            memcpy(pu32payloadSize, &u8Buf[i], 4);
            //ALOGD("Case 2: %ld.\n", *pu32payloadSize);
            if(u32ReplicatedDLength >= 8)
            {
                i += 4;
                memcpy(&u32PTS, &u8Buf[i], 4);
                *ps64PtsUs = ((int64_t)(u32PTS - (uint32_t)mExtractor->g_pFile->u64Preroll))*1000;
                //ALOGD("payload time:%d,%d,%lld",u32PTS,(uint32_t)mExtractor->g_pFile->u64Preroll,*ps64PtsUs);
                i += (u32ReplicatedDLength -4);
            }
            else
            {
                i += u32ReplicatedDLength;
                *ps64PtsUs = (int64_t)mu32PacketSendTime;
            }

            if(mbIsMultiPayloads)
            {
                if(mu8PayloadLType)
                {
                    if(i >= (ASF_PAYLOAD_HEADER_MAX_SIZE - 1))
                    {
                        ALOGE("parser asf payload header error.\n");
                        return ERROR_MALFORMED;
                    }
                    
                    memcpy(pu32payloadSize, &u8Buf[i], (1 << (mu8PayloadLType - 1)));
                    //ALOGD("Case 3: %ld.\n", *pu32payloadSize);
                    i += (1 << (mu8PayloadLType - 1)); //Jump Payload Length
                }
            }
        }
        else
        {
            ALOGE("abonamal packet size");
            *pu32payloadSize = 0xFFFFFFFF;
        }
    }
    *pu32payloadOffset = i;

    if((*pu32payloadOffset) > mExtractor->g_pFile->u32PacketSize)
    {
        ALOGE("u32PayloadLength: %d is abnormal.\n", *pu32payloadOffset);
        return ERROR_MALFORMED;
    }

    return OK;
}

status_t ASFExtractor::ASFSource::read(MediaBuffer **out, const ReadOptions *options) 
{
    *out = NULL;

    int64_t seekTimeUs;
    ReadOptions::SeekMode mode;
    size_t u32packetSize, u32packetOffset, u32PayloadSize, u32PayloadOffset;
    bool bFindAudioPayload = false;
    MediaBuffer *buffer = NULL;

    if(mTrack.mKind != Track::AUDIO)
    {
        ALOGE("Now only support audio track.");
        return ERROR_VIDEO_UNSUPPORT;
    }
    
    if (options && options->getSeekTo(&seekTimeUs, &mode)) 
    {
        if (mTrack.mKind == Track::AUDIO)
        {
            if((mExtractor->mASFContext.nbAudioTracks > 0) && (mExtractor->mASFContext.AudioTracks[0].duration > 0)) 
            {
                mu32CurrentPacket = seekTimeUs/1000* mExtractor->g_pFile->u64DataPacketsCount/ mExtractor->mASFContext.AudioTracks[0].duration;
                mCurrentTimeUs = seekTimeUs;
                //Jump Data Object Header(0x32)
                // 16:Object ID, 8:Object Size, 16:File ID, 8:Total Data packets, 2:Reserved
                mOffset = mExtractor->g_pHeader->u64Size + 0x32 + (mu32CurrentPacket * mExtractor->g_pFile->u32PacketSize);
                ms32PayloadLeft = 0;
            }
            else
            {
                ALOGE("can not find audio track.");
                return ERROR_AUDIO_UNSUPPORT;
            }
        }
    }
    //send one payload every read.

    while((ms32PayloadLeft > 0) && (!bFindAudioPayload))
    {
        if(ParseASFPayloadHeader(mDataSource, mOffset, &u32PayloadSize, &u32PayloadOffset, &mCurrentTimeUs) != OK)
        {
            return ERROR_END_OF_STREAM;
        }

        mOffset += u32PayloadOffset;
        ms32PayloadLeft--;
        if(mu8CurPayloadStreamNumber != mExtractor->mASFContext.AudioTracks[0].u8StreamID)
        {
            ALOGD("skip non audio stream.");
            continue;
        }

        if(u32PayloadSize > (ASF_MAX_PAYLOAD_SIZE))
        {
            ALOGE("Payload size is too big.");
            return ERROR_OUT_OF_RANGE;
        }
        status_t err = mBufferGroup->acquire_buffer(&buffer);
        if (err != OK) 
        {
            return err;
        }

        //fill payload data
        if (mDataSource->readAt(mOffset, ((uint8_t*)buffer->data()),
                u32PayloadSize) != (ssize_t)u32PayloadSize) 
        {
                
            ALOGD("ERROR_IO");  
            return ERROR_END_OF_STREAM;
        }

        
        mOffset += u32PayloadSize;        
        bFindAudioPayload = true;
        goto readend;
    }    
    
    while(!bFindAudioPayload)
    {
        //find next packet.        
        mOffset = mExtractor->g_pHeader->u64Size + 0x32 + (mu32CurrentPacket * mExtractor->g_pFile->u32PacketSize);
        if (ParseASFPacketHeader(mDataSource, mOffset, &u32packetSize, &u32packetOffset, &ms32PayloadLeft) != OK) {
            return ERROR_END_OF_STREAM;
        }
        mu32CurrentPacket++;

        mOffset += u32packetOffset;
        while(ms32PayloadLeft > 0)
        {
            if(ParseASFPayloadHeader(mDataSource, mOffset, &u32PayloadSize, &u32PayloadOffset, &mCurrentTimeUs) != OK)
            {
                return ERROR_END_OF_STREAM;
            }
    
            mOffset += u32PayloadOffset;
            ms32PayloadLeft--;
            if(mu8CurPayloadStreamNumber != mExtractor->mASFContext.AudioTracks[0].u8StreamID)
            {
                ALOGD("skip non audio stream.");
                continue;
            }

            if(u32PayloadSize > (ASF_MAX_PAYLOAD_SIZE))
            {
                ALOGE("Payload size is too big.");
                return ERROR_OUT_OF_RANGE;
            }
            status_t err = mBufferGroup->acquire_buffer(&buffer);
            if (err != OK) {
                return err;
            }
    
            if (mDataSource->readAt(mOffset, ((uint8_t*)buffer->data()),
                    u32PayloadSize) != (ssize_t)u32PayloadSize) 
            {
                    
                ALOGD("ERROR_IO");  
                return ERROR_END_OF_STREAM;
            }
            
            mOffset += u32PayloadSize;            
            bFindAudioPayload = true;
            break;
        }
    }

    if(buffer == NULL)
    {
        ALOGE("Parse asf data fail.");
        return ERROR_AUDIO_UNSUPPORT;
    }
readend:
    buffer->set_range(0, u32PayloadSize);
    buffer->meta_data()->setInt64(kKeyTime, mCurrentTimeUs);
    buffer->meta_data()->setInt32(kKeyIsSyncFrame, 1);

    *out = buffer;
    return OK;
}


int ASFExtractor::matchGUID(const ST_ASF_GUID *pstGUID1, const ST_ASF_GUID *pstGUID2)
{
    if ((pstGUID1->u32V1 != pstGUID2->u32V1) ||
        (memcmp(pstGUID1->u8V2, pstGUID2->u8V2, 2)) ||
        (memcmp(pstGUID1->u8V3, pstGUID2->u8V3, 2)) ||
        (memcmp(pstGUID1->u8V4, pstGUID2->u8V4, 8))) {
        return 0;
    }

    return 1;
}

ASF_GUID_TYPE ASFExtractor::getObjectType(const ST_ASF_GUID *pstGUID)
{
    ASF_GUID_TYPE eReturn = E_GUID_UNKNOWN;

    if (matchGUID(pstGUID, &stGUID_ASF_Header))
        eReturn = E_GUID_HEADER;
    else if (matchGUID(pstGUID, &stGUID_ASF_Data))
        eReturn = E_GUID_DATA;
    else if (matchGUID(pstGUID, &stGUID_ASF_Index))
        eReturn = E_GUID_INDEX;
    else if (matchGUID(pstGUID, &stGUID_ASF_SimpleIndex))
        eReturn = E_GUID_SIMPLE_INDEX;
    else if (matchGUID(pstGUID, &stGUID_File_Property))
        eReturn = E_GUID_FILE_PROPERTIES;
    else if (matchGUID(pstGUID, &stGUID_Stream_Property))
        eReturn = E_GUID_STREAM_PROPERTIES;
    else if (matchGUID(pstGUID, &stGUID_Content_Description))
        eReturn = E_GUID_CONTENT_DESCRIPTION;
    else if (matchGUID(pstGUID, &stGUID_Content_Encryption))
        eReturn = E_GUID_CONTENT_ENCRYPTION;
    else if (matchGUID(pstGUID, &stGUID_Extended_Content_Encryption))
        eReturn = E_GUID_EXTENDED_CONTENT_ENCRYPTION;
    else if (matchGUID(pstGUID, &stGUID_Digital_Signature))
        eReturn = E_GUID_DIGITAL_SIGNATURE;
    else if (matchGUID(pstGUID, &stGUID_Script_Command))
        eReturn = E_GUID_SCRIPT_COMMAND;
    else if (matchGUID(pstGUID, &stGUID_Header_Extension))
        eReturn = E_GUID_HEADER_EXTENSION;
    else if (matchGUID(pstGUID, &stGUID_Marker))
        eReturn = E_GUID_MARKER;
    else if (matchGUID(pstGUID, &stGUID_Codec_List))
        eReturn = E_GUID_CODEC_LIST;
    else if (matchGUID(pstGUID, &stGUID_Stream_Bitrate_Property))
        eReturn = E_GUID_STREAM_BITRATE_PROPERTIES;
    else if (matchGUID(pstGUID, &stGUID_Padding))
        eReturn = E_GUID_PADDING;
    else if (matchGUID(pstGUID, &stGUID_Extended_Content_Description))
        eReturn = E_GUID_EXTENDED_CONTENT_DESCRIPTION;
    else if (matchGUID(pstGUID, &stGUID_Metadata))
        eReturn = E_GUID_METADATA;
    else if (matchGUID(pstGUID, &stGUID_Language_List))
        eReturn = E_GUID_LANGUAGE_LIST;
    else if (matchGUID(pstGUID, &stGUID_Extended_Stream_Property))
        eReturn = E_GUID_EXTENDED_STREAM_PROPERTIES;
    else if (matchGUID(pstGUID, &stGUID_Advanced_Mutual_Exclusion))
        eReturn = E_GUID_ADVANCED_MUTUAL_EXCLUSION;
    else if (matchGUID(pstGUID, &stGUID_Stream_Prioritization))
        eReturn = E_GUID_STREAM_PRIORITIZATION;
    else if (matchGUID(pstGUID, &stGUID_Netflix_Index))
        eReturn = E_GUID_NETFLIX_INDEX;
#if 1 //(ENABLE_PLAYREADY == 1)
    else if (matchGUID(pstGUID, &stGUID_Protection_System_Identifier))
        eReturn = E_GUID_PROTECTED_SYSTEM_IDENTIFIER;
    else if (matchGUID(pstGUID, &stGUID_Content_Protection_System_Microsoft_PlayReady))
        eReturn = E_GUID_CONTENT_PROTECTION_SYSTEM_MICROSOFT_PLAYREADY;
#endif

    return eReturn;
}

ASF_GUID_TYPE ASFExtractor::getStreamType(const ST_ASF_GUID *pstGUID)
{
    ASF_GUID_TYPE eReturn = E_GUID_UNKNOWN;

    if (matchGUID(pstGUID, &stGUID_Stream_Type_Audio))
        eReturn = E_GUID_STREAM_TYPE_AUDIO;
    else if (matchGUID(pstGUID, &stGUID_Stream_Type_Video))
        eReturn = E_GUID_STREAM_TYPE_VIDEO;
    else if (matchGUID(pstGUID, &stGUID_Stream_Type_Command))
        eReturn = E_GUID_STREAM_TYPE_COMMAND;
    else if (matchGUID(pstGUID, &stGUID_Stream_Type_Extended))
        eReturn = E_GUID_STREAM_TYPE_EXTENDED;
    else if (matchGUID(pstGUID, &stGUID_Stream_Type_Extended_Audio))
        eReturn = E_GUID_STREAM_TYPE_EXTENDED_AUDIO;

    return eReturn;
}

ASF_GUID_TYPE ASFExtractor::getType(const ST_ASF_GUID *pstGUID)
{
    ASF_GUID_TYPE eReturn;

    eReturn = getObjectType(pstGUID);
    if (eReturn == E_GUID_UNKNOWN) {
        eReturn = getStreamType(pstGUID);
    }

    return eReturn;
}

void ASFExtractor::getGUID(off64_t offset, ST_ASF_GUID *pstGUID)
{
    uint8_t tmp[16];
    ssize_t n = mDataSource->readAt(offset, tmp, 16);

    if (n < 16) {
        return;
    }
    pstGUID->u32V1 = U32LE_AT(&tmp[0]); //msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(4);
    pstGUID->u8V2[0] = tmp[4];//(uint8_t)msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(1);
    pstGUID->u8V2[1] = tmp[5];//(uint8_t)msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(1);
    pstGUID->u8V3[0] = tmp[6];//(uint8_t)msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(1);
    pstGUID->u8V3[1] = tmp[7];//(uint8_t)msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(1);

    uint32_t u32I = 0;
    for (u32I = 0; u32I < 8; u32I++) {
        pstGUID->u8V4[u32I] = tmp[8 + u32I];//(uint8_t)msAPI_VDPlayer_BMReadBytes_AutoLoad(1);
    }

    ASF_DBG(ALOGW("GUID: %x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x%02x%02x\n",
                 pstGUID->u32V1, pstGUID->u8V2[0], pstGUID->u8V2[1], pstGUID->u8V3[0], pstGUID->u8V3[1], pstGUID->u8V4[0],
                 pstGUID->u8V4[1], pstGUID->u8V4[2], pstGUID->u8V4[3], pstGUID->u8V4[4],
                 pstGUID->u8V4[5], pstGUID->u8V4[6], pstGUID->u8V4[7]));
}

void ASFExtractor::parseObject(off64_t offset, ST_ASF_OBJECT *pstObj)
{
    uint8_t tmp[24];
    ssize_t n = mDataSource->readAt(offset, tmp, 24);

    if (n < 24) {
        return;
    }

    getGUID(offset, &pstObj->stGUID);
    pstObj->eType = getType(&pstObj->stGUID);

    pstObj->u64Size = U64LE_AT(&tmp[16]);//msAPI_VDPlayer_DemuxASF_AutoLoadDQWLE();  // invalid if broadcast
    pstObj->u64DataLen = 0;

    if (pstObj->eType == E_GUID_UNKNOWN) {
        ALOGW("Unknown ASF Object: %x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x%02x%02x, %d bytes\n",
             pstObj->stGUID.u32V1, pstObj->stGUID.u8V2[0], pstObj->stGUID.u8V2[1], pstObj->stGUID.u8V3[0], pstObj->stGUID.u8V3[1], pstObj->stGUID.u8V4[0],
             pstObj->stGUID.u8V4[1], pstObj->stGUID.u8V4[2], pstObj->stGUID.u8V4[3], pstObj->stGUID.u8V4[4],
             pstObj->stGUID.u8V4[5], pstObj->stGUID.u8V4[6], pstObj->stGUID.u8V4[7], (uint32_t) pstObj->u64Size);
    }
    ASF_DBG(ALOGW("ASF Object: %x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x%02x%02x, %d bytes\n",
                 pstObj->stGUID.u32V1, pstObj->stGUID.u8V2[0], pstObj->stGUID.u8V2[1], pstObj->stGUID.u8V3[0], pstObj->stGUID.u8V3[1], pstObj->stGUID.u8V4[0],
                 pstObj->stGUID.u8V4[1], pstObj->stGUID.u8V4[2], pstObj->stGUID.u8V4[3], pstObj->stGUID.u8V4[4],
                 pstObj->stGUID.u8V4[5], pstObj->stGUID.u8V4[6], pstObj->stGUID.u8V4[7], (uint32_t) pstObj->u64Size));
}

int32_t ASFExtractor::readWaveFormatEx(off64_t offset, PST_WAVEFORMATEX pstWaveFormatEx, uint32_t u32Length)
{
    off64_t tmpOffset = 0;

    sp<ABuffer> buffer = new ABuffer(u32Length);
    ssize_t n = mDataSource->readAt(offset, buffer->data(), buffer->size());

    const uint8_t *data = buffer->data();


    pstWaveFormatEx->u16FormatTag   = U16LE_AT(&data[tmpOffset]);//(uint16_t)msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(2);
    ASF_DBG(ALOGW("format tag: 0x%x\n", pstWaveFormatEx->u16FormatTag));
    tmpOffset += 2;
    pstWaveFormatEx->u16Channels    = U16LE_AT(&data[tmpOffset]);//(uint16_t)msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(2);
    ASF_DBG(ALOGW("channel: %d\n", pstWaveFormatEx->u16Channels));
    tmpOffset += 2;
    pstWaveFormatEx->u32SamplesPerSec = U32LE_AT(&data[tmpOffset]); //msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(4);
    ASF_DBG(ALOGW("sample per second: %d\n", pstWaveFormatEx->u32SamplesPerSec));
    tmpOffset += 4;
    pstWaveFormatEx->u32AvgBytesPerSec = U32LE_AT(&data[tmpOffset]); //msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(4);
    ASF_DBG(ALOGW("Average bytes per second: %d\n", pstWaveFormatEx->u32AvgBytesPerSec));
    tmpOffset += 4;
    pstWaveFormatEx->u16BlockAlign  = U16LE_AT(&data[tmpOffset]); //(uint16_t)msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(2);
    ASF_DBG(ALOGW("block align: %d\n", pstWaveFormatEx->u16BlockAlign));
    tmpOffset += 2;

    if (u32Length == 14) {
        pstWaveFormatEx->u16BitsPerSample = 8;
    } else {
        pstWaveFormatEx->u16BitsPerSample = U16LE_AT(&data[tmpOffset]);//(uint16_t)msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(2);
        ASF_DBG(ALOGW("bits per sample: %d\n", pstWaveFormatEx->u16BitsPerSample));
        tmpOffset += 2;
        if (pstWaveFormatEx->u16BitsPerSample == 0) {
            pstWaveFormatEx->u16BitsPerSample = 16;
        }
    }

    //eCodecId = msAPI_VDPlayer_Demux_GetWavCodecID(pstWaveFormatEx->u16FormatTag, pstWaveFormatEx->u16BitsPerSample, MFP_WAVEFORMATEX_AUDIO_TAGS);
    pstWaveFormatEx->u32SamplePerBlock = 1; // in case to be used as denominator
    if (u32Length > 16) {
        uint32_t u32ExtraDataSize;

        u32ExtraDataSize = U16LE_AT(&data[tmpOffset]); //msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(2);
        ASF_DBG(ALOGW("extra data size: %d\n", u32ExtraDataSize));
        tmpOffset += 2;
        if (((uint32_t)pstWaveFormatEx->u16FormatTag == FOURCC(0, 0, 'R', 'P')) && (u32ExtraDataSize >= 2)) {
            pstWaveFormatEx->u16FormatTag = data[tmpOffset + u32ExtraDataSize - 1];//msAPI_VDPlayer_BMPeekNthBytes(u32ExtraDataSize - 1);
            pstWaveFormatEx->u16FormatTag <<= 8;
            pstWaveFormatEx->u16FormatTag |= data[tmpOffset + u32ExtraDataSize - 2];//msAPI_VDPlayer_BMPeekNthBytes(u32ExtraDataSize - 2);
            //eCodecId = msAPI_VDPlayer_Demux_GetWavCodecID(pstWaveFormatEx->u16FormatTag, pstWaveFormatEx->u16BitsPerSample, MFP_WAVEFORMATEX_AUDIO_TAGS);
        }

        if (u32ExtraDataSize > 0) {
            if (u32ExtraDataSize > (u32Length - 18)) {
                u32ExtraDataSize = (uint16_t)(u32Length - 18);
            }

            if (u32ExtraDataSize > 1) {
                uint32_t u32Size;
                if (AUDIO_CODEC_WMAV3 == pstWaveFormatEx->u16FormatTag) {
                    uint16_t u16ValidBitsPerSample;

                    u16ValidBitsPerSample = U16LE_AT(&data[tmpOffset]);//(uint16_t)msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(2); // wSamplesPerBlock or wValidBitsPerSample in WAVEFORMATEXENSIBLE
                    ASF_DBG(ALOGW("valid bits per sample: %d\n", u16ValidBitsPerSample));
                    tmpOffset += 2;
                    if (pstWaveFormatEx->u16BitsPerSample != u16ValidBitsPerSample) {
                        ALOGW("Bits Pre Sample mis-match(%d; %d)\n", pstWaveFormatEx->u16BitsPerSample, u16ValidBitsPerSample);
                        pstWaveFormatEx->u16BitsPerSample = u16ValidBitsPerSample;
                    }
                } else {
                    pstWaveFormatEx->u32SamplePerBlock = U16LE_AT(&data[tmpOffset]);//(uint16_t)msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(2); // wSamplesPerBlock or wValidBitsPerSample in WAVEFORMATEXENSIBLE
                    ASF_DBG(ALOGW("sample per block: %d\n", pstWaveFormatEx->u32SamplePerBlock));
                    tmpOffset += 2;
                }

                u32Size = u32ExtraDataSize - 2;
                if (u32Size > 0) {
                    switch (pstWaveFormatEx->u16FormatTag) {
                    case AUDIO_CODEC_WMAV1:
                        if (u32Size >= 2) {
                            pstWaveFormatEx->u16EncodeOpt = U16LE_AT(&data[tmpOffset]);//(uint16_t)msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(2);
                            ASF_DBG(ALOGW("encode opt: %d\n", pstWaveFormatEx->u16EncodeOpt));
                            tmpOffset += 2;
                            u32Size -= 2;
                        } else {
                            //eCodecId = E_VDP_CODEC_ID_NONE;
                        }
                        break;

                    case AUDIO_CODEC_WMAV2:
                        if (u32Size >= 4) {
                            //msAPI_VDPlayer_BMFlushBytes(2);
                            tmpOffset += 2;
                            pstWaveFormatEx->u16EncodeOpt = U16LE_AT(&data[tmpOffset]);//(uint16_t)msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(2);
                            ASF_DBG(ALOGW("encode opt: %d\n", pstWaveFormatEx->u16EncodeOpt));
                            tmpOffset += 2;
                            u32Size -= 4;
                        } else {
                            //eCodecId = E_VDP_CODEC_ID_NONE;
                        }
                        break;

                    case AUDIO_CODEC_WMAV3:
                        if (u32Size >= 16) {
                            pstWaveFormatEx->u32ChannelMask = U32LE_AT(&data[tmpOffset]); //msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(4);
                            ASF_DBG(ALOGW("channel mask: %d\n", pstWaveFormatEx->u32ChannelMask));
                            tmpOffset += 4;
                            //msAPI_VDPlayer_BMFlushBytes(8);
                            tmpOffset += 8;
                            pstWaveFormatEx->u16EncodeOpt = U16LE_AT(&data[tmpOffset]);//(uint16_t)msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(2);
                            ASF_DBG(ALOGW("encode opt: %d\n", pstWaveFormatEx->u16EncodeOpt));
                            tmpOffset += 2;
                            pstWaveFormatEx->u16AdvanceEncodeOpt = U16LE_AT(&data[tmpOffset]);//(uint16_t)msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(2);
                            ASF_DBG(ALOGW("advanced encode opt: %d\n", pstWaveFormatEx->u16AdvanceEncodeOpt));
                            tmpOffset += 2;
                            u32Size -= 16;
                        } else {
                            //eCodecId = E_VDP_CODEC_ID_NONE;
                        }
                        break;

                    default:
                        break;
                    }
                    //msAPI_VDPlayer_BMFlushBytes(u32Size);
                    tmpOffset += u32Size;
                }
            } else {
                //msAPI_VDPlayer_BMFlushBytes(u32ExtraDataSize);
                tmpOffset += u32ExtraDataSize;
            }
        }
    }

    return pstWaveFormatEx->u16FormatTag;
}


status_t ASFExtractor::parseStreamProperties(off64_t offset, uint64_t u64ObjectSize)
{
    off64_t tmpOffset = 0;
    ST_WAVEFORMATEX stWaveFormatExt;
    // ST_AUDIO_INFO stAudioInfo;
    ST_ASF_GUID stGUID;
    ASF_GUID_TYPE eType;
    ST_ASF_BITMAPINFOHEADER stBitmapInfoHeader;
    uint32_t u32Width;
    uint32_t u32Height;
    uint16_t u16Flags;



    if (u64ObjectSize < 78) {
        /* invalid size for object */
        return -1;
    }

    sp<ABuffer> buffer = new ABuffer(u64ObjectSize);
    ssize_t n = mDataSource->readAt(offset, buffer->data(), buffer->size());

    const uint8_t *data = buffer->data();


    u16Flags = U16LE_AT(&data[48]); //msAPI_VDPlayer_DemuxASF_GetWLE(48);
    ASF_DBG(ALOGW("flag: %d\n", u16Flags));
    getGUID(offset, &stGUID);
    tmpOffset += 16;
    eType = getStreamType(&stGUID);

    if (mASFContext.nbStreams >= MAX_STREAM_NUM) {
        eType = E_GUID_UNKNOWN;
    } else {
        _stASF_Gbl._stHeaderExtStore[mASFContext.nbStreams].u8StreamID = u16Flags & 0x7f;
        _stASF_Gbl._stHeaderExtStore[mASFContext.nbStreams].u8Encrypted = u16Flags >> 15;
        mASFContext.nbStreams++;

#if (ENABLE_WMDRMPD == 1)
        _stASF_Gbl.stDRM.u8Encrypted |= u16Flags >> 15;
#else
        if (u16Flags >> 15) {
            // tmp: don't parse WMDRMPD
            mASFContext.nbStreams = 0;
            return -1;
        }
#endif
    }

    //msAPI_VDPlayer_BMFlushBytes(38);    // Skip to Type-Specific Data
    tmpOffset += 38; // Skip to Type-Specific Data

    u64ObjectSize -= 78;

    switch (eType) {
    case E_GUID_STREAM_TYPE_AUDIO:
    case E_GUID_STREAM_TYPE_EXTENDED_AUDIO:
        //g_pstDemuxer->pstBitsContent->u8AudioTrackIDMap[g_pstDemuxer->pstBitsContent->u8Nb_AudioTracks] = g_pstDemuxer->pstBitsContent->u8Nb_Streams - 1;
        mASFContext.AudioTracks[mASFContext.nbAudioTracks].u8StreamID = _stASF_Gbl._stHeaderExtStore[mASFContext.nbStreams - 1].u8StreamID;
        if (u64ObjectSize < 14) {
            //mASFContext.AudioTracks[mASFContext.nbAudioTracks]
            //g_pstDemuxer->pstBitsContent->enAudioCodecID[g_pstDemuxer->pstBitsContent->u8Nb_AudioTracks] = E_VDP_CODEC_ID_NONE;
            //return E_VDP_DEMUX_RET_SUCCESS;
            return OK;
        }

        memset(&stWaveFormatExt, 0, sizeof(ST_WAVEFORMATEX));
        //g_pstDemuxer->pstBitsContent->enAudioCodecID[g_pstDemuxer->pstBitsContent->u8Nb_AudioTracks] = readWaveFormatEx(offset + tmpOffset, &stWaveFormatExt, (uint32_t)u64ObjectSize);
        mASFContext.AudioTracks[mASFContext.nbAudioTracks].codecID = readWaveFormatEx(offset + tmpOffset, &stWaveFormatExt, (uint32_t)u64ObjectSize);
        mASFContext.AudioTracks[mASFContext.nbAudioTracks].bitrate = stWaveFormatExt.u32AvgBytesPerSec*8;
        mASFContext.AudioTracks[mASFContext.nbAudioTracks].blockAlign = stWaveFormatExt.u16BlockAlign;
        mASFContext.AudioTracks[mASFContext.nbAudioTracks].u16EncodeOpt = stWaveFormatExt.u16EncodeOpt;
        mASFContext.AudioTracks[mASFContext.nbAudioTracks].u16BitsPerSample = stWaveFormatExt.u16BitsPerSample;
        mASFContext.AudioTracks[mASFContext.nbAudioTracks].u32ChannelMask = stWaveFormatExt.u32ChannelMask;
        mASFContext.AudioTracks[mASFContext.nbAudioTracks].samplerate = stWaveFormatExt.u32SamplesPerSec;
        mASFContext.AudioTracks[mASFContext.nbAudioTracks].channel = stWaveFormatExt.u16Channels;

#if 0
        stAudioInfo.u32BitRate = stWaveFormatExt.u32AvgBytesPerSec * 8;
        stAudioInfo.u32BitsPerSample = stWaveFormatExt.u16BitsPerSample;
        stAudioInfo.u32BlockAlign = stWaveFormatExt.u16BlockAlign;
        stAudioInfo.u32Channel = stWaveFormatExt.u16Channels;
        stAudioInfo.u32Length = stWaveFormatExt.u16CbSize; // useless
        stAudioInfo.u32ObjectTypeID = 2; // for AAC
        stAudioInfo.u32Rate = stWaveFormatExt.u32SamplesPerSec;
        stAudioInfo.u32SampleRate = stWaveFormatExt.u32SamplesPerSec;
        stAudioInfo.u32SampleSize = stWaveFormatExt.u16BitsPerSample;
        stAudioInfo.u32SamplesPerBlock = stWaveFormatExt.u16BlockAlign * 8 / stWaveFormatExt.u16BitsPerSample;
        stAudioInfo.u32Scale = 1;
        stAudioInfo.u16EncodeOpt = stWaveFormatExt.u16EncodeOpt;
        stAudioInfo.u16AdvanceEncodeOpt = stWaveFormatExt.u16AdvanceEncodeOpt;
        stAudioInfo.u32ChannelMask = stWaveFormatExt.u32ChannelMask;
        msAPI_VDPlayer_Demuxer_ExtractAudioParam(g_pstDemuxer->pstBitsContent->u8Nb_AudioTracks,g_pstDemuxer->pstBitsContent->enAudioCodecID[g_pstDemuxer->pstBitsContent->u8Nb_AudioTracks],&stAudioInfo);
#endif
        mASFContext.nbAudioTracks++;
        break;

    case E_GUID_STREAM_TYPE_VIDEO:
        //g_pstDemuxer->pstBitsContent->s8Video_StreamId = g_pstDemuxer->pstBitsContent->u8Nb_Streams - 1;
        //g_pstDemuxer->pstBitsContent->u8VideoTrackIDMap[g_pstDemuxer->pstBitsContent->u8Nb_VideoTracks] = g_pstDemuxer->pstBitsContent->s8Video_StreamId;
        //g_pstDemuxer->pstBitsContent->u8Nb_VideoTracks++;
        mASFContext.VideoTracks[0].streamID = mASFContext.nbStreams - 1;
        mASFContext.nbVideoTracks++;

        u32Width = U32LE_AT(&data[tmpOffset]);//msAPI_VDPlayer_DemuxASF_GetDWLE(0);
        ASF_DBG(ALOGW("width: %d\n", u32Width));
        u32Height = U32LE_AT(&data[tmpOffset + 4]);//msAPI_VDPlayer_DemuxASF_GetDWLE(4);
        ASF_DBG(ALOGW("height: %d\n", u32Height));
        // u32Flags = msAPI_VDPlayer_BMPeekNthBytes(8);
        // u32DataSize = msAPI_VDPlayer_DemuxASF_GetWLE(9);
        stBitmapInfoHeader.u32BiSize = U32LE_AT(&data[tmpOffset + 11]); //msAPI_VDPlayer_DemuxASF_GetDWLE(11);
        ASF_DBG(ALOGW("BiSize: %d\n", stBitmapInfoHeader.u32BiSize));
        stBitmapInfoHeader.u32BiWidth = U32LE_AT(&data[tmpOffset + 15]); //msAPI_VDPlayer_DemuxASF_GetDWLE(15);
        ASF_DBG(ALOGW("BiWidth: %d\n", stBitmapInfoHeader.u32BiWidth));
        stBitmapInfoHeader.u32BiHeight = U32LE_AT(&data[tmpOffset + 19]); //msAPI_VDPlayer_DemuxASF_GetDWLE(19);
        ASF_DBG(ALOGW("BiHeight: %d\n", stBitmapInfoHeader.u32BiHeight));
        stBitmapInfoHeader.u32BiPlanes = U16LE_AT(&data[tmpOffset + 23]); //msAPI_VDPlayer_DemuxASF_GetWLE(23);
        ASF_DBG(ALOGW("BiPlanes: %d\n", stBitmapInfoHeader.u32BiPlanes));
        stBitmapInfoHeader.u32BiBitCount = U16LE_AT(&data[tmpOffset + 25]);//msAPI_VDPlayer_DemuxASF_GetWLE(25);
        ASF_DBG(ALOGW("BiBitCount: %d\n", stBitmapInfoHeader.u32BiBitCount));
        stBitmapInfoHeader.u32BiCompression = U32LE_AT(&data[tmpOffset + 27]);//msAPI_VDPlayer_DemuxASF_GetDWLE(27); // four-cc
        ASF_DBG(ALOGW("BiCompression: 0x%x\n", stBitmapInfoHeader.u32BiCompression));
#if ENABLE_PLAYREADY
        if (stBitmapInfoHeader.u32BiCompression == FOURCC('P','R','D','Y')) {
            // Update correct codec id from extra data
            stBitmapInfoHeader.u32BiCompression = U32LE_AT(&data[tmpOffset + 7 + stBitmapInfoHeader.u32BiSize]);//msAPI_VDPlayer_DemuxASF_GetDWLE(7+stBitmapInfoHeader.u32BiSize);
        }
#endif
        //g_pstDemuxer->pstBitsContent->enVideoCodecID[g_pstDemuxer->pstBitsContent->u8Video_Track_Id]  = msAPI_VDPlayer_Demuxer_CodecTag2ID(MFP_ASF_VIDEO_TAGS, stBitmapInfoHeader.u32BiCompression);
        mASFContext.VideoTracks[mASFContext.nbVideoTracks - 1].codecID = stBitmapInfoHeader.u32BiCompression;
        stBitmapInfoHeader.u32BiSizeImage = U32LE_AT(&data[tmpOffset + 31]);//msAPI_VDPlayer_DemuxASF_GetDWLE(31);
        stBitmapInfoHeader.u32BiXPelPerMeter = U32LE_AT(&data[tmpOffset + 35]);//msAPI_VDPlayer_DemuxASF_GetDWLE(35);
        stBitmapInfoHeader.u32BiYPelPerMeter = U32LE_AT(&data[tmpOffset + 39]);//msAPI_VDPlayer_DemuxASF_GetDWLE(39);
        stBitmapInfoHeader.u32BiClrUsed = U32LE_AT(&data[tmpOffset + 43]);//msAPI_VDPlayer_DemuxASF_GetDWLE(43);
        stBitmapInfoHeader.u32BiClrImportant = U32LE_AT(&data[tmpOffset + 47]);//msAPI_VDPlayer_DemuxASF_GetDWLE(47);

        mASFContext.VideoTracks[mASFContext.nbVideoTracks - 1].width = u32Width;
        mASFContext.VideoTracks[mASFContext.nbVideoTracks - 1].height = u32Height;

#if 0 // to do............
        if (stBitmapInfoHeader.u32BiCompression == FOURCC('W', 'V', 'C', '1')) {
            msAPI_VDPlayer_BMFlushBytes(52);
            msAPI_VDPlayer_Demuxer_WriteSeqHeader(((uint32_t)u64ObjectSize - 52), false);
        } else if (stBitmapInfoHeader.u32BiCompression  == FOURCC('W', 'M', 'V', '3')) {
            uint8_t* pu8WMV_Header = (uint8_t*)g_pstDemuxer->pMemory->u32SeqHeaderBuffer;
            uint32_t u32RCV_Tag;
            uint32_t u32I;

            msAPI_VDPlayer_BMFlushBytes(51);

            u32RCV_Tag = 0xc5000099;
            memcpy(pu8WMV_Header, &u32RCV_Tag, sizeof(uint32_t));
            pu8WMV_Header += sizeof(uint32_t);

            u32RCV_Tag = 4; // header size
            memcpy(pu8WMV_Header, &u32RCV_Tag, sizeof(uint32_t));
            pu8WMV_Header += sizeof(uint32_t);

            u32RCV_Tag = msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(4); // sequence header
            memcpy(pu8WMV_Header, &u32RCV_Tag, sizeof(uint32_t));
            pu8WMV_Header += sizeof(uint32_t);

            u32RCV_Tag = g_pstDemuxer->pstBitsContent->Container.stASF.VideoTracks[0].u32Height; // height
            memcpy(pu8WMV_Header, &u32RCV_Tag, sizeof(uint32_t));
            pu8WMV_Header += sizeof(uint32_t);

            u32RCV_Tag = g_pstDemuxer->pstBitsContent->Container.stASF.VideoTracks[0].u32Width; // width
            memcpy(pu8WMV_Header, &u32RCV_Tag, sizeof(uint32_t));
            pu8WMV_Header += sizeof(uint32_t);

            u32RCV_Tag = 0xC; // block size
            memcpy(pu8WMV_Header, &u32RCV_Tag, sizeof(uint32_t));
            pu8WMV_Header += sizeof(uint32_t);

            u32RCV_Tag = 0x0; // level
            memcpy(pu8WMV_Header, &u32RCV_Tag, sizeof(uint32_t));
            pu8WMV_Header += sizeof(uint32_t);

            u32RCV_Tag = 0x0; // hrd
            memcpy(pu8WMV_Header, &u32RCV_Tag, sizeof(uint32_t));
            pu8WMV_Header += sizeof(uint32_t);

            for (u32I = 0; u32I < _stASF_Gbl.u8StreamExtCount; u32I++) {
                if (_stASF_Gbl.stHeaderFrameRateStore[u32I].u8StreamID == _stASF_Gbl._stHeaderExtStore[g_pstDemuxer->pstBitsContent->s8Video_StreamId].u8StreamID) {
                    g_pstDemuxer->pstBitsContent->Container.stASF.VideoTracks[0].u32FrameRate = (uint32_t)((1000000000 / _stASF_Gbl.stHeaderFrameRateStore[u32I].u64AvgTimePerFrame) * 10);
                    g_pstDemuxer->pstBitsContent->Container.stASF.VideoTracks[0].u32FrameRateBase = 1000;
                    break;
                }
            }

            u32RCV_Tag = g_pstDemuxer->pstBitsContent->Container.stASF.VideoTracks[0].u32FrameRate; // fps
            memcpy(pu8WMV_Header, &u32RCV_Tag, sizeof(uint32_t));
            pu8WMV_Header += sizeof(uint32_t);

            g_pstDemuxer->pstBitsContent->u32SeqHeaderInfoSize = 36;

            msAPI_VDPlayer_BMFlushBytes((uint32_t)u64ObjectSize - 55);

#ifdef  _C_MODULE
            _VideoFile.Write((unsigned char*)g_pstDemuxer->pMemory->u32SeqHeaderBuffer, g_pstDemuxer->pstBitsContent->u32SeqHeaderInfoSize);
            _VideoFile.Flush();
#endif
        }
#if ENABLE_ASF_NON_MS_CODEC
        else if ((stBitmapInfoHeader.u32BiCompression == FOURCC('M', 'P', '4', 'S')) ||
                 (stBitmapInfoHeader.u32BiCompression == FOURCC('X', 'V', 'I', 'D'))) {
            if (u64ObjectSize > 55) {
                msAPI_VDPlayer_BMFlushBytes(55);
                msAPI_VDPlayer_Demuxer_WriteSeqHeader(((uint32_t)u64ObjectSize - 55), false);
            } else {
                msAPI_VDPlayer_BMFlushBytes((uint32_t)u64ObjectSize);
            }
        }
#endif  // #if ENABLE_ASF_NON_MS_CODEC
        else {
            msAPI_VDPlayer_BMFlushBytes((uint32_t)u64ObjectSize);
        }
#endif
        break;

    default:
        //msAPI_VDPlayer_BMFlushBytes((uint32_t)u64ObjectSize);
        break;
    }

    return OK;
}

status_t ASFExtractor::parseFileProperties(off64_t offset, uint64_t u64ObjectSize)
{
    off64_t tmpOffset = 0;
    uint32_t u32MaxPackeSize = 0;
    uint32_t u32Flag = 0;
    if (u64ObjectSize < 104) {
        return -1;
    }

    sp<ABuffer> buffer = new ABuffer(u64ObjectSize);
    ssize_t n = mDataSource->readAt(offset, buffer->data(), buffer->size());

    const uint8_t *data = buffer->data();

    getGUID(offset, &(g_pFile->stFileID));
    tmpOffset += 16;
    g_pFile->u64FileSize = U64LE_AT(&data[tmpOffset]);//msAPI_VDPlayer_DemuxASF_AutoLoadDQWLE(); // byte
    ASF_DBG(ALOGW("filesize: 0x%x.0x%x\n\n", (uint32_t)(g_pFile->u64FileSize >>32), (uint32_t)g_pFile->u64FileSize));
    tmpOffset += 8;
    g_pFile->u64CreationDate = U64LE_AT(&data[tmpOffset]); //msAPI_VDPlayer_DemuxASF_AutoLoadDQWLE();
    ASF_DBG(ALOGW("creation date: 0x%x.0x%x\n\n", (uint32_t)(g_pFile->u64CreationDate >>32), (uint32_t)g_pFile->u64CreationDate));
    tmpOffset += 8;
    g_pFile->u64DataPacketsCount = U64LE_AT(&data[tmpOffset]);//msAPI_VDPlayer_DemuxASF_AutoLoadDQWLE();
    ASF_DBG(ALOGW("packet count: 0x%x.0x%x\n\n", (uint32_t)(g_pFile->u64DataPacketsCount >>32), (uint32_t)g_pFile->u64DataPacketsCount));
    tmpOffset += 8;
    g_pFile->u64PlayDuration = U64LE_AT(&data[tmpOffset]);//msAPI_VDPlayer_DemuxASF_AutoLoadDQWLE(); // 100 ns
    ASF_DBG(ALOGW("play duration: 0x%x.0x%x\n\n", (uint32_t)(g_pFile->u64PlayDuration >>32), (uint32_t)g_pFile->u64PlayDuration));
    tmpOffset += 8;
    g_pFile->u64SendDuration = U64LE_AT(&data[tmpOffset]);//msAPI_VDPlayer_DemuxASF_AutoLoadDQWLE(); // 100 ns
    ASF_DBG(ALOGW("send duration: 0x%x.0x%x\n\n", (uint32_t)(g_pFile->u64SendDuration >>32), (uint32_t)g_pFile->u64SendDuration));
    tmpOffset += 8;
    g_pFile->u64Preroll = U64LE_AT(&data[tmpOffset]);//msAPI_VDPlayer_DemuxASF_AutoLoadDQWLE(); // specifies the amount of time to buffer data before starting to play the file, ms
    ASF_DBG(ALOGW("preroll: 0x%x.0x%x\n\n", (uint32_t)(g_pFile->u64Preroll >>32), (uint32_t)g_pFile->u64Preroll));
    tmpOffset += 8;

    //g_pstDemuxer->pstBitsContent->Container.stASF.u32Duration = (uint32_t)(g_pFile->u64PlayDuration / 10000 - g_pFile->u64Preroll);
    mASFContext.VideoTracks[0].duration = (uint32_t)(g_pFile->u64PlayDuration / 10000 - g_pFile->u64Preroll);
    mASFContext.AudioTracks[0].duration = (uint32_t)(g_pFile->u64PlayDuration / 10000 - g_pFile->u64Preroll);
    u32Flag = U32LE_AT(&data[tmpOffset]); //msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(4);
    tmpOffset += 4;
    g_pFile->bIsBroadcast = (u32Flag & 0x8) >> 3;
    g_pFile->bIsSeekable = (u32Flag & 0x4) >> 2;
    ASF_DBG(ALOGW("broadcast: %d, seekable: %d\n", g_pFile->bIsBroadcast, g_pFile->bIsSeekable));
    g_pFile->u32PacketSize = U32LE_AT(&data[tmpOffset]);//msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(4);
    ASF_DBG(ALOGW("packet size: %d\n", g_pFile->u32PacketSize));
    tmpOffset += 4;
    u32MaxPackeSize = U32LE_AT(&data[tmpOffset]);//msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(4);
    ASF_DBG(ALOGW("max packet size: %d\n", u32MaxPackeSize));
    tmpOffset += 4;
    g_pFile->u64MaxBitrate = U32LE_AT(&data[tmpOffset]);//msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(4);
    ASF_DBG(ALOGW("max bitrate: %d\n", (uint32_t)g_pFile->u64MaxBitrate));
    tmpOffset += 4;

    if (g_pFile->bIsBroadcast) {
        g_pFile->u64FileSize = U64_MAX;
        g_pFile->u64CreationDate = U64_MAX;
        g_pFile->u64DataPacketsCount = U64_MAX;
        g_pFile->u64PlayDuration = U64_MAX;
        g_pFile->u64SendDuration = U64_MAX;
        //g_pstDemuxer->pstBitsContent->bUnknownDuration = true;
    }

    if (g_pFile->u32PacketSize != u32MaxPackeSize && !g_pFile->bIsBroadcast) {
        // in ASF file minimum packet size and maximum packet size have to be same apparently
        return -1;
    } else {
        //MApp_VDPlayer_SetShareMemInfo(E_SHAREMEM_WMA_PKT_SIZE, g_pFile->u32PacketSize);
    }

    return OK;
}

status_t ASFExtractor::parseExtendedStreamProperties(off64_t offset, uint64_t u64ObjectSize)
{
    off64_t tmpOffset = 0;
    ST_ASF_STREAM_EXTEND stStreamExt;
    int32_t s32DataLen;
    uint32_t u32I;

    if ((u64ObjectSize < 88) || (u64ObjectSize > S32_MAX)) {
        return -1;
    }
    sp<ABuffer> buffer = new ABuffer(u64ObjectSize);
    ssize_t n = mDataSource->readAt(offset, buffer->data(), buffer->size());

    const uint8_t *data = buffer->data();

    stStreamExt.u64StartTime = U64LE_AT(&data[tmpOffset]);//msAPI_VDPlayer_DemuxASF_AutoLoadDQWLE();
    ASF_DBG(ALOGW("start time: %d.%d\n", (uint32_t)(stStreamExt.u64StartTime >>32), (uint32_t)stStreamExt.u64StartTime));
    tmpOffset += 8;
    stStreamExt.u64EndTime = U64LE_AT(&data[tmpOffset]);//msAPI_VDPlayer_DemuxASF_AutoLoadDQWLE();
    ASF_DBG(ALOGW("end time: %d.%d\n", (uint32_t)(stStreamExt.u64EndTime >>32), (uint32_t)stStreamExt.u64EndTime));
    tmpOffset += 8;
    stStreamExt.u32DataBitrate = U32LE_AT(&data[tmpOffset]);//msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(4);
    ASF_DBG(ALOGW("data bitrate %d\n", stStreamExt.u32DataBitrate));
    tmpOffset += 4;
    stStreamExt.u32BufferSize = U32LE_AT(&data[tmpOffset]);//msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(4);
    ASF_DBG(ALOGW("buffer size %d\n", stStreamExt.u32BufferSize));
    tmpOffset += 4;
    stStreamExt.u32InitBufFullness = U32LE_AT(&data[tmpOffset]);//msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(4);
    ASF_DBG(ALOGW("init buf fullness %d\n", stStreamExt.u32InitBufFullness));
    tmpOffset += 4;
    stStreamExt.u32DataBitrate2 = U32LE_AT(&data[tmpOffset]);//msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(4);
    ASF_DBG(ALOGW("data bitrate %d\n", stStreamExt.u32DataBitrate2));
    tmpOffset += 4;
    stStreamExt.u32BufferSize2 = U32LE_AT(&data[tmpOffset]);//msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(4);
    ASF_DBG(ALOGW("buffer size2 %d\n", stStreamExt.u32BufferSize2));
    tmpOffset += 4;
    stStreamExt.u32InitBufFullness2 = U32LE_AT(&data[tmpOffset]);//msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(4);
    ASF_DBG(ALOGW("init buf fullness2 %d\n", stStreamExt.u32InitBufFullness2));
    tmpOffset += 4;
    stStreamExt.u32MaxObjSize = U32LE_AT(&data[tmpOffset]);//msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(4);
    ASF_DBG(ALOGW("max obj size %d\n", stStreamExt.u32MaxObjSize));
    tmpOffset += 4;
    stStreamExt.u32Flags = U32LE_AT(&data[tmpOffset]);//msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(4);
    ASF_DBG(ALOGW("flag %d\n", stStreamExt.u32Flags));
    tmpOffset += 4;
    stStreamExt.u16StreamNumber = U16LE_AT(&data[tmpOffset]) & 0x07F;//((uint16_t)msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(2)) & 0x07F;
    ASF_DBG(ALOGW("stream number %d\n", stStreamExt.u16StreamNumber));
    tmpOffset += 2;
    stStreamExt.u16LangIndex = U16LE_AT(&data[tmpOffset]);//(uint16_t)msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(2);
    ASF_DBG(ALOGW("language index %d\n", stStreamExt.u16LangIndex));
    tmpOffset += 2;
    stStreamExt.u64AvgTimePerFrame = U64LE_AT(&data[tmpOffset]);//msAPI_VDPlayer_DemuxASF_AutoLoadDQWLE(); // 100 ns
    ASF_DBG(ALOGW("Avg time per frame %d\n", (uint32_t)stStreamExt.u64AvgTimePerFrame));
    tmpOffset += 8;
    stStreamExt.u16StreamNameCount = U16LE_AT(&data[tmpOffset]);//(uint16_t)msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(2);
    ASF_DBG(ALOGW("stream name count %d\n", stStreamExt.u16StreamNameCount));
    tmpOffset += 2;
    stStreamExt.u16NumPayloadExt = U16LE_AT(&data[tmpOffset]);//(uint16_t)msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(2);
    ASF_DBG(ALOGW("number payload ext %d\n", stStreamExt.u16NumPayloadExt));
    tmpOffset += 2;

    if (_stASF_Gbl.u8StreamExtCount < MAX_STREAM_NUM) {
        _stASF_Gbl.stHeaderFrameRateStore[_stASF_Gbl.u8StreamExtCount].u8StreamID = (uint8_t)stStreamExt.u16StreamNumber;
        _stASF_Gbl.stHeaderFrameRateStore[_stASF_Gbl.u8StreamExtCount].u64AvgTimePerFrame = stStreamExt.u64AvgTimePerFrame;
        _stASF_Gbl.u8StreamExtCount++;
    }

    s32DataLen = (int32_t)(u64ObjectSize - 88);
    for (u32I = 0; u32I < stStreamExt.u16StreamNameCount; u32I++) {
        uint16_t u16StrLen = U16LE_AT(&data[tmpOffset + 2]);//msAPI_VDPlayer_DemuxASF_GetWLE(2);
        ASF_DBG(ALOGW("stream length %d\n", u16StrLen));
        s32DataLen -= 4 + u16StrLen;
        //msAPI_VDPlayer_BMFlushBytes(4 + u16StrLen);
        tmpOffset += (4 + u16StrLen);
    }

    for (u32I = 0; u32I < stStreamExt.u16NumPayloadExt; u32I++) {
        uint32_t u32ExtSysLen = U16LE_AT(&data[tmpOffset + 18]);//msAPI_VDPlayer_DemuxASF_GetDWLE(18);
        ASF_DBG(ALOGW("ext sys length %d\n", u32ExtSysLen));
        s32DataLen -= (22 + u32ExtSysLen);
        //msAPI_VDPlayer_BMFlushBytes(22 + u32ExtSysLen);
        tmpOffset += (22 + u32ExtSysLen);
    }

    if (s32DataLen > 0) {
        ST_ASF_GUID stGUID;
        getGUID(offset + tmpOffset, &stGUID);
        tmpOffset += 16;

        if (getType(&stGUID) != E_GUID_STREAM_PROPERTIES) {
            return -1;
        } else {
            uint64_t u64Len = U64LE_AT(&data[tmpOffset]);//msAPI_VDPlayer_DemuxASF_AutoLoadDQWLE();
            ASF_DBG(ALOGW("stream property length %d\n", (uint32_t)u64Len));

            tmpOffset += 8;
            if (u64Len != (uint64_t)s32DataLen) {
                return -1;
            }

            if (parseStreamProperties(offset + tmpOffset, u64Len) != OK) {
                return -1;
            }
            tmpOffset += (u64Len - 24);
        }
    } else if (s32DataLen < 0) {
        return -1;
    }

    return OK;
}

status_t ASFExtractor::parseMetaData(off64_t offset, uint64_t u64ObjectSize)
{
    off64_t tmpOffset = 0;
    uint16_t u16WMAProNum;
    uint32_t u32I;
    uint32_t u32StreamID = 0;
    uint8_t  u8WMADRCStreamLastID = U8_MAX;

    sp<ABuffer> buffer = new ABuffer(u64ObjectSize);
    ssize_t n = mDataSource->readAt(offset, buffer->data(), buffer->size());

    const uint8_t *data = buffer->data();

    u16WMAProNum = U16LE_AT(&data[tmpOffset]);//(uint16_t)msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(2);
    ASF_DBG(ALOGW("WMA pro number %d\n", u16WMAProNum));
    tmpOffset += 2;

    for (u32I = 0; u32I < MAX_STREAM_NUM; u32I++) {
        _stASF_Gbl._stHeaderExtStore[u32I].u8WMADRCStreamID = U8_MAX;
        _stASF_Gbl._stHeaderExtStore[u32I].bWMADRCExist = false;
        _stASF_Gbl._stHeaderExtStore[u32I].u32WMADRCPeakReference = 0;
        _stASF_Gbl._stHeaderExtStore[u32I].u32WMADRCPeakTarget = 0;
        _stASF_Gbl._stHeaderExtStore[u32I].u32WMADRCAverageReference = 0;
        _stASF_Gbl._stHeaderExtStore[u32I].u32WMADRCAverageTarget = 0;
    }

    for (u32I = 0; u32I < u16WMAProNum; u32I++) {
        //msAPI_VDPlayer_BMFlushBytes(2); // reserved zero
        tmpOffset += 2;
        uint8_t u8WMADRCStreamTempID = (uint8_t)(U16LE_AT(&data[tmpOffset]));//(uint8_t)msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(2);
        ASF_DBG(ALOGW("WMA DRCStreamTempID %d\n", u8WMADRCStreamTempID));
        tmpOffset += 2;
        if (u8WMADRCStreamTempID != u8WMADRCStreamLastID) {
            if (u8WMADRCStreamLastID != U8_MAX) {
                u32StreamID++;
            }
            u8WMADRCStreamLastID = u8WMADRCStreamTempID;
            _stASF_Gbl._stHeaderExtStore[u32StreamID].u8WMADRCStreamID = u8WMADRCStreamTempID;
        }

        uint16_t u16FieldNameLen = U16LE_AT(&data[tmpOffset]);//(uint16_t)msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(2);
        ASF_DBG(ALOGW("field name len %d\n", u16FieldNameLen));
        tmpOffset += 2;
        //msAPI_VDPlayer_BMFlushBytes(2); // field type
        tmpOffset += 2;
        uint16_t u16FieldLen = (uint16_t)(U32LE_AT(&data[tmpOffset]));//(uint16_t)msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(4);
        ASF_DBG(ALOGW("field len %d\n", u16FieldLen));
        tmpOffset += 4;

        if (data[tmpOffset] == 'W') {
            _stASF_Gbl._stHeaderExtStore[u32StreamID].bWMADRCExist = true;

            switch (data[tmpOffset + 36]) {
            case 'e':
                _stASF_Gbl._stHeaderExtStore[u32StreamID].u32WMADRCPeakReference = U32LE_AT(&data[tmpOffset + u16FieldNameLen]);//msAPI_VDPlayer_DemuxASF_GetDWLE(u16FieldNameLen);
                ASF_DBG(ALOGW("WMA DRC Peak Ref %d\n", _stASF_Gbl._stHeaderExtStore[u32StreamID].u32WMADRCPeakReference));
                break;
            case 't':
                _stASF_Gbl._stHeaderExtStore[u32StreamID].u32WMADRCPeakTarget = U32LE_AT(&data[tmpOffset + u16FieldNameLen]);//msAPI_VDPlayer_DemuxASF_GetDWLE(u16FieldNameLen);
                ASF_DBG(ALOGW("WMA DRC Peak target %d\n", _stASF_Gbl._stHeaderExtStore[u32StreamID].u32WMADRCPeakTarget));
                break;
            case 'f':
                _stASF_Gbl._stHeaderExtStore[u32StreamID].u32WMADRCAverageReference = U32LE_AT(&data[tmpOffset + u16FieldNameLen]);//msAPI_VDPlayer_DemuxASF_GetDWLE(u16FieldNameLen);
                ASF_DBG(ALOGW("WMA DRC avg Ref %d\n", _stASF_Gbl._stHeaderExtStore[u32StreamID].u32WMADRCAverageReference));
                break;
            case 'r':
                _stASF_Gbl._stHeaderExtStore[u32StreamID].u32WMADRCAverageTarget = U32LE_AT(&data[tmpOffset + u16FieldNameLen]);//msAPI_VDPlayer_DemuxASF_GetDWLE(u16FieldNameLen);
                ASF_DBG(ALOGW("WMA DRC avg target %d\n", _stASF_Gbl._stHeaderExtStore[u32StreamID].u32WMADRCAverageTarget));
                break;
            }
        }
#if 0
#if ENABLE_3D_DETECT
        else if (data[tmpOffset] == 'N') {
            if (data[tmpOffset + 14] == 'S') {
                //MApp_VDPlayer_SetShareMemInfo(E_SHAREMEM_IS_3D, TRUE); // NVIDIA Stereoscopic Video Format
                //MFP_INF(printf("3D: Nvidia\n"));
            }
        }
#endif  // #if ENABLE_3D_DETECT
#endif
        else {
            _stASF_Gbl._stHeaderExtStore[u32StreamID].bWMADRCExist = false;
        }

        tmpOffset += (u16FieldNameLen + u16FieldLen);
        // msAPI_VDPlayer_BMFlushBytes(u16FieldNameLen + u16FieldLen);
    }

    return OK;
}

status_t ASFExtractor::parseHeaderExtension(off64_t offset, uint64_t u64ObjectSize)
{
    off64_t tmpOffset = 0;
    ST_ASF_HEADEREXT stHeaderExt;


    sp<ABuffer> buffer = new ABuffer(u64ObjectSize);
    ssize_t n = mDataSource->readAt(offset, buffer->data(), buffer->size());

    const uint8_t *data = buffer->data();

    getGUID(offset + tmpOffset, &stHeaderExt.stReserved1);
    tmpOffset += 16;

    stHeaderExt.u16Reserved2 = U16LE_AT(&data[tmpOffset]);//(uint16_t)msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(2);
    ASF_DBG(ALOGW("reserved2 %d\n", stHeaderExt.u16Reserved2));
    tmpOffset += 2;
    stHeaderExt.u64DataLen = U32LE_AT(&data[tmpOffset]);//(uint64_t)msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(4);
    ASF_DBG(ALOGW("data len %d\n", (uint32_t)stHeaderExt.u64DataLen));
    tmpOffset += 4;

    if ((stHeaderExt.u64DataLen + 46) > u64ObjectSize) {
        return -1;
    }

    while (stHeaderExt.u64DataLen > 0) {
        ST_ASF_OBJECT stCurrentExt;
        parseObject(offset + tmpOffset, &stCurrentExt);
        tmpOffset += 24;

        if (stHeaderExt.u64DataLen < stCurrentExt.u64Size) {
            return -1;
        }

        if (stCurrentExt.eType == E_GUID_EXTENDED_STREAM_PROPERTIES) {
            if (parseExtendedStreamProperties(offset + tmpOffset, stCurrentExt.u64Size) != OK) {
                return -1;
            }
            tmpOffset += (stCurrentExt.u64Size - 24);
        } else if (stCurrentExt.eType == E_GUID_METADATA) {
            if (parseMetaData(offset + tmpOffset, stCurrentExt.u64Size) != OK) {
                return -1;
            }
            tmpOffset += (stCurrentExt.u64Size - 24);
        }
#if 0
#if (ENABLE_PARSE_NETFLIX_INDEX == 1)
        else if (stCurrentExt.eType == E_GUID_NETFLIX_INDEX) {
            ST_VDP_DEMUX_KEYINDEX stKeyIndex;
            memset(&stKeyIndex, 0, sizeof(ST_VDP_DEMUX_KEYINDEX));
            g_pstDemuxer->pstBitsContent->u64PKT_StartPosition = g_pHeader->u64Size + 50;
            uint32_t u32I = 0;
            for (u32I = 0; u32I < ((uint32_t)stCurrentExt.u64Size - 24)/ 8; u32I++) {
                stKeyIndex.u32Pts = msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(4);
                stKeyIndex.u64Offset = msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(4) * g_pFile->u32PacketSize + g_pstDemuxer->pstBitsContent->u64PKT_StartPosition;
                stKeyIndex.u32Size = g_pFile->u32PacketSize;
                stKeyIndex.u32Frm_Info_Idx = g_pstDemuxer->u32Frm_Info_Idx;
                if (msAPI_VDPlayer_Demuxer_SaveVideoKeyIdx(&stKeyIndex)
                    == E_VDP_DEMUX_BUD_INDEX_TAB_RET_SUCCESS) {
                    g_pstDemuxer->u32Frm_Info_Idx++;
                }
            }
        }
#endif
#endif
        else {
            //msAPI_VDPlayer_BMFlushBytes((uint32_t)stCurrentExt.u64Size - 24);
            tmpOffset += ((uint32_t)stCurrentExt.u64Size - 24);
        }

        stHeaderExt.u64DataLen -= stCurrentExt.u64Size;
    }

    return OK;
}

status_t ASFExtractor::parseContentDescription(off64_t offset, uint64_t u64ObjectSize)
{
    off64_t tmpOffset = 0;
    uint16_t u16AuthorLength;
    uint16_t u16TitleLength;
    String8 *StrTitle;
    String8 *StrAuthor;

    if (u64ObjectSize < 34) {
        /* invalid size for object */
        return -1;
    }

    sp<ABuffer> buffer = new ABuffer(u64ObjectSize);
    ssize_t n = mDataSource->readAt(offset, buffer->data(), buffer->size());

    const uint8_t *data = buffer->data();
    memcpy(&u16TitleLength, &data[tmpOffset], 2);
    tmpOffset += 2;
    memcpy(&u16AuthorLength, &data[tmpOffset], 2);
    tmpOffset += 2;
    tmpOffset += 6;//skip copyright/descrip/Rating size

    //ALOGD("u16TitleLength = %x\n", u16TitleLength);
    //ALOGD("u16AuthorLength = %x\n", u16AuthorLength);
    const char16_t *TitleString = (const char16_t *) (&data[tmpOffset]);
    tmpOffset += u16TitleLength;
    // If the string starts with an endianness marker, skip it
    u16TitleLength /= 2;
    if ((u16TitleLength > 1) && ((uint64_t)tmpOffset <= u64ObjectSize)) {
        if (*TitleString == 0xfeff) {
            TitleString++;
            u16TitleLength--;
        }

        StrTitle = new String8;
        StrTitle->setTo(TitleString, u16TitleLength);
        mASFContext.AudioTracks[0].StrTitle = StrTitle;
        //ALOGD("StrTitle:%s",StrTitle->string());
    } else {
        mASFContext.AudioTracks[0].StrTitle = NULL;
    }

    const char16_t *AuthorString = NULL;
    if ((uint64_t)tmpOffset <= u64ObjectSize) {
        AuthorString = (const char16_t *) (&data[tmpOffset]);
    }
    // If the string starts with an endianness marker, skip it
    u16AuthorLength /= 2;
    if (AuthorString && (u16AuthorLength > 1)) {
        if (*AuthorString == 0xfeff) {
            AuthorString++;
            u16AuthorLength--;
        }

        StrAuthor = new String8;
        StrAuthor->setTo(AuthorString, u16AuthorLength);
        mASFContext.AudioTracks[0].StrAuthor = StrAuthor;
        //ALOGD("StrAuthor:%s",StrAuthor->string());
    } else {
        mASFContext.AudioTracks[0].StrAuthor = NULL;
    }
    return OK;
}

status_t ASFExtractor::parseExtendedContentDescription(off64_t offset, uint64_t u64ObjectSize)
{
    off64_t tmpOffset = 0;
    uint16_t u16ContentCount,u16Index;
    uint16_t u16AuthorLength;
    uint16_t u16TitleLength;
    String8 *StrAlbumTitle;
    String8 *StrGenre;
    String8 *StrYear;

    if (u64ObjectSize < 34) {
        /* invalid size for object */
        return -1;
    }

    sp<ABuffer> buffer = new ABuffer(u64ObjectSize);
    ssize_t n = mDataSource->readAt(offset, buffer->data(), buffer->size());

    const uint8_t *data = buffer->data();
    memcpy(&u16ContentCount, &data[tmpOffset], 2);
    tmpOffset += 2;
    mASFContext.AudioTracks[0].StrAlbumTitle = NULL;
    mASFContext.AudioTracks[0].StrYear = NULL;
    mASFContext.AudioTracks[0].StrGenre = NULL;
    for (u16Index = 0; u16Index < u16ContentCount; ++u16Index) {
        uint16_t u16DesNameLen;
        uint16_t u16DataType;
        uint16_t u16DataLen;

        memcpy(&u16DesNameLen, &data[tmpOffset],2);
        memcpy(&u16DataType, &data[tmpOffset+2+u16DesNameLen],2);
        memcpy(&u16DataLen, &data[tmpOffset+4+u16DesNameLen],2);

        //ALOGD("u16DesNameLen = %d, u16DataType=%d, u16DataLen=%d\n", u16DesNameLen, u16DataType, u16DataLen);

        if ( (u16DesNameLen == 16) && (u16DataType == 0) && (u16DataLen > 1)) {// unicode
            //"WM/Year"
            if ((data[tmpOffset+2]==0x57)&&(data[tmpOffset+3]==0x00)&&(data[tmpOffset+4]==0x4D)&&(data[tmpOffset+5]==0x00)&&
                (data[tmpOffset+6]==0x2F)&&(data[tmpOffset+7]==0x00)&&(data[tmpOffset+8]==0x59)&&(data[tmpOffset+9]==0x00)&&
                (data[tmpOffset+10]==0x65)&&(data[tmpOffset+11]==0x00)&&(data[tmpOffset+12]==0x61)&&(data[tmpOffset+13]==0x00)&&
                (data[tmpOffset+14]==0x72)&&(data[tmpOffset+15]==0x00)&&(data[tmpOffset+16]==0x00)&&(data[tmpOffset+17]==0x00)
               ) {
                const char16_t *YearString = (const char16_t *) (&data[tmpOffset+u16DesNameLen+6]);

                StrYear = new String8;
                StrYear->setTo(YearString, u16DataLen/2);
                mASFContext.AudioTracks[0].StrYear = StrYear;
                //ALOGD("StrYear:%s",StrYear->string());
            }
        } else if ( (u16DesNameLen == 28) && (u16DataType == 0) && (u16DataLen > 1) ) {// unicode
            //"WM/AlbumTitle"
            if ((data[tmpOffset+2]=='W')&&(data[tmpOffset+3]==0x00)&&(data[tmpOffset+4]=='M')&&(data[tmpOffset+5]==0x00)&&
                (data[tmpOffset+6]=='/')&&(data[tmpOffset+7]==0x00)&&(data[tmpOffset+8]=='A')&&(data[tmpOffset+9]==0x00)&&
                (data[tmpOffset+10]=='l')&&(data[tmpOffset+11]==0x00)&&(data[tmpOffset+12]=='b')&&(data[tmpOffset+13]==0x00)&&
                (data[tmpOffset+14]=='u')&&(data[tmpOffset+15]==0x00)&&(data[tmpOffset+16]=='m')&&(data[tmpOffset+17]==0x00)&&
                (data[tmpOffset+18]=='T')&&(data[tmpOffset+19]==0x00)&&(data[tmpOffset+20]=='i')&&(data[tmpOffset+21]==0x00)&&
                (data[tmpOffset+22]=='t')&&(data[tmpOffset+23]==0x00)&&(data[tmpOffset+24]=='l')&&(data[tmpOffset+25]==0x00)&&
                (data[tmpOffset+26]=='e')&&(data[tmpOffset+27]==0x00)&&(data[tmpOffset+28]==0x00)&&(data[tmpOffset+29]==0x00)
               ) {
                const char16_t *AlumTitleString = (const char16_t *) (&data[tmpOffset+u16DesNameLen+6]);

                StrAlbumTitle = new String8;
                StrAlbumTitle->setTo(AlumTitleString, u16DataLen/2);
                mASFContext.AudioTracks[0].StrAlbumTitle = StrAlbumTitle;
                //ALOGD("StrAlbumTitle:%s",StrAlbumTitle->string());
            }
        } else if ( (u16DesNameLen == 18) && (u16DataType == 0) && (u16DataLen > 1) ) {// unicode
            //"WM/Genre"
            if ((data[tmpOffset+2]=='W')&&(data[tmpOffset+3]==0x00)&&(data[tmpOffset+4]=='M')&&(data[tmpOffset+5]==0x00)&&
                (data[tmpOffset+6]=='/')&&(data[tmpOffset+7]==0x00)&&(data[tmpOffset+8]=='G')&&(data[tmpOffset+9]==0x00)&&
                (data[tmpOffset+10]=='e')&&(data[tmpOffset+11]==0x00)&&(data[tmpOffset+12]=='n')&&(data[tmpOffset+13]==0x00)&&
                (data[tmpOffset+14]=='r')&&(data[tmpOffset+15]==0x00)&&(data[tmpOffset+16]=='e')&&(data[tmpOffset+17]==0x00)&&
                (data[tmpOffset+18]==0x00)&&(data[tmpOffset+19]==0x00)
               ) {
                const char16_t *GenreString = (const char16_t *) (&data[tmpOffset+u16DesNameLen+6]);

                StrGenre = new String8;
                StrGenre->setTo(GenreString, u16DataLen/2);
                mASFContext.AudioTracks[0].StrGenre = StrGenre;
                //ALOGD("StrGenre:%s",StrGenre->string());
            }
        }

        // get to next description
        tmpOffset += 6+u16DesNameLen+u16DataLen;
    }

    return OK;
}

#if 0
    #if ENABLE_3D_DETECT
EN_VDP_DEMUX_RET ASFExtractor::parseExtendedContentDescription(uint64_t u64ObjectSize)
{
    uint32_t u32Count = msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(2);
    uint32_t u32I;

    for (u32I = 0; u32I < u32Count; u32I++) {
        uint32_t u32NameLen;
        uint32_t u32ValueType;
        uint32_t u32ValueLen;

        u32NameLen = msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(2);
        msAPI_VDPlayer_BMFlushBytes(u32NameLen);
        u32ValueType = msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(2);
        u32ValueLen = msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(2);
        if ((u32NameLen == 26) && (u32ValueType == 2) && (u32ValueLen == 4)) {
            if (msAPI_VDPlayer_DemuxASF_GetDWLE(0) == 1) {
                MApp_VDPlayer_SetShareMemInfo(E_SHAREMEM_IS_3D, TRUE);
                MFP_INF(printf("3D: 3dtv\n"));
            }
        } else if ((u32NameLen == 38) && (u32ValueType == 0)) {
            if (u32ValueLen == 26) {
                if ((msAPI_VDPlayer_BMPeekNthBytes(20) == 'R') && (msAPI_VDPlayer_BMPeekNthBytes(22) == 'F')) {
                    MApp_VDPlayer_SetShareMemInfo(E_SHAREMEM_3D_LAYOUT, E_VDP_SIDE_BY_SIDE_RF);
                    MFP_INF(printf("E_VDP_SIDE_BY_SIDE_RF\n"));
                } else if ((msAPI_VDPlayer_BMPeekNthBytes(20) == 'L') && (msAPI_VDPlayer_BMPeekNthBytes(22) == 'F')) {
                    MApp_VDPlayer_SetShareMemInfo(E_SHAREMEM_3D_LAYOUT, E_VDP_SIDE_BY_SIDE_LF);
                    MFP_INF(printf("E_VDP_SIDE_BY_SIDE_LF\n"));
                }
            } else if (u32ValueLen == 24) {
                if ((msAPI_VDPlayer_BMPeekNthBytes(18) == 'R') && (msAPI_VDPlayer_BMPeekNthBytes(20) == 'T')) {
                    MApp_VDPlayer_SetShareMemInfo(E_SHAREMEM_3D_LAYOUT, E_VDP_TOP_BOTTOM_RT);
                    MFP_INF(printf("E_VDP_TOP_BOTTOM_RT\n"));
                } else if ((msAPI_VDPlayer_BMPeekNthBytes(18) == 'L') && (msAPI_VDPlayer_BMPeekNthBytes(20) == 'T')) {
                    MApp_VDPlayer_SetShareMemInfo(E_SHAREMEM_3D_LAYOUT, E_VDP_TOP_BOTTOM_LT);
                    MFP_INF(printf("E_VDP_TOP_BOTTOM_LT\n"));
                } else if ((msAPI_VDPlayer_BMPeekNthBytes(0) == 'M') && (msAPI_VDPlayer_BMPeekNthBytes(2) == 'u')) {
                    MApp_VDPlayer_SetShareMemInfo(E_SHAREMEM_3D_LAYOUT, E_VDP_MULTI_STREAM);
                    MFP_INF(printf("E_VDP_MULTI_STREAM\n"));
                }
            } else if (u32ValueLen == 22) {
                if ((msAPI_VDPlayer_BMPeekNthBytes(0) == 'D') && (msAPI_VDPlayer_BMPeekNthBytes(2) == 'u')) {
                    MApp_VDPlayer_SetShareMemInfo(E_SHAREMEM_3D_LAYOUT, E_VDP_DUAL_STREAM);
                    MFP_INF(printf("E_VDP_DUAL_STREAM\n"));
                }
            }
        }
        msAPI_VDPlayer_BMFlushBytes(u32ValueLen);
    }

    return E_VDP_DEMUX_RET_SUCCESS;
}
    #endif // #if ENABLE_3D_DETECT
#endif

status_t ASFExtractor::parseHeaders() {
    off64_t tmpOffset = 0;
    sp<ABuffer> buffer = new ABuffer(30);
    ssize_t n = mDataSource->readAt(0, buffer->data(), buffer->size());

    const uint8_t *data = buffer->data();

    uint32_t u32SubObjCount;
    uint32_t u32I;
#if (ENABLE_WMDRMPD == 1)
    // _stASF_Gbl.stDRM.pu8DRMAddrStart = (uint8_t*)g_pstDemuxer->stDataBuffer.u32DrmPlaybackContextAddr;
    _stASF_Gbl.stDRM.pu8DRMAddr = _stASF_Gbl.stDRM.pu8DRMAddrStart + 2 * sizeof(uint32_t);
#endif

    /* read the object and check its size value */
    parseObject(tmpOffset, (ST_ASF_OBJECT*)g_pHeader);
    tmpOffset += 24;
    if (g_pHeader->u64Size < 30) {
        /* invalid size for header object */
        return -1;
    }


    /* read header object specific compulsory fields */
    g_pHeader->u16SubObjects = (uint16_t)(U32LE_AT(&data[tmpOffset]));//(uint16_t)msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(4);
    tmpOffset += 4;
    g_pHeader->u8Reserved1 = data[tmpOffset];//(uint8_t)msAPI_VDPlayer_BMReadBytes_AutoLoad(1);
    tmpOffset += 1;
    g_pHeader->u8Reserved2 = data[tmpOffset];//(uint8_t)msAPI_VDPlayer_BMReadBytes_AutoLoad(1);
    tmpOffset += 1;
    for (u32SubObjCount = 0; u32SubObjCount < g_pHeader->u16SubObjects; u32SubObjCount++) {
        ST_ASF_OBJECT stCurrent;

        parseObject(tmpOffset, &stCurrent);

        if (stCurrent.u64Size < 24) {
            /* invalid size for object */
            return -1;
        } else {
            tmpOffset += 24;
            switch (stCurrent.eType) {
            case E_GUID_STREAM_PROPERTIES:
                if (parseStreamProperties(tmpOffset, stCurrent.u64Size) != OK) {
                    return -1;
                }
                tmpOffset += (stCurrent.u64Size - 24);
                break;

            case E_GUID_FILE_PROPERTIES:
                if (parseFileProperties(tmpOffset, stCurrent.u64Size) != OK) {
                    return -1;
                }
                tmpOffset += (stCurrent.u64Size - 24);
                break;

            case E_GUID_HEADER_EXTENSION:
                if (parseHeaderExtension(tmpOffset, stCurrent.u64Size) != OK) {
                    return -1;
                }
                tmpOffset += (stCurrent.u64Size - 24);
                break;

            case E_GUID_CONTENT_DESCRIPTION:
                if (parseContentDescription(tmpOffset, stCurrent.u64Size) != OK) {
                    return -1;
                }
                tmpOffset += (stCurrent.u64Size - 24);
                break;

            case E_GUID_EXTENDED_CONTENT_DESCRIPTION:
                if (parseExtendedContentDescription(tmpOffset, stCurrent.u64Size) != OK) {
                    return -1;
                }
                tmpOffset += (stCurrent.u64Size - 24);
                break;

#if 0
#if ENABLE_3D_DETECT
            case E_GUID_EXTENDED_CONTENT_DESCRIPTION:
                if (parseExtendedContentDescription(stCurrent.u64Size) != E_VDP_DEMUX_RET_SUCCESS) {
                    return E_VDP_DEMUX_RET_FAILED;
                }
                break;
#endif
#endif
            case E_GUID_EXTENDED_CONTENT_ENCRYPTION:
                {
#if (ENABLE_WMDRMPD == 1)
                    uint32_t u32Len = msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(4);
                    _stASF_Gbl.stDRM.u32Tag = WMDRMPD_SHAREMEM_TAG_PROTECTED_HEADER;
                    memcpy(_stASF_Gbl.stDRM.pu8DRMAddr, &_stASF_Gbl.stDRM.u32Tag, sizeof(uint32_t));
                    _stASF_Gbl.stDRM.pu8DRMAddr += sizeof(uint32_t);
                    memcpy(_stASF_Gbl.stDRM.pu8DRMAddr, &u32Len, sizeof(uint32_t));
                    _stASF_Gbl.stDRM.pu8DRMAddr += sizeof(uint32_t);

                    uint16_t u16CodingType = msAPI_VDPlayer_DemuxASF_GetWLE(0);
                    if (u16CodingType == 0xFEFF) {
                        msAPI_VDPlayer_BMFlushBytes(2); // Unicode
                        u32Len -= 2;
                    }

                    for (u32I = 0; u32I < u32Len; u32I++) {
                        *_stASF_Gbl.stDRM.pu8DRMAddr = (uint8_t)msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(1);
                        _stASF_Gbl.stDRM.pu8DRMAddr++;
                    }

                    uint32_t u32AlignPaddingLen = sizeof(uint32_t) - (u32Len & (sizeof(uint32_t) - 1));
                    if (u32AlignPaddingLen != sizeof(uint32_t)) {
                        for (u32I = 0; u32I < u32AlignPaddingLen; u32I++) {
                            *_stASF_Gbl.stDRM.pu8DRMAddr = 0;
                            _stASF_Gbl.stDRM.pu8DRMAddr++;
                        }
                    }

                    _stASF_Gbl.stDRM.u32TagCount++;
                    _stASF_Gbl.stDRM.u8Encrypted = TRUE;
                    break;
#else
                    return -1;
#endif
                }

            case E_GUID_PROTECTED_SYSTEM_IDENTIFIER:
#if ENABLE_PLAYREADY
                MFP_INF(printf("parse header should get algid=aes-ctr\n"));
                _stASF_Gbl.bIsPlayReady = TRUE;
                msAPI_VDPlayer_BMFlushBytes((uint32_t)stCurrent.u64Size - 24);
                break;
#else
                return -1;
#endif
            default:
                {
                    //msAPI_VDPlayer_BMFlushBytes((uint32_t)stCurrent.u64Size - 24);
                    tmpOffset += (stCurrent.u64Size - 24);
                    break;
                }
            }
        }
    }
#if 0 // to do
    for (u32I = 0; u32I < g_pstDemuxer->pstBitsContent->u8Nb_AudioTracks; u32I++) {
        uint32_t u32J;

        g_pstDemuxer->pstBitsContent->unAudioParam[u32I].stWMA.bWMADRCExist = false;
        g_pstDemuxer->pstBitsContent->unAudioParam[u32I].stWMA.u32WMADRCPeakReference = 0;
        g_pstDemuxer->pstBitsContent->unAudioParam[u32I].stWMA.u32WMADRCPeakTarget = 0;
        g_pstDemuxer->pstBitsContent->unAudioParam[u32I].stWMA.u32WMADRCAverageReference = 0;
        g_pstDemuxer->pstBitsContent->unAudioParam[u32I].stWMA.u32WMADRCAverageTarget = 0;

        for (u32J = 0; u32J < g_pstDemuxer->pstBitsContent->u8Nb_Streams; u32J++) {
            if (g_pstDemuxer->pstBitsContent->u8AudioTrackIDMap[u32I] == _stASF_Gbl._stHeaderExtStore[u32J].u8WMADRCStreamID) {
                if (_stASF_Gbl._stHeaderExtStore[u32J].bWMADRCExist) {
                    g_pstDemuxer->pstBitsContent->unAudioParam[u32I].stWMA.bWMADRCExist = TRUE;
                    g_pstDemuxer->pstBitsContent->unAudioParam[u32I].stWMA.u32WMADRCPeakReference = _stASF_Gbl._stHeaderExtStore[u32J].u32WMADRCPeakReference;
                    g_pstDemuxer->pstBitsContent->unAudioParam[u32I].stWMA.u32WMADRCPeakTarget = _stASF_Gbl._stHeaderExtStore[u32J].u32WMADRCPeakTarget;
                    g_pstDemuxer->pstBitsContent->unAudioParam[u32I].stWMA.u32WMADRCAverageReference = _stASF_Gbl._stHeaderExtStore[u32J].u32WMADRCAverageReference;
                    g_pstDemuxer->pstBitsContent->unAudioParam[u32I].stWMA.u32WMADRCAverageTarget = _stASF_Gbl._stHeaderExtStore[u32J].u32WMADRCAverageTarget;
                }
            }
        }
    }
#endif

#if (ENABLE_WMDRMPD == 1)
    _stASF_Gbl.stDRM.u32Tag = WMDRMPD_SHAREMEM_TAG_PROTECTED;
    memcpy(_stASF_Gbl.stDRM.pu8DRMAddr, &_stASF_Gbl.stDRM.u32Tag, sizeof(uint32_t));
    _stASF_Gbl.stDRM.pu8DRMAddr += sizeof(uint32_t);
    const uint32_t u32EncryptFlagLen = sizeof(bool);
    memcpy(_stASF_Gbl.stDRM.pu8DRMAddr, &u32EncryptFlagLen, sizeof(uint32_t));
    _stASF_Gbl.stDRM.pu8DRMAddr += sizeof(uint32_t);
    if (_stASF_Gbl.stDRM.u8Encrypted) {
        uint32_t i = 0;

        /*
        //char key[8] = {0xA0, 0xF1, 0xD0, 0xB4, 0xE9, 0x6D, 0x98, 0x00};
        char base64_key[13] = {0x6f, 0x50, 0x48, 0x51, 0x74, 0x4f, 0x6c, 0x74, 0x6d, 0x41, 0x3d, 0x3d, 0x00};

        MApp_VDPlayer_SetShareMemInfo((EN_VDP_SHAREMEMORY)(E_SHAREMEM_DRM_BUFF_ADDR), 0x55AA1234);
        MApp_VDPlayer_SetShareMemInfo((EN_VDP_SHAREMEMORY)(E_SHAREMEM_DRM_BUFF_ADDR + 1), 12);
        for (i = 2; i < E_SHAREMEM_DRM_BUFF_ADDR_SIZE; i++)
        {
            MApp_VDPlayer_SetShareMemInfo((EN_VDP_SHAREMEMORY)(E_SHAREMEM_DRM_BUFF_ADDR + i), 0);
        }
        MApp_VDPlayer_SetShareMemInfo((EN_VDP_SHAREMEMORY)(E_SHAREMEM_DRM_BUFF_ADDR + 2), *((int*)(base64_key)));
        MApp_VDPlayer_SetShareMemInfo((EN_VDP_SHAREMEMORY)(E_SHAREMEM_DRM_BUFF_ADDR + 3), *((int*)(base64_key + 4)));
        MApp_VDPlayer_SetShareMemInfo((EN_VDP_SHAREMEMORY)(E_SHAREMEM_DRM_BUFF_ADDR + 4), *((int*)(base64_key + 8)));
        */

        uint8_t u8EncKey[192];
        for (u32I = 0; u32I < E_SHAREMEM_DRM_BUFF_ADDR_SIZE; u32I++) {
            uint32_t u32Key = MApp_VDPlayer_GetShareMemInfo((EN_VDP_SHAREMEMORY)(E_SHAREMEM_DRM_BUFF_ADDR + u32I));
            u8EncKey[u32I * 4 + 0] = (uint8_t)(u32Key);
            u8EncKey[u32I * 4 + 1] = (uint8_t)(u32Key >> 8);
            u8EncKey[u32I * 4 + 2] = (uint8_t)(u32Key >> 16);
            u8EncKey[u32I * 4 + 3] = (uint8_t)(u32Key >> 24);
        }

        if (*((uint32_t*)u8EncKey) == 0x55AA1234) {
            uint8_t u8DecKey[192];
            uint32_t u32KeyLen = *((uint32_t*)(u8EncKey + 4));
            WMDRMPD_SUBSTRING str;
            str.m_ich = 0;
            str.m_cch = u32KeyLen;
            WMDRMPD_B64_DecodeA((char*)(u8EncKey + 8), &str, (int*)&u32KeyLen, (char*)u8DecKey, 0);
            WMDRMPD_CPHR_Init(u32KeyLen, (char*)u8DecKey);

            for (i = 0; i < g_pstDemuxer->pstBitsContent->u8Nb_Streams; i++) {
                if (_stASF_Gbl._stHeaderExtStore[i].u8Encrypted == TRUE) {
                    break;
                }
            }

            if (i == g_pstDemuxer->pstBitsContent->u8Nb_Streams) {
                MFP_WARNING(printf("No any Encrypted Tracks founded. Assume all encrypted tracks\n"));
                for (i = 0; i < g_pstDemuxer->pstBitsContent->u8Nb_Streams; i++) {
                    _stASF_Gbl._stHeaderExtStore[i].u8Encrypted = TRUE;
                }
            }
        }
    }
    memcpy(_stASF_Gbl.stDRM.pu8DRMAddr, &_stASF_Gbl.stDRM.u8Encrypted, sizeof(bool));
    _stASF_Gbl.stDRM.pu8DRMAddr += sizeof(bool);
    _stASF_Gbl.stDRM.u32TagCount++;

    _stASF_Gbl.stDRM.u32Tag = WMDRMPD_SHAREMEM_HEADER;
    memcpy(_stASF_Gbl.stDRM.pu8DRMAddrStart, &_stASF_Gbl.stDRM.u32Tag, sizeof(uint32_t));
    memcpy(_stASF_Gbl.stDRM.pu8DRMAddrStart + sizeof(uint32_t), &_stASF_Gbl.stDRM.u32TagCount, sizeof(uint32_t));
#endif

    parseObject(tmpOffset, (ST_ASF_OBJECT*)g_pData);
    tmpOffset += 24;
    getGUID(tmpOffset, &(g_pData->stFileID));
    tmpOffset += 16;

#if 0
    char tmp[10];
    if (mDataSource->readAt(tmpOffset, tmp, 10) < 10)
        return -1;

    g_pData->u64TotalDataPackets = U64LE_AT(&tmp[0]);//msAPI_VDPlayer_DemuxASF_AutoLoadDQWLE(); // invalid if broadcast
    g_pData->u16Reserved = U16LE_AT(&tmp[8]);//(uint16_t)msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(2);
    memset(&_stPacket, 0, sizeof(ASF_PACKET_TYPE)* CTRL_MAX_HANDLER);

    g_pstDemuxer->pstBitsContent->u64PKT_StartPosition = msAPI_VDPlayer_BMFileTell();
    if (!g_pFile->bIsBroadcast) {
        g_pData->u64PKT_EndPosition = g_pstDemuxer->pstBitsContent->u64PKT_StartPosition + ((ST_ASF_OBJECT*)(g_pData))->u64Size - 50;
    } else {
        g_pData->u64PKT_EndPosition = msAPI_VDPlayer_BMFileSize();
    }
#endif
    for (u32I = 0; u32I < _stASF_Gbl.u8StreamExtCount; u32I++) {
        if ((mASFContext.nbVideoTracks > 0) && (_stASF_Gbl.stHeaderFrameRateStore[u32I].u8StreamID == _stASF_Gbl._stHeaderExtStore[mASFContext.VideoTracks[0].streamID].u8StreamID)) {
            //g_pstDemuxer->pstBitsContent->Container.stASF.VideoTracks[0].u32FrameRate = (uint32_t)((1000000000 / _stASF_Gbl.stHeaderFrameRateStore[u32I].u64AvgTimePerFrame) * 10);
            //g_pstDemuxer->pstBitsContent->Container.stASF.VideoTracks[0].u32FrameRateBase = 1000;
            mASFContext.VideoTracks[0].frameRate = (uint32_t)((1000000000 / _stASF_Gbl.stHeaderFrameRateStore[u32I].u64AvgTimePerFrame) * 10);
            mASFContext.VideoTracks[0].frameRateBase = 1000;
            if (!g_pFile->bIsBroadcast) {
                //g_pstDemuxer->pstBitsContent->Container.stASF.VideoTracks[0].u32Nb_Frames = (uint32_t)((g_pstDemuxer->pstBitsContent->Container.stASF.VideoTracks[0].u32FrameRate * g_pFile->u64PlayDuration) / 10000000);
                mASFContext.VideoTracks[0].nb_Frames = (uint32_t)((mASFContext.VideoTracks[0].frameRate * g_pFile->u64PlayDuration) / 10000000);
            } else {
                //g_pstDemuxer->pstBitsContent->Container.stASF.VideoTracks[0].u32Nb_Frames = U32_MAX;
                mASFContext.VideoTracks[0].nb_Frames = U32_MAX;
            }

            break;
        }
    }

    return OK;
}

ASFExtractor::ASFExtractor(const sp<DataSource> &dataSource)
: mDataSource(dataSource) {

    memset((uint8_t*)&mASFContext, 0, sizeof(ST_CONTAINER_ASF));
    memset(&_stASF_Gbl, 0, sizeof(ST_CONTEXT_ASF));
    memset(&_stFile, 0, sizeof(ST_ASF_FILE));
    memset(&_stHeader, 0, sizeof(ST_ASF_OBJECT_HEADER));
    memset(&_stData, 0, sizeof(ST_ASF_OBJECT_DATA));
    memset(_stPacket, 0, sizeof(ASF_PACKET_TYPE) * CTRL_MAX_HANDLER);
    memset(_stPayload, 0, sizeof(ASF_PAYLOAD_TYPE)* CTRL_MAX_HANDLER);
    memset(_stASFCtrl, 0, sizeof(ASF_CTRL_TYPE) * CTRL_MAX_HANDLER);

    g_pFile = &_stFile;
    g_pHeader = &_stHeader;
    g_pData = &_stData;

    mInitCheck = parseHeaders();

    if (mInitCheck != OK) {
        //mTracks.clear();
    }
    AddTracks();
}

const char* ASFExtractor::videoCodecTag2Mime(const CodecTag *pstTags, uint32_t u32Tag)
{
    while (pstTags->mime != MEDIA_MIMETYPE_VIDEO_UNKNOW) {
        if (   toupper((u32Tag >> 0)&0xFF) == toupper((pstTags->u32Tag >> 0)&0xFF)
               && toupper((u32Tag >> 8)&0xFF) == toupper((pstTags->u32Tag >> 8)&0xFF)
               && toupper((u32Tag >>16)&0xFF) == toupper((pstTags->u32Tag >>16)&0xFF)
               && toupper((u32Tag >>24)&0xFF) == toupper((pstTags->u32Tag >>24)&0xFF)) {
            return pstTags->mime;
        }
        pstTags++;
    }

    return MEDIA_MIMETYPE_VIDEO_UNKNOW;
}

const char* ASFExtractor::audioCodecTag2Mime(const CodecTag *pstTags, uint32_t u32Tag)
{
    while (pstTags->mime != MEDIA_MIMETYPE_AUDIO_UNKNOW) {
        if (   toupper((u32Tag >> 0)&0xFF) == toupper((pstTags->u32Tag >> 0)&0xFF)
               && toupper((u32Tag >> 8)&0xFF) == toupper((pstTags->u32Tag >> 8)&0xFF)
               && toupper((u32Tag >>16)&0xFF) == toupper((pstTags->u32Tag >>16)&0xFF)
               && toupper((u32Tag >>24)&0xFF) == toupper((pstTags->u32Tag >>24)&0xFF)) {
            return pstTags->mime;
        }
        pstTags++;
    }

    return MEDIA_MIMETYPE_AUDIO_UNKNOW;
}

void ASFExtractor::AddTracks() {
    uint8_t i;
    const char* mime;

    // add video track
    for (i = 0; i < mASFContext.nbVideoTracks; i++) {
        ALOGW("add video codec id: 0x%x\n", mASFContext.VideoTracks[i].codecID);
        sp<MetaData> meta = new MetaData;
        mime = videoCodecTag2Mime(MFP_ASF_VIDEO_TAGS, mASFContext.VideoTracks[i].codecID);
        meta->setCString(kKeyMIMEType, mime);
        meta->setInt32(kKeyWidth, mASFContext.VideoTracks[i].width);
        meta->setInt32(kKeyHeight, mASFContext.VideoTracks[i].height);
        meta->setInt32(kKeyFrameRate, mASFContext.VideoTracks[i].frameRate);
        meta->setInt64(kKeyDuration, (uint64_t)1000 * mASFContext.VideoTracks[0].duration);
        mTracks.push();
        Track *track = &mTracks.editItemAt(mTracks.size() - 1);
        track->mKind = Track::VIDEO;
        track->mMeta = meta;
    }


    // add audio track
    for (i = 0; i < mASFContext.nbAudioTracks; i++) {
        ALOGW("add audio codec id: 0x%x\n", mASFContext.AudioTracks[i].codecID);
        sp<MetaData> meta = new MetaData;
        OMX_AUDIO_WMAPROFILETYPE eProfileType;

        mime = audioCodecTag2Mime(MFP_WAVEFORMATEX_AUDIO_TAGS, mASFContext.AudioTracks[i].codecID);

        // tmp solution: audio only case,
        if ((mime == MEDIA_MIMETYPE_AUDIO_MPEG) && (mASFContext.nbVideoTracks == 0)) {
            ALOGW("asf audio only, contains mp3, use mstarplayer");
            mime = MEDIA_MIMETYPE_AUDIO_WMA;
        }

        if (mASFContext.nbVideoTracks == 0)
            mime = MEDIA_MIMETYPE_AUDIO_WMA;

        meta->setCString(kKeyMIMEType, mime);
        meta->setInt32(kKeySampleRate, mASFContext.AudioTracks[i].samplerate);
        meta->setInt32(kKeyChannelCount, mASFContext.AudioTracks[i].channel);
        meta->setInt64(kKeyDuration, (uint64_t)1000 * mASFContext.AudioTracks[0].duration);
        meta->setInt32(kKeyBitRate, mASFContext.AudioTracks[i].bitrate);        
        meta->setInt32(kKeyMsWMABlockAlign, mASFContext.AudioTracks[i].blockAlign);
        meta->setInt32(kKeyMsWMAEncodeOptions, mASFContext.AudioTracks[i].u16EncodeOpt);
        meta->setInt32(kKeyMsBitsPerSample, mASFContext.AudioTracks[i].u16BitsPerSample);
        meta->setInt32(kKeyMsWMAChannelMask, mASFContext.AudioTracks[i].u32ChannelMask);

        switch(mASFContext.AudioTracks[i].codecID)
        {
            case AUDIO_CODEC_WMAV1:
                eProfileType = OMX_AUDIO_WMAProfileL1;
                break;
            case AUDIO_CODEC_WMAV2:
                eProfileType = OMX_AUDIO_WMAProfileL2;
                break;
            case AUDIO_CODEC_WMAV3:
                eProfileType = OMX_AUDIO_WMAProfileL3;
                break;
            default:
                eProfileType = OMX_AUDIO_WMAProfileL2;
                break;
        }
        
        meta->setInt32(kKeyMsWMAProfile, eProfileType);         

        mTracks.push();
        Track *track = &mTracks.editItemAt(mTracks.size() - 1);
        track->mKind = Track::AUDIO;
        track->mMeta = meta;

    }
}

ASFExtractor::~ASFExtractor() {
    if (mASFContext.AudioTracks[0].StrTitle != NULL) {
        delete mASFContext.AudioTracks[0].StrTitle;
        mASFContext.AudioTracks[0].StrTitle = NULL;
    }
    if (mASFContext.AudioTracks[0].StrAuthor != NULL) {
        delete mASFContext.AudioTracks[0].StrAuthor;
        mASFContext.AudioTracks[0].StrAuthor = NULL;
    }
    if (mASFContext.AudioTracks[0].StrYear != NULL) {
        delete mASFContext.AudioTracks[0].StrYear;
        mASFContext.AudioTracks[0].StrYear = NULL;
    }
    if (mASFContext.AudioTracks[0].StrAlbumTitle != NULL) {
        delete mASFContext.AudioTracks[0].StrAlbumTitle;
        mASFContext.AudioTracks[0].StrAlbumTitle = NULL;
    }
    if (mASFContext.AudioTracks[0].StrGenre != NULL) {
        delete mASFContext.AudioTracks[0].StrGenre;
        mASFContext.AudioTracks[0].StrGenre = NULL;
    }
}

size_t ASFExtractor::countTracks() {
    return mASFContext.nbStreams;
}

sp<MediaSource> ASFExtractor::getTrack(size_t index) {
    return index < mASFContext.nbStreams ? new ASFSource(mDataSource, this, index) : NULL;
}

sp<MetaData> ASFExtractor::getTrackMetaData(
                                           size_t index, uint32_t flags) {                                         
    return index < mTracks.size() ? mTracks.editItemAt(index).mMeta : NULL;
}

sp<MetaData> ASFExtractor::getMetaData() {
    sp<MetaData> meta = new MetaData;

    if (mInitCheck == OK) {
        meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_CONTAINER_ASF);
        if (mASFContext.AudioTracks[0].StrTitle != NULL) {
            meta->setCString(kKeyTitle, mASFContext.AudioTracks[0].StrTitle->string());
        }
        if (mASFContext.AudioTracks[0].StrAuthor != NULL) {
            meta->setCString(kKeyAuthor, mASFContext.AudioTracks[0].StrAuthor->string());
            meta->setCString(kKeyArtist, mASFContext.AudioTracks[0].StrAuthor->string());
        }
        if (mASFContext.AudioTracks[0].StrYear != NULL) {
            meta->setCString(kKeyYear, mASFContext.AudioTracks[0].StrYear->string());
        }
        if (mASFContext.AudioTracks[0].StrAlbumTitle != NULL) {
            meta->setCString(kKeyAlbum, mASFContext.AudioTracks[0].StrAlbumTitle->string());
        }
        if (mASFContext.AudioTracks[0].StrGenre != NULL) {
            meta->setCString(kKeyGenre, mASFContext.AudioTracks[0].StrGenre->string());
        }
    }
    return meta;
}

status_t ASFExtractor::readPacket()
{
    ASF_CTRL_TYPE ASFCtrl;
    bool bIsMultipleCompressedPayload = false;
    bool bDisContinuous = false;
#if 0

#if 0
#if ENABLE_AUDIO_HANDLER
    if (ePktType == DEMUX_PACKET_AUDIO) {
        msAPI_VDPlayer_DemuxASF_SwitchASFCtrl(CTRL_AUDIO_HANDLER);
        bDisContinuous = g_pstDemuxer->bAudioDisContinuous;
        g_pstDemuxer->bAudioDisContinuous = false;
    } else
#endif
    {
        bDisContinuous = g_pstDemuxer->bDisContinuous;
        g_pstDemuxer->bDisContinuous = false;
    }

    if (bDisContinuous) {
        uint32_t u32I = 0;

        memset(g_pASFPayload, 0, sizeof(ASF_PAYLOAD_TYPE));
        g_pASFCtrl->bIsMultiplePayload = false;
        g_pASFCtrl->bIsMultipleCompressedPayload = false;
        g_pASFCtrl->bHasKeyVideoFrameStart = false;
        g_pASFCtrl->bHasKeyAudioFrameStart = false;
        g_pASFCtrl->MultipleCompressedPayloadLen = 0;
        g_pASFCtrl->MultipleCompressedPayloadLenParsed = 0;
        g_pASFCtrl->u32PaddingLen = 0;
        g_pASFCtrl->u16PayloadCounter = 0;

        for (u32I = 0; u32I < g_pstDemuxer->pstBitsContent->u8Nb_Streams; u32I++) {
            g_pASFCtrl->u32LastMediaObjectNum[u32I] = U8_MAX;
        }
    }
#endif
    memcpy(&ASFCtrl, g_pASFCtrl, sizeof(ASF_CTRL_TYPE));

    MultipleCompressedPayloadLabel:
    if ((ASFCtrl.bIsMultipleCompressedPayload) &&
        (ASFCtrl.MultipleCompressedPayloadLenParsed < ASFCtrl.MultipleCompressedPayloadLen)) {
        g_pASFPayload->u32DataLen = (uint8_t)msAPI_VDPlayer_BMReadBytes_AutoLoad(1);
        ASFCtrl.MultipleCompressedPayloadLenParsed += (g_pASFPayload->u32DataLen + 1);

        if (ASFCtrl.MultipleCompressedPayloadLenParsed >= ASFCtrl.MultipleCompressedPayloadLen) {
            ASFCtrl.bIsMultipleCompressedPayload = false;

            ASFCtrl.u16PayloadCounter++;
            if (ASFCtrl.u16PayloadCounter == g_pASFPacket->u16PayloadCount) {
                ASFCtrl.u16PayloadCounter = 0;
                ASFCtrl.bIsMultiplePayload = false;
                if (g_pASFPacket->u32PayloadDataLen != 0) {
                    ASFCtrl.u32PaddingLen += g_pASFPacket->u32PayloadDataLen;
                }
            }
        }
        bIsMultipleCompressedPayload = TRUE;
    } else {
        if (!ASFCtrl.bIsMultiplePayload) {
            uint32_t u32ReadLen;
            uint32_t u32Remainer = msAPI_VDPlayer_BMBuffer_Remainder(E_BM_STREAM_TYPE_DEMUX);
            uint8_t u8PacketFlag;
            bool bFound;
            if (u32Remainer < g_pFile->u32PacketSize + ASFCtrl.u32PaddingLen) {
#if ENABLE_AUDIO_HANDLER
                msAPI_VDPlayer_DemuxASF_SwitchASFCtrl(CTRL_AV_HANDLER);
#endif
                return E_VDP_DEMUX_RET_PACKET_NOT_IN_MEM;
            }

            msAPI_VDPlayer_BMFlushBytes(ASFCtrl.u32PaddingLen);
            if (msAPI_VDPlayer_BMFileTell() + g_pFile->u32PacketSize > g_pData->u64PKT_EndPosition) {
#if ENABLE_AUDIO_HANDLER
                msAPI_VDPlayer_DemuxASF_SwitchASFCtrl(CTRL_AV_HANDLER);
#endif
                return E_VDP_DEMUX_RET_NO_MORE_PACKET;
            }

            bFound = false;
            do {
                u8PacketFlag = (uint8_t)msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(1);
                u32ReadLen = 1;
                g_pASFPacket->u8ECLen = 0;
                if (u8PacketFlag & 0x80) {
                    uint8_t u8OpaqueData, u8ECLenType;

                    g_pASFPacket->u8ECLen = u8PacketFlag & 0x0f;
                    u8OpaqueData = (u8PacketFlag >> 4) & 0x01;
                    u8ECLenType = (u8PacketFlag >> 5) & 0x03;

                    if (u8ECLenType != 0x00 ||
                        u8OpaqueData != 0 ||
                        g_pASFPacket->u8ECLen != 0x02) {
                        // incorrect error correction flags
                        MFP_INF(printf("parser: incorrect error correction flags, %d, %d, %d\n", u8PacketFlag, ASFCtrl.u32PaddingLen, ePktType));
#if ENABLE_AUDIO_HANDLER
                        msAPI_VDPlayer_DemuxASF_SwitchASFCtrl(CTRL_AV_HANDLER);
#endif
                        return E_VDP_DEMUX_RET_NO_MORE_PACKET;
                    } else {
                        // ec_data start here to u8ECLen
                        msAPI_VDPlayer_BMFlushBytes(g_pASFPacket->u8ECLen);
                        u32ReadLen += g_pASFPacket->u8ECLen;
                    }

                    u8PacketFlag = (uint8_t)msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(1);
                    u32ReadLen++;
                }

                g_pASFPacket->u8PacketProperty = (uint8_t)msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(1);
                u32ReadLen++;

                g_pASFPacket->u32Length = GET_VALUE((u8PacketFlag >> 5) & 0x03);
                GET_VALUE((u8PacketFlag >> 1) & 0x03); // Sequence Value
                ASFCtrl.u32PaddingLen = GET_VALUE((u8PacketFlag >> 3) & 0x03);
                u32ReadLen += GET_LENGTH((u8PacketFlag >> 5) & 0x03) + GET_LENGTH((u8PacketFlag >> 1) & 0x03) + GET_LENGTH((u8PacketFlag >> 3) & 0x03);
                g_pASFPacket->u32SendTime = msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(4);
                u32ReadLen += 4;
                g_pASFPacket->u16Duration = (uint16_t)msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(2);
                u32ReadLen += 2;

                /* packet length often undefined and have to use the header packet size as the size/s */
                if (!((u8PacketFlag >> 5) & 0x03)) {
                    g_pASFPacket->u32Length = g_pFile->u32PacketSize;
                }

                // if packet length is smaller than packet, need to manually add the additional bytes into padding length
                if (g_pASFPacket->u32Length < g_pFile->u32PacketSize) {
                    ASFCtrl.u32PaddingLen += g_pFile->u32PacketSize - g_pASFPacket->u32Length;
                    //printf("add padding %d, %d, %d\n", g_pFile->u32PacketSize, g_pASFPacket->u32Length, ASFCtrl.u32PaddingLen);
                    g_pASFPacket->u32Length = g_pFile->u32PacketSize;
                }

                if (g_pASFPacket->u32Length != g_pFile->u32PacketSize) {
                    uint32_t u32PacketErrorCount = 0;
                    /* packet with invalid length value */
                    MFP_INF(printf("Packet w/invalid length %d, %d\n", g_pASFPacket->u32Length, ePktType));

                    while (msAPI_VDPlayer_BMFileTell() < g_pData->u64PKT_EndPosition) {
                        uint32_t u32Remainer = msAPI_VDPlayer_BMBuffer_Remainder(E_BM_STREAM_TYPE_DEMUX);
                        if (u32Remainer < 3) {
#if ENABLE_AUDIO_HANDLER
                            msAPI_VDPlayer_DemuxASF_SwitchASFCtrl(CTRL_AV_HANDLER);
#endif
                            return E_VDP_DEMUX_RET_PACKET_NOT_IN_MEM;
                        }

                        if ((msAPI_VDPlayer_BMPeekNthBytes(0) == 0x82) &&
                            (msAPI_VDPlayer_BMPeekNthBytes(1) == 0x0) &&
                            (msAPI_VDPlayer_BMPeekNthBytes(2) == 0x0)) {
                            bFound = TRUE;
                            break;
                        } else {
                            if (!(u32PacketErrorCount % g_pFile->u32PacketSize)) {
                                MFP_INF(printf("!"));
                            }

                            u32PacketErrorCount++;
                            msAPI_VDPlayer_BMFlushBytes(1);
                        }
                    }

#if ENABLE_AUDIO_HANDLER
                    msAPI_VDPlayer_DemuxASF_SwitchASFCtrl(CTRL_AV_HANDLER);
#endif
                    return E_VDP_DEMUX_RET_NO_MORE_PACKET;
                } else {
                    bFound = TRUE;
                }
            }while (bFound == false);

            g_pASFPacket->u32PayloadLenType = 0;
            /* check if we have multiple payloads */
            ASFCtrl.bIsMultiplePayload = u8PacketFlag & 0x01;
            ASFCtrl.u16PayloadCounter = 0;
            if (ASFCtrl.bIsMultiplePayload) {
                uint8_t u8Temp;
                u8Temp = (uint8_t)msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(1);
                u32ReadLen++;
                g_pASFPacket->u16PayloadCount = u8Temp & 0x3f;
                g_pASFPacket->u32PayloadLenType = (u8Temp >> 6) & 0x03;
            } else {
                g_pASFPacket->u16PayloadCount = 1;
                g_pASFPacket->u32PayloadLenType = 0x02; /* not used */
            }

            g_pASFPacket->u32PayloadDataLen = g_pASFPacket->u32Length - u32ReadLen - ASFCtrl.u32PaddingLen;
        }

        if (ASFCtrl.u16PayloadCounter < g_pASFPacket->u16PayloadCount) {
            uint32_t u32I;
            uint32_t u32Skip = 0;
            uint32_t u32PayloadDataLen = 0;
            uint8_t u8Temp = (uint8_t)msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(1);
            u32Skip++;
            g_pASFPayload->u8StreamNumber = u8Temp & 0x7f;
            for (u32I = 0; u32I < g_pstDemuxer->pstBitsContent->u8Nb_Streams; u32I++) {
                if (_stASF_Gbl._stHeaderExtStore[u32I].u8StreamID == g_pASFPayload->u8StreamNumber) {
                    g_pASFPayload->u8StreamNumber = (S8)u32I;
                    break;
                }
            }

            g_pASFPayload->u8KeyFrame = !!(u8Temp & 0x80);
            g_pASFPayload->u32MediaObjectNum = GET_VALUE((g_pASFPacket->u8PacketProperty >> 4) & 0x03);
            g_pASFPayload->u32MediaObjectOffset = GET_VALUE((g_pASFPacket->u8PacketProperty >> 2) & 0x03);
            g_pASFPayload->u32ReplicatedLen = GET_VALUE(g_pASFPacket->u8PacketProperty & 0x03);
            u32Skip += ((GET_LENGTH((g_pASFPacket->u8PacketProperty >> 4) & 0x03)) + GET_LENGTH((g_pASFPacket->u8PacketProperty >> 2) & 0x03) + GET_LENGTH(g_pASFPacket->u8PacketProperty & 0x03));

            if (g_pASFPayload->u32ReplicatedLen >= 8) {
                g_pASFPayload->u32MediaObjectSize = msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(4);
                g_pASFPayload->u32PTS = msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(4);
#if ENABLE_PLAYREADY
                if (_stASF_Gbl.bIsPlayReady && g_pASFPayload->u32ReplicatedLen >= 16) {
                    msAPI_VDPlayer_BMFlushBytes(g_pASFPayload->u32ReplicatedLen - 8 - 8);
                    g_pASFPayload->u64IV = msAPI_VDPlayer_DemuxASF_AutoLoadDQW();

#if PLAYREADY_DEBUG_LOG //debug
                    uint32_t *ptr = &g_pASFPayload->u64IV;
                    MFP_INF(printf("Playready IV = %X %X\n", *ptr, *(ptr+1)));  //
#endif
                } else {
                    msAPI_VDPlayer_BMFlushBytes(g_pASFPayload->u32ReplicatedLen - 8);
                }
#else
                msAPI_VDPlayer_BMFlushBytes(g_pASFPayload->u32ReplicatedLen - 8);
#endif
                u32Skip += g_pASFPayload->u32ReplicatedLen;
            } else if (g_pASFPayload->u32ReplicatedLen == 1) {
                /* in u32Compressed payload object offset is actually pts */
                g_pASFPayload->u32PTS = g_pASFPayload->u32MediaObjectOffset;
                g_pASFPayload->u32MediaObjectOffset = 0;
                g_pASFPayload->u32ReplicatedLen = 0;
                g_pASFPayload->u8PTSDelta = (uint8_t)msAPI_VDPlayer_BMReadBytes_LE_AutoLoad(1);
                u32Skip++;
                ASFCtrl.bIsMultipleCompressedPayload = TRUE;
            } else {
                g_pASFPayload->u32PTS = g_pASFPacket->u32SendTime;
            }

#if (ENABLE_DEBUG_PTS == 0)
            if (g_pASFPayload->u32PTS >= (uint32_t)g_pFile->u64Preroll) {
                g_pASFPayload->u32PTS -= (uint32_t)g_pFile->u64Preroll;
            } else {
                g_pFile->u64Preroll = g_pASFPayload->u32PTS;
                g_pASFPayload->u32PTS = 0;      // TODO: Check!! Why set PTS = 0?
                if (!g_pFile->bIsBroadcast) {
                    g_pstDemuxer->pstBitsContent->Container.stASF.u32Duration = (uint32_t)(g_pFile->u64PlayDuration / 10000 - g_pFile->u64Preroll);
                } else {
                    g_pstDemuxer->pstBitsContent->Container.stASF.u32Duration = U32_MAX;
                }
            }
#endif

            u32PayloadDataLen = g_pASFPacket->u32PayloadDataLen;
            if (ASFCtrl.bIsMultiplePayload) { // multiple payload
                if (GET_LENGTH(g_pASFPacket->u32PayloadLenType) != 2) {
                    MFP_INF(printf("parser: payload type is not equal to 2, %d\n", ePktType));
#if ENABLE_AUDIO_HANDLER
                    msAPI_VDPlayer_DemuxASF_SwitchASFCtrl(CTRL_AV_HANDLER);
#endif
                    return E_VDP_DEMUX_RET_NO_MORE_PACKET;
                }

                g_pASFPayload->u32DataLen = GET_VALUE(g_pASFPacket->u32PayloadLenType);
                u32Skip += (g_pASFPacket->u32PayloadLenType+g_pASFPayload->u32DataLen);
                u32PayloadDataLen -= u32Skip;
            } else {
                g_pASFPayload->u32DataLen = g_pASFPacket->u32PayloadDataLen - u32Skip;
                u32PayloadDataLen = 0;
            }

            g_pASFPacket->u32PayloadDataLen = u32PayloadDataLen;
            if (ASFCtrl.bIsMultipleCompressedPayload) {
                uint32_t u32I = 0;
                uint32_t u32Used = 0;

                // count how many u32Compressed payloads this payload includes
                for (u32I = 0; u32Used < g_pASFPayload->u32DataLen; u32I++) {
                    u32Used += 1 + msAPI_VDPlayer_BMPeekNthBytes(u32Used);
                }

                ASFCtrl.MultipleCompressedPayloadLen = u32Used;
                ASFCtrl.MultipleCompressedPayloadLenParsed = 0;
                goto MultipleCompressedPayloadLabel;
            }

            ASFCtrl.u16PayloadCounter++;
            if (ASFCtrl.u16PayloadCounter == g_pASFPacket->u16PayloadCount) {
                ASFCtrl.u16PayloadCounter = 0;
                ASFCtrl.bIsMultiplePayload = false;
                if (u32PayloadDataLen != 0) {
                    ASFCtrl.u32PaddingLen += u32PayloadDataLen;
                }
            }
        } else {
            // TODO: Add Error Concealment ..
        }
    }

    PKT_INFO stPKT;
    stPKT.u32Size = g_pASFPayload->u32DataLen;
    stPKT.u8Stream_Index = g_pASFPayload->u8StreamNumber;
    g_pstDemuxer->stPkt.u32Flush_Size = g_pASFPayload->u32DataLen;

    EN_BM_RET eRet = msAPI_VDPlayer_BM_MakePKTCont(g_pASFPayload->u32DataLen, &stPKT);
    switch (eRet) {
    case E_BM_RET_OK:
        break;

    case E_BM_RET_FAIL_NO_MORE_PACKET:
#if ENABLE_AUDIO_HANDLER
        msAPI_VDPlayer_DemuxASF_SwitchASFCtrl(CTRL_AV_HANDLER);
#endif
        return E_VDP_DEMUX_RET_NO_MORE_PACKET;
        break;

    case E_BM_RET_FAIL_PACKET_NOT_FOUND:
#if ENABLE_AUDIO_HANDLER
        msAPI_VDPlayer_DemuxASF_SwitchASFCtrl(CTRL_AV_HANDLER);
#endif
        return E_VDP_DEMUX_RET_PACKET_NOT_FOUND;
        break;

    case E_BM_RET_FAIL_SIZE_TOO_LARGE:
#if ENABLE_AUDIO_HANDLER
        msAPI_VDPlayer_DemuxASF_SwitchASFCtrl(CTRL_AV_HANDLER);
#endif
        return E_VDP_DEMUX_RET_PACKET_SIZE_TOO_LARGE;
        break;

    case E_BM_RET_FAIL_NOT_IN_MEM:
#if ENABLE_AUDIO_HANDLER
        msAPI_VDPlayer_DemuxASF_SwitchASFCtrl(CTRL_AV_HANDLER);
#endif
        return E_VDP_DEMUX_RET_PACKET_NOT_IN_MEM;
        break;

    case E_BM_RET_WAITING:
#if ENABLE_AUDIO_HANDLER
        msAPI_VDPlayer_DemuxASF_SwitchASFCtrl(CTRL_AV_HANDLER);
#endif
        return E_VDP_DEMUX_RET_PACKET_WAITING;
        break;

    case E_BM_RET_FAIL:
    default:
#if ENABLE_AUDIO_HANDLER
        msAPI_VDPlayer_DemuxASF_SwitchASFCtrl(CTRL_AV_HANDLER);
#endif
        return E_VDP_DEMUX_RET_FAILED;
        break;
    }

    // Update information to Demux
    g_pstDemuxer->stPkt.u64Abs_Pos        = stPKT.u64Abs_Pos;
    g_pstDemuxer->stPkt.u32St_Addr        = stPKT.u32StAddr;
    g_pstDemuxer->stPkt.u32Valid_Size     = g_pASFPayload->u32DataLen;
    g_pstDemuxer->stPkt.u32Pts            = g_pASFPayload->u32PTS;
    g_pstDemuxer->stPkt.u16Layer          = stPKT.u16Layer;
    g_pstDemuxer->stPkt.s8Stream_Index    = g_pASFPayload->u8StreamNumber;
    g_pstDemuxer->stPkt.u16Duration       = g_pASFPacket->u16Duration;
    g_pstDemuxer->stPkt.u8Flag            = false;

    memcpy(g_pASFCtrl, &ASFCtrl, sizeof(ASF_CTRL_TYPE));

#if PLAYREADY_DEBUG_LOG //debug
    MFP_INF(printf("V:(addr=%d, obj=%d, pts=%d, size=%d)\n", u32u64_0(stPKT.u64Abs_Pos), g_pASFPayload->u32MediaObjectNum, g_pstDemuxer->stPkt.u32Pts, g_pstDemuxer->stPkt.u32Valid_Size));
#endif

#if (ENABLE_PLAYREADY == 1)
    if (_stASF_Gbl.bIsPlayReady || PLAYREADY_DEBUG_LOADING) {
        msAPI_VDPlayer_DemuxASF_DecryptPlayReadyPayload();
    }
#endif

#if (ENABLE_WMDRMPD == 1)
    if (_stASF_Gbl._stHeaderExtStore[g_pstDemuxer->stPkt.s8Stream_Index].u8Encrypted) {
        uint8_t u8Last15[16];
        if (g_pstDemuxer->stPkt.u32Valid_Size > 15) {
            memcpy((void*)u8Last15, (void*)(g_pstDemuxer->stPkt.u32St_Addr + g_pstDemuxer->stPkt.u32Valid_Size - 15), 15);
            WMDRMPD_CPHR_InitDecrypt((char*)u8Last15, g_pstDemuxer->stPkt.u32Valid_Size);
        } else {
            WMDRMPD_CPHR_InitDecrypt((char*)g_pstDemuxer->stPkt.u32St_Addr, g_pstDemuxer->stPkt.u32Valid_Size);
        }
        WMDRMPD_CPHR_Decrypt(g_pstDemuxer->stPkt.u32Valid_Size, (char*)g_pstDemuxer->stPkt.u32St_Addr);
    }
#endif
    if ((g_pASFPayload->u8StreamNumber == g_pstDemuxer->pstBitsContent->s8Video_StreamId) && (ePktType != DEMUX_PACKET_AUDIO)) {
        if (g_pASFPayload->u32MediaObjectNum != g_pASFCtrl->u32LastMediaObjectNum[g_pstDemuxer->pstBitsContent->s8Video_StreamId]) {
            g_pASFCtrl->u32LastMediaObjectNum[g_pstDemuxer->pstBitsContent->s8Video_StreamId] = g_pASFPayload->u32MediaObjectNum;
            msAPI_VDPlayer_DemuxASF_CheckVideoStartCode();
#if (ENABLE_DEBUG_PTS == 1)
            MFP_INF(printf("V:(%d, %d)", g_pASFPayload->u32MediaObjectNum, g_pstDemuxer->stPkt.u32Pts));
#endif
        }
    } else if (g_pASFPayload->u8StreamNumber == g_pstDemuxer->pstBitsContent->s8Audio_StreamId) {
        if (g_pASFPayload->u32MediaObjectNum != g_pASFCtrl->u32LastMediaObjectNum[g_pstDemuxer->pstBitsContent->s8Audio_StreamId]) {
            g_pASFCtrl->u32LastMediaObjectNum[g_pstDemuxer->pstBitsContent->s8Audio_StreamId] = g_pASFPayload->u32MediaObjectNum;
#if (ENABLE_DEBUG_PTS == 1)
            MFP_INF(printf("A:(%d, %d)", g_pASFPayload->u32MediaObjectNum, g_pstDemuxer->stPkt.u32Pts));
#endif
        }
    }

    if ((!g_pASFCtrl->bHasKeyVideoFrameStart) && (g_pASFPayload->u8StreamNumber != g_pstDemuxer->pstBitsContent->s8Audio_StreamId)) {
        g_pstDemuxer->stPkt.s8Stream_Index = INVALID_STREAM_ID;
    } else if ((!g_pASFCtrl->bHasKeyAudioFrameStart) && (g_pASFPayload->u8StreamNumber == g_pstDemuxer->pstBitsContent->s8Audio_StreamId)) {
        if (g_pASFPayload->u32MediaObjectOffset != 0) {
            g_pstDemuxer->stPkt.s8Stream_Index = INVALID_STREAM_ID;
        } else {
            g_pASFCtrl->bHasKeyAudioFrameStart = TRUE;
        }
    }

    if (bIsMultipleCompressedPayload == TRUE) {
        g_pASFPayload->u32MediaObjectNum++;
        g_pASFPayload->u32PTS += g_pASFPayload->u8PTSDelta;
    }

#if ENABLE_AUDIO_HANDLER
    msAPI_VDPlayer_DemuxASF_SwitchASFCtrl(CTRL_AV_HANDLER);
#endif
    return E_VDP_DEMUX_RET_SUCCESS;
#endif
    return OK;
}


bool SniffASF(
             const sp<DataSource> &source, String8 *mimeType, float *confidence,
             sp<AMessage> *) {
    ST_ASF_GUID stGUID, *pstGUID;
    char tmp[16];
    if (source->readAt(0, tmp, 16) < 16) {
        return false;
    }

    pstGUID = &stGUID;

    pstGUID->u32V1 = ((uint32_t)tmp[3] << 24) | ((uint32_t)tmp[2] << 16) | ((uint32_t)tmp[1] << 8) | tmp[0];
    pstGUID->u8V2[0] = tmp[4];
    pstGUID->u8V2[1] = tmp[5];
    pstGUID->u8V3[0] = tmp[6];
    pstGUID->u8V3[1] = tmp[7];

    uint32_t u32I = 0;
    for (u32I = 0; u32I < 8; u32I++) {
        pstGUID->u8V4[u32I] = tmp[8 + u32I];
    }

    if ((pstGUID->u32V1 != stGUID_ASF_Header.u32V1) ||
        (memcmp(pstGUID->u8V2, stGUID_ASF_Header.u8V2, 2)) ||
        (memcmp(pstGUID->u8V3, stGUID_ASF_Header.u8V3, 2)) ||
        (memcmp(pstGUID->u8V4, stGUID_ASF_Header.u8V4, 8))) {
        return false;
    }
    mimeType->setTo(MEDIA_MIMETYPE_CONTAINER_ASF);
    *confidence = 0.2;
    return true;
}

} // namespace android
