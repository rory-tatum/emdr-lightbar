#include "unity.h"
#include "main.h"
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

void test_hello_message_returns_greeting(void) {
    TEST_ASSERT_EQUAL_STRING("Hello, World!", hello_message());
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_hello_message_returns_greeting);
    return UNITY_END();
}
