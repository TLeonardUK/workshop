// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/math/vector2.h"

namespace ws {

template <typename T>
struct base_matrix2
{
public:
	T columns[2][2];

	static const base_matrix2<T> identity;
	static const base_matrix2<T> zero;

	T* operator[](int column);
	const T* operator[](int column) const;

	base_vector2<T> get_column(int column) const;
	base_vector2<T> get_row(int row) const;
	void set_column(int column, const base_vector2<T>& vec);
	void set_row(int row, const base_vector2<T>& vec);

	base_matrix2() = default;
	base_matrix2(const base_matrix2& other) = default;
	base_matrix2(T x0, T y0, T x1, T y1);

	base_matrix2& operator=(const base_matrix2& vector) = default;
	base_matrix2& operator*=(T scalar);
	base_matrix2& operator*=(const base_matrix2& other);

	base_matrix2 transpose() const;
};

typedef base_matrix2<float> matrix2;
typedef base_matrix2<double> matrix2d;

template <typename T>
inline const base_matrix2<T> base_matrix2<T>::identity = base_matrix2<T>(
	static_cast<T>(1.0f), static_cast<T>(0.0f),
	static_cast<T>(0.0f), static_cast<T>(1.0f)
);

template <typename T>
inline const base_matrix2<T> base_matrix2<T>::zero = base_matrix2<T>(
	static_cast<T>(0.0f), static_cast<T>(0.0f),
	static_cast<T>(0.0f), static_cast<T>(0.0f)
);

template <typename T>
T* base_matrix2<T>::operator[](int column)
{
	return columns[column];
}

template <typename T>
const T* base_matrix2<T>::operator[](int column) const
{
	return columns[column];
}

template <typename T>
base_vector2<T> base_matrix2<T>::get_column(int column) const
{
	return base_vector2<T>(
		columns[column][0],
		columns[column][1]
	);
}

template <typename T>
base_vector2<T> base_matrix2<T>::get_row(int row) const
{
	return base_vector2<T>(
		columns[0][row],
		columns[1][row]
	);
}

template <typename T>
void base_matrix2<T>::set_column(int column, const base_vector2<T>& vec)
{
	columns[column][0] = vec.x;
	columns[column][1] = vec.y;
}

template <typename T>
void base_matrix2<T>::set_row(int row, const base_vector2<T>& vec)
{
	columns[0][row] = vec.x;
	columns[1][row] = vec.y;
}

template <typename T>
base_matrix2<T>::base_matrix2(T x0, T y0, T x1, T y1)
{
	columns[0][0] = x0;
	columns[0][1] = y0;
	columns[1][0] = x1;
	columns[1][1] = y1;
}

template <typename T>
inline base_matrix2<T> base_matrix2<T>::transpose() const
{
	return base_matrix2<T>(
		columns[0][0],
		columns[1][0],

		columns[0][1],
		columns[1][1]
	);
}

template <typename T>
base_matrix2<T>& base_matrix2<T>::operator*=(T scalar)
{
	columns[0][0] *= scalar;
	columns[0][1] *= scalar;
	columns[1][0] *= scalar;
	columns[1][1] *= scalar;
	return *this;
}

template <typename T>
base_matrix2<T>& base_matrix2<T>::operator*=(const base_matrix2& other)
{
	*this = (*this * other);
	return *this;
}

template <typename T>
inline base_matrix2<T> operator*(const base_matrix2<T>& first, const base_matrix2<T>& second)
{
	base_matrix2<T> result;
	result[0][0] = first[0][0] * second[0][0] + first[1][0] * second[0][1];
	result[0][1] = first[0][1] * second[0][0] + first[1][1] * second[0][1];
	result[1][0] = first[0][0] * second[1][0] + first[1][0] * second[1][1];
	result[1][1] = first[0][1] * second[1][0] + first[1][1] * second[1][1];
	return result;
}

template <typename T>
inline base_matrix2<T> operator/(const base_matrix2<T>& first, float second)
{
	return base_matrix2<T>(first) /= second;
}

template <typename T>
inline bool operator==(const base_matrix2<T>& first, const base_matrix2<T>& second)
{
	return
		first.columns[0][0] == second.columns[0][0] &&
		first.columns[0][1] == second.columns[0][1] &&
		first.columns[1][0] == second.columns[1][0] &&
		first.columns[1][1] == second.columns[1][1];
}

template <typename T>
inline bool operator!=(const base_matrix2<T>& first, const base_matrix2<T>& second)
{
	return !(first == second);
}

}; // namespace ws