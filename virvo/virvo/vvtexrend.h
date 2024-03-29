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

#ifndef _VVTEXREND_H_
#define _VVTEXREND_H_

#include <vector>

// Virvo:
#include "vvexport.h"
#include "vvrenderer.h"
#include "vvopengl.h"
#include "vvbrick.h"

class vvOffscreenBuffer;
class vvShaderFactory;
class vvShaderProgram;
class vvVolDesc;

//============================================================================
// Class Definitions
//============================================================================

/** Volume rendering engine using a texture-based algorithm.
  Textures can be drawn as planes or spheres. In planes mode a rendering
  quality can be given (determining the number of texture slices used), and
  the texture normal can be set according to the application's needs.<P>
  The data points are located at the grid as follows:<BR>
  The outermost data points reside at the very edge of the drawn object,
  the other values are evenly distributed inbetween.
  Make sure you define HAVE_CG in your compiler if you want to use Nvidia Cg.
  @author Juergen Schulze (schulze@cs.brown.de)
  @author Martin Aumueller
  @author Stefan Zellmann
  @see vvRenderer
*/

class VIRVOEXPORT vvTexRend : public vvRenderer
{
  friend class vvBrick;
  public:
    std::vector<GLint> _vertArray;
    std::vector<GLsizei> _elemCounts;
    std::vector<GLuint> _vertIndicesAll;
    std::vector<GLuint *> _vertIndices;
    vvOffscreenBuffer** _offscreenBuffers;

    vvsize3 _numBricks;                           ///< number of bricks for each dimension
    enum ErrorType                                /// Error Codes
    {
      OK = 0,                                     ///< no error
      TRAM_ERROR,                                 ///< not enough texture memory
      TEX_SIZE_UNKNOWN,                           ///< size of 3D texture is unknown
      NO3DTEX,                                    ///< 3D textures not supported on this hardware
      UNSUPPORTED                                 ///< general error code
    };
    enum GeometryType                             /// Geometry textures are projected on
    {
      VV_AUTO = 0,                                ///< automatically choose best
      VV_SLICES,                                  ///< render slices parallel to xy axis plane using 2D textures
      VV_CUBIC2D,                                 ///< render slices parallel to all axis planes using 2D textures
      VV_VIEWPORT,                                ///< render planar slices using a 3D texture
      VV_SPHERICAL,                               ///< render spheres originating at viewer using a 3D texture
      VV_BRICKS                                   ///< render volume using bricking
    };
    enum VoxelType                                /// Internal data type used in textures
    {
      VV_BEST = 0,                                ///< choose best
      VV_RGBA,                                    ///< transfer function look-up done in software
      VV_SGI_LUT,                                 ///< SGI color look-up table
      VV_PAL_TEX,                                 ///< OpenGL paletted textures
      VV_TEX_SHD,                                 ///< Nvidia texture shader
      VV_PIX_SHD,                                 ///< Fragment program (Cg or GLSL)
      VV_FRG_PRG                                  ///< ARB fragment program
    };
    enum IsectType
    {
      VERT_SHADER_ONLY = 0,
      GEOM_SHADER_ONLY,
      VERT_GEOM_COMBINED,
      CPU
    };
    enum FeatureType                              /// Rendering features
    {
      VV_MIP                                      ///< maximum intensity projection
    };
    enum SliceOrientation                         /// Slice orientation for planar 3D textures
    {
      VV_VARIABLE = 0,                            ///< choose automatically
      VV_VIEWPLANE,                               ///< parallel to view plane
      VV_CLIPPLANE,                               ///< parallel to clip plane
      VV_VIEWDIR,                                 ///< perpendicular to viewing direction
      VV_OBJECTDIR,                               ///< perpendicular to line eye-object
      VV_ORTHO                                    ///< as in orthographic projection
    };

    static const int NUM_PIXEL_SHADERS;           ///< number of pixel shaders used
  private:
    enum FragmentProgram
    {
      VV_FRAG_PROG_2D = 0,
      VV_FRAG_PROG_3D,
      VV_FRAG_PROG_PREINT,
      VV_FRAG_PROG_MAX                            // has always to be last in list
    };
    float* rgbaTF;                                ///< density to RGBA conversion table, as created by TF [0..1]
    uint8_t* rgbaLUT;                             ///< final RGBA conversion table, as transferred to graphics hardware (includes opacity and gamma correction)
    uint8_t* preintTable;                         ///< lookup table for pre-integrated rendering, as transferred to graphics hardware
    float  lutDistance;                           ///< slice distance for which LUT was computed
    vvsize3   texels;                             ///< width, height and depth of volume, including empty space [texels]
    float texMin[3];                              ///< minimum texture value of object [0..1] (to prevent border interpolation)
    float texMax[3];                              ///< maximum texture value of object [0..1] (to prevent border interpolation)
    size_t   textures;                            ///< number of textures stored in TRAM
    size_t   texelsize;                           ///< number of bytes/voxel transferred to OpenGL (depending on rendering mode)
    GLint internalTexFormat;                      ///< internal texture format (parameter for glTexImage...)
    GLenum texFormat;                             ///< texture format (parameter for glTexImage...)
    GLuint* texNames;                             ///< names of texture slices stored in TRAM
    GLuint pixLUTName;                            ///< name for transfer function texture
    GLuint fragProgName[VV_FRAG_PROG_MAX];        ///< names for fragment programs (for applying transfer function)
    GeometryType geomType;                        ///< rendering geometry actually used
    VoxelType voxelType;                          ///< voxel type actually used
    bool extTex3d;                                ///< true = 3D texturing supported
    bool extNonPower2;                            ///< true = NonPowerOf2 textures supported
    bool extColLUT;                               ///< true = SGI texture color lookup table supported
    bool extPalTex;                               ///< true = OpenGL 1.2 paletted textures supported
    bool extMinMax;                               ///< true = maximum/minimum intensity projections supported
    bool extTexShd;                               ///< true = Nvidia texture shader & texture shader 2 support
    bool extPixShd;                               ///< true = Nvidia pixel shader support (requires GeForce FX)
    bool extBlendEquation;                        ///< true = support for blend equation extension
    bool arbFrgPrg;                               ///< true = ARB fragment program support
    bool arbMltTex;                               ///< true = ARB multitexture support
    bool preIntegration;                          ///< true = try to use pre-integrated rendering (planar 3d textures)
    bool usePreIntegration;                       ///< true = pre-integrated rendering is actually used
    ptrdiff_t minSlice, maxSlice;                 ///< min/maximum slice to render [0..numSlices-1], -1 for no slice constraints
    bool _areEmptyBricksCreated;                  ///< true when brick outlines are created or assigned through constructor
    bool _areBricksCreated;                       ///< true after the first creation of the bricks
    bool _measureRenderTime;                      ///< if time needs not to be measured, a costly call to glFinish can be spared
    std::vector<BrickList> _brickList;            ///< contains all created bricks for all frames
    std::vector<BrickList> _nonemptyList;         ///< contains all non-transparent bricks for all frames
    BrickList _insideList;                        ///< contains all non-empty bricks inside the probe
    BrickList _sortedList;                        ///< contains all bricks inside the probe in a sorted order (back to front)
    bool _useOnlyOneBrick;                        ///< true if whole data fits in texture memory
    vvVector4 _frustum[6];                        ///< current planes of view frustum
    SliceOrientation _sliceOrientation;           ///< slice orientation for planar 3d textures
    IsectType _isectType;
    bool _proxyGeometryOnGpuSupported;            ///< indicate wether proxy geometry computation on gpu would work
    size_t _lastFrame;                            ///< last frame rendered

    vvOffscreenBuffer* _renderTarget;             ///< offscreen buffer used for image downscaling
                                                  ///< or an image creator making a screenshot

    vvShaderFactory* _shaderFactory;              ///< Factory for shader-creation
    vvShaderProgram* _shader;                     ///< shader performing intersection test on gpu

    int _currentShader;                           ///< ID of currently used fragment shader
    int _previousShader;                          ///< ID of previous shader

    vvVector3 _eye;                               ///< the current eye position

    // GL state variables:
    GLboolean glsTexColTable;                     ///< stores GL_TEXTURE_COLOR_TABLE_SGI
    GLboolean glsSharedTexPal;                    ///< stores GL_SHARED_TEXTURE_PALETTE_EXT

    void setVoxelType(VoxelType vt);
    void makeLUTTexture() const;
    ErrorType makeTextures2D(size_t axes);

    void sortBrickList(std::vector<vvBrick*>& list, const vvVector3&, const vvVector3&, const bool);

    ErrorType makeTextures();
    ErrorType makeEmptyBricks();
    ErrorType makeTextureBricks(std::vector<BrickList>& bricks, bool& areBricksCreated);

    void initClassificationStage(GLuint *pixLUTName, GLuint progName[VV_FRAG_PROG_MAX]) const;
    void freeClassificationStage(GLuint pixLUTName, GLuint progName[VV_FRAG_PROG_MAX]) const;
    void enableShader (vvShaderProgram* shader, GLuint lutName);
    void disableShader(vvShaderProgram* shader) const;

    void enableLUTMode(GLuint& lutName, GLuint progName[VV_FRAG_PROG_MAX]);
    void disableLUTMode();

    vvShaderProgram* initShader();
    void setupIntersectionParameters(vvShaderProgram* isectShader);

    ErrorType makeTextures3D();
    void removeTextures();
    ErrorType updateTextures3D(size_t, size_t, size_t, size_t, size_t, size_t, bool);
    ErrorType updateTextures2D(size_t, size_t, size_t, size_t, size_t, size_t, size_t);
    ErrorType updateTextureBricks(size_t, size_t, size_t, size_t, size_t, size_t);
    void setGLenvironment() const;
    void unsetGLenvironment() const;
    void renderTex3DSpherical(const vvMatrix& view);
    void renderTex3DPlanar(const vvMatrix& mv);
    void renderTexBricks(const vvMatrix& mv);
    void renderTex2DSlices(float);
    void renderTex2DCubic(vvVecmath::AxisType, float, float, float);
    VoxelType findBestVoxelType(VoxelType) const;
    GeometryType findBestGeometry(GeometryType, VoxelType) const;
    void updateLUT(float dist);
    size_t getLUTSize(vvsize3& size) const;
    size_t getPreintTableSize() const;
    void enableNVShaders() const;
    void disableNVShaders() const;
    void enableFragProg(GLuint& lutName, GLuint progName[VV_FRAG_PROG_MAX]) const;
    void disableFragProg() const;
    void enableTexture(GLenum target) const;
    void disableTexture(GLenum target) const;
    bool testBrickVisibility(const vvBrick* brick, const vvMatrix& mvpMat) const;
    bool testBrickVisibility(const vvBrick*) const;
    bool intersectsFrustum(const vvVector3 &min, const vvVector3 &max) const;
    bool insideFrustum(const vvVector3 &min, const vvVector3 &max) const;
    void markBricksInFrustum(const vvVector3& probeMin, const vvVector3& probeMax);
    void updateFrustum();
    void getBricksInProbe(std::vector<BrickList>& nonemptyList, BrickList& insideList, BrickList& sortedList,
                          vvVector3, const vvVector3, bool& roiChanged);
    void computeBrickSize();
    void initVertArray(size_t numSlices);
    void validateEmptySpaceLeaping();             ///< only leap empty bricks if tf type is compatible with this
    void evaluateLocalIllumination(vvShaderProgram* pixelShader, const vvVector3& normal);

    int  getCurrentShader() const;
    void setCurrentShader(int);
  public:
    vvTexRend(vvVolDesc*, vvRenderState, GeometryType=VV_AUTO, VoxelType=VV_BEST);
    virtual ~vvTexRend();
    void  renderVolumeGL();
    void  updateTransferFunction();
    void  updateVolumeData();
    void  updateVolumeData(size_t, size_t, size_t, size_t, size_t, size_t);
    void  fillNonemptyList(std::vector<BrickList>& nonemptyList, std::vector<BrickList>& brickList) const;
    void  activateClippingPlane();
    void  deactivateClippingPlane();
    void  setNumLights(int);
    bool  instantClassification() const;
    void  setViewingDirection(const vvVector3& vd);
    void  setObjectDirection(const vvVector3& od);
    virtual void setParameter(ParameterType param, const vvParam& value);
    virtual vvParam getParameter(ParameterType param) const;
    static bool isSupported(GeometryType);
    static bool isSupported(VoxelType);
    bool isSupported(FeatureType) const;
    GeometryType getGeomType() const;
    VoxelType getVoxelType() const;
    void renderQualityDisplay();
    void printLUT() const;
    void updateBrickGeom();
    void setComputeBrickSize(bool);
    void setBrickSize(size_t);
    size_t getBrickSize() const;
    void setTexMemorySize(size_t);
    size_t getTexMemorySize() const;
    uint8_t* getHeightFieldData(float[4][3], size_t&, size_t&);
    float getManhattenDist(float[3], float[3]) const;
    float calcQualityAndScaleImage();
};
#endif

//============================================================================
// End of File
//============================================================================
// vim: sw=2:expandtab:softtabstop=2:ts=2:cino=\:0g0t0
