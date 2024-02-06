#pragma once

#include <boost/container/flat_map.hpp>
#include <boost/container/static_vector.hpp>
#include <cstdint>
#include <limits>

enum class TaskType
{
    Compare,
    Panic,

    TaskCount
};

class Scheduler
{
private:
    static constexpr size_t TaskCount = static_cast<size_t>(TaskType::TaskCount);

public:
    template <typename Key, typename Val, size_t size>
    using TaskMap =
        boost::container::flat_multimap<Key, Val, std::less<Key>,
                                        boost::container::static_vector<std::pair<Key, Val>, size>>;
    TaskMap<uint64_t, TaskType, TaskCount> events;

    uint64_t currentTimestamp;
    uint64_t nextTimestamp;

    void addTicks(uint64_t ticks)
    {
        currentTimestamp += ticks;
    }

    void updateNextTimestamp()
    {
        nextTimestamp = events.begin()->first;
    }

    void schedule(uint64_t timestamp, TaskType type)
    {
        events.emplace(timestamp, type);
        updateNextTimestamp();
    }

    void unschedule(TaskType type)
    {
        for (auto it = events.begin(); it != events.end(); it++)
        {
            if (it->second == type)
            {
                events.erase(it);
                updateNextTimestamp();
                break;
            }
        }
    }

    void reset()
    {
        currentTimestamp = 0;
        events.clear();

        // TODO: schedule vblank?
        schedule(std::numeric_limits<uint64_t>::max(), TaskType::Panic);
    }
};