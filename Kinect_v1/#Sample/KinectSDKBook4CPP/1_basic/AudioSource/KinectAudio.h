#pragma once

#include <Windows.h>
#include <atlbase.h>

#include <vector>

#include <mfapi.h>          // IPropertyStore
#include <wmcodecdsp.h>     // MFPKEY_WMAAECMA_SYSTEM_MODE
#include <uuids.h>          // DShowRecord�̒�

#include <NuiApi.h>

#include "StaticMediaBuffer.h"

#pragma comment( lib, "Msdmo.lib" )
#pragma comment( lib, "dmoguids.lib" ) // IMediaObject
#pragma comment( lib, "amstrmid.lib" )

// Kinect�̉����֘A����
class KinectAudio
{
public:

  // �R���X�g���N�^
  KinectAudio()
    : soundSource_( 0 )
    , mediaObject_( 0 )  
    , propertyStore_( 0 )
    , beamAngle_( 0 )
    , soundSourceAngle_( 0 )
    , soundSourceAngleConfidence_( 0 )
  {
  }

  // �f�X�g���N�^
  ~KinectAudio()
  {
    close();
  }

  // ����������
  void initialize( INuiSensor* sensor )
  {
    close();

    ERROR_CHECK( sensor->NuiGetAudioSource( &soundSource_ ) );
    ERROR_CHECK( soundSource_->QueryInterface( &mediaObject_ ) );
    ERROR_CHECK( soundSource_->QueryInterface( &propertyStore_ ) );
  }

  // �I������
  void close()
  {
    if ( propertyStore_ != 0 ) {
      propertyStore_->Release();
      propertyStore_ = 0;
    }

    if ( mediaObject_ != 0 ) {
      mediaObject_->Release();
      mediaObject_ = 0;
    }

    if ( soundSource_ != 0 ) {
      soundSource_->Release();
      soundSource_ = 0;
    }
  }

  // �}�C�N�A���C�A�G�R�[�̃L�����Z���A�m�C�Y�̗}���ɂ��Đݒ肷��
  //   SINGLE_CHANNEL_AEC = 0
  //   OPTIBEAM_ARRAY_ONLY = 2
  //   OPTIBEAM_ARRAY_AND_AEC = 4
  //   SINGLE_CHANNEL_NSAGC = 5
  void setSystemMode( LONG mode )
  {
    // Set AEC-MicArray DMO system mode.
    // This must be set for the DMO to work properly
    PROPVARIANT prop;
    ::PropVariantInit( &prop );
    prop.vt = VT_I4;
    prop.lVal = mode;
    ERROR_CHECK( propertyStore_->SetValue( MFPKEY_WMAAECMA_SYSTEM_MODE, prop ) );
    ::PropVariantClear( &prop );
  }

  // �������͂��J�n����
  void start()
  {
    // �o�b�t�@���Z�b�g����
    mediaBuffer_.SetBufferLength( getWaveFormat().nSamplesPerSec * getWaveFormat().nBlockAlign );

    memset( &outputBufferStruct_, 0, sizeof(outputBufferStruct_) );
    outputBufferStruct_.pBuffer = &mediaBuffer_;

    // �������͂̐ݒ���s��
    DMO_MEDIA_TYPE mt = {0};
    ERROR_CHECK( ::MoInitMediaType( &mt, sizeof(WAVEFORMATEX) ) );

    mt.majortype = MEDIATYPE_Audio;
    mt.subtype = MEDIASUBTYPE_PCM;
    mt.lSampleSize = 0;
    mt.bFixedSizeSamples = TRUE;
    mt.bTemporalCompression = FALSE;
    mt.formattype = FORMAT_WaveFormatEx;
    memcpy( mt.pbFormat, &getWaveFormat(), sizeof(WAVEFORMATEX) );

    ERROR_CHECK( mediaObject_->SetOutputType( 0, &mt, 0 ) );
    ::MoFreeMediaType( &mt );
  }

  // �����f�[�^���擾����
  std::vector< BYTE > read()
  {
    mediaBuffer_.Clear();

    do{
      // �����f�[�^���擾����
      DWORD dwStatus;
      ERROR_CHECK( mediaObject_->ProcessOutput(0, 1, &outputBufferStruct_, &dwStatus) );
    } while ( outputBufferStruct_.dwStatus & DMO_OUTPUT_DATA_BUFFERF_INCOMPLETE );

    // �r�[���Ɖ����̕������擾����
    ERROR_CHECK( soundSource_->GetBeam( &beamAngle_ ) );
    ERROR_CHECK( soundSource_->GetPosition( &soundSourceAngle_, &soundSourceAngleConfidence_ ) );

    return mediaBuffer_.Clone();
  }

  // WAVE�t�H�[�}�b�g���擾����(�Œ�)
  const WAVEFORMATEX& getWaveFormat() const
  {
    static const WAVEFORMATEX format = {WAVE_FORMAT_PCM, 1, 16000, 32000, 2, 16, 0};
    return format;
  }

  // ���������̐M�������擾����
  double getSoundSourceAngleConfidence() const
  {
    return soundSourceAngleConfidence_;
  }

  // ���������̒l���擾����
  double getSoundSourceAngle() const
  {
    return soundSourceAngle_;
  }

  // ������������̂��߂́A�r�[���̕������擾����
  double getBeamAngle() const
  {
    return beamAngle_;
  }

private:

  // �R�s�[���֎~����
  KinectAudio( const KinectAudio& rhs );
  KinectAudio& operator = ( const KinectAudio& rhs );

private:

  INuiAudioBeam*	soundSource_;
  IMediaObject* 	mediaObject_;  
  IPropertyStore*	propertyStore_;

  CStaticMediaBuffer mediaBuffer_;
  DMO_OUTPUT_DATA_BUFFER outputBufferStruct_;

  double beamAngle_;
  double soundSourceAngle_;
  double soundSourceAngleConfidence_;	
};

