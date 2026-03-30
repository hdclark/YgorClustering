
cmake_minimum_required (VERSION 3.12.0 FATAL_ERROR)

STRING(TIMESTAMP INVOCATION_TIMESTAMP "%Y%m%d.%H%M%S") # For a time-based version number.
project(YgorClustering VERSION ${INVOCATION_TIMESTAMP} LANGUAGES CXX)

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
    if(CMAKE_CXX_FLAGS MATCHES "-march=|-mcpu=|-mtune=")
        message(STATUS "Architecture set by user.")
    else()
        if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
            # Enable to fix linking errors for toolchains that do not auto-detect atomic intrinsics (e.g., some ARM systems).
            # Note: Binaries built this way should not be distributed.
            message(STATUS "No architecture set, adding march=native flag")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
        elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
            message(STATUS "No architecture set, adding mcpu=generic flag")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=generic")
        else()
            message(WARNING "Not able to set architecture, if compilation errors occur set architecture manually")
        endif()
    endif()
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
#                              Package Configuration
####################################################################################

include(CMakePackageConfigHelpers)

set(YGORCLUSTERING_CONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/YgorClustering")

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/YgorClusteringConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/YgorClusteringConfig.cmake"
    INSTALL_DESTINATION "${YGORCLUSTERING_CONFIG_INSTALL_DIR}"
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/YgorClusteringConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

install(EXPORT YgorClusteringTargets
        FILE YgorClusteringTargets.cmake
        NAMESPACE YgorClustering::
        DESTINATION "${YGORCLUSTERING_CONFIG_INSTALL_DIR}"
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/YgorClusteringConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/YgorClusteringConfigVersion.cmake"
    DESTINATION "${YGORCLUSTERING_CONFIG_INSTALL_DIR}"
)


####################################################################################
#                                    Packaging
####################################################################################

set(CPACK_GENERATOR "DEB")
#SET(CPACK_PACKAGE_NAME "ygorclustering")

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

