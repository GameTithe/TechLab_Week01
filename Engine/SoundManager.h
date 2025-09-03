#pragma once

// Sound
#include <windows.h>
#include <xaudio2.h>
#include <vector>
#include <string>
#include <fstream>
#include <unordered_map>

#pragma comment(lib, "xaudio2.lib")
/**
 * 오디오 데이터를 저장하는 구조체
 */
struct AudioData
{
	WAVEFORMATEX waveFormat;    /** 오디오 포맷 정보 */
	std::vector<BYTE> audioBuffer;  /** 오디오 데이터 버퍼 */
};

/**
 * 사운드 타입 열거형
 */
enum class SoundType
{
	BGM,        /** 배경음악 */
	COLLIDE,    /** 충돌 소리 */
	PREY_EAT,   /** 먹이 섭취 소리 */
	DIVIDE,     /** 분열 소리 */
	UI_CLICK    /** UI 클릭 소리 */
};

/**
 * XAudio2를 사용한 Static 사운드 매니저 클래스
 */
class USoundManager
{
private:
	static IXAudio2* pXAudio2;                                      /** XAudio2 엔진 인스턴스 */
	static IXAudio2MasteringVoice* pMasterVoice;                   /** 마스터링 보이스 */
	static std::vector<IXAudio2SourceVoice*> sourceVoices;         /** 소스 보이스 벡터 */
	static std::unordered_map<SoundType, AudioData> audioDataMap;  /** 사운드별 오디오 데이터 맵 */
	static IXAudio2SourceVoice* bgmVoice;                          /** BGM 전용 보이스 */
	static bool isInitialized;                                      /** 초기화 상태 */

	/**
	 * WAV 파일 로드 내부 함수
	 * @param filename 파일명
	 * @param audioData 로드할 오디오 데이터 구조체
	 * @return 성공 시 true, 실패 시 false
	 */
	static bool LoadWavFileInternal(const std::string& filename, AudioData& audioData);

	/**
	 * 사운드 재생 내부 함수
	 * @param soundType 사운드 타입
	 * @param volume 볼륨 (0.0f ~ 1.0f)
	 * @param loop 반복 재생 여부
	 * @return 성공 시 true, 실패 시 false
	 */
	static bool PlaySoundInternal(SoundType soundType, float volume = 1.0f, bool loop = false);

public:
	/**
	 * 사운드 매니저 초기화
	 * @return 성공 시 true, 실패 시 false
	 */
	static bool Initialize();

	/**
	 * 모든 사운드 파일 로드
	 * @return 성공 시 true, 실패 시 false
	 */
	static bool LoadAllSounds();

	/**
	 * BGM 재생 (반복 재생)
	 * @param volume 볼륨 (0.0f ~ 1.0f)
	 */
	static void BGM(float volume = 0.7f);

	/**
	 * 충돌 소리 재생
	 * @param volume 볼륨 (0.0f ~ 1.0f)
	 */
	static void Collide(float volume = 0.8f);

	/**
	 * 먹이 섭취 소리 재생
	 * @param volume 볼륨 (0.0f ~ 1.0f)
	 */
	static void PreyEat(float volume = 0.8f);

	/**
	 * 분열 소리 재생
	 * @param volume 볼륨 (0.0f ~ 1.0f)
	 */
	static void Divide(float volume = 0.8f);

	/**
	 * UI 클릭 소리 재생
	 * @param volume 볼륨 (0.0f ~ 1.0f)
	 */
	static void UIClick(float volume = 0.8f);

	/**
	 * BGM 정지
	 */
	static void StopBGM();

	/**
	 * 모든 사운드 정지
	 */
	static void StopAllSounds();

	/**
	 * 완료된 사운드 정리
	 */
	static void CleanupFinishedSounds();

	/**
	 * 마스터 볼륨 설정
	 * @param volume 볼륨 (0.0f ~ 1.0f)
	 */
	static void SetMasterVolume(float volume);

	/**
	 * 디버깅용: 사운드 파일 정보 검증
	 * @param soundType 확인할 사운드 타입
	 * @return 파일 정보가 유효하면 true
	 */
	static bool ValidateSoundData(SoundType soundType);

	/**
	 * 디버깅용: 로드된 사운드 개수 반환
	 * @return 로드된 사운드 개수
	 */
	static int GetLoadedSoundCount();

	/**
	 * 디버깅용: 특정 사운드 로드 상태 확인
	 * @param soundType 확인할 사운드 타입
	 * @return 로드되었으면 true, 아니면 false
	 */
	static bool IsSoundLoaded(SoundType soundType);

	/**
	 * 리소스 정리
	 */
	static void Cleanup();
};

