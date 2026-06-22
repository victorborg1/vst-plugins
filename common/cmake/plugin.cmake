function(red_get_plugin_name NAME_VAR)
    get_filename_component(PLUGIN_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
    set(${NAME_VAR} ${PLUGIN_NAME} PARENT_SCOPE)
endfunction()

function(build_vst3 plug_sources)
    red_get_plugin_name(PLUGIN_NAME)
    set(target ${PLUGIN_NAME})
    message(STATUS "Resources source: ${CMAKE_CURRENT_SOURCE_DIR}/Resources")
    message(STATUS "Resources dest: ${CMAKE_CURRENT_BINARY_DIR}/VST3/Debug/${PLUGIN_NAME}.vst3/Contents/Resources")
    smtg_add_vst3plugin(${target} ${plug_sources})

    file(GLOB_RECURSE plug_headers
        "${CMAKE_CURRENT_SOURCE_DIR}/source/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/source/*.hpp"
    )
    target_sources(${target} PRIVATE ${plug_headers})

    target_include_directories(${target} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/source
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/common/source
        ${CMAKE_SOURCE_DIR}/common/source/gui
    )

    target_link_libraries(${target} PRIVATE
        common
        sdk
    )

    file(GLOB snapshots "resource/*_snapshot.png" "resource/*_snapshot_2.0x.png")
    list(LENGTH snapshots snap_count)
    if(snap_count GREATER 0)
        smtg_target_add_plugin_snapshots(${target} RESOURCES ${snapshots})
    endif()

    if(SMTG_MAC)
        smtg_target_set_bundle(${target}
            BUNDLE_IDENTIFIER com.redHarmonics.${target}
            COMPANY_NAME "redHarmonics"
        )
        smtg_target_set_debug_executable(${target}
            "/Applications/VST3PluginTestHost.app"
            "--pluginfolder;$(BUILT_PRODUCTS_DIR)"
        )

    elseif(SMTG_WIN)
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resource/win32.rc")
            target_sources(${target} PRIVATE resource/win32.rc)
        endif()

        # copy res from plugin
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_CURRENT_SOURCE_DIR}/res"
                "${CMAKE_BINARY_DIR}/VST3/$<CONFIG>/${PLUGIN_NAME}.vst3/Contents/res"
            COMMENT "Copying res to VST3 bundle"
        )

        # copy res from common
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_SOURCE_DIR}/common/res"
                "${CMAKE_BINARY_DIR}/VST3/$<CONFIG>/${PLUGIN_NAME}.vst3/Contents/res"
            COMMENT "Copying common res to VST3 bundle"
        )

        if(MSVC)
            smtg_target_set_debug_executable(${target}
                "$(ProgramW6432)/Steinberg/VST3PluginTestHost/VST3PluginTestHost.exe"
                "--pluginfolder \"${CMAKE_CURRENT_BINARY_DIR}/VST3/$<CONFIG>/\""
            )
        endif()
    endif()

    source_group(
        TREE ${CMAKE_CURRENT_SOURCE_DIR}/source
        PREFIX "source"
        FILES
            ${plug_sources}
            ${plug_headers}
    )
endfunction()
