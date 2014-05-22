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

#include "StreamingWavePlayer.h"
#include "KinectAudio.h"

const NUI_IMAGE_RESOLUTION CAMERA_RESOLUTION = NUI_IMAGE_RESOLUTION_640x480;

class KinectSample
{
private:

  INuiSensor* kinect;
	bool isInitialized;

	KinectAudio	audio;
	StreamingWavePlayer	player;

  HANDLE imageStreamHandle;
  HANDLE depthStreamHandle;
	HANDLE streamEvent;

  DWORD width;
  DWORD height;

public:

  KinectSample()
		: kinect( 0 )
		, isInitialized( false )
		, imageStreamHandle( 0 )
		, depthStreamHandle( 0 )
		, streamEvent( 0 )
		, width( 0 )
		, height( 0 )
  {
  }

  ~KinectSample()
  {
		close();
  }

  void initialize()
  {
    createInstance();

		if ( kinect != 0 ) {
			initInstance();
		}
  }

	void close()
	{
    // �I������
    if ( kinect != 0 ) {
      kinect->NuiShutdown();
      kinect->Release();
			kinect = 0;
			isInitialized = false;

			if ( streamEvent != 0 ) {
				::SetEvent( streamEvent );
				::CloseHandle( streamEvent );
				streamEvent = 0;
			}
    }
	}

  void run()
  {
    cv::Mat image;

    // ���C�����[�v
    while ( 1 ) {
			if ( (kinect == 0) || !isInitialized ) {
				::Sleep( 1000 );
				continue;
			}

			try {
				// �f�[�^�̍X�V��҂�
				DWORD ret = ::WaitForSingleObject( streamEvent, INFINITE );
				if ( (ret == WAIT_FAILED) || (streamEvent == 0) ) {
					continue;
				}
				::ResetEvent( streamEvent );

				drawRgbImage( image );
				drawDepthImage( image );
				drawSkeleton( image );

				// �摜��\������
				cv::imshow( "KinectSample", image );

				// Kinect������͂����������A�X�s�[�J�[�ɏo�͂���
				player.output( audio.read() );

				// �I���̂��߂̃L�[���̓`�F�b�N���A�\���̂��߂̃E�F�C�g
				int key = cv::waitKey( 10 );
				if ( key == 'q' ) {
					break;
				}
				// ��L�[
				else if ( (key >> 16) == VK_UP ) {
					LONG angle = 0;
					kinect->NuiCameraElevationGetAngle( &angle );

					angle = std::min( angle + 5, (LONG)NUI_CAMERA_ELEVATION_MAXIMUM );
					kinect->NuiCameraElevationSetAngle( angle );
				}
				// ���L�[
				else if ( (key >> 16) == VK_DOWN ) {
					LONG angle = 0;
					kinect->NuiCameraElevationGetAngle( &angle );

					angle = std::max( angle - 5, (LONG)NUI_CAMERA_ELEVATION_MINIMUM );
					kinect->NuiCameraElevationSetAngle( angle );
				}
			}
			catch ( std::exception& ex ) {
				std::cout << ex.what() << std::endl;
			}
		}
  }

private:

	static void CALLBACK StatusChanged( HRESULT hrStatus, const OLECHAR* instanceName, const OLECHAR* uniqueDeviceName, void* pUserData )
	{
		((KinectSample *)pUserData)->StatusChanged( hrStatus, instanceName, uniqueDeviceName );
	}

	void CALLBACK StatusChanged( HRESULT hrStatus, const OLECHAR* instanceName, const OLECHAR* uniqueDeviceName )
	{
		if ( hrStatus == S_OK ) {
			// �ڑ����ꂽKinect�̃C���X�^���X���쐬����
			ERROR_CHECK( ::NuiCreateSensorById( instanceName, &kinect ) );

			initInstance();
		}
		else if ( hrStatus == E_NUI_NOTCONNECTED) {
			close();
		}
	}

  void createInstance()
  {
		::NuiSetDeviceStatusCallback( StatusChanged, this );

    // �ڑ�����Ă���Kinect�̐����擾����
    int count = 0;
    ERROR_CHECK( ::NuiGetSensorCount( &count ) );
    if ( count != 0 ) {
			// �ŏ���Kinect�̃C���X�^���X���쐬����
			ERROR_CHECK( ::NuiCreateSensorByIndex( 0, &kinect ) );

			// Kinect�̏�Ԃ��擾����
			HRESULT status = kinect->NuiStatus();
			if ( status != S_OK ) {
				throw std::runtime_error( "Kinect �����p�\�ł͂���܂���" );
			}
    }
  }

	void initInstance()
	{
    // Kinect�̐ݒ������������
    ERROR_CHECK( kinect->NuiInitialize(
			NUI_INITIALIZE_FLAG_USES_COLOR |
			NUI_INITIALIZE_FLAG_USES_DEPTH_AND_PLAYER_INDEX |
			NUI_INITIALIZE_FLAG_USES_SKELETON |
			NUI_INITIALIZE_FLAG_USES_AUDIO) );

    // RGB�J����������������
    ERROR_CHECK( kinect->NuiImageStreamOpen( NUI_IMAGE_TYPE_COLOR, CAMERA_RESOLUTION,
      0, 2, 0, &imageStreamHandle ) );

    // �����J����������������
    ERROR_CHECK( kinect->NuiImageStreamOpen( NUI_IMAGE_TYPE_DEPTH_AND_PLAYER_INDEX, CAMERA_RESOLUTION,
      0, 2, 0, &depthStreamHandle ) );

		// �����J����������������(Near���[�h)
    //ERROR_CHECK( kinect->NuiImageStreamOpen( NUI_IMAGE_TYPE_DEPTH_AND_PLAYER_INDEX, CAMERA_RESOLUTION,
    //  NUI_IMAGE_STREAM_FLAG_ENABLE_NEAR_MODE, 2, 0, &depthStreamHandle ) );

		// �X�P���g��������������
    ERROR_CHECK( kinect->NuiSkeletonTrackingEnable( 0, NUI_SKELETON_TRACKING_FLAG_SUPPRESS_NO_FRAME_DATA ) );

    // �t���[���X�V�C�x���g�̃n���h�����쐬����
    streamEvent = ::CreateEvent( 0, TRUE, FALSE, 0 );
    ERROR_CHECK( kinect->NuiSetFrameEndEvent( streamEvent, 0 ) );

    // �w�肵���𑜓x�́A��ʃT�C�Y���擾����
    ::NuiImageResolutionToSize(CAMERA_RESOLUTION, width, height );

		// �������͂̏�����
    audio.initialize( kinect );
		audio.start();

    // �o�͐���J��
		player.open( &audio.getWaveFormat() );

		isInitialized = true;
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
		bool isFindFirstSkeleton = false;

		// �X�P���g���̃t���[�����擾����
    NUI_SKELETON_FRAME skeletonFrame = { 0 };
    kinect->NuiSkeletonGetNextFrame( 0, &skeletonFrame );
    //ERROR_CHECK( kinect->NuiSkeletonGetNextFrame( 0, &skeletonFrame ) );
    for ( int i = 0; i < 6; ++i ) {
      NUI_SKELETON_DATA& skeletonData = skeletonFrame.SkeletonData[i];
      if ( skeletonData.eTrackingState == NUI_SKELETON_TRACKED ) {
        for ( int j = 0; j < 20; ++j ) {
          if ( skeletonData.eSkeletonPositionTrackingState[j] != NUI_SKELETON_POSITION_NOT_TRACKED ) {
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
    ::NuiTransformSkeletonToDepthImage( position, &depthX, &depthY, CAMERA_RESOLUTION );

    LONG colorX = 0;
    LONG colorY = 0;

    kinect->NuiImageGetColorPixelCoordinatesFromDepthPixelAtResolution(
      CAMERA_RESOLUTION, CAMERA_RESOLUTION,
      0, (LONG)depthX , (LONG)depthY, 0, &colorX, &colorY );

    cv::circle( image, cv::Point( colorX, colorY ), 10, cv::Scalar( 0, 255, 0 ), 5 );
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
