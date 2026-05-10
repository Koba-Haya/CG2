import bpy
import mathutils

# --- オペレータ5：カスタムプロパティ['collider']追加 ---
class MYADDON_OT_add_collider(bpy.types.Operator):
    """今選択中のオブジェクトにコライダー用のカスタムプロパティを一括追加するクラス"""
    bl_idname = "myaddon.myaddon_ot_add_collider"
    bl_label = "コライダー 追加"
    bl_description = "['collider']カスタムプロパティを追加します"
    bl_options = {'REGISTER', 'UNDO'}

    def execute(self, context):
        # 3つのプロパティを初期値付きで追加
        context.object["collider"] = "BOX"
        context.object["collider_center"] = mathutils.Vector((0,0,0))
        context.object["collider_size"] = mathutils.Vector((2,2,2))
        return {'FINISHED'}