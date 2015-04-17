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
#define LOG_TAG "AC3Extractor"
#include <utils/Log.h>

#include "include/AC3Extractor.h"

#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/DataSource.h>
#include <media/stagefright/MediaBuffer.h>
#include <media/stagefright/MediaBufferGroup.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MediaSource.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/Utils.h>
#include <utils/String8.h>

namespace android {

static const uint32_t u32Ac3BitrateTable[38] =
{
    32000,32000,40000,40000,48000,48000,56000,56000, 64000, 64000,
    80000,80000,96000,96000,112000,112000,128000,128000, 160000, 160000,
    192000,192000,224000,224000,256000,256000,320000,320000, 384000, 384000,
    448000,448000,512000,512000,576000,576000,640000,640000
};
static const uint32_t u32Ac3FramesizeTable[3][38] =
{
    // Unit: Words(16bits)/syncframe
     {
         64,64,80,80,96,96,112,112, 128, 128,                //48K
         160,160,192,192,224,224,256,256, 320, 320,
         384,384,448,448,512,512,640,640, 768, 768,
         896,896,1024,1024,1152,1152,1280,1280},
     {
         69,70,87,88,104,105,121,122, 139, 140,              //44.1K
         174,175,208,209,243,244,278,279, 348, 349,
         417,418,487,488,557,558,696,697, 835, 836,
         975,976,1114,1115,1253,1254,1393,1394},
     {
          96,96,120,120,144,144, 168, 168, 192, 192,          //32K
          240,240,288,288,336,336, 384, 384, 480,480,
          576,576,672,672,768,768, 960, 960, 1152,1152,
          1344,1344,1536,1536,1728,1728, 1920, 1920}
};
static const uint32_t u32Ac3SampleRateTable[4] = 
{
    48000, 44100, 32000,48000
};
static const uint32_t u32ChannelTable[8] = 
{
    2,1,2,
    3,3,4,
    4,5
};
static const uint32_t u32NumBlkTable[4]=
{
    1,2,3,6
};
#define AC3_SYNC_WORD 0x0B77
#define AC3_SYNC_WORD_BE 0x770B

#define MAX_AC3_FRAME 4096

static bool Rsyn(const sp<DataSource> &spSource,
	off64_t off64Offsize,
    int32_t* s32BitRate,
    int32_t* s32SampleRate,
    int32_t* s32ChannelCount,
    int32_t* s32FrameSize)
{
    
    uint8_t   au8SiAndBsi[10];
    uint8_t   u8Fscod;
    uint8_t   u8Acmod;
    uint8_t   u8tmp;
    uint32_t  u32count;
    uint8_t   u8FrmsizCod;
    uint16_t  u16frmsz;
    uint8_t   u8lft_on;
    uint8_t   u8blk;
    
    if(spSource->readAt(off64Offsize, au8SiAndBsi, sizeof(au8SiAndBsi))
        < (ssize_t)sizeof(au8SiAndBsi))
    {
        return false;
    }
    
    uint16_t u16Syncword = U16_AT((const uint8_t *)au8SiAndBsi);
    if( u16Syncword == AC3_SYNC_WORD_BE )
    {
        //ALOGE("AC3_SYNC_WORD_LE detected..");
        //byte-swap
        for( u32count = 0 ; u32count < sizeof(au8SiAndBsi) ; u32count += 2 )
        {
            u8tmp = au8SiAndBsi[u32count];
            au8SiAndBsi[u32count] = au8SiAndBsi[u32count+1];
            au8SiAndBsi[u32count+1] = u8tmp;
        }
        u16Syncword = AC3_SYNC_WORD;
    }

    if( u16Syncword == AC3_SYNC_WORD)
    {
        if( 0x10 != au8SiAndBsi[5]>>3 ) //ac3
        {
            u8FrmsizCod = ( au8SiAndBsi[4] & 0x3F);
            if( u8FrmsizCod > 37)
            {
                return false;
            }
            
            u8Fscod = au8SiAndBsi[4]>>6;
            if( u8Fscod == 0x11 )
            {
                return false;
            }
            
            u8Acmod = ( au8SiAndBsi[6] & 0xE0)>>5;

            *s32BitRate      = u32Ac3BitrateTable[u8FrmsizCod];
            *s32FrameSize    = u32Ac3FramesizeTable[u8Fscod][u8FrmsizCod]*2;
            *s32SampleRate   = u32Ac3SampleRateTable[u8Fscod];
            *s32ChannelCount = u32ChannelTable[u8Acmod] ;
        }
        else //ac3+
        {
            u16frmsz = U16_AT((const uint8_t *)&au8SiAndBsi[2]);
            *s32FrameSize = ((u16frmsz & 0x03FF)+1)*2; //in WORD, so we have to transfer the value to bytes..
            u8Fscod = au8SiAndBsi[4]>>6;
            if(u8Fscod == 0x3)
            {
                uint8_t u8fscod2 = (au8SiAndBsi[4] & 0x3f)>>4;
                if(u8fscod2 == 0x3)
                {
                    ALOGE("Ac3 find sampleRate error");
                    return false;
                }
                *s32SampleRate = u32Ac3SampleRateTable[u8fscod2] / 2;
                u8blk = u32NumBlkTable[0x3];
            }
            else
            {
                *s32SampleRate = u32Ac3SampleRateTable[u8Fscod];
                u8blk = u32NumBlkTable[(au8SiAndBsi[4] & 0x3f)>>4];
            }
            
            u8Acmod = ( au8SiAndBsi[4] & 0x0F)>>1;
            u8lft_on   = au8SiAndBsi[4] & 0x01;
            *s32ChannelCount = u32ChannelTable[u8Acmod] + u8lft_on;
            *s32BitRate = 8*(*s32FrameSize)*(*s32SampleRate)/u8blk/256;
        }

        ALOGV("sampleRate:%d,channelCount:%d,s32FrameSize:0x%x,bitrate:%d"
            ,*s32SampleRate,*s32ChannelCount,*s32FrameSize,*s32BitRate);
    }
    else
    {
        ALOGV("ac3 resync failed.");
        return false;
    }
    //find the next fram's sync word
    if(spSource->readAt(off64Offsize+*s32FrameSize, au8SiAndBsi, sizeof(au8SiAndBsi))
        < (ssize_t)sizeof(au8SiAndBsi))
    {
        return false;
    }
    u16Syncword = U16_AT((const uint8_t *)au8SiAndBsi);
    if((u16Syncword != AC3_SYNC_WORD )&&
        (u16Syncword != AC3_SYNC_WORD_BE))
    {
        return false;
    }
    return true;
}

class AC3Source : public MediaSource {
public:
    AC3Source(
            const sp<MetaData> &spMeta, const sp<DataSource> &spSource,
            int32_t s32FrameSize,off64_t off64FirstFramePos);

    virtual status_t start(MetaData *params = NULL);
    virtual status_t stop();

    virtual sp<MetaData> getFormat();

    virtual status_t read(MediaBuffer **pbuffer, const ReadOptions *pOptions = NULL);

protected:
    virtual ~AC3Source();

private:
    sp<MetaData> m_spMeta;
    sp<DataSource> m_spDataSource;
    off64_t m_off64CurrentPos;
    int64_t m_s64CurrentTimeUs;
    int32_t m_s32FrameSize;
    bool m_bStarted;
    off64_t m_off64FirstFramePos;
    MediaBufferGroup *m_pGroup;

    AC3Source(const AC3Source &);
    AC3Source &operator=(const AC3Source &);
};

AC3Extractor::AC3Extractor(
        const sp<DataSource> &spSource, const sp<AMessage> &spMeta)
    : m_stInitCheck(NO_INIT),
      m_spDataSource(spSource)
{      
    int32_t s32BitRate;
    int32_t s32SampleRate;
    int32_t s32ChannelCount;
    bool bSuccess;

    int32_t s32MetaBitrate;
    int32_t s32MetaSampleRate;
    int32_t s32MetaChannelCount;
    int32_t s32MetaFrameSize;

    //ALOGD("AC3Extractor::AC3Extractor");
    if( spMeta != NULL 
            && spMeta->findInt32("bitrate", &s32MetaBitrate)
            && spMeta->findInt32("samplerate", &s32MetaSampleRate)
            && spMeta->findInt32("channelcount", &s32MetaChannelCount)
            && spMeta->findInt32("framesize",&s32MetaFrameSize))
    {    
        s32BitRate = s32MetaBitrate;
        s32SampleRate = s32MetaSampleRate;
        s32ChannelCount = s32MetaChannelCount;
        m_s32FrameSize = s32MetaFrameSize;
        bSuccess = true;
    }
    else
    {
        bSuccess = Rsyn(spSource,0,&s32BitRate,&s32SampleRate,&s32ChannelCount,&m_s32FrameSize);
    }
    
    if( !bSuccess )
    {
        ALOGE("Unable to resync. Signalling end of stream.");
        return;
    }
    //ALOGD("sampleRate:%d,channelCount:%d,s32FrameSize:%d,bitrate:%d"
            //,s32SampleRate,s32ChannelCount,m_s32FrameSize,s32BitRate);

    int64_t s64DurationUs;
    off64_t off64FileSize;
    if (m_spDataSource->getSize(&off64FileSize) == OK) 
    {
        if( 0 == s32BitRate)
        {
            ALOGE("The BitRate equals 0");
            return;
        }
        s64DurationUs = 8000000LL * off64FileSize/ s32BitRate;
    } 
    else 
    {
        s64DurationUs = -1;
    }
    
    m_spMeta = new MetaData;
    if (s64DurationUs >= 0)
    {
        m_spMeta->setInt64(kKeyDuration, s64DurationUs);
    }
    m_spMeta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_AC3);
    m_spMeta->setInt32(kKeyBitRate, s32BitRate);
    m_spMeta->setInt32(kKeySampleRate,s32SampleRate);
    m_spMeta->setInt32(kKeyChannelCount,s32ChannelCount);
    m_spMeta->setInt32(kKeyIsSyncFrame, 1);
    m_stInitCheck = OK;
    
}

size_t AC3Extractor::countTracks() 
{
    return m_stInitCheck != OK ? 0 : 1;
}

sp<MediaSource> AC3Extractor::getTrack(size_t u32Index) 
{
    if (m_stInitCheck != OK || u32Index != 0)
    {
        return NULL;
    }

    return new AC3Source(
            m_spMeta, m_spDataSource, m_s32FrameSize,0);
}

sp<MetaData> AC3Extractor::getTrackMetaData(size_t u32Index, uint32_t u32Flags) 
{
    if (m_stInitCheck != OK || u32Index != 0) 
    {
        return NULL;
    }

    return m_spMeta;
}

////////////////////////////////////////////////////////////////////////////////

AC3Source::AC3Source(
        const sp<MetaData> &spMeta, const sp<DataSource> &spSource,
        int32_t s32FrameSize,off64_t off64FirstFramePos)
    : m_spMeta(spMeta),
      m_spDataSource(spSource),
      m_off64CurrentPos(0),
      m_s64CurrentTimeUs(0),
      m_s32FrameSize(s32FrameSize),
      m_bStarted(false),
      m_off64FirstFramePos(off64FirstFramePos),
      m_pGroup(NULL)
{
}

AC3Source::~AC3Source() 
{
    if (m_bStarted) 
    {
        stop();
    }
}

status_t AC3Source::start(MetaData *) 
{
    if( true == m_bStarted )
    {
        return OK;
    }

    m_pGroup = new MediaBufferGroup;
    m_pGroup->add_buffer(new MediaBuffer(MAX_AC3_FRAME));

    m_off64CurrentPos = m_off64FirstFramePos;
    m_s64CurrentTimeUs = 0;

    m_bStarted = true;

    return OK;
}

status_t AC3Source::stop() 
{
    if( m_bStarted == false)
    {
        return OK;
    }

    delete m_pGroup;
    m_pGroup = NULL;

    m_bStarted = false;

    return OK;
}

sp<MetaData> AC3Source::getFormat() 
{
    return m_spMeta;
}

status_t AC3Source::read(
        MediaBuffer **pOut, const ReadOptions *pOptions) 
{
    *pOut = NULL;
    int32_t s32BitRate;
    int32_t s32FrameSize;
    int64_t s64SeekTimeUs;
    ReadOptions::SeekMode mode;
    MediaBuffer *pbuffer;

    if (pOptions && pOptions->getSeekTo(&s64SeekTimeUs, &mode)) 
    {
        if ( s64SeekTimeUs > 0) 
        {
            if( m_spMeta->findInt32(kKeyBitRate, &s32BitRate) )
            {
                ALOGE("meta not find bitrate");
                return ERROR_UNSUPPORTED;
            }
            if( 0 == m_s32FrameSize )
            {
                ALOGE("The FrameSize equals 0");
                return ERROR_UNSUPPORTED;
            }
            int64_t s64SeekFrame = s64SeekTimeUs*s32BitRate/m_s32FrameSize/8000000; 
            m_off64CurrentPos = s32BitRate*s64SeekTimeUs/8000000;

            uint8_t u8SyncHeader[MAX_AC3_FRAME];
            ssize_t s32Len = m_spDataSource->readAt(m_off64CurrentPos, u8SyncHeader, MAX_AC3_FRAME);
            if( -1 == s32Len)
            {
                ALOGE("Read Fail");
                return ERROR_IO;
            }
            //Find the nearest frame
            
            for(int u32Index = 0; u32Index < s32Len;u32Index++ )
            {
                uint16_t u16Syncword = U16_AT((const uint8_t *)(&u8SyncHeader[u32Index]));
                if( (u16Syncword == AC3_SYNC_WORD) || (u16Syncword == AC3_SYNC_WORD_BE) )
                {
                    m_off64CurrentPos += u32Index;
                    break;
                }
            }

            ALOGV("the s64SeekFrame is %lld m_off64CurrentPos %lld",s64SeekFrame,m_off64CurrentPos);
        }
        else
        {        
            s64SeekTimeUs = 0;
            m_s64CurrentTimeUs = 0;
            m_off64CurrentPos = 0;
        }
    }
    ALOGV("The s64SeekTimeUs is%lld ,CurrentTimeUs is %lld,CurrentPos is %lld.",
        s64SeekTimeUs,m_s64CurrentTimeUs,m_off64CurrentPos);

    status_t StErr = m_pGroup->acquire_buffer(&pbuffer);
    if (StErr != OK) 
    {
        return StErr;
    }
    
    int32_t s32SampleRate;
    int32_t s32ChannelCount;
    
    if( !Rsyn(m_spDataSource,m_off64CurrentPos,&s32BitRate,&s32SampleRate,&s32ChannelCount,&s32FrameSize) )
    {
        pbuffer->release();
        pbuffer = NULL;
        //ALOGD("Rsyn Fail: m_off64CurrentPos %lld s32FrameSize %d",m_off64CurrentPos,s32FrameSize);
        return ERROR_END_OF_STREAM;
    }

    if( 0 == s32BitRate)
    {
        ALOGD("The bitRate equals 0");
        pbuffer->release();
        pbuffer = NULL;
        return ERROR_MALFORMED;
    }
    if( m_spDataSource->readAt(m_off64CurrentPos, pbuffer->data(),s32FrameSize) != s32FrameSize )
    {
        return ERROR_END_OF_STREAM;
    }
    
    m_s64CurrentTimeUs=8000000LL*m_off64CurrentPos/s32BitRate;
    pbuffer->set_range(0, s32FrameSize);
    pbuffer->meta_data()->setInt64(kKeyTime, m_s64CurrentTimeUs);
    pbuffer->meta_data()->setInt32(kKeyIsSyncFrame, 1);
    *pOut = pbuffer;

    m_off64CurrentPos += s32FrameSize;
    
    return OK;
}

sp<MetaData> AC3Extractor::getMetaData() 
{
    sp<MetaData> spMeta = new MetaData;

    if (m_stInitCheck == OK) 
    {
        spMeta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_AC3);
    }
    return spMeta;
}

bool SniffAC3(
        const sp<DataSource> &spSource, String8 *pMimeType,
        float *fConfidence, sp<AMessage> *spMeta) 
{        
        int32_t s32BitRate;
        int32_t s32SampleRate;
        int32_t s32ChannelCount;
        int32_t s32FrameSize;
      
        if( !Rsyn(spSource,0,&s32BitRate,&s32SampleRate,&s32ChannelCount,&s32FrameSize) )
        {
            ALOGV("Rsyn Fail");
            return false;
        }
        
        *spMeta = new AMessage;
            
        (*spMeta)->setInt32("bitrate", s32BitRate);
        (*spMeta)->setInt32("samplerate",s32SampleRate);
        (*spMeta)->setInt32("channelcount",s32ChannelCount);
        (*spMeta)->setInt32("framesize", s32FrameSize);
        //ALOGD("sampleRate:%d,channelCount:%d,s32FrameSize:0x%x,bitrate:%d"
        //    ,s32SampleRate,s32ChannelCount,s32FrameSize,s32BitRate); 
        *pMimeType = MEDIA_MIMETYPE_AUDIO_AC3;
        *fConfidence = 0.3f;
        return true;
}


}  // namespace android
