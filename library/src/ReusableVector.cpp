#include <ReusableVector.hpp>
#include <ranges>
#include <algorithm>

std::optional<size_t> GetFirstAvailableIndex(const std::vector<bool>& availableIndices) noexcept
{
	auto result = std::ranges::find(availableIndices, true);

	if (result != std::end(availableIndices))
		return static_cast<size_t>(std::distance(std::begin(availableIndices), result));
	else
		return {};
}

std::vector<size_t> GetAvailableIndices(const std::vector<bool>& availableIndices) noexcept
{
	// Someday I will use a cutom CPU allocator for in the vector below.
	std::vector<size_t> freeIndices{};

	auto result = std::ranges::find(availableIndices, true);

	while (result != std::end(availableIndices))
	{
		freeIndices.emplace_back(
			static_cast<size_t>(std::distance(std::begin(availableIndices), result))
		);
		++result;

		result = std::find(result, std::end(availableIndices), true);
	}

	return freeIndices;
}
