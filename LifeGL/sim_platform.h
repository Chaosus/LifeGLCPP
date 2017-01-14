#pragma once

//The MIT License(MIT)
//
//Copyright (c) 2016 Yuri Rubinskiy
//
//Permission is hereby granted, free of charge, to any person obtaining a copy
//of this software and associated documentation files(the "Software"), to deal
//in the Software without restriction, including without limitation the rights
//to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
//copies of the Software, and to permit persons to whom the Software is
//furnished to do so, subject to the following conditions :
//
//The above copyright notice and this permission notice shall be included in
//all copies or substantial portions of the Software.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
//AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//THE SOFTWARE.

#ifndef WINDOWS
#define WINDOWS 0
#endif

#ifndef LINUX
#define LINUX 1
#endif

#ifndef PLATFORM
#ifdef _WIN32
#define PLATFORM WINDOWS
#else
#define PLATFORM LINUX
#endif
#endif

#if PLATFORM == WINDOWS

#pragma comment(lib,"SDL2.lib")
#pragma comment(lib,"opengl32.lib") // for all graphics stuff
#pragma comment(lib,"glu32.lib") // for gluOrtho2D

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h> // for window creation
#endif

#ifndef MAIN
#if PLATFORM == WINDOWS
#define MAIN auto CALLBACK WinMain(HINSTANCE hInst,HINSTANCE hPInst,LPSTR lpcmdl,int nshowcmd) -> int
#else
#define MAIN auto main() -> int
#endif
#endif