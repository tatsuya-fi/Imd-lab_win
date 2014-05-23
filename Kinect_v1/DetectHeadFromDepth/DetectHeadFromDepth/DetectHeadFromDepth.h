#pragma once

/// <Settings>
/// �����\�ȃp�����[�^

#define NEAR_MODE	// near���[�h���g���ꍇ�̓R�����g���O��
const NUI_IMAGE_RESOLUTION CAMERA_RESOLUTION = NUI_IMAGE_RESOLUTION_640x480;

#ifdef NEAR_MODE
const int SENCEING_MIN	= 400;		// �[�x�摜�ɕ\������ŏ�����[mm]
const int SENCEING_MAX	= 3000;		// �[�x�摜�ɕ\������ő勗��[mm]
#else
const int SENCEING_MIN	= 800;		// �[�x�摜�ɕ\������ŏ�����[mm]
const int SENCEING_MAX	= 4000;		// �[�x�摜�ɕ\������ő勗��[mm]
#endif

const USHORT KINECT_HEIGHT		= 1500;		// �ݒu����Kinect�̍���
const USHORT SHOULDER_LENGTH	= 280;		// ���̍�����[mm]
const USHORT SHOULDER_HEIGHT	= 500;		// ���܂ł̍���[mm]
//const int HEAD_LENGTH		= 280;		// �S����[mm]
//const int HEAD_HEIGHT		= 1500;

/// </Settings>

#define ERROR_CHECK( ret )										\
  if ( ret != S_OK ) {											\
    std::stringstream ss;										\
    ss << "failed " #ret " " << std::hex << ret << std::endl;	\
    throw std::runtime_error( ss.str().c_str() );				\
  }



class DetectHeadFromDepth
{
public:
	DetectHeadFromDepth();
	~DetectHeadFromDepth();
	
	void initialize();
	void run();

private:
	INuiSensor* kinect;
	HANDLE imageStreamHandle;
	HANDLE depthStreamHandle;
	HANDLE streamEvent;

	DWORD width;
	DWORD height;

	void createInstance();
	void drawRgbImage( cv::Mat& image );
	void drawDepthImage( cv::Mat& image );
	void detectEllipce( cv::Mat& image );
};