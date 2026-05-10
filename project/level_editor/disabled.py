import bpy
from . import add_disabled

# --- UI：プロパティウィンドウ内のパネル拡張3 (Disabled) ---
class OBJECT_PT_disabled(bpy.types.Panel):
    """オブジェクトのプロパティウィンドウに表示される、無効フラグ設定用カスタムパネル"""
    bl_idname = "OBJECT_PT_disabled"
    bl_label = "Disabled"
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "object"

    def draw(self, context):
        """パネル内のUIレイアウトを描画"""
        layout = self.layout
        # カスタムプロパティがあればそれを表示し、
        # なければカスタムプロパティ追加用のボタン（オペレータ）を表示する
        if "無効オプション" in context.object:
            # bool型のカスタムプロパティをパネルで表示すればチェックボックスになる。
            layout.prop(context.object, '["無効オプション"]', text="disabled")
        else:
            layout.operator(add_disabled.MYADDON_OT_add_disabled.bl_idname)