#ifndef OBJECT_MANAGER_HPP_
#define	OBJECT_MANAGER_HPP_
#include <vector>
#include <memory>
#include <functional>
#include <ranges>
#include <algorithm>
#include <concepts>

class ObjectManager
{
    struct Priority
    {
        std::function<void()> destructor;
        std::uint32_t priority;
    };

public:
    inline ObjectManager() noexcept : m_cleanedUp{ false } {}
    inline ~ObjectManager() noexcept
    {
        if (!m_cleanedUp)
            StartCleanUp();
    }

    template<typename T, typename G = T, typename... Ts>
    void CreateObject(std::unique_ptr<T>& emptyPtr, std::uint32_t priorityLevel, Ts&&... args)
    {
        emptyPtr = std::make_unique<G>(std::forward<Ts>(args)...);

        _createDestructor(emptyPtr, priorityLevel);
    }

    template<typename T, typename G = T>
    void CreateObject(std::unique_ptr<T>& emptyPtr, std::uint32_t priorityLevel)
    {
        emptyPtr = std::make_unique<G>();

        _createDestructor(emptyPtr, priorityLevel);
    }

    inline void StartCleanUp() noexcept
    {
        std::ranges::sort(
            m_destructionPriorities,
            [](const Priority& p1, const Priority& p2) {return p1.priority < p2.priority; }
        );

        for (auto& priority : m_destructionPriorities)
            priority.destructor();

        m_cleanedUp = true;
    }

private:
    template<typename T>
    void _createDestructor(T& emptyPtr, std::uint32_t priorityLevel) noexcept
    {
        Priority priority
        {
            .destructor = std::function<void()>([&] { emptyPtr.reset(); }),
            .priority   = priorityLevel
        };
        m_destructionPriorities.emplace_back(priority);
    }

private:
    std::vector<Priority> m_destructionPriorities;
    bool m_cleanedUp;
};
#endif
