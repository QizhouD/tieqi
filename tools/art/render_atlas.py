"""Renders one side and kind of soldier into an atlas strip, inside Blender.

Usage (Blender 4.x, from the repository root):

    blender --background art/ming_cavalry.blend --python tools/art/render_atlas.py -- \\
        --side ming --kind cavalry --out build/art/strips/ming_cavalry.png

The .blend file must contain:
- an object named "Figure" (usually an empty) that parents the whole model; the model faces +X, stands on the
  ground plane at the origin, and is modelled at real scale in metres;
- actions named "idle", "move", "strike", "aside", and "fallen" on the object that carries the animation (the
  armature, or "Figure" itself). "move" is sampled at 4 evenly spaced frames and "strike" at 2; the others at their
  first frame. A missing action falls back to "idle" with a warning. The fallen frame is centred in the cell
  automatically, so "fallen" may pivot about the feet;
- flat, unlit materials (an Emission shader or similar), so the ink-line style reads as flat colour.

Figures in the game are drawn larger than life (a 32-pixel cell spans 9 m), so --figure-scale (default 2.8) scales
the real-scale model up to the size of the placeholder figures.

The script sets up an orthographic camera at the three-quarter elevation used by the placeholder figures, turns on
Freestyle ink lines, renders every direction and frame at a supersampled size, scales each cell down with
tools/art/pngio.py, and writes a strip of 8 rows (directions 0 east, then clockwise) by 9 columns (frames in
atlas order). tools/art/assemble_atlas.py joins the ten strips into the full atlas.
"""

import math
import os
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import atlas_layout as layout  # noqa: E402
import pngio  # noqa: E402

try:
    import bpy
    import mathutils
except ImportError:  # pragma: no cover - only runs inside Blender
    bpy = None

# Camera elevation in degrees; 30 degrees halves the ground plane like the placeholder projection.
ELEVATION = 30.0
CAMERA_DISTANCE = 50.0
# Frames of each action, in atlas column order.
FRAME_SOURCES = [
    ("idle", 0, 1),
    ("move", 0, 4),
    ("move", 1, 4),
    ("move", 2, 4),
    ("move", 3, 4),
    ("strike", 0, 2),
    ("strike", 1, 2),
    ("aside", 0, 1),
    ("fallen", 0, 1),
]


def parse(argv):
    args = argv[argv.index("--") + 1:] if "--" in argv else []
    options = {"supersample": 4, "line": 0.5, "figure-scale": 2.8}
    i = 0
    while i < len(args):
        key = args[i].lstrip("-")
        options[key] = args[i + 1]
        i += 2
    for required in ("side", "kind", "out"):
        if required not in options:
            raise SystemExit(f"render_atlas.py needs --{required}")
    if options["side"] not in layout.SIDES or options["kind"] not in layout.KINDS:
        raise SystemExit(f"side must be one of {layout.SIDES}, kind one of {layout.KINDS}")
    options["supersample"] = int(options["supersample"])
    options["line"] = float(options["line"])
    options["figure-scale"] = float(options["figure-scale"])
    return options


def animated_object():
    """The object whose animation data drives the figure: an armature under Figure, or Figure itself."""
    figure = bpy.data.objects.get("Figure")
    if figure is None:
        raise SystemExit('the .blend file needs an object named "Figure"')
    for child in figure.children_recursive:
        if child.type == "ARMATURE":
            return figure, child
    return figure, figure


def set_pose(target, action_name, index, count):
    action = bpy.data.actions.get(action_name) or bpy.data.actions.get("idle")
    if action is None:
        raise SystemExit('the .blend file needs at least an "idle" action')
    if action.name != action_name:
        print(f"warning: no action {action_name!r}; using {action.name!r}")
    if target.animation_data is None:
        target.animation_data_create()
    target.animation_data.action = action
    start, end = action.frame_range
    frame = start + (end - start) * index / max(count, 1)
    bpy.context.scene.frame_set(int(round(frame)))


def setup_scene(size, line):
    scene = bpy.context.scene
    # Blender 4.2 to 4.5 call the engine BLENDER_EEVEE_NEXT; earlier and later versions call it BLENDER_EEVEE.
    for engine in ("BLENDER_EEVEE_NEXT", "BLENDER_EEVEE"):
        try:
            scene.render.engine = engine
            break
        except TypeError:
            continue
    scene.render.resolution_x = size
    scene.render.resolution_y = size
    scene.render.resolution_percentage = 100
    scene.render.film_transparent = True
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGBA"
    scene.view_settings.view_transform = "Standard"
    scene.render.use_freestyle = True
    scene.render.line_thickness_mode = "ABSOLUTE"
    scene.render.line_thickness = line
    # Make sure there is an ink line set with a line style, even in a file that never set up Freestyle.
    settings = bpy.context.view_layer.freestyle_settings
    settings.mode = "EDITOR"
    if not settings.linesets:
        settings.linesets.new("Ink")
    for lineset in settings.linesets:
        if lineset.linestyle is None:
            lineset.linestyle = bpy.data.linestyles.new("Ink")
        lineset.linestyle.color = (0.10, 0.07, 0.05)
        lineset.linestyle.thickness = line
    camera_data = bpy.data.cameras.new("AtlasCamera")
    camera_data.type = "ORTHO"
    camera_data.ortho_scale = layout.CELL_METERS
    # Put the feet at the anchor row: shift the frame up by the anchor's offset below the centre.
    camera_data.shift_y = (layout.ANCHOR_Y - layout.CELL_PX / 2) / layout.CELL_PX
    camera = bpy.data.objects.new("AtlasCamera", camera_data)
    scene.collection.objects.link(camera)
    elevation = math.radians(ELEVATION)
    camera.location = (0.0, -CAMERA_DISTANCE * math.cos(elevation), CAMERA_DISTANCE * math.sin(elevation))
    camera.rotation_euler = (math.radians(90.0 - ELEVATION), 0.0, 0.0)
    scene.camera = camera


def centre_on_ground(figure):
    """Moves the figure so the middle of its posed meshes sits over the origin, as the fallen frame requires."""
    bpy.context.view_layer.update()
    depsgraph = bpy.context.evaluated_depsgraph_get()
    xs = []
    ys = []
    for obj in figure.children_recursive:
        if obj.type != "MESH":
            continue
        evaluated = obj.evaluated_get(depsgraph)
        for corner in evaluated.bound_box:
            world = evaluated.matrix_world @ mathutils.Vector(corner)
            xs.append(world.x)
            ys.append(world.y)
    if xs:
        figure.location.x -= (min(xs) + max(xs)) * 0.5
        figure.location.y -= (min(ys) + max(ys)) * 0.5
    # The fallen frame is centred in the cell, not on the feet anchor: push it back across the ground until it
    # projects onto the cell centre.
    metres_per_pixel = layout.CELL_METERS / layout.CELL_PX
    figure.location.y += (layout.ANCHOR_Y - layout.CELL_PX / 2) * metres_per_pixel / math.sin(math.radians(ELEVATION))


def render_cell(path):
    bpy.context.scene.render.filepath = path
    bpy.ops.render.render(write_still=True)
    return pngio.read(path)


def main():
    options = parse(sys.argv)
    supersample = options["supersample"]
    size = layout.CELL_PX * supersample
    setup_scene(size, options["line"] * supersample)
    figure, target = animated_object()
    figure.scale = (options["figure-scale"],) * 3
    strip = pngio.Image(layout.CELL_PX * len(layout.FRAMES), layout.CELL_PX * layout.DIRECTIONS)
    with tempfile.TemporaryDirectory() as scratch:
        for direction in range(layout.DIRECTIONS):
            # Direction 0 faces east (+X); the atlas turns clockwise on screen, which is toward the camera (-Y).
            figure.rotation_euler[2] = -math.radians(45.0 * direction)
            for column, (action, index, count) in enumerate(FRAME_SOURCES):
                figure.location = (0.0, 0.0, 0.0)
                set_pose(target, action, index, count)
                if action == "fallen":
                    centre_on_ground(figure)
                cell = render_cell(os.path.join(scratch, f"cell_{direction}_{column}.png"))
                strip.blit(cell.downscale(supersample), column * layout.CELL_PX, direction * layout.CELL_PX)
                print(f"rendered {options['side']}/{options['kind']} dir {direction} {layout.FRAMES[column]}")
    os.makedirs(os.path.dirname(os.path.abspath(options["out"])), exist_ok=True)
    pngio.write(options["out"], strip)
    print(f"wrote {options['out']}")


if __name__ == "__main__":
    if bpy is None:
        raise SystemExit("run this script inside Blender: blender --background <file.blend> --python ...")
    main()
