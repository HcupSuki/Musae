message(STATUS "Looking for turtle")

set(MUSAE_TURTLE_MINIMUM_REQUIRED 0.11)

if(NOT MUSAE_BUILTIN_TURTLE)
    find_package(turtle ${MUSAE_TURTLE_MINIMUM_REQUIRED})
    if(NOT turtle_FOUND)
        set(MUSAE_BUILTIN_TURTLE ON)
        message(NOTICE "***Notice: turtle not found (minimum required is ${MUSAE_TURTLE_MINIMUM_REQUIRED}). For the time turning on MUSAE_BUILTIN_TURTLE")
    endif()
endif()

if(MUSAE_BUILTIN_TURTLE)
    message(STATUS "Musae will use built-in turtle")
    # check built-in version
    if(MUSAE_BUILTIN_TURTLE_VERSION VERSION_LESS MUSAE_TURTLE_MINIMUM_REQUIRED)
        message(NOTICE "***Notice: Provided MUSAE_BUILTIN_TURTLE_VERSION is ${MUSAE_BUILTIN_TURTLE_VERSION}, which is less than the requirement (${MUSAE_TURTLE_MINIMUM_REQUIRED}). Changing to ${MUSAE_TURTLE_MINIMUM_REQUIRED}")
        set(MUSAE_BUILTIN_TURTLE_VERSION ${MUSAE_TURTLE_MINIMUM_REQUIRED})
    endif()
    # set download dest and URL
    set(MUSAE_BUILTIN_TURTLE_SRC_DIR "${MUSAE_PROJECT_3RDPARTY_DIR}/turtle-${MUSAE_BUILTIN_TURTLE_VERSION}")
    set(MUSAE_BUILTIN_TURTLE_URL "https://github.com/niess/turtle/archive/refs/tags/v${MUSAE_BUILTIN_TURTLE_VERSION}.tar.gz")
    # reuse or download
    include(FetchContent)
    if(EXISTS "${MUSAE_BUILTIN_TURTLE_SRC_DIR}/CMakeLists.txt")
        FetchContent_Declare(turtle SOURCE_DIR "${MUSAE_BUILTIN_TURTLE_SRC_DIR}")
        message(STATUS "Reusing turtle source ${MUSAE_BUILTIN_TURTLE_SRC_DIR}")
    else()
        FetchContent_Declare(turtle SOURCE_DIR "${MUSAE_BUILTIN_TURTLE_SRC_DIR}"
                                    URL "${MUSAE_BUILTIN_TURTLE_URL}")
        message(STATUS "turtle will be downloaded from ${MUSAE_BUILTIN_TURTLE_URL} to ${MUSAE_BUILTIN_TURTLE_SRC_DIR}")
    endif()
    # set options
    # uses CACHE INTERNAL variables to propagate options. see https://discourse.cmake.org/t/what-is-the-correct-way-to-set-options-of-a-project-before-fetch-content/268/4
    # configure it
    message(STATUS "Downloading (if required) and configuring turtle (version: ${MUSAE_BUILTIN_TURTLE_VERSION})")
    FetchContent_MakeAvailable(turtle)
    set(TURTLE_INCLUDE_DIR ${MUSAE_BUILTIN_TURTLE_SRC_DIR}/include)
    message(STATUS "Downloading (if required) and configuring turtle (version: ${MUSAE_BUILTIN_TURTLE_VERSION}) - done")
    # check download
    if(NOT EXISTS "${MUSAE_BUILTIN_TURTLE_SRC_DIR}/CMakeLists.txt")
        file(REMOVE_RECURSE "${CMAKE_BINARY_DIR}/_deps/turtle-build")
        file(REMOVE_RECURSE "${CMAKE_BINARY_DIR}/_deps/turtle-subbuild")
        message(FATAL_ERROR "It seems that the download of turtle has failed. You can try running cmake again, or manually download turtle from ${MUSAE_BUILTIN_TURTLE_URL} and extract it to ${MUSAE_PROJECT_3RDPARTY_DIR} (and keep the directory structure). If the error persists, you can try cleaning the build tree and restarting the build.")
    endif()
endif()

if(NOT MUSAE_BUILTIN_TURTLE)
    message(STATUS "Looking for turtle - found (version: ${turtle_VERSION})")
else()
    message(STATUS "Looking for turtle - built-in (version: ${MUSAE_BUILTIN_TURTLE_VERSION})")
endif()
