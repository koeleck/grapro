#ifndef CORE_INSTANCE_MANAGER_H
#define CORE_INSTANCE_MANAGER_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "managers.h"
#include "instance.h"

namespace core
{

class Mesh;
class Material;

class InstanceManager
{
public:
    InstanceManager();
    ~InstanceManager();

    Instance* addInstance(const std::string& name, const Mesh* mesh, const Material* material);

    Instance* getInstance(const std::string& name);
    const Instance* getInstance(const std::string& name) const;

    std::vector<Instance*> getInstances();
    std::vector<const Instance*> getInstances() const;

    bool isModified() const;
    void setModified();
    bool update();

private:
    using InstanceMap = std::unordered_map<std::string, std::unique_ptr<Instance>>;


    InstanceMap         m_instances;
    bool                m_isModified;
};

} // namespace core

#endif // CORE_INSTANCE_MANAGER_H

