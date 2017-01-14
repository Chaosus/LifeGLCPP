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

#include <fstream>

struct rect
{
	int left;
	int right;
	int top;
	int bottom;
};

struct sim_tile
{
	rect rect;
	int state;
	int lt;
	float fade;
};

class sim
{
public:
	// METHODS
	auto isGridEnabled() const -> bool { return m_bDrawGrid; }
	auto isWrapEnabled() const -> bool { return m_bWrap; }
	auto isFadeEffectEnabled() const -> bool { return m_bFadeEffect; }
	auto getWidth() const -> unsigned short { return m_iWidth; }
	auto getHeight() const -> unsigned short { return m_iHeight; }
	auto getTileSize() const -> unsigned short { return m_iTileSize; }
	auto getFadeForce() const -> double { return m_fFadeForce; }
	auto enableGrid(bool enable) { m_bDrawGrid = enable; }
	auto enableWrap(bool enable) { m_bWrap = enable; }
	auto enableFadeEffect(bool enable) { m_bFadeEffect = enable; }
	auto toggleGrid() -> void { m_bDrawGrid = !m_bDrawGrid; }
	auto toggleWrap() -> void { m_bWrap = !m_bWrap; }
	auto toggleFadeEffect() -> void { m_bFadeEffect = !m_bFadeEffect; }
	auto setFadeForce(float fadeForce) -> void { m_fFadeForce = fadeForce; }
	auto tick() -> void;
	auto render() -> void;
	auto reset() -> void;
	auto updateInput(int mx, int my, int check) -> void;
	auto saveParams(const char* fileName) -> void;
	// CONSTRUCTORS
	sim(const char* paramsFileName);
	sim(unsigned short width, unsigned short height, unsigned short tileSize);
	~sim();
private:
	// FIELDS
	unsigned short m_iWidth;
	unsigned short m_iHeight;
	unsigned short m_iTileSize;
	bool m_bDrawGrid;
	bool m_bWrap;
	bool m_bFadeEffect;
	float m_fFadeForce;
	sim_tile* m_pField;
	float m_fColorR;
	float m_fColorG;
	float m_fColorB;
	// METHODS
	auto getOffset(int x, int y) -> int { return y + (x * m_iHeight); }
	auto renderTile(int x, int y) -> void;
	auto getTileState(int x, int y) -> int;
	auto create() -> void;
};