#include "stdafx.h"

#include "DetectHeadFromDepth.h"

DetectHeadFromDepth::DetectHeadFromDepth()
{
}

DetectHeadFromDepth::~DetectHeadFromDepth()
{
	// �I������
	if ( kinect != 0 ) {
		kinect->NuiShutdown();
		kinect->Release();
	}
}

void DetectHeadFromDepth::initialize()
{
	createInstance();

	// Kinect�̐ݒ������������
	ERROR_CHECK( kinect->NuiInitialize( NUI_INITIALIZE_FLAG_USES_COLOR | NUI_INITIALIZE_FLAG_USES_DEPTH ) );

	// RGB�J����������������
	ERROR_CHECK( kinect->NuiImageStreamOpen( NUI_IMAGE_TYPE_COLOR, CAMERA_RESOLUTION,
		0, 2, 0, &imageStreamHandle ) );

	// �����J����������������
	ERROR_CHECK( kinect->NuiImageStreamOpen( NUI_IMAGE_TYPE_DEPTH, CAMERA_RESOLUTION,
		0, 2, 0, &depthStreamHandle ) );

#ifdef NEAR_MODE
	// Near���[�h
	ERROR_CHECK( kinect->NuiImageStreamSetImageFrameFlags(
	  depthStreamHandle, NUI_IMAGE_STREAM_FLAG_ENABLE_NEAR_MODE ) );
#endif

	// �t���[���X�V�C�x���g�̃n���h�����쐬����
	streamEvent = ::CreateEvent( 0, TRUE, FALSE, 0 );
	ERROR_CHECK( kinect->NuiSetFrameEndEvent( streamEvent, 0 ) );

	// �w�肵���𑜓x�́A��ʃT�C�Y���擾����
	::NuiImageResolutionToSize(CAMERA_RESOLUTION, width, height );
}

void DetectHeadFromDepth::run()
{
	cv::Mat image;
	cv::Mat depthImage = cv::Mat::zeros(height, width, CV_8UC3);

	// ���C�����[�v
	while ( 1 ) {
		// �f�[�^�̍X�V��҂�
		DWORD ret = ::WaitForSingleObject( streamEvent, INFINITE );
		::ResetEvent( streamEvent );

		drawRgbImage( image );
		drawDepthImage( depthImage );
		// �̈���}�[�W����
		cv::dilate(depthImage, depthImage, cv::Mat(), cv::Point(-1, -1), 3);
		detectEllipce( depthImage );

		// �摜��\������
		cv::imshow( "Color image", image );
		cv::imshow( "Depth Image", depthImage );

		// �I���̂��߂̃L�[���̓`�F�b�N���A�\���̂��߂̃E�F�C�g
		int key = cv::waitKey( 10 );
		if ( key == 'q' || key == KEY_ESC ) {
			break;
		}
	}
}


void DetectHeadFromDepth::createInstance()
{
	// �ڑ�����Ă���Kinect�̐����擾����
	int count = 0;
	ERROR_CHECK( ::NuiGetSensorCount( &count ) );
	if ( count == 0 ) {
		throw std::runtime_error( "Kinect ��ڑ����Ă�������" );
	}

	// �ŏ���Kinect�̃C���X�^���X���쐬����
	ERROR_CHECK( ::NuiCreateSensorByIndex( 0, &kinect ) );

	// Kinect�̏�Ԃ��擾����
	HRESULT status = kinect->NuiStatus();
	if ( status != S_OK ) {
		throw std::runtime_error( "Kinect �����p�\�ł͂���܂���" );
	}
}

void DetectHeadFromDepth::drawRgbImage( cv::Mat& image )
{
	// RGB�J�����̃t���[���f�[�^���擾����
	NUI_IMAGE_FRAME imageFrame = { 0 };
	ERROR_CHECK( kinect->NuiImageStreamGetNextFrame( imageStreamHandle, INFINITE, &imageFrame ) );

	// �摜�f�[�^���擾����
	NUI_LOCKED_RECT colorData;
	imageFrame.pFrameTexture->LockRect( 0, &colorData, 0, 0 );

	// �摜�f�[�^���R�s�[����
	image = cv::Mat( height, width, CV_8UC4, colorData.pBits );


	// �t���[���f�[�^���������
	ERROR_CHECK( kinect->NuiImageStreamReleaseFrame( imageStreamHandle, &imageFrame ) );
}

void DetectHeadFromDepth::drawDepthImage( cv::Mat& image )
{
	// �����J�����̃t���[���f�[�^���擾����
	NUI_IMAGE_FRAME depthFrame = { 0 };
	ERROR_CHECK( kinect->NuiImageStreamGetNextFrame( depthStreamHandle, INFINITE, &depthFrame ) );

	// �����f�[�^���擾����
	NUI_LOCKED_RECT depthData = { 0 };
	depthFrame.pFrameTexture->LockRect( 0, &depthData, 0, 0 );

	USHORT* depth = (USHORT*)depthData.pBits;
	for ( int i = 0; i < (depthData.size / sizeof(USHORT)); ++i ) {
		USHORT distance = ::NuiDepthPixelToDepth( depth[i] );

		LONG depthX = i % width;
		LONG depthY = i / width;

		// �����𐳋K�����C�摜�Ɋi�[
		int index = ((depthY * width) + depthX) * 3;
		UCHAR* data = &image.data[index];

		USHORT positionHeight = KINECT_HEIGHT - distance;
		if ( SHOULDER_HEIGHT <= positionHeight ) {// && positionHeight <= SHOULDER_HEIGHT + SHOULDER_LENGTH ) {
			int distanceColor = (int)( (distance - SENCEING_MIN) * 255 / (SENCEING_MAX - SENCEING_MIN) ) * 1;
			data[0] = 0;
			data[1] = 0;
			data[2] = 0;
		//if ( SENCEING_MIN <= distance & distance <= SENCEING_MAX ) {
		//	int distanceColor = (int)( (distance - SENCEING_MIN) * 255 / (SENCEING_MAX - SENCEING_MIN) ) * 1;  // �X�P�[����������Ƌ����̕ω������₷������
		//	data[0] = distanceColor % 255;
		//	data[1] = distanceColor % 255;
		//	data[2] = distanceColor % 255;
		}else {
			data[0] = 254;
			data[1] = 254;
			data[2] = 254;
		}
	}

	// �t���[���f�[�^���������
	ERROR_CHECK( kinect->NuiImageStreamReleaseFrame( depthStreamHandle, &depthFrame ) );
}

void DetectHeadFromDepth::detectEllipce( cv::Mat& image )
{
	cv::Mat gray_img, bin_img;
	cv::cvtColor(image, gray_img, CV_BGR2GRAY);

	std::vector<std::vector<cv::Point> > contours;
	// �摜�̓�l��
	cv::threshold(gray_img, bin_img, 0, 255, cv::THRESH_BINARY|cv::THRESH_OTSU);
	// �֊s�̒��o
	cv::findContours(bin_img, contours, CV_RETR_LIST, CV_CHAIN_APPROX_NONE);

	for(int i=0; i<contours.size(); ++i) {
		size_t count = contours[i].size();
		if(count < 150 || count > 1000) continue;	// (����������|����������) �֊s������

		cv::Mat pointsf;
		cv::Mat(contours[i]).convertTo(pointsf, CV_32F);
		// �ȉ~�t�B�b�e�B���O
		cv::RotatedRect box = cv::fitEllipse(pointsf);
		// �ȉ~�̕`��
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
