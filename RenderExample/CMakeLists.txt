add_executable(RenderExampleDx11 ${PROJECT_SOURCE_DIR}/RenderExample/RenderExample.cpp)
add_executable(RenderExampleDx12 ${PROJECT_SOURCE_DIR}/RenderExample/RenderExample.cpp)

target_link_libraries(RenderExampleDx11 RenderDx11)
target_link_libraries(RenderExampleDx12 RenderDx12)

set_target_properties(RenderExampleDx11
						PROPERTIES
						RUNTIME_OUTPUT_NAME_DEBUG "RenderExampleDx11_Debug"
						VS_DEBUGGER_WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

set_target_properties(RenderExampleDx12
						PROPERTIES
						RUNTIME_OUTPUT_NAME_DEBUG "RenderExampleDx12_Debug"
						VS_DEBUGGER_WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

target_include_directories(RenderExampleDx11 PRIVATE "${PROJECT_SOURCE_DIR}/Include")
target_include_directories(RenderExampleDx12 PRIVATE "${PROJECT_SOURCE_DIR}/Include")

target_include_directories(RenderExampleDx11 PRIVATE "${PROJECT_SOURCE_DIR}/Render")
target_include_directories(RenderExampleDx12 PRIVATE "${PROJECT_SOURCE_DIR}/Render")




