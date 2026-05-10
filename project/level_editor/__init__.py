import bpy
# 分割した自作モジュールを相対インポートで結合（これまでのファイルをすべて維持）
from . import stretch_vertex
from . import create_ico_sphere
from . import export_scene
from . import add_filename
from . import add_collider
from . import file_name
from . import collider
from . import draw_collider
from . import my_menu
from . import add_disabled
from . import disabled
from . import spawn # 追加

# ブレンダーのアドオン管理画面に表示される情報
bl_info = {
    "name": "レベルエディタ",
    "author": "Hayato Kobayashi",
    "version": (1, 0),
    "blender": (3, 3, 1),
    "location": "TopBar > MyMenu",
    "description": "オブジェクトの階層構造を維持してJSON file 出力するレベルエディタ",
    "warning": "",
    "wiki_url": "",
    "tracker_url": "",
    "category": "Object",
}

# 登録対象となるクラスのリスト。分割した各モジュールからクラスを参照する
classes = (
    stretch_vertex.MYADDON_OT_stretch_vertex,
    create_ico_sphere.MYADDON_OT_create_ico_sphere,
    export_scene.MYADDON_OT_export_scene,
    add_filename.MYADDON_OT_add_filename,
    add_collider.MYADDON_OT_add_collider,
    add_disabled.MYADDON_OT_add_disabled,
    file_name.OBJECT_PT_file_name,
    collider.OBJECT_PT_collider,
    disabled.OBJECT_PT_disabled,
    my_menu.TOPBAR_MT_my_menu,
    # 新しく追加したオペレータを登録
    spawn.MYADDON_OT_spawn_import_symbol,
    spawn.MYADDON_OT_spawn_create_symbol,
)

def register():
    """アドオン有効化時にクラスをBlenderに登録する"""
    for cls in classes:
        bpy.utils.register_class(cls)
    # 既存のトップバーメニューの末尾に、定義したサブメニューを追加
    bpy.types.TOPBAR_MT_editor_menus.append(my_menu.TOPBAR_MT_my_menu.submenu)
    
    # アドオン有効化時に、3Dビューのクラスに描画関数を登録する。登録時にハンドルが返ってくる。
    draw_collider.DrawCollider.handle = bpy.types.SpaceView3D.draw_handler_add(
        draw_collider.DrawCollider.draw_collider, (), "WINDOW", "POST_VIEW"
    )
    print("レベルエディタが有効化されました。")

def unregister():
    """アドオン無効化時に登録を解除する"""
    # アドオン無効化時に、3Dビューに登録した描画関数を登録解除する。
    if draw_collider.DrawCollider.handle:
        bpy.types.SpaceView3D.draw_handler_remove(draw_collider.DrawCollider.handle, "WINDOW")

    # 登録時とは逆の順序でメニューを削除
    bpy.types.TOPBAR_MT_editor_menus.remove(my_menu.TOPBAR_MT_my_menu.submenu)
    # クラスの登録解除（逆順）
    for cls in reversed(classes):
        bpy.utils.unregister_class(cls)
    print("レベルエディタが無効化されました。")