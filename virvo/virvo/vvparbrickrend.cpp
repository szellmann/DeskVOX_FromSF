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

#include <GL/glew.h>

#include "vvbsptree.h"
#include "vvbsptreevisitors.h"
#include "vvdebugmsg.h"
#include "vvparbrickrend.h"
#include "vvpthread.h"
#include "vvrendercontext.h"
#include "vvsocketmap.h"
#include "vvtcpsocket.h"
#include "vvvoldesc.h"

#include "private/vvgltools.h"

#include <queue>
#include <sstream>

struct vvParBrickRend::Thread
{
  Thread()
    : parbrickrend(NULL)
    , renderer(NULL)
    , aabb(vvVector3(), vvVector3())
  {
  }

  vvContextOptions contextOptions;

  vvTcpSocket* socket;
  std::string filename;

  vvParBrickRend* parbrickrend;
  vvRenderer* renderer;
  vvSortLastVisitor::Texture texture;

  size_t id;
  pthread_t threadHandle;
  pthread_barrier_t* barrier;
  pthread_mutex_t* mutex;

  vvAABB aabb;

  vvMatrix mv;
  vvMatrix pr;

  enum Event
  {
    VV_NEW_PARAM = 0,
    VV_RENDER,
    VV_RESIZE,
    VV_TRANS_FUNC,
    VV_EXIT
  };

  std::queue<Event> events;
  std::queue<vvRenderState::ParameterType> newParams;
};

vvParBrickRend::vvParBrickRend(vvVolDesc* vd, vvRenderState rs,
                               const std::vector<vvParBrickRend::Param>& params,
                               const std::string& type, const vvRendererFactory::Options& options)
  : vvBrickRend(vd, rs, params.size(), type, options)
  , _thread(NULL)
{
  vvDebugMsg::msg(1, "vvParBrickRend::vvParBrickRend()");

  glewInit();

  // if the current render context has no alpha channel, we cannot use it
  int ac;
  glGetIntegerv(GL_ALPHA_BITS, &ac);
  if (ac == 0)
  {
    vvDebugMsg::msg(0, "vvParBrickRend::vvParBrickRend(): main OpenGL context has no alpha channel");
  }

  // determine the number of the thread that reuses the main context
  int reuser = -1;
  std::vector<vvParBrickRend::Param>::const_iterator it;
  size_t i;
  for (it = params.begin(), i = 0; it != params.end() && i < params.size(); ++it, ++i)
  {
    vvContextOptions co;
    co.displayName = (*it).display;
    if ((*it).reuseMainContext && vvRenderContext::matchesCurrent(co) && ac)
    {
      reuser = i;
      break;
    }
  }

  // start out with empty rendering regions
  vvAABBs emptyBox = vvAABBs(vvsize3(), vvsize3());
  setParameter(vvRenderer::VV_VISIBLE_REGION, emptyBox);
  setParameter(vvRenderer::VV_PADDING_REGION, emptyBox);

  pthread_mutex_t* mutex = new pthread_mutex_t;
  pthread_barrier_t* barrier = new pthread_barrier_t;

  size_t numBarriers = reuser == -1 ? params.size() + 1 : params.size();
  int ret = pthread_barrier_init(barrier, NULL, numBarriers);
  if (ret != 0)
  {
    vvDebugMsg::msg(0, "vvParBrickRend::vvParBrickRend(): Cannot create barrier");
    return;
  }
  ret = pthread_mutex_init(mutex, NULL);
  if (ret != 0)
  {
    vvDebugMsg::msg(0, "vvParBrickRend::vvParBrickRend(): Cannot create mutex");
    return;
  }

  const virvo::Viewport vp = vvGLTools::getViewport();
  assert(vp[2] >= 0 && vp[3] >= 0);
  _width = size_t(vp[2]);
  _height = size_t(vp[3]);

  for (size_t i = 0; i < params.size(); ++i)
  {
    Thread* thread = new Thread;
    thread->id = i;

    thread->contextOptions.displayName = params.at(i).display;

    if (params.at(i).sockidx >= 0)
    {
      thread->socket = static_cast<vvTcpSocket*>(vvSocketMap::get(params.at(i).sockidx));
    }
    else
    {
      thread->socket = NULL;
    }

    thread->filename = params.at(i).filename;

    thread->parbrickrend = this;
    thread->texture.pixels = new std::vector<float>(size_t(vp[2]) * size_t(vp[3]) * 4);
    thread->texture.rect = new vvRecti;

    thread->barrier = barrier;
    thread->mutex = mutex;

    thread->aabb = vvAABB(vd->objectCoords(_bspTree->getLeafs()[i]->getAabb().getMin()),
                          vd->objectCoords(_bspTree->getLeafs()[i]->getAabb().getMax()));

    vvGLTools::getModelviewMatrix(&thread->mv);
    vvGLTools::getProjectionMatrix(&thread->pr);

    if ((int)i == reuser)
    {
      _thread = thread;
      _thread->renderer = vvRendererFactory::create(vd, *this, _type.c_str(), _options);
      setVisibleRegion(_thread->renderer, _bspTree->getLeafs()[i]->getAabb());
    }
    else
    {
      pthread_create(&thread->threadHandle, NULL, renderFunc, thread);
    }
    _threads.push_back(thread);
  }

  for (std::vector<Thread*>::const_iterator it = _threads.begin();
       it != _threads.end(); ++it)
  {
    _textures.push_back((*it)->texture);
  }

  _sortLastVisitor = new vvSortLastVisitor;
  _sortLastVisitor->setTextures(_textures);

  pthread_barrier_wait(barrier);
}

vvParBrickRend::~vvParBrickRend()
{
  vvDebugMsg::msg(1, "vvParBrickRend::~vvParBrickRend()");

  if (_thread != NULL)
  {
    delete _thread->renderer;
  }

  for (std::vector<Thread*>::const_iterator it = _threads.begin();
       it != _threads.end(); ++it)
  {
    pthread_mutex_lock((*it)->mutex);
    (*it)->events.push(Thread::VV_EXIT);
    pthread_mutex_unlock((*it)->mutex);

    if ((*it) != _thread && pthread_join((*it)->threadHandle, NULL) != 0)
    {
      vvDebugMsg::msg(0, "vvParBrickRend::~vvParBrickRend(): Error joining thread");
    }
  }

  for (std::vector<Thread*>::const_iterator it = _threads.begin();
       it != _threads.end(); ++it)
  {
    if (it == _threads.begin())
    {
      pthread_barrier_destroy((*it)->barrier);
      pthread_mutex_destroy((*it)->mutex);
      delete (*it)->barrier;
      delete (*it)->mutex;
    }

    delete (*it)->texture.pixels;
    delete (*it)->texture.rect;
    delete *it;
  }

  delete _sortLastVisitor;
}

void vvParBrickRend::renderVolumeGL()
{
  vvDebugMsg::msg(4, "vvParBrickRend::renderVolumeGL()");

  if (!_showBricks)
  {
    const virvo::Viewport vp = vvGLTools::getViewport();
    if (size_t(vp[2]) != _width || size_t(vp[3]) != _height)
    {
      _width = static_cast<size_t>(vp[2]);
      _height = static_cast<size_t>(vp[3]);
      for (std::vector<Thread*>::iterator it = _threads.begin();
           it != _threads.end(); ++it)
      {
        pthread_mutex_lock((*it)->mutex);
        (*it)->events.push(Thread::VV_RESIZE);
        pthread_mutex_unlock((*it)->mutex);
      }
    }

    vvMatrix mv;
    vvGLTools::getModelviewMatrix(&mv);

    vvMatrix pr;
    vvGLTools::getProjectionMatrix(&pr);

    for (std::vector<Thread*>::iterator it = _threads.begin();
         it != _threads.end(); ++it)
    {
      pthread_mutex_lock((*it)->mutex);
      (*it)->mv = mv;
      (*it)->pr = pr;
      (*it)->events.push(Thread::VV_RENDER);
      pthread_mutex_unlock((*it)->mutex);
    }
   
    // TODO: if main thread renders, store color and depth buffer right here
    // TODO: configure whether main thread renders or not
    
    if (_thread != NULL)
    {
      render(_thread);
    }
    else
    {
      std::vector<Thread*>::iterator it = _threads.begin();
      pthread_barrier_wait((*it)->barrier);
      // no rendering
      pthread_barrier_wait((*it)->barrier);
    }

    vvMatrix invMV;
    invMV = mv;
    invMV.invert();

    // find eye position:
    vvVector3 eye;
    getEyePosition(&eye);

    // bsp tree maintains boxes in voxel coordinates
    vvsize3 veye = vd->voxelCoords(eye);

    // TODO: if we want to use this context for rendering,
    // store the framebuffer before rendering and restore
    // it here. buffer clearing is only a quick solution
    glClear(GL_COLOR_BUFFER_BIT);

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);

    _bspTree->setVisitor(_sortLastVisitor);
    _bspTree->traverse(veye);
  }
  else
  {
    _bspTree->setVisitor(NULL);
  }

  vvBrickRend::renderVolumeGL();
}

void vvParBrickRend::setParameter(ParameterType param, const vvParam& newValue)
{
  vvDebugMsg::msg(3, "vvParBrickRend::setParameter()");

  if (_thread != NULL && _thread->renderer != NULL)
  {
    _thread->renderer->setParameter(param, newValue);
  }

  for (std::vector<Thread*>::iterator it = _threads.begin();
       it != _threads.end(); ++it)
  {
    pthread_mutex_lock((*it)->mutex);
    (*it)->events.push(Thread::VV_NEW_PARAM);
    (*it)->newParams.push(param);
    pthread_mutex_unlock((*it)->mutex);
  }

  vvBrickRend::setParameter(param, newValue);
}

vvParam vvParBrickRend::getParameter(ParameterType param) const
{
  vvDebugMsg::msg(3, "vvParBrickRend::getParameter()");

  return vvBrickRend::getParameter(param);
}

void vvParBrickRend::updateTransferFunction()
{
  vvDebugMsg::msg(3, "vvParBrickRend::updateTransferFunction()");

  if (_thread != NULL && _thread->renderer != NULL)
  {
    _thread->renderer->updateTransferFunction();
  }
  for (std::vector<Thread*>::iterator it = _threads.begin();
       it != _threads.end(); ++it)
  {
    pthread_mutex_lock((*it)->mutex);
    (*it)->events.push(Thread::VV_TRANS_FUNC);
    pthread_mutex_unlock((*it)->mutex);
  }
}

void* vvParBrickRend::renderFunc(void* args)
{
  vvDebugMsg::msg(3, "vvParBrickRend::renderFunc()");

  Thread* thread = static_cast<Thread*>(args);

  vvRenderContext* ctx = NULL;

  if (thread->contextOptions.displayName != "")
  {
    ctx = new vvRenderContext(thread->contextOptions);

    if (!ctx->makeCurrent())
    {
      vvDebugMsg::msg(0, "vvParBrickRend::renderFunc(): cannot make render context current in worker thread");
      pthread_exit(NULL);
      return NULL;
    }
  }

  // copy options
  vvRendererFactory::Options options = thread->parbrickrend->_options;

  int s = vvSocketMap::add(thread->socket);
  std::stringstream sockstr;
  sockstr << s;
  options["sockets"] = sockstr.str();

  options["filename"] = thread->filename;

  thread->renderer = vvRendererFactory::create(thread->parbrickrend->vd, *(thread->parbrickrend),
                                               thread->parbrickrend->_type.c_str(), options);
  setVisibleRegion(thread->renderer, thread->parbrickrend->_bspTree->getLeafs().at(thread->id)->getAabb());

  pthread_barrier_wait(thread->barrier);

  while (true)
  {
    vvDebugMsg::msg(3, "vvParBrickRend::renderFunc() - render loop");
    pthread_mutex_lock(thread->mutex);
    bool haveEvent = !thread->events.empty();
    pthread_mutex_unlock(thread->mutex);

    if (haveEvent)
    {
      pthread_mutex_lock(thread->mutex);
      Thread::Event e = thread->events.front();
      pthread_mutex_unlock(thread->mutex);

      switch (e)
      {
      case Thread::VV_EXIT:
        goto cleanup;
      case Thread::VV_NEW_PARAM:
      {
        pthread_mutex_lock(thread->mutex);
        if (!thread->newParams.empty())
        {
          vvRenderState::ParameterType p = thread->newParams.front();
          vvParam param = thread->parbrickrend->getParameter(p);
          if (param.getType() != vvParam::VV_EMPTY)
          {
            thread->renderer->setParameter(p, param);
          }
          thread->newParams.pop();
        }
        pthread_mutex_unlock(thread->mutex);
        break;
      }
      case Thread::VV_RENDER:
        render(thread);
        break;
      case Thread::VV_RESIZE:
        thread->texture.pixels->resize(thread->parbrickrend->_width * thread->parbrickrend->_height * 4);
        if (ctx != NULL)
        {
          ctx->resize(thread->parbrickrend->_width, thread->parbrickrend->_height);
          glViewport(0, 0, thread->parbrickrend->_width, thread->parbrickrend->_height);
        }
        break;
      case Thread::VV_TRANS_FUNC:
        thread->renderer->updateTransferFunction();
        break;
      }

      pthread_mutex_lock(thread->mutex);
      thread->events.pop();
      pthread_mutex_unlock(thread->mutex);
    }
  }

cleanup:
  delete thread->renderer;
  delete ctx;
  pthread_exit(NULL);
  return NULL;
}

void vvParBrickRend::render(Thread* thread)
{
  pthread_barrier_wait(thread->barrier);

  vvGLTools::setModelviewMatrix(thread->mv);
  vvGLTools::setProjectionMatrix(thread->pr);

  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  thread->renderer->renderVolumeGL();
  const virvo::Viewport vp = vvGLTools::getViewport();

  vvRecti bounds = vvGLTools::getBoundingRect(thread->aabb);
  bounds.intersect(vp);

  (*thread->texture.rect)[0] = bounds[0];
  (*thread->texture.rect)[1] = bounds[1];
  (*thread->texture.rect)[2] = bounds[2];
  (*thread->texture.rect)[3] = bounds[3];

  glReadPixels((*thread->texture.rect)[0], (*thread->texture.rect)[1],
               (*thread->texture.rect)[2], (*thread->texture.rect)[2],
               GL_RGBA, GL_FLOAT, &(*thread->texture.pixels)[0]);
  pthread_barrier_wait(thread->barrier);
}
// vim: sw=2:expandtab:softtabstop=2:ts=2:cino=\:0g0t0
