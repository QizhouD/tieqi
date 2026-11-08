"""Near-view map art contract; mirrored by the presentation layer's C++ constants.

All dimensions are pixels unless suffixed METERS. Luminance is normalized Rec. 709
on the stored RGB values (not linear-light conversion), ignoring tile alpha.
"""

PIXELS_PER_METER = 8.0
TREE_CELL_PX = 192
TREE_CELLS = 8
TREE_WIDTH = 1536
TREE_HEIGHT = 192
TREE_CELL_METERS = 24.0
TREE_ANCHOR_X = 96
TREE_ANCHOR_Y = 184
TREE_ANCHOR_TOLERANCE_X = 8
TREE_ANCHOR_TOLERANCE_Y = 4
TREE_PADDING = 2
TREE_OPAQUE_ALPHA = 128
TREE_HEIGHT_MIN = 120
TREE_HEIGHT_MAX = 170
SHRUB_HEIGHT_MIN = 50
SHRUB_HEIGHT_MAX = 80
TREE_NAMES = (
    "tall pine", "crooked pine", "bare deciduous", "broad bare deciduous",
    "bare willow", "low shrub clump", "young pine", "bare sapling pair",
)

GROUND_PX = 512
GROUND_METERS = 64.0
PADDY_PX = 256
PADDY_METERS = 32.0
PADDY_ROW_PX = 5.0
LUMINANCE_MEAN_MIN = 0.86
LUMINANCE_MEAN_MAX = 0.94
LUMINANCE_DARK_PERCENTILE = 0.01
LUMINANCE_DARK_MIN = 0.45
SEAM_RATIO_MAX = 2.0
