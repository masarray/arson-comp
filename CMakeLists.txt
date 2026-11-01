cmake_minimum_required(VERSION 3.22)
project(ArSonComp VERSION 0.2.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
option(ARSON_BUILD_PLUGIN "Build VST3 and standalone" ON)
option(ARSON_SANITIZE "Enable address/undefined sanitizers for engine tests" OFF)
# JUCE defaults JUCE_ASIO=0. Native Maono PS22 / professional ASIO drivers
# are otherwise absent from the Windows Standalone "Options" device list.
# Keep switch explicit so third-party ASIO SDK licensing can be audited.
option(ARSON_WINDOWS_ASIO "Enable native ASIO devices for Windows Standalone" ON)
add_library(arson_engine STATIC engine/Compressor.cpp)
set_target_properties(arson_engine PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(arson_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/engine)
if(MSVC)
  target_compile_options(arson_engine PRIVATE /W4)
else()
  target_compile_options(arson_engine PRIVATE -Wall -Wextra -Wpedantic)
endif()
if(ARSON_SANITIZE AND NOT MSVC)
  target_compile_options(arson_engine PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
  target_link_options(arson_engine PUBLIC -fsanitize=address,undefined)
endif()
include(CTest)
if(BUILD_TESTING)
  find_package(Threads REQUIRED)
  add_executable(engine_tests tests/EngineTests.cpp)
  target_link_libraries(engine_tests PRIVATE arson_engine Threads::Threads)
  add_test(NAME engine_contract COMMAND engine_tests)
  add_executable(engine_bench tests/Benchmark.cpp)
  target_link_libraries(engine_bench PRIVATE arson_engine)
  add_executable(rt_stress tests/RealtimeStress.cpp)
  target_link_libraries(rt_stress PRIVATE arson_engine)
endif()
if(ARSON_BUILD_PLUGIN)
  if(DEFINED JUCE_SOURCE_DIR)
    add_subdirectory(${JUCE_SOURCE_DIR} ${CMAKE_BINARY_DIR}/JUCE)
  else()
    include(FetchContent)
    FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
      GIT_TAG 8.0.12 GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
    execute_process(COMMAND git rev-parse HEAD WORKING_DIRECTORY "${juce_SOURCE_DIR}"
      OUTPUT_VARIABLE JUCE_CHECKED_SHA OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ERROR_IS_FATAL ANY)
    if(NOT JUCE_CHECKED_SHA STREQUAL "29396c22c93392d6738e021b83196283d6e4d850")
      message(FATAL_ERROR "JUCE tag does not match the audited commit")
    endif()
  endif()
  juce_add_plugin(ArSonComp COMPANY_NAME "ArSon Audio" BUNDLE_ID "com.arsonaudio.comp"
    PLUGIN_MANUFACTURER_CODE ArSn PLUGIN_CODE ArCp FORMATS VST3 Standalone
    PRODUCT_NAME "ArSonComp" IS_SYNTH FALSE NEEDS_MIDI_INPUT FALSE NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE EDITOR_WANTS_KEYBOARD_FOCUS FALSE COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx Dynamics)
  # Knob/toggle artwork and the approved flat brushed-metal fader pair
  # are compiled into the VST3/Standalone. No file-system reads at runtime.
  juce_add_binary_data(ArSonCompKnobAssets
    SOURCES assets/black_brushed_knob.jpg
            assets/sliders/arsoncomp_flat_track_480.png
            assets/sliders/arsoncomp_silver_thumb_96.png
            assets/toggle/arson_toggle_on.png
            assets/toggle/arson_toggle_off.png
            assets/arsoncomp_header_logo.png)
  target_sources(ArSonComp PRIVATE plugin/Processor.cpp plugin/Editor.cpp)
  if(MSVC)
    target_compile_options(ArSonComp PRIVATE /utf-8)
  endif()
  target_compile_definitions(ArSonComp PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)
  if(WIN32 AND ARSON_WINDOWS_ASIO)
    # PUBLIC propagates to the generated Standalone wrapper and plugin smoke.
    # Uses JUCE's bundled Steinberg ASIO headers (JUCE_ASIO_USE_EXTERNAL_SDK=0).
    # VST3 still uses the host-selected driver; this affects Standalone only.
    target_compile_definitions(ArSonComp PUBLIC JUCE_ASIO=1
      JUCE_ASIO_USE_EXTERNAL_SDK=0)
  endif()
  target_link_libraries(ArSonComp PRIVATE arson_engine ArSonCompKnobAssets juce::juce_audio_utils
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags)
  if(BUILD_TESTING)
    juce_add_console_app(plugin_smoke PRODUCT_NAME "ArSonCompSmoke")
    target_sources(plugin_smoke PRIVATE tests/PluginSmoke.cpp)
    target_link_libraries(plugin_smoke PRIVATE ArSonComp ArSonCompKnobAssets
      arson_engine juce::juce_audio_utils)
    add_test(NAME plugin_state_editor COMMAND plugin_smoke "${CMAKE_BINARY_DIR}/ArSonComp-preview.png")
    set_tests_properties(plugin_state_editor PROPERTIES TIMEOUT 30)
  endif()
endif()
