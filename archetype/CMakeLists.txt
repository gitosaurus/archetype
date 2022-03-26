cmake_minimum_required(VERSION 3.10)
project(archetype)

# Configure the Compiler options, we will be using C++17 features.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(archetype)

add_executable(archetype_main main.cc)
target_link_libraries(archetype_main PUBLIC archetype)

find_package(Boost 1.66 REQUIRED COMPONENTS program_options)
find_package(Threads)
find_package(google_cloud_cpp_storage REQUIRED)

add_executable(gcs_update_server gcs_update_server.cc)
target_link_libraries(
  gcs_update_server PRIVATE Boost::headers
                            Boost::program_options
                            Threads::Threads
                            google-cloud-cpp::storage
                            archetype)

include(GNUInstallDirs)
install(TARGETS gcs_update_server RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
