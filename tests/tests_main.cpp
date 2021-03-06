//
// Created by Matthew Kang on 2020-09-01.
//

#define CATCH_CONFIG_MAIN

#include "csv.hpp"

#include "catch.hpp"
#include <filesystem>

const auto dataDir = std::filesystem::path(STDCSV_PROJECT_DIR) / "data";


TEST_CASE("Read tuples with ignored columns")
{
    const auto dataPath = dataDir / "test.csv";
    assert(std::filesystem::exists(dataPath));

    auto tups = csv::to_tuples<int, float, bool, csv::ignore, csv::ignore, int>(dataPath.string());
    REQUIRE(!tups.empty());

    const auto& t = tups.front();
    CHECK(std::get<0>(t) == 1);
    CHECK(std::get<2>(t) == true);
    CHECK(std::get<3>(t) == -6);
}

TEST_CASE("Read header")
{
    const auto dataPath = dataDir / "test_int_with_header.csv";
    assert(std::filesystem::exists(dataPath));

    auto header = csv::get_header(dataPath.string());
    CHECK(header == std::vector<std::string>{"Index", "Age", "Score"});
}

TEST_CASE("Read arrays")
{
    const auto dataPath = dataDir / "test_float.csv";
    assert(std::filesystem::exists(dataPath));

    auto arr = csv::to_arrays<double, 4>(dataPath.string());
    REQUIRE(!arr.empty());

    CHECK(arr == std::vector<std::array<double, 4>>{
            {3.3l, 25.0, 1.4738, 4789.1},
            {4.18374, 0.48734, 47839247.471890234, 0},
            {9.324123, 3.14159, 6.677, 1234.567890}
    });
}

TEST_CASE("Read arrays with header")
{
    const auto dataPath = dataDir / "test_int_with_header.csv";
    assert(std::filesystem::exists(dataPath));

    using Header = std::array<std::string, 3>;
    Header header;
    auto arr = csv::to_arrays<int, 3>(dataPath.string(), header);
    REQUIRE(!arr.empty());

    CHECK(header == Header{"Index", "Age", "Score"});
}

TEST_CASE("Read vectors")
{
    const auto dataPath = dataDir / "test_int_with_header.csv";
    assert(std::filesystem::exists(dataPath));

    using Header = std::vector<std::string>;
    Header header;
    auto arr = csv::to_vectors<int>(dataPath.string(), header);
    REQUIRE(!arr.empty());
    REQUIRE(!header.empty());

    CHECK(header == Header{"Index", "Age", "Score"});
    CHECK(arr == std::vector<std::vector<int>>{{1, 25, 100}, {2, 38, 87}, {3, 19, 55}});
}

TEST_CASE("Array check")
{
    CHECK(csv::detail::IsArray<std::array<int, 4>>::value == true);
    CHECK(csv::detail::IsArray<std::vector<int>>::value == false);
}

TEST_CASE("Tab delimiters")
{
    const auto dataPath = dataDir / "test.tsv";
    assert(std::filesystem::exists(dataPath));

    auto tups = csv::to_tuples<int, float, bool, int, int, int>(dataPath.string(), '\t');
    REQUIRE(!tups.empty());

    const auto& t = tups.back();
    CHECK(std::get<0>(t) == 2);
    CHECK(std::get<1>(t) == 5.04f);
    CHECK(std::get<2>(t) == false);
    CHECK(std::get<3>(t) == 4);
    CHECK(std::get<4>(t) == 5);
    CHECK(std::get<5>(t) == -6);
}

TEST_CASE("Tab delimiter deduction")
{
    const auto dataPath = dataDir / "test.tsv";
    assert(std::filesystem::exists(dataPath));

    auto tups = csv::to_tuples<int, float, bool, int, int, int>(dataPath.string());
    REQUIRE(!tups.empty());

    const auto& t = tups.at(1);
    CHECK(std::get<0>(t) == 3);
    CHECK(std::get<1>(t) == 1.14f);
    CHECK(std::get<2>(t) == false);
    CHECK(std::get<3>(t) == 5);
    CHECK(std::get<4>(t) == 9);
    CHECK(std::get<5>(t) == -9999);
}
