#include "streamdecoder.h"


StreamDecoder::StreamDecoder(QObject *parent) : QThread(parent)
{
    qInfo() << "11111-1";
    m_pVideoStream = nullptr;
    mfxVersion version = {0, 1};
    Initialize(MFX_IMPL_HARDWARE_ANY, version, &m_session, &m_frameAllocator);
    qInfo() << "11111-2";
    m_pDecoder = new MFXVideoDECODE(m_session);
    memset(&m_bs, 0, sizeof(m_bs));
    m_bs.Data = new mfxU8[8 * 1024 * 1024];
    m_bs.DataLength = 0;
    m_bs.MaxLength = 8 * 1024 * 1024;

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

    ::Release();
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

        /*
        for(int i = m_bs.DataOffset; i < (m_bs.DataOffset + m_bs.DataLength); i++)
            qDebug("bs byte:%x", m_bs.Data[i]);
            */
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
    /*
    paramValid.mfx.CodecId = MFX_CODEC_AVC;
    paramValid.mfx.CodecProfile = MFX_PROFILE_AVC_BASELINE;
    paramValid.mfx.FrameInfo.Width = MSDK_ALIGN32(paramValid.mfx.FrameInfo.Width);
    paramValid.mfx.FrameInfo.Height = MSDK_ALIGN32(paramValid.mfx.FrameInfo.Height);
    qInfo() << "m_pDecoder->Query(&param, &paramValid)" << paramValid.mfx.CodecProfile << paramValid.mfx.FrameInfo.ChromaFormat << " "  << paramValid.mfx.FrameInfo.Width << paramValid.mfx.FrameInfo.Height << " " << sts;
    */
    mfxFrameAllocRequest request;
    memset(&request, 0, sizeof(request));

    sts = m_pDecoder->QueryIOSurf(&paramValid, &request);
    request.Type |= MFX_MEMTYPE_VIDEO_MEMORY_DECODER_TARGET;
    request.Type |= WILL_READ;
    qDebug() << "m_pDecoder->QueryIOSurf" << request.Type;
    // error code process


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
        memcpy(&(m_pFrameSurfaces[i]->Info), &(paramValid.mfx.FrameInfo), sizeof(mfxFrameInfo));

    }
    m_nSurfacePoolSize = m_response.NumFrameActual;




    sts = m_pDecoder->Init(&paramValid);



    m_bRun = true;
    start();
    return SC_SUCCESS;
}
void StreamDecoder::StopDecoder()
{
    if(m_bRun)
    {

        m_bRun = false;
        wait();

        m_pDecoder->Close();
        for(int i = 0; i < m_response.NumFrameActual; i++)
        {
            delete m_pFrameSurfaces[i];

        }

        delete []m_pFrameSurfaces;

        m_frameAllocator.Free(m_frameAllocator.pthis, &m_response);
    }

}
void StreamDecoder::PauseDecoder()
{

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
            // qInfo() << "1121212345"<<buffer.toHex();
            break;
        }

        if((nPos = buffer.indexOf(split2)) != -1)
        {

            file.seek(file.pos() - (buffer.size() - nPos - split2.size()));
            buffer = buffer.left(nPos);
            buffer.prepend(split2);
            //qInfo() << "u1121212345"<<buffer.toHex();
            break;
        }
    }

    if(buffer.isEmpty())
        return false;

    framecount++;
    // qDebug() << "ReadPacket2" << GetTickCount64() - tick;
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
    qulonglong tick = GetTickCount64();

    qDebug() << "StreamDecoder::ReadPacket() begin";
    if(m_pVideoStream == nullptr)
        return false;

    PAV_PACKET packet = nullptr;
    if(!m_pVideoStream->GetVideoPacket(packet))
        return false;
    qDebug() << "ReadPacket1" << GetTickCount64() - tick;

    qDebug() << "bitstream : offset " << m_bs.DataOffset << " " << m_bs.DataLength << " " << m_bs.MaxLength;
    memmove(m_bs.Data, m_bs.Data + m_bs.DataOffset, m_bs.DataLength);
    m_bs.DataOffset = 0;

    qDebug() << "ReadPacket2" << GetTickCount64() - tick;

    m_bs.DecodeTimeStamp = packet->timestamp * 90;
    m_bs.TimeStamp = (packet->timestamp + packet->cts) * 90;
    memcpy(m_bs.Data + m_bs.DataLength, packet->data.data(), packet->data.size());

    qDebug() << "ReadPacket3" << GetTickCount64() - tick;

    m_bs.DataLength += packet->data.size();
    qDebug() << "ReadPacket4" << GetTickCount64() - tick;
    delete packet;
    qDebug() << "ReadPacket5" << GetTickCount64() - tick;
    qDebug() << "StreamDecoder::ReadPacket() success" << m_bs.DataLength << " " << m_bs.MaxLength;
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

        quint64 first_frame_tick = 0;

        while(m_bRun)
        {

            quint64 tick = GetTickCount64();

            nFreeSurfaceIndex = GetFreeSurfaceIndex(m_pFrameSurfaces, m_nSurfacePoolSize);
            qDebug() << "GetFreeSurfaceIndex " << nFreeSurfaceIndex;
            if(nFreeSurfaceIndex == MFX_ERR_NOT_FOUND)
            {
                msleep(10);
                continue;
            }

            qDebug() << "t1" << GetTickCount64() - tick;
            pOutSurface = nullptr;

            if(m_bs.DataLength > 0 || ReadPacket())
            {
                qDebug() << "t2:1" << GetTickCount64() - tick;
                sts = m_pDecoder->DecodeFrameAsync(&m_bs, m_pFrameSurfaces[nFreeSurfaceIndex], &pOutSurface, &syncp);

                if(sts == MFX_ERR_MORE_DATA)
                {
                    if(!ReadPacket())
                    {
                        if(m_pVideoStream->IsVideoStreamEnd())
                            break;
                    }

                    continue;
                }

                qDebug() << "m_pDecoder->DecodeFrameAsync " << sts;
            }
            else
            {
                if(m_pVideoStream->IsVideoStreamEnd())
                {
                    sts = m_pDecoder->DecodeFrameAsync(NULL, m_pFrameSurfaces[nFreeSurfaceIndex], &pOutSurface, &syncp);
                    if(sts == MFX_ERR_MORE_DATA)
                        break;
                }
                else
                    continue;
            }


            if(sts != MFX_ERR_NONE)
                continue;
            qDebug() << "t2" << GetTickCount64() - tick;
            qDebug() << GetTickCount();
            m_session.SyncOperation(syncp, 60000);

            qDebug() << "t3" << GetTickCount64() - tick;
            if(pOutSurface)
            {

                qDebug() << "Out Frame time stamp" << pOutSurface->Data.TimeStamp / 90;
                PAV_FRAME pFrame = new AV_FRAME();
                pFrame->height = pOutSurface->Info.Height;
                qInfo() << "pOutSurface " << pOutSurface->Info.CropH << pOutSurface->Info.CropW << pOutSurface->Info.Width << pOutSurface->Info.Height;
                pFrame->width = pOutSurface->Info.Width;
                pFrame->timestamp = pOutSurface->Data.TimeStamp / 90;
                WriteRawFrame(pOutSurface, &pFrame->y, &pFrame->uv, &m_frameAllocator);
                // static quint8* pBuffer = new quint8[4096 * 2160 * 3];
                // memset(pBuffer, 0, pFrame->width * pFrame->height * 3);
                // pFrame->y.append((const char*)pBuffer, pFrame->width * pFrame->height * 3);


                if(first_frame_tick == 0)
                    first_frame_tick = GetTickCount() - pFrame->timestamp;
                quint32 duration = GetTickCount() - first_frame_tick;
                if(pFrame->timestamp > duration)
                    msleep(pFrame->timestamp - duration);

                emit frameArrived(pFrame);

            }

            qDebug() << "t4" << GetTickCount64() - tick;
        }


    }while(FALSE);

    emit decodeFinished();
}
