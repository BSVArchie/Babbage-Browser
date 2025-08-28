add_library(wallet STATIC
    WalletManager.cpp
)

target_include_directories(wallet PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${OPENSSL_INCLUDE_DIR}
    "C:/Users/archb/Dev/vcpkg/installed/x64-windows-static/include"
)

target_link_libraries(wallet PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
)
