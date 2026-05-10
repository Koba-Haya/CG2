import bpy

# --- オペレータ6：カスタムプロパティ['無効オプション']追加 ---
class MYADDON_OT_add_disabled(bpy.types.Operator):
    """今選択中のオブジェクトに '無効オプション' というカスタムプロパティを追加するクラス"""
    bl_idname = "myaddon.myaddon_ot_add_disabled"
    bl_label = "Add Disabled"
    bl_description = "['無効オプション']カスタムプロパティを追加します"
    bl_options = {'REGISTER', 'UNDO'}

    def execute(self, context):
        # 今選択中のオブジェクトに対してカスタムプロパティをTrue（無効）で追加
        # python の bool 型の値は True / False のいずれかである。
        context.object["無効オプション"] = True
        return {'FINISHED'}