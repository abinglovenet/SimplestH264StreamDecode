#include "mainwindow.h"

#include <QApplication>
//#define ENABLE_MSG 1
void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
#ifdef ENABLE_MSG
    //QByteArray localMsg = msg.toLocal8Bit();
    switch (type) {
    case QtDebugMsg:
        //fprintf(stderr, "Debug: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtInfoMsg:
        //OutputDebugString(msg.toStdWString().c_str());
        //fprintf(stderr, "Info:%llu  %s (%s:%u, %s)\n", GetTickCount64(), localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtWarningMsg:
        //fprintf(stderr, "Warning: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtCriticalMsg:
        //fprintf(stderr, "Critical: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtFatalMsg:
        //fprintf(stderr, "Fatal: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        abort();
    }
#endif
}

int main(int argc, char *argv[])
{
    qInstallMessageHandler(myMessageOutput);
    //qInstallMessageHandler(0);
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
