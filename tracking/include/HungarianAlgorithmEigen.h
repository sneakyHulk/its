/**
 * @class HungarianAlgorithmEigen
 *
 * @brief Implements the Hungarian Algorithm using the Eigen library for solving the assignment problem.
 *
 * This class is designed to solve the assignment problem, which is to find the minimum cost way of assigning tasks to agents.
 * It makes use of the Eigen library for matrix operations, providing an efficient implementation suitable for large scale problems.
 *
 * Usage:
 * Eigen::MatrixXd cost_matrix = ...; // Define the cost matrix
 * Eigen::VectorXi assignment;
 * HungarianAlgorithmEigen solver;
 * double cost = solver.SolveAssignmentProblem(cost_matrix, assignment);
 *
 * Note:
 * The algorithm assumes that the input cost matrix contains only non-negative values. It is not thread-safe and is designed for single-threaded environments.
 */

#pragma once

#include <Eigen/Dense>
#include <cfloat>
#include <iostream>

// The HungarianAlgorithmEigen class encapsulates the Hungarian Algorithm
// for solving the assignment problem, which finds the minimum cost matching
// between elements of two sets. It uses the Eigen library to handle matrix operations.
class HungarianAlgorithmEigen {
   public:
	// Constructor and destructor
	HungarianAlgorithmEigen() = default;
	~HungarianAlgorithmEigen() = default;

	// Solves the assignment problem given a distance matrix and returns the total cost.
	// The assignment of rows to columns is returned in the 'Assignment' vector.
	double solve_assignment_problem(Eigen::MatrixXd& dist_matrix, Eigen::VectorXi& assignment) {
		// Ensure that the distance matrix contains only non-negative values.
		if (dist_matrix.array().minCoeff() < 0) {
			std::cerr << "All matrix elements have to be non-negative." << std::endl;
		}

		// Copy the input distance matrix into the local member variable for manipulation.
		this->dist_matrix = dist_matrix;

		// Initialize helper arrays used in the algorithm, such as the star and prime matrices.
		init_helper_arrays(dist_matrix.rows(), dist_matrix.cols());

		// Execute the main steps of the Hungarian algorithm to find the optimal assignment.
		execute_hungarian_algorithm();

		// Set all elements of the assignment vector to a default value to indicate no assignment.
		assignment.setConstant(DEFAULT_ASSIGNMENT_VALUE);

		// Construct the assignment vector from the star matrix indicating the optimal assignment.
		construct_assignment_vector(assignment);

		// Calculate and return the total cost associated with the optimal assignment.
		return calculate_total_cost(assignment);
	}

   private:
	// Constants used as special values indicating not found or default assignments.
	const int NOT_FOUND_VALUE = -1;
	const int DEFAULT_ASSIGNMENT_VALUE = -1;

	// The distance matrix representing the cost of assignment between rows and columns.
	Eigen::MatrixXd dist_matrix;

	// Matrices and vectors used in the Hungarian algorithm
	Eigen::Array<bool, Eigen::Dynamic, Eigen::Dynamic> star_matrix, new_star_matrix, prime_matrix;
	Eigen::Array<bool, Eigen::Dynamic, 1> covered_columns, covered_rows;

	// Finds a starred zero in a column, if any.
	Eigen::Index find_star_in_column(int col) {
		for (Eigen::Index i = 0; i < star_matrix.rows(); ++i) {
			if (star_matrix(i, col)) {
				return i;  // Return the row index where the star was found.
			}
		}
		return NOT_FOUND_VALUE;
	}

	// Finds a primed zero in a row, if any.
	Eigen::Index find_prime_in_row(int row) {
		for (Eigen::Index j = 0; j < prime_matrix.cols(); ++j) {
			if (prime_matrix(row, j)) {
				return j;  // Return the column index where the prime was found.
			}
		}
		return NOT_FOUND_VALUE;
	}

	// Updates the star and prime matrices given a row and column.
	void update_star_and_prime_matrices(int row, int col) {
		// Make a working copy of star_matrix
		new_star_matrix = star_matrix;

		// Star the current zero.
		new_star_matrix(row, col) = true;

		// Loop to update stars and primes based on the newly starred zero.
		while (true) {
			// If there are no starred zeros in the current column, break the loop.
			if (!star_matrix.col(col).any()) {
				break;
			}

			// Find the starred zero in the column and unstar it.
			Eigen::Index const star_row = find_star_in_column(col);
			new_star_matrix(star_row, col) = false;

			// If there are no primed zeros in the row of the starred zero, break the loop.
			if (!prime_matrix.row(star_row).any()) {
				break;
			}

			// Find the primed zero in the row and star it.
			Eigen::Index const prime_col = find_prime_in_row(star_row);
			new_star_matrix(star_row, prime_col) = true;

			// Move to the column of the newly starred zero.
			col = prime_col;
		}

		// Apply the changes from the working copy to the actual star_matrix.
		star_matrix = new_star_matrix;
		// Clear the prime_matrix and covered rows for the next steps.
		prime_matrix.setConstant(false);
		covered_rows.setConstant(false);
	}

	// Reduces the matrix by subtracting the minimum value from each uncovered row
	// and adding it to each covered column, thus creating at least one new zero.
	void reduce_matrix_by_minimum_value() {
		// Determine the dimensions for operations.
		int num_rows = covered_rows.size();
		int num_columns = covered_columns.size();

		// Create a masked array with high values for covered rows/columns.
		Eigen::ArrayXXd masked_array = dist_matrix.array() + DBL_MAX * (covered_rows.replicate(1, num_columns).cast<double>() + covered_columns.transpose().replicate(num_rows, 1).cast<double>());

		// Find the minimum value in the uncovered elements.
		double min_uncovered_value = masked_array.minCoeff();

		// Adjust the matrix values based on uncovered rows and columns.
		Eigen::ArrayXXd row_adjustments = covered_rows.cast<double>() * min_uncovered_value;
		Eigen::ArrayXXd col_adjustments = (1.0 - covered_columns.cast<double>()) * min_uncovered_value;
		dist_matrix += (row_adjustments.replicate(1, num_columns) - col_adjustments.transpose().replicate(num_rows, 1)).matrix();
	}

	// Covers columns without a starred zero and potentially modifies star/prime matrices.
	void cover_columns_lacking_stars() {
		// Retrieve the dimensions of the matrix.
		int num_rows = dist_matrix.rows();
		int num_columns = dist_matrix.cols();

		// Flag to check if uncovered zeros are found in the iteration.
		bool zeros_found = true;
		while (zeros_found) {
			zeros_found = false;
			for (int col = 0; col < num_columns; col++) {
				// Skip already covered columns.
				if (covered_columns(col)) continue;

				// Identify uncovered zeros in the current column.
				Eigen::Array<bool, Eigen::Dynamic, 1> uncovered_zeros_in_column = (dist_matrix.col(col).array().abs() < DBL_EPSILON) && !covered_rows;

				Eigen::Index row;
				// Check if there is an uncovered zero in the column.
				double max_in_uncovered_zeros = uncovered_zeros_in_column.cast<double>().maxCoeff(&row);

				// If an uncovered zero is found, prime it.
				if (max_in_uncovered_zeros == 1.0) {
					prime_matrix(row, col) = true;

					// Check for a star in the same row.
					Eigen::Index star_col;
					bool has_star = star_matrix.row(row).maxCoeff(&star_col);
					if (!has_star) {
						// If no star is found, update the star and prime matrices, and covered columns.
						update_star_and_prime_matrices(row, col);
						covered_columns = (star_matrix.colwise().any()).transpose();
						return;
					} else {
						// If a star is found, cover the row and uncover the column where the star is found.
						covered_rows(row) = true;
						covered_columns(star_col) = false;
						zeros_found = true;  // Continue the while loop.
						break;
					}
				}
			}
		}

		// If no more uncovered zeros are found, reduce the matrix and try again.
		reduce_matrix_by_minimum_value();
		cover_columns_lacking_stars();
	}

	// Executes the steps of the Hungarian algorithm.
	void execute_hungarian_algorithm() {
		// If there are fewer rows than columns, we operate on rows first.
		if (dist_matrix.rows() <= dist_matrix.cols()) {
			for (int row = 0; row < dist_matrix.rows(); ++row) {
				// Subtract the minimum value in the row to create zeros.
				double min_value = dist_matrix.row(row).minCoeff();
				dist_matrix.row(row).array() -= min_value;

				// Identify zeros that are not covered by any line (row or column).
				Eigen::ArrayXd current_row = dist_matrix.row(row).array();
				Eigen::Array<bool, Eigen::Dynamic, 1> uncovered_zeros = (current_row.abs() < DBL_EPSILON) && !covered_columns;

				Eigen::Index col;
				// Star a zero if it is the only uncovered zero in its row.
				double max_in_uncovered_zeros = uncovered_zeros.cast<double>().maxCoeff(&col);
				if (max_in_uncovered_zeros == 1.0) {
					star_matrix(row, col) = true;
					covered_columns(col) = true;  // Cover the column containing the starred zero.
				}
			}
		} else  // If there are more rows than columns, we operate on columns first.
		{
			for (int col = 0; col < dist_matrix.cols(); ++col) {
				// Subtract the minimum value in the column to create zeros.
				double min_value = dist_matrix.col(col).minCoeff();
				dist_matrix.col(col).array() -= min_value;

				// Identify zeros that are not covered by any line.
				Eigen::ArrayXd current_column = dist_matrix.col(col).array();
				Eigen::Array<bool, Eigen::Dynamic, 1> uncovered_zeros = (current_column.abs() < DBL_EPSILON) && !covered_rows;

				Eigen::Index row;
				// Star a zero if it is the only uncovered zero in its column.
				double max_in_uncovered_zeros = uncovered_zeros.cast<double>().maxCoeff(&row);
				if (max_in_uncovered_zeros == 1.0) {
					star_matrix(row, col) = true;
					covered_columns(col) = true;
					covered_rows(row) = true;  // Temporarily cover the row to avoid multiple stars in one row.
				}
			}

			// Uncover all rows for the next step.
			for (int row = 0; row < dist_matrix.rows(); ++row) {
				covered_rows(row) = false;
			}
		}

		// If not all columns are covered, move to the next step to cover them.
		if (covered_columns.count() != std::min(dist_matrix.rows(), dist_matrix.cols())) {
			cover_columns_lacking_stars();
		}
	}

	// Initializes helper arrays used by the algorithm.
	void init_helper_arrays(int num_rows, int num_columns) {
		covered_columns = Eigen::Array<bool, Eigen::Dynamic, 1>::Constant(num_columns, false);
		covered_rows = Eigen::Array<bool, Eigen::Dynamic, 1>::Constant(num_rows, false);
		star_matrix = Eigen::Array<bool, Eigen::Dynamic, Eigen::Dynamic>::Constant(num_rows, num_columns, false);
		new_star_matrix = Eigen::Array<bool, Eigen::Dynamic, Eigen::Dynamic>::Constant(num_rows, num_columns, false);
		prime_matrix = Eigen::Array<bool, Eigen::Dynamic, Eigen::Dynamic>::Constant(num_rows, num_columns, false);
	}

	// Constructs the assignment vector from the star matrix.
	void construct_assignment_vector(Eigen::VectorXi& assignment) {
		// Iterate over each row to determine the assignment for that row.
		for (int row = 0; row < star_matrix.rows(); ++row) {
			Eigen::Index col;
			// Check if there is a starred zero in the current row.
			if (bool has_star = star_matrix.row(row).maxCoeff(&col)) {
				// If there is a starred zero, assign the corresponding column to this row.
				assignment[row] = col;
			} else {
				// If there is no starred zero, indicate that the row is not assigned.
				assignment[row] = DEFAULT_ASSIGNMENT_VALUE;
			}
		}
	}

	// Calculates the total cost of the assignment.
	double calculate_total_cost(Eigen::VectorXi& assignment) {
		double total_cost = 0.0;  // Initialize the total cost to zero.

		// Iterate over each assignment to calculate the total cost.
		for (int row = 0; row < dist_matrix.rows(); ++row) {
			// Check if the current row has a valid assignment.
			if (assignment(row) >= 0) {
				// Add the cost of the assigned column in the current row to the total cost.
				total_cost += dist_matrix(row, assignment(row));
			}
			// Note: If the assignment is not valid (indicated by DEFAULT_ASSIGNMENT_VALUE),
			// it is not included in the total cost calculation.
		}

		return total_cost;  // Return the calculated total cost.
	}
};