cmake_minimum_required(VERSION 3.22)

project(LAB1898StyleMaster VERSION 0.1.0 LANGUAGES C CXX)

# JUCE is fetched at configure time. The 8.0.17-security tag keeps this project
# on the JUCE 8 line specified in the original technical brief.
include(FetchContent)
set(JUCE_BUILD_EXTRAS OFF CACHE BOOL "" FORCE)
set(JUCE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.17-security
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(LAB1898StyleMaster
    COMPANY_NAME             "LAB 1898"
    BUNDLE_ID                com.lab1898.stylemaster
    PLUGIN_MANUFACTURER_CODE Lb18
    PLUGIN_CODE              StM1
    PRODUCT_NAME             "LAB 1898 Style Master"
    DESCRIPTION              "Style-driven mastering and tone processor"
    FORMATS                  VST3 Standalone
    IS_SYNTH                 FALSE
    NEEDS_MIDI_INPUT         FALSE
    NEEDS_MIDI_OUTPUT        FALSE
    IS_MIDI_EFFECT           FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD  FALSE
)

juce_generate_juce_header(LAB1898StyleMaster)

target_sources(LAB1898StyleMaster PRIVATE
    Source/Plugin/PluginProcessor.cpp
    Source/Plugin/PluginEditor.cpp
    Source/DSP/Engine.cpp
    Source/Analysis/ReferenceAnalyzer.cpp
    Source/UI/NeumorphicLookAndFeel.cpp
)

target_compile_features(LAB1898StyleMaster PRIVATE cxx_std_17)
target_compile_definitions(LAB1898StyleMaster PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_REPORT_APP_USAGE=0
)

target_include_directories(LAB1898StyleMaster PRIVATE Source)
target_link_libraries(LAB1898StyleMaster PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_gui_extra
    PUBLIC juce::juce_recommended_config_flags
           juce::juce_recommended_lto_flags
           juce::juce_recommended_warning_flags
)

if(MSVC)
    target_compile_options(LAB1898StyleMaster PRIVATE /W4 /permissive- /EHsc)
endif()
