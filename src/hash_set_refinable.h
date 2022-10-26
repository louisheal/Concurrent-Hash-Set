#ifndef HASH_SET_REFINABLE_H
#define HASH_SET_REFINABLE_H

#include <atomic>
#include <cassert>
#include <mutex>
#include <set>
#include <vector>
#include <shared_mutex>

#include "src/hash_set_base.h"

template<typename T>
class HashSetRefinable : public HashSetBase<T> {
public:
    explicit HashSetRefinable(size_t initial_capacity) : size_(0), table_(initial_capacity), locks_(std::make_unique<std::vector<std::mutex>>(initial_capacity)),
                                                         bucket_count_(initial_capacity) {}

    bool Add(T elem) final {
      read_write_lock_.lock_shared();
      size_t lock_num = std::hash<T>()(elem) % bucket_count_.load();
      (*locks_)[lock_num].lock();
      read_write_lock_.unlock();
      bool result = table_[lock_num].insert(elem).second;
      (*locks_)[lock_num].unlock();
      if (result) {
        size_.fetch_add(1);
      }
      if (policy()) {
        resize();
      }
      return result;
    }

    bool Remove(T elem) final {
      read_write_lock_.lock_shared();
      size_t lock_num = std::hash<T>()(elem) % bucket_count_.load();
      (*locks_)[lock_num].lock();
      read_write_lock_.unlock();
      bool result = table_[lock_num].erase(elem);
      (*locks_)[lock_num].unlock();
      if (result) {
        size_.fetch_sub(1);
      }
      return result;
    }

    [[nodiscard]] bool Contains(T elem) final {
      read_write_lock_.lock_shared();
      size_t lock_num = std::hash<T>()(elem) % bucket_count_.load();
      (*locks_)[lock_num].lock();
      read_write_lock_.unlock();
      bool result = table_[lock_num].find(elem) != table_[lock_num].end();
      (*locks_)[lock_num].unlock();
      return result;
    }

    [[nodiscard]] size_t Size() const final {
      return size_.load();
    }

private:

//    void Acquire(T elem) {
//      size_t num_locks = (*locks_).size();
//      size_t lock_num =
//    }

    bool policy() {
      return size_.load() / table_.size() > 4;
    }

    void resize() {

      size_t old_capacity = table_.size();

      read_write_lock_.lock();
      for (auto &lock: (*locks_)) {
        lock.lock();
      }

      if (old_capacity != table_.size()) {
        read_write_lock_.unlock();
        for (auto &lock: (*locks_)) {
          lock.unlock();
        }
        return;
      }

      size_t new_capacity = 2 * old_capacity;
      bucket_count_.store(new_capacity);
      auto new_locks(std::make_unique<std::vector<std::mutex>> (new_capacity));

      std::vector<std::set<int>> old_table;
      old_table.assign(table_.begin(), table_.end());

      table_.clear();
      table_.resize(new_capacity);

      for (std::set<int> &bucket: old_table) {
        for (int elem: bucket) {
          size_t bucket_num = std::hash<int>()(elem) % new_capacity;
          table_[bucket_num].insert(elem);
        }
      }

      locks_.release();
      locks_ = std::move(new_locks);

      read_write_lock_.unlock();
      for (auto &lock: (*locks_)) {
        lock.unlock();
      }
    }

private:
    //  Number of elements in the hashset
    std::atomic<size_t> size_;
    //  Vector of buckets for elements
    std::vector<std::set<T>> table_;
    //  Vector of mutexes for each stripe
    std::unique_ptr<std::vector<std::mutex>> locks_;
    //  Number of buckets
    std::atomic<size_t> bucket_count_;
    //  Read / Write lock
    std::shared_mutex read_write_lock_;
};

#endif  // HASH_SET_REFINABLE_H
