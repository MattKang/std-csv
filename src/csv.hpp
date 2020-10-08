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

using ignore = std::tuple<>;

/// @brief Defines std::tuple<FilteredTs...>
template<typename... Ts>
using FilteredTuple = decltype(std::tuple_cat(
        std::declval<std::conditional_t<std::is_same<ignore, Ts>::value, std::tuple<>, std::tuple<Ts>>>()...));

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
struct IsArray : std::false_type { };

template<typename T, size_t N>
struct IsArray<std::array<T, N>> : std::true_type { };

template<typename T>
struct IsTuple : std::false_type { };

template<typename... Ts>
struct IsTuple<std::tuple<Ts...>> : std::true_type { };

template<typename T>
struct FilteredTypes { using type = T; };

template<typename... Ts>
struct FilteredTypes<std::tuple<Ts...>> { using type = FilteredTuple<Ts...>; };
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

template<typename UnwantedT = ignore, typename... Ts>
auto filterTupleByType(std::tuple<Ts...> tup)
{
    return getTupleBySequence(tup, FilteredIndexSequence<UnwantedT, Ts...>{});
}

template<typename T, typename CharT>
T parseRow(std::basic_istream<CharT>& row, char delimiter)
{
    if constexpr (std::is_same_v<T, ignore>)
    {
        row.ignore(std::numeric_limits<std::streamsize>::max(), delimiter);
        return {};
    }
    else if constexpr (std::is_same_v<T, std::string>)
    {
        std::string value;
        std::getline(row, value, delimiter);
        if (row.eof() && value.back() == '\r')
        {
            value.pop_back();
        }
        return value;
    }
    else if constexpr (std::is_same_v<T, bool>)
    {
        std::string valueAsString;
        std::getline(row, valueAsString, delimiter);
        const auto first = valueAsString[valueAsString.find_first_of("tTfF01")];
        return (!valueAsString.empty()
                && (first == 't' || first == 'T' || first == '1'));
    }
    else
    {
        T value = std::numeric_limits<T>::quiet_NaN();
        std::string valueAsString;
        std::getline(row, valueAsString, delimiter);
        std::istringstream(valueAsString) >> value;
        return value;
    }
}

template<typename ...Ts, typename CharT>
FilteredTuple<Ts...> parseRowToTuples(std::basic_istream<CharT>& row, char delimiter, std::tuple<Ts...>)
{
    std::string value;
    auto unfilteredTuple = std::tuple<Ts...>{detail::parseRow<Ts>(row, delimiter)...};
    return filterTupleByType(unfilteredTuple);
}

template<typename RowT, typename CharT>
auto parseCsv(std::basic_istream<CharT>& file, char delimiter)
{
    using RowOutT = std::conditional_t<IsTuple<RowT>::value, typename FilteredTypes<RowT>::type, RowT>;
    std::vector<RowOutT> data;
    std::string line;
    while (std::getline(file, line))
    {
        std::istringstream stream(line);
        RowOutT values;
        if constexpr (IsArray<RowOutT>::value)
        {
            for (auto& v : values)
            {
                v = detail::parseRow<typename RowOutT::value_type>(stream, delimiter);
            }
        }
        else if constexpr (IsTuple<RowOutT>::value)
        {
            values = detail::parseRowToTuples(stream, delimiter, RowT{});
        }
        else
        {
            while (!stream.eof())
            {
                values.push_back(detail::parseRow<typename RowOutT::value_type>(stream, delimiter));
            }
        }
        data.push_back(std::move(values));
    }
    return data;
}

template<typename ContainerT>
ContainerT getHeader(std::ifstream& file, char delimiter)
{
    std::string line;
    std::getline(file, line);
    std::istringstream stream(line);
    return detail::parseCsv<ContainerT>(stream, delimiter).front();
}

template<typename ContainerT, typename HeaderT = ignore>
auto toContainers(const std::string_view& path, char delimiter, HeaderT&& header = {})
{
    // Open file
    auto file = std::ifstream(path.data());
    if (file)
    {
        // Check delimiter
        if (delimiter == '\0')
        {
            delimiter = detail::getDelimiter(file);
        }
        // Read header
        if constexpr (!std::is_same_v<HeaderT, ignore>)
        {
            header = detail::getHeader<std::decay_t<HeaderT>>(file, delimiter);
        }
        // Read CSV
        return detail::parseCsv<ContainerT>(file, delimiter);
    }
    return decltype(detail::parseCsv<ContainerT>(file, delimiter)){};
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

template<typename ValueT, size_t nColumns>
std::vector<std::array<ValueT, nColumns>> toArrays(const std::string_view& path, char delimiter = '\0')
{
    return detail::toContainers<std::array<ValueT, nColumns>>(path, delimiter);
}

template<typename ValueT, size_t nColumns>
std::vector<std::array<ValueT, nColumns>> toArrays(const std::string_view& path,
                                                   std::array<std::string, nColumns>& header,
                                                   char delimiter = '\0')
{
    return detail::toContainers<std::array<ValueT, nColumns>>(path, delimiter, header);
}

template<typename... ColumnTs>
std::vector<FilteredTuple<ColumnTs...>> toTuples(const std::string_view& path, char delimiter = '\0')
{
    return detail::toContainers<std::tuple<ColumnTs...>>(path, delimiter);
}

template<typename... ColumnTs, size_t nColumns = std::tuple_size_v<FilteredTuple<ColumnTs...>>>
std::vector<FilteredTuple<ColumnTs...>> toTuples(const std::string_view& path,
                                                 std::array<std::string, nColumns>& header,
                                                 char delimiter = '\0')
{
    return detail::toContainers<std::tuple<ColumnTs...>>(path, delimiter, header);
}

template<typename ValueT>
std::vector<std::vector<ValueT>> toVectors(const std::string_view& path, char delimiter = '\0')
{
    return detail::toContainers<std::vector<ValueT>>(path, delimiter);
}

template<typename ValueT>
std::vector<std::vector<ValueT>> toVectors(const std::string_view& path,
                                           std::vector<std::string>& header,
                                           char delimiter = '\0')
{
    return detail::toContainers<std::vector<ValueT>>(path, delimiter, header);
}

} // namespace csv

#endif //CSV_HPP
