// Virvo - Virtual Reality Volume Rendering
// Copyright (C) 2010 University of Cologne
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

#ifndef VV_CUDA_H
#define VV_CUDA_H

#include "vvexport.h"
#include "vvplatform.h"
#include <cstdlib>

class VIRVOEXPORT vvCuda
{
public:
  static bool init();
  static bool initGlInterop();
};

#endif // VV_CUDA_H

//============================================================================
// End of File
//============================================================================

// vim: sw=2:expandtab:softtabstop=2:ts=2:cino=\:0g0t0
