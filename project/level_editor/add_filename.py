import bpy

# --- オペレータ4：カスタムプロパティ['file_name']追加 ---
class MYADDON_OT_add_filename(bpy.types.Operator):
    """今選択中のオブジェクトに 'file_name' というカスタムプロパティを追加するクラス"""
    bl_idname = "myaddon.myaddon_ot_add_filename"
    bl_label = "FileName 追加"
    bl_description = "['file_name']カスタムプロパティを追加します"
    bl_options = {'REGISTER', 'UNDO'}

    def execute(self, context):
        # 今選択中のオブジェクトに対してカスタムプロパティを辞書形式で追加
        context.object["file_name"] = ""
        return {'FINISHED'}