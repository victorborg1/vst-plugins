
file(GLOB_RECURSE RED_COMMON_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/WindowManager/PlatformView_win32.cpp"
)
add_library(common STATIC ${RED_COMMON_SOURCES})

target_include_directories(common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/source
)

target_link_libraries(common PUBLIC
    glad
    glm
    stb
    freetype
    resampler
    sdk
)

# opengl
if(WIN32)
    target_link_libraries(common PUBLIC opengl32)
elseif(APPLE)
    target_link_libraries(common PUBLIC "-framework OpenGL")
elseif(UNIX)
    find_package(OpenGL REQUIRED)
    target_link_libraries(common PUBLIC OpenGL::GL)
endif()


source_group(
    TREE ${CMAKE_CURRENT_SOURCE_DIR}/source
    PREFIX "common"
    FILES ${RED_COMMON_SOURCES}
)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/plugin.cmake)