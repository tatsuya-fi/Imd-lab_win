
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
  // Near���[�h�ɂ���
  ERROR_CHECK( kinect->NuiImageStreamOpen( NUI_IMAGE_TYPE_DEPTH_AND_PLAYER_INDEX, CAMERA_RESOLUTION,
    NUI_IMAGE_FRAME_FLAG_NEAR_MODE_ENABLED, 2, 0, &depthStreamHandle ) );
  //ERROR_CHECK( kinect->NuiImageStreamOpen( NUI_IMAGE_TYPE_DEPTH_AND_PLAYER_INDEX, CAMERA_RESOLUTION, 0, 2, 0, &depthStreamHandle ) );

  ERROR_CHECK( kinect->NuiImageStreamOpen( NUI_IMAGE_TYPE_DEPTH_AND_PLAYER_INDEX, CAMERA_RESOLUTION, 0, 2, 0, &depthStreamHandle ) );

  // �X�P���g��������������
  // Near���[�h�ł̃X�P���g���g���b�L���O����сASeated���[�h�ɂ���
  ERROR_CHECK( kinect->NuiSkeletonTrackingEnable( 0, 
    NUI_SKELETON_TRACKING_FLAG_SUPPRESS_NO_FRAME_DATA |
	  NUI_SKELETON_TRACKING_FLAG_ENABLE_IN_NEAR_RANGE | 
	  NUI_SKELETON_TRACKING_FLAG_ENABLE_SEATED_SUPPORT ) );
  //ERROR_CHECK( kinect->NuiSkeletonTrackingEnable( 0, NUI_SKELETON_TRACKING_FLAG_SUPPRESS_NO_FRAME_DATA ) );

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
    setDepthImage( depthImage );
    setSkeleton( depthImage );

    cv::imshow( "RGB Image", rgbImage );

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

void KinectControl::setDepthImage( cv::Mat& image )
{
  image = cv::Mat( height, width, CV_16UC1, cv::Scalar ( 0 ) );

  // �����J�����̃t���[���f�[�^���擾����
  NUI_IMAGE_FRAME depthFrame = { 0 };
  HRESULT ret = kinect->NuiImageStreamGetNextFrame( depthStreamHandle, 0, &depthFrame );

  if( ret == S_OK ) {
    // �����f�[�^���擾����
    NUI_LOCKED_RECT depthData = { 0 };
    depthFrame.pFrameTexture->LockRect( 0, &depthData, 0, 0 );

    USHORT* depth = (USHORT*)depthData.pBits;
    for ( int i = 0; i < (depthData.size / sizeof(USHORT)); ++i ) {
      USHORT distance = ::NuiDepthPixelToDepth( depth[i] );

      LONG depthX = i % width;
      LONG depthY = i / width;
      LONG colorX;
      LONG colorY;

      // �����J�����̍��W���ARGB�J�����̍��W�ɕϊ�����
      kinect->NuiImageGetColorPixelCoordinatesFromDepthPixelAtResolution( CAMERA_RESOLUTION, CAMERA_RESOLUTION, 0, depthX , depthY, 0, &colorX, &colorY );

      // �f�v�X�摜�Ɋi�[
      image.at<USHORT>( colorY, colorX ) = distance;
    }

    // �t���[���f�[�^���������
    ERROR_CHECK( kinect->NuiImageStreamReleaseFrame( depthStreamHandle, &depthFrame ) );
  }
}

void KinectControl::setSkeleton( cv::Mat& image )
{ 
  try { 
    // �X�P���g���̃t���[�����擾����
    NUI_SKELETON_FRAME skeletonFrame = { 0 };
    HRESULT ret = kinect->NuiSkeletonGetNextFrame( 0, &skeletonFrame );

    if( ret == S_OK ) {
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
  }
  catch ( std::exception& ex ) {
    std::cout << ex.what() << std::endl;
  }
  catch ( ... ) {
    std::cout << "unknown exception" << std::endl;
  }
}

void KinectControl::setJoint( cv::Mat& image, int joint, Vector4 position )
{
  // ����
  if( joint == 7 ) {
    lhPos = position;
    setHandImage( lhPos, lwPos, rgbImage, "LeftHand" );
  }

  // �����
  if( joint == 6 ) {
    lwPos = position;
  }

  // �E��
  if( joint == 11 ) {
    rhPos = position;
    setHandImage( rhPos, rwPos, rgbImage, "RightHand" );
  }

  // �E���
  if( joint == 10 ) {
    rwPos = position;
  }
}

void KinectControl::setHandImage( Vector4 handPos, Vector4 wristPos, cv::Mat &image, std::string handName )
{
  // ��̒��S����c��18cm����
  handPos.x -= 0.18;
  handPos.y += 0.18;
  cv::Point2f ltPos;

  // �f�v�X�摜�n�֕ϊ�
  ::NuiTransformSkeletonToDepthImage( handPos, &ltPos.x, &ltPos.y, CAMERA_RESOLUTION );

  // ��̒��S����c��18cm�E��
  handPos.x += 0.36;  // ���0.18�����Ă���̂Ŕ{����
  handPos.y -= 0.36;  // ���0.18�����Ă���̂Ŕ{����
  cv::Point2f rbPos;

  // �f�v�X�摜�n�֕ϊ�
  ::NuiTransformSkeletonToDepthImage( handPos, &rbPos.x, &rbPos.y, CAMERA_RESOLUTION );

  // ���̈ʒu
  cv::Point2f wrPos;
  ::NuiTransformSkeletonToDepthImage( wristPos, &wrPos.x, &wrPos.y, CAMERA_RESOLUTION );

  // RGB�J�����̍��W�ɕϊ�����
  LONG ltPosCX, ltPosCY, rbPosCX, rbPosCY, wrPosCX, wrPosCY;
  kinect->NuiImageGetColorPixelCoordinatesFromDepthPixelAtResolution( CAMERA_RESOLUTION, CAMERA_RESOLUTION, 0, (LONG)ltPos.x , (LONG)ltPos.y, 0, &ltPosCX, &ltPosCY );
  kinect->NuiImageGetColorPixelCoordinatesFromDepthPixelAtResolution( CAMERA_RESOLUTION, CAMERA_RESOLUTION, 0, (LONG)rbPos.x , (LONG)rbPos.y, 0, &rbPosCX, &rbPosCY );
  kinect->NuiImageGetColorPixelCoordinatesFromDepthPixelAtResolution( CAMERA_RESOLUTION, CAMERA_RESOLUTION, 0, (LONG)wrPos.x , (LONG)wrPos.y, 0, &wrPosCX, &wrPosCY );

  // �摜�T�C�Y���Ȃ�
  if( rbPosCX < width && rbPosCY < height && ltPosCX > 0 && ltPosCY > 0 ) {

    // ��̈�̋�`
    cv::Rect handRect( ltPosCX, ltPosCY, abs( rbPosCX - ltPosCX ), abs( rbPosCY - ltPosCY ) );

    // �蒆�S�̋����l�imm�j
    handPos.x -= 0.18;
    handPos.y += 0.18;
    cv::Point2f cPos;
    ::NuiTransformSkeletonToDepthImage( handPos, &cPos.x, &cPos.y, CAMERA_RESOLUTION );
    LONG cPosX, cPosY;
    kinect->NuiImageGetColorPixelCoordinatesFromDepthPixelAtResolution( CAMERA_RESOLUTION, CAMERA_RESOLUTION, 0, (LONG)cPos.x , (LONG)cPos.y, 0, &cPosX, &cPosY );
    cPos.x = cPosX;
    cPos.y = cPosY;
    USHORT handDist = depthImage.at<USHORT>( cPos.y, cPos.x );

    // �G���[�l�i0�j��8192�ɕύX
    cv::Mat depth = depthImage.clone();
    cv::Mat handMask( depth, handRect );
    for( int i = 0; i < handMask.rows; ++i ) {
      for( int j = 0; j < handMask.cols; ++j ) {
        if( handMask.at<USHORT>( i, j ) == 0 )
          handMask. at<USHORT>( i, j ) = 8192;
      }
    }
    // threshold�p��CV_32F�֕ϊ�
    handMask.convertTo( handMask, CV_32F );

    // �蒆�S�̎�O30cm,��5cm�����o���}�X�N
    cv::Mat nearMask, farMask;
    cv::threshold( handMask, nearMask, handDist - 300, 8192, cv::THRESH_BINARY );
    cv::threshold( handMask, farMask, handDist + 50, 8192, cv::THRESH_BINARY_INV );
    cv::bitwise_and( nearMask, farMask, handMask );
    handMask.convertTo( handMask, CV_8U, 255.0 / 8192.0 );

    // 300x300�s�N�Z���Ƀ��T�C�Y
    cv::resize( handMask, handMask, cv::Size( 300, 300 ) );

    // �֊s���o
    cv::Mat skinMask = handMask.clone();
    std::vector<std::vector<cv::Point> > contours;
    cv::findContours( skinMask, contours, CV_RETR_LIST, CV_CHAIN_APPROX_NONE );

    // ��ԑ傫�ȗ̈�𒲂ׂ�
    int maxContNo = 0;
    std::vector<cv::Point> handContour;
    if( !contours.empty() ) {
      for( int i = 1; i < contours.size(); ++i ) {
        if( cv::contourArea( contours.at( i ) ) > cv::contourArea( contours.at( maxContNo ) ) )
          maxContNo = i;
      }
      handContour = contours.at( maxContNo );
    }

    if( !handContour.empty() ) {

      // ���S��ݒ�
      cv::Point grav( handMask.cols / 2, handMask.rows / 2 );

      // �}�X�N��ł̎��ʒu
      cv::Point wrPosM( (int)( (double)( wrPosCX - handRect.x ) / (double)handRect.width * (double)handMask.cols ), (int)( (double)( wrPosCY - handRect.y ) / (double)handRect.height * (double)handMask.cols ) );

      // ���S����e�֊s�_�ւ̋���
      std::vector<double> cgDists;
      for( int i = 0; i < handContour.size(); ++i ) {
        double xdiff = pow( (double)( handContour.at( i ).x - grav.x ), 2 );
        double ydiff = pow( (double)( handContour.at( i ).y - grav.y ), 2 );
        cgDists.push_back( sqrt( xdiff + ydiff ) );
      }

      // ��[�_�̒T��
      std::vector<int> curveNo;
      for( int i = 0; i < cgDists.size(); ++i ) {
        int step = 50;

        // ���݂̗֊s�_�܂ł̋���
        double now = cgDists.at( i );

        // step�O�̗֊s�_�܂ł̋���
        int before = i - step;
        if( before < 0 ) {
          before = cgDists.size() + ( i - step );
          if ( before >= cgDists.size() ) {
            before = cgDists.size() - 1;
          }
        }

        double bDist = cgDists.at( before );

        // step��̗֊s�_�܂ł̋���
        int next = i + step;
        if( next >= cgDists.size() ) {
          next = ( i + step ) - cgDists.size();
          if ( next < 0 ) {
            next = 0;
          }
          else if ( next >= cgDists.size() ) {
            next = cgDists.size() - 1;
          }
        }

        double nDist = cgDists.at( next );

        // ��̈��`�̒���
        int side = MAX( handMask.rows, handMask.cols );

        // ���S���猩�ēʂ�
        if( now > bDist && now > nDist ) {

          // ���S�ɋ߂�������������
          if( now > side / 2 * 0.4 && now < side / 2 * 0.8 ) {
            curveNo.push_back( i );
          }
        }
      }

      if( !curveNo.empty() ) {

        // �w����_��
        cv::Mat hand( handMask.size(), handMask.type(), cv::Scalar( 0 ) );
        for( int i = 0; i < curveNo.size(); ++i ) {
          hand.at<UCHAR>( handContour.at( curveNo.at( i ) ) ) = 255;
        }
        cv::dilate( hand, hand, cv::Mat(), cv::Point( -1, -1 ), 3 );

        std::vector<std::vector<cv::Point> > fingers;
        cv::findContours( hand, fingers, CV_RETR_LIST, CV_CHAIN_APPROX_NONE );

        if( !fingers.empty() ) {

          // ���Ǝ蒆�S�̒��_
          cv::Point palm( ( grav.x + wrPosM.x ) / 2 , ( grav.y + wrPosM.y ) / 2 );

          // ��̂Ђ牡�f�����̌X���Ɛؕ�
          double m = -1 / ( (double)( grav.y - wrPosM.y ) / (double)( grav.x - wrPosM.x ) );
          double b = (double)palm.y - m * palm.x;

          // �֊s��`��
          //hand = cv::Mat::zeros( hand.size(), hand.type() );
          //cv::drawContours( hand, contours, maxContNo, cv::Scalar( 255 ), 1 );
          //cv::circle( hand, wrPosM, 5, cv::Scalar( 255 ), -1 );
          //cv::line( hand, palm, cv::Point( palm.x - 50, m * ( palm.x - 50 ) + b ), cv::Scalar( 255 ), 1 );
          //cv::line( hand, palm, cv::Point( palm.x + 50, m * ( palm.x + 50 ) + b ), cv::Scalar( 255 ), 1 );

          // ��񑤂̕���
          bool positive;
          if( wrPosM.y - m * wrPosM.x - b > 0 )
            positive = true;
          else
            positive = false;

          // �e���_��̓_��̏d�S���w��Ƃ���
          std::vector<cv::Point> fingerTip;
          for( int i = 0; i < fingers.size(); ++i ) {
            cv::Moments mom = cv::moments( fingers.at( i ) );
            cv::Point center( (int)( mom.m10 / mom.m00 ), (int)( mom.m01 / mom.m00 ) );

            // ��̈�摜�Ɍ��_��`��
            cv::circle( hand, center, 5, cv::Scalar( 255 ), -1 );

            // ���̃T�C�Y��
            cv::Point tmp = center;
            center.x = (int)( (double)center.x / (double)handMask.cols * (double)handRect.width );
            center.y = (int)( (double)center.y / (double)handMask.rows * (double)handRect.height );
            cv::Point fingerC( ltPosCX + center.x, ltPosCY + center.y );

            // ��񑤂����̈�Ȃ�
            if( positive == true ) {
              if( tmp.y - m * tmp.x - b < 0 )
                fingerTip.push_back( fingerC );
            }
            else {
              if( tmp.y - m * tmp.x - b > 0 )
                fingerTip.push_back( fingerC );
            }
          }

          // ���摜�ɕ`��
          grav.x = (int)( (double)grav.x / (double)handMask.rows * (double)handRect.width );
          grav.y = (int)( (double)grav.y / (double)handMask.rows * (double)handRect.height );
          wrPosM.x = (int)( (double)wrPosM.x / (double)handMask.rows * (double)handRect.width );
          wrPosM.y = (int)( (double)wrPosM.y / (double)handMask.rows * (double)handRect.height );
          for( int i = 0; i < fingerTip.size(); ++i ) {
            cv::Point gravC( ltPosCX + grav.x, ltPosCY + grav.y );
            cv::Point wristC( ltPosCX + wrPosM.x, ltPosCY + wrPosM.y );

            cv::circle( image, fingerTip.at( i ), 8, cv::Scalar( 0, 255, 255 ), -1 );
            cv::circle( image, fingerTip.at( i ), 4, cv::Scalar( 0, 0, 255 ), -1 );
            cv::circle( image, gravC, 8, cv::Scalar( 0, 255, 0 ), -1 );
            cv::circle( image, gravC, 4, cv::Scalar( 255, 0, 0 ), -1 );
            cv::circle( image, wristC, 8, cv::Scalar( 255, 255, 0 ), -1 );
            cv::line( image, wristC, gravC, cv::Scalar( 255, 0, 0 ), 2 );
          }

          // ����񂯂�
          std::string prs;
          switch( fingerTip.size() ) {
          case 0:
            prs = "ROCK";
            break;
          case 2:
            prs = "SCISSORS";
            break;
          case 5:
            prs = "PAPER";
            break;
          }

          // �w�{����\��
          std::stringstream ss;
          if( handName == "LeftHand" ) {
            ss << fingerTip.size() << " fingers " << prs;
            cv::putText( image, ss.str(), cv::Point( 0, 30 ), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar( 0, 0, 255 ), 3 );
          }
          if( handName == "RightHand" ) {
            ss << fingerTip.size() << " fingers " << prs;
            cv::putText( image, ss.str(), cv::Point( 0, 50 ), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar( 255, 0, 0 ), 3 );
          }

          //cv::imshow( handName, hand );
        }
      }
    }
  }
}
