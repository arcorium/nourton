#pragma once
#include <mutex>

#include <util/concept.h>

namespace ar
{
	template <typename T, typename Mtx = std::mutex>
	class safe_object
	{
		safe_object() = default;

	public:
		Mtx mutex_;
		T object_;
	};
}
