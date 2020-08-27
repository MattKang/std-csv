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

// defines std::tuple<FilteredTs...>
template<typename ...Ts>
using FilteredTuple = decltype(std::tuple_cat(
        std::declval<std::conditional_t<std::is_same<IGNORE, Ts>::value, std::tuple<>, std::tuple<Ts>>>()...));

namespace detail
{

char getDelimiter(const std::string_view& line)
{
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
    else
    {
        T value = std::numeric_limits<T>::quiet_NaN();
        std::string valueAsString;
        std::getline(stream, valueAsString, delimiter);
        std::istringstream(valueAsString) >> value;
        return value;
    }
}

template<typename>
struct IndexSequence;

template<typename ...IntegralConstants>
struct IndexSequence<std::tuple<IntegralConstants...>>
{
    using type = std::index_sequence<IntegralConstants::value...>;
};

template<typename UnwantedT, typename Sequence, typename ...Ts>
struct FilteredIndexSequenceImpl;

template<typename UnwantedT, size_t... Indices, typename... Ts>
struct FilteredIndexSequenceImpl<UnwantedT, std::index_sequence<Indices...>, Ts...>
{
    using FilteredIntegralConstantTuple = decltype(std::tuple_cat(
            std::declval<std::conditional_t<std::is_same<UnwantedT, Ts>::value,
                                            std::tuple<>,
                                            std::tuple<std::integral_constant<size_t, Indices>>
                                           >
                        >()...));
    using type = typename IndexSequence<FilteredIntegralConstantTuple>::type;
};

template<typename UnwantedT, typename ...Ts>
using FilteredIndexSequence = typename FilteredIndexSequenceImpl<UnwantedT, std::index_sequence_for<Ts...>, Ts...>::type;

template<typename... Ts, size_t... Indices>
auto getTupleBySequence(const std::tuple<Ts...>& tup, std::index_sequence<Indices...>)
{
    return std::make_tuple(std::get<Indices>(tup)...);
}

template<typename UnwantedT = IGNORE, typename... Ts>
auto filterTupleByType(const std::tuple<Ts...>& tup)
{
    return getTupleBySequence(tup, FilteredIndexSequence<UnwantedT, Ts...>{});
}

template<typename VectorT, size_t... Indices>
std::decay_t<VectorT> getVectorBySequence(VectorT&& vec, std::index_sequence<Indices...>)
{
    return {vec.at(Indices)...};
}

} // namespace detail


template<typename CharT>
std::vector<std::string> getHeader(CharT&& filename, char delimiter = '\0')
{
    // Open file
    std::ifstream file(std::forward<CharT>(filename));
    if (!file)
    {
        return {};
    }

    std::string line;
    std::getline(file, line);

    file.close();

    // Determine delimiter
    if (delimiter == '\0')
    {
        delimiter = detail::getDelimiter(line);
    }

    // Get header
    std::istringstream stream(line);
    std::string label;
    std::vector<std::string> header;
    while (std::getline(stream, label, delimiter))
    {
        header.push_back(label);
    }

    return header;
}

template<typename DataT, size_t nColumns>
std::vector<std::array<DataT, nColumns>> toArrays(const std::string_view& filename, char delimiter = ',')
{
    using ArrayT = std::array<DataT, nColumns>;
    // Open file
    std::ifstream file(filename.data());
    if (!file)
    {
        return {};
    }

    std::string line;
    std::getline(file, line);

    // Determine delimiter
    if (delimiter == '\0')
    {
        delimiter = detail::getDelimiter(line);
    }

    // Get data
    std::vector<ArrayT> data;
    do
    {
        std::istringstream stream(line);
        std::string value;
        ArrayT values;
        for (auto& v : values)
        {
            v = detail::parseStream<DataT>(stream, delimiter);
        }
        data.push_back(values);
    }
    while (std::getline(file, line));

    file.close();
    return data;
}

template<typename DataT, size_t nColumns, typename CharT>
std::vector<std::array<DataT, nColumns>> toArrays(CharT&& filename,
                                                  std::array<std::string, nColumns>& header,
                                                  char delimiter = '\0')
{
    // Open file
    std::ifstream file(std::forward<CharT>(filename));
    if (!file)
    {
        return {};
    }

    std::string line;
    std::getline(file, line);

    // Determine delimiter
    if (delimiter == '\0')
    {
        delimiter = detail::getDelimiter(line);
    }

    // Get header
    std::istringstream stream(line);
    std::string label;
    for (auto& h : header)
    {
        if (std::getline(stream, label, delimiter))
        {
            h = label;
        }
    }

    return toArrays<DataT, nColumns>(std::move(file), delimiter);
}

template<typename... ColumnTs, typename CharT>
std::vector<FilteredTuple<ColumnTs...>> toTuples(CharT&& filename, char delimiter = '\0')
{
    // Open file
    std::ifstream file(std::forward<CharT>(filename));
    if (!file)
    {
        return {};
    }

    std::string line;
    std::getline(file, line);

    // Determine delimiter
    if (delimiter == '\0')
    {
        delimiter = detail::getDelimiter(line);
    }

    // Get data
    std::vector<FilteredTuple<ColumnTs...>> data;
    do
    {
        std::istringstream stream(line);
        std::tuple<ColumnTs...> unfilteredTuple(detail::parseStream<ColumnTs>(stream, delimiter)...);
        data.emplace_back(detail::filterTupleByType(unfilteredTuple));
    }
    while (std::getline(file, line));

    file.close();
    return data;
}

template<typename... ColumnTs>
std::vector<FilteredTuple<ColumnTs...>> toTuples(const std::string_view& filename,
                                                 std::vector<std::string>& header,
                                                 char delimiter = '\0')
{
    // Open file
    std::ifstream file(filename.data());
    if (!file)
    {
        return {};
    }

    std::string line;
    std::getline(file, line);

    // Determine delimiter
    if (delimiter == '\0')
    {
        delimiter = detail::getDelimiter(line);
    }

    // Get header
    header.clear();
    std::istringstream stream(line);
    std::string label;
    while (std::getline(stream, label, delimiter))
    {
        header.push_back(label);
    }

    // Remove ignored columns
    header = detail::getVectorBySequence(std::move(header),
                                         detail::FilteredIndexSequence(std::tuple<ColumnTs...>{},
                                                                       std::make_index_sequence<sizeof...(ColumnTs)>{}));

    return toTuples<ColumnTs...>(std::move(file), delimiter);
}

template<typename DataT, typename CharT>
std::vector<std::vector<DataT>> toVectors(CharT&& filename, char delimiter = '\0')
{
    // Open file
    std::ifstream file(std::forward<CharT>(filename));
    if (!file)
    {
        return {};
    }

    std::string line;
    std::getline(file, line);

    // Determine delimiter
    if (delimiter == '\0')
    {
        delimiter = detail::getDelimiter(line);
    }

    // Get data
    std::vector<std::vector<DataT>> data;
    do
    {
        std::istringstream stream(line);
        std::vector<DataT> values;
        while (!stream.eof())
        {
            values.emplace_back(detail::parseStream<DataT>(stream, delimiter));
        }
        data.push_back(values);
    }
    while (std::getline(file, line));

    file.close();
    return data;
}

template<typename... ColumnTs>
std::vector<FilteredTuple<ColumnTs...>> toVectors(const std::string_view& filename,
                                                  std::vector<std::string>& header,
                                                  char delimiter = '\0')
{
    // Open file
    std::ifstream file(filename.data());
    if (!file)
    {
        return {};
    }

    std::string line;
    std::getline(file, line);

    // Determine delimiter
    if (delimiter == '\0')
    {
        delimiter = detail::getDelimiter(line);
    }

    // Get header
    header.clear();
    std::istringstream stream(line);
    std::string label;
    while (std::getline(stream, label, delimiter))
    {
        header.push_back(label);
    }

    return toVectors<ColumnTs...>(std::move(file), delimiter);
}

} // namespace csv

#endif //CSV_HPP
