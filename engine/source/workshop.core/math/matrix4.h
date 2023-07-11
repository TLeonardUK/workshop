// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/math/vector3.h"
#include "workshop.core/math/vector4.h"
#include "workshop.core/math/quat.h"
#include "workshop.core/math/vector3.h"

namespace ws {

// 4x4 matrix, stored in column-major order.

template <typename T>
struct base_matrix4
{
public:
	T columns[4][4];

	static const base_matrix4<T> identity;
	static const base_matrix4<T> zero;

	T* operator[](int column);
	const T* operator[](int column) const;

	base_vector4<T> get_column(int column) const;
	base_vector4<T> get_row(int row) const;
	void set_column(int column, const base_vector4<T>& vec);
	void set_row(int row, const base_vector4<T>& vec);

	base_matrix4() = default;
	base_matrix4(const base_matrix4& other) = default;
	base_matrix4(T x0, T y0, T z0, T w0,
				T x1, T y1, T z1, T w1,
				T x2, T y2, T z2, T w2,
				T x3, T y3, T z3, T w3);

	base_matrix4(base_vector4<T> col1,
				base_vector4<T> col2,
				base_vector4<T> col3,
				base_vector4<T> col4);

	base_matrix4& operator=(const base_matrix4& vector) = default;
	base_matrix4& operator*=(T scalar);
	base_matrix4& operator*=(const base_matrix4& other);

	base_vector3<T> extract_scale() const;
	base_vector3<T> extract_rotation() const;

	base_vector3<T> transform_direction(const base_vector3<T>& vec) const;
	base_vector4<T> transform_direction(const base_vector4<T>& vec) const;
	base_vector3<T> transform_vector(const base_vector3<T>& vec) const;
	base_vector4<T> transform_vector(const base_vector4<T>& vec) const;
	base_vector3<T> transform_location(const base_vector3<T>& vec) const;
	base_vector4<T> transform_location(const base_vector4<T>& vec) const;
	base_matrix4 inverse() const;
	base_quat<T> to_quat() const;
	base_matrix4 transpose() const;
	
	static base_matrix4 translate(const vector3& position);
	static base_matrix4 scale(const vector3& scale);
	static base_matrix4 rotation(const quat& quat);
	static base_matrix4 look_at(const base_vector3<T>& eye, const base_vector3<T>& center, const base_vector3<T>& up);
	static base_matrix4 orthographic(T left, T right, T bottom, T top, T nearZ, T farZ);
	static base_matrix4 perspective(T fovRadians, T aspect, T zNear, T zFar);
};

typedef base_matrix4<float> matrix4;
typedef base_matrix4<double> matrix4d;

template <typename T>
inline const base_matrix4<T> base_matrix4<T>::identity = base_matrix4<T>(
	static_cast<T>(1.0f), static_cast<T>(0.0f), static_cast<T>(0.0f), static_cast<T>(0.0f),
	static_cast<T>(0.0f), static_cast<T>(1.0f), static_cast<T>(0.0f), static_cast<T>(0.0f),
	static_cast<T>(0.0f), static_cast<T>(0.0f), static_cast<T>(1.0f), static_cast<T>(0.0f),
	static_cast<T>(0.0f), static_cast<T>(0.0f), static_cast<T>(0.0f), static_cast<T>(1.0f)
);

template <typename T>
inline const base_matrix4<T> base_matrix4<T>::zero = base_matrix4<T>(
	static_cast<T>(0.0f), static_cast<T>(0.0f), static_cast<T>(0.0f), static_cast<T>(0.0f),
	static_cast<T>(0.0f), static_cast<T>(0.0f), static_cast<T>(0.0f), static_cast<T>(0.0f),
	static_cast<T>(0.0f), static_cast<T>(0.0f), static_cast<T>(0.0f), static_cast<T>(0.0f),
	static_cast<T>(0.0f), static_cast<T>(0.0f), static_cast<T>(0.0f), static_cast<T>(0.0f)
);

template <typename T>
inline T* base_matrix4<T>::operator[](int column)
{
	return columns[column];
}

template <typename T>
inline const T* base_matrix4<T>::operator[](int column) const
{
	return columns[column];
}

template <typename T>
inline base_vector4<T> base_matrix4<T>::get_column(int column) const
{
	return base_vector4<T>(
		columns[column][0],
		columns[column][1],
		columns[column][2],
		columns[column][3]
	);
}

template <typename T>
inline base_vector4<T> base_matrix4<T>::get_row(int row) const
{
	return base_vector4<T>(
		columns[0][row],
		columns[1][row],
		columns[2][row],
		columns[3][row]
	);
}

template <typename T>
inline void base_matrix4<T>::set_column(int column, const base_vector4<T>& vec)
{
	columns[column][0] = vec.x;
	columns[column][1] = vec.y;
	columns[column][2] = vec.z;
	columns[column][3] = vec.w;
}

template <typename T>
inline void base_matrix4<T>::set_row(int row, const base_vector4<T>& vec)
{
	columns[0][row] = vec.x;
	columns[1][row] = vec.y;
	columns[2][row] = vec.z;
	columns[3][row] = vec.w;
}

template <typename T>
inline base_matrix4<T>::base_matrix4(
	T x0, T y0, T z0, T w0,
	T x1, T y1, T z1, T w1,
	T x2, T y2, T z2, T w2,
	T x3, T y3, T z3, T w3)
{
	columns[0][0] = x0;
	columns[0][1] = y0;
	columns[0][2] = z0;
	columns[0][3] = w0;

	columns[1][0] = x1;
	columns[1][1] = y1;
	columns[1][2] = z1;
	columns[1][3] = w1;

	columns[2][0] = x2;
	columns[2][1] = y2;
	columns[2][2] = z2;
	columns[2][3] = w2;

	columns[3][0] = x3;
	columns[3][1] = y3;
	columns[3][2] = z3;
	columns[3][3] = w3;
}

template <typename T>
inline base_matrix4<T>::base_matrix4(
	base_vector4<T> col1,
	base_vector4<T> col2,
	base_vector4<T> col3,
	base_vector4<T> col4)
{
	columns[0][0] = col1.x;
	columns[0][1] = col1.y;
	columns[0][2] = col1.z;
	columns[0][3] = col1.w;

	columns[1][0] = col2.x;
	columns[1][1] = col2.y;
	columns[1][2] = col2.z;
	columns[1][3] = col2.w;

	columns[2][0] = col3.x;
	columns[2][1] = col3.y;
	columns[2][2] = col3.z;
	columns[2][3] = col3.w;

	columns[3][0] = col4.x;
	columns[3][1] = col4.y;
	columns[3][2] = col4.z;
	columns[3][3] = col4.w;
}

template <typename T>
inline base_matrix4<T>& base_matrix4<T>::operator*=(T scalar)
{
	columns[0][0] *= scalar;
	columns[0][1] *= scalar;
	columns[0][2] *= scalar;
	columns[0][3] *= scalar;

	columns[1][0] *= scalar;
	columns[1][1] *= scalar;
	columns[1][2] *= scalar;
	columns[1][3] *= scalar;

	columns[2][0] *= scalar;
	columns[2][1] *= scalar;
	columns[2][2] *= scalar;
	columns[2][3] *= scalar;

	columns[3][0] *= scalar;
	columns[3][1] *= scalar;
	columns[3][2] *= scalar;
	columns[3][3] *= scalar;
	return *this;
}

template <typename T>
inline base_matrix4<T>& base_matrix4<T>::operator*=(const base_matrix4& other)
{
	*this = (*this * other);
	return *this;
}

template <typename T>
inline base_vector3<T> base_matrix4<T>::extract_scale() const
{
	return base_vector3<T>(
		get_column(0).length(),
		get_column(1).length(),
		get_column(2).length()
	);
}

template <typename T>
inline base_vector3<T> base_matrix4<T>::extract_rotation() const
{
	return to_quat();
}

template <typename T>
inline base_vector3<T> base_matrix4<T>::transform_direction(const base_vector3<T>& vec) const
{
	base_vector4<T> result(
		           	1.0f * vec.x + columns[1][0] * vec.y + columns[2][0] * vec.z + columns[3][0] * 0.0f,
		columns[0][1] * vec.x +          1.0f * vec.y + columns[2][1] * vec.z + columns[3][1] * 0.0f,
		columns[0][2] * vec.x + columns[1][2] * vec.y +          1.0f * vec.z + columns[3][2] * 0.0f,
		columns[0][3] * vec.x + columns[1][3] * vec.y + columns[2][3] * vec.z +          1.0f * 0.0f);

	return base_vector3<T>(result.x, result.y, result.z);
}

template <typename T>
inline base_vector4<T> base_matrix4<T>::transform_direction(const base_vector4<T>& vec) const
{
	base_vector4<T> result(
					1.0f * vec.x + columns[1][0] * vec.y + columns[2][0] * vec.z + columns[3][0] * vec.w,
		columns[0][1] * vec.x +			 1.0f * vec.y + columns[2][1] * vec.z + columns[3][1] * vec.w,
		columns[0][2] * vec.x + columns[1][2] * vec.y +			 1.0f * vec.z + columns[3][2] * vec.w,
		columns[0][3] * vec.x + columns[1][3] * vec.y + columns[2][3] * vec.z +			 1.0f * vec.w);

	return base_vector4<T>(result.x, result.y, result.z, result.w);
}

template <typename T>
inline base_vector3<T> base_matrix4<T>::transform_vector(const base_vector3<T>& vec) const
{
	base_vector4<T> result(
		columns[0][0] * vec.x + columns[1][0] * vec.y + columns[2][0] * vec.z + columns[3][0] * 0.0f,
		columns[0][1] * vec.x + columns[1][1] * vec.y + columns[2][1] * vec.z + columns[3][1] * 0.0f,
		columns[0][2] * vec.x + columns[1][2] * vec.y + columns[2][2] * vec.z + columns[3][2] * 0.0f,
		columns[0][3] * vec.x + columns[1][3] * vec.y + columns[2][3] * vec.z + columns[3][3] * 0.0f);

	return base_vector3<T>(result.x, result.y, result.z);
}

template <typename T>
inline base_vector4<T> base_matrix4<T>::transform_vector(const base_vector4<T>& vec) const
{
	base_vector4<T> result(
		columns[0][0] * vec.x + columns[1][0] * vec.y + columns[2][0] * vec.z + columns[3][0] * vec.w,
		columns[0][1] * vec.x + columns[1][1] * vec.y + columns[2][1] * vec.z + columns[3][1] * vec.w,
		columns[0][2] * vec.x + columns[1][2] * vec.y + columns[2][2] * vec.z + columns[3][2] * vec.w,
		columns[0][3] * vec.x + columns[1][3] * vec.y + columns[2][3] * vec.z + columns[3][3] * vec.w);

	return base_vector4<T>(result.x, result.y, result.z, result.w);
}

template <typename T>
inline base_vector3<T> base_matrix4<T>::transform_location(const base_vector3<T>& vec) const
{
	base_vector4<T> result(
		columns[0][0] * vec.x + columns[1][0] * vec.y + columns[2][0] * vec.z + columns[3][0] * 1.0f,
		columns[0][1] * vec.x + columns[1][1] * vec.y + columns[2][1] * vec.z + columns[3][1] * 1.0f,
		columns[0][2] * vec.x + columns[1][2] * vec.y + columns[2][2] * vec.z + columns[3][2] * 1.0f,
		columns[0][3] * vec.x + columns[1][3] * vec.y + columns[2][3] * vec.z + columns[3][3] * 1.0f);

	return base_vector3<T>(result.x, result.y, result.z);
}

template <typename T>
inline base_vector4<T> base_matrix4<T>::transform_location(const base_vector4<T>& vec) const
{
	base_vector4<T> result(
		columns[0][0] * vec.x + columns[1][0] * vec.y + columns[2][0] * vec.z + columns[3][0] * vec.w,
		columns[0][1] * vec.x + columns[1][1] * vec.y + columns[2][1] * vec.z + columns[3][1] * vec.w,
		columns[0][2] * vec.x + columns[1][2] * vec.y + columns[2][2] * vec.z + columns[3][2] * vec.w,
		columns[0][3] * vec.x + columns[1][3] * vec.y + columns[2][3] * vec.z + columns[3][3] * vec.w);

	return base_vector4<T>(result.x, result.y, result.z, result.w);
}

template <typename T>
inline base_matrix4<T> base_matrix4<T>::inverse() const
{
	T coef00 = columns[2][2] * columns[3][3] - columns[3][2] * columns[2][3];
	T coef02 = columns[1][2] * columns[3][3] - columns[3][2] * columns[1][3];
	T coef03 = columns[1][2] * columns[2][3] - columns[2][2] * columns[1][3];

	T coef04 = columns[2][1] * columns[3][3] - columns[3][1] * columns[2][3];
	T coef06 = columns[1][1] * columns[3][3] - columns[3][1] * columns[1][3];
	T coef07 = columns[1][1] * columns[2][3] - columns[2][1] * columns[1][3];

	T coef08 = columns[2][1] * columns[3][2] - columns[3][1] * columns[2][2];
	T coef10 = columns[1][1] * columns[3][2] - columns[3][1] * columns[1][2];
	T coef11 = columns[1][1] * columns[2][2] - columns[2][1] * columns[1][2];

	T coef12 = columns[2][0] * columns[3][3] - columns[3][0] * columns[2][3];
	T coef14 = columns[1][0] * columns[3][3] - columns[3][0] * columns[1][3];
	T coef15 = columns[1][0] * columns[2][3] - columns[2][0] * columns[1][3];

	T coef16 = columns[2][0] * columns[3][2] - columns[3][0] * columns[2][2];
	T coef18 = columns[1][0] * columns[3][2] - columns[3][0] * columns[1][2];
	T coef19 = columns[1][0] * columns[2][2] - columns[2][0] * columns[1][2];

	T coef20 = columns[2][0] * columns[3][1] - columns[3][0] * columns[2][1];
	T coef22 = columns[1][0] * columns[3][1] - columns[3][0] * columns[1][1];
	T coef23 = columns[1][0] * columns[2][1] - columns[2][0] * columns[1][1];

	base_vector4<T> fac0(coef00, coef00, coef02, coef03);
	base_vector4<T> fac1(coef04, coef04, coef06, coef07);
	base_vector4<T> fac2(coef08, coef08, coef10, coef11);
	base_vector4<T> fac3(coef12, coef12, coef14, coef15);
	base_vector4<T> fac4(coef16, coef16, coef18, coef19);
	base_vector4<T> fac5(coef20, coef20, coef22, coef23);

	base_vector4<T> vec0(columns[1][0], columns[0][0], columns[0][0], columns[0][0]);
	base_vector4<T> vec1(columns[1][1], columns[0][1], columns[0][1], columns[0][1]);
	base_vector4<T> vec2(columns[1][2], columns[0][2], columns[0][2], columns[0][2]);
	base_vector4<T> vec3(columns[1][3], columns[0][3], columns[0][3], columns[0][3]);

	base_vector4<T> inv0(vec1 * fac0 - vec2 * fac1 + vec3 * fac2);
	base_vector4<T> inv1(vec0 * fac0 - vec2 * fac3 + vec3 * fac4);
	base_vector4<T> inv2(vec0 * fac1 - vec1 * fac3 + vec3 * fac5);
	base_vector4<T> inv3(vec0 * fac2 - vec1 * fac4 + vec2 * fac5);

	base_vector4<T> signA(+1, -1, +1, -1);
	base_vector4<T> signB(-1, +1, -1, +1);
	base_matrix4<T> inverse(inv0 * signA, inv1 * signB, inv2 * signA, inv3 * signB);

	base_vector4<T> row0(inverse[0][0], inverse[1][0], inverse[2][0], inverse[3][0]);

	base_vector4<T> dot0(
		columns[0][0] * row0.x,
		columns[0][1] * row0.y,
		columns[0][2] * row0.z,
		columns[0][3] * row0.w);
	T dot1 = (dot0.x + dot0.y) + (dot0.z + dot0.w);

	T one_over_determinant = static_cast<T>(1) / dot1;

	return inverse * one_over_determinant;
}

template <typename T>
inline base_quat<T> base_matrix4<T>::to_quat() const
{
	return base_matrix3<T>(
		columns[0][0], columns[0][1], columns[0][2],
		columns[1][0], columns[1][1], columns[1][2],
		columns[2][0], columns[2][1], columns[2][2]
	).to_quat();
}

template <typename T>
inline base_matrix4<T> base_matrix4<T>::transpose() const
{
	return base_matrix4<T>(
		columns[0][0],
		columns[1][0],
		columns[2][0],
		columns[3][0],

		columns[0][1],
		columns[1][1],
		columns[2][1],
		columns[3][1],

		columns[0][2],
		columns[1][2],
		columns[2][2],
		columns[3][2],

		columns[0][3],
		columns[1][3],
		columns[2][3],
		columns[3][3]
	);
}

template <typename T>
inline base_matrix4<T> base_matrix4<T>::translate(const vector3& position)
{
	base_matrix4 result = identity;
    result[3][0] = position.x;
    result[3][1] = position.y;
    result[3][2] = position.z;
	return result; 
}

template <typename T>
inline base_matrix4<T> base_matrix4<T>::scale(const vector3& scale)
{
	base_matrix4 result = identity;
	result[0][0] = scale.x;
	result[1][1] = scale.y;
	result[2][2] = scale.z;
	return result;
}

template <typename T>
inline base_matrix4<T> base_matrix4<T>::rotation(const quat& quat)
{
	base_matrix4 result = identity;

	T qxx(quat.x * quat.x);
	T qyy(quat.y * quat.y);
	T qzz(quat.z * quat.z);
	T qxz(quat.x * quat.z);
	T qxy(quat.x * quat.y);
	T qyz(quat.y * quat.z);
	T qwx(quat.w * quat.x);
	T qwy(quat.w * quat.y);
	T qwz(quat.w * quat.z);

	result[0][0] = T(1) - T(2) * (qyy + qzz);
	result[0][1] = T(2) * (qxy + qwz);
	result[0][2] = T(2) * (qxz - qwy);
	result[0][3] = T(0);

	result[1][0] = T(2) * (qxy - qwz);
	result[1][1] = T(1) - T(2) * (qxx + qzz);
	result[1][2] = T(2) * (qyz + qwx);
	result[1][3] = T(0);

	result[2][0] = T(2) * (qxz + qwy);
	result[2][1] = T(2) * (qyz - qwx);
	result[2][2] = T(1) - T(2) * (qxx + qyy);
	result[2][3] = T(0);

	result[3][0] = T(0);
	result[3][1] = T(0);
	result[3][2] = T(0);
	result[3][3] = T(1);

	return result;
}

template <typename T>
inline base_matrix4<T> base_matrix4<T>::look_at(const base_vector3<T>& eye, const base_vector3<T>& center, const base_vector3<T>& up)
{
	base_vector3<T> eye_direction = (center - eye);
	base_vector3<T> r2 = eye_direction.normalize();
	base_vector3<T> r0 = base_vector3<T>::cross(up, r2).normalize();
	base_vector3<T> r1 = base_vector3<T>::cross(r2, r0);

	base_vector3<T> neg_eye_position = -eye;

	float d0 = base_vector3<T>::dot(r0, neg_eye_position);
	float d1 = base_vector3<T>::dot(r1, neg_eye_position);
	float d2 = base_vector3<T>::dot(r2, neg_eye_position);

	base_matrix4<T> result = base_matrix4<T>(
		r0.x, r0.y, r0.z, d0,
		r1.x, r1.y, r1.z, d1,
		r2.x, r2.y, r2.z, d2,
		0.0f, 0.0f, 0.0f, 1.0f
	);

	return result;
}

template <typename T>
inline base_matrix4<T> base_matrix4<T>::orthographic(T left, T right, T bottom, T top, T nearZ, T farZ)
{
	float a = 1.0f / (farZ - nearZ);
	float b = -a * nearZ;

	base_matrix4<T> result = base_matrix4<T>(
		2.0f / (right - left),				0.0f,								0.0f,	0.0f,
		0.0f,								2.0f / (bottom - top),				0.0f,	0.0f,
		0.0f,								0.0f,								a,		0.0f,
		-(right + left) / (right - left),	-(bottom + top) / (bottom - top),	b,		1.0f
	);

	return result.transpose();
}

template <typename T>
inline base_matrix4<T> base_matrix4<T>::perspective(T fovRadians, T aspect, T zNear, T zFar)
{
	float h = 1.0f / tan(fovRadians * 0.5f);
	float w = h / aspect;
	float a = zFar / (zFar - zNear);
	float b = (-zNear * zFar) / (zFar - zNear);

	base_matrix4<T> result = base_matrix4<T>(
		w, 0.0f, 0.0f, 0.0f,
		0.0f, h, 0.0f, 0.0f,
		0.0f, 0.0f, a, b,
		0.0f, 0.0f, 1.0f, 0.0f
	);

	return result;
}

template <typename T>
inline base_matrix4<T> operator*(const base_matrix4<T>& first, const base_matrix4<T>& second)
{
	base_matrix4<T> result;

	result[0][0] = first[0][0] * second[0][0] + first[1][0] * second[0][1] + first[2][0] * second[0][2] + first[3][0] * second[0][3];
	result[0][1] = first[0][1] * second[0][0] + first[1][1] * second[0][1] + first[2][1] * second[0][2] + first[3][1] * second[0][3];
	result[0][2] = first[0][2] * second[0][0] + first[1][2] * second[0][1] + first[2][2] * second[0][2] + first[3][2] * second[0][3];
	result[0][3] = first[0][3] * second[0][0] + first[1][3] * second[0][1] + first[2][3] * second[0][2] + first[3][3] * second[0][3];

	result[1][0] = first[0][0] * second[1][0] + first[1][0] * second[1][1] + first[2][0] * second[1][2] + first[3][0] * second[1][3];
	result[1][1] = first[0][1] * second[1][0] + first[1][1] * second[1][1] + first[2][1] * second[1][2] + first[3][1] * second[1][3];
	result[1][2] = first[0][2] * second[1][0] + first[1][2] * second[1][1] + first[2][2] * second[1][2] + first[3][2] * second[1][3];
	result[1][3] = first[0][3] * second[1][0] + first[1][3] * second[1][1] + first[2][3] * second[1][2] + first[3][3] * second[1][3];

	result[2][0] = first[0][0] * second[2][0] + first[1][0] * second[2][1] + first[2][0] * second[2][2] + first[3][0] * second[2][3];
	result[2][1] = first[0][1] * second[2][0] + first[1][1] * second[2][1] + first[2][1] * second[2][2] + first[3][1] * second[2][3];
	result[2][2] = first[0][2] * second[2][0] + first[1][2] * second[2][1] + first[2][2] * second[2][2] + first[3][2] * second[2][3];
	result[2][3] = first[0][3] * second[2][0] + first[1][3] * second[2][1] + first[2][3] * second[2][2] + first[3][3] * second[2][3];

	result[3][0] = first[0][0] * second[3][0] + first[1][0] * second[3][1] + first[2][0] * second[3][2] + first[3][0] * second[3][3];
	result[3][1] = first[0][1] * second[3][0] + first[1][1] * second[3][1] + first[2][1] * second[3][2] + first[3][1] * second[3][3];
	result[3][2] = first[0][2] * second[3][0] + first[1][2] * second[3][1] + first[2][2] * second[3][2] + first[3][2] * second[3][3];
	result[3][3] = first[0][3] * second[3][0] + first[1][3] * second[3][1] + first[2][3] * second[3][2] + first[3][3] * second[3][3];

	return result;
}

template <typename T>
inline base_matrix4<T> operator*(const base_matrix4<T>& first, float second)
{
	base_matrix4<T> result = first;
	result *= second;
	return result;
}

template <typename T>
inline base_vector3<T> operator*(const base_vector3<T>& vec, const base_matrix4<T>& mat)
{
	return mat.transform_location(vec);
}

template <typename T>
inline base_matrix4<T> operator/(const base_matrix4<T>& first, float second)
{
	return base_matrix4<T>(first) /= second;
}

template <typename T>
inline bool operator==(const base_matrix4<T>& first, const base_matrix4<T>& second)
{
	return
		first.columns[0][0] == second.columns[0][0] &&
		first.columns[0][1] == second.columns[0][1] &&
		first.columns[0][2] == second.columns[0][2] &&
		first.columns[0][3] == second.columns[0][3] &&

		first.columns[1][0] == second.columns[1][0] &&
		first.columns[1][1] == second.columns[1][1] &&
		first.columns[1][2] == second.columns[1][2] &&
		first.columns[1][3] == second.columns[1][3] &&

		first.columns[2][0] == second.columns[2][0] &&
		first.columns[2][1] == second.columns[2][1] &&
		first.columns[2][2] == second.columns[2][2] &&
		first.columns[2][3] == second.columns[2][3] &&

		first.columns[3][0] == second.columns[3][0] &&
		first.columns[3][1] == second.columns[3][1] &&
		first.columns[3][2] == second.columns[3][2] &&
		first.columns[3][3] == second.columns[3][3];
}

template <typename T>
inline bool operator!=(const base_matrix4<T>& first, const base_matrix4<T>& second)
{
	return !(first == second);
}

}; // namespace ws
