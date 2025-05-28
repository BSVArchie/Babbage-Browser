add_library(wallet STATIC
    WalletManager.cpp
)

target_include_directories(wallet PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${OPENSSL_INCLUDE_DIR}
)

target_link_libraries(wallet PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
)
