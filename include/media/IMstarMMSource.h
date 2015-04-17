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

#ifndef ANDROID_IMstarMMSource_H
#define ANDROID_IMstarMMSource_H

#include <utils/Errors.h>  // for status_t
#include <utils/KeyedVector.h>
#include <utils/RefBase.h>
#include <utils/String8.h>
#include <binder/IInterface.h>
#include <binder/Parcel.h>
#include <binder/MemoryHeapBase.h>

namespace android {

//if you change the sequence ,must same chage mediaplayer.java
typedef enum {
    E_DATASOURCE_PLAYER_UNKNOW = 0,
    E_DATASOURCE_PLAYER_MOVIE,
    E_DATASOURCE_PLAYER_MUSIC,
} EN_MS_DATASOURCE_PLAYER_TYPE;

//if you change the sequence ,must same chage mediaplayer.java
typedef enum {
    E_DATASOURCE_CONTENT_TYPE_MASS_STORAGE = 0,
    E_DATASOURCE_CONTENT_TYPE_NETWORK_STREAM_WITH_SEEK,
    E_DATASOURCE_CONTENT_TYPE_NETWORK_STREAM_WITHOUT_SEEK,
    E_DATASOURCE_CONTENT_TYPE_ES,
} EN_MS_DATASOURCE_CONTENT_TYPE;

//if you change the sequence ,must same chage mediaplayer.java
typedef enum {
    /// Video codec type is unknow.
    E_DATASOURCE_ES_VIDEO_CODEC_UNKNOW = -1,
    /// Video codec type is MPEG 1.
    E_DATASOURCE_ES_VIDEO_CODEC_MPEG1VIDEO,
    /// Video codec type is MPEG 2.
    E_DATASOURCE_ES_VIDEO_CODEC_MPEG2VIDEO,
    /// Video codec type is MPEG 4.
    E_DATASOURCE_ES_VIDEO_CODEC_MPEG4,
    /// Video codec type is H263.
    E_DATASOURCE_ES_VIDEO_CODEC_H263,
    /// Video codec type is DIVX3.
    E_DATASOURCE_ES_VIDEO_CODEC_DIVX3,
    /// Video codec type is DIVX4.
    E_DATASOURCE_ES_VIDEO_CODEC_DIVX4,
    /// Video codec type is DIVX.
    E_DATASOURCE_ES_VIDEO_CODEC_DIVX,
    /// Video codec type is H264.
    E_DATASOURCE_ES_VIDEO_CODEC_H264,
    /// Video codec type is Advance H264.
    E_DATASOURCE_ES_VIDEO_CODEC_AVS,
    /// Video codec type is RM V3.0.
    E_DATASOURCE_ES_VIDEO_CODEC_RV30,
    /// Video codec type is RM V4.0.
    E_DATASOURCE_ES_VIDEO_CODEC_RV40,
    /// Video codec type is motion JPG.
    E_DATASOURCE_ES_VIDEO_CODEC_MJPEG,
    /// Video codec type is VC1.
    E_DATASOURCE_ES_VIDEO_CODEC_VC1,
    /// Video codec type is WMV 3.0.
    E_DATASOURCE_ES_VIDEO_CODEC_WMV3,
    /// Video codec type is FLV.
    E_DATASOURCE_ES_VIDEO_CODEC_FLV,
    /// Video codec type is FOURCC-EX.
    E_DATASOURCE_ES_VIDEO_CODEC_FOURCCEX,
    /// Video codec type is TS.
    E_DATASOURCE_ES_VIDEO_CODEC_TS,
} EN_MS_DATASOURCE_ES_VIDEO_CODEC;

//if you change the sequence ,must same chage mediaplayer.java
typedef enum {
    /// Audio codec type is none.
    E_DATASOURCE_ES_AUDIO_CODEC_UNKNOW = -1,
    /// Audio codec type is WMA.
    E_DATASOURCE_ES_AUDIO_CODEC_WMA,
    /// Audio codec type is DTS.
    E_DATASOURCE_ES_AUDIO_CODEC_DTS,
    /// Audio codec type is MP3.
    E_DATASOURCE_ES_AUDIO_CODEC_MP3,
    /// Audio codec type is MPEG.
    E_DATASOURCE_ES_AUDIO_CODEC_MPEG,
    /// Audio codec type is AC3.
    E_DATASOURCE_ES_AUDIO_CODEC_AC3,
    /// Audio codec type is AC3 plus.
    E_DATASOURCE_ES_AUDIO_CODEC_AC3_PLUS,
    /// Audio codec type is AAC.
    E_DATASOURCE_ES_AUDIO_CODEC_AAC,
    /// Audio codec type is PCM.
    E_DATASOURCE_ES_AUDIO_CODEC_PCM,
    /// Audio codec type is ADPCM
    E_DATASOURCE_ES_AUDIO_CODEC_ADPCM,
    /// Audio codec type is RAAC.
    E_DATASOURCE_ES_AUDIO_CODEC_RAAC,
    /// Audio codec type is COOK.
    E_DATASOURCE_ES_AUDIO_CODEC_COOK,
    /// Audio codec type is FLAC.
    E_DATASOURCE_ES_AUDIO_CODEC_FLAC,
    /// Audio codec type is VORBIS.
    E_DATASOURCE_ES_AUDIO_CODEC_VORBIS,
    /// Audio codec type is AMR NB.
    E_DATASOURCE_ES_AUDIO_CODEC_AMR_NB,
    /// Audio codec type is AMR WB.
    E_DATASOURCE_ES_AUDIO_CODEC_AMR_WB
} EN_MS_DATASOURCE_ES_AUDIO_CODEC;

//if you change the sequence ,must same chage mediaplayer.java
typedef enum {
    /// Media format type unknown
    E_DATASOURCE_MEDIA_FORMAT_TYPE_UNKNOWN = -1,
    /// Media format type avi
    E_DATASOURCE_MEDIA_FORMAT_TYPE_AVI,
    /// Media format type mp4
    E_DATASOURCE_MEDIA_FORMAT_TYPE_MP4,
    /// Media format type mkv
    E_DATASOURCE_MEDIA_FORMAT_TYPE_MKV,
    /// Media format type asf
    E_DATASOURCE_MEDIA_FORMAT_TYPE_ASF,
    /// Media format type rm
    E_DATASOURCE_MEDIA_FORMAT_TYPE_RM,
    /// Media format type ts
    E_DATASOURCE_MEDIA_FORMAT_TYPE_TS,
    /// Media format type mpg
    E_DATASOURCE_MEDIA_FORMAT_TYPE_MPG,
    /// Media format type flv
    E_DATASOURCE_MEDIA_FORMAT_TYPE_FLV,
    /// Media format type esdata
    E_DATASOURCE_MEDIA_FORMAT_TYPE_ESDATA,
    /// Media format type max
    E_DATASOURCE_MEDIA_FORMAT_TYPE_MAX,
} EN_MS_DATASOURCE_MEDIA_FORMAT_TYPE;

//if you change the sequence ,must same chage mediaplayer.java
typedef enum {
    /// Normal APP
    E_DATASOURCE_AP_NORMAL = 0,
    /// Netflix
    E_DATASOURCE_AP_NETFLIX,
    /// DLNA
    E_DATASOURCE_AP_DLNA,
    /// HBBTV
    E_DATASOURCE_AP_HBBTV,
    /// WEBBROWSER
    E_DATASOURCE_AP_WEBBROWSER,
    /// WMDRM10
    E_DATASOURCE_AP_WMDRM10,
    /// Android USB
    E_DATASOURCE_AP_ANDROID_USB,
    /// Android Streaming
    E_DATASOURCE_AP_ANDROID_STREAMING,
    E_DATASOURCE_AP_RVU,
} EN_MS_DATASOURCE_AP_TYPE;

/// Defined INPUT DATA TYPE
typedef enum {
    /// INPUT DATA AUDIO
    E_ES_DATA_TYPE_AUDIO,
    /// INPUT DATA VIDEO
    E_ES_DATA_TYPE_VIDEO,
    /// INPUT DATA VIDEO and AUDIO
    E_ES_DATA_TYPE_VIDEO_AUDIO,
    /// AUDIO CONFIG DATA
    E_ES_DATA_TYPE_AUDIO_CONFIG,
    /// VIDEO CONFIG DATA
    E_ES_DATA_TYPE_VIDEO_CONFIG,
    /// INPUT DATA END
    E_ES_DATA_TYPE_DATA_END,
}EN_ES_DATA_TYPE;

typedef struct {
    EN_MS_DATASOURCE_ES_VIDEO_CODEC codec_type;
    int32_t width;
    int32_t height;
    int32_t config_len;
    uint8_t config[256];
} ST_ES_VIDEO_CONFIG,
*PST_ES_VIDEO_CONFIG;

typedef struct {
    EN_MS_DATASOURCE_ES_AUDIO_CODEC codec_type;
    int32_t sample_rate;
    int32_t channel_num;
    int32_t bits_per_sample;
    int32_t frame_length;
    int32_t config_len;
    uint8_t config[256];
} ST_ES_AUDIO_CONFIG,
*PST_ES_AUDIO_CONFIG;

typedef enum {
    /// MSTPLAYER
    E_MS_PLAYER_TYPE_MSTPLAYER,
    /// MSTARPLAYER
    E_MS_PLAYER_TYPE_MSTARPLAYER,
    /// None
    E_MS_PLAYER_TYPE_NONE,
}EN_MS_PLAYER_TYPE;

/// Define control flag for TEE encryption type
typedef enum
{
    /// Not TEE
    E_MS_TEE_ENCRYPT_CTL_FLAG_NONE     = 0x00000000,
    /// If TEE is, set this flag
    E_MS_TEE_ENCRYPT_CTL_FLAG_ISTEE    = 0x00000001,
    /// encrypt type is VES
    E_MS_TEE_ENCRYPT_CTL_FLAG_VES      = 0x00000002,
    /// encrypt type is AES
    E_MS_TEE_ENCRYPT_CTL_FLAG_AES      = 0x00000004,
    /// encrypt type is VPES
    E_MS_TEE_ENCRYPT_CTL_FLAG_VPES     = 0x00000008,
    /// encrypt type is APES
    E_MS_TEE_ENCRYPT_CTL_FLAG_APES     = 0x00000010,
    /// encrypt type is All
    E_MS_TEE_ENCRYPT_CTL_FLAG_ALL      = 0x00000020,
} E_MS_TEE_ENCRYPT_CTL_FLAG;

typedef struct {
    /// Data start address
    uint32_t u32StartAddr;
    /// Data u32Size u32Size
    uint32_t u32Size;
    /// Data u32TimeStamp
    uint32_t u32TimeStamp;
    /// enDataType type
    EN_ES_DATA_TYPE enDataType;
}ST_ES_DATA_INFO;

typedef enum {
    /// Default APP mode ( AVSync, No min rebuffering mode, queue audio data, No slow sync)
    E_DATASOURCE_APP_MODE_DEFAULT = 0,
    /// Low latency APP mode ( No AVSync, min rebuffering mode, No queue audio data, No slow sync)
    E_DATASOURCE_APP_MODE_LOW_LATENCY,
    /// Fast Start APP mode (AVSync, min rebuffering mode, No queue audio data, slow sync)
    E_DATASOURCE_APP_MODE_FAST_START,
} EN_MS_DATASOURCE_APP_MODE;

typedef struct {
    EN_MS_DATASOURCE_PLAYER_TYPE ePlayMode;
    EN_MS_DATASOURCE_CONTENT_TYPE eContentType;
    EN_MS_DATASOURCE_ES_VIDEO_CODEC eESVideoCodec;
    EN_MS_DATASOURCE_ES_AUDIO_CODEC eESAudioCodec;
    EN_MS_DATASOURCE_MEDIA_FORMAT_TYPE eMediaFormat;
    EN_MS_DATASOURCE_AP_TYPE eAppType;
    EN_MS_DATASOURCE_APP_MODE eAppMode;
    EN_MS_PLAYER_TYPE ePlayerType;
    uint32_t u32TeeEncryptFlag;
    bool bUseRingBuff;
    bool bPC2TVMode;
    bool bAutoPauseResume;
}ST_MS_DATASOURCE_INFO;

class BaseDataSource : public RefBase {
public:
    BaseDataSource(){}
    virtual ~BaseDataSource(){}
    virtual status_t initCheck() {return INVALID_OPERATION;}

    virtual ssize_t readAt(off_t offset, void *data, size_t size) {return 0;}

    virtual ssize_t readAt(void* data, size_t size) {return 0;}

    virtual int seekAt(unsigned long long offset) {return -1;}

    virtual unsigned long long tellAt() {return -1;}

    virtual status_t getSize(unsigned long long *size) {return INVALID_OPERATION;}

    virtual sp<IMemory> getESData() {return NULL;}

};

class IMstarMMSource: public IInterface {
public:
    DECLARE_META_INTERFACE(MstarMMSource);

    virtual sp<IMemoryHeap> getDataSourceBuff() = 0;

    virtual status_t initCheck() {return INVALID_OPERATION;}

    virtual ssize_t readAt(off_t offset, void *data, size_t size) {return 0;}

    virtual ssize_t readAt(void* data, size_t size) {return 0;}

    virtual int seekAt(unsigned long long offset) {return -1;}

    virtual unsigned long long tellAt() {return -1;}

    virtual status_t getSize(unsigned long long *size) {return INVALID_OPERATION;}
    virtual status_t getDataSourceInfo(ST_MS_DATASOURCE_INFO  &dataSourceInfo) {return INVALID_OPERATION;}
    virtual sp<IMemory> getESData() = 0;
};

// ----------------------------------------------------------------------------

class BnMstarMMSource: public BnInterface<IMstarMMSource> {
public:
    virtual status_t    onTransact( uint32_t code,
                                    const Parcel& data,
                                    Parcel* reply,
                                    uint32_t flags = 0);
};

};  // namespace android

#endif // ANDROID_IMEDIAPLAYERSERVICE_H
