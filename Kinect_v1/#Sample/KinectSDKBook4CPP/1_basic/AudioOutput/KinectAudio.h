﻿#pragma once

#include <Windows.h>
#include <atlbase.h>

#include <vector>

#include <mfapi.h>          // IPropertyStore
#include <wmcodecdsp.h>     // MFPKEY_WMAAECMA_SYSTEM_MODE
#include <uuids.h>          // DShowRecordの中

#include <NuiApi.h>

#include "StaticMediaBuffer.h"

#pragma comment( lib, "Msdmo.lib" )
#pragma comment( lib, "dmoguids.lib" ) // IMediaObject
#pragma comment( lib, "amstrmid.lib" )

// Kinectの音声関連処理
class KinectAudio
{
public:

  // コンストラクタ
  KinectAudio()
    : soundSource_( 0 )
    , mediaObject_( 0 )  
    , propertyStore_( 0 )
  {
  }

  // デストラクタ
  ~KinectAudio()
  {
    close();
  }

  // 初期化する
  void initialize( INuiSensor* sensor )
  {
    close();

    ERROR_CHECK( sensor->NuiGetAudioSource( &soundSource_ ) );
    ERROR_CHECK( soundSource_->QueryInterface( &mediaObject_ ) );
    ERROR_CHECK( soundSource_->QueryInterface( &propertyStore_ ) );
  }

  // 終了する
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

  // 音声入力を開始する
  void start()
  {
    // バッファをセットする
    mediaBuffer_.SetBufferLength( getWaveFormat().nSamplesPerSec * getWaveFormat().nBlockAlign );

    memset( &outputBufferStruct_, 0, sizeof(outputBufferStruct_) );
    outputBufferStruct_.pBuffer = &mediaBuffer_;

    // 音声入力の設定を行う
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

  // 音声データを取得する
  std::vector< BYTE > read()
  {
    mediaBuffer_.Clear();

    do{
      // 音声データを取得する
      DWORD dwStatus;
      ERROR_CHECK( mediaObject_->ProcessOutput(0, 1, &outputBufferStruct_, &dwStatus) );
    } while ( outputBufferStruct_.dwStatus & DMO_OUTPUT_DATA_BUFFERF_INCOMPLETE );

    return mediaBuffer_.Clone();
  }

  // WAVEフォーマットを取得する(固定)
  const WAVEFORMATEX& getWaveFormat() const
  {
    static const WAVEFORMATEX format = {WAVE_FORMAT_PCM, 1, 16000, 32000, 2, 16, 0};
    return format;
  }

private:

  // コピーを禁止する
  KinectAudio( const KinectAudio& rhs );
  KinectAudio& operator = ( const KinectAudio& rhs );

private:

  INuiAudioBeam*	soundSource_;
  IMediaObject* 	mediaObject_;  
  IPropertyStore*	propertyStore_;

  CStaticMediaBuffer mediaBuffer_;
  DMO_OUTPUT_DATA_BUFFER outputBufferStruct_;
};

