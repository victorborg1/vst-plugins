

# glad
add_library(glad STATIC
    glad/src/glad.c
)
target_include_directories(glad PUBLIC glad/include)


# glm
add_library(glm INTERFACE)
target_include_directories(glm INTERFACE glm)


# stb 
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE stb)


# freetype
set(FREETYPE_SOURCES
    freetype/src/autofit/autofit.c
    freetype/src/base/ftbase.c
    freetype/src/base/ftbitmap.c
    freetype/src/base/ftdebug.c
    freetype/src/base/ftinit.c
    freetype/src/base/ftsystem.c
    freetype/src/base/ftglyph.c
    freetype/src/bdf/bdf.c
    freetype/src/cff/cff.c
    freetype/src/cid/type1cid.c
    freetype/src/gzip/ftgzip.c
    freetype/src/lzw/ftlzw.c
    freetype/src/pcf/pcf.c
    freetype/src/pfr/pfr.c
    freetype/src/psaux/psaux.c
    freetype/src/pshinter/pshinter.c
    freetype/src/psnames/psnames.c
    freetype/src/raster/raster.c
    freetype/src/sfnt/sfnt.c
    freetype/src/smooth/smooth.c
    freetype/src/truetype/truetype.c
    freetype/src/type1/type1.c
    freetype/src/type42/type42.c
    freetype/src/winfonts/winfnt.c
)

add_library(freetype STATIC ${FREETYPE_SOURCES})
target_include_directories(freetype PUBLIC freetype/include)

if(WIN32)
    target_compile_definitions(freetype PRIVATE
        _CRT_SECURE_NO_WARNINGS
        _CRT_NONSTDC_NO_WARNINGS
        FT2_BUILD_LIBRARY
    )
else()
    target_compile_definitions(freetype PRIVATE FT2_BUILD_LIBRARY)
endif()


# de Soras
#file(GLOB RESAMPLER_SOURCES resampler/*.cpp)

#add_library(resampler STATIC ${RESAMPLER_SOURCES})
#target_include_directories(resampler PUBLIC resampler)
