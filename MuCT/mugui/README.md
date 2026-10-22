# mugui — MuCT 重建结果可视化 GUI

替代 `mugrid_visualize.ipynb` 与 `mugrid_visualize_terrainrecon.ipynb` 的交互式查看功能，
并新增 Opacity 半球视图。PyQt5 + VTK 原生渲染。

## 运行

```bash
cd /home/qisongran/MuCT_Project/MuCT
muCT/bin/python mugui/main.py
```

> 若启动报 Qt 平台插件错误（xcb 加载失败），多半是系统 Qt5 与 venv 自带 Qt5
> 版本冲突；`main.py` 已自动把 venv 自带的 Qt5 运行库加进 `LD_LIBRARY_PATH`，
> 一般无需手动处理。

## 三个标签页

| 标签页 | 数据 | 说明 |
|---|---|---|
| 密度重建 | `.npy`+同名`.txt` 或 CSV（`X (m),Y (m),Z (m),rho (g/cm3)`） | 每个有效体素一个六面体 cell；`-1`=无效体素；默认剔除首层 z（与 notebook `rho[:,:,1:]` 一致） |
| 地形重建 | `.npy`+`.txt` 或 CSV（`X,Y,height`） | 高程曲面 + 真实地形半透明叠加；中值滤波(3×3)/高斯平滑可选；`-55` 无效值显示为洞 |
| Opacity 半球 | SimBox CSV（可多选，每文件一个探测器） | 半径与数值成正比 `r = r_min+(r_max-r_min)·(v/vmax)^γ`，颜色映射 g/cm² |

## 功能

- 场景预设（`scene_presets.json`）：SimBox / Tianqin 的探测器位置、视野范围、
  辅助几何（world box、隧道盒）与真实地形 CSV；按文件名自动匹配，也可手动选择
  「自定义」并逐行输入探测点（`名称 x y z`）。
- 密度视图：颜色方案（RdPu/turbo/viridis…）、vmin/vmax 双滑块、最小阈值过滤、
  透明模式、隐藏区间（复刻 notebook 的 `opacity_func`）、真实地形薄壳、
  测量点红点+标签、坐标轴角标。
- 地形视图：中值滤波、高斯平滑 σ、无效值洞开关、高度范围与配色（灰度/地形色…）、
  真实地形叠加透明度。
- Opacity 视图：r_min/r_max、γ（开方/四次方根/线性）、对数颜色刻度、隐藏 0 值 bin、
  每 bin 细分（2×2~8×8）、探测器红球。
- 通用：鼠标交互（左键旋转/滚轮缩放/中键平移）、「重置相机」、PNG 截图（1x/2x/4x）。

## 目录

```
mugui/
  main.py             # 入口（含 Qt5 库路径修复）
  app.py              # MainWindow：三标签页 + 工具栏 + 状态栏
  widgets.py          # 控制面板控件（RangeSlider/SliderSpin/预设行/三个面板）
  loaders.py          # 数据加载与格式自动识别（含自检: python loaders.py）
  render.py           # VTK 管线（体素/曲面/半球/场景元素/三个视图类）
  scene_presets.json  # 场景预设
```

## 实现说明

- 体素网格用 numpy 向量化构建；阈值过滤通过 numpy 原位更新 keep/ghost 掩码实现，
  拖动滑块无需重建网格。
- 地形曲面为四边形网格（有效 cell 才构建 → 天然打洞），与 notebook 的
  `terrain.T` 方向一致。
- 不开启 VTK 深度剥离：部分软渲染驱动（llvmpipe）下大透明曲面会整体消失。
- 中文图例使用系统 Noto Sans CJK 字体（VTK fontconfig 解析）。
