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

//#define LOG_NDEBUG 0
#define LOG_TAG "WifiDisplaySink"
#include <utils/Log.h>

#include "WifiDisplaySink.h"
#include "RTPSink.h"

#include <media/stagefright/foundation/ParsedMessage.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/MediaErrors.h>
// MStar Android Patch Begin
#ifdef BUILD_MSTARTV
#include <hdcp22_app.h>
#include <hdcpCallback.h>
#include <mapi_hdcp2.h>

#ifdef BUILD_GOOGLETV
#define HDCP2_KEY_PATH "/certificate/hdcp2_x/HDCP2.bin"
#else
#define HDCP2_KEY_PATH "/data/hdcp2_key.bin"
#endif
#else
#define HDCP2_KEY_PATH ""
#endif

#define WFD_VERSION "1.1"

#define MAX_RETRY_TIMES 6

// MStar Android Patch End

namespace android {
// MStar Android Patch Begin
#ifdef BUILD_MSTARTV
mapi_hdcp2 *pmapi_hdcp;
extern "C" void  setAESKey(void *param);
extern "C" void  setRIVKey(void *param);
extern "C" void  getSecureStKey(void *param);
void setAESKey(void *param){
    int i;
    unsigned char *key = (unsigned char*)param;
    ALOGD("AESKey:");
    for(i=0; i<16; i++)
        ALOGD("0x%x", key[i]);
   pmapi_hdcp->mapi_HDCP2_SetKey(key);

}

void  setRIVKey(void *param){
    int i;
    unsigned char *key = (unsigned char *)param;
    ALOGD("RIVKey:");
    for(i=0; i<8; i++)
        ALOGD("0x%x", key[i]);
    pmapi_hdcp->mapi_HDCP2_SetRiv(key);

   }

void getSecreStKey(void *param){
    unsigned int key_len = sizeof(ST_HdcpKeyStruct);
    unsigned char *key = (unsigned char *)param;
     pmapi_hdcp->GetDecryptedHdcp2Key(key, key_len);
}

struct WifiDisplaySink::HdcpThread : public Thread {
    HdcpThread(WifiDisplaySink *session);

protected:
    virtual ~HdcpThread();

private:
    WifiDisplaySink *mSession;
    COMMON_DATA  *hdcp2_config;

    virtual bool threadLoop();

    DISALLOW_EVIL_CONSTRUCTORS(HdcpThread);
};

WifiDisplaySink::HdcpThread::HdcpThread(WifiDisplaySink *session)
    : mSession(session) {
    hdcp2_config = (COMMON_DATA *) calloc(1, sizeof(COMMON_DATA));
    if (hdcp2_config == NULL) {
        ALOGD("create hdcp2_config fail");
    }
}

WifiDisplaySink::HdcpThread::~HdcpThread() {
    ALOGD("entring %s", __func__);
    if (hdcp2_config != NULL) {
        Terminate_HDCP(hdcp2_config);
        free(hdcp2_config);
        hdcp2_config = NULL;
    }
}

bool WifiDisplaySink::HdcpThread::threadLoop() {
    ALOGI( "entering %s", __func__ );
    hdcp_set_reasonable_defaults(hdcp2_config);
    hdcp2_config->usHdcpPort= 10100;//3066;

    hdcp_rx_main(hdcp2_config);

    ALOGI( "exiting %s (exiting thread)", __func__ );
    return false;
}
#endif
// MStar Android Patch End

WifiDisplaySink::WifiDisplaySink(
        const sp<ANetworkSession> &netSession,
        const sp<IGraphicBufferProducer> &surfaceTex)
    : mState(UNDEFINED),
      mNetSession(netSession),
      mSurfaceTex(surfaceTex),
      // MStar Android Patch Begin
      mRTPSink(NULL),
      // MStar Android Patch End
      mSessionID(0),
      mNextCSeq(1),
      mRetryTimes(MAX_RETRY_TIMES) {
      ALOGD("WFD version: %s", WFD_VERSION);
}

WifiDisplaySink::~WifiDisplaySink() {
    // MStar Android Patch Begin
#ifdef BUILD_MSTARTV
    pmapi_hdcp->DestroyInstance();
#endif
    // MStar Android Patch End
}

void WifiDisplaySink::start(const char *sourceHost, int32_t sourcePort) {
    sp<AMessage> msg = new AMessage(kWhatStart, id());
    msg->setString("sourceHost", sourceHost);
    msg->setInt32("sourcePort", sourcePort);
    msg->post();
    // MStar Android Patch Begin
#ifdef BUILD_MSTARTV
    HDCP_registerEventCB(HDCP_EVENT_SET_AES128_KEY, setAESKey);
    HDCP_registerEventCB(HDCP_EVENT_SET_AES128_RIVKEY, setRIVKey);
    HDCP_registerEventCB(HDCP_EVENT_GET_HDCP_KEY, getSecreStKey);
    pmapi_hdcp = mapi_hdcp2::GetInstance();
    ALOGD("pmapi_hdcp: %p", pmapi_hdcp);
#endif
    // MStar Android Patch End
}

void WifiDisplaySink::start(const char *uri) {
    sp<AMessage> msg = new AMessage(kWhatStart, id());
    msg->setString("setupURI", uri);
    msg->post();
}

// static
bool WifiDisplaySink::ParseURL(
        const char *url, AString *host, int32_t *port, AString *path,
        AString *user, AString *pass) {
    host->clear();
    *port = 0;
    path->clear();
    user->clear();
    pass->clear();

    if (strncasecmp("rtsp://", url, 7)) {
        return false;
    }

    const char *slashPos = strchr(&url[7], '/');

    if (slashPos == NULL) {
        host->setTo(&url[7]);
        path->setTo("/");
    } else {
        host->setTo(&url[7], slashPos - &url[7]);
        path->setTo(slashPos);
    }

    ssize_t atPos = host->find("@");

    if (atPos >= 0) {
        // Split of user:pass@ from hostname.

        AString userPass(*host, 0, atPos);
        host->erase(0, atPos + 1);

        ssize_t colonPos = userPass.find(":");

        if (colonPos < 0) {
            *user = userPass;
        } else {
            user->setTo(userPass, 0, colonPos);
            pass->setTo(userPass, colonPos + 1, userPass.size() - colonPos - 1);
        }
    }

    const char *colonPos = strchr(host->c_str(), ':');

    if (colonPos != NULL) {
        char *end;
        unsigned long x = strtoul(colonPos + 1, &end, 10);

        if (end == colonPos + 1 || *end != '\0' || x >= 65536) {
            return false;
        }

        *port = x;

        size_t colonOffset = colonPos - host->c_str();
        size_t trailing = host->size() - colonOffset;
        host->erase(colonOffset, trailing);
    } else {
        *port = 554;
    }

    return true;
}

void WifiDisplaySink::onMessageReceived(const sp<AMessage> &msg) {
    switch (msg->what()) {
        case kWhatStart:
        {
            int32_t sourcePort;

            if (msg->findString("setupURI", &mSetupURI)) {
                AString path, user, pass;
                CHECK(ParseURL(
                            mSetupURI.c_str(),
                            &mRTSPHost, &sourcePort, &path, &user, &pass)
                        && user.empty() && pass.empty());
            } else {
                CHECK(msg->findString("sourceHost", &mRTSPHost));
                CHECK(msg->findInt32("sourcePort", &sourcePort));
            }

            mRTSPPort = sourcePort;
            sp<AMessage> notify = new AMessage(kWhatRTSPNotify, id());

            status_t err = mNetSession->createRTSPClient(
                    mRTSPHost.c_str(), sourcePort, notify, &mSessionID);
            CHECK_EQ(err, (status_t)OK);

            mState = CONNECTING;
            break;
        }

        case kWhatReconnect:
        {
            ALOGD("start to reconnect");
            sp<AMessage> notify = new AMessage(kWhatRTSPNotify, id());
            mNetSession->createRTSPClient(
                    mRTSPHost.c_str(), mRTSPPort, notify, &mSessionID);
            break;
        }

        case kWhatRTSPNotify:
        {
            int32_t reason;
            CHECK(msg->findInt32("reason", &reason));

            switch (reason) {
                case ANetworkSession::kWhatError:
                {
                    int32_t sessionID;
                    CHECK(msg->findInt32("sessionID", &sessionID));

                    int32_t err;
                    CHECK(msg->findInt32("err", &err));

                    AString detail;
                    CHECK(msg->findString("detail", &detail));

                    ALOGE("An error occurred in session %d (%d, '%s/%s').",
                          sessionID,
                          err,
                          detail.c_str(),
                          strerror(-err));

                    if (sessionID == mSessionID) {
                        // The control connection is dead now.
                        mNetSession->destroySession(mSessionID);
                        if (mRetryTimes > 0) {
                            mRetryTimes--;
                            ALOGD("reconnect left times: %d", mRetryTimes);
                            sp<AMessage> msg = new AMessage(kWhatReconnect, id());
                            msg->post((MAX_RETRY_TIMES-mRetryTimes)*1000*500);

                        } else {
                            ALOGI("Lost control connection.");
                            mSessionID = 0;
                            looper()->stop();
                        }
                    }
                    break;
                }

                case ANetworkSession::kWhatConnected:
                {
                    ALOGI("We're now connected.");
                    mState = CONNECTED;
                    mRetryTimes = 0;

                    if (!mSetupURI.empty()) {
                        status_t err =
                            sendDescribe(mSessionID, mSetupURI.c_str());

                        CHECK_EQ(err, (status_t)OK);
                    }
                    break;
                }

                case ANetworkSession::kWhatData:
                {
                    onReceiveClientData(msg);
                    break;
                }

                case ANetworkSession::kWhatBinaryData:
                {
                    CHECK(sUseTCPInterleaving);

                    int32_t channel;
                    CHECK(msg->findInt32("channel", &channel));

                    sp<ABuffer> data;
                    CHECK(msg->findBuffer("data", &data));

                    mRTPSink->injectPacket(channel == 0 /* isRTP */, data);
                    break;
                }

                default:
                    TRESPASS();
            }
            break;
        }

        case kWhatStop:
        {
            looper()->stop();
            break;
        }

        default:
            TRESPASS();
    }
}

void WifiDisplaySink::registerResponseHandler(
        int32_t sessionID, int32_t cseq, HandleRTSPResponseFunc func) {
    ResponseID id;
    id.mSessionID = sessionID;
    id.mCSeq = cseq;
    mResponseHandlers.add(id, func);
}

status_t WifiDisplaySink::sendM2(int32_t sessionID) {
    AString request = "OPTIONS * RTSP/1.0\r\n";
    AppendCommonResponse(&request, mNextCSeq);

    request.append(
            "Require: org.wfa.wfd1.0\r\n"
            "\r\n");

    status_t err =
        mNetSession->sendRequest(sessionID, request.c_str(), request.size());

    if (err != OK) {
        return err;
    }

    registerResponseHandler(
            sessionID, mNextCSeq, &WifiDisplaySink::onReceiveM2Response);

    ++mNextCSeq;

    return OK;
}

status_t WifiDisplaySink::onReceiveM2Response(
        int32_t sessionID, const sp<ParsedMessage> &msg) {
    int32_t statusCode;
    if (!msg->getStatusCode(&statusCode)) {
        return ERROR_MALFORMED;
    }

    if (statusCode != 200) {
        return ERROR_UNSUPPORTED;
    }

    return OK;
}

status_t WifiDisplaySink::onReceiveDescribeResponse(
        int32_t sessionID, const sp<ParsedMessage> &msg) {
    int32_t statusCode;
    if (!msg->getStatusCode(&statusCode)) {
        return ERROR_MALFORMED;
    }

    if (statusCode != 200) {
        return ERROR_UNSUPPORTED;
    }

    return sendSetup(sessionID, mSetupURI.c_str());
}

status_t WifiDisplaySink::onReceiveSetupResponse(
        int32_t sessionID, const sp<ParsedMessage> &msg) {
    int32_t statusCode;
    if (!msg->getStatusCode(&statusCode)) {
        return ERROR_MALFORMED;
    }

    if (statusCode != 200) {
        return ERROR_UNSUPPORTED;
    }

    if (!msg->findString("session", &mPlaybackSessionID)) {
        return ERROR_MALFORMED;
    }

    if (!ParsedMessage::GetInt32Attribute(
                mPlaybackSessionID.c_str(),
                "timeout",
                &mPlaybackSessionTimeoutSecs)) {
        mPlaybackSessionTimeoutSecs = -1;
    }

    ssize_t colonPos = mPlaybackSessionID.find(";");
    if (colonPos >= 0) {
        // Strip any options from the returned session id.
        mPlaybackSessionID.erase(
                colonPos, mPlaybackSessionID.size() - colonPos);
    }

    status_t err = configureTransport(msg);

    if (err != OK) {
        return err;
    }

    mState = PAUSED;

    return sendPlay(
            sessionID,
            !mSetupURI.empty()
                ? mSetupURI.c_str() : "rtsp://x.x.x.x:x/wfd1.0/streamid=0");
}

status_t WifiDisplaySink::configureTransport(const sp<ParsedMessage> &msg) {
    // MStar Android Patch Begin
    bool google_rtsp = true;
    bool realtek_rtsp = true;
    // MStar Android Patch End
    if (sUseTCPInterleaving) {
        return OK;
    }

    AString transport;
    if (!msg->findString("transport", &transport)) {
        ALOGE("Missing 'transport' field in SETUP response.");
        return ERROR_MALFORMED;
    }

    AString sourceHost;
    if (!ParsedMessage::GetAttribute(
                transport.c_str(), "source", &sourceHost)) {
        sourceHost = mRTSPHost;
    }
    // MStar Android Patch Begin
    AString serverPortStr;
    if (!ParsedMessage::GetAttribute(
                transport.c_str(), "server_port", &serverPortStr)) {
        ALOGE("Missing 'server_port' in Transport field.");
        return OK;//enhance compatibility

    }

    int rtpPort, rtcpPort;
    if (sscanf(serverPortStr.c_str(), "%d-%d", &rtpPort, &rtcpPort) != 2
            || rtpPort <= 0 || rtpPort > 65535
            || rtcpPort <=0 || rtcpPort > 65535
            || rtcpPort != rtpPort + 1) {

        google_rtsp = false;
    } else {
        google_rtsp = true;
        realtek_rtsp = false;
    }

    if (!google_rtsp) {
        if (sscanf(serverPortStr.c_str(), "%d", &rtpPort) != 1
            || rtpPort <= 0 || rtpPort > 65535){
            realtek_rtsp = false;
        } else{
            realtek_rtsp = true;
        }
    }

    if (google_rtsp == realtek_rtsp) {
        ALOGE("Invalid server_port description '%s'.",
                serverPortStr.c_str());

        return ERROR_MALFORMED;
    }

    if (rtpPort & 1) {
        ALOGW("Server picked an odd numbered RTP port.");
    }

    if (google_rtsp) {
        return mRTPSink->connect(sourceHost.c_str(), rtpPort, rtcpPort);
    } else if(realtek_rtsp) {
        return mRTPSink->connect(sourceHost.c_str(), rtpPort);
    }

    return ERROR_MALFORMED;
    // MStar Android Patch End
}

status_t WifiDisplaySink::onReceivePlayResponse(
        int32_t sessionID, const sp<ParsedMessage> &msg) {
    int32_t statusCode;
    if (!msg->getStatusCode(&statusCode)) {
        return ERROR_MALFORMED;
    }

    if (statusCode != 200) {
        return ERROR_UNSUPPORTED;
    }

    mState = PLAYING;

    return OK;
}

void WifiDisplaySink::onReceiveClientData(const sp<AMessage> &msg) {
    int32_t sessionID;
    CHECK(msg->findInt32("sessionID", &sessionID));

    sp<RefBase> obj;
    CHECK(msg->findObject("data", &obj));

    sp<ParsedMessage> data =
        static_cast<ParsedMessage *>(obj.get());
    // MStar Android Patch Begin
    ALOGD("session %d received '%s'",
            sessionID, data->debugString().c_str());
    // MStar Android Patch End
    AString method;
    AString uri;
    data->getRequestField(0, &method);

    int32_t cseq;
    if (!data->findInt32("cseq", &cseq)) {
        sendErrorResponse(sessionID, "400 Bad Request", -1 /* cseq */);
        return;
    }

    if (method.startsWith("RTSP/")) {
        // This is a response.

        ResponseID id;
        id.mSessionID = sessionID;
        id.mCSeq = cseq;

        ssize_t index = mResponseHandlers.indexOfKey(id);

        if (index < 0) {
            ALOGW("Received unsolicited server response, cseq %d", cseq);
            return;
        }

        HandleRTSPResponseFunc func = mResponseHandlers.valueAt(index);
        mResponseHandlers.removeItemsAt(index);

        status_t err = (this->*func)(sessionID, data);
        CHECK_EQ(err, (status_t)OK);
    } else {
        AString version;
        data->getRequestField(2, &version);
        if (!(version == AString("RTSP/1.0"))) {
            sendErrorResponse(sessionID, "505 RTSP Version not supported", cseq);
            return;
        }

        if (method == "OPTIONS") {
            onOptionsRequest(sessionID, cseq, data);
        } else if (method == "GET_PARAMETER") {
            onGetParameterRequest(sessionID, cseq, data);
        } else if (method == "SET_PARAMETER") {
            onSetParameterRequest(sessionID, cseq, data);
        } else {
            sendErrorResponse(sessionID, "405 Method Not Allowed", cseq);
        }
    }
}

void WifiDisplaySink::onOptionsRequest(
        int32_t sessionID,
        int32_t cseq,
        const sp<ParsedMessage> &data) {
    AString response = "RTSP/1.0 200 OK\r\n";
    AppendCommonResponse(&response, cseq);
    response.append("Public: org.wfa.wfd1.0, GET_PARAMETER, SET_PARAMETER\r\n");
    response.append("\r\n");

    status_t err = mNetSession->sendRequest(sessionID, response.c_str());
    CHECK_EQ(err, (status_t)OK);

    err = sendM2(sessionID);
    CHECK_EQ(err, (status_t)OK);
}

void WifiDisplaySink::onGetParameterRequest(
        int32_t sessionID,
        int32_t cseq,
        const sp<ParsedMessage> &data) {
    // MStar Android Patch Begin
    status_t err;
    AString strParameters;

    strParameters = data->getContent();
    if (mRTPSink == NULL) {
        mRTPSink = new RTPSink(mNetSession, mSurfaceTex);
        looper()->registerHandler(mRTPSink);

        err = mRTPSink->init(sUseTCPInterleaving);
        if (err != OK) {
            looper()->unregisterHandler(mRTPSink->id());
            mRTPSink.clear();
            CHECK_EQ(err, (status_t)OK);
        }
    }

    AString body = "wfd_video_formats: 30 00 01 08 0001ffff 00000000 00000000 00 0000 0000 00 none none\r\n";//1080x720 p30

    body.append("wfd_audio_codecs: LPCM 00000003 00\r\n");

    body.append("wfd_uibc_capability: none\r\n");

    if (access(HDCP2_KEY_PATH, F_OK) == 0 && strstr(strParameters.c_str(), "wfd_content_protection")) {
        body.append("wfd_content_protection: HDCP2.1 port=10100\r\n");
#ifdef BUILD_MSTARTV
        ALOGD("start hdcp thread");
        mThread = new HdcpThread(this);
        status_t err = mThread->run("HdcpSession");
        if (err != OK) {
            mThread.clear();
            ALOGI("start hdcp thread failed");
        }
#endif
    } else {
        body.append("wfd_content_protection: none\r\n");
    }
    //body.append("wfd_audio_codecs: AAC 0000000f 00\r\n");//lpcm do not support
    body.append(StringPrintf("wfd_client_rtp_ports: RTP/AVP/UDP;unicast %d 0 mode=play\r\n", mRTPSink->getRTPPort()));

    AString response = "RTSP/1.0 200 OK\r\n";
    AppendCommonResponse(&response, cseq);
    response.append("Content-Type: text/parameters\r\n");
    response.append(StringPrintf("Content-Length: %d\r\n", body.size()));
    response.append("\r\n");
    response.append(body);

    err = mNetSession->sendRequest(sessionID, response.c_str());
    // MStar Android Patch End
    CHECK_EQ(err, (status_t)OK);
}

status_t WifiDisplaySink::sendDescribe(int32_t sessionID, const char *uri) {
    uri = "rtsp://xwgntvx.is.livestream-api.com/livestreamiphone/wgntv";
    uri = "rtsp://v2.cache6.c.youtube.com/video.3gp?cid=e101d4bf280055f9&fmt=18";

    AString request = StringPrintf("DESCRIBE %s RTSP/1.0\r\n", uri);
    AppendCommonResponse(&request, mNextCSeq);

    request.append("Accept: application/sdp\r\n");
    request.append("\r\n");

    status_t err = mNetSession->sendRequest(
            sessionID, request.c_str(), request.size());

    if (err != OK) {
        return err;
    }

    registerResponseHandler(
            sessionID, mNextCSeq, &WifiDisplaySink::onReceiveDescribeResponse);

    ++mNextCSeq;

    return OK;
}

status_t WifiDisplaySink::sendSetup(int32_t sessionID, const char *uri) {
    // MStar Android Patch Begin
#if 0
    mRTPSink = new RTPSink(mNetSession, mSurfaceTex);
    looper()->registerHandler(mRTPSink);

    status_t err = mRTPSink->init(sUseTCPInterleaving);

    if (err != OK) {
        looper()->unregisterHandler(mRTPSink->id());
        mRTPSink.clear();
        return err;
    }
#endif
    // MStar Android Patch End

    AString request = StringPrintf("SETUP %s RTSP/1.0\r\n", uri);

    AppendCommonResponse(&request, mNextCSeq);

    if (sUseTCPInterleaving) {
        request.append("Transport: RTP/AVP/TCP;interleaved=0-1\r\n");
    } else {
        int32_t rtpPort = mRTPSink->getRTPPort();

        request.append(
                StringPrintf(
                    "Transport: RTP/AVP/UDP;unicast;client_port=%d-%d\r\n",
                    rtpPort, rtpPort + 1));
    }

    request.append("\r\n");

    ALOGV("request = '%s'", request.c_str());
    // MStar Android Patch Begin
    //err = mNetSession->sendRequest(sessionID, request.c_str(), request.size());
    status_t err = mNetSession->sendRequest(sessionID, request.c_str(), request.size());
    // MStar Android Patch End

    if (err != OK) {
        return err;
    }

    registerResponseHandler(
            sessionID, mNextCSeq, &WifiDisplaySink::onReceiveSetupResponse);

    ++mNextCSeq;

    return OK;
}

status_t WifiDisplaySink::sendPlay(int32_t sessionID, const char *uri) {
    AString request = StringPrintf("PLAY %s RTSP/1.0\r\n", uri);

    AppendCommonResponse(&request, mNextCSeq);

    request.append(StringPrintf("Session: %s\r\n", mPlaybackSessionID.c_str()));
    request.append("\r\n");

    status_t err =
        mNetSession->sendRequest(sessionID, request.c_str(), request.size());

    if (err != OK) {
        return err;
    }

    registerResponseHandler(
            sessionID, mNextCSeq, &WifiDisplaySink::onReceivePlayResponse);

    ++mNextCSeq;

    return OK;
}

void WifiDisplaySink::onSetParameterRequest(
        int32_t sessionID,
        int32_t cseq,
        const sp<ParsedMessage> &data) {
    // MStar Android Patch Begin
    const char *content = data->getContent();
    AString response = "RTSP/1.0 200 OK\r\n";
    AppendCommonResponse(&response, cseq);
    response.append("\r\n");

    status_t err = mNetSession->sendRequest(sessionID, response.c_str());
    CHECK_EQ(err, (status_t)OK);

    usleep(1*1000*1000);//1s

    if (strstr(content, "wfd_trigger_method: SETUP\r\n") != NULL) {
        status_t err =
            sendSetup(
                    sessionID,
                    "rtsp://x.x.x.x:x/wfd1.0/streamid=0");

        CHECK_EQ(err, (status_t)OK);
    } else if (strstr(content, "wfd_trigger_method: TEARDOWN\r\n") != NULL) {
        ALOGI("receive TEARDOWN");

        if (sessionID == mSessionID) {
            ALOGI("clear mRTPSink");
            // The control connection is dead now.
            mNetSession->destroySession(mSessionID);
            mSessionID = 0;

            looper()->unregisterHandler(mRTPSink->id());
            mRTPSink.clear();
            looper()->stop();
        }
    }
    // MStar Android Patch End
}

void WifiDisplaySink::sendErrorResponse(
        int32_t sessionID,
        const char *errorDetail,
        int32_t cseq) {
    AString response;
    response.append("RTSP/1.0 ");
    response.append(errorDetail);
    response.append("\r\n");

    AppendCommonResponse(&response, cseq);

    response.append("\r\n");

    status_t err = mNetSession->sendRequest(sessionID, response.c_str());
    CHECK_EQ(err, (status_t)OK);
}

// static
void WifiDisplaySink::AppendCommonResponse(AString *response, int32_t cseq) {
    time_t now = time(NULL);
    struct tm *now2 = gmtime(&now);
    char buf[128];
    strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S %z", now2);

    response->append("Date: ");
    response->append(buf);
    response->append("\r\n");

    response->append("User-Agent: stagefright/1.1 (Linux;Android 4.1)\r\n");

    if (cseq >= 0) {
        response->append(StringPrintf("CSeq: %d\r\n", cseq));
    }
}

}  // namespace android
