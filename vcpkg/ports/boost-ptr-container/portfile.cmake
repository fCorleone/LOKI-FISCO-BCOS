# Automatically generated by scripts/boost/generate-ports.ps1

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO boostorg/ptr_container
    REF boost-1.81.0
    SHA512 c7907e4ab0d6ae37c81e22650e77b16d8fa8ab085332db78b3508b35a9ad31a93bdeddfbcc374c7da05f89d36b08263d53ae09a6d1b1c4a90ac514f0584f5562
    HEAD_REF master
)

include(${CURRENT_INSTALLED_DIR}/share/boost-vcpkg-helpers/boost-modular-headers.cmake)
boost_modular_headers(SOURCE_PATH ${SOURCE_PATH})