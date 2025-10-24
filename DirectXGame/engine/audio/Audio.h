#pragma once
#include <cassert>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <wrl.h>
#include <xaudio2.h>

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "Mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")

struct AudioClip {
  WAVEFORMATEX *wfex = nullptr;
  std::vector<BYTE> pcm;
  float defaultVolume = 1.0f;
  IXAudio2SourceVoice *voice = nullptr; // ← 生ポインタに
};

class AudioManager {
public:
  // namespace省略
  template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

public:
  ~AudioManager() { Shutdown(); }

  bool Initialize(); // MFStartup + XAudio2Create + MasteringVoice
  void Shutdown();   // 後始末

  // 読み込み（wav / mp3 / wma / m4a など MF が読めるものは基本OK）
  bool Load(const std::string &name, const std::wstring &path,
            float defaultVolume = 1.0f);

  // 再生制御
  bool Play(const std::string &name, bool loop = false, float volume = -1.0f);
  void Stop(const std::string &name);
  void SetVolume(const std::string &name, float volume); // 0.0f～1.0f

private:
  // MF SourceReader で全部 PCM へ落とす
  bool DecodeFileToPcm(const std::wstring &path, std::vector<BYTE> &outPcm,
                       WAVEFORMATEX **outWfex);

  ComPtr<IXAudio2> xaudio_;
  IXAudio2MasteringVoice *masterVoice_ = nullptr;
  bool mfStarted_ = false;

  std::unordered_map<std::string, AudioClip> clips_;
};
