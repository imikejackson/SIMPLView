/* ============================================================================
 * Copyright (c) 2012 Michael A. Jackson (BlueQuartz Software)
 * Copyright (c) 2012 Dr. Michael A. Groeber (US Air Force Research Laboratories)
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
#include "ModifiedLambertProjectionArray.h"

#include <list>

#include "MXA/Utilities/StringUtils.h"


#include "H5Support/H5Utilities.h"

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
ModifiedLambertProjectionArray::ModifiedLambertProjectionArray() :
    m_Name("")
{
  m_IsAllocated = true;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
ModifiedLambertProjectionArray::~ModifiedLambertProjectionArray()
{
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void ModifiedLambertProjectionArray::SetName(const std::string &name)
{
  m_Name = name;
}
// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
std::string ModifiedLambertProjectionArray::GetName()
{
  return m_Name;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void ModifiedLambertProjectionArray::takeOwnership()
{
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void ModifiedLambertProjectionArray::releaseOwnership()
{
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void* ModifiedLambertProjectionArray::GetVoidPointer(size_t i)
{
#ifndef NDEBUG
  if(m_ModifiedLambertProjectionArray.size() > 0)
  {
    BOOST_ASSERT(i < m_ModifiedLambertProjectionArray.size());
  }
#endif
  if(i >= this->GetNumberOfTuples())
  {
    return 0x0;
  }
  return (void*)(&(m_ModifiedLambertProjectionArray[i]));
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
size_t ModifiedLambertProjectionArray::GetNumberOfTuples()
{
  return m_ModifiedLambertProjectionArray.size();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
size_t ModifiedLambertProjectionArray::GetSize()
{
  return m_ModifiedLambertProjectionArray.size();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void ModifiedLambertProjectionArray::SetNumberOfComponents(int nc)
{
  if (nc != 1)
  {
    BOOST_ASSERT(false);
  }
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
int ModifiedLambertProjectionArray::GetNumberOfComponents()
{
  return 1;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
size_t ModifiedLambertProjectionArray::GetTypeSize()
{
  return sizeof(ModifiedLambertProjection);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
int ModifiedLambertProjectionArray::EraseTuples(std::vector<size_t> &idxs)
{
  int err = 0;

  // If nothing is to be erased just return
  if(idxs.size() == 0)
  {
    return 0;
  }

  if (idxs.size() >= GetNumberOfTuples() )
  {
    Resize(0);
    return 0;
  }

  // Sanity Check the Indices in the vector to make sure we are not trying to remove any indices that are
  // off the end of the array and return an error code.
  for(std::vector<size_t>::size_type i = 0; i < idxs.size(); ++i)
  {
    if (idxs[i] >= m_ModifiedLambertProjectionArray.size()) { return -100; }
  }


  std::vector<ModifiedLambertProjection::Pointer> replacement(m_ModifiedLambertProjectionArray.size() - idxs.size());
  size_t idxsIndex = 0;
  size_t rIdx = 0;
  for(size_t dIdx = 0; dIdx < m_ModifiedLambertProjectionArray.size(); ++dIdx)
  {
    if (dIdx != idxs[idxsIndex])
    {
      replacement[rIdx] = m_ModifiedLambertProjectionArray[dIdx];
      ++rIdx;
    }
    else
    {
      ++idxsIndex;
      if (idxsIndex == idxs.size() ) { idxsIndex--;}
    }
  }
  m_ModifiedLambertProjectionArray = replacement;
  return err;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
int ModifiedLambertProjectionArray::CopyTuple(size_t currentPos, size_t newPos)
{
  m_ModifiedLambertProjectionArray[newPos] = m_ModifiedLambertProjectionArray[currentPos];
  return 0;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void ModifiedLambertProjectionArray::InitializeTuple(size_t i, double p)
{
  BOOST_ASSERT(false);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void ModifiedLambertProjectionArray::initializeWithZeros()
{

  for(size_t i = 0; i < m_ModifiedLambertProjectionArray.size(); ++i)
  {
    m_ModifiedLambertProjectionArray[i]->initializeSquares(1,1);
  }

}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
int32_t ModifiedLambertProjectionArray::RawResize(size_t size)
{
  m_ModifiedLambertProjectionArray.resize(size);
  return 1;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
int32_t ModifiedLambertProjectionArray::Resize(size_t numTuples)
{
  return RawResize(numTuples);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void ModifiedLambertProjectionArray::printTuple(std::ostream &out, size_t i, char delimiter)
{
  BOOST_ASSERT(false);
}
// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void ModifiedLambertProjectionArray::printComponent(std::ostream &out, size_t i, int j)
{
  BOOST_ASSERT(false);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
int ModifiedLambertProjectionArray::writeH5Data(hid_t parentId)
{
  herr_t err = 0;
  hid_t gid = H5Utilities::createGroup(parentId, DREAM3D::HDF5::Statistics);
  if (gid < 0)
  {
    return -1;
  }
  // We start numbering our phases at 1. Anything in slot 0 is considered "Dummy" or invalid
  for(size_t i = 1; i < m_ModifiedLambertProjectionArray.size(); ++i)
  {
    if (m_ModifiedLambertProjectionArray[i].get() != NULL) {
    std::string indexString = StringUtils::numToString(i);
    hid_t tupleId = H5Utilities::createGroup(gid, indexString);
    err |= m_ModifiedLambertProjectionArray[i]->writeHDF5Data(tupleId);
    err |= H5Utilities::closeHDF5Object(tupleId);
    }
  }
  err |= H5Utilities::closeHDF5Object(gid);
  return err;
}
// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
int ModifiedLambertProjectionArray::readH5Data(hid_t parentId)
{
  int err = 0;
  std::string statsType;
  hid_t gid = H5Utilities::openHDF5Object(parentId, DREAM3D::HDF5::Statistics);
  if(gid < 0)
  {
    return err;
  }

  std::list<std::string> names;
  err = H5Utilities::getGroupObjects(gid, H5Utilities::H5Support_GROUP, names);
  if(err < 0)
  {
    err |= H5Utilities::closeHDF5Object(gid);
    return err;
  }

  for (std::list<std::string>::iterator iter = names.begin(); iter != names.end(); ++iter)
  {
    int index = 0;
    statsType = "";
    StringUtils::stringToNum(index, *iter);
    H5Lite::readStringAttribute(gid, *iter, DREAM3D::HDF5::StatsType, statsType);
    hid_t statId = H5Utilities::openHDF5Object(gid, *iter);
    if(statId < 0)
    {
      continue;
      err |= -1;
    }
    err |= H5Utilities::closeHDF5Object(statId);
  }

  // Do not forget to close the object
  err |= H5Utilities::closeHDF5Object(gid);

  return err;
}