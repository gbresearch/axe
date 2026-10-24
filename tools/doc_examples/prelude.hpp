// Supporting includes and namespace imports only; all example code is extracted
// from the current guide by check_docs.py, never duplicated in this fixture.
#include <axe.h>
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <map>
#include <numeric>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>
using namespace axe;
using namespace axe::shortcuts;
using namespace std::string_literals;
inline void require(bool matched) { if (!matched) std::abort(); }
