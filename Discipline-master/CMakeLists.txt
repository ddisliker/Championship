cmake_minimum_required(VERSION 3.15)
project(MyDB VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type")

# Флаги компилятора
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -O2)
endif()

# Собираем все исходники из папки src (чтобы не перечислять их руками)
file(GLOB_RECURSE SQL_ENGINE_SOURCES "src/*.cpp")

# 1. Основной консольный исполняемый файл (оригинальный)
add_executable(mydb main.cpp ${SQL_ENGINE_SOURCES})
target_include_directories(mydb PRIVATE include)

# 2. Наш НОВЫЙ исполняемый файл для веб-сервера
add_executable(mydb_server server.cpp ${SQL_ENGINE_SOURCES})
target_include_directories(mydb_server PRIVATE include)

# Поддержка Filesystem для старых версий GCC
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_libraries(mydb stdc++fs)
    target_link_libraries(mydb_server stdc++fs)
endif()

# Установка
install(TARGETS mydb mydb_server DESTINATION bin)

# Тесты (оригинальные)
add_executable(mydb_test tests/test_main.cpp ${SQL_ENGINE_SOURCES})
target_include_directories(mydb_test PRIVATE include)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_libraries(mydb_test stdc++fs)
endif()
