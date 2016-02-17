//The MIT License(MIT)
//
//Copyright (c) 2016 Yuri Roubinsky
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

#define WINDOWS 0
#define LINUX 1

#ifdef _WIN32
#define PLATFORM WINDOWS
#else
#define PLATFORM LINUX
#endif

#if PLATFORM == WINDOWS
#define MAIN auto CALLBACK WinMain(HINSTANCE hInst,HINSTANCE hPInst,LPSTR lpcmdl,int nshowcmd) -> int
#else
#define MAIN auto main() -> int
#endif

#if PLATFORM == WINDOWS
#pragma comment(lib,"SDL2.lib")
#pragma comment(lib,"opengl32.lib") // for all graphics stuff
#pragma comment(lib,"glu32.lib") // for gluOrtho2D
#endif

#if PLATFORM == WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h> // for window creation
#endif

#include <sstream> // for update title
#include <memory> // for unique_ptr
#include <string> // for strings
#include <cmath> // for math
#include <vector> // for vector's
#include <fstream> // for read/write parameter file
#include <thread> // for separate update thread
#include <chrono> // for time
#include <regex> // for help loading data from params.txt
#include <gl/GL.h> // for all graphics stuff
#include <gl/GLU.h> // for helper opengl functions
#include <SDL.h> // for window creation

struct Rect
{
	int left;
	int right;
	int top;
	int bottom;
};

struct Tile
{
	Rect rect;
	int state;
	int lt;
	double fade;
};

const bool bDefVSync{ false };
const int iDefSizeX{ 60 };
const int iDefSizeY{ 60 };
const int iDefTileSize{ 10 };
const float iDefSpeed{ 0.03f };
const double dDefFadeForce{ 0.1 };
const std::string strMainTitle{ "LifeGL v1.05" };
const std::string strParamFileName{ "params.txt" };
const bool bUseSeparateThread{ true }; // If disabled the speed of simulation will be limited to vsync and processing speed of main loop

bool bVSync = bDefVSync; // Vertical synchronization
bool bFadeEffect{ true };
bool bDrawGrid{ true };
double dFps;
std::ostringstream ssSpeed;
std::ostringstream ssStatus;
std::ostringstream ssFPS;
bool bMouseLB;
bool bMouseRB;
int iSizeX = iDefSizeX;
int iSizeY = iDefSizeY;
int iTileSize = iDefTileSize;
double dFadeForce = dDefFadeForce;
bool bWrap{ true };
double dSpeed = iDefSpeed;
bool bShowInfo{ true };
int iSavedSizeX;
int iSavedSizeY;
int iSavedTileSize;
bool bSavedWrap;
double dSavedSpeed;
int iResX;
int iResY;
bool bStart;
bool bShutdown{ false };
bool bRequestThreadStop;
bool bThreadStopped{ true };
std::unique_ptr<Tile[]> pField{ nullptr };

inline auto GetOffset(int x, int y) -> int
{
	return y + (x * iSizeY);
}

inline auto IsParamsChanged() -> bool
{
	return !(iSizeX == iSavedSizeX && iSizeY == iSavedSizeY && iTileSize == iSavedTileSize && bWrap == bSavedWrap && dSpeed == dSavedSpeed);
}

inline auto FileExist(const std::string& name) -> bool
{
	using namespace std;
	ifstream f(name.c_str());
	if (f.good()) {
		f.close();
		return true;
	}
	else {
		f.close();
		return false;
	}
}

// METHOD DECLARATIONS

auto Reset() -> void;
auto UpdateDelaySS() -> void;
auto UpdateStatusSS() -> void;
auto UpdateFPSSS() -> void;
auto BeginFPS()->void;
auto EndFPS() -> void;
auto ClampD(double, double, double) -> double;
auto CheckAllTile(int,int,bool) -> void;
auto CheckAllTileC(int,int) -> void;
auto DrawField() -> void;
auto DrawTile(int,int) -> void;
auto Exit() -> void;
auto Tick() -> void;
auto GetTileState(int, int) -> int;
auto Render() -> void;
auto ProcessEvent(SDL_Window* window, const SDL_Event& e) -> void;
auto IsParamsChanged() -> bool;
auto SaveParams() -> void;
auto LoadParams() -> void;
auto ExtractDec(const std::string& str) -> int;
auto ExtractDouble(const std::string& str) -> double;

MAIN
{
	using namespace std;

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0)
	{
		return -1;
	}

	atexit(SDL_Quit);

	SDL_Window* window;

	LoadParams();

	iResX = iSizeX*iTileSize;
	iResY = iSizeY*iTileSize;

	// Create a windowed mode window and its OpenGL context.

	window = SDL_CreateWindow(strMainTitle.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, iResX, iResY, SDL_WINDOW_OPENGL);

	if (window == nullptr)
	{
		return -1;
	}

	// Start simulation.
	Reset();
	
	// Setup OpenGL.
	glClearColor(0, 0, 0, 0);

	int wndszX, wndszY;
	SDL_GetWindowSize(window, &wndszX, &wndszY);
	 
	SDL_GLContext context = SDL_GL_CreateContext(window);

    gluOrtho2D(0, wndszX, wndszY, 0);

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
	std::chrono::duration<double> interval(dSpeed);
	std::chrono::duration<double> total(interval);

	if (bUseSeparateThread)
	{
		// Setup game thread.
		thread tickthread([]
		{
			std::chrono::time_point<std::chrono::system_clock> t_start, t_end;
			std::chrono::duration<double> t_interval(dSpeed);
			std::chrono::duration<double> t_total(t_interval);
			do
			{
				if (!bThreadStopped)
				{
					if (t_total >= t_interval)
					{
						if (bStart)
						{
							Tick();
						}
							
						t_start = std::chrono::system_clock::now();
						t_interval = std::chrono::duration<double>(dSpeed);

						if (bRequestThreadStop)
						{
							bThreadStopped = true;
							bRequestThreadStop = false;
							break;
						}
					}

					t_end = std::chrono::system_clock::now();
					t_total = (t_end - t_start);
				}
			} while (true);
		});

		tickthread.detach();
	}

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
			ProcessEvent(window, e);
		}

		if (!bUseSeparateThread)
		{
			if (!bThreadStopped)
			{
				if (total >= interval)
				{
					if (bStart)
					{
						Tick();
					}

					start = std::chrono::system_clock::now();
					interval = std::chrono::duration<double>(dSpeed);

					if (bRequestThreadStop)
					{
						bThreadStopped = true;
						bRequestThreadStop = false;
					}
				}
				end = std::chrono::system_clock::now();
				total = (end - start);
			}
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
			SDL_SetWindowTitle(window, title.data());
		}

		// Swap front and back buffers.
		SDL_GL_SwapWindow(window); 	
		
		EndFPS();
	}
	
	// wait until separate thread is stopped

	if (bUseSeparateThread)
	{
		while (!bThreadStopped) {}
	}

	SaveParams();
	pField.release();
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
		case SDLK_g:
			
			// GRID DRAWING ENABLING
			
			bDrawGrid = !bDrawGrid;
			break;
		case SDLK_d:
			bShowInfo = !bShowInfo;
			if (!bShowInfo)
			{
				SDL_SetWindowTitle(window, strMainTitle.c_str());
			}
			break;
		case SDLK_w:

			// WRAPPING ENABLING
			
			bWrap = !bWrap;
			UpdateStatusSS();
			break;
		case SDLK_s:

			// SET SPEED TO DEFAULT VALUE
			
			dSpeed = iDefSpeed;
			UpdateDelaySS();
			break;
		case SDLK_x:
		case SDLK_EQUALS:
			// DECREASE SIMULATOR SPEED
			
			dSpeed = ClampD(dSpeed - 1.0 / 500.0,0,0.1f);
			UpdateDelaySS();
			break;
		case SDLK_z:
		case SDLK_MINUS:
			// INCREASE SIMULATOR SPEED
			
			dSpeed = ClampD(dSpeed + 1.0 / 500.0,0,0.1f);
			UpdateDelaySS();
			break;
		case SDLK_r:
			bThreadStopped = true;
			Reset();
			break;
		case SDLK_LSHIFT:
		case SDLK_SPACE:
			bStart = !bStart;
			break;
		}
		break;
	case SDL_MOUSEMOTION:

		if (bMouseLB)
		{
			CheckAllTile(e.motion.x, e.motion.y, true);
		}
		else if (bMouseRB)
		{
			CheckAllTile(e.motion.x, e.motion.y, false);
		}

		break; 

	case SDL_MOUSEBUTTONDOWN:

		switch (e.button.button)
		{
		case SDL_BUTTON_LEFT:
			bMouseLB = true; CheckAllTile(e.button.x, e.button.y, true);
			break;
		case SDL_BUTTON_RIGHT:
			bMouseRB = true; CheckAllTile(e.button.x, e.button.y, false);
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
			dSpeed += fDeltaY;
			dSpeed = ClampD(dSpeed, 0, 0.1);
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
	ssStatus << std::boolalpha << " Run:" << bStart << ",Wrap:" << bWrap;
}

auto UpdateDelaySS() -> void
{
	ssSpeed.str("");
	ssSpeed << ",Delay:" << dSpeed;
}

auto Render() -> void
{
	glClear(GL_COLOR_BUFFER_BIT); 

	DrawField();
}

auto Reset() -> void
{
	tryagain:
	if (bThreadStopped)
	{
		pField = std::make_unique<Tile[]>(iResX*iResY);

		for (int x = 0; x < iResX; x++)
		{
			for (int y = 0; y < iResY; y++)
			{
				auto offset = GetOffset(x, y);

				pField[offset].state = 0;
				pField[offset].lt = 0;
				pField[offset].fade = 0;

				int _x = x*(iTileSize);
				int _y = y*(iTileSize);
				int _rx = _x + iTileSize;
				int _ry = _y + iTileSize;

				pField[offset].rect.left = _x;
				pField[offset].rect.right = _rx;
				pField[offset].rect.top = _y;
				pField[offset].rect.bottom = _ry;
			}
		}

		bThreadStopped = false;
	}
	else
	{
		bRequestThreadStop = true;
		goto tryagain;
	}
}

auto DrawField() -> void
{
	for (int x = 0; x < iSizeX; x++)
	{
		for (int y = 0; y < iSizeY; y++)
		{ 
			DrawTile(x, y);
		}
	}

	if (bDrawGrid)
	{
		glLineWidth(1.0);
		glColor3f(1.0, 1.0, 1.0);
		glBegin(GL_LINES);

		for (int x = 0; x < iSizeX; x++)
		{
			for (int y = 0; y < iSizeY; y++)
			{
				auto offset = GetOffset(x, y);

				Rect r = pField[offset].rect;
				glVertex2i(r.left, r.top);
				glVertex2i(r.left, r.bottom);

				glVertex2i(r.left, r.top);
				glVertex2i(r.right, r.top);
			}
		}

		/* Draw end lines */
		int _x = iResX;
		int _y = iResY;
		glVertex2i(_x, 0);
		glVertex2i(_x, _y);
		glVertex2i(0, _y);
		glVertex2i(_x, _y);

		glEnd();
	}
}

auto DrawTile(int x, int y) -> void
{
	auto offset = GetOffset(x, y);


	if (bFadeEffect)
	{		 
		glColor3f(pField[offset].fade, 0, 0); 
	}
	else
	{
		if (pField[offset].state > 0)
			glColor3f(1, 0, 0);
		else
			return;
	} 

	Rect rect = pField[offset].rect;
	glRecti(rect.left, rect.top, rect.right, rect.bottom);
}

auto CheckAllTile(int mx, int my, bool enable) -> void
{
	for (int x = 0; x < iSizeX; x++)
	{
		for (int y = 0; y < iSizeY; y++)
		{
			auto offset = GetOffset(x, y);
			Rect rect = pField[offset].rect;

			if (mx>rect.left && mx < rect.right && my > rect.top && my < rect.bottom)
			{
				if (enable)
					pField[offset].fade = 1;
				else
					pField[offset].fade = 0;

				if (pField[offset].state != enable)
				{
					pField[offset].state = enable;
				}
			}
		}
	}
}

auto CheckAllTileC(int mx, int my) -> void
{
	for (int x = 0; x < iSizeX; x++)
	{
		for (int y = 0; y < iSizeY; y++)
		{
			auto offset = GetOffset(x, y); 
			Rect rect = pField[offset].rect;

			if (mx>rect.left && mx <rect.right && my > rect.top && my < rect.bottom)
			{
				if (pField[offset].state != 1)
				{
					pField[offset].state = 1;
					pField[offset].fade = 1;
				}
				else
				{
					pField[offset].state = 0;
					pField[offset].fade = 0;
				}
			}
		}
	}
}

auto ClampD(double n, double min, double max) -> double
{
	return n < min ? min : n > max ? max : n;
}

auto Wrap(int kX, int const kLowerBound, int const kUpperBound) -> int
{
	int range_size = kUpperBound - kLowerBound + 1;

	if (kX < kLowerBound)
		kX += range_size * ((kLowerBound - kX) / range_size + 1);

	return kLowerBound + (kX - kLowerBound) % range_size;
}

auto GetTileState(int x, int y) -> int
{
	int _x;
	int _y;

	if (bWrap)
	{
		_x = Wrap(x, 0, iSizeX - 1);
		_y = Wrap(y, 0, iSizeY - 1);
	}
	else
	{
		if (x < 0 || x >= iSizeX || y < 0 || y >= iSizeY)return -1;
		else
		{
			_x = x;
			_y = y;
		}
	}

	auto offset = GetOffset(_x, _y);
	return pField[offset].state;
}

auto Tick() -> void
{
	for (int x = 0; x < iSizeX; x++)
	{
		for (int y = 0; y < iSizeY; y++)
		{
			int n[8] = { GetTileState(x - 1, y - 1), GetTileState(x, y - 1), GetTileState(x + 1, y - 1),
				GetTileState(x - 1, y), GetTileState(x + 1, y),
				GetTileState(x - 1, y + 1), GetTileState(x, y + 1), GetTileState(x + 1, y + 1) };

			auto offset = GetOffset(x, y);

			pField[offset].lt = 0;

			for (int i = 0; i < 8; i++)
			{
				if (n[i] == 1)pField[offset].lt++;
			}
		}
	}

	for (int x = 0; x < iSizeX; x++)
	{
		for (int y = 0; y < iSizeY; y++)
		{
			auto offset = GetOffset(x, y);

			switch (pField[offset].lt)
			{
			case 2:

				break;
			case 3:
				pField[offset].state = 1;
				if (bFadeEffect)
				 pField[offset].fade = 1;
				break;
			default:
				pField[offset].state = 0;
				if (bFadeEffect)
					pField[offset].fade = ClampD(pField[offset].fade - dFadeForce,0,1.0);
				break;
			}
		}
	}
}

auto SaveParams() -> void
{
	using namespace std;
	if (IsParamsChanged())
	{
		fstream file;
		file.open("params.txt", ios_base::out | ios_base::trunc);

		file << "VSync" << bVSync << "Width:" << iSizeX << "\nHeight:" << iSizeY << "\nTSize:" << iTileSize << "\nWrap:" << bWrap << "\nSpeed:" << dSpeed << "\nFForce:" << dFadeForce << "\n";

		file.close();
	}
}

auto ExtractDec(const std::string& str) -> int
{
	using namespace std;

	regex filter{ R"(\d+$)" };
	smatch match;
	if (regex_search(str, match, filter))
	{ 
		auto v = match.str();
		return atoi(v.c_str());
	}

	return 0;
}

auto ExtractDouble(const std::string& str) -> double
{
	using namespace std;

	regex filter{ R"((\d.\d+$)|(\d+$))" };
	smatch match;
	if (regex_search(str, match, filter))
	{
		auto v = match.str();
		return strtod(v.c_str(),nullptr);
	}

	return 0;
}

auto LoadParams() -> void
{
	using namespace std;
	if (FileExist(strParamFileName))
	{
		ifstream file{ strParamFileName };

		string vsync,width, height, tsize, wrap, speed, fforce;
		file >> vsync >> width >> height >> tsize >> wrap >> speed >> fforce;

		bVSync = ExtractDec(vsync);
		iSavedSizeX = iSizeX = ExtractDec(width);
		iSavedSizeY = iSizeY = ExtractDec(height);
		iSavedTileSize = iTileSize = ExtractDec(tsize);
		bSavedWrap = bWrap = ExtractDec(wrap)==0?false:true;
		dSavedSpeed = dSpeed = ExtractDouble(speed);
		dFadeForce = ExtractDouble(fforce);

		file.close();
	}
}