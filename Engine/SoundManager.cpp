#include "SoundManager.h"

/** Static 멤버 변수 정의 */
IXAudio2* USoundManager::pXAudio2 = nullptr;
IXAudio2MasteringVoice* USoundManager::pMasterVoice = nullptr;
std::vector<IXAudio2SourceVoice*> USoundManager::sourceVoices;
std::unordered_map<SoundType, AudioData> USoundManager::audioDataMap;
IXAudio2SourceVoice* USoundManager::bgmVoice = nullptr;
bool USoundManager::isInitialized = false;

// Private 함수 구현
bool USoundManager::LoadWavFileInternal(const std::string& filename, AudioData& audioData)
{
	std::ifstream file(filename, std::ios::binary);
	if (!file.is_open())
	{
		return false;
	}

	/** WAV 파일 헤더 읽기 및 검증 */
	char riffHeader[4];
	file.read(riffHeader, 4);
	if (strncmp(riffHeader, "RIFF", 4) != 0)
	{
		file.close();
		return false;
	}

	DWORD fileSize;
	file.read(reinterpret_cast<char*>(&fileSize), 4);

	char waveHeader[4];
	file.read(waveHeader, 4);
	if (strncmp(waveHeader, "WAVE", 4) != 0)
	{
		file.close();
		return false;
	}

	/** fmt 청크 찾기 */
	char chunkId[4];
	DWORD chunkSize;
	bool fmtFound = false;

	while (file.read(chunkId, 4))
	{
		file.read(reinterpret_cast<char*>(&chunkSize), 4);

		if (strncmp(chunkId, "fmt ", 4) == 0)
		{
			fmtFound = true;

			/** fmt 데이터 읽기 */
			WORD audioFormat;
			WORD channels;
			DWORD sampleRate;
			DWORD byteRate;
			WORD blockAlign;
			WORD bitsPerSample;

			file.read(reinterpret_cast<char*>(&audioFormat), 2);
			file.read(reinterpret_cast<char*>(&channels), 2);
			file.read(reinterpret_cast<char*>(&sampleRate), 4);
			file.read(reinterpret_cast<char*>(&byteRate), 4);
			file.read(reinterpret_cast<char*>(&blockAlign), 2);
			file.read(reinterpret_cast<char*>(&bitsPerSample), 2);

			/** 지원되는 포맷인지 확인 */
			if (audioFormat != 1)
			{ /** PCM이 아님 */
				file.close();
				return false;
			}

			/** WAVEFORMATEX 구조체 설정 */
			audioData.waveFormat.wFormatTag = audioFormat;
			audioData.waveFormat.nChannels = channels;
			audioData.waveFormat.nSamplesPerSec = sampleRate;
			audioData.waveFormat.nAvgBytesPerSec = byteRate;
			audioData.waveFormat.nBlockAlign = blockAlign;
			audioData.waveFormat.wBitsPerSample = bitsPerSample;
			audioData.waveFormat.cbSize = 0;

			/** 남은 fmt 청크 데이터 건너뛰기 */
			if (chunkSize > 16)
			{
				file.seekg(chunkSize - 16, std::ios::cur);
			}
			break;
		}
		else
		{
			/** 다른 청크는 건너뛰기 */
			file.seekg(chunkSize, std::ios::cur);
		}
	}

	if (!fmtFound)
	{
		file.close();
		return false;
	}

	/** data 청크 찾기 */
	file.seekg(12, std::ios::beg); /** RIFF 헤더 다음부터 다시 시작 */
	bool dataFound = false;

	while (file.read(chunkId, 4))
	{
		file.read(reinterpret_cast<char*>(&chunkSize), 4);

		if (strncmp(chunkId, "data", 4) == 0)
		{
			dataFound = true;

			/** 오디오 데이터 읽기 */
			audioData.audioBuffer.resize(chunkSize);
			file.read(reinterpret_cast<char*>(audioData.audioBuffer.data()), chunkSize);
			break;
		}
		else
		{
			/** 다른 청크는 건너뛰기 */
			file.seekg(chunkSize, std::ios::cur);
		}
	}

	file.close();

	if (!dataFound || audioData.audioBuffer.empty())
	{
		return false;
	}

	return true;
}

bool USoundManager::PlaySoundInternal(SoundType soundType, float volume, bool loop)
{
	if (!isInitialized)
	{
		return false;
	}

	/** 디버깅: 맵 내용 확인 */
	if (audioDataMap.empty())
	{
		/** 맵이 비어있음 - LoadAllSounds()를 호출했는지 확인 필요 */
		return false;
	}

	auto it = audioDataMap.find(soundType);
	if (it == audioDataMap.end())
	{
		/** 해당 사운드 타입을 찾을 수 없음 */
		return false;
	}

	HRESULT hr;
	IXAudio2SourceVoice* pSourceVoice = nullptr;

	/** 소스 보이스 생성 */
	hr = pXAudio2->CreateSourceVoice(&pSourceVoice, &it->second.waveFormat);
	if (FAILED(hr))
	{
		return false;
	}

	/** 볼륨 설정 */
	pSourceVoice->SetVolume(volume);

	/** 오디오 버퍼 설정 */
	XAUDIO2_BUFFER buffer = {};
	buffer.AudioBytes = static_cast<UINT32>(it->second.audioBuffer.size());
	buffer.pAudioData = it->second.audioBuffer.data();
	buffer.Flags = XAUDIO2_END_OF_STREAM;

	/** 반복 재생 설정 */
	if (loop)
	{
		buffer.LoopCount = XAUDIO2_LOOP_INFINITE;
	}

	/** 버퍼 제출 */
	hr = pSourceVoice->SubmitSourceBuffer(&buffer);
	if (FAILED(hr))
	{
		pSourceVoice->DestroyVoice();
		return false;
	}

	/** 재생 시작 */
	hr = pSourceVoice->Start(0);
	if (FAILED(hr))
	{
		pSourceVoice->DestroyVoice();
		return false;
	}

	/** BGM인 경우 별도 관리 */
	if (soundType == SoundType::BGM)
	{
		if (bgmVoice)
		{
			bgmVoice->Stop(0);
			bgmVoice->DestroyVoice();
		}
		bgmVoice = pSourceVoice;
	}
	else
	{
		/** 일반 효과음은 벡터에 추가 */
		sourceVoices.push_back(pSourceVoice);
	}

	return true;
}

// Public 함수 구현
bool USoundManager::Initialize()
{
	if (isInitialized)
	{
		return true;
	}

	HRESULT hr;

	/** COM 라이브러리 초기화 (이미 초기화된 경우 무시) */
	CoInitializeEx(nullptr, COINIT_MULTITHREADED);

	/** XAudio2 엔진 생성 */
	hr = XAudio2Create(&pXAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
	if (FAILED(hr))
	{
		return false;
	}

	/** 마스터링 보이스 생성 */
	hr = pXAudio2->CreateMasteringVoice(&pMasterVoice);
	if (FAILED(hr))
	{
		return false;
	}

	isInitialized = true;
	return true;
}

bool USoundManager::LoadAllSounds()
{
	if (!isInitialized)
	{
		return false;
	}

	/** 기존 데이터 초기화 */
	audioDataMap.clear();

	bool allLoaded = true;

	/** 각 사운드 파일 로드 */
	AudioData bgmData;
	if (LoadWavFileInternal("BGM.wav", bgmData))
	{
		audioDataMap[SoundType::BGM] = std::move(bgmData);
	}
	else
	{
		allLoaded = false;
	}

	AudioData collideData;
	if (LoadWavFileInternal("Collide.wav", collideData))
	{
		audioDataMap[SoundType::COLLIDE] = std::move(collideData);
	}
	else
	{
		allLoaded = false;
	}

	AudioData preyEatData;
	if (LoadWavFileInternal("PreyEat.wav", preyEatData))
	{
		audioDataMap[SoundType::PREY_EAT] = std::move(preyEatData);
	}
	else
	{
		allLoaded = false;
	}

	AudioData divideData;
	if (LoadWavFileInternal("Divide.wav", divideData))
	{
		audioDataMap[SoundType::DIVIDE] = std::move(divideData);
	}
	else
	{
		allLoaded = false;
	}

	AudioData uiClickData;
	if (LoadWavFileInternal("UIClick.wav", uiClickData))
	{
		audioDataMap[SoundType::UI_CLICK] = std::move(uiClickData);
	}
	else
	{
		allLoaded = false;
	}

	return allLoaded;
}

void USoundManager::BGM(float volume)
{
	PlaySoundInternal(SoundType::BGM, volume, true);
}

void USoundManager::Collide(float volume)
{
	PlaySoundInternal(SoundType::COLLIDE, volume, false);
}

void USoundManager::PreyEat(float volume)
{
	PlaySoundInternal(SoundType::PREY_EAT, volume, false);
}

void USoundManager::Divide(float volume)
{
	PlaySoundInternal(SoundType::DIVIDE, volume, false);
}

void USoundManager::UIClick(float volume)
{
	PlaySoundInternal(SoundType::UI_CLICK, volume, false);
}

void USoundManager::StopBGM()
{
	if (bgmVoice)
	{
		bgmVoice->Stop(0);
		bgmVoice->DestroyVoice();
		bgmVoice = nullptr;
	}
}

void USoundManager::StopAllSounds()
{
	/** BGM 정지 */
	StopBGM();

	/** 모든 효과음 정지 */
	for (auto& voice : sourceVoices)
	{
		if (voice)
		{
			voice->Stop(0);
			voice->DestroyVoice();
		}
	}
	sourceVoices.clear();
}

void USoundManager::CleanupFinishedSounds()
{
	for (auto it = sourceVoices.begin(); it != sourceVoices.end();)
	{
		XAUDIO2_VOICE_STATE state;
		(*it)->GetState(&state);

		/** 재생이 완료된 보이스 제거 */
		if (state.BuffersQueued == 0)
		{
			(*it)->DestroyVoice();
			it = sourceVoices.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void USoundManager::SetMasterVolume(float volume)
{
	if (pMasterVoice)
	{
		pMasterVoice->SetVolume(volume);
	}
}

bool USoundManager::ValidateSoundData(SoundType soundType)
{
	auto it = audioDataMap.find(soundType);
	if (it == audioDataMap.end())
	{
		return false;
	}

	const AudioData& data = it->second;

	/** 기본적인 오디오 포맷 검증 */
	if (data.waveFormat.wFormatTag != 1) return false; /** PCM이 아님 */
	if (data.waveFormat.nChannels == 0 || data.waveFormat.nChannels > 2) return false; /** 모노/스테레오만 지원 */
	if (data.waveFormat.nSamplesPerSec < 8000 || data.waveFormat.nSamplesPerSec > 48000) return false; /** 일반적인 샘플레이트 범위 */
	if (data.waveFormat.wBitsPerSample != 16 && data.waveFormat.wBitsPerSample != 8) return false; /** 8bit 또는 16bit만 지원 */
	if (data.audioBuffer.empty()) return false; /** 오디오 데이터가 없음 */

	return true;
}

int USoundManager::GetLoadedSoundCount()
{
	return static_cast<int>(audioDataMap.size());
}

bool USoundManager::IsSoundLoaded(SoundType soundType)
{
	return audioDataMap.find(soundType) != audioDataMap.end();
}

void USoundManager::Cleanup()
{
	if (!isInitialized)
	{
		return;
	}

	StopAllSounds();

	if (pMasterVoice)
	{
		pMasterVoice->DestroyVoice();
		pMasterVoice = nullptr;
	}

	if (pXAudio2)
	{
		pXAudio2->Release();
		pXAudio2 = nullptr;
	}

	audioDataMap.clear();
	isInitialized = false;

	CoUninitialize();
}