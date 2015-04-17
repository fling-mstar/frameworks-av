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

#ifndef BYPASSENC_API_H_

#define BYPASSENC_API_H_
#include <ByPassEncoder.h>

#if 0
#define     MSTR_BYPASS_ENC_DEMUX   "\x47\x53\x54\x52\x00\x00\x01\x67"      // stream is demuxed
#define     MSTR_BYPASS_ENC                "\x47\x53\x54\x52\x00\x00\x01\x68"       // stream is not demuxed
typedef struct _MS_Bypass_FrameInfo
{
    unsigned char starcode[8];
    unsigned char  *pBypassBuf;
    unsigned int    nByPassSize;
    int                 nStreamType;
    void                *mXuHandle;
} MS_Bypass_FrameInfo_t;
#endif

/**
This structure contains the encoding parameters.
*/
#define SETTING_UPDATE_INTRA    (1<<0)
#define SETTING_UPDATE_BITRATE  (1<<1)
#define SETTING_UPDATE_FRAMERATE  (1<<2)
typedef struct tagBypassEncParam
{
    int width;      /* width of an input frame in pixel */
    int height;     /* height of an input frame in pixel */
    int32_t bitrate;    /* target encoding bit rate in bits/second */
    int32_t frame_rate;  /* frame rate in the unit of frames per 1000 second */
    int32_t key_interval;
} BypassEncParams;

/**
 This enumeration is used for the status returned from the library interface.
*/
typedef enum
{
    BYPASSENC_SUCCESS = 0,
    BYPASSENC_FAIL = 1,
    BYPASSENC_NODATA = 2,
    
    BYPASSENC_WRONG_STATE = -5,
} BypassEnc_Status;

typedef enum
{
    BYPASSENC_FRAMETYPE_I = 1, 
    BYPASSENC_FRAMETYPE_P = 2,
    BYPASSENC_FRAMETYPE_SPS = 4, 

    BYPASSENC_FRAMETYPE_DUMMY = 13   /* Not a AVC NAL */
} BypassFrameType;

#define OSCL_EXPORT_REF __attribute__((visibility("default")))

OSCL_EXPORT_REF BypassEnc_Status BypassEncInitialize(MS_Bypass_FrameInfo_t *FrmInfo, BypassEncParams *encParam,
        void* extSPS, void* extPPS);

OSCL_EXPORT_REF BypassEnc_Status BypassEncDeInitialize();

OSCL_EXPORT_REF BypassEnc_Status BypassEncSettingsUpdate(unsigned int type, unsigned int value);

OSCL_EXPORT_REF BypassEnc_Status BypassEncConfigGet(unsigned char *stream, unsigned int stream_len, unsigned int *length);

OSCL_EXPORT_REF BypassEnc_Status BypassEncodeNAL(MS_Bypass_FrameInfo_t *FrmInfo, unsigned char *buffer, unsigned int *buf_nal_size, int *nal_type);
#endif  // BYPASSENC_API_H_
