cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Work around Xtensa GCC 14 ICEs seen in ESP-IDF v5.4.4 builds
# (e.g. esp_lcd_panel_rgb.c in IRA pass).
idf_build_set_property(C_COMPILE_OPTIONS "-O0" APPEND)
idf_build_set_property(CXX_COMPILE_OPTIONS "-O0" APPEND)

project(exercise_touchscreen)
