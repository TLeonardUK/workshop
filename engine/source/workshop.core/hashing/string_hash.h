// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <shared_mutex>

namespace ws {

// Represents a string as a numeric hash. Allows for fast comparisons.
// A static dictionary is kept during the lifetime of the application 
// that stores all strings that have been hashed to allow resolving
// them back to strings.
class string_hash
{
public:
    static string_hash empty;

    string_hash() = default;
    string_hash(const string_hash& hash) = default;
    string_hash(string_hash&& hash) = default;
    explicit string_hash(const char* value);
    explicit string_hash(const std::string& value);

    string_hash& operator=(const string_hash& vector) = default;
    string_hash& operator=(string_hash&& vector) = default;

    auto operator<=>(const string_hash& other) const = default;

    // Slow, use with care. Hashing should generally
    // be one way outside of debugging.
    const char* get_string();

    size_t get_hash() const;

    // Same as get_string, use for interop/replacement of std::string
    const char* c_str();

private:
    struct db_bucket
    {
        std::vector<std::unique_ptr<std::string>> strings;
    };

    struct db_index
    {
        size_t bucket;
        size_t offset;

        auto operator<=>(const db_index& other) const = default;
    };

    db_index intern(const char* value);

    bool try_find_index(size_t hash, const char* value, string_hash::db_index& index);
    string_hash::db_index create_index(size_t hash, const char* value);

private:    
    db_index m_index;

    inline static std::shared_mutex m_db_mutex;
    inline static std::unordered_map<size_t, db_bucket> m_db;

};

}; // namespace workshop

// ================================================================================================
// Specialization of std::hash for string_id's
// ================================================================================================
template<>
struct std::hash<ws::string_hash>
{
    std::size_t operator()(const ws::string_hash& k) const
    {
        return k.get_hash();
    }
};

// Used defined literal, allows defining a string with a _sh prefix that is only calculated once.
struct string_hash_container 
{
    const char* raw_string;

    constexpr string_hash_container(const char* in_raw_string)
        : raw_string(in_raw_string)
    {
    }
};

template<string_hash_container container>
ws::string_hash operator ""_sh()
{
    static ws::string_hash hashed(container.raw_string);
    return hashed;
}