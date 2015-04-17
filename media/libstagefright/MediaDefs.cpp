/*
 * Copyright (C) 2009 The Android Open Source Project
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

#include <media/stagefright/MediaDefs.h>

namespace android {

const char *MEDIA_MIMETYPE_IMAGE_JPEG = "image/jpeg";

const char *MEDIA_MIMETYPE_VIDEO_VP8 = "video/x-vnd.on2.vp8";
const char *MEDIA_MIMETYPE_VIDEO_VP9 = "video/x-vnd.on2.vp9";
const char *MEDIA_MIMETYPE_VIDEO_AVC = "video/avc";
const char *MEDIA_MIMETYPE_VIDEO_MPEG4 = "video/mp4v-es";
const char *MEDIA_MIMETYPE_VIDEO_H263 = "video/3gpp";
const char *MEDIA_MIMETYPE_VIDEO_MPEG2 = "video/mpeg2";
const char *MEDIA_MIMETYPE_VIDEO_RAW = "video/raw";
// MStar Android Patch Begin
const char *MEDIA_MIMETYPE_VIDEO_RV  = "video/realvideo";
const char *MEDIA_MIMETYPE_VIDEO_RV10 = "video/realvideo10";
const char *MEDIA_MIMETYPE_VIDEO_RV20 = "video/realvideo20";
const char *MEDIA_MIMETYPE_VIDEO_FOURCC = "video/fourcc";
const char *MEDIA_MIMETYPE_VIDEO_DIVX4 = "video/divx4";
const char *MEDIA_MIMETYPE_VIDEO_DIVX = "video/divx";
const char *MEDIA_MIMETYPE_VIDEO_DIVX3 = "video/divx3";
const char *MEDIA_MIMETYPE_VIDEO_MP42 = "video/mp42";
const char *MEDIA_MIMETYPE_VIDEO_WMV1 = "video/wmv1";
const char *MEDIA_MIMETYPE_VIDEO_WMV2 = "video/wmv2";
const char *MEDIA_MIMETYPE_VIDEO_WMV3 = "video/wmv3";
const char *MEDIA_MIMETYPE_VIDEO_WVC1 = "video/wvc1";
const char *MEDIA_MIMETYPE_VIDEO_VC1 = "video/VC1";
const char *MEDIA_MIMETYPE_VIDEO_MJPG = "video/mjpeg";
const char *MEDIA_MIMETYPE_VIDEO_FLV = "video/flv";
const char *MEDIA_MIMETYPE_VIDEO_AVS = "video/avs";
const char *MEDIA_MIMETYPE_VIDEO_MVC = "video/mvc";
const char *MEDIA_MIMETYPE_VIDEO_HEVC = "video/hevc";
const char *MEDIA_MIMETYPE_VIDEO_BYPASS = "video/bypass";
const char *MEDIA_MIMETYPE_VIDEO_VP6 = "video/x-vnd.on2.vp6";
const char *MEDIA_MIMETYPE_VIDEO_VP7 = "video/x-vnd.on2.vp7";
const char *MEDIA_MIMETYPE_VIDEO_I420 = "video/i420";
const char *MEDIA_MIMETYPE_VIDEO_SVQ3 = "video/svq3";
const char *MEDIA_MIMETYPE_VIDEO_IV32 = "video/iv32";
const char *MEDIA_MIMETYPE_VIDEO_UNKNOW = "video/unknow";
// MStar Android Patch End

const char *MEDIA_MIMETYPE_AUDIO_AMR_NB = "audio/3gpp";
const char *MEDIA_MIMETYPE_AUDIO_AMR_WB = "audio/amr-wb";
const char *MEDIA_MIMETYPE_AUDIO_MPEG = "audio/mpeg";
const char *MEDIA_MIMETYPE_AUDIO_MPEG_LAYER_I = "audio/mpeg-L1";
const char *MEDIA_MIMETYPE_AUDIO_MPEG_LAYER_II = "audio/mpeg-L2";
const char *MEDIA_MIMETYPE_AUDIO_AAC = "audio/mp4a-latm";
const char *MEDIA_MIMETYPE_AUDIO_QCELP = "audio/qcelp";
const char *MEDIA_MIMETYPE_AUDIO_VORBIS = "audio/vorbis";
const char *MEDIA_MIMETYPE_AUDIO_G711_ALAW = "audio/g711-alaw";
const char *MEDIA_MIMETYPE_AUDIO_G711_MLAW = "audio/g711-mlaw";
const char *MEDIA_MIMETYPE_AUDIO_RAW = "audio/raw";
const char *MEDIA_MIMETYPE_AUDIO_FLAC = "audio/flac";
const char *MEDIA_MIMETYPE_AUDIO_AAC_ADTS = "audio/aac-adts";
const char *MEDIA_MIMETYPE_AUDIO_MSGSM = "audio/gsm";

// MStar Android Patch Begin
const char *MEDIA_MIMETYPE_AUDIO_COOK = "audio/cook";
const char *MEDIA_MIMETYPE_AUDIO_PCM = "audio/pcm";
const char *MEDIA_MIMETYPE_AUDIO_PCM_u8 = "audio/pcm_u8";
const char *MEDIA_MIMETYPE_AUDIO_PCM_alaw = "audio/pcm_alaw";
const char *MEDIA_MIMETYPE_AUDIO_PCM_ulaw = "audio/pcm_ulaw";
const char *MEDIA_MIMETYPE_AUDIO_ADPCM = "audio/adpcm";
const char *MEDIA_MIMETYPE_AUDIO_LPCM = "audio/lpcm";
const char *MEDIA_MIMETYPE_AUDIO_WMA = "audio/x-ms-wma";
const char *MEDIA_MIMETYPE_AUDIO_WMAPRO = "audio/wmapro";
const char *MEDIA_MIMETYPE_AUDIO_AC3 = "audio/ac3";
const char *MEDIA_MIMETYPE_AUDIO_AC3P = "audio/ac3p";
const char *MEDIA_MIMETYPE_AUDIO_DTS = "audio/dts";
const char *MEDIA_MIMETYPE_AUDIO_DTS_LBR = "audio/dts-lbr";
const char *MEDIA_MIMETYPE_AUDIO_DRA = "audio/dra";
const char *MEDIA_MIMETYPE_AUDIO_UNKNOW = "audio/unknow";
// MStar Android Patch End

const char *MEDIA_MIMETYPE_CONTAINER_MPEG4 = "video/mp4";
const char *MEDIA_MIMETYPE_CONTAINER_WAV = "audio/x-wav";
const char *MEDIA_MIMETYPE_CONTAINER_OGG = "application/ogg";
const char *MEDIA_MIMETYPE_CONTAINER_MATROSKA = "video/x-matroska";
const char *MEDIA_MIMETYPE_CONTAINER_MPEG2TS = "video/mp2ts";
const char *MEDIA_MIMETYPE_CONTAINER_AVI = "video/avi";
const char *MEDIA_MIMETYPE_CONTAINER_MPEG2PS = "video/mp2p";

const char *MEDIA_MIMETYPE_CONTAINER_WVM = "video/wvm";
// MStar Android Patch Begin
const char *MEDIA_MIMETYPE_CONTAINER_RM = "video/x-pn-realvideo";
const char *MEDIA_MIMETYPE_CONTAINER_FLV = "video/flv";
const char *MEDIA_MIMETYPE_CONTAINER_ASF = "video/x-ms-asf";
// MStar Android Patch End

const char *MEDIA_MIMETYPE_TEXT_3GPP = "text/3gpp-tt";
const char *MEDIA_MIMETYPE_TEXT_SUBRIP = "application/x-subrip";

}  // namespace android
