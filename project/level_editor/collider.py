import bpy
from . import add_collider

# --- UI：プロパティウィンドウ内のパネル拡張2 (Collider) ---
class OBJECT_PT_collider(bpy.types.Panel):
    """オブジェクトのプロパティウィンドウに表示される、コライダー設定用カスタムパネル"""
    bl_idname = "OBJECT_PT_collider"
    bl_label = "Collider"
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "object"

    def draw(self, context):
        """パネル内の項目を追加"""
        layout = self.layout
        # 'collider' プロパティがあるかチェック
        if "collider" in context.object:
            layout.prop(context.object, '["collider"]', text="Type")
            layout.prop(context.object, '["collider_center"]', text="Center")
            layout.prop(context.object, '["collider_size"]', text="Size")
        else:
            # プロパティがなければ、追加ボタンを表示
            layout.operator(add_collider.MYADDON_OT_add_collider.bl_idname)