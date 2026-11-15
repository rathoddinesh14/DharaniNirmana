add_executable(dharaninirmana_terrain_tests
    height_field_tests.cpp
)

target_link_libraries(dharaninirmana_terrain_tests
    PRIVATE DharaniNirmana::Terrain
)

target_compile_features(dharaninirmana_terrain_tests PRIVATE cxx_std_20)

add_test(NAME DharaniNirmana.Terrain COMMAND dharaninirmana_terrain_tests)
