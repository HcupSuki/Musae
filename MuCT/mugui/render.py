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
# 项目内置中文字体（SIL OFL 1.1，随仓库分发），系统无中文字体时仍可正常显示
CJK_FALLBACK = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                            'fonts', 'NotoSansCJKsc-Regular.otf')
CJK_FALLBACK_SYS = '/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc'

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


def make_cell_array(conn, cell_size):
    """固定点数单元 → vtkCellArray。

    使用 VTK 9 推荐的 SetData(offsets, connectivity) 接口，避免已弃用的
    legacy SetCells(n, legacy_array) 在新版本上的兼容性问题。
    """
    conn = np.asarray(conn, dtype=np.int64)
    if conn.size == 0:
        return vtk.vtkCellArray()
    conn = conn.reshape(-1, cell_size)
    n = len(conn)
    offsets = np.arange(0, (n + 1) * cell_size, cell_size, dtype=np.int64)
    cells = vtk.vtkCellArray()
    cells.SetData(nps.numpy_to_vtkIdTypeArray(offsets, deep=True),
                  nps.numpy_to_vtkIdTypeArray(conn.ravel(), deep=True))
    return cells


def set_cjk(tprop, size=14, color=(0.0, 0.0, 0.0)):
    """给 vtkTextProperty 设置中文可用字体（优先项目内置字体）。

    注意：VTK 9.x 中 SetFontFile 只有把字体族设为魔法串 'File'
    （GetFontFamily() 返回 VTK_FONT_FILE）时才会加载字体文件；
    设置普通族名只会走 fontconfig/内嵌字体，中文字形会静默丢失。
    """
    for path in (CJK_FALLBACK, CJK_FALLBACK_SYS):
        if os.path.exists(path):
            tprop.SetFontFamilyAsString('File')
            tprop.SetFontFile(path)
            break
    else:
        # 找不到中文字体文件：保留族名交给 fontconfig/回退字体
        tprop.SetFontFamilyAsString(CJK_FAMILY)
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
    （rho/abs/keep/ghost，与 ugrid 的 cell data 共享内存，可原位更新）。

    每个体素使用 8 个独立点，并把 rho 同时写成点标量。这样：
    - 颜色按体素平色显示，不会在相邻体素间被插值模糊；
    - 规避部分 VTK 9.6 + 软件 OpenGL 环境下 cell scalar 渲染整块消失的问题。
    Cell data 仍保留 rho/abs/keep/ghost 供 vtkThreshold 过滤。
    """
    nx, ny, nz = rho.shape
    valid = np.isfinite(rho) & (rho != loaders.INVALID_DENSITY)
    idx = np.argwhere(valid)  # (n,3): i,j,k
    if len(idx) == 0:
        raise ValueError('没有有效体素可显示')
    n = len(idx)
    i, j, k = idx[:, 0].astype(np.int64), idx[:, 1].astype(np.int64), idx[:, 2].astype(np.int64)
    di = np.array([0, 1, 1, 0, 0, 1, 1, 0], np.int64)
    dj = np.array([0, 0, 1, 1, 0, 0, 1, 1], np.int64)
    dk = np.array([0, 0, 0, 0, 1, 1, 1, 1], np.int64)
    dx = X[1] - X[0] if len(X) > 1 else 1.0
    dy = Y[1] - Y[0] if len(Y) > 1 else 1.0
    dz = Z[1] - Z[0] if len(Z) > 1 else 1.0
    x0 = X[0] - dx / 2
    y0 = Y[0] - dy / 2
    z0 = Z[0] - dz / 2
    # 每个体素的 8 个角点（独立存储，不共享点）
    px = (x0 + i * dx)[:, None] + di[None, :] * dx
    py = (y0 + j * dy)[:, None] + dj[None, :] * dy
    pz = (z0 + k * dz)[:, None] + dk[None, :] * dz
    pts = np.stack([px, py, pz], axis=-1).reshape(-1, 3)
    conn = np.arange(n * 8, dtype=np.int64).reshape(n, 8)

    ug = vtk.vtkUnstructuredGrid()
    ug.SetPoints(make_points(pts))
    ug.SetCells(vtk.VTK_HEXAHEDRON, make_cell_array(conn, 8))

    rho_v = rho[valid].astype(np.float64)
    abs_v = np.abs(rho_v)
    arrays = {}

    # 点标量：用于颜色映射；每个体素的 8 个点取同一个值 → 平色
    for name, arr in (('rho', np.repeat(rho_v, 8)),
                      ('abs', np.repeat(abs_v, 8))):
        va = nps.numpy_to_vtk(arr, deep=True)
        va.SetName(name)
        ug.GetPointData().AddArray(va)
    ug.GetPointData().SetScalars(ug.GetPointData().GetArray('rho'))

    # Cell data：用于阈值/透明过滤
    for name, arr in (('rho', rho_v),
                      ('abs', abs_v),
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

    pd = vtk.vtkPolyData()
    pd.SetPoints(make_points(pts))
    pd.SetPolys(make_cell_array(quads, 4))
    sa = nps.numpy_to_vtk(zz, deep=False)
    sa.SetName('height')
    pd.GetPointData().SetScalars(sa)
    return pd


# ================================================================ Opacity 半球

def build_opacity_grid(result, origin, r_min, r_max, vmax, gamma, subdiv, hide_zero):
    """Opacity 角分布 → (球面端面, 从探测器到端面的柱状侧面)。

    半径: r(v) = r_min + (r_max - r_min) × (v/vmax)^γ

    返回 (caps, walls, lines)：
    - caps  : 每个 bin 在半径 r 处的球面四边形（端面，颜色=质量厚度）；
    - walls : 以探测器位置为顶点、连到端面四条边的三角形侧面；
    - lines : 从探测器连到端面四个角的四条轮廓线。
    这样每个 bin 看起来是从探测器出发的“柱状体/立体角”，而不只是
    一层距离不同的球面片。caps/walls 用点标量 'v' 着色（平色），避免
    cell scalar 在部分 VTK/OpenGL 驱动上不渲染。
    """
    te = result['theta_edges']
    pe = result['phi_edges']
    vals = result['values']
    n = subdiv + 1
    keep_mask = np.ones(vals.shape, dtype=bool)
    if hide_zero:
        keep_mask &= vals != 0
    kept = np.argwhere(keep_mask)
    if len(kept) == 0:
        raise ValueError('没有可显示的 bin（全部为 0？）')

    origin = np.asarray(origin, dtype=np.float64)
    cap_pts_all, cap_quads_all, cap_scal_all = [], [], []
    wall_pts_all, wall_tris_all, wall_scal_all = [], [], []
    line_pts_all, line_cells_all = [], []
    cap_off = wall_off = line_off = 0
    j = np.arange(subdiv)
    i = np.arange(subdiv)
    for ti, pi in kept:
        v = max(float(vals[ti, pi]), 0.0)
        r = r_min + (r_max - r_min) * (v / vmax) ** gamma
        ths = np.linspace(te[ti], te[ti + 1], n)
        phs = np.linspace(pe[pi], pe[pi + 1], n)
        TH, PH = np.meshgrid(ths, phs, indexing='ij')
        st = np.sin(TH)
        p = np.stack([r * st * np.cos(PH), r * st * np.sin(PH), r * np.cos(TH)],
                     axis=-1).reshape(-1, 3)
        p += origin

        # 端面：n×n 点、subdiv×subdiv 个四边形
        cap_pts_all.append(p)
        cap_scal_all.append(np.full(n * n, v))
        a, b = np.meshgrid(np.arange(subdiv), np.arange(subdiv), indexing='ij')
        q = (a * n + b).ravel()
        cap_quads_all.append(np.stack([q, q + n, q + n + 1, q + 1], axis=1) + cap_off)
        cap_off += n * n

        # 侧面：端面点 + 一个探测器位置顶点（apex）
        apex = n * n
        wall_pts_all.append(np.vstack([p, origin[None, :]]))
        wall_scal_all.append(np.full(n * n + 1, v))
        top = np.stack([np.full(subdiv, apex), j, j + 1], axis=1)
        bottom = np.stack([np.full(subdiv, apex), subdiv * n + j + 1,
                           subdiv * n + j], axis=1)
        left = np.stack([np.full(subdiv, apex), (i + 1) * n, i * n], axis=1)
        right = np.stack([np.full(subdiv, apex), i * n + subdiv,
                          (i + 1) * n + subdiv], axis=1)
        wall_tris_all.append(np.vstack([top, bottom, left, right]) + wall_off)
        wall_off += n * n + 1

        # 从探测器位置连到端面四个角的四条线（用户要求的“四条线”）
        corners = [0, subdiv, subdiv * n, subdiv * n + subdiv]
        line_pts_all.append(np.vstack([origin[None, :], p[corners]]))
        line_cells_all.append(np.array([
            [line_off, line_off + 1], [line_off, line_off + 2],
            [line_off, line_off + 3], [line_off, line_off + 4]], np.int64))
        line_off += 5

    def make_grid(pts_all, cells, cell_type, scal_all):
        ug = vtk.vtkUnstructuredGrid()
        ug.SetPoints(make_points(np.concatenate(pts_all, axis=0)))
        ug.SetCells(cell_type, make_cell_array(np.concatenate(cells, axis=0),
                                               cells[0].shape[1]))
        sc = nps.numpy_to_vtk(np.concatenate(scal_all), deep=True)
        sc.SetName('v')
        ug.GetPointData().SetScalars(sc)
        return ug

    caps = make_grid(cap_pts_all, cap_quads_all, vtk.VTK_QUAD, cap_scal_all)
    walls = make_grid(wall_pts_all, wall_tris_all, vtk.VTK_TRIANGLE, wall_scal_all)
    lines = vtk.vtkUnstructuredGrid()
    lines.SetPoints(make_points(np.concatenate(line_pts_all, axis=0)))
    lines.SetCells(vtk.VTK_LINE,
                   make_cell_array(np.concatenate(line_cells_all, axis=0), 2))
    return caps, walls, lines


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

    pd = vtk.vtkPolyData()
    pd.SetPoints(make_points(pts))
    pd.SetPolys(make_cell_array(quads, 4))
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
            # 标签紧跟红球上方，避免跟随半径放大后“飘”得太远
            label.SetPosition(pos[0], pos[1],
                              pos[2] + max(radius * 2.0, font_size * 0.8))
            labels.append(label)
        else:
            # 占位，保证 labels 与 actors 一一对应
            labels.append(None)
    return actors, labels


def build_terrain_actor(res, lut, opacity):
    """已加载的真实地形（load_terrain_csv 的返回值）→ 半透明曲面演员。"""
    if res is None:
        return None
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


def terrain_lut(res):
    """真实地形高度 → 配色表；平坦地形也给出可用的颜色范围。"""
    z = np.asarray(res['Z'], dtype=np.float64)
    z = z[np.isfinite(z)]
    if len(z) == 0:
        hmin, hmax = 0.0, 1.0
    else:
        hmin, hmax = float(z.min()), float(z.max())
    return make_lut('terrain', hmin, hmax), hmin, hmax


def build_terrain_shell(csv_path, lut, opacity):
    """真实地形薄壳（半透明曲面）。"""
    if not csv_path or not os.path.exists(csv_path):
        return None
    return build_terrain_actor(loaders.load_terrain_csv(csv_path), lut, opacity)


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

    def center_camera(self, points):
        """把镜头中心和视野范围平移到探测点（多点取平均）。"""
        pts = np.asarray(points, dtype=np.float64)
        if pts.size == 0:
            return
        target = pts.mean(axis=0)
        cam = self.renderer.GetActiveCamera()
        focal = np.asarray(cam.GetFocalPoint(), dtype=np.float64)
        pos = np.asarray(cam.GetPosition(), dtype=np.float64)
        delta = target - focal
        if self._xlim is not None:
            self._xlim = [self._xlim[0] + delta[0], self._xlim[1] + delta[0]]
            self._ylim = [self._ylim[0] + delta[1], self._ylim[1] + delta[1]]
            self._zlim = [self._zlim[0] + delta[2], self._zlim[1] + delta[2]]
        cam.SetFocalPoint(*target)
        cam.SetPosition(*(pos + delta))
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
        self.terrain_csv = None
        self.scene_warning = None

    # ----- 数据加载 -----
    def load(self, result):
        self.result = result
        self.renderer.RemoveAllViewProps()
        self.renderer.AddViewProp(self._bar)
        self.scene_actors = []
        self.scene_warning = None
        self.terrain_csv = None

        X, Y, Z, rho = result['X'], result['Y'], result['Z'], result['rho']
        self.grids = {}
        for drop in (False, True):
            r, zz = rho, Z
            if drop:
                if rho.shape[2] <= 1:
                    # 只有一层 z，无法剔除首层；复用完整网格
                    self.grids[True] = self.grids[False]
                    self.drop_layer = False
                    continue
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
            # 颜色取点标量（体素平色）；cell data 只用于阈值过滤
            mapper.SetScalarModeToUsePointFieldData()
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
        def axis_lim(a):
            d = float(a[1] - a[0]) if len(a) > 1 else 1.0
            return [float(a[0]) - d / 2, float(a[-1]) + d / 2]
        xlim, ylim, zlim = axis_lim(X), axis_lim(Y), axis_lim(Z)
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
        self.scene_warning = None
        self._detectors = spec.get('detectors', [])
        for extra in spec.get('extras', []):
            if extra['type'] == 'box_edges':
                actor = build_box_edges(extra)
            elif extra['type'] == 'cuboids':
                actor = build_cuboids(extra)
            else:
                continue
            self.scene_actors.append(('extra', actor))
        self._load_preset_terrain(spec)
        self._refresh_scene()
        self.render()

    def _load_preset_terrain(self, spec):
        tc = spec.get('terrain_csv')
        self.terrain_csv = None
        if not tc:
            return
        if not os.path.exists(tc):
            self.scene_warning = f'真实地形文件不存在：{tc}（可在“场景”区域点击「加载真实地形 CSV…」）'
            return
        try:
            self._set_terrain_res(loaders.load_terrain_csv(tc), tc)
        except Exception as e:
            self.scene_warning = f'真实地形加载失败：{e}'

    def _set_terrain_res(self, res, source):
        """替换真实地形薄壳演员（res=None 表示清除）。"""
        for kind, a in list(self.scene_actors):
            if kind == 'terrain':
                self.renderer.RemoveActor(a)
                self.scene_actors.remove((kind, a))
        self.terrain_csv = source
        if res is None:
            return
        lut, _, _ = terrain_lut(res)
        actor = build_terrain_actor(res, lut, self._terrain_opacity)
        if actor is not None:
            self.scene_actors.append(('terrain', actor))

    def set_terrain_csv(self, path):
        """手动指定真实地形 CSV（预设文件缺失时使用）。"""
        res = loaders.load_terrain_csv(path)
        self._set_terrain_res(res, path)
        self.scene_warning = None
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
            if lab is not None:
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
                if detectors:
                    self.center_camera([d['position'] for d in detectors])
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
        self._lut = make_lut('terrain', 0.0, 1.0)
        self._bar = make_scalar_bar('高度 (m)', self._lut)
        self.renderer.AddViewProp(self._bar)
        self._actor_recon = None
        self._actor_real = None
        self._cmap = 'terrain'
        self._vmin = self._vmax = 0.0
        self._median = True
        self._sigma = 0.0
        self._holes = True
        self._real_on = True
        self._real_opacity = 0.3
        self._real_res = None
        self.terrain_csv = None
        self.scene_warning = None

    def load(self, result):
        self.result = result
        self.renderer.RemoveAllViewProps()
        self.renderer.AddViewProp(self._bar)
        self._real_res = None
        self.terrain_csv = None
        self.scene_warning = None

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
        vals = []
        if self.result is not None:
            vals.append(self._processed())
        if self._real_res is not None:
            vals.append(self._real_res['Z'])
        if not vals:
            return
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
        """地形页只加载真实地形薄壳，不加载隧道/探测器。"""
        self.scene_warning = None
        tc = spec.get('terrain_csv')
        self._real_res = None
        self.terrain_csv = None
        if tc:
            if os.path.exists(tc):
                try:
                    self._real_res = loaders.load_terrain_csv(tc)
                    self.terrain_csv = tc
                except Exception as e:
                    self.scene_warning = f'真实地形加载失败：{e}'
            else:
                self.scene_warning = f'真实地形文件不存在：{tc}（可在“场景”区域点击「加载真实地形 CSV…」）'
        self._update_lut_range()
        self._build_real()
        self.render()

    def set_terrain_csv(self, path):
        """手动指定真实地形 CSV（预设文件缺失时使用）。"""
        self._real_res = loaders.load_terrain_csv(path)
        self.terrain_csv = path
        self.scene_warning = None
        if self.result is None:
            res = self._real_res
            X, Y, Z = res['X'], res['Y'], res['Z']
            xlim = [X[0] - (X[1] - X[0]) / 2, X[-1] + (X[1] - X[0]) / 2]
            ylim = [Y[0] - (Y[1] - Y[0]) / 2, Y[-1] + (Y[1] - Y[0]) / 2]
            zlim = [float(np.nanmin(Z)), float(np.nanmax(Z))]
            self.set_view_range(xlim, ylim, zlim, reset_camera=True)
        self._update_lut_range()
        self._build_real()
        self.render()

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
        # 面板里用中文标签，matplotlib 需要英文色图名
        self._cmap = {'灰度': 'gray', '地形色': 'terrain'}.get(name, name)
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

    def apply_preset(self, name, detectors=None):
        if name == '自动匹配' or name is None:
            name = self.presets.match(self.result['source']) if self.result else None
        if name == '自定义':
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
    """标签页 3：Opacity 角分布（每个 bin 从探测器拉出柱状体，多文件）。"""

    def __init__(self, interactor, presets):
        super().__init__(interactor, presets)
        self.files = []          # dict(result, actors, visible, name)
        # 探测器/标签放在独立的 overlay renderer 层：柱状体的侧面会挡住
        # 原点处的红球和标签，放到 overlay 后它们始终可见。
        self.overlay = vtk.vtkRenderer()
        self.overlay.SetLayer(1)
        self.overlay.SetBackgroundAlpha(0.0)
        self.overlay.SetInteractive(0)
        self.rw.SetNumberOfLayers(2)
        self.rw.AddRenderer(self.overlay)
        self.overlay.SetActiveCamera(self.renderer.GetActiveCamera())
        self._lut = make_lut('viridis', 0.1, 1.0)
        self._bar = make_scalar_bar('质量厚度 (g/cm²)', self._lut)
        self.renderer.AddViewProp(self._bar)
        self._r_min, self._r_max = 10.0, 250.0
        self._vmax = 100.0        # 数据最大值：柱状体长度按它归一
        self._color_max = 100.0   # 颜色标尺上限：可在面板单独调
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
        self._preset_name = '自动匹配'
        self._scene_detectors = []
        self.terrain_csv = None
        self.scene_warning = None
        self.origin_warning = None
        self.radius_autoset = False
        self._detector_radius = 8.0
        self.detector_autoset = False

    # ----- 相机 -----
    def reset_camera(self):
        """Opacity 默认用斜侧视角，能看清柱状体的长度，而不是从上往下看。"""
        if self._xlim is None:
            return
        cx = (self._xlim[0] + self._xlim[1]) / 2
        cy = (self._ylim[0] + self._ylim[1]) / 2
        cz = (self._zlim[0] + self._zlim[1]) / 2
        s = self._span
        cam = self.renderer.GetActiveCamera()
        cam.SetFocalPoint(cx, cy, cz)
        cam.SetPosition(cx + 0.75 * s, cy - 0.75 * s, cz + 0.65 * s)
        cam.SetViewUp(0.0, 0.0, 1.0)
        cam.SetClippingRange(0.01 * s, 12.0 * s)
        self.renderer.ResetCameraClippingRange()
        self.render()

    # ----- 场景/预设 -----
    def _custom_center(self):
        if self._scene_detectors:
            pts = np.asarray([d['position'] for d in self._scene_detectors],
                             dtype=np.float64)
            return pts.mean(axis=0)
        return np.array([0.0, 0.0, 0.0])

    def _default_view_around(self, center, radius=None):
        c = np.asarray(center, dtype=np.float64)
        r = max(50.0, float(self._r_max if radius is None else radius))
        self.set_view_range([c[0] - 1.5 * r, c[0] + 1.5 * r],
                            [c[1] - 1.5 * r, c[1] + 1.5 * r],
                            [c[2] - 1.2 * r, c[2] + 1.2 * r],
                            reset_camera=True)

    def _clear_preset_scene(self, keep_detectors=False):
        """移除上一个预设的隧道/探测器；预设地形也移除，手动地形保留。"""
        preset_terrain = self._spec.get('terrain_csv') if self._spec else None
        for kind, a in list(self.scene_actors):
            if kind in ('extra', 'detector', 'label'):
                if kind in ('detector', 'label'):
                    self.overlay.RemoveActor(a)
                else:
                    self.renderer.RemoveActor(a)
                self.scene_actors.remove((kind, a))
            elif (kind == 'terrain' and preset_terrain
                  and self.terrain_csv == preset_terrain):
                # 这是上一个预设自动加载的地形，不是用户手动选的
                self.renderer.RemoveActor(a)
                self.scene_actors.remove((kind, a))
                self.terrain_csv = None
        self._spec = None
        if not keep_detectors:
            self._scene_detectors = []

    def _apply_preset_spec(self, name, state_name):
        if self.presets is None:
            return False
        spec = self.presets.get(name)
        if spec is None:
            return False
        self._spec = spec
        self._preset_name = state_name
        vr = spec['view_range']
        self.set_view_range(vr['xlim'], vr['ylim'], vr['zlim'], reset_camera=True)
        self._build_scene()
        return True

    def _prepare_first_file(self, path, detector=None):
        """第一个 opacity 文件：只在“自动匹配”模式下才按文件名套预设。"""
        if self._preset_name == '自定义':
            if self._xlim is None:
                self._default_view_around(self._custom_center())
            return
        if self._preset_name in (None, '自动匹配'):
            preset = self.presets.match(path) if self.presets else None
            if preset:
                self._apply_preset_spec(preset, '自动匹配')
            else:
                # 文件名不属于任何预设：清掉上一个预设的隧道/探测器，
                # 探测器原点默认 (0,0,0)，保留手动加载的地形。
                self._clear_preset_scene(keep_detectors=False)
                self._preset_name = '自动匹配'
                # 没有自定义探测器时，至少画一个原点红球；
                # 文件名里有 A/B/C 就顺便显示对应标签。
                self._scene_detectors = [{'name': detector or '',
                                          'position': [0.0, 0.0, 0.0]}]
                self._default_view_around(self._custom_center())
            return
        # 用户明确选了某个预设：保持用户选择
        if self._spec is None:
            self._apply_preset_spec(self._preset_name, self._preset_name)

    # ----- 文件管理 -----
    def add_file(self, path):
        result = loaders.load_opacity_csv(path)
        name = os.path.basename(path)
        self.files.append({'result': result, 'actors': [], 'visible': True, 'name': name})
        # 第一个文件时决定场景
        if len(self.files) == 1:
            self._prepare_first_file(path, result['detector'])
        # 第一个文件：按场景跨度给一组可见的默认半径
        if len(self.files) == 1:
            self._r_min = max(1.0, self._span * 0.02)
            self._r_max = max(self._r_min * 2.0, self._span * 0.30)
            self.radius_autoset = True
            # 默认红点半径与密度页一致（span×0.012），不再用 r_max×0.12
            self._detector_radius = max(0.5, self._span * 0.012)
            self.detector_autoset = True
        # vmax 取所有已加载文件的最大值；半径按 vmax 归一，需重建全部
        self._vmax = max(f['result']['vmax'] for f in self.files)
        if self._vmax <= 0:
            self._vmax = 1.0
        self._color_max = self._vmax
        self._vmin = 0.0
        self._rebuild_all()
        self._apply_lut()
        self._refresh_scene()
        self.render()

    def remove_file(self, idx):
        if not (0 <= idx < len(self.files)):
            return
        f = self.files.pop(idx)
        for a in f['actors']:
            self.renderer.RemoveActor(a)
        self._vmax = max((x['result']['vmax'] for x in self.files), default=1.0)
        if self._vmax <= 0:
            self._vmax = 1.0
        self._color_max = self._vmax
        self._vmin = 0.0
        self._rebuild_all()
        self._apply_lut()
        self._refresh_scene()
        self.render()

    def set_file_visible(self, idx, on):
        self.files[idx]['visible'] = on
        for a in self.files[idx]['actors']:
            a.SetVisibility(1 if on else 0)
        self.render()

    def _detector_pos(self, name):
        if self._spec:
            for d in self._spec.get('detectors', []):
                if d['name'] == name:
                    return d['position']
        if name:
            for d in self._scene_detectors:
                if d['name'] == name:
                    return d['position']
        # 文件名里没有探测器/预设里找不到：自定义模式用第一个自定义点，
        # 否则用原点 (0,0,0)。RuYuan 这类无预设数据默认就在原点。
        if self._preset_name == '自定义' and self._scene_detectors:
            return self._scene_detectors[0]['position']
        return [0.0, 0.0, 0.0]

    def _rebuild_file(self, idx):
        f = self.files[idx]
        for a in f['actors']:
            self.renderer.RemoveActor(a)
        f['actors'] = []
        det = f['result']['detector']
        origin = self._detector_pos(det)
        self.origin_warning = None
        try:
            caps, walls, lines = build_opacity_grid(
                f['result'], origin, self._r_min, self._r_max,
                self._vmax, self._gamma, self._subdiv, self._hide_zero)
        except ValueError:
            return

        # 端面：opaque，显示该 bin 的颜色
        # 侧面：半透明，做出从探测器到端面的柱状体
        # 四条角线：从探测器连到端面四个角，勾出每个柱状体的轮廓
        for ug, opacity, flat, is_line in (
                (caps, 0.92, False, False),
                (walls, 0.35, True, False),
                (lines, 1.0, False, True)):
            mapper = vtk.vtkDataSetMapper()
            mapper.SetInputData(ug)
            actor = vtk.vtkActor()
            actor.SetMapper(mapper)
            prop = actor.GetProperty()
            if is_line:
                mapper.ScalarVisibilityOff()
                prop.SetColor(0.08, 0.08, 0.08)
                prop.SetOpacity(0.32)
                prop.SetLineWidth(1.0)
                prop.SetLighting(0)
            else:
                mapper.SetLookupTable(self._lut)
                mapper.SetScalarModeToUsePointFieldData()
                mapper.SelectColorArray('v')
                mapper.SetScalarRange(*self._lut.GetRange())
                prop.SetOpacity(opacity)
                prop.SetAmbient(0.4)
                prop.SetDiffuse(0.75)
                if flat:
                    prop.SetInterpolationToFlat()
            actor.SetVisibility(1 if f['visible'] else 0)
            f['actors'].append(actor)
            self.renderer.AddActor(actor)

    def _rebuild_all(self):
        for i in range(len(self.files)):
            self._rebuild_file(i)
        self.render()

    def _build_scene(self):
        for kind, a in self.scene_actors:
            if kind in ('detector', 'label'):
                self.overlay.RemoveActor(a)
            else:
                self.renderer.RemoveActor(a)
        self.scene_actors = []
        self.scene_warning = None
        self._scene_detectors = self._spec.get('detectors', [])
        for extra in self._spec.get('extras', []):
            if extra['type'] == 'box_edges':
                actor = build_box_edges(extra)
            elif extra['type'] == 'cuboids':
                actor = build_cuboids(extra)
            else:
                continue
            self.scene_actors.append(('extra', actor))
        self._load_preset_terrain()
        self._refresh_scene()

    def _load_preset_terrain(self):
        tc = self._spec.get('terrain_csv') if self._spec else None
        self.terrain_csv = None
        if not tc:
            return
        if not os.path.exists(tc):
            self.scene_warning = f'真实地形文件不存在：{tc}（可在“场景”区域点击「加载真实地形 CSV…」）'
            return
        try:
            self._set_terrain_res(loaders.load_terrain_csv(tc), tc)
        except Exception as e:
            self.scene_warning = f'真实地形加载失败：{e}'

    def _set_terrain_res(self, res, source):
        for kind, a in list(self.scene_actors):
            if kind == 'terrain':
                self.renderer.RemoveActor(a)
                self.scene_actors.remove((kind, a))
        self.terrain_csv = source
        if res is None:
            return
        lut, _, _ = terrain_lut(res)
        actor = build_terrain_actor(res, lut, self._terrain_opacity)
        if actor is not None:
            self.scene_actors.append(('terrain', actor))

    def set_terrain_csv(self, path):
        """手动指定真实地形 CSV（预设文件缺失时使用）。"""
        res = loaders.load_terrain_csv(path)
        self._set_terrain_res(res, path)
        self.scene_warning = None
        self._refresh_scene()
        self.render()

    def _refresh_scene(self):
        for kind, a in self.scene_actors:
            if kind in ('detector', 'label'):
                self.overlay.RemoveActor(a)
        self.scene_actors = [(k, a) for k, a in self.scene_actors
                             if k not in ('detector', 'label')]
        # 探测器球：预设模式只显示已加载文件对应的那些；自定义模式全部显示；
        # 无预设的默认原点探测器（name=''）也要显示。
        loaded = {f['result']['detector'] for f in self.files}
        if self._preset_name == '自定义':
            dets = list(self._scene_detectors)
        else:
            dets = [d for d in self._scene_detectors
                    if d['name'] in loaded or (None in loaded and d['name'] == '')]
        fs = max(10, int(self._span * 0.03))
        actors, labels = build_detector_actors(dets, self._detector_radius, fs)
        for a, lab in zip(actors, labels):
            self.scene_actors.append(('detector', a))
            if lab is not None:
                self.scene_actors.append(('label', lab))
        for kind, a in self.scene_actors:
            vis = {'extra': self._extras_on, 'terrain': self._terrain_on,
                   'detector': self._detectors_on, 'label': self._detectors_on}[kind]
            a.SetVisibility(1 if vis else 0)
            if kind in ('detector', 'label'):
                self.overlay.AddActor(a)
            else:
                self.renderer.AddActor(a)

    # ----- 参数更新 -----
    def _apply_lut(self):
        vmin = self._vmin
        vmax = self._color_max
        if vmax <= 0:
            vmax = 1.0
        if self._log:
            # 用正的最小值兜底（对数刻度要求 > 0）
            allv = np.concatenate([f['result']['values'].ravel() for f in self.files]) \
                if self.files else np.array([1.0])
            posv = allv[allv > 0]
            vmin = float(posv.min()) if len(posv) else 1e-3
            vmin = max(vmin, 1e-3)
        self._lut = make_lut(self._cmap, vmin, vmax, log=self._log)
        self._bar.SetLookupTable(self._lut)
        for f in self.files:
            for actor in f['actors']:
                actor.GetMapper().SetLookupTable(self._lut)
                actor.GetMapper().SetScalarRange(vmin, vmax)
        self.render()

    def set_cmap(self, name):
        self._cmap = name
        self._apply_lut()

    def set_range(self, vmin, vmax):
        # 只改颜色范围；柱状体长度仍按数据 vmax 归一
        self._vmin, self._color_max = vmin, vmax
        self._apply_lut()

    def set_log(self, on):
        self._log = on
        self._apply_lut()

    def set_radius(self, r_min, r_max):
        # 只改柱状体长度；探测器红点大小由独立滑杆控制
        self._r_min, self._r_max = r_min, r_max
        self._rebuild_all()
        self.render()

    def set_detector_radius(self, radius):
        self._detector_radius = max(0.1, float(radius))
        self.detector_autoset = False
        self._refresh_scene()
        self.render()

    def set_vmax_radius(self, vmax):
        self._vmax = vmax
        self._color_max = vmax
        self._rebuild_all()
        self._apply_lut()

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
            self._preset_name = '自动匹配'
            source = self.files[0]['result']['source'] if self.files else None
            preset = self.presets.match(source) if (source and self.presets) else None
            if preset:
                self._apply_preset_spec(preset, '自动匹配')
            else:
                self._clear_preset_scene(keep_detectors=False)
                self._default_view_around(self._custom_center())
            self._rebuild_all()
            self._refresh_scene()
            self.render()
            return
        if name == '自定义':
            self._preset_name = '自定义'
            if detectors is not None:
                self._scene_detectors = detectors
            # 自定义场景不再保留上一个预设的隧道，但保留手动加载的地形
            self._clear_preset_scene(keep_detectors=True)
            had_range = self._xlim is not None
            if self._scene_detectors and not had_range:
                self._default_view_around(self._custom_center())
            self._rebuild_all()
            self._refresh_scene()
            if self._scene_detectors and had_range:
                # 保留用户当前缩放，只把镜头中心平移到探测点
                self.center_camera([d['position'] for d in self._scene_detectors])
            self.render()
            return
        if self.presets is None:
            return
        self._preset_name = name
        if not self._apply_preset_spec(name, name):
            return
        self._rebuild_all()
        self.render()
