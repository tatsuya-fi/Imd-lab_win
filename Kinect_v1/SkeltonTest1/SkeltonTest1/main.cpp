//
//	KinectからSkeltonを検出して右手と頭を結んだ直線がディスプレイに
//	交わる部分にカーソルを表示する．
//	Kinectとディスプレイの座標系のキャリブレーションは手動で行う．
//


#include "skeltonTest1.h"


KinectSample::KinectSample()
{
}

KinectSample::~KinectSample()
{
	// 終了処理
	if (kinect != 0) {
		kinect->NuiShutdown();
		kinect->Release();
	}
}

void KinectSample::initialize()
{
	createInstance();

	// Kinectの設定を初期化する
	ERROR_CHECK(kinect->NuiInitialize(NUI_INITIALIZE_FLAG_USES_COLOR | NUI_INITIALIZE_FLAG_USES_DEPTH_AND_PLAYER_INDEX | NUI_INITIALIZE_FLAG_USES_SKELETON));

	// RGBカメラを初期化する
	ERROR_CHECK(kinect->NuiImageStreamOpen(NUI_IMAGE_TYPE_COLOR, CAMERA_RESOLUTION,
		0, 2, 0, &imageStreamHandle));

	// 距離カメラを初期化する
	ERROR_CHECK(kinect->NuiImageStreamOpen(NUI_IMAGE_TYPE_DEPTH_AND_PLAYER_INDEX, CAMERA_RESOLUTION,
		0, 2, 0, &depthStreamHandle));

	// Nearモード
	//ERROR_CHECK( kinect->NuiImageStreamSetImageFrameFlags(
	//  depthStreamHandle, NUI_IMAGE_STREAM_FLAG_ENABLE_NEAR_MODE ) );

	// スケルトンを初期化する
	ERROR_CHECK(kinect->NuiSkeletonTrackingEnable(0, NUI_SKELETON_TRACKING_FLAG_ENABLE_SEATED_SUPPORT));

	// フレーム更新イベントのハンドルを作成する
	streamEvent = ::CreateEvent(0, TRUE, FALSE, 0);
	ERROR_CHECK(kinect->NuiSetFrameEndEvent(streamEvent, 0));

	// 指定した解像度の、画面サイズを取得する
	::NuiImageResolutionToSize(CAMERA_RESOLUTION, width, height);
}

void KinectSample::run()
{
	cv::Mat image;
	

	// メインループ
	while (1) {
		// カーソルを保存するフラグ
		saveFlag = false;

		// キー入力チェック兼、表示のためのウェイト
		int key = cv::waitKey(10);
		switch (key)
		{
		case 0x20:	// Space
			saveFlag = true;
			break;
		default:
			break;
		}
		// 終了チェック
		if (key == 'q' || key == 0x1b) {	// q or Esc
			break;
		}

		// データの更新を待つ
		DWORD ret = ::WaitForSingleObject(streamEvent, INFINITE);
		::ResetEvent(streamEvent);

		drawRgbImage(image);
		drawDepthImage(image);
		drawSkeleton(image);

		// 画像を表示する
		cv::namedWindow("SkeltonTest1", CV_WINDOW_FREERATIO);
		cv::imshow("SkeltonTest1", image);

	}
}

void KinectSample::createInstance()
{
	// 接続されているKinectの数を取得する
	int count = 0;
	ERROR_CHECK(::NuiGetSensorCount(&count));
	if (count == 0) {
		throw std::runtime_error("Kinect を接続してください");
	}

	// 最初のKinectのインスタンスを作成する
	ERROR_CHECK(::NuiCreateSensorByIndex(0, &kinect));

	// Kinectの状態を取得する
	HRESULT status = kinect->NuiStatus();
	if (status != S_OK) {
		throw std::runtime_error("Kinect が利用可能ではありません");
	}
}

void KinectSample::drawRgbImage(cv::Mat& image)
{
	// RGBカメラのフレームデータを取得する
	NUI_IMAGE_FRAME imageFrame = { 0 };
	ERROR_CHECK(kinect->NuiImageStreamGetNextFrame(imageStreamHandle, INFINITE, &imageFrame));

	// 画像データを取得する
	NUI_LOCKED_RECT colorData;
	imageFrame.pFrameTexture->LockRect(0, &colorData, 0, 0);

	// 画像データをコピーする
	image = cv::Mat(height, width, CV_8UC4, colorData.pBits);

	// フレームデータを解放する
	ERROR_CHECK(kinect->NuiImageStreamReleaseFrame(imageStreamHandle, &imageFrame));
}

void KinectSample::drawDepthImage(cv::Mat& image)
{
	// 距離カメラのフレームデータを取得する
	NUI_IMAGE_FRAME depthFrame = { 0 };
	ERROR_CHECK(kinect->NuiImageStreamGetNextFrame(depthStreamHandle, 0, &depthFrame));

	// 距離データを取得する
	NUI_LOCKED_RECT depthData = { 0 };
	depthFrame.pFrameTexture->LockRect(0, &depthData, 0, 0);
	if (depthData.Pitch == 0) {
		std::cout << "zero" << std::endl;
	}

	USHORT* depth = (USHORT*)depthData.pBits;
	for (int i = 0; i < (depthData.size / sizeof(USHORT)); ++i) {
		USHORT distance = ::NuiDepthPixelToDepth(depth[i]);
		USHORT player = ::NuiDepthPixelToPlayerIndex(depth[i]);

		LONG depthX = i % width;
		LONG depthY = i / width;
		LONG colorX = depthX;
		LONG colorY = depthY;

		// 距離カメラの座標を、RGBカメラの座標に変換する
		kinect->NuiImageGetColorPixelCoordinatesFromDepthPixelAtResolution(
			CAMERA_RESOLUTION, CAMERA_RESOLUTION,
			0, depthX, depthY, 0, &colorX, &colorY);

		// プレイヤー
		//if ( player != 0 ) {
		//  int index = ((colorY * width) + colorX) * 4;
		//  UCHAR* data = &image.data[index];
		//  data[0] = 0;
		//  data[1] = 0;
		//  data[2] = 255;
		//}
		//else {
		//  // 一定以上の距離を描画しない
		//  if ( distance >= 1000 ) {
		//    int index = ((colorY * width) + colorX) * 4;
		//    UCHAR* data = &image.data[index];
		//    data[0] = 255;
		//    data[1] = 255;
		//    data[2] = 255;
		//  }
		//}
	}

	// フレームデータを解放する
	ERROR_CHECK(kinect->NuiImageStreamReleaseFrame(depthStreamHandle, &depthFrame));
}

void KinectSample::drawSkeleton(cv::Mat& image)
{
	cv::Mat disp = cv::Mat::zeros(1050 - 80, 1680 - 16, CV_8UC4);

	// スケルトンのフレームを取得する
	NUI_SKELETON_FRAME skeletonFrame = { 0 };
	kinect->NuiSkeletonGetNextFrame(0, &skeletonFrame);
	//ERROR_CHECK( kinect->NuiSkeletonGetNextFrame( 0, &skeletonFrame ) );
	for (int i = 0; i < NUI_SKELETON_COUNT; ++i) {
		NUI_SKELETON_DATA& skeletonData = skeletonFrame.SkeletonData[i];
		if (skeletonData.eTrackingState == NUI_SKELETON_TRACKED) {
			for (int j = 0; j < NUI_SKELETON_POSITION_COUNT; ++j) {
				if (skeletonData.eSkeletonPositionTrackingState[j] != NUI_SKELETON_POSITION_NOT_TRACKED) {
					drawJoint(image, skeletonData.SkeletonPositions[j]);
				}
			}
			// ディスプレイ上にカーソルを表示
			if (skeletonData.eSkeletonPositionTrackingState[NUI_SKELETON_POSITION_HAND_LEFT] != NUI_SKELETON_POSITION_NOT_TRACKED
				&& skeletonData.eSkeletonPositionTrackingState[NUI_SKELETON_POSITION_HEAD] != NUI_SKELETON_POSITION_NOT_TRACKED) {
				drawCursor(disp, skeletonData.SkeletonPositions[NUI_SKELETON_POSITION_HEAD], skeletonData.SkeletonPositions[NUI_SKELETON_POSITION_HAND_LEFT]);
			}
		}
		else if (skeletonData.eTrackingState == NUI_SKELETON_POSITION_ONLY) {	// ジョイントの座標が追跡されていない状態
			drawJoint(image, skeletonData.Position);
		}
	}

	cv::imshow("cursor", disp);
}

void KinectSample::drawJoint(cv::Mat& image, Vector4 position)
{
	FLOAT depthX = 0, depthY = 0;
	::NuiTransformSkeletonToDepthImage(position, &depthX, &depthY, CAMERA_RESOLUTION);
	//cout << depthX << ", " << depthY << endl;

	LONG colorX = 0, colorY = 0;

	kinect->NuiImageGetColorPixelCoordinatesFromDepthPixelAtResolution(
		CAMERA_RESOLUTION, CAMERA_RESOLUTION,
		0, (LONG)depthX, (LONG)depthY, 0, &colorX, &colorY);

	// cout << depthX << ", " << depthY << endl;

	cv::circle(image, cv::Point(colorX, colorY), 10, cv::Scalar(0, 255, 0), 5);
	
}

void KinectSample::drawCursor(cv::Mat& image, Vector4 head, Vector4 hand)
{
	// 手と頭の座標をdisp3d座標に変換
	//cv::Mat headCameraMat = (cv::Mat_<float>(4, 1) << -head.x+0.2, -head.y-0.1, head.z, head.w);
	//cv::Mat handCameraMat = (cv::Mat_<float>(4, 1) << -hand.x+0.2, -hand.y-0.1, hand.z, hand.w);
	cv::Mat headCameraMat = (cv::Mat_<float>(4, 1) << -head.x, -head.y, head.z, head.w);
	cv::Mat handCameraMat = (cv::Mat_<float>(4, 1) << -hand.x, -hand.y, hand.z, hand.w);


	// 座標変換
	cv::Mat headDisp3dMat = disp3dToPreCameraTransMat.inv() * cameraToPreCameraTransMat * headCameraMat;
	cv::Mat handDisp3dMat = disp3dToPreCameraTransMat.inv() * cameraToPreCameraTransMat * handCameraMat;

	//cout << "head" << headDisp3dMat << endl;
	//cout << "hand" << handDisp3dMat << endl;
	// ここまでは合ってる

	//////
	/// test
	/// 直接x,y座標を平面の座標に用いる

	//int handx = handDisp3dMat.at<float>(0, 0) * disp3dTodisp2d;
	//int handy = handDisp3dMat.at<float>(1, 0) * disp3dTodisp2d;

	//cv::circle(image, cv::Point(handx, handy), 10, cv::Scalar(255, 0, 255), -1);
	//cout << "(handx, handy) = (" << handx << ", " << handy << ")" << endl;

	//////

	FLOAT xvec = handDisp3dMat.at<float>(0, 0) - headDisp3dMat.at<float>(0, 0);
	FLOAT yvec = handDisp3dMat.at<float>(1, 0) - headDisp3dMat.at<float>(1, 0);
	FLOAT zvec = handDisp3dMat.at<float>(2, 0) - headDisp3dMat.at<float>(2, 0);

	FLOAT a = -handDisp3dMat.at<float>(2, 0) / zvec;		// z=0(ディスプレイと上記のベクトルが交わる点)

	Vector4 cursorDisp3d;
	cursorDisp3d.w = 1;
	cursorDisp3d.x = a * xvec + headDisp3dMat.at<float>(0, 0);
	cursorDisp3d.y = a * yvec + headDisp3dMat.at<float>(1, 0);
	cursorDisp3d.z = 0;

	// disp2d座標系に変換
	LONG colorX = (LONG)(cursorDisp3d.x * disp3dTodisp2d);
	LONG colorY = (LONG)(cursorDisp3d.y * disp3dTodisp2d);

	///
	//FLOAT vectorLength = sqrt(pow(xvec, 2) + pow(yvec, 2) + pow(zvec, 2));	// handからheadへのベクトルの長さ
	//FLOAT a = -handDisp3dMat.at<float>(2,0) * vectorLength / zvec;		// z=0(ディスプレイと上記のベクトルが交わる点)

	//Vector4 cursorDisp3d;
	//cursorDisp3d.w = 1;
	//cursorDisp3d.x = a * xvec / vectorLength + handDisp3dMat.at<float>(0,0);
	//cursorDisp3d.y = a * yvec / vectorLength + handDisp3dMat.at<float>(1,0);
	//cursorDisp3d.z = handDisp3dMat.at<float>(2,0);

	//// disp2d座法系に変換
	//LONG colorX = (LONG)(cursorDisp3d.x * disp3dTodisp2d);
	//LONG colorY = (LONG)(cursorDisp3d.y * disp3dTodisp2d);


	////////////////////

	//Vector4 cursorPosition;	// スクリーン上のカーソルの位置
	//cursorPosition.w = 1;
	//cursorPosition.x = a * xvec / vectorLength + hand.x;
	//cursorPosition.y = a * yvec / vectorLength + hand.y;
	//cursorPosition.z = hand.z;
	//// cursorPosition.z = a * zvec / vectorLength + hand.z;
	//
	//FLOAT depthX = 0, depthY = 0;
	//::NuiTransformSkeletonToDepthImage(cursorPosition, &depthX, &depthY, CAMERA_RESOLUTION);
	//
	////cout << cursorPosition.x << ", " << cursorPosition.y << ", " << cursorPosition.z << endl;
	//LONG colorX = 0, colorY = 0;
	//kinect->NuiImageGetColorPixelCoordinatesFromDepthPixelAtResolution(
	//	CAMERA_RESOLUTION, CAMERA_RESOLUTION,
	//	0, (LONG)depthX, (LONG)depthY, 0, &colorX, &colorY);

	
	//cout << "(colorX, colorY) = (" << colorX << ", " << colorY << ")" << endl;


	// カーソルの大きさを求める
	float scale = 2.5;
	float distance = 1/sqrt(pow(head.x - hand.x, 2) + pow(head.y - hand.y, 2) + pow(head.z - hand.z, 2)) * pow(scale, 4);
	cout << distance << endl;

	cv::circle(image, cv::Point(colorX, colorY), distance, cv::Scalar(0, 0, 255), -1);


	// カーソルを保存
	if (saveFlag == true)
	{
		saveCursor(colorX, colorY);
	}
	// 保存したカーソルを描画
	INT i = 0;
	while (savedCursor[i].x != 0 && savedCursor[i].y != 0) {
		cv::circle(image, cv::Point(savedCursor[i].x, savedCursor[i].y), 10, cv::Scalar(255, 0, 0), -1);
		i++;
	}

}

void KinectSample::saveCursor(INT x, INT y)
{
	INT i = 0;

	while (savedCursor[i].x != 0 && savedCursor[i].y != 0) {
		i++;
	}
	savedCursor[i].x = x;
	savedCursor[i].y = y;
	cout << i << endl;
}

void main()
{

  try {
    KinectSample kinect;
    kinect.initialize();
    kinect.run();
  }
  catch ( std::exception& ex ) {
    std::cout << ex.what() << std::endl;
  }
}
