#pragma once

struct Vector_2d
{
	double x, y;
};

Vector_2d operator-(const Vector_2d& p)
{
	return Vector_2d{ -p.x, -p.y };
}

Vector_2d operator+(const Vector_2d& p1, const Vector_2d& p2)
{
	return Vector_2d{ p1.x + p2.x, p1.y + p2.y };
}

Vector_2d operator-(const Vector_2d& p1, const Vector_2d& p2)
{
	return Vector_2d{ p1.x - p2.x, p1.y - p2.y };
}

Vector_2d operator*(double a, const Vector_2d& p)
{
	return Vector_2d{ p.x * a, p.y * a };
}

Vector_2d operator*(const Vector_2d& p, double a)
{
	return a * p;
}

Vector_2d operator/(const Vector_2d& p, double a)
{
	return Vector_2d{ p.x / a, p.y / a };
}

Vector_2d operator+=(Vector_2d& p1, const Vector_2d& p2)
{
	p1 = p1 + p2;
	return p1;
}

Vector_2d operator-=(Vector_2d& p1, const Vector_2d& p2)
{
	p1 = p1 - p2;
	return p1;
}

Vector_2d operator*=(Vector_2d& p, const double a)
{
	p = p * a;
	return p;
}

Vector_2d operator/=(Vector_2d& p, const double a)
{
	p = p / a;
	return p;
}

double distanceSquared(double x1, double y1, double x2 = 0.0, double y2 = 0.0)
{
	return (x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1);
}

double distanceSquared(Vector_2d a, Vector_2d b = Vector_2d(0.0, 0.0))
{
	return distanceSquared(a.x, a.y, b.x, b.y);
}

double dotProduct(double x1, double y1, double x2, double y2)
{
	return x1 * x2 + y1 * y2;
}

double dotProduct(Vector_2d a, Vector_2d b)
{
	return dotProduct(a.x, a.y, b.x, b.y);
}

double crossProduct(double x1, double y1, double x2, double y2)
{
	return x1 * y2 - x2 * y1;
}

double crossProduct(Vector_2d a, Vector_2d b)
{
	return crossProduct(a.x, a.y, b.x, b.y);
}

Vector_2d getVectorFromPositions(Vector_2d a, Vector_2d b)
{
	return Vector_2d(b.x - a.x, b.y - a.y);
}

//return a 90 deegres counterclockwise vector
Vector_2d getPerpendicularVector(Vector_2d vector)
{
	return Vector_2d(-vector.y, vector.x);
}

double rsqrt(double number)
{
	double y = number;
	double x2 = y * 0.5;
	std::int64_t i = *(std::int64_t*)&y;
	// The magic number is for doubles is from https://cs.uwaterloo.ca/~m32rober/rsqrt.pdf
	i = 0x5fe6eb50c7b537a9 - (i >> 1);
	y = *(double*)&i;
	y = y * (1.5 - (x2 * y * y));   // 1st iteration
	//      y  = y * ( 1.5 - ( x2 * y * y ) );   // 2nd iteration, this can be removed
	return y;
}

Vector_2d normalizeVector(Vector_2d a)
{
	return a * rsqrt(distanceSquared(a));
}

double length(Vector_2d a)
{
	return std::sqrt(distanceSquared(a));
}

Vector_2d projectVector(Vector_2d a, Vector_2d b)
{
	b = normalizeVector(b);
	return dotProduct(a, b) * b;
}