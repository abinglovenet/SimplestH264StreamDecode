#include "streamdecoder.h"
#define MAX_BUFFERED_FRAMES 6


#define CURRENT_TICK(zero_tick) (GetTickCount() - zero_tick)


void WINAPI ScheduleOutputFrameProc(UINT wTimerID, UINT msg,DWORD dwUser,DWORD dwl,DWORD dw2)
{

    StreamDecoder* pDecoder = (StreamDecoder*)dwUser;
    pDecoder->ScheduleOutputFrame();
    return;
}

StreamDecoder::StreamDecoder(QObject *parent) : QThread(parent)
{
    m_pVideoStream = nullptr;
    mfxVersion version = {0, 1};
    Initialize(MFX_IMPL_HARDWARE_ANY, version, &m_session, &m_frameAllocator);
    m_pDecoder = new MFXVideoDECODE(m_session);
    memset(&m_bs, 0, sizeof(m_bs));
    m_bs.Data = new mfxU8[8 * 1024 * 1024];
    m_bs.DataLength = 0;
    m_bs.MaxLength = 8 * 1024 * 1024;
    m_nFrameRate = 25;

#ifdef _READ_H264_FROM_FILE
    QString fileName = QString::asprintf("C:\\Users\\Public\\test.h264", this);

    file.setFileName(fileName);

    file.open(QIODevice::ReadWrite);
    framecount = 0;
#endif
}

StreamDecoder::~StreamDecoder()
{
    qInfo() << "delete stream decoder...1";
    StopDecoder();
    qInfo() << "delete stream decoder...2";
    delete m_pDecoder;
    qInfo() << "delete stream decoder...3";

    //::Release();
    delete m_bs.Data;
}

void StreamDecoder::Release()
{
    delete this;
}

void StreamDecoder::SetStream(IStreamReader* stream)
{
    m_pVideoStream = stream;
}

void StreamDecoder::SetFrameRate(int nFrameRate)
{
    m_nFrameRate = nFrameRate;
    m_nSkipRate = 0;
    m_nSkipCount = 0;


    int nDuration = 1000 / nFrameRate;
    int nMod = 1000 % nFrameRate;
    if(1000 % nFrameRate != 0)
    {
    for(int i = 1; i <= nFrameRate; i++)
    {
        if((i * nDuration * nFrameRate) % nMod == 0)
        {
            m_nSkipCount = i;
            m_nSkipRate = (i * nDuration * nFrameRate) / nMod;
            break;
        }
    }
    }
}

STREAM_CODER_ERRORCODE  StreamDecoder::RunDecoder()
{
    /********** ACTIONS***********
 * 1.DECODE HEADER
 * 2.QUERYIOSURFACE
 * 3.ALLOC SURFACE
 * 4.START THREAD
 * *********************************/

    mfxStatus sts;

    mfxVideoParam param;
    memset(&param, 0, sizeof(param));
    param.mfx.CodecId = MFX_CODEC_AVC;
    param.AsyncDepth = 1;
    param.IOPattern = MFX_IOPATTERN_OUT_VIDEO_MEMORY;



    while((sts = m_pDecoder->DecodeHeader(&m_bs, &param)) == MFX_ERR_MORE_DATA)
    {
        qInfo() << "DecodeHeader Need more data";
        ReadPacket();
    }


    if(sts != MFX_ERR_NONE)
    {
        qDebug() << "decode header failed" << sts;
        return SC_DECODE_HEADER_FAILED;
    }

    mfxVideoParam paramValid;
    memset(&paramValid, 0, sizeof(paramValid));
    paramValid.mfx.CodecId = MFX_CODEC_AVC;
    sts = m_pDecoder->Query(&param, &paramValid);

    mfxFrameAllocRequest request;
    memset(&request, 0, sizeof(request));

    sts = m_pDecoder->QueryIOSurf(&paramValid, &request);
    request.Type |= MFX_MEMTYPE_VIDEO_MEMORY_DECODER_TARGET;
    request.Type |= WILL_READ;
    qDebug() << "m_pDecoder->QueryIOSurf" << request.Type;
    // error code process


    request.NumFrameSuggested += MAX_BUFFERED_FRAMES * 2;
    sts = m_frameAllocator.Alloc(m_frameAllocator.pthis, &request, &m_response);
    qDebug() << "m_frameAllocator.Alloc " << sts << m_response.NumFrameActual;
    // error code process


    m_pFrameSurfaces = new mfxFrameSurface1*[m_response.NumFrameActual];
    for(int i = 0; i < m_response.NumFrameActual; i++)
    {
        m_pFrameSurfaces[i] = new mfxFrameSurface1;

        memset(m_pFrameSurfaces[i], 0, sizeof(mfxFrameSurface1));
        m_pFrameSurfaces[i]->Data.MemType = request.Type;
        m_pFrameSurfaces[i]->Data.MemId = m_response.mids[i];
        m_pFrameSurfaces[i]->reserved[0] = (mfxU32)new QMutex();
        m_pFrameSurfaces[i]->reserved[1] = 0;
        memcpy(&(m_pFrameSurfaces[i]->Info), &(paramValid.mfx.FrameInfo), sizeof(mfxFrameInfo));

    }
    m_nSurfacePoolSize = m_response.NumFrameActual;




    sts = m_pDecoder->Init(&paramValid);



    m_bRun = true;
    start(QThread::TimeCriticalPriority);


    m_timerId = timeSetEvent(1000 / m_nFrameRate, 1, ScheduleOutputFrameProc, (DWORD_PTR)this, TIME_PERIODIC | TIME_KILL_SYNCHRONOUS | TIME_CALLBACK_FUNCTION);

    m_nTickCount = 0;
    m_nTotalFrames = 0;
    m_beginTick = 0;
    m_recentBeginTick = 0;
    m_nRecentFrameNum = 0;
    return SC_SUCCESS;
}
void StreamDecoder::StopDecoder()
{
    if(m_bRun)
    {

        timeKillEvent(m_timerId);

        m_bRun = false;
        wait();

        m_pDecoder->Close();
        for(int i = 0; i < m_response.NumFrameActual; i++)
        {
            delete (QMutex*)m_pFrameSurfaces[i]->reserved[0];
            delete m_pFrameSurfaces[i];

        }

        delete []m_pFrameSurfaces;

        m_frameAllocator.Free(m_frameAllocator.pthis, &m_response);

        for(int i = 0; i < m_bufFrames.size(); i++)
            delete m_bufFrames[i];
        m_bufFrames.clear();
    }

}
void StreamDecoder::PauseDecoder()
{

}

void StreamDecoder::DisableOutputFrame()
{
    timeKillEvent(m_timerId);
}

char szBuffer[512];
void StreamDecoder::ScheduleOutputFrame()
{

    PAV_FRAME pRenderFrame = nullptr;

    {
        QMutexLocker locker(&m_mutexFrames);
        if(!m_bufFrames.isEmpty())
        {
            pRenderFrame = m_bufFrames.takeFirst();


            if(m_nSkipRate != 0)
            {
                ++m_nTickCount;

                if(m_nTickCount % (m_nSkipRate + m_nSkipCount) == 0 || m_nTickCount % (m_nSkipRate + m_nSkipCount) > m_nSkipRate)
                {
                    m_bufFrames.push_front(pRenderFrame);
                    return;
                }


            }
        }


    }

    if(pRenderFrame != nullptr)
    {

        ++m_nTotalFrames;
        ++m_nRecentFrameNum;


        if(m_beginTick == 0)
            m_beginTick = GetTickCount() - pRenderFrame->timestamp;


        if(m_recentBeginTick == 0)
        {
            m_recentBeginTick = m_beginTick;
        }

        if((GetTickCount() - m_recentBeginTick) >= 1000)
        {
            sprintf(szBuffer, "total framerate:%f current framerate:%f", (float)m_nTotalFrames * 1000 / (GetTickCount() - m_beginTick), (float)m_nRecentFrameNum * 1000 / (GetTickCount() - m_recentBeginTick));
            OutputDebugStringA(szBuffer);
            m_recentBeginTick = GetTickCount();
            m_nRecentFrameNum = 0;
        }
        emit frameArrived(pRenderFrame);


    }



}
bool StreamDecoder::ReadPacket()
{
#ifdef _READ_H264_FROM_FILE
    // ignore stream packet
    PAV_PACKET temp;
    m_pVideoStream->GetVideoPacket(temp);

    qDebug() << "bitstream : offset " << m_bs.DataOffset << " " << m_bs.DataLength << " " << m_bs.MaxLength;
    memmove(m_bs.Data, m_bs.Data + m_bs.DataOffset, m_bs.DataLength);
    m_bs.DataOffset = 0;

    QByteArray buffer;
    QByteArray split1, split2;
    split1.append((char)0x00);
    split1.append((char)0x00);
    split1.append((char)0x00);
    split1.append((char)0x01);

    split2.append((char)0x00);
    split2.append((char)0x00);
    split2.append((char)0x01);

    int nPos = 0;
    while(buffer.append(file.read(1024)).size() > 0)
    {

        // qInfo() << buffer.toHex();

        if((nPos = buffer.indexOf(split1)) != -1)
        {

            file.seek(file.pos() - (buffer.size() - nPos - split1.size()));
            buffer = buffer.left(nPos);
            buffer.prepend(split1);
            qInfo() << "position" << file.pos() << buffer.size();

            break;
        }

        if((nPos = buffer.indexOf(split2)) != -1)
        {

            file.seek(file.pos() - (buffer.size() - nPos - split2.size()));
            buffer = buffer.left(nPos);
            buffer.prepend(split2);

            break;
        }
    }

    if(buffer.isEmpty())
        return false;

    framecount++;

    //#ifdef TRACE_IMAGE
    if(buffer.size() >= 40000)
    {
        QString fileName = QString::asprintf("C:\\Users\\Public\\temp\\%x.jpeg", framecount * 40);
        QFile file(fileName);
        file.open(QIODevice::ReadWrite);
        file.write(buffer);
        file.close();
    }
    //#endif
    qInfo() << buffer.toHex().left(32) << buffer.toHex().right(32);
    m_bs.DecodeTimeStamp = framecount * 40 * 90;
    m_bs.TimeStamp = MFX_TIMESTAMP_UNKNOWN;
    memcpy(m_bs.Data + m_bs.DataLength, buffer.data(), buffer.size());
    m_bs.DataLength += buffer.size();
    return true;
#endif

    if(m_pVideoStream == nullptr)
        return false;


    PAV_PACKET packet = nullptr;
    if(!m_pVideoStream->GetVideoPacket(packet))
        return false;

    memmove(m_bs.Data, m_bs.Data + m_bs.DataOffset, m_bs.DataLength);
    m_bs.DataOffset = 0;


    m_bs.DecodeTimeStamp = packet->timestamp * 90;
    m_bs.TimeStamp = (packet->timestamp + packet->cts) * 90;
    memcpy(m_bs.Data + m_bs.DataLength, packet->data.data(), packet->data.size());

    m_bs.DataLength += packet->data.size();
    delete packet;

    return true;
}

void StreamDecoder::run()
{

    do
    {
        if(nullptr == m_pVideoStream)
            break;


        mfxStatus sts;
        int nFreeSurfaceIndex;
        mfxFrameSurface1* pOutSurface;
        mfxSyncPoint syncp;
        bool bNotifyFirstFrame = false;


        while(m_bRun)
        {

            nFreeSurfaceIndex = GetFreeSurfaceIndex(m_pFrameSurfaces, m_nSurfacePoolSize);
            if(nFreeSurfaceIndex == MFX_ERR_NOT_FOUND)
            {
                //OutputDebugStringA(" wait1......");
                Sleep(1);
                continue;
            }

            pOutSurface = nullptr;



            if(m_bs.DataLength > 0 || ReadPacket())
            {

                sts = m_pDecoder->DecodeFrameAsync(&m_bs, m_pFrameSurfaces[nFreeSurfaceIndex], &pOutSurface, &syncp);

            }
            else
            {
                if(!m_pVideoStream->IsVideoStreamEnd())
                {
                    //OutputDebugStringA(" wait2......");
                    Sleep(1);
                    continue;
                }

                sts = m_pDecoder->DecodeFrameAsync(NULL, m_pFrameSurfaces[nFreeSurfaceIndex], &pOutSurface, &syncp);
                if(sts == MFX_ERR_MORE_DATA)
                    break;


            }


            if(sts != MFX_ERR_NONE)
            {
                char szDebug[128] = {0};
                sprintf(szDebug, "wait3..... sts:%d", sts);
                //OutputDebugStringA(szDebug);
                //Sleep(1);
                continue;
            }

            m_session.SyncOperation(syncp, 60);
            if(pOutSurface)
            {

                //qInfo() << "Out Frame time stamp" << pOutSurface->Data.TimeStamp / 90;
                PAV_FRAME pFrame = new AV_FRAME();
                pFrame->height = pOutSurface->Info.Height;

                pFrame->width = pOutSurface->Info.Width;
                pFrame->timestamp = pOutSurface->Data.TimeStamp / 90;
                pFrame->pSurface = pOutSurface;

                //lock
                pFrame->pSurface->reserved[1] = 1;
                // WriteRawFrame(pOutSurface, &pFrame->data, &m_frameAllocator);
                QMutexLocker locker(&m_mutexFrames);
                m_bufFrames.push_back(pFrame);


                if(!bNotifyFirstFrame)
                {
                    emit firstFrameNotify();
                    bNotifyFirstFrame = true;
                }

            }



        }


    }while(FALSE);

    emit decodeFinished();

}
