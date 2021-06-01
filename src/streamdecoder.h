#ifndef STREAMDECODER_H
#define STREAMDECODER_H

#include <QObject>
#include <QThread>
#include <QDebug>
#include <QFile>
#include <QList>

#include "mfxvideo++.h"
#include "common_utils.h"
#include "istreamreader.h"


typedef struct
{
    quint16 width;
    quint16 height;
    quint32 timestamp;
    quint32 cts;
    QByteArray data;

    // This data is used when the GPU processes frames directly
    mfxFrameSurface1* pSurface;
}AV_FRAME, *PAV_FRAME;

enum STREAM_CODER_ERRORCODE{
    SC_SUCCESS = 0,

    SC_DECODE_HEADER_FAILED,
};

enum STREAM_CODER_STATUS{
    IDLE = 0,
    RUNNING,
    PAUSED
};


class StreamDecoder : public QThread
{
    Q_OBJECT


public:
    explicit StreamDecoder(QObject *parent = nullptr);
    ~StreamDecoder();

    void Release();
public:
    void SetStream(IStreamReader* stream);
    void SetFrameRate(int nFrameRate);

    STREAM_CODER_ERRORCODE  RunDecoder();
    void StopDecoder();
    void PauseDecoder();

    STREAM_CODER_STATUS GetStatus();

    void DisableOutputFrame();
    void ScheduleOutputFrame();
signals:
    void firstFrameNotify();
    void frameArrived(void* pFrame);
    void decodeFinished();
    void errorOccur(STREAM_CODER_ERRORCODE);

protected:
    bool ReadPacket();

    virtual void run();
protected:
    MFXVideoSession m_session;
    MFXVideoDECODE* m_pDecoder;

    mfxFrameAllocator m_frameAllocator;
    mfxFrameAllocResponse m_response;
    mfxFrameSurface1** m_pFrameSurfaces;
    int m_nSurfacePoolSize;
    mfxBitstream m_bs;

    IStreamReader*     m_pVideoStream;
    int m_nFrameRate;
    int m_nSkipRate;
    int m_nSkipCount;


    QList<PAV_FRAME> m_bufFrames;
    QMutex m_mutexFrames;
    MMRESULT m_timerId;


    int m_nTickCount;
    int m_nTotalFrames;
    quint32 m_beginTick;
    quint32 m_recentBeginTick;
    int m_nRecentFrameNum;


    bool m_bRun;
};

#endif // STREAMDECODER_H
