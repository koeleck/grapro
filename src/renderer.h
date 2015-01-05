#ifndef RENDERER_H
#define RENDERER_H

#include <vector>
#include <unordered_map>
#include "core/instance.h"
#include "core/program.h"
#include "gl/opengl.h"

class Renderer
{
public:
    Renderer();
    ~Renderer();

    void setGeometry(std::vector<const core::Instance*> geometry);
    void render();

private:
    using ProgramMap = std::unordered_map<unsigned char, core::Program>;

    std::vector<const core::Instance*>  m_geometry;
    ProgramMap                          m_programs;

};

#endif // RENDERER_H

