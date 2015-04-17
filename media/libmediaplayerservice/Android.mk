LOCAL_PATH:= $(call my-dir)

#
# libmediaplayerservice
#

include $(CLEAR_VARS)

LOCAL_SRC_FILES:=               \
    ActivityManager.cpp         \
    Crypto.cpp                  \
    Drm.cpp                     \
    HDCP.cpp                    \
    MediaPlayerFactory.cpp      \
    MediaPlayerService.cpp      \
    MediaRecorderClient.cpp     \
    MetadataRetrieverClient.cpp \
    MidiFile.cpp                \
    MidiMetadataRetriever.cpp   \
    RemoteDisplay.cpp           \
    SharedLibrary.cpp           \
    StagefrightPlayer.cpp       \
    StagefrightRecorder.cpp     \
    TestPlayerStub.cpp          \

LOCAL_SHARED_LIBRARIES :=       \
    libbinder                   \
    libcamera_client            \
    libcutils                   \
    liblog                      \
    libdl                       \
    libgui                      \
    libmedia                    \
    libsonivox                  \
    libstagefright              \
    libstagefright_foundation   \
    libstagefright_httplive     \
    libstagefright_omx          \
    libstagefright_wfd          \
    libutils                    \
    libvorbisidec               \

LOCAL_STATIC_LIBRARIES :=       \
    libstagefright_nuplayer     \
    libstagefright_rtsp         \

LOCAL_C_INCLUDES :=                                                 \
    $(call include-path-for, graphics corecg)                       \
    $(TOP)/frameworks/av/media/libstagefright/include               \
    $(TOP)/frameworks/av/media/libstagefright/rtsp                  \
    $(TOP)/frameworks/av/media/libstagefright/wifi-display          \
    $(TOP)/frameworks/native/include/media/openmax                  \
    $(TOP)/external/tremolo/Tremolo                                 \

# MStar Android Patch Begin
SUPPORT_MSTARPLAYER := false
SUPPORT_IMAGEPLAYER := true

ifeq ($(BUILD_WITH_MSTAR_MM),true)
LOCAL_C_INCLUDES += \
    device/mstar/common/libraries/media/mmplayer/mmp/platform/mstplayer \
    device/mstar/common/libraries/media/mmplayer/subtitle \
    device/mstar/common/libraries/media/mmplayer/contentSource/inc

LOCAL_CFLAGS += -DBUILD_WITH_MSTAR_MM -DANDROID_JB -D__ICS__

LOCAL_CFLAGS += -DJB_43

LOCAL_SHARED_LIBRARIES += libmmp

ifeq ($(SUPPORT_MSTARPLAYER),true)
LOCAL_C_INCLUDES += \
    device/mstar/common/libraries/media/mmplayer/mstarplayer/include \
    device/mstar/common/libraries/media/mmplayer/mstarplayer/client/exported_api

LOCAL_CFLAGS += -DSUPPORT_MSTARPLAYER

LOCAL_SHARED_LIBRARIES += libmmplayer
endif

ifeq ($(SUPPORT_IMAGEPLAYER),true)
LOCAL_C_INCLUDES += \
    device/mstar/common/libraries/media/mmplayer/imgplayer

LOCAL_CFLAGS += -DSUPPORT_IMAGEPLAYER

LOCAL_SHARED_LIBRARIES += libimgplayer
endif

ifeq ($(BUILD_WITH_MARLIN),true)
LOCAL_C_INCLUDES += \
    $(MARLIN_ROOT)/Embedded/AOSP/Source
LOCAL_CFLAGS += -DBUILD_WITH_MARLIN
LOCAL_STATIC_LIBRARIES += \
    libstagefright_nuplayer_wasabi \
    libWasabi
ifeq ($(BUILD_WITH_TEE),true)
LOCAL_STATIC_LIBRARIES += \
    libSkbCA
LOCAL_SHARED_LIBRARIES += \
    libSema \
    libTEEClient
endif
endif

include external/stlport/libstlport.mk
endif

ifeq ($(BUILD_WITH_MSS_PLAYREADY),true)
LOCAL_SHARED_LIBRARIES += \
    libstlport \
    libz \
    libmss
endif #ifeq ($(BUILD_WITH_MSS_PLAYREADY),true)
# MStar Android Patch End

LOCAL_MODULE:= libmediaplayerservice

include $(BUILD_SHARED_LIBRARY)

include $(call all-makefiles-under,$(LOCAL_PATH))
