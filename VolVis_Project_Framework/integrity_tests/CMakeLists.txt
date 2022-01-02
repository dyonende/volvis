add_executable(IntegrityTests
	"src/main.cpp"
	"src/tests.cpp")
target_link_libraries(IntegrityTests PRIVATE VolVis Catch2::Catch2)
target_compile_features(IntegrityTests PRIVATE cxx_std_17)
set_project_warnings(IntegrityTests)
