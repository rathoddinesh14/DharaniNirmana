add_executable(dharaninirmana_core_tests
    test_main.cpp
)

target_link_libraries(dharaninirmana_core_tests
    PRIVATE DharaniNirmana::Core
)

target_compile_features(dharaninirmana_core_tests PRIVATE cxx_std_20)

add_test(NAME DharaniNirmana.Core COMMAND dharaninirmana_core_tests)
