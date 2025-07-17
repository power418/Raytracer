cmake_minimum_required(VERSION 3.15)

add_executable(${PROJECT_NAME} WIN32 main.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE d3d12 d3d11 dxgi d3dcompiler dxguid)
target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})