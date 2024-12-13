message(STATUS "Looking for Mustard")

set(MUSAE_MUSTARD_MINIMUM_REQUIRED 0.8.0)

if(NOT MUSAE_BUILTIN_MUSTARD)
    find_package(Mustard ${MUSAE_MUSTARD_MINIMUM_REQUIRED})
    if(NOT Mustard_FOUND)
        set(MUSAE_BUILTIN_MUSTARD ON)
        message(NOTICE "***Notice: Mustard not found (minimum required is ${MUSAE_MUSTARD_MINIMUM_REQUIRED}). For the time turning on MUSAE_BUILTIN_MUSTARD")
    endif()
endif()

if(MUSAE_BUILTIN_MUSTARD)
    message(STATUS "Musae will use built-in Mustard")
    # check built-in version
    if(MUSAE_BUILTIN_MUSTARD_VERSION VERSION_LESS MUSAE_MUSTARD_MINIMUM_REQUIRED)
        message(NOTICE "***Notice: Provided MUSAE_BUILTIN_MUSTARD_VERSION is ${MUSAE_BUILTIN_MUSTARD_VERSION}, which is less than the requirement (${MUSAE_MUSTARD_MINIMUM_REQUIRED}). Changing to ${MUSAE_MUSTARD_MINIMUM_REQUIRED}")
        set(MUSAE_BUILTIN_MUSTARD_VERSION ${MUSAE_MUSTARD_MINIMUM_REQUIRED})
    endif()
    # set download dest and URL
    set(MUSAE_BUILTIN_MUSTARD_SRC_DIR "${MUSAE_PROJECT_3RDPARTY_DIR}/Mustard-main")
    set(MUSAE_BUILTIN_MUSTARD_URL "https://github.com/zhao-shihan/Mustard/archive/refs/heads/main.zip")
    # reuse or download
    include(FetchContent)
    if(EXISTS "${MUSAE_BUILTIN_MUSTARD_SRC_DIR}/CMakeLists.txt")
        FetchContent_Declare(Mustard SOURCE_DIR "${MUSAE_BUILTIN_MUSTARD_SRC_DIR}")
        message(STATUS "Reusing Mustard source ${MUSAE_BUILTIN_MUSTARD_SRC_DIR}")
    else()
        FetchContent_Declare(Mustard SOURCE_DIR "${MUSAE_BUILTIN_MUSTARD_SRC_DIR}"
                                     URL "${MUSAE_BUILTIN_MUSTARD_URL}")
        message(STATUS "Mustard will be downloaded from ${MUSAE_BUILTIN_MUSTARD_URL} to ${MUSAE_BUILTIN_MUSTARD_SRC_DIR}")
    endif()
    # set options
    set(MUSTARD_ENABLE_ASAN_IN_DEBUG_BUILD ${MUSAE_ENABLE_ASAN_IN_DEBUG_BUILD} CACHE INTERNAL "")
    set(MUSTARD_ENABLE_IPO ${MUSAE_ENABLE_IPO} CACHE INTERNAL "")
    set(MUSTARD_ENABLE_UBSAN_IN_DEBUG_BUILD ${MUSAE_ENABLE_UBSAN_IN_DEBUG_BUILD} CACHE INTERNAL "")
    set(MUSTARD_FULL_UNITY_BUILD ${MUSAE_FULL_UNITY_BUILD} CACHE INTERNAL "")
    set(MUSTARD_SHOW_EVEN_MORE_COMPILER_WARNINGS ${MUSAE_SHOW_EVEN_MORE_COMPILER_WARNINGS} CACHE INTERNAL "")
    set(MUSTARD_USE_SHARED_MSVC_RT ${MUSAE_USE_SHARED_MSVC_RT} CACHE INTERNAL "")
    # configure it
    message(STATUS "Downloading (if required) and configuring Mustard (version: ${MUSAE_BUILTIN_MUSTARD_VERSION})")
    FetchContent_MakeAvailable(Mustard)
    message(STATUS "Downloading (if required) and configuring Mustard (version: ${MUSAE_BUILTIN_MUSTARD_VERSION}) - done")
    # check download
    if(NOT EXISTS "${MUSAE_BUILTIN_MUSTARD_SRC_DIR}/CMakeLists.txt")
        file(REMOVE_RECURSE "${CMAKE_BINARY_DIR}/_deps/mustard-build")
        file(REMOVE_RECURSE "${CMAKE_BINARY_DIR}/_deps/mustard-subbuild")
        message(FATAL_ERROR "It seems that the download of Mustard has failed. You can try running cmake again, or manually download Mustard from ${MUSAE_BUILTIN_MUSTARD_URL} and extract it to ${MUSAE_PROJECT_3RDPARTY_DIR} (and keep the directory structure). If the error persists, you can try cleaning the build tree and restarting the build.")
    endif()
endif()

if(NOT MUSAE_BUILTIN_MUSTARD)
    message(STATUS "Looking for Mustard - found (version: ${Mustard_VERSION})")
else()
    message(STATUS "Looking for Mustard - built-in (version: ${MUSAE_BUILTIN_MUSTARD_VERSION})")
endif()
