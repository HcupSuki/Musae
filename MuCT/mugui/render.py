# -*- coding: utf-8 -*-
"""VTK 渲染管线：三个视图（密度体素 / 地形曲面 / Opacity 半球）+ 场景元素。

- 体素用 numpy 向量化构建 vtkUnstructuredGrid（六面体），阈值过滤通过
  numpy 原位更新 keep/ghost 掩码数组实现，拖动滑块无需重建网格。
- 地形曲面：vtkStructuredGrid 直接以高度作 z（与 notebook 的 warp 等价），
  无效值置 NaN 显示为洞。
- Opacity 半球：每 bin 一个球面四边形（可细分），半径与数值成正比。
"""
import json
import os

import numpy as np
import vtk
from vtk.util import numpy_support as nps

try:
    import matplotlib
    import matplotlib.colors as mcolors

    def _get_cmap(name):
        if hasattr(matplotlib, 'colormaps'):  # mpl >= 3.6
            return matplotlib.colormaps[name]
        return matplotlib.cm.get_cmap(name)
except ImportError:
    _get_cmap = None

import loaders

BG_COLOR = (1.0, 1.0, 1.0)
CJK_FAMILY = 'Noto Sans CJK SC'
CJK_FALLBACK = '/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc'

CMAP_OPTIONS = ('RdPu', 'turbo', 'viridis', 'plasma', 'inferno', 'magma',
                'jet', 'rainbow', 'coolwarm', 'terrain', 'gray')


# ================================================================ 小工具

def make_points(nparr):
    """numpy (N,3) → vtkPoints（共享内存，不复制）。"""
    vpts = vtk.vtkPoints()
    vpts.SetDataTypeToDouble()
    vpts.SetNumberOfPoints(len(nparr))
    nps.vtk_to_numpy(vpts.GetData())[:] = nparr
    return vpts


def set_cjk(tprop, size=14, color=(0.0, 0.0, 0.0)):
    """给 vtkTextProperty 设置中文可用字体。"""
    tprop.SetFontFamilyAsString(CJK_FAMILY)
    if os.path.exists(CJK_FALLBACK):
        tprop.SetFontFile(CJK_FALLBACK)
    tprop.SetColor(*color)
    if size:
        tprop.SetFontSize(size)
    return tprop


def make_lut(name, vmin, vmax, log=False):
    """配色方案 → vtkLookupTable（matplotlib 色图统一路径）。"""
    if _get_cmap is None:
        name = 'viridis'
    try:
        cmap = _get_cmap(name)
    except (KeyError, ValueError):
        cmap = _get_cmap('viridis')
    if vmax <= vmin:
        # 数据范围退化（如平坦地形）：扩 1 单位，让颜色落在色图中段
        vmin, vmax = vmin - 1.0, vmax + 1.0
    colors = cmap(np.linspace(0.0, 1.0, 256))[:, :3]
    lut = vtk.vtkLookupTable()
    lut.SetNumberOfTableValues(len(colors))
    for i, c in enumerate(colors):
        lut.SetTableValue(i, c[0], c[1], c[2], 1.0)
    lut.SetRange(vmin, vmax)
    if log:
        lut.SetScaleToLog10()
    lut.Build()
    return lut


def make_scalar_bar(title, lut, color=(0.0, 0.0, 0.0)):
    """中文标题的标量条。"""
    bar = vtk.vtkScalarBarActor()
    bar.SetLookupTable(lut)
    bar.SetTitle(title)
    set_cjk(bar.GetTitleTextProperty(), size=16, color=color)
    lp = bar.GetLabelTextProperty()
    lp.SetColor(*color)
    lp.SetFontSize(13)
    lp.SetFontFamilyAsString('Arial')
    bar.SetNumberOfLabels(5)
    bar.SetLabelFormat('%-#6.3g')
    bar.SetWidth(0.09)
    bar.SetHeight(0.55)
    bar.SetPosition(0.88, 0.18)
    bar.SetBarRatio(0.3)
    bar.UnconstrainedFontSizeOn()
    return bar


def make_threshold(ugrid, array_name):
    """vtkThreshold：保留 array_name 值为 1 的 cell。"""
    t = vtk.vtkThreshold()
    t.SetInputData(ugrid)
    t.SetInputArrayToProcess(0, 0, 0, vtk.vtkDataObject.FIELD_ASSOCIATION_CELLS, array_name)
    t.SetThresholdFunction(vtk.vtkThreshold.THRESHOLD_BETWEEN)
    t.SetLowerThreshold(0.5)
    t.SetUpperThreshold(1.5)
    return t


# ================================================================ 体素网格

def build_voxel_grid(X, Y, Z, rho):
    """体素中心网格 → vtkUnstructuredGrid（六面体单元）。

    返回 (ugrid, arrays)：arrays 为 numpy 掩码数组的 dict
    （rho/abs/keep/ghost，与 ugrid 共享内存，可原位更新）。
    """
    nx, ny, nz = rho.shape
    valid = np.isfinite(rho) & (rho != loaders.INVALID_DENSITY)
    idx = np.argwhere(valid)  # (n,3): i,j,k
    if len(idx) == 0:
        raise ValueError('没有有效体素可显示')
    n = len(idx)
    i, j, k = idx[:, 0].astype(np.int64), idx[:, 1].astype(np.int64), idx[:, 2].astype(np.int64)
    # 六面体角点（规则格点）序号: node = i*(ny+1)*(nz+1) + j*(nz+1) + k
    di = np.array([0, 1, 1, 0, 0, 1, 1, 0], np.int64)
    dj = np.array([0, 0, 1, 1, 0, 0, 1, 1], np.int64)
    dk = np.array([0, 0, 0, 0, 1, 1, 1, 1], np.int64)
    sy, sz = (ny + 1), (nz + 1)
    base = i * (sy * sz) + j * sz + k
    nodes = base[:, None] + (di * (sy * sz) + dj * sz + dk)[None, :]  # (n,8)
    uniq, inv = np.unique(nodes.ravel(), return_inverse=True)
    conn = inv.reshape(n, 8)
    nk = uniq % sz
    nj = (uniq // sz) % sy
    ni = uniq // (sy * sz)
    dx = X[1] - X[0] if len(X) > 1 else 1.0
    dy = Y[1] - Y[0] if len(Y) > 1 else 1.0
    dz = Z[1] - Z[0] if len(Z) > 1 else 1.0
    pts = np.stack([X[0] - dx / 2 + ni * dx,
                    Y[0] - dy / 2 + nj * dy,
                    Z[0] - dz / 2 + nk * dz], axis=1)

    legacy = np.empty((n, 9), np.int64)
    legacy[:, 0] = 8
    legacy[:, 1:] = conn
    cell_ids = nps.numpy_to_vtkIdTypeArray(legacy.ravel(), deep=False)
    cells = vtk.vtkCellArray()
    cells.SetCells(n, cell_ids)

    ug = vtk.vtkUnstructuredGrid()
    ug.SetPoints(make_points(pts))
    ug.SetCells(vtk.VTK_HEXAHEDRON, cells)

    rho_v = rho[valid]
    arrays = {}
    for name, arr in (('rho', rho_v.astype(np.float64)),
                      ('abs', np.abs(rho_v)),
                      ('keep', np.ones(n, np.int8)),
                      ('ghost', np.zeros(n, np.int8))):
        va = nps.numpy_to_vtk(arr, deep=False)
        va.SetName(name)
        ug.GetCellData().AddArray(va)
        arrays[name] = arr
    ug.GetCellData().SetScalars(ug.GetCellData().GetArray('rho'))
    return ug, arrays


# ================================================================ 地形曲面

def build_height_surface(X, Y, Z, invalid=None, as_holes=True):
    """高度曲面 → vtkPolyData（四边形）。X:(NX,), Y:(NY,), Z:(NX,NY)。

    npy 下标 [i,j] → (X[i],Y[j])（与 notebook 的 terrain.T 一致）。
    无效值（如 -55）/NaN 所在 cell 不构建 → 显示为洞。
    """
    nx, ny = len(X), len(Y)
    zt = Z.T.astype(np.float64)  # (NY, NX)
    valid = np.isfinite(zt)
    if invalid is not None and as_holes:
        valid &= zt != invalid

    xx = np.tile(np.asarray(X, dtype=np.float64), ny)
    yy = np.repeat(np.asarray(Y, dtype=np.float64), nx)
    zz = zt.ravel()
    pts = np.stack([xx, yy, zz], axis=1)

    if nx < 2 or ny < 2:
        return None
    cell_valid = valid[:-1, :-1] & valid[:-1, 1:] & valid[1:, :-1] & valid[1:, 1:]
    ii, jj = np.where(cell_valid)   # ii=行(y 下标), jj=列(x 下标)
    p = ii * nx + jj                # 点序号 = 行*nx + 列
    quads = np.stack([p, p + 1, p + nx + 1, p + nx], axis=1)

    legacy = np.empty((len(quads), 5), np.int64)
    legacy[:, 0] = 4
    legacy[:, 1:] = quads
    cell_ids = nps.numpy_to_vtkIdTypeArray(legacy.ravel(), deep=False)
    cells = vtk.vtkCellArray()
    cells.SetCells(len(quads), cell_ids)

    pd = vtk.vtkPolyData()
    pd.SetPoints(make_points(pts))
    pd.SetPolys(cells)
    sa = nps.numpy_to_vtk(zz, deep=False)
    sa.SetName('height')
    pd.GetPointData().SetScalars(sa)
    return pd


# ================================================================ Opacity 半球

def build_opacity_grid(result, origin, r_min, r_max, vmax, gamma, subdiv, hide_zero):
    """球面 bin 网格 → vtkUnstructuredGrid（四边形单元，cell scalar=质量厚度）。

    半径: r(v) = r_min + (r_max - r_min) × (v/vmax)^γ
    """
    te = result['theta_edges']
    pe = result['phi_edges']
    vals = result['values']
    nt, np_ = vals.shape
    n = subdiv + 1
    keep_mask = np.ones(vals.shape, dtype=bool)
    if hide_zero:
        keep_mask &= vals != 0
    kept = np.argwhere(keep_mask)
    if len(kept) == 0:
        raise ValueError('没有可显示的 bin（全部为 0？）')

    pts_all, quads_all, scal_all, off = [], [], [], 0
    for ti, pi in kept:
        v = float(vals[ti, pi])
        r = r_min + (r_max - r_min) * (v / vmax) ** gamma
        ths = np.linspace(te[ti], te[ti + 1], n)
        phs = np.linspace(pe[pi], pe[pi + 1], n)
        TH, PH = np.meshgrid(ths, phs, indexing='ij')
        st = np.sin(TH)
        p = np.stack([r * st * np.cos(PH), r * st * np.sin(PH), r * np.cos(TH)],
                     axis=-1).reshape(-1, 3)
        p += np.asarray(origin, dtype=np.float64)
        pts_all.append(p)
        a, b = np.meshgrid(np.arange(subdiv), np.arange(subdiv), indexing='ij')
        q = (a * n + b).ravel()
        quads_all.append(np.stack([q, q + n, q + n + 1, q + 1], axis=1) + off)
        off += n * n
        scal_all.append(np.full(subdiv * subdiv, v))

    pts = np.concatenate(pts_all, axis=0)
    quads = np.concatenate(quads_all, axis=0)
    legacy = np.empty((len(quads), 5), np.int64)
    legacy[:, 0] = 4
    legacy[:, 1:] = quads
    cell_ids = nps.numpy_to_vtkIdTypeArray(legacy.ravel(), deep=False)
    cells = vtk.vtkCellArray()
    cells.SetCells(len(quads), cell_ids)

    ug = vtk.vtkUnstructuredGrid()
    ug.SetPoints(make_points(pts))
    ug.SetCells(vtk.VTK_QUAD, cells)
    sc = nps.numpy_to_vtk(np.concatenate(scal_all), deep=False)
    sc.SetName('v')
    ug.GetCellData().SetScalars(sc)
    return ug


# ================================================================ 场景元素

def build_box_edges(spec):
    """线框长方体（world box 等）。"""
    c = spec['center']
    s = spec['size']
    cube = vtk.vtkCubeSource()
    cube.SetXLength(s[0])
    cube.SetYLength(s[1])
    cube.SetZLength(s[2])
    cube.SetCenter(*c)
    edges = vtk.vtkExtractEdges()
    edges.SetInputConnection(cube.GetOutputPort())
    mapper = vtk.vtkPolyDataMapper()
    mapper.SetInputConnection(edges.GetOutputPort())
    actor = vtk.vtkActor()
    actor.SetMapper(mapper)
    actor.GetProperty().SetColor(*[x / 255.0 for x in spec['color']])
    actor.GetProperty().SetLineWidth(spec.get('line_width', 2))
    return actor


def build_cuboids(spec):
    """旋转长方体组（Tianqin 隧道）。positions 为旋转前左下前角点。"""
    ang = np.deg2rad(spec['angle_deg'])
    cos, sin = np.cos(ang), np.sin(ang)
    pts_all, quads_all = [], []
    off = 0
    for (x, y, z), (lx, ly, lz) in zip(spec['positions'], spec['sizes']):
        bottom = np.array([
            [x, y, z],
            [x + lx * cos, y - lx * sin, z],
            [x + lx * cos + ly * sin, y - lx * sin + ly * cos, z],
            [x + ly * sin, y + ly * cos, z],
        ])
        top = bottom + [0, 0, lz]
        pts = np.vstack([bottom, top])
        pts_all.append(pts)
        v = np.arange(8) + off
        quads_all.append(np.array([
            [v[0], v[1], v[2], v[3]],      # 底面
            [v[4], v[7], v[6], v[5]],      # 顶面
            [v[0], v[4], v[5], v[1]],      # 前
            [v[1], v[5], v[6], v[2]],      # 右
            [v[2], v[6], v[7], v[3]],      # 后
            [v[3], v[7], v[4], v[0]],      # 左
        ]))
        off += 8
    pts = np.concatenate(pts_all, axis=0)
    quads = np.concatenate(quads_all, axis=0)
    legacy = np.empty((len(quads), 5), np.int64)
    legacy[:, 0] = 4
    legacy[:, 1:] = quads
    cell_ids = nps.numpy_to_vtkIdTypeArray(legacy.ravel(), deep=False)
    cells = vtk.vtkCellArray()
    cells.SetCells(len(quads), cell_ids)

    pd = vtk.vtkPolyData()
    pd.SetPoints(make_points(pts))
    pd.SetPolys(cells)
    mapper = vtk.vtkPolyDataMapper()
    mapper.SetInputData(pd)
    actor = vtk.vtkActor()
    actor.SetMapper(mapper)
    actor.GetProperty().SetColor(*[x / 255.0 for x in spec['color']])
    actor.GetProperty().SetOpacity(spec.get('opacity', 0.5))
    actor.GetProperty().SetEdgeVisibility(1)
    actor.GetProperty().SetEdgeColor(0.0, 0.0, 0.0)
    actor.GetProperty().SetLineWidth(1)
    return actor


def build_detector_actors(detectors, radius, font_size):
    """探测器红点 + 名称标签（billboard）。"""
    actors, labels = [], []
    for d in detectors:
        pos = d['position']
        name = d.get('name', '')
        sphere = vtk.vtkSphereSource()
        sphere.SetRadius(radius)
        sphere.SetThetaResolution(20)
        sphere.SetPhiResolution(20)
        sphere.SetCenter(*pos)
        mapper = vtk.vtkPolyDataMapper()
        mapper.SetInputConnection(sphere.GetOutputPort())
        actor = vtk.vtkActor()
        actor.SetMapper(mapper)
        actor.GetProperty().SetColor(0.85, 0.1, 0.1)
        actor.GetProperty().SetSpecular(0.4)
        actors.append(actor)
        if name:
            label = vtk.vtkBillboardTextActor3D()
            label.SetInput(name)
            tp = label.GetTextProperty()
            set_cjk(tp, size=font_size, color=(0.15, 0.15, 0.15))
            tp.SetBold(1)
            tp.SetJustificationToCentered()
            label.SetPosition(pos[0], pos[1], pos[2] + radius * 2.5)
            labels.append(label)
    return actors, labels


def build_terrain_shell(csv_path, lut, opacity):
    """真实地形薄壳（半透明曲面）。"""
    if not csv_path or not os.path.exists(csv_path):
        return None
    res = loaders.load_terrain_csv(csv_path)
    pd = build_height_surface(res['X'], res['Y'], res['Z'].T, invalid=None)
    if pd is None:
        return None
    mapper = vtk.vtkPolyDataMapper()
    mapper.SetInputData(pd)
    mapper.SetLookupTable(lut)
    mapper.SetScalarRange(*lut.GetRange())
    mapper.ScalarVisibilityOn()
    actor = vtk.vtkActor()
    actor.SetMapper(mapper)
    actor.GetProperty().SetOpacity(opacity)
    return actor


# ================================================================ 场景预设

class ScenePresets:
    """读取 scene_presets.json；按文件名自动匹配预设。"""

    def __init__(self, json_path=None):
        if json_path is None:
            json_path = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                     'scene_presets.json')
        self.root = os.path.dirname(os.path.dirname(os.path.abspath(json_path)))
        with open(json_path, 'r', encoding='utf-8') as f:
            self.data = json.load(f)

    def names(self):
        return ['自动匹配'] + list(self.data.keys()) + ['自定义']

    def match(self, path):
        base = os.path.basename(path)
        if 'SimBox' in base and 'SimBox' in self.data:
            return 'SimBox'
        if 'Tianqin' in base and 'Tianqin' in self.data:
            return 'Tianqin'
        return None

    def get(self, name):
        """返回预设 dict（terrain_csv 解析为绝对路径）；自定义返回 None。"""
        if name not in self.data:
            return None
        spec = dict(self.data[name])
        tc = spec.get('terrain_csv')
        if tc:
            spec['terrain_csv'] = os.path.join(self.root, tc)
        return spec

    def detector_map(self, name):
        spec = self.get(name)
        if not spec:
            return {}
        return {d['name']: d['position'] for d in spec.get('detectors', [])}


# ================================================================ 视图基类

class ViewBase:
    """每个标签页一个：vtkRenderer + 默认相机 + 坐标轴角标 + 截图。"""

    def __init__(self, interactor, presets=None):
        self.interactor = interactor
        self.presets = presets
        self.rw = interactor.GetRenderWindow()
        self.renderer = vtk.vtkRenderer()
        self.renderer.SetBackground(*BG_COLOR)
        # 注意：不开启深度剥离。llvmpipe 等软渲染下开启后大透明曲面（真实地形
        # 880×577）会整体消失；VTK 默认的按深度排序混合已满足本应用需求。
        self.rw.AddRenderer(self.renderer)
        style = vtk.vtkInteractorStyleTrackballCamera()
        interactor.SetInteractorStyle(style)
        self._xlim = self._ylim = self._zlim = None
        self._span = 100.0
        # 坐标轴角标
        self.axes_actor = vtk.vtkAxesActor()
        self.axes_widget = vtk.vtkOrientationMarkerWidget()
        self.axes_widget.SetOrientationMarker(self.axes_actor)
        self.axes_widget.SetInteractor(interactor)
        self.axes_widget.SetViewport(0.0, 0.0, 0.18, 0.18)
        self.axes_widget.SetInteractive(0)
        self.axes_widget.SetEnabled(0)

    def render(self):
        self.rw.Render()

    # ----- 相机 -----
    def set_view_range(self, xlim, ylim, zlim, reset_camera=False):
        self._xlim, self._ylim, self._zlim = xlim, ylim, zlim
        self._span = max(xlim[1] - xlim[0], ylim[1] - ylim[0], zlim[1] - zlim[0])
        if reset_camera or not self._cam_initialized():
            self.reset_camera()
        else:
            self.renderer.ResetCameraClippingRange()

    def _cam_initialized(self):
        cam = self.renderer.GetActiveCamera()
        pos = cam.GetPosition()
        foc = cam.GetFocalPoint()
        return not (pos == (0.0, 0.0, 1.0) and foc == (0.0, 0.0, 0.0))

    def reset_camera(self):
        """默认视角：从 +Z 上方斜视视野中心，view_up=(0,0,1)。"""
        if self._xlim is None:
            return
        cx = (self._xlim[0] + self._xlim[1]) / 2
        cy = (self._ylim[0] + self._ylim[1]) / 2
        cz = (self._zlim[0] + self._zlim[1]) / 2
        s = self._span
        cam = self.renderer.GetActiveCamera()
        cam.SetFocalPoint(cx, cy, cz)
        cam.SetPosition(cx + 0.3 * s, cy + 0.35 * s, cz + 1.15 * s)
        cam.SetViewUp(0.0, 0.0, 1.0)
        cam.SetClippingRange(0.01 * s, 12.0 * s)
        self.renderer.ResetCameraClippingRange()
        self.render()

    # ----- 截图 -----
    def screenshot(self, path, mag=1):
        self.rw.Render()
        w2i = vtk.vtkWindowToImageFilter()
        w2i.SetInput(self.rw)
        w2i.SetScale(mag)
        w2i.SetInputBufferTypeToRGB()
        w2i.ReadFrontBufferOff()
        writer = vtk.vtkPNGWriter()
        writer.SetInputConnection(w2i.GetOutputPort())
        writer.SetFileName(path)
        writer.Write()

    def set_axes_visible(self, on):
        self.axes_widget.SetEnabled(1 if on else 0)
        self.render()


# ================================================================ 密度视图

class DensityView(ViewBase):
    """标签页 1：密度体素重建。"""

    def __init__(self, interactor, presets):
        super().__init__(interactor, presets)
        self.result = None
        self.grids = {}          # drop_layer → (ugrid, arrays)
        self.drop_layer = True
        self._active = None
        self._filter_keep = None
        self._filter_ghost = None
        self._lut = make_lut('RdPu', 0.0, 1.0)
        self._bar = make_scalar_bar('密度 (g/cm³)', self._lut)
        self.renderer.AddViewProp(self._bar)
        self._actor_keep = None
        self._actor_ghost = None
        self.scene_actors = []   # 探测器/辅助几何/地形薄壳
        self._cmap = 'RdPu'
        self._t_min = None       # 最小阈值（None=数据最小值，不过滤）
        self._transparent = False
        self._interval_on = False
        self._interval = (1.5, 3.2)
        self._terrain_on = True
        self._terrain_opacity = 0.35
        self._detectors_on = True
        self._extras_on = True
        self._detectors = []

    # ----- 数据加载 -----
    def load(self, result):
        self.result = result
        self.renderer.RemoveAllViewProps()
        self.renderer.AddViewProp(self._bar)
        self.scene_actors = []

        X, Y, Z, rho = result['X'], result['Y'], result['Z'], result['rho']
        self.grids = {}
        for drop in (False, True):
            r, zz = rho, Z
            if drop:
                r, zz = r[:, :, 1:], Z[1:]
            self.grids[drop] = build_voxel_grid(X, Y, zz, r)
        ug, arrays = self.grids[True]
        self._active = arrays
        self._filter_keep = make_threshold(ug, 'keep')
        self._filter_ghost = make_threshold(ug, 'ghost')

        valid = rho[np.isfinite(rho) & (rho != loaders.INVALID_DENSITY)]
        vmin, vmax = float(valid.min()), float(valid.max())
        self._vmin, self._vmax = vmin, vmax
        self._t_min = vmin
        self._lut = make_lut(self._cmap, vmin, vmax)
        self._apply_lut()

        for filt, op in ((self._filter_keep, 1.0), (self._filter_ghost, 0.15)):
            mapper = vtk.vtkDataSetMapper()
            mapper.SetInputConnection(filt.GetOutputPort())
            mapper.SetLookupTable(self._lut)
            mapper.SetScalarModeToUseCellFieldData()
            mapper.SelectColorArray('rho')
            mapper.SetScalarRange(vmin, vmax)
            actor = vtk.vtkActor()
            actor.SetMapper(mapper)
            actor.GetProperty().SetOpacity(op)
            actor.GetProperty().SetAmbient(0.25)
            actor.GetProperty().SetDiffuse(0.7)
            actor.GetProperty().SetSpecular(0.1)
            if op >= 1.0:
                self._actor_keep = actor
            else:
                self._actor_ghost = actor
            self.renderer.AddActor(actor)
        self._refresh_masks()

        # 探测器默认：预设匹配 → 预设；否则取 txt 参数 MEASUREMENT_LOCATION
        self._detectors = self._default_detectors()
        # 场景
        xlim = [float(X[0]) - (X[1] - X[0]) / 2, float(X[-1]) + (X[1] - X[0]) / 2]
        ylim = [float(Y[0]) - (Y[1] - Y[0]) / 2, float(Y[-1]) + (Y[1] - Y[0]) / 2]
        zlim = [float(Z[0]) - (Z[1] - Z[0]) / 2, float(Z[-1]) + (Z[1] - Z[0]) / 2]
        if self.presets is not None:
            preset = self.presets.match(result['source'])
            if preset:
                spec = self.presets.get(preset)
                vr = spec['view_range']
                self.set_view_range(vr['xlim'], vr['ylim'], vr['zlim'], reset_camera=True)
                self._build_scene(spec)
                return
        self.set_view_range(xlim, ylim, zlim, reset_camera=True)
        self._refresh_scene()
        self.render()

    def _default_detectors(self):
        ml = self.result['params'].get('MEASUREMENT_LOCATION')
        if not ml:
            return []
        names = ['A', 'B', 'C', 'D', 'E', 'F', 'G', 'H']
        return [{'name': names[i] if i < len(names) else str(i + 1),
                 'position': list(p)} for i, p in enumerate(ml)]

    def _build_scene(self, spec):
        """按预设重建探测器/辅助几何/地形薄壳（相机不动）。"""
        for _, a in self.scene_actors:
            self.renderer.RemoveActor(a)
        self.scene_actors = []
        self._detectors = spec.get('detectors', [])
        for extra in spec.get('extras', []):
            if extra['type'] == 'box_edges':
                actor = build_box_edges(extra)
            elif extra['type'] == 'cuboids':
                actor = build_cuboids(extra)
            else:
                continue
            self.scene_actors.append(('extra', actor))
        # 地形薄壳
        tc = spec.get('terrain_csv')
        if tc and os.path.exists(tc):
            res = loaders.load_terrain_csv(tc)
            hmin = float(np.nanmin(res['Z']))
            hmax = float(np.nanmax(res['Z']))
            lut = make_lut('terrain', hmin, hmax)
            actor = build_terrain_shell(tc, lut, self._terrain_opacity)
            if actor is not None:
                self.scene_actors.append(('terrain', actor))
        self._refresh_scene()
        self.render()

    def _refresh_scene(self):
        # 移除旧的探测器/标签演员（幂等），再按当前状态重建
        for kind, a in self.scene_actors:
            if kind in ('detector', 'label'):
                self.renderer.RemoveActor(a)
        self.scene_actors = [(k, a) for k, a in self.scene_actors
                             if k not in ('detector', 'label')]
        radius = self._span * 0.012
        fs = max(10, int(self._span * 0.03))
        actors, labels = build_detector_actors(self._detectors, radius, fs)
        for a, lab in zip(actors, labels):
            self.scene_actors.append(('detector', a))
            self.scene_actors.append(('label', lab))
        for kind, a in self.scene_actors:
            if kind in ('extra', 'terrain', 'detector', 'label'):
                vis = {'extra': self._extras_on, 'terrain': self._terrain_on,
                       'detector': self._detectors_on, 'label': self._detectors_on}[kind]
                a.SetVisibility(1 if vis else 0)
                self.renderer.AddActor(a)

    # ----- 参数更新 -----
    def _apply_lut(self):
        self._lut.SetRange(self._vmin, self._vmax)
        self._lut.Build()
        for actor in (self._actor_keep, self._actor_ghost):
            if actor is not None:
                actor.GetMapper().SetScalarRange(self._vmin, self._vmax)
        self.render()

    def _refresh_masks(self):
        if self._active is None:
            return
        a = self._active
        absrho = a['abs']
        keep = np.ones(len(absrho), np.int8)
        ghost = np.zeros(len(absrho), np.int8)
        if self._interval_on:
            lo, hi = self._interval
            in_band = (absrho >= lo) & (absrho <= hi)
            keep[in_band] = 0
        below = absrho < self._t_min
        if self._transparent:
            ghost[below] = 1
        else:
            keep[below] = 0
        np.copyto(a['keep'], keep)
        np.copyto(a['ghost'], ghost)
        ug = self.grids[self.drop_layer][0]
        ug.GetCellData().GetArray('keep').Modified()
        ug.GetCellData().GetArray('ghost').Modified()
        self.render()

    def set_cmap(self, name):
        self._cmap = name
        self._lut = make_lut(name, self._vmin, self._vmax)
        self._bar.SetLookupTable(self._lut)
        for actor in (self._actor_keep, self._actor_ghost):
            if actor is not None:
                actor.GetMapper().SetLookupTable(self._lut)
        self.render()

    def set_range(self, vmin, vmax):
        self._vmin, self._vmax = vmin, vmax
        self._apply_lut()

    def set_threshold(self, t):
        self._t_min = t
        self._refresh_masks()

    def set_transparent(self, on):
        self._transparent = on
        self._refresh_masks()

    def set_interval(self, on, lo=None, hi=None):
        self._interval_on = on
        if lo is not None and hi is not None:
            self._interval = (lo, hi)
        self._refresh_masks()

    def set_drop_layer(self, on):
        if on == self.drop_layer:
            return
        self.drop_layer = on
        ug, arrays = self.grids[on]
        self._active = arrays
        self._filter_keep.SetInputData(ug)
        self._filter_ghost.SetInputData(ug)
        self._refresh_masks()

    def set_terrain(self, on, opacity=None):
        self._terrain_on = on
        if opacity is not None:
            self._terrain_opacity = opacity
            for kind, a in self.scene_actors:
                if kind == 'terrain':
                    a.GetProperty().SetOpacity(opacity)
        for kind, a in self.scene_actors:
            if kind == 'terrain':
                a.SetVisibility(1 if on else 0)
        self.render()

    def set_detectors(self, on):
        self._detectors_on = on
        for kind, a in self.scene_actors:
            if kind in ('detector', 'label'):
                a.SetVisibility(1 if on else 0)
        self.render()

    def set_extras(self, on):
        self._extras_on = on
        for kind, a in self.scene_actors:
            if kind == 'extra':
                a.SetVisibility(1 if on else 0)
        self.render()

    def apply_preset(self, name, detectors=None):
        """切换场景预设：更新视野/辅助几何/探测器（相机保持）。"""
        if name == '自动匹配' or name is None:
            name = self.presets.match(self.result['source']) if self.result else None
        if name == '自定义':
            if detectors is not None:
                self._detectors = detectors
                self._refresh_scene()
                self.render()
            return
        if name is None or self.presets is None:
            return
        spec = self.presets.get(name)
        if spec is None:
            return
        vr = spec['view_range']
        self.set_view_range(vr['xlim'], vr['ylim'], vr['zlim'], reset_camera=False)
        self._build_scene(spec)


# ================================================================ 地形视图

class TerrainView(ViewBase):
    """标签页 2：地形重建曲面 + 真实地形叠加。"""

    def __init__(self, interactor, presets):
        super().__init__(interactor, presets)
        self.result = None
        self._lut = make_lut('gray', 0.0, 1.0)
        self._bar = make_scalar_bar('高度 (m)', self._lut)
        self.renderer.AddViewProp(self._bar)
        self._actor_recon = None
        self._actor_real = None
        self.scene_actors = []
        self._cmap = 'gray'
        self._vmin = self._vmax = 0.0
        self._median = True
        self._sigma = 0.0
        self._holes = True
        self._real_on = True
        self._real_opacity = 0.3
        self._detectors_on = True
        self._extras_on = True
        self._detectors = []
        self._real_res = None

    def load(self, result):
        self.result = result
        self.renderer.RemoveAllViewProps()
        self.renderer.AddViewProp(self._bar)
        self.scene_actors = []
        self._detectors = self._default_detectors()
        self._real_res = None

        self._build_recon()
        self._update_lut_range()

        preset = self.presets.match(result['source']) if self.presets else None
        if preset:
            spec = self.presets.get(preset)
            vr = spec['view_range']
            self.set_view_range(vr['xlim'], vr['ylim'], vr['zlim'], reset_camera=True)
            self._build_scene(spec)
        else:
            X, Y, Z = result['X'], result['Y'], result['terrain']
            xlim = [X[0] - (X[1] - X[0]) / 2, X[-1] + (X[1] - X[0]) / 2]
            ylim = [Y[0] - (Y[1] - Y[0]) / 2, Y[-1] + (Y[1] - Y[0]) / 2]
            zlim = [float(np.nanmin(Z)), float(np.nanmax(Z))]
            self.set_view_range(xlim, ylim, zlim, reset_camera=True)

    def _default_detectors(self):
        ml = self.result['params'].get('MEASUREMENT_LOCATION')
        if not ml:
            return []
        names = ['A', 'B', 'C', 'D', 'E']
        return [{'name': names[i] if i < len(names) else str(i + 1),
                 'position': list(p)} for i, p in enumerate(ml)]

    def _processed(self):
        t = self.result['terrain'].astype(np.float64)
        if self._median:
            from scipy import ndimage
            t = ndimage.median_filter(t, size=3)
        if self._sigma > 0:
            from scipy import ndimage
            t = ndimage.gaussian_filter(t, sigma=self._sigma)
        return t

    def _build_recon(self):
        if self._actor_recon is not None:
            self.renderer.RemoveActor(self._actor_recon)
        X, Y = self.result['X'], self.result['Y']
        pd = build_height_surface(X, Y, self._processed(),
                                  invalid=loaders.INVALID_TERRAIN, as_holes=self._holes)
        if pd is None:
            self._actor_recon = None
            return
        mapper = vtk.vtkPolyDataMapper()
        mapper.SetInputData(pd)
        mapper.SetLookupTable(self._lut)
        mapper.SetScalarRange(self._vmin, self._vmax)
        actor = vtk.vtkActor()
        actor.SetMapper(mapper)
        self._actor_recon = actor
        self.renderer.AddActor(actor)

    def _build_real(self):
        if self._actor_real is not None:
            self.renderer.RemoveActor(self._actor_real)
        if self._real_res is None:
            self._actor_real = None
            return
        pd = build_height_surface(self._real_res['X'], self._real_res['Y'],
                                  self._real_res['Z'].T, invalid=None)
        if pd is None:
            self._actor_real = None
            return
        mapper = vtk.vtkPolyDataMapper()
        mapper.SetInputData(pd)
        mapper.SetLookupTable(self._lut)
        mapper.SetScalarRange(self._vmin, self._vmax)
        actor = vtk.vtkActor()
        actor.SetMapper(mapper)
        actor.GetProperty().SetOpacity(self._real_opacity)
        actor.SetVisibility(1 if self._real_on else 0)
        self._actor_real = actor
        self.renderer.AddActor(actor)

    def _update_lut_range(self):
        vals = [self._processed()]
        if self._real_res is not None:
            vals.append(self._real_res['Z'])
        allv = np.concatenate([v.ravel() for v in vals])
        allv = allv[np.isfinite(allv)]
        allv = allv[allv != loaders.INVALID_TERRAIN]
        if len(allv):
            self._vmin, self._vmax = float(np.nanmin(allv)), float(np.nanmax(allv))
        self._lut = make_lut(self._cmap, self._vmin, self._vmax)
        self._bar.SetLookupTable(self._lut)
        for actor in (self._actor_recon, self._actor_real):
            if actor is not None:
                actor.GetMapper().SetLookupTable(self._lut)
                actor.GetMapper().SetScalarRange(self._vmin, self._vmax)

    def _build_scene(self, spec):
        for _, a in self.scene_actors:
            self.renderer.RemoveActor(a)
        self.scene_actors = []
        self._detectors = spec.get('detectors', [])
        for extra in spec.get('extras', []):
            if extra['type'] == 'box_edges':
                actor = build_box_edges(extra)
            elif extra['type'] == 'cuboids':
                actor = build_cuboids(extra)
            else:
                continue
            self.scene_actors.append(('extra', actor))
        tc = spec.get('terrain_csv')
        if tc and os.path.exists(tc):
            self._real_res = loaders.load_terrain_csv(tc)
            self._update_lut_range()
            self._build_real()
        self._refresh_scene()
        self.render()

    def _refresh_scene(self):
        for kind, a in self.scene_actors:
            if kind in ('detector', 'label'):
                self.renderer.RemoveActor(a)
        self.scene_actors = [(k, a) for k, a in self.scene_actors
                             if k not in ('detector', 'label')]
        radius = self._span * 0.012
        fs = max(10, int(self._span * 0.03))
        actors, labels = build_detector_actors(self._detectors, radius, fs)
        for a, lab in zip(actors, labels):
            self.scene_actors.append(('detector', a))
            self.scene_actors.append(('label', lab))
        for kind, a in self.scene_actors:
            vis = {'extra': self._extras_on, 'detector': self._detectors_on,
                   'label': self._detectors_on}[kind]
            a.SetVisibility(1 if vis else 0)
            self.renderer.AddActor(a)

    # ----- 参数更新 -----
    def set_median(self, on):
        self._median = on
        self._build_recon()
        self._update_lut_range()
        self.render()

    def set_sigma(self, sigma):
        self._sigma = sigma
        self._build_recon()
        self.render()

    def set_holes(self, on):
        self._holes = on
        self._build_recon()
        self.render()

    def set_cmap(self, name):
        self._cmap = name
        self._update_lut_range()
        self.render()

    def set_range(self, vmin, vmax):
        self._vmin, self._vmax = vmin, vmax
        self._lut.SetRange(vmin, vmax)
        self._lut.Build()
        for actor in (self._actor_recon, self._actor_real):
            if actor is not None:
                actor.GetMapper().SetScalarRange(vmin, vmax)
        self.render()

    def set_real(self, on, opacity=None):
        self._real_on = on
        if opacity is not None:
            self._real_opacity = opacity
            if self._actor_real is not None:
                self._actor_real.GetProperty().SetOpacity(opacity)
        if self._actor_real is not None:
            self._actor_real.SetVisibility(1 if on else 0)
        self.render()

    def set_detectors(self, on):
        self._detectors_on = on
        for kind, a in self.scene_actors:
            if kind in ('detector', 'label'):
                a.SetVisibility(1 if on else 0)
        self.render()

    def set_extras(self, on):
        self._extras_on = on
        for kind, a in self.scene_actors:
            if kind == 'extra':
                a.SetVisibility(1 if on else 0)
        self.render()

    def apply_preset(self, name, detectors=None):
        if name == '自动匹配' or name is None:
            name = self.presets.match(self.result['source']) if self.result else None
        if name == '自定义':
            if detectors is not None:
                self._detectors = detectors
                self._refresh_scene()
                self.render()
            return
        if name is None or self.presets is None:
            return
        spec = self.presets.get(name)
        if spec is None:
            return
        vr = spec['view_range']
        self.set_view_range(vr['xlim'], vr['ylim'], vr['zlim'], reset_camera=False)
        self._build_scene(spec)


# ================================================================ Opacity 视图

class OpacityView(ViewBase):
    """标签页 3：Opacity 半球（多文件，每文件对应一个探测器）。"""

    def __init__(self, interactor, presets):
        super().__init__(interactor, presets)
        self.files = []          # dict(result, actor, visible, name)
        self._lut = make_lut('viridis', 0.1, 1.0)
        self._bar = make_scalar_bar('质量厚度 (g/cm²)', self._lut)
        self.renderer.AddViewProp(self._bar)
        self._r_min, self._r_max = 5.0, 40.0
        self._vmax = 100.0
        self._gamma = 0.5
        self._subdiv = 4
        self._hide_zero = True
        self._log = False
        self._vmin = 0.0
        self._cmap = 'viridis'
        self._detectors_on = True
        self._terrain_on = True
        self._terrain_opacity = 0.35
        self._extras_on = True
        self.scene_actors = []
        self._spec = None
        self._scene_detectors = []

    # ----- 文件管理 -----
    def add_file(self, path):
        result = loaders.load_opacity_csv(path)
        name = os.path.basename(path)
        self.files.append({'result': result, 'actor': None, 'visible': True, 'name': name})
        # 第一个文件时应用场景预设
        if len(self.files) == 1 and self.presets is not None:
            preset = self.presets.match(path)
            if preset:
                self._spec = self.presets.get(preset)
                vr = self._spec['view_range']
                self.set_view_range(vr['xlim'], vr['ylim'], vr['zlim'], reset_camera=True)
                self._build_scene()
            else:
                self.set_view_range([-50, 50], [-50, 50], [-10, 90], reset_camera=True)
        # vmax 取所有已加载文件的最大值
        self._vmax = max(f['result']['vmax'] for f in self.files)
        if self._vmax <= 0:
            self._vmax = 1.0
        self._vmin = 0.0
        self._rebuild_file(len(self.files) - 1)
        self._apply_lut()
        self.render()

    def remove_file(self, idx):
        if not (0 <= idx < len(self.files)):
            return
        f = self.files.pop(idx)
        if f['actor'] is not None:
            self.renderer.RemoveActor(f['actor'])
        self.render()

    def set_file_visible(self, idx, on):
        self.files[idx]['visible'] = on
        if self.files[idx]['actor'] is not None:
            self.files[idx]['actor'].SetVisibility(1 if on else 0)
        self.render()

    def _detector_pos(self, name):
        if self._spec:
            for d in self._spec.get('detectors', []):
                if d['name'] == name:
                    return d['position']
        return [0.0, 0.0, 0.0]

    def _rebuild_file(self, idx):
        f = self.files[idx]
        if f['actor'] is not None:
            self.renderer.RemoveActor(f['actor'])
        origin = self._detector_pos(f['result']['detector'])
        try:
            ug = build_opacity_grid(f['result'], origin, self._r_min, self._r_max,
                                    self._vmax, self._gamma, self._subdiv, self._hide_zero)
        except ValueError:
            f['actor'] = None
            return
        mapper = vtk.vtkDataSetMapper()
        mapper.SetInputData(ug)
        mapper.SetLookupTable(self._lut)
        mapper.SetScalarModeToUseCellFieldData()
        mapper.SelectColorArray('v')
        mapper.SetScalarRange(*self._lut.GetRange())
        actor = vtk.vtkActor()
        actor.SetMapper(mapper)
        actor.GetProperty().SetOpacity(0.9)
        actor.SetVisibility(1 if f['visible'] else 0)
        f['actor'] = actor
        self.renderer.AddActor(actor)

    def _rebuild_all(self):
        for i in range(len(self.files)):
            self._rebuild_file(i)
        self.render()

    def _build_scene(self):
        for _, a in self.scene_actors:
            self.renderer.RemoveActor(a)
        self.scene_actors = []
        self._scene_detectors = self._spec.get('detectors', [])
        for extra in self._spec.get('extras', []):
            if extra['type'] == 'box_edges':
                actor = build_box_edges(extra)
            elif extra['type'] == 'cuboids':
                actor = build_cuboids(extra)
            else:
                continue
            self.scene_actors.append(('extra', actor))
        tc = self._spec.get('terrain_csv')
        if tc and os.path.exists(tc):
            res = loaders.load_terrain_csv(tc)
            hmin = float(np.nanmin(res['Z']))
            hmax = float(np.nanmax(res['Z']))
            lut = make_lut('terrain', hmin, hmax)
            actor = build_terrain_shell(tc, lut, self._terrain_opacity)
            if actor is not None:
                self.scene_actors.append(('terrain', actor))
        self._refresh_scene()

    def _refresh_scene(self):
        for kind, a in self.scene_actors:
            if kind in ('detector', 'label'):
                self.renderer.RemoveActor(a)
        self.scene_actors = [(k, a) for k, a in self.scene_actors
                             if k not in ('detector', 'label')]
        # 探测器球：只显示已加载文件对应的那些
        loaded = {f['result']['detector'] for f in self.files}
        dets = [d for d in self._scene_detectors if d['name'] in loaded]
        radius = self._r_max * 0.12
        fs = max(10, int(self._span * 0.03))
        actors, labels = build_detector_actors(dets, radius, fs)
        for a, lab in zip(actors, labels):
            self.scene_actors.append(('detector', a))
            self.scene_actors.append(('label', lab))
        for kind, a in self.scene_actors:
            vis = {'extra': self._extras_on, 'terrain': self._terrain_on,
                   'detector': self._detectors_on, 'label': self._detectors_on}[kind]
            a.SetVisibility(1 if vis else 0)
            self.renderer.AddActor(a)

    # ----- 参数更新 -----
    def _apply_lut(self):
        vmin = self._vmin
        if self._log:
            # 用正的最小值兜底（对数刻度要求 > 0）
            allv = np.concatenate([f['result']['values'].ravel() for f in self.files]) \
                if self.files else np.array([1.0])
            posv = allv[allv > 0]
            vmin = float(posv.min()) if len(posv) else 1e-3
            vmin = max(vmin, 1e-3)
        self._lut = make_lut(self._cmap, vmin, self._vmax, log=self._log)
        self._bar.SetLookupTable(self._lut)
        for f in self.files:
            if f['actor'] is not None:
                f['actor'].GetMapper().SetLookupTable(self._lut)
                f['actor'].GetMapper().SetScalarRange(vmin, self._vmax)
        self.render()

    def set_cmap(self, name):
        self._cmap = name
        self._apply_lut()

    def set_range(self, vmin, vmax):
        self._vmin, self._vmax = vmin, vmax
        self._apply_lut()

    def set_log(self, on):
        self._log = on
        self._apply_lut()

    def set_radius(self, r_min, r_max):
        self._r_min, self._r_max = r_min, r_max
        # 探测器球半径随 r_max 变化 → 重建场景球
        for kind, a in self.scene_actors:
            if kind in ('detector', 'label'):
                self.renderer.RemoveActor(a)
        self.scene_actors = [(k, a) for k, a in self.scene_actors
                             if k not in ('detector', 'label')]
        self._rebuild_all()
        self._refresh_scene()
        self.render()

    def set_vmax_radius(self, vmax):
        self._vmax = vmax
        self._rebuild_all()

    def set_gamma(self, gamma):
        self._gamma = gamma
        self._rebuild_all()

    def set_subdiv(self, n):
        self._subdiv = n
        self._rebuild_all()

    def set_hide_zero(self, on):
        self._hide_zero = on
        self._rebuild_all()

    def set_detectors(self, on):
        self._detectors_on = on
        for kind, a in self.scene_actors:
            if kind in ('detector', 'label'):
                a.SetVisibility(1 if on else 0)
        self.render()

    def set_terrain(self, on, opacity=None):
        self._terrain_on = on
        if opacity is not None:
            self._terrain_opacity = opacity
            for kind, a in self.scene_actors:
                if kind == 'terrain':
                    a.GetProperty().SetOpacity(opacity)
        for kind, a in self.scene_actors:
            if kind == 'terrain':
                a.SetVisibility(1 if on else 0)
        self.render()

    def set_extras(self, on):
        self._extras_on = on
        for kind, a in self.scene_actors:
            if kind == 'extra':
                a.SetVisibility(1 if on else 0)
        self.render()

    def apply_preset(self, name, detectors=None):
        if name == '自动匹配' or name is None:
            name = self.presets.match(self.files[0]['result']['source']) if self.files else None
        if name == '自定义':
            if detectors is not None:
                self._scene_detectors = detectors
                for kind, a in self.scene_actors:
                    if kind in ('detector', 'label'):
                        self.renderer.RemoveActor(a)
                self.scene_actors = [(k, a) for k, a in self.scene_actors
                                     if k not in ('detector', 'label')]
                self._rebuild_all()
                self._refresh_scene()
                self.render()
            return
        if name is None or self.presets is None:
            return
        spec = self.presets.get(name)
        if spec is None:
            return
        self._spec = spec
        vr = spec['view_range']
        self.set_view_range(vr['xlim'], vr['ylim'], vr['zlim'], reset_camera=False)
        self._build_scene()
        self._rebuild_all()
