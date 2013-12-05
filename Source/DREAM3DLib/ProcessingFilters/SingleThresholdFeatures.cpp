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
#include "SingleThresholdFeatures.h"

#include "DREAM3DLib/Common/Constants.h"
#include "DREAM3DLib/Common/ThresholdFilterHelper.h"


// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
SingleThresholdFeatures::SingleThresholdFeatures():
  AbstractFilter(),
  m_DataContainerName(DREAM3D::Defaults::VolumeDataContainerName),
  m_CellFeatureAttributeMatrixName(DREAM3D::Defaults::CellFeatureAttributeMatrixName),
  m_CellAttributeMatrixName(DREAM3D::Defaults::CellAttributeMatrixName),
  m_SelectedFeatureArrayName(""),
  m_ComparisonOperator(0),
  m_ComparisonValue(0.0),
  m_OutputArrayName(DREAM3D::FeatureData::GoodFeatures),
  m_Output(NULL)
{
  setupFilterParameters();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
SingleThresholdFeatures::~SingleThresholdFeatures()
{
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SingleThresholdFeatures::setupFilterParameters()
{
  FilterParameterVector parameters;
  {
    FilterParameter::Pointer option = FilterParameter::New();
    option->setHumanLabel("Input Feature Array Name");
    option->setPropertyName("SelectedFeatureArrayName");
    option->setWidgetType(FilterParameter::VolumeFeatureArrayNameSelectionWidget);
    option->setValueType("string");
    option->setUnits("");
    parameters.push_back(option);
  }
  {
    ChoiceFilterParameter::Pointer option = ChoiceFilterParameter::New();
    option->setHumanLabel("Comparison Operator");
    option->setPropertyName("ComparisonOperator");
    option->setWidgetType(FilterParameter::ChoiceWidget);
    option->setValueType("unsigned int");
    QVector<QString> choices;
    choices.push_back(DREAM3D::Comparison::Strings::LessThan);
    choices.push_back(DREAM3D::Comparison::Strings::GreaterThan);
    choices.push_back(DREAM3D::Comparison::Strings::Equal);
    option->setChoices(choices);
    parameters.push_back(option);
  }
  {
    FilterParameter::Pointer option = FilterParameter::New();
    option->setHumanLabel("Value");
    option->setPropertyName("ComparisonValue");
    option->setWidgetType(FilterParameter::DoubleWidget);
    option->setValueType("double");
    parameters.push_back(option);
  }
  {
    ChoiceFilterParameter::Pointer parameter = ChoiceFilterParameter::New();
    parameter->setHumanLabel("Output Array Name");
    parameter->setPropertyName("OutputArrayName");
    parameter->setWidgetType(FilterParameter::ChoiceWidget);
    parameter->setValueType("string");
    parameter->setEditable(true);
    QVector<QString> choices;
    choices.push_back(DREAM3D::FeatureData::GoodFeatures);
    parameter->setChoices(choices);
    parameters.push_back(parameter);
  }
  setFilterParameters(parameters);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SingleThresholdFeatures::readFilterParameters(AbstractFilterParametersReader* reader, int index)
{
  reader->openFilterGroup(this, index);
  /* Code to read the values goes between these statements */
  /* FILTER_WIDGETCODEGEN_AUTO_GENERATED_CODE BEGIN*/
  setSelectedFeatureArrayName( reader->readString( "SelectedFeatureArrayName", getSelectedFeatureArrayName() ) );
  setComparisonOperator( reader->readValue("ComparisonOperator", getComparisonOperator()) );
  setComparisonValue( reader->readValue("ComparisonValue", getComparisonValue()) );
  setOutputArrayName( reader->readString( "OutputArrayName", getOutputArrayName() ) );
  /* FILTER_WIDGETCODEGEN_AUTO_GENERATED_CODE END*/
  reader->closeFilterGroup();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
int SingleThresholdFeatures::writeFilterParameters(AbstractFilterParametersWriter* writer, int index)
{
  writer->openFilterGroup(this, index);
  writer->writeValue("SelectedFeatureArrayName", getSelectedFeatureArrayName() );
  writer->writeValue("ComparisonOperator", getComparisonOperator() );
  writer->writeValue("ComparisonValue", getComparisonValue() );
  writer->writeValue("OutputArrayName", getOutputArrayName() );
  writer->closeFilterGroup();
  return ++index; // we want to return the next index that was just written to
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SingleThresholdFeatures::dataCheck()
{
  setErrorCondition(0);

  VolumeDataContainer* m = getDataContainerArray()->getPrereqDataContainer<VolumeDataContainer, AbstractFilter>(this, getDataContainerName(), false);
  if(getErrorCondition() < 0) { return; }
  AttributeMatrix* cellFeatureAttrMat = m->createNonPrereqAttributeMatrix<AbstractFilter>(this, getCellFeatureAttributeMatrixName(), DREAM3D::AttributeMatrixType::CellFeature);
  if(getErrorCondition() < 0) { return; }

  QVector<int> dims(1, 1);
  m_OutputPtr = cellFeatureAttrMat->createNonPrereqArray<DataArray<bool>, AbstractFilter, bool>(this, m_OutputArrayName, true, dims); /* Assigns the shared_ptr<> to an instance variable that is a weak_ptr<> */
  if( NULL != m_OutputPtr.lock().get() ) /* Validate the Weak Pointer wraps a non-NULL pointer to a DataArray<T> object */
  { m_Output = m_OutputPtr.lock()->getPointer(0); } /* Now assign the raw pointer to data from the DataArray<T> object */

  if(m_SelectedFeatureArrayName.isEmpty() == true)
  {
    setErrorCondition(-11000);
    notifyErrorMessage("An array from the Volume DataContainer must be selected.", getErrorCondition());
  }
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SingleThresholdFeatures::preflight()
{

  dataCheck();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void SingleThresholdFeatures::execute()
{
  VolumeDataContainer* m = getDataContainerArray()->getPrereqDataContainer<VolumeDataContainer, AbstractFilter>(this, getDataContainerName(), false);
  if(getErrorCondition() < 0) { return; }
  setErrorCondition(0);
  int64_t totalPoints = m->getAttributeMatrix(getCellAttributeMatrixName())->getNumTuples();
  size_t totalFeatures = m->getAttributeMatrix(getCellFeatureAttributeMatrixName())->getNumTuples();
  dataCheck();
  if (getErrorCondition() < 0)
  {
    return;
  }

  IDataArray::Pointer inputData = m->getAttributeMatrix(getCellFeatureAttributeMatrixName())->getAttributeArray(m_SelectedFeatureArrayName);

  if (NULL == inputData.get())
  {

    QString ss = QObject::tr("Selected array '%1' does not exist in the Voxel Data Container. Was it spelled correctly?").arg(m_SelectedFeatureArrayName);
    setErrorCondition(-11001);
    notifyErrorMessage(ss, getErrorCondition());
    return;
  }

  IDataArray::Pointer goodFeaturesPtr = m->getAttributeMatrix(getCellFeatureAttributeMatrixName())->getAttributeArray(m_OutputArrayName);
  BoolArrayType* goodFeatures = BoolArrayType::SafeObjectDownCast<IDataArray*, BoolArrayType*>(goodFeaturesPtr.get());
  if (NULL == goodFeatures)
  {
    setErrorCondition(-11002);
    notifyErrorMessage("Could not properly cast the output array to a BoolArrayType", getErrorCondition());
    return;
  }


  ThresholdFilterHelper filter(static_cast<DREAM3D::Comparison::Enumeration>(m_ComparisonOperator), m_ComparisonValue, goodFeatures);

  filter.execute(inputData.get(), goodFeaturesPtr.get());


  m->getAttributeMatrix(getCellFeatureAttributeMatrixName())->addAttributeArray(goodFeaturesPtr->GetName(), goodFeaturesPtr);
  notifyStatusMessage("Complete");
}

