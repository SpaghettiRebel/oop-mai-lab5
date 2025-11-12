#pragma once

#include <memory_resource>
#include <memory>
#include <cstddef>
#include <iterator>
#include <type_traits>

template <typename T>
class PmrQueue {
public:
    using value_type = T;
    using allocator_type = std::pmr::polymorphic_allocator<T>;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    explicit PmrQueue(size_type initial_capacity = 16,
                      std::pmr::memory_resource* mr = std::pmr::get_default_resource());

    PmrQueue(const PmrQueue& other);
    PmrQueue& operator=(const PmrQueue& other);
    PmrQueue(PmrQueue&& other) noexcept;
    PmrQueue& operator=(PmrQueue&& other) noexcept;
    ~PmrQueue();

    void push(const T& value);
    void push(T&& value);
    template <typename... Args>
    void emplace(Args&&... args);

    void pop();
    T& front();
    const T& front() const;
    T& back();
    const T& back() const;

    bool empty() const noexcept;
    size_type size() const noexcept;
    size_type capacity() const noexcept;
    void clear() noexcept;
    void swap(PmrQueue& other) noexcept;

    std::pmr::memory_resource* memory_resource() const noexcept;

    class iterator;
    class const_iterator;

    iterator begin() noexcept;
    iterator end() noexcept;
    const_iterator begin() const noexcept;
    const_iterator end() const noexcept;
    const_iterator cbegin() const noexcept;
    const_iterator cend() const noexcept;

private:
    friend class iterator;
    friend class const_iterator;

    allocator_type alloc_;
    T* buffer_;
    size_type capacity_;
    size_type head_;  
    size_type count_; 

    size_type physical_index(size_type logical_index) const noexcept;
    void ensure_capacity_for_one_more();
    void reserve(size_type new_cap);
    void reallocate_and_move(size_type new_capacity);
    void clear_and_deallocate() noexcept;
    T& element_at(size_type logical_index);
    const T& element_at(size_type logical_index) const;
};


template <typename T>
class PmrQueue<T>::iterator {
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = PmrQueue::value_type;
    using difference_type = PmrQueue::difference_type;
    using pointer = value_type*;
    using reference = value_type&;

    iterator() noexcept : parent_(nullptr), index_(0) {}
    reference operator*() const { return parent_->element_at(index_); }
    pointer operator->() const { return &parent_->element_at(index_); }
    iterator& operator++() { ++index_; return *this; }           
    iterator operator++(int) { iterator tmp = *this; ++*this; return tmp; }
    bool operator==(const iterator& o) const noexcept { return parent_ == o.parent_ && index_ == o.index_; }
    bool operator!=(const iterator& o) const noexcept { return !(*this == o); }

private:
    friend class PmrQueue;
    explicit iterator(PmrQueue* parent, size_type idx) noexcept : parent_(parent), index_(idx) {}
    PmrQueue* parent_;
    size_type index_;
};


template <typename T>
class PmrQueue<T>::const_iterator {
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = PmrQueue::value_type;
    using difference_type = PmrQueue::difference_type;
    using pointer = const value_type*;
    using reference = const value_type&;

    const_iterator() noexcept : parent_(nullptr), index_(0) {}
    reference operator*() const { return parent_->element_at(index_); }
    pointer operator->() const { return &parent_->element_at(index_); }
    const_iterator& operator++() { ++index_; return *this; }
    const_iterator operator++(int) { const_iterator tmp = *this; ++*this; return tmp; }
    bool operator==(const const_iterator& o) const noexcept { return parent_ == o.parent_ && index_ == o.index_; }
    bool operator!=(const const_iterator& o) const noexcept { return !(*this == o); }

private:
    friend class PmrQueue;
    explicit const_iterator(const PmrQueue* parent, size_type idx) noexcept : parent_(parent), index_(idx) {}
    const PmrQueue* parent_;
    size_type index_;
};

#include "queue.tpp"
