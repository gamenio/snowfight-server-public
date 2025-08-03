#ifndef __POINT_H__
#define __POINT_H__

#include "Common.h"
#include "game/server/protocol/Parcel.h"

class Point
{
public:
	Point() : 
		x(0), 
		y(0)
	{

	}

	Point(float _x, float _y) : 
		x(_x), 
		y(_y)
	{

	}

	Point(Point const& right) : 
		x(0), 
		y(0)
	{
		this->copyFrom(right);
	}


	Point& operator=(Point const& right)
	{
		if (this != &right)
			this->copyFrom(right);

		return *this;
	}

	Point& operator-=(Point const& right)
	{
		this->x -= right.x;
		this->y -= right.y;
		return *this;
	}

	Point operator-(Point const& right) const 
	{
		Point ret(*this);
		ret -= right;
		return ret;
	}

	Point& operator+=(Point const& right)
	{
		this->x += right.x;
		this->y += right.y;

		return *this;
	}

	Point operator+(Point const& right) const
	{
		Point ret(*this);
		ret += right;
		return ret;
	}

	Point& operator*=(float val)
	{
		this->x *= val;
		this->y *= val;

		return *this;
	}

	Point operator*(float val) const
	{
		Point ret(*this);
		ret *= val;
		return ret;
	}

	bool operator==(Point const& right) const
	{
		return (this->x == right.x && this->y == right.y);
	}


	bool operator!=(Point const& right) const
	{
		return !(*this == right);
	}

	// 计算该点到另外一个点的距离
	float getDistance(Point const& other) const
	{
		return (*this - other).getLength();
	}

	// 计算该点到另外一个点平方距离
	float getDistanceSquared(Point const& other) const
	{
		float dx = other.x - x;
		float dy = other.y - y;
		return (dx * dx + dy * dy);
	}

	// 计算从原点到该点的距离
	float getLength() const 
	{
		return sqrtf(x*x + y*y);
	}


	// 计算从原点到该点的平方距离
	float getLengthSquared() const
	{
		return (x * x + y * y);
	}

	void setPoint(float _x, float _y)
	{
		this->x = _x;
		this->y = _y;
	}

	float x;
	float y;

	static const Point ZERO;

private:
	void copyFrom(Point const& right)
	{
		this->x = right.x;
		this->y = right.y;
	}

};


#endif //__POINT_H__