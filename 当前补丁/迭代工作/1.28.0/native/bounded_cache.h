#pragma once

#include <cstddef>
#include <list>
#include <unordered_map>
#include <utility>

// Byte limits describe retained payloads; container bookkeeping is bounded by maxEntries.
// All application caches are owned by the UI thread.
template<class Key, class Value, class Hash = std::hash<Key>>
class BoundedLruCache {
    struct Entry { Key key; Value value; size_t bytes; };
    using Entries = std::list<Entry>;
    Entries entries_;
    std::unordered_map<Key, typename Entries::iterator, Hash> index_;
    size_t bytes_ = 0, maxEntries_, maxBytes_;
public:
    BoundedLruCache(size_t maxEntries, size_t maxBytes)
        : maxEntries_(maxEntries), maxBytes_(maxBytes) {}
    BoundedLruCache(const BoundedLruCache&) = delete;
    BoundedLruCache& operator=(const BoundedLruCache&) = delete;
    Value* find(const Key& key) {
        auto it = index_.find(key);
        if (it == index_.end()) return nullptr;
        entries_.splice(entries_.begin(), entries_, it->second);
        return &it->second->value;
    }
    bool take(const Key& key, Value& value) {
        auto it = index_.find(key);
        if (it == index_.end()) return false;
        value = std::move(it->second->value);
        bytes_ -= it->second->bytes;
        entries_.erase(it->second);
        index_.erase(it);
        return true;
    }
    bool put(Key key, Value value, size_t bytes) {
        if (!maxEntries_ || bytes > maxBytes_) return false;
        auto previous = index_.find(key);
        if (previous != index_.end()) {
            bytes_ -= previous->second->bytes;
            entries_.erase(previous->second);
            index_.erase(previous);
        }
        while (!entries_.empty() && (entries_.size() >= maxEntries_ || bytes_ > maxBytes_ - bytes)) {
            bytes_ -= entries_.back().bytes;
            index_.erase(entries_.back().key);
            entries_.pop_back();
        }
        entries_.push_front(Entry{std::move(key), std::move(value), bytes});
        index_.emplace(entries_.front().key, entries_.begin());
        bytes_ += bytes;
        return true;
    }
    void clear() { index_.clear(); entries_.clear(); bytes_ = 0; }
    size_t size() const { return entries_.size(); }
    size_t bytes() const { return bytes_; }
};
