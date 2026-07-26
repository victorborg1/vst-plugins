cmake_minimum_required(VERSION 3.20)
include(../common/cmake/plugin.cmake)

set(plug_sources
    source/Factory.cpp 
    source/Processor.cpp
    source/gui/Editor.cpp
    source/gui/Waveform.cpp
    source/dsp/Tempo.cpp
)

build_vst3("${plug_sources}")