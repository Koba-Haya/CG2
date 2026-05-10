import bpy
from . import add_filename

# --- UI：プロパティウィンドウ内のパネル拡張1 (FileName) ---
class OBJECT_PT_file_name(bpy.types.Panel):
    """オブジェクトのプロパティウィンドウに表示される、動的に表示が切り替わるカスタムパネル"""
    bl_idname = "OBJECT_PT_file_name"
    bl_label = "FileName"
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "object"

    def draw(self, context):
        """パネル内のUIレイアウトを描画し、プロパティの有無で表示を切り替える"""
        layout = self.layout
        # オブジェクトに 'file_name' カスタムプロパティがあるかチェック
        if "file_name" in context.object:
            # 既にプロパティがあれば、直接編集できるプロパティフィールドを表示
            layout.prop(context.object, '["file_name"]', text=self.bl_label)
        else:
            # プロパティがなければ、追加するためのオペレータボタンを表示
            layout.operator(add_filename.MYADDON_OT_add_filename.bl_idname)