#pragma once

#include "sw_fwd.h"  // Forward declaration
#include "shared.h"

// https://en.cppreference.com/w/cpp/memory/weak_ptr
template <typename T>
class WeakPtr {
private:
    T* observe_data_ptr_ = nullptr;
    ControlBlockBase* observe_cb_ = nullptr;

    template <typename Y>
    friend class SharedPtr;

public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    WeakPtr() = default;

    WeakPtr(const WeakPtr& other)
        : observe_data_ptr_(other.observe_data_ptr_), observe_cb_(other.observe_cb_) {
        if (observe_cb_) {
            observe_cb_->IncrementWeak();
        }
    }

    template <typename Y>
    WeakPtr(const WeakPtr<Y>& other)
        : observe_data_ptr_(other.observe_data_ptr_), observe_cb_(other.observe_cb_) {
        if (observe_cb_) {
            observe_cb_->IncrementWeak();
        }
    }
    WeakPtr(WeakPtr&& other)
        : observe_data_ptr_(other.observe_data_ptr_), observe_cb_(other.observe_cb_) {
        other.observe_data_ptr_ = nullptr;
        other.observe_cb_ = nullptr;
    }

    // Demote `SharedPtr`
    // #2 from https://en.cppreference.com/w/cpp/memory/weak_ptr/weak_ptr
    WeakPtr(const SharedPtr<T>& other)
        : observe_data_ptr_(other.data_ptr_), observe_cb_(other.control_block_) {
        if (observe_cb_) {
            observe_cb_->IncrementWeak();
        }
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    WeakPtr& operator=(const WeakPtr& other) {
        if (this == &other) {
            return *this;
        }

        if (observe_cb_) {
            observe_cb_->DecrementWeak();
        }

        observe_data_ptr_ = other.observe_data_ptr_;
        observe_cb_ = other.observe_cb_;

        if (observe_cb_) {
            observe_cb_->IncrementWeak();
        }
        return *this;
    }
    WeakPtr& operator=(WeakPtr&& other) {
        if (&other == this) {
            return *this;
        }

        if (observe_cb_) {
            observe_cb_->DecrementWeak();
        }

        observe_data_ptr_ = other.observe_data_ptr_;
        observe_cb_ = other.observe_cb_;
        other.observe_data_ptr_ = nullptr;
        other.observe_cb_ = nullptr;
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~WeakPtr() {
        if (observe_cb_) {
            observe_cb_->DecrementWeak();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Reset() {
        if (observe_cb_) {
            observe_cb_->DecrementWeak();
        }
        observe_data_ptr_ = nullptr;
        observe_cb_ = nullptr;
    }
    void Swap(WeakPtr& other) {
        std::swap(observe_cb_, other.observe_cb_);
        std::swap(observe_data_ptr_, other.observe_data_ptr_);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    size_t UseCount() const {
        return observe_cb_ ? observe_cb_->GetCount() : 0;
    }

    bool Expired() const {
        return UseCount() == 0;
    }

    SharedPtr<T> Lock() const {
        if (observe_cb_ && observe_cb_->GetCount() > 0) {
            observe_cb_->IncrementStrong();
            return SharedPtr<T>(observe_data_ptr_, observe_cb_);
        } else {
            return SharedPtr<T>();
        }
    }
};

template <typename T>
SharedPtr<T>::SharedPtr(const WeakPtr<T>& other)
    : data_ptr_(other.observe_data_ptr_), control_block_(other.observe_cb_) {
    if (!other.observe_cb_ || other.observe_cb_->GetCount() == 0) {
        throw BadWeakPtr();
    }
    control_block_->IncrementStrong();
}
