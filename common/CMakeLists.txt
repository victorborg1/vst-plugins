
file(GLOB_RECURSE RED_COMMON_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/*.h"
)

list(FILTER RED_COMMON_SOURCES EXCLUDE REGEX
    "PlatformView_.*\\.(cpp|mm)$"
)

if(WIN32)

    list(APPEND RED_COMMON_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/source/gui/WindowManager/PlatformView_win32.cpp"
    )

elseif(UNIX AND NOT APPLE)

    list(APPEND RED_COMMON_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/source/gui/WindowManager/PlatformView_linux.cpp"
    )

elseif(APPLE)

    list(APPEND RED_COMMON_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/source/gui/WindowManager/PlatformView_mac.mm"
    )

endif()

add_library(common STATIC ${RED_COMMON_SOURCES})




target_include_directories(common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/source
)

target_link_libraries(common PUBLIC
    glad
    glm
    stb
    freetype
    #resampler
    sdk
)

# opengl
if(WIN32)
    target_link_libraries(common PUBLIC opengl32)
elseif(APPLE)
    target_link_libraries(common PUBLIC "-framework OpenGL")
elseif(UNIX)
    find_package(OpenGL REQUIRED)
    find_package(X11 REQUIRED)
    target_link_libraries(common 
        PUBLIC 
            OpenGL::GL
            X11::X11
            dl
    )
endif()


source_group(
    TREE ${CMAKE_CURRENT_SOURCE_DIR}/source
    PREFIX "common"
    FILES ${RED_COMMON_SOURCES}
)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/plugin.cmake)
