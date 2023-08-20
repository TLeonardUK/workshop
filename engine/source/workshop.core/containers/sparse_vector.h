// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/memory/memory.h"

#include <string>
#include <array>
#include <vector>
#include <functional>

namespace ws {

// ================================================================================================
//  This class represents a vector that is backed by a reserved virtual memory address space.
//  Indices can be allocated and freed sparsely and the vector will attempt to only allocate
//  memory to relevant pages of the vector that are being used.
// 
//  Due to linear address space, inserting/removing/accessing is all O(1).
// ================================================================================================
template <typename element_type>
class sparse_vector
{
public:
    sparse_vector(size_t max_elements);
    ~sparse_vector();

    // Inserts the given element into the vector and returns the index it was inserted at.
    size_t insert(const element_type& type);

    // Removes the given index in the vector and allows it to be reused.
    void remove(size_t index);

    // Gets the given index in the vector.
    element_type& at(size_t index);

    // Gets the given index in the vector.
    element_type& operator[](size_t index);

private:

    void commit_region(size_t index);
    void decommit_region(size_t index);

private:
    struct page
    {
        uint8_t* memory;
        size_t commit_count;
    };

    size_t m_max_elements;

    size_t m_page_size;
    uint8_t* m_memory_base;
    std::vector<page> m_pages;

    std::vector<size_t> m_free_indices;

};

template <typename element_type>
inline sparse_vector<element_type>::sparse_vector(size_t max_elements)
    : m_max_elements(max_elements)
{
    m_page_size = get_page_size();

    size_t size = max_elements * sizeof(element_type);
    size_t page_count = (size + (m_page_size - 1)) / m_page_size;

    m_memory_base = (uint8_t*)reserve_virtual_memory(page_count * m_page_size);

    for (size_t i = 0; i < max_elements; i++)
    {
        m_free_indices.push_back(max_elements - (i + 1));
    }

    m_pages.resize(page_count);
    for (size_t i = 0; i < page_count; i++)
    {
        page& page_instance = m_pages[i];
        page_instance.memory = m_memory_base + (i * m_page_size);
        page_instance.commit_count = 0;
    }
}

template <typename element_type>
inline sparse_vector<element_type>::~sparse_vector()
{
    free_virtual_memory(m_memory_base);
    m_memory_base = nullptr;
}

template <typename element_type>
inline void sparse_vector<element_type>::commit_region(size_t index)
{
    size_t start_offset = index * sizeof(element_type);
    size_t end_offset = ((index + 1) * sizeof(element_type)) - 1;

    size_t start_page = start_offset / m_page_size;
    size_t end_page = end_offset / m_page_size;

    for (size_t i = start_page; i <= end_page; i++)
    {
        if (m_pages[i].commit_count == 0)
        {
            commit_virtual_memory(m_pages[i].memory, m_page_size);
        }
        m_pages[i].commit_count++;
    }
}

template <typename element_type>
inline void sparse_vector<element_type>::decommit_region(size_t index)
{
    size_t start_offset = index * sizeof(element_type);
    size_t end_offset = ((index + 1) * sizeof(element_type)) - 1;

    size_t start_page = start_offset / m_page_size;
    size_t end_page = end_offset / m_page_size;

    for (size_t i = start_page; i <= end_page; i++)
    {
        db_assert(m_pages[i].commit_count > 0);

        m_pages[i].commit_count--;
        if (m_pages[i].commit_count == 0)
        {
            decommit_virtual_memory(m_pages[i].memory, m_page_size);
        }
    }
}

template <typename element_type>
inline size_t sparse_vector<element_type>::insert(const element_type& type)
{
    if (m_free_indices.empty())
    {
        db_fatal(core, "Ran out of free indices in sparse_vector.");
    }

    size_t index = m_free_indices.back();
    m_free_indices.pop_back();
    commit_region(index);

    element_type* element = reinterpret_cast<element_type*>(m_memory_base + (index * sizeof(element_type)));
    new(element) element_type();
    *element = type;

    return index;
}

template <typename element_type>
inline void sparse_vector<element_type>::remove(size_t index)
{
    element_type* element = reinterpret_cast<element_type*>(m_memory_base + (index * sizeof(element_type)));
    element->~element_type();

    decommit_region(index);

    m_free_indices.push_back(index);
}

template <typename element_type>
inline element_type& sparse_vector<element_type>::at(size_t index)
{
    return *reinterpret_cast<element_type*>(m_memory_base + (index * sizeof(element_type)));
}

template <typename element_type>
inline element_type& sparse_vector<element_type>::operator[](size_t index)
{
    return at(index);
}

}; // namespace workshop
