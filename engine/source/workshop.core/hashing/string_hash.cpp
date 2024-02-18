// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/hashing/string_hash.h"
#include "workshop.core/hashing/hash.h"
#include "workshop.core/containers/string.h"
#include "workshop.core/utils/string_formatter.h"

namespace ws {

string_hash string_hash::empty = string_hash("");

string_hash::string_hash(const char* value)
{
    m_index = intern(value);
}

string_hash::string_hash(const std::string& value)
{
    m_index = intern(value.c_str());
}

bool string_hash::try_find_index(size_t hash, const char* value, string_hash::db_index& index)
{
    std::shared_lock lock(m_db_mutex);

    // If no buckets yet for this hash, make one.
    if (auto iter = m_db.find(hash); iter == m_db.end())
    {
        return false;
    }
    // Otherwise look to see if the value already exists in bucket, if it does
    // return it, otherwise intern a new string.
    else
    {
        db_bucket& bucket = iter->second;

        auto string_iter = std::find_if(bucket.strings.begin(), bucket.strings.end(), [&value](const std::unique_ptr<std::string>& str) {
            return (*str) == value;
        });

        if (string_iter == bucket.strings.end())
        {
            return false;
        }
        else
        {
            size_t offset = std::distance(bucket.strings.begin(), string_iter);
            index = { hash, offset };
            return true;
        }
    }
}

string_hash::db_index string_hash::create_index(size_t hash, const char* value)
{
    std::unique_lock lock(m_db_mutex);

    // If no buckets yet for this hash, make one.
    if (auto iter = m_db.find(hash); iter == m_db.end())
    {
        db_bucket new_bucket;
        new_bucket.strings.push_back(std::make_unique<std::string>(value));

        m_db[hash] = std::move(new_bucket);

        return { hash, 0 };
    }
    // Otherwise look to see if the value already exists in bucket, if it does
    // return it, otherwise intern a new string.
    else
    {
        db_bucket& bucket = iter->second;

        auto string_iter = std::find_if(bucket.strings.begin(), bucket.strings.end(), [&value](const std::unique_ptr<std::string>& str) {
            return (*str) == value;
        });

        if (string_iter == bucket.strings.end())
        {
            bucket.strings.push_back(std::make_unique<std::string>(value));
            return { hash, bucket.strings.size() - 1 };
        }
        else
        {
            size_t offset = std::distance(bucket.strings.begin(), string_iter);
            return { hash, offset };
        }
    }
}

string_hash::db_index string_hash::intern(const char* value)
{
    std::string lowercase = string_lower(value);
    size_t hash = const_hash(lowercase.c_str(), lowercase.size());

    string_hash::db_index index;
    if (try_find_index(hash, value, index))
    {
        return index;
    }

    return create_index(hash, value);
}

const char* string_hash::get_string()
{
    std::shared_lock lock(m_db_mutex);

    if (auto iter = m_db.find(m_index.bucket); iter != m_db.end())
    {
        db_bucket& bucket = iter->second;
        return (*bucket.strings[m_index.offset]).c_str();
    }
    
    return "";
}

const char* string_hash::c_str()
{
    return get_string();
}

size_t string_hash::get_hash() const
{
    size_t h = 0;
    hash_combine(h, m_index.bucket);
    hash_combine(h, m_index.offset);
    return h;
}

}; // namespace workshop
