cmake_minimum_required(VERSION 2.8)
project(rasterizer)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall")

set(SOURCE_FILES
  src/canvas.cpp
  src/canvas.hpp
  src/main.cpp)

include_directories(/usr/local/include)

find_library(GLUI_LIBRARY glui)
if(APPLE)
  link_directories(/usr/local/lib)
  find_library(GLUT_LIBRARY GLUT)
  find_library(OPENGL_LIBRARY OpenGL)
  mark_as_advanced(GLUI_LIBRARY GLUT_LIBRARY OPENGL_LIBRARY)
  set(EXTRA_LIBS ${GLUI_LIBRARY} ${GLUT_LIBRARY} ${OPENGL_LIBRARY})
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")
else(APPLE)
  find_library(GLUT_LIBRARY glut)
  find_library(GLU_LIBRARY GLU)
  find_library(GL_LIBRARY GL)
  mark_as_advanced(GLUI_LIBRARY GLUT_LIBRARY GLU_LIBRARY GL_LIBRARY)
  set(EXTRA_LIBS ${GLUI_LIBRARY} ${GLUT_LIBRARY} ${GLU_LIBRARY} ${GL_LIBRARY})
endif(APPLE)

add_executable(rasterizer ${SOURCE_FILES})
target_link_libraries(rasterizer ${EXTRA_LIBS})

# Local Variables:
# mode: cmake
# End:
