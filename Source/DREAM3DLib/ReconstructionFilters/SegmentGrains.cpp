/* ============================================================================
 * Copyright (c) 2011 Michael A. Jackson (BlueQuartz Software)
 * Copyright (c) 2011 Dr. Michael A. Groeber (US Air Force Research Laboratories)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * Neither the name of Michael A. Groeber, Michael A. Jackson, the US Air Force,
 * BlueQuartz Software nor the names of its contributors may be used to endorse
 * or promote products derived from this software without specific prior written
 * permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  This code was written under United States Air Force Contract number
 *                           FA8650-07-D-5800
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "SegmentGrains.h"
#include <map>
#include <assert.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

#include <cstddef>
#include <vector>
#include <string>
#include <iostream>
#include <cmath>
#include <fstream>
#include <list>
#include <algorithm>
#include <numeric>

#include "DREAM3DLib/Common/Constants.h"
#include "DREAM3DLib/Common/DREAM3DMath.h"
#include "DREAM3DLib/Common/OrientationMath.h"
#include "DREAM3DLib/Common/DREAM3DRandom.h"

#include "DREAM3DLib/OrientationOps/CubicOps.h"
#include "DREAM3DLib/OrientationOps/HexagonalOps.h"
#include "DREAM3DLib/OrientationOps/OrthoRhombicOps.h"


#define ERROR_TXT_OUT 1
#define ERROR_TXT_OUT1 1

const static float m_pi = M_PI;

using namespace std;

#define NEW_SHARED_ARRAY(var, type, size)\
  boost::shared_array<type> var##Array(new type[size]);\
  type* var = var##Array.get();

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
SegmentGrains::SegmentGrains() :
    m_ErrorCondition(0)
{
  Seed = MXA::getMilliSeconds();

  m_HexOps = HexagonalOps::New();
  m_OrientationOps.push_back(m_HexOps.get());
  m_CubicOps = CubicOps::New();
  m_OrientationOps.push_back(m_CubicOps.get());
  m_OrthoOps = OrthoRhombicOps::New();
  m_OrientationOps.push_back(m_OrthoOps.get());
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
SegmentGrains::~SegmentGrains()
{
}

void SegmentGrains::execute()
{

	int err = 0;
  DREAM3D_RANDOMNG_NEW()

  form_grains();

  // If there is an error set this to something negative and also set a message
  m_ErrorMessage = "PackGrainsGen2 Completed";
  m_ErrorCondition = 0;
}

void SegmentGrains::form_grains()
{
  DREAM3D_RANDOMNG_NEW()
  int seed = 0;
  int noseeds = 0;
  size_t graincount = 1;
  size_t goodgraincount = 0;
  int neighbor;
  float q1[5];
  float q2[5];
//  float qa[5];
//  float qb[5];
  float w;
  float n1, n2, n3;
  int randpoint = 0;
  int good = 0;
  int col, row, plane;
  size_t size = 0;
  size_t initialVoxelsListSize = 1000;
  size_t initialMergeListSize = 10;
 // int vid;
  std::vector<int> voxelslist(initialVoxelsListSize, -1);
//  std::vector<int>* vlist;
  std::vector<int> mergelist(initialMergeListSize, -1);
  int neighpoints[6];
  neighpoints[0] = -(m_DataContainer->xpoints * m_DataContainer->ypoints);
  neighpoints[1] = -m_DataContainer->xpoints;
  neighpoints[2] = -1;
  neighpoints[3] = 1;
  neighpoints[4] = m_DataContainer->xpoints;
  neighpoints[5] = (m_DataContainer->xpoints * m_DataContainer->ypoints);
  Ebsd::CrystalStructure phase1, phase2;

  // Precalculate some constants
  int totalPMinus1 = m_DataContainer->totalpoints - 1;

  // Burn volume with tight orientation tolerance to simulate simultaneous growth/aglomeration
  while (noseeds == 0)
  {
    seed = -1;
    int counter = 0;
    randpoint = int(float(rg.genrand_res53()) * float(totalPMinus1));
    while (seed == -1 && counter < m_DataContainer->totalpoints)
    {
      if (randpoint > totalPMinus1) randpoint = randpoint - m_DataContainer->totalpoints;
      if (m_DataContainer->grain_indicies[randpoint] == -1 && m_DataContainer->phases[randpoint] > 0) seed = randpoint;

      randpoint++;
      counter++;
    }
    if (seed == -1) noseeds = 1;
    if (seed >= 0)
    {
      size = 0;
      m_DataContainer->grain_indicies[seed] = graincount;
      voxelslist[size] = seed;
      size++;
      for (size_t j = 0; j < size; ++j)
      {
        int currentpoint = voxelslist[j];
        col = currentpoint % m_DataContainer->xpoints;
        row = (currentpoint / m_DataContainer->xpoints) % m_DataContainer->ypoints;
        plane = currentpoint / (m_DataContainer->xpoints * m_DataContainer->ypoints);
        phase1 = m_DataContainer->crystruct[m_DataContainer->phases[currentpoint]];
        for (int i = 0; i < 6; i++)
        {
          q1[0] = 1;
          q1[1] = m_DataContainer->quats[currentpoint * 5 + 1];
          q1[2] = m_DataContainer->quats[currentpoint * 5 + 2];
          q1[3] = m_DataContainer->quats[currentpoint * 5 + 3];
          q1[4] = m_DataContainer->quats[currentpoint * 5 + 4];
          good = 1;
          neighbor = currentpoint + neighpoints[i];
          if (i == 0 && plane == 0) good = 0;
          if (i == 5 && plane == (m_DataContainer->zpoints - 1)) good = 0;
          if (i == 1 && row == 0) good = 0;
          if (i == 4 && row == (m_DataContainer->ypoints - 1)) good = 0;
          if (i == 2 && col == 0) good = 0;
          if (i == 3 && col == (m_DataContainer->xpoints - 1)) good = 0;
          if (good == 1 && m_DataContainer->grain_indicies[neighbor] == -1 && m_DataContainer->phases[neighbor] > 0)
          {
            w = 10000.0;
            q2[0] = 1;
            q2[1] = m_DataContainer->quats[neighbor*5 + 1];
            q2[2] = m_DataContainer->quats[neighbor*5 + 2];
            q2[3] = m_DataContainer->quats[neighbor*5 + 3];
            q2[4] = m_DataContainer->quats[neighbor*5 + 4];
            phase2 = m_DataContainer->crystruct[m_DataContainer->phases[neighbor]];
            if (phase1 == phase2) w = m_OrientationOps[phase1]->getMisoQuat( q1, q2, n1, n2, n3);
            if (w < m_misorientationtolerance)
            {
              m_DataContainer->grain_indicies[neighbor] = graincount;
              voxelslist[size] = neighbor;
              size++;
              if (size >= voxelslist.size()) voxelslist.resize(size + initialVoxelsListSize, -1);
            }
          }
        }
      }
      voxelslist.erase(std::remove(voxelslist.begin(), voxelslist.end(), -1), voxelslist.end());
      if (m_DataContainer->m_Grains[graincount]->voxellist != NULL)
      {
        delete m_DataContainer->m_Grains[graincount]->voxellist;
      }
      m_DataContainer->m_Grains[graincount]->voxellist = new std::vector<int>(voxelslist.size());
      m_DataContainer->m_Grains[graincount]->voxellist->swap(voxelslist);
      m_DataContainer->m_Grains[graincount]->active = 1;
      m_DataContainer->m_Grains[graincount]->phase = m_DataContainer->phases[seed];
      graincount++;
      if (graincount >= m_DataContainer->m_Grains.size())
      {
        size_t oldSize = m_DataContainer->m_Grains.size();
        m_DataContainer->m_Grains.resize(m_DataContainer->m_Grains.size() + 100);
        for (size_t g = oldSize; g < m_DataContainer->m_Grains.size(); ++g)
        {
          m_DataContainer->m_Grains[g] = Grain::New();
        }
      }
      voxelslist.clear();
      voxelslist.resize(initialVoxelsListSize, -1);
    }
  }
}

