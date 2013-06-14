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

#include "vvrendererfactory.h"

#ifdef HAVE_CONFIG_H
#include "vvconfig.h"
#endif

#include "vvdebugmsg.h"
#include "vvdynlib.h"
#include "vvvoldesc.h"
#include "vvtexrend.h"
#include "vvsoftsw.h"
#include "vvibrclient.h"
#include "vvimageclient.h"
#ifdef HAVE_VOLPACK
#include "vvrendervp.h"
#endif
#include "vvparbrickrend.h"
#include "vvserbrickrend.h"
#include "vvsocketmap.h"
#include "vvtcpsocket.h"

#include "private/vvlog.h"

#include <map>
#include <string>
#include <cstring>
#include <algorithm>
#include <vector>
#include <sstream>

#define EAX 0x0
#define EBX 0x1
#define ECX 0x2
#define EDX 0x3

#ifdef _MSC_VER

#include <intrin.h>

#else // g++/clang

static void __cpuid(int reg[4], int type)
{
  __asm__ __volatile__ (
    "cpuid": "=a" (reg[EAX]), "=b" (reg[EBX]), "=c" (reg[ECX]), "=d" (reg[EDX]) : "a" (type)
  );
}

#endif

namespace {

typedef std::map<std::string, vvRenderer::RendererType> RendererTypeMap;
typedef std::map<std::string, vvTexRend::GeometryType> GeometryTypeMap;
typedef std::map<std::string, vvTexRend::VoxelType> VoxelTypeMap;
typedef std::map<std::string, std::string> RendererAliasMap;

RendererAliasMap rendererAliasMap;
RendererTypeMap rendererTypeMap;
GeometryTypeMap geometryTypeMap;
VoxelTypeMap voxelTypeMap;
std::vector<std::string> rayRendArchs;

void init()
{
  if(!rendererTypeMap.empty())
    return;

  vvDebugMsg::msg(3, "vvRendererFactory::init()");

  // used in vview
  rendererAliasMap["0"] = "default";
  rendererAliasMap["1"] = "slices";
  rendererAliasMap["2"] = "cubic2d";
  rendererAliasMap["3"] = "planar";
  rendererAliasMap["4"] = "spherical";
  rendererAliasMap["5"] = "bricks";
  rendererAliasMap["6"] = "soft";
  rendererAliasMap["8"] = "rayrend";
  rendererAliasMap["9"] = "volpack";
  rendererAliasMap["10"] = "ibr";
  rendererAliasMap["11"] = "image";
  rendererAliasMap["12"] = "softrayrend";
  rendererAliasMap["13"] = "serbrick";
  rendererAliasMap["14"] = "parbrick";
  // used in COVER and Inventor renderer
  rendererAliasMap["tex2d"] = "slices";
  rendererAliasMap["slices2d"] = "slices";
  rendererAliasMap["preint"] = "planar";
  rendererAliasMap["fragprog"] = "planar";
  rendererAliasMap["tex"] = "planar";
  rendererAliasMap["tex3d"] = "planar";
  rendererAliasMap["brick"] = "bricks";

  voxelTypeMap["default"] = vvTexRend::VV_BEST;
  voxelTypeMap["rgba"] = vvTexRend::VV_RGBA;
  voxelTypeMap["arb"] = vvTexRend::VV_FRG_PRG;
  voxelTypeMap["paltex"] = vvTexRend::VV_PAL_TEX;
  voxelTypeMap["sgilut"] = vvTexRend::VV_SGI_LUT;
  voxelTypeMap["shader"] = vvTexRend::VV_PIX_SHD;
  voxelTypeMap["regcomb"] = vvTexRend::VV_TEX_SHD;

  // TexRend
  rendererTypeMap["default"] = vvRenderer::TEXREND;
  geometryTypeMap["default"] = vvTexRend::VV_AUTO;

  rendererTypeMap["slices"] = vvRenderer::TEXREND;
  geometryTypeMap["slices"] = vvTexRend::VV_SLICES;

  rendererTypeMap["cubic2d"] = vvRenderer::TEXREND;
  geometryTypeMap["cubic2d"] = vvTexRend::VV_CUBIC2D;

  rendererTypeMap["planar"] = vvRenderer::TEXREND;
  geometryTypeMap["planar"] = vvTexRend::VV_VIEWPORT;

  rendererTypeMap["spherical"] = vvRenderer::TEXREND;
  geometryTypeMap["spherical"] = vvTexRend::VV_SPHERICAL;

  rendererTypeMap["bricks"] = vvRenderer::TEXREND;
  geometryTypeMap["bricks"] = vvTexRend::VV_BRICKS;

  // other renderers
  rendererTypeMap["generic"] = vvRenderer::GENERIC;
  rendererTypeMap["soft"] = vvRenderer::SOFTSW;
  rendererTypeMap["rayrend"] = vvRenderer::RAYREND;
  rendererTypeMap["volpack"] = vvRenderer::VOLPACK;
  rendererTypeMap["image"] = vvRenderer::REMOTE_IMAGE;
  rendererTypeMap["ibr"] = vvRenderer::REMOTE_IBR;
  rendererTypeMap["serbrick"] = vvRenderer::SERBRICKREND;
  rendererTypeMap["parbrick"] = vvRenderer::PARBRICKREND;

  // ray rend architectures
  rayRendArchs.push_back("cuda");
  rayRendArchs.push_back("fpu");
  rayRendArchs.push_back("sse");
  rayRendArchs.push_back("sse4_1");
}

static bool test_bit(int value, int bit)
{
  return (value & (1 << bit)) != 0;
}


static bool archSupported(std::string const& arch)
{
  if (arch == "fpu")
  {
    return true;
  }

  if (arch == "cuda")
  {
#if VV_HAVE_CUDA
    return true;
#else
    return false;
#endif
  }

  int reg[4];
  __cpuid(reg, 1);

  if (arch == "mmx")
    return test_bit(reg[EDX], 23);
  if (arch == "sse")
    return test_bit(reg[EDX], 25);
  if (arch == "sse2")
    return test_bit(reg[EDX], 26);
  if (arch == "sse3")
    return test_bit(reg[ECX], 0);
  if (arch == "ssse3")
    return test_bit(reg[ECX], 9);
  if (arch == "sse4_1")
    return test_bit(reg[ECX], 19);
  if (arch == "sse4_2")
    return test_bit(reg[ECX], 20);
  if (arch == "avx")
    return test_bit(reg[ECX], 28);

  return false;
}


std::string findRayRendPlugin(std::string const& plugindir, std::string const& arch)
{
  std::stringstream namestr;
  namestr << "librayrend";
  if (arch == "best")
  {
    // TODO: determine *best available*
    namestr << "sse4_1";
  }
  else
  {
    namestr << arch;
  }
  namestr << ".";
#if defined(_WIN32) // TODO: resolve issues with cross compilation etc.
  namestr << "dll";
#elif defined __APPLE__
  namestr << "dylib";
#else
  namestr << "so";
#endif
  std::string name = namestr.str();

  VV_LOG(1) << "Locating plugin " << name << " in " << plugindir;

  bool found = false;
  std::vector<std::string> entrylist = virvo::toolshed::entryList(plugindir);
  for (std::vector<std::string>::const_iterator it = entrylist.begin();
       it != entrylist.end(); ++it)
  {
    if (*it == name)
    {
      found = true;
      break;
    }
  }

  if (found)
  {
    std::stringstream pathstr;
    pathstr << plugindir;
#ifdef _WIN32
    pathstr << "\\";
#else
    pathstr << "/";
#endif
    pathstr << name;
    std::string path = pathstr.str();
    return path;
  }
  else
  {
    return std::string();
  }
}

std::vector<std::string> split(const std::string &s, char delim)
{
  std::vector<std::string> elems;
  std::stringstream ss(s);
  std::string item;
  while(std::getline(ss, item, delim)) {
    elems.push_back(item);
  }
  return elems;
}

struct ParsedOptions
{
  vvRendererFactory::Options options;
  std::string voxeltype;
  std::vector<vvTcpSocket*> sockets;
  std::string arch; // fpu|sse|sse4_1|best (default)
  std::vector<std::string> filenames;
  size_t bricks;
  std::vector<std::string> displays;
  std::string brickrenderer;

  ParsedOptions()
    : voxeltype("default")
    , arch("best")
    , bricks(1)
    , brickrenderer("planar")
  {

  }

  ParsedOptions(std::string str)
    : voxeltype("default")
    , arch("best")
    , bricks(1)
    , brickrenderer("planar")
  {
    std::vector<std::string> optlist = split(str, ',');
    for(std::vector<std::string>::iterator it = optlist.begin();
        it != optlist.end();
        ++it)
    {
      std::vector<std::string> list = split(*it, '=');
      if(list.empty())
        continue;

      std::string &option = list[0];
      std::transform(option.begin(), option.end(), option.begin(), ::tolower);
      switch(list.size())
      {
      case 1:
        singleOption(list[0], "");
        break;
      case 2:
        singleOption(list[0], list[1]);
        break;
      default:
        vvDebugMsg::msg(1, "option value not handled for: ", list[0].c_str());
        break;
      }
    }
  }

  ParsedOptions(const vvRendererFactory::Options &options)
    : options(options)
    , voxeltype("default")
    , arch("best")
    , bricks(1)
    , brickrenderer("planar")
  {
    for(std::map<std::string, std::string>::const_iterator it = options.begin();
        it != options.end();
        ++it)
    {
      singleOption(it->first, it->second);
    }
  }

  bool singleOption(const std::string &opt, const std::string &val)
  {
    if(val.empty())
    {
      voxeltype = val;
    }
    else
    {
      if(opt == "voxeltype")
      {
        voxeltype = val;
      }
      else if(opt == "sockets")
      {
        sockets.clear();
        std::vector<std::string> sockstr = split(val, ',');
        for (std::vector<std::string>::const_iterator it = sockstr.begin();
             it != sockstr.end(); ++it)
        {
          vvTcpSocket* sock = static_cast<vvTcpSocket*>(vvSocketMap::get(atoi((*it).c_str())));
          sockets.push_back(sock);
        }
      }
      else if(opt == "arch")
      {
        arch = val;
      }
      else if(opt == "filename")
      {
        filenames = split(val, ',');
      }
      else if(opt == "bricks")
      {
        bricks = atoi(val.c_str());
      }
      else if (opt == "displays")
      {
        displays = split(val, ',');
      }
      else if (opt == "brickrenderer")
      {
        brickrenderer = val;
      }
      else
      {
        vvDebugMsg::msg(1, "option not handled: ", opt.c_str());
        return false;
      }
    }
    return true;
  }
};

vvRenderer *create(vvVolDesc *vd, const vvRenderState &rs, const char *t, const ParsedOptions &options)
{
  init();

  if(!t || !strcmp(t, "default"))
    t = getenv("VV_RENDERER");
  if(!t)
    t = "default";
  std::string type(t);
  std::transform(type.begin(), type.end(), type.begin(), ::tolower);
  RendererAliasMap::iterator ait = rendererAliasMap.find(type);
  if(ait != rendererAliasMap.end())
    type = ait->second.c_str();
  RendererTypeMap::iterator it = rendererTypeMap.find(type);
  if(it == rendererTypeMap.end())
  {
    type = "default";
    it = rendererTypeMap.find(type);
  }
  assert(it != rendererTypeMap.end());

  vvTcpSocket* sock = NULL;
  std::string filename;
  std::string arch = options.arch;

  if (options.sockets.size() > 0)
  {
    sock = options.sockets[0];
  }

  if (options.filenames.size() > 0)
  {
    filename = options.filenames[0];
  }

  switch(it->second)
  {
  case vvRenderer::SERBRICKREND:
    return new vvSerBrickRend(vd, rs, options.bricks, options.brickrenderer, options.options);
  case vvRenderer::PARBRICKREND:
  {
    std::vector<vvParBrickRend::Param> params;

    size_t numbricks = options.displays.size();
    numbricks = std::max(options.sockets.size(), numbricks);
    numbricks = std::max(options.filenames.size(), numbricks);

    for (size_t i = 0; i < numbricks; ++i)
    {
      vvParBrickRend::Param p;
      if (options.displays.size() > i)
      {
        p.display = options.displays[i];
      }

      if (options.sockets.size() > i)
      {
        p.sockidx = vvSocketMap::getIndex(options.sockets[i]);
      }

      if (options.filenames.size() > i)
      {
        p.filename = options.filenames[i];
      }

      params.push_back(p);
    }
    return new vvParBrickRend(vd, rs, params, options.brickrenderer, options.options);
  }
  case vvRenderer::GENERIC:
    return new vvRenderer(vd, rs);
  case vvRenderer::REMOTE_IMAGE:
    return new vvImageClient(vd, rs, sock, filename.c_str());
  case vvRenderer::REMOTE_IBR:
    return new vvIbrClient(vd, rs, sock, filename.c_str());
  case vvRenderer::SOFTSW:
    return new vvSoftShearWarp(vd, rs);
#ifdef HAVE_VOLPACK
  case vvRenderer::VOLPACK:
    return new vvVolPack(vd, rs);
#endif
  case vvRenderer::RAYREND:
  {
    const char* pluginEnv = "VV_PLUGIN_PATH";
    char* pluginPath = getenv(pluginEnv);
    std::string ppath = pluginPath == NULL ? "." : pluginPath;

    // if VV_PLUGIN_PATH not set, try "."
    std::string path = findRayRendPlugin(ppath, arch);
    if (!path.empty())
    {
      VV_SHLIB_HANDLE handle = vvDynLib::open(path.c_str(), 1 /* RTLD_LAZY */);
      vvRenderer* (*create)(vvVolDesc*, vvRenderState const&);
      *(void**)(&create) = vvDynLib::sym(handle, "createSoftRayRend");
      if (create != NULL)
      {
        return (*create)(vd, rs);
      }
      else
      {
        VV_LOG(0) << vvDynLib::error();
      }
    }
    else
    {
      VV_LOG(0) << "No ray casting plugin loaded for architecture " << arch << std::endl;
    }
  }
  // fall through
  case vvRenderer::TEXREND:
  default:
    {
      vvTexRend::VoxelType vox= vd->getBPV()<3 ? vvTexRend::VV_BEST : vvTexRend::VV_RGBA;

      if(vox == vvTexRend::VV_BEST)
      {
        VoxelTypeMap::iterator vit = voxelTypeMap.find(options.voxeltype);
        if(vit != voxelTypeMap.end())
          vox = vit->second;
      }
      vvTexRend::GeometryType geo = vvTexRend::VV_AUTO;
      GeometryTypeMap::iterator git = geometryTypeMap.find(type);
      if(git != geometryTypeMap.end())
        geo = git->second;
      return new vvTexRend(vd, rs, geo, vox);
    }
    break;
  }
  return NULL; // fix warning
}

} // namespace

vvRenderer *vvRendererFactory::create(vvVolDesc *vd, const vvRenderState &rs, const char *t, const char *o)
{
  vvDebugMsg::msg(3, "vvRendererFactory::create: type=", t);
  vvDebugMsg::msg(3, "vvRendererFactory::create: options=", o);

  if(!o)
    o = "default";
  ParsedOptions options(o);

  return ::create(vd, rs, t, options);
}



vvRenderer *vvRendererFactory::create(vvVolDesc *vd,
    const vvRenderState &rs,
    const char *t,
    const vvRendererFactory::Options &opts)
{
  vvDebugMsg::msg(3, "vvRendererFactory::create: type=", t);

  ParsedOptions options(opts);

  return ::create(vd, rs, t, options);
}

bool vvRendererFactory::hasRenderer(const std::string& name)
{
  init();

  std::string str = name;
  std::transform(str.begin(), str.end(), str.begin(), ::tolower);

  if (str == "rayrend")
  {
    return hasRenderer(vvRenderer::RAYREND);
  }
#ifdef HAVE_OPENGL
  else if (str == "slices")
  {
    return hasRenderer(vvRenderer::TEXREND) && vvTexRend::isSupported(vvTexRend::VV_SLICES);
  }
  else if (str == "cubic2d")
  {
    return hasRenderer(vvRenderer::TEXREND) && vvTexRend::isSupported(vvTexRend::VV_CUBIC2D);
  }
  else if (str == "planar")
  {
    return hasRenderer(vvRenderer::TEXREND) && vvTexRend::isSupported(vvTexRend::VV_VIEWPORT);
  }
  else if (str == "bricks")
  {
    return hasRenderer(vvRenderer::TEXREND) && vvTexRend::isSupported(vvTexRend::VV_BRICKS);
  }
  else if (str == "spherical")
  {
    return hasRenderer(vvRenderer::TEXREND) && vvTexRend::isSupported(vvTexRend::VV_SPHERICAL);
  }
#endif
  else if (str == "serbrick")
  {
    return hasRenderer(vvRenderer::SERBRICKREND);
  }
  else if (str == "parbrick")
  {
    return hasRenderer(vvRenderer::PARBRICKREND);
  }
  else
  {
    return false;
  }
}

bool vvRendererFactory::hasRenderer(vvRenderer::RendererType type)
{
  init();

  switch (type)
  {
  case vvRenderer::RAYREND:
  {
    const char* pluginEnv = "VV_PLUGIN_PATH";
    char* pluginPath = getenv(pluginEnv);
    std::string ppath = pluginPath == NULL ? "." : pluginPath;

    for (std::vector<std::string>::const_iterator it = rayRendArchs.begin();
         it != rayRendArchs.end(); ++it)
    {
      // at least one plugin for an arbitrary architecture available?
      if (!findRayRendPlugin(ppath, *it).empty())
      {
        return true;
      }
    }
    return false;
  }
  case vvRenderer::TEXREND:
#ifdef HAVE_OPENGL
    return true;
#else
    return false;
#endif
  case vvRenderer::VOLPACK:
#ifdef HAVE_VOLPACK
    return true;
#else
    return false;
#endif
  default:
    return true;
  }
}

bool vvRendererFactory::hasRayRenderer(std::string const& arch)
{
  const char* pluginEnv = "VV_PLUGIN_PATH";
  char* pluginPath = getenv(pluginEnv);
  std::string ppath = pluginPath == NULL ? "." : pluginPath;

  if (!archSupported(arch))
  {
    return false;
  }

  // if VV_PLUGIN_PATH not set, try "."
  return !findRayRendPlugin(ppath, arch).empty(); // TODO: e. g. try to open a symbol for validation
}

//============================================================================
// End of File
//============================================================================
// vim: sw=2:expandtab:softtabstop=2:ts=2:cino=\:0g0t0

