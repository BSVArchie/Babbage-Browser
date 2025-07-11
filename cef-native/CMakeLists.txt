cmake_minimum_required(VERSION 3.15)
project(BitcoinBrowserShell)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Paths
set(CEF_ROOT "../cef-binaries")
set(WRAPPER_LIB_PATH "${CEF_ROOT}/libcef_dll/wrapper/build/Release")
set(RELEASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../cef-binaries/Release")
set(RESOURCES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../cef-binaries/Resources")
set(OUTPUT_DIR "$<TARGET_FILE_DIR:BitcoinBrowserShell>")
set(CMAKE_TOOLCHAIN_FILE "C:/Users/archb/Dev/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

message(STATUS "OpenSSL include dir: ${OPENSSL_INCLUDE_DIR}")

# Output
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Shell source
set(SOURCES
    cef_browser_shell.cpp
    src/handlers/simple_handler.cpp
    src/handlers/simple_render_process_handler.cpp
    src/handlers/simple_app.cpp
    src/core/WalletManager.cpp
    src/core/IdentityHandler.cpp
    src/core/NavigationHandler.cpp
    # Add other source files here
)

# Create executable
add_executable(BitcoinBrowserShell WIN32 ${SOURCES})

set_target_properties(BitcoinBrowserShell PROPERTIES
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
)

target_sources(BitcoinBrowserShell PRIVATE
    src/core/WalletManager.cpp
)

# Include CEF headers
target_include_directories(BitcoinBrowserShell PRIVATE
    ${CEF_ROOT}
    ${CEF_ROOT}/include
    ${CEF_ROOT}/include/base
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core
    ${CMAKE_CURRENT_SOURCE_DIR}/include/handlers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/handlers
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
    ${OpenSSL_INCLUDE_DIR}
)

# Link libs
target_link_directories(BitcoinBrowserShell PRIVATE
    "${CEF_ROOT}/Release"
    "${WRAPPER_LIB_PATH}"
)

target_link_libraries(BitcoinBrowserShell PRIVATE
    libcef
    libcef_dll_wrapper.lib
    cef_sandbox
    user32
    gdi32
    ole32
    oleaut32
    comdlg32
    shlwapi
    uuid
    winmm
    dbghelp
    delayimp
    shell32
    advapi32
    version
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
)

# Delay-load
set_property(TARGET BitcoinBrowserShell PROPERTY LINK_FLAGS "/DELAYLOAD:libcef.dll")

# Required macros
add_definitions(-DUNICODE -D_UNICODE)

# Copy all runtime files after building
add_custom_command(TARGET BitcoinBrowserShell POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Copying CEF runtime files..."

    # Copy everything from Release/
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${RELEASE_DIR}"
        "${OUTPUT_DIR}"

    # Copy everything from Resources/
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${RESOURCES_DIR}"
        "${OUTPUT_DIR}"

    COMMAND ${CMAKE_COMMAND} -E echo "CEF files copied."
    COMMENT "Copying all CEF binaries and resources"
)

add_subdirectory(src/core)
add_subdirectory(tests)
