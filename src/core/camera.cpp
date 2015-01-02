#include <cassert>
#include <cstring>
#include <glm/gtc/matrix_transform.hpp>

#include "camera.h"
#include "shader_interface.h"

#include "log/log.h"

//////////////////////////////////////////////////////////////////////////

namespace core
{

/*************************************************************************
 *
 * Camera
 *
 ************************************************************************/

//////////////////////////////////////////////////////////////////////////

Camera::Camera(const glm::dvec3& pos, const glm::dvec3& center,
        const CameraType camtype, void* const ptr)
  : m_modified{true},
    m_useFixedYawAxis{false},
    m_type{camtype},
    m_position{pos},
    m_orientation{},
    m_data{ptr}
{
    lookAt(center);

}

//////////////////////////////////////////////////////////////////////////

CameraType Camera::type() const
{
    return m_type;
}

//////////////////////////////////////////////////////////////////////////

void Camera::invalidate()
{
    m_modified = true;
}

//////////////////////////////////////////////////////////////////////////

const glm::dmat4& Camera::getViewMatrix() const
{
    update();
    return m_viewmat;
}

//////////////////////////////////////////////////////////////////////////

const glm::dmat4& Camera::getProjMatrix() const
{
    update();
    return m_projmat;
}

//////////////////////////////////////////////////////////////////////////

const glm::dmat4& Camera::getProjViewMatrix() const
{
    update();
    return m_projviewmat;
}

//////////////////////////////////////////////////////////////////////////

void Camera::move(const glm::dvec3& dir)
{
    m_position += dir.x * getRight();
    m_position += dir.y * getUp();
    m_position += dir.z * getDirection();

    invalidate();
}

//////////////////////////////////////////////////////////////////////////

void Camera::setPosition(const glm::dvec3& pos)
{
    m_position = pos;

    invalidate();
}

//////////////////////////////////////////////////////////////////////////

const glm::dvec3& Camera::getPosition() const
{
    return m_position;
}

//////////////////////////////////////////////////////////////////////////

void Camera::lookAt(const glm::dvec3& center)
{
    setDirection(center - m_position);
}

//////////////////////////////////////////////////////////////////////////

void Camera::setOrientation(const glm::dquat& orientation)
{
    m_orientation = glm::normalize(orientation);

    invalidate();
}

//////////////////////////////////////////////////////////////////////////

const glm::dquat& Camera::getOrientation() const
{
    return m_orientation;
}

//////////////////////////////////////////////////////////////////////////

void Camera::setDirection(const glm::dvec3& dir)
{
    if (dir == glm::dvec3(0.0))
        return;

    const glm::dvec3 zaxis = glm::normalize(dir);
    if (m_useFixedYawAxis) {
        const glm::dvec3 xaxis = glm::normalize(glm::cross(zaxis, m_fixedYawAxis));
        const glm::dvec3 yaxis = glm::normalize(glm::cross(xaxis, zaxis));
        const glm::dmat3 mat(xaxis, yaxis, -zaxis);

        m_orientation = glm::normalize(glm::quat_cast(mat));
    } else {
        const glm::dvec3 current_z = getDirection();
        const double d = glm::dot(zaxis, current_z);

        glm::dvec3 raxis;
        double angle;
        constexpr double threshold = 0.99995;
        if (d > threshold) {
            // no rotation
            return;
        } else if (d < -threshold) {
            raxis = getUp();
            angle = glm::pi<double>();
        } else {
            raxis = glm::normalize(glm::cross(current_z, zaxis));
            angle = glm::acos(d);
        }
        glm::dquat rot = glm::angleAxis(angle, raxis);

        m_orientation = glm::normalize(rot * m_orientation);
    }

    invalidate();
}

//////////////////////////////////////////////////////////////////////////

glm::dvec3 Camera::getDirection() const
{
    return m_orientation * glm::dvec3(0.0, 0.0, -1.0);
}

//////////////////////////////////////////////////////////////////////////

glm::dvec3 Camera::getUp() const
{
    return m_orientation * glm::dvec3(0.0, 1.0, 0.0);
}

//////////////////////////////////////////////////////////////////////////

glm::dvec3 Camera::getRight() const
{
    return m_orientation * glm::dvec3(1.0, 0.0, 0.0);
}

//////////////////////////////////////////////////////////////////////////

void Camera::rotate(const glm::dvec3& axis, const double angle)
{
    glm::dquat q = glm::angleAxis(angle, glm::normalize(axis));
    m_orientation = glm::normalize(m_orientation * q);

    invalidate();
}

//////////////////////////////////////////////////////////////////////////

void Camera::yaw(const double angle)
{
    if (m_useFixedYawAxis) {
        m_orientation = glm::normalize(glm::angleAxis(angle, m_fixedYawAxis) * m_orientation);
    } else {
        m_orientation = glm::normalize(m_orientation * glm::angleAxis(angle, glm::dvec3(0.0, 1.0, 0.0)));
    }
    invalidate();
}

//////////////////////////////////////////////////////////////////////////

void Camera::roll(const double angle)
{
    m_orientation = glm::normalize(m_orientation * glm::angleAxis(angle, glm::dvec3(0.0, 0.0, 1.0)));
    invalidate();
}

//////////////////////////////////////////////////////////////////////////

void Camera::pitch(const double angle)
{
    m_orientation = glm::normalize(m_orientation * glm::angleAxis(angle, glm::dvec3(1.0, 0.0, 0.0)));
    invalidate();
}

//////////////////////////////////////////////////////////////////////////

void Camera::setFixedYawAxis(const bool use, const glm::dvec3& axis)
{
    if (!use) {
        m_useFixedYawAxis = false;
    } else {
        m_useFixedYawAxis = true;
        m_fixedYawAxis = glm::normalize(axis);
    }
}

//////////////////////////////////////////////////////////////////////////

bool Camera::hasFixedYawAxis() const
{
    return m_useFixedYawAxis;
}

//////////////////////////////////////////////////////////////////////////

const glm::dvec3& Camera::getFixedYawAxis() const
{
    return m_fixedYawAxis;
}

//////////////////////////////////////////////////////////////////////////

void Camera::update() const
{
    if (!m_modified)
        return;

    updateProjMat();

    glm::dmat4 translation(1.0);
    translation[3] = glm::dvec4(-m_position, 1.0);

    m_viewmat = glm::mat4_cast(glm::conjugate(m_orientation)) * translation;
    m_projviewmat = m_projmat * m_viewmat;

    m_modified = false;
}

//////////////////////////////////////////////////////////////////////////

void Camera::updateBuffer() const
{
    if (!m_modified)
        return;

    update();

    shader::CameraStruct data;
    data.ViewMatrix = glm::mat4(m_viewmat);
    data.ProjMatrix = glm::mat4(m_projmat);
    data.ProjViewMatrix = glm::mat4(m_projviewmat);
    data.CameraPosition = glm::vec4(m_position, 1.f);

    std::memcpy(m_data, &data, sizeof(shader::CameraStruct));
}

//////////////////////////////////////////////////////////////////////////

/*************************************************************************
 *
 * PerspectiveCamera
 *
 ************************************************************************/

PerspectiveCamera::PerspectiveCamera(const glm::dvec3& pos,
        const glm::dvec3& center, const double fovy,
        const double aspect_ratio, const double near, void* ptr)
  : Camera(pos, center, CameraType::PERSPECTIVE, ptr),
    m_fovy{fovy},
    m_aspect_ratio{aspect_ratio},
    m_near{near}
{
    m_projmat = glm::infinitePerspective(m_fovy, m_aspect_ratio, m_near);
}

//////////////////////////////////////////////////////////////////////////

void PerspectiveCamera::setFOVY(const double fovy)
{
    m_fovy = fovy;
    invalidate();
}

//////////////////////////////////////////////////////////////////////////

double PerspectiveCamera::getFOVY() const
{
    return m_fovy;
}

//////////////////////////////////////////////////////////////////////////


void PerspectiveCamera::setAspectRatio(const double ratio)
{
    m_aspect_ratio = ratio;
    invalidate();
}

//////////////////////////////////////////////////////////////////////////

double PerspectiveCamera::getAspectRatio() const
{
    return m_aspect_ratio;
}

//////////////////////////////////////////////////////////////////////////

void PerspectiveCamera::setNear(const double near)
{
    m_near = near;
    invalidate();
}

//////////////////////////////////////////////////////////////////////////

double PerspectiveCamera::getNear() const
{
    return m_near;
}

//////////////////////////////////////////////////////////////////////////

void PerspectiveCamera::updateProjMat() const
{
    m_projmat = glm::infinitePerspective(m_fovy, m_aspect_ratio, m_near);
}

//////////////////////////////////////////////////////////////////////////

/*************************************************************************
 *
 * OrthogonalCamera
 *
 ************************************************************************/

OrthogonalCamera::OrthogonalCamera(const glm::dvec3& pos,
        const glm::dvec3& center, const double left, const double right,
        const double bottom, const double top, const double zNear,
        const double zFar, void* ptr)
  : Camera(pos, center, CameraType::ORTHOGONAL, ptr),
    m_left{left},
    m_right{right},
    m_bottom{bottom},
    m_top{top},
    m_zNear{zNear},
    m_zFar{zFar}
{
    m_projmat = glm::ortho(m_left, m_right, m_bottom, m_top,
            m_zNear, m_zFar);
}

//////////////////////////////////////////////////////////////////////////

void OrthogonalCamera::setLeft(const double left)
{
    m_left = left;
    invalidate();
}

//////////////////////////////////////////////////////////////////////////

double OrthogonalCamera::getLeft() const
{
    return m_left;
}

//////////////////////////////////////////////////////////////////////////

void OrthogonalCamera::setRight(const double right)
{
    m_right = right;
    invalidate();
}

//////////////////////////////////////////////////////////////////////////

double OrthogonalCamera::getRight() const
{
    return m_right;
}

//////////////////////////////////////////////////////////////////////////

void OrthogonalCamera::setTop(const double top)
{
    m_top = top;
    invalidate();
}

//////////////////////////////////////////////////////////////////////////

double OrthogonalCamera::getTop() const
{
    return m_top;
}

//////////////////////////////////////////////////////////////////////////

void OrthogonalCamera::setBottom(const double bottom)
{
    m_bottom = bottom;
    invalidate();
}

//////////////////////////////////////////////////////////////////////////

double OrthogonalCamera::getBottom() const
{
    return m_bottom;
}

//////////////////////////////////////////////////////////////////////////

void OrthogonalCamera::setZNear(const double zNear)
{
    m_zNear = zNear;
    invalidate();
}

//////////////////////////////////////////////////////////////////////////

double OrthogonalCamera::getZNear() const
{
    return m_zNear;
}

//////////////////////////////////////////////////////////////////////////

void OrthogonalCamera::setZFar(const double zFar)
{
    m_zFar = zFar;
    invalidate();
}

//////////////////////////////////////////////////////////////////////////

double OrthogonalCamera::getZFar() const
{
    return m_zFar;
}

//////////////////////////////////////////////////////////////////////////

void OrthogonalCamera::updateProjMat() const
{
    m_projmat = glm::ortho(m_left, m_right, m_bottom, m_top,
            m_zNear, m_zFar);
}

//////////////////////////////////////////////////////////////////////////

} // namespace core
