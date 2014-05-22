#ifndef _SKELTON_TEST
#define _SKELTON_TEST


#include <iostream>
#include <sstream>

// NuiApi.h�̑O��Windows.h���C���N���[�h����
#include <Windows.h>
#include <NuiApi.h>

#include <opencv2/opencv.hpp>
#include "myOpenCV.h"


#define ERROR_CHECK( ret )										\
	if (ret != S_OK) {											\
	std::stringstream ss;										\
	ss << "failed " #ret " " << std::hex << ret << std::endl;	\
	throw std::runtime_error(ss.str().c_str());					\
}


using namespace std;


const NUI_IMAGE_RESOLUTION CAMERA_RESOLUTION = NUI_IMAGE_RESOLUTION_640x480;


// �e���W�ϊ��s��
const cv::Mat cameraToPreCameraTransMat = (cv::Mat_<float>(4, 4) <<		// ���݂̃J�������W�n����(�����ʒu��)�J�������W�n�ւ̕ϊ��s��
	-0.6970703, 0.24027081, -0.67556548, 0.35205233,
0.023071989, 0.94921792, 0.31379095, -0.28363347,
0.71664596, 0.20314437, -0.66720647, 0.36001566,
0, 0, 0, 1
);
const cv::Mat disp3dToPreCameraTransMat = (cv::Mat_<float>(4, 4) <<	// �f�B�X�v���C(�P��[m])����(�����ʒu��)�J�������W�n�ւ̕ϊ��s��
	0.98487496, 0.0035287007, 0.17312445, -0.12823059,
0.011400914, 0.99436021, -0.10500347, -0.10807244,
-0.17289107, 0.10599659, 0.97928023, 0.6883387,
0,0,0,1
);
const float disp3dTodisp2d = 40 / (0.0112*1) ;	// �X�N���[����̓_���s�N�Z���P�ʂ�fx, fy





class KinectSample
{
private:

	INuiSensor* kinect;
	HANDLE imageStreamHandle;
	HANDLE depthStreamHandle;
	HANDLE streamEvent;

	DWORD width;
	DWORD height;

	BOOL saveFlag;
	POINT savedCursor[256];

public:

	KinectSample();
	~KinectSample();
	void initialize();
	void run();

private:

	void createInstance();
	void drawRgbImage(cv::Mat& image);
	void drawDepthImage(cv::Mat& image);
	void drawSkeleton(cv::Mat& image);
	void drawJoint(cv::Mat& image, Vector4 position);
	void drawCursor(cv::Mat& image,Vector4 head, Vector4 hand);
	void saveCursor(INT x, INT y);

};

#endif