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

#include "sdl_app.h"
#include <SDL.h>

sdl_app::sdl_app() : m_bInitialized(false)
{
}

bool sdl_app::init()
{
	bool result = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) == 0;
	if(result)
	{
		m_bInitialized = true;
	}
	return result;
}

sdl_app::~sdl_app()
{
	if(m_bInitialized)
	{
		SDL_Quit();
		m_bInitialized = false;
	}
}