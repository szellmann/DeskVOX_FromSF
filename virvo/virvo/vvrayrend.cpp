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


#include "vvrayrend.h"


#ifdef HAVE_CUDA


#include "vvrayrend-common.h"

#include <GL/glew.h>

#include "cuda/array.h"
#include "cuda/memory.h"
#include "cuda/symbol.h"
#include "cuda/texture.h"

#include "vvcudaimg.h"
#include "vvcudatools.h"
#include "vvcudautils.h"
#include "vvdebugmsg.h"
#include "vvvoldesc.h"

#include "private/vvgltools.h"
#include "private/vvlog.h"

namespace cu = virvo::cuda;


// in vvrayrend.cu
extern cu::Texture cVolTexture8;
extern cu::Texture cVolTexture16;
extern cu::Texture cTFTexture;

// in vvrayrend.cu
extern cu::Symbol<matrix4x4> cInvViewMatrix;
extern cu::Symbol<matrix4x4> cMvPrMatrix;

// in vvrayrend.cu
extern "C" void CallRayRendKernel(const RayRendKernelParams& params,
                                  uchar4* d_output, const uint width, const uint height,
                                  const float4 backgroundColor,
                                  const uint texwidth, const float dist,
                                  const float3 volPos, const float3 volSizeHalf,
                                  const float3 probePos, const float3 probeSizeHalf,
                                  const float3 Lpos, const float3 V,
                                  float constAtt, float linearAtt, float quadAtt,
                                  const bool clipPlane,
                                  const bool clipSphere,
                                  const bool useSphereAsProbe,
                                  const float3 sphereCenter, const float sphereRadius,
                                  const float3 planeNormal, const float planeDist,
                                  void* d_depth, int dp,
                                  const float2 ibrPlanes,
                                  const IbrMode ibrMode,
                                  bool twoPassIbr);


IbrMode getIbrMode(vvRenderer::IbrMode mode)
{
  switch (mode)
  {
  case vvRenderer::VV_ENTRANCE:
    return VV_ENTRANCE;
  case vvRenderer::VV_EXIT:
    return VV_EXIT;
  case vvRenderer::VV_MIDPOINT:
    return VV_MIDPOINT;
  case vvRenderer::VV_THRESHOLD:
    return VV_THRESHOLD;
  case vvRenderer::VV_PEAK:
    return VV_PEAK;
  case vvRenderer::VV_GRADIENT:
    return VV_GRADIENT;
  case VV_REL_THRESHOLD:
    return VV_REL_THRESHOLD;
  case VV_EN_EX_MEAN:
    return VV_EN_EX_MEAN;
  default:
    return VV_NONE;
  }
}


struct vvRayRend::Impl
{
private:
  cu::Array* volumeArrays;
  size_t numVolumeArrays;
public:
  cu::Array transferFuncArray;
  cu::AutoPointer<void> depth;
  cudaChannelFormatDesc channelDesc;

  Impl()
    : volumeArrays(0)
    , numVolumeArrays(0)
    , transferFuncArray()
    , depth()
    , channelDesc()
  {
  }

  ~Impl()
  {
    delete [] volumeArrays;
  }

  cu::Array& getVolumeArray(size_t index) const
  {
    assert( index < numVolumeArrays );
    return volumeArrays[index];
  }

  void allocateVolumeArrays(size_t size)
  {
    // Create a new array
    cu::Array* newArrays = size == 0 ? 0 : new cu::Array[size];

#if 1
    // Move the old arrays into the new ones
    for (size_t n = 0; n < numVolumeArrays && n < size; ++n)
    {
      newArrays[n].reset(volumeArrays[n].release());
    }
#endif

    // Delete the old arrays
    delete [] volumeArrays;

    // Update members
    volumeArrays = newArrays;
    numVolumeArrays = size;
  }
};


vvRayRend::vvRayRend(vvVolDesc* vd, vvRenderState renderState)
  : vvIbrRenderer(vd, renderState)
  , impl(new Impl)
{
  vvDebugMsg::msg(1, "vvRayRend::vvRayRend()");

  glewInit();

  bool ok;

  // Free "cuda error cache".
  vvCudaTools::checkError(&ok, cudaGetLastError(), "vvRayRend::vvRayRend() - free cuda error cache");

  _volumeCopyToGpuOk = true;

  _twoPassIbr = (_ibrMode == VV_REL_THRESHOLD || _ibrMode == VV_EN_EX_MEAN);

  _rgbaTF = NULL;

  intImg = new vvCudaImg(0, 0);
  allocIbrArrays(0, 0);

  const vvCudaImg::Mode mode = dynamic_cast<vvCudaImg*>(intImg)->getMode();
  if (mode == vvCudaImg::TEXTURE)
  {
    setWarpMode(CUDATEXTURE);
  }

  factorViewMatrix();

  initVolumeTexture();

  updateTransferFunction();
}

vvRayRend::~vvRayRend()
{
  vvDebugMsg::msg(1, "vvRayRend::~vvRayRend()");

  delete[] _rgbaTF;
}

size_t vvRayRend::getLUTSize() const
{
   vvDebugMsg::msg(2, "vvRayRend::getLUTSize()");
   return (vd->getBPV()==2) ? 4096 : 256;
}

void vvRayRend::updateTransferFunction()
{
  vvDebugMsg::msg(3, "vvRayRend::updateTransferFunction()");

  size_t lutEntries = getLUTSize();
  delete[] _rgbaTF;
  _rgbaTF = new float[4 * lutEntries];

  vd->computeTFTexture(lutEntries, 1, 1, _rgbaTF);

  cudaChannelFormatDesc channelDesc = cudaCreateChannelDesc<float4>();

  if (!impl->transferFuncArray.allocate(channelDesc, lutEntries, 1))
  {
  }
  if (!impl->transferFuncArray.upload(0, 0, _rgbaTF, lutEntries * 4 * sizeof(float)))
  {
  }

  cTFTexture.setFilterMode(cudaFilterModeLinear);
  cTFTexture.setNormalized(true);    // access with normalized texture coordinates
  cTFTexture.setAddressMode(cudaAddressModeClamp);   // wrap texture coordinates

  cTFTexture.bind(impl->transferFuncArray, channelDesc);
}

void vvRayRend::compositeVolume(int, int)
{
  vvDebugMsg::msg(3, "vvRayRend::compositeVolume()");

  if(!_volumeCopyToGpuOk)
  {
    std::cerr << "vvRayRend::compositeVolume() aborted because of previous CUDA-Error" << std::endl;
    return;
  }
  vvDebugMsg::msg(1, "vvRayRend::compositeVolume()");

  const virvo::Viewport vp = vvGLTools::getViewport();

  allocIbrArrays(vp[2], vp[3]);
  size_t w = vvToolshed::getTextureSize(vp[2]);
  size_t h = vvToolshed::getTextureSize(vp[3]);
  intImg->setSize(w, h);

  vvCudaImg* cudaImg = dynamic_cast<vvCudaImg*>(intImg);
  if (cudaImg == NULL)
  {
    vvDebugMsg::msg(0, "vvRayRend::compositeVolume() - cannot map CUDA image");
    return;
  }
  cudaImg->map();

  const vvVector3 size(vd->getSize());

  vvVector3 probePosObj;
  vvVector3 probeSizeObj;
  vvVector3 probeMin;
  vvVector3 probeMax;
  calcProbeDims(probePosObj, probeSizeObj, probeMin, probeMax);

  vvVector3 clippedProbeSizeObj = probeSizeObj;
  for (int i=0; i<3; ++i)
  {
    if (clippedProbeSizeObj[i] < vd->getSize()[i])
    {
      clippedProbeSizeObj[i] = vd->getSize()[i];
    }
  }

  if (_isROIUsed && !_sphericalROI)
  {
    drawBoundingBox(probeSizeObj, _roiPos, _probeColor);
  }

  const float diagonalVoxels = sqrtf(float(vd->vox[0] * vd->vox[0] +
                                           vd->vox[1] * vd->vox[1] +
                                           vd->vox[2] * vd->vox[2]));
  size_t numSlices = std::max(size_t(1), static_cast<size_t>(_quality * diagonalVoxels));

  vvMatrix Mv, MvPr;
  vvGLTools::getModelviewMatrix(&Mv);
  vvGLTools::getProjectionMatrix(&MvPr);
  MvPr.multiplyRight(Mv);

  cMvPrMatrix.update(MvPr.data(), 16 * sizeof(float));

  vvMatrix invMv;
  invMv = vvMatrix(Mv);
  invMv.invert();

  vvMatrix pr;
  vvGLTools::getProjectionMatrix(&pr);

  vvMatrix invMvpr;
  vvGLTools::getModelviewMatrix(&invMvpr);
  invMvpr.multiplyLeft(pr);
  invMvpr.invert();

  cInvViewMatrix.update(invMvpr.data(), 16 * sizeof(float));

  const float3 volPos = make_float3(vd->pos[0], vd->pos[1], vd->pos[2]);
  float3 probePos = volPos;
  if (_isROIUsed && !_sphericalROI)
  {
    probePos = make_float3(probePosObj[0],  probePosObj[1], probePosObj[2]);
  }
  vvVector3 sz = vd->getSize();
  const float3 volSize = make_float3(sz[0], sz[1], sz[2]);
  float3 probeSize = make_float3(probeSizeObj[0], probeSizeObj[1], probeSizeObj[2]);
  if (_sphericalROI)
  {
    probeSize = make_float3((float)vd->vox[0], (float)vd->vox[1], (float)vd->vox[2]);
  }

  const bool isOrtho = pr.isProjOrtho();

  vvVector3 eye;
  getEyePosition(&eye);

  vvVector3 origin;

  // use GL_LIGHT0 for local lighting
  GLfloat lv[4];
  float constAtt = 1.0f;
  float linearAtt = 0.0f;
  float quadAtt = 0.0f;
  if (glIsEnabled(GL_LIGHTING))
  {
    glGetLightfv(GL_LIGHT0, GL_POSITION, &lv[0]);
    glGetLightfv(GL_LIGHT0, GL_CONSTANT_ATTENUATION, &constAtt);
    glGetLightfv(GL_LIGHT0, GL_LINEAR_ATTENUATION, &linearAtt);
    glGetLightfv(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, &quadAtt);
  }
  // position of light source. transform eye coordinates to object coordinates
  vvVector3 LposEye = vvVector3(lv[0], lv[1], lv[2]);
  LposEye.multiply(invMv);
  const float3 Lpos = make_float3(LposEye[0], LposEye[1], LposEye[2]);

  vvVector3 normal;
  getShadingNormal(normal, origin, eye, invMv, isOrtho);

  // viewing direction equals normal direction
  const float3 V = make_float3(normal[0], normal[1], normal[2]);

  // Clip sphere.
  const float3 center = make_float3(_clipSphereCenter[0], _clipSphereCenter[1], _clipSphereCenter[2]);
  const float radius  = _clipSphereRadius;

  // Clip plane.
  const float3 pnormal = normalize(make_float3(_clipPlaneNormal[0], _clipPlaneNormal[1], _clipPlaneNormal[2]));
  const float pdist = (-_clipPlaneNormal).dot(_clipPlanePoint);

  if (_clipMode == 1 && _clipPlanePerimeter)
  {
    drawPlanePerimeter(size, vd->pos, _clipPlanePoint, _clipPlaneNormal, _clipPlaneColor);
  }

  GLfloat bgcolor[4];
  glGetFloatv(GL_COLOR_CLEAR_VALUE, bgcolor);
  float4 backgroundColor = make_float4(bgcolor[0], bgcolor[1], bgcolor[2], bgcolor[3]);

  if (vd->bpc == 1)
  {
    cVolTexture8.bind(impl->getVolumeArray(vd->getCurrentFrame()), impl->channelDesc);
  }
  else if (vd->bpc == 2)
  {
    cVolTexture16.bind(impl->getVolumeArray(vd->getCurrentFrame()), impl->channelDesc);
  }

  RayRendKernelParams kernelParams;

  kernelParams.blockDimX            = 16;
  kernelParams.blockDimY            = 16;
  kernelParams.bpc                  = getVolDesc()->bpc;
  kernelParams.illumination         = _lighting;
  kernelParams.opacityCorrection    = _opacityCorrection;
  kernelParams.earlyRayTermination  = _earlyRayTermination;
  kernelParams.clipMode             = getParameter(vvRenderState::VV_CLIP_MODE);
  kernelParams.mipMode              = getParameter(vvRenderState::VV_MIP_MODE);
  kernelParams.useIbr               = getParameter(vvRenderState::VV_USE_IBR);

  CallRayRendKernel(kernelParams,
                    cudaImg->getDeviceImg(), vp[2], vp[3],
                    backgroundColor, intImg->width,diagonalVoxels / (float)numSlices,
                    volPos, volSize * 0.5f,
                    probePos, probeSize * 0.5f,
                    Lpos, V,
                    constAtt, linearAtt, quadAtt,
                    kernelParams.clipMode == 1, kernelParams.clipMode == 2, false,
                    center, radius * radius,
                    pnormal, pdist, impl->depth.get(), _depthPrecision,
                    make_float2(_depthRange[0], _depthRange[1]),
                    getIbrMode(_ibrMode), _twoPassIbr);

  cudaImg->unmap();

  // For bounding box, tf palette display, etc.
  vvRenderer::renderVolumeGL();
}

void vvRayRend::getColorBuffer(uchar** colors) const
{
  cudaMemcpy(*colors, static_cast<vvCudaImg*>(intImg)->getDeviceImg(), intImg->width*intImg->height*4, cudaMemcpyDeviceToHost);
}

void vvRayRend::getDepthBuffer(uchar** depths) const
{
  cudaMemcpy(*depths, impl->depth.get(), intImg->width*intImg->height*_depthPrecision/8, cudaMemcpyDeviceToHost);
}

//----------------------------------------------------------------------------
// see parent
void vvRayRend::setParameter(ParameterType param, const vvParam& newValue)
{
  vvDebugMsg::msg(3, "vvRayRend::setParameter()");

  switch (param)
  {
  case vvRenderer::VV_SLICEINT:
    {
      if (_interpolation != newValue.asBool())
      {
        _interpolation = newValue;
        initVolumeTexture();
        updateTransferFunction();
      }
    }
    break;
  default:
    vvIbrRenderer::setParameter(param, newValue);
    break;
  }
}

//----------------------------------------------------------------------------
// see parent
vvParam vvRayRend::getParameter(ParameterType param) const
{
  vvDebugMsg::msg(3, "vvRayRend::getParameter()");

  switch (param)
  {
  case vvRenderer::VV_SLICEINT:
    return _interpolation;
  default:
    return vvIbrRenderer::getParameter(param);
  }
}

void vvRayRend::initVolumeTexture()
{
  vvDebugMsg::msg(3, "vvRayRend::initVolumeTexture()");

  bool ok;

  cudaExtent volumeSize = make_cudaExtent(vd->vox[0], vd->vox[1], vd->vox[2]);
  if (vd->bpc == 1)
  {
    impl->channelDesc = cudaCreateChannelDesc<uchar>();
  }
  else if (vd->bpc == 2)
  {
    impl->channelDesc = cudaCreateChannelDesc<ushort>();
  }

  impl->allocateVolumeArrays(vd->frames);

  bool outOfMem = false;
  size_t outOfMemFrame = 0;
  for (size_t f=0; f<vd->frames; ++f)
  {
    _volumeCopyToGpuOk = impl->getVolumeArray(f).allocate3D(impl->channelDesc, volumeSize);

    size_t availableMem;
    size_t totalMem;
    vvCudaTools::checkError(&ok, cudaMemGetInfo(&availableMem, &totalMem),
                       "vvRayRend::initVolumeTexture() - get mem info from device");

    if(!_volumeCopyToGpuOk)
    {
      outOfMem = true;
      outOfMemFrame = f;
      break;
    }

    VV_LOG(1) << "Total CUDA memory (MB):     " << (size_t)(totalMem/1024/1024) << std::endl;
    VV_LOG(1) << "Available CUDA memory (MB): " << (size_t)(availableMem/1024/1024) << std::endl;

    cudaMemcpy3DParms copyParams = { 0 };

    const size_t size = vd->getFrameBytes();
    if (vd->bpc == 1)
    {
      copyParams.srcPtr = make_cudaPitchedPtr(vd->getRaw(f), volumeSize.width*vd->bpc, volumeSize.width, volumeSize.height);
    }
    else if (vd->bpc == 2)
    {
      uchar* raw = vd->getRaw(f);
      uchar* data = new uchar[size];

      for (size_t i=0; i<size; i+=2)
      {
        int val = ((int) raw[i] << 8) | (int) raw[i + 1];
        val >>= 4;
        data[i] = raw[i];
        data[i + 1] = val;
      }
      copyParams.srcPtr = make_cudaPitchedPtr(data, volumeSize.width*vd->bpc, volumeSize.width, volumeSize.height);
    }
    copyParams.dstArray = impl->getVolumeArray(f).get();
    copyParams.extent = volumeSize;
    copyParams.kind = cudaMemcpyHostToDevice;
    vvCudaTools::checkError(&ok, cudaMemcpy3D(&copyParams),
                       "vvRayRend::initVolumeTexture() - copy volume frame to 3D array");
  }

  if (outOfMem)
  {
    std::cerr << "Couldn't accomodate the volume" << std::endl;
    for (size_t f=0; f<=outOfMemFrame; ++f)
    {
      impl->getVolumeArray(f).reset();
    }
  }

  if (vd->bpc == 1)
  {
    //
    // XXX:
    // why do we do this right here?
    //
    for (size_t f=0; f<outOfMemFrame; ++f)
    {
      impl->getVolumeArray(f).reset();
    }
  }

  if (_volumeCopyToGpuOk)
  {
    if (vd->bpc == 1)
    {
        cVolTexture8.setNormalized(true);
        cVolTexture8.setFilterMode(_interpolation ? cudaFilterModeLinear : cudaFilterModePoint);
        cVolTexture8.setAddressMode(cudaAddressModeClamp);

        ok = cVolTexture8.bind(impl->getVolumeArray(0), impl->channelDesc);
    }
    else if (vd->bpc == 2)
    {
        cVolTexture16.setNormalized(true);
        cVolTexture16.setFilterMode(_interpolation ? cudaFilterModeLinear : cudaFilterModePoint);
        cVolTexture16.setAddressMode(cudaAddressModeClamp);

        ok = cVolTexture16.bind(impl->getVolumeArray(0), impl->channelDesc);
    }
  }
}

void vvRayRend::factorViewMatrix()
{
  vvDebugMsg::msg(3, "vvRayRend::factorViewMatrix()");

  virvo::Viewport vp = vvGLTools::getViewport();
  size_t w = vvToolshed::getTextureSize(vp[2]);
  size_t h = vvToolshed::getTextureSize(vp[3]);

  if (intImg->width != static_cast<int>(w) || intImg->height != static_cast<int>(h))
  {
    intImg->setSize(w, h);
    allocIbrArrays(w, h);
  }

  iwWarp.identity();
  iwWarp.translate(-1.0f, -1.0f, 0.0f);
  iwWarp.scaleLocal(1.0f / (static_cast<float>(vp[2]) * 0.5f), 1.0f / (static_cast<float>(vp[3]) * 0.5f), 0.0f);
}

void vvRayRend::findAxisRepresentations()
{
  // Overwrite default behavior.
}

bool vvRayRend::allocIbrArrays(size_t w, size_t h)
{
  vvDebugMsg::msg(3, "vvRayRend::allocIbrArrays()");

  size_t size = w * h * _depthPrecision/8;

  // deallocate the current and allocate new buffer
  if (!impl->depth.allocate(size))
    return false;

  cudaMemset(impl->depth.get(), 0, size);

  return true;
}

vvRenderer* createSoftRayRend(vvVolDesc* vd, vvRenderState const& rs)
{
  return new vvRayRend(vd, rs);
}

#endif // HAVE_CUDA
