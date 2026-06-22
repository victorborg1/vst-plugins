cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.13 CACHE STRING "")

project(eigendsp-plugins VERSION 1.0.0)

if(MSVC)
    add_compile_options("/Zc:__cplusplus")
endif()


# vst3 path. override with -DVST3_SDK_PATH=
set(VST3_SDK_PATH "C:/SDKs/vst-sdk_3.8.0_build-66_2025-10-20/VST_SDK/vst3sdk"
    CACHE PATH "Path to VST3 SDK")

if(NOT EXISTS ${VST3_SDK_PATH})
    message(FATAL_ERROR "VST3 SDK not found at: ${VST3_SDK_PATH}")
endif()

option(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES "" OFF)
option(SMTG_ENABLE_VST3_HOSTING_EXAMPLES "" OFF)
option(SMTG_CREATE_PLUGIN_LINK "" OFF)
option(SMTG_RUN_VST_VALIDATOR "" OFF)


# external stuff
add_subdirectory(external)


# vst3sdk
add_subdirectory(${VST3_SDK_PATH} ${PROJECT_BINARY_DIR}/vst3sdk)
smtg_enable_vst3_sdk()


# common lib 
add_subdirectory(common)


# plugins
add_subdirectory(plug-Granulator)
add_subdirectory(plug-Ducker)
add_subdirectory(plug-Polymeter)
add_subdirectory(plug-Arp)
add_subdirectory(plug-BitDeath)