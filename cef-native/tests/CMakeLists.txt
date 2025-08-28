add_executable(wallet_test test_wallet.cpp)

target_include_directories(wallet_test PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    "C:/Users/archb/Dev/vcpkg/installed/x64-windows-static/include"
)

target_link_libraries(wallet_test PRIVATE wallet)
