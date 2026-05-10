import bpy

# --- オペレータ1：頂点を伸ばす --- 
class MYADDON_OT_stretch_vertex(bpy.types.Operator):
    """名前が 'Cube' のオブジェクトの特定の頂点を移動させるクラス"""
    bl_idname = "myaddon.myaddon_ot_stretch_vertex" 
    bl_label = "頂点を伸ばす"
    bl_description = "Cubeの0番目の頂点座標をX軸方向に移動させます"
    # 実行履歴に登録し、Undo（取り消し）を可能にする
    bl_options = {'REGISTER', 'UNDO'}

    def execute(self, context):
        # シーン内にCubeが存在するかチェック
        if "Cube" in bpy.data.objects:
            # 頂点データを直接書き換えて移動させる
            bpy.data.objects["Cube"].data.vertices[0].co.x += 1.0
            self.report({'INFO'}, "頂点を伸ばしました。")
        else:
            # Cubeがない場合は警告を出す
            self.report({'WARNING'}, "Cubeが見つかりません。")
        
        return {'FINISHED'}