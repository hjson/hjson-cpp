#include <cstdio>


void test_value();
void test_marshal();


int main() {
  char tmp[1];
  test_value();
  test_marshal();

  tmp[1000] = 'a';
  printf("%s\n", tmp);

  return 0;
}
