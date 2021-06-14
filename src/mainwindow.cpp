#include <QTime>
#include <QProcess>
#include <QMessageBox>

#include "mainwindow.h"
#include "ui_mainwindow.h"


#define HIDE_PLAYBAR_MOUSE_IDLE_DURATION 10000
#define MAX_TIMESTAMP(x) (x + 900)
#define MIN_TIMESTAMP(x) ((x > 360) ? (x - 360) : 0)

int  __stdcall  zplayCallbackFunc(void* instance, void *user_data, TCallbackMessage message, unsigned int param1, unsigned int param2)
{
    MainWindow* processor = (MainWindow*) user_data;

    switch(message)
    {
    case MsgStreamNeedMoreData: // stream needs more data
    {
        processor->TransferAudioDataToZPlay();
        return 2;

    }
        break;
    case MsgStop:
    {
        processor->NotifyAudioDecodeFinished();
    }
        break;
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

    m_pDirectXRender = new QDirectXRender(ui->centralwidget);
    ui->centralwidget->layout()->addWidget(m_pDirectXRender);

    HWND hWnd = (HWND)m_pDirectXRender->winId();
    ::InitDirectX(hWnd);


    m_pDirectXRender->ResetRender();
    m_pDirectXRender->installEventFilter(this);
    ui->controlWidget->installEventFilter(this);
    //m_pRender = new QImageRender(ui->centralwidget);
    //ui->centralwidget->layout()->addWidget(m_pRender);

    m_pMediaReader = nullptr;
    m_pStreamDecoder = nullptr;
    m_pAudioDecoder = nullptr;
    m_pSpeaker = nullptr;
/*
    file.setFileName("C:\\Users\\Public\\test.mp3");
    file.open(QIODevice::ReadWrite);
*/
    m_InitCursor = cursor();
    update_player_state(PLAYER_IDLE);
}

MainWindow::~MainWindow()
{
    ui->centralwidget->layout()->removeWidget(m_pDirectXRender);
    delete m_pDirectXRender;
    ::UnInitDirectX();
    delete ui;
}

void MainWindow::NotifyAudioDecodeFinished()
{
    emit audioDecodeFinished();
}

void MainWindow::TransferAudioDataToZPlay()
{
    ////qInfo() << "TransferAudioDataToZPlay";
    if(m_nPlayerState != PLAYER_PLAYING)
        return;

    PAV_PACKET pPacket = nullptr;


    {
        QMutexLocker locker(&m_locker);

        // Need to further handle the exception
        if(m_audioFrames.isEmpty())
        {
            if(m_pMediaReader->GetStreamReader()->IsAudioStreamEnd())
            {
                m_pAudioPlayer->PushDataToStream(NULL, 0);

            }
            return;
        }
        for(int i = 0; i < m_audioFrames.size(); i++)
        {
            pPacket = m_audioFrames.at(i);
            m_pAudioPlayer->PushDataToStream(pPacket->data.data(), pPacket->data.size());
            delete pPacket;
        }

        m_audioFrames.clear();
    }
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_pDirectXRender) {
        if (event->type() == QEvent::MouseButtonDblClick) {
            on_fullscreen_clicked();
        }
    }

    if((event->type() <= QEvent::MouseMove && event->type() >= QEvent::MouseButtonPress) ||
            (event->type() == QEvent::Enter) ||
            (event->type() == QEvent::Move))
    {
        m_nLastMouseEventTick = ::GetTickCount();
    }

    qInfo() <<"event Filter" << m_nLastMouseEventTick << event->type();

    return false;
}
void MainWindow::closeEvent(QCloseEvent * event)
{
    on_close_player();
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

void MainWindow::on_first_frame_arrived()
{

}

void MainWindow::on_frame_arrived(void* pFrame)
{
    if(m_nPlayerState == PLAYER_IDLE)
    {
        delete (PAV_FRAME)pFrame;
        return;
    }


    {
        QMutexLocker locker(&m_locker);
        m_currentTimeStamp = ((PAV_FRAME)pFrame)->timestamp;

    }
    emit update_timestamp(m_currentTimeStamp);

    m_pDirectXRender->SetFrame((PAV_FRAME)pFrame);

    delete (PAV_FRAME)pFrame;


}

void MainWindow::on_audio_packet()
{


}

void MainWindow::on_audio_frame()
{
    /*
    //qDebug() << "MainWindow::on_audio_frame()";
    if(m_pAudioDecoder->bufferAvailable())
    m_audioFrames.push_back(m_pAudioDecoder->read());
    */
}

void MainWindow::on_video_decode_finshed()
{
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

    m_pStreamDecoder->DisableOutputFrame();

   // m_pRender->Clear();
    m_pDirectXRender->Clear();
    ui->time->setText("");
    ui->progress->setValue(0);
    ui->progress->setRange(0, 0);

    if(m_pAudioPlayer != nullptr)
    {

        m_pAudioPlayer->Close();
        m_pAudioPlayer->Release();
        m_pAudioPlayer = nullptr;
    }

    if(m_pStreamDecoder != nullptr)
    {
        m_pStreamDecoder->Release();
        m_pStreamDecoder = nullptr;
    }

    if(m_pMediaReader != nullptr)
    {
        m_pMediaReader->Release();
        m_pMediaReader = nullptr;
    }

    m_audioHeadFrame.clear();
    for(int i = 0; i < m_audioFrames.size(); i++)
        delete m_audioFrames.at(i);
    m_audioFrames.clear();

    m_pDirectXRender->setMouseTracking(false);
    setMouseTracking(false);
    m_clock.stop();
    disconnect(&m_clock, SIGNAL(timeout()), this, SLOT(on_clock_notify()));
    ui->controlWidget->show();
    setCursor(m_InitCursor);
    update_player_state(PLAYER_IDLE);
}

void MainWindow::on_clock_notify()
{

    if(ui->controlWidget->isFloating())
    {
        //qInfo() << "Clock" << GetTickCount() << m_nLastMouseEventTick << (GetTickCount() - m_nLastMouseEventTick);
        if((GetTickCount() - m_nLastMouseEventTick) >= HIDE_PLAYBAR_MOUSE_IDLE_DURATION)
        {
            ui->controlWidget->hide();
            if(isFullScreen())
            {
                setCursor(QCursor(Qt::BlankCursor));
            }
        }
        else
        {
            ui->controlWidget->show();
            setCursor(m_InitCursor);

        }

    }
}
void MainWindow::on_update_timestamp(quint64 cur)
{
    if(m_context.duration <= 0)
        return;


    if(m_nPlayerState != PLAYER_PLAYING)
        return;

    PAV_PACKET pPacket = nullptr;

    quint64 maxTimeStamp = 0;
    quint64 minTimeStamp = 0;

    if(m_bNeedAVSync)
    {
        QMutexLocker locker(&m_locker);
        maxTimeStamp = MAX_TIMESTAMP(m_currentTimeStamp);
        minTimeStamp = MIN_TIMESTAMP(m_currentTimeStamp);
    }

    while(true)
    {
        QMutexLocker locker(&m_locker);
        if(m_bNeedAVSync)
        {
            m_pMediaReader->GetStreamReader()->GetAudioPacket(pPacket, maxTimeStamp);
        }
        else
            m_pMediaReader->GetStreamReader()->GetAudioPacket(pPacket);

        if(pPacket == nullptr)
            break;

        m_audioFrames.push_back(pPacket);
        pPacket = nullptr;




        int i = 0;
        for(i = 0; i < m_audioFrames.size(); i++)
        {
            if(m_audioFrames.at(i)->timestamp >= minTimeStamp)
                break;

            delete m_audioFrames.at(i);
        }

        while(i-- > 0)
        {
            m_audioFrames.takeFirst();
        }
    }



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


DWORD RunProcess ( LPCTSTR lpAppPath, LPWSTR lpCmdLine, LPCTSTR lpCurDir)
{
    OutputDebugString(lpCmdLine);
    OutputDebugString(lpCurDir);
    STARTUPINFO startupInfo;
    PROCESS_INFORMATION	processInformation;
    ZeroMemory( &startupInfo, sizeof( STARTUPINFO ));
    startupInfo.cb = sizeof( STARTUPINFO );
    ZeroMemory( &processInformation, sizeof( PROCESS_INFORMATION ));

    DWORD dwExitCode = DWORD(-1);
    if ( ::CreateProcess(	lpAppPath,					// lpszImageName
        lpCmdLine,							// lpszCommandLine
        0,									// lpsaProcess
        0,									// lpsaThread
        FALSE,								// fInheritHandles
        CREATE_SUSPENDED,					// fdwCreate
        0,									// lpvEnvironment
        lpCurDir,							// lpszCurDir
        &startupInfo,						// lpsiStartupInfo
        &processInformation					// lppiProcInfo
        ))
    {
        ::ResumeThread(processInformation.hThread);

        WaitForSingleObject( processInformation.hProcess, INFINITE );

        GetExitCodeProcess( processInformation.hProcess, &dwExitCode );
    }
    else
    {
        dwExitCode = GetLastError();
    }

    return dwExitCode;

}
void MainWindow::on_open_clicked()
{
    QStringList filters;
    filters << "Video files (*.flv *.mp4 *.wmv *.rmvb *.mkv)"
            << "Any files (*)";

    QString fileName = QFileDialog::getOpenFileName(this, tr("打开文件"),
                                                    QDir::currentPath() + "\\demos\\",
                                                    filters.join(";;"));

    if(fileName.isEmpty())
        return;
    QFileInfo info(fileName);
    if(info.suffix().compare("flv", Qt::CaseInsensitive) != 0)
    {
        QString pathConverter = QDir::currentPath() + "/ffmpeg/ffmpeg.exe";
        QStringList param;
        param <<QString("\"") + pathConverter + QString("\"") << "-i" << QString("\"") + fileName + QString("\"") << "-y" << "-vcodec" << "h264" << "-acodec" << "libmp3lame" << QString("\"") + QDir::currentPath() + "/demos/" + info.completeBaseName() + ".flv\"";


        int errorcode = 0;

        TCHAR szCmdLine[1024] = {0};
        QString paramString = param.join(QString(" "));
        _tcscpy(szCmdLine, paramString.toStdWString().c_str());

        if((errorcode = RunProcess(NULL, szCmdLine, QDir::currentPath().toStdWString().c_str())))
        {
            QString error("Transcoding failed:%1");
            QMessageBox::warning(this, "Waring", error.arg(errorcode));
            return;
        }

        qInfo() <<"transcode finished";
        fileName =  QDir::currentPath() + "/demos/" + info.completeBaseName() + ".flv";
    }


    if(!fileName.isEmpty())
    {
        m_pMediaReader = new FlvReader();

        m_pStreamDecoder = new StreamDecoder();


        connect(this, SIGNAL(update_timestamp(quint64)), this, SLOT(on_update_timestamp(quint64)));
        connect(m_pStreamDecoder, SIGNAL(frameArrived(void*)), this, SLOT(on_frame_arrived(void*)), Qt::DirectConnection);
        connect((FlvReader*)m_pMediaReader, SIGNAL(new_audio_packet()), this, SLOT(on_audio_packet()));
        connect(m_pAudioDecoder, SIGNAL(bufferReady()), this, SLOT(on_audio_frame()));
        connect(this, SIGNAL(audioDecodeFinished()), this, SLOT(on_audio_decode_finished()));
        connect(m_pStreamDecoder, SIGNAL(decodeFinished()), this, SLOT(on_video_decode_finshed()));
        connect(m_pStreamDecoder, SIGNAL(firstFrameNotify()), this, SLOT(on_first_frame_arrived()));


        m_pMediaReader->Open(fileName);
        m_pMediaReader->GetMediaContext(m_context);

        m_pAudioPlayer = CreateZPlay();
        m_pAudioPlayer->SetSettings(sidWaveBufferSize, 900);

        while(true)
        {
            PAV_PACKET pPacket = nullptr;

            m_pMediaReader->GetStreamReader()->GetAudioPacket(pPacket);


            if(pPacket == nullptr)
            {
                continue;
            }

            m_audioHeadFrame.append(pPacket->data);

            delete pPacket;


            if(m_pAudioPlayer->OpenStream(1, 1, m_audioHeadFrame.data(), m_audioHeadFrame.size(), sfMp3))
            {
                m_pAudioPlayer->SetCallbackFunc(zplayCallbackFunc, (TCallbackMessage) (MsgStreamNeedMoreData | MsgStop) , this);
                break;
            }

        }
        m_pAudioPlayer->Play();


        m_pStreamDecoder->SetStream(m_pMediaReader->GetStreamReader());
        m_pStreamDecoder->SetFrameRate(m_context.framerate);

        m_pStreamDecoder->RunDecoder();




        m_bNeedAVSync = true;
        m_currentTimeStamp = 0;

        m_bVideoDecoderFinished = false;
        m_bAudioDecodeFinished = false;
        update_player_state(PLAYER_PLAYING);

        connect(&m_clock, SIGNAL(timeout()), this, SLOT(on_clock_notify()));
        m_clock.start(1000);
        setMouseTracking(true);
        m_pDirectXRender->setMouseTracking(true);
    }
}

void MainWindow::on_close_clicked()
{
    on_close_player();
}

void MainWindow::on_fullscreen_clicked()
{
    if(this->isFullScreen())
    {
        showNormal();
    }
    else
    {
        showFullScreen();
    }
}
