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