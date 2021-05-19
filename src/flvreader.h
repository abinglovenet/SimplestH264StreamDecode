#ifndef FLVREADER_H
#define FLVREADER_H

#include <QDebug>
#include <QString>
#include <QByteArrayList>
#include <QFile>
#include <QtEndian>
#include <QThread>
#include <QMutexLocker>

#include "mfxvideo++.h"
#include "common_utils.h"
#include "imediareader.h"

enum FLV_TAG_TYPES
{
    FLV_TAG_VIDEO = 0x09,
    FLV_TAG_AUDIO = 0x08,
    FLV_TAG_SCRIPT = 0x12

};

#pragma pack(push, 1)
typedef struct
{
    quint8 f;
    quint8 l;
    quint8 v;

    quint8 version;

    quint8 reserved1 : 5;
    quint8 audio : 1;
    quint8 reserved2 : 1;
    quint8 video : 1;

    quint32 dataoffset;

}FLV_HEADER, *PFLV_HEADER;

typedef struct
{
    quint32 type : 8;
    quint32 size : 24;

    quint32 timestamp : 24;
    quint32 exstamp : 8;

    quint8  streamid[3];


}FLV_TAG_HEADER, *PFLV_TAG_HEADER;

typedef struct
{
    quint8 soundtype:1;
    quint8 soundsize:1;
    quint8 soundrate:2;
    quint8 soundformat:4;



}AUDIO_TAG_PARAM, *PAUDIO_TAG_PARAM;

// detail format AAC...





typedef struct
{
    quint8 codec_id : 4;
    quint8 frame_type : 4;
}VIDEO_TAG_PARAM, *PVIDEO_TAG_PARAM;

// DETAIL_FORMAT -H264
typedef struct{
    quint32 packet_type : 8;
    quint32 composition_time : 24;
}AVC_VIDEO_PACKET_HEADER, *PAVC_VIDEO_PACKET_HEADER;

// DETAIL_FORMAT -H264:CONFIG
typedef struct
{
    quint8 version;
    quint8 profile;
    quint8 profile_compatibility;
    quint8 level;
    quint8 size_min;
}AVC_VIDEO_CONFIG_HEADER, *PAVC_VIDEO_CONFIG_HEADER;

// DETAIL_FORMAT -H264:SPS
typedef struct
{
    quint8  num_sps;
}AVC_VIDEO_CONFIG_SPS, *PAVC_VIDEO_CONFIG_SPS;
// DETAIL_FORMAT -H264:PPS
typedef struct
{
    quint8  num_pps;
}AVC_VIDEO_CONFIG_PPS, *PAVC_VIDEO_CONFIG_PPS;
// DETAIL_FORMAT -H264:NALU

// DETAIL_FORMAT -H264:EOS


// SCRIPT TAG ...


#pragma pack(pop)
class FlvReader : public QThread, public IMediaReader, public IStreamReader
{
    Q_OBJECT
public:
    FlvReader();
    ~FlvReader();

public:
    // IMediaReader
    virtual bool Open(QString path);
    virtual void Close();

    virtual bool GetMediaContext(MEDIA_CONTEXT &context);
    virtual IStreamReader* GetStreamReader();

    virtual bool Seek(qlonglong llpos);

    virtual void Release();

    // IStreamReader
    virtual bool GetVideoStreamContext(VIDEO_STREAM_CONTEXT& context);
    virtual bool GetVideoPacket(PAV_PACKET& pFrame);
    virtual bool IsVideoStreamEnd();

    virtual bool GetAudioStreamContext(AUDIO_STREAM_CONTEXT& context);
    virtual bool GetAudioPacket(PAV_PACKET& pFrame, quint64 maxTimeStamp);
    virtual bool GetAudioPacket(PAV_PACKET& pFrame);
    virtual bool IsAudioStreamEnd();

protected:
    bool SeekToNextVideoTag();

    // video tag process
    void ParseVideoTag();
    // video tag process : H264
    void ParseAVCVideoPacket();
    void ParseAVCVideoConfig(AVC_VIDEO_PACKET_HEADER header);
    void ParseAVCVideoNALU(AVC_VIDEO_PACKET_HEADER header);
    void ParseAVCVideoEOS();

    bool SeekToNextAudioTag();

    // audio tag process
    void ParseAudioTag();

    virtual void run();
protected:
    FLV_HEADER m_header;
    FLV_TAG_HEADER m_currentTagHeaderForVideo;
    FLV_TAG_HEADER m_currentTagHeaderForAudio;

    MEDIA_CONTEXT m_context;

    QList<PAV_PACKET> m_listVideoPackets;
    QList<PAV_PACKET> m_listAudioPackets;

    QFile m_fVideoReader;
    QFile m_fAudioReader;

    QMutex m_mutexVideo;
    QMutex m_mutexAudio;

    bool m_bVideoStreamEnd;
    bool m_bAudioStreamEnd;

    bool m_bRun;
signals:
    void new_audio_packet();
    void new_video_packet();
};

#endif // FLVREADER_H
