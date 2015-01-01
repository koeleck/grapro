#include <utility>

#include "camera_manager.h"
#include "shader_interface.h"
#include "log/log.h"

namespace core
{

/****************************************************************************/

static constexpr int MAX_CAMERAS = 1024;

CameraManager::CameraManager()
  : m_camera_buffer(GL_SHADER_STORAGE_BUFFER, MAX_CAMERAS * sizeof(shader::CameraStruct),
          GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT),
    m_default_cam{nullptr}
{
}

/****************************************************************************/

PerspectiveCamera* CameraManager::createPerspectiveCam(const std::string& name,
        const glm::dvec3& pos,
        const glm::dvec3& center,
        double fovy, double aspect_ratio, double near)
{
    if (m_cameras.size() == MAX_CAMERAS) {
        LOG_ERROR("Maximum number of cameras reached!");
        abort();
    }
    if (m_camera_names.find(name) != m_camera_names.end()) {
        LOG_ERROR("Camera already exists: ", name);
        abort();
    }

    const auto offset = m_camera_buffer.alloc(sizeof(shader::CameraStruct), 16);
    void* const ptr = m_camera_buffer.offsetToPointer(offset);

    m_cameras.emplace_back(new PerspectiveCamera(pos, center, fovy, aspect_ratio, near, ptr));
    m_camera_names.emplace(name, m_cameras.size());

    return reinterpret_cast<PerspectiveCamera*>(m_cameras.back().get());
}

/****************************************************************************/

OrthogonalCamera* CameraManager::createOrthogonalCam(const std::string& name,
            const glm::dvec3& pos,
            const glm::dvec3& center,
            double left, double right, double bottom, double top,
            double zNear, double zFar)
{
    if (m_cameras.size() == MAX_CAMERAS) {
        LOG_ERROR("Maximum number of cameras reached!");
        abort();
    }
    if (m_camera_names.find(name) != m_camera_names.end()) {
        LOG_ERROR("Camera already exists: ", name);
        abort();
    }

    const auto offset = m_camera_buffer.alloc(sizeof(shader::CameraStruct), 16);
    void* const ptr = m_camera_buffer.offsetToPointer(offset);

    m_cameras.emplace_back(new OrthogonalCamera(pos, center, left, right, bottom, top, zNear, zFar, ptr));
    m_camera_names.emplace(name, m_cameras.size());

    return reinterpret_cast<OrthogonalCamera*>(m_cameras.back().get());
}

/****************************************************************************/

Camera* CameraManager::getCam(const std::string& name)
{
    auto it = m_camera_names.find(name);
    if (it == m_camera_names.end())
        return nullptr;
    return m_cameras[it->second].get();
}

/****************************************************************************/

const Camera* CameraManager::getCam(const std::string& name) const
{
    auto it = m_camera_names.find(name);
    if (it == m_camera_names.end())
        return nullptr;
    return m_cameras[it->second].get();
}

/****************************************************************************/

void CameraManager::makeDefault(const Camera* cam)
{
    m_default_cam = const_cast<Camera*>(cam);
    if (m_default_cam != nullptr) {
        glBindBufferRange(GL_SHADER_STORAGE_BUFFER, bindings::CAMERA,
                m_camera_buffer.buffer(),
                m_camera_buffer.pointerToOffset(cam->m_data),
                sizeof(shader::CameraStruct));
    }
}

/****************************************************************************/

Camera* CameraManager::getDefaultCam()
{
    return m_default_cam;
}

/****************************************************************************/

const Camera* CameraManager::getDefaultCam() const
{
    return m_default_cam;
}

/****************************************************************************/

} // namespace core
