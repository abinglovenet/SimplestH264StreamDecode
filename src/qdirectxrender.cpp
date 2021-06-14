#include "qdirectxrender.h"

#define OBJECT_RELEASE(x) if(x){x->Release(); x = NULL;}
//--------------------------------------------------------------------------------------
// Helper for compiling shaders with D3DX11
//--------------------------------------------------------------------------------------
HRESULT CompileShaderFromFile(char* fiel_name, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
{
    HRESULT hr = S_OK;

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
    // Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows
    // the shaders to be optimized and to run exactly the way they will run in
    // the release configuration of this program.
    dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif


    CA2WEX<1024> a2w(fiel_name);
    ID3DBlob* pErrorBlob;
    hr = D3DCompileFromFile((LPWSTR)a2w, NULL, NULL, szEntryPoint, szShaderModel, dwShaderFlags, 0,  ppBlobOut, &pErrorBlob);
    if ( FAILED(hr) )
    {
        if ( pErrorBlob != NULL )
        {
            //OutputDebugStringA(( char* ) pErrorBlob->GetBufferPointer());
        }
        if ( pErrorBlob ) pErrorBlob->Release();
        return hr;
    }
    if ( pErrorBlob ) pErrorBlob->Release();

    return S_OK;
}



class QRenderThread : public QThread
{

public:
    explicit QRenderThread(){

    }
    ~QRenderThread(){
        Stop();
    }

    void Run(QDirectXRender* pRender, int duration = 40);

    void Stop();
public slots:
    void renderFrame();
protected:
    QTimer* m_pTimer;
    QDirectXRender* m_pRender;

    bool m_bRun;
protected:
    virtual void	run();
};



void QRenderThread::Run(QDirectXRender* pRender, int duration)
{

    m_pRender = pRender;

m_bRun = true;
start(HighestPriority);

//OutputDebugStringA("qrenderthread run-------");
}
void QRenderThread::Stop()
{
    //m_pTimer->stop();
    //delete m_pTimer;
m_bRun = false;
    //quit();
    wait();
}

void QRenderThread::renderFrame()
{
    m_pRender->timeRendering();
}
void QRenderThread::run()
{
while(m_bRun)
{
    renderFrame();
    msleep(16);
    //OutputDebugStringA("i am rendering");
}

/*
m_pTimer = new QTimer();
connect(m_pTimer, SIGNAL(timeout()), this, SLOT(renderFrame()), Qt::DirectConnection);
m_pTimer->setTimerType(Qt::PreciseTimer);
 m_pTimer->start(10);
    exec();
    */

/*
while(true)
{
    //qInfo() << "11111" << GetTickCount();
    msleep(40);
}
*/
}

struct SimpleVertex
{
    XMFLOAT4 Pos;
    XMFLOAT2 TexCoord;
};

QRenderThread g_renderThread;
QDirectXRender::QDirectXRender(QWidget *parent) : QWidget(parent)
{
    g_pD3D11SwapChain = NULL;
    g_pD3D11Ctx = NULL;
    g_pD3D11Device = NULL;

    g_pVertexBuffer = NULL;
    g_pVertexLayout = NULL;
    g_pVertexShader = NULL;
    g_pPixelShader = NULL;
    g_pRenderTargetView = NULL;
    m_pCurrentSurface = NULL;

    setAttribute(Qt::WA_PaintOnScreen, 1);
/*
    connect(&m_tickRender, SIGNAL(timeout()), this, SLOT(timeRendering()));
    m_tickRender.setInterval(1000 / 60);
    m_tickRender.start();
*/
    g_renderThread.Run(this);
}

QDirectXRender::~QDirectXRender()
{
    g_renderThread.Stop();
    //m_tickRender.stop();
    Clear();

    if ( g_pD3D11Ctx ) g_pD3D11Ctx->ClearState();

    OBJECT_RELEASE ( g_pVertexBuffer );
    OBJECT_RELEASE ( g_pVertexLayout );
    OBJECT_RELEASE ( g_pVertexShader );
    OBJECT_RELEASE ( g_pPixelShader );
    OBJECT_RELEASE ( g_pRenderTargetView );
    if ( g_pD3D11SwapChain ) g_pD3D11SwapChain = NULL;
    if ( g_pD3D11Ctx ) g_pD3D11Ctx = NULL;
    if ( g_pD3D11Device ) g_pD3D11Device = NULL;
}

void QDirectXRender::SetFrame(PAV_FRAME pFrame)
{
    QMutexLocker render(&m_mutexRender);
    if(NULL == pFrame)
        return;

    m_pCurrentSurface = pFrame->pSurface;

    RenderSurface(pFrame->pSurface);

    if(m_pCurrentSurface != NULL)
    {
        QMutexLocker lock((QMutex*)m_pCurrentSurface->reserved[0]);
        m_pCurrentSurface->reserved[1] = 0;
    }



}

void QDirectXRender::Clear()
{
    if(g_pD3D11Ctx == NULL)
        return;

    ReleaseSRVs();
    if(m_pCurrentSurface)
    {
        QMutexLocker lock((QMutex*)m_pCurrentSurface->reserved[0]);
        m_pCurrentSurface->reserved[1] = 0;
        m_pCurrentSurface = NULL;
    }
    float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    g_pD3D11Ctx->ClearRenderTargetView(g_pRenderTargetView, ClearColor);
    g_pD3D11SwapChain->Present(1, 0);
}

bool QDirectXRender::ResetRender()
{
    //
    HRESULT hr = S_OK;

    // Prepare for calling resetbuffer
    ReleaseSRVs();

    if ( g_pD3D11Ctx )
        g_pD3D11Ctx->ClearState();

    OBJECT_RELEASE ( g_pVertexBuffer );
    OBJECT_RELEASE ( g_pVertexLayout );
    OBJECT_RELEASE ( g_pVertexShader );
    OBJECT_RELEASE ( g_pPixelShader );
    OBJECT_RELEASE ( g_pRenderTargetView );


    if(g_pD3D11SwapChain == NULL)
        g_pD3D11SwapChain = ::GetDirectXSwapChain();

    if(g_pD3D11Ctx == NULL)
        g_pD3D11Ctx = ::GetDirectXDeviceContext();

    if(g_pD3D11Device == NULL)
        g_pD3D11Device = ::GetDirectXDeviceHanle();

    if(g_pD3D11Device == NULL)
        return false;

    hr = g_pD3D11SwapChain->ResizeBuffers(0, width(), height(), DXGI_FORMAT_UNKNOWN, 0);

    ID3D11Texture2D* pBackBuffer = NULL;
    g_pD3D11SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), ( LPVOID* ) &pBackBuffer);
    if ( FAILED(hr) )
        return false;

    hr = g_pD3D11Device->CreateRenderTargetView(pBackBuffer, NULL, &g_pRenderTargetView);
    pBackBuffer->Release();
    if ( FAILED(hr) ) return false;

    g_pD3D11Ctx->OMSetRenderTargets(1, &g_pRenderTargetView, NULL);
    float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    g_pD3D11Ctx->ClearRenderTargetView(g_pRenderTargetView, ClearColor);
    g_pD3D11SwapChain->Present(1, 0);

    D3D11_VIEWPORT view_port;
    view_port.Width = width();
    view_port.Height = height();
    view_port.MinDepth = 0.0f;
    view_port.MaxDepth = 1.0f;
    view_port.TopLeftX = 0;
    view_port.TopLeftY = 0;
    g_pD3D11Ctx->RSSetViewports(1, &view_port);


    D3D11_RECT scissor = { 0 /* left */, 0 /* top */, width() /* right */, height() /* bottom */ };
    g_pD3D11Ctx->RSSetScissorRects(1, &scissor);


    SimpleVertex vertices[] =
    {

        { XMFLOAT4(-1.0f, -1.0f, 0, 1.0f), XMFLOAT2(0, 1.0) },
        { XMFLOAT4(-1.0f, 1.0f, 0, 1.0f), XMFLOAT2(0, 0) },
        { XMFLOAT4(1.0f, 1.0f, 0, 1.0f), XMFLOAT2(1.0, 0) },
        { XMFLOAT4(1.0f, 1.0f, 0, 1.0f), XMFLOAT2(1.0, 0) },
        { XMFLOAT4(1.0f, -1.0f, 0, 1.0f), XMFLOAT2(1.0, 1.0) },
        { XMFLOAT4(-1.0f, -1.0f, 0, 1.0f), XMFLOAT2(0, 1.0) }
    };
    D3D11_BUFFER_DESC vertex_desc;
    vertex_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertex_desc.ByteWidth = sizeof( SimpleVertex ) * 6;
    vertex_desc.CPUAccessFlags = 0;
    vertex_desc.MiscFlags = 0;
    vertex_desc.StructureByteStride = 0;
    vertex_desc.Usage = D3D11_USAGE_DEFAULT;

    D3D11_SUBRESOURCE_DATA vertex_data;
    vertex_data.pSysMem = vertices;
    vertex_data.SysMemPitch = 0;
    vertex_data.SysMemSlicePitch = 0;
    hr = g_pD3D11Device->CreateBuffer(&vertex_desc, &vertex_data, &g_pVertexBuffer);
    if ( FAILED(hr) ) return false;


    UINT stride = sizeof(SimpleVertex);
    UINT offset = 0;
    g_pD3D11Ctx->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);


    g_pD3D11Ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // VS Shader
    ID3DBlob* pVSBlob = NULL;
    hr = CompileShaderFromFile("shader.hlsl", "VS", "vs_5_0", &pVSBlob);
    if ( FAILED(hr) )
    {
        return false;
    }

    hr = g_pD3D11Device->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &g_pVertexShader);
    if ( FAILED(hr) )
    {
        pVSBlob->Release(); return false;
    }

    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    UINT numElements = ARRAYSIZE(layout);


    // Input layout
    hr = g_pD3D11Device->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &g_pVertexLayout);
    pVSBlob->Release();
    if ( FAILED(hr) )
        return false;

    g_pD3D11Ctx->IASetInputLayout(g_pVertexLayout);


    // PS Shader
    ID3DBlob* pPSBlob = NULL;
    hr = CompileShaderFromFile("shader.hlsl", "PS", "ps_5_0", &pPSBlob);
    if ( FAILED(hr) )
    {
        return false;
    }

    hr = g_pD3D11Device->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &g_pPixelShader);
    pPSBlob->Release();
    if ( FAILED(hr) ) return false;

    g_pD3D11Ctx->VSSetShader(g_pVertexShader, NULL, 0);
    g_pD3D11Ctx->PSSetShader(g_pPixelShader, NULL, 0);

    //RenderSurface(m_pCurrentSurface);
    return true;
}

void QDirectXRender::ReleaseSRVs()
{
    for(QMap<mfxFrameSurface1*, PSRV>::iterator it = m_mapSRVs.begin(); it != m_mapSRVs.end(); it++)
    {
        PSRV pSRV = it.value();
        OBJECT_RELEASE(pSRV->views[0]);
        OBJECT_RELEASE(pSRV->views[1]);
        delete it.value();
    }

    m_mapSRVs.clear();

}

void QDirectXRender::RenderSurface(mfxFrameSurface1* pSurface)
{
    if(NULL == pSurface)
        return;

    // create shader rv
    PSRV pSRV = nullptr;
    if(m_mapSRVs.find(pSurface) != m_mapSRVs.end())
        pSRV = m_mapSRVs[pSurface];
    else
    {
        pSRV = new SRV();
        D3D11_SHADER_RESOURCE_VIEW_DESC psRVDes;
        psRVDes.Format = DXGI_FORMAT_R8_UINT;
        psRVDes.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
        psRVDes.Texture2D.MipLevels = 1;
        psRVDes.Texture2D.MostDetailedMip = 0;
        ID3D11Texture2D* pTexture = ::GetTexture(pSurface);
        if(pTexture == NULL)
            qInfo() << "Unable to find the corresponding texture";
        g_pD3D11Device->CreateShaderResourceView(pTexture, &psRVDes, &pSRV->views[0]);

        psRVDes.Format = DXGI_FORMAT_R8G8_UINT;
        psRVDes.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
        psRVDes.Texture2D.MipLevels = 1;
        psRVDes.Texture2D.MostDetailedMip = 0;
        g_pD3D11Device->CreateShaderResourceView(pTexture, &psRVDes, &pSRV->views[1]);
        m_mapSRVs.insert(pSurface, pSRV);
    }


    g_pD3D11Ctx->PSSetShaderResources( 0, 2, pSRV->views);
    g_pD3D11Ctx->Draw(6, 0);
    //g_pD3D11SwapChain->Present(1, 0);

}
void QDirectXRender::resizeEvent(QResizeEvent * event)
{
     QMutexLocker render(&m_mutexRender);
    ResetRender();
}

QPaintEngine * QDirectXRender::paintEngine() const
{
    return 0;
}


void QDirectXRender::paintEvent(QPaintEvent *)
{
     QMutexLocker render(&m_mutexRender);
    if(g_pD3D11SwapChain)
        g_pD3D11SwapChain->Present(1, 0);
}

void QDirectXRender::timeRendering()
{
     QMutexLocker render(&m_mutexRender);
    if(g_pD3D11SwapChain)
        g_pD3D11SwapChain->Present(1, 0);
}
