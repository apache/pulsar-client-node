cd %~dp0
set arch=%1
set /P BASE_URL=<..\..\build-support\cpp-base-url.txt
curl -O -L %BASE_URL%/%arch%-windows-static.tar.gz
tar -xvzf %arch%-windows-static.tar.gz
mv %arch%-windows-static pulsar-cpp
dir

