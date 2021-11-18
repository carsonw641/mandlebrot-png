cmake_minimum_required (VERSION 3.8)

project ("Mandlebrot")

set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Threads REQUIRED)
find_package(libpng CONFIG REQUIRED)

# Add source to this project's executable.
add_executable (Mandlebrot "main.cpp" )

target_link_libraries(Mandlebrot PRIVATE png_static Threads::Threads)

# cmake .. "-DCMAKE_TOOLCHAIN_FILE=~/vcpkg/scripts/buildsystems/vcpkg.cmake"