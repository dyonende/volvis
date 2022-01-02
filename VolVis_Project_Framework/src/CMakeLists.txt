target_sources(VolVis
	PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/ui/full_screen_texture_gl.cpp"
		"${CMAKE_CURRENT_LIST_DIR}/ui/gl_error.cpp"
		"${CMAKE_CURRENT_LIST_DIR}/ui/menu.cpp" 
		"${CMAKE_CURRENT_LIST_DIR}/ui/opengl.cpp"
		"${CMAKE_CURRENT_LIST_DIR}/ui/trackball.cpp"
		"${CMAKE_CURRENT_LIST_DIR}/ui/transfer_func.cpp"
		"${CMAKE_CURRENT_LIST_DIR}/ui/transfer_func_2d.cpp"
		"${CMAKE_CURRENT_LIST_DIR}/ui/window.cpp"
		"${CMAKE_CURRENT_LIST_DIR}/ui/surface_cube.cpp"
		"${CMAKE_CURRENT_LIST_DIR}/ui/wireframe_cube.cpp"

		#"${CMAKE_CURRENT_LIST_DIR}/imgui/imgui.cpp"
		#"${CMAKE_CURRENT_LIST_DIR}/imgui/imgui_draw.cpp"
		#"${CMAKE_CURRENT_LIST_DIR}/imgui/imgui_widgets.cpp"
		#"${CMAKE_CURRENT_LIST_DIR}/imgui/imgui_impl_glfw.cpp"
		#"${CMAKE_CURRENT_LIST_DIR}/imgui/imgui_impl_opengl3.cpp"

		"${CMAKE_CURRENT_LIST_DIR}/render/renderer.cpp"

		"${CMAKE_CURRENT_LIST_DIR}/volume/volume.cpp" 
		"${CMAKE_CURRENT_LIST_DIR}/volume/gradient_volume.cpp")

# Wrap in separate library so that the compiler warnings that we set for our own code doens't affect this third-party code.
add_library(ImGuiWrapper
	"${CMAKE_CURRENT_LIST_DIR}/imgui/imgui_impl_glfw.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/imgui/imgui_impl_opengl3.cpp")
target_link_libraries(ImGuiWrapper PUBLIC imgui::imgui)
target_link_libraries(VolVis PRIVATE ImGuiWrapper)
