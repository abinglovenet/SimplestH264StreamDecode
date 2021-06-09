#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include <QAudioDecoder>
#include <QAudioOutput>
#include <QList>
#include <QBuffer>

#include "imediareader.h"
#include "streamdecoder.h"
#include "flvreader.h"
#include "qimagerender.h"
#include "libzplay\libzplay.h"
#include "qdirectxrender.h"

using namespace libZPlay;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE


enum PLAYER_STATE
{
    PLAYER_IDLE = 0,
    PLAYER_PLAYING,
    PLAYER_PAUSE
};


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();


    void NotifyAudioDecodeFinished();
    void TransferAudioDataToZPlay();
protected:
    IMediaReader* m_pMediaReader;
    StreamDecoder* m_pStreamDecoder;
    QImageRender* m_pRender;
    QDirectXRender* m_pDirectXRender;
    bool m_bVideoDecoderFinished;

    ZPlay *m_pAudioPlayer;
    bool m_bNeedAVSync;
    quint64 m_currentTimeStamp;
    QByteArray m_audioHeadFrame;
    QAudioOutput* m_pSpeaker;
    QIODevice* m_pSpeakerIO;
    QAudioDecoder* m_pAudioDecoder;
    QBuffer m_audioDecoderBuffer;
    bool m_bAudioDecodeFinished;

    QList<PAV_PACKET> m_audioFrames;

    QMutex m_locker;
    int m_nPlayerState;

    MEDIA_CONTEXT m_context;
    //test
    //QFile file;
signals:
    void audioDecodeFinished();
    void update_timestamp(quint64 cur);
public slots:

    // video frame notify
    void on_first_frame_arrived();
    void on_frame_arrived(void* pFrame);

    // audio frame notify
    void on_audio_packet();
    void on_audio_frame();


    void on_video_decode_finshed();
    void on_audio_decode_finished();
    void on_update_timestamp(quint64 cur);
private slots:
    void on_open_clicked();
    void on_close_player();

    void update_player_state(int player_state);
    void on_close_clicked();


    void on_fullscreen_clicked();

protected:
    virtual bool eventFilter(QObject *obj, QEvent *event);
    virtual void	resizeEvent(QResizeEvent * event);
    virtual void	closeEvent(QCloseEvent * event);
private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
