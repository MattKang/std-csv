//
// Created by Matthew Kang on 2019-04-25.
//

#ifndef CSV_H
#define CSV_H

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
template<typename... Ts>
using FilteredTuple = decltype(std::tuple_cat(
        std::declval<std::conditional_t<std::is_same<IGNORE, Ts>::value, std::tuple<>, std::tuple<Ts>>>()...));


namespace detail
{

template<typename CharT>
char getDelimiter(CharT&& line)
{
    char delimiter = ' '; // space-separated
    if (std::regex_search(line, std::regex(","))) // comma-separated
    {
        delimiter = ',';
    }
    else if (std::regex_search(line, std::regex(";"))) // semicolon-separated
    {
        delimiter = ';';
    }
    else if (std::regex_search(line, std::regex("\\t"))) // tab-separated
    {
        delimiter = '\t';
    }
    return delimiter;
}

template<typename T>
T parseStream(std::istringstream& stream, char delimiter = ',')
{
    T value = std::numeric_limits<T>::quiet_NaN();
    std::string valueAsString;
    std::getline(stream, valueAsString, delimiter);
    std::istringstream(valueAsString) >> value;
    return value;
}

template<>
IGNORE parseStream(std::istringstream& stream, char delimiter)
{
    stream.ignore(std::numeric_limits<std::streamsize>::max(), delimiter);
    return {};
}

template<>
std::string parseStream(std::istringstream& stream, char delimiter)
{
    std::string value;
    std::getline(stream, value, delimiter);
    return value;
}

template<typename>
struct IntegralConstantTupleToSequence;

template<typename... Ts>
struct IntegralConstantTupleToSequence<std::tuple<Ts...>>
{
    using type = std::index_sequence<Ts::value...>;
};

template<typename UnwantedT = IGNORE, typename... Ts, size_t... Indices>
auto getFilteredSequence(const std::tuple<Ts...>&, std::index_sequence<Indices...>)
{
    using FilteredIntegralConstantTupleT = decltype(std::tuple_cat(
            std::declval<std::conditional_t<std::is_same<UnwantedT, Ts>::value,
                                            std::tuple<>,
                                            std::tuple<std::integral_constant<size_t, Indices>>
                                           >
                        >()...));
    return typename IntegralConstantTupleToSequence<FilteredIntegralConstantTupleT>::type{};
}

template<typename... Ts, size_t... Indices>
auto getTupleBySequence(const std::tuple<Ts...>& tup, std::index_sequence<Indices...>)
{
    return std::make_tuple(std::get<Indices>(tup)...);
}

template<typename UnwantedT = IGNORE, typename... Ts>
auto filterTuple(const std::tuple<Ts...>& tup)
{
    return getTupleBySequence(tup, getFilteredSequence<UnwantedT>(tup, std::make_index_sequence<sizeof...(Ts)>{}));
}

template<typename VectorT, size_t... Indices>
std::decay_t<VectorT> getVectorBySequence(VectorT&& vec, std::index_sequence<Indices...>)
{
    return {vec.at(Indices)...};
}

} // namespace detail


template<typename CharT>
std::vector<std::string> getHeader(CharT&& filename)
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
    const char delimiter = detail::getDelimiter(line);

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

template<typename DataT, size_t NColumns, typename CharT>
std::vector<std::array<DataT, NColumns>> toArrays(CharT&& filename, bool skipFirstLineHeader = false)
{
    using ArrayT = std::array<DataT, NColumns>;
    // Open file
    std::ifstream file(std::forward<CharT>(filename));
    if (!file)
    {
        return {};
    }

    std::string line;
    std::getline(file, line);

    // Determine delimiter
    const char delimiter = detail::getDelimiter(line);

    // Skip header
    if (skipFirstLineHeader)
    {
        std::getline(file, line); // Get next line
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

template<typename DataT, size_t NColumns, typename CharT>
std::vector<std::array<DataT, NColumns>> toArrays(CharT&& filename, std::array<std::string, NColumns>& header)
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
    const char delimiter = detail::getDelimiter(line);

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

    return toArrays<DataT, NColumns>(std::move(file), false); // don't skip first line (header) as we already extracted it
}

template<typename... ColumnTs, typename CharT>
std::vector<FilteredTuple<ColumnTs...>> toTuples(CharT&& filename, bool skipFirstLineHeader = false)
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
    const char delimiter = detail::getDelimiter(line);

    // Skip header
    if (skipFirstLineHeader)
    {
        std::getline(file, line); // Get next line
    }

    // Get data
    std::vector<FilteredTuple<ColumnTs...>> data;
    do
    {
        std::istringstream stream(line);
        std::tuple<ColumnTs...> unfilteredTuple(detail::parseStream<ColumnTs>(stream, delimiter)...);
        data.emplace_back(detail::filterTuple(unfilteredTuple));
    }
    while (std::getline(file, line));

    file.close();
    return data;
}

template<typename... ColumnTs, typename CharT>
std::vector<FilteredTuple<ColumnTs...>> toTuples(CharT&& filename, std::vector<std::string>& header)
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
    const char delimiter = detail::getDelimiter(line);

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
                                         detail::getFilteredSequence(std::tuple<ColumnTs...>{},
                                                                     std::make_index_sequence<sizeof...(ColumnTs)>{}));

    return toTuples<ColumnTs...>(std::move(file), false); // don't skip first line (header) as we already extracted it
}

template<typename DataT, typename CharT>
std::vector<std::vector<DataT>> toVectors(CharT&& filename, bool skipFirstLineHeader = false)
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
    const char delimiter = detail::getDelimiter(line);

    // Skip header
    if (skipFirstLineHeader)
    {
        std::getline(file, line); // Get next line
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

template<typename... ColumnTs, typename CharT>
std::vector<FilteredTuple<ColumnTs...>> toVectors(CharT&& filename, std::vector<std::string>& header)
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
    const char delimiter = detail::getDelimiter(line);

    // Get header
    header.clear();
    std::istringstream stream(line);
    std::string label;
    while (std::getline(stream, label, delimiter))
    {
        header.push_back(label);
    }

    return toVectors<ColumnTs...>(std::move(file), false); // don't skip first line (header) as we already extracted it
}

} // namespace csv

#endif //CSV_H
