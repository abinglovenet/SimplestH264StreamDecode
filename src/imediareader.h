#ifndef IMEDIAREADER_H
#define IMEDIAREADER_H

#include <QObject>
#include "istreamreader.h"
typedef struct
{
    quint64 duration;
    quint64 framerate;
}MEDIA_CONTEXT, *PMEDIA_CONTEXT;

class IMediaReader
{

public:
    IMediaReader();

public:
    virtual bool Open(QString path) = 0;
    virtual void Close() = 0;

    virtual bool GetMediaContext(MEDIA_CONTEXT &context) = 0;
    virtual IStreamReader* GetStreamReader() = 0;

    virtual bool Seek(qlonglong llpos) = 0;

    virtual void Release() = 0;

};

#endif // IMEDIAREADER_H
