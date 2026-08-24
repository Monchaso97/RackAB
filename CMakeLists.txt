cmake_minimum_required(VERSION 3.22)
project(RackAB VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- JUCE ---
# Override with -DJUCE_PATH=/path/to/JUCE (needed on macOS / other machines).
if (NOT DEFINED JUCE_PATH)
    if (WIN32)
        set(JUCE_PATH "C:/Users/santi/Downloads/JUCE")
    else()
        set(JUCE_PATH "$ENV{HOME}/JUCE")
    endif()
endif()
add_subdirectory("${JUCE_PATH}" JUCE)

# Enable macOS AudioUnit hosting too when building on a Mac.
if (APPLE)
    set(RACKAB_HOST_AU 1)
else()
    set(RACKAB_HOST_AU 0)
endif()

juce_add_plugin(RackAB
    COMPANY_NAME "SlideOne"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Sld1
    PLUGIN_CODE Rkab
    FORMATS VST3 Standalone
    PRODUCT_NAME "RackAB")

target_sources(RackAB PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/HostChain.cpp)

target_compile_definitions(RackAB PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_PLUGINHOST_VST3=1
    JUCE_PLUGINHOST_AU=${RACKAB_HOST_AU})

if (MSVC)
    target_compile_options(RackAB PRIVATE /utf-8)
endif()

target_link_libraries(RackAB PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)
