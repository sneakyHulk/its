#pragma once
#include <Eigen/Eigen>
#include <concepts>

template <std::size_t rows, std::size_t cols, typename T1, typename... T>
static Eigen::Matrix<double, rows, cols> make_matrix(T1 const& first, T const&... other) {
	Eigen::Matrix<double, rows, cols> m;

	((m << first), ..., other);
	return m;
}

namespace Eigen {
	template <typename scalar, int... args>
	void to_json(nlohmann::json& j, Matrix<scalar, args...> const& matrix) {
		for (int row = 0; row < matrix.rows(); ++row) {
			nlohmann::json column = nlohmann::json::array();
			for (int col = 0; col < matrix.cols(); ++col) {
				column.push_back(matrix(row, col));
			}
			j.push_back(column);
		}
	}

	template <typename scalar, int... args>
	void from_json(nlohmann::json const& j, Matrix<scalar, args...>& matrix) {
		assert(j.size() == matrix.rows());
		for (std::size_t row = 0; row < j.size(); ++row) {
			const auto& jrow = j.at(row);
			assert(jrow.size() == matrix.cols());
			for (std::size_t col = 0; col < jrow.size(); ++col) {
				const auto& value = jrow.at(col);
				matrix(row, col) = value.get<scalar>();
			}
		}
	}
}  // namespace Eigen

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