#ifndef QIMAGERENDER_H
#define QIMAGERENDER_H


#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShader>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QDebug>
#include "istreamreader.h"

class QImageRender : public QOpenGLWidget,  protected QOpenGLFunctions
{
public:
    QImageRender(QWidget *parent = nullptr);
    ~QImageRender();

    void SetFrame(PAV_FRAME pFrame);
    void Clear();

    QOpenGLBuffer m_vboDisplayRegion;
    QOpenGLBuffer m_vboTexCoord;

    QOpenGLTexture* m_pTexture;
    QOpenGLShaderProgram* m_pShaderProgram;
protected:
    virtual void initializeGL();
    virtual void resizeGL(int w, int h);
    virtual void paintGL();
};

#endif // QIMAGERENDER_H
