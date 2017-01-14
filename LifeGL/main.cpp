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

#include "sim_platform.h" // for main definition
#include "sim.h" // for simulation
#include "sim_common.h" // for some common functions
#include "sdl_app.h" // shell for SDL initialization
#include "sdl_form.h" // shell for SDL window

#include <sstream> // for update title
#include <memory> // for unique_ptr
#include <string> // for strings
#include <cmath> // for math
#include <vector> // for vector's
#include <fstream> // for read/write parameter file
#include <thread> // for separate update thread
#include <chrono> // for time
#include <regex> // for help loading data from params.txt

const bool bDefVSync{ false };
const std::string strMainTitle{ "LifeGL v1.7" };
const std::string strParamFileName{ "params.txt" };
const double dMinDelay{ 0.0 };
const double dMaxDelay{ 0.1 };
const double dDefDelay{ 0.03f };

bool bVSync{ bDefVSync }; // Vertical synchronization
double dFps{ 0.0 };
std::ostringstream ssSpeed;
std::ostringstream ssStatus;
std::ostringstream ssFPS;
bool bMouseLB{ false };
bool bMouseRB{ false };
double dDelay{ dDefDelay };
bool bShowInfo{ true };
bool bShutdown{ false };
bool bRequestThreadStop{ false };
bool bThreadStopped{ true };
std::unique_ptr<sim> pSim { nullptr };
bool bStart{ false };

// METHOD DECLARATIONS

auto UpdateDelaySS() -> void;
auto UpdateStatusSS() -> void;
auto UpdateFPSSS() -> void;
auto BeginFPS()->void;
auto EndFPS() -> void;
auto Exit() -> void;
auto Render() -> void;
auto ProcessEvent(SDL_Window* window, const SDL_Event& e) -> void;
auto LoadSettings() -> void;
auto SaveSettings() -> void;
auto ExtractDec(const std::string& str) -> int;
auto ExtractDouble(const std::string& str) -> double;

std::unique_ptr<sdl_app> pApp;
std::unique_ptr<sdl_form> pForm;

MAIN
{
	using namespace std;

	pApp = make_unique<sdl_app>();

	if (!pApp->init())
	{
		return -1;
	}

	pForm = make_unique<sdl_form>();

	LoadSettings(); // load settings

	pSim = make_unique<sim>("params.txt"); // create and initialize simulation field from params.txt

	auto resX = pSim->getWidth()*pSim->getTileSize();
	auto resY = pSim->getHeight()*pSim->getTileSize();

	// Create a windowed mode window and its OpenGL context.

	if(!pForm->init(resX, resY, strMainTitle.c_str(), pSim.get()))
	{
		return -1;
	}
 
	

	if (!bVSync)
	{
		if (SDL_GL_SetSwapInterval(0) != 0)
		{
			return -1;
		}
		ssFPS.precision(4);
	}
	else
	{
		if (SDL_GL_SetSwapInterval(1) != 0)
		{
			return -1;
		}
		ssFPS.precision(2);
	}

	ssSpeed.precision(4);
	ssStatus.precision(2);

	std::chrono::time_point<std::chrono::system_clock> start, end;
	std::chrono::duration<double> interval(dDelay);
	std::chrono::duration<double> total(interval);

	// Setup game thread.
	thread tickthread([]
	{
		std::chrono::time_point<std::chrono::system_clock> t_start, t_end;
		std::chrono::duration<double> t_interval(dDelay);
		std::chrono::duration<double> t_total(t_interval);
		do
		{
			if (t_total >= t_interval)
			{
				if (bStart)
				{
					pSim->tick();
				}
							
				t_start = std::chrono::system_clock::now();
				t_interval = std::chrono::duration<double>(dDelay);

				if (bRequestThreadStop)
				{
					bRequestThreadStop = false;
					break;
				}
			}

			t_end = std::chrono::system_clock::now();
			t_total = (t_end - t_start);
		} while (true);

		bThreadStopped = true;
	});

	tickthread.detach();

	UpdateDelaySS();
	UpdateStatusSS();

	SDL_Event e;

	// Loop until the user closes the window.
	while (!bShutdown)
	{
		BeginFPS();

		// Poll for and process events.

		while(SDL_PollEvent(&e))
		{
			ProcessEvent(pForm->getWindowPtr(), e);
		}

		Render();
		
		if (bShowInfo)
		{
			// Fill title with useful information.
			std::string title;
			title.append(strMainTitle.c_str());
			title.append(ssFPS.str());
			title.append(ssStatus.str());
			title.append(ssSpeed.str());
			SDL_SetWindowTitle(pForm->getWindowPtr(), title.data());
		}

		// Swap front and back buffers.
		SDL_GL_SwapWindow(pForm->getWindowPtr());
		
		EndFPS();
	}

	bRequestThreadStop = true;

	// wait 'til separate thread is stopped

	while (!bThreadStopped) {}

	// save simulation parameters

	pSim->saveParams("params.txt");

	SaveSettings();

	return 0;
}

double beginTime{ 0.0 };
double deltaTime{ 0.0 };
int frameCount{ 0 };
double fps{ 0.0 };

auto GetTime() -> double
{
	return SDL_GetTicks() / 1000;
}

auto BeginFPS() -> void
{
	beginTime = GetTime();
}

auto EndFPS() -> void
{
	double currentTime = GetTime() - beginTime;

	if (deltaTime > 1.0)
	{
		fps = (double)frameCount / deltaTime;
		dFps = fps;
		UpdateFPSSS();

		deltaTime = 0.0;

		frameCount = 0;
	}
	else
	{
		frameCount++;
		deltaTime += currentTime;
	}
}

void Exit()
{
	bRequestThreadStop = true;
	bShutdown = true;
}

auto ProcessEvent(SDL_Window* window,const SDL_Event& e) -> void
{
	static bool lshift_pressed = false;
	static bool reset_pressed = false;

	switch (e.type)
	{
	case SDL_QUIT:
		Exit();
		break;
	case SDL_KEYDOWN:
		switch (e.key.keysym.sym)
		{
		case SDLK_ESCAPE:
			Exit();
			break;
		case SDLK_g: // TOGGLE GRID
			pSim->toggleGrid();
			break;
		case SDLK_d:
			bShowInfo = !bShowInfo;
			if (!bShowInfo)
			{
				SDL_SetWindowTitle(window, strMainTitle.c_str());
			}
			break;
		case SDLK_w: // TOGGLE WRAP
			pSim->toggleWrap();
			UpdateStatusSS();
			break;
		case SDLK_s: // SET SPEED TO DEFAULT VALUE
			dDelay = dDefDelay;
			UpdateDelaySS();
			break;
		case SDLK_x:
		case SDLK_EQUALS: // DECREASE SIMULATOR SPEED		
			dDelay = clamp(dDelay - 1.0 / 500.0,0.0,0.1);
			UpdateDelaySS();
			break;
		case SDLK_z:
		case SDLK_MINUS: // INCREASE SIMULATOR SPEED	
			dDelay = clamp(dDelay + 1.0 / 500.0,0.0,0.1);
			UpdateDelaySS();
			break;
		case SDLK_r: // RESET & PAUSE SIMULATION

			if(!reset_pressed)
			{
				bStart = false;
				pSim->reset();
				reset_pressed = true;
			}

			break;
		case SDLK_LSHIFT:
			if (!bStart)
			{
				if (!lshift_pressed) 
				{
					pSim->tick();
					lshift_pressed = true;
				}
			}
			break;
		case SDLK_SPACE:
			bStart = !bStart;
			UpdateStatusSS();
			break;
		}
		break;
	case SDL_MOUSEMOTION:

		if (bMouseLB)
		{
			pSim->updateInput(e.motion.x, e.motion.y, true);
		}
		else if (bMouseRB)
		{
			pSim->updateInput(e.motion.x, e.motion.y, false);
		}

		break; 

	case SDL_KEYUP:
		switch (e.key.keysym.sym)
		{
		case SDLK_r:
			reset_pressed = false;
			break;
		case SDLK_LSHIFT:
			lshift_pressed = false;
			break;
		}
		break;

	case SDL_MOUSEBUTTONDOWN:

		switch (e.button.button)
		{
		case SDL_BUTTON_LEFT:
			bMouseLB = true; pSim->updateInput(e.button.x, e.button.y, true);
			break;
		case SDL_BUTTON_RIGHT:
			bMouseRB = true; pSim->updateInput(e.button.x, e.button.y, false);
			break;
		}

		break;
	case SDL_MOUSEBUTTONUP:

		switch (e.button.button)
		{
		case SDL_BUTTON_LEFT:
			bMouseLB = false;
			break;
		case SDL_BUTTON_RIGHT:
			bMouseRB = false;
			break;
		}
		break;
	
	case SDL_MOUSEWHEEL:
		
		int delta = e.wheel.y;

		float fDeltaY{ 0.0f };
		bool wheelChanged = false;

		if (delta < 0)
		{
			fDeltaY = -delta / 500.0f;
			wheelChanged = true;
		}
		else if (delta > 0)
		{
			fDeltaY = -delta / 500.0f;
			wheelChanged = true;
		}

		if (wheelChanged)
		{
			dDelay += fDeltaY;
			dDelay = clamp(dDelay, dMinDelay, dMaxDelay);
			UpdateDelaySS();
		}

		break;
	}
}

auto UpdateFPSSS() -> void
{
	ssFPS.str("");
	ssFPS << " FPS:" << dFps;
}

auto UpdateStatusSS() -> void
{ 
	ssStatus.str("");
	ssStatus << std::boolalpha << " Run:" << bStart << ",Wrap:" << pSim->isWrapEnabled();
}

auto UpdateDelaySS() -> void
{
	ssSpeed.str("");
	ssSpeed << ",Delay:" << dDelay;
}

auto Render() -> void
{
	pForm->clear();
	pSim->render();
}

auto Reset() -> void
{
	tryagain:
	if (bThreadStopped)
	{
		pSim->reset();
		bThreadStopped = false;
	}
	else
	{
		bRequestThreadStop = true;
		goto tryagain;
	}
}

auto SaveSettings() -> void
{
	const char* fileName = "settings.txt";

	std::ofstream file{ fileName };

	file << "VerticalSync:" << bVSync;

	file << "\nDelay:" << dDelay;

	file.close();
}

auto LoadSettings() -> void
{
	const char* fileName = "settings.txt";

	if (isFileExist(fileName))
	{
		std::ifstream file{ fileName };

		std::string vsync, delay;

		file >> vsync >> delay;

		bVSync = extractDec(vsync) == 0 ? false : true;

		dDelay = clamp(extractDouble(delay), dMinDelay, dMaxDelay);

		file.close();
	}
	else
	{
		bVSync = false;
		
		dDelay = dDefDelay;
	}
}