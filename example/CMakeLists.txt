cmake_minimum_required(VERSION 2.8)
project(example)
enable_testing()

find_package(PDFium REQUIRED)

add_executable(example example.c)
target_link_libraries(example pdfium)

# Optional: copy library alongside with the excecutable
add_custom_command(
	TARGET example POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:pdfium> $<TARGET_FILE_DIR:example>)

add_test(example example)
