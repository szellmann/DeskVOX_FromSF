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

#ifndef _VVTFWIDGET_H_
#define _VVTFWIDGET_H_

// C++:
#include <stdio.h>
#include <list>
#include <string>

// Virvo:
#include "vvcolor.h"
#include "vvexport.h"
#include "vvinttypes.h"
#include "vvvecmath.h"

/** Specifies a 3D point with an opacity. 
  @see vvTFCustom
*/
class VIRVOEXPORT vvTFPoint
{
  public:
    vvVector3 _pos;
    float _opacity;   ///< opacity at this point in the TF [0..1]
    
    vvTFPoint();
    vvTFPoint(float, float, float=-1.0f, float=-1.0f);

    void setPos(const vvVector3& pos);
    void setOpacity(float opacity);
    vvVector3 pos() const;
    float opacity() const;
};

/** Base class of transfer function widgets.
  @author Jurgen P. Schulze (jschulze@ucsd.edu)
  @see vvTransFunc
*/
class VIRVOEXPORT vvTFWidget
{
  protected:
    static const char* NO_NAME;
    char* _name;                                  ///< widget name (bone, soft tissue, etc)

  public:
    enum WidgetType
    {
      TF_COLOR,
      TF_PYRAMID,
      TF_BELL,
      TF_SKIP,
      TF_CUSTOM,

      TF_CUSTOM_2D,
      TF_MAP,

      TF_UNKNOWN
    };
    static const int MAX_STR_LEN = 65535;

    vvVector3 _pos;                               ///< position of widget's center [volume data space]
    float _opacity;                               ///< maximum opacity [0..1]

    vvTFWidget();
    vvTFWidget(float, float, float);
    vvTFWidget(vvTFWidget*);
    virtual ~vvTFWidget();

    void setOpacity(float opacity);
    float opacity() const;

    virtual void setName(const char*);
    virtual const char* getName();
    void setPos(const vvVector3& pos);
    void setPos(float x, float y, float z);
    vvVector3 pos() const;
    virtual void readName(FILE*);
    void write(FILE*);
    virtual const char* toString() = 0;
    virtual void fromString(const std::string& str) = 0;
    virtual float getOpacity(float, float=-1.0f, float=-1.0f);
    virtual bool getColor(vvColor&, float, float=-1.0f, float=-1.0f);

    static vvTFWidget* produce(WidgetType type);
    static WidgetType getWidgetType(const char* str);
};

/** Transfer function widget shaped like a Gaussian bell.
 */
class VIRVOEXPORT vvTFBell : public vvTFWidget
{
  protected:
    bool _ownColor;                               ///< true = use widget's own color for TF; false=use background color for TF

  public:
    vvColor _col;                                 ///< RGB color
    vvVector3 _size;                              ///< width, height, depth of bell's bounding box [volume data space]

    vvTFBell();
    vvTFBell(vvTFBell*);
    vvTFBell(vvColor, bool, float, float, float, float=0.5f, float=1.0f, float=0.5f, float=1.0f);
    vvTFBell(FILE*);

    void setColor(const vvColor& col);
    void setSize(const vvVector3& size);
    vvColor color() const;
    vvVector3 size() const;

    virtual const char* toString();
    virtual void fromString(const std::string& str);
    virtual bool getColor(vvColor&, float, float=-1.0f, float=-1.0f);
    virtual float getOpacity(float, float=-1.0f, float=-1.0f);
    virtual bool hasOwnColor();
    virtual void setOwnColor(bool);
};

/** Pyramid-shaped transfer function widget:
  the pyramid has four sides and its tip can be flat (frustum).
*/
class VIRVOEXPORT vvTFPyramid : public vvTFWidget
{
  protected:
    bool _ownColor;                               ///< true = use widget's own color for TF; false=use background color for TF

  public:
    vvColor _col;                                 ///< RGB color
    vvVector3 _top;                               ///< width at top [volume data space]
    vvVector3 _bottom;                            ///< width at bottom of pyramid [volume data space]

    vvTFPyramid();
    vvTFPyramid(vvTFPyramid*);
    vvTFPyramid(vvColor, bool, float, float, float, float, float=0.5f, float=1.0f, float=0.0f, float=0.5f, float=1.0f, float=0.0f);
    vvTFPyramid(FILE*);

    void setColor(const vvColor& col);
    void setTop(const vvVector3& top);
    void setBottom(const vvVector3& bottom);
    vvColor color() const;
    vvVector3 top() const;
    vvVector3 bottom() const;

    virtual const char* toString();
    virtual void fromString(const std::string& str);
    virtual bool getColor(vvColor&, float, float=-1.0f, float=-1.0f);
    virtual float getOpacity(float, float=-1.0f, float=-1.0f);
    virtual bool hasOwnColor();
    virtual void setOwnColor(bool);
};

/** Transfer function widget specifying a color point in TF space.
 */
class VIRVOEXPORT vvTFColor : public vvTFWidget
{
  public:
    void setColor(const vvColor& col);
    vvColor color() const;

    vvColor _col;                                 ///< RGB color

    vvTFColor();
    vvTFColor(vvTFColor*);
    vvTFColor(vvColor, float, float=0.0f, float=0.0f);
    vvTFColor(FILE*);
    virtual const char* toString();
    virtual void fromString(const std::string& str);
};

/** Transfer function widget to skip an area of the transfer function when rendering.
 */
class VIRVOEXPORT vvTFSkip : public vvTFWidget
{
  public:
    void setSize(const vvVector3& size);
    vvVector3 size() const;

    vvVector3 _size;         ///< width, height, depth of skipped area [volume data space]

    vvTFSkip();
    vvTFSkip(vvTFSkip*);
    vvTFSkip(float, float, float=0.5f, float=0.0f, float=0.5f, float=0.0f);
    vvTFSkip(FILE*);
    virtual const char* toString();
    virtual void fromString(const std::string& str);
    virtual float getOpacity(float, float=-1.0f, float=-1.0f);
};

/** Transfer function widget to specify a custom transfer function widget with control points. 
  The widget defines a rectangular area in which the user can specify control points between 
  which the opacity function will be computed linearly.
 */
class VIRVOEXPORT vvTFCustom : public vvTFWidget
{
  public:
    vvVector3 _size;               ///< width, height, depth of TF area [volume data space]
    std::list<vvTFPoint*> _points; ///< list of control points; coordinates are relative to widget center
    vvTFPoint* _currentPoint;      ///< currently selected point

    vvTFCustom();
    vvTFCustom(vvTFCustom*);
    vvTFCustom(float, float, float=0.5f, float=0.0f, float=0.5f, float=0.0f);
    vvTFCustom(FILE*);
    virtual ~vvTFCustom();
    virtual const char* toString();
    virtual void fromString(const std::string& str);
    virtual float getOpacity(float, float=-1.0f, float=-1.0f);
    vvTFPoint* addPoint(float, float=-1.0f, float=-1.0f);
    void removeCurrentPoint();
    vvTFPoint* selectPoint(float, float, float, float, float=-1.0f, float=0.0f, float=-1.0f, float=0.0f);
    void setCurrentPoint(float, float, float=-1.0f, float=-1.0f);
    void moveCurrentPoint(float, float, float=0.0f, float=0.0f);
    void sortPoints();
    void setSize(float, float=-1.0f, float=-1.0f);
};


/** 06/2008 L. Dematte'
  Transfer function widget to specify a custom 2D transfer function widget with control points. 
  Points will be used to create a custom "tent" or an "extruded" shape (basically a polyline
  with alpha value as height).
 */
class VIRVOEXPORT vvTFCustom2D : public vvTFWidget
{
protected:
    bool _ownColor;    

public:
    //float _size[3];                ///< width, height, depth of TF area [volume data space]
    std::list<vvTFPoint*> _points; ///< list of control points; coordinates are relative to widget center
    bool _mapDirty;

    vvColor _col;

    float _opacity;
    bool _extrude;
    vvTFPoint* _centralPoint;      ///< central point

    vvTFCustom2D(bool extrude, float opacity, float xCenter, float yCenter);
    vvTFCustom2D(vvTFCustom2D*);
    vvTFCustom2D(FILE*);
    virtual ~vvTFCustom2D();
    virtual const char* toString();
    virtual void fromString(const std::string& str);
    virtual float getOpacity(float, float=-1.0f, float=-1.0f);
    vvTFPoint* addPoint(float opacity, float x, float y);
    void addPoint(vvTFPoint* newPoint);


    virtual bool getColor(vvColor&, float, float=-1.0f, float=-1.0f);
    virtual bool hasOwnColor();
    virtual void setOwnColor(bool);

private:
   vvVector3 _size;               // width, height, depth of TF area [volume data space]
   float* _map;
   int _dim;                      // dimension of the map

   void addMapPoint(int x, int y, float value);
   void drawFreeContour();
   void uniformFillFreeArea();

   void internalFloodFill(float* map, int x, int y, int xDim, int yDim, float oldV, float newV);
   void midPointLine(float* map, int x0, int y0, int x1, int y1, float alpha0, float alpha1);
};

/** 06/2008 L. Dematte'
  Transfer function widget to specify a custom transfer function.
  Values for each point in space are given.
 */
class VIRVOEXPORT vvTFCustomMap : public vvTFWidget
{
  protected:
    bool _ownColor;                // true = use widget's own color for TF; false=use background color for TF

  public:
    vvColor _col;                  // RGB color
    vvVector3 _size;               // width, height, depth of TF area [volume data space]
    float* _map;
    vvVector3i _dim;                 // dimensions of the map [widget data space]

    vvTFCustomMap();
    vvTFCustomMap(float x, float w, float y=0.5f, float h=0.0f, float z=0.5f, float d=0.0f);
    vvTFCustomMap(vvColor, bool, float x, float w, float y=0.5f, float h=0.0f, float z=0.5f, float d=0.0f);
    vvTFCustomMap(vvTFCustomMap*);
    vvTFCustomMap(FILE*);
    virtual ~vvTFCustomMap();
    virtual const char* toString();
    virtual void fromString(const std::string& str);
    virtual float getOpacity(float, float=-1.0f, float=-1.0f);
    void setOpacity(float val, float x, float y=-1.0f, float z=-1.0f);

    virtual bool getColor(vvColor&, float, float=-1.0f, float=-1.0f);
    virtual bool hasOwnColor();
    virtual void setOwnColor(bool);

private:
    int computeIdx(float x, float y, float z);
};

#endif

//============================================================================
// End of File
//============================================================================
// vim: sw=2:expandtab:softtabstop=2:ts=2:cino=\:0g0t0
