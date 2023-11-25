cmake_minimum_required(VERSION 3.5)
project(cerberus)
set(N64_FILES
    core.cxx
    core/n64_impl.cxx
    core/n64_cpu.cxx
    core/n64_rcp.cxx
    core/n64_rsp.cxx
    core/n64_rdp.cxx
    core/n64_vi.cxx
    core/n64_ai.cxx
)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
add_subdirectory(vendored/fmt)
add_library(cerberus SHARED ${N64_FILES})
target_include_directories(cerberus PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} vendored/fmt/include core/hydra/include)
target_link_libraries(cerberus fmt::fmt)