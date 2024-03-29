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

#ifndef VV_RECT_H
#define VV_RECT_H

template <typename T>
class vvBaseRect
{
public:
  vvBaseRect();
  vvBaseRect(T x, T y, T width, T height);

  T values[4];

  bool contains(const vvBaseRect<T>& rhs);

  bool overlaps(const vvBaseRect<T>& rhs);
  void intersect(const vvBaseRect<T>& rhs);

  inline T &operator[](unsigned int i)
  {
    return values[i];
  }

  inline T operator[](unsigned int i) const
  {
    return values[i];
  }
};

typedef vvBaseRect<int> vvRecti;
typedef vvBaseRect<float> vvRectf;
typedef vvBaseRect<double> vvRectd;
typedef vvRecti vvRect;

namespace virvo
{
typedef vvRecti Viewport;
}

#include "vvrect.impl.h"

#endif // VV_RECT_H

