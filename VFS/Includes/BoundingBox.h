// Author : Jihong Shin (snowapril)

#if !defined(VFS_BOUNDING_BOX_H)
#define VFS_BOUNDING_BOX_H

#include <VulkanFramework/pch.h>
#include <Common/Utils.h>

namespace vfs
{
	template <typename Type>
	class BoundingBox
	{
	public:
		BoundingBox() = default;
		explicit BoundingBox(Type minCorner, Type maxCorner)
			: _minCorner(minCorner), _maxCorner(maxCorner) {};

	public:
		template <typename Point>
		inline bool isInsideBoundingBox(const Point& point)
		{
			return _minCorner <= Type(point) && Type(point) <= _maxCorner;
		}
		template <typename Point>
		inline void updateBoundingBox(const Point& point)
		{
			// TODO(snowapril) : need to check element by element
			_minCorner = glm::min(_minCorner, Type(point));
			_maxCorner = glm::max(_maxCorner, Type(point));
		}
		template <typename Point>
		inline void updateBoundingBox(const BoundingBox<Point>& otherBB)
		{
			_minCorner = glm::min(_minCorner, Type(otherBB._minCorner));
			_maxCorner = glm::max(_maxCorner, Type(otherBB._maxCorner));
		}
		inline Type getMinCorner(void) const
		{
			return _minCorner;
		}
		inline Type getMaxCorner(void) const
		{
			return _maxCorner;
		}
		inline Type getCenter(void) const
		{
			return (_minCorner + _maxCorner) * 0.5f;
		}
	private:
		Type _minCorner;
		Type _maxCorner;
	};
};

#endif