#include "stdafx.h"

#include "DetectHeadFromDepth.h"

DetectHeadFromDepth::DetectHeadFromDepth()
{
}

DetectHeadFromDepth::~DetectHeadFromDepth()
{
	// 終了処理
	if ( kinect != 0 ) {
		kinect->NuiShutdown();
		kinect->Release();
	}
}

void DetectHeadFromDepth::initialize()
{
	createInstance();

	// Kinectの設定を初期化する
	ERROR_CHECK( kinect->NuiInitialize( NUI_INITIALIZE_FLAG_USES_COLOR | NUI_INITIALIZE_FLAG_USES_DEPTH ) );

	// RGBカメラを初期化する
	ERROR_CHECK( kinect->NuiImageStreamOpen( NUI_IMAGE_TYPE_COLOR, CAMERA_RESOLUTION,
		0, 2, 0, &imageStreamHandle ) );

	// 距離カメラを初期化する
	ERROR_CHECK( kinect->NuiImageStreamOpen( NUI_IMAGE_TYPE_DEPTH, CAMERA_RESOLUTION,
		0, 2, 0, &depthStreamHandle ) );

#ifdef NEAR_MODE
	// Nearモード
	ERROR_CHECK( kinect->NuiImageStreamSetImageFrameFlags(
	  depthStreamHandle, NUI_IMAGE_STREAM_FLAG_ENABLE_NEAR_MODE ) );
#endif

	// フレーム更新イベントのハンドルを作成する
	streamEvent = ::CreateEvent( 0, TRUE, FALSE, 0 );
	ERROR_CHECK( kinect->NuiSetFrameEndEvent( streamEvent, 0 ) );

	// 指定した解像度の、画面サイズを取得する
	::NuiImageResolutionToSize(CAMERA_RESOLUTION, width, height );
}

void DetectHeadFromDepth::run()
{
	cv::Mat image;
	cv::Mat depthImage = cv::Mat::zeros(height, width, CV_8UC3);

	// メインループ
	while ( 1 ) {
		// データの更新を待つ
		DWORD ret = ::WaitForSingleObject( streamEvent, INFINITE );
		::ResetEvent( streamEvent );

		drawRgbImage( image );
		drawDepthImage( depthImage );
		// 領域をマージする
		cv::dilate(depthImage, depthImage, cv::Mat(), cv::Point(-1, -1), 3);
		detectEllipce( depthImage );

		// 画像を表示する
		cv::imshow( "Color image", image );
		cv::imshow( "Depth Image", depthImage );

		// 終了のためのキー入力チェック兼、表示のためのウェイト
		int key = cv::waitKey( 10 );
		if ( key == 'q' || key == KEY_ESC ) {
			break;
		}
	}
}


void DetectHeadFromDepth::createInstance()
{
	// 接続されているKinectの数を取得する
	int count = 0;
	ERROR_CHECK( ::NuiGetSensorCount( &count ) );
	if ( count == 0 ) {
		throw std::runtime_error( "Kinect を接続してください" );
	}

	// 最初のKinectのインスタンスを作成する
	ERROR_CHECK( ::NuiCreateSensorByIndex( 0, &kinect ) );

	// Kinectの状態を取得する
	HRESULT status = kinect->NuiStatus();
	if ( status != S_OK ) {
		throw std::runtime_error( "Kinect が利用可能ではありません" );
	}
}

void DetectHeadFromDepth::drawRgbImage( cv::Mat& image )
{
	// RGBカメラのフレームデータを取得する
	NUI_IMAGE_FRAME imageFrame = { 0 };
	ERROR_CHECK( kinect->NuiImageStreamGetNextFrame( imageStreamHandle, INFINITE, &imageFrame ) );

	// 画像データを取得する
	NUI_LOCKED_RECT colorData;
	imageFrame.pFrameTexture->LockRect( 0, &colorData, 0, 0 );

	// 画像データをコピーする
	image = cv::Mat( height, width, CV_8UC4, colorData.pBits );


	// フレームデータを解放する
	ERROR_CHECK( kinect->NuiImageStreamReleaseFrame( imageStreamHandle, &imageFrame ) );
}

void DetectHeadFromDepth::drawDepthImage( cv::Mat& image )
{
	// 距離カメラのフレームデータを取得する
	NUI_IMAGE_FRAME depthFrame = { 0 };
	ERROR_CHECK( kinect->NuiImageStreamGetNextFrame( depthStreamHandle, INFINITE, &depthFrame ) );

	// 距離データを取得する
	NUI_LOCKED_RECT depthData = { 0 };
	depthFrame.pFrameTexture->LockRect( 0, &depthData, 0, 0 );

	USHORT* depth = (USHORT*)depthData.pBits;
	for ( int i = 0; i < (depthData.size / sizeof(USHORT)); ++i ) {
		USHORT distance = ::NuiDepthPixelToDepth( depth[i] );

		LONG depthX = i % width;
		LONG depthY = i / width;

		// 距離を正規化し，画像に格納
		int index = ((depthY * width) + depthX) * 3;
		UCHAR* data = &image.data[index];

		USHORT positionHeight = KINECT_HEIGHT - distance;
		if ( SHOULDER_HEIGHT <= positionHeight ) {// && positionHeight <= SHOULDER_HEIGHT + SHOULDER_LENGTH ) {
			int distanceColor = (int)( (distance - SENCEING_MIN) * 255 / (SENCEING_MAX - SENCEING_MIN) ) * 1;
			data[0] = 0;
			data[1] = 0;
			data[2] = 0;
		//if ( SENCEING_MIN <= distance & distance <= SENCEING_MAX ) {
		//	int distanceColor = (int)( (distance - SENCEING_MIN) * 255 / (SENCEING_MAX - SENCEING_MIN) ) * 1;  // スケールをかけると距離の変化が見やすくする
		//	data[0] = distanceColor % 255;
		//	data[1] = distanceColor % 255;
		//	data[2] = distanceColor % 255;
		}else {
			data[0] = 254;
			data[1] = 254;
			data[2] = 254;
		}
	}

	// フレームデータを解放する
	ERROR_CHECK( kinect->NuiImageStreamReleaseFrame( depthStreamHandle, &depthFrame ) );
}

void DetectHeadFromDepth::detectEllipce( cv::Mat& image )
{
	cv::Mat gray_img, bin_img;
	cv::cvtColor(image, gray_img, CV_BGR2GRAY);

	std::vector<std::vector<cv::Point> > contours;
	// 画像の二値化
	cv::threshold(gray_img, bin_img, 0, 255, cv::THRESH_BINARY|cv::THRESH_OTSU);
	// 輪郭の抽出
	cv::findContours(bin_img, contours, CV_RETR_LIST, CV_CHAIN_APPROX_NONE);

	for(int i=0; i<contours.size(); ++i) {
		size_t count = contours[i].size();
		if(count < 150 || count > 1000) continue;	// (小さすぎる|多きすぎる) 輪郭を除去

		cv::Mat pointsf;
		cv::Mat(contours[i]).convertTo(pointsf, CV_32F);
		// 楕円フィッティング
		cv::RotatedRect box = cv::fitEllipse(pointsf);
		// 楕円の描画
		cv::ellipse(image, box, cv::Scalar(0,69,255), 2, CV_AA);
	}
	
	cv::imshow("bin image", bin_img);
}

void main()
{

	try {
		DetectHeadFromDepth kinect;
		kinect.initialize();
		kinect.run();
	}
	catch ( std::exception& ex ) {
		std::cout << ex.what() << std::endl;
	}
}
