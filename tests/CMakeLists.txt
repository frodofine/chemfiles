# Update this value if you need to update the data file set
set(TESTS_DATA_GIT "2dcc78d4e7f9ffdcf5c3a35d1c882d6dc1405cd8")

if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/data/${TESTS_DATA_GIT}")
    message(STATUS "Downloading test data files")
    file(DOWNLOAD
        "https://github.com/chemfiles/tests-data/archive/${TESTS_DATA_GIT}.tar.gz"
        "${CMAKE_CURRENT_BINARY_DIR}/${TESTS_DATA_GIT}.tar.gz"
        SHOW_PROGRESS
        EXPECTED_HASH SHA1=07bae77d519d274a9611940329e1cc94dc6286f3

    )

    message(STATUS "Unpacking test data files")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E remove_directory data
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )

    execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar xf ${TESTS_DATA_GIT}.tar.gz
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )

    execute_process(
        COMMAND ${CMAKE_COMMAND} -E rename tests-data-${TESTS_DATA_GIT} data
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )

    execute_process(
        COMMAND ${CMAKE_COMMAND} -E touch data/${TESTS_DATA_GIT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
endif()

option(CHFL_TESTS_USE_VALGRIND OFF "Run tests using valgrind")
if(CHFL_TESTS_USE_VALGRIND)
    find_program(VALGRIND_EXECUTABLE valgrind)
    set(
        TEST_RUNNER
        ${VALGRIND_EXECUTABLE} "--dsymutil=yes" "--error-exitcode=125"
        "--leak-check=full" "--show-leak-kinds=all" "--track-origins=yes"
        "--suppressions=${CMAKE_CURRENT_SOURCE_DIR}/valgrind.supp"
        # don't warn for replacement of operator new/delete
        "--soname-synonyms=somalloc=nouserintercepts"
    )
else()
    set(TEST_RUNNER "")
endif()

# Pick up CMAKE_CROSSCOMPILING_EMULATOR to run tests if it is defined
set(TEST_RUNNER "${CMAKE_CROSSCOMPILING_EMULATOR};${TEST_RUNNER}")

if(MSVC)
    string(REGEX REPLACE "/Wall" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    string(REGEX REPLACE "/Wall" "/W4" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
endif()

add_subdirectory(external)

function(chfl_define_test _name_)
    add_test(NAME ${_name_}
        COMMAND ${TEST_RUNNER} ./${_name_}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )

    set_tests_properties(${_name_}
        PROPERTIES ENVIRONMENT "CHFL_TESTS_USE_VALGRIND=${CHFL_TESTS_USE_VALGRIND}"
    )

    if(WIN32)
        # We need to set the path to allow access to chemfiles.dll
        # (and any other DLL)
        STRING(REPLACE ";" "\\;" PATH_STRING "$ENV{PATH}")
        set_tests_properties(${_name_}
            PROPERTIES ENVIRONMENT "PATH=${PATH_STRING}\;$<TARGET_FILE_DIR:chemfiles>"
        )
    endif()
endfunction()

function(chfl_unit_test _name_ _file_)
    add_executable(${_name_} ${_file_} ${CHEMFILES_OBJECTS})
    # We need to pretend we are inside the DLL to access all functions
    target_compile_definitions(${_name_} PRIVATE chemfiles_EXPORTS)
    target_link_libraries(${_name_} $<TARGET_PROPERTY:chemfiles,LINK_LIBRARIES>)
    set_target_properties(${_name_} PROPERTIES LINKER_LANGUAGE CXX)
    target_include_directories(${_name_} SYSTEM PRIVATE ${EXTERNAL_INCLUDES})

    chfl_define_test(${_name_})
endfunction()

function(chfl_cpp_test _file_)
    get_filename_component(_name_ ${_file_} NAME_WE)
    chfl_unit_test(${_name_} ${_file_})
    target_link_libraries(${_name_} test_helpers)
endfunction()

file(GLOB all_test_files
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/files/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/formats/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/selections/*.cpp
)

foreach(test_file IN LISTS all_test_files)
    chfl_cpp_test(${test_file})
endforeach()

function(chfl_capi_test _file_)
    get_filename_component(_name_ ${_file_} NAME_WE)
    set(_name_ "c-${_name_}")
    chfl_unit_test(${_name_} ${_file_})
    target_link_libraries(${_name_} test_helpers)
endfunction()

file(GLOB capi_test_files ${CMAKE_CURRENT_SOURCE_DIR}/capi/*.cpp)
foreach(test_file IN LISTS capi_test_files)
    chfl_capi_test(${test_file})
endforeach()

file(RELATIVE_PATH VERSION_FILE_PATH ${CMAKE_CURRENT_BINARY_DIR} ${PROJECT_SOURCE_DIR}/VERSION)
target_compile_definitions(c-misc PRIVATE "-DVERSION_FILE_PATH=\"${VERSION_FILE_PATH}\"")

# BOOST asio (used for hostname test) requires some additional system
# libraries, link to them explicitly here
if("${CMAKE_CXX_COMPILER_ID}" MATCHES "PGI" AND "${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
    set(EXTRA_BOOST_ASIO_LIBS atomic)
elseif("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "^(arm.*|ARM.*)" OR
       "${CMAKE_SYSTEM_PROCESSOR}" MATCHES "^(aarch64.*|AARCH64.*)" OR
       "${CMAKE_SYSTEM_PROCESSOR}" MATCHES "^(powerpc|ppc)64le" OR
       "${CMAKE_SYSTEM_PROCESSOR}" MATCHES "^(powerpc|ppc)64"
   )
    find_package(Threads REQUIRED)
    set(EXTRA_BOOST_ASIO_LIBS ${CMAKE_THREAD_LIBS_INIT})
endif()
target_link_libraries(utils ${EXTRA_BOOST_ASIO_LIBS})

function(chfl_doctest _name_ _file_)
    if(NOT ${CHFL_BUILD_DOCTESTS})
        return()
    endif()

    set(_run_ TRUE)
    file(STRINGS ${_file_} _FILE_CONTENT_)
    foreach(_line_ ${_FILE_CONTENT_})
        if("${_line_}" MATCHES "\\[no-run\\]")
            set(_run_ FALSE)
        endif()
    endforeach()

    add_executable(${_name_} ${_file_})
    target_link_libraries(${_name_} chemfiles)
    target_link_libraries(${_name_} ${NETCDF_LIBRARIES} ${ZLIB_LIBRARIES} ${LIBLZMA_LIBRARY})
    set_target_properties(${_name_} PROPERTIES LINKER_LANGUAGE CXX)
    set_target_properties(${_name_} PROPERTIES COMPILE_FLAGS "-UNDEBUG")
    target_include_directories(${_name_} SYSTEM PRIVATE ${EXTERNAL_INCLUDES})

    if(${_run_})
        chfl_define_test(${_name_})
    endif()

    if(${CMAKE_CXX_COMPILER_ID} MATCHES "PGI" AND ${CMAKE_SYSTEM_NAME} MATCHES "Linux")
        target_link_libraries(${_name_} atomic)
    endif()
endfunction()

function(chfl_c_doctest _file_)
    get_filename_component(_name_ ${_file_} NAME_WE)
    get_filename_component(_root_ ${_file_} PATH)
    get_filename_component(_root_ ${_root_} NAME)
    if(${_root_} MATCHES "chfl_")
        set(_name_ "${_root_}_${_name_}")
    endif()

    chfl_doctest(${_name_} ${_file_})

    if(NOT MSVC AND CHFL_BUILD_DOCTESTS)
        target_link_libraries(${_name_} m)
    endif()
endfunction()

function(chfl_cpp_doctest _file_)
    get_filename_component(_name_ ${_file_} NAME_WE)
    get_filename_component(_root_ ${_file_} PATH)
    get_filename_component(_root_ ${_root_} NAME)
    if(NOT ${_root_} MATCHES "doc")
        set(_name_ "${_root_}-${_name_}")
    endif()

    chfl_doctest(${_name_} ${_file_})
    if(${CHFL_BUILD_DOCTESTS})
        target_link_libraries(${_name_} test_helpers)
    endif()
endfunction()

file(GLOB_RECURSE doctest_files ${CMAKE_CURRENT_SOURCE_DIR}/doc/**.cpp)
foreach(test_file IN LISTS doctest_files)
    chfl_cpp_doctest(${test_file})
endforeach()

file(GLOB_RECURSE capi_doctest_files ${CMAKE_CURRENT_SOURCE_DIR}/doc/capi/**.c)
foreach(test_file IN LISTS capi_doctest_files)
    chfl_c_doctest(${test_file})
endforeach()

# Write sample configuration files for tests
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/../.chemfiles.toml
"[types]
Oh = \"O\"
Oz = \"O\"
N2 = \"N\"

[atoms.CH3]
mass = 15.0
charge = 0
full_name = \"methyl\"

[atoms.Zn]
charge = 1.8

[atoms.SO4]
mass = 96.0
charge = -2
full_name = \"sulfate\"
vdw_radius = 3.68
covalent_radius = 2.42
")

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/chemfiles.toml
"[types]
# This override the root .chemfiles.toml file
N2 = \"N4\"
")

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/local-file.toml
"[types]
Oz = \"F\"
")

# Code linting with Python scripts
if(${CMAKE_VERSION} VERSION_GREATER "3.12")
    # cmake 3.12 replaced find_package(PythonInterp) with find_package(Python)
    find_package(Python REQUIRED)
    set(PYTHON_EXECUTABLE ${Python_EXECUTABLE})
else()
    # cmake 2.8.12 only knows about python <= 3.3
    set(Python_ADDITIONAL_VERSIONS 3.9 3.8 3.7 3.6 3.5 3.4)
    find_package(PythonInterp REQUIRED)
endif()

function(chfl_lint _file_)
    get_filename_component(_name_ ${_file_} NAME_WE)
    add_test(NAME ${_name_}
        COMMAND ${PYTHON_EXECUTABLE} ${_file_}
    )
endfunction()

file(GLOB lint_scripts ${CMAKE_CURRENT_SOURCE_DIR}/lints/*.py)
foreach(script IN LISTS lint_scripts)
    chfl_lint(${script})
endforeach()
