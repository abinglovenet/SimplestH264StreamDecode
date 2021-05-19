#include <QTime>

#include "mainwindow.h"
#include "ui_mainwindow.h"



#define FIX_TIMESTAMP(x) (x+100)
#define MAX_TIMESTAMP(x) (x+100)
#define MIN_TIMESTAMP(x) ((x-100) > 0 ? x - 100 : x)

int  __stdcall  zplayCallbackFunc(void* instance, void *user_data, TCallbackMessage message, unsigned int param1, unsigned int param2)
{
    MainWindow* processor = (MainWindow*) user_data;

    //qDebug() << "zPLAYCALLBACK";
    switch(message)
    {
    case MsgStreamNeedMoreData: // stream needs more data
    {
        quint64 tick = GetTickCount64();
        processor->TransferAudioDataToZPlay();
        //qInfo() << "zplay delay" << GetTickCount64() - tick;
        return 2;

    }
    default:
        break;

    }

    return 0;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);


    m_pRender = new QImageRender(ui->centralwidget);
    ui->centralwidget->layout()->addWidget(m_pRender);

    m_pMediaReader = nullptr;
    m_pStreamDecoder = nullptr;
    m_pAudioDecoder = nullptr;
    m_pSpeaker = nullptr;

    file.setFileName("C:\\Users\\Public\\test.mp3");
    file.open(QIODevice::ReadWrite);


    update_player_state(PLAYER_IDLE);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::TransferAudioDataToZPlay()
{
    //qInfo() << "TransferAudioDataToZPlay";
    PAV_PACKET pPacket = nullptr;
    int nPushCount = 0;

    quint64 maxTimeStamp = 0;
    quint64 minTimeStamp = 0;

    while(true)
    {
        if(m_pMediaReader->GetStreamReader()->IsAudioStreamEnd())
        {
            emit audioDecodeFinished();
            break;
        }

        {
            QMutexLocker locker(&m_locker);
            maxTimeStamp = MAX_TIMESTAMP(m_currentTimeStamp);
            minTimeStamp = MIN_TIMESTAMP(m_currentTimeStamp);
        }
        if(m_bNeedAVSync)
        {

            m_pMediaReader->GetStreamReader()->GetAudioPacket(pPacket, maxTimeStamp);

        }
        else
            m_pMediaReader->GetStreamReader()->GetAudioPacket(pPacket);

        if(pPacket == nullptr)
        {
            ::Sleep(2);
            break;
        }

        if(pPacket->timestamp < minTimeStamp)
        {
            delete pPacket;
            pPacket = nullptr;
            continue;
        }


        nPushCount++;
        int ret = m_pAudioPlayer->PushDataToStream(pPacket->data.data(), pPacket->data.size());
        //qInfo() << "Push ret" << ret;
        delete pPacket;
        pPacket= nullptr;


    }
    qInfo() << nPushCount;

}

void	MainWindow::resizeEvent(QResizeEvent * event)
{
    if(ui->centralwidget->height() <= 0)
        return;

    bool bUseHeightAsBase = ui->centralwidget->width() / ui->centralwidget->height() > 16 / 9;

    if(bUseHeightAsBase)
    {
        int calcWidth = ui->centralwidget->height() * 16 / 9;
        ui->centralwidget->layout()->setContentsMargins((ui->centralwidget->width() - calcWidth) / 2, 0, (ui->centralwidget->width() - calcWidth) / 2, 0);
    }
    else
    {
        int calcHeight = ui->centralwidget->width() * 9 / 16;
        ui->centralwidget->layout()->setContentsMargins(0, (ui->centralwidget->height() - calcHeight) / 2, 0, (ui->centralwidget->height() - calcHeight) / 2);
    }

}
void MainWindow::update_player_state(int player_state)
{
    // UI change coding...
    switch(player_state)
    {
    case PLAYER_IDLE:
    {
        ui->open->setEnabled(true);
        ui->close->setEnabled(false);
    }
        break;
    case PLAYER_PLAYING:
    {
        ui->progress->setRange(0, m_context.duration / 1000);
        ui->open->setEnabled(false);
        ui->close->setEnabled(true);
    }
        break;
    }


    m_nPlayerState = player_state;
}

void MainWindow::on_frame_arrived(void* pFrame)
{

    if(m_bNeedAVSync)
    {
        QMutexLocker locker(&m_locker);
        m_currentTimeStamp = ((PAV_FRAME)pFrame)->timestamp;

    }

    update_timestamp(((PAV_FRAME)pFrame)->timestamp);
    while(m_bWaitFirstAudioFrame)
    {
        PAV_PACKET pPacket = nullptr;
        if(m_bNeedAVSync)
            m_pMediaReader->GetStreamReader()->GetAudioPacket(pPacket, (m_currentTimeStamp));
        else
            m_pMediaReader->GetStreamReader()->GetAudioPacket(pPacket);


        if(pPacket == nullptr)
            break;

        m_audioHeadFrame.append(pPacket->data);

        delete pPacket;


        if(m_audioHeadFrame.size() >= 1800)
        {



            m_pAudioPlayer->SetCallbackFunc(zplayCallbackFunc, (TCallbackMessage) MsgStreamNeedMoreData, this);
            int ret = m_pAudioPlayer->OpenStream(1, 1, m_audioHeadFrame.data(), m_audioHeadFrame.size(), sfMp3);
            m_pAudioPlayer->Play();
            qDebug() << "m_pAudioPlayer->OpenStream" << ret << m_pAudioPlayer->GetError();
            m_bWaitFirstAudioFrame = false;
        }
    }

    qInfo() << "got u" << ((PAV_FRAME)pFrame)->y.size();
    m_pRender->SetFrame((PAV_FRAME)pFrame);
    delete (PAV_FRAME)pFrame;
    /*
qDebug() << "FrameArrive:" << currentTime;
    do
    {
    if(m_audioFrames.isEmpty())
        break;
    QAudioBuffer frame = m_audioFrames.front();
    qDebug() << "FrameArrive 1:" << frame.startTime();
    if(frame.startTime() > currentTime)
        break;

        m_pSpeakerIO->write((const char*)frame.data(), frame.byteCount());
        m_audioFrames.pop_front();



    }while(true);
    */
}

void MainWindow::on_audio_packet()
{

    while(m_bWaitFirstAudioFrame)
    {
        PAV_PACKET pPacket = nullptr;
        if(m_bNeedAVSync)
            m_pMediaReader->GetStreamReader()->GetAudioPacket(pPacket, FIX_TIMESTAMP(m_currentTimeStamp));
        else
            m_pMediaReader->GetStreamReader()->GetAudioPacket(pPacket);


        if(pPacket == nullptr)
            break;

        m_audioHeadFrame.append(pPacket->data);

        delete pPacket;


        if(m_audioHeadFrame.size() >= 1800)
        {



            m_pAudioPlayer->SetCallbackFunc(zplayCallbackFunc, (TCallbackMessage) MsgStreamNeedMoreData, this);
            int ret = m_pAudioPlayer->OpenStream(1, 1, m_audioHeadFrame.data(), m_audioHeadFrame.size(), sfMp3);

            m_pAudioPlayer->Play();
            qDebug() << "m_pAudioPlayer->OpenStream" << ret << m_pAudioPlayer->GetError();
            m_bWaitFirstAudioFrame = false;
        }
    }
    /*
    qDebug()  << "audio packet arrive";
    PAV_PACKET pPacket = nullptr;
    while(m_pMediaReader->GetStreamReader()->GetAudioPacket(pPacket))
    {

        qDebug() << "write audio packet" << m_audioDecoderBuffer.size();
        m_audioDecoderBuffer.buffer().append(pPacket->data);
            delete pPacket;

        m_pAudioDecoder->start();


        qDebug() << "write audio packet" << pPacket << pPacket->timestamp <<   m_pAudioDecoder->state();;


        qDebug() << "write audio packet finished" << m_audioDecoderBuffer.size();
    pPacket = nullptr;

    }
    */
}
void MainWindow::on_audio_frame()
{
    /*
    qDebug() << "MainWindow::on_audio_frame()";
    if(m_pAudioDecoder->bufferAvailable())
    m_audioFrames.push_back(m_pAudioDecoder->read());
    */
}

void MainWindow::on_video_decode_finshed()
{
      qInfo() << "on_video_decode_finished";
    m_bVideoDecoderFinished = true;

    if(m_bNeedAVSync)
    {
        QMutexLocker locker(&m_locker);
        m_currentTimeStamp = m_context.duration;

    }

    if(m_bAudioDecodeFinished && m_bVideoDecoderFinished)
    {
        on_close_player();
    }
}
void MainWindow::on_audio_decode_finished()
{
    qInfo() << "on_audio_decode_finished";
    m_bAudioDecodeFinished = true;
    if(m_bAudioDecodeFinished && m_bVideoDecoderFinished)
    {
        on_close_player();
    }
}
void MainWindow::on_close_player()
{
    if(m_nPlayerState != PLAYER_PLAYING)
        return;

    qInfo() <<"11111111111111111";
    if(m_pAudioPlayer != nullptr)
    {
        m_pAudioPlayer->Release();
        m_pAudioPlayer = nullptr;
    }
    qInfo() <<"111111111111111112";
    if(m_pStreamDecoder != nullptr)
    {
        m_pStreamDecoder->Release();
        m_pStreamDecoder = nullptr;
    }
    qInfo() <<"111111111111111113";
    if(m_pMediaReader != nullptr)
    {
        m_pMediaReader->Release();
        m_pMediaReader = nullptr;
    }
    qInfo() <<"111111111111111114";

    m_pRender->Clear();
    ui->time->setText("");
    ui->progress->setValue(0);
    ui->progress->setRange(0, 0);

    update_player_state(PLAYER_IDLE);
}

void MainWindow::update_timestamp(quint64 cur)
{
    if(m_context.duration <= 0)
        return;



    ui->progress->setValue(cur / 1000);


    QString time;
    if(m_context.duration < 3600 * 1000)
    {
        int play_secs = cur / 1000 % 60;
        int play_minutes = cur / 1000 / 60;
        int total_secs = m_context.duration / 1000 % 60;
        int total_minutes =  m_context.duration / 1000 / 60;
        time = QString::asprintf("%02d:%02d/%02d:%02d", play_minutes, play_secs, total_minutes, total_secs);
    }
    else
    {
        int play_secs = cur / 1000 % 60;
        int play_minutes = cur / 1000 / 60 % 60;
        int play_hours = cur / 1000 / 60 / 60;
        int total_secs = m_context.duration / 1000 % 60;
        int total_minutes =  m_context.duration / 1000 / 60 % 60;
        int total_hours = m_context.duration / 1000 / 60 / 60;
        time = QString::asprintf("%d:%02d:%02d/%d:%02d:%02d", play_hours, play_minutes, play_secs, total_hours, total_minutes, total_secs);

    }

    ui->time->setText(time);
}
void MainWindow::on_open_clicked()
{
    QStringList filters;
    filters << "Video files (*.flv)"
            << "Any files (*)";

    QString fileName = QFileDialog::getOpenFileName(this, tr("打开文件"),
                                                    QDir::currentPath() + "\\demos\\",
                                                    filters.join(";;"));

    if(!fileName.isEmpty())
    {
        m_pMediaReader = new FlvReader();

        qInfo() << "11111";
        m_pStreamDecoder = new StreamDecoder();
        qInfo() << "2222";

        /*
        QAudioFormat format;
        // Set up the format, eg.
        format.setSampleRate(44100);
        format.setChannelCount(2);
        format.setSampleSize(32);
        format.setCodec("audio/pcm");
        format.setByteOrder(QAudioFormat::LittleEndian);
        format.setSampleType(QAudioFormat::UnSignedInt);

        m_pSpeaker = new QAudioOutput(format, this);
        //connect(audio, SIGNAL(stateChanged(QAudio::State)), this, SLOT(handleStateChanged(QAudio::State)));
        m_pSpeakerIO = m_pSpeaker->start();
                m_pAudioDecoder = new QAudioDecoder();
                m_pAudioDecoder->setAudioFormat(format);
                m_pAudioDecoder->setSourceDevice(&m_audioDecoderBuffer);
m_audioDecoderBuffer.open(QIODevice::ReadWrite);
                //m_pAudioDecoder->setSourceFilename("C:\\Users\\Public\\fans_qinghuaci.mp3");

                m_pAudioDecoder->start();
                qDebug() << "Audio Decoder:" << m_pAudioDecoder->state();
*/
        connect(m_pStreamDecoder, SIGNAL(frameArrived(void*)), this, SLOT(on_frame_arrived(void*)));
        connect((FlvReader*)m_pMediaReader, SIGNAL(new_audio_packet()), this, SLOT(on_audio_packet()));
        connect(m_pAudioDecoder, SIGNAL(bufferReady()), this, SLOT(on_audio_frame()));
        connect(this, SIGNAL(audioDecodeFinished()), this, SLOT(on_audio_decode_finished()));
        connect(m_pStreamDecoder, SIGNAL(decodeFinished()), this, SLOT(on_video_decode_finshed()));

        m_pMediaReader->Open(fileName);
        m_pMediaReader->GetMediaContext(m_context);

        m_pStreamDecoder->SetStream(m_pMediaReader->GetStreamReader());
        m_pStreamDecoder->RunDecoder();

        m_pAudioPlayer = CreateZPlay();
        m_bNeedAVSync = true;
        m_bWaitFirstAudioFrame = true;
        m_currentTimeStamp = 0;

        m_bVideoDecoderFinished = false;
        m_bAudioDecodeFinished = false;
        update_player_state(PLAYER_PLAYING);
    }
}

void MainWindow::on_close_clicked()
{
    on_close_player();
}
