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
    m_pTextureUV = nullptr;
}

QImageRender::~QImageRender()
{
    makeCurrent();
    delete program;
    vbo.destroy();
    delete m_pTexture;

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
    QString fileName = QString::asprintf("C:\\Users\\Public\\temp\\%x.rgb", &pFrame->y);
    file.setFileName(fileName);
    file.open(QIODevice::ReadWrite);
    file.write(pFrame->y);
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

        //m_pTexture->setData(QImage("C:\\Users\\Public\\temp\\93675b8.png"));
        qInfo() << "1111------";
        m_pTexture->setSize(pFrame->width, pFrame->height, 1);
        qInfo() << "11112------";
        m_pTexture->allocateStorage();
        qInfo() << "11113------";

        m_pTexture->setMipBaseLevel(0);
        m_pTexture->setMipMaxLevel(10);
        // 设置最近的过滤模式，以缩小纹理
        m_pTexture->setMinificationFilter(QOpenGLTexture::NearestMipMapNearest); //滤波
        qInfo() << "11114------";
        // 设置双线性过滤模式，以放大纹理
        m_pTexture->setMagnificationFilter(QOpenGLTexture::NearestMipMapNearest);
        qInfo() << "11115------";
        // 重复使用纹理坐标
        // 纹理坐标(1.1, 1.2)与(0.1, 0.2)相同
        m_pTexture->setWrapMode(QOpenGLTexture::Repeat);
        qInfo() << "11116------";

    }
    /*
    if(m_pTextureUV && m_pTextureUV->width() != pFrame->width / 2)
    {
        delete m_pTextureUV;
        m_pTextureUV = nullptr;
    }
    if(m_pTextureUV == nullptr)
    {
        m_pTextureUV = new QOpenGLTexture(QOpenGLTexture::Target2D);
        m_pTextureUV->setFormat(QOpenGLTexture::RG8U);

        //m_pTexture->setData(QImage("C:\\Users\\Public\\temp\\93675b8.png"));
        m_pTextureUV->setSize(pFrame->width / 2, pFrame->height / 2, 1);
        m_pTextureUV->allocateStorage();

        // 设置最近的过滤模式，以缩小纹理
        m_pTextureUV->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear); //滤波
        // 设置双线性过滤模式，以放大纹理
        m_pTextureUV->setMagnificationFilter(QOpenGLTexture::Linear);
        // 重复使用纹理坐标
        // 纹理坐标(1.1, 1.2)与(0.1, 0.2)相同
        m_pTextureUV->setWrapMode(QOpenGLTexture::Repeat);

    }
    */

    qInfo() << "begin texture - data" << pFrame->y.size();
    if(m_pTexture)
        m_pTexture->setData(QOpenGLTexture::RGB, QOpenGLTexture::UInt8, pFrame->y.data());
    //m_pTextureUV->setData(QOpenGLTexture::RG, QOpenGLTexture::UInt8, pFrame->uv.data());
    qInfo() << "end texture - data";
    update();
}

void QImageRender::initializeGL()
{
    initializeOpenGLFunctions();

    glViewport(0, 0, 100, 100);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    /*
    const char* pstrVertexShader =
   "#version 330 core\nlayout(location = 0) in vec4 position;\r\n\
    layout(location = 1) in vec2 textCoord; // 纹理坐标\r\n\
    varying vec2 fTexCoord;\r\n\
    void main()\r\n\
    {\r\n\
        gl_Position = position;\r\n\
        fTexCoord = textCoord;\r\n\
    }";

    const char* pstrFragmentShader= "#version 330 core\nprecision mediump float;\r\n\
    varying vec4 fColor;\r\n\
    varying vec2 fTexCoord;\r\n\
    uniform sampler2D texMap;\r\n\
    void main()\r\n\
    {\r\n\
        gl_FragColor = vec4(1.0, 0.0,  0.0, 1.0);\r\n\
        //gl_FragColor = texture2D(texMap, fTexCoord);\r\n\
    }";
*/
    GLfloat points[] = {1.0,0.0,0.0,1.0,
                        0.0,1.0,0.0,1.0,
                        1.0,1.0,0.0,1.0};

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
    /*
    float texCoords[6][2] = {
        {0, 1},
        {1, 1},
        {0, 0},
        {0, 0},
        {1, 0},
        {1, 1}

    };
*/
    qDebug() << pstrVertexShader;
    qDebug() << pstrFragmentShader;

    program = new  QOpenGLShaderProgram(this);
    program->addShaderFromSourceCode(QOpenGLShader::Vertex, pstrVertexShader);
    bool bRet = program->addShaderFromSourceCode(QOpenGLShader::Fragment, pstrFragmentShader);
    program->link();
    program->bind();

    /*
    //定点着色器
    GLuint vertextShader;
    vertextShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertextShader, 1, &pstrVertexShader, NULL);
    glCompileShader(vertextShader);

    //用于判断一个着色器是否编译成功
    GLint success;
    GLchar infoLog[1024];
    glGetShaderiv(vertextShader,GL_COMPILE_STATUS,&success);
    if(!success){
        glGetShaderInfoLog(vertextShader,512,NULL,infoLog);
        qDebug()<<"vertextShader COMPILE FAILED" << infoLog;
    }

    if(success)
    {
         qDebug()<<"vertexShader COMPILE success";
    }
    //像素着色器
    GLuint fragmentShader;
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(fragmentShader, 1, &pstrFragmentShader, NULL);
    glCompileShader(fragmentShader);

    //用于判断一个着色器是否编译成功
    glGetShaderiv(fragmentShader,GL_COMPILE_STATUS,&success);
    if(!success){
        glGetShaderInfoLog(fragmentShader,512,NULL,infoLog);
        qDebug()<<"fragmentShader COMPILE FAILED" << infoLog;
    }

    if(success)
    {
         qDebug()<<"fragmentShader COMPILE success";
    }
    //创建一个着色器程序对象：多个着色器最后链接的版本
    GLuint shaderProgram;
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram,vertextShader);
    glAttachShader(shaderProgram,fragmentShader);
    glLinkProgram(shaderProgram);


    glValidateProgram(shaderProgram);


    glGetProgramiv(shaderProgram, GL_VALIDATE_STATUS, &success);
    if(success)
    {
        qDebug() << "success";
    }
    glGetProgramInfoLog(shaderProgram,1024,NULL,infoLog);
    qDebug() << "program:"<<infoLog;

    glUseProgram(shaderProgram);
*/
    /*
    GLsizei size;
    GLuint shaders[2];
    glGetAttachedShaders(shaderProgram, 2, &size, shaders);
    qDebug() << "Attach shaders:" << size;
    */

    /*
    GLuint vertexBuffer[2];
    glGenBuffers(2, vertexBuffer);
    glBindBuffer(GL_VERTEX_ARRAY, vertexBuffer[0]);
    glBufferData(GL_VERTEX_ARRAY, 72, points, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);


    glBindBuffer(GL_VERTEX_ARRAY, vertexBuffer[1]);
    glBufferData(GL_VERTEX_ARRAY, 48, texCoords, GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
*/

    vbo.create();
    vbo.bind();
    vbo.allocate(vertexCoords, sizeof(vertexCoords));
    vbo.setUsagePattern(QOpenGLBuffer::StaticDraw);  //设置为一次修改，多次使用(坐标不变,变得只是像素点)

    //初始化VAO,设置顶点数据状态(顶点，法线，纹理坐标等)
    //vao.create();
    //vao.bind();

    // void setAttributeBuffer(int location, GLenum type, int offset, int tupleSize, int stride = 0);
    program->setAttributeBuffer(0, GL_FLOAT, 0,                  3, 0);   //设置aPos顶点属性
    program->enableAttributeArray(0); //使能


    vbo1.create();
    vbo1.bind();
    vbo1.allocate(texCoords, sizeof(texCoords));
    vbo1.setUsagePattern(QOpenGLBuffer::StaticDraw);  //设置为一次修改，多次使用(坐标不变,变得只是像素点)

    program->setAttributeBuffer(1, GL_FLOAT, 0, 2, 0);   //设置aPos顶点属性
    program->enableAttributeArray(1); //使能
    //program->enableAttributeArray(1);

    // glGetProgramiv(shaderProgram, GL_ACTIVE_ATTRIBUTES, &number);

    // glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    // glGetAttachedShaders(shaderProgram, 2, &size, shaders);
    //qDebug() << "Attach shaders:" << size << "active attrs:" << number << " link:" << success;
    /*
    GLuint texture;
    glGenTextures(1, &texture);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(glGetUniformLocation(program->programId(), "texMap"), 0);

*/
    glActiveTexture(GL_TEXTURE0);
    //glActiveTexture(GL_TEXTURE1);




    // m_pTexture->setData(QImage("C:\\Users\\Public\\temp\\93675b8.png"));
    // m_pTexture = new QOpenGLTexture(QImage("C:\\Users\\Public\\temp\\93675b8.png"));
    //qDebug() << "Texture:" << m_pTexture->format() << m_pTexture->width();

    //m_pTexture->setSize(1280, 720);
    //m_pTexture->setFormat(QOpenGLTexture::TextureFormat::RGB8U);
    //m_pTexture->allocateStorage();



    program->setUniformValue("texMap", 0);
    //program->setUniformValue("texMapUV", 1);



    //m_pTexture->setMagnificationFilter();


    //为当前环境初始化OpenGL函数
    // initializeOpenGLFunctions();



    /*
       //初始化纹理对象
       for(int i=0;i<3;i++)
       {

           m_textureYUV[i]  = new QOpenGLTexture(QOpenGLTexture::Target2D);

           if(i==0)
           {
                   m_textureYUV[i]->setSize(VideoWidth,VideoHeight);
           }
           else
           {
                   m_textureYUV[i]->setSize(VideoWidth/2,VideoHeight/2);
           }

           m_textureYUV[i]->setMinMagFilters(QOpenGLTexture::LinearMipMapLinear,QOpenGLTexture::Linear);
           m_textureYUV[i]->create();
           m_textureYUV[i]->setFormat(QOpenGLTexture::R8_UNorm);
           m_textureYUV[i]->allocateStorage();        //存储配置(放大缩小过滤、格式、size)
           m_textureYUV[i]->setData(QOpenGLTexture::Red,QOpenGLTexture::UInt8,yuvArr[i]);

       }
  */
    /*
      QOpenGLShaderProgram* program = new  QOpenGLShaderProgram(this);
      program->addShaderFromSourceCode(QOpenGLShader::Fragment, vsrc);
      program->addShaderFromSourceCode(QOpenGLShader::Vertex, fsrc);
      program->link();
      program->bind();


      float vertexCoords[6][3] = {
          {-1.0, 1.0, -1.0},
          {1, 1, -1},
          {-1, -1, -1},
          {-1, -1, -1},
          {1, -1, -1},
          {1, 1, -1}

      };
*/
    /* OK
      initializeOpenGLFunctions();

         glClearColor(1.0f, 1.0f, 1.0f, 1.0f);    //设置背景色为白色




         program = new  QOpenGLShaderProgram(this);

         program->addShaderFromSourceCode(QOpenGLShader::Fragment,fsrc);

         program->addShaderFromSourceCode(QOpenGLShader::Vertex,vsrc);

         program->link();
         program->bind();


         //初始化VBO,将顶点数据存储到buffer中,等待VAO激活后才能释放

         float vertices[] = {
              //顶点坐标               //纹理坐标的Y方向需要是反的,因为opengl中的坐标系是Y原点位于下方
             -1.0f, -1.0f, 0.0f,  0.0f, 1.0f,        //左下
             1.0f , -1.0f, 0.0f,  1.0f, 1.0f,        //右下
             -1.0f, 1.0f,  0.0f,  0.0f, 0.0f,        //左上
             1.0f,  1.0f,  0.0f,  1.0f, 0.0f         //右上

         };

         float vertexCoords[6][3] = {
             {-1.0, 1.0, -1.0},
             {1, 1, -1},
             {-1, -1, -1},
             {-1, -1, -1},
             {1, -1, -1},
             {0, 1, -1}

         };


         vbo.create();
         vbo.bind();
         vbo.bind();              //绑定到当前的OpenGL上下文,
         vbo.allocate(vertexCoords, sizeof(vertexCoords));
         vbo.setUsagePattern(QOpenGLBuffer::StaticDraw);  //设置为一次修改，多次使用(坐标不变,变得只是像素点)

         //初始化VAO,设置顶点数据状态(顶点，法线，纹理坐标等)
         //vao.create();
         //vao.bind();

         // void setAttributeBuffer(int location, GLenum type, int offset, int tupleSize, int stride = 0);
         program->setAttributeBuffer(0, GL_FLOAT, 0,                  3, 0);   //设置aPos顶点属性
         //program->setAttributeBuffer(1, GL_FLOAT, 2 * sizeof(float),  2, 5 * sizeof(float));   //设置aColor顶点颜色

         program->enableAttributeArray(0); //使能
         //program->enableAttributeArray(1);



         //解绑所有对象
         //vao.release();
         //vbo.release();
         */
}
void QImageRender::resizeGL(int w, int h)
{
    //glViewport(0, 0, w, h);
}
void QImageRender::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);
    //vao.bind();

    if(m_pTexture != nullptr)
    {

        m_pTexture->bind(0);
        //m_pTextureUV->bind(1);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        m_pTexture->release();
        //m_pTextureUV->release();

    }

    qDebug() << "QImageRender::paintGL()";
    //vao.release();
}
