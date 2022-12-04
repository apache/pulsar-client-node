cd %~dp0
set arch=%1
if "%arch%" == "" (
reg Query "HKLM\Hardware\Description\System\CentralProcessor\0" | find /i "x86" > NUL && set arch=x86 || set arch=x64
)
set /P BASE_URL=<..\..\build-support\cpp-base-url.txt
curl -O -L %BASE_URL%/%arch%-windows-static.tar.gz
tar -xvzf %arch%-windows-static.tar.gz
move %arch%-windows-static pulsar-cpp
dir


