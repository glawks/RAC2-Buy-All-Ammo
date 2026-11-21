cmake_minimum_required(VERSION 3.20)
project(rac2_buy_all_ammo LANGUAGES C CXX)

# Download the pinned SDK with tools/fetch_sdk.py, or pass -DOPENRAC_SDK=/path/to/sdk.
if(NOT OpenRACMod_DIR)
  if(DEFINED OPENRAC_SDK)
    set(OpenRACMod_DIR "${OPENRAC_SDK}/cmake")
  else()
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/sdk.lock" lock)
    string(JSON sdk_version GET "${lock}" version)
    set(OpenRACMod_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.sdk/openrac-mod-sdk-${sdk_version}/cmake")
  endif()
endif()
find_package(OpenRACMod 1.20 REQUIRED)

openrac_add_mod(rac2_buy_all_ammo MANIFEST openrac-mod.json SOURCES src/mod.cpp)
# The OpenRAC SDK packages README/LICENSE automatically but not a manifest's touch file.
# Include this layout in every native platform ZIP, so Android's vendor Square action loads.
get_target_property(RAC2_PACKAGE_DIR rac2_buy_all_ammo OPENRAC_MOD_PACKAGE_DIR)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/touch.json"
               "${RAC2_PACKAGE_DIR}/touch.json" COPYONLY)
target_include_directories(rac2_buy_all_ammo PRIVATE src)

include(CTest)
if(BUILD_TESTING)
  add_executable(purchase_plan_test tests/purchase_plan_test.cpp)
  target_compile_features(purchase_plan_test PRIVATE cxx_std_17)
  target_include_directories(purchase_plan_test PRIVATE src)
  add_test(NAME purchase_plan COMMAND purchase_plan_test)
endif()
