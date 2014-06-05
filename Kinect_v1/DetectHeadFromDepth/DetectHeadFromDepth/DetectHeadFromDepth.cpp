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
	// メインループ
	while ( 1 ) {
		cv::Mat image;
		cv::Mat depthImage				= cv::Mat::zeros(height, width, CV_8UC3);
		cv::Mat depthImageHeadBuf		= cv::Mat::zeros(height, width, CV_8UC3);
		cv::Mat depthImageShoulderBuf	= cv::Mat::zeros(height, width, CV_8UC3);
		cv::Mat heightMat				= cv::Mat::zeros(height, width, CV_16U);
		cv::Mat pointsMat				= cv::Mat::zeros(height, width, CV_32FC3);  // The camera coordinate value of each pixel in the depthImage 

		// データの更新を待つ
		DWORD ret = ::WaitForSingleObject( streamEvent, INFINITE );
		::ResetEvent( streamEvent );

		// 距離画像の作成
		makeDepthImage( depthImage, heightMat, pointsMat );
		// RGB画像の描画
		drawRgbImage( image );
		// 領域をマージする
		cv::dilate(depthImage, depthImage, cv::Mat(), cv::Point(-1, -1), 3);	//　膨張
		//cv::medianBlur(depthImage, depthImage, 3);

		// 矩形を抽出(boxes)
		boxesInit();
		detectEllipce( depthImage, depthImage, TRUE );
		cv::imshow("test", depthImage);

		// ユーザの高さごとに頭と肩領域を検出
		drawDepthImage( heightMat, depthImage, depthImageHeadBuf, depthImageShoulderBuf, pointsMat );
		cv::dilate(depthImageHeadBuf, depthImageHeadBuf, cv::Mat(), cv::Point(-1, -1), 5);	//　膨張
		cv::dilate(depthImageShoulderBuf, depthImageShoulderBuf, cv::Mat(), cv::Point(-1, -1), 5);	//　膨張
		// 頭描画
		drawUserHead( depthImage );
		// 手を検出
		//detectHand ( depthImage, pointsMat );

		// 検出された頭，肩領域に楕円フィッティング
		detectEllipce( depthImageHeadBuf, depthImage, FALSE );
		detectEllipce( depthImageShoulderBuf, depthImage, FALSE );

		// ウィンドウの作成
		cv::namedWindow( COLOR_IMAGE_WINNAME, CV_WINDOW_AUTOSIZE );
		cv::namedWindow( DEPTH_IMAGE_WINNAME, CV_WINDOW_AUTOSIZE );
		//ウインドウへコールバック関数とコールバック関数からイベント情報を受け取る変数を渡す。
		cv::setMouseCallback( COLOR_IMAGE_WINNAME, &mfunc, &mparam );
		// 画像を表示する / Show each images
		cv::imshow( COLOR_IMAGE_WINNAME, image );
		cv::imshow( DEPTH_IMAGE_WINNAME, depthImage );

		//cv::imshow("Head detection", depthImageHeadBuf);

		// 終了のためのキー入力チェック兼、表示のためのウェイト
		int key = cv::waitKey( 10 );
		if ( key == 'q' || key == KEY_ESC ) {
			cv::destroyAllWindows();
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

void DetectHeadFromDepth::makeDepthImage( cv::Mat& depthImage, cv::Mat& heightMat, cv::Mat& pointsMat ) {
	// 距離カメラのフレームデータを取得する
	NUI_IMAGE_FRAME depthFrame = { 0 };
	ERROR_CHECK( kinect->NuiImageStreamGetNextFrame( depthStreamHandle, INFINITE, &depthFrame ) );

	// 距離データを取得する / Get depth data
	NUI_LOCKED_RECT depthData = { 0 };
	depthFrame.pFrameTexture->LockRect( 0, &depthData, 0, 0 );

	//　クリックフラグを立てる / Make a click flag
	BOOL depthCheckFlag = FALSE;
	if ( mparam.event == cv::EVENT_LBUTTONDOWN ) {
		depthCheckFlag = TRUE;
	}

	USHORT* depth = (USHORT*)depthData.pBits;
	for ( int i = 0; i < (depthData.size / sizeof(USHORT)); ++i ) {
		USHORT distance = ::NuiDepthPixelToDepth( depth[i] );

		LONG depthX = i % width;
		LONG depthY = i / width;
			 
		int index = ((depthY * width) + depthX) * 3;
		UCHAR* dataDepth	= &depthImage.data[index];
		USHORT positionHeight;
		distance == 0 ?	positionHeight = 0 : positionHeight = KINECT_HEIGHT - distance;

		// RGB画像がクリックされたときその座標のdepthを調べる
		if ( depthCheckFlag && mparam.x == depthX && mparam.y == depthY ) {
				//cout << "Distance from the ground: " << positionHeight << endl;
				cout << "Distance from the KINECT: " << distance << endl;
		}

		// 高さ情報を記録
		heightMat.at<USHORT>(depthY, depthX) = positionHeight;

		// ユーザ領域を記憶
		if ( USER_DETECT_HEIGHT <= positionHeight && positionHeight <= HEAD_HEIGHT_MAX ) {
			dataDepth[0] = 255;
			dataDepth[1] = 255;
			dataDepth[2] = 255;
		}
		else {
			dataDepth[0] = 0;
			dataDepth[1] = 0;
			dataDepth[2] = 0;
		}

		// ポイントクラウドを記憶
		Vector4 realPoint = NuiTransformDepthImageToSkeleton(depthX, depthY, distance, CAMERA_RESOLUTION );
		pointsMat.at<cv::Vec3f>(depthY,depthX)[0] = realPoint.x;
		pointsMat.at<cv::Vec3f>(depthY,depthX)[1] = realPoint.y;
		pointsMat.at<cv::Vec3f>(depthY,depthX)[2] = realPoint.z;
	}

	// フレームデータを解放する
	ERROR_CHECK( kinect->NuiImageStreamReleaseFrame( depthStreamHandle, &depthFrame ) );
}

//void DetectHeadFromDepth::makeCameraCoordinateImage( cv::Mat& heightMat, cv::Mat& cameraCoordinateImage ) {
//	cv::Mat debugPoint;
//	cv::Mat debugPointImage = cv::Mat::ones(3,1,CV_32FC3);
//	for ( int h=0; h<height; h++ ) {
//		for ( int w=0; w<width; w++ ) {
//			cv::Mat imagePoint = ( cv::Mat_<float>(3,1) << w, h, 1 ) * (float)heightMat.at<USHORT>(h,w);
//			cv::Mat cameraPoint = KINECT_IN_PARAM.inv() * imagePoint;
//			cameraCoordinateImage.at<cv::Vec3b>(h,w)[0] = cameraPoint.at<float>(0,0);
//			cameraCoordinateImage.at<cv::Vec3b>(h,w)[1] = cameraPoint.at<float>(1,0);
//			cameraCoordinateImage.at<cv::Vec3b>(h,w)[2] = cameraPoint.at<float>(2,0);
//
//			//debugPointImage.at<float>(0,0) = cameraCoordinateImage.at<cv::Vec3b>(h,w)[0];
//			//debugPointImage.at<float>(1,0) = cameraCoordinateImage.at<cv::Vec3b>(h,w)[1];
//			//debugPointImage.at<float>(2,0) = cameraCoordinateImage.at<cv::Vec3b>(h,w)[2];
//			//debugPoint = KINECT_IN_PARAM * debugPointImage;
//			//debugPoint /= debugPoint.at<float>(2,0);
//			//cout << debugPoint << endl;
//
//		}
//	}
//}

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

// 簡単な初期化
void DetectHeadFromDepth::boxesInit() {
	for ( int i = 0; i < USER_NUM_MAX; i++ ) {
		boxes[i].height = 0;
		boxes[i].width  = 0;
	}
}

void DetectHeadFromDepth::detectEllipce( cv::Mat& srcImg, cv::Mat& dstImg, BOOL param )
{
	cv::Mat gray_img, bin_img;
	cv::cvtColor(srcImg, gray_img, CV_BGR2GRAY);

	std::vector<std::vector<cv::Point> > contours;
	// 画像の二値化
	cv::threshold(gray_img, bin_img, 0, 255, cv::THRESH_BINARY|cv::THRESH_OTSU);
	// 輪郭の抽出
	cv::findContours(bin_img, contours, CV_RETR_LIST, CV_CHAIN_APPROX_NONE);

	int num = 0;
	for(int i=0; i<contours.size(); ++i) {
		size_t count = contours[i].size();
		if(count < 500 || count > 1500) continue;	// (小さすぎる|多きすぎる) 輪郭を除去

		cv::Mat pointsf;
		cv::Mat(contours[i]).convertTo(pointsf, CV_32F);
		// 楕円フィッティング
		cv::RotatedRect box = cv::fitEllipse(pointsf);
		// 楕円の描画
		cv::ellipse(dstImg, box, cv::Scalar(0,69,255), 2, CV_AA);

		// paramを用いてユーザ検出か、頭肩検出かを分離
		if ( param && i < USER_NUM_MAX ) {
			BOOL isResisteredUser = FALSE;
			for ( int m = 0; m < num; m++ ){
				if ( boxes[m].contains(cv::Point(box.center.x, box.center.y)) ) {
					isResisteredUser = TRUE;
				}
			}
			if ( !isResisteredUser ) {
				boxes[num] = box.boundingRect();
				num++;
			}
		}
	}
	//cv::imshow("bin image", bin_img);
}

void DetectHeadFromDepth::drawDepthImage( cv::Mat& heightMat, cv::Mat& depthImage, cv::Mat& headImage, cv::Mat& shoulderImage, cv::Mat& pointsMat )
{
	//USHORT headHeight[USER_NUM_MAX];


	// 矩形ごとにもっとも高い位置(ユーザの頭)を探索
	for ( int i = 0; i < USER_NUM_MAX; i++ ) {
		if ( boxes[i].height == 0 || boxes[i].width == 0 ) {
			break;
		}
		// Init
		userHeads[i].headHeight = 0;
		userHeads[i].headX2d = 0;
		userHeads[i].headY2d = 0;

		for ( int y = boxes[i].y; y < boxes[i].y + boxes[i].height; y++ ) {
			for ( int x = boxes[i].x; x < boxes[i].x + boxes[i].width; x++ ) {
				if ( 0 <= x && x < width && 0 <= y && y < height ) {
					if ( heightMat.at<USHORT>(y, x) > userHeads[i].headHeight && heightMat.at<USHORT>(y,x) < HEAD_HEIGHT_MAX ) {
						userHeads[i].headHeight = heightMat.at<USHORT>(y, x);
						userHeads[i].headX2d = x;
						userHeads[i].headY2d = y;
						userHeads[i].headX   = pointsMat.at<cv::Vec3f>(y,x)[0];
						userHeads[i].headY   = pointsMat.at<cv::Vec3f>(y,x)[1];
						userHeads[i].headZ   = pointsMat.at<cv::Vec3f>(y,x)[2];
					}
				}
			}
		}
		// ユーザの範囲をOFSETだけ拡大
		const int OFSET = 150;	// 探索オフセット[pixel]
		if ( boxes[i].x <= OFSET ) {
			boxes[i].x		 = 0;
			boxes[i].width  += boxes[i].x + OFSET;
		}
		else {
			boxes[i].x		-= OFSET;
			boxes[i].width	+= 2 * OFSET;
		}
		if ( boxes[i].y <= OFSET )
		{
			boxes[i].y		 = 0;
			boxes[i].height += boxes[i].y + OFSET;
		}
		else {
			boxes[i].y		-= OFSET;
			boxes[i].height	+= 2 * OFSET;
		}
	}

	///
	/// 頭と肩を描画
	///

	for ( int y = 0; y < heightMat.size().height; y++ ) {
		for ( int x = 0; x < heightMat.size().width; x++ ) {
			BOOL isUserArea = FALSE;
			int index = ((y * width) + x) * 3;
			UCHAR* data			= &depthImage.data[index];
			UCHAR* dataShoulder = &shoulderImage.data[index];
			UCHAR* dataHead		= &headImage.data[index];

			for ( int i = 0; i < USER_NUM_MAX; i++ ) {
				if ( boxes[i].height != 0 && boxes[i].width != 0 && boxes[i].contains(cv::Point(x,y)) ) {
					// 肩を描画
					//if ( headHeight[i] - (HEAD_LENGTH + SHOULDER_LENGTH) <= heightMat.at<USHORT>(y, x) && heightMat.at<USHORT>(y, x) <= headHeight[i] - HEAD_LENGTH ){		// 頭を含まず楕円マッチング
					if ( userHeads[i].headHeight - (HEAD_LENGTH + SHOULDER_LENGTH) <= heightMat.at<USHORT>(y, x) && heightMat.at<USHORT>(y, x) <= HEAD_HEIGHT_MAX){		// 頭を含んで楕円マッチング
						data[0] = 255;
						data[1] = 100;
						data[2] = 0;
						dataShoulder[0] = 255;
						dataShoulder[1] = 255;
						dataShoulder[2] = 255;
					}else {
						data[0] = 0;
						data[1] = 0;
						data[2] = 0;
						dataShoulder[0] = 0;
						dataShoulder[1] = 0;
						dataShoulder[2] = 0;
					} 
					// 頭を描画
					if ( userHeads[i].headHeight - HEAD_LENGTH <= heightMat.at<USHORT>(y, x) && heightMat.at<USHORT>(y, x) <= HEAD_HEIGHT_MAX ){
						data[0] = 0;
						data[1] = 180;
						data[2] = 255;
						dataHead[0] = 255;
						dataHead[1] = 255;
						dataHead[2] = 255;
					}else {
						dataHead[0] = 0;
						dataHead[1] = 0;
						dataHead[2] = 0;
					}
					isUserArea = TRUE;
					break;
				}
			}
			if ( !isUserArea ) {
				data[0] = 0;
				data[1] = 0;
				data[2] = 0;
			}
		}
	}

	//cv::rectangle( depthImage, boxes[0], 255, 2, 8, 0);	// debug
}

void DetectHeadFromDepth::drawUserHead ( cv::Mat& depthImage ) {
	for ( int i=0; i<USER_NUM_MAX; i++) {
		if ( userHeads[i].headHeight > 0 ) {
			cv::circle(depthImage, cv::Point(userHeads[i].headX2d, userHeads[i].headY2d), 8, CV_RGB(0,255,0), 3, CV_AA);
		}
		else {
			break;
		}
	}
}

void DetectHeadFromDepth::detectHand( cv::Mat& depthImage, cv::Mat& pointsMat ) {
	Vector4 handPoints[USER_NUM_MAX];	// 円に重なった部分の平均値
	int countIntersection = 0;
	for ( int num = 0; num < USER_NUM_MAX ; num++ ) {
		// Init
		handPoints[num].w = 1;
		handPoints[num].x = 0;
		handPoints[num].y = 0;
		handPoints[num].z = 0;

		if ( userHeads[num].headHeight > 0 && userHeads[num].headX2d > 0 && userHeads[num].headY2d > 0 ) {
			for ( int h = 0; h < height; h++ ) {
				for ( int w = 0; w < width; w++ ) {
					//cout << userHeads[num].headHeight << endl;
					//FLOAT length = sqrt( pow(userHeads[num].headX, 2) + pow(userHeads[num].headY , 2) + pow(userHeads[num].headZ, 2) );
					FLOAT length = sqrt( pow(userHeads[num].headX - pointsMat.at<cv::Vec3f>(h,w)[0], 2) + pow(userHeads[num].headY - pointsMat.at<cv::Vec3f>(h,w)[1], 2) + pow(userHeads[num].headZ - pointsMat.at<cv::Vec3f>(h,w)[2], 2) );
					if ( HAND_CIRCLE_RADIUS - 20 < length && length < HAND_CIRCLE_RADIUS + 20 ) { // 球上に点があるときの処理
						cout << w << ", " << h <<endl;
						handPoints[num].x += pointsMat.at<cv::Vec3f>(h,w)[0];
						handPoints[num].y += pointsMat.at<cv::Vec3f>(h,w)[1];
						handPoints[num].z += pointsMat.at<cv::Vec3f>(h,w)[2];
						countIntersection++;
						//cout << countIntersection << endl;
					}
				}
			}
			if ( countIntersection > 0 ) {
				handPoints[num].x /= countIntersection;
				handPoints[num].y /= countIntersection;
				handPoints[num].z /= countIntersection;
				LONG handPointX;
				LONG handPointY;
				USHORT dis; // debug
				NuiTransformSkeletonToDepthImage( handPoints[num], &handPointX, &handPointY, &dis );
				cv::circle(depthImage, cv::Point(handPointX, handPointY), 8, CV_RGB(0,0,255), 3, CV_AA);
			}
		}
		else {
			break;
		}
	}
}

///
/// コールバック関数
///
void mfunc(int event, int x, int y, int flags, void  *param){

	// default
	MouseParam *mParam = (MouseParam*)param;
	mParam->x = x;
	mParam->y = y;
	mParam->event = event;
	mParam->flags = flags;

}

void main()
{

	try {
		DetectHeadFromDepth kinect;
		kinect.initialize();
		kinect.run();
		exit(0);
	}
	catch ( std::exception& ex ) {
		std::cout << ex.what() << std::endl;
	}
}
