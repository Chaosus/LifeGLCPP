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

#include "sim.h"
#include "sim_common.h"
#include "sim_gl.h"

sim::sim(const char* fileName) :
m_bDrawGrid(true), m_bFadeEffect(true),
m_fColorR(0.0f), m_fColorG(1.0f), m_fColorB(0.0f),
m_pField(nullptr)
{
	using namespace std;

	if (isFileExist(fileName))
	{
		ifstream file{ fileName };

		string width, height, tsize, wrap, fforce;

		file >> width >> height >> tsize >> wrap >> fforce;
		 
		m_iWidth = extractDec(width);
		m_iHeight = extractDec(height);
		m_iTileSize = extractDec(tsize);
		m_bWrap = extractDec(wrap) == 0 ? false : true;
		m_fFadeForce = extractFloat(fforce);

		file.close();

		create();
	}
}

sim::sim(unsigned short width, unsigned short height, unsigned short tileSize) 
: m_iWidth(width), m_iHeight(height), m_iTileSize(tileSize), 
  m_bDrawGrid(true), m_bWrap(true), m_bFadeEffect(true), m_fFadeForce(0.1f),
  m_fColorR(0.0f), m_fColorG(1.0f), m_fColorB(0.0f),
  m_pField(nullptr)
{
	create();
}

sim::~sim()
{
	if(m_pField != nullptr){ delete[] m_pField; m_pField = nullptr; }
}

auto sim::getTileState(int x, int y) -> int
{
	int _x;
	int _y;

	if (m_bWrap)
	{
		_x = wrap(x, 0, m_iWidth - 1);
		_y = wrap(y, 0, m_iHeight - 1);
	}
	else
	{
		if (x < 0 || x >= m_iWidth || y < 0 || y >= m_iHeight) return -1;
		else
		{
			_x = x;
			_y = y;
		}
	}

	return m_pField[getOffset(_x, _y)].state;
}

void sim::tick()
{
	for (int x = 0; x < m_iWidth; x++)
	{
		for (int y = 0; y < m_iHeight; y++)
		{
			int n[8] = { getTileState(x - 1, y - 1), getTileState(x, y - 1), getTileState(x + 1, y - 1), getTileState(x - 1, y), getTileState(x + 1, y), getTileState(x - 1, y + 1), getTileState(x, y + 1), getTileState(x + 1, y + 1) };

			auto offset = getOffset(x, y);

			m_pField[offset].lt = 0;

			for (int i = 0; i < 8; i++)
			{
				if (n[i] == 1)m_pField[offset].lt++;
			}
		}
	}

	for (int x = 0; x < m_iWidth; x++)
	{
		for (int y = 0; y < m_iHeight; y++)
		{
			auto offset = getOffset(x, y);

			switch (m_pField[offset].lt)
			{
			case 2:
				break;
			case 3:
				m_pField[offset].state = 1;
				if (m_bFadeEffect)
					m_pField[offset].fade = 1;
				break;
			default:
				m_pField[offset].state = 0;
				if (m_bFadeEffect)
					m_pField[offset].fade = clamp(m_pField[offset].fade - m_fFadeForce, 0.0f, 1.0f);
				break;
			}
		}
	}
}

void sim::renderTile(int x, int y)
{
	auto offset = getOffset(x, y);

	if (m_bFadeEffect)
	{
		float f = m_pField[offset].fade;
		glColor3f(m_fColorR * f, m_fColorG * f, m_fColorB * f);
	}
	else
	{
		if (m_pField[offset].state > 0)
			glColor3f(m_fColorR, m_fColorG, m_fColorB);
		else
			return;
	}

	auto rect = m_pField[offset].rect;
	glRecti(rect.left, rect.top, rect.right, rect.bottom);
}

void sim::create()
{
	if(m_pField != nullptr) return;

	m_pField = new sim_tile[m_iWidth * m_iHeight];

	for (int x = 0; x < m_iWidth; x++)
	{
		for (int y = 0; y < m_iHeight; y++)
		{
			auto offset = getOffset(x, y);

			m_pField[offset].state = 0;
			m_pField[offset].lt = 0;
			m_pField[offset].fade = 0;

			int _x = x*(m_iTileSize);
			int _y = y*(m_iTileSize);
			int _rx = _x + m_iTileSize;
			int _ry = _y + m_iTileSize;

			m_pField[offset].rect.left = _x;
			m_pField[offset].rect.right = _rx;
			m_pField[offset].rect.top = _y;
			m_pField[offset].rect.bottom = _ry;
		}
	}
}

void sim::reset()
{
	for (int x = 0; x < m_iWidth; x++)
	{
		for (int y = 0; y < m_iHeight; y++)
		{
			auto offset = getOffset(x, y);

			m_pField[offset].state = 0;
			m_pField[offset].lt = 0;
			m_pField[offset].fade = 0;
		}
	}
}

void sim::render()
{
	for (int x = 0; x < m_iWidth; x++)
	{
		for (int y = 0; y < m_iHeight; y++)
		{
			renderTile(x, y);
		}
	}

	if (m_bDrawGrid)
	{
		glLineWidth(1.0);
		glColor3f(1.0, 1.0, 1.0);
		glBegin(GL_LINES);

		for (int x = 0; x < m_iWidth; x++)
		{
			for (int y = 0; y < m_iHeight; y++)
			{
				auto offset = getOffset(x, y);

				rect r = m_pField[offset].rect;
				glVertex2i(r.left, r.top);
				glVertex2i(r.left, r.bottom);

				glVertex2i(r.left, r.top);
				glVertex2i(r.right, r.top);
			}
		}

		/* Draw end lines */
		int _x = m_iWidth;
		int _y = m_iHeight;
		glVertex2i(_x, 0);
		glVertex2i(_x, _y);
		glVertex2i(0, _y);
		glVertex2i(_x, _y);

		glEnd();
	}
}

auto sim::updateInput(int mx, int my, int enable) -> void
{
	for (int x = 0; x < m_iWidth; x++)
	{
		for (int y = 0; y < m_iHeight; y++)
		{
			auto offset = getOffset(x, y);
			rect rect = m_pField[offset].rect;

			if (mx>rect.left && mx < rect.right && my > rect.top && my < rect.bottom)
			{
				if (enable)
					m_pField[offset].fade = 1;
				else
					m_pField[offset].fade = 0;

					m_pField[offset].state = enable;
			}
		}
	}
}

auto sim::saveParams(const char* fileName) -> void
{
	using namespace std;

	ofstream params(fileName, ios_base::out | ios_base::trunc);

	params << "Width:" << getWidth()
				 << "\nHeight:" << getHeight()
				 << "\nTileSize:" << getTileSize()
				 << "\nWrap:" << isWrapEnabled()
				 << "\nFadeForce:" << getFadeForce();

	params.close();
}