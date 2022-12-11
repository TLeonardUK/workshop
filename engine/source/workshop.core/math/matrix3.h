// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/math/quat.h"

namespace ws {

template <typename T>
struct base_matrix3
{
public:
	float columns[3][3];

	static const base_matrix3<T> identity;
	static const base_matrix3<T> zero;

	T* operator[](int column);
	const T* operator[](int column) const;

	base_vector4<T> get_column(int column) const;
	base_vector4<T> get_row(int row) const;
	void set_column(int column, const base_vector3<T>& vec);
	void set_row(int row, const base_vector3<T>& vec);

	base_matrix3() = default;
	base_matrix3(const base_matrix3& other) = default;
	base_matrix3(T x0, T y0, T z0, 
				T x1, T y1, T z1, 
				T x2, T y2, T z2);

	base_matrix3& operator=(const base_matrix3& vector) = default;
	base_matrix3& operator*=(T scalar);
	base_matrix3& operator*=(const base_matrix3& other);

	base_quat<T> to_quat();
};

typedef base_matrix3<float> matrix3;
typedef base_matrix3<double> matrix3d;

template <typename T>
inline const base_matrix3<T> base_matrix3<T>::identity = base_matrix3<T>(
	static_cast<T>(1.0f), static_cast<T>(0.0f), static_cast<T>(0.0f),
	static_cast<T>(0.0f), static_cast<T>(1.0f), static_cast<T>(0.0f),
	static_cast<T>(0.0f), static_cast<T>(0.0f), static_cast<T>(1.0f)
);

template <typename T>
inline const base_matrix3<T> base_matrix3<T>::zero = base_matrix3<T>(
	static_cast<T>(0.0f), static_cast<T>(0.0f), static_cast<T>(0.0f),
	static_cast<T>(0.0f), static_cast<T>(0.0f), static_cast<T>(0.0f),
	static_cast<T>(0.0f), static_cast<T>(0.0f), static_cast<T>(0.0f)
);

template <typename T>
inline T* base_matrix3<T>::operator[](int column)
{
	return columns[column];
}

template <typename T>
inline const T* base_matrix3<T>::operator[](int column) const
{
	return columns[column];
}

template <typename T>
inline base_vector4<T> base_matrix3<T>::get_column(int column) const
{
	return base_vector3<T>(
		columns[column][0],
		columns[column][1],
		columns[column][2]
	);
}

template <typename T>
inline base_vector4<T> base_matrix3<T>::get_row(int row) const
{
	return base_vector3<T>(
		columns[0][row],
		columns[1][row],
		columns[2][row]
	);
}

template <typename T>
inline void base_matrix3<T>::set_column(int column, const base_vector3<T>& vec)
{
	columns[column][0] = vec.x;
	columns[column][1] = vec.y;
	columns[column][2] = vec.z;
}

template <typename T>
inline void base_matrix3<T>::set_row(int row, const base_vector3<T>& vec)
{
	columns[0][row] = vec.x;
	columns[1][row] = vec.y;
	columns[2][row] = vec.z;
}

template <typename T>
inline base_matrix3<T>::base_matrix3(
	T x0, T y0, T z0,
	T x1, T y1, T z1, 
	T x2, T y2, T z2)
{
	columns[0][0] = x0;
	columns[0][1] = y0;
	columns[0][2] = z0;
	columns[1][0] = x1;
	columns[1][1] = y1;
	columns[1][2] = z1;
	columns[2][0] = x2;
	columns[2][1] = y2;
	columns[2][2] = z2;
}

template <typename T>
inline base_matrix3<T>& base_matrix3<T>::operator*=(T scalar)
{
	columns[0][0] *= scalar;
	columns[0][1] *= scalar;
	columns[0][2] *= scalar;
	columns[1][0] *= scalar;
	columns[1][1] *= scalar;
	columns[1][2] *= scalar;
	columns[2][0] *= scalar;
	columns[2][1] *= scalar;
	columns[2][2] *= scalar;
	return *this;
}

template <typename T>
inline base_matrix3<T>& base_matrix3<T>::operator*=(const base_matrix3& other)
{
	*this = (*this * other);
	return *this;
}

template <typename T>
inline base_quat<T> base_matrix3<T>::to_quat()
{
	T fourXSquaredMinus1 = columns[0][0] - columns[1][1] - columns[2][2];
	T fourYSquaredMinus1 = columns[1][1] - columns[0][0] - columns[2][2];
	T fourZSquaredMinus1 = columns[2][2] - columns[0][0] - columns[1][1];
	T fourWSquaredMinus1 = columns[0][0] + columns[1][1] + columns[2][2];

	int biggestIndex = 0;
	T fourBiggestSquaredMinus1 = fourWSquaredMinus1;
	if (fourXSquaredMinus1 > fourBiggestSquaredMinus1)
	{
		fourBiggestSquaredMinus1 = fourXSquaredMinus1;
		biggestIndex = 1;
	}
	if (fourYSquaredMinus1 > fourBiggestSquaredMinus1)
	{
		fourBiggestSquaredMinus1 = fourYSquaredMinus1;
		biggestIndex = 2;
	}
	if (fourZSquaredMinus1 > fourBiggestSquaredMinus1)
	{
		fourBiggestSquaredMinus1 = fourZSquaredMinus1;
		biggestIndex = 3;
	}

	T biggestVal = sqrt(fourBiggestSquaredMinus1 + static_cast<T>(1)) * static_cast<T>(0.5);
	T mult = static_cast<T>(0.25) / biggestVal;

	switch (biggestIndex)
	{
	case 0:
		return base_quat<T>((columns[1][2] - columns[2][1]) * mult, (columns[2][0] - columns[0][2]) * mult, (columns[0][1] - columns[1][0]) * mult, biggestVal);
	case 1:
		return base_quat<T>(biggestVal, (columns[0][1] + columns[1][0]) * mult, (columns[2][0] + columns[0][2]) * mult, (columns[1][2] - columns[2][1]) * mult);
	case 2:
		return base_quat<T>((columns[0][1] + columns[1][0]) * mult, biggestVal, (columns[1][2] + columns[2][1]) * mult, (columns[2][0] - columns[0][2]) * mult);
	case 3:
		return base_quat<T>((columns[2][0] + columns[0][2]) * mult, (columns[1][2] + columns[2][1]) * mult, biggestVal, (columns[0][1] - columns[1][0]) * mult);
	default: 
		db_assert(false);
		return base_quat<T>(0, 0, 0, 1);
	}
}

template <typename T>
inline base_matrix3<T> operator*(const base_matrix3<T>& first, const base_matrix3<T>& second)
{
	base_matrix2<T> result;

	result[0][0] = first[0][0] * second[0][0] + first[1][0] * second[0][1] + first[2][0] * second[0][2];
	result[0][1] = first[0][1] * second[0][0] + first[1][1] * second[0][1] + first[2][1] * second[0][2];
	result[0][2] = first[0][2] * second[0][0] + first[1][2] * second[0][1] + first[2][2] * second[0][2];
	result[1][0] = first[0][0] * second[1][0] + first[1][0] * second[1][1] + first[2][0] * second[1][2];
	result[1][1] = first[0][1] * second[1][0] + first[1][1] * second[1][1] + first[2][1] * second[1][2];
	result[1][2] = first[0][2] * second[1][0] + first[1][2] * second[1][1] + first[2][2] * second[1][2];
	result[2][0] = first[0][0] * second[2][0] + first[1][0] * second[2][1] + first[2][0] * second[2][2];
	result[2][1] = first[0][1] * second[2][0] + first[1][1] * second[2][1] + first[2][1] * second[2][2];
	result[2][2] = first[0][2] * second[2][0] + first[1][2] * second[2][1] + first[2][2] * second[2][2];

	return result;
}

template <typename T>
inline base_matrix3<T> operator/(const base_matrix3<T>& first, float second)
{
	return base_matrix3<T>(first) /= second;
}

template <typename T>
inline bool operator==(const base_matrix3<T>& first, const base_matrix3<T>& second)
{
	return
		first.columns[0][0] == second.columns[0][0] &&
		first.columns[0][1] == second.columns[0][1] &&
		first.columns[0][2] == second.columns[0][2] &&

		first.columns[1][0] == second.columns[1][0] &&
		first.columns[1][1] == second.columns[1][1] &&
		first.columns[1][2] == second.columns[1][2] &&

		first.columns[2][0] == second.columns[2][0] &&
		first.columns[2][1] == second.columns[2][1] &&
		first.columns[2][2] == second.columns[2][2];
}

template <typename T>
inline bool operator!=(const base_matrix3<T>& first, const base_matrix3<T>& second)
{
	return !(first == second);
}

}; // namespace ws