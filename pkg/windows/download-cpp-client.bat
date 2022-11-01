cd %~dp0
set arch=%1
:: TODO: Fetch from official release, change to apache release version and path.
set /P CPP_VERSION=<..\..\tmp-pulsar-client-cpp-version.txt
curl -O -L https://github.com/BewareMyPower/pulsar-client-cpp/releases/download/%CPP_VERSION%/pulsar-client-cpp-%arch%-windows-static.zip
7z x pulsar-client-cpp-%arch%-windows-static.zip -opulsar-cpp
dir

