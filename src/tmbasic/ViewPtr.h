#pragma once

#include "../common.h"

namespace tmbasic {

// tvision will take ownership when we add it to its parent
template <typename T>
class ViewPtr {
   public:
    template <typename... Args>
    explicit ViewPtr(Args&&... args) : _ptr(new T(std::forward<Args>(args)...)) {}

    ~ViewPtr() {
        if (_owned) {
            delete _ptr;
        }
    }

    T* operator->() { return _ptr; }

    operator T*() { return _ptr; }

    void addTo(TGroup* parent) {
        assert(_ptr != nullptr);
        parent->insert(_ptr);  // ownership transfer
        _owned = false;
    }

   private:
    T* _ptr;
    bool _owned = true;
};

}  // namespace tmbasic
