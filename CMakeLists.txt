cmake_minimum_required(VERSION 3.18)
project(raycandle VERSION 2.1.0 LANGUAGES C)
if(POLICY CMP0144)
  cmake_policy(SET CMP0144 NEW)
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)  # needed to link static raylib into .so

# Use the user's existing static raylib. Never fetch or build it.
if(WIN32)
  set(_raylib_default "C:/raylib/raylib/src")
else()
  set(_raylib_default "")
endif()
if(DEFINED ENV{RAYLIB_ROOT} AND NOT DEFINED RAYLIB_ROOT)
  set(_raylib_default "$ENV{RAYLIB_ROOT}")
endif()
set(RAYLIB_ROOT "${_raylib_default}" CACHE PATH "Directory containing raylib.h and static raylib")
find_path(RAYLIB_INCLUDE_DIR NAMES raylib.h
  HINTS "${RAYLIB_ROOT}" "${RAYLIB_ROOT}/src" "${RAYLIB_ROOT}/include")
if(MSVC AND EXISTS "${RAYLIB_ROOT}/libraylib.a")
  message(FATAL_ERROR "Found MinGW libraylib.a but CMake selected MSVC. Use bundled MinGW (automatically selected by setup.py) or configure with a MinGW generator.")
endif()
if(MSVC)
  # MSVC requires a compatible static .lib, not a MinGW .a.
  find_library(RAYLIB_STATIC_LIBRARY NAMES raylib_static raylib
    HINTS "${RAYLIB_ROOT}" "${RAYLIB_ROOT}/lib" "${RAYLIB_ROOT}/src")
else()
  # Explicit archive lookup supports the raylib source tree (src/libraylib.a).
  find_file(RAYLIB_STATIC_LIBRARY NAMES libraylib.a
    HINTS "${RAYLIB_ROOT}" "${RAYLIB_ROOT}/src" "${RAYLIB_ROOT}/lib" "${RAYLIB_ROOT}/lib64")
  if(NOT RAYLIB_STATIC_LIBRARY)
    find_library(RAYLIB_STATIC_LIBRARY NAMES raylib
      HINTS "${RAYLIB_ROOT}" "${RAYLIB_ROOT}/src" "${RAYLIB_ROOT}/lib")
  endif()
endif()
if(NOT RAYLIB_INCLUDE_DIR OR NOT RAYLIB_STATIC_LIBRARY)
  message(FATAL_ERROR
    "Expected installed STATIC raylib: raylib.h and "
    "${CMAKE_STATIC_LIBRARY_PREFIX}raylib${CMAKE_STATIC_LIBRARY_SUFFIX}. "
    "Set -DRAYLIB_ROOT to the directory containing both files. "
    "MinGW libraylib.a requires a MinGW generator/compiler on Windows. "
    "No download or raylib build will be attempted.")
endif()
message(STATUS "Using raylib header: ${RAYLIB_INCLUDE_DIR}/raylib.h")
message(STATUS "Using static raylib: ${RAYLIB_STATIC_LIBRARY}")
add_library(raycandle_raylib STATIC IMPORTED)
set_target_properties(raycandle_raylib PROPERTIES
  IMPORTED_LOCATION "${RAYLIB_STATIC_LIBRARY}"
  INTERFACE_INCLUDE_DIRECTORIES "${RAYLIB_INCLUDE_DIR}")
set(RAYCANDLE_RAYLIB_TARGET raycandle_raylib)

add_library(raycandle SHARED
  csrc/utils.c csrc/str.c csrc/figure.c csrc/node.c
  csrc/axes.c csrc/ready_signal.c csrc/artist.c csrc/view.c)
target_include_directories(raycandle PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/csrc")
target_link_libraries(raycandle PRIVATE ${RAYCANDLE_RAYLIB_TARGET})
if(WIN32)
  target_link_libraries(raycandle PRIVATE winmm gdi32 opengl32 shell32)
else()
  find_package(Threads REQUIRED)
  target_link_libraries(raycandle PRIVATE Threads::Threads m)
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # These libraries are needed when the user supplies a bare static libraylib.a
    # rather than a CMake raylib target with its own interface dependencies.
    find_package(OpenGL QUIET)
    if(TARGET OpenGL::GL)
      target_link_libraries(raycandle PRIVATE OpenGL::GL)
    endif()
    find_package(X11 QUIET)
    if(X11_FOUND)
      target_link_libraries(raycandle PRIVATE ${X11_LIBRARIES})
    endif()
    target_link_libraries(raycandle PRIVATE ${CMAKE_DL_LIBS})
  endif()
endif()
set_target_properties(raycandle PROPERTIES OUTPUT_NAME raycandle WINDOWS_EXPORT_ALL_SYMBOLS ON)
if(WIN32)
  set_target_properties(raycandle PROPERTIES PREFIX "")
else()
  set_target_properties(raycandle PROPERTIES PREFIX "lib")
endif()

# Keep the CFFI ABI header beside the native binary in the Python package.
# csrc/cffi.h is deliberately a clean, CFFI-readable public declaration header.
# Never copy the full native raycandle.h (which includes platform-specific types).
set(RAYCANDLE_PACKAGE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/raycandle")
add_custom_command(TARGET raycandle POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E make_directory "${RAYCANDLE_PACKAGE_DIR}"
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/csrc/cffi.h"
    "${RAYCANDLE_PACKAGE_DIR}/raycandle.h"
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different
    "$<TARGET_FILE:raycandle>"
    "${RAYCANDLE_PACKAGE_DIR}/$<TARGET_FILE_NAME:raycandle>"
  COMMENT "Staging native library and clean CFFI header in raycandle/"
)

install(TARGETS raycandle RUNTIME DESTINATION raycandle
  LIBRARY DESTINATION raycandle ARCHIVE DESTINATION lib)
install(FILES csrc/cffi.h DESTINATION raycandle RENAME raycandle.h)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/csrc/assets")
  install(DIRECTORY csrc/assets/ DESTINATION raycandle/assets
    FILES_MATCHING PATTERN "*.ttf" PATTERN "*.md")
endif()
