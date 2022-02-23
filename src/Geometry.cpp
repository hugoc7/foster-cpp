#include <Geometry.h>
#include <utility>

Vector2 operator-(Vector2 const& u, Vector2 const& v) {
	return Vector2{ u.x - v.x, u.y - v.y };
}

Vector2 operator+(Vector2 const& u, Vector2 const& v) {
	return Vector2{ u.x + v.x, u.y + v.y };
}

Vector2 operator*(Matrix2 const& m, Vector2 const& v) {
	return dot(m, v);
}

Vector2 operator*(float s, Vector2 const& v) {
	Vector2 res{ s * v.x, s * v.y };
	return res;
}

float determinant(Vector2 u, Vector2 v) {
	return u.x * v.y - u.y * v.x;
}
float determinant(Matrix2 m) {
	return m.a*m.d - m.b*m.c;
}

float dot(Vector2 u, Vector2 v) {
	return u.x * v.x + u.y * v.y;
}

Vector2 dot(Matrix2 m, Vector2 v) {
	Vector2 res { m.a * v.x + m.b * v.y, m.c * v.x + m.d * v.y };
	return res;
}

Matrix2 dot(Matrix2 m, float s) {
	Matrix2 res{ m };
	res.a *= s;
	res.b *= s;
	res.c *= s;
	res.d *= s;
	return res;
}

Matrix2 inverse(Matrix2 m, float determinant) {
	Matrix2 inv{ m.d, -1.0f * m.b, -1.0f * m.c, m.a };
	return dot(inv, 1.0f / determinant);
}

float norme(Vector2 v) {
	return std::sqrt(dot(v, v));
}
Vector2 normalVector(Vector2 v) {
	return Vector2(-1.0f * v.y, v.x);
}