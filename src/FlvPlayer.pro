QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets multimedia

CONFIG += c++11

DEFINES += DX11_D3D  QT_NO_DEBUG_OUTPUT

THIRDPART = ..\thirdpart
RESOURCE_FILES = ..\resources
# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += $$THIRDPART\
INCLUDEPATH += $$THIRDPART\IntelMediaSDK\include
#INCLUDEPATH += $$(VCINSTALLDIR)\atlmfc\include
INCLUDEPATH += common

SOURCES += \
    common/cmd_options.cpp \
#    common/common_directx.cpp \
#    common/common_directx11.cpp \
    common/common_utils.cpp \
#    common/common_utils_linux.cpp \
    common/common_utils_windows.cpp \
#    common/common_vaapi.cpp \
#    common/ocl_process.cpp \
    flvreader.cpp \
    imediareader.cpp \
    istreamreader.cpp \
    main.cpp \
    mainwindow.cpp \
    streamdecoder.cpp \
    qimagerender.cpp \
    yuv2rgb16tab.c \
    yuv420rgb888c.c \
    qdirectxrender.cpp

HEADERS += \
    common/bits/linux_defs.h \
    common/bits/windows_defs.h \
    common/cmd_options.h \
#    common/common_directx.h \
#    common/common_directx11.h \
    common/common_utils.h \
#    common/common_vaapi.h \
#    common/ocl_process.h \
    libzplay.h\
    flvreader.h \
    imediareader.h \
    istreamreader.h \
    mainwindow.h \
    streamdecoder.h \
    qimagerender.h \
    yuv2rgb.h \
    qdirectxrender.h

message($$DEFINES);
contains(DEFINES, DX11_D3D){
SOURCES += common/common_directx11.cpp
HEADERS += common/common_directx11.h
}

FORMS += \
    mainwindow.ui


LIBS += "$$THIRDPART\IntelMediaSDK\lib\win32\libmfx.lib" \
        DXGI.lib \
        D3D11.lib \
        d3dcompiler.lib\
        dxguid.lib\
        $$THIRDPART/libzplay/libzplay.lib \
        Winmm.lib

QMAKE_LFLAGS_DEBUG += /NODEFAULTLIB:LIBCMT.lib
QMAKE_LFLAGS_RELEASE += /NODEFAULTLIB:LIBCMT.lib

CONFIG(debug, debug|release):QMAKE_POST_LINK = "$$RESOURCE_FILES\install.bat debug"
CONFIG(release, debug|release):QMAKE_POST_LINK = "$$RESOURCE_FILES\install.bat release"

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    common/CMakeLists.txt \
    shader.hlsl
