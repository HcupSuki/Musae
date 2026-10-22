# -*- coding: utf-8 -*-
"""数据加载与格式自动识别。

支持的数据格式（见 gui_plan.md 附录 A）：
- 密度重建 : output/*/*.npy（3D 数组，-1=无效体素）+ 同名 .txt（JSON 参数，key 含逗号）
- 密度 CSV  : X (m),Y (m),Z (m),rho (g/cm3)，每行一个体素中心
- 地形重建 : *.npy（2D，-55=无效）+ 同名 .txt
- 真实地形 : X,Y,height → pivot 成规则网格
- Opacity  : u,v,w,bincontent,error（40 phi × 20 theta = 800 行）
"""
import json
import os
import re

import numpy as np
import pandas as pd

INVALID_DENSITY = -1.0
INVALID_TERRAIN = -55.0

# 列名变体容忍：strip + 小写后比较
_COL_ALIASES = {
    'x': ('x', 'x (m)', 'x(m)', 'x_m', 'x[m]'),
    'y': ('y', 'y (m)', 'y(m)', 'y_m', 'y[m]'),
    'z': ('z', 'z (m)', 'z(m)', 'z_m', 'z[m]'),
    'rho': ('rho', 'rho (g/cm3)', 'rho(g/cm3)', 'density', 'density (g/cm3)'),
    'height': ('height', 'height (m)', 'h', 'z (m)', 'z'),
    'u': ('u',), 'v': ('v',), 'w': ('w',),
    'bincontent': ('bincontent', 'content', 'value'),
    'error': ('error', 'err', 'uncertainty'),
}


def _find_col(cols, *names):
    """在列名列表中找第一个匹配（忽略大小写与首尾空格）。"""
    norm = {str(c).strip().lower(): str(c) for c in cols}
    for n in names:
        if n in norm:
            return norm[n]
    return None


# ---------------------------------------------------------------- 参数 txt

def parse_txt_params(txt_path):
    """解析参数 txt（JSON）。key 形如 "X_MIN, X_MAX" 的按逗号拆成多个变量。"""
    with open(txt_path, 'r', encoding='utf-8') as f:
        data = json.load(f)
    params = {}
    for key, val in data.items():
        names = [s.strip() for s in key.split(',')]
        names = [n for n in names if n]
        if len(names) == 1:
            params[names[0]] = val
        else:
            for n, v in zip(names, val):
                params[n] = v
    return params


def _grid_axes(pmin, pmax, n):
    """体素中心坐标：linspace(MIN+d/2, MAX-d/2, N)。"""
    d = (pmax - pmin) / n
    return np.linspace(pmin + d / 2.0, pmax - d / 2.0, n), d


# ---------------------------------------------------------------- 密度

def load_density_npy(path):
    """读密度 npy（+同名 txt）→ X/Y/Z 体素中心、rho(NX,NY,NZ)、params。"""
    txt_path = os.path.splitext(path)[0] + '.txt'
    if not os.path.exists(txt_path):
        raise FileNotFoundError(f'找不到同名参数文件: {txt_path}')
    rho = np.load(path)
    params = parse_txt_params(txt_path)
    nx, ny, nz = int(params['N_X']), int(params['N_Y']), int(params['N_Z'])
    if rho.shape != (nx, ny, nz):
        raise ValueError(f'npy 形状 {rho.shape} 与 txt 的 (N_X,N_Y,N_Z)=({nx},{ny},{nz}) 不符')
    X, _ = _grid_axes(params['X_MIN'], params['X_MAX'], nx)
    Y, _ = _grid_axes(params['Y_MIN'], params['Y_MAX'], ny)
    Z, _ = _grid_axes(params['Z_MIN'], params['Z_MAX'], nz)
    return {'X': X, 'Y': Y, 'Z': Z, 'rho': np.asarray(rho, dtype=np.float64),
            'params': params, 'invalid': INVALID_DENSITY,
            'source': path, 'type': 'density'}


def load_density_csv(path):
    """读密度 CSV（每行一个体素中心+值）→ 与 npy 版相同结构的 dict。"""
    df = pd.read_csv(path)
    xc = _find_col(df.columns, *_COL_ALIASES['x'])
    yc = _find_col(df.columns, *_COL_ALIASES['y'])
    zc = _find_col(df.columns, *_COL_ALIASES['z'])
    rc = _find_col(df.columns, *_COL_ALIASES['rho'])
    missing = [n for n, c in (('X', xc), ('Y', yc), ('Z', zc), ('rho', rc)) if c is None]
    if missing:
        raise ValueError(f'CSV 缺少列: {missing}（实际列: {list(df.columns)}）')
    xs = np.asarray(df[xc], dtype=np.float64)
    ys = np.asarray(df[yc], dtype=np.float64)
    zs = np.asarray(df[zc], dtype=np.float64)
    rv = np.asarray(df[rc], dtype=np.float64)
    # 由唯一坐标推断规则网格（CSV 可能是稀疏的：只存有效体素）
    x_uniq = np.unique(np.round(xs, 6))
    y_uniq = np.unique(np.round(ys, 6))
    z_uniq = np.unique(np.round(zs, 6))
    nx, ny, nz = len(x_uniq), len(y_uniq), len(z_uniq)
    for name, u, v in (('X', x_uniq, xs), ('Y', y_uniq, ys), ('Z', z_uniq, zs)):
        if len(u) > 1 and not np.allclose(np.diff(u), u[1] - u[0], rtol=1e-4):
            raise ValueError(f'CSV 的 {name} 坐标不是等间距网格: {u}')
        if not np.all(np.isin(np.round(v, 6), u)):
            raise ValueError(f'CSV 的 {name} 坐标不在规则网格上（含离群值）')
    ii = np.searchsorted(x_uniq, np.round(xs, 6))
    jj = np.searchsorted(y_uniq, np.round(ys, 6))
    kk = np.searchsorted(z_uniq, np.round(zs, 6))
    rho = np.full((nx, ny, nz), INVALID_DENSITY)
    rho[ii, jj, kk] = rv
    # 由体素中心反推边界
    X, dx = _grid_axes(x_uniq[0] - (x_uniq[1] - x_uniq[0]) / 2,
                       x_uniq[-1] + (x_uniq[1] - x_uniq[0]) / 2, nx)
    Y, dy = _grid_axes(y_uniq[0] - (y_uniq[1] - y_uniq[0]) / 2,
                       y_uniq[-1] + (y_uniq[1] - y_uniq[0]) / 2, ny)
    Z, dz = _grid_axes(z_uniq[0] - (z_uniq[1] - z_uniq[0]) / 2,
                       z_uniq[-1] + (z_uniq[1] - z_uniq[0]) / 2, nz)
    params = {'X_MIN': X[0] - dx / 2, 'X_MAX': X[-1] + dx / 2,
              'Y_MIN': Y[0] - dy / 2, 'Y_MAX': Y[-1] + dy / 2,
              'Z_MIN': Z[0] - dz / 2, 'Z_MAX': Z[-1] + dz / 2,
              'N_X': nx, 'N_Y': ny, 'N_Z': nz}
    return {'X': X, 'Y': Y, 'Z': Z, 'rho': rho, 'params': params,
            'invalid': INVALID_DENSITY, 'source': path, 'type': 'density'}


# ---------------------------------------------------------------- 地形

def load_terrain_npy(path):
    """读地形重建 npy（+同名 txt）→ X/Y 网格中心、terrain(NX,NY)、params。

    注意：notebook 渲染时用 terrain.T 作高度（Z_T[j,i] = terrain[i,j]），
    即 npy 下标 [i,j] 对应 (X_T[i], Y_T[j])，渲染时需转置。
    """
    txt_path = os.path.splitext(path)[0] + '.txt'
    if not os.path.exists(txt_path):
        raise FileNotFoundError(f'找不到同名参数文件: {txt_path}')
    terrain = np.load(path)
    params = parse_txt_params(txt_path)
    nx, ny = int(params['N_X']), int(params['N_Y'])
    if terrain.shape != (nx, ny):
        raise ValueError(f'npy 形状 {terrain.shape} 与 txt 的 (N_X,N_Y)=({nx},{ny}) 不符')
    X, _ = _grid_axes(params['X_MIN'], params['X_MAX'], nx)
    Y, _ = _grid_axes(params['Y_MIN'], params['Y_MAX'], ny)
    return {'X': X, 'Y': Y, 'terrain': np.asarray(terrain, dtype=np.float64),
            'params': params, 'invalid': INVALID_TERRAIN,
            'source': path, 'type': 'terrain'}


def load_terrain_csv(path):
    """读地形 CSV（X,Y,height）→ pivot 成规则网格（真实地形薄壳 / 地形重建 CSV 通用）。"""
    df = pd.read_csv(path)
    xc = _find_col(df.columns, *_COL_ALIASES['x'])
    yc = _find_col(df.columns, *_COL_ALIASES['y'])
    hc = _find_col(df.columns, *_COL_ALIASES['height'])
    missing = [n for n, c in (('X', xc), ('Y', yc), ('height', hc)) if c is None]
    if missing:
        raise ValueError(f'CSV 缺少列: {missing}（实际列: {list(df.columns)}）')
    try:
        piv = df.pivot(index=yc, columns=xc, values=hc)
    except ValueError:
        piv = df.pivot_table(index=yc, columns=xc, values=hc, aggfunc='mean')
    X = np.asarray(piv.columns.values, dtype=np.float64)
    Y = np.asarray(piv.index.values, dtype=np.float64)
    Z = piv.values.astype(np.float64)  # (NY, NX)，Z[j,i]
    return {'X': X, 'Y': Y, 'Z': Z, 'source': path, 'type': 'terrain_csv'}


def load_true_terrain(path):
    """真实地形薄壳（与 load_terrain_csv 相同格式）。"""
    return load_terrain_csv(path)


# ---------------------------------------------------------------- Opacity

def _centered_edges(centers):
    """由 bin 中心估计 bin 边界（相邻中点，首末对称外推）。"""
    mid = (centers[1:] + centers[:-1]) / 2.0
    left = centers[0] - (centers[1] - centers[0]) / 2.0
    right = centers[-1] + (centers[-1] - centers[-2]) / 2.0
    return np.concatenate([[left], mid, [right]])


def load_opacity_csv(path):
    """读 Opacity CSV（u,v,w,bincontent,error）→ 球面 bin 网格。

    文件按 theta（外层）× phi（内层）排列：同 w 的 40 行为一个 theta bin。
    返回 dict: theta_edges(ntheta+1), phi_edges(nphi+1),
               values(ntheta,nphi), errors(ntheta,nphi), detector, filter_kind
    """
    df = pd.read_csv(path)
    uc = _find_col(df.columns, *_COL_ALIASES['u'])
    vc = _find_col(df.columns, *_COL_ALIASES['v'])
    wc = _find_col(df.columns, *_COL_ALIASES['w'])
    bc = _find_col(df.columns, *_COL_ALIASES['bincontent'])
    ec = _find_col(df.columns, *_COL_ALIASES['error'])
    missing = [n for n, c in (('u', uc), ('v', vc), ('w', wc), ('bincontent', bc)) if c is None]
    if missing:
        raise ValueError(f'CSV 缺少列: {missing}（实际列: {list(df.columns)}）')
    u = np.asarray(df[uc], dtype=np.float64)
    v = np.asarray(df[vc], dtype=np.float64)
    w = np.asarray(df[wc], dtype=np.float64)
    val = np.asarray(df[bc], dtype=np.float64)
    err = np.asarray(df[ec], dtype=np.float64) if ec is not None else np.zeros_like(val)

    phi = np.arctan2(v, u)
    w_round = np.round(w, 6)
    uniq_w = sorted(set(w_round.tolist()), reverse=True)  # w 降序 → theta 升序
    theta_centers = np.arccos(np.clip(np.array(uniq_w), -1.0, 1.0))

    groups = {}
    for uw in uniq_w:
        m = w_round == uw
        order = np.argsort(phi[m])
        groups[uw] = (val[m][order], phi[m][order], err[m][order])
    ntheta, nphi = len(uniq_w), len(groups[uniq_w[0]][1])
    values = np.stack([groups[uw][0] for uw in uniq_w])   # (ntheta, nphi)
    errors = np.stack([groups[uw][2] for uw in uniq_w])
    phi_centers = groups[uniq_w[0]][1]
    for uw in uniq_w[1:]:
        if not np.allclose(groups[uw][1], phi_centers, atol=1e-4):
            raise ValueError(f'phi bin 中心在不同 theta 间不一致: {path}')

    phi_edges = _centered_edges(phi_centers)
    theta_edges = _centered_edges(theta_centers)
    theta_edges[0] = max(theta_edges[0], 0.0)  # 下边界不越过极点

    stem = os.path.basename(path)
    # 兼容 SimBox_A_...、TianqinNewPbCube_A_...、NewTianqin_C_... 等命名：
    # 取文件名中第一个独立的单个大写字母（A-H）作为探测器编号。
    m = re.search(r'(?:^|_)([A-H])(?=_|\.|$)', stem)
    detector = m.group(1) if m else None
    filt = None
    for kw in ('GaussianFilter', 'MeanFilter', 'MedianFilter'):
        if kw in stem:
            filt = kw
            break
    return {'theta_edges': theta_edges, 'phi_edges': phi_edges,
            'values': values, 'errors': errors,
            'detector': detector, 'filter_kind': filt,
            'source': path, 'type': 'opacity',
            'vmin': float(val.min()), 'vmax': float(val.max())}


# ---------------------------------------------------------------- 自动识别

def detect_file_type(path):
    """按扩展名+内容自动识别数据类型。

    返回: 'density' | 'terrain' | 'terrain_csv' | 'opacity' | 'unknown'
    """
    ext = os.path.splitext(path)[1].lower()
    if ext == '.csv':
        df = pd.read_csv(path, nrows=3)
        cols = {str(c).strip().lower() for c in df.columns}
        if any(a in cols for a in _COL_ALIASES['bincontent']):
            return 'opacity'
        if any(a in cols for a in _COL_ALIASES['rho']):
            return 'density'
        if any(a in cols for a in _COL_ALIASES['height']):
            return 'terrain_csv'
        return 'unknown'
    if ext == '.npy':
        txt_path = os.path.splitext(path)[0] + '.txt'
        if not os.path.exists(txt_path):
            return 'unknown'
        params = parse_txt_params(txt_path)
        if 'N_Z' in params:
            return 'density'
        if 'N_X' in params and 'N_Y' in params:
            return 'terrain'
        return 'unknown'
    return 'unknown'


LOADERS = {
    'density_npy': load_density_npy,
    'density_csv': load_density_csv,
    'terrain_npy': load_terrain_npy,
    'terrain_csv': load_terrain_csv,
}


def load_any(path):
    """自动识别并加载文件。"""
    kind = detect_file_type(path)
    if kind == 'density':
        loader = load_density_npy if path.lower().endswith('.npy') else load_density_csv
    elif kind == 'terrain':
        loader = load_terrain_npy
    elif kind == 'terrain_csv':
        loader = load_terrain_csv
    elif kind == 'opacity':
        loader = load_opacity_csv
    else:
        raise ValueError(f'无法识别的文件类型: {path}')
    return loader(path)


# ---------------------------------------------------------------- 自检

if __name__ == '__main__':
    import sys
    here = os.path.dirname(os.path.abspath(__file__))
    root = os.path.dirname(here)
    checks = []

    def ck(name, cond, info=''):
        checks.append((name, bool(cond), info))

    # 1. 密度 npy（SimBox）
    d = load_density_npy(os.path.join(root, 'output/SimBox/SimBox_L-BFGS-B_finebin_03-02.npy'))
    ck('密度npy 形状', d['rho'].shape == (120, 90, 35), str(d['rho'].shape))
    ck('密度npy 范围', d['X'][0] < d['X'][-1] and d['Z'][0] < d['Z'][-1],
       f"X[{d['X'][0]:.1f},{d['X'][-1]:.1f}] Z[{d['Z'][0]:.1f},{d['Z'][-1]:.1f}]")
    rho_v = d['rho'][d['rho'] != d['invalid']]
    ck('密度npy 有效值', len(rho_v) > 0, f'valid={len(rho_v)} min={rho_v.min():.3g} max={rho_v.max():.3g}')
    print(f"[密度npy] {d['params'].get('MEASUREMENT_LOCATION')}")

    # 2. 密度 CSV
    d2 = load_density_csv(os.path.join(root, 'output/NewTianqin/unfix_CProblem/NewTianqin_PSim_Narrowrange.csv'))
    ck('密度CSV 网格', d2['rho'].shape == (int(d2['params']['N_X']), int(d2['params']['N_Y']), int(d2['params']['N_Z'])),
       str(d2['rho'].shape))

    # 3. 地形重建 npy
    t = load_terrain_npy(os.path.join(root, 'output/Tianqin/Tianqin_terrainrecon_real_GaussianFilter_guessrho2.49_04-15.npy'))
    ck('地形npy 形状', t['terrain'].shape == (42, 40), str(t['terrain'].shape))
    tv = t['terrain'][t['terrain'] != t['invalid']]
    ck('地形npy 有效值', len(tv) > 0, f'valid={len(tv)} min={tv.min():.3g} max={tv.max():.3g}')

    # 4. 真实地形 CSV（pivot）
    tr = load_true_terrain(os.path.join(root, 'input/Terrain/SimBox_Nx2_Ny2.csv'))
    ck('真实地形 pivot', tr['Z'].shape == (len(tr['Y']), len(tr['X'])),
       f'{tr["Z"].shape} X[{tr["X"][0]:.0f},{tr["X"][-1]:.0f}] Y[{tr["Y"][0]:.0f},{tr["Y"][-1]:.0f}]')
    tr2 = load_true_terrain(os.path.join(root, 'input/Terrain/Compress_data_NEW_2m_Nx880_Ny577.csv'))
    ck('真实地形(880×577)', tr2['Z'].shape == (577, 880), str(tr2['Z'].shape))

    # 5. Opacity CSV
    o = load_opacity_csv(os.path.join(root, 'input/SimBox/SimBox_A_2e9_p3_z60_b5_r3_2_13.csv'))
    ck('opacity 网格', o['values'].shape == (20, 40), str(o['values'].shape))
    ck('opacity phi 覆盖', abs(o['phi_edges'][0] + np.pi) < 0.2 and abs(o['phi_edges'][-1] - np.pi) < 0.2,
       f'phi[{o["phi_edges"][0]:.3f},{o["phi_edges"][-1]:.3f}]')
    ck('opacity theta 覆盖', o['theta_edges'][0] >= 0 and o['theta_edges'][-1] <= np.pi / 2,
       f'theta[{o["theta_edges"][0]*57.3:.1f}°,{o["theta_edges"][-1]*57.3:.1f}°]')
    ck('opacity 探测器', o['detector'] == 'A', str(o['detector']))
    print(f"[opacity] vmin={o['vmin']:.3g} vmax={o['vmax']:.3g} filter={o['filter_kind']}")

    # 6. 自动识别
    ck('识别 密度npy', detect_file_type(os.path.join(root, 'output/SimBox/SimBox_L-BFGS-B_finebin_03-02.npy')) == 'density')
    ck('识别 密度CSV', detect_file_type(os.path.join(root, 'output/NewTianqin/unfix_CProblem/NewTianqin_PSim_Narrowrange.csv')) == 'density')
    ck('识别 地形npy', detect_file_type(os.path.join(root, 'output/Tianqin/Tianqin_terrainrecon_real_GaussianFilter_guessrho2.49_04-15.npy')) == 'terrain')
    ck('识别 地形CSV', detect_file_type(os.path.join(root, 'input/Terrain/SimBox_Nx2_Ny2.csv')) == 'terrain_csv')
    ck('识别 opacity', detect_file_type(os.path.join(root, 'input/SimBox/SimBox_A_2e9_p3_z60_b5_r3_2_13.csv')) == 'opacity')

    print()
    nfail = 0
    for name, ok, info in checks:
        mark = 'OK ' if ok else 'FAIL'
        nfail += 0 if ok else 1
        print(f'[{mark}] {name}  {info}')
    print(f'\n{len(checks)-nfail}/{len(checks)} 通过')
    sys.exit(1 if nfail else 0)
