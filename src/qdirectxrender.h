#ifndef QDIRECTXRENDER_H
#define QDIRECTXRENDER_H

#include <QWidget>
#include <QTimer>
#include "streamdecoder.h"
#include "common_directx11.h"

typedef struct {
    ID3D11ShaderResourceView* views[2];
}SRV, *PSRV;
class QDirectXRender : public QWidget
{
    Q_OBJECT
public:
    explicit QDirectXRender(QWidget *parent = 0);
    ~QDirectXRender();

    void SetFrame(PAV_FRAME pFrame);
    void Clear();

    bool ResetRender();
signals:

public slots:
    void timeRendering();
protected:
    void ReleaseSRVs();
    void RenderSurface(mfxFrameSurface1* pSurface);

    virtual QPaintEngine * paintEngine() const;
    virtual void paintEvent(QPaintEvent *);
    virtual void	resizeEvent(QResizeEvent * event);
protected:
    mfxFrameSurface1* m_pCurrentSurface;
    QMap<mfxFrameSurface1*, PSRV> m_mapSRVs;


    CComPtr<IDXGISwapChain>                 g_pD3D11SwapChain;
    CComPtr<ID3D11Device>                   g_pD3D11Device;
    CComPtr<ID3D11DeviceContext>            g_pD3D11Ctx;
    ID3D11RenderTargetView* g_pRenderTargetView;
    ID3D11VertexShader*     g_pVertexShader;
    ID3D11PixelShader*      g_pPixelShader;
    ID3D11InputLayout*      g_pVertexLayout;
    ID3D11Buffer*           g_pVertexBuffer;

    QTimer m_tickRender;

};

#endif // QDIRECTXRENDER_H
