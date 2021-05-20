#ifndef ISTREAMREADER_H
#define ISTREAMREADER_H

#include <QObject>
#include <QByteArray>
typedef struct {}AUDIO_FRAME_HEADER, *PAUDIO_FRAME_HEADER;
typedef struct {}VIDEO_FRAME_HEADER, *PVIDEO_FRAME_HEADER;

typedef struct{

}VIDEO_STREAM_CONTEXT, *PVIDEO_STREAM_CONTEXT;

typedef struct{

}AUDIO_STREAM_CONTEXT, *PAUDIO_STREAM_CONTEXT;

typedef struct
{
    quint16 width;
    quint16 height;
    quint32 timestamp;
    quint32 cts;
    QByteArray data;

    /*
    QByteArray y;
    QByteArray uv;
    */
}AV_PACKET, *PAV_PACKET, AV_FRAME, *PAV_FRAME;

class IStreamReader
{
public:
    IStreamReader();

public:
    virtual bool GetVideoStreamContext(VIDEO_STREAM_CONTEXT& context) = 0;
    virtual bool GetVideoPacket(PAV_PACKET& pFrame) = 0;
    virtual bool IsVideoStreamEnd() = 0;

    virtual bool GetAudioStreamContext(AUDIO_STREAM_CONTEXT& context) = 0;
    virtual bool GetAudioPacket(PAV_PACKET& pFrame, quint64 max_timestamp) = 0;
    virtual bool GetAudioPacket(PAV_PACKET& pFrame) = 0;
    virtual bool IsAudioStreamEnd() = 0;

};

#endif // ISTREAMREADER_H
