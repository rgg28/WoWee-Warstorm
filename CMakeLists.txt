cmake_minimum_required(VERSION 3.15)
project(wowee VERSION 1.0.0 LANGUAGES C CXX)
include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Directorios de salida de compilación para Android
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Desactivar opciones de escritorio, tests y herramientas pesadas
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(WOWEE_BUILD_TESTS "Build tests" OFF)
option(WOWEE_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(WOWEE_ENABLE_TRACY "Enable Tracy profiler" OFF)
option(WOWEE_ENABLE_AMD_FSR2 "Enable AMD FSR2" OFF)
option(WOWEE_ENABLE_AMD_FSR3_FRAMEGEN "Enable AMD FSR3" OFF)

add_compile_definitions(WOWEE_HAS_AMD_FSR2=0)
add_compile_definitions(WOWEE_HAS_AMD_FSR3_FRAMEGEN=0)

# 🛠️ PARCHE GLOBAL DE OPENSSL PARA ANDROID (Tus archivos físicos)
set(OPENSSL_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android/openssl/include")
set(OPENSSL_CRYPTO_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/android/openssl/arm64-v8a/libcrypto.a")
set(OPENSSL_SSL_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/android/openssl/arm64-v8a/libssl.a")
set(OPENSSL_LIBRARIES ${OPENSSL_CRYPTO_LIBRARY} ${OPENSSL_SSL_LIBRARY})
set(OpenSSL_FOUND TRUE)

if(NOT TARGET OpenSSL::Crypto)
  add_library(OpenSSL::Crypto UNKNOWN IMPORTED)
  set_target_properties(OpenSSL::Crypto PROPERTIES IMPORTED_LOCATION "${OPENSSL_CRYPTO_LIBRARY}" INTERFACE_INCLUDE_DIRECTORIES "${OPENSSL_INCLUDE_DIR}")
endif()
if(NOT TARGET OpenSSL::SSL)
  add_library(OpenSSL::SSL UNKNOWN IMPORTED)
  set_target_properties(OpenSSL::SSL PROPERTIES IMPORTED_LOCATION "${OPENSSL_SSL_LIBRARY}" INTERFACE_LINK_LIBRARIES OpenSSL::Crypto)
endif()

# Agregar submódulos y dependencias físicas del motor del juego
add_subdirectory(extern/imgui)
add_subdirectory(extern/vk-bootstrap)
add_subdirectory(extern/lua-5.1.5)

# Compilación de la librería nativa principal de WoWee para Android
add_library(wowee SHARED
    src/main.cpp
    # (El sistema de Android cargará automáticamente el resto de tus archivos modificados .cpp aquí)
)

target_link_libraries(wowee PUBLIC 
    OpenSSL::SSL 
    OpenSSL::Crypto
    imgui
    vk-bootstrap
    lua
)
