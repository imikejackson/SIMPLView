/*
 * Your License or Copyright can go here
 */

#include "FindGBCD_MetricBased.h"

#include "SIMPLib/Common/Constants.h"
#include "SIMPLib/FilterParameters/AbstractFilterParametersReader.h"
#include "SIMPLib/FilterParameters/AbstractFilterParametersWriter.h"
#include "SIMPLib/FilterParameters/IntFilterParameter.h"
#include "SIMPLib/FilterParameters/AxisAngleFilterParameter.h"
#include "SIMPLib/FilterParameters/ChoiceFilterParameter.h"
#include "SIMPLib/FilterParameters/IntFilterParameter.h"
#include "SIMPLib/FilterParameters/OutputFileFilterParameter.h"
#include "SIMPLib/FilterParameters/BooleanFilterParameter.h"
#include "SIMPLib/FilterParameters/OutputFileFilterParameter.h"
#include "SIMPLib/FilterParameters/DataArraySelectionFilterParameter.h"
#include "SIMPLib/FilterParameters/DataArraySelectionFilterParameter.h"
#include "SIMPLib/FilterParameters/BooleanFilterParameter.h"
#include "SIMPLib/FilterParameters/DataArraySelectionFilterParameter.h"
#include "SIMPLib/FilterParameters/DataArraySelectionFilterParameter.h"
#include "SIMPLib/FilterParameters/DataArraySelectionFilterParameter.h"
#include "SIMPLib/FilterParameters/DataArraySelectionFilterParameter.h"
#include "SIMPLib/FilterParameters/DataArraySelectionFilterParameter.h"
#include "SIMPLib/FilterParameters/DataArraySelectionFilterParameter.h"
#include "SurfaceMeshing/SurfaceMeshingConstants.h"

// Include the MOC generated file for this class
#include "moc_FindGBCD_MetricBased.cpp"

#include "SIMPLib/FilterParameters/SeparatorFilterParameter.h"
#include <QtCore/QDir>
#include "OrientationLib/SpaceGroupOps/SpaceGroupOps.h"
#include <cmath>
#include "OrientationLib/OrientationMath/OrientationTransforms.hpp"

#ifdef SIMPLib_USE_PARALLEL_ALGORITHMS
#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>
#include <tbb/partitioner.h>
#include <tbb/task_scheduler_init.h>
#include "tbb/concurrent_vector.h"
#endif


class TriAreaAndNormals {

public:
  double area;
  float normal_grain1_x;
  float normal_grain1_y;
  float normal_grain1_z;
  float normal_grain2_x;
  float normal_grain2_y;
  float normal_grain2_z;

  TriAreaAndNormals(double __area, float n1x, float n1y, float n1z, float n2x, float n2y, float n2z) :
    area(__area),
    normal_grain1_x(n1x),
    normal_grain1_y(n1y),
    normal_grain1_z(n1z),
    normal_grain2_x(n2x),
    normal_grain2_y(n2y),
    normal_grain2_z(n2z)
  {
  }

  TriAreaAndNormals() {
    TriAreaAndNormals(0.0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
  }
};



class TrisSelector {

#ifdef SIMPLib_USE_PARALLEL_ALGORITHMS
  tbb::concurrent_vector<TriAreaAndNormals>* selectedTris;
#else
  QVector<TriAreaAndNormals>* selectedTris;
#endif

  QVector<float> samplPtsX;
  QVector<float> samplPtsY;
  QVector<float> samplPtsZ;

  float m_misorResol; 
  int32_t m_PhaseOfInterest;
  float (&gFixedT)[3][3];

  QVector<SpaceGroupOps::Pointer> m_OrientationOps;
  uint32_t cryst;
  int32_t nsym;

  uint32_t* m_CrystalStructures;
  float* m_Eulers;
  int32_t* m_Phases;
  int32_t* m_FaceLabels;
  double* m_FaceNormals;
  double* m_FaceAreas;
  int32_t* m_FeatureFaceLabels;

  double &totalFaceArea;

public: 
  TrisSelector(

#ifdef SIMPLib_USE_PARALLEL_ALGORITHMS
    tbb::concurrent_vector<TriAreaAndNormals>* __selectedTris,
#else
    QVector<TriAreaAndNormals>* __selectedTris,
#endif
    QVector<float> __samplPtsX,
    QVector<float> __samplPtsY,
    QVector<float> __samplPtsZ,
    float __m_misorResol,
    int32_t __m_PhaseOfInterest,
    float(&__gFixedT)[3][3],
    uint32_t* __m_CrystalStructures,
    float* __m_Eulers,
    int32_t* __m_Phases,
    int32_t* __m_FaceLabels,
    double* __m_FaceNormals,
    double* __m_FaceAreas,
    int32_t* __m_FeatureFaceLabels,
    double &__totalFaceArea
  ) :
    selectedTris(__selectedTris),
    samplPtsX(__samplPtsX),
    samplPtsY(__samplPtsY),
    samplPtsZ(__samplPtsZ),
    m_misorResol(__m_misorResol),
    m_PhaseOfInterest(__m_PhaseOfInterest),
    gFixedT(__gFixedT),
    m_CrystalStructures(__m_CrystalStructures),
    m_Eulers(__m_Eulers),
    m_Phases(__m_Phases),
    m_FaceLabels(__m_FaceLabels),
    m_FaceNormals(__m_FaceNormals),
    m_FaceAreas(__m_FaceAreas),
    m_FeatureFaceLabels(__m_FeatureFaceLabels),
    totalFaceArea(__totalFaceArea)
  {  
    m_OrientationOps = SpaceGroupOps::getOrientationOpsQVector();
    cryst = __m_CrystalStructures[__m_PhaseOfInterest]; 
    nsym = m_OrientationOps[cryst]->getNumSymOps();
  }

  virtual ~TrisSelector()
  {    
  }

  void select(size_t start, size_t end) const
  {
    float g1ea[3] = { 0.0f, 0.0f, 0.0f };
    float g2ea[3] = { 0.0f, 0.0f, 0.0f };

    float g1[3][3] = { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };
    float g2[3][3] = { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };

    float g1s[3][3] = { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };
    float g2s[3][3] = { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };

    float sym1[3][3] = { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };
    float sym2[3][3] = { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };

    float g2sT[3][3] = { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };

    float dg[3][3] = { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };
    float dgT[3][3] = { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };

    float diffFromFixed[3][3] = { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };

    float normal_lab[3] = { 0.0f, 0.0f, 0.0f };
    float normal_grain1[3] = { 0.0f, 0.0f, 0.0f };
    float normal_grain2[3] = { 0.0f, 0.0f, 0.0f };

    for (size_t triIdx = start; triIdx < end; triIdx++) {

      int32_t feature1 = m_FaceLabels[2 * triIdx];
      int32_t feature2 = m_FaceLabels[2 * triIdx + 1];

      if (feature1 < 1 || feature2 < 1) { continue; }
      if (m_Phases[feature1] != m_Phases[feature2])  { continue; }
      if (m_Phases[feature1] != m_PhaseOfInterest || m_Phases[feature2] != m_PhaseOfInterest) { continue; }

      totalFaceArea += m_FaceAreas[triIdx];

      normal_lab[0] = m_FaceNormals[3 * triIdx];
      normal_lab[1] = m_FaceNormals[3 * triIdx + 1];
      normal_lab[2] = m_FaceNormals[3 * triIdx + 2];

      for (int whichEa = 0; whichEa < 3; whichEa++)
      {
        g1ea[whichEa] = m_Eulers[3 * feature1 + whichEa];
        g2ea[whichEa] = m_Eulers[3 * feature2 + whichEa];
      }

      FOrientArrayType om(9, 0.0f);
      FOrientTransformsType::eu2om(FOrientArrayType(g1ea, 3), om);
      om.toGMatrix(g1);
      FOrientTransformsType::eu2om(FOrientArrayType(g2ea, 3), om);
      om.toGMatrix(g2);


      for (int j = 0; j < nsym; j++)
      {
        // rotate g1 by symOp
        m_OrientationOps[cryst]->getMatSymOp(j, sym1);
        MatrixMath::Multiply3x3with3x3(sym1, g1, g1s);
        // get the crystal directions along the triangle normals
        MatrixMath::Multiply3x3with3x1(g1s, normal_lab, normal_grain1);

        for (int k = 0; k < nsym; k++)
        {
          // calculate the symmetric misorienation
          m_OrientationOps[cryst]->getMatSymOp(k, sym2);
          // rotate g2 by symOp
          MatrixMath::Multiply3x3with3x3(sym2, g2, g2s);
          // transpose rotated g2
          MatrixMath::Transpose3x3(g2s, g2sT);
          // calculate delta g
          MatrixMath::Multiply3x3with3x3(g1s, g2sT, dg); //dg -- the misorientation between adjacent grains
          MatrixMath::Transpose3x3(dg, dgT);

          for (int transpose = 0; transpose <= 1; transpose++)
          {
            // check if dg is close to gFix
            if (transpose == 0)
            {
              MatrixMath::Multiply3x3with3x3(dg, gFixedT, diffFromFixed);
            }
            else
            {
              MatrixMath::Multiply3x3with3x3(dgT, gFixedT, diffFromFixed);
            }

            float diffAngle = acosf((diffFromFixed[0][0] + diffFromFixed[1][1] + diffFromFixed[2][2] - 1.0f) * 0.5f);

            if (diffAngle < m_misorResol)
            {
              MatrixMath::Multiply3x3with3x1(dgT, normal_grain1, normal_grain2); // minus sign before normal_grain2 will be added later

              if (transpose == 0)
              {
                (*selectedTris).push_back(TriAreaAndNormals(
                  m_FaceAreas[triIdx],
                  normal_grain1[0],
                  normal_grain1[1],
                  normal_grain1[2],
                  -normal_grain2[0],
                  -normal_grain2[1],
                  -normal_grain2[2]
                  ));
              }
              else
              {
                (*selectedTris).push_back(TriAreaAndNormals(
                  m_FaceAreas[triIdx],
                  -normal_grain2[0],
                  -normal_grain2[1],
                  -normal_grain2[2],
                  normal_grain1[0],
                  normal_grain1[1],
                  normal_grain1[2]
                  ));
              }
            }
          }
        }
      }
    }
  }

#ifdef SIMPLib_USE_PARALLEL_ALGORITHMS
  void operator()(const tbb::blocked_range<size_t>& r) const
  {
    select(r.begin(), r.end());
  }
#endif
};


class ProbeDistrib
{
	QVector<double>* distribValues;
	QVector<double>* errorValues;
	QVector<float> samplPtsX;
	QVector<float> samplPtsY;
	QVector<float> samplPtsZ;
#ifdef SIMPLib_USE_PARALLEL_ALGORITHMS
  tbb::concurrent_vector<TriAreaAndNormals> selectedTris;
#else
  QVector<TriAreaAndNormals> selectedTris;
#endif
	float planeResolSq;
	double totalFaceArea;
	int numDistinctGBs;
	double ballVolume;
	float (&gFixedT)[3][3];

public:
	ProbeDistrib(
		QVector<double>* __distribValues,
		QVector<double>* __errorValues,
		QVector<float> __samplPtsX,
		QVector<float> __samplPtsY,
		QVector<float> __samplPtsZ,
#ifdef SIMPLib_USE_PARALLEL_ALGORITHMS
    tbb::concurrent_vector<TriAreaAndNormals> __selectedTris,
#else
    QVector<TriAreaAndNormals> __selectedTris,
#endif
		float __planeResolSq,
		double __totalFaceArea,
		int __numDistinctGBs,
		double __ballVolume,
		float (&__gFixedT)[3][3]
	) :
		distribValues(__distribValues),
		errorValues(__errorValues),
		samplPtsX(__samplPtsX),
		samplPtsY(__samplPtsY),
		samplPtsZ(__samplPtsZ),
		selectedTris(__selectedTris),
		planeResolSq(__planeResolSq),
		totalFaceArea(__totalFaceArea),
		numDistinctGBs(__numDistinctGBs),
		ballVolume(__ballVolume),
    gFixedT(__gFixedT)
	{
	}

	virtual ~ProbeDistrib()
	{
	}

	void probe(size_t start, size_t end) const {

		for (size_t ptIdx = start; ptIdx < end; ptIdx++)
		{
			float fixedNormal1[3] = { samplPtsX.at(ptIdx), samplPtsY.at(ptIdx), samplPtsZ.at(ptIdx) };
			float fixedNormal2[3] = { 0.0f, 0.0f, 0.0f };
			MatrixMath::Multiply3x3with3x1(gFixedT, fixedNormal1, fixedNormal2);

			for (int triRepresIdx = 0; triRepresIdx < selectedTris.size(); triRepresIdx++)
			{
				for (int inversion = 0; inversion <= 1; inversion++)
				{
					float sign = 1.0f;
					if (inversion == 1) sign = -1.0f;

          float theta1 = acosf(sign * (
            selectedTris[triRepresIdx].normal_grain1_x * fixedNormal1[0] +
            selectedTris[triRepresIdx].normal_grain1_y * fixedNormal1[1] +
            selectedTris[triRepresIdx].normal_grain1_z * fixedNormal1[2]));

          float theta2 = acosf(-sign * (
            selectedTris[triRepresIdx].normal_grain2_x * fixedNormal2[0] +
            selectedTris[triRepresIdx].normal_grain2_y * fixedNormal2[1] +
            selectedTris[triRepresIdx].normal_grain2_z * fixedNormal2[2]));

					float distSq = 0.5f * (theta1 * theta1 + theta2 * theta2);

					if (distSq < planeResolSq)
					{
						(*distribValues)[ptIdx] += selectedTris[triRepresIdx].area;
					}
				}
			}
			(*errorValues)[ptIdx] = sqrt((*distribValues)[ptIdx] / totalFaceArea / double(numDistinctGBs)) / ballVolume;

			(*distribValues)[ptIdx] /= totalFaceArea;
			(*distribValues)[ptIdx] /= ballVolume;
		}
	}

#ifdef SIMPLib_USE_PARALLEL_ALGORITHMS
	void operator()(const tbb::blocked_range<size_t>& r) const
	{
		probe(r.begin(), r.end());
	}
#endif
};


// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
FindGBCD_MetricBased::FindGBCD_MetricBased() :
  SurfaceMeshFilter(),
  m_PhaseOfInterest(1),
  m_ChosenLimitDists(DEFAULT_RESOL_CHOICE),
  m_NumSamplPts(3000),
  m_AddMorePtsNearEquator(true),
  m_DistOutputFile(""),
  m_ErrOutputFile(""),
  m_SaveRelativeErr(false),

  m_CrystalStructuresArrayPath(DREAM3D::Defaults::ImageDataContainerName, DREAM3D::Defaults::CellEnsembleAttributeMatrixName, DREAM3D::EnsembleData::CrystalStructures),
  m_FeatureEulerAnglesArrayPath(DREAM3D::Defaults::ImageDataContainerName, DREAM3D::Defaults::CellFeatureAttributeMatrixName, DREAM3D::FeatureData::AvgEulerAngles),
  m_FeaturePhasesArrayPath(DREAM3D::Defaults::ImageDataContainerName, DREAM3D::Defaults::CellFeatureAttributeMatrixName, DREAM3D::FeatureData::Phases),
  m_SurfaceMeshFaceLabelsArrayPath(DREAM3D::Defaults::TriangleDataContainerName, DREAM3D::Defaults::FaceAttributeMatrixName, DREAM3D::FaceData::SurfaceMeshFaceLabels),
  m_SurfaceMeshFaceNormalsArrayPath(DREAM3D::Defaults::TriangleDataContainerName, DREAM3D::Defaults::FaceAttributeMatrixName, DREAM3D::FaceData::SurfaceMeshFaceNormals),
  m_SurfaceMeshFaceAreasArrayPath(DREAM3D::Defaults::TriangleDataContainerName, DREAM3D::Defaults::FaceAttributeMatrixName, DREAM3D::FaceData::SurfaceMeshFaceAreas),
  m_SurfaceMeshFeatureFaceLabelsArrayPath(DREAM3D::Defaults::TriangleDataContainerName, DREAM3D::Defaults::FaceFeatureAttributeMatrixName, "FaceLabels"),

  m_CrystalStructures(NULL),
  m_FeatureEulerAngles(NULL),
  m_FeaturePhases(NULL),
  m_SurfaceMeshFaceLabels(NULL),
  m_SurfaceMeshFaceNormals(NULL),
  m_SurfaceMeshFeatureFaceLabels(NULL),
  m_SurfaceMeshFaceAreas(NULL)
{
	m_MisorientationRotation.angle = 38.94f;
	m_MisorientationRotation.h = 1.0f;
	m_MisorientationRotation.k = 1.0f;
	m_MisorientationRotation.l = 0.0f;

	setupFilterParameters();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
FindGBCD_MetricBased::~FindGBCD_MetricBased()
{
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void FindGBCD_MetricBased::setupFilterParameters()
{
	FilterParameterVector parameters;
	parameters.push_back(IntFilterParameter::New("Phase of Interest", "PhaseOfInterest", getPhaseOfInterest(), FilterParameter::Parameter));
	parameters.push_back(AxisAngleFilterParameter::New("Fixed Misorientation", "MisorientationRotation", getMisorientationRotation(), FilterParameter::Parameter));
	{
		ChoiceFilterParameter::Pointer parameter = ChoiceFilterParameter::New();
		parameter->setHumanLabel("Limiting Distances");
		parameter->setPropertyName("ChosenLimitDists");

		QVector<QString> choices;

		for (int choiceIdx = 0; choiceIdx < NUM_RESOL_CHOICES; choiceIdx++)
		{
			QString misorResStr;
			QString planeResStr;
			QString degSymbol = QChar(0x00B0);

			misorResStr.setNum(RESOL_CHOICES[choiceIdx][0], 'f', 0);
			planeResStr.setNum(RESOL_CHOICES[choiceIdx][1], 'f', 0);

			choices.push_back(misorResStr + degSymbol + " for Misorientations; " + planeResStr + degSymbol + " for Plane Inclinations");
		}

		parameter->setChoices(choices);
		parameter->setCategory(FilterParameter::Parameter);
		parameters.push_back(parameter);


	}
	parameters.push_back(IntFilterParameter::New("Number of Sampling Points (on a Hemisphere)", "NumSamplPts", getNumSamplPts(), FilterParameter::Parameter));
	parameters.push_back(BooleanFilterParameter::New("Include Points from the Southern Hemisphere from the Equator's Vicinity", "AddMorePtsNearEquator", getAddMorePtsNearEquator(), FilterParameter::Parameter));
	parameters.push_back(OutputFileFilterParameter::New("Save Distribution to", "DistOutputFile", getDistOutputFile(), FilterParameter::Parameter, ""));
	parameters.push_back(OutputFileFilterParameter::New("Save Distribution Errors to", "ErrOutputFile", getErrOutputFile(), FilterParameter::Parameter, ""));
	parameters.push_back(BooleanFilterParameter::New("Save Relative Errors Instead of Their Absolute Values", "SaveRelativeErr", getSaveRelativeErr(), FilterParameter::Parameter));


	parameters.push_back(SeparatorFilterParameter::New("Cell Ensemble Data", FilterParameter::RequiredArray));
	{
		DataArraySelectionFilterParameter::RequirementType req = DataArraySelectionFilterParameter::CreateRequirement(DREAM3D::TypeNames::UInt32, 1, DREAM3D::AttributeMatrixType::CellEnsemble, DREAM3D::GeometryType::ImageGeometry);
		parameters.push_back(DataArraySelectionFilterParameter::New("Crystal Structures", "CrystalStructuresArrayPath", getCrystalStructuresArrayPath(), FilterParameter::RequiredArray, req));
	}


	parameters.push_back(SeparatorFilterParameter::New("Cell Feature Data", FilterParameter::RequiredArray));
	{
		DataArraySelectionFilterParameter::RequirementType req = DataArraySelectionFilterParameter::CreateRequirement(DREAM3D::TypeNames::Float, 3, DREAM3D::AttributeMatrixType::CellFeature, DREAM3D::GeometryType::ImageGeometry);
		parameters.push_back(DataArraySelectionFilterParameter::New("Average Euler Angles", "FeatureEulerAnglesArrayPath", getFeatureEulerAnglesArrayPath(), FilterParameter::RequiredArray, req));
	}
  {
	  DataArraySelectionFilterParameter::RequirementType req = DataArraySelectionFilterParameter::CreateRequirement(DREAM3D::TypeNames::Int32, 1, DREAM3D::AttributeMatrixType::CellFeature, DREAM3D::GeometryType::ImageGeometry);
	  parameters.push_back(DataArraySelectionFilterParameter::New("Phases", "FeaturePhasesArrayPath", getFeaturePhasesArrayPath(), FilterParameter::RequiredArray, req));
  }


  parameters.push_back(SeparatorFilterParameter::New("Face Data", FilterParameter::RequiredArray));
  {
	  DataArraySelectionFilterParameter::RequirementType req = DataArraySelectionFilterParameter::CreateRequirement(DREAM3D::TypeNames::Int32, 2, DREAM3D::AttributeMatrixType::Face, DREAM3D::GeometryType::TriangleGeometry);
	  parameters.push_back(DataArraySelectionFilterParameter::New("Face Labels", "SurfaceMeshFaceLabelsArrayPath", getSurfaceMeshFaceLabelsArrayPath(), FilterParameter::RequiredArray, req));
  }
  {
	  DataArraySelectionFilterParameter::RequirementType req = DataArraySelectionFilterParameter::CreateRequirement(DREAM3D::TypeNames::Double, 3, DREAM3D::AttributeMatrixType::Face, DREAM3D::GeometryType::TriangleGeometry);
	  parameters.push_back(DataArraySelectionFilterParameter::New("Face Normals", "SurfaceMeshFaceNormalsArrayPath", getSurfaceMeshFaceNormalsArrayPath(), FilterParameter::RequiredArray, req));
  }
  {
	  DataArraySelectionFilterParameter::RequirementType req = DataArraySelectionFilterParameter::CreateRequirement(DREAM3D::TypeNames::Double, 1, DREAM3D::AttributeMatrixType::Face, DREAM3D::GeometryType::TriangleGeometry);
	  parameters.push_back(DataArraySelectionFilterParameter::New("Face Areas", "SurfaceMeshFaceAreasArrayPath", getSurfaceMeshFaceAreasArrayPath(), FilterParameter::RequiredArray, req));
  }
  parameters.push_back(SeparatorFilterParameter::New("Face Feature Data", FilterParameter::RequiredArray));
  {
	  DataArraySelectionFilterParameter::RequirementType req = DataArraySelectionFilterParameter::CreateRequirement(DREAM3D::TypeNames::Int32, 2, DREAM3D::AttributeMatrixType::Face, DREAM3D::GeometryType::TriangleGeometry);
	  parameters.push_back(DataArraySelectionFilterParameter::New("Feature Face Labels", "SurfaceMeshFeatureFaceLabelsArrayPath", getSurfaceMeshFeatureFaceLabelsArrayPath(), FilterParameter::RequiredArray, req));
  }
 
  setFilterParameters(parameters);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void FindGBCD_MetricBased::readFilterParameters(AbstractFilterParametersReader* reader, int index)
{
	reader->openFilterGroup(this, index);
	setPhaseOfInterest(reader->readValue("PhaseOfInterest", getPhaseOfInterest()));
	setMisorientationRotation(reader->readAxisAngle("MisorientationRotation", getMisorientationRotation(), -1));
	setChosenLimitDists(reader->readValue("ChosenLimitDists", getChosenLimitDists()));
	setNumSamplPts(reader->readValue("NumSamplPts", getNumSamplPts()));
	setAddMorePtsNearEquator(reader->readValue("AddMorePtsNearEquator", getAddMorePtsNearEquator()));
	setDistOutputFile(reader->readString("DistOutputFile", getDistOutputFile()));
	setErrOutputFile(reader->readString("ErrOutputFile", getErrOutputFile()));
	setSaveRelativeErr(reader->readValue("SaveRelativeErr", getSaveRelativeErr()));

	setCrystalStructuresArrayPath(reader->readDataArrayPath("CrystalStructures", getCrystalStructuresArrayPath()));
	setFeatureEulerAnglesArrayPath(reader->readDataArrayPath("FeatureEulerAngles", getFeatureEulerAnglesArrayPath()));
	setFeaturePhasesArrayPath(reader->readDataArrayPath("FeaturePhases", getFeaturePhasesArrayPath()));
	setSurfaceMeshFaceLabelsArrayPath(reader->readDataArrayPath("SurfaceMeshFaceLabels", getSurfaceMeshFaceLabelsArrayPath()));
	setSurfaceMeshFaceNormalsArrayPath(reader->readDataArrayPath("SurfaceMeshFaceNormals", getSurfaceMeshFaceNormalsArrayPath()));
	setSurfaceMeshFeatureFaceLabelsArrayPath(reader->readDataArrayPath("SurfaceMeshFeatureFaceLabels", getSurfaceMeshFeatureFaceLabelsArrayPath()));
	setSurfaceMeshFaceAreasArrayPath(reader->readDataArrayPath("SurfaceMeshFaceAreas", getSurfaceMeshFaceAreasArrayPath()));
	reader->closeFilterGroup();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
int FindGBCD_MetricBased::writeFilterParameters(AbstractFilterParametersWriter* writer, int index)
{
	writer->openFilterGroup(this, index);
	SIMPL_FILTER_WRITE_PARAMETER(PhaseOfInterest)
	SIMPL_FILTER_WRITE_PARAMETER(MisorientationRotation)
	SIMPL_FILTER_WRITE_PARAMETER(ChosenLimitDists)
	SIMPL_FILTER_WRITE_PARAMETER(NumSamplPts)
	SIMPL_FILTER_WRITE_PARAMETER(AddMorePtsNearEquator)
	SIMPL_FILTER_WRITE_PARAMETER(DistOutputFile)
	SIMPL_FILTER_WRITE_PARAMETER(ErrOutputFile)
	SIMPL_FILTER_WRITE_PARAMETER(SaveRelativeErr)

	SIMPL_FILTER_WRITE_PARAMETER(CrystalStructuresArrayPath)
	SIMPL_FILTER_WRITE_PARAMETER(FeatureEulerAnglesArrayPath)
	SIMPL_FILTER_WRITE_PARAMETER(FeaturePhasesArrayPath)
	SIMPL_FILTER_WRITE_PARAMETER(SurfaceMeshFaceLabelsArrayPath)
	SIMPL_FILTER_WRITE_PARAMETER(SurfaceMeshFaceNormalsArrayPath)
	SIMPL_FILTER_WRITE_PARAMETER(SurfaceMeshFeatureFaceLabelsArrayPath)
	SIMPL_FILTER_WRITE_PARAMETER(SurfaceMeshFaceAreasArrayPath)
	writer->closeFilterGroup();
	return ++index; // we want to return the next index that was just written to
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void FindGBCD_MetricBased::dataCheck()
{
	setErrorCondition(0);


	// Fixed Misorientation (filter params.)
	if (getMisorientationRotation().angle <= 0.0 || getMisorientationRotation().angle > 180.0) {

		QString degSymbol = QChar(0x00B0);
		QString ss = "The misorientation angle should be in the range (0, 180" + degSymbol + "]";
		notifyErrorMessage(getHumanLabel(), ss, -1);
		setErrorCondition(-1);
	}
	if (getMisorientationRotation().h == 0.0f && getMisorientationRotation().k == 0.0f && getMisorientationRotation().l == 0.0f) {

		QString ss = QObject::tr("All three indices of the misorientation axis cannot be 0");
		notifyErrorMessage(getHumanLabel(), ss, -1);
		setErrorCondition(-1);
	}


	// Number of Sampling Points (filter params.)
	if (getNumSamplPts() < 1) {

		QString ss = QObject::tr("The number of sampling points must be greater than zero");
		notifyErrorMessage(getHumanLabel(), ss, -1);
		setErrorCondition(-1);
	}
	if (getNumSamplPts() > 5000) { // set some reasonable value, but allow user to use more if he/she knows what he/she does

		QString ss = QObject::tr("Most likely, you do not need to use that many sampling points");
		notifyWarningMessage(getHumanLabel(), ss, -1);
	}


	// Output files (filter params.)
	if (getDistOutputFile().isEmpty() == true)
	{
		QString ss = QObject::tr("The output file for saving the distribution must be set");
		notifyErrorMessage(getHumanLabel(), ss, -1000);
		setErrorCondition(-1);
	}
	if (getErrOutputFile().isEmpty() == true)
	{
		QString ss = QObject::tr("The output file for saving the distribution errors must be set");
		notifyErrorMessage(getHumanLabel(), ss, -1000);
		setErrorCondition(-1);
	}


	QFileInfo distOutFileInfo(getDistOutputFile());
	QDir distParentPath = distOutFileInfo.path();
	if (distParentPath.exists() == false)
	{
		QString ss = QObject::tr("The directory path for the distribution output file does not exist. DREAM.3D will attempt to create this path during execution of the filter");
		notifyWarningMessage(getHumanLabel(), ss, -1);
	}


	QFileInfo errOutFileInfo(getErrOutputFile());
	QDir errParentPath = errOutFileInfo.path();
	if (errParentPath.exists() == false)
	{
		QString ss = QObject::tr("The directory path for the distribution errors output file does not exist. DREAM.3D will attempt to create this path during execution of the filter");
		notifyWarningMessage(getHumanLabel(), ss, -1);
	}


  if (distOutFileInfo.suffix().compare("") == 0)
  {
    setDistOutputFile(getDistOutputFile().append(".dat"));
  }
  if (errOutFileInfo.suffix().compare("") == 0)
  {
    setErrOutputFile(getErrOutputFile().append(".dat"));
  }


  // Make sure the file name ends with _1 so the GMT scripts work correctly
  QString distFName = distOutFileInfo.baseName();
  if (distFName.endsWith("_1") == false)
  {
    distFName = distFName + "_1";
    QString absPath = distOutFileInfo.absolutePath() + "/" + distFName + ".dat";
    setDistOutputFile(absPath);
  }

  QString errFName = errOutFileInfo.baseName();
  if (errFName.endsWith("_1") == false)
  {
    errFName = errFName + "_1";
    QString absPath = distOutFileInfo.absolutePath() + "/" + errFName + ".dat";
    setDistOutputFile(absPath);
  }


  if (getDistOutputFile().isEmpty() == false && getDistOutputFile() == getErrOutputFile())
  {
    QString ss = QObject::tr("The output files must be different");
    notifyErrorMessage(getHumanLabel(), ss, -1);
    setErrorCondition(-1);
  }


	// Crystal Structures (DREAM file)
	QVector<size_t> cDims(1, 1);
	m_CrystalStructuresPtr = getDataContainerArray()->getPrereqArrayFromPath<DataArray<unsigned int>, AbstractFilter>(this, getCrystalStructuresArrayPath(), cDims); /* Assigns the shared_ptr<> to an instance variable that is a weak_ptr<> */
	if (NULL != m_CrystalStructuresPtr.lock().get()) /* Validate the Weak Pointer wraps a non-NULL pointer to a DataArray<T> object */
	{
		m_CrystalStructures = m_CrystalStructuresPtr.lock()->getPointer(0);
	} /* Now assign the raw pointer to data from the DataArray<T> object */


	// Phase of Interest  (filter params.)
  if (NULL != m_CrystalStructuresPtr.lock().get()) {
    if (getPhaseOfInterest() >= m_CrystalStructuresPtr.lock()->getNumberOfTuples() || getPhaseOfInterest() <= 0)
    {
      QString ss = QObject::tr("The phase index is either larger than the number of Ensembles or smaller than 1");
      notifyErrorMessage(getHumanLabel(), ss, -1);
      setErrorCondition(-381);
    }
  }


	// Euler Angels (DREAM file)
	cDims[0] = 3;
	m_FeatureEulerAnglesPtr = getDataContainerArray()->getPrereqArrayFromPath<DataArray<float>, AbstractFilter>(this, getFeatureEulerAnglesArrayPath(), cDims); /* Assigns the shared_ptr<> to an instance variable that is a weak_ptr<> */
	if (NULL != m_FeatureEulerAnglesPtr.lock().get()) /* Validate the Weak Pointer wraps a non-NULL pointer to a DataArray<T> object */
	{
		m_FeatureEulerAngles = m_FeatureEulerAnglesPtr.lock()->getPointer(0);
	} /* Now assign the raw pointer to data from the DataArray<T> object */


	// Phases (DREAM file)
	cDims[0] = 1;
	m_FeaturePhasesPtr = getDataContainerArray()->getPrereqArrayFromPath<DataArray<int32_t>, AbstractFilter>(this, getFeaturePhasesArrayPath(), cDims); /* Assigns the shared_ptr<> to an instance variable that is a weak_ptr<> */
	if (NULL != m_FeaturePhasesPtr.lock().get()) /* Validate the Weak Pointer wraps a non-NULL pointer to a DataArray<T> object */
	{
		m_FeaturePhases = m_FeaturePhasesPtr.lock()->getPointer(0);
	} /* Now assign the raw pointer to data from the DataArray<T> object */


	// Face Labels (DREAM file)
	cDims[0] = 2;
	m_SurfaceMeshFaceLabelsPtr = getDataContainerArray()->getPrereqArrayFromPath<DataArray<int32_t>, AbstractFilter>(this, getSurfaceMeshFaceLabelsArrayPath(), cDims); /* Assigns the shared_ptr<> to an instance variable that is a weak_ptr<> */
	if (NULL != m_SurfaceMeshFaceLabelsPtr.lock().get()) /* Validate the Weak Pointer wraps a non-NULL pointer to a DataArray<T> object */
	{
		m_SurfaceMeshFaceLabels = m_SurfaceMeshFaceLabelsPtr.lock()->getPointer(0);
	} /* Now assign the raw pointer to data from the DataArray<T> object */


	// Face Normals (DREAM file)
	cDims[0] = 3;
	m_SurfaceMeshFaceNormalsPtr = getDataContainerArray()->getPrereqArrayFromPath<DataArray<double>, AbstractFilter>(this, getSurfaceMeshFaceNormalsArrayPath(), cDims); /* Assigns the shared_ptr<> to an instance variable that is a weak_ptr<> */
	if (NULL != m_SurfaceMeshFaceNormalsPtr.lock().get()) /* Validate the Weak Pointer wraps a non-NULL pointer to a DataArray<T> object */
	{
		m_SurfaceMeshFaceNormals = m_SurfaceMeshFaceNormalsPtr.lock()->getPointer(0);
	} /* Now assign the raw pointer to data from the DataArray<T> object */


	// Face Areas (DREAM file)
	cDims[0] = 1;
	m_SurfaceMeshFaceAreasPtr = getDataContainerArray()->getPrereqArrayFromPath<DataArray<double>, AbstractFilter>(this, getSurfaceMeshFaceAreasArrayPath(), cDims); /* Assigns the shared_ptr<> to an instance variable that is a weak_ptr<> */
	if (NULL != m_SurfaceMeshFaceAreasPtr.lock().get()) /* Validate the Weak Pointer wraps a non-NULL pointer to a DataArray<T> object */
	{
		m_SurfaceMeshFaceAreas = m_SurfaceMeshFaceAreasPtr.lock()->getPointer(0);
	} /* Now assign the raw pointer to data from the DataArray<T> object */


	// Feature Face Labels (DREAM file)
	cDims[0] = 2;
	m_SurfaceMeshFeatureFaceLabelsPtr = getDataContainerArray()->getPrereqArrayFromPath<DataArray<int32_t>, AbstractFilter>(this, getSurfaceMeshFeatureFaceLabelsArrayPath(), cDims); /* Assigns the shared_ptr<> to an instance variable that is a weak_ptr<> */
	if (NULL != m_SurfaceMeshFeatureFaceLabelsPtr.lock().get()) /* Validate the Weak Pointer wraps a non-NULL pointer to a DataArray<T> object */
	{
		m_SurfaceMeshFeatureFaceLabels = m_SurfaceMeshFeatureFaceLabelsPtr.lock()->getPointer(0);
	} /* Now assign the raw pointer to data from the DataArray<T> object */


}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void FindGBCD_MetricBased::preflight()
{
	// These are the REQUIRED lines of CODE to make sure the filter behaves correctly
	setInPreflight(true); // Set the fact that we are preflighting.
	emit preflightAboutToExecute(); // Emit this signal so that other widgets can do one file update
	emit updateFilterParameters(this); // Emit this signal to have the widgets push their values down to the filter
	dataCheck(); // Run our DataCheck to make sure everthing is setup correctly
	emit preflightExecuted(); // We are done preflighting this filter
	setInPreflight(false); // Inform the system this filter is NOT in preflight mode anymore.
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void FindGBCD_MetricBased::execute()
{
	setErrorCondition(0);
	dataCheck();
	if (getErrorCondition() < 0) { return; }


	// -------------------- set resolutions and 'ball volumes' based on user's selection --------------------
	float m_misorResol = RESOL_CHOICES[getChosenLimitDists()][0];
	float m_planeResol = RESOL_CHOICES[getChosenLimitDists()][1];

	m_misorResol *= SIMPLib::Constants::k_PiOver180;
	m_planeResol *= SIMPLib::Constants::k_PiOver180;
	float m_PlaneResolSq = m_planeResol * m_planeResol;


	// We want to work with the raw pointers for speed so get those pointers.
	uint32_t* m_CrystalStructures = m_CrystalStructuresPtr.lock()->getPointer(0);
	float* m_Eulers = m_FeatureEulerAnglesPtr.lock()->getPointer(0);
	int32_t* m_Phases = m_FeaturePhasesPtr.lock()->getPointer(0);
	int32_t* m_FaceLabels = m_SurfaceMeshFaceLabelsPtr.lock()->getPointer(0);
	double* m_FaceNormals = m_SurfaceMeshFaceNormalsPtr.lock()->getPointer(0);
	double* m_FaceAreas = m_SurfaceMeshFaceAreasPtr.lock()->getPointer(0);
	int32_t* m_FeatureFaceLabels = m_SurfaceMeshFeatureFaceLabelsPtr.lock()->getPointer(0);

	// -------------------- check if directiories are ok and if output files can be opened --------------------

	// Make sure any directory path is also available as the user may have just typed
	// in a path without actually creating the full path
	QFileInfo distOutFileInfo(getDistOutputFile());

	QDir distOutFileDir(distOutFileInfo.path());
	if (!distOutFileDir.mkpath("."))
	{
		QString ss;
		ss = QObject::tr("Error creating parent path '%1'").arg(distOutFileDir.path());
		notifyErrorMessage(getHumanLabel(), ss, -1);
		setErrorCondition(-1);
		return;
	}

	QFile distOutFile(getDistOutputFile());
	if (!distOutFile.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		QString ss = QObject::tr("Error opening output file '%1'").arg(getDistOutputFile());
		setErrorCondition(-100);
		notifyErrorMessage(getHumanLabel(), ss, getErrorCondition());
		return;
	}

	QFileInfo errOutFileInfo(getDistOutputFile());

	QDir errOutFileDir(errOutFileInfo.path());
	if (!errOutFileDir.mkpath("."))
	{
		QString ss;
		ss = QObject::tr("Error creating parent path '%1'").arg(errOutFileDir.path());
		notifyErrorMessage(getHumanLabel(), ss, -1);
		setErrorCondition(-1);
		return;
	}

	QFile errOutFile(getDistOutputFile());
	if (!errOutFile.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		QString ss = QObject::tr("Error opening output file '%1'").arg(getDistOutputFile());
		setErrorCondition(-100);
		notifyErrorMessage(getHumanLabel(), ss, getErrorCondition());
		return;
	}

	// Open the output files, should be opened and checked before starting computations TODO
	FILE* fDist = NULL;
	fDist = fopen(m_DistOutputFile.toLatin1().data(), "wb");
	if (NULL == fDist)
	{
		QString ss = QObject::tr("Error opening distribution output file '%1'").arg(m_DistOutputFile);
		setErrorCondition(-1);
		notifyErrorMessage(getHumanLabel(), ss, getErrorCondition());
		return;
	}

	FILE* fErr = NULL;
	fErr = fopen(m_ErrOutputFile.toLatin1().data(), "wb");
	if (NULL == fErr)
	{
		QString ss = QObject::tr("Error opening distribution errors output file '%1'").arg(m_ErrOutputFile);
		setErrorCondition(-1);
		notifyErrorMessage(getHumanLabel(), ss, getErrorCondition());
		return;
	}


  // ------------------- before computing the distribution, we must find normalization factors ----------------------
  double ballVolume = BALL_VOLS_M3M[getChosenLimitDists()];
  {
    QVector<SpaceGroupOps::Pointer> m_OrientationOps = SpaceGroupOps::getOrientationOpsQVector();
    int32_t cryst = m_CrystalStructures[m_PhaseOfInterest];
    int32_t nsym = m_OrientationOps[cryst]->getNumSymOps();

    if (cryst != 1) {
      double symFactor = double(nsym) / 24.0;
      symFactor *= symFactor;
      ballVolume *= symFactor;
    }
  }


	// ------------------------------ generation of sampling points ------------------------------

	QString ss = QObject::tr("--> Generating sampling points");
	notifyStatusMessage(getMessagePrefix(), getHumanLabel(), ss);

	// generate "Golden Section Spiral", see http://www.softimageblog.com/archives/115

	int numSamplPts_WholeSph = 2 * m_NumSamplPts; // here we generate points on the whole sphere
	QVector<float> samplPtsX_WholeSph(0);
	QVector<float> samplPtsY_WholeSph(0);
	QVector<float> samplPtsZ_WholeSph(0);

	QVector<float> samplPtsX(0);
	QVector<float> samplPtsY(0);
	QVector<float> samplPtsZ(0);

	float _inc = 2.3999632f; // = pi * (3 - sqrt(5))
	float _off = 2.0f / float(numSamplPts_WholeSph);

	for (int ptIdx_WholeSph = 0; ptIdx_WholeSph < numSamplPts_WholeSph; ptIdx_WholeSph++)
	{
		if (getCancel() == true) { return; }

		float _y = (float(ptIdx_WholeSph) * _off) - 1.0f + (0.5f * _off);
		float _r = sqrtf(fmaxf(1.0f - _y*_y, 0.0f));
		float _phi = float(ptIdx_WholeSph) * _inc;

		samplPtsX_WholeSph.push_back(cosf(_phi) * _r);
		samplPtsY_WholeSph.push_back(_y);
		samplPtsZ_WholeSph.push_back(sinf(_phi) * _r);
	}

	// now, select the points from the upper hemisphere,
	// and - if needed - from lower hemisphere from the neighborhood of equator

	float howFarBelowEquator = 0.0f;
  if (getAddMorePtsNearEquator() == true) howFarBelowEquator = -3.0001f / sqrtf(float(numSamplPts_WholeSph));

	for (int ptIdx_WholeSph = 0; ptIdx_WholeSph < numSamplPts_WholeSph; ptIdx_WholeSph++)
	{
		if (getCancel() == true) { return; }

		if (samplPtsZ_WholeSph[ptIdx_WholeSph] > howFarBelowEquator)
		{
			samplPtsX.push_back(samplPtsX_WholeSph[ptIdx_WholeSph]);
			samplPtsY.push_back(samplPtsY_WholeSph[ptIdx_WholeSph]);
			samplPtsZ.push_back(samplPtsZ_WholeSph[ptIdx_WholeSph]);
		}
	}


	// convert axis angle to matrix representation of misorientation
	float gFixed[3][3] = { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };
	float gFixedT[3][3] = { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };

	{
		float gFixedAngle = m_MisorientationRotation.angle * SIMPLib::Constants::k_PiOver180;
		float gFixedAxis[3] = { m_MisorientationRotation.h, m_MisorientationRotation.k, m_MisorientationRotation.l };
		MatrixMath::Normalize3x1(gFixedAxis);
		FOrientArrayType om(9);
		FOrientTransformsType::ax2om(FOrientArrayType(gFixedAxis[0], gFixedAxis[1], gFixedAxis[2], gFixedAngle), om);
		om.toGMatrix(gFixed);
	}

	MatrixMath::Transpose3x3(gFixed, gFixedT);

	int32_t numMeshTris = m_SurfaceMeshFaceAreasPtr.lock()->getNumberOfTuples();
	

	// ---------  find triangles (and equivalent crystallographic parameters) with +- the fixed misorientation ---------
  double totalFaceArea = 0.0;

#ifdef SIMPLib_USE_PARALLEL_ALGORITHMS
  tbb::task_scheduler_init init;
  bool doParallel = true;
  tbb::concurrent_vector<TriAreaAndNormals> selectedTris(0);
#else
  QVector<TriAreaAndNormals> selectedTris(0);
#endif


  size_t trisChunkSize = 50000; 
  if (numMeshTris < trisChunkSize) { trisChunkSize = numMeshTris; }

  for (size_t i = 0; i < numMeshTris; i = i + trisChunkSize)
  {
    if (getCancel() == true) { return; }
    ss = QObject::tr("--> step 1/2: selecting triangles with the specified misorientation (%1\% completed)").arg(int(100.0 * float(i) / float(numMeshTris)));
    notifyStatusMessage(getMessagePrefix(), getHumanLabel(), ss);
    if (i + trisChunkSize >= numMeshTris)
    {
      trisChunkSize = numMeshTris - i;
    }

#ifdef SIMPLib_USE_PARALLEL_ALGORITHMS
    if (doParallel == true)
    {
      tbb::parallel_for(tbb::blocked_range<size_t>(i, i + trisChunkSize),
        TrisSelector(
        &selectedTris,

        samplPtsX,
        samplPtsY,
        samplPtsZ,

        m_misorResol,
        m_PhaseOfInterest,
        gFixedT,

        m_CrystalStructures,
        m_Eulers,
        m_Phases,
        m_FaceLabels,
        m_FaceNormals,
        m_FaceAreas,
        m_FeatureFaceLabels,
        totalFaceArea
        ), tbb::auto_partitioner());
    }
    else
#endif
    {
      TrisSelector serial(
        &selectedTris,

        samplPtsX,
        samplPtsY,
        samplPtsZ,

        m_misorResol,
        m_PhaseOfInterest,
        gFixedT,

        m_CrystalStructures,
        m_Eulers,
        m_Phases,
        m_FaceLabels,
        m_FaceNormals,
        m_FaceAreas,
        m_FeatureFaceLabels,
        totalFaceArea
        );
      serial.select(i, i + trisChunkSize);
    }
  }

  // ------------------------  find the number of distinct boundaries --------------------------
  int32_t numDistinctGBs = 0;
  int32_t numFaceFeatures = m_SurfaceMeshFeatureFaceLabelsPtr.lock()->getNumberOfTuples();
  
  for (int featureFaceIdx = 0; featureFaceIdx < numFaceFeatures; featureFaceIdx++)
  {
    int32_t feature1 = m_FeatureFaceLabels[2 * featureFaceIdx];
    int32_t feature2 = m_FeatureFaceLabels[2 * featureFaceIdx + 1];

    if (feature1 < 1 || feature2 < 1) { continue; }
    if (m_FeaturePhases[feature1] != m_FeaturePhases[feature2])  { continue; }
    if (m_FeaturePhases[feature1] != m_PhaseOfInterest || m_FeaturePhases[feature2] != m_PhaseOfInterest) { continue; }

    numDistinctGBs++;
  }


	// ----------------- determining distribution values at the sampling points (and their errors) -------------------

	QVector<double> distribValues(samplPtsX.size(), 0.0);
	QVector<double> errorValues(samplPtsX.size(), 0.0);

	size_t pointsChunkSize = 100; 
	if (samplPtsX.size() < pointsChunkSize) { pointsChunkSize = samplPtsX.size(); }

	for (size_t i = 0; i < samplPtsX.size(); i = i + pointsChunkSize)
	{
		if (getCancel() == true) { return; }
		ss = QObject::tr("--> step2/2: computing distribution values at the section of interest (%1\% completed)").arg(int(100.0 * float(i) / float(samplPtsX.size())));
		notifyStatusMessage(getMessagePrefix(), getHumanLabel(), ss);
		if (i + pointsChunkSize >= samplPtsX.size())
		{
			pointsChunkSize = samplPtsX.size() - i;
		}

#ifdef SIMPLib_USE_PARALLEL_ALGORITHMS
		if (doParallel == true)
		{
			tbb::parallel_for(tbb::blocked_range<size_t>(i, i + pointsChunkSize),
				ProbeDistrib(
				&distribValues,
				&errorValues,
				samplPtsX,
				samplPtsY,
				samplPtsZ,
				selectedTris,
				m_PlaneResolSq,
        totalFaceArea,
				numDistinctGBs,
				ballVolume,
				gFixedT
				), tbb::auto_partitioner());

		}
		else
#endif
		{
			ProbeDistrib serial(
				&distribValues,
				&errorValues,
				samplPtsX,
				samplPtsY,
				samplPtsZ,
				selectedTris,
				m_PlaneResolSq,
        totalFaceArea,
				numDistinctGBs,
				ballVolume,
				gFixedT
				);
			serial.probe(i, i + pointsChunkSize);
		}
	}


	// ------------------------------------------- writing the output --------------------------------------------

	fprintf(fDist, "%.1f %.1f %.1f %.1f\n", m_MisorientationRotation.h, m_MisorientationRotation.k, m_MisorientationRotation.l, m_MisorientationRotation.angle);
	fprintf(fErr, "%.1f %.1f %.1f %.1f\n", m_MisorientationRotation.h, m_MisorientationRotation.k, m_MisorientationRotation.l, m_MisorientationRotation.angle);

	for (int ptIdx = 0; ptIdx < samplPtsX.size(); ptIdx++) {

		float zenith = acosf(samplPtsZ.at(ptIdx));
		float azimuth = atan2f(samplPtsY.at(ptIdx), samplPtsX.at(ptIdx));

		float zenithDeg = SIMPLib::Constants::k_180OverPi * zenith;
		float azimuthDeg = SIMPLib::Constants::k_180OverPi * azimuth;

		fprintf(fDist, "%.2f %.2f %.4f\n", azimuthDeg, 90.0f - zenithDeg,  distribValues[ptIdx]);

		if (m_SaveRelativeErr == false)
		{
      fprintf(fErr, "%.2f %.2f %.4f\n", azimuthDeg, 90.0f - zenithDeg, errorValues[ptIdx]);
		}
		else
		{
			double saneErr = 100.0;
			if (distribValues[ptIdx] > 1e-10) {
				saneErr = fmin(100.0, 100.0 * errorValues[ptIdx] / distribValues[ptIdx]);
			}
      fprintf(fErr, "%.4f %.4f %.2f %.2f %.1f\n", azimuthDeg, 90.0f - zenithDeg,  saneErr);
		}
	}
	fclose(fDist);
	fclose(fErr);

	if (getErrorCondition() < 0)
	{
		QString ss = QObject::tr("Something went wrong");
		setErrorCondition(-1);
		notifyErrorMessage(getHumanLabel(), ss, getErrorCondition());
		return;
	}

	notifyStatusMessage(getHumanLabel(), "Complete");
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
AbstractFilter::Pointer FindGBCD_MetricBased::newFilterInstance(bool copyFilterParameters)
{
	FindGBCD_MetricBased::Pointer filter = FindGBCD_MetricBased::New();
	if (true == copyFilterParameters)
	{
		copyFilterParameterInstanceVariables(filter.get());
	}
	return filter;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
const QString FindGBCD_MetricBased::getCompiledLibraryName()
{
	return SurfaceMeshingConstants::SurfaceMeshingBaseName;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
const QString FindGBCD_MetricBased::getGroupName()
{
	return DREAM3D::FilterGroups::Unsupported;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
const QString FindGBCD_MetricBased::getSubGroupName()
{
	return "Surface Meshing";
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
const QString FindGBCD_MetricBased::getHumanLabel()
{
	return "Find GBCD (Metric-based Approach)";
}

bool FindGBCD_MetricBased::doublesEqual(double x, double y)
{
  return fabsf(x - y) < 1e-8;
}
