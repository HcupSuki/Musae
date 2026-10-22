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
        self.setWindowTitle('MuCT Reconstruction Viewer')
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
        path, _ = QFileDialog.getSaveFileName(self, 'Save screenshot', default, 'PNG (*.png)')
        if not path:
            return
        if not path.endswith('.png'):
            path += '.png'
        mag = int(self.mag_combo.currentText()[0])
        try:
            view.screenshot(path, mag)
            self.statusBar().showMessage(f'已保存截图: {path}', 8000)
        except Exception as e:
            QMessageBox.critical(self, 'Screenshot failed', str(e))

    def _reset_camera(self):
        self._current_view().reset_camera()
        self.statusBar().showMessage('已重置相机视角', 3000)

    def _load_true_terrain(self, view, panel=None):
        path, _ = QFileDialog.getOpenFileName(
            self, 'Select true terrain CSV', os.getcwd(), 'CSV (*.csv)')
        if not path:
            return
        try:
            if loaders.detect_file_type(path) != 'terrain_csv':
                raise ValueError('不是地形 CSV（需要 X,Y,height 列）')
            view.set_terrain_csv(path)
            if panel is not None:
                self._sync_terrain_label(panel, view)
            self.statusBar().showMessage(f'已加载真实地形: {path}', 8000)
        except Exception as e:
            QMessageBox.critical(self, 'Load failed', str(e))

    @staticmethod
    def _sync_terrain_label(panel, view):
        """把视图当前真实地形状态同步到“场景”区域的小标签。"""
        if panel is None:
            return
        panel.set_terrain_label(getattr(view, 'terrain_csv', None),
                                getattr(view, 'scene_warning', None))

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
        p.terrainRequested.connect(lambda: self._load_true_terrain(v, p))
        p.presetChanged.connect(lambda name: self._on_density_preset(name))
        p.detectorsChanged.connect(
            lambda dets: self._on_density_preset(p.preset(), dets))

    def _on_density_preset(self, name, detectors=None):
        v, p = self.density_view, self.density_panel
        if detectors is None:
            detectors = p.detectors()
        v.apply_preset(name, detectors)
        self._sync_terrain_label(p, v)
        if name == '自定义':
            self.statusBar().showMessage(
                f'已应用 {len(detectors or [])} 个自定义探测器', 5000)

    def _load_density(self):
        path, _ = QFileDialog.getOpenFileName(
            self, 'Select density reconstruction data', os.getcwd(), 'Data files (*.npy *.csv)')
        if not path:
            return
        try:
            if loaders.detect_file_type(path) != 'density':
                raise ValueError('不是密度数据（需要 .npy+.txt 或含 rho 列的 CSV）')
            result = loaders.load_any(path)
            self.density_view.load(result)
        except Exception as e:
            QMessageBox.critical(self, 'Load failed', str(e))
            return
        v = self.density_view
        valid = result['rho'] != loaders.INVALID_DENSITY
        n = int(np.count_nonzero(valid))
        p = self.density_panel
        label = (f"{os.path.basename(path)}\n"
                 f"网格 {result['rho'].shape[0]}×{result['rho'].shape[1]}×{result['rho'].shape[2]}"
                 f"，有效体素 {n}")
        if getattr(v, 'scene_warning', None):
            label += '\n' + v.scene_warning
        p.set_file_label(label)
        p.set_range(v._vmin, v._vmax)
        p.set_threshold_span(v._vmin, v._vmax)
        preset = self.presets.match(path)
        p.set_preset(preset)
        self._sync_terrain_label(p, v)
        msg = (f"已加载 {path}：有效体素 {n}，rho∈[{v._vmin:.3g}, {v._vmax:.3g}] g/cm³")
        if getattr(v, 'scene_warning', None):
            msg += '；' + v.scene_warning
        self.statusBar().showMessage(msg, 15000)

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
        p.axesToggled.connect(v.set_axes_visible)
        p.terrainRequested.connect(lambda: self._load_true_terrain(v, p))
        p.presetChanged.connect(lambda name: self._on_terrain_preset(name))

    def _on_terrain_preset(self, name):
        v, p = self.terrain_view, self.terrain_panel
        v.apply_preset(name)
        self._sync_terrain_label(p, v)

    def _load_terrain(self):
        path, _ = QFileDialog.getOpenFileName(
            self, 'Select terrain reconstruction data', os.getcwd(), 'Data files (*.npy *.csv)')
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
            QMessageBox.critical(self, 'Load failed', str(e))
            return
        v = self.terrain_view
        p = self.terrain_panel
        t = result['terrain']
        tv = t[np.isfinite(t)]
        label = (f"{os.path.basename(path)}\n网格 {t.shape[0]}×{t.shape[1]}（N_X×N_Y）")
        if getattr(v, 'scene_warning', None):
            label += '\n' + v.scene_warning
        p.set_file_label(label)
        p.set_range(v._vmin, v._vmax)
        preset = self.presets.match(path)
        p.set_preset(preset)
        self._sync_terrain_label(p, v)
        msg = (f"已加载 {path}：{t.shape[0]}×{t.shape[1]}，"
               f"高度∈[{tv.min():.3g}, {tv.max():.3g}] m")
        if getattr(v, 'scene_warning', None):
            msg += '；' + v.scene_warning
        self.statusBar().showMessage(msg, 15000)

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
        p.detectorSizeChanged.connect(v.set_detector_radius)
        p.terrainToggled.connect(lambda on: v.set_terrain(on, p.terrain_opacity_value()))
        p.terrainOpacityChanged.connect(lambda op: v.set_terrain(p.terrain_on(), op))
        p.extrasToggled.connect(v.set_extras)
        p.axesToggled.connect(v.set_axes_visible)
        p.terrainRequested.connect(lambda: self._load_true_terrain(v, p))
        p.presetChanged.connect(lambda name: self._on_opacity_preset(name))
        p.detectorsChanged.connect(
            lambda dets: self._on_opacity_preset(p.preset(), dets))

    def _on_opacity_preset(self, name, detectors=None):
        v, p = self.opacity_view, self.opacity_panel
        if detectors is None:
            detectors = p.detectors()
        v.apply_preset(name, detectors)
        self._sync_terrain_label(p, v)
        if name == '自定义':
            self.statusBar().showMessage(
                f'已应用 {len(detectors or [])} 个自定义探测器', 5000)

    def _load_opacity_files(self):
        paths, _ = QFileDialog.getOpenFileNames(
            self, 'Select opacity CSV (multi-select)', os.getcwd(), 'CSV (*.csv)')
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
                QMessageBox.critical(self, f'Load failed: {os.path.basename(path)}', str(e))
        if loaded:
            v = self.opacity_view
            p = self.opacity_panel
            p.set_range(0.0, v._vmax)
            if getattr(v, 'radius_autoset', False):
                p.set_radius(v._r_min, v._r_max)
                v.radius_autoset = False
            if getattr(v, 'detector_autoset', False):
                p.set_detector_size(v._detector_radius)
                v.detector_autoset = False
            # 不再根据文件名强行改写预设下拉框：
            # 用户选了「自定义」/某个预设就保持，只有「自动匹配」才由 view 自己匹配。
            active = p.preset()
            matched = self.presets.match(loaded[0])
            self._sync_terrain_label(p, v)
            nfiles = len(self.opacity_view.files)
            msg = (f'已加载 {nfiles} 个文件，数据 vmax={v._vmax:.3g} g/cm²'
                   f'（柱状体长度按此归一，颜色上限可单独调）')
            if active == '自动匹配':
                if matched:
                    msg += f'；自动匹配场景: {matched}'
                else:
                    msg += '；未匹配预设，探测器原点按 (0,0,0)'
            elif active == '自定义':
                msg += '；使用自定义探测器'
            warns = [w for w in (getattr(v, 'origin_warning', None),
                                 getattr(v, 'scene_warning', None)) if w]
            for warn in warns:
                msg += '；' + warn
            # 有警告时保持显示，避免一闪而过
            self.statusBar().showMessage(msg, 0 if warns else 15000)
        else:
            self.statusBar().showMessage('没有文件被加载', 5000)

    def _remove_opacity_files(self):
        p = self.opacity_panel
        rows = sorted(p.selected_rows(), reverse=True)
        if not rows:
            # 某些样式下点复选框不会选中行；退化为移除当前行
            cur = p.file_list.currentRow()
            if cur >= 0:
                rows = [cur]
        if not rows:
            self.statusBar().showMessage(
                '请先在文件列表中选择要移除的文件（点文件名或复选框）', 5000)
            return
        n = len(rows)
        for r in rows:
            self.opacity_view.remove_file(r)
        p.remove_selected()
        p.set_range(0.0, self.opacity_view._vmax)
        self.statusBar().showMessage(
            f'已移除 {n} 个文件，当前 {len(self.opacity_view.files)} 个，'
            f'vmax={self.opacity_view._vmax:.3g}', 5000)
