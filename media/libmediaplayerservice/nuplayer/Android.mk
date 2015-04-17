LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:=                       \
        GenericSource.cpp               \
        HTTPLiveSource.cpp              \
        NuPlayer.cpp                    \
        NuPlayerDecoder.cpp             \
        NuPlayerDriver.cpp              \
        NuPlayerRenderer.cpp            \
        NuPlayerStreamListener.cpp      \
        RTSPSource.cpp                  \
        StreamingSource.cpp             \
        mp4/MP4Source.cpp               \

LOCAL_C_INCLUDES := \
	$(TOP)/frameworks/av/media/libstagefright/httplive            \
	$(TOP)/frameworks/av/media/libstagefright/include             \
	$(TOP)/frameworks/av/media/libstagefright/mpeg2ts             \
	$(TOP)/frameworks/av/media/libstagefright/rtsp                \
	$(TOP)/frameworks/native/include/media/openmax

# MStar Android Patch Begin
ifeq ($(BUILD_WITH_MARLIN),true)
LOCAL_C_INCLUDES += \
    $(MARLIN_ROOT)/Embedded/AOSP/Source                           \
    $(MARLIN_ROOT)/Source/Interface/Core                          \
    $(MARLIN_ROOT)/Source/Interface/Extended                      \
    $(MARLIN_ROOT)/Source/Common/Core                             \
    $(MARLIN_ROOT)/Embedded/MediaInput                            \
    $(MARLIN_ROOT)/ThirdParty/Atomix/Source/Core                  \
    $(MARLIN_ROOT)/ThirdParty/Sushi/Source/Core/Interface         \
    $(MARLIN_ROOT)/ThirdParty/Sushi/Source/Sushi                  \
    $(MARLIN_ROOT)/ThirdParty/Sushi/Source/Common/Core            \
    $(MARLIN_ROOT)/ThirdParty/Neptune/Source/Core
LOCAL_CFLAGS += -DBUILD_WITH_MARLIN
endif

ifeq ($(BUILD_WITH_MSS_PLAYREADY),true)
LOCAL_C_INCLUDES += \
    $(TOP)/device/mstar/common/libraries/media/mmplayer/mss/src
LOCAL_CFLAGS += -DBUILD_WITH_MSS_PLAYREADY
endif #ifeq ($(BUILD_WITH_MSS_PLAYREADY),true)
# MStar Android Patch End

LOCAL_MODULE:= libstagefright_nuplayer

LOCAL_MODULE_TAGS := eng

include $(BUILD_STATIC_LIBRARY)
