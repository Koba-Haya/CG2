#include "Audio.h"
using Microsoft::WRL::ComPtr;

bool AudioManager::Initialize() {
  HRESULT hr = S_OK;
  if (!mfStarted_) {
    hr = MFStartup(MF_VERSION);
    if (FAILED(hr))
      return false;
    mfStarted_ = true;
  }
  hr = XAudio2Create(&xaudio_, 0, XAUDIO2_DEFAULT_PROCESSOR);
  if (FAILED(hr))
    return false;
  hr = xaudio_->CreateMasteringVoice(&masterVoice_);
  return SUCCEEDED(hr);
}

void AudioManager::Shutdown() {
  for (auto &[k, v] : clips_) {
    if (v.voice) {
      v.voice->Stop();
      v.voice->FlushSourceBuffers();
      v.voice->DestroyVoice(); // ← 正しい解放
      v.voice = nullptr;
    }
    if (v.wfex) {
      CoTaskMemFree(v.wfex);
      v.wfex = nullptr;
    }
    v.pcm.clear();
  }
  clips_.clear();

  if (masterVoice_) {
    masterVoice_->DestroyVoice();
    masterVoice_ = nullptr;
  }
  xaudio_.Reset();

  if (mfStarted_) {
    MFShutdown();
    mfStarted_ = false;
  }
}

bool AudioManager::DecodeFileToPcm(const std::wstring &path,
                                   std::vector<BYTE> &outPcm,
                                   WAVEFORMATEX **outWfex) {
  ComPtr<IMFSourceReader> reader;
  HRESULT hr = MFCreateSourceReaderFromURL(path.c_str(), nullptr, &reader);
  if (FAILED(hr))
    return false;

  // 出力フォーマットを PCM に固定（16bit, nChannels/Rate は元に追従）
  ComPtr<IMFMediaType> typeOut;
  hr = MFCreateMediaType(&typeOut);
  if (FAILED(hr))
    return false;
  typeOut->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
  typeOut->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
  hr = reader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM,
                                   nullptr, typeOut.Get());
  if (FAILED(hr))
    return false;

  // 実際の PCM の WAVEFORMATEX を取得
  ComPtr<IMFMediaType> typeReal;
  hr = reader->GetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM,
                                   &typeReal);
  if (FAILED(hr))
    return false;

  WAVEFORMATEX *wfex = nullptr;
  UINT32 cb = 0;
  hr = MFCreateWaveFormatExFromMFMediaType(typeReal.Get(), &wfex, &cb);
  if (FAILED(hr))
    return false;

  // 全サンプルを読みきって連結
  outPcm.clear();
  while (true) {
    DWORD flags = 0;
    ComPtr<IMFSample> sample;
    hr = reader->ReadSample((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0,
                            nullptr, &flags, nullptr, &sample);
    if (FAILED(hr)) {
      CoTaskMemFree(wfex);
      return false;
    }
    if (flags & MF_SOURCE_READERF_ENDOFSTREAM)
      break;
    if (!sample)
      continue;

    ComPtr<IMFMediaBuffer> buffer;
    hr = sample->ConvertToContiguousBuffer(&buffer);
    if (FAILED(hr)) {
      CoTaskMemFree(wfex);
      return false;
    }

    BYTE *pData = nullptr;
    DWORD maxLen = 0, curLen = 0;
    hr = buffer->Lock(&pData, &maxLen, &curLen);
    if (FAILED(hr)) {
      CoTaskMemFree(wfex);
      return false;
    }

    size_t old = outPcm.size();
    outPcm.resize(old + curLen);
    std::memcpy(outPcm.data() + old, pData, curLen);

    buffer->Unlock();
  }

  *outWfex = wfex;
  return true;
}

bool AudioManager::Load(const std::string &name, const std::wstring &path,
                        float defaultVolume) {
  AudioClip clip;
  if (!DecodeFileToPcm(path, clip.pcm, &clip.wfex))
    return false;
  clip.defaultVolume = defaultVolume;
  clips_[name] = std::move(clip);
  return true;
}

bool AudioManager::Play(const std::string &name, bool loop, float volume) {
  auto it = clips_.find(name);
  if (it == clips_.end())
    return false;
  AudioClip &c = it->second;

  if (!c.voice) {
    IXAudio2SourceVoice *v = nullptr;
    HRESULT hr = xaudio_->CreateSourceVoice(&v, c.wfex);
    if (FAILED(hr))
      return false;
    c.voice = v; // ← そのまま代入
  }

  c.voice->Stop();
  c.voice->FlushSourceBuffers();

  XAUDIO2_BUFFER buf{};
  buf.pAudioData = c.pcm.data();
  buf.AudioBytes = (UINT32)c.pcm.size();
  buf.Flags = XAUDIO2_END_OF_STREAM;
  buf.LoopCount = loop ? XAUDIO2_LOOP_INFINITE : 0;

  HRESULT hr = c.voice->SubmitSourceBuffer(&buf);
  if (FAILED(hr))
    return false;

  float vol = (volume >= 0.0f) ? volume : c.defaultVolume;
  c.voice->SetVolume(vol);

  hr = c.voice->Start();
  return SUCCEEDED(hr);
}

void AudioManager::Stop(const std::string &name) {
  auto it = clips_.find(name);
  if (it == clips_.end())
    return;
  if (it->second.voice) {
    it->second.voice->Stop();
    it->second.voice->FlushSourceBuffers();
    // ボイス自体は再利用したいなら Destroy しない。再生ごとに作り直すなら
    // Destroy して null に。
  }
}

void AudioManager::SetVolume(const std::string &name, float volume) {
  auto it = clips_.find(name);
  if (it == clips_.end() || !it->second.voice)
    return;
  it->second.voice->SetVolume(volume);
}
