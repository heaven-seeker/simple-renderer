#pragma once

#include <cmath>
#include <cassert>
#include <iostream>

namespace geom {
    /* Vector Implementation */
    template<int n> struct vec {
        double data[n] = {0};
        double& operator[](const int i) {assert(i >= 0 && i < n); return data[i];}
        double  operator[](const int i) const { assert(i>=0 && i<n); return data[i]; }
    };

    template<> struct vec<2> {
        union {
            struct { double x, y; };
            double data[2];
        };

        constexpr vec() : x(0), y(0) {}
        constexpr vec(double x, double y) : x(x), y(y) {}

        double& operator[](const int i) {
            assert(i>=0 && i<2); return data[i];
        }
        double operator[](const int i) const {
            assert(i>=0 && i<2); return data[i];
        }
    };

    template<> struct vec<3> {
        union {
            struct { double x, y, z; };
            double data[3];
        };

        constexpr vec() : x(0), y(0), z(0) {}
        constexpr vec(double x, double y, double z) : x(x), y(y), z(z) {}

        double& operator[](const int i) {
            assert(i>=0 && i<3); return data[i];
        }
        double operator[](const int i) const {
            assert(i>=0 && i<3); return data[i];
        }
    };

    template<> struct vec<4> {
        union {
            struct { double x, y , z, w; };
            double data[4];
        };

        constexpr vec() : x(0), y(0), z(0), w(0) {}
        constexpr vec(double x, double y, double z, double w) : x(x), y(y), z(z), w(w) {}

        double& operator[](const int i) {
            assert(i>=0 && i<4);
            return data[i];
        }
        double operator[](const int i) const {
            assert(i>=0 && i<4);
            return data[i];
        }

        vec<3> xyz() const {
            return vec<3>(x, y, z);
        }
    };

    typedef vec<2> vec2;
    typedef vec<3> vec3;
    typedef vec<4> vec4;

    template<int n> vec<n> operator+(const vec<n>& v1, const vec<n>& v2) {
        vec<n> result;
        for (int i = 0; i < n; i++) {
            result[i] = v1[i] + v2[i];
        }
        return result;
    }

    template<int n> vec<n> operator-(const vec<n>& v1, const vec<n>& v2) {
        vec<n> result;
        for (int i = 0; i < n; i++) {
            result[i] = v1[i] - v2[i];
        }
        return result;
    }

    template<int n> vec<n> operator*(const vec<n>& v1, const double scalar) {
        vec<n> result;
        for (int i = 0; i < n; i++) {
            result[i] = v1[i] * scalar;
        }
        return result;
    }

    template<int n> vec<n> operator*(const double scalar, const vec<n>& v1) {
        return v1 * scalar;
    }

    template<int n> vec<n> operator/(const vec<n>& v1, const double scalar) {
        vec<n> result;
        for (int i = 0; i < n; i++) {
            result[i] = v1[i] / scalar;
        }
        return result;
    }

    template<int n> vec<n> operator/(const double scalar, const vec<n>& v1) {
        return v1 / scalar;
    }

    template<int n> std::ostream& operator<<(std::ostream& out, const vec<n>& v) {
        for (int i = 0; i < n; i++) {
            out << v[i] << " ";
        }
        return out;
    }

    template<int n>
    double norm(const vec<n>& v) {
        double s = 0;
        for (int i = 0; i < n; i++) {
            s += v[i] * v[i];
        }
        return std::sqrt(s);
    }

    template<int n>
    vec<n> normalize(const vec<n>& v) {
        double nrm = norm(v);
        vec<n> res;
        if (nrm > 1e-5) {
            for (int i = 0; i < n; i++) {
                res[i] = v[i] / nrm;
            }
        }
        return res;
    }

    template<int n>
    double dot(const vec<n>& v1, const vec<n>& v2) {
        double res = 0;
        for (int i = 0; i < n; i++) {
            res += v1[i] * v2[i];
        }
        return res;
    }

    inline vec3 cross(const vec3& v1, const vec3& v2) {
        vec3 result;
        result[0] = v1.y * v2.z - v1.z * v2.y;
        result[1] = v1.z * v2.x - v1.x * v2.z;
        result[2] = v1.x * v2.y - v1.y * v2.x;
        return result;
    }
    /* End Vector Implementation */


    /* Matrix Implementation */
    template<int R, int C> struct matrix {
        vec<C> rows[R];


        vec<C>& operator[](const int i) { assert(i>=0 && i<R); return rows[i]; }
        const vec<C>& operator[](const int i) const {assert(i>=0 && i<R); return rows[i]; }
        static matrix<R, C> identity();

        double det() const;
        matrix<R, C> inverse() const;

    private:
        double cofactor(int row, int col) const;
    };

    template<int R, int C>
    matrix<R-1, C-1> get_minor(const matrix<R, C>& m, int row, int col) {
        matrix<R-1, C-1> result;
        for (int i = 0; i < R-1; i++) {
            for (int j = 0; j < C-1; j++) {
                result[i][j] = m[i < row ? i : i + 1][j < col ? j : j + 1];
            }
        }
        return result;
    }

    template<int R, int C>
    double matrix<R, C>::cofactor(int row, int col) const {
        double sign = ((row + col) % 2 == 0) ? 1.0 : -1.0;
        return sign * get_minor(*this, row, col).det();
    }

    template<int R, int C>
    double matrix<R, C>::det() const {
        static_assert(R == C, "Matrix must be square for determinant");

        if constexpr (R == 1) {
            return rows[0][0];
        } else {
            double d = 0;
            for (int j = 0; j < C; j++) {
                d += rows[0][j] * cofactor(0, j);
            }
            return d;
        }
    }

    template<int R, int C>
    matrix<R, C> matrix<R, C>::inverse() const {
        static_assert(R == C, "Matrix must be square for inversion");
        double d = det();
        if (std::abs(d) < 1e-10) return identity();

        matrix<R, C> res;
        for (int i = 0; i < R; i++) {
            for (int j = 0; j < C; j++) {
                res[j][i] = cofactor(i, j) / d;
            }
        }
        return res;
    }

    template<int R, int C>
    matrix<R, C> matrix<R, C>::identity() {
        static_assert(R == C, "Identity matrix must be square (Rows == Cols)!");

        matrix<R, C> result;
        for (int i = 0; i < R; i++) {
            for (int j = 0; j < C; j++) {
                result[i][j] = (i == j ? 1.0 : 0.0);
            }
        }
        return result;
    }

    template<int R, int C>
    matrix<R, C> operator+(const matrix<R, C>& m1, const matrix<R, C>& m2) {
        matrix<R, C> result;
        for (int i = 0; i < R; i++) {
            for (int j = 0; j < C; j++) {
                result[i][j] = m1[i][j] + m2[i][j];
            }
        }
        return result;
    }

    template<int R, int C>
    matrix<R, C> operator-(const matrix<R, C>& m1, const matrix<R, C>& m2) {
        matrix<R, C> result;
        for (int i = 0; i < R; i++) {
            for (int j = 0; j < C; j++) {
                result[i][j] = m1[i][j] - m2[i][j];
            }
        }
        return result;
    }

    template<int R, int C>
    vec<R> operator*(const matrix<R, C>& m, const vec<C>& v) {
        vec<R> res, temp;
        for (int i = 0; i < R; i++) {
            res[i] = dot(m[i], v);
        }
        return res;
    }

    template<int R, int C>
    matrix<C, R> transpose(const matrix<R, C>& m) {
        matrix<C, R> result;
        for (int i = 0; i < R; i++) {
            for (int j = 0; j < C; j++) {
                result[j][i] = m[i][j];
            }
        }
        return result;
    }

    template<int R, int C>
    vec<C> operator*(const vec<R>& v, const matrix<R, C>& m) {
        return transpose(m) * v;
    }

    template<int R, int K, int C>
    matrix<R, C> operator*(const matrix<R, K>& m1, const matrix<K, C>& m2) {
        matrix<R, C> result;
        for (int i = 0; i < R; i++) {
            for (int j = 0; j < C; j++) {
                double sum = 0;
                for (int k = 0; k < K; k++) {
                    sum += m1[i][k] * m2[k][j];
                }
                result[i][j] = sum;
            }
        }
        return result;
    }
    /* End Matrix Implementation */
}