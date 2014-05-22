#include "stdafx.h"


#define ERROR_CHECK( ret )										\
  if ( ret != S_OK ) {											\
    std::stringstream ss;										\
    ss << "failed " #ret " " << std::hex << ret << std::endl;	\
    throw std::runtime_error( ss.str().c_str() );				\
  }

const NUI_IMAGE_RESOLUTION CAMERA_RESOLUTION = NUI_IMAGE_RESOLUTION_640x480;
const int SENCEING_MIN = 400;		// �v���ł���ŏ�����[mm]
const int SENCEING_MAX = 8000;		// �v���ł���ő勗��[mm]

class KinectSample
{
private:

  INuiSensor* kinect;
  HANDLE imageStreamHandle;
  HANDLE depthStreamHandle;
	HANDLE streamEvent;

  DWORD width;
  DWORD height;

public:

  KinectSample()
  {
  }

  ~KinectSample()
  {
    // �I������
    if ( kinect != 0 ) {
      kinect->NuiShutdown();
      kinect->Release();
    }
  }

  void initialize()
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

    // Near���[�h
    //ERROR_CHECK( kinect->NuiImageStreamSetImageFrameFlags(
    //  depthStreamHandle, NUI_IMAGE_STREAM_FLAG_ENABLE_NEAR_MODE ) );

    // �t���[���X�V�C�x���g�̃n���h�����쐬����
    streamEvent = ::CreateEvent( 0, TRUE, FALSE, 0 );
    ERROR_CHECK( kinect->NuiSetFrameEndEvent( streamEvent, 0 ) );

    // �w�肵���𑜓x�́A��ʃT�C�Y���擾����
    ::NuiImageResolutionToSize(CAMERA_RESOLUTION, width, height );
  }

  void run()
  {
    cv::Mat image;
	cv::Mat depthImage = cv::Mat::zeros(height, width, CV_8UC1);

    // ���C�����[�v
    while ( 1 ) {
      // �f�[�^�̍X�V��҂�
      DWORD ret = ::WaitForSingleObject( streamEvent, INFINITE );
      ::ResetEvent( streamEvent );

      drawRgbImage( image );
      drawDepthImage( depthImage );

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

private:

  void createInstance()
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

  void drawRgbImage( cv::Mat& image )
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

  void drawDepthImage( cv::Mat& image )
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
		int index = ((depthY * width) + depthX);
        UCHAR* data = &image.data[index];
		if ( SENCEING_MIN <= distance & distance <= SENCEING_MAX ) {
		  int distanceColor = (int)( (distance - SENCEING_MIN) * 255 / (SENCEING_MAX - SENCEING_MIN) ) * 10;
		  *data = distanceColor % 255;
        }else {
		  *data = 0;
		}

		if(depthX == 0 && depthY==0){
			cout << distance << endl;
		}
      }

      // �t���[���f�[�^���������
      ERROR_CHECK( kinect->NuiImageStreamReleaseFrame( depthStreamHandle, &depthFrame ) );
  }
};

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
