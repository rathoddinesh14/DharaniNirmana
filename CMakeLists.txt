cmake_minimum_required(VERSION 3.25)

project(DharaniNirmana
    VERSION 0.1.0
    DESCRIPTION "A modular C++20 terrain generation engine"
    LANGUAGES CXX
)

option(DHARANINIRMANA_BUILD_TESTS "Build DharaniNirmana unit tests" ON)
option(DHARANINIRMANA_BUILD_RENDERING "Build optional Vishwakarma rendering integration" OFF)

# The rendering adapter is intentionally not part of this foundation increment.
# Keep the public option now so the build contract is stable; fail clearly if
# someone enables an integration that has not yet been implemented.
if(DHARANINIRMANA_BUILD_RENDERING)
    message(FATAL_ERROR
        "DharaniNirmana Rendering is not implemented in this increment. "
        "Keep DHARANINIRMANA_BUILD_RENDERING=OFF.")
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

# Public dependency graph. Keep these edges aligned with docs/architecture/DEPENDENCY_RULES.md.
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
