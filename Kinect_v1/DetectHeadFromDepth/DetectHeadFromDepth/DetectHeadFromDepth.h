#pragma once


///
/// <Settings> �����\�ȃp�����[�^
/// 
 #define NEAR_MODE		// near���[�h���g���ꍇ�̓R�����g���O��

const UINT USER_NUM_MAX = 5;		// ���o�ł��郆�[�U�̍ő�l��

//const USHORT USER_DETECT_HEIGHT = 1200;		// �ŏ��Ƀ��[�U�����o����Ƃ���臒l(�n�ʂ���̍���)[mm]
const USHORT USER_DETECT_HEIGHT = 1200;		// Debug
const USHORT HEAD_HEIGHT_MAX	= 2400;		// ���[�U�̐g���̍ő�l(�v�����s���Ĉُ�ɑ傫���Ȃ����l��؂�臒l)

//const USHORT KINECT_HEIGHT		= 2700;		// �ݒu����Kinect�̍���[mm]
const USHORT KINECT_HEIGHT		= 1900;		// Debug
const USHORT DESK_HEIGHT		= 900;		//�@�f�X�N�̍���(���̈�ԒႢ�ʒu)[mm]
const USHORT SHOULDER_LENGTH	= 200;		// ���̍�����[mm]
const USHORT SHOULDER_HEIGHT	= 1200;		// ���܂ł̍���[mm]
const USHORT HEAD_LENGTH		= 230;		// �S����[mm]
const USHORT HEAD_HEIGHT		= 1500;
///
/// </Settings>
///


const NUI_IMAGE_RESOLUTION CAMERA_RESOLUTION = NUI_IMAGE_RESOLUTION_640x480;

#ifdef NEAR_MODE
const int SENCEING_MIN	= 400;		// �[�x�摜�ɕ\������ŏ�����[mm]
const int SENCEING_MAX	= 3000;		// �[�x�摜�ɕ\������ő勗��[mm]
#else
const int SENCEING_MIN	= 800;		// �[�x�摜�ɕ\������ŏ�����[mm]
const int SENCEING_MAX	= 4000;		// �[�x�摜�ɕ\������ő勗��[mm]
#endif

#define COLOR_IMAGE_WINNAME "rgbImage"
#define DEPTH_IMAGE_WINNAME "depthImage"

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

	cv::Rect boxes[USER_NUM_MAX];
	void boxesInit();

	void createInstance();
	void makeDepthImage( cv::Mat& depthImage, cv::Mat& heightMat );
	void drawRgbImage( cv::Mat& image );
	void drawDepthImage( cv::Mat& heightMat, cv::Mat& depthImage, cv::Mat& headImage, cv::Mat& shoulderImage );
	void detectUsers( cv::Mat& image );
	void detectEllipce( cv::Mat& srcImg, cv::Mat& dstImg, BOOL param );
	
};

MouseParam mparam;
void mfunc(int event, int x, int y, int flags, void  *param);