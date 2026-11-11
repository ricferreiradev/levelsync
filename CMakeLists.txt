cmake_minimum_required(VERSION 3.22)

project(LevelSync VERSION 0.5.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "LevelSync requires Windows x64. Configure with -A x64.")
endif()

set(JUCE_DIR "C:/Dev/JUCE" CACHE PATH "Local JUCE source directory")
if(NOT EXISTS "${JUCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "JUCE not found at ${JUCE_DIR}. Set JUCE_DIR to the JUCE source directory.")
endif()
add_subdirectory("${JUCE_DIR}" "${CMAKE_BINARY_DIR}/JUCE" EXCLUDE_FROM_ALL)

juce_add_plugin(LevelSync
    COMPANY_NAME "LevelSync"
    BUNDLE_ID "com.levelsync.plugin"
    PLUGIN_MANUFACTURER_CODE Lsyn
    PLUGIN_CODE Lvsy
    PRODUCT_NAME "LevelSync"
    FORMATS VST3 Standalone
    VST3_CATEGORIES Fx Dynamics
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

target_sources(LevelSync PRIVATE
    Source/PluginProcessor.h
    Source/PluginProcessor.cpp
    Source/PluginEditor.h
    Source/PluginEditor.cpp
    Source/DSP/LoudnessDetector.h
    Source/DSP/BalanceController.h
    Source/DSP/GainSmoother.h
    Source/DSP/VocalActivityDetector.h
    Source/DSP/MusicSilenceDetector.h
    Source/DSP/BalanceEngine.h
    Source/DSP/ReferenceLearner.h
    Source/DSP/TelemetryQueue.h
    Source/DSP/PerformanceMetrics.h
    Source/DSP/SpectralAnalyzer.h
    Source/DSP/MaskingEstimator.h
    Source/DSP/SpectralReferenceLearner.h
    Source/DSP/PerceptualBalanceController.h
    Source/UI/LevelSyncLookAndFeel.h
    Source/UI/MeterComponents.h
    Source/UI/MeterComponents.cpp
)

include(CTest)
if(BUILD_TESTING)
    add_executable(LevelSyncDSPTests Tests/DSPTests.cpp)
    target_include_directories(LevelSyncDSPTests PRIVATE Source)
    target_link_libraries(LevelSyncDSPTests PRIVATE juce::juce_dsp)
    add_test(NAME LevelSync.DSP COMMAND LevelSyncDSPTests)

    add_executable(LevelSyncProcessorTests Tests/ProcessorTests.cpp)
    target_link_libraries(LevelSyncProcessorTests PRIVATE LevelSync)
    target_include_directories(LevelSyncProcessorTests PRIVATE Source "${JUCE_DIR}/modules")
    target_compile_definitions(LevelSyncProcessorTests PRIVATE JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1)
    add_test(NAME LevelSync.Processor COMMAND LevelSyncProcessorTests)

    add_executable(LevelSyncOfflineMetrics Tests/OfflineMetrics.cpp)
    target_include_directories(LevelSyncOfflineMetrics PRIVATE Source Tests)
    target_link_libraries(LevelSyncOfflineMetrics PRIVATE juce::juce_dsp)
    add_test(NAME LevelSync.OfflineMetrics COMMAND LevelSyncOfflineMetrics)
endif()

target_compile_definitions(LevelSync PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(LevelSync PRIVATE
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_audio_devices
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)
