#include "KinectAudio.h"


#define CHECKHR( x ) { HRESULT hr = x; if (FAILED(hr)) { char buf[256]; sprintf( buf, "%d: %08X\n", __LINE__, hr ); throw std::runtime_error( buf );} }

// ����������
void KinectAudio::initialize( INuiSensor* sensor )
{
}

// �G�R�[�̃L�����Z���ƁA�m�C�Y�̗}���ɂ��Đݒ肷��
void KinectAudio::setSystemMode( LONG mode )
{
}

// �������͂��J�n����
void KinectAudio::start()
{
}

// �����f�[�^���擾����
std::vector< BYTE > KinectAudio::read()
{
}
