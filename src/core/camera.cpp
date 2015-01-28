#include <cassert>
#include <cstring>
#include <limits>
#include <glm/gtc/matrix_transform.hpp>

#include "camera.h"
#include "shader_interface.h"
#include "camera_manager.h"
#include "aabb.h"

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
        const CameraType camtype, shader::CameraStruct* const ptr)
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
    res::cameras->setModified();
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

    shader::CameraStruct data;
    data.ViewMatrix_T = glm::transpose(glm::mat4(m_viewmat));
    data.ProjMatrix_T = glm::transpose(glm::mat4(m_projmat));
    data.ProjViewMatrix_T = glm::transpose(glm::mat4(m_projviewmat));
    data.InvProjViewMatrix_T = glm::transpose(glm::mat4(glm::inverse(m_projviewmat)));
    data.CameraPosition = glm::vec4(m_position, 1.f);

    std::memcpy(m_data, &data, sizeof(shader::CameraStruct));

    m_modified = false;
}

//////////////////////////////////////////////////////////////////////////

/*************************************************************************
 *
 * PerspectiveCamera
 *
 ************************************************************************/

PerspectiveCamera::PerspectiveCamera(const glm::dvec3& pos,
        const glm::dvec3& center, const double fovy,
        const double aspect_ratio, const double near,
        shader::CameraStruct* ptr)
  : PerspectiveCamera(pos, center, fovy, aspect_ratio,
            near, std::numeric_limits<double>::infinity(), ptr)
{
}

/************************************************************************/

PerspectiveCamera::PerspectiveCamera(const glm::dvec3& pos,
        const glm::dvec3& center, const double fovy,
        const double aspect_ratio, const double near,
        const double far, shader::CameraStruct* ptr)
  : Camera(pos, center, CameraType::PERSPECTIVE, ptr),
    m_fovy{fovy},
    m_aspect_ratio{aspect_ratio},
    m_near{near},
    m_far{far},
    m_uFactor{glm::tan(static_cast<float>(m_fovy) / 2.f)},
    m_rFactor{m_uFactor * static_cast<float>(m_aspect_ratio)}
{
}

//////////////////////////////////////////////////////////////////////////

void PerspectiveCamera::setFOVY(const double fovy)
{
    m_fovy = fovy;
    m_uFactor = glm::tan(static_cast<float>(m_fovy) / 2.f);
    m_rFactor = m_uFactor * static_cast<float>(m_aspect_ratio);
    invalidate();
}

//////////////////////////////////////////////////////////////////////////

double PerspectiveCamera::getFOVY() const
{
    return m_fovy;
}

//////////////////////////////////////////////////////////////////////////

double PerspectiveCamera::getFOV() const
{
    return 2.0 * glm::atan(m_aspect_ratio * glm::tan(m_fovy / 2.0));
}

//////////////////////////////////////////////////////////////////////////

void PerspectiveCamera::setAspectRatio(const double ratio)
{
    m_aspect_ratio = ratio;
    m_uFactor = glm::tan(static_cast<float>(m_fovy) / 2.f);
    m_rFactor = m_uFactor * static_cast<float>(m_aspect_ratio);
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

void PerspectiveCamera::setFar(const double far)
{
    m_far = far;
    invalidate();
}

//////////////////////////////////////////////////////////////////////////

double PerspectiveCamera::getFar() const
{
    return m_far;
}

//////////////////////////////////////////////////////////////////////////

bool PerspectiveCamera::isInfinitePerspective() const
{
    return m_far == std::numeric_limits<double>::infinity();
}

//////////////////////////////////////////////////////////////////////////

void PerspectiveCamera::updateProjMat() const
{
    if (m_far == std::numeric_limits<double>::infinity()) {
        m_projmat = glm::infinitePerspective(m_fovy, m_aspect_ratio, m_near);
    } else {
        m_projmat = glm::perspective(m_fovy, m_aspect_ratio, m_near, m_far);
    }
}

//////////////////////////////////////////////////////////////////////////

bool PerspectiveCamera::inFrustum(const AABB& bbox) const
{
    int nOutOfLeft=0, nOutOfRight=0, nOutOfFar=0, nOutOfNear=0, nOutOfTop=0, nOutOfBottom=0;

    glm::vec3 corner[2];
    corner[0] = bbox.pmin - glm::vec3(getPosition());
    corner[1] = bbox.pmax - glm::vec3(getPosition());

    const glm::vec3 forward = glm::vec3(getDirection());
    const glm::vec3 right = glm::vec3(getRight());
    const glm::vec3 up = glm::vec3(getUp());

    const float near = static_cast<float>(m_near);
    const float far = static_cast<float>(m_far);

    for (int i = 0; i < 8; ++i) {
        bool isInRightTest = false;
        bool isInUpTest = false;
        bool isInFrontTest = false;
        const glm::vec3 p(corner[(i>>0) & 0x01].x,
                          corner[(i>>1) & 0x01].y,
                          corner[(i>>2) & 0x01].z);

        const float f = glm::dot(p, forward);
        const float u = glm::dot(p, up);
        const float r = glm::dot(p, right);

        if (r< -m_rFactor * f)
            ++nOutOfLeft;
        else if (r > m_rFactor * f)
            ++nOutOfRight;
        else
            isInRightTest=true;

        if (u < -m_uFactor * f)
            ++nOutOfBottom;
        else if (u > m_uFactor * f)
            ++nOutOfTop;
        else
            isInUpTest=true;

        if (f < near)
            ++nOutOfNear;
        else if (f > far)
            ++nOutOfFar;
        else
            isInFrontTest = true;

        if (isInRightTest && isInFrontTest && isInUpTest)
            return true;
    }

    if (nOutOfLeft == 8 || nOutOfRight == 8 || nOutOfFar == 8 ||
        nOutOfNear == 8 || nOutOfTop == 8 || nOutOfBottom == 8)
    {
        return false;
    }
    return true;
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
        const double zFar, shader::CameraStruct* ptr)
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

bool OrthogonalCamera::inFrustum(const AABB& bbox) const
{
    // TODO
    return true;
}

//////////////////////////////////////////////////////////////////////////

} // namespace core
