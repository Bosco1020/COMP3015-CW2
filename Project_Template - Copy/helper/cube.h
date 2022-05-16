#pragma once

#include "Additional files/drawable.h"
#include "Additional files/trianglemesh.h"

class Cube : public TriangleMesh
{
public:
    Cube(GLfloat size = 1.0f);
};
