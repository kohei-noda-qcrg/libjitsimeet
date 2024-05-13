#pragma once
#include <vector>

#include "util/critical.hpp"
#include "util/event.hpp"

template <class T>
class CriticalBuffer {
  private:
    Critical<std::vector<T>> buffer[2];
    std::atomic_int          flip = 0;

  public:
    auto push(T item) -> void {
        auto [lock, data] = buffer[flip].access();
        data.push_back(std::move(item));
    }

    auto swap() -> std::vector<T>& {
        buffer[!flip].unsafe_access().clear();
        flip ^= 1;
        auto [lock, data] = buffer[!flip].access();
        return data;
    }
};

template <class T>
class EventBuffer : public CriticalBuffer<T> {
  private:
    Event event;

  public:
    auto push(T item) -> void {
        CriticalBuffer<T>::push(std::move(item));
        event.wakeup();
    }

    auto wait() -> void {
        event.wait();
    }
};
