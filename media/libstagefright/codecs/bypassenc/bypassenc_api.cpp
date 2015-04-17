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

//#define LOG_NDEBUG 0
#define LOG_TAG "BYPASSENC_API"
#include <utils/Log.h>

#include "bypassenc_api.h"
#include "OMX_Video.h"

#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/MediaBufferGroup.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/Utils.h>

#include "AitUVC.h"

//#define MST_BYPASS_ENC_DUMP

static const uint8_t kNalUnitTypeIDR = 0x05;
static const uint8_t kNalUnitTypeSeqParamSet = 0x07;
static const uint8_t kNalUnitTypePicParamSet = 0x08;

static     void                *mXuHandle = NULL;
static int ConvertVendorNalTypeToBypassType(int vendor, int type)
{
    int frametype = BYPASSENC_FRAMETYPE_DUMMY;
    if(MSTR_BYPASS_VENDOR_AIT == vendor) {
        switch(type) {
            case AIT_H264_IFRAME:
                frametype = BYPASSENC_FRAMETYPE_I;
                break;
            case AIT_H264_PFRAME:
                frametype = BYPASSENC_FRAMETYPE_P;
                break;
        }
    }

    return frametype;
}

static int findNALStartCode(
        const uint8_t *data, size_t length, uint8_t start_code) {
    int bytesLeft = length;
    bool find = false;
    
    while (bytesLeft > 4  && !find) {
        if (!memcmp("\x00\x00\x00\x01", &data[length - bytesLeft], 4) && (start_code == (data[length - bytesLeft+4] & 0x1f))) {
            find = true;
        } else {
            --bytesLeft;
        }
    }
    ALOGV("findNextStartCode[%d]? %d, at %d", start_code, find, (length - bytesLeft));
    
    if (!find) {
        return -1;
    } else {
        return length - bytesLeft;
    }
}

OSCL_EXPORT_REF BypassEnc_Status BypassEncInitialize(MS_Bypass_FrameInfo_t *FrmInfo, BypassEncParams *encParam,
        void* extSPS, void* extPPS)
{
    uint8_t     vendor = MSTR_BYPASS_VENDOR_IN_START_CODE(FrmInfo->starcode);
    
    ALOGI("Bypass Vendor is: %d\n", vendor);
    if(MSTR_BYPASS_VENDOR_AIT == vendor) {
        int result = 0;
        result = SkypeXU_SetIFrame((SkypeXUHandle) FrmInfo->mXuHandle);
        ALOGI("SkypeXU_SetIFrame result: %x\n", result);
        
        result = SkypeXU_SetFrameRate((SkypeXUHandle) FrmInfo->mXuHandle, encParam->frame_rate);
        ALOGI("SkypeXU_SetFrameRate: %d, result: %x\n", encParam->frame_rate, result);
        
        result = SkypeXU_SetBitrate((SkypeXUHandle) FrmInfo->mXuHandle, encParam->bitrate/1000);
        ALOGI("SkypeXU_SetBitrate: %d, result: %x\n", encParam->bitrate/1000, result);

        result = AitXU_SetPFrameCount(((SkypeXUHandle) FrmInfo->mXuHandle)->aitxu, encParam->key_interval);
        ALOGI("AitXU_SetPFrameCount: %d, result: %x\n", encParam->key_interval, result);
        
        mXuHandle = FrmInfo->mXuHandle;
    } else {
        ALOGE("Fatal Error!! Unknown vendor %d\n", vendor);
    }
    return BYPASSENC_SUCCESS;
}

OSCL_EXPORT_REF BypassEnc_Status BypassEncDeInitialize()
{
    mXuHandle = 0;
    return BYPASSENC_SUCCESS;
}

OSCL_EXPORT_REF BypassEnc_Status BypassEncSettingsUpdate(unsigned int type, unsigned int value)
{
    ALOGI("BypassEncSettingsUpdate type: %d, value: %d", type, value);
    if(!mXuHandle) {
        ALOGE("mXuHandle is not assigned");
        return BYPASSENC_WRONG_STATE;
    }
    if(type & SETTING_UPDATE_INTRA) {
        SkypeXU_SetIFrame((SkypeXUHandle) mXuHandle);
    }    
    if(type & SETTING_UPDATE_BITRATE) {
        SkypeXU_SetBitrate((SkypeXUHandle) mXuHandle, value/1000);
    }
    if(type & SETTING_UPDATE_FRAMERATE) {
        //Q16 format
        SkypeXU_SetFrameRate((SkypeXUHandle) mXuHandle, value>>16);
    }
    return BYPASSENC_SUCCESS;
}

OSCL_EXPORT_REF BypassEnc_Status BypassEncConfigGet(unsigned char *stream, unsigned int stream_len, unsigned int *length)
{
    int offset = 0;
    
    offset = findNALStartCode(stream, stream_len, kNalUnitTypeIDR);
    if (offset < 0) {
        return BYPASSENC_FAIL;
    } else {
        *length = offset;
    }
    return BYPASSENC_SUCCESS;
}

OSCL_EXPORT_REF BypassEnc_Status BypassEncodeNAL(MS_Bypass_FrameInfo_t *FrmInfo, unsigned char *buffer, unsigned int *buf_nal_size, int *nal_type)
{
    BypassEnc_Status status = BYPASSENC_SUCCESS;
    int             streamType = BYPASSENC_FRAMETYPE_I;
    uint32_t    h264StreamSize = 0;
    uint8_t      *encodedBuffer;
    uint8_t     vendor = MSTR_BYPASS_VENDOR_IN_START_CODE(FrmInfo->starcode);
#if defined(MST_BYPASS_ENC_DUMP)    
    FILE *file = NULL;
    static bool start_saving = false;
#endif

    if(!memcmp(MSTR_BYPASS_ENC, &FrmInfo->starcode, sizeof(FrmInfo->starcode)-1)) {
        if(MSTR_BYPASS_VENDOR_AIT == vendor) {
            streamType = AitH264Demuxer_MJPG_H264((unsigned char*)FrmInfo->pBypassBuf, FrmInfo->nByPassSize, 
                buffer, *buf_nal_size, &h264StreamSize);
            
        } else {
            ALOGE("Fatal Error!! Unknown vendor %d\n", vendor);
            return BYPASSENC_FAIL;
        }
        encodedBuffer = buffer;
    } else {
        streamType = FrmInfo->nStreamType;
        encodedBuffer = FrmInfo->pBypassBuf;
        h264StreamSize = FrmInfo->nByPassSize;
   }

    streamType = ConvertVendorNalTypeToBypassType(vendor, streamType);
#if defined(MST_BYPASS_ENC_DUMP)                
    file = fopen("/var/tmp/media/local_dump.h264", "ab");
#endif    
    // start code
    if(BYPASSENC_FRAMETYPE_DUMMY == streamType) {
        ALOGW("waiting for incoming encoded camera video frames\n");
        status = BYPASSENC_NODATA;
    } else if (!memcmp("\x00\x00\x00\x01\x67", encodedBuffer, 5)) {
        streamType = BYPASSENC_FRAMETYPE_I;
    } else {
        streamType = BYPASSENC_FRAMETYPE_P;
    }

    *nal_type = streamType;
    *buf_nal_size = h264StreamSize;
#if defined(MST_BYPASS_ENC_DUMP)
    if (file != NULL) {
        if (BYPASSENC_FRAMETYPE_I == streamType)
            start_saving = true;
        if((BYPASSENC_FRAMETYPE_DUMMY != streamType) && (start_saving))
            fwrite((unsigned char *)encodedBuffer, h264StreamSize, 1, file);    // dump to file
        fclose(file);
    }
#endif          
    
    return status;
}
