#include <iostream>
#include <sstream>

// NuiApi.h�̑O��Windows.h���C���N���[�h����
#include <Windows.h>
#include <NuiApi.h>

#include <opencv2/opencv.hpp>



#define ERROR_CHECK( ret )  \
  if ( ret != S_OK ) {    \
    std::stringstream ss;	\
    ss << "failed " #ret " " << std::hex << ret << std::endl;			\
    throw std::runtime_error( ss.str().c_str() );			\
  }

const NUI_IMAGE_RESOLUTION CAMERA_RESOLUTION = NUI_IMAGE_RESOLUTION_640x480;

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
    ERROR_CHECK( kinect->NuiInitialize( NUI_INITIALIZE_FLAG_USES_COLOR | NUI_INITIALIZE_FLAG_USES_DEPTH_AND_PLAYER_INDEX ) );

    // RGB�J����������������
    ERROR_CHECK( kinect->NuiImageStreamOpen( NUI_IMAGE_TYPE_COLOR, CAMERA_RESOLUTION,
      0, 2, 0, &imageStreamHandle ) );

    // �����J����������������
    ERROR_CHECK( kinect->NuiImageStreamOpen( NUI_IMAGE_TYPE_DEPTH_AND_PLAYER_INDEX, CAMERA_RESOLUTION,
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

    // ���C�����[�v
    while ( 1 ) {
      // �f�[�^�̍X�V��҂�
      DWORD ret = ::WaitForSingleObject( streamEvent, INFINITE );
      ::ResetEvent( streamEvent );

      drawRgbImage( image );
      drawDepthImage( image );

      // �摜��\������
      cv::imshow( "KinectSample", image );

      // �I���̂��߂̃L�[���̓`�F�b�N���A�\���̂��߂̃E�F�C�g
      int key = cv::waitKey( 10 );
      if ( key == 'q' ) {
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
      ERROR_CHECK( kinect->NuiImageStreamGetNextFrame( depthStreamHandle, 0, &depthFrame ) );

      // �����f�[�^���擾����
      NUI_LOCKED_RECT depthData = { 0 };
      depthFrame.pFrameTexture->LockRect( 0, &depthData, 0, 0 );

      USHORT* depth = (USHORT*)depthData.pBits;
      for ( int i = 0; i < (depthData.size / sizeof(USHORT)); ++i ) {
        USHORT distance = ::NuiDepthPixelToDepth( depth[i] );
        USHORT player = ::NuiDepthPixelToPlayerIndex( depth[i] );

        LONG depthX = i % width;
        LONG depthY = i / width;
        LONG colorX = 0;
        LONG colorY = 0;

        // �����J�����̍��W���ARGB�J�����̍��W�ɕϊ�����
        kinect->NuiImageGetColorPixelCoordinatesFromDepthPixelAtResolution(
          CAMERA_RESOLUTION, CAMERA_RESOLUTION,
          0, depthX , depthY, depth[i], &colorX, &colorY );

        // �ϊ����ꂽ���W�𗘗p���āA�\���摜�̃s�N�Z���f�[�^���擾����
        int index = ((colorY * width) + colorX) * 4;
        UCHAR* data = &image.data[index];

        // �v���C���[
        if ( player != 0 ) {
            data[0] = 0;
            data[1] = 0;
            data[2] = 255;
        }
        else {
	        // ���ȏ�̋�����`�悵�Ȃ�
          if ( distance >= 1000 ) {
            data[0] = 255;
            data[1] = 255;
            data[2] = 255;
          }
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
