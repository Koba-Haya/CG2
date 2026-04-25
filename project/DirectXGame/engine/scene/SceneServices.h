#pragma once

class Input;
class AudioManager;
class ImGuiManager;
class AbsoluteFrameWork;

struct SceneServices {
  Input *input = nullptr;
  AudioManager *audio = nullptr;
  ImGuiManager *imgui = nullptr;
  AbsoluteFrameWork *framework = nullptr;
};
