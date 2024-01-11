#pragma once

#include "sw_fwd.h"  // Forward declaration

#include <cstddef>  // std::nullptr_t

// https://en.cppreference.com/w/cpp/memory/shared_ptr

class ControlBlockBase {
public:
    ControlBlockBase() = default;

    size_t GetCount() const {
        return ref_count_;
    }

    void IncrementStrong() {
        ++ref_count_;
    }

    void DecrementStrong() {
        if (--ref_count_ == 0) {
            OnZeroReferences();
            if (weak_count_ == 0) {
                delete this;
            }
        }
    }

    void IncrementWeak() {
        ++weak_count_;
    }

    void DecrementWeak() {
        if (--weak_count_ == 0 && ref_count_ == 0) {
            delete this;
        }
    }

    virtual ~ControlBlockBase() = default;

protected:
    virtual void OnZeroReferences() = 0;

private:
    size_t ref_count_ = 1;
    size_t weak_count_ = 0;
};

template <typename T>
class PtrControlBlock : public ControlBlockBase {
public:
    explicit PtrControlBlock(T* ptr) : data_ptr_(ptr) {
    }

protected:
    void OnZeroReferences() override {
        delete data_ptr_;
    }

private:
    T* data_ptr_;
};

template <typename T>
class ObjectControlBlock : public ControlBlockBase {
public:
    template <typename... Args>
    explicit ObjectControlBlock(Args&&... args) {
        new (GetDataPtr()) T(std::forward<Args>(args)...);
    }

    T* GetDataPtr() {
        return reinterpret_cast<T*>(&storage_);
    }

protected:
    void OnZeroReferences() override {
        GetDataPtr()->~T();
    }

private:
    std::aligned_storage_t<sizeof(T), alignof(T)> storage_;
};

template <typename T>
class SharedPtr {
private:
    T* data_ptr_ = nullptr;
    ControlBlockBase* control_block_ = nullptr;

    template <typename Y>
    friend class WeakPtr;

    template <typename Y>
    friend class SharedPtr;

public:
    template <typename Y, typename... Args>
    friend SharedPtr<Y> MakeShared(Args&&... args);
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors
    SharedPtr() = default;

    SharedPtr(std::nullptr_t) : SharedPtr() {
    }

    explicit SharedPtr(T* ptr) : data_ptr_(ptr), control_block_(new PtrControlBlock<T>(ptr)) {
    }

    template <typename Y>
    SharedPtr(Y* ptr) : data_ptr_(ptr), control_block_(new PtrControlBlock<Y>(ptr)) {
    }

    template <typename Y>
    SharedPtr(const SharedPtr<Y>& other)
        : data_ptr_(other.data_ptr_), control_block_(other.control_block_) {
        if (control_block_) {
            control_block_->IncrementStrong();
        }
    }

    SharedPtr(const SharedPtr& other)
        : data_ptr_(other.data_ptr_), control_block_(other.control_block_) {
        if (control_block_) {
            control_block_->IncrementStrong();
        }
    }

    SharedPtr(SharedPtr&& other)
        : data_ptr_(other.data_ptr_), control_block_(other.control_block_) {
        other.data_ptr_ = nullptr;
        other.control_block_ = nullptr;
    }

    template <typename Y>
    SharedPtr(SharedPtr<Y>&& other)
        : data_ptr_(other.data_ptr_), control_block_(other.control_block_) {
        other.data_ptr_ = nullptr;
        other.control_block_ = nullptr;
    }

    SharedPtr(T* ptr, ControlBlockBase* block) : data_ptr_(ptr), control_block_(block) {
    }
    // Aliasing constructor
    // #8 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr

    template <typename Y>
    SharedPtr(const SharedPtr<Y>& other, T* ptr)
        : data_ptr_(ptr), control_block_(other.control_block_) {
        if (control_block_) {
            control_block_->IncrementStrong();
        }
    }
    // Promote `WeakPtr`
    // #11 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    explicit SharedPtr(const WeakPtr<T>& other);

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    SharedPtr& operator=(const SharedPtr& other) {
        if (&other == this) {
            return *this;
        }

        if (control_block_) {
            control_block_->DecrementStrong();
        }

        data_ptr_ = other.data_ptr_;
        control_block_ = other.control_block_;

        if (control_block_) {
            control_block_->IncrementStrong();
        }

        return *this;
    }

    SharedPtr& operator=(SharedPtr&& other) {
        if (&other == this) {
            return *this;
        }

        if (control_block_) {
            control_block_->DecrementStrong();
        }

        data_ptr_ = other.data_ptr_;
        control_block_ = other.control_block_;
        other.data_ptr_ = nullptr;
        other.control_block_ = nullptr;

        return *this;
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor
    ~SharedPtr() {
        if (control_block_) {
            control_block_->DecrementStrong();
        }
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Reset() {
        if (control_block_) {
            control_block_->DecrementStrong();
        }

        data_ptr_ = nullptr;
        control_block_ = nullptr;
    }

    template <typename Y>
    void Reset(Y* ptr) {
        if (data_ptr_ == ptr) {
            return;
        }

        if (control_block_) {
            control_block_->DecrementStrong();
        }

        if (ptr) {
            data_ptr_ = ptr;
            control_block_ = new PtrControlBlock<Y>(ptr);
        } else {
            data_ptr_ = nullptr;
            control_block_ = nullptr;
        }
    }

    void Swap(SharedPtr& other) {
        std::swap(data_ptr_, other.data_ptr_);
        std::swap(control_block_, other.control_block_);
    }

    // Observers
    T* Get() const {
        return data_ptr_;
    }

    T& operator*() const {
        return *data_ptr_;
    }

    T* operator->() const {
        return data_ptr_;
    }

    size_t UseCount() const {
        return control_block_ ? control_block_->GetCount() : 0;
    }

    explicit operator bool() const {
        return data_ptr_ != nullptr;
    }
};

template <typename T, typename U>
inline bool operator==(const SharedPtr<T>& left, const SharedPtr<U>& right) {
    return left.Get() == right.Get();
}

// Allocate memory only once
template <typename T, typename... Args>
SharedPtr<T> MakeShared(Args&&... args) {
    auto* control_block = new ObjectControlBlock<T>(std::forward<Args>(args)...);
    return SharedPtr<T>(control_block->GetDataPtr(), control_block);
}
// Look for usage examples in tests
template <typename T>
class EnableSharedFromThis {
public:
    SharedPtr<T> SharedFromThis();
    SharedPtr<const T> SharedFromThis() const;

    WeakPtr<T> WeakFromThis() noexcept;
    WeakPtr<const T> WeakFromThis() const noexcept;
};
