#ifndef HASH_SET_SEQUENTIAL_H
#define HASH_SET_SEQUENTIAL_H

#include <cassert>
#include <functional>
#include <set>
#include <vector>

#include "src/hash_set_base.h"

template<typename T>
class HashSetSequential : public HashSetBase<T> {
public:
    explicit HashSetSequential(size_t initial_capacity) : capacity_(initial_capacity) {
        table_ = std::vector<std::set<T>>(initial_capacity);
        size_ = 0;
    }

    explicit HashSetSequential() : capacity_(16) {
        table_ = std::vector<std::set<T>>(16);
        size_ = 0;
    }

    bool Add(T elem) override {
        size_t bucketNum = std::hash<T>()(elem) % capacity_;
        std::set<T> bucket = table_[bucketNum];
        auto result = bucket.insert(elem);
        size_ += result.second ? 1 : 0;
        if (policy()) {
            resize();
        }

        return result.second;
    }

    bool Remove(T elem) override {
        size_t bucketNum = std::hash<T>()(elem) % capacity_;
        std::set<T> bucket = table_[bucketNum];

        bool result = bucket.erase(elem);
        size_ -= result ? 1 : 0;

        return result;
    }

    [[nodiscard]] bool Contains(T elem) final override{
        size_t bucketNum = std::hash<T>()(elem) % capacity_;
        std::set<T> bucket = table_[bucketNum];

        return bucket.find(elem) != bucket.end();
    }

    [[nodiscard]] size_t Size() const final override{
        return size_;
    }

private:
    bool policy() {
        return size_ / table_.size() > 4;
    }

    void resize() {
        int old_capacity = table_.size();
        // This assignment creates a copy of the table.
        // Play with references here
        std::vector<std::set<T>> old_table = table_;
        table_ = std::vector<std::set<T>>(capacity_);
        for (int i = 0; i < old_capacity; i++) {
            std::set<T> bucket = old_table[i];
            for (auto& elem : bucket) {
                size_t new_bucket_num = bucket[std::hash<T>()(elem) % table_.size()];
                table_[new_bucket_num].insert(elem);
            }
        }
    }

private:
    std::vector<std::set<T>> table_;
    size_t capacity_;
    size_t size_;

};

#endif  // HASH_SET_SEQUENTIAL_H
