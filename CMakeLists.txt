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
# 🔥 CORRECCIÓN PARA VULKAN EN ANDROID:
# Desactiva la tabla de despacho avanzada de VkBootstrap que rompe la compilación por incompatibilidad de cabeceras.
add_compile_definitions(VK_BOOTSTRAP_DISABLE_DISPATCH_TABLE=1)

# 🔥 CORRECCIÓN GLM EXPERIMENTAL:
# Permite utilizar las extensiones de cuaterniones y funciones matemáticas en game_handler_callbacks.cpp
add_compile_definitions(GLM_ENABLE_EXPERIMENTAL=1)

# 🔥 CORRECCIÓN ADVERTENCIAS PUNTEROS NULOS (ANDROID NDK CLANG):
# Apaga de forma estricta las advertencias de nulabilidad cosméticas de la librería local vk_mem_alloc.h
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wno-nullability-completeness)
endif()

# 🛠️ MAPEO DE OPENSSL DINÁMICO (Actualizado a OpenSSL 3.0)
set(OPENSSL_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android/openssl/include")
set(OPENSSL_CRYPTO_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/android/openssl/arm64-v8a/libcrypto_3.so") # 👈 Actualizado con _3
set(OPENSSL_SSL_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/android/openssl/arm64-v8a/libssl_3.so")       # 👈 Actualizado con _3
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

# 🔥 CORRECCIÓN PARA IMGUI, VK-BOOTSTRAP Y LUA:
# Escanear los archivos fuente (.cpp y .c) de extern/ para que se compilen e incluyan en la build.
file(GLOB IMGUI_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/extern/imgui/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/extern/imgui/backends/imgui_impl_sdl3.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/extern/imgui/backends/imgui_impl_vulkan.cpp"
)
file(GLOB VK_BOOTSTRAP_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/extern/vk-bootstrap/src/*.cpp"
)
# Escanear dinámicamente todos los archivos .c de tu carpeta real de Lua 5.1.5
file(GLOB LUA_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/extern/lua-5.1.5/src/*.c"
)

# Compilación de la librería nativa principal sumando los fuentes de las librerías en extern/
add_library(wowee SHARED
    ${WOWEE_SOURCES}
    ${IMGUI_SOURCES}
    ${VK_BOOTSTRAP_SOURCES}
    ${LUA_SOURCES}
)

# 🔄 ESCANEO RECURSIVO AUTOMÁTICO DE SUBDIRECTORIOS EN EXTERN
file(GLOB_RECURSE EXTERN_ITEMS LIST_DIRECTORIES true "${CMAKE_CURRENT_SOURCE_DIR}/extern/*")

set(AUTOMATIC_EXTERN_INCLUDES "")
foreach(ITEM ${EXTERN_ITEMS})
    if(IS_DIRECTORY ${ITEM})
        list(APPEND AUTOMATIC_EXTERN_INCLUDES ${ITEM})
    endif()
endforeach()

# Mapeamos los directorio de inclusión nativos del motor y submódulos físicos
target_include_directories(wowee PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src            # 🔥 CORRECCIÓN: Permite resolver subcarpetas internas (como src/rendering/)
    ${CMAKE_CURRENT_SOURCE_DIR}/extern
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/imgui/backends
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/lua-5.1.5/src      # 🔥 CORRECCIÓN: Directorio de cabeceras de Lua mapeado correctamente
    ${AUTOMATIC_EXTERN_INCLUDES} # 👈 Mapea dinámicamente cada subcarpeta encontrada (incluyendo extern/vk-bootstrap)
)

# 🔥 CORRECCIÓN DEL LINKER: Enlazamos de forma explícita las librerías nativas de Android
# 'vulkan' resuelve las llamadas directas de la API. 'android' y 'log' completan el soporte de la plataforma.
target_link_libraries(wowee PUBLIC 
    OpenSSL::SSL 
    OpenSSL::Crypto
    SDL3::SDL3
    vulkan
    android
    log
)
