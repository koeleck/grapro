#ifndef CORE_3D_CAMERA_H
#define CORE_3D_CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace core
{

class AABB;

enum class CameraType : unsigned char
{
    PERSPECTIVE = 0x01,
    ORTHOGONAL = 0x02
};

/*************************************************************************
 *
 * Camera base class
 *
 ************************************************************************/
class Camera
{
public:
    virtual ~Camera() = default;

    const glm::dmat4& getProjMatrix() const;

    const glm::dmat4& getViewMatrix() const;

    const glm::dmat4& getProjViewMatrix() const;

    void move(const glm::dvec3& dir);

    void setPosition(const glm::dvec3& pos);

    const glm::dvec3& getPosition() const;

    void setOrientation(const glm::dquat& orientation);

    const glm::dquat& getOrientation() const;

    void lookAt(const glm::dvec3& center);

    void setDirection(const glm::dvec3& dir);

    glm::dvec3 getDirection() const;

    glm::dvec3 getUp() const;

    glm::dvec3 getRight() const;

    void rotate(const glm::dvec3& axis, double angle);

    void yaw(double angle);

    void roll(double angle);

    void pitch(double angle);

    void setFixedYawAxis(bool use, const glm::dvec3& axis);

    bool hasFixedYawAxis() const;

    const glm::dvec3& getFixedYawAxis() const;

    void update() const;

    CameraType type() const;

    virtual bool inFrustum(const AABB& bbox) const = 0;

protected:
    Camera(const glm::dvec3& pos, const glm::dvec3& center,
            CameraType type, void* ptr);

    virtual void updateProjMat() const = 0;
    void invalidate();

    mutable glm::dmat4  m_projmat;

private:
    friend class CameraManager;

    mutable bool        m_modified;
    bool                m_useFixedYawAxis;
    CameraType          m_type;
    glm::dvec3          m_position;
    glm::dquat          m_orientation;
    glm::dvec3          m_fixedYawAxis;
    mutable glm::dmat4  m_viewmat;
    mutable glm::dmat4  m_projviewmat;
    void*               m_data;
};

/*************************************************************************
 *
 * Perspective Camera
 *
 ************************************************************************/

class PerspectiveCamera
  : public Camera
{
public:
    virtual ~PerspectiveCamera() = default;

    void setFOVY(double fovy);
    double getFOVY() const;

    void setAspectRatio(double ratio);
    double getAspectRatio() const;

    void setNear(double near);
    double getNear() const;

    virtual bool inFrustum(const AABB& bbox) const override;

protected:
    PerspectiveCamera(const glm::dvec3& pos, const glm::dvec3& center,
            double fovy, double aspect_ratio, double near,
            void* ptr);

private:
    friend class CameraManager;


    virtual void updateProjMat() const override;

    double          m_fovy;
    double          m_aspect_ratio;
    double          m_near;
    float           m_uFactor;
    float           m_rFactor;
};

/*************************************************************************
 *
 * Orthogonal Camera
 *
 ************************************************************************/

class OrthogonalCamera
  : public Camera
{
public:
    virtual ~OrthogonalCamera() = default;

    void setLeft(double left);
    double getLeft() const;

    void setRight(double left);
    double getRight() const;

    void setTop(double left);
    double getTop() const;

    void setBottom(double left);
    double getBottom() const;

    void setZNear(double left);
    double getZNear() const;

    void setZFar(double left);
    double getZFar() const;

    virtual bool inFrustum(const AABB& bbox) const override;

protected:
    OrthogonalCamera(const glm::dvec3& pos, const glm::dvec3& center,
            double left, double right, double bottom, double top,
            double zNear, double zFar, void* ptr);

private:
    friend class CameraManager;

    virtual void updateProjMat() const override;

    double          m_left;
    double          m_right;
    double          m_bottom;
    double          m_top;
    double          m_zNear;
    double          m_zFar;
};

} // namespace core

#endif // CORE_3D_CAMERA_H

