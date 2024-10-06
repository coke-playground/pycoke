cmake_minimum_required(VERSION 3.15)

project(cpp_pycoke CXX)

set(CMAKE_CXX_STANDARD 20)

if (APPLE)
    add_compile_options(-fvisibility=default)
endif ()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter")

find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

add_subdirectory(workflow)
set(WORKFLOW_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/workflow/_include/)

add_subdirectory(pybind11)

set(COKE_BUILD_STATIC TRUE)
add_subdirectory(coke)

# workflow create include directory during the build phase, build it first
add_dependencies(coke-static workflow-static)

pybind11_add_module(cpp_pycoke
    src/pycoke.cpp
    src/simple_test.cpp
    src/complex_work.cpp
    src/background_worker.cpp
)

target_link_libraries(cpp_pycoke
    PRIVATE
    coke::coke-static
    workflow-static
    Threads::Threads
    OpenSSL::SSL OpenSSL::Crypto
)

target_compile_definitions(cpp_pycoke
    PRIVATE
    VERSION_INFO=${PYCOKE_VERSION_INFO}
)
