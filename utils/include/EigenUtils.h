#pragma once
#include <Eigen/Eigen>

template <std::size_t rows, std::size_t cols, typename T1, typename... T>
static Eigen::Matrix<double, rows, cols> make_matrix(T1 const& first, T const&... other) {
	Eigen::Matrix<double, rows, cols> m;

	((m << first), ..., other);
	return m;
}

template <std::size_t rows, std::size_t cols, typename Scalar>
inline static Eigen::Matrix<Scalar, rows, cols> to_eigen_matrix(const std::vector<std::vector<Scalar>>& vectors) {
	Eigen::Matrix<Scalar, rows, cols> M(vectors.size(), vectors.front().size());
	for (size_t i = 0; i < vectors.size(); i++)
		for (size_t j = 0; j < vectors.front().size(); j++) M(i, j) = vectors[i][j];
	return M;
}

template <typename Scalar, typename Matrix>
inline static std::vector<std::vector<Scalar>> to_stl_matrix(const Matrix& M) {
	std::vector<std::vector<Scalar>> m;
	m.resize(M.rows(), std::vector<Scalar>(M.cols(), 0));
	for (size_t i = 0; i < m.size(); i++)
		for (size_t j = 0; j < m.front().size(); j++) m[i][j] = M(i, j);
	return m;
}