#ifndef CORE_MATERIAL_H
#define CORE_MATERIAL_H

#include "gl/gl_sys.h"
#include <glm/vec3.hpp>

namespace core
{

class Texture;

namespace shader
{
struct MaterialStruct;
} // namespace shader

class Material
{
public:
    bool hasDiffuseTexture() const;
    bool hasSpecularTexture() const;
    bool hasEmissiveTexture() const;
    bool hasGlossyTexture() const;
    bool hasAlphaTexture() const;
    bool hasNormalTexture() const;
    bool hasAmbientTexture() const;

    const Texture* getDiffuseTexture() const;
    const Texture* getSpecularTexture() const;
    const Texture* getEmissiveTexture() const;
    const Texture* getGlossyTexture() const;
    const Texture* getAlphaTexture() const;
    const Texture* getNormalTexture() const;
    const Texture* getAmbientTexture() const;

    void setDiffuseTexture(const Texture* texture);
    void setSpecularTexture(const Texture* texture);
    void setEmissiveTexture(const Texture* texture);
    void setGlossyTexture(const Texture* texture);
    void setAlphaTexture(const Texture* texture);
    void setNormalTexture(const Texture* texture);
    void setAmbientTexture(const Texture* texture);

    const glm::vec3& getDiffuseColor() const;
    const glm::vec3& getSpecularColor() const;
    const glm::vec3& getAmbientColor() const;
    const glm::vec3& getEmissiveColor() const;
    const glm::vec3& getTransparentColor() const;
    float getGlossiness() const;
    float getOpacity() const;

    void setDiffuseColor(const glm::vec3& color);
    void setSpecularColor(const glm::vec3& color);
    void setAmbientColor(const glm::vec3& color);
    void setEmissiveColor(const glm::vec3& color);
    void setTransparentColor(const glm::vec3& color);
    void setGlossiness(float value);
    void setOpacity(float value);

    GLuint getIndex() const;

private:
    friend class MaterialManager;
    Material(GLuint index, void* ptr);

    const Texture*  m_diffuse_texture;
    const Texture*  m_specular_texture;
    const Texture*  m_glossy_texture;
    const Texture*  m_normal_texture;
    const Texture*  m_emissive_texture;
    const Texture*  m_alpha_texture;
    const Texture*  m_ambient_texture;
    glm::vec3       m_diffuse_color;
    glm::vec3       m_specular_color;
    glm::vec3       m_ambient_color;
    glm::vec3       m_emissive_color;
    glm::vec3       m_transparent_color;
    float           m_glossiness;
    float           m_opacity;

    GLuint          m_index;
    shader::MaterialStruct* m_data;
};

} // namespace core

#endif // CORE_MATERIAL_H

