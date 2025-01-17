# This is the main CMake file for NCEPLIBS-ip.
#
# Mark Potts, Kyle Gerheiser
cmake_minimum_required(VERSION 3.15)

file(STRINGS "VERSION" pVersion)

project(
  g2c
  VERSION ${pVersion}
  LANGUAGES C)

include(GNUInstallDirs)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")


# Handle user options.
option(ENABLE_DOCS "Enable generation of doxygen-based documentation." OFF)
option(USE_PNG      "Use PNG library"      ON)
option(USE_Jasper   "Use Jasper library"   ON)
option(USE_OpenJPEG "Use OpenJPEG library" OFF)

# The user must select either Jasper of OpenJPEG
if(USE_Jasper AND USE_OpenJPEG)
  message(FATAL_ERROR "Either Jasper or OpenJPEG should be used, not both.")
endif()


if(NOT CMAKE_BUILD_TYPE MATCHES "^(Debug|Release|RelWithDebInfo|MinSizeRel)$")
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE
      "Release"
      CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
                                               "MinSizeRel" "RelWithDebInfo")
endif()

if(USE_PNG)
  find_package(ZLIB REQUIRED)
  find_package(PkgConfig REQUIRED)
  find_package(PNG REQUIRED)
else()
  message(STATUS "Will not build PNG support")
endif()

if(USE_Jasper)
  find_package(Jasper 2.0.25 REQUIRED)
else()
  message(STATUS "Will not build Jasper support")
endif()

if(USE_OpenJPEG)
  find_package(OpenJPEG REQUIRED)
else()
  message(STATUS "Will not build OpenJPEG support")
endif()

if(CMAKE_C_COMPILER_ID MATCHES "^(Intel)$")
  set(CMAKE_C_FLAGS "-g -traceback ${CMAKE_C_FLAGS}")
  set(CMAKE_C_FLAGS_RELEASE "-O3")
  set(CMAKE_C_FLAGS_DEBUG "-O0")
elseif(CMAKE_C_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
  set(CMAKE_C_FLAGS "-g ${CMAKE_C_FLAGS}")
  set(CMAKE_C_FLAGS_RELEASE "-O3")
  set(CMAKE_C_FLAGS_DEBUG "-O0 -ggdb -Wall")
endif()

set(lib_name ${PROJECT_NAME})

add_library(${lib_name} STATIC
    src/grib2_int.h
    src/cmplxpack.c
    src/compack.c
    src/comunpack.c
    src/drstemplates.c
    src/g2_addfield.c
    src/g2_addgrid.c
    src/g2_addlocal.c
    src/g2_create.c
    src/g2_free.c
    src/g2_getfld.c
    src/g2_gribend.c
    src/g2_info.c
    src/g2_miss.c
    src/g2_unpack1.c
    src/g2_unpack2.c
    src/g2_unpack3.c
    src/g2_unpack4.c
    src/g2_unpack5.c
    src/g2_unpack6.c
    src/g2_unpack7.c
    src/gbits.c
    src/getdim.c
    src/getpoly.c
    src/gridtemplates.c
    src/int_power.c
    src/misspack.c
    src/mkieee.c
    src/pack_gp.c
    src/pdstemplates.c
    src/rdieee.c
    src/reduce.c
    src/seekgb.c
    src/simpack.c
    src/simunpack.c
    src/specpack.c
    src/specunpack.c
    src/util.c
)

set_property(TARGET ${lib_name} PROPERTY C_STANDARD 99)

if(PNG_FOUND)
  message(STATUS "Found PNG:")
  message(STATUS "      PNG include directories ${PNG_INCLUDE_DIR}")
  message(STATUS "      PNG libraries ${PNG_LIBRARIES}")
  target_sources(${lib_name} PRIVATE src/dec_png.c
                                     src/enc_png.c
                                     src/pngpack.c
                                     src/pngunpack.c)
  target_compile_definitions(${lib_name} PRIVATE USE_PNG)
  target_link_libraries(${lib_name} INTERFACE ${PNG_LIBRARIES})
  target_include_directories(${lib_name} PRIVATE "${PNG_INCLUDE_DIRS}")
  set(G2_PNG_ENABLED ON)
endif()

if(JASPER_FOUND)
  message(STATUS "Found JASPER:")
  message(STATUS "      JASPER include directories ${JASPER_INCLUDE_DIR}")
  message(STATUS "      JASPER libraries ${JASPER_LIBRARIES}")
  target_sources(${lib_name} PRIVATE src/dec_jpeg2000.c
                                     src/enc_jpeg2000.c
                                     src/jpcpack.c
                                     src/jpcunpack.c)
  target_compile_definitions(${lib_name} PRIVATE USE_JPEG2000)
  target_link_libraries(${lib_name} INTERFACE ${JASPER_LIBRARIES})
  target_include_directories(${lib_name} PRIVATE "${JASPER_INCLUDE_DIR}")
  set(G2_JPEG2000_ENABLED ON)
endif()

if(OPENJPEG_FOUND)
  message(STATUS "Found OpenJPEG:")
  message(STATUS "      OpenJPEG include directories ${OpenJPEG_INCLUDE_DIRS}")
  message(STATUS "      OpenJPEG libraries ${OpenJPEG_LIBRARIES}")
  target_sources(${lib_name} PRIVATE src/decenc_openjpeg.c
                                     src/jpcpack.c
                                     src/jpcunpack.c)
  target_compile_definitions(${lib_name} PRIVATE USE_OPENJPEG)
  target_link_libraries(${lib_name} INTERFACE ${OpenJPEG_LIBRARIES})
  target_include_directories(${lib_name} PRIVATE "${OpenJPEG_INCLUDE_DIRS}")
  set(G2_JPEG2000_ENABLED ON)
endif()

# Create this header file, which has the contents of the VERSION file
# substituted in.
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/src/grib2.h.in" "${CMAKE_CURRENT_BINARY_DIR}/src/grib2.h"
  @ONLY)

set_target_properties(${lib_name} PROPERTIES PUBLIC_HEADER "${CMAKE_CURRENT_BINARY_DIR}/src/grib2.h")

target_include_directories(
  ${lib_name}
  PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/src;${CMAKE_CURRENT_SOURCE_DIR}/src>"
         $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>
)

target_link_libraries(${lib_name} INTERFACE m)

### Install g2c library
install(
  TARGETS ${lib_name}
  EXPORT ${PROJECT_NAME}Exports
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

### Package config
include(CMakePackageConfigHelpers)
set(CONFIG_INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})

export(EXPORT ${PROJECT_NAME}Exports
  NAMESPACE ${PROJECT_NAME}::
  FILE ${PROJECT_NAME}-targets.cmake)

# g2c-config.cmake
configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/PackageConfig.cmake.in ${CMAKE_BINARY_DIR}/${PROJECT_NAME}-config.cmake
  INSTALL_DESTINATION ${CONFIG_INSTALL_DESTINATION})
install(FILES ${CMAKE_BINARY_DIR}/${PROJECT_NAME}-config.cmake
  DESTINATION ${CONFIG_INSTALL_DESTINATION})

# g2c-config-version.cmake
write_basic_package_version_file(
  ${CMAKE_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion)
install(FILES ${CMAKE_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake
  DESTINATION ${CONFIG_INSTALL_DESTINATION})

# g2c-targets.cmake and g2c-targets-<build-type>.cmake
install(EXPORT ${PROJECT_NAME}Exports
  NAMESPACE ${PROJECT_NAME}::
  FILE ${PROJECT_NAME}-targets.cmake
  DESTINATION ${CONFIG_INSTALL_DESTINATION})

# Determine whether or not to generate documentation.
if(ENABLE_DOCS)
  find_package(Doxygen REQUIRED)
endif()
add_subdirectory(docs)

# Run unit tests.
include(CTest)
add_subdirectory(tests)
