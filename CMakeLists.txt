cmake_minimum_required(VERSION 3.25)

project(DharaniNirmana
    VERSION 0.1.0
    DESCRIPTION "A modular C++20 terrain generation engine"
    LANGUAGES CXX
)

option(DHARANINIRMANA_BUILD_TESTS "Build DharaniNirmana unit tests" ON)
option(DHARANINIRMANA_BUILD_RENDERING "Build optional Vishwakarma rendering integration" OFF)
option(DHARANINIRMANA_FETCH_ARYABHATTA
    "Fetch the pinned Aryabhatta revision when no installed package is available"
    ON
)

# Aryabhatta remains Core's approved foundational mathematical dependency.
find_package(Aryabhatta CONFIG QUIET)

if(NOT Aryabhatta_FOUND)
    if(NOT DHARANINIRMANA_FETCH_ARYABHATTA)
        message(FATAL_ERROR
            "Aryabhatta was not found. Install Aryabhatta or enable "
            "DHARANINIRMANA_FETCH_ARYABHATTA."
        )
    endif()

    include(FetchContent)

    FetchContent_Declare(
        aryabhatta
        GIT_REPOSITORY https://github.com/rathoddinesh14/Aryabhatta.git
        # Pin to the reviewed double-precision implementation. Do not track main.
        GIT_TAG 04408d3f001c037364f98807a2dca097b693c012
        GIT_SHALLOW FALSE
    )

    FetchContent_MakeAvailable(aryabhatta)
endif()

if(NOT TARGET Aryabhatta::Math)
    message(FATAL_ERROR
        "Aryabhatta was found/fetched, but it did not provide Aryabhatta::Math."
    )
endif()

# The rendering adapter remains outside this foundation increment.
if(DHARANINIRMANA_BUILD_RENDERING)
    message(FATAL_ERROR
        "DharaniNirmana Rendering is not implemented yet. "
        "Keep DHARANINIRMANA_BUILD_RENDERING=OFF."
    )
endif()

function(dharaninirmana_add_module target_name)
    add_library(DharaniNirmana_${target_name} INTERFACE)
    add_library(DharaniNirmana::${target_name} ALIAS DharaniNirmana_${target_name})
    target_compile_features(DharaniNirmana_${target_name} INTERFACE cxx_std_20)
    target_include_directories(DharaniNirmana_${target_name}
        INTERFACE
            $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
    )
endfunction()

dharaninirmana_add_module(Core)
dharaninirmana_add_module(Terrain)
dharaninirmana_add_module(Generation)
dharaninirmana_add_module(Processing)
dharaninirmana_add_module(Geometry)

# Keep the dependency public for consumers of the engine's Aryabhatta-based
# public APIs.
target_link_libraries(DharaniNirmana_Core
    INTERFACE
        Aryabhatta::Math
)

# Public dependency graph. Keep these edges aligned with the architecture docs.
target_link_libraries(DharaniNirmana_Terrain INTERFACE DharaniNirmana::Core)
target_link_libraries(DharaniNirmana_Generation INTERFACE
    DharaniNirmana::Core
    DharaniNirmana::Terrain
)
target_link_libraries(DharaniNirmana_Processing INTERFACE
    DharaniNirmana::Core
    DharaniNirmana::Terrain
)
target_link_libraries(DharaniNirmana_Geometry INTERFACE
    DharaniNirmana::Core
    DharaniNirmana::Terrain
)

if(DHARANINIRMANA_BUILD_TESTS)
    include(CTest)
    if(BUILD_TESTING)
        add_subdirectory(tests/core)
    endif()
endif()
