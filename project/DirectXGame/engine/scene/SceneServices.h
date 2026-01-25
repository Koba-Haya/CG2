#pragma once

class DirectXCommon;
class Input;
class AudioManager;
class ImGuiManager;
class AbsoluteFrameWork;

struct SceneServices {
  DirectXCommon *dx = nullptr;
  Input *input = nullptr;
  AudioManager *audio = nullptr;
  ImGuiManager *imgui = nullptr;
  AbsoluteFrameWork *framework = nullptr;
};
