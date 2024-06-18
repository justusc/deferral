#include "deferral.hh"

#include <gtest/gtest.h>

class DeferralTest : public ::testing::Test {
protected:
  DeferralTest() {}
  virtual ~DeferralTest() {}
  virtual void SetUp() override {}
  virtual void TearDown() override {}
};

TEST_F(DeferralTest, TestExit) {
  int x = 0;
  {
    defer { x = 1; };
    EXPECT_EQ(x, 0);
  }
  EXPECT_EQ(x, 1);
}

TEST_F(DeferralTest, TestExitThrow) {
  int x = 0;
  try {
    defer { x = 1; };
    EXPECT_EQ(x, 0);
    throw 0;
  } catch(...) {}
  EXPECT_EQ(x, 1);
}

TEST_F(DeferralTest, TestExitRelease) {
  int x = 0;
  {
    defer_(d) { x = 1; };
    d.release();
    EXPECT_EQ(x, 0);
  }

  EXPECT_EQ(x, 0);

  {
    defer_(d) { x = 1; };
    EXPECT_EQ(x, 0);
  }

  EXPECT_EQ(x, 1);
}

TEST_F(DeferralTest, TestSuccessFail) {
  int x = 0;
  int y = 0;
  {
    defer_success { x = 1; };
    defer_fail { y = 1; };
    EXPECT_EQ(x, 0);
    EXPECT_EQ(y, 0);
  }
  EXPECT_EQ(x, 1);
  EXPECT_EQ(y, 0);
}

TEST_F(DeferralTest, TestSuccessFailThrow) {
  int x = 0;
  int y = 0;
  try {
    defer_success { x = 1; };
    defer_fail { y = 1; };
    EXPECT_EQ(x, 0);
    EXPECT_EQ(y, 0);
    throw 0;
  } catch(...) {}
  EXPECT_EQ(x, 0);
  EXPECT_EQ(y, 1);
}

TEST_F(DeferralTest, TestSuccessFailRelease) {
  int x = 0;
  int y = 0;
  {
    defer_success_(d) { x = 1; };
    defer_fail_(e) { y = 1; };

    d.release();
    e.release();
    EXPECT_EQ(x, 0);
    EXPECT_EQ(y, 0);
  }

  EXPECT_EQ(x, 0);
  EXPECT_EQ(y, 0);

  {
    defer_success_(d) { x = 1; };
    defer_fail_(e) { y = 1; };
    EXPECT_EQ(x, 0);
    EXPECT_EQ(y, 0);
  }

  EXPECT_EQ(x, 1);
  EXPECT_EQ(y, 0);
}
TEST_F(DeferralTest, TestSuccessFailReleaseThrow) {
  int x = 0;
  int y = 0;
  {
    defer_success_(d) { x = 1; };
    defer_fail_(e) { y = 1; };
    d.release();
    e.release();
    EXPECT_EQ(x, 0);
    EXPECT_EQ(y, 0);
  }

  EXPECT_EQ(x, 0);
  EXPECT_EQ(y, 0);

  try {
    defer_success_(d) { x = 1; };
    defer_fail_(e) { y = 1; };
    throw 0;
    EXPECT_EQ(x, 0);
    EXPECT_EQ(y, 0);
  } catch(...) {}

  EXPECT_EQ(x, 0);
  EXPECT_EQ(y, 1);
}

#if __cplusplus >= 201703L

TEST_F(DeferralTest, TestTypeDeductionGuides) {
  using namespace deferral;

  int x = 0;
  int y = 0;
  int z = 0;
  {
    auto d = DeferExit{[&]() { x = 1; }};
    auto f = DeferFail{[&]() { y = 1; }};
    auto s = DeferSuccess{[&]() { z = 1; }};
    EXPECT_EQ(x, 0);
    EXPECT_EQ(y, 0);
    EXPECT_EQ(z, 0);
  }

  EXPECT_EQ(x, 1);
  EXPECT_EQ(y, 0);
  EXPECT_EQ(z, 1);
}

TEST_F(DeferralTest, TestTypeDeductionGuidesWithThrow) {
  using namespace deferral;

  int x = 0;
  int y = 0;
  int z = 0;
  try {
    auto d = DeferExit{[&]() { x = 1; }};
    auto f = DeferFail{[&]() { y = 1; }};
    auto s = DeferSuccess{[&]() { z = 1; }};

    throw 0;
    EXPECT_EQ(x, 0);
    EXPECT_EQ(y, 0);
    EXPECT_EQ(z, 0);
  } catch(...) {}

  EXPECT_EQ(x, 1);
  EXPECT_EQ(y, 1);
  EXPECT_EQ(z, 0);
}

#endif // __cplusplus >= 201703L

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
