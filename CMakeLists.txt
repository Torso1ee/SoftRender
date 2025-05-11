cmake_minimum_required(VERSION 3.10.0)
project(SoftRender VERSION 0.1.0 LANGUAGES C CXX)

add_executable(SoftRender main.cpp model.cpp tgaimage.cpp SoftRenderer.cpp our_gl.cpp)

