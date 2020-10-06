# std-csv
You don't have time to read a description. You want your CSV now. C++17. Header-only. Read to arrays, tuples, vectors. Delimiter deduction (commas, spaces, tabs).

```cpp
#include "csv.h"

char* filename; // or std::string
auto data = csv::toVectors<float>(filename);
```
You're done.

Read to a vector of vectors:
```cpp
auto data = csv::toVectors<float>(filename); // std::vector<std::vector<float>>
```

Read to a vector of arrays:
```cpp
auto data = csv::toArrays<float, 3>(filename); // std::vector<std::array<float, 3>>
```

Read to a vector of tuples:
```cpp
auto data = csv::toTuples<int, double, std::string>(filename); // std::vector<std::tuple<int, double, std::string>>
```

Skip columns:
```cpp
auto data = csv::toTuples<int, csv::IGNORE, std::string>(filename); // std::vector<std::tuple<int, std::string>>
```

Read the header:
```cpp
std::vector<std::string> header;

// read on its own
header = csv::getHeader(filename);

// read with the CSV
auto data = csv::toVectors<float>(filename, header);

// read just the columns you want
auto data = csv::toTuples<int, csv::IGNORE, std::string>(filename, header);
```
