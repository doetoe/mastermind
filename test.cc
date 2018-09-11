#include <algorithm>
#include <set>
using namespace std;

template<typename T>
struct Counter {
  using value_type = T;
  void push_back(const T&) {++count;}
  size_t count = 0;
};

template<typename S>
size_t intersection_size(const S& s1, const S& s2)
{
  Counter<typename S::value_type> c;
  set_intersection(s1.begin(), s1.end(), s2.begin(), s2.end(),
                   back_inserter(c));
  return c.count;
}

int main() {
  set<int> s1 = {1,3,6,5,3,2,5,6,6};
  set<int> s2 = {4,3,3,5};
  return intersection_size(s1, s2);
}
