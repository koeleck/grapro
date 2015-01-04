#include "instance_manager.h"

namespace core
{

/****************************************************************************/

constexpr int MAX_NUM_INSTANCES = 1024;
InstanceManager::InstanceManager()
  : m_instance_buffer(GL_SHADER_STORAGE_BUFFER, MAX_NUM_INSTANCES),
    m_isModified{true}
{
    bind();
}

/****************************************************************************/

InstanceManager::~InstanceManager() = default;

/****************************************************************************/

Instance* InstanceManager::addInstance(const std::string& name,
        const Mesh* mesh, const Material* material)
{
    auto it = m_instance_names.find(name);
    if (it != m_instance_names.end()) {
        LOG_WARNING("Overwriting existing instance ", name);
        Instance* inst = m_instances[it->second].get();
        inst->setMesh(mesh);
        inst->setMaterial(material);
        return inst;
    }

    const GLintptr offset = m_instance_buffer.alloc();
    void* data_ptr = m_instance_buffer.offsetToPointer(offset);
    GLuint index = static_cast<GLuint>(offset / static_cast<long>(sizeof(shader::InstanceStruct)));

    m_instance_names.emplace(name, m_instances.size());
    m_instances.emplace_back(new Instance(mesh, material, index, data_ptr));
    return m_instances.back().get();
}

/****************************************************************************/

Instance* InstanceManager::getInstance(const std::string& name)
{
    auto it = m_instance_names.find(name);
    if (it == m_instance_names.end()) {
        LOG_ERROR("Unkown instance: ", name);
        return nullptr;
    }
    return m_instances[it->second].get();
}

/****************************************************************************/

const Instance* InstanceManager::getInstance(const std::string& name) const
{
    auto it = m_instance_names.find(name);
    if (it == m_instance_names.end()) {
        LOG_ERROR("Unkown instance: ", name);
        return nullptr;
    }
    return m_instances[it->second].get();
}

/****************************************************************************/

std::vector<Instance*> InstanceManager::getInstances()
{
    std::vector<Instance*> result;
    result.reserve(m_instances.size());
    for (const auto& ptr : m_instances)
        result.emplace_back(ptr.get());
    return result;
}

/****************************************************************************/

std::vector<const Instance*> InstanceManager::getInstances() const
{
    std::vector<const Instance*> result;
    result.reserve(m_instances.size());
    for (const auto& ptr : m_instances)
        result.emplace_back(ptr.get());
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
        instance->update();
    }
    return true;
}

/****************************************************************************/

void InstanceManager::bind() const
{
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindings::INSTANCE,
            m_instance_buffer.buffer());
}

/****************************************************************************/

} // namespace core

