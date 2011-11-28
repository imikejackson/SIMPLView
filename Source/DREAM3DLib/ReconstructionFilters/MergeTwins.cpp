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

#include "MergeTwins.h"
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
MergeTwins::MergeTwins() :
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
MergeTwins::~MergeTwins()
{
}

void MergeTwins::execute()
{

	int err = 0;
  DREAM3D_RANDOMNG_NEW()

  merge_twins();
  characterize_twins();
  renumber_grains();

  // If there is an error set this to something negative and also set a message
  m_ErrorMessage = "PackGrainsGen2 Completed";
  m_ErrorCondition = 0;
}

void MergeTwins::merge_twins()
{
  float angcur = 180.0f;
  vector<int > twinlist;
  float w;
  float n1, n2, n3;
  float angtol = 2.0f;
  float axistol = 2.0f*M_PI/180.0f;
  float q1[5];
  float q2[5];
  size_t numgrains = m_DataContainer->m_Grains.size();
  Ebsd::CrystalStructure phase1, phase2;

  for (size_t i = 1; i < numgrains; i++)
  {
	if (m_DataContainer->m_Grains[i]->twinnewnumber == -1 && m_DataContainer->m_Grains[i]->phase > 0)
    {
      twinlist.push_back(i);
      for (size_t j = 0; j < twinlist.size(); j++)
      {
        int firstgrain = twinlist[j];
        std::vector<int>* nlist = m_DataContainer->m_Grains[firstgrain]->neighborlist;
        int size = int(nlist->size());
        for (int l = 0; l < size; l++)
        {
          angcur = 180.0f;
          int twin = 0;
          size_t neigh = nlist->at(l);
          if (neigh != i && m_DataContainer->m_Grains[neigh]->twinnewnumber == -1 && m_DataContainer->m_Grains[neigh]->phase > 0)
          {
            w = 10000.0f;
            q1[1] = m_DataContainer->m_Grains[firstgrain]->avg_quat[1]/m_DataContainer->m_Grains[firstgrain]->avg_quat[0];
            q1[2] = m_DataContainer->m_Grains[firstgrain]->avg_quat[2]/m_DataContainer->m_Grains[firstgrain]->avg_quat[0];
            q1[3] = m_DataContainer->m_Grains[firstgrain]->avg_quat[3]/m_DataContainer->m_Grains[firstgrain]->avg_quat[0];
            q1[4] = m_DataContainer->m_Grains[firstgrain]->avg_quat[4]/m_DataContainer->m_Grains[firstgrain]->avg_quat[0];
            phase1 = m_DataContainer->crystruct[m_DataContainer->m_Grains[firstgrain]->phase];
            q2[1] = m_DataContainer->m_Grains[neigh]->avg_quat[1]/m_DataContainer->m_Grains[neigh]->avg_quat[0];
            q2[2] = m_DataContainer->m_Grains[neigh]->avg_quat[2]/m_DataContainer->m_Grains[neigh]->avg_quat[0];
            q2[3] = m_DataContainer->m_Grains[neigh]->avg_quat[3]/m_DataContainer->m_Grains[neigh]->avg_quat[0];
            q2[4] = m_DataContainer->m_Grains[neigh]->avg_quat[4]/m_DataContainer->m_Grains[neigh]->avg_quat[0];
            phase2 = m_DataContainer->crystruct[m_DataContainer->m_Grains[neigh]->phase];
            if (phase1 == phase2 && phase1 > 0) w = m_OrientationOps[phase1]->getMisoQuat( q1, q2, n1, n2, n3);
//			OrientationMath::axisAngletoRod(w, n1, n2, n3, r1, r2, r3);
			float axisdiff111 = acosf(fabs(n1)*0.57735f+fabs(n2)*0.57735f+fabs(n3)*0.57735f);
			float angdiff60 = fabs(w-60.0f);
            if (axisdiff111 < axistol && angdiff60 < angtol) twin = 1;
            if (twin == 1)
            {
              m_DataContainer->m_Grains[neigh]->gottwinmerged = true;
              m_DataContainer->m_Grains[neigh]->twinnewnumber = i;
              twinlist.push_back(neigh);
            }
          }
        }
      }
    }
    twinlist.clear();
  }
  for (int k = 0; k < (m_DataContainer->xpoints * m_DataContainer->ypoints * m_DataContainer->zpoints); k++)
  {
    int grainname = m_DataContainer->grain_indicies[k];
    if (m_DataContainer->m_Grains[grainname]->gottwinmerged == true)
    {
      int twinnewnumber = m_DataContainer->m_Grains[grainname]->twinnewnumber;
      m_DataContainer->grain_indicies[k] = twinnewnumber;
    }
  }
}

void MergeTwins::renumber_grains()
{
  size_t numgrains = m_DataContainer->m_Grains.size();
  int graincount = 1;
  std::vector<int > newnames(numgrains);
  for (size_t i = 1; i < numgrains; i++)
  {
    if (m_DataContainer->m_Grains[i]->gottwinmerged != true)
    {
      newnames[i] = graincount;
      float ea1good = m_DataContainer->m_Grains[i]->euler1;
      float ea2good = m_DataContainer->m_Grains[i]->euler2;
      float ea3good = m_DataContainer->m_Grains[i]->euler3;
      int size = m_DataContainer->m_Grains[i]->numvoxels;
      int numneighbors = m_DataContainer->m_Grains[i]->numneighbors;
      std::vector<int>* nlist = m_DataContainer->m_Grains[i]->neighborlist;
      m_DataContainer->m_Grains[graincount]->numvoxels = size;
      m_DataContainer->m_Grains[graincount]->numneighbors = numneighbors;
      if (m_DataContainer->m_Grains[graincount]->neighborlist == NULL)
      {
        m_DataContainer->m_Grains[graincount]->neighborlist = new std::vector<int>(numneighbors);
      }
      if (NULL != nlist)
      {
        m_DataContainer->m_Grains[graincount]->neighborlist->swap(*nlist);
      }
      m_DataContainer->m_Grains[graincount]->euler1 = ea1good;
      m_DataContainer->m_Grains[graincount]->euler2 = ea2good;
      m_DataContainer->m_Grains[graincount]->euler3 = ea3good;
      graincount++;
    }
  }
#if 0
  tbb::parallel_for(tbb::blocked_range<size_t>(0, totalpoints ), ParallelRenumberGrains( this) );
#else
 for (int j = 0; j < m_DataContainer->totalpoints; j++)
  {
    int grainname = m_DataContainer->grain_indicies[j];
    if (grainname >= 1)
    {
      int newgrainname = newnames[grainname];
      m_DataContainer->grain_indicies[j] = newgrainname;
    }
  }
#endif
}

void MergeTwins::characterize_twins()
{
  size_t numgrains = m_DataContainer->m_Grains.size();
  for (size_t i = 0; i < numgrains; i++)
  {

  }
}

