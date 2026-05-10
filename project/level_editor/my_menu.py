import bpy
from . import stretch_vertex
from . import create_ico_sphere
from . import export_scene
from . import add_filename
from . import add_collider
from . import add_disabled
from . import spawn # 新しく追加したモジュール

# --- UI：トップバーに表示される独自メニュー ---
class TOPBAR_MT_my_menu(bpy.types.Menu):
    """トップバーのメニュー項目を定義するクラス"""
    bl_idname = "TOPBAR_MT_my_menu"
    bl_label = "MyMenu"
    bl_description = "自作のレベル編集機能にアクセスするメニュー"

    def draw(self, context):
        """メニューの中身を描画"""
        layout = self.layout
        # 各オペレータをメニューに追加（これまでの機能をすべて維持）
        layout.operator(stretch_vertex.MYADDON_OT_stretch_vertex.bl_idname, text=stretch_vertex.MYADDON_OT_stretch_vertex.bl_label)
        layout.operator(create_ico_sphere.MYADDON_OT_create_ico_sphere.bl_idname, text=create_ico_sphere.MYADDON_OT_create_ico_sphere.bl_label)
        layout.operator(export_scene.MYADDON_OT_export_scene.bl_idname, text=export_scene.MYADDON_OT_export_scene.bl_label)
        
        # メニュー内に区切り線を引く
        layout.separator()
        
        # カスタムプロパティ操作系のオペレータ（既存）
        layout.operator(add_filename.MYADDON_OT_add_filename.bl_idname, text=add_filename.MYADDON_OT_add_filename.bl_label)
        layout.operator(add_collider.MYADDON_OT_add_collider.bl_idname, text=add_collider.MYADDON_OT_add_collider.bl_label)
        layout.operator(add_disabled.MYADDON_OT_add_disabled.bl_idname, text=add_disabled.MYADDON_OT_add_disabled.bl_label)

        # 今回追加：出現ポイントシンボルの作成
        layout.separator()
        layout.operator(spawn.MYADDON_OT_spawn_create_symbol.bl_idname, text=spawn.MYADDON_OT_spawn_create_symbol.bl_label)
        
        # ヘルプ項目（既存）
        layout.separator()
        layout.operator("wm.url_open_preset", text="Manual", icon='HELP')
        layout.operator("wm.url_open_preset", text="Support", icon='URL')

    def submenu(self, context):
        """メニューを登録するための補助関数"""
        self.layout.menu(TOPBAR_MT_my_menu.bl_idname)