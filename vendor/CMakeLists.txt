add_subdirectory(json)
add_subdirectory(glfw)
add_subdirectory(zydis)
add_subdirectory(safetyhook)

# set(SD_FLASH_ATTN TRUE)
set(SD_FAST_SOFTMAX TRUE)
set(SD_CUBLAS TRUE)
add_subdirectory(stable-diffusion.cpp)

# ---------------------------
# imgui
# ---------------------------
file(GLOB IMGUI_SOURCES CONFIGURE_DEPENDS "imgui/*.h" "imgui/*.cpp")
add_library(imgui STATIC
    ${IMGUI_SOURCES}
    imgui/backends/imgui_impl_dx9.cpp
    imgui/backends/imgui_impl_win32.cpp
    imgui/backends/imgui_impl_win32.h)
target_link_libraries(imgui PRIVATE glfw)
target_include_directories(imgui SYSTEM PUBLIC imgui)

# ---------------------------
# imgui-filebrowser
# ---------------------------
add_library(imgui-filebrowser INTERFACE)
target_include_directories(imgui-filebrowser SYSTEM INTERFACE "imgui-filebrowser")
