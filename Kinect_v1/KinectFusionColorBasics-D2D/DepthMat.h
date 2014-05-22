#pragma once

#include <iostream>
#include <sstream>

#include <Windows.h>
#include <NuiApi.h>

#include "myOpenCV.h"

#define ERROR_CHECK(ret)                                            \
if (ret != S_OK) {													\
	std::stringstream ss;                                           \
	ss << "failed " #ret " " << std::hex << ret << std::endl;       \
	throw std::runtime_error(ss.str().c_str());                     \
}


class DepthMat
{
public:
	DepthMat();
	~DepthMat();

	void setImageSize(NUI_IMAGE_RESOLUTION cameraResolution);
	void setINuiSensor(INuiSensor *handle);
	void setRgbImage(cv::Mat &image, NUI_IMAGE_FRAME &imageFrame);

	void getCameraImgToDispTransform(const char *windowname, cv::Mat &cameraToDispTransform);

	int setMat(Matrix4& transMat, int matNum);
	void DepthMat::caliculateCamToPrecamTransMat();
	void DepthMat::getPreCam3dPoint(Matrix4& camTrans, Vector4& preCamPoint);
	void DepthMat::drawPreCam(cv::Mat& image, Vector4 position);

private:
	INuiSensor *kinect;


	DWORD width;
	DWORD height;

	cv::Mat cameraToWorldTransMat0 = cv::Mat::eye(4, 4, CV_32F);
	cv::Mat cameraToWorldTransMat1 = cv::Mat::eye(4, 4, CV_32F);
	
	

};

const static cv::Mat internalParam = (cv::Mat_<float>(3, 4) <<
	526.37, 0, 313.69, 0,
	0, 526.37, 259.02, 0,
	0, 0, 1, 0);