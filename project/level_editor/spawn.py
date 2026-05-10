import bpy
import os
import bpy.ops

# --- オペレータ：出現ポイントのシンボルを読み込む ---
class MYADDON_OT_spawn_import_symbol(bpy.types.Operator):
    """ゲームエンジン用の出現ポイントの見た目（シンボル）をデータベースに読み込むクラス"""
    bl_idname = "myaddon.myaddon_ot_spawn_import_symbol"
    bl_label = "出現ポイントシンボルImport"
    bl_description = "出現ポイントのシンボルをImportします"
    
    # 複製元となるオブジェクトの内部名と、設定するtype名
    prototype_object_name = "ProttypePlayerSpawn"
    object_name = "PlayerSpawn"

    def execute(self, context):
        print("出現ポイントのシンボルをImportします")

        # 重複ロード防止。すでに読み込み済み（プロトタイプが存在）なら、二重には読み込まない。
        # 処理、メモリの無駄遣いを回避する仕組み。
        spawn_object = bpy.data.objects.get(self.prototype_object_name)
        if spawn_object is not None:
            return {'CANCELLED'}

        # スクリプトが配置されているディレクトリの名前を取得する
        addon_directory = os.path.dirname(__file__)
        # ディレクトリからのモデルファイルの相対パスを記述
        relative_path = "player/player.obj"
        # 合成してモデルファイルのフルパスを得る
        full_path = os.path.join(addon_directory, relative_path)

        # オブジェクトをインポート
        # 第一引数に 'EXEC_DEFAULT' を指定することで、インポート時の設定ダイアログを回避して
        # スクリプトから即座に実行できるようにするメリットがある。
        bpy.ops.wm.obj_import('EXEC_DEFAULT',
            filepath=full_path, display_type='THUMBNAIL',
            forward_axis='Z', up_axis='Y')

        # 回転を適用。インポート直後のモデルに回転の適用を行い、トランスフォームをリセットしておく。
        bpy.ops.object.transform_apply(location=False,
            rotation=True, scale=False, properties=False,
            isolate_users=False)

        # アクティブなオブジェクト（インポートされたもの）を取得
        object = bpy.context.active_object
        # 内部管理用の名前（プロトタイプ名）に変更
        object.name = self.prototype_object_name

        # オブジェクトの種類を設定
        # カスタムプロパティ「type」に専用の値を設定し、エクスポーターやゲーム側で判別できるようにする。
        object["type"] = self.object_name

        # メモリ（bpy.data）上には保持するが、3Dビュー（シーン）からは外す
        # これにより、モデルデータの読み込み処理と、実際の配置処理を完全に分離できる。
        bpy.context.collection.objects.unlink(object)

        return {'FINISHED'}

# --- オペレータ：出現ポイントのシンボルを作成・配置する ---
class MYADDON_OT_spawn_create_symbol(bpy.types.Operator):
    """データベースにあるシンボルをコピーして、シーン内に配置するクラス"""
    bl_idname = "myaddon.myaddon_ot_spawn_create_symbol"
    bl_label = "出現ポイントシンボルの作成"
    bl_description = "出現ポイントのシンボルを作成します"
    bl_options = {'REGISTER', 'UNDO'}

    # 配置時のデフォルト名
    object_name = "PlayerSpawn"

    def execute(self, context):
        # 読み込み済みのプロトタイプオブジェクトを検索
        spawn_object = bpy.data.objects.get(MYADDON_OT_spawn_import_symbol.prototype_object_name)

        # まだ一度も読み込まれていない場合
        if spawn_object is None:
            # 読み込みオペレータを内部から呼び出し、プロトタイプを用意する
            bpy.ops.myaddon.myaddon_ot_spawn_import_symbol('EXEC_DEFAULT')
            # 改めて検索
            spawn_object = bpy.data.objects.get(MYADDON_OT_spawn_import_symbol.prototype_object_name)

        print("出現ポイントのシンボルを作成します")

        # 他のオブジェクトへの操作を避けるため、選択をすべて解除
        bpy.ops.object.select_all(action='DESELECT')

        # プロトタイプ（非表示）を複製して、新しい実体を作る
        object = spawn_object.copy()

        # 複製したオブジェクトを現在のシーン（コレクション）にリンクして見えるようにする
        bpy.context.collection.objects.link(object)

        # 表示用の分かりやすい名前に変更
        object.name = self.object_name

        return {'FINISHED'}