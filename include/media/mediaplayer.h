/*
 * Copyright (C) 2007 The Android Open Source Project
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

#ifndef ANDROID_MEDIAPLAYER_H
#define ANDROID_MEDIAPLAYER_H

#include <arpa/inet.h>

#include <binder/IMemory.h>
#include <media/IMediaPlayerClient.h>
#include <media/IMediaPlayer.h>
#include <media/IMediaDeathNotifier.h>
#include <media/IStreamSource.h>
// MStar Android Patch Begin
#include <media/MstarMMSource.h>
// MStar Android Patch End

#include <utils/KeyedVector.h>
#include <utils/String8.h>

class ANativeWindow;

namespace android {

class Surface;
class IGraphicBufferProducer;

enum media_event_type {
    MEDIA_NOP               = 0, // interface test message
    MEDIA_PREPARED          = 1,
    MEDIA_PLAYBACK_COMPLETE = 2,
    MEDIA_BUFFERING_UPDATE  = 3,
    MEDIA_SEEK_COMPLETE     = 4,
    MEDIA_SET_VIDEO_SIZE    = 5,
    MEDIA_STARTED           = 6,
    MEDIA_PAUSED            = 7,
    MEDIA_STOPPED           = 8,
    MEDIA_SKIPPED           = 9,
    MEDIA_TIMED_TEXT        = 99,
    MEDIA_ERROR             = 100,
    MEDIA_INFO              = 200,
    MEDIA_SUBTITLE_DATA     = 201,
};

// Generic error codes for the media player framework.  Errors are fatal, the
// playback must abort.
//
// Errors are communicated back to the client using the
// MediaPlayerListener::notify method defined below.
// In this situation, 'notify' is invoked with the following:
//   'msg' is set to MEDIA_ERROR.
//   'ext1' should be a value from the enum media_error_type.
//   'ext2' contains an implementation dependant error code to provide
//          more details. Should default to 0 when not used.
//
// The codes are distributed as follow:
//   0xx: Reserved
//   1xx: Android Player errors. Something went wrong inside the MediaPlayer.
//   2xx: Media errors (e.g Codec not supported). There is a problem with the
//        media itself.
//   3xx: Runtime errors. Some extraordinary condition arose making the playback
//        impossible.
//
enum media_error_type {
    // 0xx
    MEDIA_ERROR_UNKNOWN = 1,
    // 1xx
    MEDIA_ERROR_SERVER_DIED = 100,
    // 2xx
    MEDIA_ERROR_NOT_VALID_FOR_PROGRESSIVE_PLAYBACK = 200,
    // 3xx
};


// Info and warning codes for the media player framework.  These are non fatal,
// the playback is going on but there might be some user visible issues.
//
// Info and warning messages are communicated back to the client using the
// MediaPlayerListener::notify method defined below.  In this situation,
// 'notify' is invoked with the following:
//   'msg' is set to MEDIA_INFO.
//   'ext1' should be a value from the enum media_info_type.
//   'ext2' contains an implementation dependant info code to provide
//          more details. Should default to 0 when not used.
//
// The codes are distributed as follow:
//   0xx: Reserved
//   7xx: Android Player info/warning (e.g player lagging behind.)
//   8xx: Media info/warning (e.g media badly interleaved.)
//
enum media_info_type {
    // 0xx
    MEDIA_INFO_UNKNOWN = 1,
    // The player was started because it was used as the next player for another
    // player, which just completed playback
    MEDIA_INFO_STARTED_AS_NEXT = 2,
    // The player just pushed the very first video frame for rendering
    MEDIA_INFO_RENDERING_START = 3,
    // 7xx
    // The video is too complex for the decoder: it can't decode frames fast
    // enough. Possibly only the audio plays fine at this stage.
    MEDIA_INFO_VIDEO_TRACK_LAGGING = 700,
    // MediaPlayer is temporarily pausing playback internally in order to
    // buffer more data.
    MEDIA_INFO_BUFFERING_START = 701,
    // MediaPlayer is resuming playback after filling buffers.
    MEDIA_INFO_BUFFERING_END = 702,
    // Bandwidth in recent past
    MEDIA_INFO_NETWORK_BANDWIDTH = 703,
    // MStar Android Patch Begin
    MEDIA_INFO_BUFFERING_PERCENT = 704,
    MEDIA_INFO_PLAYING_BITRATE   = 705,
    MEDIA_INFO_HTTP_RESPONSE_CODE = 706,
    // MStar Android Patch End

    // 8xx
    // Bad interleaving means that a media has been improperly interleaved or not
    // interleaved at all, e.g has all the video samples first then all the audio
    // ones. Video is playing but a lot of disk seek may be happening.
    MEDIA_INFO_BAD_INTERLEAVING = 800,
    // The media is not seekable (e.g live stream).
    MEDIA_INFO_NOT_SEEKABLE = 801,
    // New media metadata is available.
    MEDIA_INFO_METADATA_UPDATE = 802,

    //9xx
    MEDIA_INFO_TIMED_TEXT_ERROR = 900,

    // MStar Android Patch Begin
    MEDIA_INFO_SUBTITLE_UPDATA = 1000,
    // Online play cache percentage
    MEDIA_INFO_NETWORK_CACHE_PERCENT = 1001,
    // Audio format unsupport.
    MEDIA_INFO_AUDIO_UNSUPPORT = 1002,
    // Video format unsupport.
    MEDIA_INFO_VIDEO_UNSUPPORT = 1003,
    // Video decoder report continuous error
    MEDIA_INFO_VIDEO_DECODE_CONTINUOUS_ERR = 1004,
    // Video display by hardware
    MEDIA_INFO_VIDEO_DISPLAY_BY_HARDWARE = 1005,
    // MStar Android Patch End
};



enum media_player_states {
    MEDIA_PLAYER_STATE_ERROR        = 0,
    MEDIA_PLAYER_IDLE               = 1 << 0,
    MEDIA_PLAYER_INITIALIZED        = 1 << 1,
    MEDIA_PLAYER_PREPARING          = 1 << 2,
    MEDIA_PLAYER_PREPARED           = 1 << 3,
    MEDIA_PLAYER_STARTED            = 1 << 4,
    MEDIA_PLAYER_PAUSED             = 1 << 5,
    MEDIA_PLAYER_STOPPED            = 1 << 6,
    MEDIA_PLAYER_PLAYBACK_COMPLETE  = 1 << 7
};

// Keep KEY_PARAMETER_* in sync with MediaPlayer.java.
// The same enum space is used for both set and get, in case there are future keys that
// can be both set and get.  But as of now, all parameters are either set only or get only.
enum media_parameter_keys {
    // Streaming/buffering parameters
    KEY_PARAMETER_CACHE_STAT_COLLECT_FREQ_MS = 1100,            // set only

    // Return a Parcel containing a single int, which is the channel count of the
    // audio track, or zero for error (e.g. no audio track) or unknown.
    KEY_PARAMETER_AUDIO_CHANNEL_COUNT = 1200,                   // get only

    // Playback rate expressed in permille (1000 is normal speed), saved as int32_t, with negative
    // values used for rewinding or reverse playback.
    KEY_PARAMETER_PLAYBACK_RATE_PERMILLE = 1300,                // set only

    // MStar Android Patch Begin
    KEY_PARAMETER_DEMUX_RESET = 2000,                           // set only
    KEY_PARAMETER_SET_SEAMLESS_MODE = 2001,                     // set only
    KEY_PARAMETER_DATASOURCE_SWITCH = 2002,                     // set only
    KEY_PARAMETER_PAYLOAD_SHOT = 2003,                          // set only
    KEY_PARAMETER_SWITCH_TO_PUSH_DATA_MODE = 2004,              // set only
    KEY_PARAMETER_CHANGE_PROGRAM = 2005,                        // set only
    KEY_PARAMETER_MEDIA_CREATE_THUMBNAIL_MODE = 2006,
    KEY_PARAMETER_VIDEO_ONLY_MODE = 2007,
    KEY_PARAMETER_AVCODEC_CHANGED = 2008,
    KEY_PARAMETER_SET_TS_INFO = 2009,
    KEY_PARAMETER_ACCEPT_NEXT_SEAMLESS = 2010,
    KEY_PARAMETER_SET_SUBTITLE_INDEX = 2011,                    // set only
    KEY_PARAMETER_GET_SUBTITLE_TRACK_INFO = 2012,               // get only
    KEY_PARAMETER_GET_ALL_SUBTITLE_TRACK_INFO = 2013,
    KEY_PARAMETER_SET_RESUME_PLAY = 2014,
    KEY_PARAMETER_GET_DIVX_DRM_IS_RENTAL_FILE  = 2015,
    KEY_PARAMETER_GET_DIVX_DRM_RENTAL_LIMIT  = 2016,
    KEY_PARAMETER_GET_DIVX_DRM_RENTAL_USE_COUNT  = 2017,
    KEY_PARAMETER_GET_BUFFER_STATUS = 2018,
    KEY_PARAMETER_SET_VIDEO_DECODE_ALL = 2019,
    KEY_PARAMETER_SET_VIDEO_DECODE_I_ONLY = 2020,
    KEY_PARAMETER_CHANGE_AV_SYNCHRONIZATION = 2021,             // 1:Sync, 0:Non-Sync
    KEY_PARAMETER_GET_CONTAINER_PTS = 2022,                     // get only
    KEY_PARAMETER_SET_LOW_LATENCY_MODE = 2023,
    // Java app informs player to request Resource Manager v4 for dual decode or PIP mode
    KEY_PARAMETER_SET_DUAL_DECODE_PIP = 2024,                   // set only

    // DIVX
    KEY_PARAMETER_GET_TITLE_COUNT = 2025,
    KEY_PARAMETER_GET_ALL_TITLE_NAME = 2026,
    KEY_PARAMETER_GET_EDITION_COUNT = 2027,
    KEY_PARAMETER_GET_ALL_EDITION_NAME = 2028,
    KEY_PARAMETER_GET_AUTHOR_CHAPTER_COUNT = 2029,
    KEY_PARAMETER_GET_ALL_CHAPTER_NAME = 2030,
    KEY_PARAMETER_GET_LAW_RATING = 2031,
    KEY_PARAMETER_GET_ALL_AUDIO_TRACK_INFO = 2032,
    KEY_PARAMETER_GET_TITLE_EDITION = 2033,
    KEY_PARAMETER_GET_CHAPTER = 2034,
    KEY_PARAMETER_SET_TITLE_EDITION = 2035,
    KEY_PARAMETER_SET_CHAPTER = 2036,
    KEY_PARAMETER_GET_ACTIVE_AUDIO_TRACK_INFO = 2037,
    KEY_PARAMETER_GET_ACTIVE_AUDIO_TRACK_NAME = 2038,
    KEY_PARAMETER_GET_ACTIVE_SUBTITLE_TRACK_NAME = 2039,

    // Multi thumbnail usage
    KEY_PARAMETER_SET_MULTI_THUMBS = 2040,

    // Set video hw display app mode to local playback
    KEY_PARAMETER_SET_PQ_LOCAL = 2041,

    KEY_PARAMETER_SET_REBUFFER_DURATION = 2042, // unit: second

    // Set video hw display app mode to game mode
    KEY_PARAMETER_SET_GAME_MODE = 2043,

    // DIVX
    KEY_PARAMETER_GET_ALL_AUDIO_TRACK_NAME = 2044,
    KEY_PARAMETER_GET_ALL_SUBTITLE_TRACK_NAME = 2045,

    KEY_PARAMETER_SET_FAST_START_MODE = 2046, // set minimal rebuffer mode and start player without sync av
    KEY_PARAMETER_SET_AUDIO_DROP_THRESHOLD = 2047, // set audio drop threshold (ms) for low latency mode
   // Content Source
    KEY_PARAMETER_GET_CS_BITRATES_INFO = 2048,
    KEY_PARAMETER_SET_CS_BITRATES = 2049,
    // IMAGE PLAYER
    KEY_PARAMETER_SET_IMAGE_SAMPLESIZE_SURFACESIZE = 3000,
    KEY_PARAMETER_ROTATE = 3001,
    KEY_PARAMETER_SCALE = 3002,
    KEY_PARAMETER_ROTATE_SCALE = 3003,
    KEY_PARAMETER_CROP_RECT = 3004,
    KEY_PARAMETER_DECODE_NEXT = 3005,
    KEY_PARAMETER_SHOW_NEXT = 3006,
    // MStar Android Patch End
};

// Keep INVOKE_ID_* in sync with MediaPlayer.java.
enum media_player_invoke_ids {
    INVOKE_ID_GET_TRACK_INFO = 1,
    INVOKE_ID_ADD_EXTERNAL_SOURCE = 2,
    INVOKE_ID_ADD_EXTERNAL_SOURCE_FD = 3,
    INVOKE_ID_SELECT_TRACK = 4,
    INVOKE_ID_UNSELECT_TRACK = 5,
    INVOKE_ID_SET_VIDEO_SCALING_MODE = 6,

    // MStar Android Patch Begin
#ifdef BUILD_WITH_MARLIN
    // Keep INVOKE_ID_WASABI_DRM in sync with com.intertrust.wasabi.Drm.hava!
    INVOKE_ID_WASABI_DRM = 9876,
#endif
    INVOKE_ID_PUSH_ES_DATA = 1000,
    INVOKE_ID_FLUSH_ES_DATA = 1001,
    INVOKE_ID_GET_MS_TRACK_INFO = 1002,
    // MStar Android Patch End
};

// Keep MEDIA_TRACK_TYPE_* in sync with MediaPlayer.java.
enum media_track_type {
    MEDIA_TRACK_TYPE_UNKNOWN = 0,
    MEDIA_TRACK_TYPE_VIDEO = 1,
    MEDIA_TRACK_TYPE_AUDIO = 2,
    MEDIA_TRACK_TYPE_TIMEDTEXT = 3,
    MEDIA_TRACK_TYPE_SUBTITLE = 4,
    // MStar Android Patch Begin
    MEDIA_TRACK_TYPE_TIMEDBITMAP = 5,
    // MStar Android Patch End
};

// ----------------------------------------------------------------------------
// ref-counted object for callbacks
class MediaPlayerListener: virtual public RefBase
{
public:
    virtual void notify(int msg, int ext1, int ext2, const Parcel *obj) = 0;
};

class MediaPlayer : public BnMediaPlayerClient,
                    public virtual IMediaDeathNotifier
{
public:
    MediaPlayer();
    ~MediaPlayer();
            void            died();
            void            disconnect();

            status_t        setDataSource(
                    const char *url,
                    const KeyedVector<String8, String8> *headers);

            status_t        setDataSource(int fd, int64_t offset, int64_t length);
            status_t        setDataSource(const sp<IStreamSource> &source);
            // MStar Android Patch Begin
            status_t        setDataSource(const sp<BaseDataSource> &source);
            void            setDataSourceInfo(const ST_MS_DATASOURCE_INFO &dataSourceInfo);
            sp<MediaPlayerListener> getListener();
            // MStar Android Patch End
            status_t        setVideoSurfaceTexture(
                                    const sp<IGraphicBufferProducer>& bufferProducer);
            status_t        setListener(const sp<MediaPlayerListener>& listener);
            status_t        prepare();
            status_t        prepareAsync();
            status_t        start();
            status_t        stop();
            status_t        pause();
            bool            isPlaying();
            status_t        getVideoWidth(int *w);
            status_t        getVideoHeight(int *h);
            status_t        seekTo(int msec);
            status_t        getCurrentPosition(int *msec);
            status_t        getDuration(int *msec);
            status_t        reset();
            status_t        setAudioStreamType(audio_stream_type_t type);
            status_t        setLooping(int loop);
            bool            isLooping();
            status_t        setVolume(float leftVolume, float rightVolume);
            void            notify(int msg, int ext1, int ext2, const Parcel *obj = NULL);
    static  status_t        decode(const char* url, uint32_t *pSampleRate, int* pNumChannels,
                                   audio_format_t* pFormat,
                                   const sp<IMemoryHeap>& heap, size_t *pSize);
    static  status_t        decode(int fd, int64_t offset, int64_t length, uint32_t *pSampleRate,
                                   int* pNumChannels, audio_format_t* pFormat,
                                   const sp<IMemoryHeap>& heap, size_t *pSize);
            status_t        invoke(const Parcel& request, Parcel *reply);
            status_t        setMetadataFilter(const Parcel& filter);
            status_t        getMetadata(bool update_only, bool apply_filter, Parcel *metadata);
            status_t        setAudioSessionId(int sessionId);
            int             getAudioSessionId();
            status_t        setAuxEffectSendLevel(float level);
            status_t        attachAuxEffect(int effectId);
            status_t        setParameter(int key, const Parcel& request);
            status_t        getParameter(int key, Parcel* reply);
            status_t        setRetransmitEndpoint(const char* addrString, uint16_t port);
            status_t        setNextMediaPlayer(const sp<MediaPlayer>& player);

            status_t updateProxyConfig(
                    const char *host, int32_t port, const char *exclusionList);

            // MStar Android Patch Begin
            bool            setPlayMode(int speed);
            int             getPlayMode();
            status_t        setPlayModePermille(int speed_permille);
            int             getPlayModePermille();
            status_t        setAudioTrack(int track);
            status_t        setSubtitleTrack(int track);
            status_t        setSubtitleDataSource(const char *url);
            void            offSubtitleTrack();
            void            onSubtitleTrack();
            char*           getSubtitleData(int *size);
            char*           getAudioTrackStringData(int *size,int infoType);
            status_t        setSubtitleSurface(const sp<IGraphicBufferProducer>& surface);
            int             setSubtitleSync(int timeMs);
            void            divx_SetTitle(uint32_t u32ID);
            status_t        divx_GetTitle(uint32_t *pu32ID);
            void            divx_SetEdition(uint32_t u32ID);
            status_t        divx_GetEdition(uint32_t *pu32ID);
            void            divx_SetChapter(uint32_t u32ID);
            status_t        divx_GetChapter(uint32_t *pu32ID);
            void            divx_SetAutochapter(uint32_t u32ID);
            status_t        divx_GetAutochapter(uint32_t *pu32ID);
            status_t        divx_GetAutochapterTime(uint32_t u32ID, uint32_t *pu32IdTime);
            status_t        divx_GetChapterTime(uint32_t u32ID, uint32_t *pu32IdTime);
            status_t        divx_SetResumePlay(const Parcel &inResumeInfo);
            status_t        divx_SetReplayFlag(bool isResumePlay);
            status_t        captureMovieThumbnail(sp<IMemory>& mem, Parcel *pstCaptureParam, int* pCapBufPitch);

            status_t        setVideoDisplayAspectRatio(int eRatio);
            status_t        getVideoPTS(int *msec);
            // MStar Android Patch End
private:
            void            clear_l();
            status_t        seekTo_l(int msec);
            status_t        prepareAsync_l();
            status_t        getDuration_l(int *msec);
            status_t        attachNewPlayer(const sp<IMediaPlayer>& player);
            status_t        reset_l();
            status_t        doSetRetransmitEndpoint(const sp<IMediaPlayer>& player);

    sp<IMediaPlayer>            mPlayer;
    thread_id_t                 mLockThreadId;
    Mutex                       mLock;
    Mutex                       mNotifyLock;
    Condition                   mSignal;
    sp<MediaPlayerListener>     mListener;
    void*                       mCookie;
    media_player_states         mCurrentState;
    int                         mCurrentPosition;
    int                         mSeekPosition;
    bool                        mPrepareSync;
    status_t                    mPrepareStatus;
    audio_stream_type_t         mStreamType;
    bool                        mLoop;
    float                       mLeftVolume;
    float                       mRightVolume;
    int                         mVideoWidth;
    int                         mVideoHeight;
    int                         mAudioSessionId;
    float                       mSendLevel;
    struct sockaddr_in          mRetransmitEndpoint;
    bool                        mRetransmitEndpointValid;

    // MStar Android Patch Begin
    sp<MstarMMSource>           extDataSource;
    ST_MS_DATASOURCE_INFO       mDataSourceInfo;
    // MStar Android Patch End
};

}; // namespace android

#endif // ANDROID_MEDIAPLAYER_H
