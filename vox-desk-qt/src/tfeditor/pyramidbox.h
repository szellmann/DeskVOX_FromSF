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

#pragma once

#include <virvo/vvcolor.h>
#include <virvo/vvvecmath.h>

#include <QGroupBox>

namespace tf
{
class PyramidBox : public QGroupBox
{
  Q_OBJECT
public:
  PyramidBox(QWidget* parent = 0);
  ~PyramidBox();

  void setHasColor(bool hascolor);
  void setColor(const vvColor& color);
  void setTop(const vvVector3& top);
  void setBottom(const vvVector3& bottom);
  void setOpacity(float opacity);
private:
  struct Impl;
  Impl* impl;
private slots:
  void getColor();
  void emitTop(int sliderval);
  void emitBottom(int sliderval);
  void emitOpacity(int sliderval);
signals:
  void hasColor(bool hascolor);
  void color(const QColor& color);
  void top(const vvVector3& top);
  void bottom(const vvVector3& bottom);
  void opacity(float opacity);
};
}

