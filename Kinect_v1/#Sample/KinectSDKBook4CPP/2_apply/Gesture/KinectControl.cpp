#include "KinectControl.h"


KinectControl::KinectControl()
{
}


KinectControl::~KinectControl()
{
  if ( kinect != 0 ) {
    kinect->NuiShutdown();
    kinect->Release();
  }
}

void KinectControl::initialize()
{
  createInstance();

  // Kinect�̐ݒ������������
  ERROR_CHECK( kinect->NuiInitialize( NUI_INITIALIZE_FLAG_USES_COLOR | NUI_INITIALIZE_FLAG_USES_DEPTH_AND_PLAYER_INDEX | NUI_INITIALIZE_FLAG_USES_SKELETON ) );

  // RGB�J����������������
  ERROR_CHECK( kinect->NuiImageStreamOpen( NUI_IMAGE_TYPE_COLOR, CAMERA_RESOLUTION, 0, 2, 0, &imageStreamHandle ) );

  // �����J����������������
  ERROR_CHECK( kinect->NuiImageStreamOpen( NUI_IMAGE_TYPE_DEPTH_AND_PLAYER_INDEX, CAMERA_RESOLUTION, 0, 2, 0, &depthStreamHandle ) );

  // �X�P���g��������������
  ERROR_CHECK( kinect->NuiSkeletonTrackingEnable( 0, NUI_SKELETON_TRACKING_FLAG_SUPPRESS_NO_FRAME_DATA ) );

  // �t���[���X�V�C�x���g�̃n���h�����쐬����
  streamEvent = ::CreateEvent( 0, TRUE, FALSE, 0 );
  ERROR_CHECK( kinect->NuiSetFrameEndEvent( streamEvent, 0 ) );

  // �w�肵���𑜓x�́A��ʃT�C�Y���擾����
  ::NuiImageResolutionToSize(CAMERA_RESOLUTION, width, height );
}

void KinectControl::run()
{
  // ���C�����[�v
  while ( 1 ) {
    // �f�[�^�̍X�V��҂�
    DWORD ret = ::WaitForSingleObject( streamEvent, INFINITE );
    ::ResetEvent( streamEvent );

    setRgbImage( rgbImage );
    setSkeleton( rgbImage );

    // �摜��\������
    cv::imshow( "RGBCamera", rgbImage );

    // �I���̂��߂̃L�[���̓`�F�b�N���A�\���̂��߂̃E�F�C�g
    int key = cv::waitKey( 10 );
    if ( key == 'q' ) {
      break;
    }
  }
}

void KinectControl::createInstance()
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

void KinectControl::setRgbImage( cv::Mat& image )
{
  try {
    // RGB�J�����̃t���[���f�[�^���擾����
    NUI_IMAGE_FRAME imageFrame = { 0 };
    ERROR_CHECK( kinect->NuiImageStreamGetNextFrame( imageStreamHandle, 0, &imageFrame ) );

    // �摜�f�[�^���擾����
    NUI_LOCKED_RECT colorData;
    imageFrame.pFrameTexture->LockRect( 0, &colorData, 0, 0 );

    // �摜�f�[�^���R�s�[����
    image = cv::Mat( height, width, CV_8UC4, colorData.pBits );

    // �t���[���f�[�^���������
    ERROR_CHECK( kinect->NuiImageStreamReleaseFrame( imageStreamHandle, &imageFrame ) );
  }
  catch ( std::exception& ex ) {
    std::cout << "KinectControl::setRgbImage" << ex.what() << std::endl;
  }
}

void KinectControl::setSkeleton( cv::Mat& image )
{ 
  try {
    // �X�P���g���̃t���[�����擾����
    NUI_SKELETON_FRAME skeletonFrame = { 0 };
    ERROR_CHECK( kinect->NuiSkeletonGetNextFrame( 0, &skeletonFrame ) );

    for ( int i = 0; i < NUI_SKELETON_COUNT; ++i ) {
      NUI_SKELETON_DATA& skeletonData = skeletonFrame.SkeletonData[i];
      if ( skeletonData.eTrackingState == NUI_SKELETON_TRACKED ) {

        // �e�W���C���g���Ƃ�
        for ( int j = 0; j < NUI_SKELETON_POSITION_COUNT; ++j ) {
          if ( skeletonData.eSkeletonPositionTrackingState[j] != NUI_SKELETON_POSITION_NOT_TRACKED ) {
            setJoint( image, j, skeletonData.SkeletonPositions[j] );
          }
        }
      }
      else if ( skeletonData.eTrackingState == NUI_SKELETON_POSITION_ONLY ) {
        setJoint( image, -1, skeletonData.Position );
      }
    }
  }
  catch ( std::exception& ex ) {
    std::cout << "KinectControl::setSkeleton" << ex.what() << std::endl;
  }
}

void KinectControl::setJoint( cv::Mat& image, int joint, Vector4 position )
{
  try {
    FLOAT depthX = 0, depthY = 0;
    ::NuiTransformSkeletonToDepthImage( position, &depthX, &depthY, CAMERA_RESOLUTION );

    LONG colorX = 0;
    LONG colorY = 0;

    kinect->NuiImageGetColorPixelCoordinatesFromDepthPixelAtResolution( CAMERA_RESOLUTION, CAMERA_RESOLUTION, 0, (LONG)depthX , (LONG)depthY, 0, &colorX, &colorY );

    cv::circle( image, cv::Point( colorX, colorY ), 5, cv::Scalar( 0, 255, 0 ), 2 );
    std::stringstream ss;
    ss << joint;
    cv::putText( image, ss.str(), cv::Point( colorX, colorY ), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar( 0, 0, 255 ), 2 );
  }
  catch ( std::exception& ex ) {
    std::cout << "KinectControl::setJoint" << ex.what() << std::endl;
  }
}
