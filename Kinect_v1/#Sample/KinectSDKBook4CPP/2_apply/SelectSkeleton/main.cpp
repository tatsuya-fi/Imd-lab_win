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
    : activeTrackId( 0 )
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
    ERROR_CHECK( kinect->NuiInitialize(
      NUI_INITIALIZE_FLAG_USES_COLOR |
      NUI_INITIALIZE_FLAG_USES_SKELETON ) );

    // RGB�J����������������
    ERROR_CHECK( kinect->NuiImageStreamOpen( NUI_IMAGE_TYPE_COLOR,
      CAMERA_RESOLUTION, 0, 2, 0, &imageStreamHandle ) );

    // �X�P���g��������������
    ERROR_CHECK( kinect->NuiSkeletonTrackingEnable(
      0, NUI_SKELETON_TRACKING_FLAG_TITLE_SETS_TRACKED_SKELETONS ) );

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
      drawSkeleton( image );

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

  void drawSkeleton( cv::Mat& image )
  {
    // �X�P���g���̃t���[�����擾����
    NUI_SKELETON_FRAME skeletonFrame = { 0 };
    kinect->NuiSkeletonGetNextFrame( 0, &skeletonFrame );

    selectActiveSkeleton( skeletonFrame );

    for ( int i = 0; i < NUI_SKELETON_COUNT; ++i ) {
      NUI_SKELETON_DATA& skeletonData = skeletonFrame.SkeletonData[i];
      if ( skeletonData.eTrackingState == NUI_SKELETON_TRACKED ) {
        for ( int j = 0; j < NUI_SKELETON_POSITION_COUNT; ++j ) {
          if ( skeletonData.eSkeletonPositionTrackingState[j] 
            != NUI_SKELETON_POSITION_NOT_TRACKED ) {
              drawJoint( image, skeletonData.SkeletonPositions[j] );
          }
        }
      }
      else if ( skeletonData.eTrackingState == NUI_SKELETON_POSITION_ONLY ) {
        drawJoint( image, skeletonData.Position );
      }
    }
  }

  void drawJoint( cv::Mat& image, Vector4 position )
  {
    FLOAT depthX = 0, depthY = 0;
    ::NuiTransformSkeletonToDepthImage(
      position, &depthX, &depthY, CAMERA_RESOLUTION );

    LONG colorX = 0;
    LONG colorY = 0;

    kinect->NuiImageGetColorPixelCoordinatesFromDepthPixelAtResolution(
      CAMERA_RESOLUTION, CAMERA_RESOLUTION,
      0, (LONG)depthX , (LONG)depthY, 0, &colorX, &colorY );

    cv::circle( image, cv::Point( colorX, colorY ), 10, cv::Scalar( 0, 255, 0 ), 5 );
  }

  DWORD activeTrackId;
  void selectActiveSkeleton( NUI_SKELETON_FRAME& skeletonFrame )
  {
    const int center = width / 2;
    FLOAT minpos = 0;
    DWORD trackedId = 0;

    // ��Ԓ��S�ɋ߂��v���C���[��T��
    for ( int i = 0; i < NUI_SKELETON_COUNT; ++i ) {
      NUI_SKELETON_DATA& skeletonData = skeletonFrame.SkeletonData[i];
      if ( skeletonData.eTrackingState != NUI_SKELETON_NOT_TRACKED ) {
        FLOAT depthX = 0, depthY = 0;
        ::NuiTransformSkeletonToDepthImage(
          skeletonData.Position, &depthX, &depthY, CAMERA_RESOLUTION );

        if ( abs(minpos - center) > abs(depthX - center) ) {
          minpos = depthX;
          trackedId = skeletonData.dwTrackingID;
        }
      }
    }

    // ���ݒǐՂ��Ă���v���C���[�łȂ���΁A���S�ɋ߂��v���C���[���A�N�e�B�u�ɂ���
    if ( (trackedId != 0) || (trackedId != activeTrackId) ) {
      DWORD trackedIds[] = { trackedId, 0 };
      kinect->NuiSkeletonSetTrackedSkeletons( trackedIds );
      activeTrackId = trackedId;
    }
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
