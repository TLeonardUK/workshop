// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include <functional>
#include <algorithm>
#include <memory>
#include <unordered_set>

namespace ws {

// ================================================================================================
// This file contains a very basic set of types for primitive
// event registering and dispatch.
// 
// You can use it roughly like this:
// 
//   using MyEvent = event<MyDelegate>;
// 
//   MyEvent OnSomething;
//   MyEvent::delegate_ptr ptr = OnSomething.add(Callback);
//   OnSomething.broadcast(1);
// ================================================================================================

class delegate_base
{
public:
    using ptr = std::shared_ptr<delegate_base>;
};

template <typename... parameters>
class delegate : public delegate_base
{
public:
    using callback_function_type = std::function<void(parameters...)>;
    using ptr = std::shared_ptr<delegate>;

    delegate(const callback_function_type& callback)
        : m_callback(callback)
    {
    }

    void invoke(parameters... Args)
    {
        m_callback(Args...);
    }

private:
    callback_function_type m_callback;

};

template <typename... parameters>
class event
{
public:
    using delegate_t = delegate<parameters...>;
    using key_t = uint64_t;

    using delegate_ptr = typename delegate_t::ptr;
    using hook_function = std::function<void()>;

    // Adds a delegate that will be called when the event signals.
    // The return value is a key that must be used to unregister the delegate
    // when it is no longer valid.
    key_t add(const typename delegate_t::callback_function_type& function)
    {
        std::scoped_lock lock(m_event_state->mutex);

        key_t delegate_key = m_event_state->next_key++;

        delegate_ptr result = std::make_shared<delegate_t>(function);
        m_event_state->delegate_set.insert(result.get());
        m_event_state->managed_delegates.insert({ delegate_key, result });

        return delegate_key;
    }

    // Removes a delegate previously added with add()
    void remove(key_t key)
    {
        std::scoped_lock lock(m_event_state->mutex);

        if (auto iter = m_event_state->managed_delegates.find(key); iter != m_event_state->managed_delegates.end())
        {
            delegate_t* delegate_instance = iter->second.get();

            if (auto instance_iter = m_event_state->delegate_set.find(delegate_instance); instance_iter != m_event_state->delegate_set.end())
            {
                m_event_state->delegate_set.erase(instance_iter);
            }

            m_event_state->managed_delegates.erase(iter);
        }
    }

    // Returns a shared_ptr which will automatically remove the delegate when it falls out of scope.
    delegate_ptr add_shared(const typename delegate_t::callback_function_type& function)
    {
        std::scoped_lock lock(m_event_state->mutex);

        std::shared_ptr<event_state_t> state = m_event_state;
        auto deleter = [state](delegate_t* to_delete) mutable {
            std::scoped_lock lock(state->mutex);

            if (auto iter = state->delegate_set.find(to_delete); iter != state->delegate_set.end())
            {
                state->delegate_set.erase(iter);
            }

            delete to_delete;
        };

        delegate_ptr result(new delegate_t(function), deleter);

        m_event_state->delegate_set.insert(result.get());

        return result;
    }

    template<typename... parameters>
    void broadcast(parameters... Args)
    {
        std::scoped_lock lock(m_event_state->mutex);

        // We generate a copy of the map before iterating over it to prevent issues in situations
        // where we unregister during the invokation. This needs handling in a better way, this
        // is a waste of performance. 
        std::unordered_set<delegate_t*> delegate_set_copy = m_event_state->delegate_set;
        for (delegate_t* instance : delegate_set_copy)
        {   
            instance->invoke(Args...);
        }
    }

private:
    struct event_state_t
    {
        std::recursive_mutex mutex;
        std::unordered_set<delegate_t*> delegate_set;
        std::unordered_map<key_t, delegate_ptr> managed_delegates;
        std::atomic<key_t> next_key = 0;
    };

    std::shared_ptr<event_state_t> m_event_state = std::make_shared<event_state_t>();
};

}; // namespace workshop
