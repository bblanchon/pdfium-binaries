cmake_minimum_required(VERSION 3.5)
project(example)

find_package(PDFium REQUIRED)

add_executable(example example.c)
set_target_properties(example PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(example pdfium)
target_compile_options(example PRIVATE -pthread)
target_link_options(example PRIVATE -pthread)

# Optional: copy the library alongside the executable
add_custom_command(
	TARGET
		example
	POST_BUILD
	COMMAND
		${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:pdfium> $<TARGET_FILE_DIR:example>
)
