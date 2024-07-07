add_executable(RenderExampleImguiDx11 
${PROJECT_SOURCE_DIR}/RenderExampleImGui/RenderExampleImgui.cpp
${PROJECT_SOURCE_DIR}/RenderExampleImGui/imgui_impl_render.cpp
${PROJECT_SOURCE_DIR}/RenderExampleImGui/imgui_impl_render.h
)

add_executable(RenderExampleImguiDx12 
${PROJECT_SOURCE_DIR}/RenderExampleImgui/RenderExampleImgui.cpp
${PROJECT_SOURCE_DIR}/RenderExampleImGui/imgui_impl_render.cpp
${PROJECT_SOURCE_DIR}/RenderExampleImGui/imgui_impl_render.h
)

target_link_libraries(RenderExampleImguiDx11 RenderDx11)
target_link_libraries(RenderExampleImguiDx12 RenderDx12)

set_target_properties(RenderExampleImguiDx11
PROPERTIES
RUNTIME_OUTPUT_NAME_DEBUG "RenderExampleImGuiDx11_Debug"
VS_DEBUGGER_WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

set_target_properties(RenderExampleImguiDx12
PROPERTIES
RUNTIME_OUTPUT_NAME_DEBUG "RenderExampleImGuiDx12_Debug"
VS_DEBUGGER_WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

target_include_directories(RenderExampleImguiDx11
PRIVATE 
"${PROJECT_SOURCE_DIR}/Include"
"${PROJECT_SOURCE_DIR}/Render"
"${PROJECT_SOURCE_DIR}/imgui"
)

target_include_directories(RenderExampleImguiDx12
PRIVATE 
"${PROJECT_SOURCE_DIR}/Include"
"${PROJECT_SOURCE_DIR}/Render"
"${PROJECT_SOURCE_DIR}/imgui"
)

target_sources(RenderExampleImguiDx11 
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

target_sources(RenderExampleImguiDx12 
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

