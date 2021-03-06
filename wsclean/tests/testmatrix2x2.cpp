#include <boost/test/unit_test.hpp>

#include "../matrix2x2.h"

#include <iostream>

BOOST_AUTO_TEST_SUITE(matrix2x2)

BOOST_AUTO_TEST_CASE( eigenvalue1 )
{
	double unit[4] = { 1.0, 0.0, 0.0, 1.0 };
	double e1, e2;
	Matrix2x2::EigenValues(unit, e1, e2);
	BOOST_CHECK_CLOSE(e1, 1.0, 1e-6);
	BOOST_CHECK_CLOSE(e2, 1.0, 1e-6);
}

BOOST_AUTO_TEST_CASE( eigenvalue2 )
{
	double unit[4] = { 0.0, 1.0, -2.0, -3.0 };
	double e1, e2;
	Matrix2x2::EigenValues(unit, e1, e2);
	if(e1 < e2) std::swap(e1, e2);
	BOOST_CHECK_CLOSE(e1, -1.0, 1e-6);
	BOOST_CHECK_CLOSE(e2, -2.0, 1e-6);
}

BOOST_AUTO_TEST_CASE( eigenvalue3 )
{
	double unit[4] = { 0.0, -2.0, 1.0, -3.0 };
	double e1, e2;
	Matrix2x2::EigenValues(unit, e1, e2);
	if(e1 < e2) std::swap(e1, e2);
	BOOST_CHECK_CLOSE(e1, -1.0, 1e-6);
	BOOST_CHECK_CLOSE(e2, -2.0, 1e-6);
}

BOOST_AUTO_TEST_CASE( eigenvalue4 )
{
	double unit[4] = { 0.0, 1.0, -1.0, 0.0 };
	double e1, e2;
	Matrix2x2::EigenValues(unit, e1, e2);
	if(e1 < e2) std::swap(e1, e2);
	BOOST_CHECK(!std::isfinite(e1));
	BOOST_CHECK(!std::isfinite(e2));
}

BOOST_AUTO_TEST_CASE( eigenvector2 )
{
	double unit[4] = { 0.0, 1.0, -2.0, -3.0 };
	double e1, e2, vec1[2], vec2[2];
	Matrix2x2::EigenValuesAndVectors(unit, e1, e2, vec1, vec2);
	if(e1 < e2) {
		std::swap(e1, e2);
		std::swap(vec1, vec2);
	}
	BOOST_CHECK_CLOSE(e1, -1.0, 1e-6);
	BOOST_CHECK_CLOSE(vec1[0]/vec1[1], -1.0, 1e-6); // vec1 = c [-1, 1]
	BOOST_CHECK_CLOSE(e2, -2.0, 1e-6);
	BOOST_CHECK_CLOSE(vec2[0]/vec2[1], -0.5, 1e-6); // vec2 = c [-1, 2]
}

BOOST_AUTO_TEST_CASE( eigenvector3 )
{
	double unit[4] = { 0.0, -2.0, 1.0, -3.0 };
	double e1, e2, vec1[2], vec2[2];
	Matrix2x2::EigenValuesAndVectors(unit, e1, e2, vec1, vec2);
	if(e1 < e2) {
		std::swap(e1, e2);
		std::swap(vec1, vec2);
	}
	BOOST_CHECK_CLOSE(e1, -1.0, 1e-6);
	BOOST_CHECK_CLOSE(vec1[0]/vec1[1], 2.0, 1e-6); // vec1 = c [2, 1]
	BOOST_CHECK_CLOSE(e2, -2.0, 1e-6);
	BOOST_CHECK_CLOSE(vec2[0]/vec2[1], 1.0, 1e-6); // vec2 = c [1, 1]
}

BOOST_AUTO_TEST_CASE( eigenvector4 )
{
	double unit[4] = { 1.0, 2.0, 3.0, -4.0 };
	double e1, e2, vec1[2], vec2[2];
	Matrix2x2::EigenValuesAndVectors(unit, e1, e2, vec1, vec2);
	if(e1 < e2) {
		std::swap(e1, e2);
		std::swap(vec1, vec2);
	}
	BOOST_CHECK_CLOSE(e1, 2.0, 1e-6);
	BOOST_CHECK_CLOSE(vec1[0]/vec1[1], 2.0, 1e-6); // vec1 = c [2, 1]
	BOOST_CHECK_CLOSE(e2, -5.0, 1e-6);
	BOOST_CHECK_CLOSE(vec2[1]/vec2[0], -3.0, 1e-6); // vec2 = c [-2, 6]
}

BOOST_AUTO_TEST_CASE( eigenvector5 )
{
	double m[4] = { 1.0, 0.0, 0.0, 0.5 };
	double e1, e2, vec1[2], vec2[2];
	Matrix2x2::EigenValuesAndVectors(m, e1, e2, vec1, vec2);
	if(e1 < e2) {
		std::swap(e1, e2);
		std::swap(vec1, vec2);
	}
	BOOST_CHECK_CLOSE(e1, 1.0, 1e-6);
	BOOST_CHECK_CLOSE(vec1[1]/vec1[0], 0.0, 1e-6);
	BOOST_CHECK_CLOSE(e2, 0.5, 1e-6);
	BOOST_CHECK_CLOSE(vec2[0]/vec2[1], 0.0, 1e-6);
}

BOOST_AUTO_TEST_CASE( cholesky_real )
{
	std::complex<double> matrix[4] = {1., 2., 2., 13.};
	BOOST_CHECK(Matrix2x2::Cholesky(matrix));
	std::complex<double> answer[4] = {1., 0., 2., 3.};
	for(size_t i=0; i!=4; ++i) {
		BOOST_CHECK_CLOSE(matrix[i].real(), answer[i].real(), 1e-6);
		BOOST_CHECK_CLOSE(matrix[i].imag(), answer[i].imag(), 1e-6);
	}
}

BOOST_AUTO_TEST_CASE( cholesky_complex )
{
	std::complex<double> matrix[4] = {{1., 0.}, {2., -5.}, {2., 5.}, {38., 0.}};
	BOOST_CHECK(Matrix2x2::Cholesky(matrix));
	std::complex<double> answer[4] = {{1., 0.}, {0., 0.}, {2., 5.}, {3., 0.}};

	for(size_t i=0; i!=4; ++i) {
		BOOST_CHECK_CLOSE(matrix[i].real(), answer[i].real(), 1e-6);
		BOOST_CHECK_CLOSE(matrix[i].imag(), answer[i].imag(), 1e-6);
	}
}

BOOST_AUTO_TEST_CASE( cholesky_not_positive )
{
	std::complex<double> diag_not_positive[4] = {{0., 0.}, {0., 0.}, {0., 0.}, {1., 0.}}; // diagonal not positive
	BOOST_CHECK(!Matrix2x2::CheckedCholesky(diag_not_positive));
	std::complex<double> diag_not_real[4] = {{1., 0.}, {0., 0.}, {0., 0.}, {1., 1.}}; // diagonal not real
	BOOST_CHECK(!Matrix2x2::CheckedCholesky(diag_not_real));
	std::complex<double> not_hermitian[4] = {{1., 0.}, {1., 0.}, {2., 0.}, {1., 0.}}; // not hermitian
	BOOST_CHECK(!Matrix2x2::CheckedCholesky(not_hermitian));
}

BOOST_AUTO_TEST_CASE( evdecomposition )
{
	MC2x2 a(1, 2, 3, 4), b(5, 6, 7, 8);
	MC2x2 jones = a.MultiplyHerm(b) + b.MultiplyHerm(a);
	MC2x2 r = jones;
	r *= r.HermTranspose();
	std::complex<double> e1, e2, vec1[2], vec2[2];
	Matrix2x2::EigenValuesAndVectors(r.Data(), e1, e2, vec1, vec2);
	double v1norm = std::norm(vec1[0]) + std::norm(vec1[1]);
	vec1[0] /= sqrt(v1norm); vec1[1] /= sqrt(v1norm);
	double v2norm = std::norm(vec2[0]) + std::norm(vec2[1]);
	vec2[0] /= sqrt(v2norm); vec2[1] /= sqrt(v2norm);
	
	MC2x2 u(vec1[0], vec2[0], vec1[1], vec2[1]), e(e1, 0, 0, e2);
	MC2x2 res = u.Multiply(e).MultiplyHerm(u);
	for(size_t i=0; i!=4; ++i)
		BOOST_CHECK_CLOSE(res[i].real(), r[i].real(), 1e-6);

	MC2x2 decomposed = r.DecomposeHermitianEigenvalue();
	decomposed *= decomposed.HermTranspose();
	for(size_t i=0; i!=4; ++i)
		BOOST_CHECK_CLOSE(decomposed[i].real(), r[i].real(), 1e-6);
}

BOOST_AUTO_TEST_SUITE_END()
