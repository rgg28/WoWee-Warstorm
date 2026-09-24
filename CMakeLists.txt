cmake_minimum_required(VERSION 3.15)
project(wowee VERSION 1.0.0 LANGUAGES C CXX)
include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Directorios de salida de compilación nativa para Android
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Desactivar opciones pesadas de escritorio y pruebas
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(WOWEE_BUILD_TESTS "Build tests" OFF)
option(WOWEE_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(WOWEE_ENABLE_TRACY "Enable Tracy profiler" OFF)
option(WOWEE_ENABLE_AMD_FSR2 "Enable AMD FSR2" OFF)
option(WOWEE_ENABLE_AMD_FSR3_FRAMEGEN "Enable AMD FSR3" OFF)

add_compile_definitions(WOWEE_HAS_AMD_FSR2=0)
add_compile_definitions(WOWEE_HAS_AMD_FSR3_FRAMEGEN=0)

# Compilación limpia de la librería nativa principal de WoWee para Android Studio
add_library(wowee SHARED
    src/main.cpp
)

# Mapeamos los directorios de inclusión nativos del motor, submódulos e incluye
target_include_directories(wowee PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/vk-bootstrap/src
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/lua-5.1.5/src
)

target_link_libraries(wowee PUBLIC 
    OpenSSL::SSL 
    OpenSSL::Crypto
)
