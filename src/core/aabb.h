#ifndef CORE_AABB_H
#define CORE_AABB_H

#include <glm/glm.hpp>
#include <limits>

namespace core
{

struct AABB
{
    glm::vec3   pmin;
    glm::vec3   pmax;

    AABB() noexcept
      : pmin{std::numeric_limits<float>::infinity()},
        pmax{-std::numeric_limits<float>::infinity()}
    {
    }

    AABB(const glm::vec3& p) noexcept
      : pmin{p},
        pmax{p}
    {
    }

    void expandBy(const glm::vec3& p) noexcept
    {
        pmin = glm::min(pmin, p);
        pmax = glm::max(pmax, p);
    }

    void expandBy(const AABB& other) noexcept
    {
        pmin = glm::min(pmin, other.pmin);
        pmax = glm::max(pmax, other.pmax);
    }

    bool contains(const glm::vec3& p) const noexcept
    {
        return p.x >= pmin.x && p.x <= pmax.x &&
               p.y >= pmin.y && p.y <= pmax.y &&
               p.z >= pmin.z && p.z <= pmax.z;
    }

    bool intersects(const AABB& other) const noexcept
    {
        if (glm::any(glm::lessThan(pmax, other.pmin)))
            return false;
        if (glm::any(glm::greaterThan(pmin, other.pmax)))
            return false;
        return true;
    }
};

} // namespace core

#endif // CORE_AABB_H

