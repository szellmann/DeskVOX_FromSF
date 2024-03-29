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

#include <cmath>
#include <set>

#include "vvbsptree.h"
#include "vvdebugmsg.h"
#include "vvopengl.h"
#include "vvtoolshed.h"
#include "vvvoldesc.h"

#include "private/vvgltools.h"

//============================================================================
// vvBspNode Method Definitions
//============================================================================

vvBspNode::vvBspNode(const vvAABBs& aabb)
  : _aabb(aabb)
{
  _childLeft = NULL;
  _childRight = NULL;
}

vvBspNode::~vvBspNode()
{
  delete _childLeft;
  delete _childRight;
}

void vvBspNode::accept(vvVisitor* visitor)
{
  visitor->visit(this);
}

void vvBspNode::addChild(vvBspNode* child)
{
  if (_childLeft == NULL)
  {
    _childLeft = child;
  }
  else
  {
    _childRight = child;
  }
}

bool vvBspNode::isLeaf() const
{
  return (_childLeft == NULL && _childRight == NULL);
}

void vvBspNode::setId(size_t id)
{
  _id = id;
}

void vvBspNode::setAabb(const vvAABBs& aabb)
{
  _aabb = aabb;
}

size_t vvBspNode::getId() const
{
  return _id;
}

vvBspNode* vvBspNode::getChildLeft() const
{
  return _childLeft;
}

vvBspNode* vvBspNode::getChildRight() const
{
  return _childRight;
}

const vvAABBs& vvBspNode::getAabb() const
{
  return _aabb;
}

void vvBspNode::clipProbe(vvVector3& probeMin, vvVector3& probeMax,
                          vvVector3&, vvVector3&) const
{
  vvsize3 probeMinI(static_cast<size_t>(probeMin[0]),
                    static_cast<size_t>(probeMin[1]),
                    static_cast<size_t>(probeMin[2]));

  vvsize3 probeMaxI(static_cast<size_t>(probeMax[0]),
                    static_cast<size_t>(probeMax[1]),
                    static_cast<size_t>(probeMax[2]));

  vvAABBs probe(probeMinI, probeMaxI);
  probe.intersect(_aabb);

  for (size_t i = 0; i < 3; ++i)
  {
    probeMin[i] = static_cast<float>(probe.getMin()[i]);
    probeMax[i] = static_cast<float>(probe.getMax()[i]);
  }
}

//============================================================================
// vvBspTree Method Definitions
//============================================================================

vvBspTree::vvBspTree(vvVolDesc* vd, const vvBspData& data)
  : _vd(vd)
  , _root(NULL)
  , _visitor(NULL)
  , _data(data)
{
  // load balance vector is optional
  if (_data.loadBalance.size() < 1)
  {
    const float fraction = 1.0f / static_cast<float>(_data.numLeafs);

    for (size_t i = 0; i < _data.numLeafs; ++i)
    {
      _data.loadBalance.push_back(fraction);
    }
  }

  float totalLoad = 0.0f;
  for (std::vector<float>::const_iterator it = _data.loadBalance.begin();
       it != _data.loadBalance.end(); ++it)
  {
    totalLoad += *it;
  }

  // check if total load sums up to appr. 1
  if ((totalLoad < 1.0f - FLT_EPSILON) || (totalLoad > 1.0f + FLT_EPSILON))
  {
    _root = NULL;
    vvDebugMsg::msg(0, "vvBspTree::vvBspTree() - Error: load balance must sum up to 1: ", totalLoad);
    return;
  }

  vvsize3 voxMin(0, 0, 0);
  vvsize3 voxMax = vd->vox;
  _leafs.resize(_data.loadBalance.size());

  if (_leafs.size() < 1)
  {
    vvDebugMsg::msg(0, "vvBspTree::vvBspTree() - Error: no leafs");
    return;
  }

  if (_leafs.size() > 1)
  {
    _root = new vvBspNode(vvAABBs(voxMin, voxMax));
    buildHierarchy(_root, 0);
  }
  else
  {
    _root = new vvBspNode(vvAABBs(voxMin, voxMax));
    _root->setId(0);
    _leafs[0] = _root;
  }
}

vvBspTree::~vvBspTree()
{
  // only delete _root and _leafs, the renderer is responsible
  // for deleting the single _visitor instance
  delete _root;
}

void vvBspTree::traverse(const vvsize3& pos) const
{
  traverse(pos, _root);
}

const std::vector<vvBspNode*>& vvBspTree::getLeafs() const
{
  return _leafs;
}

void vvBspTree::setVisitor(vvVisitor* visitor)
{
  _visitor = visitor;
}

void vvBspTree::buildHierarchy(vvBspNode* node, size_t leafIdx)
{
  const float fraction = calcRelativeFraction(leafIdx);
  const vvAABBs aabb = node->getAabb();
  vvVecmath::AxisType axis;
  const size_t length = aabb.getLongestSide(axis);
  const float split = static_cast<float>(length) * fraction;
  std::pair<vvAABBs, vvAABBs> splitted = aabb.split(axis, static_cast<size_t>(split));

  if (leafIdx == _leafs.size() - 2)
  {
    vvBspNode* childLeft = new vvBspNode(splitted.first);
    vvBspNode* childRight = new vvBspNode(splitted.second);
    node->addChild(childLeft);
    node->addChild(childRight);

    // make left child a leaf
    _leafs.at(leafIdx) = childLeft;
    childLeft->setId(leafIdx);

    // make right child a leaf
    _leafs.at(leafIdx + 1) = childRight;
    childRight->setId(leafIdx + 1);
  }
  else if (leafIdx < _leafs.size() - 2)
  {
    vvBspNode* childLeft = new vvBspNode(splitted.first);
    vvBspNode* childRight = new vvBspNode(splitted.second);
    node->addChild(childLeft);
    node->addChild(childRight);

    // make left child a leaf
    _leafs.at(leafIdx) = childLeft;
    childLeft->setId(leafIdx);
    
    // recurse with right child
    buildHierarchy(childRight, leafIdx + 1);
  }
}

float vvBspTree::calcRelativeFraction(size_t leafIdx)
{
  float total = 0.0f;
  for (size_t i = leafIdx; i < _leafs.size(); ++i)
  {
    total += _data.loadBalance[i];
  }
  return _data.loadBalance[leafIdx] / total;
}

void vvBspTree::traverse(const vvsize3& pos, vvBspNode* node) const
{
  if (node->isLeaf())
  {
    // since this bsp tree implementation utilized the visitor
    // pattern, rendering is initiated using the visit() / accept()
    // mechanism of the half space node / its visitor
    node->accept(_visitor);
  }
  else
  {
    vvsize3 minval = node->getChildLeft()->getAabb().getMin();
    vvsize3 maxval = node->getChildLeft()->getAabb().getMax();

    for (size_t i = 0; i < 3; ++i)
    {
      if (minval[i] == node->getAabb().getMin()[i])
      {
        minval[i] = 0;
      }

      if (maxval[i] == node->getAabb().getMax()[i])
      {
        maxval[i] = std::numeric_limits<size_t>::max();
      }
    }
    const vvAABBs aabb(minval, maxval);

    // back-to-front traversal
    if (aabb.contains(pos))
    {
      traverse(pos, node->getChildRight());
      traverse(pos, node->getChildLeft());
    }
    else
    {
      traverse(pos, node->getChildLeft());
      traverse(pos, node->getChildRight());
    }
  }
}
// vim: sw=2:expandtab:softtabstop=2:ts=2:cino=\:0g0t0
