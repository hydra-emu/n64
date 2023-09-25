cmake_minimum_required(VERSION 3.5)
project(n64hydra)
set(N64_FILES
    core.cxx
    core/n64_impl.cxx
    core/n64_cpu.cxx
    core/n64_rcp.cxx
    core/n64_rsp.cxx
    core/n64_rdp.cxx
    core/n64_vi.cxx
    core/n64_ai.cxx
    vendored/miniaudio.c
)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
add_subdirectory(vendored/fmt)
add_library(n64hydra SHARED ${N64_FILES})
target_include_directories(n64hydra PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} vendored/fmt/include vendored/)
target_link_libraries(n64hydra fmt::fmt)