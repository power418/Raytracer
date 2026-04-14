cmake_minimum_required(VERSION 3.15)

add_executable(
  ${PROJECT_NAME}
  WIN32
  main.cpp
  AntiAliasing.cpp
  Camera.cpp
  DemoApp.cpp
  SelectionController.cpp
  EndlessGrid.cpp
  GraphicsAdapter.cpp
  GpuRayTracer.cpp
  Renderer.cpp
  Window.cpp
  Collision.cpp
  ${PROJECT_SOURCE_DIR}/include/Camera.hpp
  ${PROJECT_SOURCE_DIR}/include/Collision.hpp
  ${PROJECT_SOURCE_DIR}/include/DemoApp.hpp
  ${PROJECT_SOURCE_DIR}/include/EndlessGrid.hpp
  ${PROJECT_SOURCE_DIR}/include/GraphicsAdapter.hpp
  ${PROJECT_SOURCE_DIR}/include/GpuRayTracer.hpp
  ${PROJECT_SOURCE_DIR}/include/AntiAliasing.hpp
  ${PROJECT_SOURCE_DIR}/include/RenderMode.hpp
  ${PROJECT_SOURCE_DIR}/include/Renderer.hpp
  ${PROJECT_SOURCE_DIR}/include/SceneTypes.hpp
  ${PROJECT_SOURCE_DIR}/include/SelectionController.hpp
  ${PROJECT_SOURCE_DIR}/include/Window.hpp
  ${PROJECT_SOURCE_DIR}/utils/Input.hpp
  ${PROJECT_SOURCE_DIR}/utils/KeyMap.hpp
  ${PROJECT_SOURCE_DIR}/utils/Math.hpp
  ${PROJECT_SOURCE_DIR}/utils/MathTypes.hpp
  ${PROJECT_SOURCE_DIR}/utils/MatrixMath.hpp
  ${PROJECT_SOURCE_DIR}/utils/VectorMath.hpp
  ${PROJECT_SOURCE_DIR}/res/shaders/fullscreen_vs.hlsl
  ${PROJECT_SOURCE_DIR}/res/shaders/endless_grid_vs.hlsl
  ${PROJECT_SOURCE_DIR}/res/shaders/endless_grid_ps.hlsl
  ${PROJECT_SOURCE_DIR}/res/shaders/copy_ps.hlsl
  ${PROJECT_SOURCE_DIR}/res/shaders/fxaa_ps.hlsl
  ${PROJECT_SOURCE_DIR}/res/shaders/raytrace_cs.hlsl
  ${PROJECT_SOURCE_DIR}/res/shaders/smaa_edge_ps.hlsl
  ${PROJECT_SOURCE_DIR}/res/shaders/smaa_blend_ps.hlsl
)

set_source_files_properties(
  ${PROJECT_SOURCE_DIR}/res/shaders/fullscreen_vs.hlsl
  PROPERTIES
    VS_SHADER_TYPE Vertex
    VS_SHADER_MODEL 5.0
    VS_SHADER_ENTRYPOINT main
)

set_source_files_properties(
  ${PROJECT_SOURCE_DIR}/res/shaders/endless_grid_vs.hlsl
  PROPERTIES
    VS_SHADER_TYPE Vertex
    VS_SHADER_MODEL 5.0
    VS_SHADER_ENTRYPOINT main
)

set_source_files_properties(
  ${PROJECT_SOURCE_DIR}/res/shaders/endless_grid_ps.hlsl
  PROPERTIES
    VS_SHADER_TYPE Pixel
    VS_SHADER_MODEL 5.0
    VS_SHADER_ENTRYPOINT main
)

set_source_files_properties(
  ${PROJECT_SOURCE_DIR}/res/shaders/copy_ps.hlsl
  PROPERTIES
    VS_SHADER_TYPE Pixel
    VS_SHADER_MODEL 5.0
    VS_SHADER_ENTRYPOINT main
)

set_source_files_properties(
  ${PROJECT_SOURCE_DIR}/res/shaders/fxaa_ps.hlsl
  PROPERTIES
    VS_SHADER_TYPE Pixel
    VS_SHADER_MODEL 5.0
    VS_SHADER_ENTRYPOINT main
)

set_source_files_properties(
  ${PROJECT_SOURCE_DIR}/res/shaders/raytrace_cs.hlsl
  PROPERTIES
    VS_SHADER_TYPE Compute
    VS_SHADER_MODEL 5.0
    VS_SHADER_ENTRYPOINT main
)

set_source_files_properties(
  ${PROJECT_SOURCE_DIR}/res/shaders/smaa_edge_ps.hlsl
  PROPERTIES
    VS_SHADER_TYPE Pixel
    VS_SHADER_MODEL 5.0
    VS_SHADER_ENTRYPOINT main
)

set_source_files_properties(
  ${PROJECT_SOURCE_DIR}/res/shaders/smaa_blend_ps.hlsl
  PROPERTIES
    VS_SHADER_TYPE Pixel
    VS_SHADER_MODEL 5.0
    VS_SHADER_ENTRYPOINT main
)

target_link_libraries(${PROJECT_NAME} PRIVATE d3d12 d3d11 dxgi d3dcompiler dxguid)
target_include_directories(
  ${PROJECT_NAME}
  PRIVATE
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/include
)

# Copy shader sources next to the executable so D3DCompileFromFile can find them at runtime
add_custom_command(
  TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${PROJECT_SOURCE_DIR}/res/shaders"
    "$<TARGET_FILE_DIR:${PROJECT_NAME}>/res/shaders"
  COMMENT "Copying shader sources to output directory"
)
