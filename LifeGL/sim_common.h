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

template<class T> inline auto clamp(T value, T min, T max) -> T 
{ 
	return value < min ? min : value > max ? max : value;
}

template<class T> inline auto wrap(T kX, T const kLowerBound, T const kUpperBound) -> T
{
	T range_size = kUpperBound - kLowerBound + 1;

	if (kX < kLowerBound)
		kX += range_size * ((kLowerBound - kX) / range_size + 1);

	return kLowerBound + (kX - kLowerBound) % range_size;
}

#include <fstream>

inline auto isFileExist(const std::string& name) -> bool
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

#include <regex>

inline auto extractDec(const std::string& str) -> int
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

inline auto extractDouble(const std::string& str) -> double
{
	using namespace std;

	regex filter{ R"((\d.\d+$)|(\d+$))" };
	smatch match;
	if (regex_search(str, match, filter))
	{
		auto v = match.str();
		return strtod(v.c_str(), nullptr);
	}

	return 0;
}


inline auto extractFloat(const std::string& str) -> float
{
	using namespace std;

	regex filter{ R"((\d.\d+$)|(\d+$))" };
	smatch match;
	if (regex_search(str, match, filter))
	{
		auto v = match.str();
		return strtof(v.c_str(), nullptr);
	}

	return 0;
}