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

#ifndef _VV_SERVER_H_
#define _VV_SERVER_H_

#include <virvo/vvremoteevents.h>
#include <virvo/vvrendererfactory.h>
#include <virvo/vvrendercontext.h>
#include <virvo/vvrequestmanagement.h>

#include <string>

class vvResourceManager;
class vvRemoteServer;
class vvSocketIO;
class vvTcpSocket;
class vvVolDesc;

/**
 * Virvo Server main class.
 *
 * Server unit for remote rendering.
 *
 * Options:
 *  -port:    set port, else default will be used.
 *  -mode:    choose server mode (default: server only)
 *  -bonjour: Use bonjour to broadcast this service (default: off)
 *  -debug:   set debug level, else default will be used.  *
 * @author Juergen Schulze (schulze@cs.brown.de)
 * @author Stavros Delisavas (stavros.delisavas@uni-koeln.de)
 */
class vvServer
{
public:
  static const int            DEFAULTSIZE;   ///< default window size (width and height) in pixels
  static const unsigned short DEFAULT_PORT;  ///< default port for socket connections
  enum ServerRequest
  {
    STATUS,
    RENDERING
  };

  enum ServerMode
  {
    SERVER,
    RM,
    RM_WITH_SERVER
  };

  vvServer(bool useBonjour);
  virtual ~vvServer();

  /** Main Virvo server routine.
    @param argc,argv command line arguments
    @return 0 if the program finished ok, 1 if an error occurred
  */
  int run(int argc, char** argv);

  void setPort(unsigned short port);

protected:
  struct ThreadData
  {
    ThreadData();
    ~ThreadData();

    vvResourceManager *instance;
    vvRenderContext   *renderContext;
    vvContextOptions   contextOptions;
    vvRemoteServer    *server;
    vvRenderer::RendererType remoteServerType;
    std::string        renderertype;
    vvRenderer        *renderer;
    vvVolDesc         *vd;
    vvRequest         *request;
    vvRendererFactory::Options opt;
  };
  void displayHelpInfo();                         ///< Display command usage help on the command line.
  bool parseCommandLine(int argc, char *argv[]);  ///< Parse command line arguments.
  virtual bool serverLoop();
  virtual void handleNextConnection(vvTcpSocket *sock) = 0;
  virtual bool handleEvent(ThreadData *tData, virvo::RemoteEvent, const vvSocketIO& io);

  bool createRenderContext(ThreadData* tData, const int w, const int h);
  virtual bool createRemoteServer(ThreadData* tData, vvTcpSocket* sock);

  unsigned short   _port;         ///< port the server renderer uses to listen for incoming connections
  ServerMode       _sm;           ///< indicating current server mode (default: single server)
  bool             _useBonjour;   ///< indicating the use of bonjour
  bool             _daemonize;    ///< run in background as a unix daemon
  std::string      _daemonName;   ///< name of the daemon for reference in syslog

  static void handleSignal(int sig);               ///< Handle signals sent to a daemon
};

#endif // _VV_SERVER_H_

//===================================================================
// End of File
//===================================================================
// vim: sw=2:expandtab:softtabstop=2:ts=2:cino=\:0g0t0
