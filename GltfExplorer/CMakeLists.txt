add_executable(GltfExplorerDx12
"${PROJECT_SOURCE_DIR}/GltfExplorer/GltfExplorerMain.cpp"
"${PROJECT_SOURCE_DIR}/GltfExplorer/imgui_impl_render.cpp"
"${PROJECT_SOURCE_DIR}/GltfExplorer/imgui_impl_render.h"
"${PROJECT_SOURCE_DIR}/GltfExplorer/GltfLoader.cpp"
"${PROJECT_SOURCE_DIR}/GltfExplorer/GltfLoader.h"
"${PROJECT_SOURCE_DIR}/GltfExplorer/Logging.cpp"
"${PROJECT_SOURCE_DIR}/GltfExplorer/Logging.h"
"${PROJECT_SOURCE_DIR}/GltfExplorer/Scene.cpp"
"${PROJECT_SOURCE_DIR}/GltfExplorer/Scene.h"
"${PROJECT_SOURCE_DIR}/GltfExplorer/TextureLoader.cpp"
"${PROJECT_SOURCE_DIR}/GltfExplorer/TextureLoader.h"
"${PROJECT_SOURCE_DIR}/GltfExplorer/Camera/Camera.cpp"
"${PROJECT_SOURCE_DIR}/GltfExplorer/Camera/Camera.h"
"${PROJECT_SOURCE_DIR}/GltfExplorer/Camera/FlyCamera.cpp"
"${PROJECT_SOURCE_DIR}/GltfExplorer/Camera/FlyCamera.h"
)

target_link_libraries(GltfExplorerDx12 RenderDx12)

set_target_properties(GltfExplorerDx12
PROPERTIES
RUNTIME_OUTPUT_NAME_DEBUG "GltfExplorerDx12_Debug"
VS_DEBUGGER_WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

set_target_properties(GltfExplorerDx12
PROPERTIES
RUNTIME_OUTPUT_NAME_DEBUG "GltfExplorerDx12_Debug"
VS_DEBUGGER_WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

target_include_directories(GltfExplorerDx12
PRIVATE 
"${PROJECT_SOURCE_DIR}/Include"
"${PROJECT_SOURCE_DIR}/Render"
"${PROJECT_SOURCE_DIR}/imgui"
)

target_sources(GltfExplorerDx12
PRIVATE
"${PROJECT_SOURCE_DIR}/imgui/imconfig.h"
"${PROJECT_SOURCE_DIR}/imgui/imgui.cpp"
"${PROJECT_SOURCE_DIR}/imgui/imgui.h"
"${PROJECT_SOURCE_DIR}/imgui/imgui_demo.cpp"
"${PROJECT_SOURCE_DIR}/imgui/imgui_draw.cpp"
"${PROJECT_SOURCE_DIR}/imgui/imgui_internal.h"
"${PROJECT_SOURCE_DIR}/imgui/imgui_tables.cpp"
"${PROJECT_SOURCE_DIR}/imgui/imgui_widgets.cpp"
"${PROJECT_SOURCE_DIR}/imgui/backends/imgui_impl_win32.cpp"
"${PROJECT_SOURCE_DIR}/imgui/backends/imgui_impl_win32.h"
)

