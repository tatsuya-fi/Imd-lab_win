#include "DepthMat.h"

DepthMat::DepthMat()
{
}


DepthMat::~DepthMat()
{
}

void DepthMat::setImageSize(NUI_IMAGE_RESOLUTION cameraResolution)
{
	::NuiImageResolutionToSize(cameraResolution, width, height);
}

void DepthMat::setINuiSensor(INuiSensor *handle)
{
	if (handle != NULL) {
		kinect = handle;
	}
	else {
		std::cout << "INuiSensor is not found." << std::endl;
	}
}

void DepthMat::setRgbImage(cv::Mat& image, NUI_IMAGE_FRAME& imageFrame)
{

	//// RGBカメラのフレームデータを取得する
	//NUI_IMAGE_FRAME imageFrame = { 0 };
	//ERROR_CHECK(kinect->NuiImageStreamGetNextFrame(imageStreamHandle, 0, &imageFrame));

	// 画像データを取得する
	NUI_LOCKED_RECT colorData;
	imageFrame.pFrameTexture->LockRect(0, &colorData, 0, 0);

	// 画像データをコピーする
	//cv::Mat imageBuf = cv::Mat(height, width, CV_8UC4, colorData.pBits);
	//image = imageBuf.clone();
	image = cv::Mat(height, width, CV_8UC4, colorData.pBits);

	 cv::flip(image, image, 1); // 垂直軸に対して反転


}

void DepthMat::getCameraImgToDispTransform(const char *windowname, cv::Mat& cameraToDispTransform)
{
	
}
	
int DepthMat::setMat(Matrix4& transMat, int matNum)
{
	cv::Mat mat;

	if (matNum == 0)
		mat = cameraToWorldTransMat0;
	else if (matNum == 1)
		mat = cameraToWorldTransMat1;
	else
		return -1;

	// 変換行列をMatに格納（転置する）
	mat.at<float>(0, 0) = transMat.M11;		mat.at<float>(0, 1) = transMat.M21;		mat.at<float>(0, 2) = transMat.M31;		mat.at<float>(0, 3) = transMat.M41;
	mat.at<float>(1, 0) = transMat.M12;		mat.at<float>(1, 1) = transMat.M22;		mat.at<float>(1, 2) = transMat.M32;		mat.at<float>(1, 3) = transMat.M42;
	mat.at<float>(2, 0) = transMat.M13;		mat.at<float>(2, 1) = transMat.M23;		mat.at<float>(2, 2) = transMat.M33;		mat.at<float>(2, 3) = transMat.M43;
	mat.at<float>(3, 0) = transMat.M14;		mat.at<float>(3, 1) = transMat.M24;		mat.at<float>(3, 2) = transMat.M34;		mat.at<float>(3, 3) = transMat.M44;

	// camera to world へ変換
	mat = mat.inv();

	// CHECK OK
	//std::ofstream out("mat.txt");
	//out << mat << std::endl;
}

void DepthMat::caliculateCamToPrecamTransMat()
{
	cv::Mat cameraToPreCameraTransMat = cameraToWorldTransMat0.inv() * cameraToWorldTransMat1;

	std::ofstream ofs("cameraToPreCameraMat.txt");
	ofs << cameraToPreCameraTransMat << std::endl;
}

/// <input>  Matrix4& camTrans     ワールド座標系to現在のカメラ座標系の変換行列
/// <output> Vector4& preCamPoint  現在のカメラ座標系での前のカメラの位置
void DepthMat::getPreCam3dPoint(Matrix4& camTrans, Vector4& preCamPoint)
{
	cv::Mat camTransMat = (cv::Mat_<float>(4, 4) <<
		camTrans.M11, camTrans.M21, camTrans.M31, camTrans.M41,
		camTrans.M12, camTrans.M22, camTrans.M32, camTrans.M42,
		camTrans.M13, camTrans.M23, camTrans.M33, camTrans.M43,
		camTrans.M14, camTrans.M24, camTrans.M34, camTrans.M44);
	cv::Mat preCamZero = cv::Mat::zeros(4, 1, CV_32F);
	preCamZero.at<float>(3, 0) = 1;

	cv::Mat preCamPointMat = camTransMat * cameraToWorldTransMat0 * preCamZero;

	preCamPoint.x = preCamPointMat.at<float>(0, 0);
	preCamPoint.y = preCamPointMat.at<float>(1, 0);
	preCamPoint.z = preCamPointMat.at<float>(2, 0);
	preCamPoint.w = preCamPointMat.at<float>(3, 0);

	// CHECK OK
	//std::ofstream debug("debug.txt", std::ios::app);
	//debug << preCamPointMat << std::endl;

}

//void DepthMat::drawPreCam(cv::Mat& image, Vector4 position)
//{
//	FLOAT depthX = 0, depthY = 0;
//	::NuiTransformSkeletonToDepthImage(position, &depthX, &depthY, NUI_IMAGE_RESOLUTION_640x480);
//
//	LONG colorX = 0, colorY = 0;
//
//	kinect->NuiImageGetColorPixelCoordinatesFromDepthPixelAtResolution(
//		NUI_IMAGE_RESOLUTION_640x480, NUI_IMAGE_RESOLUTION_640x480,
//		0, (LONG)depthX, (LONG)depthY, 0, &colorX, &colorY);
//
//
//	cv::circle(image, cv::Point(width-colorX, colorY), 10, cv::Scalar(255, 0, 0), 5);
//
//		std::ofstream debug("debug.txt", std::ios::app);
//		debug << cv::Point(colorX, colorY) << std::endl;
//}

void DepthMat::drawPreCam(cv::Mat& image, Vector4 position)
{
	cv::Mat positionMat = (cv::Mat_<float>(4, 1) << position.x, position.y, position.z, position.w);
	// 画像座標系に変換
	cv::Mat position2dMat = internalParam * positionMat;
	position2dMat = position2dMat / position2dMat.at<float>(2, 0);

	LONG colorX = (LONG)position2dMat.at<float>(0, 0);
	LONG colorY = (LONG)position2dMat.at<float>(1, 0);

	// (注)Kinectの画像座標はx軸が反転しているのでそれを考慮して表示すること
	cv::circle(image, cv::Point(width - colorX, colorY), 10, cv::Scalar(255, 0, 0), 5);

	std::ofstream debug("debug.txt", std::ios::app);
	debug << position2dMat << std::endl;

}

