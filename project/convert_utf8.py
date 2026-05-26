import os
import codecs

files = [
    r"AbsoluteEngine\graphics\pipeline\UnifiedPipeline.h",
    r"AbsoluteEngine\graphics\pipeline\UnifiedPipeline.cpp",
    r"AbsoluteEngine\graphics\Renderer.h",
    r"AbsoluteEngine\graphics\Renderer.cpp",
    r"Application\scene\DevScene.h",
    r"Application\scene\DevScene.cpp",
    r"Application\scene\GameScene.h",
    r"Application\scene\GameScene.cpp",
    r"AbsoluteEngine\resources\shaders\BoxFilter.PS.hlsl"
]

for f in files:
    if os.path.exists(f):
        with open(f, 'rb') as fp:
            content = fp.read()
        
        # 既存のBOMを取り除く
        if content.startswith(codecs.BOM_UTF8):
            content = content[len(codecs.BOM_UTF8):]
            
        try:
            # UTF-8でデコードできるか確認
            text = content.decode('utf-8')
        except UnicodeDecodeError:
            # 失敗したらShift-JISと仮定
            try:
                text = content.decode('shift_jis')
            except Exception as e:
                print(f"Failed to decode {f}: {e}")
                continue
                
        with open(f, 'wb') as fp:
            fp.write(codecs.BOM_UTF8)
            fp.write(text.encode('utf-8'))
        print(f"Converted {f}")
    else:
        print(f"Not found: {f}")
