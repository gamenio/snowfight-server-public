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

	// Calculate the distance between the point and another point
	float getDistance(Point const& other) const
	{
		return (*this - other).getLength();
	}

	// Calculate the square distance between the point and another point
	float getDistanceSquared(Point const& other) const
	{
		float dx = other.x - x;
		float dy = other.y - y;
		return (dx * dx + dy * dy);
	}

	// Calculate the distance from the origin to the point
	float getLength() const 
	{
		return sqrtf(x*x + y*y);
	}


	// Calculate the square distance from the origin to the point
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


#endif // __POINT_H__