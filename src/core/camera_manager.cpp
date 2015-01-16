#include <utility>

#include "camera_manager.h"
#include "shader_interface.h"
#include "log/log.h"

namespace core
{

/****************************************************************************/

static constexpr int MAX_CAMERAS = 1024;

CameraManager::CameraManager()
  : m_camera_buffer(GL_SHADER_STORAGE_BUFFER, MAX_CAMERAS),
    m_default_cam{nullptr},
    m_isModified{true}
{
}

/****************************************************************************/

PerspectiveCamera* CameraManager::createPerspectiveCam(const std::string& name,
        const glm::dvec3& pos,
        const glm::dvec3& center,
        const double fovy, const double aspect_ratio,
        const double near, const double far)
{
    if (m_cameras.size() == MAX_CAMERAS) {
        LOG_ERROR("Maximum number of cameras reached!");
        abort();
    }
    if (m_camera_names.find(name) != m_camera_names.end()) {
        LOG_ERROR("Camera already exists: ", name);
        abort();
    }

    const auto offset = m_camera_buffer.alloc();
    auto* const ptr = m_camera_buffer.offsetToPointer(offset);

    m_cameras.emplace_back(new PerspectiveCamera(pos, center, fovy, aspect_ratio, near, far, ptr));
    m_camera_names.emplace(name, m_cameras.size());

    if (m_cameras.size() == 1) {
        makeDefault(m_cameras.back().get());
    }

    return reinterpret_cast<PerspectiveCamera*>(m_cameras.back().get());
}

/****************************************************************************/

OrthogonalCamera* CameraManager::createOrthogonalCam(const std::string& name,
            const glm::dvec3& pos,
            const glm::dvec3& center,
            const double left, const double right,
            const double bottom, const double top,
            const double zNear, const double zFar)
{
    if (m_cameras.size() == MAX_CAMERAS) {
        LOG_ERROR("Maximum number of cameras reached!");
        abort();
    }
    if (m_camera_names.find(name) != m_camera_names.end()) {
        LOG_ERROR("Camera already exists: ", name);
        abort();
    }

    const auto offset = m_camera_buffer.alloc();
    auto* const ptr = m_camera_buffer.offsetToPointer(offset);

    m_cameras.emplace_back(new OrthogonalCamera(pos, center, left, right, bottom, top, zNear, zFar, ptr));
    m_camera_names.emplace(name, m_cameras.size());

    if (m_cameras.size() == 1) {
        makeDefault(m_cameras.back().get());
    }

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
    if (cam == nullptr)
        return;
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

bool CameraManager::isModified() const
{
    return m_isModified;
}

/****************************************************************************/

void CameraManager::setModified()
{
    m_isModified = true;
}

/****************************************************************************/

bool CameraManager::update()
{
    if (!m_isModified)
        return false;
    for (auto& cam : m_cameras) {
        cam->update();
    }
    m_isModified = false;
    return true;
}

/****************************************************************************/

} // namespace core
