// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.core/hashing/string_hash.h"
#include "workshop.core/hashing/hash.h"
#include "workshop.core/containers/string.h"

namespace ws {

string_hash string_hash::empty = "";

string_hash::string_hash(const char* value)
{
    m_index = intern(value);
}

string_hash::string_hash(const std::string& value)
{
    m_index = intern(value.c_str());
}

string_hash::db_index string_hash::intern(const char* value)
{
    std::string lowercase = string_lower(value);
    size_t hash = const_hash(lowercase.c_str(), lowercase.size());

    std::scoped_lock lock(m_db_mutex);

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

const char* string_hash::get_string()
{
    std::scoped_lock lock(m_db_mutex);

    if (auto iter = m_db.find(m_index.bucket); iter != m_db.end())
    {
        db_bucket& bucket = iter->second;
        return (*bucket.strings[m_index.offset]).c_str();
    }
    
    return "";
}

size_t string_hash::get_hash() const
{
    size_t h = 0;
    hash_combine(h, m_index.bucket);
    hash_combine(h, m_index.offset);
    return h;
}

}; // namespace workshop
