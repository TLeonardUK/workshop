// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include <string>
#include <unordered_map>
#include <mutex>

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
    string_hash(const char* value);
    string_hash(const std::string& value);

    string_hash& operator=(const string_hash& vector) = default;
    string_hash& operator=(string_hash&& vector) = default;

    auto operator<=>(const string_hash& other) const = default;

    // Slow, use with care. Hashing should generally
    // be one way outside of debugging.
    const char* get_string();

    size_t get_hash() const;

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

private:    
    db_index m_index;

    inline static std::mutex m_db_mutex;
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
