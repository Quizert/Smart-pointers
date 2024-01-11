#include <cstddef>
#include <utility>

class SimpleCounter {
public:
    size_t IncRef() {
        return ++count_;
    }

    size_t DecRef() {
        if (count_ > 0) {
            return --count_;
        }
        return 0;
    }

    size_t RefCount() const {
        return count_;
    }

private:
    size_t count_ = 0;
};

struct DefaultDelete {
    template <typename T>
    static void Destroy(T* object) {
        delete object;
    }
};

template <typename Derived, typename Counter, typename Deleter>
class RefCounted {
public:
    // Increase reference counter.
    void IncRef() {
        counter_.IncRef();
    }

    // Decrease reference counter.
    // Destroy object using Deleter when the last instance dies.
    void DecRef() {
        if (counter_.DecRef() == 0) {
            Deleter::Destroy(static_cast<Derived*>(this));
        }
    }

    // Get current counter value (the number of strong references).
    size_t RefCount() const {
        return counter_.RefCount();
    }

private:
    Counter counter_;
};

template <typename Derived, typename D = DefaultDelete>
using SimpleRefCounted = RefCounted<Derived, SimpleCounter, D>;

template <typename T>
class IntrusivePtr {
    template <typename Y>
    friend class IntrusivePtr;

public:
    // Constructors
    IntrusivePtr() = default;

    IntrusivePtr(std::nullptr_t) : IntrusivePtr() {
    }

    IntrusivePtr(T* data_ptr) : data_ptr_(data_ptr) {
        if (data_ptr) {
            data_ptr->IncRef();
        }
    }

    IntrusivePtr(const IntrusivePtr& other) : data_ptr_(other.data_ptr_) {
        if (data_ptr_) {
            data_ptr_->IncRef();
        }
    }

    template <typename Y>
    IntrusivePtr(const IntrusivePtr<Y>& other) : data_ptr_(other.Get()) {
        if (data_ptr_) {
            data_ptr_->IncRef();
        }
    }

    IntrusivePtr(IntrusivePtr&& other) : data_ptr_(other.data_ptr_) {
        other.data_ptr_ = nullptr;
    }

    template <typename Y>
    IntrusivePtr(IntrusivePtr<Y>&& other) : data_ptr_(other.Get()) {
        other.data_ptr_ = nullptr;
    }

    // `operator=`-s
    IntrusivePtr& operator=(const IntrusivePtr& other) {
        if (&other == this) {
            return *this;
        }

        if (data_ptr_) {
            data_ptr_->DecRef();
        }

        data_ptr_ = other.data_ptr_;

        if (data_ptr_) {
            data_ptr_->IncRef();
        }
        return *this;
    }

    IntrusivePtr& operator=(IntrusivePtr&& other) {
        if (&other == this) {
            return *this;
        }
        if (data_ptr_) {
            data_ptr_->DecRef();
        }
        data_ptr_ = other.data_ptr_;
        other.data_ptr_ = nullptr;
        return *this;
    }

    // Destructor
    ~IntrusivePtr() {
        if (data_ptr_) {
            data_ptr_->DecRef();
        }
    }

    // Modifiers
    void Reset() {
        if (data_ptr_) {
            data_ptr_->DecRef();
            data_ptr_ = nullptr;
        }
    }

    void Reset(T* new_ptr) {
        if (data_ptr_ != new_ptr) {
            if (data_ptr_) {
                data_ptr_->DecRef();
            }
            data_ptr_ = new_ptr;
            if (data_ptr_) {
                data_ptr_->IncRef();
            }
        }
    }

    void Swap(IntrusivePtr& other) {
        std::swap(data_ptr_, other.data_ptr_);
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
        return data_ptr_ ? data_ptr_->RefCount() : 0;
    }

    explicit operator bool() const {
        return data_ptr_ != nullptr;
    }

private:
    T* data_ptr_ = nullptr;
};

template <typename T, typename... Args>
IntrusivePtr<T> MakeIntrusive(Args&&... args) {
    return IntrusivePtr<T>(new T(std::forward<Args>(args)...));
}
