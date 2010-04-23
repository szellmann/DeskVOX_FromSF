// Virvo - Virtual Reality Volume Rendering
// Copyright (C) 1999-2003 University of Stuttgart, 2004-2005 Brown University
// Contact: Jurgen P. Schulze, jschulze@ucsd.edu
//
// This file is part of Virvo.
//
// Virvo is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library (see license.txt); if not, write to the
// Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

#include "vvbrick.h"
#include "vvtexrend.h"

#include <math.h>

void Brick::render(vvTexRend* renderer, const vvVector3& normal,
                   const vvVector3& farthest, const vvVector3& delta,
                   const vvVector3& probeMin, const vvVector3& probeMax,
                   GLuint*& texNames, vvShaderManager* isectShader, const bool setupEdges)
{
  vvVector3 dist = max;
  dist.sub(&min);

  vvVector3 texRange;
  vvVector3 texMin;

  glBindTexture(GL_TEXTURE_3D_EXT, texNames[index]);

  vvVector3 texPoint;
  texPoint.copy(&farthest);

  if(renderer->_proxyGeometryOnGpu)
  {
    glEnableClientState(GL_VERTEX_ARRAY);

    // Clip probe object to brick extends.
    vvVector3 minClipped;
    vvVector3 maxClipped;

    for (int i = 0; i < 3; ++i)
    {
      if (min[i] < probeMin[i])
      {
        minClipped[i] = probeMin[i];
      }
      else
      {
        minClipped[i] = min[i];
      }

      if (max[i] > probeMax[i])
      {
        maxClipped[i] = probeMax[i];
      }
      else
      {
        maxClipped[i] = max[i];
      }
      const float overlapNorm = (float)(brickTexelOverlap[i]) / (float)texels[i];
      texRange[i] = (1.0f - overlapNorm);
      texMin[i] = (1.0f / (2.0f * (float)(renderer->_renderState._brickTexelOverlap) * (float)texels[i]));
    }

    vvAABB box(minClipped, maxClipped);
    const vvVector3 (&verts)[8] = box.calcVertices();

    float minDot;
    float maxDot;
    const ushort idx = getFrontIndex(verts, texPoint, normal, minDot, maxDot);
    isectShader->setArrayParameter3f(0, "vertices", 0, verts[0].e[0], verts[0].e[1], verts[0].e[2]);
    if(setupEdges)
    {
      for (int i = 1; i < 8; ++i)
      {
        isectShader->setArrayParameter3f(0, "vertices", i, verts[i].e[0]-verts[0].e[0],
                                                           verts[i].e[1]-verts[0].e[1],
                                                           verts[i].e[2]-verts[0].e[2]);
      }
    }

    isectShader->setParameter4f(0, "brickMin", min[0], min[1], min[2], -texPoint.length());
    isectShader->setParameter3f(0, "brickDimInv", 1.0f/dist[0], 1.0f/dist[1], 1.0f/dist[2]);
    // Mind that textures overlap a little bit for correct interpolation at the borders.
    // Thus add that little difference.
    isectShader->setParameter3f(0, "texRange", texRange[0], texRange[1], texRange[2]);
    isectShader->setParameter3f(0, "texMin", texMin[0], texMin[1], texMin[2]);

    const float deltaInv = 1.0f / delta.length();

    const int startSlices = (int)ceilf(minDot * deltaInv);
    const int endSlices = (int)floorf(maxDot * deltaInv);

    isectShader->setParameter1i(0, "frontIndex", idx);

    const int primCount = (endSlices - startSlices) + 1;

    glVertexPointer(2, GL_INT, 0, &renderer->_vertArray[startSlices*12]);
    glMultiDrawElements(GL_POLYGON, &renderer->_elemCounts[0], GL_UNSIGNED_INT, (const GLvoid**)&renderer->_vertIndices[0], primCount);

    glDisableClientState(GL_VERTEX_ARRAY);
  }
  else // render proxy geometry on gpu? else then:
  {
    // Clip probe object to brick extends.
    vvVector3 minClipped;
    vvVector3 maxClipped;

    for (int i = 0; i < 3; ++i)
    {
      if (min[i] < probeMin[i])
      {
        minClipped[i] = probeMin[i];
      }
      else
      {
        minClipped[i] = min[i];
      }

      if (max[i] > probeMax[i])
      {
        maxClipped[i] = probeMax[i];
      }
      else
      {
        maxClipped[i] = max[i];
      }
      const float overlapNorm = (float)(brickTexelOverlap[i]) / (float)texels[i];
      texRange[i] = (1.0f - overlapNorm);
      texMin[i] = (1.0f / (2.0f * (float)(renderer->_renderState._brickTexelOverlap) * (float)texels[i]));
    }

    vvAABB box(minClipped, maxClipped);
    const vvVector3 (&verts)[8] = box.calcVertices();

    // Abuse getFrontIndex to calcuate minDot and maxDot.
    float minDot;
    float maxDot;
    getFrontIndex(verts, texPoint, normal, minDot, maxDot);

    const float deltaInv = 1.0f / delta.length();

    const int startSlices = (int)ceilf(minDot * deltaInv);
    const int endSlices = (int)floorf(maxDot * deltaInv);

    vvVector3 startPoint;
    startPoint.copy(farthest);
    vvVector3 add;
    add.copy(delta);
    add.scale(startSlices);
    startPoint.add(&add);

    for (int i = startSlices; i <= endSlices; ++i)
    {
      vvVector3 isect[6];
      const int isectCnt = isect->isectPlaneCuboid(&normal, &startPoint, &minClipped, &maxClipped);
      startPoint.add(&delta);

      if (isectCnt < 3) continue;                 // at least 3 intersections needed for drawing

      // Check volume section mode:
      if (renderer->minSlice != -1 && i < renderer->minSlice) continue;
      if (renderer->maxSlice != -1 && i > renderer->maxSlice) continue;

      // Put the intersecting 3 to 6 vertices in cyclic order to draw adjacent
      // and non-overlapping triangles:
      isect->cyclicSort(isectCnt, &normal);

      // Generate vertices in texture coordinates:
      vvVector3 texcoord[6];
      for (int j = 0; j < isectCnt; ++j)
      {
        for (int k = 0; k < 3; ++k)
        {
          texcoord[j][k] = (isect[j][k] - min.e[k]) / dist[k];
          texcoord[j][k] = texcoord[j][k] * texRange[k] + texMin[k];
        }
      }

      glBegin(GL_TRIANGLE_FAN);
      glColor4f(1.0, 1.0, 1.0, 1.0);
      glNormal3f(normal[0], normal[1], normal[2]);
      for (int j = 0; j < isectCnt; ++j)
      {
        // The following lines are the bottleneck of this method:
        glTexCoord3f(texcoord[j][0], texcoord[j][1], texcoord[j][2]);
        glVertex3f(isect[j][0], isect[j][1], isect[j][2]);
      }
      glEnd();
    }
  }
}

void Brick::renderOutlines(const vvVector3& probeMin, const vvVector3& probeMax)
{
  vvVector3 minClipped;
  vvVector3 maxClipped;
  for (int i = 0; i < 3; i++)
  {
    if (min.e[i] < probeMin.e[i])
    {
      minClipped.e[i] = probeMin.e[i];
    }
    else
    {
      minClipped.e[i] = min.e[i];
    }

    if (max.e[i] > probeMax.e[i])
    {
      maxClipped.e[i] = probeMax.e[i];
    }
    else
    {
      maxClipped.e[i] = max.e[i];
    }
  }
  glBegin(GL_LINES);
    glColor3f(1.0f, 1.0f, 1.0f);
    glVertex3f(minClipped[0], minClipped[1], minClipped[2]);
    glVertex3f(maxClipped[0], minClipped[1], minClipped[2]);

    glVertex3f(minClipped[0], minClipped[1], minClipped[2]);
    glVertex3f(minClipped[0], maxClipped[1], minClipped[2]);

    glVertex3f(minClipped[0], minClipped[1], minClipped[2]);
    glVertex3f(minClipped[0], minClipped[1], maxClipped[2]);

    glVertex3f(maxClipped[0], maxClipped[1], maxClipped[2]);
    glVertex3f(minClipped[0], maxClipped[1], maxClipped[2]);

    glVertex3f(maxClipped[0], maxClipped[1], maxClipped[2]);
    glVertex3f(maxClipped[0], minClipped[1], maxClipped[2]);

    glVertex3f(maxClipped[0], maxClipped[1], maxClipped[2]);
    glVertex3f(maxClipped[0], maxClipped[1], minClipped[2]);

    glVertex3f(maxClipped[0], minClipped[1], minClipped[2]);
    glVertex3f(maxClipped[0], maxClipped[1], minClipped[2]);

    glVertex3f(maxClipped[0], minClipped[1], minClipped[2]);
    glVertex3f(maxClipped[0], minClipped[1], maxClipped[2]);

    glVertex3f(minClipped[0], maxClipped[1], minClipped[2]);
    glVertex3f(maxClipped[0], maxClipped[1], minClipped[2]);

    glVertex3f(minClipped[0], maxClipped[1], minClipped[2]);
    glVertex3f(minClipped[0], maxClipped[1], maxClipped[2]);

    glVertex3f(minClipped[0], minClipped[1], maxClipped[2]);
    glVertex3f(maxClipped[0], minClipped[1], maxClipped[2]);

    glVertex3f(minClipped[0], minClipped[1], maxClipped[2]);
    glVertex3f(minClipped[0], maxClipped[1], maxClipped[2]);
  glEnd();
}

bool Brick::upload3DTexture(GLuint& texName, uchar* texData,
                            GLenum texFormat, GLint internalTexFormat,
                            const bool interpolation)
{
  glBindTexture(GL_TEXTURE_3D_EXT, texName);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  glTexParameteri(GL_TEXTURE_3D_EXT, GL_TEXTURE_MAG_FILTER, (interpolation) ? GL_LINEAR : GL_NEAREST);
  glTexParameteri(GL_TEXTURE_3D_EXT, GL_TEXTURE_MIN_FILTER, (interpolation) ? GL_LINEAR : GL_NEAREST);
  glTexParameteri(GL_TEXTURE_3D_EXT, GL_TEXTURE_WRAP_R_EXT, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_3D_EXT, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_3D_EXT, GL_TEXTURE_WRAP_T, GL_CLAMP);

  glTexImage3D(GL_PROXY_TEXTURE_3D_EXT, 0, internalTexFormat,
    texels[0], texels[1], texels[2], 0, texFormat, GL_UNSIGNED_BYTE, NULL);

  GLint glWidth;
  glGetTexLevelParameteriv(GL_PROXY_TEXTURE_3D_EXT, 0, GL_TEXTURE_WIDTH, &glWidth);
  if (glWidth != 0)
  {
    glTexImage3D(GL_TEXTURE_3D_EXT, 0, internalTexFormat, texels[0], texels[1], texels[2], 0,
      texFormat, GL_UNSIGNED_BYTE, texData);
    return true;
  }
  else
  {
    return false;
  }
}

/** Get front index of the brick based upon the current modelview matrix.
  @param vertices  NOTE THE FOLLOWING: albeit vertices are constituents of
                   bricks, these are passed along with the function call in
                   order not to recalculate them. This is due to the fact that
                   AABBs actually don't store pointers to their eight verts,
                   but only to two corners.
  @param point     The point to calc the distance from. The distance of interest
                   is the one from point to the first and last vertex along
                   normal
  @param normal    The normal to calc the distance along.
  @param minDot    The minimum dot product of vector point-vertex and normal,
                   passed along for later calculations.
  @param maxDot    The maximum dot product of vector point-vertex and normal,
                   passed along for later calculations.
*/
ushort Brick::getFrontIndex(const vvVector3* vertices,
                            const vvVector3& point,
                            const vvVector3& normal,
                            float& minDot,
                            float& maxDot)
{

  // Get vertices with max and min distance to point along normal.
  maxDot = -FLT_MAX;
  minDot = FLT_MAX;
  ushort frontIndex;

  for (int i=0; i<8; i++)
  {
    vvVector3 v = vertices[i];
    v.sub(&point);
    const float dot = v.dot(&normal);
    if (dot > maxDot)
    {
      maxDot = dot;
      frontIndex = i;
    }

    if (dot < minDot)
    {
      minDot = dot;
    }
  }
  return frontIndex;
}

void Brick::sortByCenter(Brick** bricks, const int numBricks, const vvVector3& axis)
{
  Brick* tmp;
  Brick* tmp2;
  Brick* tmp3;
  vvVector3* axisGetter;
  int i, j, k;
  int a;

  axisGetter = new vvVector3(0, 1, 2);
  a = axis.dot(axisGetter);
  delete axisGetter;

  // Selection sort.
  for (i = 0; i < numBricks; ++i)
  {
    for (j = i; j < numBricks; ++j)
    {
      tmp = bricks[j];
      for (k = i + 1; k < numBricks; ++k)
      {
        tmp2 = bricks[k];
        if (tmp->getAABB().calcCenter().e[a] > tmp2->getAABB().calcCenter().e[a])
        {
          tmp3 = bricks[j];
          bricks[j] = bricks[k];
          bricks[k] = tmp3;
        }
      }
    }
  }
}