
cmake_minimum_required(VERSION 3.4.1)

add_subdirectory(librtmp)

add_library( # Sets the name of the library.
        native-lib

        # Sets the library as a shared library.
        SHARED

        # Provides a relative path to your source file(s).
        native-lib.cpp

        VideoChannel.cpp

        AudioChannel.cpp)

include_directories(include) #这个路径是相对CMakeLists.text的dir，不同版本AS中初始CMakeLists路径不同，这里有可能不同，比如会是include_directories(src/main/cpp/include)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${CMAKE_SOURCE_DIR}/libs/${ANDROID_ABI}" ) #CMAKE_CXX_FLAGS会被添加到build.ninja的FLAGS中，ANDROID_ABI是内置变量，不要直接写死armeabi-v7a

find_library( # Sets the name of the path variable.
        log-lib

        # Specifies the name of the NDK library that
        # you want CMake to locate.
        log)


target_link_libraries( # Specifies the target library.
        native-lib
        rtmp
        ${log-lib}
        x264
        faac)