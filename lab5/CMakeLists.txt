cmake_minimum_required(VERSION 3.14)
project(LAB5 VERSION 1.0 LANGUAGES CXX)

# --- Настройки стандарта ---
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Папки ---
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(INC_DIR ${CMAKE_SOURCE_DIR}/include)
set(BIN_DIR ${CMAKE_BINARY_DIR}/bin)
set(LIB_DIR ${CMAKE_BINARY_DIR}/lib)

# --- Поиск исходников библиотеки (все .cpp в src, кроме main.cpp) ---
file(GLOB ALL_SRC "${SRC_DIR}/*.cpp")
list(FILTER ALL_SRC EXCLUDE REGEX ".*/main\\.cpp$")

# --- Логика выбора STATIC или INTERFACE библиотеки в зависимости от исходников ---
if(ALL_SRC)
    add_library(lab5lib STATIC ${ALL_SRC})
    target_include_directories(lab5lib PUBLIC ${INC_DIR})
    target_compile_features(lab5lib PUBLIC cxx_std_20)
    if (MSVC)
        target_compile_options(lab5lib PRIVATE /W4 /permissive-)
    else()
        target_compile_options(lab5lib PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    set_target_properties(lab5lib PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${LIB_DIR})
else()
    # Всё в заголовках/шаблонах — INTERFACE библиотека
    add_library(lab5lib INTERFACE)
    target_include_directories(lab5lib INTERFACE ${INC_DIR})
    target_compile_features(lab5lib INTERFACE cxx_std_20)
endif()

# --- Исполняемый файл (main.cpp) ---
set(MAIN_SRC ${SRC_DIR}/main.cpp)
if(EXISTS ${MAIN_SRC})
    add_executable(lab5_app ${MAIN_SRC})
    target_include_directories(lab5_app PRIVATE ${INC_DIR})
    target_link_libraries(lab5_app PRIVATE lab5lib)
    set_target_properties(lab5_app PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${BIN_DIR})
else()
    message(WARNING "main.cpp not found in ${SRC_DIR} — executable target not создан.")
endif()

# --- Опция сборки тестов (googletest) ---
option(BUILD_TESTS "Build unit tests with GoogleTest" ON)

if(BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/heads/main.zip
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()

    file(GLOB TEST_SOURCES "${CMAKE_SOURCE_DIR}/tests/*.cpp")
    if(TEST_SOURCES)
        add_executable(lab5_tests ${TEST_SOURCES})
        target_include_directories(lab5_tests PRIVATE ${INC_DIR})
        target_link_libraries(lab5_tests PRIVATE lab5lib GTest::gtest_main)
        set_target_properties(lab5_tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${BIN_DIR})
        include(GoogleTest)
        gtest_discover_tests(lab5_tests)
    else()
        message(STATUS "No tests found in ${CMAKE_SOURCE_DIR}/tests (skipping test target).")
    endif()
endif()

include(CTest)
