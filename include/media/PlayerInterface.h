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

#ifndef ANDROID_PlAYERINTERFACE_H
#define ANDROID_PlAYERINTERFACE_H

#include <binder/IMemory.h>
#include <media/IMediaPlayerClient.h>
#include <media/IMediaPlayer.h>
#include <media/IMediaDeathNotifier.h>
#include <media/IStreamSource.h>

#include <utils/KeyedVector.h>
#include <utils/String8.h>
#include <utils/Vector.h>
#include <media/mediaplayer.h>

class ANativeWindow;

namespace android {

#define MM_INTERFACE_THREAD_SLEEP_MS	30000

typedef enum {
    E_MM_INTERFACE_EXIT_OK = 0, //playback ok, and exit to ap
    E_MM_INTERFACE_EXIT_ERROR, //playback  error, and exit to ap
    E_MM_INTERFACE_SUFFICIENT_DATA, //when playing, data enough to continue play, and codec wil resume
    E_MM_INTERFACE_INSUFFICIENT_DATA, //when playing, run short of data, and codec wil pause

    E_MM_INTERFACE_START_PLAY, //player init ok, and start playing
    E_MM_INTERFACE_RENDERING_START,
    E_MM_INTERFACE_NULL, //playback notify null
} EN_MM_INTERFACE_NOTIFY_TYPE;

typedef enum {
    E_MM_INTERFACE_STREAM_TYPE_UNKNOW = 0,
    E_MM_INTERFACE_STREAM_TYPE_MOVIE,
    E_MM_INTERFACE_STREAM_TYPE_MUISC,
} EN_MM_INTERFACE_STREAM_TYPE;


//if you change the sequence ,must same chage mediaplayer.java
typedef enum {
    E_MM_INTERFACE_SEEKABLE_MODE_WITH_SEEK,
    E_MM_INTERFACE_SEEKABLE_MODE_WITHOUT_SEEK,
    E_MM_INTERFACE_SEEKABLE_MODE_ES,
} EN_MM_INTERFACE_SEEKABLE_MODE;


typedef enum {
    /// Video codec type is unknow.
    E_PLAYER_VIDEO_CODEC_UNKNOW = -1,
    /// Video codec type is MPEG 4.
    E_PLAYER_VIDEO_CODEC_MPEG4,
    /// Video codec type is motion JPG.
    E_PLAYER_VIDEO_CODEC_MJPEG,
    /// Video codec type is H264.
    E_PLAYER_VIDEO_CODEC_H264,
    /// Video codec type is RealVideo.
    E_PLAYER_VIDEO_CODEC_RM,
    E_PLAYER_VIDEO_CODEC_TS,
    /// Video codec type is MPEG1/2.
    E_PLAYER_VIDEO_CODEC_MPG,
    /// Video codec type is VC1.
    E_PLAYER_VIDEO_CODEC_VC1,
    /// Video codec type is Audio Video Standard.
    E_PLAYER_VIDEO_CODEC_AVS,
    /// Video codec type is FLV.
    E_PLAYER_VIDEO_CODEC_FLV,
    /// Video codec type is MVC.
    E_PLAYER_VIDEO_CODEC_MVC,
    /// Video codec type is VP6.
    E_PLAYER_VIDEO_CODEC_VP6,
    /// Video codec type is VP8.
    E_PLAYER_VIDEO_CODEC_VP8,
} EN_MM_INTERFACE_VIDEO;

/// Define audio codec type for the stream except ES
typedef enum {
    /// Audio codec type is none.
    E_PLAYER_AUDIO_CODEC_UNKNOW = -1,
    /// Audio codec type is WMA.
    E_PLAYER_AUDIO_CODEC_WMA,
    /// Audio codec type is DTS.
    E_PLAYER_AUDIO_CODEC_DTS,
    /// Audio codec type is MP3.
    E_PLAYER_AUDIO_CODEC_MP3,
    /// Audio codec type is MPEG.
    E_PLAYER_AUDIO_CODEC_MPEG,
    /// Audio codec type is AC3.
    E_PLAYER_AUDIO_CODEC_AC3,
    /// Audio codec type is AC3 plus.
    E_PLAYER_AUDIO_CODEC_AC3_PLUS,
    /// Audio codec type is AAC.
    E_PLAYER_AUDIO_CODEC_AAC,
    /// Audio codec type is PCM.
    E_PLAYER_AUDIO_CODEC_PCM,
    /// Audio codec type is ADPCM
    E_PLAYER_AUDIO_CODEC_ADPCM,
    /// Audio codec type is RAAC.
    E_PLAYER_AUDIO_CODEC_RAAC,
    /// Audio codec type is COOK.
    E_PLAYER_AUDIO_CODEC_COOK,
    /// Audio codec type is FLAC.
    E_PLAYER_AUDIO_CODEC_FLAC,
    /// Audio codec type is VORBIS.
    E_PLAYER_AUDIO_CODEC_VORBIS,
    /// Audio codec type is AMR NB.
    E_PLAYER_AUDIO_CODEC_AMR_NB,
    /// Audio codec type is AMR WB.
    E_PLAYER_AUDIO_CODEC_AMR_WB,
    /// Audio codec type is HEAAC.
    E_PLAYER_AUDIO_CODEC_HEAAC
} EN_MM_INTERFACE_AUDIO;


typedef enum {
    /// Video codec type is unknow.
    E_MM_INTERFACE_VIDEO_CODEC_UNKNOW = -1,
    /// Video codec type is MPEG 1.
    E_MM_INTERFACE_VIDEO_CODEC_MPEG1VIDEO,
    /// Video codec type is MPEG 2.
    E_MM_INTERFACE_VIDEO_CODEC_MPEG2VIDEO,
    /// Video codec type is MPEG 4.
    E_MM_INTERFACE_VIDEO_CODEC_MPEG4,
    /// Video codec type is H263.
    E_MM_INTERFACE_VIDEO_CODEC_H263,
    /// Video codec type is DIVX3.
    E_MM_INTERFACE_VIDEO_CODEC_DIVX3,
    /// Video codec type is DIVX4.
    E_MM_INTERFACE_VIDEO_CODEC_DIVX4,
    /// Video codec type is DIVX.
    E_MM_INTERFACE_VIDEO_CODEC_DIVX,
    /// Video codec type is H264.
    E_MM_INTERFACE_VIDEO_CODEC_H264,
    /// Video codec type is Advance H264.
    E_MM_INTERFACE_VIDEO_CODEC_AVS,
    /// Video codec type is RM V3.0.
    E_MM_INTERFACE_VIDEO_CODEC_RV30,
    /// Video codec type is RM V4.0.
    E_MM_INTERFACE_VIDEO_CODEC_RV40,
    /// Video codec type is motion JPG.
    E_MM_INTERFACE_VIDEO_CODEC_MJPEG,
    /// Video codec type is VC1.
    E_MM_INTERFACE_VIDEO_CODEC_VC1,
    /// Video codec type is WMV 3.0.
    E_MM_INTERFACE_VIDEO_CODEC_WMV3,
    /// Video codec type is FLV.
    E_MM_INTERFACE_VIDEO_CODEC_FLV,
    /// Video codec type is FOURCC-EX.
    E_MM_INTERFACE_VIDEO_CODEC_FOURCCEX,
    /// Video codec type is TS.
    E_MM_INTERFACE_VIDEO_CODEC_TS,
} EN_MM_INTERFACE_VIDEO_CODEC;

//if you change the sequence ,must same chage mediaplayer.java
typedef enum {
    /// Audio codec type is none.
    E_MM_INTERFACE_AUDIO_CODEC_UNKNOW = -1,
    /// Audio codec type is WMA.
    E_MM_INTERFACE_AUDIO_CODEC_WMA,
    /// Audio codec type is DTS.
    E_MM_INTERFACE_AUDIO_CODEC_DTS,
    /// Audio codec type is MP3.
    E_MM_INTERFACE_AUDIO_CODEC_MP3,
    /// Audio codec type is MPEG.
    E_MM_INTERFACE_AUDIO_CODEC_MPEG,
    /// Audio codec type is AC3.
    E_MM_INTERFACE_AUDIO_CODEC_AC3,
    /// Audio codec type is AC3 plus.
    E_MM_INTERFACE_AUDIO_CODEC_AC3_PLUS,
    /// Audio codec type is AAC.
    E_MM_INTERFACE_AUDIO_CODEC_AAC,
    /// Audio codec type is PCM.
    E_MM_INTERFACE_AUDIO_CODEC_PCM,
    /// Audio codec type is ADPCM
    E_MM_INTERFACE_AUDIO_CODEC_ADPCM,
    /// Audio codec type is RAAC.
    E_MM_INTERFACE_AUDIO_CODEC_RAAC,
    /// Audio codec type is COOK.
    E_MM_INTERFACE_AUDIO_CODEC_COOK,
    /// Audio codec type is FLAC.
    E_MM_INTERFACE_AUDIO_CODEC_FLAC,
    /// Audio codec type is VORBIS.
    E_MM_INTERFACE_AUDIO_CODEC_VORBIS,
    /// Audio codec type is AMR NB.
    E_MM_INTERFACE_AUDIO_CODEC_AMR_NB,
    /// Audio codec type is AMR WB.
    E_MM_INTERFACE_AUDIO_CODEC_AMR_WB
} EN_MM_INTERFACE_AUDIO_CODEC;

//if you change the sequence ,must same chage mediaplayer.java
typedef enum {
    /// Media format type unknown
    E_MM_INTERFACE_MEDIA_FORMAT_TYPE_UNKNOWN = -1,
    /// Media format type avi
    E_MM_INTERFACE_MEDIA_FORMAT_TYPE_AVI,
    /// Media format type mp4
    E_MM_INTERFACE_MEDIA_FORMAT_TYPE_MP4,
    /// Media format type mkv
    E_MM_INTERFACE_MEDIA_FORMAT_TYPE_MKV,
    /// Media format type asf
    E_MM_INTERFACE_MEDIA_FORMAT_TYPE_ASF,
    /// Media format type rm
    E_MM_INTERFACE_MEDIA_FORMAT_TYPE_RM,
    /// Media format type ts
    E_MM_INTERFACE_MEDIA_FORMAT_TYPE_TS,
    /// Media format type mpg
    E_MM_INTERFACE_MEDIA_FORMAT_TYPE_MPG,
    /// Media format type flv
    E_MM_INTERFACE_MEDIA_FORMAT_TYPE_FLV,
    /// Media format type esdata
    E_MM_INTERFACE_MEDIA_FORMAT_TYPE_ESDATA,
    /// Media format type max
    E_MM_INTERFACE_MEDIA_FORMAT_TYPE_MAX,
} EN_MM_INTERFACE_MEDIA_FORMAT_TYPE;

//if you change the sequence ,must same chage mediaplayer.java
typedef enum {
    /// Normal APP
    E_MM_INTERFACE_AP_NORMAL = 0,
    /// Netflix
    E_MM_INTERFACE_AP_NETFLIX,
    /// DLNA
    E_MM_INTERFACE_AP_DLNA,
    /// HBBTV
    E_MM_INTERFACE_AP_HBBTV,
    /// WEBBROWSER
    E_MM_INTERFACE_AP_WEBBROWSER,
    /// WMDRM10
    E_MM_INTERFACE_AP_WMDRM10,
    /// Android USB
    E_MM_INTERFACE_AP_ANDROID_USB,
    /// Android Streaming
    E_MM_INTERFACE_AP_ANDROID_STREAMING,
    E_MM_INTERFACE_AP_RVU,

} EN_MM_INTERFACE_AP_TYPE;

typedef enum {
    E_MM_INTERFACE_STATE_NULL = 0,
    E_MM_INTERFACE_STATE_WAIT_INIT,
    E_MM_INTERFACE_STATE_INIT_DONE,
    E_MM_INTERFACE_STATE_DATA_INSUFFICENT,
    E_MM_INTERFACE_STATE_DATA_SUFFICENT,
    E_MM_INTERFACE_STATE_PLAYING,
    E_MM_INTERFACE_STATE_PAUSE,
    E_MM_INTERFACE_STATE_PLAY_DONE,

} EN_MM_INTERFACE_PLAYING_STATE;

typedef enum {
    E_MM_INTERFACE_OPTION_NULL = 0,
    E_MM_INTERFACE_OPTION_SET_START_TIME,
    E_MM_INTERFACE_OPTION_SET_TOTAL_TIME,
    E_MM_INTERFACE_OPTION_RESET_BUF,
    E_MM_INTERFACE_OPTION_ENABLE_SEAMLESS,
    E_MM_INTERFACE_OPTION_CHANGE_PROGRAM,
    E_MM_INTERFACE_OPTION_SET_TS_INFO,
    E_MM_INTERFACE_OPTION_SET_VIDEO_DECODE_ALL,
    E_MM_INTERFACE_OPTION_SET_VIDEO_DECODE_I_ONLY,
}EN_MM_INTERFACE_OPTOIN_TYPE;

typedef enum {
    E_MM_INTERFACE_INFO_NULL = 0,
    E_MM_INTERFACE_INFO_TOTAL_TIME,
    E_MM_INTERFACE_INFO_CUR_TIME,
    E_MM_INTERFACE_INFO_BUFFER_PERCENT,
    E_MM_INTERFACE_INFO_ACCETPT_NEXT_SEAMLESS,
    E_MM_INTERFACE_INFO_GET_BUFFER_TOTAL_SIZE,
    E_MM_INTERFACE_INFO_GET_BUFFER_USED_SIZE,
}EN_MM_INTERFACE_INFO_TYPE;

typedef enum {
    /// Default APP mode ( AVSync, No min rebuffering mode, queue audio data, No slow sync)
    E_MM_INTERFACE_APP_MODE_DEFAULT = 0,
    /// Low latency APP mode ( No AVSync, min rebuffering mode, No queue audio data, No slow sync)
    E_MM_INTERFACE_APP_MODE_LOW_LATENCY,
    /// Fast Start APP mode (AVSync, min rebuffering mode, No queue audio data, slow sync)
    E_MM_INTERFACE_APP_MODE_FAST_START,
} EN_MM_INTERFACE_APP_MODE;

typedef enum {
    E_MM_INTERFACE_PLAYER_TYPE_MSTPLAYER,
    E_MM_INTERFACE_PLAYER_TYPE_MSTARPLAYER,
    E_MM_INTERFACE_PLAYER_TYPE_NONE,

}EN_MM_INTERFACE_PLAYER_TYPE;

/// Define control flag for TEE encryption type
typedef enum
{
    /// Not TEE
    E_MM_INTERFACE_TEE_ENCRYPT_CTL_FLAG_NONE     = 0x00000000,
    /// If TEE is, set this flag
    E_MM_INTERFACE_TEE_ENCRYPT_CTL_FLAG_ISTEE    = 0x00000001,
    /// encrypt type is VES
    E_MM_INTERFACE_TEE_ENCRYPT_CTL_FLAG_VES      = 0x00000002,
    /// encrypt type is AES
    E_MM_INTERFACE_TEE_ENCRYPT_CTL_FLAG_AES      = 0x00000004,
    /// encrypt type is VPES
    E_MM_INTERFACE_TEE_ENCRYPT_CTL_FLAG_VPES     = 0x00000008,
    /// encrypt type is APES
    E_MM_INTERFACE_TEE_ENCRYPT_CTL_FLAG_APES     = 0x00000010,
    /// encrypt type is All
    E_MM_INTERFACE_TEE_ENCRYPT_CTL_FLAG_ALL      = 0x00000020,
} E_MM_INTERFACE_TEE_ENCRYPT_CTL_FLAG;

typedef struct {
    EN_MM_INTERFACE_STREAM_TYPE eStreamType;
    EN_MM_INTERFACE_SEEKABLE_MODE eSeekableMode;
    EN_MM_INTERFACE_VIDEO_CODEC eESVideoCodec;
    EN_MM_INTERFACE_AUDIO_CODEC eESAudioCodec;
    EN_MM_INTERFACE_MEDIA_FORMAT_TYPE eMediaFormat;
    EN_MM_INTERFACE_AP_TYPE eAppType;
    EN_MM_INTERFACE_APP_MODE eAppMode;
    EN_MM_INTERFACE_PLAYER_TYPE ePlayerType;
    unsigned int u32TeeEncryptFlag;
    bool bUseRingBuff;
    bool bPC2TVMode;
    bool bAutoPauseResume;
}ST_MM_INTERFACE_DATASOURCE_INFO;

typedef struct {
    int u32ProgID;
    /// Video Codec ID
    EN_MM_INTERFACE_VIDEO enVideoCodecType;
    /// Audio Codec ID
    EN_MM_INTERFACE_AUDIO enAudioCodecType;
    /// Video PID
    int u32VideoPID;
    /// Audio PID
    int u32AudioPID;
    /// Transport packet length
    int u32TransportPacketLen;     // TS 188, 192, 204 bytes or DS length.
}ST_MM_INTERFACE_TS_INFO;

typedef void (*CmdCallback) (EN_MM_INTERFACE_NOTIFY_TYPE eCmd, unsigned int u32Param, unsigned int u32Info, void* pInstance);//the callback

class PlayerInterface;
class InterfaceListener: public MediaPlayerListener {
public:
    InterfaceListener( PlayerInterface* Player);
    ~InterfaceListener( );
    void notify(int msg, int ext1, int ext2, const Parcel *obj);

private:
    PlayerInterface*mPlayer;
};


class PlayerInterface : public MediaPlayer {
public:
    typedef enum {
        /// No seamless mode
        E_MM_INTERFACE_SEAMLESS_NONE,
        /// Seamless FreeZ
        E_MM_INTERFACE_SEAMLESS_FREEZ,
        /// Seamless Smoth. only for mp4 and flv container
        E_MM_INTERFACE_SEAMLESS_SMOTH,
    }EN_MM_INTERFACE_SEAMLESS_MODE;


    PlayerInterface();
    ~PlayerInterface();

    void RegisterCallBack(CmdCallback pCmdCbFunc, void* pInstance = NULL);

    status_t SetDataSource(ST_MM_INTERFACE_DATASOURCE_INFO* pstSourceInfo,sp <BaseDataSource> DataSource);
    status_t SetSurface(const sp<IGraphicBufferProducer>& surfaceTexture);
    status_t Play(char* url);
    status_t Stop();
    status_t Pause();
    status_t Resume();
    status_t SeekTo(int msec);
    bool SetPlayMode(int speed);
    status_t SetPlayModePermille(int speed_permille);
    status_t SetOption(EN_MM_INTERFACE_OPTOIN_TYPE eOption, int arg1 );
    int GetOption(EN_MM_INTERFACE_INFO_TYPE eInfo );
    bool PushPayload();
    bool SwitchToPushPayloadMode();

    status_t _StateProcess();
    void MsgNotify(int msg, int ext1, int ext2, const Parcel *obj = NULL);

    EN_MM_INTERFACE_PLAYING_STATE mePlayingState;
    int mPercentRightNow;



    class MonitorThread : public Thread {
    public:
        MonitorThread(PlayerInterface *player):Thread(false),mPlayer(player){}
        virtual status_t readyToRun();
        virtual bool threadLoop();
    protected:
        ~MonitorThread() {requestExit();}
    private:
        PlayerInterface *mPlayer;
    };



private:
    sp<IGraphicBufferProducer>  msurfaceTexture;
    sp<InterfaceListener>  mListener;
    sp <BaseDataSource> mDataSource;
    ST_MS_DATASOURCE_INFO mstSourceInfo;
    sp<MonitorThread> mThread;
    mutable Mutex mLock;
    int mStartTime;
    thread_id_t                  mLockPiThreadId;
    Vector<EN_MM_INTERFACE_PLAYING_STATE>   mMyVector;
    void* mpApInstance;
    CmdCallback mCallback;

};

}; // namespace android

#endif // ANDROID_PlAYERINTERFACE_H
