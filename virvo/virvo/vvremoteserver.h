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

#ifndef VVREMOTESERVER_H
#define VVREMOTESERVER_H

#include "vvexport.h"
#include "vvremoteevents.h"
#include "vvsocket.h"

class vvMatrix;
class vvRenderer;
class vvSocketIO;
class vvVolDesc;

class VIRVOEXPORT vvRemoteServer
{
public:
  enum ErrorType
  {
    VV_OK = 0,
    VV_SOCKET_ERROR,
    VV_FILEIO_ERROR,
    VV_RENDERCONTEXT_ERROR
  };

  vvRemoteServer(vvSocket *socket);
  virtual ~vvRemoteServer();

  virtual bool processEvent(virvo::RemoteEvent event, vvRenderer* renderer);
protected:
  vvSocketIO* _socketio;                    ///< socket for remote rendering

  int _codetype;

  virtual void renderImage(const vvMatrix& pr, const vvMatrix& mv, vvRenderer* renderer) = 0;
  virtual void resize(int w, int h) { (void)w; (void)h; }
private:
  vvRemoteServer::ErrorType initSocket();
};

#endif // VVREMOTESERVER_H
// vim: sw=2:expandtab:softtabstop=2:ts=2:cino=\:0g0t0
