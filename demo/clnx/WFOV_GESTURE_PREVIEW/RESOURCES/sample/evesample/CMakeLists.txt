
cmake_minimum_required(VERSION 3.22 FATAL_ERROR)

add_executable(EveFpgaSdkSample)

target_sources(EveFpgaSdkSample
    PRIVATE
        pch.h

        EveFpgaSdkSample.cpp
)

target_include_directories(EveFpgaSdkSample PRIVATE "/opt/EVE-6.7.5-Source/include/EVE")
target_link_libraries(EveFpgaSdkSample PRIVATE /opt/EVE-6.7.5-Source/lib/libEveSDK.so)

if(MSVC)
    target_link_libraries(EveFpgaSdkSample PRIVATE "mfplat" "mf" "mfuuid" "mfreadwrite")
endif()
