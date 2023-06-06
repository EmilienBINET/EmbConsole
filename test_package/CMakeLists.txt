cmake_minimum_required(VERSION 3.15)
project(PackageTest CXX)

find_package(embconsole CONFIG REQUIRED)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(example src/example.cpp)
target_link_libraries(example embconsole::embconsole)
target_link_libraries(example Threads::Threads)