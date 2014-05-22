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

  HANDLE imageStreamEvent[3];
  bool hasSkeletonEngine;

  DWORD width;
  DWORD height;

public:

  KinectSample()
    : kinect( 0 )
    , hasSkeletonEngine( false )
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

  void initialize( int id = 0 )
  {
    createInstance( id );

    // Kinect�̐ݒ������������
    HRESULT ret = kinect->NuiInitialize(
      NUI_INITIALIZE_FLAG_USES_COLOR |
      NUI_INITIALIZE_FLAG_USES_DEPTH_AND_PLAYER_INDEX |
      NUI_INITIALIZE_FLAG_USES_SKELETON );
    if ( ret != S_OK ) {
      // ��̃v���Z�X�ŁA������Kinect����X�P���g�����擾���邱�Ƃ��ł��Ȃ�
      // RBB����ы����J�����̂ݗ��p�\(�v���C���[���X�P���g���G���W�����g�p���Ă��邽�ߕs��)
      if ( ret == E_NUI_SKELETAL_ENGINE_BUSY ) {
        ERROR_CHECK( kinect->NuiInitialize(
          NUI_INITIALIZE_FLAG_USES_COLOR |
          NUI_INITIALIZE_FLAG_USES_DEPTH ) );
      }
      else {
        ERROR_CHECK( ret );
      }
    }

    // �ҋ@�C�x���g�̍쐬
    imageStreamEvent[0] = ::CreateEvent( 0, TRUE, FALSE, 0 );
    imageStreamEvent[1] = ::CreateEvent( 0, TRUE, FALSE, 0 );
    imageStreamEvent[2] = ::CreateEvent( 0, TRUE, FALSE, 0 );

    // RGB�J����������������
    ERROR_CHECK( kinect->NuiImageStreamOpen( NUI_IMAGE_TYPE_COLOR, CAMERA_RESOLUTION,
      0, 2, imageStreamEvent[0], &imageStreamHandle ) );

    // �X�P���g���G���W���̏�����Ԃ��擾����
    hasSkeletonEngine = ::HasSkeletalEngine( kinect );

    // �X�P���g���G���W�������p�ł���ꍇ�A�����J��������уv���C���[�A�X�P���g����L���ɂ���
    if ( hasSkeletonEngine ) {
      // �����J����������������
      ERROR_CHECK( kinect->NuiImageStreamOpen(
        NUI_IMAGE_TYPE_DEPTH_AND_PLAYER_INDEX, CAMERA_RESOLUTION,
        0, 2, imageStreamEvent[1], &depthStreamHandle ) );

      // �X�P���g��������������
      ERROR_CHECK( kinect->NuiSkeletonTrackingEnable(
        imageStreamEvent[2], NUI_SKELETON_TRACKING_FLAG_SUPPRESS_NO_FRAME_DATA ) );
    }
    // �X�P���g���G���W�������p�ł��Ȃ��ꍇ�A�����J�����̂ݗL���ɂ���
    else {
      // �����J����������������
      imageStreamEvent[1] = ::CreateEvent( 0, TRUE, FALSE, 0 );
      ERROR_CHECK( kinect->NuiImageStreamOpen( NUI_IMAGE_TYPE_DEPTH, CAMERA_RESOLUTION,
        0, 2,  imageStreamEvent[1], &depthStreamHandle ) );
    }

    // �w�肵���𑜓x�́A��ʃT�C�Y���擾����
    ::NuiImageResolutionToSize(CAMERA_RESOLUTION, width, height );
  }

  void run()
  {
    cv::Mat image;

    std::stringstream ss;
    ss << "KinectSample:" << kinect->NuiInstanceIndex();

    // ���C�����[�v
    while ( 1 ) {
      try {
        // �f�[�^�̍X�V��҂�
        //	�X�P���g���G���W��������΁A�ҋ@����C�x���g��RGB�A�����A�X�P���g����3��
        //	�X�P���g���G���W�����Ȃ���΁A�ҋ@����C�x���g��RGB�A����2��
        ::WaitForMultipleObjects( (hasSkeletonEngine ? 3 : 2),
          imageStreamEvent, TRUE, INFINITE );

        drawRgbImage( image );
        drawDepthImage( image );

        // �X�P���g���G���W�������p�\�ł���΁A�X�P���g����\������
        if ( hasSkeletonEngine ) {
          drawSkeleton( image );
        }

        // �摜��\������
        cv::imshow( ss.str(), image );
      }
      catch ( std::exception& ex ) {
        std::cout << ex.what() << std::endl;
      }

      // �I���̂��߂̃L�[���̓`�F�b�N���A�\���̂��߂̃E�F�C�g
      int key = cv::waitKey( 10 );
      if ( key == 'q' ) {
        break;
      }
    }
  }

private:

  // Kinect�̏�Ԃ��ς�������ɌĂ΂��R�[���o�b�N�֐�(�N���X�֐�)
  static void CALLBACK StatusChanged( HRESULT hrStatus,
    const OLECHAR* instanceName, const OLECHAR* uniqueDeviceName, void* pUserData )
  {
  }

  void createInstance( int id = 0 )
  {
    // V1.0 �ł�NuiInitialize�Ɋ��m�̖�肪���邽�߁A����v���Z�X�ŕ���Kinect���g�p����ꍇ��
    // NuiInitialize�̑O��NuiSetDeviceStatusCallback()���Ăяo���K�v������B
    // http://blogs.msdn.com/b/windows_multimedia_jp/archive/2012/02/28/microsoft-kinect-for-windows-sdk-v1-0.aspx
    ::NuiSetDeviceStatusCallback( StatusChanged, 0 );

    // �ڑ�����Ă���Kinect�̐����擾����
    int count = 0;
    ERROR_CHECK( ::NuiGetSensorCount( &count ) );
    if ( count == 0 ) {
      throw std::runtime_error( "Kinect ��ڑ����Ă�������" );
    }

    // �ŏ���Kinect�̃C���X�^���X���쐬����
    ERROR_CHECK( ::NuiCreateSensorByIndex( id, &kinect ) );

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
    ERROR_CHECK( kinect->NuiImageStreamGetNextFrame(
      imageStreamHandle, INFINITE, &imageFrame ) );

    // �摜�f�[�^���擾����
    NUI_LOCKED_RECT colorData;
    imageFrame.pFrameTexture->LockRect( 0, &colorData, 0, 0 );

    // �摜�f�[�^���R�s�[����
    image = cv::Mat( height, width, CV_8UC4, colorData.pBits );

    // �t���[���f�[�^���������
    ERROR_CHECK( kinect->NuiImageStreamReleaseFrame(
      imageStreamHandle, &imageFrame ) );
  }

  void drawDepthImage( cv::Mat& image )
  {
    // �����J�����̃t���[���f�[�^���擾����
    NUI_IMAGE_FRAME depthFrame = { 0 };
    ERROR_CHECK( kinect->NuiImageStreamGetNextFrame(
      depthStreamHandle, INFINITE, &depthFrame ) );

    // �����f�[�^���擾����
    NUI_LOCKED_RECT depthData = { 0 };
    depthFrame.pFrameTexture->LockRect( 0, &depthData, 0, 0 );

    USHORT* depth = (USHORT*)depthData.pBits;
    for ( int i = 0; i < (depthData.size / sizeof(USHORT)); ++i ) {
      USHORT distance = ::NuiDepthPixelToDepth( depth[i] );
      USHORT player = ::NuiDepthPixelToPlayerIndex( depth[i] );

      LONG depthX = i % width;
      LONG depthY = i / width;
      LONG colorX = depthX;
      LONG colorY = depthY;

      // �����J�����̍��W���ARGB�J�����̍��W�ɕϊ�����
      //kinect->NuiImageGetColorPixelCoordinatesFromDepthPixelAtResolution(
      //  CAMERA_RESOLUTION, CAMERA_RESOLUTION,
      //  0, depthX , depthY, 0, &colorX, &colorY );

      // �v���C���[
      if ( player != 0 ) {
        int index = ((colorY * width) + colorX) * 4;
        UCHAR* data = &image.data[index];
        data[0] = 0;
        data[1] = 0;
        data[2] = 255;
      }
      else {
        // ���ȏ�̋�����`�悵�Ȃ�
        if ( distance >= 1000 ) {
          int index = ((colorY * width) + colorX) * 4;
          UCHAR* data = &image.data[index];
          data[0] = 255;
          data[1] = 255;
          data[2] = 255;
        }
      }
    }

    // �t���[���f�[�^���������
    ERROR_CHECK( kinect->NuiImageStreamReleaseFrame( depthStreamHandle, &depthFrame ) );
  }

  void drawSkeleton( cv::Mat& image )
  {
    // �X�P���g���̃t���[�����擾����
    NUI_SKELETON_FRAME skeletonFrame = { 0 };
    HRESULT ret = kinect->NuiSkeletonGetNextFrame( 0, &skeletonFrame );
    if ( ret != S_OK ) {
      return;
    }

    // �X�P���g����\������
    for ( int i = 0; i < NUI_SKELETON_COUNT; ++i ) {
      NUI_SKELETON_DATA& skeletonData = skeletonFrame.SkeletonData[i];
      // �X�P���g�����ǐՂ���Ă�����
      if ( skeletonData.eTrackingState == NUI_SKELETON_TRACKED ) {
        for ( int j = 0; j < NUI_SKELETON_POSITION_COUNT; ++j ) {
          if ( skeletonData.eSkeletonPositionTrackingState[j] 
            != NUI_SKELETON_POSITION_NOT_TRACKED ) {
              drawJoint( image, skeletonData.SkeletonPositions[j] );
          }
        }
      }
      // �X�P���g���̈ʒu�̂ݒǐՂ��Ă�����
      // Near���[�h�̑S�v���C���[����сADefault���[�h�̃X�P���g���ǐՂ���Ă���v���C���[�ȊO
      else if ( skeletonData.eTrackingState == NUI_SKELETON_POSITION_ONLY ) {
        drawJoint( image, skeletonData.Position );
      }
    }
  }

  void drawJoint( cv::Mat& image, Vector4 position )
  {
    // �X�P���g���̍��W���A�����J�����̍��W�ɕϊ�����
    FLOAT depthX = 0, depthY = 0;
    ::NuiTransformSkeletonToDepthImage( position, &depthX, &depthY, CAMERA_RESOLUTION );

    LONG colorX = 0;
    LONG colorY = 0;

    // �����J�����̍��W���ARGB�J�����̍��W�ɕϊ�����
    kinect->NuiImageGetColorPixelCoordinatesFromDepthPixelAtResolution(
      CAMERA_RESOLUTION, CAMERA_RESOLUTION,
      0, (LONG)depthX , (LONG)depthY, 0, &colorX, &colorY );

    cv::circle( image, cv::Point( colorX, colorY ), 10, cv::Scalar( 0, 255, 0 ), 5 );
  }
};

DWORD WINAPI ThreadEntry( LPVOID lpThreadParameter )
{
  KinectSample* kinect = (KinectSample*)lpThreadParameter;
  kinect->run();

  return 0;
}

void main()
{
  try {
    int count = 0;
    ERROR_CHECK( ::NuiGetSensorCount( &count ) );
    if ( count == 0 ) {
      throw std::runtime_error( "Kinect ��ڑ����Ă�������" );
    }

    std::vector< KinectSample > kinects( count );
    std::vector< HANDLE > hThread( count );

    for ( int i = 0; i < kinects.size(); ++i )  {
      DWORD id = 0;
      kinects[i].initialize( i );
      hThread[i] = ::CreateThread( 0, 0, ThreadEntry, &kinects[i], 0, &id );
    }

    ::WaitForMultipleObjects( hThread.size(), &hThread[0], TRUE, INFINITE );
  }
  catch ( std::exception& ex ) {
    std::cout << ex.what() << std::endl;
  }
}
