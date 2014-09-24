
cmake_minimum_required (VERSION 2.8.9)
project(gst-gles++)

SET ( CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMake/Modules )
SET ( CMAKE_INCLUDE_DIRECTORIES_PROJECT_BEFORE ON )


include_directories( ${CMAKE_INSTALL_PREFIX}/include/ )
include_directories( ${CMAKE_INSTALL_PREFIX}/include/freetype2/  )
include_directories( ${CMAKE_INSTALL_PREFIX}/include/sigc++-2.0/ )
include_directories( ${CMAKE_INSTALL_PREFIX}/include/fedlibrary/include )
include_directories( ${CMAKE_INSTALL_PREFIX}/include/fedlibrary/template_include )
include_directories( ${CMAKE_INSTALL_PREFIX}/include/libgles++ )

find_package( GLIB      REQUIRED )
find_package( GObject   REQUIRED )
find_package( GStreamer REQUIRED )


include_directories( ${GLIB_INCLUDE_DIRS}            )
include_directories( ${GOBJECT_INCLUDE_DIR}          )
include_directories( ${GSTREAMER_BASE_INCLUDE_DIRS}  )
include_directories( ${GSTREAMER_INCLUDE_DIRS}       )
include_directories( ${GSTREAMER_VIDEO_INCLUDE_DIRS} )

include_directories( ../gscore/include/plugin/ )
include_directories( ./include/ )


link_directories( ${CMAKE_FIND_ROOT_PATH}/lib )
link_directories( ${CMAKE_FIND_ROOT_PATH}/usr/lib )

IF(NOT CMAKE_BUILD_TYPE )
  SET(CMAKE_BUILD_TYPE Release CACHE STRING
    "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
    FORCE)
ENDIF()

IF( CMAKE_BUILD_TYPE STREQUAL "Debug" )
  set(  FEDLIB_VER
        -3.1.0-debug
     )
  set(  LIBGLES_VER
        -0.1.0-debug
     )
  set(  GST_GLES_VERSION
        -1.0.0-debug
     )
ELSE()
  set(  FEDLIB_VER
        -3.1.0
     )
  set(  LIBGLES_VER
        -0.1.0
     )
  set(  GST_GLES_VERSION
        -1.0.0
     )
ENDIF()

option (HAVE_NEON          "NEON technology"             OFF) 
option (HAVE_HARD_FLOAT    "Hard Float Support"          OFF) 
option (FORCE_PIC          "Force -fPIC option"           ON)
option (HAVE_OPENGL_ES_API "OPENGL ES API"               OFF)


if (HAVE_NEON)
  add_definitions( -mfpu=neon )
endif (HAVE_NEON)

if (HAVE_HARD_FLOAT)
  add_definitions( -mfloat-abi=hard )
endif (HAVE_HARD_FLOAT)

if (FORCE_PIC)
  # This option is available since cmake  (VERSION 2.8.9)
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif (FORCE_PIC)

if (HAVE_OPENGL_ES_API)
  add_definitions( -DOPENGL_ES_API )
endif (HAVE_OPENGL_ES_API)


set(  GST_GLES_INCLUDE
        ./include/gst-gles++-video-sink.h
)

set(  GST_GLES_SRC  
        ./src/gst-gles++.cpp
        ./src/gst-gles++-video-sink.cpp
)

set(  GST_GLES_LIBRARIES
      fedlibrary${FEDLIB_VER}
      ${GLIB_LIBRARIES}
      ${GOBJECT_LIBRARIES}
      ${GSTREAMER_BASE_LIBRARIES}
      ${GSTREAMER_LIBRARIES}
      ${GSTREAMER_VIDEO_LIBRARIES}
      gles++${LIBGLES_VER}
      glfw3
      rt
      Xrandr
      Xi
      Xxf86vm
      m
      Xrender
      Xext
      X11
      pthread
      xcb
      Xau
      Xdmcp
)

if (HAVE_OPENGL_ES_API)
set( GL_LIBRARIES 
     EGL
     GLESv2
   )
else()
set( GL_LIBRARIES 
     GL 
   )
endif (HAVE_OPENGL_ES_API)

IF( CMAKE_BUILD_TYPE STREQUAL "Debug" )
add_definitions(
                -ggdb3
		-D_DEBUG
)
ENDIF()

add_definitions(
                -Wall
                -std=c++0x
                -fpermissive
                -D__STDC_CONSTANT_MACROS
                -D_NO_FED_NAMESPACE
                -D_TRACING_EXCEPTION_RAISE
                -D_TRACING_EXCEPTION_CATCH
                -D_ENABLE_ERROR_INFO
                -D_ENABLE_LOG_INFO
                -D_ENABLE_RAW_INFO
                -D_ENABLE_METHOD_TRACING
                -D_ENABLE_VERBOSE_INFO 
                -D_ENABLE_ASSERTION_TRACING
		-DGLFW_INCLUDE_ES2
               )



add_library   ( gst-gles++${GST_GLES_VERSION}      MODULE  ${GST_GLES_SRC}     )


target_link_libraries( gst-gles++${GST_GLES_VERSION}   ${GST_GLES_LIBRARIES} ${GL_LIBRARIES} )
target_link_libraries( gst-gles++${GST_GLES_VERSION}   ${CMAKE_DL_LIBS}         )

