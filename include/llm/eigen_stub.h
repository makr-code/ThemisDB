/**
 * @file eigen_stub.h
 * @brief Eigen matrix-library stub for environments without Eigen.
 *
 * Provides minimal type aliases and no-op implementations so that
 * translation units that optionally use Eigen can compile cleanly
 * when the library is not available.
 */

// Lightweight Eigen stub used when Eigen headers are unavailable on the build host.
#pragma once

#include <vector>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <random>

namespace Eigen {

    class VectorXf; // forward declaration for RowRef::transpose

    // Float matrix (minimal subset used in ThemisDB tests)
    class MatrixXf {
    public:
        MatrixXf() : rows_(0), cols_(0) {}
        MatrixXf(int r, int c) { resize(r, c); }
        void resize(int r, int c) { rows_ = r; cols_ = c; data_.assign((size_t)r * c, 0.0f); }
        void setZero() { std::fill(data_.begin(), data_.end(), 0.0f); }
        int rows() const { return rows_; }
        int cols() const { return cols_; }
        float& operator()(int i, int j) { return data_[(size_t)i * cols_ + j]; }
        const float& operator()(int i, int j) const { return data_[(size_t)i * cols_ + j]; }

        bool isZero() const {
            for (const auto& v : data_) if (v != 0.0f) return false;
            return true;
        }

        static MatrixXf Zero(int r, int c) { MatrixXf m(r, c); m.setZero(); return m; }
        static MatrixXf Random(int r, int c) {
            MatrixXf m(r, c);
            std::mt19937 rng(42);
            std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
            for (auto &v : m.data_) v = dist(rng);
            return m;
        }

        // Row proxy to support .row(i).transpose()
        struct RowRef {
            MatrixXf& parent;
            int r;
            VectorXf transpose() const;
        };

        RowRef row(int r) { return RowRef{*this, r}; }

        MatrixXf transpose() const {
            MatrixXf out(cols_, rows_);
            for (int i = 0; i < rows_; ++i)
                for (int j = 0; j < cols_; ++j)
                    out(j, i) = (*this)(i, j);
            return out;
        }

        std::vector<float> data_;
    private:
        int rows_ = 0;
        int cols_ = 0;
    };

    class VectorXf {
    public:
        VectorXf() : n_(0) {}
        VectorXf(int n) : n_(n), data_(n, 0.0f) {}
        void resize(int n) { n_ = n; data_.assign(n, 0.0f); }
        int size() const { return n_; }
        float& operator()(int i) { return data_[(size_t)i]; }
        const float& operator()(int i) const { return data_[(size_t)i]; }
        float sum() const { float s = 0.0f; for (auto v : data_) s += v; return s; }
        void setZero() { std::fill(data_.begin(), data_.end(), 0.0f); }
        static VectorXf Random(int n) {
            VectorXf v(n);
            std::mt19937 rng(42);
            std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
            for (auto &x : v.data_) x = dist(rng);
            return v;
        }
        std::vector<float> data_;
    private:
        int n_ = 0;
    };

    inline VectorXf MatrixXf::RowRef::transpose() const {
        VectorXf v(parent.cols());
        for (int j = 0; j < parent.cols(); ++j) v(j) = parent(r, j);
        return v;
    }

    // Matrix-vector multiplication
    inline VectorXf operator*(const MatrixXf& m, const VectorXf& v) {
        VectorXf out(m.rows());
        for (int i = 0; i < m.rows(); ++i) {
            float s = 0.0f;
            for (int j = 0; j < m.cols(); ++j) s += m(i, j) * v(j);
            out(i) = s;
        }
        return out;
    }

    // Matrix-Matrix multiplication
    inline MatrixXf operator*(const MatrixXf& a, const MatrixXf& b) {
        MatrixXf out(a.rows(), b.cols());
        out.setZero();
        for (int i = 0; i < a.rows(); ++i)
            for (int k = 0; k < a.cols(); ++k)
                for (int j = 0; j < b.cols(); ++j)
                    out(i, j) += a(i, k) * b(k, j);
        return out;
    }

    // Double precision counterparts
    class MatrixXd {
    public:
        MatrixXd() : rows_(0), cols_(0) {}
        MatrixXd(int r, int c) { resize(r, c); }
        void resize(int r, int c) { rows_ = r; cols_ = c; data_.assign((size_t)r * c, 0.0); }
        void setZero() { std::fill(data_.begin(), data_.end(), 0.0); }
        int rows() const { return rows_; }
        int cols() const { return cols_; }
        double& operator()(int i, int j) { return data_[(size_t)i * cols_ + j]; }
        const double& operator()(int i, int j) const { return data_[(size_t)i * cols_ + j]; }
        bool isZero() const {
            for (const auto& v : data_) if (v != 0.0) return false;
            return true;
        }

        static MatrixXd Zero(int r, int c) { MatrixXd m(r, c); m.setZero(); return m; }
        static MatrixXd Random(int r, int c) {
            MatrixXd m(r, c);
            std::mt19937 rng(42);
            std::uniform_real_distribution<double> dist(-1.0, 1.0);
            for (auto &v : m.data_) v = dist(rng);
            return m;
        }

        struct RowRef {
            MatrixXd& parent;
            int r;
            std::vector<double> transpose() const {
                std::vector<double> out(parent.cols());
                for (int j = 0; j < parent.cols(); ++j) out[j] = parent(r, j);
                return out;
            }
        };

        RowRef row(int r) { return RowRef{*this, r}; }

        MatrixXd transpose() const {
            MatrixXd out(cols_, rows_);
            for (int i = 0; i < rows_; ++i)
                for (int j = 0; j < cols_; ++j)
                    out(j, i) = (*this)(i, j);
            return out;
        }
        std::vector<double> data_;
    private:
        int rows_ = 0;
        int cols_ = 0;
    };

    // simple VectorXd
    class VectorXd {
    public:
        VectorXd() : n_(0) {}
        VectorXd(int n) : n_(n), data_(n, 0.0) {}
        void resize(int n) { n_ = n; data_.assign(n, 0.0); }
        int size() const { return n_; }
        double& operator()(int i) { return data_[(size_t)i]; }
        const double& operator()(int i) const { return data_[(size_t)i]; }
        double sum() const { double s = 0.0; for (auto v : data_) s += v; return s; }
        void setZero() { std::fill(data_.begin(), data_.end(), 0.0); }
        std::vector<double> data_;
    private:
        int n_ = 0;
    };

    // MatrixXd * VectorXd
    inline VectorXd operator*(const MatrixXd& m, const VectorXd& v) {
        VectorXd out(m.rows());
        for (int i = 0; i < m.rows(); ++i) {
            double s = 0.0;
            for (int j = 0; j < m.cols(); ++j) s += m(i, j) * v(j);
            out(i) = s;
        }
        return out;
    }

    // Map emulation for assignment
    template <typename T>
    class Map {
    public:
        Map(T* data, int r, int c) : ptr_(data), r_(r), c_(c) {}
        Map& operator=(const T& other) {
            for (int i = 0; i < r_; ++i)
                for (int j = 0; j < c_; ++j)
                    ptr_[i * c_ + j] = other(i, j);
            return *this;
        }
    private:
        T* ptr_;
        int r_, c_;
    };

} // namespace Eigen
