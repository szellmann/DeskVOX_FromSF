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

#ifndef VV_INTERACTOR_H
#define VV_INTERACTOR_H

#include <virvo/vvvecmath.h>

class QMouseEvent;

class vvInteractor
{
public:
  vvInteractor();
  virtual ~vvInteractor();

  virtual void render() const {}
  virtual void mouseMoveEvent(QMouseEvent* /* event */) {}
  virtual void mousePressEvent(QMouseEvent* /* event */) {}
  virtual void mouseReleaseEvent(QMouseEvent* /* event */) {}

  void setEnabled(bool enabled);
  void setFocus();
  void clearFocus();
  void setVisible(bool visible);
  void setPos(const vvVector3& pos);

  bool enabled() const;
  bool hasFocus() const;
  bool visible() const;
  vvVector3 pos() const;
protected:
  bool _enabled;
  bool _hasFocus;
  bool _visible;
  vvVector3 _pos;
};

#endif

