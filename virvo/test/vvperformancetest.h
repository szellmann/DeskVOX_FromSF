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

#ifndef _VV_PERFORMANCETEST_H_
#define _VV_PERFORMANCETEST_H_

#include <vector>

#include "../src/vvtexrend.h"
#include "../src/vvvecmath.h"

#define NUM_COL_HEADERS 9

class vvTestResult
{
  public:
    vvTestResult();
    virtual ~vvTestResult();

    void setDiffTimes(const std::vector<float> diffTimes);

    std::vector<float> getDiffTimes() const;
    float getTotalTime() const;
    float getAvgTime() const;
    float getMaxTime() const;
    float getMinTime() const;
    float getVariance() const;

    void calc();
  private:
    std::vector<float> _diffTimes;
    float _totalTime;
    float _avgTime;
    float _maxTime;
    float _minTime;
    float _variance;
};

class vvPerformanceTest
{
  public:
    enum TestAnimation
    {
      VV_ROT_X = 0,
      VV_ROT_Y,
      VV_ROT_Z,
      VV_ROT_RAND
    };

    vvPerformanceTest();
    virtual ~vvPerformanceTest();

    void writeResultFiles();

    void setId(const int id);
    void setIterations(const int iterations);
    void setVerbose(const bool verbose);
    void setQuality(const float quality);
    void setBrickDims(const vvVector3& brickDims);
    void setBrickDimX(const float brickDimX);
    void setBrickDimY(const float brickDimY);
    void setBrickDimZ(const float brickDimZ);
    void setGeomType(const vvTexRend::GeometryType geomType);
    void setVoxelType(const vvTexRend::VoxelType voxelType);
    void setFrames(const int frames);
    void setTestAnimation(const TestAnimation testAnimation);

    int getId() const;
    int getIterations() const;
    bool getVerbose() const;
    float getQuality() const;
    vvVector3 getBrickDims() const;
    vvTexRend::GeometryType getGeomType() const;
    vvTexRend::VoxelType getVoxelType() const;
    int getFrames() const;
    TestAnimation getTestAnimation() const;
    vvTestResult* getTestResult() const;
  private:
    int _id;
    int _iterations;
    bool _verbose;
    float _quality;
    vvVector3 _brickDims;
    vvTexRend::GeometryType _geomType;
    vvTexRend::VoxelType _voxelType;
    int _frames;
    TestAnimation _testAnimation;
    vvTestResult* _testResult;
};

class vvTestSuite
{
  public:
    vvTestSuite(const char* pathToFile);
    virtual ~vvTestSuite();

    bool isInitialized() const;

    std::vector<vvPerformanceTest*> getTests() const;
  private:
    const char* _pathToFile;

    bool _initialized;

    const char* _columnHeaders[NUM_COL_HEADERS];
    int _headerPos[NUM_COL_HEADERS];

    std::vector<vvPerformanceTest*> _tests;

    void init();
    void initColumnHeaders();
    void initHeader(char* str, const int col);
    void initValue(vvPerformanceTest* test, char* str, const int col);
    char* getStripped(const char* item);
    void toUpper(char* str);
    bool isHeader(const char* str);
    void setHeaderPos(const char* header, const int pos);
    int getHeaderPos(const char* header);
    const char* getHeaderName(const int pos);
};

#endif