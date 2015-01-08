#include <cassert>
#include <algorithm>
#include <cstring>
#include <vector>
#include <limits>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "assimporter.h"
#include "scene.h"
#include "mesh.h"
#include "material.h"
#include "light.h"
#include "node.h"
#include "camera.h"
#include "texture.h"

#include "log/log.h"

/***************************************************************************/

namespace
{

using namespace import;

int cam_counter = 0;
int light_counter = 0;
int material_counter = 0;
int mesh_counter = 0;
int node_counter = 0;
const char* DEFAULT_CAM_NAME = "unkown_camera";
const char* DEFAULT_LIGHT_NAME = "unkown_light";
const char* DEFAULT_MATERIAL_NAME = "unknown_material";
const char* DEFAULT_MESH_NAME = "unkown_mesh";
const char* DEFAULT_NODE_NAME = "unkown_node";

struct AssimpMaterial
{
    AssimpMaterial(const aiMaterial* mat)
      : diffuse_tex{-1}, specular_tex{-1},
        exponent_tex{-1}, normal_tex{-1},
        emissive_tex{-1}, alpha_tex{-1},
        ambient_tex{-1}, ptr{mat}
    {
    }
    int diffuse_tex;
    int specular_tex;
    int exponent_tex;
    int normal_tex;
    int emissive_tex;
    int alpha_tex;
    int ambient_tex;
    const aiMaterial* ptr;

    bool needsTexcoords() const noexcept
    {
        return diffuse_tex   != -1 || specular_tex != -1 ||
                exponent_tex != -1 || normal_tex   != -1 ||
                emissive_tex != -1 || alpha_tex    != -1 ||
                ambient_tex  != -1;
    }

    bool needsTangents() const noexcept
    {
        return normal_tex != -1;
    }
};

struct AssimpNode
{
    AssimpNode(const aiNode* node, unsigned int index)
      : ptr{node}, idx{index}
    {
    }
    const aiNode* ptr;
    unsigned int  idx;
};

struct AssimpMesh
{
    AssimpMesh(const aiMesh* mesh)
      : has_texcoords{false},
        has_vertex_colors{false},
        has_tangents{false},
        ptr{mesh}
    {
    }

    bool has_texcoords;
    bool has_vertex_colors;
    bool has_tangents;
    const aiMesh* ptr;
};

glm::vec3 to_glm(const aiVector3D& vec3);
glm::vec3 to_glm(const aiColor3D& col);
glm::vec3 to_glm(const aiColor4D& col);
glm::mat4 to_glm(const aiMatrix4x4& mat);

// local function declarations:
std::size_t getSceneInfo(const std::string& filename, const aiScene* scene,
        std::vector<std::pair<std::string, const aiCamera*>>& cameras,
        std::vector<std::pair<std::string, const aiLight*>>& lights,
        std::vector<std::pair<std::string, AssimpMaterial>>& materials,
        std::vector<std::string>& textures,
        std::vector<std::pair<std::string, AssimpMesh>>& meshes,
        std::vector<std::pair<std::string, AssimpNode>>& nodes);
std::string getName(const aiString& name, const char* default_name, int& counter);
std::string getName(const aiCamera* cam);
std::string getName(const aiLight* light);
std::string getName(const aiMaterial* mat);
std::string getName(const aiMesh* mesh);
std::string getName(const aiNode* node);

char* dumpCamera(char* ptr, const std::pair<std::string, const aiCamera*>& cam);
char* dumpMaterial(char* ptr, const std::pair<std::string, AssimpMaterial>& mat);
char* dumpTexture(char* ptr, const std::string& tex);
char* dumpMesh(char* ptr, const std::pair<std::string, AssimpMesh>& mesh);
char* dumpLight(char* ptr, const std::pair<std::string, const aiLight*>& light);
char* dumpNode(char* ptr, const std::pair<std::string, AssimpNode>& node);

} // anonymous namespace

/***************************************************************************/

namespace import
{

std::unique_ptr<Scene> assimport(const std::string& filename)
{
    Assimp::Importer importer;

    importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE,
            aiPrimitiveType_POINT | aiPrimitiveType_LINE);

    const aiScene* scene = importer.ReadFile(filename.c_str(),
            aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices |
            aiProcess_Triangulate | aiProcess_GenSmoothNormals |
            aiProcess_ImproveCacheLocality | aiProcess_RemoveRedundantMaterials |
            aiProcess_FindDegenerates | aiProcess_SortByPType |
            aiProcess_ValidateDataStructure | aiProcess_FindInvalidData |
            aiProcess_FindInstances);

    if (!scene) {
        LOG_ERROR(importer.GetErrorString());
        return nullptr;
    }

    std::vector<std::pair<std::string, const aiCamera*>> cameras;
    std::vector<std::pair<std::string, const aiLight*>> lights;
    std::vector<std::pair<std::string, AssimpMaterial>> materials;
    std::vector<std::string> textures;
    std::vector<std::pair<std::string, AssimpMesh>> meshes;
    std::vector<std::pair<std::string, AssimpNode>> nodes;
    const auto scene_size = getSceneInfo(filename, scene, cameras, lights, materials,
            textures, meshes, nodes);

    if (scene_size == 0) {
        return nullptr;
    }

    char* data = new char[scene_size];

    // initialize scene struct
    Scene* my_scene = reinterpret_cast<Scene*>(data);
    data += sizeof(Scene);

    my_scene->name = data;
    std::strcpy(data, filename.c_str());
    data += filename.size() + 1;

    my_scene->size = static_cast<std::uint32_t>(scene_size);
    my_scene->num_materials = static_cast<std::uint32_t>(materials.size());
    my_scene->num_textures = static_cast<std::uint32_t>(textures.size());
    my_scene->num_meshes = static_cast<std::uint32_t>(meshes.size());
    my_scene->num_lights = static_cast<std::uint32_t>(lights.size());
    my_scene->num_cameras = static_cast<std::uint32_t>(cameras.size());
    my_scene->num_nodes = static_cast<std::uint32_t>(nodes.size());

    // arrays
    if (my_scene->num_materials > 0) {
        my_scene->materials = reinterpret_cast<Material**>(data);
        data += my_scene->num_materials * sizeof(Material*);
    } else {
        my_scene->materials = nullptr;
    }

    if (my_scene->num_meshes > 0) {
        my_scene->meshes = reinterpret_cast<Mesh**>(data);
        data += my_scene->num_meshes * sizeof(Mesh*);
    } else {
        my_scene->meshes = nullptr;
    }

    if (my_scene->num_lights > 0) {
        my_scene->lights = reinterpret_cast<Light**>(data);
        data += my_scene->num_lights * sizeof(Light*);
    } else {
        my_scene->lights = nullptr;
    }

    if (my_scene->num_textures > 0) {
        my_scene->textures = reinterpret_cast<Texture**>(data);
        data += my_scene->num_textures * sizeof(Texture**);
    } else {
        my_scene->textures = nullptr;
    }

    if (my_scene->num_cameras > 0) {
        my_scene->cameras = reinterpret_cast<Camera**>(data);
        data += my_scene->num_cameras * sizeof(Camera*);
    } else {
        my_scene->cameras = nullptr;
    }

    if (my_scene->num_nodes > 0) {
        my_scene->nodes = reinterpret_cast<Node**>(data);
        data += my_scene->num_nodes * sizeof(Node*);
    } else {
        my_scene->nodes = nullptr;
    }

    // dump data
    for (std::size_t i = 0; i < materials.size(); ++i) {
        my_scene->materials[i] = reinterpret_cast<Material*>(data);
        data = dumpMaterial(data, materials[i]);
    }

    for (std::size_t i = 0; i < meshes.size(); ++i) {
        my_scene->meshes[i] = reinterpret_cast<Mesh*>(data);
        data = dumpMesh(data, meshes[i]);
    }

    for (std::size_t i = 0; i < lights.size(); ++i) {
        my_scene->lights[i] = reinterpret_cast<Light*>(data);
        data = dumpLight(data, lights[i]);
    }

    for (std::size_t i = 0; i < textures.size(); ++i) {
        my_scene->textures[i] = reinterpret_cast<Texture*>(data);
        data = dumpTexture(data, textures[i]);
    }

    for (std::size_t i = 0; i < cameras.size(); ++i) {
        my_scene->cameras[i] = reinterpret_cast<Camera*>(data);
        data = dumpCamera(data, cameras[i]);
    }

    for (std::size_t i = 0; i < nodes.size(); ++i) {
        my_scene->nodes[i] = reinterpret_cast<Node*>(data);
        data = dumpNode(data, nodes[i]);
    }

    assert(data == reinterpret_cast<char*>(my_scene) + scene_size);

    return std::unique_ptr<Scene>(my_scene);
}

} // namespace import

/***************************************************************************/
/***************************************************************************/

namespace
{

/***************************************************************************/

std::size_t getSceneInfo(const std::string& filename, const aiScene* scene,
        std::vector<std::pair<std::string, const aiCamera*>>& cameras,
        std::vector<std::pair<std::string, const aiLight*>>& lights,
        std::vector<std::pair<std::string, AssimpMaterial>>& materials,
        std::vector<std::string>& textures,
        std::vector<std::pair<std::string, AssimpMesh>>& meshes,
        std::vector<std::pair<std::string, AssimpNode>>& nodes)
{
    std::size_t size = sizeof(Scene) + filename.size() + 1;

    // Cameras
    size += scene->mNumCameras * sizeof(Camera*);
    for (unsigned int i = 0; i < scene->mNumCameras; ++i) {
        const aiCamera* cam = scene->mCameras[i];
        std::string cam_name = getName(cam);
        size += sizeof(Camera) + cam_name.size() + 1;
        cameras.emplace_back(std::move(cam_name), cam);
    }

    // Lights
    size += scene->mNumLights * sizeof(Light*);
    for (unsigned int i = 0; i < scene->mNumLights; ++i) {
        const aiLight* light = scene->mLights[i];
        std::string light_name = getName(light);
        size += sizeof(Light) + light_name.size() + 1;
        lights.emplace_back(std::move(light_name), light);
    }

    // Materials & Textures
    size += scene->mNumMaterials * sizeof(Material*);
    for (unsigned int i = 0; i < scene->mNumMaterials; ++i) {
        const aiMaterial* mat = scene->mMaterials[i];
        std::string matname = getName(mat);
        size += sizeof(Material) + matname.size() + 1;

        AssimpMaterial mat_props(mat);

        const auto num_diffuse = mat->GetTextureCount(aiTextureType_DIFFUSE);
        const auto num_specular = mat->GetTextureCount(aiTextureType_SPECULAR);
        const auto num_exponent = mat->GetTextureCount(aiTextureType_SHININESS);
        const auto num_emissive = mat->GetTextureCount(aiTextureType_EMISSIVE);
        const auto num_normal = mat->GetTextureCount(aiTextureType_HEIGHT);
        const auto num_alpha = mat->GetTextureCount(aiTextureType_OPACITY);
        const auto num_ambient = mat->GetTextureCount(aiTextureType_AMBIENT);

        aiString ai_texname;
        if (num_diffuse != 0) {
            if (num_diffuse > 1)
                LOG_WARNING(logtag::Import, "Can't deal with multiple diffuse textures");

            mat->Get(AI_MATKEY_TEXTURE_DIFFUSE(0), ai_texname);
            auto texname = std::string(ai_texname.C_Str());
            const auto it = std::find(textures.begin(), textures.end(), texname);
            if (it == textures.end()) {
                mat_props.diffuse_tex = static_cast<int>(textures.size());
                textures.emplace_back(std::move(texname));
            } else {
                mat_props.diffuse_tex = static_cast<int>(std::distance(textures.begin(), it));
            }
        }
        if (num_specular != 0) {
            if (num_specular > 1)
                LOG_WARNING(logtag::Import, "Can't deal with multiple specular textures");

            mat->Get(AI_MATKEY_TEXTURE_SPECULAR(0), ai_texname);
            auto texname = std::string(ai_texname.C_Str());
            const auto it = std::find(textures.begin(), textures.end(), texname);
            if (it == textures.end()) {
                mat_props.specular_tex = static_cast<int>(textures.size());
                textures.emplace_back(std::move(texname));
            } else {
                mat_props.specular_tex = static_cast<int>(std::distance(textures.begin(), it));
            }
        }
        if (num_exponent != 0) {
            if (num_exponent > 1)
                LOG_WARNING(logtag::Import, "Can't deal with multiple specular exponent textures");

            mat->Get(AI_MATKEY_TEXTURE_SHININESS(0), ai_texname);
            auto texname = std::string(ai_texname.C_Str());
            const auto it = std::find(textures.begin(), textures.end(), texname);
            if (it == textures.end()) {
                mat_props.exponent_tex = static_cast<int>(textures.size());
                textures.emplace_back(std::move(texname));
            } else {
                mat_props.exponent_tex = static_cast<int>(std::distance(textures.begin(), it));
            }
        }
        if (num_emissive != 0) {
            if (num_emissive > 1)
                LOG_WARNING(logtag::Import, "Can't deal with multiple emissive textures");

            mat->Get(AI_MATKEY_TEXTURE_EMISSIVE(0), ai_texname);
            auto texname = std::string(ai_texname.C_Str());
            const auto it = std::find(textures.begin(), textures.end(), texname);
            if (it == textures.end()) {
                mat_props.emissive_tex = static_cast<int>(textures.size());
                textures.emplace_back(std::move(texname));
            } else {
                mat_props.emissive_tex = static_cast<int>(std::distance(textures.begin(), it));
            }
        }
        if (num_normal != 0) {
            if (num_normal > 1)
                LOG_WARNING(logtag::Import, "Can't deal with multiple normal textures");

            mat->Get(AI_MATKEY_TEXTURE_HEIGHT(0), ai_texname);
            auto texname = std::string(ai_texname.C_Str());
            const auto it = std::find(textures.begin(), textures.end(), texname);
            if (it == textures.end()) {
                mat_props.normal_tex = static_cast<int>(textures.size());
                textures.emplace_back(std::move(texname));
            } else {
                mat_props.normal_tex = static_cast<int>(std::distance(textures.begin(), it));
            }
        }
        if (num_alpha != 0) {
            if (num_alpha > 1)
                LOG_WARNING(logtag::Import, "Can't deal with multiple alpha textures");

            mat->Get(AI_MATKEY_TEXTURE_OPACITY(0), ai_texname);
            auto texname = std::string(ai_texname.C_Str());
            const auto it = std::find(textures.begin(), textures.end(), texname);
            if (it == textures.end()) {
                mat_props.alpha_tex = static_cast<int>(textures.size());
                textures.emplace_back(std::move(texname));
            } else {
                mat_props.alpha_tex = static_cast<int>(std::distance(textures.begin(), it));
            }
        }
        if (num_ambient != 0) {
            if (num_ambient > 1)
                LOG_WARNING(logtag::Import, "Can't deal with multiple ambient textures");

            mat->Get(AI_MATKEY_TEXTURE_AMBIENT(0), ai_texname);
            auto texname = std::string(ai_texname.C_Str());
            const auto it = std::find(textures.begin(), textures.end(), texname);
            if (it == textures.end()) {
                mat_props.ambient_tex = static_cast<int>(textures.size());
                textures.emplace_back(std::move(texname));
            } else {
                mat_props.ambient_tex = static_cast<int>(std::distance(textures.begin(), it));
            }
        }

        materials.emplace_back(std::move(matname), mat_props);
    }

    size += textures.size() * sizeof(Texture*);
    for (const auto& tex : textures) {
        size += sizeof(Texture) + tex.size() + 1;
    }

    // Meshes
    size += scene->mNumMeshes * sizeof(Mesh*);
    for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
        const aiMesh* mesh = scene->mMeshes[i];
        std::string meshname = getName(mesh);
        AssimpMesh mesh_props(mesh);
        const AssimpMaterial& mat = materials[mesh->mMaterialIndex].second;

        if (mesh->mPrimitiveTypes != aiPrimitiveType_TRIANGLE) {
            LOG_ERROR(logtag::Import, "Can't import non triangulated mesh");
            return 0;
        }

        size_t per_vertex_size = 2 * sizeof(glm::vec3); // position + normal

        if (mesh->HasNormals() == false) {
            LOG_ERROR(logtag::Import, "Can't import a mesh without normals");
            return 0;
        }
        if (mat.needsTexcoords()) {
            if (mesh->HasTextureCoords(0) == false) {
                LOG_ERROR(logtag::Import, "Textured mesh doesn't provide texture coordinates");
                return 0;
            }
            mesh_props.has_texcoords = true;
            per_vertex_size += sizeof(glm::vec2);
            if (mat.needsTangents()) {
                if (mesh->HasTangentsAndBitangents() == false) {
                    LOG_ERROR(logtag::Import, "Mesh doesn't have tangents for normal texture");
                    return 0;
                }
                mesh_props.has_tangents = true;
                per_vertex_size += sizeof(glm::vec3);
            }
        } else if (mesh->HasVertexColors(0)) {
            mesh_props.has_vertex_colors = true;
            per_vertex_size += sizeof(glm::vec3);
        }

        size += sizeof(Mesh) + meshname.size() + 1 +
                mesh->mNumVertices * per_vertex_size +
                mesh->mNumFaces * 3 * sizeof(std::uint32_t);

        meshes.emplace_back(std::move(meshname), mesh_props);
    }

    // nodes
    aiNode* stack[1024];
    stack[0] = scene->mRootNode;
    std::size_t pos = 1;
    do {
        const aiNode* node = stack[--pos];

        std::string nodename = getName(node);
        for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
            nodes.emplace_back(nodename + '_' + std::to_string(i),
                    AssimpNode(node, node->mMeshes[i]));
            size += sizeof(Node) + nodes.back().first.size() + 1;
        }
        for (unsigned int i = 0; i < node->mNumChildren; ++i) {
            stack[pos++] = node->mChildren[i];
        }
    } while (pos != 0);
    size += nodes.size() * sizeof(Node*);

    return size;
}

/***************************************************************************/

char* dumpCamera(char* ptr, const std::pair<std::string, const aiCamera*>& cam)
{
    Camera* my_cam = reinterpret_cast<Camera*>(ptr);
    ptr += sizeof(Camera);

    my_cam->name = ptr;
    my_cam->position = to_glm(cam.second->mPosition);
    my_cam->direction = to_glm(cam.second->mLookAt);
    my_cam->up = to_glm(cam.second->mUp);
    my_cam->aspect_ratio = cam.second->mAspect;
    my_cam->hfov = cam.second->mHorizontalFOV;

    std::strcpy(ptr, cam.first.c_str());
    ptr += cam.first.size() + 1;
    return ptr;
}

/***************************************************************************/

char* dumpMaterial(char* ptr, const std::pair<std::string, AssimpMaterial>& mat)
{
    Material* my_mat = reinterpret_cast<Material*>(ptr);
    ptr += sizeof(Material);

    my_mat->name = ptr;
    my_mat->diffuse_texture = mat.second.diffuse_tex;
    my_mat->specular_texture = mat.second.specular_tex;
    my_mat->exponent_texture = mat.second.exponent_tex;
    my_mat->normal_texture = mat.second.normal_tex;
    my_mat->emissive_texture = mat.second.emissive_tex;
    my_mat->alpha_texture = mat.second.alpha_tex;
    my_mat->ambient_texture = mat.second.ambient_tex;

    const aiMaterial* aimat = mat.second.ptr;
    aiColor3D color;
    if (AI_SUCCESS == aimat->Get(AI_MATKEY_COLOR_DIFFUSE, color)) {
        my_mat->diffuse_color = to_glm(color);
    } else {
        my_mat->diffuse_color = glm::vec3(.0f);
    }

    if (AI_SUCCESS == aimat->Get(AI_MATKEY_COLOR_SPECULAR, color)) {
        my_mat->specular_color = to_glm(color);
    } else {
        my_mat->specular_color = glm::vec3(.0f);
    }

    if (AI_SUCCESS == aimat->Get(AI_MATKEY_COLOR_AMBIENT, color)) {
        my_mat->ambient_color = to_glm(color);
    } else {
        my_mat->ambient_color = glm::vec3(.0f);
    }

    if (AI_SUCCESS == aimat->Get(AI_MATKEY_COLOR_EMISSIVE, color)) {
        my_mat->emissive_color = to_glm(color);
    } else {
        my_mat->emissive_color = glm::vec3(.0f);
    }

    if (AI_SUCCESS == aimat->Get(AI_MATKEY_COLOR_TRANSPARENT, color)) {
        my_mat->transparent_color = to_glm(color);
    } else {
        my_mat->transparent_color = glm::vec3(.0f);
    }

    if (AI_SUCCESS != aimat->Get(AI_MATKEY_SHININESS, my_mat->specular_exponent))
        my_mat->specular_exponent = 1.f;

    if (AI_SUCCESS != aimat->Get(AI_MATKEY_OPACITY, my_mat->opacity))
        my_mat->opacity = 1.f;

    std::strcpy(ptr, mat.first.c_str());
    ptr += mat.first.size() + 1;
    return ptr;
}

/***************************************************************************/

char* dumpTexture(char* ptr, const std::string& tex)
{
    Texture* my_tex = reinterpret_cast<Texture*>(ptr);
    ptr += sizeof(Texture);
    my_tex->name = ptr;
    my_tex->image = nullptr;
    std::strcpy(ptr, tex.c_str());
    ptr += tex.size() + 1;
    return ptr;
}

/***************************************************************************/

char* dumpMesh(char* ptr, const std::pair<std::string, AssimpMesh>& mesh)
{
    const aiMesh* aimesh = mesh.second.ptr;
    Mesh* my_mesh = reinterpret_cast<Mesh*>(ptr);
    ptr += sizeof(Mesh);

    my_mesh->name = ptr;
    std::strcpy(ptr, mesh.first.c_str());
    ptr += mesh.first.size() + 1;

    my_mesh->indices = reinterpret_cast<std::uint32_t*>(ptr);
    ptr += aimesh->mNumFaces * 3 * sizeof(std::uint32_t);
    for (unsigned int i = 0; i < aimesh->mNumFaces; ++i) {
        const std::size_t base = i * 3;
        my_mesh->indices[base    ] = aimesh->mFaces[i].mIndices[0];
        my_mesh->indices[base + 1] = aimesh->mFaces[i].mIndices[1];
        my_mesh->indices[base + 2] = aimesh->mFaces[i].mIndices[2];
    }

    my_mesh->vertices = reinterpret_cast<glm::vec3*>(ptr);
    ptr += aimesh->mNumVertices * sizeof(glm::vec3);
    my_mesh->bbox = core::AABB();
    for (unsigned int i = 0; i < aimesh->mNumVertices; ++i) {
        my_mesh->vertices[i] = to_glm(aimesh->mVertices[i]);
        my_mesh->bbox.expandBy(my_mesh->vertices[i]);
    }

    my_mesh->normals = reinterpret_cast<glm::vec3*>(ptr);
    ptr += aimesh->mNumVertices * sizeof(glm::vec3);
    for (unsigned int i = 0; i < aimesh->mNumVertices; ++i) {
        my_mesh->normals[i] = to_glm(aimesh->mNormals[i]);
    }

    if (mesh.second.has_tangents) {
        my_mesh->tangents = reinterpret_cast<glm::vec4*>(ptr);
        ptr += aimesh->mNumVertices * sizeof(glm::vec3);
        for (unsigned int i = 0; i < aimesh->mNumVertices; ++i) {
            const auto tangent = to_glm(aimesh->mTangents[i]);
            const auto normal = to_glm(aimesh->mNormals[i]);
            const auto bitan = to_glm(aimesh->mBitangents[i]);

            const auto bitan2 = glm::cross(normal, tangent);
            const auto sign = (glm::dot(bitan, bitan2) > .0f) ? 1.f : -1.f;
            my_mesh->tangents[i] = glm::vec4(tangent, sign);
        }
    } else {
        my_mesh->tangents = nullptr;
    }

    if (mesh.second.has_vertex_colors) {
        my_mesh->vertex_colors = reinterpret_cast<glm::vec3*>(ptr);
        ptr += aimesh->mNumVertices * sizeof(glm::vec3);
        for (unsigned int i = 0; i < aimesh->mNumVertices; ++i) {
            my_mesh->vertex_colors[i] = to_glm(aimesh->mColors[0][i]);
        }
    } else {
        my_mesh->vertex_colors = nullptr;
    }

    if (mesh.second.has_texcoords) {
        my_mesh->texcoords = reinterpret_cast<glm::vec2*>(ptr);
        ptr += aimesh->mNumVertices * sizeof(glm::vec2);
        for (unsigned int i = 0; i < aimesh->mNumVertices; ++i) {
            my_mesh->texcoords[i] = glm::vec2(to_glm(aimesh->mTextureCoords[0][i]));
        }
    } else {
        my_mesh->texcoords = nullptr;
    }

    my_mesh->material_index = aimesh->mMaterialIndex;
    my_mesh->num_vertices = aimesh->mNumVertices;
    my_mesh->num_indices = 3 * aimesh->mNumFaces;

    return ptr;
}

/***************************************************************************/

char* dumpLight(char* ptr, const std::pair<std::string, const aiLight*>& light)
{
    Light* my_light = reinterpret_cast<Light*>(ptr);
    ptr += sizeof(Light);

    my_light->name = ptr;
    my_light->position = to_glm(light.second->mPosition);
    my_light->direction = to_glm(light.second->mDirection);
    my_light->color = to_glm(light.second->mColorDiffuse);
    my_light->angle_inner_cone = light.second->mAngleInnerCone;
    my_light->angle_outer_cone = light.second->mAngleOuterCone;
    my_light->constant_attenuation = light.second->mAttenuationConstant;
    my_light->linear_attenuation = light.second->mAttenuationLinear;
    my_light->quadratic_attenuation = light.second->mAttenuationQuadratic;
    my_light->max_distance = std::numeric_limits<float>::infinity();
    switch (light.second->mType) {
    case aiLightSource_DIRECTIONAL:
        my_light->type = LightType::DIRECTIONAL;
        break;
    case aiLightSource_SPOT:
        my_light->type = LightType::SPOT;
        break;
    case aiLightSource_POINT:
    default:
        my_light->type = LightType::POINT;
        break;
    }

    std::strcpy(ptr, light.first.c_str());
    ptr += light.first.size() + 1;

    return ptr;
}

/***************************************************************************/

char* dumpNode(char* ptr, const std::pair<std::string, AssimpNode>& node)
{
    Node* my_node = reinterpret_cast<Node*>(ptr);
    ptr += sizeof(Node);

    my_node->name = ptr;
    std::strcpy(ptr, node.first.c_str());
    ptr += node.first.size() + 1;

    const aiNode* ainode = node.second.ptr;
    glm::mat4 transformation = to_glm(ainode->mTransformation);

    my_node->transformation = transformation;

    my_node->position = glm::vec3(transformation[3]);

    glm::mat3 rot{glm::uninitialize};

    glm::vec3 axis{transformation[0]};
    my_node->scale.x = glm::length(axis);
    rot[0] = axis / my_node->scale.x;

    axis = glm::vec3(transformation[1]);
    my_node->scale.y = glm::length(axis);
    rot[1] = axis / my_node->scale.y;

    axis = glm::vec3(transformation[2]);
    my_node->scale.z = glm::length(axis);
    rot[2] = axis / my_node->scale.z;

    if (glm::dot(rot[0], glm::cross(rot[1], rot[2])) < .0f) {
        my_node->scale.x *= -1.f;
        rot[0] = -rot[0];
    }

    my_node->rotation = glm::quat_cast(rot);

    my_node->mesh_index = node.second.idx;

    return ptr;
}

/***************************************************************************/

std::string getName(const aiString& name, const char* default_name, int& counter)
{
    if (name.length == 0) {
        return default_name + std::to_string(counter++);
    }
    return name.C_Str();
}

/***************************************************************************/

std::string getName(const aiCamera* cam)
{
    return getName(cam->mName, DEFAULT_CAM_NAME, cam_counter);
}

/***************************************************************************/

std::string getName(const aiLight* light)
{
    return getName(light->mName, DEFAULT_LIGHT_NAME, light_counter);
}

/***************************************************************************/

std::string getName(const aiMaterial* mat)
{
    aiString name;
    mat->Get(AI_MATKEY_NAME, name);
    return getName(name, DEFAULT_MATERIAL_NAME, material_counter);
}

/***************************************************************************/

std::string getName(const aiMesh* mesh)
{
    return getName(mesh->mName, DEFAULT_MESH_NAME, mesh_counter);
}

/***************************************************************************/

std::string getName(const aiNode* node)
{
    //return getName(node->mName, DEFAULT_NODE_NAME, node_counter);
    aiString tmp;
    return getName(tmp, DEFAULT_NODE_NAME, node_counter);
}

/***************************************************************************/

glm::vec3 to_glm(const aiVector3D& vec3)
{
    return glm::vec3(vec3.x, vec3.y, vec3.z);
}

/***************************************************************************/

glm::vec3 to_glm(const aiColor3D& col)
{
    return glm::vec3(col.r, col.g, col.b);
}

/***************************************************************************/

glm::vec3 to_glm(const aiColor4D& col)
{
    return glm::vec3(col.r, col.g, col.b);
}

/***************************************************************************/

glm::mat4 to_glm(const aiMatrix4x4& mat)
{
    const glm::mat4* tmp = reinterpret_cast<const glm::mat4*>(&mat);
    return glm::transpose(*tmp);

}

/***************************************************************************/

} // anonmymous namespace
