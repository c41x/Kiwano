cmake_minimum_required(VERSION 2.8)

project(kiwano)

if(CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y -Wall -Wno-unused-value -Wno-int-to-pointer-cast -Wno-strict-aliasing -Wno-strict-overflow -msse -msse2 -msse3")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
endif()

include_directories("${PROJECT_SOURCE_DIR}/JUCE/")
link_libraries(uuid wsock32 wininet version ole32 ws2_32 oleaut32 imm32 comdlg32 shlwapi rpcrt4 winmm opengl32)
add_executable(kiwano
  "${PROJECT_SOURCE_DIR}/src/main.cpp"
  "${PROJECT_SOURCE_DIR}/JUCE/modules/juce_core/juce_core.cpp"
  "${PROJECT_SOURCE_DIR}/JUCE/modules/juce_gui_basics/juce_gui_basics.cpp"
  "${PROJECT_SOURCE_DIR}/JUCE/modules/juce_events/juce_events.cpp"
  "${PROJECT_SOURCE_DIR}/JUCE/modules/juce_opengl/juce_opengl.cpp"
  "${PROJECT_SOURCE_DIR}/JUCE/modules/juce_graphics/juce_graphics.cpp"
  "${PROJECT_SOURCE_DIR}/JUCE/modules/juce_data_structures/juce_data_structures.cpp")