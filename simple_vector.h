#pragma once

#include "array_ptr.h"

#include <cassert>
#include <initializer_list>
#include <stdexcept>
#include <array>
#include <utility>
#include <iostream>

class ReserveProxyObj {
public:
    ReserveProxyObj(int capacity) {
        capacity_ = capacity;
    }

    size_t GetCapacity() {
        return capacity_;
    }

private:
    size_t capacity_ = 0;
};

template <typename Type>
class SimpleVector {
public:
    using Iterator = Type*;
    using ConstIterator = const Type*;
    using Array = ArrayPtr<Type>;

    SimpleVector() noexcept = default;

    // Creates vector of another vector
    SimpleVector(SimpleVector&& other) noexcept {
        SimpleVector tmp_vector;

        Array tmp_array(other.GetSize());
        std::copy(std::make_move_iterator(other.begin()), std::make_move_iterator(other.end()), tmp_array.Get());

        tmp_vector.size_ = std::exchange(other.size_, 0);
        tmp_vector.capacity_ = std::exchange(other.capacity_, 0);

        
        tmp_vector.array_.swap(tmp_array);
        Swap(tmp_vector);
    }

    // Creates vector of another vector's copy
    SimpleVector(const SimpleVector& other) {
        try {
            SimpleVector tmp_copy(other.size_);
            tmp_copy.size_ = other.size_;
            tmp_copy.capacity_ = other.capacity_;

            std::copy(other.begin(), other.end(), tmp_copy.array_.Get());
            this->Swap(tmp_copy);
        } catch (std::bad_alloc&) {
            throw;
        }
    }

    // Creates vector of size default items
    explicit SimpleVector(size_t size) {
        CreateArrayAndSetupSize(size);
    }

    // Creates vector of size elements which are equal to value
    SimpleVector(size_t size, const Type& value) {
        CreateArrayAndSetupSize(size);
        std::fill(begin(), end(), value);
    }

    // Creates vector of std::initializer_list
    SimpleVector(std::initializer_list<Type> init) {
        CreateArrayAndSetupSize(init.size());
        std::copy(init.begin(), init.end(), begin());
    }

    // Creates vector with required capacity
    SimpleVector(ReserveProxyObj input_class) {
        capacity_ = input_class.GetCapacity();
        
        Array tmp_array(GetCapacity());
        array_.swap(tmp_array);
    }

    SimpleVector& operator=(const SimpleVector& rhs){
        if (*this == rhs) {
            return *this;
        }
        SimpleVector tmp_copy(rhs.size_);
        tmp_copy.size_ = rhs.size_;
        tmp_copy.capacity_ = rhs.capacity_;

        std::copy(rhs.begin(), rhs.end(), tmp_copy.array_.Get());
        this->Swap(tmp_copy);

        return *this;
    }

    SimpleVector& operator=(SimpleVector&& rhs) noexcept {
        SimpleVector tmp_copy(rhs.size_);
        tmp_copy.size_ = std::exchange(rhs.size_, 0);
        tmp_copy.capacity_ = std::exchange(rhs.capacity_, 0);

        std::copy(std::make_move_iterator(rhs.begin()), std::make_move_iterator(rhs.end()), tmp_copy.array_.Get());
        this->Swap(tmp_copy);
        return *this;
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity <= GetCapacity()) {
            return;
        }

        Array new_array(new_capacity);
        std::copy(begin(), end(), new_array.Get());
        array_.swap(new_array);

        capacity_ = new_capacity;
    }

    // Moves item to the end
    // Doubles capacity if it is out of free space
   void PushBack(Type&& item) {
        if (GetSize() < GetCapacity()) {
            array_[GetSize()] = std::move(item);
            size_++;
            return;
        }

        if (GetCapacity() == 0) {
            Array new_array(1);
            new_array[0] = std::move(item);

            size_ = capacity_ = 1;
            array_.swap(new_array);
        } else {
            Array new_array(GetCapacity() * 2);
            std::copy(std::make_move_iterator(begin()), std::make_move_iterator(end()), new_array.Get());
            new_array[GetSize()] = std::move(item);

            capacity_ = GetCapacity() * 2;
            size_ = GetSize() + 1;
            array_.swap(new_array);
        }
    }

    // Copies item to the end
    // Doubles capacity if it out of free space
    void PushBack(const Type& item) {  
        PushBack(std::move(item));
    }

    // Inserts value at position pos
    // Returns iterator to inserted item
    // Doubles capacity if it out of free space
    // If capacity equals to 0, it must become 1
    Iterator Insert(ConstIterator pos, Type&& value) {
        Iterator position = const_cast<Iterator>(pos);
        size_t index = std::distance(begin(), position);

        if (index == GetSize()) {
            PushBack(std::move(value));
            return end() - 1;
        }

        if (GetSize() != GetCapacity()) {
            std::copy_backward(std::make_move_iterator(begin() + index), std::make_move_iterator(end()), end() + 1);
            size_++;
            array_[index] = std::move(value);
            return begin() + index;
        }

        if (GetCapacity() == 0) {
            Array new_array(1);
            new_array[0] = std::move(value);

            size_ = capacity_ = 1;
            array_.swap(new_array);

            return end() - 1;
        } else {
            Array new_array(GetCapacity() * 2);
            std::copy(std::make_move_iterator(begin()), std::make_move_iterator(begin() + index), new_array.Get());
            new_array[index] = std::move(value);
            std::copy(std::make_move_iterator(begin() + index), std::make_move_iterator(end()), new_array.Get() + index + 1);

            capacity_ = GetCapacity() * 2;
            size_ = GetSize() + 1;
            array_.swap(new_array);

            return begin() + index;
        }
    }

    // Inserts value at position pos
    // Returns iterator to inserted item
    // Doubles capacity if it out of free space
    // If capacity equals to 0, it must become 1
    Iterator Insert(ConstIterator pos, const Type& value) {
        Insert(pos, std::move(value));
    }

    // Kinda delete the last elemet
    // Vector must not be empty
    void PopBack() noexcept {
        if (GetSize() > 0) {
            size_--;
        }
    }

    // Deleted item at position pos
    Iterator Erase(ConstIterator pos) {
        Iterator position = const_cast<Iterator>(pos);
        size_t index = std::distance(begin(), position);

        if (index == GetSize() - 1) {
            PopBack();
            return end();
        }

        std::copy(std::make_move_iterator(begin() + index + 1), std::make_move_iterator(end()), begin() + index);
        size_--;

        return begin() + index;
    }

    // Swaps values with another vector
    void Swap(SimpleVector& other) noexcept {
        size_t tmp_size = GetSize();
        size_t tmp_capacity = GetCapacity();

        size_ = other.GetSize();
        capacity_ = other.GetCapacity();

        other.size_ = tmp_size;
        other.capacity_ = tmp_capacity;

        array_.swap(other.array_);
    }

    // Returns count of items
    size_t GetSize() const noexcept {
        return size_;
    }

    // Return vector's capacity
    size_t GetCapacity() const noexcept {
        return capacity_;
    }

    // Tells if vector is empty
    bool IsEmpty() const noexcept {
        return (GetSize() == 0);
    }

    // Returns link to the item with index "index"
    Type& operator[](size_t index) noexcept {
        return array_[index];
    }

    // Returns constant link to the item with index "index"
    const Type& operator[](size_t index) const noexcept {
        return array_[index];
    }

    // Returns constant link to the item with index "index"
    // Throws std::out_of_range exception if index >= size
    Type& At(size_t index) {
        if (index >= GetSize()) {
            throw std::out_of_range("");
        }
        return array_[index];
    }

    // Returns constant link to the item with index "index"
    // Throws std::out_of_range exception if index >= size
    const Type& At(size_t index) const {
        if (index >= GetSize()) {
            throw std::out_of_range("");
        }
        return array_[index];
    }

    // Zeros vector's size, capacity is not changing
    void Clear() noexcept {
        size_ = 0;
    }

    // Chages vector's size
    // When incresing the size, new elements get default Type value
    void Resize(size_t new_size) {
        if (new_size == GetSize()) {
            return;
        } else if (new_size < GetSize()) {
            size_ = new_size;
        } else {
            try {
                Array new_array(new_size);
                Type default_item = Type();
                std::fill(new_array.Get(), new_array.Get() + new_size, default_item);
                std::copy(begin(), end(), new_array.Get());
                
                size_ = capacity_ = new_size;
                array_.swap(new_array);
            } catch (std::bad_alloc&) {
                throw;
            }
        }
    }

    // Returns iterator to vector's beginning
    Iterator begin() noexcept {
        return array_.Get();
    }

    // Returns iterator to the after last element
    Iterator end() noexcept {
        return (array_.Get() + GetSize());
    }

    // Returns constant iterator to vector's beginning
    ConstIterator begin() const noexcept {
        return cbegin();
    }

    // Returns constant iterator to the after last element
    ConstIterator end() const noexcept {
        return cend();
    }

    // Returns constant iterator to vector's beginning
    ConstIterator cbegin() const noexcept {
        return array_.Get();
    }

    // Returns constant iterator to the after last element
    ConstIterator cend() const noexcept {
        return (array_.Get() + GetSize());
    }

private:
    Array array_;
    size_t capacity_ = 0;
    size_t size_ = 0;

    void CreateArrayAndSetupSize(size_t size) {
        if (size == 0) {
            return;
        }
        try {
            Type default_item = Type();
            Array tmp_array(size);
            std::fill(tmp_array.Get(), tmp_array.Get() + size, default_item);
            size_ = capacity_ = size;

            array_.swap(tmp_array);
        } catch (std::bad_alloc&) {
            throw;
        }
    }
};

template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    if (lhs.GetSize() != rhs.GetSize()) {
        return false;
    }
    for (size_t i = 0; i < lhs.GetSize(); ++i) {
        if (lhs.At(i) != rhs.At(i)) {
            return false;
        }
    }
    return true;
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs == rhs);
}

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return (lhs < rhs || lhs == rhs);
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs <= rhs);
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return (lhs > rhs || lhs == rhs);
}

ReserveProxyObj Reserve(size_t capacity_to_reserve) {
    return ReserveProxyObj(capacity_to_reserve);
}