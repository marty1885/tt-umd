
add_library(test_common INTERFACE)
target_link_libraries(test_common INTERFACE umd_device gtest_main gtest pthread)
target_include_directories(test_common INTERFACE 
    ${PROJECT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/simulation)
if($ENV{ARCH_NAME} STREQUAL "wormhole_b0")
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/wormhole)
else()
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/$ENV{ARCH_NAME})
endif()

add_custom_target(umd_tests DEPENDS umd_unit_tests simulation_tests)
