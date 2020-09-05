//
// Created by Matthew Kang on 2019-04-25.
//

#ifndef CSV_HPP
#define CSV_HPP

#include <array>
#include <fstream>
#include <regex>
#include <sstream>
#include <tuple>
#include <vector>

namespace csv
{

using IGNORE = std::tuple<>;

/// @brief Defines std::tuple<FilteredTs...>
template<typename ...Ts>
using FilteredTuple = decltype(std::tuple_cat(
        std::declval<std::conditional_t<std::is_same<IGNORE, Ts>::value, std::tuple<>, std::tuple<Ts>>>()...));

namespace detail
{

/// @defgroup Helper types
/// @{
template<typename>
struct IndexSequence;

template<typename ...IntegralConstants>
struct IndexSequence<std::tuple<IntegralConstants...>>
{
    using type = std::index_sequence<IntegralConstants::value...>;
};

template<typename UnwantedT, typename Sequence, typename ...Ts>
struct FilteredIndexSequenceImpl;

template<typename UnwantedT, size_t... indices, typename... Ts>
struct FilteredIndexSequenceImpl<UnwantedT, std::index_sequence<indices...>, Ts...>
{
    using FilteredIntegralConstantTuple = decltype(std::tuple_cat(
            std::declval<std::conditional_t<std::is_same<UnwantedT, Ts>::value,
                                            std::tuple<>,
                                            std::tuple<std::integral_constant<size_t, indices>>
                                           >
                        >()...));
    using type = typename IndexSequence<FilteredIntegralConstantTuple>::type;
};

template<typename UnwantedT, typename ...Ts>
using FilteredIndexSequence = typename FilteredIndexSequenceImpl<UnwantedT,
                                                                 std::index_sequence_for<Ts...>,
                                                                 Ts...>::type;

template<typename T>
struct is_array : std::false_type { };

template<typename T, size_t N>
struct is_array<std::array<T, N>> : std::true_type { };
/// @}

template<typename CharT>
char getDelimiter(std::basic_istream<CharT>& file)
{
    std::string line;
    const auto position = file.tellg();
    std::getline(file, line);
    file.seekg(position); // rewind our line get

    char delimiter = ' '; // space-separated
    if (std::regex_search(line.cbegin(), line.cend(), std::regex(","))) // comma-separated
    {
        delimiter = ',';
    }
    else if (std::regex_search(line.cbegin(), line.cend(), std::regex(";"))) // semicolon-separated
    {
        delimiter = ';';
    }
    else if (std::regex_search(line.cbegin(), line.cend(), std::regex("\\t"))) // tab-separated
    {
        delimiter = '\t';
    }

    return delimiter;
}

template<typename... Ts, size_t... indices>
auto getTupleBySequence(const std::tuple<Ts...>& tup, std::index_sequence<indices...>)
{
    return std::make_tuple(std::get<indices>(tup)...);
}

template<typename UnwantedT = IGNORE, typename... Ts>
auto filterTupleByType(const std::tuple<Ts...>& tup)
{
    return getTupleBySequence(tup, FilteredIndexSequence<UnwantedT, Ts...>{});
}

template<typename T>
T parseStream(std::istringstream& stream, char delimiter)
{
    if constexpr (std::is_same_v<T, IGNORE>)
    {
        stream.ignore(std::numeric_limits<std::streamsize>::max(), delimiter);
        return {};
    }
    else if constexpr (std::is_same_v<T, std::string>)
    {
        std::string value;
        std::getline(stream, value, delimiter);
        return value;
    }
    else if constexpr (std::is_same_v<T, bool>)
    {
        std::string valueAsString;
        std::getline(stream, valueAsString, delimiter);
        const auto first = valueAsString[valueAsString.find_first_of("tTfF01")];
        if (!valueAsString.empty()
            && (first == 't' || first == 'T' || first == '1'))
        {
            return true;
        }
        return false;
    }
    else
    {
        T value = std::numeric_limits<T>::quiet_NaN();
        std::string valueAsString;
        std::getline(stream, valueAsString, delimiter);
        std::istringstream(valueAsString) >> value;
        return value;
    }
}

template<typename T, typename CharT>
std::vector<T> parseCsv(std::basic_istream<CharT>& file, char delimiter)
{
    std::vector<T> data;
    std::string line;
    while (std::getline(file, line))
    {
        std::istringstream stream(line);
        T values;
        if constexpr (is_array<T>::value)
        {
            for (auto& v : values)
            {
                v = detail::parseStream<typename T::value_type>(stream, delimiter);
            }
        }
        else
        {
            while (!stream.eof())
            {
                values.push_back(detail::parseStream<typename T::value_type>(stream, delimiter));
            }
        }
        data.push_back(std::move(values));
    }
    return data;
}

template<typename ...Ts, typename CharT>
std::vector<FilteredTuple<Ts...>> parseCsvToTuples(std::basic_istream<CharT>& file, char delimiter)
{
    std::vector<FilteredTuple<Ts...>> data;
    std::string line;
    while (std::getline(file, line))
    {
        std::istringstream stream(line);
        auto unfilteredTuple = std::tuple<Ts...>{detail::parseStream<Ts>(stream, delimiter)...};
        data.push_back(filterTupleByType(unfilteredTuple));
    }
    return data;
}

template<typename ArrayT>
ArrayT getHeader(std::ifstream& file, char delimiter)
{
    std::string line;
    std::getline(file, line);
    std::istringstream stream(line);
    return detail::parseCsv<ArrayT>(stream, delimiter).front();
}
} // namespace detail

std::vector<std::string> getHeader(const std::string_view& path, char delimiter = '\0')
{
    // Open file
    if (auto file = std::ifstream(path.data()))
    {
        // Check delimiter
        if (delimiter == '\0')
        {
            delimiter = detail::getDelimiter(file);
        }
        // Read header
        return detail::getHeader<std::vector<std::string>>(file, delimiter);
    }
    return {};
}

template<typename DataT, size_t nColumns>
std::vector<std::array<DataT, nColumns>> toArrays(const std::string_view& path, char delimiter = '\0')
{
    // Open file
    if (auto file = std::ifstream(path.data()))
    {
        // Check delimiter
        if (delimiter == '\0')
        {
            delimiter = detail::getDelimiter(file);
        }
        // Read CSV
        return detail::parseCsv<std::array<DataT, nColumns>>(file, delimiter);
    }
    return {};
}

template<typename DataT, size_t nColumns>
std::vector<std::array<DataT, nColumns>> toArrays(const std::string_view& path,
                                                  std::array<std::string, nColumns>& header,
                                                  char delimiter = '\0')
{
    // Open file
    if (auto file = std::ifstream(path.data()))
    {
        // Check delimiter
        if (delimiter == '\0')
        {
            delimiter = detail::getDelimiter(file);
        }
        // Read header
        header = detail::getHeader<std::array<std::string, nColumns>>(file, delimiter);
        // Read CSV
        return detail::parseCsv<std::array<DataT, nColumns>>(file, delimiter);
    }
    return {};
}

template<typename... ColumnTs>
std::vector<FilteredTuple<ColumnTs...>> toTuples(const std::string_view& path, char delimiter = '\0')
{
    // Open file
    if (auto file = std::ifstream(path.data()))
    {
        // Check delimiter
        if (delimiter == '\0')
        {
            delimiter = detail::getDelimiter(file);
        }
        // Read CSV
        return detail::parseCsvToTuples<ColumnTs...>(file, delimiter);
    }
    return {};
}

template<typename... ColumnTs, size_t nColumns = std::tuple_size_v<FilteredTuple<ColumnTs...>>>
std::vector<FilteredTuple<ColumnTs...>> toTuples(const std::string_view& path,
                                                 std::array<std::string, nColumns>& header,
                                                 char delimiter = '\0')
{
    // Open file
    if (auto file = std::ifstream(path.data()))
    {
        // Check delimiter
        if (delimiter == '\0')
        {
            delimiter = detail::getDelimiter(file);
        }
        // Read header
        header = detail::getTupleBySequence(detail::getHeader<std::array<std::string, nColumns>>(file, delimiter),
                                            detail::FilteredIndexSequence<IGNORE, ColumnTs...>{});
        // Read CSV
        return detail::parseCsvToTuples<ColumnTs...>(file, delimiter);
    }
    return {};
}

template<typename DataT>
std::vector<std::vector<DataT>> toVectors(const std::string_view& path, char delimiter = '\0')
{
    if (auto file = std::ifstream(path.data()))
    {
        // Check delimiter
        if (delimiter == '\0')
        {
            delimiter = detail::getDelimiter(file);
        }
        // Read CSV
        return detail::parseCsv<std::vector<DataT>>(file, delimiter);
    }
    return {};
}

template<typename DataT, typename... ColumnTs>
std::vector<std::vector<DataT>> toVectors(const std::string_view& path,
                                          std::vector<std::string>& header,
                                          char delimiter = '\0')
{
    // Open file
    if (auto file = std::ifstream(path.data()))
    {
        // Check delimiter
        if (delimiter == '\0')
        {
            delimiter = detail::getDelimiter(file);
        }
        // Read header
        header = detail::getHeader<std::vector<std::string>>(file, delimiter);
        // Read CSV
        return detail::parseCsv<std::vector<DataT>>(file, delimiter);
    }
    return {};
}

} // namespace csv

#endif //CSV_HPP
