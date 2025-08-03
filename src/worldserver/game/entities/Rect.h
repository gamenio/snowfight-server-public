#ifndef __RECT_H__
#define __RECT_H__

#include "Point.h"
#include "Size.h"

class Rect
{
public:
	Rect() :
		origin(Point::ZERO),
		size(Size::ZERO)
	{

	}

	Rect(Size const& _size, Point const& center):
		origin(center.x - _size.width / 2.0f, center.y - _size.height / 2.0f),
		size(_size)
	{

	}

	Rect(Point const& _origin, Size const& _size) :
		origin(_origin),
		size(_size)
	{

	}


	Rect(float _x, float _y, float _width, float _height):
		origin(_x, _y),
		size(_width, _height)
	{

	}

	float getMinX() const { return origin.x; }
	float getMinY() const { return origin.y; }

	float getMidX() const { return origin.x + size.width / 2.0f; }
	float getMidY() const { return origin.y + size.height / 2.0f; }

	float getMaxX() const { return origin.x + size.width; }
	float getMaxY() const { return origin.y + size.height; }

	void setRect(float x, float y, float width, float height)
	{
		origin.x = x;
		origin.y = y;

		size.width = width;
		size.height = height;
	}

	bool containsPoint(Point point) const
	{
		bool bRet = false;

		if (point.x >= getMinX() && point.x <= getMaxX()
			&& point.y >= getMinY() && point.y <= getMaxY())
		{
			bRet = true;
		}

		return bRet;
	}

	bool intersectsRect(Rect const& rect) const
	{
		return !(getMaxX() < rect.getMinX() ||
			rect.getMaxX() < getMinX() ||
			getMaxY() < rect.getMinY() ||
			rect.getMaxY() < getMinY());
	}

	bool intersectsLine(Point const& p1, Point const& p2);

	void merge(Rect const& rect)
	{
		float minX = std::min(getMinX(), rect.getMinX());
		float minY = std::min(getMinY(), rect.getMinY());
		float maxX = std::max(getMaxX(), rect.getMaxX());
		float maxY = std::max(getMaxY(), rect.getMaxY());
		setRect(minX, minY, maxX - minX, maxY - minY);
	}


	Point origin;
	Size size;

	static const Rect ZERO;
};

#endif //__RECT_H__
