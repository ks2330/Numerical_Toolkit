#include <gtest/gtest.h>
#include <Eigen/Dense>

TEST(MatrixTest, Addition) {
    Eigen::Matrix2d m;
    m << 1, 2, 
         3, 4;

    Eigen::Matrix2d expected;
    expected << 2, 4, 
                6, 8;

    // Test summing m with itself using isApprox for safe floating-point comparison
    EXPECT_TRUE((m + m).isApprox(expected)) << "m + m did not match expected results.";
}


TEST(MatrixTest, Multiplication) {
    Eigen::Matrix2d m;
    m << 1, 2, 
         3, 4;

    Eigen::Matrix2d expected;
    expected << 7, 10, 
                15, 22;

    // Test multiplying m by itself using isApprox for safe floating-point comparison
    EXPECT_TRUE((m * m).isApprox(expected)) << "m * m did not match expected results.";
}

TEST(MatrixTest, Transpose) {
    Eigen::Matrix2d m;
    m << 1, 2, 
         3, 4;

    Eigen::Matrix2d expected;
    expected << 1, 3, 
                2, 4;
    // Test transposing m using isApprox for safe floating-point comparison
    EXPECT_TRUE(m.transpose().isApprox(expected)) << "m.transpose() did not match expected results.";
}

TEST(MatrixTest, Transpose_Multiplication) {
    Eigen::Matrix2d m;
    m << 1, 2, 
         3, 4;

    Eigen::Vector2d u(2,0);

    Eigen::RowVector2d expected;
    expected << 2, 4;

    EXPECT_TRUE((u.transpose() * m).isApprox(expected)) << "u.transpose() * m did not match expected results.";
}