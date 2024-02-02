#pragma once

#include <boost/container/flat_map.hpp>
#include <cstdint>

enum class TaskType {
    Compare,

    TaskCount
};

class Scheduler {
private:
    static constexpr size_t TaskCount = static_cast<size_t>(TaskType::TaskCount);
public:
    template <typename Key, typename Val, size_t size>
	using TaskMap = boost::container::flat_multimap<Key, Val, std::less<Key>, boost::container::static_vector<std::pair<Key, Val>, size>>;
    TaskMap<uint64_t, TaskType, TaskCount> events;
};