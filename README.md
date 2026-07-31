# My VST plugins

Change the CMakeLists to use your path to the VST3SDK


cmake -B build -G "Visual Studio 17 2022" -A x64

cmake --build build --config Release


## Linux

cmake -B build \
    -G "Unix Makefiles" \
    -DVST3_SDK_PATH="$HOME/SDKs/vst3sdk" \
    -DCMAKE_BUILD_TYPE=Release

cmake --build build

## Windows

cmake -B build
    -G "Visual Studio 17 2022"
    -A x64
    -DVST3_SDK_PATH="C:/SDKs/vst3sdk"

cmake --build build --config Release


