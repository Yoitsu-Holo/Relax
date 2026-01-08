# Code Coverage Configuration
#
# This module provides functions to enable code coverage for tests
#
# Usage:
#   1. Add to your main CMakeLists.txt:
#      set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
#      include(CodeCoverage)
#
#   2. Call setup_target_for_coverage() for each test target

# Check for coverage prerequisites
find_program(GCOV_PATH gcov)
find_program(LCOV_PATH lcov)
find_program(GENHTML_PATH genhtml)

if(NOT GCOV_PATH)
    message(STATUS "gcov not found, coverage reports will not be available")
endif()

if(NOT LCOV_PATH)
    message(STATUS "lcov not found, HTML coverage reports will not be available")
endif()

if(NOT GENHTML_PATH)
    message(STATUS "genhtml not found, HTML coverage reports will not be available")
endif()

# Function to setup coverage for a target
function(setup_target_for_coverage TARGET_NAME)
    if(ENABLE_COVERAGE)
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(${TARGET_NAME} PRIVATE
                --coverage
                -fprofile-arcs
                -ftest-coverage
                -g
                -O0
            )
            target_link_libraries(${TARGET_NAME}
                --coverage
            )
        endif()
    endif()
endfunction()

# Function to create coverage report target
function(create_coverage_report REPORT_NAME TEST_TARGETS)
    if(ENABLE_COVERAGE AND LCOV_PATH AND GENHTML_PATH)
        # Check if gcovr is available
        find_program(GCOVR_PATH gcovr)

        # Create output directory - 使用标准化的目录结构
        if(${REPORT_NAME} STREQUAL "all")
            set(COVERAGE_OUTPUT_DIR ${CMAKE_BINARY_DIR}/coverage/lcov)
            set(GCOVR_OUTPUT_DIR ${CMAKE_BINARY_DIR}/coverage/gcovr)
        else()
            set(COVERAGE_OUTPUT_DIR ${CMAKE_BINARY_DIR}/coverage/${REPORT_NAME}_lcov)
            set(GCOVR_OUTPUT_DIR ${CMAKE_BINARY_DIR}/coverage/${REPORT_NAME}_gcovr)
        endif()

        # Add custom target for coverage report
        add_custom_target(${REPORT_NAME}_coverage
            # Create coverage output directory
            COMMAND ${CMAKE_COMMAND} -E make_directory ${COVERAGE_OUTPUT_DIR}

            # Clear previous coverage data
            COMMAND ${LCOV_PATH} --directory . --zerocounters

            # Run tests
            COMMAND ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION> --verbose

            # Capture coverage data
            COMMAND ${LCOV_PATH} --directory . --capture --output-file ${COVERAGE_OUTPUT_DIR}.info --ignore-errors inconsistent

            # Remove unwanted coverage data (system headers, etc.)
            COMMAND ${LCOV_PATH} --remove ${COVERAGE_OUTPUT_DIR}.info
                '/usr/*'
                '*/test/*'
                '*/googletest/*'
                '*/benchmark/*'
                '*/build/*'
                --output-file ${COVERAGE_OUTPUT_DIR}.cleaned.info
                --ignore-errors inconsistent,unused

            # Generate HTML report
            COMMAND ${GENHTML_PATH} -o ${COVERAGE_OUTPUT_DIR} ${COVERAGE_OUTPUT_DIR}.cleaned.info --ignore-errors mismatch,source

            # Print summary
            COMMAND ${LCOV_PATH} --list ${COVERAGE_OUTPUT_DIR}.cleaned.info --ignore-errors inconsistent

            # Generate gcovr reports if gcovr is available
            COMMAND ${CMAKE_COMMAND} -E echo ""
            COMMAND ${CMAKE_COMMAND} -E echo "Generating gcovr coverage reports..."
            COMMAND ${CMAKE_COMMAND} -E make_directory ${GCOVR_OUTPUT_DIR}
            COMMAND ${GCOVR_PATH}
                --root ${CMAKE_SOURCE_DIR}
                --html --html-details
                --output ${GCOVR_OUTPUT_DIR}/index.html
                --exclude '.*test.*'
                --exclude '.*build.*'
                --exclude '.*/usr/.*'
                --print-summary
            COMMAND ${GCOVR_PATH}
                --root ${CMAKE_SOURCE_DIR}
                --xml
                --output ${GCOVR_OUTPUT_DIR}/coverage.xml
                --exclude '.*test.*'
                --exclude '.*build.*'
                --exclude '.*/usr/.*'

            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Generating ${REPORT_NAME} coverage reports (lcov + gcovr)"
        )

        # Print report location
        add_custom_command(TARGET ${REPORT_NAME}_coverage POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo ""
            COMMAND ${CMAKE_COMMAND} -E echo "========================================="
            COMMAND ${CMAKE_COMMAND} -E echo "Coverage reports generated:"
            COMMAND ${CMAKE_COMMAND} -E echo "  LCOV HTML: ${COVERAGE_OUTPUT_DIR}/index.html"
            COMMAND ${CMAKE_COMMAND} -E echo "  gcovr HTML: ${GCOVR_OUTPUT_DIR}/index.html"
            COMMAND ${CMAKE_COMMAND} -E echo "  gcovr XML: ${GCOVR_OUTPUT_DIR}/coverage.xml"
            COMMAND ${CMAKE_COMMAND} -E echo "========================================="
        )
    endif()
endfunction()

# Function to add gcovr coverage report (alternative to lcov)
function(create_gcovr_coverage_report REPORT_NAME)
    find_program(GCOVR_PATH gcovr)

    if(ENABLE_COVERAGE AND GCOVR_PATH)
        # 设置输出目录 - 使用标准化的目录结构
        if(${REPORT_NAME} STREQUAL "all")
            set(COVERAGE_OUTPUT_DIR ${CMAKE_BINARY_DIR}/coverage/gcovr)
            set(XML_OUTPUT_DIR ${CMAKE_BINARY_DIR}/coverage/gcovr)
        else()
            set(COVERAGE_OUTPUT_DIR ${CMAKE_BINARY_DIR}/coverage/${REPORT_NAME}_gcovr)
            set(XML_OUTPUT_DIR ${CMAKE_BINARY_DIR}/coverage/${REPORT_NAME}_gcovr)
        endif()

        add_custom_target(${REPORT_NAME}_gcovr_coverage
            # Create coverage output directory
            COMMAND ${CMAKE_COMMAND} -E make_directory ${COVERAGE_OUTPUT_DIR}

            # Run tests
            COMMAND ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION> --verbose

            # Generate HTML report
            COMMAND ${GCOVR_PATH}
                --root ${CMAKE_SOURCE_DIR}
                --html --html-details
                --output ${COVERAGE_OUTPUT_DIR}/index.html
                --exclude '.*test.*'
                --exclude '.*build.*'
                --exclude '.*/usr/.*'
                --print-summary

            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Generating ${REPORT_NAME} gcovr coverage report"
        )

        # Also create XML report for CI systems
        add_custom_target(${REPORT_NAME}_gcovr_xml
            # Create coverage output directory
            COMMAND ${CMAKE_COMMAND} -E make_directory ${XML_OUTPUT_DIR}

            COMMAND ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION> --verbose
            COMMAND ${GCOVR_PATH}
                --root ${CMAKE_SOURCE_DIR}
                --xml
                --output ${XML_OUTPUT_DIR}/coverage.xml
                --exclude '.*test.*'
                --exclude '.*build.*'
                --exclude '.*/usr/.*'
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Generating ${REPORT_NAME} XML coverage report"
        )
    elseif(ENABLE_COVERAGE)
        message(STATUS "gcovr not found, alternative coverage reports will not be available")
    endif()
endfunction()