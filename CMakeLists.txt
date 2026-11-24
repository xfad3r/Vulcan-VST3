cmake_minimum_required(VERSION 3.22)

project(VulcanByDJSmartie VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Automatically fetch Microsoft.Web.WebView2 SDK headers on Windows
if(WIN32)
    set(WEBVIEW2_DIR "${CMAKE_BINARY_DIR}/_deps/webview2")
    set(WEBVIEW2_NUPKG "${CMAKE_BINARY_DIR}/_deps/webview2.nupkg")

    if(NOT EXISTS "${WEBVIEW2_DIR}/build/native/include/WebView2.h")
        message(STATUS "Downloading Microsoft.Web.WebView2 SDK headers...")
        file(DOWNLOAD
            "https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2/1.0.2903.40"
            "${WEBVIEW2_NUPKG}"
            SHOW_PROGRESS
            STATUS WEBVIEW2_DL_STATUS
        )
        list(GET WEBVIEW2_DL_STATUS 0 WEBVIEW2_DL_CODE)
        if(NOT WEBVIEW2_DL_CODE EQUAL 0)
            message(FATAL_ERROR "Failed to download WebView2 SDK: ${WEBVIEW2_DL_STATUS}")
        endif()

        file(MAKE_DIRECTORY "${WEBVIEW2_DIR}")
        file(ARCHIVE_EXTRACT INPUT "${WEBVIEW2_NUPKG}" DESTINATION "${WEBVIEW2_DIR}")
    endif()

    set(JUCE_WEBVIEW2_PACKAGE_LOCATION "${WEBVIEW2_DIR}" CACHE PATH "" FORCE)
endif()

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(VulcanByDJSmartie
    COMPANY_NAME "DJSmartie"
    PLUGIN_MANUFACTURER_CODE Djsm
    PLUGIN_CODE Vulc
    FORMATS VST3 Standalone
    PRODUCT_NAME "VULCAN by DJSmartie"
    DESCRIPTION "VULCAN by DJSmartie"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

juce_generate_juce_header(VulcanByDJSmartie)

# Bundle WebUI/index.html directly into the compiled binary (zero external files)
juce_add_binary_data(VulcanWebUI
    SOURCES
        WebUI/index.html
)

target_sources(VulcanByDJSmartie
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
)

if(WIN32)
    target_include_directories(VulcanByDJSmartie
        PRIVATE
            "${WEBVIEW2_DIR}/build/native/include"
    )
endif()

target_compile_definitions(VulcanByDJSmartie
    PUBLIC
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2=1
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(VulcanByDJSmartie
    PRIVATE
        VulcanWebUI
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
