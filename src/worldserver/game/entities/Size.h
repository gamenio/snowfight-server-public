#ifndef __SIZE_H__
#define __SIZE_H__

#include "Common.h"

class Size
{
public:
	Size():
		width(0),
		height(0)
	{

	}

	Size(float _width, float _height) :
		width(_width),
		height(_height)
	{

	}

	void setSize(float _width, float _height)
	{
		width = _width;
		height = _height;
	}

	float width;
	float height;

	static const Size ZERO;
};

#endif //__SIZE_H__