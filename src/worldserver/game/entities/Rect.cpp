#include "Rect.h"

#include "Point.h"

const Rect Rect::ZERO = Rect(0, 0, 0, 0);

inline bool intervalOverlap(float x1, float x2, float x3, float x4)
{
	float t;
	if (x3 > x4)
	{
		t = x3;
		x3 = x4;
		x4 = t;
	}

	if (x3 > x2 || x4 < x1)
		return false;
	else
		return true;
}

bool Rect::intersectsLine(Point const& p1, Point const& p2)
{
	Point A = p1, B = p2;
	if (A.y == B.y)
	{
		if (A.y <= this->getMaxY() && A.y >= this->getMinY())
		{
			return intervalOverlap(this->getMinX(), this->getMaxX(), A.x, B.x);
		}
		else
		{
			return false;
		}

	}
	if (A.y > B.y)
		std::swap(A, B);

	float k = (B.x - A.x) / (B.y - A.y);
	Point C, D;
	if (A.y < this->getMinY())
	{
		C.y = this->getMinY();
		C.x = k * (C.y - A.y) + A.x;
	}
	else
		C = A;

	if (B.y > this->getMaxY())
	{
		D.y = this->getMaxY();
		D.x = k * (D.y - A.y) + A.x;
	}
	else
		D = B;

	if (D.y >= C.y)
	{
		return intervalOverlap(this->getMinX(), this->getMaxX(), D.x, C.x);
	}
	else
	{
		return false;
	}
}