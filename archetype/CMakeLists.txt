cmake_minimum_required(VERSION 3.10)
project(archetype)

# Configure the Compiler options, we will be using C++17 features.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(archetype)

add_executable(archetype_run main.cc)
target_link_libraries(archetype_run PUBLIC archetype)

include(GNUInstallDirs)
install(TARGETS archetype_run RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
