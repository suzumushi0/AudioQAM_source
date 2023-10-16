cmake_minimum_required(VERSION 3.14.0)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.13 CACHE STRING "")

set(vst3sdk_SOURCE_DIR "D:/VST/VST_SDK/vst3sdk")
if(NOT vst3sdk_SOURCE_DIR)
    message(FATAL_ERROR "Path to VST3 SDK is empty!")
endif()

project(AudioQAM
    # This is your plug-in version number. Change it here only.
    # Version number symbols usable in C++ can be found in
    # source/version.h and ${PROJECT_BINARY_DIR}/projectversion.h.
    VERSION 1.1.1.0 
    DESCRIPTION "AudioQAM VST 3 Plug-in"
)

# suzumushi
unset(SMTG_CREATE_PLUGIN_LINK)
set (SMTG_CXX_STANDARD "20")

set(SMTG_VSTGUI_ROOT "${vst3sdk_SOURCE_DIR}")

add_subdirectory(${vst3sdk_SOURCE_DIR} ${PROJECT_BINARY_DIR}/vst3sdk)
smtg_enable_vst3_sdk()

smtg_add_vst3plugin(AudioQAM
    source/version.h
    source/AQcids.h
    source/AQprocessor.h
    source/AQprocessor.cpp
    source/AQcontroller.h
    source/AQcontroller.cpp
    source/AQentry.cpp
    source/AQparam.h
    source/AQHilbert.h
    source/AQDDS.h
    source/AQFIRfilters.h
    source/SO2ndordIIRfilters.h
    source/SODDL.h
    source/SOextparam.h
    source/SOextparam.cpp
)

#- VSTGUI Wanted ----
if(SMTG_ENABLE_VSTGUI_SUPPORT)
    target_sources(AudioQAM
        PRIVATE
            resource/AudioQAM.uidesc
    )
    target_link_libraries(AudioQAM
        PRIVATE
            vstgui_support
    )
    smtg_target_add_plugin_resources(AudioQAM
        RESOURCES
            "resource/AudioQAM.uidesc"
    )
endif(SMTG_ENABLE_VSTGUI_SUPPORT)
# -------------------

smtg_target_add_plugin_snapshots (AudioQAM
    RESOURCES
        resource/ABBF37A85C5951B5867058CE44D5B3BB_snapshot.png
        resource/ABBF37A85C5951B5867058CE44D5B3BB_snapshot_2.0x.png
)

target_link_libraries(AudioQAM
    PRIVATE
        sdk
)

smtg_target_configure_version_file(AudioQAM)

if(SMTG_MAC)
    smtg_target_set_bundle(AudioQAM
        BUNDLE_IDENTIFIER foo
        COMPANY_NAME "suzumushi"
    )
    smtg_target_set_debug_executable(AudioQAM
        "/Applications/VST3PluginTestHost.app"
        "--pluginfolder;$(BUILT_PRODUCTS_DIR)"
    )
elseif(SMTG_WIN)
    target_sources(AudioQAM PRIVATE 
        resource/win32resource.rc
    )
    if(MSVC)
        set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT AudioQAM)

        smtg_target_set_debug_executable(AudioQAM
            "$(ProgramW6432)/Steinberg/VST3PluginTestHost/VST3PluginTestHost.exe"
            "--pluginfolder \"$(OutDir)/\""
        )
    endif()
endif(SMTG_MAC)
