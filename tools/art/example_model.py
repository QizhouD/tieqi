"""Builds a simple example soldier as a .blend file, inside Blender.

Usage (from the repository root):

    blender --background --factory-startup --python tools/art/example_model.py -- \\
        --side jp --out build/art/jp_samurai.blend

The figure is made of primitives on a small armature and has every action render_atlas.py expects (idle, move,
strike, aside, fallen). It is not game art: it shows artists the structure a model needs and gives the pipeline a
smoke test that runs without any model files.
"""

import math
import os
import sys

try:
    import bpy
except ImportError:  # pragma: no cover - only runs inside Blender
    bpy = None

COLOURS = {
    "ming": {"coat": (0.66, 0.22, 0.16), "trim": (0.85, 0.71, 0.40), "helmet": (0.23, 0.16, 0.13)},
    "jp": {"coat": (0.18, 0.23, 0.34), "trim": (0.67, 0.71, 0.79), "helmet": (0.11, 0.11, 0.12)},
}
SKIN = (0.85, 0.71, 0.54)
METAL = (0.19, 0.17, 0.16)


def parse(argv):
    args = argv[argv.index("--") + 1:] if "--" in argv else []
    options = {"side": "jp"}
    for i in range(0, len(args) - 1, 2):
        options[args[i].lstrip("-")] = args[i + 1]
    if "out" not in options:
        raise SystemExit("example_model.py needs --out")
    return options


def flat_material(name, colour):
    material = bpy.data.materials.new(name)
    material.use_nodes = True
    nodes = material.node_tree.nodes
    nodes.clear()
    emission = nodes.new("ShaderNodeEmission")
    # The palette is written in sRGB; shader colours are linear.
    emission.inputs["Color"].default_value = (*(channel ** 2.2 for channel in colour), 1.0)
    output = nodes.new("ShaderNodeOutputMaterial")
    material.node_tree.links.new(emission.outputs["Emission"], output.inputs["Surface"])
    return material


def add_mesh(kind, name, location, scale, material, rotation=(0.0, 0.0, 0.0)):
    if kind == "cube":
        bpy.ops.mesh.primitive_cube_add(size=1.0, location=location, rotation=rotation)
    elif kind == "sphere":
        bpy.ops.mesh.primitive_uv_sphere_add(radius=0.5, location=location, segments=16, ring_count=8)
    else:
        bpy.ops.mesh.primitive_cylinder_add(radius=0.5, depth=1.0, location=location, rotation=rotation, vertices=12)
    obj = bpy.context.active_object
    obj.name = name
    obj.scale = scale
    obj.data.materials.append(material)
    return obj


def parent_to_bone(obj, rig, bone):
    # Evaluate the new object's transform first, or its scale and rotation are lost.
    bpy.context.view_layer.update()
    world = obj.matrix_world.copy()
    obj.parent = rig
    obj.parent_type = "BONE"
    obj.parent_bone = bone
    obj.matrix_world = world


def key_pose(rig, frame, rotations, root_drop=0.0):
    """rotations: bone name -> (x, y, z) degrees in the bone's local space. With these bones, local Z swings a
    limb forward and back (toward +X), and local Y twists it about its own length."""
    for bone in rig.pose.bones:
        bone.rotation_mode = "XYZ"
        bone.rotation_euler = [math.radians(v) for v in rotations.get(bone.name, (0.0, 0.0, 0.0))]
        bone.keyframe_insert("rotation_euler", frame=frame)
        bone.location = (0.0, 0.0, 0.0)
        if bone.name == "root":
            bone.location = (0.0, -root_drop, 0.0)
        bone.keyframe_insert("location", frame=frame)


def new_action(rig, name):
    action = bpy.data.actions.new(name)
    action.use_fake_user = True
    rig.animation_data.action = action
    return action


def build(side):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    colours = COLOURS[side]
    coat = flat_material("coat", colours["coat"])
    trim = flat_material("trim", colours["trim"])
    helmet = flat_material("helmet", colours["helmet"])
    skin = flat_material("skin", SKIN)
    metal = flat_material("metal", METAL)

    figure = bpy.data.objects.new("Figure", None)
    bpy.context.scene.collection.objects.link(figure)

    # Armature: the root runs up the spine; legs hang from the hips; the arm holds the weapon. Facing +X.
    bpy.ops.object.armature_add(enter_editmode=True, location=(0.0, 0.0, 0.0))
    rig = bpy.context.active_object
    rig.name = "Rig"
    bones = rig.data.edit_bones
    root = bones[0]
    root.name = "root"
    root.head = (0.0, 0.0, 0.0)
    root.tail = (0.0, 0.0, 0.9)
    for name, y in (("leg_l", 0.12), ("leg_r", -0.12)):
        leg = bones.new(name)
        leg.head = (0.0, y, 0.9)
        leg.tail = (0.0, y, 0.08)
        leg.parent = root
    torso = bones.new("torso")
    torso.head = (0.0, 0.0, 0.9)
    torso.tail = (0.0, 0.0, 1.5)
    torso.parent = root
    arm = bones.new("arm")
    arm.head = (0.0, -0.22, 1.35)
    arm.tail = (0.35, -0.22, 1.1)
    arm.parent = torso
    bpy.ops.object.mode_set(mode="OBJECT")
    rig.parent = figure

    parts = [
        (add_mesh("cylinder", "leg_l", (0.0, 0.12, 0.48), (0.13, 0.13, 0.85), coat), "leg_l"),
        (add_mesh("cylinder", "leg_r", (0.0, -0.12, 0.48), (0.13, 0.13, 0.85), coat), "leg_r"),
        (add_mesh("cube", "torso", (0.0, 0.0, 1.18), (0.26, 0.44, 0.6), coat), "torso"),
        (add_mesh("cube", "sash", (0.0, 0.0, 0.95), (0.28, 0.46, 0.08), trim), "torso"),
        (add_mesh("sphere", "head", (0.02, 0.0, 1.62), (0.24, 0.24, 0.27), skin), "torso"),
        (add_mesh("cylinder", "hat", (0.02, 0.0, 1.75), (0.5, 0.5, 0.06), helmet), "torso"),
        (add_mesh("cube", "blade", (0.55, -0.22, 1.2), (0.85, 0.04, 0.05), metal, (0.0, math.radians(20), 0.0)),
         "arm"),
    ]
    for obj, bone in parts:
        parent_to_bone(obj, rig, bone)

    rig.animation_data_create()
    new_action(rig, "idle")
    key_pose(rig, 1, {})
    new_action(rig, "move")
    for frame, swing in ((1, 25.0), (5, 0.0), (9, -25.0), (13, 0.0), (17, 25.0)):
        key_pose(rig, frame, {"leg_l": (0.0, 0.0, swing), "leg_r": (0.0, 0.0, -swing), "arm": (0.0, 0.0, -swing * 0.3)})
    new_action(rig, "strike")
    key_pose(rig, 1, {"arm": (0.0, 0.0, 50.0), "torso": (0.0, 0.0, 0.0)})
    key_pose(rig, 9, {"arm": (0.0, 0.0, -45.0), "torso": (0.0, 0.0, -8.0), "leg_l": (0.0, 0.0, 20.0)})
    new_action(rig, "aside")
    key_pose(rig, 1, {"torso": (0.0, 35.0, 0.0)})
    new_action(rig, "fallen")
    key_pose(rig, 1, {"root": (0.0, 0.0, -85.0)})
    rig.animation_data.action = bpy.data.actions["idle"]


def main():
    options = parse(sys.argv)
    build(options["side"])
    out = os.path.abspath(options["out"])
    os.makedirs(os.path.dirname(out), exist_ok=True)
    bpy.ops.wm.save_as_mainfile(filepath=out)
    print(f"wrote {out}")


if __name__ == "__main__":
    if bpy is None:
        raise SystemExit("run this script inside Blender: blender --background --python ...")
    main()
