#include <algorithm>
#include <stdexcept>
#include <utility>
#include <new>
#include <type_traits>

template <typename T>
PmrQueue<T>::PmrQueue(size_type initial_capacity, std::pmr::memory_resource* mr)
    : alloc_(mr), buffer_(nullptr), capacity_(0), head_(0), count_(0)
{
    if (initial_capacity == 0) initial_capacity = 1;
    reserve(initial_capacity);
}

template <typename T>
PmrQueue<T>::PmrQueue(const PmrQueue& other)
    : alloc_(other.alloc_.resource()), buffer_(nullptr), capacity_(0), head_(0), count_(0)
{
    if (other.capacity_ > 0) {
        reserve(other.capacity_);
        size_type i = 0;
        
        try {
            for (; i < other.count_; ++i) {
                const T& src = other.element_at(i);
                std::allocator_traits<allocator_type>::construct(alloc_, buffer_ + i, src);
            }
        } catch (...) {
            for (size_type j = 0; j < i; ++j)
                std::allocator_traits<allocator_type>::destroy(alloc_, buffer_ + j);
            std::allocator_traits<allocator_type>::deallocate(alloc_, buffer_, capacity_);
            buffer_ = nullptr;
            capacity_ = 0;
            throw;
        }
        count_ = other.count_;
        head_ = 0;
    }
}

template <typename T>
PmrQueue<T>& PmrQueue<T>::operator=(const PmrQueue& other) {
    if (this == &other) return *this;
    PmrQueue tmp(other);
    swap(tmp);
    return *this;
}

template <typename T>
PmrQueue<T>::PmrQueue(PmrQueue&& other) noexcept
    : alloc_(other.alloc_), buffer_(other.buffer_), capacity_(other.capacity_),
      head_(other.head_), count_(other.count_)
{
    other.buffer_ = nullptr;
    other.capacity_ = 0;
    other.head_ = other.count_ = 0;
}

template <typename T>
PmrQueue<T>& PmrQueue<T>::operator=(PmrQueue&& other) noexcept {
    if (this == &other) return *this;
    clear_and_deallocate();
    alloc_ = other.alloc_;
    buffer_ = other.buffer_;
    capacity_ = other.capacity_;
    head_ = other.head_;
    count_ = other.count_;
    other.buffer_ = nullptr;
    other.capacity_ = 0;
    other.head_ = other.count_ = 0;
    return *this;
}

template <typename T>
PmrQueue<T>::~PmrQueue() {
    clear_and_deallocate();
}

template <typename T>
void PmrQueue<T>::push(const T& value) {
    ensure_capacity_for_one_more();
    size_type pos = physical_index(count_);
    std::allocator_traits<allocator_type>::construct(alloc_, buffer_ + pos, value);
    ++count_;
}

template <typename T>
void PmrQueue<T>::push(T&& value) {
    ensure_capacity_for_one_more();
    size_type pos = physical_index(count_);
    std::allocator_traits<allocator_type>::construct(alloc_, buffer_ + pos, std::move(value));
    ++count_;
}

template <typename T>
template <typename... Args>
void PmrQueue<T>::emplace(Args&&... args) {
    ensure_capacity_for_one_more();
    size_type pos = physical_index(count_);
    std::allocator_traits<allocator_type>::construct(alloc_, buffer_ + pos, std::forward<Args>(args)...);
    ++count_;
}

template <typename T>
void PmrQueue<T>::pop() {
    if (empty()) throw std::out_of_range("pop from empty queue");
    std::allocator_traits<allocator_type>::destroy(alloc_, buffer_ + head_);
    head_ = (head_ + 1) % capacity_;
    --count_;
}

template <typename T>
T& PmrQueue<T>::front() {
    if (empty()) throw std::out_of_range("front on empty queue");
    return buffer_[head_];
}
template <typename T>
const T& PmrQueue<T>::front() const {
    if (empty()) throw std::out_of_range("front on empty queue");
    return buffer_[head_];
}
template <typename T>
T& PmrQueue<T>::back() {
    if (empty()) throw std::out_of_range("back on empty queue");
    return buffer_[physical_index(count_ - 1)];
}
template <typename T>
const T& PmrQueue<T>::back() const {
    if (empty()) throw std::out_of_range("back on empty queue");
    return buffer_[physical_index(count_ - 1)];
}

template <typename T>
bool PmrQueue<T>::empty() const noexcept { return count_ == 0; }

template <typename T>
typename PmrQueue<T>::size_type PmrQueue<T>::size() const noexcept { return count_; }

template <typename T>
typename PmrQueue<T>::size_type PmrQueue<T>::capacity() const noexcept { return capacity_; }

template <typename T>
void PmrQueue<T>::clear() noexcept {
    for (size_type i = 0; i < count_; ++i) {
        std::allocator_traits<allocator_type>::destroy(alloc_, buffer_ + physical_index(i));
    }
    head_ = 0;
    count_ = 0;
}

template <typename T>
void PmrQueue<T>::swap(PmrQueue& other) noexcept {
    using std::swap;
    swap(alloc_, other.alloc_);
    swap(buffer_, other.buffer_);
    swap(capacity_, other.capacity_);
    swap(head_, other.head_);
    swap(count_, other.count_);
}

template <typename T>
std::pmr::memory_resource* PmrQueue<T>::memory_resource() const noexcept {
    return alloc_.resource();
}

template <typename T>
typename PmrQueue<T>::iterator PmrQueue<T>::begin() noexcept {
    return iterator(this, 0);
}
template <typename T>
typename PmrQueue<T>::iterator PmrQueue<T>::end() noexcept {
    return iterator(this, count_);
}
template <typename T>
typename PmrQueue<T>::const_iterator PmrQueue<T>::begin() const noexcept {
    return const_iterator(this, 0);
}
template <typename T>
typename PmrQueue<T>::const_iterator PmrQueue<T>::end() const noexcept {
    return const_iterator(this, count_);
}
template <typename T>
typename PmrQueue<T>::const_iterator PmrQueue<T>::cbegin() const noexcept {
    return const_iterator(this, 0);
}
template <typename T>
typename PmrQueue<T>::const_iterator PmrQueue<T>::cend() const noexcept {
    return const_iterator(this, count_);
}


template <typename T>
typename PmrQueue<T>::size_type PmrQueue<T>::physical_index(size_type logical_index) const noexcept {
    return (head_ + logical_index) % capacity_;
}

template <typename T>
void PmrQueue<T>::ensure_capacity_for_one_more() {
    if (count_ < capacity_) return;
    size_type new_cap = std::max<size_type>(1, capacity_ * 2);
    reallocate_and_move(new_cap);
}

template <typename T>
void PmrQueue<T>::reserve(size_type new_cap) {
    if (new_cap <= capacity_) return;
    reallocate_and_move(new_cap);
}

template <typename T>
void PmrQueue<T>::reallocate_and_move(size_type new_capacity) {
    T* new_buf = std::allocator_traits<allocator_type>::allocate(alloc_, new_capacity);
    size_type constructed = 0;

    try {
        for (size_type i = 0; i < count_; ++i) {
            T& src = element_at(i);
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::allocator_traits<allocator_type>::construct(alloc_, new_buf + i, std::move(src));
            } else {
                std::allocator_traits<allocator_type>::construct(alloc_, new_buf + i, src);
            }
            ++constructed;
        }
    } catch (...) {
        for (size_type j = 0; j < constructed; ++j)
            std::allocator_traits<allocator_type>::destroy(alloc_, new_buf + j);
        std::allocator_traits<allocator_type>::deallocate(alloc_, new_buf, new_capacity);
        throw;
    }

    for (size_type i = 0; i < count_; ++i) {
        std::allocator_traits<allocator_type>::destroy(alloc_, buffer_ + physical_index(i));
    }
    if (buffer_) {
        std::allocator_traits<allocator_type>::deallocate(alloc_, buffer_, capacity_);
    }

    buffer_ = new_buf;
    capacity_ = new_capacity;
    head_ = 0;
}

template <typename T>
T& PmrQueue<T>::element_at(size_type logical_index) {
    return buffer_[physical_index(logical_index)];
}
template <typename T>
const T& PmrQueue<T>::element_at(size_type logical_index) const {
    return buffer_[physical_index(logical_index)];
}

template <typename T>
void PmrQueue<T>::clear_and_deallocate() noexcept {
    if (!buffer_) return;

    for (size_type i = 0; i < count_; ++i) {
        std::allocator_traits<allocator_type>::destroy(alloc_, buffer_ + physical_index(i));
    }

    std::allocator_traits<allocator_type>::deallocate(alloc_, buffer_, capacity_);
    buffer_ = nullptr;
    capacity_ = 0;
    head_ = 0;
    count_ = 0;
}
