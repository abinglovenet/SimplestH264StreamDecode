echo %1
xcopy ..\thirdpart\libzplay\libzplay.dll %1 /R /Y /Q
mkdir %1\demos
xcopy %~dp0\demos %1\demos /R /S /E /Y /Q
mkdir %1\platforms
xcopy %~dp0\platforms %1\platforms  /R /S /E /Y /Q