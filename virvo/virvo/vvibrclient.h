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

#ifndef _VV_IBRCLIENT_H_
#define _VV_IBRCLIENT_H_

#include "vvexport.h"
#include "vvopengl.h"
#include "vvremoteclient.h"

#include <vector>

class vvRenderer;
class vvSlaveVisitor;
class vvVolDesc;
class vvShaderFactory;
class vvShaderProgram;
class vvIbrImage;

class VIRVOEXPORT vvIbrClient : public vvRemoteClient
{
public:
  vvIbrClient(vvVolDesc *vd, vvRenderState renderState,
              vvTcpSocket* socket, const std::string& filename = "");
  ~vvIbrClient();

  ErrorType render();                                     ///< render image with depth-values
  virtual void setParameter(ParameterType param, const vvParam& newValue);

private:
  struct Thread;
  Thread *_thread;                                        ///< threads, mutexes, barriers, ...
  bool   _newFrame;                                       ///< flag indicating a new ibr-frame waiting to be rendered
  bool   _haveFrame;                                      ///< flag indicating that at least one frame has been received
  bool   _synchronous;                                    ///< display what was received w/y delay
  vvIbrImage *_image;                                     ///< image, protected by _imageMutex
  GLuint _pointVBO;                                       ///< Vertex Buffer Object id for point-pixels

  GLuint _rgbaTex;                                        ///< Texture names for RGBA image
  GLuint _depthTex;                                       ///< Texture names for depth image

  std::vector<GLuint> _indexArray[4];                     ///< four possible traversal directions for drawing the vertices

  vvMatrix _imgMatrix;                                    ///< Reprojection matrix of _ibrImg
  vvMatrix _imgMv;                                        ///< model-view matrix of _ibrImg
  vvMatrix _imgPr;                                        ///< Projection matrix of _ibrImg
  virvo::Viewport _imgVp;                                 ///< Viewport of _ibrImg
  float _imgDepthRange[2];                                ///< Depth range of _ibrImg
  void initIbrFrame();                                    ///< initialize pixel-points in object space

  vvShaderProgram* _shader;

  void createThreads();                                   ///< creates threads for every socket connection
  void destroyThreads();                                  ///< quits threads
  static void* getImageFromSocket(void* threadargs);      ///< get image from socket connection and wait for next
};

#endif
// vim: sw=2:expandtab:softtabstop=2:ts=2:cino=\:0g0t0
