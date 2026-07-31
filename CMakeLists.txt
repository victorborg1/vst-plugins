cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.13 CACHE STRING "") # fix later
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

project(eigendsp-plugins VERSION 1.0.0)

if(MSVC)
    add_compile_options("/Zc:__cplusplus")
endif()


# vst3 path. override with -DVST3_SDK_PATH=
set(VST3_SDK_PATH ""
    CACHE PATH "Path to VST3 SDK")

if(NOT EXISTS "${VST3_SDK_PATH}")
    message(FATAL_ERROR "VST3 SDK not found at: ${VST3_SDK_PATH}")
endif()

option(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES "" OFF)
option(SMTG_ENABLE_VST3_HOSTING_EXAMPLES "" OFF)
option(SMTG_CREATE_PLUGIN_LINK "" OFF)
option(SMTG_RUN_VST_VALIDATOR "" OFF)
set(SMTG_ENABLE_VSTGUI_SUPPORT OFF CACHE BOOL "" FORCE)


# vst3sdk
add_subdirectory(${VST3_SDK_PATH} ${PROJECT_BINARY_DIR}/vst3sdk)
smtg_enable_vst3_sdk()

# external stuff
add_subdirectory(external)


# common lib 
add_subdirectory(common)


# plugins
add_subdirectory(plug-Granulator)
add_subdirectory(plug-Ducker)
add_subdirectory(plug-Polymeter)
add_subdirectory(plug-Arp)
add_subdirectory(plug-BitDeath)
