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

#ifndef MEDIA_ERRORS_H_

#define MEDIA_ERRORS_H_

#include <utils/Errors.h>

namespace android {

enum {
    MEDIA_ERROR_BASE        = -1000,

    ERROR_ALREADY_CONNECTED = MEDIA_ERROR_BASE,
    ERROR_NOT_CONNECTED     = MEDIA_ERROR_BASE - 1,
    ERROR_UNKNOWN_HOST      = MEDIA_ERROR_BASE - 2,
    ERROR_CANNOT_CONNECT    = MEDIA_ERROR_BASE - 3,
    ERROR_IO                = MEDIA_ERROR_BASE - 4,
    ERROR_CONNECTION_LOST   = MEDIA_ERROR_BASE - 5,
    ERROR_MALFORMED         = MEDIA_ERROR_BASE - 7,
    ERROR_OUT_OF_RANGE      = MEDIA_ERROR_BASE - 8,
    ERROR_BUFFER_TOO_SMALL  = MEDIA_ERROR_BASE - 9,
    ERROR_UNSUPPORTED       = MEDIA_ERROR_BASE - 10,
    ERROR_END_OF_STREAM     = MEDIA_ERROR_BASE - 11,

    // Not technically an error.
    INFO_FORMAT_CHANGED    = MEDIA_ERROR_BASE - 12,
    INFO_DISCONTINUITY     = MEDIA_ERROR_BASE - 13,
    INFO_OUTPUT_BUFFERS_CHANGED = MEDIA_ERROR_BASE - 14,

    // The following constant values should be in sync with
    // drm/drm_framework_common.h
    DRM_ERROR_BASE = -2000,

    ERROR_DRM_UNKNOWN                       = DRM_ERROR_BASE,
    ERROR_DRM_NO_LICENSE                    = DRM_ERROR_BASE - 1,
    ERROR_DRM_LICENSE_EXPIRED               = DRM_ERROR_BASE - 2,
    ERROR_DRM_SESSION_NOT_OPENED            = DRM_ERROR_BASE - 3,
    ERROR_DRM_DECRYPT_UNIT_NOT_INITIALIZED  = DRM_ERROR_BASE - 4,
    ERROR_DRM_DECRYPT                       = DRM_ERROR_BASE - 5,
    ERROR_DRM_CANNOT_HANDLE                 = DRM_ERROR_BASE - 6,
    ERROR_DRM_TAMPER_DETECTED               = DRM_ERROR_BASE - 7,
    ERROR_DRM_NOT_PROVISIONED               = DRM_ERROR_BASE - 8,
    ERROR_DRM_DEVICE_REVOKED                = DRM_ERROR_BASE - 9,
    ERROR_DRM_RESOURCE_BUSY                 = DRM_ERROR_BASE - 10,

    ERROR_DRM_VENDOR_MAX                    = DRM_ERROR_BASE - 500,
    ERROR_DRM_VENDOR_MIN                    = DRM_ERROR_BASE - 999,

    // Heartbeat Error Codes
    HEARTBEAT_ERROR_BASE = -3000,
    ERROR_HEARTBEAT_TERMINATE_REQUESTED                     = HEARTBEAT_ERROR_BASE,

    // MStar Android Patch Begin
    USER_ERROR_BASE = -5000,
    //Online play is connected to server timeout
    ERROR_CONNECTED_TIMEOUT                     = USER_ERROR_BASE,
    //The audio format is not supported
    ERROR_AUDIO_UNSUPPORT                       = USER_ERROR_BASE-1,
    //The video format is not supported
    ERROR_VIDEO_UNSUPPORT                       = USER_ERROR_BASE-2,
    //The file format is not supported
    ERROR_FILE_FORMAT_UNSUPPORT                 = USER_ERROR_BASE-3,
    //Out of memory
    ERROR_OUT_OF_MEMORY                         = USER_ERROR_BASE-4,
    //The video continuous skip frames too much
    ERROR_VIDEO_SKIP_TOO_MUCH                   = USER_ERROR_BASE-5,
    //The video resource had been stolen
    ERROR_VIDEO_RESOURCE_LOST                   = USER_ERROR_BASE-6,
    //DivX DRM video rental expire
    ERROR_DIVX_DRM_RENTAL_EXPIRE                = USER_ERROR_BASE-7,
    //DivX DRM authentication fail
    ERROR_DIVX_DRM_AUTH_FAIL                    = USER_ERROR_BASE-8,
    //The server has responded, but download data fail
    ERROR_DOWNLOAD_FAIL                         = USER_ERROR_BASE-9,
    //Online playlist parsing failed
    ERROR_PLAYLIST_PARSE_FAIL                   = USER_ERROR_BASE-10,
    //Time out
    ERROR_TIMEOUT                               = USER_ERROR_BASE-11,
    // MStar Android Patch End
};

}  // namespace android

#endif  // MEDIA_ERRORS_H_
