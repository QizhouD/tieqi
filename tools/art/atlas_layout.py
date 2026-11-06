"""The soldier atlas layout, mirrored from src/app/SoldierAtlas.h and src/app/Soldiers.h.

Keep these numbers in step with the C++ constants; tools/art/check_atlas.py validates an atlas against them.
"""

CELL_PX = 32
CELL_METERS = 9.0
ANCHOR_X = 16
ANCHOR_Y = 26

SIDES = ["ming", "jp"]
KINDS = ["cavalry", "spear", "gun", "samurai", "general"]
DIRECTIONS = 8

# Column = frame.
FRAMES = ["idle", "move1", "move2", "move3", "move4", "strike1", "strike2", "aside", "fallen"]

FIGURE_ROWS = len(SIDES) * len(KINDS) * DIRECTIONS
EXTRA_ROW = FIGURE_ROWS
ROWS = FIGURE_ROWS + 1
EXTRA_COLUMNS = {"dust": 0, "ming_banner": 1, "jp_vanguard_banner": 2, "jp_main_banner": 3}

WIDTH = CELL_PX * len(FRAMES)
HEIGHT = CELL_PX * ROWS


def row(side, kind, direction):
    """Atlas row of a figure: (side * kinds + kind) * 8 + direction; direction 0 east, then clockwise."""
    return (SIDES.index(side) * len(KINDS) + KINDS.index(kind)) * DIRECTIONS + direction
