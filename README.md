# My VST plugins


## Linux

cmake -B build \
    -G "Unix Makefiles" \
    -DVST3_SDK_PATH="$HOME/SDKs/vst3sdk" \
    -DCMAKE_BUILD_TYPE=Release

cmake --build build

## Windows

cmake -B build -G "Visual Studio 17 2022" -A x64 -DVST3_SDK_PATH="C:/SDKs/vst-sdk_3.8.0_build-66_2025-10-20/VST_SDK/vst3sdk"

cmake --build build --config Release


