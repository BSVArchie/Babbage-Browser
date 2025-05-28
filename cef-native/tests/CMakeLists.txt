add_executable(wallet_test test_wallet.cpp)

target_include_directories(wallet_test PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(wallet_test PRIVATE wallet)
