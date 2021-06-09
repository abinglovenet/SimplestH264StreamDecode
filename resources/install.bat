echo %1
xcopy ..\thirdpart\libzplay\libzplay.dll %1 /R /Y /Q
xcopy ..\thirdpart\libzplay\libzplay.dll  /R /Y /Q
xcopy ..\src\shader.hlsl %1 /R /Y /Q
xcopy ..\src\shader.hlsl  /R /Y /Q
mkdir demos
xcopy %~dp0\demos demos /R /S /E /Y /Q
mkdir %1\demos
xcopy %~dp0\demos %1\demos /R /S /E /Y /Q

mkdir ffmpeg
xcopy %~dp0\ffmpeg ffmpeg /R /S /E /Y /Q
mkdir %1\ffmpeg
xcopy %~dp0\ffmpeg %1\ffmpeg /R /S /E /Y /Q