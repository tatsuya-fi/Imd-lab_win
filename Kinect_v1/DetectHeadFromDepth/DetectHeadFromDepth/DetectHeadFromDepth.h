#pragma once


///
/// <Settings> 調整可能なパラメータ
/// 
 #define NEAR_MODE		// nearモードを使う場合はコメントを外す

const UINT USER_NUM_MAX = 5;		// 検出できるユーザの最大人数

//const USHORT USER_DETECT_HEIGHT = 1200;		// 最初にユーザを検出するときの閾値(地面からの高さ)[mm]
const USHORT USER_DETECT_HEIGHT = 1200;		// Debug
const USHORT HEAD_HEIGHT_MAX	= 2400;		// ユーザの身長の最大値(計測失敗して異常に大きくなった値を切る閾値)

//const USHORT KINECT_HEIGHT		= 2700;		// 設置したKinectの高さ[mm]
const USHORT KINECT_HEIGHT		= 1900;		// Debug
const USHORT DESK_HEIGHT		= 900;		//　デスクの高さ(肩の一番低い位置)[mm]
const USHORT SHOULDER_LENGTH	= 200;		// 肩の高さ幅[mm]
const USHORT SHOULDER_HEIGHT	= 1200;		// 肩までの高さ[mm]
const USHORT HEAD_LENGTH		= 230;		// 全頭高[mm]
const USHORT HEAD_HEIGHT		= 1500;
///
/// </Settings>
///


const NUI_IMAGE_RESOLUTION CAMERA_RESOLUTION = NUI_IMAGE_RESOLUTION_640x480;

#ifdef NEAR_MODE
const int SENCEING_MIN	= 400;		// 深度画像に表示する最小距離[mm]
const int SENCEING_MAX	= 3000;		// 深度画像に表示する最大距離[mm]
#else
const int SENCEING_MIN	= 800;		// 深度画像に表示する最小距離[mm]
const int SENCEING_MAX	= 4000;		// 深度画像に表示する最大距離[mm]
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