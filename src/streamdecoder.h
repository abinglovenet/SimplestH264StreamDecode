#ifndef STREAMDECODER_H
#define STREAMDECODER_H

#include <QObject>
#include <QThread>
#include <QDebug>
#include <QFile>

#include "mfxvideo++.h"
#include "common_utils.h"
#include "istreamreader.h"

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

    STREAM_CODER_ERRORCODE  RunDecoder();
    void StopDecoder();
    void PauseDecoder();

    STREAM_CODER_STATUS GetStatus();
signals:
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


    bool m_bRun;

    // test
    QFile file;
    int framecount;
};

#endif // STREAMDECODER_H
