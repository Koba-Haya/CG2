import bpy
import bpy_extras # ファイル保存ダイアログ（ExportHelper）を利用するためにインポート
import math       # 回転の値をラジアンから度数法（Degree）に変換するために使用
import gpu        # 描画周りを手広くサポートするモジュール
import gpu_extras.batch # ジオメトリバッチ（シェーダ、トポロジー、頂点、インデックスをまとめる）を提供
import copy       # リストを丸ごとコピーして増やしたい場合に使用するPython標準モジュール
import mathutils  # ベクトルや行列の演算をサポートするモジュール
import json       # JSON形式でのデータ出力に使用するモジュール

# ブレンダーのアドオン管理画面に表示される情報
bl_info = {
    "name": "レベルエディタ",
    "author": "Hayato Kobayashi",
    "version": (1, 0),
    "blender": (3, 3, 1),
    "location": "TopBar > MyMenu",
    "description": "オブジェクトの階層構造を維持してJSONファイル出力するレベルエディタ",
    "warning": "",
    "wiki_url": "",
    "tracker_url": "",
    "category": "Object",
}

# --- 描画拡張：コライダー描画クラス ---
class DrawCollider:
    """コライダー描画関数と、それに関連する静的メンバ変数をまとめるためのクラス"""
    # 描画ハンドル
    handle = None

    # 3Dビューに登録する描画関数
    def draw_collider():
        # 頂点データ（動的配列）
        vertices = {"pos": []}
        # インデックスデータ（動的配列）
        indices = []

        # 立方体の頂点となる8点分のローカル座標をデータとして用意（オブジェクト中心からのオフセット）
        offsets = [
            [-0.5, -0.5, -0.5], # 左下前
            [+0.5, -0.5, -0.5], # 右下前
            [-0.5, +0.5, -0.5], # 左上前
            [+0.5, +0.5, -0.5], # 右上前
            [-0.5, -0.5, +0.5], # 左下奥
            [+0.5, -0.5, +0.5], # 右下奥
            [-0.5, +0.5, +0.5], # 左上奥
            [+0.5, +0.5, +0.5], # 右上奥
        ]

        # 現在シーンのオブジェクトリストを走査し、全オブジェクトについて処理
        for object in bpy.context.scene.objects:
            
            # コライダープロパティがなければ、描画をスキップ
            if not "collider" in object:
                continue

            # 中心点、サイズの変数を宣言
            center = mathutils.Vector((0,0,0))
            size = mathutils.Vector((2,2,2))

            # プロパティから値を取得
            center[0] = object["collider_center"][0]
            center[1] = object["collider_center"][1]
            center[2] = object["collider_center"][2]
            size[0] = object["collider_size"][0]
            size[1] = object["collider_size"][1]
            size[2] = object["collider_size"][2]

            # 追加前の頂点数を記録（インデックス指定の基準にするため）
            start = len(vertices["pos"])

            # Boxの8頂点分回す
            for offset in offsets:
                # オブジェクトのローカルな中心座標（center）をベースにする
                pos = copy.copy(center)
                # 中心点を基準に各頂点ごとにずらす
                pos[0] += offset[0] * size[0]
                pos[1] += offset[1] * size[1]
                pos[2] += offset[2] * size[2]
                
                # ローカル座標からワールド座標に変換（行列の掛け算 @ を使用）
                pos = object.matrix_world @ pos

                # 共有する頂点データリストに座標を追加
                vertices["pos"].append(pos)

            # ラインリスト（12本分）のインデックスデータを追加 
            # 前面を構成する辺
            indices.append([start + 0, start + 1])
            indices.append([start + 2, start + 3])
            indices.append([start + 0, start + 2])
            indices.append([start + 1, start + 3])
            # 奥面を構成する辺
            indices.append([start + 4, start + 5])
            indices.append([start + 6, start + 7])
            indices.append([start + 4, start + 6])
            indices.append([start + 5, start + 7])
            # 手前と奥を繋ぐ辺
            indices.append([start + 0, start + 4])
            indices.append([start + 1, start + 5])
            indices.append([start + 2, start + 6])
            indices.append([start + 3, start + 7])

        # ビルトインのシェーダ（色指定のみの3D描画用）を取得
        shader = gpu.shader.from_builtin("UNIFORM_COLOR")
        # バッチを作成（シェーダ、トポロジー"LINES"、頂点、インデックスを指定
        batch = gpu_extras.batch.batch_for_shader(shader, "LINES", vertices, indices=indices)

        # シェーダのパラメータ設定（水色）
        color = [0.5, 1.0, 1.0, 1.0]
        shader.bind()
        shader.uniform_float("color", color)
        # 全オブジェクト分を1回で描画
        batch.draw(shader)

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

# --- オペレータ2：ICO球生成 ---
class MYADDON_OT_create_ico_sphere(bpy.types.Operator):
    """標準機能を使ってICO球をシーンに追加するクラス"""
    bl_idname = "myaddon.myaddon_ot_create_object" 
    bl_label = "ICO球生成"
    bl_description = "ICO球を生成します"
    bl_options = {'REGISTER', 'UNDO'}

    def execute(self, context):
        # Blender標準のメッシュ生成コマンド（bpy.ops）を内部で実行
        bpy.ops.mesh.primitive_ico_sphere_add()
        self.report({'INFO'}, "ICO球を生成しました。")
        return {'FINISHED'}

# --- オペレータ3：シーン出力 (JSON形式・再帰パッキング版) ---
class MYADDON_OT_export_scene(bpy.types.Operator, bpy_extras.io_utils.ExportHelper):
    """シーン内の全オブジェクトを親子関係を保ち、JSON形式でファイル出力するクラス"""
    bl_idname = "myaddon.myaddon_ot_export_scene"
    bl_label = "シーン出力"
    bl_description = "現在のシーン情報を階層構造を維持したJSONファイルとして保存します"
    bl_options = {'REGISTER', 'UNDO'}

    # 出力するファイルの拡張子を.jsonに設定
    filename_ext = ".json"

    def parse_scene_recursive_json(self, data_parent, object, level):
        """深さ優先探索（DFS）を用いて、オブジェクト1個分の情報をdictにパッキングし、親のリストに追加する"""
        
        # シーンのオブジェクト1個分のjsonオブジェクト(dict)生成
        json_object = dict()
        # オブジェクト種類と名前を格納
        json_object["type"] = object.type
        json_object["name"] = object.name

        # トランスフォーム行列（matrix_local）から位置・回転・スケールを分離取得
        trans, rot, scale = object.matrix_local.decompose()
        # 回転情報をクォータニオンからオイラー角に変換し、さらに度数法に変換
        rot = rot.to_euler()
        rot_x = math.degrees(rot.x)
        rot_y = math.degrees(rot.y)
        rot_z = math.degrees(rot.z)

        # トランスフォーム情報をディクショナリに登録
        transform = dict()
        # ゲームエンジン側の座標系（例：Yが上、Zが奥）に合わせる場合はここで軸を入れ替える
        transform["translation"] = (trans.x, trans.y, trans.z)
        transform["rotation"] = (rot_x, rot_y, rot_z)
        transform["scaling"] = (scale.x, scale.y, scale.z)
        # まとめて1個分のjsonオブジェクトに登録
        json_object["transform"] = transform

        # カスタムプロパティ 'file_name' がある場合は追加
        if "file_name" in object:
            json_object["file_name"] = object["file_name"]

        # カスタムプロパティ 'collider' がある場合
        if "collider" in object:
            collider = dict()
            collider["type"] = object["collider"]
            # mathutils.Vector型はそのままではエンコードできないため、to_list()でリストに変換する
            collider["center"] = object["collider_center"].to_list()
            collider["size"] = object["collider_size"].to_list()
            json_object["collider"] = collider

        # 1個分の情報をまとめた後、親オブジェクトの引数リストに子供として登録
        data_parent.append(json_object)

        # 子オブジェクトが存在する場合、レベルを1つ上げて自分自身を呼び出す（再帰）
        if len(object.children) > 0:
            json_object["children"] = list()
            for child in object.children:
                self.parse_scene_recursive_json(json_object["children"], child, level + 1)

    def export_json(self):
        """JSON形式でのパッキングからファイル書き出しまでを行うメイン処理"""
        # 保存する情報をまとめるルートのdict（連想配列）を作成
        json_object_root = dict()
        # ローダー側でのファイルチェック用識別名 [cite: 77]
        json_object_root["name"] = "scene"
        json_object_root["objects"] = list()

        # シーン内の全オブジェクトを走査してパック
        for object in bpy.context.scene.objects:
            # 親がいるオブジェクトは子として再帰的に呼ばれるため、ここではルートのみ処理 
            if object.parent:
                continue
            # ルートオブジェクト（深さ0）として解析を開始 
            self.parse_scene_recursive_json(json_object_root["objects"], object, 0)

        # オブジェクトをJSON文字列にエンコード（改行・インデント付き）
        # ensure_ascii=Falseで日本語化けを防ぎ、indent=4で見やすく整形する 
        json_text = json.dumps(json_object_root, ensure_ascii=False, cls=json.JSONEncoder, indent=4)
        
        # コンソールに表示（確認用）
        print(json_text)

        # ファイルをテキスト形式で安全に開く（UTF-8指定）
        with open(self.filepath, "wt", encoding="utf-8") as file:
            # JSON文字列を一括で書き込む
            file.write(json_text)

    def execute(self, context):
        # JSON形式のエクスポート処理を実行
        print("シーン情報をExportします")
        self.export_json()
        self.report({'INFO'}, "シーン情報をExportしました")
        print("シーン情報をExportしました")
        return {'FINISHED'}

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
            layout.operator(MYADDON_OT_add_filename.bl_idname)

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
            layout.operator(MYADDON_OT_add_collider.bl_idname)

# --- UI：トップバーに表示される独自メニュー ---
class TOPBAR_MT_my_menu(bpy.types.Menu):
    """トップバーのメニュー項目を定義するクラス"""
    bl_idname = "TOPBAR_MT_my_menu"
    bl_label = "MyMenu"
    bl_description = "自作のレベル編集機能にアクセスするメニュー"

    def draw(self, context):
        """メニューの中身を描画"""
        layout = self.layout
        # 各オペレータをメニューに追加
        layout.operator(MYADDON_OT_stretch_vertex.bl_idname, text=MYADDON_OT_stretch_vertex.bl_label)
        layout.operator(MYADDON_OT_create_ico_sphere.bl_idname, text=MYADDON_OT_create_ico_sphere.bl_label)
        layout.operator(MYADDON_OT_export_scene.bl_idname, text=MYADDON_OT_export_scene.bl_label)
        
        # メニュー内に区切り線を引く
        layout.separator()
        
        # 外部URLを開く既存のオペレータを使ってヘルプ項目を作成
        layout.operator("wm.url_open_preset", text="Manual", icon='HELP')
        layout.operator("wm.url_open_preset", text="Tutorial", icon='HELP')
        layout.operator("wm.url_open_preset", text="Support", icon='HELP')

    def submenu(self, context):
        # 既存のメニュー構造に自分自身を追加するための関数
        self.layout.menu(TOPBAR_MT_my_menu.bl_idname)

# 登録対象となるクラスのリスト。順序は依存関係に合わせる
classes = (
    MYADDON_OT_stretch_vertex,
    MYADDON_OT_create_ico_sphere,
    MYADDON_OT_export_scene,
    MYADDON_OT_add_filename,
    MYADDON_OT_add_collider,
    OBJECT_PT_file_name,
    OBJECT_PT_collider,
    TOPBAR_MT_my_menu,
)

def register():
    """アドオン有効化時にクラスをBlenderに登録する"""
    for cls in classes:
        bpy.utils.register_class(cls)
    # 既存のトップバーメニューの末尾に、定義したサブメニューを追加
    bpy.types.TOPBAR_MT_editor_menus.append(TOPBAR_MT_my_menu.submenu)
    
    # アドオン有効化時に、3Dビューのクラスに描画関数を登録する。登録時にハンドルが返ってくる。
    DrawCollider.handle = bpy.types.SpaceView3D.draw_handler_add(DrawCollider.draw_collider, (), "WINDOW", "POST_VIEW")
    print("レベルエディタが有効化されました。")

def unregister():
    """アドオン無効化時に登録を解除する"""
    # アドオン無効化時に、3Dビューに登録した描画関数を登録解除する。
    if DrawCollider.handle:
        bpy.types.SpaceView3D.draw_handler_remove(DrawCollider.handle, "WINDOW")

    # 登録時とは逆の順序でメニューを削除
    bpy.types.TOPBAR_MT_editor_menus.remove(TOPBAR_MT_my_menu.submenu)
    # クラスの登録解除
    for cls in reversed(classes):
        bpy.utils.unregister_class(cls)
    print("レベルエディタが無効化されました。")

# スクリプトとして直接実行された場合の処理
if __name__ == "__main__":
    register()