#include "instance_manager.h"
#include "log/log.h"

namespace core
{

/****************************************************************************/

InstanceManager::InstanceManager()
  : m_isModified{true}
{
}

/****************************************************************************/

InstanceManager::~InstanceManager() = default;

/****************************************************************************/

Instance* InstanceManager::addInstance(const std::string& name,
        const Mesh* mesh, const Material* material)
{
    auto it = m_instances.find(name);
    if (it != m_instances.end()) {
        LOG_WARNING("Overwriting existing instance ", name);
        Instance* inst = it->second.get();
        inst->setMesh(mesh);
        inst->setMaterial(material);
        return inst;
    }

    auto res = m_instances.emplace(name,
            std::unique_ptr<Instance>(new Instance(mesh, material)));
    return res.first->second.get();
}

/****************************************************************************/

Instance* InstanceManager::getInstance(const std::string& name)
{
    auto it = m_instances.find(name);
    if (it == m_instances.end()) {
        LOG_ERROR("Unkown instance: ", name);
        return nullptr;
    }
    return it->second.get();
}

/****************************************************************************/

const Instance* InstanceManager::getInstance(const std::string& name) const
{
    auto it = m_instances.find(name);
    if (it == m_instances.end()) {
        LOG_ERROR("Unkown instance: ", name);
        return nullptr;
    }
    return it->second.get();
}

/****************************************************************************/

std::vector<Instance*> InstanceManager::getInstances()
{
    std::vector<Instance*> result;
    result.reserve(m_instances.size());
    for (const auto& it : m_instances)
        result.emplace_back(it.second.get());
    return result;
}

/****************************************************************************/

std::vector<const Instance*> InstanceManager::getInstances() const
{
    std::vector<const Instance*> result;
    result.reserve(m_instances.size());
    for (const auto& it : m_instances)
        result.emplace_back(it.second.get());
    return result;
}

/****************************************************************************/

bool InstanceManager::isModified() const
{
    return m_isModified;
}

/****************************************************************************/

void InstanceManager::setModified()
{
    m_isModified = true;
}

/****************************************************************************/

bool InstanceManager::update()
{
    if (!m_isModified)
        return false;
    for (auto& instance : m_instances) {
        instance.second->update();
    }
    return true;
}

/****************************************************************************/

} // namespace core

