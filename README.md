# std-csv
You don't have time to read a description. You want your CSV now. C++17. Header-only. Read to arrays, tuples, vectors. Delimiter deduction (commas, spaces, tabs).

```cpp
#include "csv.h"

char* filename; // or std::string
auto data = csv::toVectors<float>(filename);
```
You're done.

Read to a vector of arrays:
```cpp
std::vector<std::array<float, 3>> data;
data = csv::toArrays<float, 3>(filename);
```

Read to a vector of tuples:
```cpp
std::vector<std::tuple<int, double, std::string>> data;
data = csv::toTuples<int, double, std::string>(filename);
```

Skip columns:
```cpp
std::vector<std::tuple<int, std::string>> data;
data = csv::toTuples<int, csv::IGNORE, std::string>(filename);
```

Get header:
```cpp
// full header
std::vector<std::string> fullHeader = csv::getHeader(filename);
// just the columns you want
std::vector<std::string> header;
csv::toTuples<int, csv::IGNORE, std::string>(filename, header);
```

Skip header:
```cpp
bool skipHeader = true;
auto data = csv::toVectors<unsigned long>(filename, skipHeader);
```
