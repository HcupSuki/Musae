# -*- coding: utf-8 -*-
"""MuCT 重建结果可视化 GUI 入口。

运行方式（在 MuCT/ 目录下）：
    muCT/bin/python mugui/main.py
"""
import os
import sys

# 优先加载 venv 自带的 Qt5 运行库：否则 libqxcb.so 可能链接到系统版
# libQt5XcbQpa（版本不匹配 → xcb 平台插件加载失败）。必须在导入 PyQt5 前设置。
def _find_qt_lib():
    base = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'muCT', 'lib')
    if not os.path.isdir(base):
        return None
    for py in sorted(os.listdir(base)):
        q = os.path.join(base, py, 'site-packages', 'PyQt5', 'Qt5', 'lib')
        if os.path.isdir(q):
            return q
    return None

_QT_LIB = _find_qt_lib()
if _QT_LIB:
    os.environ['LD_LIBRARY_PATH'] = _QT_LIB + os.pathsep + os.environ.get('LD_LIBRARY_PATH', '')

from PyQt5.QtCore import Qt
from PyQt5.QtWidgets import QApplication

from app import MainWindow


def main():
    QApplication.setAttribute(Qt.AA_EnableHighDpiScaling, True)
    QApplication.setAttribute(Qt.AA_UseHighDpiPixmaps, True)
    app = QApplication(sys.argv)
    win = MainWindow()
    win.show()
    sys.exit(app.exec_())


if __name__ == '__main__':
    main()
