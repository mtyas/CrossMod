cmake_minimum_required(VERSION 3.22)

project(MidiFlux VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add JUCE and CLAP extensions
add_subdirectory(libs/JUCE EXCLUDE_FROM_ALL)
add_subdirectory(libs/clap-juce-extensions EXCLUDE_FROM_ALL)

set(PLUGIN_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND PLUGIN_FORMATS AU)
endif()

juce_add_plugin(MidiFlux
    COMPANY_NAME "MidiFlux Audio"
    BUNDLE_ID com.midiflux.midiflux
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    PLUGIN_MANUFACTURER_CODE MFlu
    PLUGIN_CODE MFlx
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "MidiFlux"
)

target_sources(MidiFlux PRIVATE
    # Common
    Source/Common/ScaleTheory.h
    Source/Common/RandomUtils.h
    Source/Common/MidiBlock.h
    Source/Common/MidiBlockFactory.h
    Source/Common/MidiBlockFactory.cpp

    # Blocks
    Source/Blocks/ProbabilityBlock.h
    Source/Blocks/ProbabilityBlock.cpp
    Source/Blocks/MutatorBlock.h
    Source/Blocks/MutatorBlock.cpp
    Source/Blocks/ArpeggiatorBlock.h
    Source/Blocks/ArpeggiatorBlock.cpp
    Source/Blocks/ChordBlock.h
    Source/Blocks/ChordBlock.cpp
    Source/Blocks/HumanizerBlock.h
    Source/Blocks/HumanizerBlock.cpp
    Source/Blocks/ScaleQuantizeBlock.h
    Source/Blocks/ScaleQuantizeBlock.cpp
    Source/Blocks/TimeQuantizeBlock.h
    Source/Blocks/TimeQuantizeBlock.cpp
    Source/Blocks/TransposeBlock.h
    Source/Blocks/TransposeBlock.cpp
    Source/Blocks/FilterBlock.h
    Source/Blocks/FilterBlock.cpp
    Source/Blocks/MapperBlock.h
    Source/Blocks/MapperBlock.cpp
    Source/Blocks/TransformBlock.h
    Source/Blocks/TransformBlock.cpp
    Source/Blocks/LfoBlock.h
    Source/Blocks/LfoBlock.cpp
    Source/Blocks/RatchetBlock.h
    Source/Blocks/RatchetBlock.cpp
    Source/Blocks/DelayBlock.h
    Source/Blocks/DelayBlock.cpp
    Source/Blocks/EuclideanBlock.h
    Source/Blocks/EuclideanBlock.cpp
    Source/Blocks/HarmonizerBlock.h
    Source/Blocks/HarmonizerBlock.cpp

    # Engine
    Source/Engine/MidiChainProcessor.h
    Source/Engine/MidiChainProcessor.cpp
    Source/Engine/PresetManager.h
    Source/Engine/PresetManager.cpp
    Source/Engine/UndoHistoryManager.h
    Source/Engine/UndoHistoryManager.cpp

    # UI
    Source/UI/CustomLookAndFeel.h
    Source/UI/CustomLookAndFeel.cpp
    Source/UI/BlockCardComponent.h
    Source/UI/BlockCardComponent.cpp
    Source/UI/RackComponent.h
    Source/UI/RackComponent.cpp
    Source/UI/HeaderComponent.h
    Source/UI/HeaderComponent.cpp
    Source/UI/MidiMonitorComponent.h
    Source/UI/MidiMonitorComponent.cpp
    Source/UI/VirtualKeyboardComponent.h
    Source/UI/VirtualKeyboardComponent.cpp

    # Plugin Core
    Source/PluginProcessor.h
    Source/PluginProcessor.cpp
    Source/PluginEditor.h
    Source/PluginEditor.cpp
)

target_include_directories(MidiFlux PRIVATE
    Source
)

target_compile_definitions(MidiFlux PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(MidiFlux PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_data_structures
    juce::juce_graphics
    juce::juce_core
    clap_juce_extensions
)

clap_juce_extensions_plugin(TARGET MidiFlux
    CLAP_ID "com.midiflux.midiflux"
    CLAP_FEATURES note-effect utility midi
)

# Automated Unit Tests
add_executable(MidiFluxTests Tests/UnitTests.cpp)
target_link_libraries(MidiFluxTests PRIVATE
    MidiFlux
    juce::juce_core
)
target_include_directories(MidiFluxTests PRIVATE Source)

