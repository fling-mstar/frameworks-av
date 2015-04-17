/*
 * Copyright 2012, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "RemoteDisplay.h"

#include "source/WifiDisplaySource.h"

#include <media/IRemoteDisplayClient.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/foundation/ANetworkSession.h>
// MStar Android Patch Begin
#include "sink/WifiDisplaySink.h"
// MStar Android Patch End

namespace android {
// MStar Android Patch Begin
RemoteDisplay::RemoteDisplay(
        const sp<IRemoteDisplayClient> &client,
        const char *iface) {

    char *sourceinfo = NULL;
    char *sourceip = NULL;
    char *sourceport = NULL;
    char seps[] = ":";

    sourceinfo = strdup(iface);
    sourceip = strtok(sourceinfo, seps);
    sourceport = strtok(NULL, seps);
    if (sourceip == NULL || sourceport == NULL) {
        ALOGE("sourceip or sourceport should not be null");
        free(sourceinfo);
        return ;
    }

    ALOGI("sourceip %s   sourceport: %s", sourceip, sourceport);

    mLooper = new ALooper;
    mLooper->setName("wfd_looper");

    mNetSession = new ANetworkSession;
    mNetSession->start();

    mSink = new WifiDisplaySink(mNetSession);

    mLooper->registerHandler(mSink);

    mLooper->start();

    mSink->start(sourceip, atoi(sourceport));
    free(sourceinfo);
}
// MStar Android Patch End

RemoteDisplay::~RemoteDisplay() {
}

status_t RemoteDisplay::pause() {
    return mSource->pause();
}

status_t RemoteDisplay::resume() {
    return mSource->resume();
}

status_t RemoteDisplay::dispose() {
    // MStar Android Patch Begin
    //mSource->stop();
    //mSource.clear();
    // MStar Android Patch End

    mLooper->stop();
    mNetSession->stop();

    return OK;
}

}  // namespace android
