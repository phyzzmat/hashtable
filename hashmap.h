#include<iostream>
#include<vector>
#include<stdexcept>
#include<list>

template<class KeyType, class ValueType> class Iter;
template<class KeyType, class ValueType> class ConstIter;

// HashMap with separate chaining.
// Operates using two containers:
// - the chain list, where chains are implemented with std::list<> and contain indices of data entries in main storage;
// - the main data storage, implemented with std::vector<std::pair<>>.
// Resize policy:
// - if the number of elements exceeds current capacity, capacity is increased to double the original plus one;
// - if the capacity exceeds double the number of elements multiplied by some constant, the size is reduced by half.

template<class KeyType, class ValueType, class Hash = std::hash<KeyType> >
class HashMap {
  public:
    typedef Iter<KeyType, ValueType> iterator;
    typedef ConstIter<KeyType, ValueType> const_iterator;

    HashMap(const Hash &hasher = Hash()) : hasher_(hasher) {}

    // Constructor from two iterators
    template<class Iterator>
    HashMap(Iterator first, Iterator last, const Hash &h = Hash()) {
        hasher_ = h;
        while (first != last) {
            insert(*first);
            ++first;
        };
    };

    // Constructor from initializer list
    HashMap(std::initializer_list<std::pair<KeyType, ValueType>> init_list, const Hash &h = Hash()) {
        hasher_ = h;
        for (auto element : init_list) insert(element);
    };

    // O(1)
    iterator begin() {
        if (hashmap_.empty()) return iterator(nullptr);
        return iterator(&hashmap_[0]);
    }

    // O(1)
    iterator end() {
        if (hashmap_.empty()) return iterator(nullptr);
        return iterator(&hashmap_[0] + stored_elements);
    }

    // O(1)
    const_iterator begin() const {
        if (hashmap_.empty()) return const_iterator(nullptr);
        return const_iterator(&hashmap_[0]);
    }

    // O(1)
    const_iterator end() const {
        if (hashmap_.empty()) return const_iterator(nullptr);
        return const_iterator(&hashmap_[0] + stored_elements);
    }

    // Returns non-const iterator to key if it exists, end() otherwise.
    // Time complexity: amortized O(1).
    iterator find(KeyType key) {
        if (capacity == 0) return end();
        int candidate_position = hasher_(key) % capacity;
        for (auto element : indices_[candidate_position]) {
            if (hashmap_[element].first == key) {
                return iterator(&hashmap_[0] + element);
            }
        }
        return end();
    }

    // Returns const iterator to key if it exists, end() otherwise.
    // Time complexity: amortized O(1), individual query expected O(1), provided the hash function is good enough.
    const_iterator find(KeyType key) const {
        if (capacity == 0) return end();
        int candidate_position = hasher_(key) % capacity;
        for (auto element : indices_[candidate_position]) {
            if (hashmap_[element].first == key) {
                return const_iterator(&hashmap_[0] + element);
            }
        }
        return end();
    }

    // Empties the hashmap.
    // Time complexity: O(n), where n is the number of elements contained inside the hashmap.
    void clear() {
        capacity = 0;
        stored_elements = 0;
        hashmap_.clear();
        indices_.clear();
        indices_.resize(1);
    }

    // Inserts an element in case there does not already exist one with the same key.
    // Time complexity: amortized O(1), individual query O(n).
    void insert(std::pair<KeyType, ValueType> x) {
        if (find(x.first) != end()) {
            return;
        }
        if (stored_elements >= capacity) {
            capacity = capacity * kScalingFactor + 1;
            rebuild();
        }
        ++stored_elements;
        int pos = hasher_(x.first) % capacity;
        indices_[pos].push_back(stored_elements - 1);
        hashmap_.push_back(x);
    }

    // Erases an element by key if there exists one, otherwise does nothing.
    // Time complexity: amortized O(1), individual query O(n).
    void erase(KeyType key) {
        if (find(key) == end()) {
            return;
        }
        std::pair<KeyType, ValueType> last_element = hashmap_.back();
        if (last_element.first == key) {
            hashmap_.pop_back();
            size_t chain_storage_location = hasher_(last_element.first) % capacity;
            --stored_elements;
            auto it = indices_[chain_storage_location].begin();
            while (*it != stored_elements) {
                ++it;
            }
            indices_[chain_storage_location].erase(it);
        } else {
            int last_element_chain_storage_location = hasher_(last_element.first) % capacity;
            int chain_storage_location = hasher_(key) % capacity;
            size_t data_storage_location = 0;
            for (auto i : indices_[chain_storage_location]) {
                if (hashmap_[i].first == key) data_storage_location = i;
            }
            --stored_elements;
            auto it = indices_[last_element_chain_storage_location].begin();
            while (*it != stored_elements) {
                ++it;
            }
            *it = data_storage_location;
            it = indices_[chain_storage_location].begin();
            while (*it != data_storage_location) {
                ++it;
            }
            indices_[chain_storage_location].erase(it);
            hashmap_[data_storage_location] = last_element;
            hashmap_.pop_back();
        }
        if (stored_elements * kScalingFactor <= capacity) {
            capacity /= kScalingFactor;
            rebuild();
        }
    }

    // Returns number of elements contained inside the hashmap. O(1)
    size_t size() const {
        return stored_elements;
    }

    // Checks if the hashmap contains no elements. O(1)
    bool empty() const {
        return size() == 0;
    }

    // If there already exists an element with given key, returns a non-const iterator pointing to it.
    // Otherwise, inserts a new entry with this key and default value to the hashmap.
    // Time complexity: amortized O(1), individual query O(n).
    ValueType &operator[](KeyType key) {
        if (find(key) == end()) insert({key, ValueType()});
        return find(key)->second;
    }

    // If there exists an element with given key, returns a constant iterator pointing to it.
    // Otherwise, throws the std::out_of_range() exception.
    // Time complexity: amortized O(1), individual query expected O(1) provided the hash function is good enough.
    const ValueType &at(KeyType key) const {
        if (find(key) == end()) throw std::out_of_range("");
        return find(key)->second;
    }

    // O(1)
    Hash hash_function() const {
        return hasher_;
    }

  private:

    size_t capacity = 0, stored_elements = 0;
    // The minimum discrepancy between hashmap capacity and number of stored elements for it to be rebuilt
    static constexpr size_t kScalingFactor = 2;

    std::vector<std::list<size_t>> indices_;
    std::vector<std::pair<KeyType, ValueType>> hashmap_;
    Hash hasher_;

    // Rebuilds and resizes hashmap according to its new capacity.
    void rebuild() {
        std::vector<std::pair<KeyType, ValueType>> old_hashmap;
        old_hashmap = hashmap_;
        stored_elements = 0;
        hashmap_.clear();
        indices_.clear();
        indices_.resize(capacity);
        for (auto element : old_hashmap) {
            insert(element);
        }
    }
};


// Iterator for HashMap
template<class KeyType, class ValueType>
class Iter {
public:
    Iter() = default;

    Iter(std::pair<KeyType, ValueType> *ptr) {
        iter = reinterpret_cast<std::pair<const KeyType, ValueType> *>(ptr);
    }

    std::pair<const KeyType, ValueType> operator*() {
        return *iter;
    }

    std::pair<const KeyType, ValueType> *operator->() {
        return iter;
    }

    bool operator==(Iter other) const {
        return iter == other.iter;
    }

    bool operator!=(Iter other) const {
        return iter != other.iter;
    }

    Iter operator++() {
        ++iter;
        return *this;
    }

    Iter operator++(int) {
        Iter copy_iter = *this;
        ++*this;
        return copy_iter;
    }

private:
    std::pair<const KeyType, ValueType> *iter;
};

// Constant iterator for HashMap
template<class KeyType, class ValueType>
class ConstIter {
public:
    ConstIter() = default;

    ConstIter(const std::pair<KeyType, ValueType> *ptr) {
        const_iter = reinterpret_cast<const std::pair<const KeyType, ValueType> *>(ptr);
    }

    const std::pair<const KeyType, ValueType> operator*() const {
        return *const_iter;
    }

    const std::pair<const KeyType, ValueType> *operator->() const {
        return const_iter;
    }

    bool operator==(ConstIter other) const {
        return const_iter == other.const_iter;
    }

    bool operator!=(ConstIter other) const {
        return const_iter != other.const_iter;
    }

    ConstIter operator++() {
        ++const_iter;
        return *this;
    }

    ConstIter operator++(int) {
        ConstIter copy_const_iter = *this;
        ++*this;
        return copy_const_iter;
    }

private:
    const std::pair<const KeyType, ValueType> *const_iter;
};
