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

# 🛠️ MAPEO DE OPENSSL DINÁMICO (.so.1.1)
set(OPENSSL_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android/openssl/include")
set(OPENSSL_CRYPTO_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/android/openssl/arm64-v8a/libcrypto.so.1.1")
set(OPENSSL_SSL_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/android/openssl/arm64-v8a/libssl.so.1.1")
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

# 📦 MAPEO DE SDL3 (UBICACIONES PROVISTAS POR EL SCRIPT DE ACTIONS)
set(SDL3_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/android/sdl3/arm64-v8a/libSDL3.so")

if(NOT TARGET SDL3::SDL3)
  add_library(SDL3::SDL3 SHARED IMPORTED)
  set_target_properties(SDL3::SDL3 PROPERTIES 
    IMPORTED_LOCATION "${SDL3_LIBRARY}"
  )
endif()

# 🔍 Escanear recursivamente todos los archivos .cpp y .c dentro de la carpeta src/
file(GLOB_RECURSE WOWEE_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

# Compilación de la librería nativa principal usando la lista automática de archivos encontrados
add_library(wowee SHARED
    ${WOWEE_SOURCES}
)

# Mapeamos los directorios de inclusión nativos del motor y submódulos físicos
target_include_directories(wowee PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/extern             # 👈 AQUÍ ESTÁ EL CAMBIO: Permite encontrar miniaudio.h de forma global
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/vk-bootstrap/src
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/lua-5.1.5/src
)

# Enlazamos las librerías nativas al ejecutable/librería principal
target_link_libraries(wowee PUBLIC 
    OpenSSL::SSL 
    OpenSSL::Crypto
    SDL3::SDL3     
)
