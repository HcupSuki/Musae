# -*- coding: utf-8 -*-
"""MuCT 重建结果可视化 GUI 入口。

运行方式（在 MuCT/ 目录下）：
    muCT_local/bin/python mugui/main.py
"""
import os
import sys

# 优先加载 venv 自带的 Qt5 运行库：否则 libqxcb.so 可能链接到系统版
# libQt5XcbQpa（版本不匹配 → xcb 平台插件加载失败）。必须在导入 PyQt5 前设置。
def _find_qt_lib():
    here = os.path.dirname(os.path.abspath(__file__))
    # 兼容仓库里的 muCT_local 以及旧文档里的 muCT 两种 venv 目录名
    for venv in ('muCT_local', 'muCT'):
        base = os.path.join(here, '..', venv, 'lib')
        if not os.path.isdir(base):
            continue
        for py in sorted(os.listdir(base)):
            q = os.path.join(base, py, 'site-packages', 'PyQt5', 'Qt5', 'lib')
            if os.path.isdir(q):
                return q
    return None

_QT_LIB = _find_qt_lib()
if _QT_LIB:
    os.environ['LD_LIBRARY_PATH'] = _QT_LIB + os.pathsep + os.environ.get('LD_LIBRARY_PATH', '')

from PyQt5.QtCore import Qt
from PyQt5.QtGui import QFont, QFontDatabase
from PyQt5.QtWidgets import QApplication

from app import MainWindow

# 项目内置中文字体（SIL OFL 1.1，随仓库分发）：系统无中文字体时避免界面出现口口口
_CJK_FONT_FILE = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                              'fonts', 'NotoSansCJKsc-Regular.otf')


def _load_cjk_font(app):
    """注册内置中文字体并设为全局默认字体。"""
    if not os.path.isfile(_CJK_FONT_FILE):
        return
    fid = QFontDatabase.addApplicationFont(_CJK_FONT_FILE)
    families = QFontDatabase.applicationFontFamilies(fid)
    if families:
        app.setFont(QFont(families[0], 10))


def main():
    QApplication.setAttribute(Qt.AA_EnableHighDpiScaling, True)
    QApplication.setAttribute(Qt.AA_UseHighDpiPixmaps, True)
    app = QApplication(sys.argv)
    _load_cjk_font(app)
    win = MainWindow()
    win.show()
    sys.exit(app.exec_())


if __name__ == '__main__':
    main()
