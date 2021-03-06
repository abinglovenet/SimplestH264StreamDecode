// Copyright (c) 2019 Intel Corporation
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "common_directx11.h"

#include<map>

HWND                                    g_hWnd;
CComPtr<IDXGISwapChain>                 g_pD3D11SwapChain = NULL;
CComPtr<ID3D11Device>                   g_pD3D11Device = NULL;
CComPtr<ID3D11DeviceContext>            g_pD3D11Ctx = NULL;
CComPtr<IDXGIFactory2>                  g_pDXGIFactory = NULL;
IDXGIAdapter*                           g_pAdapter = NULL;

std::map<mfxMemId*, mfxHDL>             allocResponses;
std::map<mfxHDL, mfxFrameAllocResponse> allocDecodeResponses;
std::map<mfxHDL, int>                   allocDecodeRefCount;

typedef struct {
    mfxMemId    memId;
    mfxMemId    memIdStage;
    mfxU16      rw;
} CustomMemId;

const struct {
    mfxIMPL impl;       // actual implementation
    mfxU32  adapterID;  // device adapter number
} implTypes[] = {
{MFX_IMPL_HARDWARE, 0},
{MFX_IMPL_HARDWARE2, 1},
{MFX_IMPL_HARDWARE3, 2},
{MFX_IMPL_HARDWARE4, 3}
};

// =================================================================
// DirectX functionality required to manage DX11 device and surfaces
//

IDXGIAdapter* GetIntelDeviceAdapterHandle(mfxSession session)
{
    mfxU32  adapterNum = 0;
    mfxIMPL impl;

    MFXQueryIMPL(session, &impl);

    mfxIMPL baseImpl = MFX_IMPL_BASETYPE(impl); // Extract Media SDK base implementation type

    // get corresponding adapter number
    for (mfxU8 i = 0; i < sizeof(implTypes)/sizeof(implTypes[0]); i++) {
        if (implTypes[i].impl == baseImpl) {
            adapterNum = implTypes[i].adapterID;
            break;
        }
    }

    HRESULT hres = CreateDXGIFactory(__uuidof(IDXGIFactory2), (void**)(&g_pDXGIFactory) );
    if (FAILED(hres)) return NULL;

    IDXGIAdapter* adapter;
    hres = g_pDXGIFactory->EnumAdapters(adapterNum, &adapter);
    if (FAILED(hres)) return NULL;

    return adapter;
}

bool InitDirectX(HWND hWnd)
{
    g_hWnd = hWnd;
    MFXVideoSession session;
    mfxVersion version = {0, 1};
    mfxIMPL impl = MFX_IMPL_HARDWARE_ANY;
    impl |= MFX_IMPL_VIA_D3D11;
    session.Init(impl, &version);
    mfxHDL handle;
    CreateHWDevice(session, &handle, hWnd, true);
    session.Close();
    return true;
}

CComPtr<ID3D11Device> GetDirectXDeviceHanle()
{
    return g_pD3D11Device;
}
CComPtr<ID3D11DeviceContext> GetDirectXDeviceContext()
{
    return g_pD3D11Ctx;
}
CComPtr<IDXGISwapChain> GetDirectXSwapChain()
{
    return g_pD3D11SwapChain;
}
void UnInitDirectX()
{
    ::CleanupHWDevice();
}

ID3D11Texture2D* GetTexture(mfxFrameSurface1* pSurface)
{
    CustomMemId*        memId       = (CustomMemId*)pSurface->Data.MemId;
    return (ID3D11Texture2D*)memId->memId;
}

// Create HW device context
mfxStatus CreateHWDevice(mfxSession session, mfxHDL* deviceHandle, HWND hWnd, bool /*bCreateSharedHandles*/)
{
    // Window handle not required by DX11 since we do not showcase rendering.

    HRESULT res = S_OK;

    static D3D_FEATURE_LEVEL FeatureLevels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0
    };
    D3D_FEATURE_LEVEL pFeatureLevelsOut;

    g_pAdapter = GetIntelDeviceAdapterHandle(session);
    if (NULL == g_pAdapter)
        return MFX_ERR_DEVICE_FAILED;
    UINT dxFlags = 0;
    //UINT dxFlags = D3D11_CREATE_DEVICE_DEBUG;
    DXGI_SWAP_CHAIN_DESC swap_desc;
    ZeroMemory(&swap_desc, sizeof(swap_desc));
    swap_desc.BufferCount = 1;
    swap_desc.BufferDesc.Width = 0;
    swap_desc.BufferDesc.Height = 0;
    swap_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swap_desc.BufferDesc.RefreshRate.Numerator = 60;
    swap_desc.BufferDesc.RefreshRate.Denominator = 1;
    swap_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_desc.OutputWindow = hWnd;
    swap_desc.SampleDesc.Count = 1;
    swap_desc.SampleDesc.Quality = 0;
    swap_desc.Windowed = TRUE;
    res = D3D11CreateDeviceAndSwapChain( g_pAdapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, dxFlags, FeatureLevels, sizeof(FeatureLevels) / sizeof(FeatureLevels[0]), D3D11_SDK_VERSION, &swap_desc, &g_pD3D11SwapChain,   &g_pD3D11Device, &pFeatureLevelsOut, &g_pD3D11Ctx);
    //res = D3D11CreateDeviceAndSwapChain( g_pAdapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, dxFlags, FeatureLevels, sizeof(FeatureLevels) / sizeof(FeatureLevels[0]), D3D11_SDK_VERSION, &swap_desc, &g_pD3D11SwapChain,   &g_pD3D11Device, &pFeatureLevelsOut, &g_pD3D11Ctx);

    if (FAILED(res))
        return MFX_ERR_DEVICE_FAILED;

    // turn on multithreading for the DX11 context
    CComQIPtr<ID3D10Multithread> p_mt(g_pD3D11Ctx);
    if (p_mt)
        p_mt->SetMultithreadProtected(true);
    else
        return MFX_ERR_DEVICE_FAILED;

    *deviceHandle = (mfxHDL)g_pD3D11Device;

    return MFX_ERR_NONE;
}


void SetHWDeviceContext(CComPtr<ID3D11DeviceContext> devCtx)
{
    g_pD3D11Ctx = devCtx;
    devCtx->GetDevice(&g_pD3D11Device);
}

// Free HW device context
void CleanupHWDevice()
{

    if (g_pD3D11Ctx)
        g_pD3D11Ctx->ClearState();

    if (g_pD3D11Ctx)
    {
        g_pD3D11Ctx = NULL;
    }

    if (g_pD3D11SwapChain)
    {

        g_pD3D11SwapChain = NULL;
    }

    if (g_pD3D11Device)
    {

        g_pD3D11Device = NULL;
    }
    g_pDXGIFactory = NULL;
    g_pAdapter->Release();
    g_pAdapter = NULL;
}

CComPtr<ID3D11DeviceContext> GetHWDeviceContext()
{
    return g_pD3D11Ctx;
}

void ClearYUVSurfaceD3D(mfxMemId /*memId*/)
{
    // TBD
}

void ClearRGBSurfaceD3D(mfxMemId /*memId*/)
{
    // TBD
}

//
// Intel Media SDK memory allocator entrypoints....
//
mfxStatus _simple_alloc(mfxFrameAllocRequest* request, mfxFrameAllocResponse* response)
{
    HRESULT hRes;

    //qDebug() << request->Info.Width << "x" << request->Info.Height;
    // Determine surface format
    DXGI_FORMAT format;
    if (MFX_FOURCC_NV12 == request->Info.FourCC)
        format = DXGI_FORMAT_NV12;
    else if (MFX_FOURCC_RGB4 == request->Info.FourCC)
        format = DXGI_FORMAT_B8G8R8A8_UNORM;
    else if (MFX_FOURCC_YUY2== request->Info.FourCC)
        format = DXGI_FORMAT_YUY2;
    else if (MFX_FOURCC_P8 == request->Info.FourCC ) //|| MFX_FOURCC_P8_TEXTURE == request->Info.FourCC
        format = DXGI_FORMAT_P8;
    else
        format = DXGI_FORMAT_UNKNOWN;

    if (DXGI_FORMAT_UNKNOWN == format)
        return MFX_ERR_UNSUPPORTED;

    //qDebug() << "format:" << format;

    // Allocate custom container to keep texture and stage buffers for each surface
    // Container also stores the intended read and/or write operation.
    CustomMemId** mids = (CustomMemId**)calloc(request->NumFrameSuggested, sizeof(CustomMemId*));
    if (!mids) return MFX_ERR_MEMORY_ALLOC;

    for (int i=0; i<request->NumFrameSuggested; i++) {
        mids[i] = (CustomMemId*)calloc(1, sizeof(CustomMemId));
        if (!mids[i]) {
            return MFX_ERR_MEMORY_ALLOC;
        }
        mids[i]->rw = request->Type & 0xF000; // Set intended read/write operation
        //mids[i]->rw |= WILL_READ;
        //mids[i]->rw |= WILL_WRITE;
    }

    request->Type = request->Type & 0x0FFF;

    // because P8 data (bitstream) for h264 encoder should be allocated by CreateBuffer()
    // but P8 data (MBData) for MPEG2 encoder should be allocated by CreateTexture2D()
    if (request->Info.FourCC == MFX_FOURCC_P8) {
        D3D11_BUFFER_DESC desc = { 0 };

        if (!request->NumFrameSuggested) return MFX_ERR_MEMORY_ALLOC;

        desc.ByteWidth           = request->Info.Width * request->Info.Height;
        desc.Usage               = D3D11_USAGE_STAGING;
        desc.BindFlags           = 0;
        desc.CPUAccessFlags      = D3D11_CPU_ACCESS_READ;
        desc.MiscFlags           = 0;
        desc.StructureByteStride = 0;

        ID3D11Buffer* buffer = 0;
        hRes = g_pD3D11Device->CreateBuffer(&desc, 0, &buffer);
        if (FAILED(hRes))
            return MFX_ERR_MEMORY_ALLOC;

        mids[0]->memId = reinterpret_cast<ID3D11Texture2D*>(buffer);
    } else {
        D3D11_TEXTURE2D_DESC desc = {0};

        desc.Width              = request->Info.Width;
        desc.Height             = request->Info.Height;
        desc.MipLevels          = 1;
        desc.ArraySize          = 1; // number of subresources is 1 in this case
        desc.Format             = format;
        desc.SampleDesc.Count   = 1;
        desc.Usage              = D3D11_USAGE_DEFAULT;
        desc.BindFlags          = D3D11_BIND_DECODER;
        desc.MiscFlags          = 0;
        // desc.CPUAccessFlags      = D3D11_CPU_ACCESS_READ;
        //desc.MiscFlags            = D3D11_RESOURCE_MISC_SHARED;

        if ( (MFX_MEMTYPE_FROM_VPPIN & request->Type) &&
             (DXGI_FORMAT_B8G8R8A8_UNORM == desc.Format) ) {
            desc.BindFlags = D3D11_BIND_RENDER_TARGET;
            if (desc.ArraySize > 2)
                return MFX_ERR_MEMORY_ALLOC;
        }

        if ( (MFX_MEMTYPE_FROM_VPPOUT & request->Type) ||
             (MFX_MEMTYPE_VIDEO_MEMORY_PROCESSOR_TARGET & request->Type)) {
            desc.BindFlags = D3D11_BIND_RENDER_TARGET;
            if (desc.ArraySize > 2)
                return MFX_ERR_MEMORY_ALLOC;
        }

        if ( DXGI_FORMAT_P8 == desc.Format )
            desc.BindFlags = 0;

        desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;

        ID3D11Texture2D* pTexture2D;

        // Create surface textures
        for (size_t i = 0; i < request->NumFrameSuggested / desc.ArraySize; i++) {
            hRes = g_pD3D11Device->CreateTexture2D(&desc, NULL, &pTexture2D);

            if (FAILED(hRes))
            {
                //qDebug() << "Create Texture2D Failed";
                return MFX_ERR_MEMORY_ALLOC;
            }

            mids[i]->memId = pTexture2D;
            //qDebug("Create Texture2D  success : %x", pTexture2D);
        }

        desc.ArraySize      = 1;
        desc.Usage          = D3D11_USAGE_STAGING;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;// | D3D11_CPU_ACCESS_WRITE;
        desc.BindFlags      = 0;
        desc.MiscFlags      = 0;
        //desc.MiscFlags        = D3D11_RESOURCE_MISC_SHARED;

        // Create surface staging textures
        for (size_t i = 0; i < request->NumFrameSuggested; i++) {
            hRes = g_pD3D11Device->CreateTexture2D(&desc, NULL, &pTexture2D);

            if (FAILED(hRes))
                return MFX_ERR_MEMORY_ALLOC;

            mids[i]->memIdStage = pTexture2D;
            //qDebug("Create Texture2D stage  success : %x", pTexture2D);
        }
    }


    response->mids = (mfxMemId*)mids;
    response->NumFrameActual = request->NumFrameSuggested;

    for(int i = 0; i < response->NumFrameActual; i++)
        //qDebug("surface: response mid:%x - %x  num frame suggested:%d", response->mids[i], mids[i]->memId, request->NumFrameSuggested);
    return MFX_ERR_NONE;
}

mfxStatus simple_alloc(mfxHDL pthis, mfxFrameAllocRequest* request, mfxFrameAllocResponse* response)
{
    mfxStatus sts = MFX_ERR_NONE;

    if (request->Type & MFX_MEMTYPE_SYSTEM_MEMORY)
        return MFX_ERR_UNSUPPORTED;

    if (allocDecodeResponses.find(pthis) != allocDecodeResponses.end() &&
            MFX_MEMTYPE_EXTERNAL_FRAME & request->Type &&
            MFX_MEMTYPE_FROM_DECODE & request->Type) {
        // Memory for this request was already allocated during manual allocation stage. Return saved response
        //   When decode acceleration device (DXVA) is created it requires a list of d3d surfaces to be passed.
        //   Therefore Media SDK will ask for the surface info/mids again at Init() stage, thus requiring us to return the saved response
        //   (No such restriction applies to Encode or VPP)
        *response = allocDecodeResponses[pthis];
        allocDecodeRefCount[pthis]++;
    } else {
        sts = _simple_alloc(request, response);

        if (MFX_ERR_NONE == sts) {
            if ( MFX_MEMTYPE_EXTERNAL_FRAME & request->Type &&
                 MFX_MEMTYPE_FROM_DECODE & request->Type) {
                // Decode alloc response handling
                allocDecodeResponses[pthis] = *response;
                allocDecodeRefCount[pthis]++;
            } else {
                // Encode and VPP alloc response handling
                allocResponses[response->mids] = pthis;
            }
        }
    }

    return sts;
}

mfxStatus simple_lock(mfxHDL pthis, mfxMemId mid, mfxFrameData* ptr)
{
    pthis; // To suppress warning for this unused parameter

    HRESULT hRes = S_OK;

    D3D11_TEXTURE2D_DESC        desc = {0};
    D3D11_MAPPED_SUBRESOURCE    lockedRect = {0};

    CustomMemId*        memId       = (CustomMemId*)mid;
    ID3D11Texture2D*    pSurface    = (ID3D11Texture2D*)memId->memId;
    ID3D11Texture2D*    pStage      = (ID3D11Texture2D*)memId->memIdStage;

    D3D11_MAP   mapType  = D3D11_MAP_READ;
    UINT        mapFlags = D3D11_MAP_FLAG_DO_NOT_WAIT;

    if (NULL == pStage) {
        hRes = g_pD3D11Ctx->Map(pSurface, 0, mapType, mapFlags, &lockedRect);
        desc.Format = DXGI_FORMAT_P8;
    } else {
        pSurface->GetDesc(&desc);

        // copy data only in case of user wants o read from stored surface
        if (memId->rw & WILL_READ)
        {
            g_pD3D11Ctx->CopySubresourceRegion(pStage, 0, 0, 0, 0, pSurface, 0, NULL);
            //qDebug() << "i want read";
        }

        do {
            hRes = g_pD3D11Ctx->Map(pStage, 0, mapType, mapFlags, &lockedRect);
            if (S_OK != hRes && DXGI_ERROR_WAS_STILL_DRAWING != hRes)
            {
                //qDebug() << "lock failed";
                return MFX_ERR_LOCK_MEMORY;
            }
        } while (DXGI_ERROR_WAS_STILL_DRAWING == hRes);
    }

    if (FAILED(hRes))
        return MFX_ERR_LOCK_MEMORY;

    switch (desc.Format) {
    case DXGI_FORMAT_NV12:
        ptr->Pitch = (mfxU16)lockedRect.RowPitch;
        ptr->Y = (mfxU8*)lockedRect.pData;
        ptr->U = (mfxU8*)lockedRect.pData + desc.Height * lockedRect.RowPitch;
        ptr->V = ptr->U + 1;
        break;
    case DXGI_FORMAT_B8G8R8A8_UNORM :
        ptr->Pitch = (mfxU16)lockedRect.RowPitch;
        ptr->B = (mfxU8*)lockedRect.pData;
        ptr->G = ptr->B + 1;
        ptr->R = ptr->B + 2;
        ptr->A = ptr->B + 3;
        break;
    case DXGI_FORMAT_YUY2:
        ptr->Pitch = (mfxU16)lockedRect.RowPitch;
        ptr->Y = (mfxU8*)lockedRect.pData;
        ptr->U = ptr->Y + 1;
        ptr->V = ptr->Y + 3;
        break;
    case DXGI_FORMAT_P8 :
        ptr->Pitch = (mfxU16)lockedRect.RowPitch;
        ptr->Y = (mfxU8*)lockedRect.pData;
        ptr->U = 0;
        ptr->V = 0;
        break;
    default:
        return MFX_ERR_LOCK_MEMORY;
    }

    return MFX_ERR_NONE;
}

mfxStatus simple_unlock(mfxHDL pthis, mfxMemId mid, mfxFrameData* ptr)
{
    pthis; // To suppress warning for this unused parameter

    CustomMemId*        memId       = (CustomMemId*)mid;
    ID3D11Texture2D*    pSurface    = (ID3D11Texture2D*)memId->memId;
    ID3D11Texture2D*    pStage      = (ID3D11Texture2D*)memId->memIdStage;

    if (NULL == pStage) {
        g_pD3D11Ctx->Unmap(pSurface, 0);
    } else {
        g_pD3D11Ctx->Unmap(pStage, 0);
        // copy data only in case of user wants to write to stored surface
        if (memId->rw & WILL_WRITE)
            g_pD3D11Ctx->CopySubresourceRegion(pSurface, 0, 0, 0, 0, pStage, 0, NULL);
    }

    if (ptr) {
        ptr->Pitch=0;
        ptr->U=ptr->V=ptr->Y=0;
        ptr->A=ptr->R=ptr->G=ptr->B=0;
    }

    return MFX_ERR_NONE;
}

mfxStatus simple_gethdl(mfxHDL pthis, mfxMemId mid, mfxHDL* handle)
{
    pthis; // To suppress warning for this unused parameter

    if (NULL == handle)
        return MFX_ERR_INVALID_HANDLE;

    mfxHDLPair*     pPair = (mfxHDLPair*)handle;
    CustomMemId*    memId = (CustomMemId*)mid;

    pPair->first  = memId->memId; // surface texture
    pPair->second = 0;

    return MFX_ERR_NONE;
}


mfxStatus _simple_free(mfxFrameAllocResponse* response)
{
    if (response->mids) {
        for (mfxU32 i = 0; i < response->NumFrameActual; i++) {
            if (response->mids[i]) {
                CustomMemId*        mid         = (CustomMemId*)response->mids[i];
                ID3D11Texture2D*    pSurface    = (ID3D11Texture2D*)mid->memId;
                ID3D11Texture2D*    pStage      = (ID3D11Texture2D*)mid->memIdStage;

                if (pSurface)
                    pSurface->Release();
                if (pStage)
                    pStage->Release();

                free(mid);
            }
        }
        free(response->mids);
        response->mids = NULL;
    }

    return MFX_ERR_NONE;
}

mfxStatus simple_free(mfxHDL pthis, mfxFrameAllocResponse* response)
{
    if (NULL == response)
        return MFX_ERR_NULL_PTR;

    if (allocResponses.find(response->mids) == allocResponses.end()) {
        // Decode free response handling
        if (--allocDecodeRefCount[pthis] == 0) {
            _simple_free(response);
            allocDecodeResponses.erase(pthis);
            allocDecodeRefCount.erase(pthis);
        }
    } else {
        // Encode and VPP free response handling
        allocResponses.erase(response->mids);
        _simple_free(response);
    }

    return MFX_ERR_NONE;
}
