#pragma once

struct Vector2 {
	float x;
	float y;

	Vector2& operator+=(const Vector2& v) {
		x += v.x;
		y += v.y;
		return *this;
	};
};


struct Matrix2 {
	
	Matrix2(Vector2 const& u, Vector2 const& v) : a{ u.x }, b{ v.x }, c{ u.y }, d{ v.y } {
	};
	Matrix2(float a, float b, float c, float d) : a{ a }, b{ b }, c{ c }, d{ d } {};
	float a;
	float b;
	float c;
	float d;
};


float determinant(Vector2 u, Vector2 v);
float determinant(Matrix2 m);
float dot(Vector2 u, Vector2 v);


Vector2 dot(Matrix2 m, Vector2 v);

Matrix2 dot(Matrix2 m, float s);

Matrix2 inverse(Matrix2 m, float determinant);
Vector2 operator-(Vector2 const& u, Vector2 const& v);
Vector2 operator+(Vector2 const& u, Vector2 const& v);
Vector2 operator*(Matrix2 const& m, Vector2 const& v);
Vector2 operator*(float s, Vector2 const& v);