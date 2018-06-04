//
//  CloneSpline.h
//  ClonerMulti
//
//  Created by Hunyadi János on 2015. 01. 03..
//  Copyright (c) 2015. Janos Hunyadi. All rights reserved.
//



#include "CloneSpline.h"


// --- Instance Spline ------
MStatus ClonerMultiThread::instanceSpline()
{


	MStatus status;

	m_tr_matA.clear();
	m_tr_matA.setLength(m_numDup);

	MVector currentNormal = m_firstUpVec;
	currentNormal.normalize();

	MVector prevNormal;
	MMatrix rotMatrix;


	// curve data
	MFnNurbsCurve curveFn(m_inCurve, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	if (curveFn.length() <= 0.0)
	{
		return MStatus::kSuccess;
	}


	int numdup_divider = m_numDup-1;
	if (numdup_divider < 1)
	{
		numdup_divider = 1;
	}

	// Set up a temp numdup divider just for the curve
	double length_curve = curveFn.length();
	double length_section = (length_curve / double(numdup_divider) );


	double offsetV = m_offsetZ;
	if (offsetV == 0.0){ offsetV = 0.001;}

	for(unsigned int i=0; i < m_numDup; i++)
	{

		double tempLength = double(i) * length_section;
		tempLength += offsetV;


		if (m_loopOffset)
		{



			double  timesLength = ( offsetV / length_curve);

			int  timesLengthFloor = 0;

			if (offsetV > 0 )
			{
				timesLengthFloor = int(floor(timesLength));
			}

			else
			{
				timesLengthFloor = int(ceil(timesLength));
			}



			offsetV -= (length_curve*double(timesLengthFloor));


			if (tempLength < 0)
			{
				tempLength = length_curve + tempLength;
			}

			else if (tempLength > length_curve )
			{
				tempLength -= length_curve;

			}
		}


		// Calculate matrix
		MPoint p;

		double param = curveFn.findParamFromLength( tempLength  );

		curveFn.getPointAtParam(param, p, MSpace::kObject );


		MVector tan = curveFn.tangent(param , MSpace::kObject);
		tan.normalize();

		MVector cross1 = currentNormal^tan;
		cross1.normalize();

		MVector cross2 =  tan^cross1;

		if(m_orientationType == 1)
		{
			cross2 = MVector(1.0,0.0,0.0);
		}

		if(m_orientationType == 2)
		{
			cross2 = MVector(0.0,1.0,0.0);
		}

		if(m_orientationType == 3)
		{
			cross2 = MVector(0.0,0.0,1.0);
		}


		cross2.normalize();
		currentNormal = cross2;


		double m[4][4] = {{tan.x, tan.y , tan.z, 0.0},
		{ cross2.x, cross2.y, cross2.z, 0.0},
		{ cross1.x, cross1.y, cross1.z, 0.0},
		{ p.x, p.y, p.z, 1.0}};

		rotMatrix = m;


		//if (m_orientationType == 1) {double m[4][4] = {{0.0, 1.0 , 0.0, 0.0},{ 1.0, 0.0, 0.0, 0.0},{ 0.0, 0.0, 1.0, 0.0},{ p.x, p.y, p.z, 1.0}};rotMatrix = m;}
		//if (m_orientationType == 2) {double m[4][4] = {{1.0, 0.0 , 0.0, 0.0},{ 0.0, 1.0, 0.0, 0.0},{ 0.0, 0.0, 1.0, 0.0},{ p.x, p.y, p.z, 1.0}};rotMatrix = m;}
		//if (m_orientationType == 3) {double m[4][4] = {{1.0, 0.0 , 0.0, 0.0},{ 0.0, 0.0, 1.0, 0.0},{ 0.0, -1.0, 0.0, 0.0},{ p.x, p.y, p.z, 1.0}};rotMatrix = m;}


		double off_ramp_mult = 1.0;

		if (i < int(m_offsetProfileA.length()))
		{
			off_ramp_mult = m_offsetProfileA[i];
		}



		double scale_ramp_mult = 1.0;

		if (i < int(m_scaleProfileA.length()))
		{
			scale_ramp_mult = m_scaleProfileA[i];
		}


		double rot_ramp_mult = 1.0;

		if (i < int(m_rotateProfileA.length()))
		{
			rot_ramp_mult = m_rotateProfileA[i];
		}




		// Translation X
		MFloatVector v_baseOffX(m_offsetX * off_ramp_mult, 0.0, 0.0);


		// Translation Y
		MFloatVector v_baseOffY(0.0, m_offsetY * off_ramp_mult, 0.0);


		// Rotation

		double rot[3] = {m_rotateX * 0.5f * ( M_PI / 180.0f ) * rot_ramp_mult, m_rotateY * 0.5f * ( M_PI / 180.0f ) * rot_ramp_mult,  m_rotateZ * 0.5f * ( M_PI / 180.0f ) * rot_ramp_mult };

		// Scale
		const double scaleV[3] = {  double(m_scaleX) * scale_ramp_mult,  double(m_scaleY) * scale_ramp_mult,  double(m_scaleZ) * scale_ramp_mult };




		// Random Transform
		MFloatVector v_rndOffV(m_rndOffsetXA[i] * off_ramp_mult, m_rndOffsetYA[i] * off_ramp_mult,m_rndOffsetZA[i] * off_ramp_mult);
		// Random Rotate

		double rot_rnd[3] = {m_rndRotateXA[i] * 0.5f * ( M_PI / 180.0f ) * rot_ramp_mult, m_rndRotateYA[i] * 0.5f * ( M_PI / 180.0f ) * rot_ramp_mult,  m_rndRotateZA[i] * 0.5f * ( M_PI / 180.0f ) * rot_ramp_mult };

		// Random Scale
		const double scaleV_rnd[3] = {  double(1.0+m_rndScaleXA[i]) * scale_ramp_mult,  double(1.0+m_rndScaleYA[i]) * scale_ramp_mult,  double(1.0+m_rndScaleZA[i]) * scale_ramp_mult };

		// Matrix
		MTransformationMatrix tr_mat(rotMatrix);


		status = tr_mat.addTranslation(v_baseOffX, MSpace::kObject);
		CHECK_MSTATUS_AND_RETURN_IT(status);
		status = tr_mat.addTranslation(v_baseOffY, MSpace::kObject);
		CHECK_MSTATUS_AND_RETURN_IT(status);
		status = tr_mat.addTranslation(v_rndOffV, MSpace::kObject);
		CHECK_MSTATUS_AND_RETURN_IT(status);

		status = tr_mat.setScale(scaleV, MSpace::kObject);
		CHECK_MSTATUS_AND_RETURN_IT(status);
		status = tr_mat.addScale(scaleV_rnd, MSpace::kObject);
		CHECK_MSTATUS_AND_RETURN_IT(status);

		status = tr_mat.addRotation(rot, MTransformationMatrix::kXYZ, MSpace::kObject);
		CHECK_MSTATUS_AND_RETURN_IT(status);
		status = tr_mat.addRotation(rot_rnd, MTransformationMatrix::kXYZ, MSpace::kObject);
		CHECK_MSTATUS_AND_RETURN_IT(status);





		m_tr_matA.set(tr_mat.asMatrix() * m_curveTrMat, i);





	}



	return MS::kSuccess;

}


