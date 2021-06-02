#include "flvreader.h"



FlvReader::FlvReader()
{

}

FlvReader::~FlvReader()
{
    Close();
}
// IMediaReader
bool FlvReader::Open(QString path)
{
    do
    {
        m_fVideoReader.setFileName(path);
        m_fAudioReader.setFileName(path);
        m_fVideoReader.open(QIODevice::ReadOnly);
        m_fAudioReader.open(QIODevice::ReadOnly);


        qint64 size = m_fVideoReader.bytesAvailable();
        m_fVideoReader.read((char*)&m_header, sizeof(m_header));
        m_fAudioReader.read((char*)&m_header, sizeof(m_header));

        qint64 lastValidPos = m_fVideoReader.pos();

        // previous tag size
        quint32 nPreviousTagSize;
        if(m_fVideoReader.read((char*)&nPreviousTagSize, 4) == -1)
            break;



        FLV_TAG_HEADER tag_header;
        if(m_fVideoReader.read((char*)&tag_header, sizeof(FLV_TAG_HEADER)) == -1)
            break;

        tag_header.size = qFromBigEndian(tag_header.size) >> 8;
        if(tag_header.type != FLV_TAG_SCRIPT)
            break;


        // AMF Name
        QByteArray data_script = m_fVideoReader.read(tag_header.size);
        quint8 type = data_script.at(0);

        data_script.remove(0, 1);

        if(type == 0x02)
        {
            quint16 name_length = qFromBigEndian<quint16>((const uchar*)data_script.left(2).toStdString().c_str());

            data_script.remove(0, 2);
            QByteArray name = data_script.left(name_length);
            data_script.remove(0, name_length);


        }

        // AMF Value
        type = data_script.at(0);
        data_script.remove(0, 1);

        if(type == 0x08)
        {
            quint32 count = qFromBigEndian<quint32>((const uchar*)data_script.left(4).toStdString().c_str());
            data_script.remove(0, 4);

            qInfo() << "Media information:";
            while(count--)
            {
                // Property

                quint16 prop_name_length = qFromBigEndian<quint16>((const uchar*)data_script.left(2).toStdString().c_str());
                data_script.remove(0, 2);
                QByteArray prop_name = data_script.left(prop_name_length);
                data_script.remove(0, prop_name_length);

                quint8 prop_value_type = (quint8)data_script.left(1).at(0);
                data_script.remove(0, 1);

                QByteArray prop_value;
                if(prop_value_type == 0x00)
                {
                    prop_value = data_script.left(sizeof(double));
                    data_script.remove(0, sizeof(double));
                    qint64 temp = qFromBigEndian<qint64>((const uchar*)prop_value.toStdString().c_str());
                    qInfo() << prop_name << ":" << *((double*)&temp);

                }
                else if(prop_value_type == 0x01)
                {
                    prop_value = data_script.left(sizeof(bool));
                    data_script.remove(0, sizeof(bool));
                    qInfo() << prop_name << ":" << prop_value;
                }
                else if(prop_value_type == 0x02)
                {
                    quint16 length = qFromBigEndian<quint16>((const uchar*)data_script.left(2).toStdString().c_str());
                    data_script.remove(0, 2);
                    prop_value = data_script.left(length);
                    data_script.remove(0, length);

                    qInfo() << prop_name << ":" << prop_value;
                }
                if(strcmp(prop_name.toStdString().c_str(), "duration") == 0)
                {
                    qint64 temp = qFromBigEndian<qint64>((const uchar*)prop_value.toStdString().c_str());
                    m_context.duration = *((double*)&temp) * 1000;

                }

                if(strcmp(prop_name.toStdString().c_str(), "framerate") == 0)
                {
                    qint64 temp = qFromBigEndian<qint64>((const uchar*)prop_value.toStdString().c_str());
                    m_context.framerate = *((double*)&temp);

                }



            }
        }


        lastValidPos = m_fVideoReader.pos();
        // previous tag size
        if(m_fVideoReader.read((char*)&nPreviousTagSize, 4) == -1)
            break;

        if(m_fVideoReader.read((char*)&tag_header, sizeof(FLV_TAG_HEADER)) == -1)
            break;

        if(tag_header.type == FLV_TAG_SCRIPT)
            break;

        m_fVideoReader.seek(lastValidPos);
        m_fAudioReader.seek(lastValidPos);
        m_bRun = true;


        start();


        return true;
    }while(false);

    Close();
    return false;
}
void FlvReader::Close()
{


    m_bRun = false;
    wait();

    m_fVideoReader.close();
    m_fAudioReader.close();

    for(int i = 0; i < m_listAudioPackets.size(); i++)
        delete m_listAudioPackets.at(i);

    m_listAudioPackets.clear();

    for(int i = 0; i < m_listVideoPackets.size(); i++)
        delete m_listVideoPackets.at(i);

    m_listVideoPackets.clear();

}

bool FlvReader::GetMediaContext(MEDIA_CONTEXT &context)
{
    memcpy(&context, &m_context, sizeof(MEDIA_CONTEXT));
    return true;
}
IStreamReader* FlvReader::GetStreamReader(){
    return this;
}
bool FlvReader::Seek(qlonglong llpos)
{
    return true;
}

void FlvReader::Release()
{
    delete this;
}

// IStreamReader
bool FlvReader::GetVideoStreamContext(VIDEO_STREAM_CONTEXT& context)
{
    return true;
}

bool FlvReader::GetVideoPacket(PAV_PACKET& pPacket)
{
    QMutexLocker locker(&m_mutexVideo);

    if(m_listVideoPackets.isEmpty())
        return false;

    pPacket = m_listVideoPackets.takeFirst();
    return true;
}


bool FlvReader::IsVideoStreamEnd()
{
    qulonglong tick = GetTickCount64();

    QMutexLocker locker(&m_mutexVideo);
    return m_bVideoStreamEnd && m_listVideoPackets.isEmpty();
}


bool FlvReader::GetAudioStreamContext(AUDIO_STREAM_CONTEXT& context)
{
    return true;
}

bool FlvReader::GetAudioPacket(PAV_PACKET& pPacket, quint64 maxTimeStamp)
{
    QMutexLocker locker(&m_mutexAudio);

    if(m_listAudioPackets.isEmpty())
        return false;

    if(m_listAudioPackets.first()->timestamp > maxTimeStamp)
    {
        return false;
    }

    pPacket = m_listAudioPackets.takeFirst();

    return true;
}
bool FlvReader::GetAudioPacket(PAV_PACKET& pPacket)
{

    QMutexLocker locker(&m_mutexAudio);

    if(m_listAudioPackets.isEmpty())
        return false;

    pPacket = m_listAudioPackets.takeFirst();

    return true;
}

bool FlvReader::IsAudioStreamEnd()
{
    QMutexLocker locker(&m_mutexAudio);
    return m_bAudioStreamEnd && m_listAudioPackets.isEmpty();
}


bool FlvReader::SeekToNextVideoTag()
{
    do
    {

        qint64 minTagHeaderSize = sizeof(FLV_TAG_HEADER) + 4;
        qint64 byteAvail = m_fVideoReader.bytesAvailable();
        if(byteAvail < minTagHeaderSize)
        {
            //qDebug()<<"m_fVideoReader.bytesAvailable() < minTagHeaderSize| bytesAvail:" << byteAvail  << "minTagHeaderSize:" << minTagHeaderSize ;
            if(!m_fVideoReader.atEnd())
            {
                m_fVideoReader.seek(m_fVideoReader.size());
            }

            break;
        }


        // previous tag size
        quint32 nPreviousTagSize;
        if(m_fVideoReader.read((char*)&nPreviousTagSize, 4) == -1)
        {
            //qDebug()<<"m_fVideoReader.read(4).isEmpty()";

            break;
        }

        //qDebug() << "previous tag size:%d" << qFromBigEndian(nPreviousTagSize);

        if(m_fVideoReader.read((char*)&m_currentTagHeaderForVideo, sizeof(FLV_TAG_HEADER)) == -1)
        {
            //qDebug()<<"m_fVideoReader.read((char*)&m_currentTagHeaderForVideo, sizeof(FLV_TAG_HEADER)) == -1)";
            break;
        }

        m_currentTagHeaderForVideo.timestamp = qFromBigEndian(m_currentTagHeaderForVideo.timestamp) >> 8;
        m_currentTagHeaderForVideo.timestamp |= m_currentTagHeaderForVideo.exstamp << 24;
        m_currentTagHeaderForVideo.size = qFromBigEndian(m_currentTagHeaderForVideo.size) >> 8;
        //qDebug() << sizeof(FLV_TAG_HEADER) << " " << m_currentTagHeaderForVideo.size;

        if(m_currentTagHeaderForVideo.type == FLV_TAG_VIDEO && m_fVideoReader.bytesAvailable() >= m_currentTagHeaderForVideo.size)
        {
            return true;
        }

        if((m_currentTagHeaderForVideo.type != FLV_TAG_VIDEO) &&
                (m_currentTagHeaderForVideo.type != FLV_TAG_AUDIO) &&
                (m_currentTagHeaderForVideo.type != FLV_TAG_SCRIPT))
        {
            //qDebug()<<"m_currentTagHeaderForVideo.type : "<<m_currentTagHeaderForVideo.type;
            break;
        }


        if(m_fVideoReader.bytesAvailable() < m_currentTagHeaderForVideo.size)
        {
            //qDebug()<<"m_fVideoReader.bytesAvailable() < m_currentTagHeaderForVideo.size "<<m_fVideoReader.bytesAvailable() <<":"<<m_currentTagHeaderForVideo.size;
            m_fVideoReader.seek(m_fVideoReader.size());
            break;
        }
        if(m_fVideoReader.read(m_currentTagHeaderForVideo.size).isEmpty())
        {
            //qDebug() << "m_fVideoReader.read(m_currentTagHeaderForVideo.size).isEmpty()";
            break;
        }
    }while(true);

    //qDebug()<<"FlvReader::SeekToNextVideoTag failed";
    return false;
}

// video tag process
void FlvReader::ParseVideoTag()
{

    VIDEO_TAG_PARAM param;
    m_fVideoReader.read((char*)&param, sizeof(param));

    if(param.codec_id == 7)
    {
        ParseAVCVideoPacket();
    }
    else
    {
        m_fVideoReader.read(m_currentTagHeaderForVideo.size - sizeof(param));
    }
}
// video tag process : H264
void FlvReader::ParseAVCVideoPacket()
{
    AVC_VIDEO_PACKET_HEADER header;
    m_fVideoReader.read((char*)&header, sizeof(header));
    header.composition_time = qFromBigEndian(header.composition_time) >> 8;

    switch(header.packet_type)
    {
    case 0:
        ParseAVCVideoConfig(header);
        break;
    case 1:
        ParseAVCVideoNALU(header);
        break;
    case 2:
        ParseAVCVideoEOS();
        break;
    }

}

void FlvReader::ParseAVCVideoConfig(AVC_VIDEO_PACKET_HEADER header)
{
    AVC_VIDEO_CONFIG_HEADER avc_config;
    m_fVideoReader.read((char*)&avc_config, sizeof(avc_config));
    AVC_VIDEO_CONFIG_SPS sps;
    m_fVideoReader.read((char*)&sps, sizeof(sps));
    sps.num_sps &= 0x1F;


    for(int i = 0; i < sps.num_sps; i++)
    {
        quint16 length = 0;
        m_fVideoReader.read((char*)&length, 2);

        length = qFromBigEndian(length);

        QByteArray sps_data = m_fVideoReader.read(length);
        //qDebug() << "sps length" << length << " " << sps_data.size();

        PAV_PACKET packet = new AV_PACKET;
        packet->timestamp = m_currentTagHeaderForVideo.timestamp;
        packet->data = sps_data;
        packet->data.prepend((char)0x01);
        packet->data.prepend((char)0x00);
        packet->data.prepend((char)0x00);
        packet->data.prepend((char)0x00);

        QMutexLocker locker(&m_mutexVideo);
        m_listVideoPackets.push_back(packet);
    }

    AVC_VIDEO_CONFIG_PPS pps;
    m_fVideoReader.read((char*)&pps, sizeof(pps));
    for(int i = 0; i < pps.num_pps; i++)
    {
        quint16 length = 0;
        m_fVideoReader.read((char*)&length, 2);
        length = qFromBigEndian(length);
        QByteArray pps_data = m_fVideoReader.read(length);

        PAV_PACKET packet = new AV_PACKET;
        packet->timestamp = m_currentTagHeaderForVideo.timestamp;
        packet->data = pps_data;
        packet->data.prepend((char)0x01);
        packet->data.prepend((char)0x00);
        packet->data.prepend((char)0x00);
        packet->data.prepend((char)0x00);

        QMutexLocker locker(&m_mutexVideo);
        m_listVideoPackets.push_back(packet);
    }
}
void FlvReader::ParseAVCVideoNALU(AVC_VIDEO_PACKET_HEADER header)
{

    quint32 unit_readed_bytes = sizeof(VIDEO_TAG_PARAM) + sizeof(AVC_VIDEO_PACKET_HEADER);
    while(unit_readed_bytes < m_currentTagHeaderForVideo.size)
    {
        quint32 length = 0;
        m_fVideoReader.read((char*)&length, 4);
        length = qFromBigEndian(length);
        QByteArray nalu_data = m_fVideoReader.read(length);


        PAV_PACKET packet = new AV_PACKET;
        packet->timestamp = m_currentTagHeaderForVideo.timestamp;
        packet->cts = header.composition_time;

        packet->data = nalu_data;
        packet->data.prepend((char)0x01);
        packet->data.prepend((char)0x00);
        packet->data.prepend((char)0x00);
        packet->data.prepend((char)0x00);

        QMutexLocker locker(&m_mutexVideo);
        m_listVideoPackets.push_back(packet);
        unit_readed_bytes += (4 + length);

    }
}
void FlvReader::ParseAVCVideoEOS()
{

}


bool FlvReader::SeekToNextAudioTag()
{
    do
    {

        qint64 minTagHeaderSize = sizeof(FLV_TAG_HEADER) + 4;
        qint64 byteAvail = m_fAudioReader.bytesAvailable();
        if(byteAvail < minTagHeaderSize)
        {
            //qDebug()<<"m_fAudioReader.bytesAvailable() < minTagHeaderSize| bytesAvail:" << byteAvail  << "minTagHeaderSize:" << minTagHeaderSize ;
            if(!m_fAudioReader.atEnd())
            {
                m_fAudioReader.seek(m_fAudioReader.size());
            }

            break;
        }


        // previous tag size
        quint32 nPreviousTagSize;
        if(m_fAudioReader.read((char*)&nPreviousTagSize, 4) == -1)
        {
            //qDebug()<<"m_fAudioReader.read(4).isEmpty()";

            break;
        }

        if(m_fAudioReader.read((char*)&m_currentTagHeaderForAudio, sizeof(FLV_TAG_HEADER)) == -1)
        {
            //qDebug()<<"m_fAudioReader.read((char*)&m_currentTagHeaderForAudio, sizeof(FLV_TAG_HEADER)) == -1)";
            break;
        }

        m_currentTagHeaderForAudio.timestamp = qFromBigEndian(m_currentTagHeaderForAudio.timestamp) >> 8;
        m_currentTagHeaderForAudio.timestamp |= m_currentTagHeaderForAudio.exstamp << 24;
        m_currentTagHeaderForAudio.size = qFromBigEndian(m_currentTagHeaderForAudio.size) >> 8;
        //qDebug() << sizeof(FLV_TAG_HEADER) << " " << m_currentTagHeaderForAudio.size;

        if(m_currentTagHeaderForAudio.type == FLV_TAG_AUDIO && m_fAudioReader.bytesAvailable() >= m_currentTagHeaderForAudio.size)
        {
            return true;
        }

        if((m_currentTagHeaderForAudio.type != FLV_TAG_VIDEO) &&
                (m_currentTagHeaderForAudio.type != FLV_TAG_AUDIO) &&
                (m_currentTagHeaderForAudio.type != FLV_TAG_SCRIPT))
        {
            //qDebug()<<"m_currentTagHeaderForAudio.type : "<<m_currentTagHeaderForAudio.type;
            break;
        }


        if(m_fAudioReader.bytesAvailable() < m_currentTagHeaderForAudio.size)
        {
            //qDebug()<<"m_fAudioReader.bytesAvailable() < m_currentTagHeaderForAudio.size "<<m_fAudioReader.bytesAvailable() <<":"<<m_currentTagHeaderForAudio.size;
            m_fAudioReader.seek(m_fAudioReader.size());
            break;
        }
        if(m_fAudioReader.read(m_currentTagHeaderForAudio.size).isEmpty())
        {
            //qDebug() << "m_fAudioReader.read(m_currentTagHeaderForAudio.size).isEmpty()";
            break;
        }
    }while(true);

    return false;

}

void FlvReader::ParseAudioTag()
{
    AUDIO_TAG_PARAM param;
    m_fAudioReader.read((char*)&param, sizeof(param));

    QByteArray data = m_fAudioReader.read(m_currentTagHeaderForAudio.size - sizeof(param));
    if(param.soundformat == 2)
    {
        PAV_PACKET pPacket = new AV_PACKET();
        pPacket->data = data;
        pPacket->timestamp = m_currentTagHeaderForAudio.timestamp;

        QMutexLocker locker(&m_mutexAudio);
        m_listAudioPackets.push_back(pPacket);
        emit new_audio_packet();
    }
}

void FlvReader::run()
{
    while(m_bRun)
    {
        int delay = 10;



        if(SeekToNextVideoTag())
            ParseVideoTag();




        bool bVideoStreamEnd = m_fVideoReader.atEnd();

        int nPendedVideoPackets = 0;
        {
            QMutexLocker locker(&m_mutexVideo);
            m_bVideoStreamEnd = bVideoStreamEnd;
            nPendedVideoPackets = m_listVideoPackets.size();
        }

        if(SeekToNextAudioTag())
            ParseAudioTag();

        bool bAudioStreamEnd = m_fAudioReader.atEnd();
        int nPendedAudioPackets = 0;
        {
            QMutexLocker locker(&m_mutexAudio);
            m_bAudioStreamEnd = bAudioStreamEnd;
            nPendedAudioPackets = m_listAudioPackets.size();
        }



        if(bVideoStreamEnd && bAudioStreamEnd)
            m_bRun = false;


        if(nPendedAudioPackets > 30 && nPendedVideoPackets > 20)
            delay += (nPendedVideoPackets - 20) * 40  < (nPendedAudioPackets - 30) * 26 ? (nPendedVideoPackets - 20) * 40 : (nPendedAudioPackets - 30) * 26;
        if(delay < 10)
            delay = 10;
        msleep(delay);
    }



}



