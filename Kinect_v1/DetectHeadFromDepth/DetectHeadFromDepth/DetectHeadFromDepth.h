#pragma once

/// <Settings>
/// 調整可能なパラメータ

#define NEAR_MODE	// nearモードを使う場合はコメントを外す
const NUI_IMAGE_RESOLUTION CAMERA_RESOLUTION = NUI_IMAGE_RESOLUTION_640x480;

#ifdef NEAR_MODE
const int SENCEING_MIN	= 400;		// 深度画像に表示する最小距離[mm]
const int SENCEING_MAX	= 3000;		// 深度画像に表示する最大距離[mm]
#else
const int SENCEING_MIN	= 800;		// 深度画像に表示する最小距離[mm]
const int SENCEING_MAX	= 4000;		// 深度画像に表示する最大距離[mm]
#endif

const USHORT KINECT_HEIGHT		= 1500;		// 設置したKinectの高さ
const USHORT SHOULDER_LENGTH	= 280;		// 肩の高さ幅[mm]
const USHORT SHOULDER_HEIGHT	= 500;		// 肩までの高さ[mm]
//const int HEAD_LENGTH		= 280;		// 全頭高[mm]
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