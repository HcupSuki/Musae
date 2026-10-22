# -*- coding: utf-8 -*-
"""左侧控制面板控件：双滑块、滑杆+数字、场景预设行、三个标签页面板。"""
from PyQt5.QtCore import Qt, pyqtSignal
from PyQt5.QtGui import QPainter, QColor, QPen
from PyQt5.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, QLabel, QPushButton,
                             QComboBox, QCheckBox, QDoubleSpinBox, QSlider,
                             QListWidget, QListWidgetItem, QPlainTextEdit, QGroupBox,
                             QFrame)


# ================================================================ 基础控件

def make_group(title, layout):
    g = QGroupBox(title)
    g.setLayout(layout)
    return g


class RangeSlider(QWidget):
    """双端滑块（配合两个 QDoubleSpinBox 使用）。"""
    rangeChanged = pyqtSignal(float, float)

    def __init__(self, parent=None):
        super().__init__(parent)
        self._lo, self._hi = 0.0, 1.0
        self._span_lo, self._span_hi = 0.0, 1.0
        self._drag = None
        self.setFixedHeight(26)
        self.setMouseTracking(True)

    def set_span(self, lo, hi):
        self._span_lo, self._span_hi = float(lo), float(hi)
        if self._span_hi <= self._span_lo:
            self._span_hi = self._span_lo + 1e-6
        self._lo = min(max(self._lo, self._span_lo), self._span_hi)
        self._hi = min(max(self._hi, self._span_lo), self._span_hi)
        self.update()

    def set_values(self, lo, hi, emit=False):
        self._lo = min(max(float(lo), self._span_lo), self._span_hi)
        self._hi = min(max(float(hi), self._span_lo), self._span_hi)
        self.update()
        if emit:
            self.rangeChanged.emit(self._lo, self._hi)

    def values(self):
        return (self._lo, self._hi)

    def _val2x(self, v):
        m = 8
        w = max(1, self.width() - 2 * m)
        return m + (v - self._span_lo) / (self._span_hi - self._span_lo) * w

    def _x2val(self, x):
        m = 8
        w = max(1, self.width() - 2 * m)
        t = (x - m) / w
        t = min(max(t, 0.0), 1.0)
        return self._span_lo + t * (self._span_hi - self._span_lo)

    def paintEvent(self, ev):
        p = QPainter(self)
        p.setRenderHint(QPainter.Antialiasing)
        x1, x2 = self._val2x(self._lo), self._val2x(self._hi)
        # 轨道
        p.setPen(QPen(QColor('#c0c0c0'), 4))
        p.drawLine(8, self.height() // 2, self.width() - 8, self.height() // 2)
        # 已选区间
        p.setPen(QPen(QColor('#4a90d9'), 4))
        p.drawLine(int(x1), self.height() // 2, int(x2), self.height() // 2)
        # 两个手柄
        for x in (x1, x2):
            p.setBrush(QColor('#ffffff'))
            p.setPen(QPen(QColor('#4a90d9'), 2))
            p.drawRect(int(x) - 5, 2, 10, self.height() - 4)
        p.end()

    def _hit(self, pos):
        x = pos.x()
        if abs(x - self._val2x(self._lo)) <= 9:
            return 'lo'
        if abs(x - self._val2x(self._hi)) <= 9:
            return 'hi'
        return None

    def mousePressEvent(self, ev):
        if ev.button() == Qt.LeftButton:
            self._drag = self._hit(ev.pos())

    def mouseMoveEvent(self, ev):
        if self._drag is None:
            return
        v = self._x2val(ev.x())
        if self._drag == 'lo':
            self._lo = min(v, self._hi)
        else:
            self._hi = max(v, self._lo)
        self.update()
        self.rangeChanged.emit(self._lo, self._hi)

    def mouseReleaseEvent(self, ev):
        self._drag = None


class SliderSpin(QWidget):
    """滑杆 + 数字输入（线性映射到 [lo, hi]）。"""
    valueChanged = pyqtSignal(float)

    def __init__(self, lo=0.0, hi=1.0, val=0.0, decimals=3, step=None, parent=None):
        super().__init__(parent)
        self._lo, self._hi = float(lo), float(hi)
        lay = QHBoxLayout(self)
        lay.setContentsMargins(0, 0, 0, 0)
        self.slider = QSlider(Qt.Horizontal)
        self.slider.setRange(0, 1000)
        self.slider.setFixedWidth(130)
        self.spin = QDoubleSpinBox()
        self.spin.setRange(-1e12, 1e12)
        self.spin.setDecimals(decimals)
        self.spin.setSingleStep(step if step is not None else (hi - lo) / 100.0)
        self.spin.setFixedWidth(105)
        self.spin.setValue(val)
        lay.addWidget(self.slider)
        lay.addWidget(self.spin)
        self.slider.valueChanged.connect(self._slider_moved)
        self.spin.valueChanged.connect(self._spin_moved)
        self._sync_slider()

    def set_span(self, lo, hi, step=None):
        self._lo, self._hi = float(lo), float(hi)
        if step is not None:
            self.spin.setSingleStep(step)
        self._sync_slider()

    def set_value(self, v, emit=False):
        self.spin.blockSignals(True)
        self.spin.setValue(v)
        self.spin.blockSignals(False)
        self._sync_slider()
        if emit:
            self.valueChanged.emit(self.value())

    def value(self):
        return self.spin.value()

    def _sync_slider(self):
        t = (self.spin.value() - self._lo) / (self._hi - self._lo) if self._hi > self._lo else 0.0
        self.slider.blockSignals(True)
        self.slider.setValue(int(round(min(max(t, 0.0), 1.0) * 1000)))
        self.slider.blockSignals(False)

    def _slider_moved(self, iv):
        v = self._lo + (self._hi - self._lo) * iv / 1000.0
        self.spin.blockSignals(True)
        self.spin.setValue(v)
        self.spin.blockSignals(False)
        self.valueChanged.emit(v)

    def _spin_moved(self, v):
        self._sync_slider()
        self.valueChanged.emit(v)


def parse_detectors(text):
    """解析手动探测器输入：每行 "名称 x y z"。"""
    dets = []
    for line in text.splitlines():
        parts = line.split()
        if len(parts) == 4:
            try:
                dets.append({'name': parts[0],
                             'position': [float(x) for x in parts[1:4]]})
            except ValueError:
                continue
    return dets


class PresetRow(QWidget):
    """场景预设下拉框 + 手动探测点编辑器（自定义时显示）。"""
    presetChanged = pyqtSignal(str)
    detectorsChanged = pyqtSignal(list)

    def __init__(self, presets, parent=None):
        super().__init__(parent)
        lay = QVBoxLayout(self)
        lay.setContentsMargins(0, 0, 0, 0)
        row = QHBoxLayout()
        row.addWidget(QLabel('场景预设:'))
        self.combo = QComboBox()
        self.combo.addItems(presets.names())
        row.addWidget(self.combo, 1)
        lay.addLayout(row)
        self.editor = QPlainTextEdit()
        self.editor.setPlaceholderText('每行一个探测点：名称 x y z\n如: A -242.5 133.5 -51.51')
        self.editor.setFixedHeight(70)
        self.editor.setVisible(False)
        lay.addWidget(self.editor)
        self.combo.currentTextChanged.connect(self._on_preset)
        self.editor.textChanged.connect(
            lambda: self.detectorsChanged.emit(parse_detectors(self.editor.toPlainText())))

    def _on_preset(self, name):
        self.editor.setVisible(name == '自定义')
        self.presetChanged.emit(name)

    def preset(self):
        return self.combo.currentText()

    def detectors(self):
        return parse_detectors(self.editor.toPlainText())

    def set_preset(self, name):
        idx = self.combo.findText(name)
        if idx >= 0:
            self.combo.blockSignals(True)
            self.combo.setCurrentIndex(idx)
            self.combo.blockSignals(False)
        self.editor.setVisible(name == '自定义')


# ================================================================ 密度面板

class DensityPanel(QWidget):
    loadClicked = pyqtSignal()
    cmapChanged = pyqtSignal(str)
    rangeChanged = pyqtSignal(float, float)
    thresholdChanged = pyqtSignal(float)
    transparentToggled = pyqtSignal(bool)
    intervalToggled = pyqtSignal(bool)
    intervalChanged = pyqtSignal(float, float)
    dropToggled = pyqtSignal(bool)
    terrainToggled = pyqtSignal(bool)
    terrainOpacityChanged = pyqtSignal(float)
    detectorsToggled = pyqtSignal(bool)
    extrasToggled = pyqtSignal(bool)
    axesToggled = pyqtSignal(bool)
    presetChanged = pyqtSignal(str)
    detectorsChanged = pyqtSignal(list)

    def __init__(self, presets, parent=None):
        super().__init__(parent)
        lay = QVBoxLayout(self)
        # 加载
        self.btn_load = QPushButton('加载数据 (.npy/.csv)…')
        self.lbl_file = QLabel('未加载')
        self.lbl_file.setWordWrap(True)
        self.lbl_file.setStyleSheet('color: #666;')
        lay.addWidget(self.btn_load)
        lay.addWidget(self.lbl_file)
        lay.addWidget(self._hline())
        # 场景
        self.preset_row = PresetRow(presets)
        lay.addWidget(self.preset_row)
        # 颜色
        row = QHBoxLayout()
        row.addWidget(QLabel('颜色方案:'))
        self.cmap = QComboBox()
        self.cmap.addItems(['RdPu', 'turbo', 'viridis', 'plasma', 'inferno',
                            'magma', 'jet', 'rainbow', 'coolwarm'])
        row.addWidget(self.cmap, 1)
        lay.addLayout(row)
        lay.addWidget(QLabel('密度范围 (g/cm³):'))
        self.range_slider = RangeSlider()
        lay.addWidget(self.range_slider)
        rrow = QHBoxLayout()
        rrow.addWidget(QLabel('min'))
        self.range_min = QDoubleSpinBox()
        self.range_max = QDoubleSpinBox()
        for s in (self.range_min, self.range_max):
            s.setRange(-1e12, 1e12)
            s.setDecimals(3)
            rrow.addWidget(s)
        rrow.addWidget(QLabel('max'))
        lay.addLayout(rrow)
        # 阈值
        lay.addWidget(QLabel('最小阈值 |rho|:'))
        self.threshold = SliderSpin(0, 1, 0)
        lay.addWidget(self.threshold)
        self.chk_transparent = QCheckBox('透明模式（阈值以下半透明显示）')
        lay.addWidget(self.chk_transparent)
        irow = QHBoxLayout()
        self.chk_interval = QCheckBox('隐藏区间 |rho|∈')
        irow.addWidget(self.chk_interval)
        self.interval_lo = QDoubleSpinBox()
        self.interval_hi = QDoubleSpinBox()
        for s in (self.interval_lo, self.interval_hi):
            s.setRange(0, 1e12)
            s.setDecimals(2)
            s.setFixedWidth(70)
        self.interval_lo.setValue(1.5)
        self.interval_hi.setValue(3.2)
        irow.addWidget(self.interval_lo)
        irow.addWidget(self.interval_hi)
        irow.addStretch(1)
        lay.addLayout(irow)
        # 显示
        self.chk_drop = QCheckBox('剔除首层 z（与 notebook 一致）')
        self.chk_drop.setChecked(True)
        lay.addWidget(self.chk_drop)
        lay.addWidget(self._hline())
        self.chk_terrain = QCheckBox('显示真实地形薄壳')
        self.chk_terrain.setChecked(True)
        lay.addWidget(self.chk_terrain)
        trow = QHBoxLayout()
        trow.addWidget(QLabel('  薄壳透明度:'))
        self.terrain_opacity = SliderSpin(0.05, 1.0, 0.35, decimals=2, step=0.05)
        lay.addLayout(trow)
        lay.addWidget(self.terrain_opacity)
        self.chk_detectors = QCheckBox('显示测量点（红点+标签）')
        self.chk_detectors.setChecked(True)
        lay.addWidget(self.chk_detectors)
        self.chk_extras = QCheckBox('显示辅助几何（隧道/边界）')
        self.chk_extras.setChecked(True)
        lay.addWidget(self.chk_extras)
        self.chk_axes = QCheckBox('显示坐标轴角标')
        lay.addWidget(self.chk_axes)
        lay.addStretch(1)
        # 信号
        self.btn_load.clicked.connect(self.loadClicked)
        self.cmap.currentTextChanged.connect(self.cmapChanged)
        self.range_slider.rangeChanged.connect(self._range_slider_moved)
        self.range_min.valueChanged.connect(self._range_spin_moved)
        self.range_max.valueChanged.connect(self._range_spin_moved)
        self.threshold.valueChanged.connect(self.thresholdChanged)
        self.chk_transparent.toggled.connect(self.transparentToggled)
        self.chk_interval.toggled.connect(self.intervalToggled)
        self.interval_lo.valueChanged.connect(self._interval_moved)
        self.interval_hi.valueChanged.connect(self._interval_moved)
        self.chk_drop.toggled.connect(self.dropToggled)
        self.chk_terrain.toggled.connect(self.terrainToggled)
        self.terrain_opacity.valueChanged.connect(self.terrainOpacityChanged)
        self.chk_detectors.toggled.connect(self.detectorsToggled)
        self.chk_extras.toggled.connect(self.extrasToggled)
        self.chk_axes.toggled.connect(self.axesToggled)
        self.preset_row.presetChanged.connect(self.presetChanged)
        self.preset_row.detectorsChanged.connect(self.detectorsChanged)

    def _hline(self):
        f = QFrame()
        f.setFrameShape(QFrame.HLine)
        f.setFrameShadow(QFrame.Sunken)
        return f

    def _range_slider_moved(self, lo, hi):
        self.range_min.blockSignals(True)
        self.range_max.blockSignals(True)
        self.range_min.setValue(lo)
        self.range_max.setValue(hi)
        self.range_min.blockSignals(False)
        self.range_max.blockSignals(False)
        self.rangeChanged.emit(lo, hi)

    def _range_spin_moved(self, _):
        lo, hi = self.range_min.value(), self.range_max.value()
        self.range_slider.blockSignals(True)
        self.range_slider.set_values(lo, hi)
        self.range_slider.blockSignals(False)
        if hi >= lo:
            self.rangeChanged.emit(lo, hi)

    def _interval_moved(self, _):
        self.intervalChanged.emit(self.interval_lo.value(), self.interval_hi.value())

    # ----- 供 app 调用的 setter -----
    def set_file_label(self, text):
        self.lbl_file.setText(text)

    def set_range(self, vmin, vmax):
        self.range_slider.set_span(vmin, vmax)
        self.range_slider.blockSignals(True)
        self.range_slider.set_values(vmin, vmax)
        self.range_slider.blockSignals(False)
        for s, v in ((self.range_min, vmin), (self.range_max, vmax)):
            s.blockSignals(True)
            s.setValue(v)
            s.blockSignals(False)

    def set_threshold_span(self, lo, hi):
        self.threshold.set_span(lo, hi)
        self.threshold.set_value(lo)

    def set_preset(self, name):
        self.preset_row.set_preset(name if name else '自动匹配')

    def preset(self):
        return self.preset_row.preset()

    def detectors(self):
        return self.preset_row.detectors()

    def interval(self):
        return (self.interval_lo.value(), self.interval_hi.value())

    def interval_on(self):
        return self.chk_interval.isChecked()

    def terrain_opacity_value(self):
        return self.terrain_opacity.value()

    def terrain_on(self):
        return self.chk_terrain.isChecked()


# ================================================================ 地形面板

class TerrainPanel(QWidget):
    loadClicked = pyqtSignal()
    medianToggled = pyqtSignal(bool)
    sigmaChanged = pyqtSignal(float)
    holesToggled = pyqtSignal(bool)
    cmapChanged = pyqtSignal(str)
    rangeChanged = pyqtSignal(float, float)
    realToggled = pyqtSignal(bool)
    realOpacityChanged = pyqtSignal(float)
    detectorsToggled = pyqtSignal(bool)
    extrasToggled = pyqtSignal(bool)
    axesToggled = pyqtSignal(bool)
    presetChanged = pyqtSignal(str)
    detectorsChanged = pyqtSignal(list)

    def __init__(self, presets, parent=None):
        super().__init__(parent)
        lay = QVBoxLayout(self)
        self.btn_load = QPushButton('加载地形数据 (.npy/.csv)…')
        self.lbl_file = QLabel('未加载')
        self.lbl_file.setWordWrap(True)
        self.lbl_file.setStyleSheet('color: #666;')
        lay.addWidget(self.btn_load)
        lay.addWidget(self.lbl_file)
        lay.addWidget(self._hline())
        self.preset_row = PresetRow(presets)
        lay.addWidget(self.preset_row)
        # 预处理
        self.chk_median = QCheckBox('中值滤波（size=3，与 notebook 一致）')
        self.chk_median.setChecked(True)
        lay.addWidget(self.chk_median)
        srow = QHBoxLayout()
        srow.addWidget(QLabel('高斯平滑 σ:'))
        self.sigma = QDoubleSpinBox()
        self.sigma.setRange(0, 20)
        self.sigma.setDecimals(1)
        self.sigma.setSingleStep(0.5)
        self.sigma.setValue(0)
        srow.addWidget(self.sigma)
        srow.addWidget(QLabel('(0=关闭)'))
        srow.addStretch(1)
        lay.addLayout(srow)
        self.chk_holes = QCheckBox('无效值(-55)显示为洞')
        self.chk_holes.setChecked(True)
        lay.addWidget(self.chk_holes)
        # 高度范围
        lay.addWidget(QLabel('高度范围 (m):'))
        self.range_slider = RangeSlider()
        lay.addWidget(self.range_slider)
        rrow = QHBoxLayout()
        rrow.addWidget(QLabel('min'))
        self.range_min = QDoubleSpinBox()
        self.range_max = QDoubleSpinBox()
        for s in (self.range_min, self.range_max):
            s.setRange(-1e12, 1e12)
            s.setDecimals(1)
            rrow.addWidget(s)
        rrow.addWidget(QLabel('max'))
        lay.addLayout(rrow)
        row = QHBoxLayout()
        row.addWidget(QLabel('配色:'))
        self.cmap = QComboBox()
        self.cmap.addItems(['灰度', '地形色', 'viridis', 'turbo', 'plasma', 'jet'])
        row.addWidget(self.cmap, 1)
        lay.addLayout(row)
        # 叠加
        lay.addWidget(self._hline())
        self.chk_real = QCheckBox('显示真实地形叠加')
        self.chk_real.setChecked(True)
        lay.addWidget(self.chk_real)
        trow = QHBoxLayout()
        trow.addWidget(QLabel('  透明度:'))
        self.real_opacity = SliderSpin(0.05, 1.0, 0.3, decimals=2, step=0.05)
        lay.addLayout(trow)
        lay.addWidget(self.real_opacity)
        self.chk_detectors = QCheckBox('显示测量点（红点+标签）')
        self.chk_detectors.setChecked(True)
        lay.addWidget(self.chk_detectors)
        self.chk_extras = QCheckBox('显示辅助几何（隧道/边界）')
        self.chk_extras.setChecked(True)
        lay.addWidget(self.chk_extras)
        self.chk_axes = QCheckBox('显示坐标轴角标')
        lay.addWidget(self.chk_axes)
        lay.addStretch(1)
        # 信号
        self.btn_load.clicked.connect(self.loadClicked)
        self.chk_median.toggled.connect(self.medianToggled)
        self.sigma.valueChanged.connect(self.sigmaChanged)
        self.chk_holes.toggled.connect(self.holesToggled)
        self.cmap.currentTextChanged.connect(self.cmapChanged)
        self.range_slider.rangeChanged.connect(self._range_slider_moved)
        self.range_min.valueChanged.connect(self._range_spin_moved)
        self.range_max.valueChanged.connect(self._range_spin_moved)
        self.chk_real.toggled.connect(self.realToggled)
        self.real_opacity.valueChanged.connect(self.realOpacityChanged)
        self.chk_detectors.toggled.connect(self.detectorsToggled)
        self.chk_extras.toggled.connect(self.extrasToggled)
        self.chk_axes.toggled.connect(self.axesToggled)
        self.preset_row.presetChanged.connect(self.presetChanged)
        self.preset_row.detectorsChanged.connect(self.detectorsChanged)

    def _hline(self):
        f = QFrame()
        f.setFrameShape(QFrame.HLine)
        f.setFrameShadow(QFrame.Sunken)
        return f

    def _range_slider_moved(self, lo, hi):
        self.range_min.blockSignals(True)
        self.range_max.blockSignals(True)
        self.range_min.setValue(lo)
        self.range_max.setValue(hi)
        self.range_min.blockSignals(False)
        self.range_max.blockSignals(False)
        self.rangeChanged.emit(lo, hi)

    def _range_spin_moved(self, _):
        lo, hi = self.range_min.value(), self.range_max.value()
        self.range_slider.blockSignals(True)
        self.range_slider.set_values(lo, hi)
        self.range_slider.blockSignals(False)
        if hi >= lo:
            self.rangeChanged.emit(lo, hi)

    def set_file_label(self, text):
        self.lbl_file.setText(text)

    def set_range(self, vmin, vmax):
        self.range_slider.set_span(vmin, vmax)
        self.range_slider.blockSignals(True)
        self.range_slider.set_values(vmin, vmax)
        self.range_slider.blockSignals(False)
        for s, v in ((self.range_min, vmin), (self.range_max, vmax)):
            s.blockSignals(True)
            s.setValue(v)
            s.blockSignals(False)

    def set_preset(self, name):
        self.preset_row.set_preset(name if name else '自动匹配')

    def preset(self):
        return self.preset_row.preset()

    def detectors(self):
        return self.preset_row.detectors()

    def real_on(self):
        return self.chk_real.isChecked()

    def real_opacity_value(self):
        return self.real_opacity.value()


# ================================================================ Opacity 面板

class OpacityPanel(QWidget):
    filesRequested = pyqtSignal()
    removeRequested = pyqtSignal()
    fileVisibilityChanged = pyqtSignal(int, bool)
    radiusChanged = pyqtSignal(float, float)
    gammaChanged = pyqtSignal(float)
    cmapChanged = pyqtSignal(str)
    rangeChanged = pyqtSignal(float, float)
    logToggled = pyqtSignal(bool)
    hideZeroToggled = pyqtSignal(bool)
    subdivChanged = pyqtSignal(int)
    detectorsToggled = pyqtSignal(bool)
    terrainToggled = pyqtSignal(bool)
    terrainOpacityChanged = pyqtSignal(float)
    extrasToggled = pyqtSignal(bool)
    axesToggled = pyqtSignal(bool)
    presetChanged = pyqtSignal(str)
    detectorsChanged = pyqtSignal(list)

    def __init__(self, presets, parent=None):
        super().__init__(parent)
        lay = QVBoxLayout(self)
        self.btn_load = QPushButton('加载 CSV（可多选）…')
        lay.addWidget(self.btn_load)
        self.file_list = QListWidget()
        self.file_list.setFixedHeight(96)
        lay.addWidget(self.file_list)
        self.btn_remove = QPushButton('移除选中文件')
        lay.addWidget(self.btn_remove)
        lay.addWidget(self._hline())
        self.preset_row = PresetRow(presets)
        lay.addWidget(self.preset_row)
        # 半径
        lay.addWidget(QLabel('半球半径 r = r_min + (r_max-r_min)·(v/vmax)^γ'))
        rrow = QHBoxLayout()
        rrow.addWidget(QLabel('r_min'))
        self.r_min = QDoubleSpinBox()
        self.r_min.setRange(0.1, 500)
        self.r_min.setDecimals(1)
        self.r_min.setValue(5.0)
        rrow.addWidget(self.r_min)
        rrow.addWidget(QLabel('r_max'))
        self.r_max = QDoubleSpinBox()
        self.r_max.setRange(0.1, 2000)
        self.r_max.setDecimals(1)
        self.r_max.setValue(40.0)
        rrow.addWidget(self.r_max)
        lay.addLayout(rrow)
        grow = QHBoxLayout()
        grow.addWidget(QLabel('γ:'))
        self.gamma = QComboBox()
        self.gamma.addItem('开方 (0.5)', 0.5)
        self.gamma.addItem('四次方根 (0.25)', 0.25)
        self.gamma.addItem('线性 (1.0)', 1.0)
        grow.addWidget(self.gamma, 1)
        lay.addLayout(grow)
        # 颜色
        row = QHBoxLayout()
        row.addWidget(QLabel('配色:'))
        self.cmap = QComboBox()
        self.cmap.addItems(['viridis', 'turbo', 'plasma', 'inferno', 'magma',
                            'jet', 'rainbow', 'coolwarm'])
        row.addWidget(self.cmap, 1)
        lay.addLayout(row)
        lay.addWidget(QLabel('颜色范围 (g/cm²):'))
        rrow2 = QHBoxLayout()
        rrow2.addWidget(QLabel('min'))
        self.range_min = QDoubleSpinBox()
        self.range_max = QDoubleSpinBox()
        for s in (self.range_min, self.range_max):
            s.setRange(0, 1e9)
            s.setDecimals(1)
            rrow2.addWidget(s)
        rrow2.addWidget(QLabel('max'))
        lay.addLayout(rrow2)
        self.chk_log = QCheckBox('对数颜色刻度')
        lay.addWidget(self.chk_log)
        self.chk_hide_zero = QCheckBox('隐藏 0 值 bin')
        self.chk_hide_zero.setChecked(True)
        lay.addWidget(self.chk_hide_zero)
        srow = QHBoxLayout()
        srow.addWidget(QLabel('每 bin 细分:'))
        self.subdiv = QComboBox()
        self.subdiv.addItems(['2×2', '4×4', '6×6', '8×8'])
        srow.addWidget(self.subdiv)
        srow.addStretch(1)
        lay.addLayout(srow)
        # 显示
        lay.addWidget(self._hline())
        self.chk_detectors = QCheckBox('显示探测器（红球+标签）')
        self.chk_detectors.setChecked(True)
        lay.addWidget(self.chk_detectors)
        self.chk_terrain = QCheckBox('显示真实地形薄壳')
        self.chk_terrain.setChecked(True)
        lay.addWidget(self.chk_terrain)
        trow = QHBoxLayout()
        trow.addWidget(QLabel('  薄壳透明度:'))
        self.terrain_opacity = SliderSpin(0.05, 1.0, 0.35, decimals=2, step=0.05)
        lay.addLayout(trow)
        lay.addWidget(self.terrain_opacity)
        self.chk_extras = QCheckBox('显示辅助几何（隧道/边界）')
        self.chk_extras.setChecked(True)
        lay.addWidget(self.chk_extras)
        self.chk_axes = QCheckBox('显示坐标轴角标')
        lay.addWidget(self.chk_axes)
        lay.addStretch(1)
        # 信号
        self.btn_load.clicked.connect(self.filesRequested)
        self.btn_remove.clicked.connect(self.removeRequested)
        self.file_list.itemChanged.connect(self._item_changed)
        self.r_min.valueChanged.connect(lambda _: self.radiusChanged.emit(self.r_min.value(), self.r_max.value()))
        self.r_max.valueChanged.connect(lambda _: self.radiusChanged.emit(self.r_min.value(), self.r_max.value()))
        self.gamma.currentIndexChanged.connect(
            lambda _: self.gammaChanged.emit(self.gamma.currentData()))
        self.cmap.currentTextChanged.connect(self.cmapChanged)
        self.range_min.valueChanged.connect(self._range_moved)
        self.range_max.valueChanged.connect(self._range_moved)
        self.chk_log.toggled.connect(self.logToggled)
        self.chk_hide_zero.toggled.connect(self.hideZeroToggled)
        self.subdiv.currentIndexChanged.connect(
            lambda _: self.subdivChanged.emit(int(self.subdiv.currentText()[0])))
        self.chk_detectors.toggled.connect(self.detectorsToggled)
        self.chk_terrain.toggled.connect(self.terrainToggled)
        self.terrain_opacity.valueChanged.connect(self.terrainOpacityChanged)
        self.chk_extras.toggled.connect(self.extrasToggled)
        self.chk_axes.toggled.connect(self.axesToggled)
        self.preset_row.presetChanged.connect(self.presetChanged)
        self.preset_row.detectorsChanged.connect(self.detectorsChanged)

    def _hline(self):
        f = QFrame()
        f.setFrameShape(QFrame.HLine)
        f.setFrameShadow(QFrame.Sunken)
        return f

    def _range_moved(self, _):
        lo, hi = self.range_min.value(), self.range_max.value()
        if hi >= lo:
            self.rangeChanged.emit(lo, hi)

    def _item_changed(self, item):
        self.fileVisibilityChanged.emit(self.file_list.row(item), item.checkState() == Qt.Checked)

    def add_file_item(self, name):
        item = QListWidgetItem(name)
        item.setFlags(item.flags() | Qt.ItemIsUserCheckable)
        item.setCheckState(Qt.Checked)
        self.file_list.addItem(item)

    def remove_selected(self):
        for item in self.file_list.selectedItems():
            self.file_list.takeItem(self.file_list.row(item))

    def selected_rows(self):
        return [self.file_list.row(i) for i in self.file_list.selectedItems()]

    def set_range(self, vmin, vmax):
        for s, v in ((self.range_min, vmin), (self.range_max, vmax)):
            s.blockSignals(True)
            s.setValue(v)
            s.blockSignals(False)

    def set_preset(self, name):
        self.preset_row.set_preset(name if name else '自动匹配')

    def preset(self):
        return self.preset_row.preset()

    def detectors(self):
        return self.preset_row.detectors()

    def terrain_on(self):
        return self.chk_terrain.isChecked()

    def terrain_opacity_value(self):
        return self.terrain_opacity.value()
