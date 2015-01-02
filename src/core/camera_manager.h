#ifndef CORE_CAMERA_MANAGER_H
#define CORE_CAMERA_MANAGER_H

#include <string>
#include <unordered_map>
#include <memory>

#include "buffer_storage_pool.h"
#include "camera.h"
#include "managers.h"
#include "shader_interface.h"

namespace core
{

class CameraManager
{
public:
    CameraManager();

    PerspectiveCamera* createPerspectiveCam(const std::string& name,
            const glm::dvec3& pos,
            const glm::dvec3& center,
            double fovy, double aspect_ratio, double near);

    OrthogonalCamera* createOrthogonalCam(const std::string& name,
            const glm::dvec3& pos,
            const glm::dvec3& center,
            double left, double right, double bottom, double top,
            double zNear, double zFar);

    Camera* getCam(const std::string& name);

    const Camera* getCam(const std::string& name) const;

    void makeDefault(const Camera* cam);

    Camera* getDefaultCam();

    const Camera* getDefaultCam() const;

private:
    using CameraMap = std::unordered_map<std::string, std::size_t>;
    using CameraVector = std::vector<std::unique_ptr<Camera>>;
    using Pool = BufferStoragePool<sizeof(shader::CameraStruct)>;
    CameraMap                   m_camera_names;
    CameraVector                m_cameras;
    Pool                        m_camera_buffer;
    Camera*                     m_default_cam;
};

} // namespace core

#endif // CORE_CAMERA_MANAGER_H

