cmake_minimum_required(VERSION 3.22)

project(VulcanByDJSmartie VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
