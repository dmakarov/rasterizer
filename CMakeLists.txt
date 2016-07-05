cmake_minimum_required(VERSION 2.8)
cmake_policy(SET CMP0015 NEW)

project(rasterizer)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall")

set(SOURCE_FILES
  src/canvas.cpp
  src/canvas.h
  src/control.cpp
  src/control.h
  src/editor.cpp
  src/editor.h
  src/rasterizer.cpp
  src/rasterizer.h
  src/main.cpp)

include_directories(/usr/local/include)

set(wxWidgets_USE_LIBS)
find_package(wxWidgets REQUIRED gl core base)
# Available libraries as reported by wx-config --help:
# adv aui base core gl html net propgrid qa ribbon richtext stc webview xml xrc
if(wxWidgets_FOUND)
  include("${wxWidgets_USE_FILE}")
  set(EXTRA_LIBS ${wxWidgets_LIBRARIES})
  add_executable(rasterizer ${SOURCE_FILES})
  target_link_libraries(rasterizer ${EXTRA_LIBS})
else(wxWidgets_FOUND)
  message("wxWidgets not found!")
endif(wxWidgets_FOUND)

# UNIT TESTS
option(build_tests "Build all unit tests." OFF)

if (build_tests)

  enable_testing()

  link_directories(build/gtest)
  add_executable(rasterizer_unittest tests/rasterizer_unittest.cpp src/rasterizer.cpp)
  target_include_directories(rasterizer_unittest PUBLIC src ../googletest/googletest/include)
  target_link_libraries(rasterizer_unittest gtest)
  add_test(rasterizer_unittest rasterizer_unittest)

endif()

# Local Variables:
# mode: cmake
# End:
