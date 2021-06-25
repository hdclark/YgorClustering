
cmake_minimum_required (VERSION 3.12.0 FATAL_ERROR)
project (YgorClustering LANGUAGES CXX)

####################################################################################
#                                  Dependencies
####################################################################################
# Note: Dependencies are listed in CPACK list below.

find_package(Threads REQUIRED)

####################################################################################
#                                  Configuration
####################################################################################
# Note: This library is header-only, but specifies build rules that apply to end- users.

# Set the release type. 
set(CMAKE_BUILD_TYPE Release) # Comment this line to use fallback default.
if(NOT CMAKE_BUILD_TYPE)
    # Default to debug builds.
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "default to debug" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF) # Disable GNU extensions (e.g., std=gnu++14).
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -Wall -Wextra -Wpedantic -Wno-unused-parameter")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

include(CheckCXXSymbolExists)
check_cxx_symbol_exists(__arm__     "cstdio" ARCH_IS_ARM)
check_cxx_symbol_exists(__aarch64__ "cstdio" ARCH_IS_ARM64)
if(ARCH_IS_ARM OR ARCH_IS_ARM64)
    message(STATUS "Detected ARM architecture.")

    # Enable to fix linking errors for toolchains that do not auto-detect atomic intrinsics (e.g., some ARM systems).
    # Note: Binaries built this way should not be distributed.
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
endif()

set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(BUILD_SHARED_LIBS TRUE)
set(POSITION_INDEPENDENT_CODE TRUE)
set(THREADS_PREFER_PTHREAD_FLAG ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # For clang-tidy et al.

# Use the directory where CMakeLists.txt is for inclusions.
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_INCLUDE_CURRENT_DIR_IN_INTERFACE ON)

include(GNUInstallDirs)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})


####################################################################################
#                                 Subdirectories
####################################################################################

add_subdirectory(src)
add_subdirectory(examples)


####################################################################################
#                                    Packaging
####################################################################################

set(CPACK_GENERATOR "DEB")
#SET(CPACK_PACKAGE_NAME "ygorclustering")

STRING(TIMESTAMP INVOCATION_TIMESTAMP "%Y%m%d.%H%M%S") # For a time-based version number.
SET(CPACK_PACKAGE_VERSION "${INVOCATION_TIMESTAMP}")

#SET(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64") # i386, amd64, armel, armhf, ...
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "C++ header-only library for data clustering.")
SET(CPACK_PACKAGE_CONTACT "hdeanclark@gmail.com")
SET(CPACK_PACKAGE_MAINTAINER "Haley Clark <hdeanclark@gmail.com>")
SET(CPACK_DEBIAN_PACKAGE_SECTION "Science")

set(BUILD_DEPENDENCY_PACKAGES "")
#if(WITH_BOOST)
list(APPEND BUILD_DEPENDENCY_PACKAGES "libboost-dev")
#endif()
list(JOIN BUILD_DEPENDENCY_PACKAGES ", " CPACK_DEBIAN_PACKAGE_DEPENDS)

include(CPack)

