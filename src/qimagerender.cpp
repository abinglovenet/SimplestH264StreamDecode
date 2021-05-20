#include "qimagerender.h"
#include <QFile>

#define GL_VERSION  "#version 330 core\n"
#define GET_GLSTR(x) GL_VERSION#x

static int VideoWidth=640;
static int VideoHeight=340;


const char *pstrVertexShader = GET_GLSTR(

            layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

void main()
{
    gl_Position = vec4(aPos, 1.0);
    TexCoord = aTexCoord;
}

);

const char *pstrFragmentShader =GET_GLSTR(

            out vec4 FragColor;
        in vec2 TexCoord;
uniform sampler2D texMap;
uniform sampler2D texMapUV;



highp mat3 yuv2rgb = mat3(1, 1, 1,
                          0, -0.34414, 1.772,
                          1.402, -0.71414, 0);

void main()
{



    FragColor = vec4(texture2D(texMap, TexCoord).rgb, 1.0);


}
);

QImageRender::QImageRender(QWidget *parent)
    : QOpenGLWidget(parent)
{
    m_pTexture = nullptr;
}

QImageRender::~QImageRender()
{
    makeCurrent();

    m_vboDisplayRegion.destroy();
    m_vboTexCoord.destroy();
    delete m_pTexture;

    delete m_pShaderProgram;

}

void QImageRender::Clear()
{

    SetFrame(nullptr);
}

void QImageRender::SetFrame(PAV_FRAME pFrame)
{
#ifdef TRACE_IMAGE
    qInfo() << "111111112423424" << pFrame->width << pFrame->height;
    static QFile file("");
    QString fileName = QString::asprintf("C:\\Users\\Public\\temp\\%x.rgb", &pFrame->data);
    file.setFileName(fileName);
    file.open(QIODevice::ReadWrite);
    file.write(pFrame->data);
    file.close();
    return;
#endif



    makeCurrent();
    if(!pFrame)
    {
        delete m_pTexture;
        m_pTexture = nullptr;
        return;
    }

    if(m_pTexture && m_pTexture->width() != pFrame->width)
    {

        delete m_pTexture;
        m_pTexture = nullptr;
    }
    if(m_pTexture == nullptr && pFrame->width > 0 && pFrame->height > 0)
    {
        m_pTexture = new QOpenGLTexture(QOpenGLTexture::Target2D);
        m_pTexture->setFormat(QOpenGLTexture::RGB8_UNorm);

        m_pTexture->setSize(pFrame->width, pFrame->height, 1);

        m_pTexture->allocateStorage();

        m_pTexture->setMipBaseLevel(0);
        m_pTexture->setMipMaxLevel(10);
        // 设置最近的过滤模式，以缩小纹理
        m_pTexture->setMinificationFilter(QOpenGLTexture::NearestMipMapNearest); //滤波
        qInfo() << "11114------";
        // 设置双线性过滤模式，以放大纹理
        m_pTexture->setMagnificationFilter(QOpenGLTexture::NearestMipMapNearest);
        qInfo() << "11115------";
        // 重复使用纹理坐标
        m_pTexture->setWrapMode(QOpenGLTexture::Repeat);
        qInfo() << "11116------";

    }


    qInfo() << "begin texture - data" << pFrame->data.size();
    if(m_pTexture)
        m_pTexture->setData(QOpenGLTexture::RGB, QOpenGLTexture::UInt8, pFrame->data.data());
    //m_pTextureUV->setData(QOpenGLTexture::RG, QOpenGLTexture::UInt8, pFrame->uv.data());
    qInfo() << "end texture - data";
    update();
}

void QImageRender::initializeGL()
{
    initializeOpenGLFunctions();

    glViewport(0, 0, 100, 100);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);



    float vertexCoords[6][3] = {
        {-1.0, 1.0, -1.0},
        {1, 1, -1},
        {-1, -1, -1},
        {-1, -1, -1},
        {1, -1, -1},
        {1, 1, -1}

    };

    float texCoords[6][2] = {
        {0, 0},
        {1, 0},
        {0, 1},
        {0, 1},
        {1, 1},
        {1, 0}

    };

    qDebug() << pstrVertexShader;
    qDebug() << pstrFragmentShader;

    m_pShaderProgram = new  QOpenGLShaderProgram(this);
    m_pShaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, pstrVertexShader);
    bool bRet = m_pShaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, pstrFragmentShader);
    m_pShaderProgram->link();
    m_pShaderProgram->bind();


    m_vboDisplayRegion.create();
    m_vboDisplayRegion.bind();
    m_vboDisplayRegion.allocate(vertexCoords, sizeof(vertexCoords));
    m_vboDisplayRegion.setUsagePattern(QOpenGLBuffer::StaticDraw);  //设置为一次修改，多次使用(坐标不变,变得只是像素点)


    m_pShaderProgram->setAttributeBuffer(0, GL_FLOAT, 0, 3, 0);
    m_pShaderProgram->enableAttributeArray(0); //使能


    m_vboTexCoord.create();
    m_vboTexCoord.bind();
    m_vboTexCoord.allocate(texCoords, sizeof(texCoords));
    m_vboTexCoord.setUsagePattern(QOpenGLBuffer::StaticDraw);  //设置为一次修改，多次使用(坐标不变,变得只是像素点)

    m_pShaderProgram->setAttributeBuffer(1, GL_FLOAT, 0, 2, 0);
    m_pShaderProgram->enableAttributeArray(1);


    glActiveTexture(GL_TEXTURE0);

    m_pShaderProgram->setUniformValue("texMap", 0);

}
void QImageRender::resizeGL(int w, int h)
{
    //glViewport(0, 0, w, h);
}
void QImageRender::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);


    if(m_pTexture != nullptr)
    {
        m_pTexture->bind(0);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        m_pTexture->release();
    }

    qDebug() << "QImageRender::paintGL()";

}
