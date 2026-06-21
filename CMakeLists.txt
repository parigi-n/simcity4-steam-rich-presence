cmake_minimum_required(VERSION 3.20)
project(SC4SteamRichPresence CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_SHARED_LIBRARY_PREFIX "")

set(GZCOM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/gzcom-dll/gzcom-dll)

file(GLOB GZCOM_ALL_SOURCES "${GZCOM_DIR}/src/*.cpp")
set(GZCOM_EXCLUDED
    "${GZCOM_DIR}/src/main.cpp"
    "${GZCOM_DIR}/src/cGZExampleDllDirector.cpp"
    "${GZCOM_DIR}/src/cGZCustomDllDirector.cpp"
    "${GZCOM_DIR}/src/cGZExtraCheatsDllDirector.cpp"
    "${GZCOM_DIR}/src/cGZExtraExtraCheatsDllDirector.cpp"
    "${GZCOM_DIR}/src/cGZCityHallUpgradeDllDirector.cpp"
)
list(REMOVE_ITEM GZCOM_ALL_SOURCES ${GZCOM_EXCLUDED})

set(STEAMWORKS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/steamworks_sdk)

add_library(SC4SteamRichPresence SHARED
    src/SC4SteamRichPresenceDirector.cpp
    src/Log.cpp
    src/SteamAPI.cpp
    src/RichPresenceService.cpp
    ${GZCOM_ALL_SOURCES}
)

target_include_directories(SC4SteamRichPresence PRIVATE
    ${GZCOM_DIR}/include
    ${STEAMWORKS_DIR}/public
)

target_compile_definitions(SC4SteamRichPresence PRIVATE
    WIN32_LEAN_AND_MEAN
    _WIN32_WINNT=0x0501
)

target_link_options(SC4SteamRichPresence PRIVATE
    -static-libgcc
    -static-libstdc++
    -static
    -Wl,--enable-stdcall-fixup
    -Wl,--kill-at
)

target_link_libraries(SC4SteamRichPresence PRIVATE user32 kernel32)

set_target_properties(SC4SteamRichPresence PROPERTIES
    OUTPUT_NAME "SC4SteamRichPresence"
    SUFFIX ".dll"
)
