cmake_minimum_required(VERSION 3.20)

project(ClotProgrammingLanguage VERSION 0.2.0 LANGUAGES C CXX)

option(CLOT_ENABLE_LLVM "Enable LLVM compilation backend" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE CLOT_SOURCES CONFIGURE_DEPENDS "src/*.cpp")

add_executable(clot ${CLOT_SOURCES})

target_include_directories(clot PRIVATE include)

if(MSVC)
    target_compile_options(clot PRIVATE /W4 /permissive-)
else()
    target_compile_options(clot PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(CLOT_ENABLE_LLVM)
    find_package(LLVM CONFIG QUIET)
    if(LLVM_FOUND)
        message(STATUS "LLVM encontrado: ${LLVM_PACKAGE_VERSION}")

        target_compile_definitions(clot PRIVATE CLOT_HAS_LLVM=1)
        target_include_directories(clot SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS})

        if(LLVM_DEFINITIONS)
            separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND "${LLVM_DEFINITIONS}")
            target_compile_definitions(clot PRIVATE ${LLVM_DEFINITIONS_LIST})
        endif()

        llvm_map_components_to_libnames(CLOT_LLVM_LIBS
            core
            support
            analysis
            passes
            target
            mc
            codegen
            targetparser
            native
        )

        target_link_libraries(clot PRIVATE ${CLOT_LLVM_LIBS})
    else()
        message(WARNING "LLVM no encontrado. --mode compile no estara disponible.")
    endif()
endif()

install(TARGETS clot RUNTIME DESTINATION bin)
