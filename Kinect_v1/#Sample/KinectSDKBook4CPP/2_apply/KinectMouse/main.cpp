#include <iostream>
#include <sstream>

// NuiApi.h�̑O��Windows.h���C���N���[�h����
#include <Windows.h>
#include <NuiApi.h>

#include <opencv2/opencv.hpp>

#define ERROR_CHECK( ret )  \
  if ( ret != S_OK ) {      \
    std::stringstream ss;	  \
    ss << "failed " #ret " " << std::hex << ret << std::endl;			\
    throw std::runtime_error( ss.str().c_str() );			\
  }

const NUI_IMAGE_RESOLUTION CAMERA_RESOLUTION = NUI_IMAGE_RESOLUTION_640x480;

// �}�E�X���͂𑗐M����
class SendInput
{
public:

  SendInput()
  {
  }

  // �}�E�X�J�[�\���𓮂���
  static void MouseMove( int x, int y, SIZE screen )
  {
    // X,Y���W��0-65535�͈̔͂ɕϊ�����
    INPUT input = { 0 };
    input.type = INPUT_MOUSE;
    input.mi.dx = (LONG)((x * 65535) / screen.cx);
    input.mi.dy = (LONG)((y * 65535) / screen.cy);
    input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;

    ::SendInput( 1, &input, sizeof(input) );
  }

  // ���N���b�N
  static void LeftClick()
  {
    // �}�E�X�̍��{�^����Down->Up����
    INPUT input[] = {
      { INPUT_MOUSE, 0, 0, 0, MOUSEEVENTF_LEFTDOWN },
      { INPUT_MOUSE, 0, 0, 0, MOUSEEVENTF_LEFTUP },
    };

    ::SendInput( sizeof(input) / sizeof(input[0]), input, sizeof(input[0]) );
  }
};

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
    ERROR_CHECK( kinect->NuiInitialize( 
      NUI_INITIALIZE_FLAG_USES_COLOR | 
      NUI_INITIALIZE_FLAG_USES_SKELETON ) );

    // RGB�J����������������
    ERROR_CHECK( kinect->NuiImageStreamOpen( NUI_IMAGE_TYPE_COLOR, CAMERA_RESOLUTION,
      0, 2, 0, &imageStreamHandle ) );

    // �X�P���g��������������
    ERROR_CHECK( kinect->NuiSkeletonTrackingEnable(
      0, NUI_SKELETON_TRACKING_FLAG_SUPPRESS_NO_FRAME_DATA ) );

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
      skeletonMouse();

      // �摜��\������
      cv::imshow( "RGBCamera", image );

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
      ERROR_CHECK( kinect->NuiImageStreamReleaseFrame( imageStreamHandle, &imageFrame ) );
  }

  // �X�P���g�����g�p���ă}�E�X������s��
  void skeletonMouse()
  {
    // �X�P���g���̃t���[�����擾����
    NUI_SKELETON_FRAME skeletonFrame = { 0 };
    kinect->NuiSkeletonGetNextFrame( 0, &skeletonFrame );

    // �g���b�L���O���Ă���ŏ��̃X�P���g����T��
    NUI_SKELETON_DATA* skeletonData = 0;
    for ( int i = 0; i < NUI_SKELETON_COUNT; ++i ) {
      NUI_SKELETON_DATA& data = skeletonFrame.SkeletonData[i];
      if ( data.eTrackingState == NUI_SKELETON_TRACKED ) {
        skeletonData = &data;
        break;
      }
    }
    // �ǐՂ��Ă���X�P���g�����Ȃ���ΏI��
    if ( skeletonData == 0 ) {
      return;
    }

    // �E�肪�ǐՂ���Ă��Ȃ���ΏI��
    if ( skeletonData->eSkeletonPositionTrackingState[NUI_SKELETON_POSITION_HAND_RIGHT]
      == NUI_SKELETON_POSITION_NOT_TRACKED ) {
        return;
    }



    // �E��̍��W���擾����
    Vector4 rh = skeletonData->SkeletonPositions[NUI_SKELETON_POSITION_HAND_RIGHT];

    // �E��̍��W���ADepth�̍��W(2����)�ɕϊ�����
    FLOAT depthX = 0, depthY = 0;
    ::NuiTransformSkeletonToDepthImage( rh, &depthX, &depthY, CAMERA_RESOLUTION );

    // �X�N���[�����W���擾����
    SIZE screen = {
      ::GetSystemMetrics( SM_CXVIRTUALSCREEN ),
      ::GetSystemMetrics( SM_CYVIRTUALSCREEN )
    };

    // �E��̍��W���A�X�N���[�����W�ɕϊ�����
    int x = (depthX * screen.cx) / width;
    int y = (depthY * screen.cy) / height;

    // �}�E�X�𓮂���
    SendInput::MouseMove( x, y, screen );

    // �N���b�N��F�������ꍇ�ɁA�E�N���b�N���s��
    if ( isClicked( FramePoint( skeletonFrame.liTimeStamp.QuadPart, depthX, depthY ) ) ) {
      SendInput::LeftClick();
    }
  }

  // ��ƂȂ�t���[��
  struct FramePoint
  {
    LARGE_INTEGER TimeStamp;  // �^�C���X�^���v
    FLOAT X;                  // X���W
    FLOAT Y;                  // Y���W

    FramePoint( LONGLONG timeStamp = 0, FLOAT x = 0.0f, FLOAT y = 0.0f )
      : X( x )
      , Y( y )
    {
      TimeStamp.QuadPart = timeStamp;
    }
  };

  FramePoint  basePoint;

  static const int milliseconds = 2000;        // �F������܂ł̒�~���Ԃ̐ݒ�
  static const int threshold = 10;             // ���W�̕ω��ʂ�臒l

  // �N���b�N���ꂽ�����肷��
  bool isClicked( FramePoint& currentPoint )
  {
    // milliseconds���Ԍo�߂����� steady
    if ( (currentPoint.TimeStamp.QuadPart - basePoint.TimeStamp.QuadPart) > milliseconds ) {
      basePoint = currentPoint;
      return true;
    }

    // ���W�̕ω��ʂ�threshold�ȏ�Ȃ�΁AbasePoint���X�V���ď��߂���v��
    if ( abs( currentPoint.X - basePoint.X ) > threshold ||
         abs( currentPoint.Y - basePoint.Y ) > threshold ) {
          // ���W���������̂Ŋ�_�𓮂����ʒu�ɂ��炵�āA�ŏ�����v��
          basePoint = currentPoint;
    }
    
    return false;
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
