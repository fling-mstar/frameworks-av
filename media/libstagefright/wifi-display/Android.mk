LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
        MediaSender.cpp                 \
        Parameters.cpp                  \
        rtp/RTPSender.cpp               \
        source/Converter.cpp            \
        source/MediaPuller.cpp          \
        source/PlaybackSession.cpp      \
        source/RepeaterSource.cpp       \
        source/TSPacketizer.cpp         \
        source/WifiDisplaySource.cpp    \
        VideoFormats.cpp                \

LOCAL_C_INCLUDES:= \
        $(TOP)/frameworks/av/media/libstagefright \
        $(TOP)/frameworks/native/include/media/openmax \
        $(TOP)/frameworks/av/media/libstagefright/mpeg2ts \

LOCAL_SHARED_LIBRARIES:= \
        libbinder                       \
        libcutils                       \
        liblog                          \
        libgui                          \
        libmedia                        \
        libstagefright                  \
        libstagefright_foundation       \
        libui                           \
        libutils                        \

# MStar Android Patch Begin
LOCAL_SRC_FILES += \
        sink/LinearRegression.cpp       \
        sink/RTPSink.cpp                \
        sink/TunnelRenderer.cpp         \
        sink/WifiDisplaySink.cpp        \

ifneq ($(BUILD_MSTARTV),)
LOCAL_C_INCLUDES += \
        $(TOP)/device/mstar/common/libraries/security \
        $(TOP)/device/mstar/common/libraries/hdcp22/src \
        $(TOP)/device/mstar/common/libraries/hdcp22/include \
        $(TOP)/device/mstar/common/libraries/hdcp22/src/app
LOCAL_CFLAGS += -DBUILD_MSTARTV

LOCAL_SHARED_LIBRARIES += \
        libssl \
        libcrypto \
        libHdcpSecureApi
LOCAL_STATIC_LIBRARIES += libhdcp22
endif
# MStar Android Patch End

LOCAL_MODULE:= libstagefright_wfd

LOCAL_MODULE_TAGS:= optional

include $(BUILD_SHARED_LIBRARY)
