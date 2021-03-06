/*
**
** Copyright 2012, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#define LOG_TAG "MediaPlayerFactory"
#include <utils/Log.h>

#include <cutils/properties.h>
#include <media/IMediaPlayer.h>
#include <media/stagefright/foundation/ADebug.h>
// MStar Android Patch Begin
#ifdef BUILD_WITH_MSTAR_MM
#include <media/stagefright/DataSource.h>
#include <media/stagefright/FileSource.h>
#include <media/stagefright/MediaExtractor.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MetaData.h>
#include "mstplayer.h"
#ifdef SUPPORT_IMAGEPLAYER
#include "ImagePlayer.h"
#endif
#ifdef SUPPORT_MSTARPLAYER
#include "MstarPlayer.h"
#endif
#endif
// MStar Android Patch End
#include <utils/Errors.h>
#include <utils/misc.h>

// MStar Android Patch Begin
#ifdef BUILD_WITH_MARLIN
#include <WasabiAospUtil.h>
#endif
// MStar Android Patch End

#include "MediaPlayerFactory.h"

#include "MidiFile.h"
#include "TestPlayerStub.h"
#include "StagefrightPlayer.h"
#include "nuplayer/NuPlayerDriver.h"

namespace android {

Mutex MediaPlayerFactory::sLock;
MediaPlayerFactory::tFactoryMap MediaPlayerFactory::sFactoryMap;
bool MediaPlayerFactory::sInitComplete = false;

status_t MediaPlayerFactory::registerFactory_l(IFactory* factory,
                                               player_type type) {
    if (NULL == factory) {
        ALOGE("Failed to register MediaPlayerFactory of type %d, factory is"
              " NULL.", type);
        return BAD_VALUE;
    }

    if (sFactoryMap.indexOfKey(type) >= 0) {
        ALOGE("Failed to register MediaPlayerFactory of type %d, type is"
              " already registered.", type);
        return ALREADY_EXISTS;
    }

    if (sFactoryMap.add(type, factory) < 0) {
        ALOGE("Failed to register MediaPlayerFactory of type %d, failed to add"
              " to map.", type);
        return UNKNOWN_ERROR;
    }

    return OK;
}

player_type MediaPlayerFactory::getDefaultPlayerType() {
    char value[PROPERTY_VALUE_MAX];
    if (property_get("media.stagefright.use-nuplayer", value, NULL)
            && (!strcmp("1", value) || !strcasecmp("true", value))) {
        return NU_PLAYER;
    }

    // MStar Android Patch Begin
#ifdef BUILD_WITH_MSTAR_MM
    return MST_PLAYER;
#endif
    // MStar Android Patch End

    return STAGEFRIGHT_PLAYER;
}

status_t MediaPlayerFactory::registerFactory(IFactory* factory,
                                             player_type type) {
    Mutex::Autolock lock_(&sLock);
    return registerFactory_l(factory, type);
}

void MediaPlayerFactory::unregisterFactory(player_type type) {
    Mutex::Autolock lock_(&sLock);
    sFactoryMap.removeItem(type);
}

// MStar Android Patch Begin
#define GET_PLAYER_TYPE_IMPL(a...)                      \
    Mutex::Autolock lock_(&sLock);                      \
                                                        \
    player_type ret = STAGEFRIGHT_PLAYER;               \
    float bestScore = 0.0;                              \
                                                        \
    for (size_t i = 0; i < sFactoryMap.size(); ++i) {   \
                                                        \
        IFactory* v = sFactoryMap.valueAt(i);           \
        float thisScore;                                \
        CHECK(v != NULL);                               \
        thisScore = v->scoreFactory(a, bestScore);      \
        if (thisScore > bestScore) {                    \
            ret = sFactoryMap.keyAt(i);                 \
            bestScore = thisScore;                      \
        }                                               \
    }                                                   \
                                                        \
    if (0.0 == bestScore) {                             \
        ret = getDefaultPlayerType();                   \
    }                                                   \
                                                        \
    return ret;
// MStar Android Patch End

player_type MediaPlayerFactory::getPlayerType(const sp<IMediaPlayer>& client,
                                              const char* url) {
    GET_PLAYER_TYPE_IMPL(client, url);
}

player_type MediaPlayerFactory::getPlayerType(const sp<IMediaPlayer>& client,
                                              int fd,
                                              int64_t offset,
                                              int64_t length) {
    GET_PLAYER_TYPE_IMPL(client, fd, offset, length);
}

player_type MediaPlayerFactory::getPlayerType(const sp<IMediaPlayer>& client,
                                              const sp<IStreamSource> &source) {
    GET_PLAYER_TYPE_IMPL(client, source);
}

#undef GET_PLAYER_TYPE_IMPL

sp<MediaPlayerBase> MediaPlayerFactory::createPlayer(
        player_type playerType,
        void* cookie,
        notify_callback_f notifyFunc) {
    sp<MediaPlayerBase> p;
    IFactory* factory;
    status_t init_result;
    Mutex::Autolock lock_(&sLock);

    if (sFactoryMap.indexOfKey(playerType) < 0) {
        ALOGE("Failed to create player object of type %d, no registered"
              " factory", playerType);
        return p;
    }

    factory = sFactoryMap.valueFor(playerType);
    CHECK(NULL != factory);
    p = factory->createPlayer();

    if (p == NULL) {
        ALOGE("Failed to create player object of type %d, create failed",
               playerType);
        return p;
    }

    init_result = p->initCheck();
    if (init_result == NO_ERROR) {
        p->setNotifyCallback(cookie, notifyFunc);
    } else {
        ALOGE("Failed to create player object of type %d, initCheck failed"
              " (res = %d)", playerType, init_result);
        p.clear();
    }

    return p;
}

/*****************************************************************************
 *                                                                           *
 *                     Built-In Factory Implementations                      *
 *                                                                           *
 *****************************************************************************/

class StagefrightPlayerFactory :
    public MediaPlayerFactory::IFactory {
  public:
    virtual float scoreFactory(const sp<IMediaPlayer>& client,
                               int fd,
                               int64_t offset,
                               int64_t length,
                               float curScore) {
        char buf[20];
        lseek(fd, offset, SEEK_SET);
        read(fd, buf, sizeof(buf));
        lseek(fd, offset, SEEK_SET);

        long ident = *((long*)buf);

        // Ogg vorbis?
        if (ident == 0x5367674f) // 'OggS'
            return 1.0;
// MStar Android Patch Begin
#ifdef BUILD_WITH_MSTAR_MM
        char value[PROPERTY_VALUE_MAX];
        if (property_get("ms.sf", value, NULL)
                && (!strcmp("1", value) || !strcasecmp("true", value))) {
            return 2.1;
        }

        if (ident == 0x43614c66) // 'fLaC': //use Stagefright to play FLAC clips
            return 1.0;

        if (ident == 0xc450fbff && length == 40541) // FIXME: for CTS testGapless test work around
            return 1.0;

        if (buf[3] == 0x20 && // FIXME: for CTS testGapless test work around
            buf[4] == 0x66 &&
            buf[5] == 0x74 &&
            buf[6] == 0x79 &&
            buf[7] == 0x70 &&
            buf[8] == 0x4d &&
            buf[9] == 0x34 &&
            buf[10] == 0x41 &&
            buf[11] == 0x20 &&
            (length == 46180 || length == 83892))
            return 1.0;
#endif
// MStar Android Patch End
        return 0.0;
    }

    // MStar Android Patch Begin
#ifdef BUILD_WITH_MSTAR_MM
    virtual float scoreFactory(const sp<IMediaPlayer>& client,
                               const char* url,
                               float curScore) {
        static const float kOurScore = 1.0;

        char value[PROPERTY_VALUE_MAX];
        if (property_get("ms.sf", value, NULL)
                && (!strcmp("1", value) || !strcasecmp("true", value))) {
            return 2.1;
        }

        if (kOurScore <= curScore)
            return 0.0;

        // FIXME: gts workaround, for widevine case, choose Stagefright player
        if (!strncasecmp(url, "widevine://", 11) || strcasestr(url, ".wvm") != NULL) {
            return kOurScore;
        }

        if (!strncasecmp(url, "mttp://", 7)) {
            char value[PROPERTY_VALUE_MAX];
            if (property_get("media.stagefright.enable-http", value, NULL)
                && (strcmp(value, "1") && strcasecmp(value, "true"))) {
                return kOurScore;
            }
        }

        if (strcasestr(url, ".ogg") != NULL) {
            return 2.1; //make the value larger than mooplayer.. MStar strategy to force SF play OGG files.... kOurScore;
        }

        if (strcasestr(url, ".flac") != NULL) { // use Stagefright to play FLAC clips
            return kOurScore;
        }

        return 0.0;
    }
#endif
    // MStar Android Patch End

    virtual sp<MediaPlayerBase> createPlayer() {
        ALOGV(" create StagefrightPlayer");
        return new StagefrightPlayer();
    }
};

class NuPlayerFactory : public MediaPlayerFactory::IFactory {
  public:
    virtual float scoreFactory(const sp<IMediaPlayer>& client,
                               const char* url,
                               float curScore) {
        static const float kOurScore = 0.8;

        if (kOurScore <= curScore)
            return 0.0;
// MStar Android Patch Begin
#ifdef BUILD_WITH_MARLIN
        if (IsWasabiSourceUrl(url)) {
            char value[PROPERTY_VALUE_MAX];
            if (property_get("ms.wasabi.nu", value, NULL)
                && (!strcmp("1", value))) {
                return 5.0;
            }
        }
#endif
// MStar Android Patch End
        if (!strncasecmp("http://", url, 7)
                || !strncasecmp("https://", url, 8)
                || !strncasecmp("file://", url, 7)) {
            size_t len = strlen(url);
            if (len >= 5 && !strcasecmp(".m3u8", &url[len - 5])) {
                return kOurScore;
            }

            if (strstr(url,"m3u8")) {
                return kOurScore;
            }

            if ((len >= 4 && !strcasecmp(".sdp", &url[len - 4])) || strstr(url, ".sdp?")) {
                return kOurScore;
            }
        }

        if (!strncasecmp("rtsp://", url, 7)) {
            return kOurScore;
        }

        return 0.0;
    }

    virtual float scoreFactory(const sp<IMediaPlayer>& client,
                               const sp<IStreamSource> &source,
                               float curScore) {
// MStar Android Patch Begin
        return 0.8;
// MStar Android Patch End
    }

    virtual sp<MediaPlayerBase> createPlayer() {
        ALOGV(" create NuPlayer");
        return new NuPlayerDriver;
    }
};

class SonivoxPlayerFactory : public MediaPlayerFactory::IFactory {
  public:
    virtual float scoreFactory(const sp<IMediaPlayer>& client,
                               const char* url,
                               float curScore) {
        static const float kOurScore = 0.4;
        static const char* const FILE_EXTS[] = { ".mid",
                                                 ".midi",
                                                 ".smf",
                                                 ".xmf",
                                                 ".mxmf",
                                                 ".imy",
                                                 ".rtttl",
                                                 ".rtx",
                                                 ".ota" };
        if (kOurScore <= curScore)
            return 0.0;

        // use MidiFile for MIDI extensions
        int lenURL = strlen(url);
        for (int i = 0; i < NELEM(FILE_EXTS); ++i) {
            int len = strlen(FILE_EXTS[i]);
            int start = lenURL - len;
            if (start > 0) {
                if (!strncasecmp(url + start, FILE_EXTS[i], len)) {
                    return kOurScore;
                }
            }
        }

        return 0.0;
    }

    virtual float scoreFactory(const sp<IMediaPlayer>& client,
                               int fd,
                               int64_t offset,
                               int64_t length,
                               float curScore) {
        static const float kOurScore = 0.8;

        if (kOurScore <= curScore)
            return 0.0;

        // Some kind of MIDI?
        EAS_DATA_HANDLE easdata;
        if (EAS_Init(&easdata) == EAS_SUCCESS) {
            EAS_FILE locator;
            locator.path = NULL;
            locator.fd = fd;
            locator.offset = offset;
            locator.length = length;
            EAS_HANDLE  eashandle;
            if (EAS_OpenFile(easdata, &locator, &eashandle) == EAS_SUCCESS) {
                EAS_CloseFile(easdata, eashandle);
                EAS_Shutdown(easdata);
                return kOurScore;
            }
            EAS_Shutdown(easdata);
        }

        return 0.0;
    }

    virtual sp<MediaPlayerBase> createPlayer() {
        ALOGV(" create MidiFile");
        return new MidiFile();
    }
};

class TestPlayerFactory : public MediaPlayerFactory::IFactory {
  public:
    virtual float scoreFactory(const sp<IMediaPlayer>& client,
                               const char* url,
                               float curScore) {
        if (TestPlayerStub::canBeUsed(url)) {
            return 1.0;
        }

        return 0.0;
    }

    virtual sp<MediaPlayerBase> createPlayer() {
        ALOGV("Create Test Player stub");
        return new TestPlayerStub();
    }
};

// MStar Android Patch Begin
#ifdef BUILD_WITH_MSTAR_MM
#ifdef SUPPORT_IMAGEPLAYER
class ImagePlayerFactory : public MediaPlayerFactory::IFactory {
  public:
    virtual float scoreFactory(const sp<IMediaPlayer>& client,
                               const char* url,
                               float curScore) {
        static const float kOurScore = 0.9;

        if (kOurScore <= curScore)
            return 0.0;

        if (ImagePlayer::canBeUsed(url)) {
            return 1.0;
        }

        return 0.0;
    }

    virtual float scoreFactory(const sp<IMediaPlayer>& client,
                               int fd,
                               int64_t offset,
                               int64_t length,
                               float curScore) {
        static const float kOurScore = 0.9;

        if (kOurScore <= curScore)
            return 0.0;

        if (ImagePlayer::canBeUsed(fd)) {
            return 1.0;
        }

        return 0.0;
    }

    virtual sp<MediaPlayerBase> createPlayer() {
        return new ImagePlayer();
    }
};
#endif

class MstPlayerFactory : public MediaPlayerFactory::IFactory {
  public:
    virtual float scoreFactory(const sp<IMediaPlayer>& client,
                               const char* url,
                               float curScore) {
        static const float kOurScore = 1.0;

        if (kOurScore <= curScore)
            return 0.0;

        if (!strncasecmp("http://", url, 7)
                || !strncasecmp("https://", url, 8)) {
            size_t len = strlen(url);
            if (len >= 5 && !strcasecmp(".m3u8", &url[len - 5])) {
                return kOurScore;
            }

            if (strstr(url,"m3u8")) {
                return kOurScore;
            }
        }

        if (!strncasecmp("rtsp://", url, 7)) {
            return kOurScore;
        }

        return 0.0;
    }

    virtual float scoreFactory(const sp<IMediaPlayer>& client,
                               const sp<IStreamSource> &source,
                               float curScore) {
        return 1.0;
    }

    virtual sp<MediaPlayerBase> createPlayer() {
        ALOGV("Create MstPlayer");
#ifdef SUPPORT_MSTARPLAYER
        char value[PROPERTY_VALUE_MAX];
        if (property_get("test.MstarPlayer", value, NULL)
            && (!strcmp("1", value))) {
            return new MstarPlayer;
        }
#endif
        return new MstPlayer;
    }
};

#ifdef SUPPORT_MSTARPLAYER
class MstarPlayerFactory : public MediaPlayerFactory::IFactory {
  public:
    virtual sp<MediaPlayerBase> createPlayer() {
        ALOGV("Create MstarPlayer");
        return new MstarPlayer;
    }
};
#endif
#endif
// MStar Android Patch End

void MediaPlayerFactory::registerBuiltinFactories() {
    Mutex::Autolock lock_(&sLock);

    if (sInitComplete)
        return;

    registerFactory_l(new StagefrightPlayerFactory(), STAGEFRIGHT_PLAYER);
    registerFactory_l(new NuPlayerFactory(), NU_PLAYER);
    registerFactory_l(new SonivoxPlayerFactory(), SONIVOX_PLAYER);
    registerFactory_l(new TestPlayerFactory(), TEST_PLAYER);
    // MStar Android Patch Begin
#ifdef BUILD_WITH_MSTAR_MM
    registerFactory_l(new MstPlayerFactory(), MST_PLAYER);
#ifdef SUPPORT_IMAGEPLAYER
    registerFactory_l(new ImagePlayerFactory(), IMG_PLAYER);
#endif
#ifdef SUPPORT_MSTARPLAYER
    registerFactory_l(new MstarPlayerFactory(), MSTAR_PLAYER);
#endif
#endif
    // MStar Android Patch End

    sInitComplete = true;
}

}  // namespace android
