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

#ifndef _VVBRICK_H_
#define _VVBRICK_H_

#include "vvaabb.h"
#include "vvexport.h"
#include "vvinttypes.h"
#include "vvopengl.h"

#include <limits.h>
#include <vector>

class vvTexRend;
class vvShaderProgram;

class VIRVOEXPORT vvBrick
{
public:
  vvBrick()                                     ///< dflt. constructor (needed for C++ templates)
  : minValue(INT_MAX)
  , maxValue(INT_MIN)
  , visible(true)
  , insideProbe(true)
  , index(-1)
  , dist(-1.f)
  {
    startOffset[0] = startOffset[1] = startOffset[2] = 0;
    texels[0] = texels[1] = texels[2] = 0;
  }

  vvBrick(const vvBrick* rhs)                   ///< copy constructor (from ptr)
  {
    pos = vvVector3(&rhs->pos);
    min = vvVector3(&rhs->min);
    max = vvVector3(&rhs->max);
    texRange = vvVector3(&rhs->texRange);
    texMin = vvVector3(&rhs->texMin);
    minValue = rhs->minValue;
    maxValue = rhs->maxValue;
    visible = rhs->visible;
    insideProbe = rhs->insideProbe;
    index = rhs->index;
    startOffset[0] = rhs->startOffset[0];
    startOffset[1] = rhs->startOffset[1];
    startOffset[2] = rhs->startOffset[2];
    texels[0] = rhs->texels[0];
    texels[1] = rhs->texels[1];
    texels[2] = rhs->texels[2];
    dist = rhs->dist;
  }

  inline bool operator<(const vvBrick& rhs) const      ///< compare bricks based upon dist to eye position
  {
    return (dist < rhs.dist);
  }

  void render(vvTexRend* const renderer, const vvVector3& normal,
              const vvVector3& farthest, const vvVector3& delta,
              const vvVector3& probeMin, const vvVector3& probeMax,
              GLuint*& texNames, vvShaderProgram* shader) const;

  void renderOutlines(const vvVector3& probeMin, const vvVector3& probeMax) const;
  bool upload3DTexture(const GLuint& texName, const uchar* texData,
                       GLenum texFormat, GLint internalTexFormat,
                       bool interpolation = true) const;

  vvAABB getAABB() const
  {
    return vvAABB(min, max);
  }

  ushort getFrontIndex(const vvVector3* vertices,   ///< front index of the brick dependent upon the current model view
                       const vvVector3& point,
                       const vvVector3& normal,
                       float& minDot,
                       float& maxDot) const;

  void print() const;

  static void sortByCenter(std::vector<vvBrick*>& bricks,
                           const vvVector3& axis);
                                                    ///< and assuming that vertices are ordered back to front
  vvVector3 pos;                                    ///< center position of brick
  vvVector3 min;                                    ///< minimum position of brick
  vvVector3 max;                                    ///< maximum position of brick
  vvVector3 texRange;
  vvVector3 texMin;
  int minValue;                                     ///< min scalar value after lut, needed for empty space leaping
  int maxValue;                                     ///< max scalar value after lut, needed for empty space leaping
  bool visible;                                     ///< if brick isn't visible, it won't be rendered at all
  bool insideProbe;                                 ///< true iff brick is completely included inside probe
  int index;                                        ///< index for texture object
  int startOffset[3];                               ///< startvoxel of brick
  int texels[3];                                    ///< number of texels in each dimension
  float dist;                                       ///< distance from plane given by eye and normal
                                                    ///< or distance from (0|0|0) to brick center. Both used for sorting

  struct Compare
  {
    bool operator()(vvBrick *a, vvBrick *b) const
    {
      return a->dist > b->dist;
    }
  };
};

typedef std::vector<vvBrick*> BrickList;

#endif // _VVBRICK_H_
// vim: sw=2:expandtab:softtabstop=2:ts=2:cino=\:0g0t0
