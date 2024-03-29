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

#ifndef _VV_BSPTREEVISITORS_H_
#define _VV_BSPTREEVISITORS_H_

#include "vvopengl.h"
#include "vvvisitor.h"

class vvImage;
class vvRenderer;

#include <vector>

/*!
 sort-last alpha compositing visitor
 */
class vvSortLastVisitor : public vvVisitor
{
public:
  struct Texture
  {
    vvRecti* rect;
    std::vector<float>* pixels;
  };

  vvSortLastVisitor();
  ~vvSortLastVisitor();
  void visit(vvVisitable* obj) const;

  void setTextures(const std::vector<Texture>& textures);
private:
  std::vector<Texture> _textures;
};

/*!
 simple visitor drawing bsp-nodes (aka bricks) using OpenGL
 */
class vvSimpleRenderVisitor : public vvVisitor
{
public:
  vvSimpleRenderVisitor(const std::vector<vvRenderer*>& renderers);
  ~vvSimpleRenderVisitor();
  void visit(vvVisitable* obj) const;
private:
  std::vector<vvRenderer*> _renderers;
};

/*!
 visitor drawing the outlines of the bsp-nodes
 */
class vvShowBricksVisitor : public vvVisitor
{
public:
  vvShowBricksVisitor(vvVolDesc* vd);
  ~vvShowBricksVisitor();
  void visit(vvVisitable* obj) const;
private:
  vvVolDesc* _vd;
};

#endif
// vim: sw=2:expandtab:softtabstop=2:ts=2:cino=\:0g0t0
