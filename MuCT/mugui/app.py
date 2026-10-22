# -*- coding: utf-8 -*-
"""MainWindow：QTabWidget（密度/地形/Opacity 三标签页）+ 工具栏 + 状态栏。"""
import os

import numpy as np
from PyQt5.QtCore import Qt
from PyQt5.QtWidgets import (QMainWindow, QTabWidget, QSplitter, QScrollArea,
                             QFileDialog, QMessageBox, QComboBox, QPushButton,
                             QLabel, QToolBar)
from vtkmodules.qt.QVTKRenderWindowInteractor import QVTKRenderWindowInteractor

import loaders
from render import DensityView, TerrainView, OpacityView, ScenePresets
from widgets import DensityPanel, TerrainPanel, OpacityPanel


class MainWindow(QMainWindow):

    def __init__(self):
        super().__init__()
        self.setWindowTitle('MuCT 重建结果可视化')
        self.resize(1560, 960)
        self.presets = ScenePresets()

        self.tabs = QTabWidget()
        self.density_view, self.density_panel = self._make_tab(
            DensityPanel, DensityView, '密度重建')
        self.terrain_view, self.terrain_panel = self._make_tab(
            TerrainPanel, TerrainView, '地形重建')
        self.opacity_view, self.opacity_panel = self._make_tab(
            OpacityPanel, OpacityView, 'Opacity 半球')
        self.setCentralWidget(self.tabs)
        self._build_toolbar()
        self.statusBar().showMessage('就绪：加载数据文件开始（.npy+同名.txt 或 CSV）')

        self._wire_density()
        self._wire_terrain()
        self._wire_opacity()

    # ------------------------------------------------------------ 布局
    def _make_tab(self, panel_cls, view_cls, title):
        panel = panel_cls(self.presets)
        scroll = QScrollArea()
        scroll.setWidget(panel)
        scroll.setWidgetResizable(True)
        scroll.setFixedWidth(410)
        iw = QVTKRenderWindowInteractor(scroll)
        view = view_cls(iw, self.presets)
        iw.Initialize()
        iw.Start()
        splitter = QSplitter(Qt.Horizontal)
        splitter.addWidget(scroll)
        splitter.addWidget(iw)
        splitter.setSizes([410, 1150])
        self.tabs.addTab(splitter, title)
        return view, panel

    def _build_toolbar(self):
        bar = QToolBar('工具')
        bar.setMovable(False)
        self.addToolBar(bar)
        bar.addWidget(QLabel(' 截图分辨率: '))
        self.mag_combo = QComboBox()
        self.mag_combo.addItems(['1x', '2x', '4x'])
        bar.addWidget(self.mag_combo)
        btn_shot = QPushButton('截图 PNG')
        btn_shot.clicked.connect(self._screenshot)
        bar.addWidget(btn_shot)
        bar.addSeparator()
        btn_reset = QPushButton('重置相机')
        btn_reset.clicked.connect(self._reset_camera)
        bar.addWidget(btn_reset)

    def _current_view(self):
        idx = self.tabs.currentIndex()
        return (self.density_view, self.terrain_view, self.opacity_view)[idx]

    # ------------------------------------------------------------ 通用动作
    def _screenshot(self):
        view = self._current_view()
        default = os.path.join(os.path.expanduser('~'), 'mugui_screenshot.png')
        path, _ = QFileDialog.getSaveFileName(self, '保存截图', default, 'PNG (*.png)')
        if not path:
            return
        if not path.endswith('.png'):
            path += '.png'
        mag = int(self.mag_combo.currentText()[0])
        try:
            view.screenshot(path, mag)
            self.statusBar().showMessage(f'已保存截图: {path}', 8000)
        except Exception as e:
            QMessageBox.critical(self, '截图失败', str(e))

    def _reset_camera(self):
        self._current_view().reset_camera()
        self.statusBar().showMessage('已重置相机视角', 3000)

    # ------------------------------------------------------------ 密度页
    def _wire_density(self):
        p, v = self.density_panel, self.density_view
        p.loadClicked.connect(self._load_density)
        p.cmapChanged.connect(v.set_cmap)
        p.rangeChanged.connect(v.set_range)
        p.thresholdChanged.connect(v.set_threshold)
        p.transparentToggled.connect(v.set_transparent)
        p.intervalToggled.connect(lambda on: v.set_interval(on, *p.interval()))
        p.intervalChanged.connect(lambda lo, hi: v.set_interval(p.interval_on(), lo, hi))
        p.dropToggled.connect(v.set_drop_layer)
        p.terrainToggled.connect(lambda on: v.set_terrain(on, p.terrain_opacity_value()))
        p.terrainOpacityChanged.connect(lambda op: v.set_terrain(p.terrain_on(), op))
        p.detectorsToggled.connect(v.set_detectors)
        p.extrasToggled.connect(v.set_extras)
        p.axesToggled.connect(v.set_axes_visible)
        p.presetChanged.connect(
            lambda name: v.apply_preset(name, p.detectors()))
        p.detectorsChanged.connect(
            lambda dets: v.apply_preset(p.preset(), dets) if p.preset() == '自定义' else None)

    def _load_density(self):
        path, _ = QFileDialog.getOpenFileName(
            self, '选择密度重建数据', os.getcwd(), '数据文件 (*.npy *.csv)')
        if not path:
            return
        try:
            if loaders.detect_file_type(path) != 'density':
                raise ValueError('不是密度数据（需要 .npy+.txt 或含 rho 列的 CSV）')
            result = loaders.load_any(path)
            self.density_view.load(result)
        except Exception as e:
            QMessageBox.critical(self, '加载失败', str(e))
            return
        v = self.density_view
        valid = result['rho'] != loaders.INVALID_DENSITY
        n = int(np.count_nonzero(valid))
        p = self.density_panel
        p.set_file_label(f"{os.path.basename(path)}\n"
                         f"网格 {result['rho'].shape[0]}×{result['rho'].shape[1]}×{result['rho'].shape[2]}"
                         f"，有效体素 {n}")
        p.set_range(v._vmin, v._vmax)
        p.set_threshold_span(v._vmin, v._vmax)
        preset = self.presets.match(path)
        p.set_preset(preset)
        self.statusBar().showMessage(
            f"已加载 {path}：有效体素 {n}，rho∈[{v._vmin:.3g}, {v._vmax:.3g}] g/cm³", 8000)

    # ------------------------------------------------------------ 地形页
    def _wire_terrain(self):
        p, v = self.terrain_panel, self.terrain_view
        p.loadClicked.connect(self._load_terrain)
        p.medianToggled.connect(v.set_median)
        p.sigmaChanged.connect(v.set_sigma)
        p.holesToggled.connect(v.set_holes)
        p.cmapChanged.connect(v.set_cmap)
        p.rangeChanged.connect(v.set_range)
        p.realToggled.connect(lambda on: v.set_real(on, p.real_opacity_value()))
        p.realOpacityChanged.connect(lambda op: v.set_real(p.real_on(), op))
        p.detectorsToggled.connect(v.set_detectors)
        p.extrasToggled.connect(v.set_extras)
        p.axesToggled.connect(v.set_axes_visible)
        p.presetChanged.connect(
            lambda name: v.apply_preset(name, p.detectors()))
        p.detectorsChanged.connect(
            lambda dets: v.apply_preset(p.preset(), dets) if p.preset() == '自定义' else None)

    def _load_terrain(self):
        path, _ = QFileDialog.getOpenFileName(
            self, '选择地形重建数据', os.getcwd(), '数据文件 (*.npy *.csv)')
        if not path:
            return
        try:
            kind = loaders.detect_file_type(path)
            if kind not in ('terrain', 'terrain_csv'):
                raise ValueError('不是地形数据（需要 .npy+.txt 或 X,Y,height 列 CSV）')
            result = loaders.load_any(path)
            if result['type'] == 'terrain_csv':
                # 归一化成与 npy 相同的结构（terrain 数组为 (NX,NY)）
                result = {'X': result['X'], 'Y': result['Y'],
                          'terrain': result['Z'].T, 'params': {},
                          'invalid': None, 'source': path, 'type': 'terrain'}
            self.terrain_view.load(result)
        except Exception as e:
            QMessageBox.critical(self, '加载失败', str(e))
            return
        v = self.terrain_view
        p = self.terrain_panel
        t = result['terrain']
        tv = t[np.isfinite(t)]
        p.set_file_label(f"{os.path.basename(path)}\n网格 {t.shape[0]}×{t.shape[1]}"
                         f"（N_X×N_Y）")
        p.set_range(v._vmin, v._vmax)
        preset = self.presets.match(path)
        p.set_preset(preset)
        self.statusBar().showMessage(
            f"已加载 {path}：{t.shape[0]}×{t.shape[1]}，高度∈[{tv.min():.3g}, {tv.max():.3g}] m", 8000)

    # ------------------------------------------------------------ Opacity 页
    def _wire_opacity(self):
        p, v = self.opacity_panel, self.opacity_view
        p.filesRequested.connect(self._load_opacity_files)
        p.removeRequested.connect(self._remove_opacity_files)
        p.fileVisibilityChanged.connect(v.set_file_visible)
        p.radiusChanged.connect(v.set_radius)
        p.gammaChanged.connect(v.set_gamma)
        p.cmapChanged.connect(v.set_cmap)
        p.rangeChanged.connect(v.set_range)
        p.logToggled.connect(v.set_log)
        p.hideZeroToggled.connect(v.set_hide_zero)
        p.subdivChanged.connect(v.set_subdiv)
        p.detectorsToggled.connect(v.set_detectors)
        p.terrainToggled.connect(lambda on: v.set_terrain(on, p.terrain_opacity_value()))
        p.terrainOpacityChanged.connect(lambda op: v.set_terrain(p.terrain_on(), op))
        p.extrasToggled.connect(v.set_extras)
        p.axesToggled.connect(v.set_axes_visible)
        p.presetChanged.connect(
            lambda name: v.apply_preset(name, p.detectors()))
        p.detectorsChanged.connect(
            lambda dets: v.apply_preset(p.preset(), dets) if p.preset() == '自定义' else None)

    def _load_opacity_files(self):
        paths, _ = QFileDialog.getOpenFileNames(
            self, '选择 Opacity CSV（可多选）', os.getcwd(), 'CSV (*.csv)')
        if not paths:
            return
        loaded = []
        for path in paths:
            try:
                if loaders.detect_file_type(path) != 'opacity':
                    raise ValueError('不是 opacity 数据（需要 u,v,w,bincontent 列）')
                self.opacity_view.add_file(path)
                self.opacity_panel.add_file_item(os.path.basename(path))
                loaded.append(path)
            except Exception as e:
                QMessageBox.critical(self, f'加载失败: {os.path.basename(path)}', str(e))
        if loaded:
            v = self.opacity_view
            p = self.opacity_panel
            p.set_range(0.0, v._vmax)
            preset = self.presets.match(loaded[0])
            p.set_preset(preset)
            nfiles = len(self.opacity_view.files)
            self.statusBar().showMessage(
                f'已加载 {nfiles} 个文件，vmax={v._vmax:.3g} g/cm²（颜色上限，半球半径按此归一）', 8000)
        else:
            self.statusBar().showMessage('没有文件被加载', 5000)

    def _remove_opacity_files(self):
        rows = sorted(self.opacity_panel.selected_rows(), reverse=True)
        for r in rows:
            self.opacity_view.remove_file(r)
        self.opacity_panel.remove_selected()
        self.statusBar().showMessage(
            f'当前 {len(self.opacity_view.files)} 个文件', 5000)
