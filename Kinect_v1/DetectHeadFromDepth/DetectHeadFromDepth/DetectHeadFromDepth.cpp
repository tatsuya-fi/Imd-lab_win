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
	// ���C�����[�v
	while ( 1 ) {
		cv::Mat image;
		cv::Mat depthImage				= cv::Mat::zeros(height, width, CV_8UC3);
		cv::Mat depthImageHeadBuf		= cv::Mat::zeros(height, width, CV_8UC3);
		cv::Mat depthImageShoulderBuf	= cv::Mat::zeros(height, width, CV_8UC3);
		cv::Mat heightMat				= cv::Mat::zeros(height, width, CV_16U);

		// �f�[�^�̍X�V��҂�
		DWORD ret = ::WaitForSingleObject( streamEvent, INFINITE );
		::ResetEvent( streamEvent );

		// �����摜�̍쐬
		makeDepthImage( depthImage, heightMat );
		// RGB�摜�̕`��
		drawRgbImage( image );
		// �̈���}�[�W����
		cv::dilate(depthImage, depthImage, cv::Mat(), cv::Point(-1, -1), 5);	//�@�c��
		//cv::medianBlur(depthImage, depthImage, 3);
		cv::imshow("test", depthImage);

		// ��`�𒊏o(boxes)
		boxesInit();
		detectEllipce( depthImage, depthImage, TRUE );
		// ���[�U�̍������Ƃɓ��ƌ��̈�����o
		drawDepthImage( heightMat, depthImage, depthImageHeadBuf, depthImageShoulderBuf );
		// ���o���ꂽ���C���̈�ɑȉ~�t�B�b�e�B���O
		detectEllipce( depthImageHeadBuf, depthImage, FALSE );
		detectEllipce( depthImageShoulderBuf, depthImage, FALSE );

		// �E�B���h�E�̍쐬
		cv::namedWindow( COLOR_IMAGE_WINNAME, CV_WINDOW_AUTOSIZE );
		cv::namedWindow( DEPTH_IMAGE_WINNAME, CV_WINDOW_AUTOSIZE );
		//�E�C���h�E�փR�[���o�b�N�֐��ƃR�[���o�b�N�֐�����C�x���g�����󂯎��ϐ���n���B
		cv::setMouseCallback( COLOR_IMAGE_WINNAME, &mfunc, &mparam );
		// �摜��\������
		cv::imshow( COLOR_IMAGE_WINNAME, image );
		cv::imshow( DEPTH_IMAGE_WINNAME, depthImage );

		cv::imshow("Head detection", depthImageHeadBuf);

		// �I���̂��߂̃L�[���̓`�F�b�N���A�\���̂��߂̃E�F�C�g
		int key = cv::waitKey( 10 );
		if ( key == 'q' || key == KEY_ESC ) {
			cv::destroyAllWindows();
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

void DetectHeadFromDepth::makeDepthImage( cv::Mat& depthImage, cv::Mat& heightMat ) {
	// �����J�����̃t���[���f�[�^���擾����
	NUI_IMAGE_FRAME depthFrame = { 0 };
	ERROR_CHECK( kinect->NuiImageStreamGetNextFrame( depthStreamHandle, INFINITE, &depthFrame ) );

	// �����f�[�^���擾����
	NUI_LOCKED_RECT depthData = { 0 };
	depthFrame.pFrameTexture->LockRect( 0, &depthData, 0, 0 );

	//�@�N���b�N�t���O�𗧂Ă�
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

		// RGB�摜���N���b�N���ꂽ�Ƃ����̍��W��depth�𒲂ׂ�
		if ( depthCheckFlag && mparam.x == depthX && mparam.y == depthY ) {
				//cout << "Distance from the ground: " << positionHeight << endl;
				cout << "Distance from the KINECT: " << distance << endl;
		}

		// ���������L�^
		USHORT* p = &heightMat.at<USHORT>(depthY, depthX);
		p[0] = positionHeight;
		//heightMat.at<USHORT>(depthY, depthX) = positionHeight;

		// ���[�U�̈���L��
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
	}

	// �t���[���f�[�^���������
	ERROR_CHECK( kinect->NuiImageStreamReleaseFrame( depthStreamHandle, &depthFrame ) );
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

// �ȒP�ȏ�����
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
	// �摜�̓�l��
	cv::threshold(gray_img, bin_img, 0, 255, cv::THRESH_BINARY|cv::THRESH_OTSU);
	// �֊s�̒��o
	cv::findContours(bin_img, contours, CV_RETR_LIST, CV_CHAIN_APPROX_NONE);

	int num = 0;
	for(int i=0; i<contours.size(); ++i) {
		size_t count = contours[i].size();
		if(count < 500 || count > 1500) continue;	// (����������|����������) �֊s������

		cv::Mat pointsf;
		cv::Mat(contours[i]).convertTo(pointsf, CV_32F);
		// �ȉ~�t�B�b�e�B���O
		cv::RotatedRect box = cv::fitEllipse(pointsf);
		// �ȉ~�̕`��
		cv::ellipse(dstImg, box, cv::Scalar(0,69,255), 2, CV_AA);

		// param��p���ă��[�U���o���A�������o���𕪗�
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

void DetectHeadFromDepth::drawDepthImage( cv::Mat& heightMat, cv::Mat& depthImage, cv::Mat& headImage, cv::Mat& shoulderImage )
{
	USHORT headHeight[USER_NUM_MAX];


	// ��`���Ƃɂ����Ƃ������ʒu(���[�U�̓�)��T��
	for ( int i = 0; i < USER_NUM_MAX; i++ ) {
		if ( boxes[i].height == 0 || boxes[i].width == 0 ) {
			break;
		}
		headHeight[i] = 0;
		for ( int y = boxes[i].y; y < boxes[i].y + boxes[i].height; y++ ) {
			for ( int x = boxes[i].x; x < boxes[i].x + boxes[i].width; x++ ) {
				if ( 0 <= x && x < width && 0 <= y && y < height ) {
					if ( heightMat.at<USHORT>(y, x) > headHeight[i] && heightMat.at<USHORT>(y,x) < HEAD_HEIGHT_MAX ) {
						headHeight[i] = heightMat.at<USHORT>(y, x);
					}
				}
			}
		}
		// ���[�U�͈̔͂�OFSET�����g��
		const int OFSET = 150;	// �T���I�t�Z�b�g[pixel]
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
	/// ���ƌ���`��
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
					//&& ( boxes[i].contains(cv::Point(x, y)) || boxes[i].contains(cv::Point(x-OFSET, y-OFSET)) || boxes[i].contains(cv::Point(x+OFSET, y-OFSET)) || boxes[i].contains(cv::Point(x+OFSET, y+OFSET)) || boxes[i].contains(cv::Point(x-OFSET, y+OFSET)) ) ) {
					// ����`��
					//if ( headHeight[i] - (HEAD_LENGTH + SHOULDER_LENGTH) <= heightMat.at<USHORT>(y, x) && heightMat.at<USHORT>(y, x) <= headHeight[i] - HEAD_LENGTH ){		// �����܂܂��ȉ~�}�b�`���O
					if ( headHeight[i] - (HEAD_LENGTH + SHOULDER_LENGTH) <= heightMat.at<USHORT>(y, x) && heightMat.at<USHORT>(y, x) <= HEAD_HEIGHT_MAX){		// �����܂�őȉ~�}�b�`���O
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
					// ����`��
					if ( headHeight[i] - HEAD_LENGTH <= heightMat.at<USHORT>(y, x) && heightMat.at<USHORT>(y, x) <= HEAD_HEIGHT_MAX ){
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



///
/// �R�[���o�b�N�֐�
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
