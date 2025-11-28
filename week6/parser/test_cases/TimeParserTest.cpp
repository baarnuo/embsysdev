#include <gtest/gtest.h>
#include "../TimeParser.h"

// Test suite: TimeParserTest
TEST(TimeParserTest, TestCaseCorrectTime) {
    char time_test[] = "000001";
    EXPECT_EQ(time_parse(time_test), 1);

    char time_test2[] = "101010";
    EXPECT_EQ(time_parse(time_test2), 10*3600 + 10*60 + 10);
}

TEST(TimeParserTest, TestCaseIncorrectTime) {
    char time_test[] = "000099";
    EXPECT_EQ(time_parse(time_test), TIME_VALUE_ERROR);

    char time_test2[] = "999999";
    EXPECT_EQ(time_parse(time_test2), TIME_VALUE_ERROR);
}

TEST(TimeParserTest, TestCaseTimeLength) {
    char time_test[] = "00001";
    EXPECT_EQ(time_parse(time_test), TIME_LEN_ERROR);

    char time_test2[] = "0000001";
    EXPECT_EQ(time_parse(time_test2), TIME_LEN_ERROR);
}

TEST(TimeParserTest, TestCaseIsTimeNull) {
    char *time_test = NULL;
    EXPECT_EQ(time_parse(time_test), TIME_NULL_ERROR);
}

TEST(TimeParserTest, TestCaseIsTimeZero) {
    char time_test[] = "000000";
    EXPECT_EQ(time_parse(time_test), TIME_VALUE_ERROR);
}
