"""Shared procedural figure geometry and rigid-part armatures for the M5 atlas.

Run the family entry points in Blender. Dimensions are metres before the atlas's
deliberate enlargement. Small equipment is exaggerated for a 32-pixel cell.
"""

import argparse
from pathlib import Path
import sys

import bpy
from mathutils import Vector

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from example_model import flat_material, key_pose, new_action, parent_to_bone


def material_set(side):
    colours = {
        "coat": ("#a8382a" if side == "ming" else "#2f3a56"),
        "trim": ("#d9b466" if side == "ming" else "#aab4c9"),
        "armour": ("#65392d" if side == "ming" else "#30343c"),
        "ink": "#272522", "skin": "#c9a27c", "steel": "#b5b8ae",
        "wood": "#796047", "horse": "#80604a", "horse_light": "#a18160",
        "mane": "#3c3029", "cloth": "#d2c3a0", "flash": "#f4cf76",
    }
    return {name: flat_material(name, tuple(int(value[i:i + 2], 16) / 255 for i in (1, 3, 5)))
            for name, value in colours.items()}


class FigureBuilder:
    def __init__(self, side, kind):
        bpy.ops.wm.read_factory_settings(use_empty=True)
        self.side, self.kind = side, kind
        self.mounted = kind in ("cavalry", "general")
        self.general = kind == "general"
        self.mats = material_set(side)
        self.figure = bpy.data.objects.new("Figure", None)
        bpy.context.scene.collection.objects.link(self.figure)
        bpy.ops.object.armature_add(enter_editmode=True)
        self.rig = bpy.context.object
        self.rig.name = "Rig"
        bones = self.rig.data.edit_bones
        bones.remove(bones[0])
        # All bones point up: local Y is world Z, local Z is world -Y.
        self.bone("root", (0, 0, 0), None)
        hip = 1.55 if self.mounted else 0.78
        self.hip = hip
        self.bone("torso", (0, 0, hip), "root")
        self.bone("head", (0, 0, hip + 0.63), "torso")
        for sign, suffix in ((1, "l"), (-1, "r")):
            self.bone("leg_" + suffix, (0, sign * 0.19, hip), "torso" if self.mounted else "root")
            self.bone("arm_" + suffix, (0, sign * 0.28, hip + 0.48), "torso")
        self.bone("weapon", (0.25, -0.34, hip + 0.3), "torso")
        self.bone("flash", (1.35, -0.34, hip + 0.52), "weapon")
        if self.mounted:
            for x, end in ((0.58, "front"), (-0.62, "rear")):
                for y, suffix in ((0.25, "l"), (-0.25, "r")):
                    self.bone(end + "_" + suffix, (x, y, 1.05), "root")
            self.bone("neck", (0.55, 0, 1.3), "root")
            self.bone("tail", (-0.95, 0, 1.3), "root")
        bpy.ops.object.mode_set(mode="OBJECT")
        self.rig.parent = self.figure

    def bone(self, name, head, parent):
        bones = self.rig.data.edit_bones
        bone = bones.new(name)
        bone.head = head
        bone.tail = Vector(head) + Vector((0, 0, 0.3))
        if parent:
            bone.parent = bones[parent]

    def mesh(self, shape, name, pos, size, mat, bone="torso", rotation=None):
        if shape == "box":
            bpy.ops.mesh.primitive_cube_add(size=1, location=pos)
        elif shape == "cone":
            bpy.ops.mesh.primitive_cone_add(vertices=12, radius1=0.5, radius2=0.04, depth=1, location=pos)
        elif shape == "ellipsoid":
            bpy.ops.mesh.primitive_uv_sphere_add(segments=12, ring_count=6, radius=0.5, location=pos)
        else:
            bpy.ops.mesh.primitive_cylinder_add(vertices=10, radius=0.5, depth=1, location=pos)
        obj = bpy.context.object
        obj.name = name
        obj.scale = size
        if rotation:
            obj.rotation_euler = rotation
        obj.data.materials.append(self.mats[mat])
        parent_to_bone(obj, self.rig, bone)
        return obj

    def rod(self, name, a, b, width, mat, bone="torso"):
        delta = Vector(b) - Vector(a)
        obj = self.mesh("cylinder", name, (Vector(a) + Vector(b)) / 2,
                        (width, width, delta.length), mat, bone)
        # Set the world orientation before restoring the bone-relative matrix.
        world = obj.matrix_world.copy()
        rotation = delta.to_track_quat("Z", "Y").to_matrix().to_4x4()
        from mathutils import Matrix
        obj.matrix_world = Matrix.Translation(world.translation) @ rotation @ Matrix.Diagonal(
            Vector((width, width, delta.length, 1)))
        return obj

    def horse(self):
        self.mesh("ellipsoid", "stocky barrel", (-0.08, 0, 1.13), (1.9, 0.75, 0.86), "horse", "root")
        self.mesh("ellipsoid", "chest", (0.56, 0, 1.26), (0.7, 0.8, 0.95), "horse_light", "root")
        self.rod("sloping neck", (0.58, 0, 1.35), (0.93, 0, 1.91), 0.51, "horse", "neck")
        self.mesh("ellipsoid", "long horse head", (1.1, 0, 1.94), (0.77, 0.35, 0.42), "horse_light", "neck",
                  (0, 0.3, 0))
        self.mesh("ellipsoid", "muzzle", (1.4, 0, 1.84), (0.35, 0.33, 0.3), "mane", "neck")
        for y in (-0.13, 0.13):
            self.mesh("cone", "ear", (0.93, y, 2.22), (0.15, 0.14, 0.32), "horse", "neck")
            self.mesh("ellipsoid", "eye", (1.15, y * 1.28, 2.01), (0.075, 0.035, 0.075), "ink", "neck")
            self.rod("bridle", (1.28, y * 1.4, 1.77), (0.96, y * 1.4, 2.13), 0.055, "trim", "neck")
            self.rod("rein", (1.26, y * 1.4, 1.82), (0.24, y * 1.9, 1.98), 0.035, "ink", "torso")
        self.rod("mane", (0.46, 0, 1.45), (0.81, 0, 2.06), 0.2, "mane", "neck")
        for x, end in ((0.58, "front"), (-0.62, "rear")):
            for y, suffix in ((0.25, "l"), (-0.25, "r")):
                bone = end + "_" + suffix
                knee = (x + (0.08 if end == "front" else -0.16), y, 0.52)
                self.rod("upper " + bone, (x, y, 1.06), knee, 0.22, "horse", bone)
                self.rod("shin " + bone, knee, (x + 0.04, y, 0.12), 0.12, "horse_light", bone)
                self.mesh("box", "hoof " + bone, (x + 0.08, y, 0.09), (0.25, 0.19, 0.17), "ink", bone)
        self.rod("tail", (-0.94, 0, 1.35), (-1.2, 0, 0.53), 0.24, "mane", "tail")
        self.mesh("box", "saddle blanket", (-0.1, 0, 1.46), (0.92, 0.92, 0.13), "coat", "root")
        for y in (-0.42, 0.42):
            self.mesh("box", "blanket skirt", (-0.15, y, 1.25), (0.93, 0.08, 0.45), "coat", "root")
            self.mesh("box", "blanket border", (-0.15, y * 1.04, 1.04), (0.94, 0.035, 0.08), "trim", "root")
        self.mesh("ellipsoid", "saddle", (-0.07, 0, 1.54), (0.7, 0.65, 0.27), "ink", "root")

    def soldier(self):
        h = self.hip
        self.mesh("ellipsoid", "padded coat", (0, 0, h + 0.28), (0.48, 0.72, 0.68), "coat")
        self.mesh("box", "cuirass", (0.06, 0, h + 0.3), (0.46, 0.62, 0.51), "armour")
        self.mesh("box", "sash", (0.04, 0, h + 0.06), (0.49, 0.67, 0.1), "trim")
        for y in (-0.21, 0.21):
            suffix = "l" if y > 0 else "r"
            knee = (0.13 if self.mounted else 0, y * (1.9 if self.mounted else 1), h - 0.35)
            foot = (0.08, knee[1], h - (0.63 if self.mounted else 0.7))
            self.rod("trousers", (0, y, h), knee, 0.25, "coat", "leg_" + suffix)
            self.rod("greave", knee, foot, 0.17, "armour", "leg_" + suffix)
            self.mesh("box", "boot", (foot[0] + 0.06, foot[1], foot[2]), (0.34, 0.23, 0.17), "ink", "leg_" + suffix)
            self.mesh("box", "armour skirt", (-0.02, y * 1.38, h - 0.04), (0.43, 0.13, 0.35), "coat" if self.side == "ming" else "armour")
            self.mesh("box", "shoulder", (0, y * 1.6, h + 0.47), (0.4, 0.22, 0.24), "coat", "arm_" + suffix)
            elbow = (0.13, y * 1.7, h + 0.2)
            hand = (0.32, y * 1.6, h + 0.27)
            self.rod("upper sleeve", (0, y * 1.45, h + 0.43), elbow, 0.22, "coat", "arm_" + suffix)
            self.rod("forearm", elbow, hand, 0.18, "armour", "arm_" + suffix)
            self.mesh("ellipsoid", "hand", hand, (0.2, 0.18, 0.17), "skin", "arm_" + suffix)
        self.mesh("ellipsoid", "face", (0.035, 0, h + 0.72), (0.33, 0.34, 0.4), "skin", "head")
        self.mesh("box", "nose", (0.2, 0, h + 0.72), (0.09, 0.09, 0.12), "skin", "head")
        if self.side == "ming":
            # Broad trim and a few large rivets survive reduction better than a fine grid.
            self.mesh("box", "brigandine front", (0.3, 0, h + 0.3), (0.045, 0.57, 0.5), "coat")
            for y in (-0.18, 0.18):
                for z in (h + 0.19, h + 0.4):
                    self.mesh("ellipsoid", "brigandine rivet", (0.33, y, z), (0.05, 0.065, 0.065), "trim")
            self.mesh("ellipsoid", "bowl helmet", (0, 0, h + 0.88), (0.47, 0.5, 0.35), "armour", "head")
            self.mesh("cylinder", "helmet rim", (0, 0, h + 0.83), (0.53, 0.56, 0.065), "trim", "head")
            self.mesh("box", "neck guard", (-0.17, 0, h + 0.68), (0.16, 0.48, 0.32), "coat", "head")
            self.rod("plume tube", (0, 0, h + 0.99), (0, 0, h + 1.15), 0.09, "trim", "head")
            self.mesh("ellipsoid", "red tassel", (-0.08, 0, h + 1.18), (0.3, 0.16, 0.25), "coat", "head")
        else:
            for z in (h + 0.17, h + 0.31, h + 0.45):
                self.mesh("box", "armour lacing", (0.3, 0, z), (0.04, 0.59, 0.04), "trim")
            if self.kind in ("spear", "gun"):
                self.mesh("cone", "jingasa", (0, 0, h + 0.91), (0.82, 0.82, 0.29), "ink", "head")
                self.mesh("cylinder", "jingasa rim", (0, 0, h + 0.78), (0.82, 0.82, 0.04), "trim", "head")
            else:
                self.mesh("ellipsoid", "kabuto bowl", (0, 0, h + 0.91), (0.5, 0.54, 0.39), "ink", "head")
                for z, width in ((0.68, 0.64), (0.77, 0.59), (0.85, 0.54)):
                    self.mesh("box", "shikoro", (-0.13, 0, h + z), (0.37, width, 0.09), "armour", "head")
                for sign in (-1, 1):
                    self.rod("helmet crest", (0.22, sign * 0.1, h + 0.99),
                             (0.25, sign * (0.38 if self.general else 0.24), h + (1.28 if self.general else 1.16)),
                             0.09, "trim", "head")
                if not self.general:
                    self.rod("sashimono pole", (-0.29, 0, h + 0.1), (-0.29, 0, h + 1.21), 0.045, "wood")
                    self.mesh("box", "plain sashimono", (-0.51, 0, h + 0.99), (0.39, 0.045, 0.45), "cloth")
        if self.general:
            self.mesh("box", "gilded breastplate", (0.32, 0, h + 0.3), (0.06, 0.38, 0.28), "trim")
            self.mesh("box", "cape" if self.side == "ming" else "jinbaori", (-0.3, 0, h + 0.08),
                      (0.14, 0.82, 0.88), "coat", rotation=(0, -0.25, 0))
            for y in (-0.39, 0.39):
                self.mesh("box", "command coat border", (-0.32, y, h + 0.05), (0.14, 0.055, 0.85), "trim")

    def weapons(self):
        h = self.hip
        if self.kind == "spear":
            # Compressed pole length preserves body size and the cell padding in every view.
            self.rod("long spear shaft", (-1.35, -0.34, h - 0.05), (1.55, -0.34, h + 1.0), 0.065, "wood", "weapon")
            self.rod("spear blade", (1.55, -0.34, h + 1.0), (1.83, -0.34, h + 1.1), 0.11, "steel", "weapon")
        elif self.kind == "gun":
            self.rod("arquebus stock", (-0.27, -0.34, h + 0.2), (0.72, -0.34, h + 0.45), 0.14, "wood", "weapon")
            self.rod("arquebus barrel", (0.35, -0.34, h + 0.4), (1.38, -0.34, h + 0.54), 0.085, "steel", "weapon")
            self.mesh("box", "matchlock", (0.34, -0.43, h + 0.43), (0.18, 0.08, 0.13), "trim", "weapon")
            self.mesh("ellipsoid", "muzzle flash", (1.48, -0.34, h + 0.54), (0.42, 0.21, 0.21), "flash", "flash")
        elif self.side == "ming" and self.kind == "cavalry":
            self.rod("three eyed gun haft", (-0.15, -0.34, h + 0.1), (0.62, -0.34, h + 0.54), 0.12, "wood", "weapon")
            for y, z in ((-0.06, 0), (0.06, 0), (0, 0.11)):
                self.rod("three eyed barrel", (0.54, -0.34 + y, h + 0.49 + z), (1.08, -0.34 + y, h + 0.8 + z),
                         0.095, "steel", "weapon")
            # Stowed sabre and bow are broad silhouettes, not subpixel strings.
            self.rod("sabre scabbard", (-0.18, 0.43, h + 0.08), (-0.8, 0.43, h - 0.36), 0.095, "ink")
            self.rod("bow upper", (-0.21, 0.4, h + 0.72), (-0.48, 0.4, h + 0.15), 0.07, "wood")
            self.rod("bow lower", (-0.48, 0.4, h + 0.15), (-0.2, 0.4, h - 0.4), 0.07, "wood")
        else:
            self.rod("sword grip", (0.17, -0.34, h + 0.25), (0.43, -0.34, h + 0.42), 0.11, "ink", "weapon")
            self.rod("sword guard", (0.35, -0.49, h + 0.38), (0.35, -0.19, h + 0.38), 0.07, "trim", "weapon")
            self.rod("sabre blade", (0.43, -0.34, h + 0.42), (1.12, -0.34, h + 0.93), 0.09, "steel", "weapon")
            self.rod("sabre tip", (1.12, -0.34, h + 0.93), (1.25, -0.34, h + 1.1), 0.065, "steel", "weapon")
            if self.side == "ming" and self.kind == "samurai":
                self.mesh("ellipsoid", "round shield", (0.25, 0.4, h + 0.16), (0.17, 0.72, 0.75), "wood", "arm_l")
                self.mesh("ellipsoid", "shield boss", (0.35, 0.4, h + 0.16), (0.09, 0.22, 0.22), "trim", "arm_l")

    def pose(self, frame, rotations, bob=0, flash=False):
        key_pose(self.rig, frame, rotations, root_drop=-bob)
        bone = self.rig.pose.bones["flash"]
        bone.scale = (1, 1, 1) if flash else (0.001, 0.001, 0.001)
        bone.keyframe_insert("scale", frame=frame)

    def animate(self):
        self.rig.animation_data_create()
        new_action(self.rig, "idle")
        self.pose(1, {})
        new_action(self.rig, "move")
        for frame, swing in ((1, 24), (5, 0), (9, -24), (13, 0), (17, 24)):
            pose = {"arm_l": (0, 0, swing * 0.25), "arm_r": (0, 0, -swing * 0.25),
                    "weapon": (0, 0, -swing * 0.15)}
            if self.mounted:
                pose.update({"front_l": (0, 0, swing), "rear_r": (0, 0, swing),
                             "front_r": (0, 0, -swing), "rear_l": (0, 0, -swing),
                             "neck": (0, 0, swing * 0.12), "tail": (swing * 0.4, 0, 0)})
            else:
                pose.update({"leg_l": (0, 0, swing), "leg_r": (0, 0, -swing)})
            self.pose(frame, pose, 0.045 if frame in (5, 13) else 0)
        new_action(self.rig, "strike")
        # Renderer samples frames 1 and 9 of a 1..17 action range.
        if self.kind == "gun":
            ready = {"weapon": (0, 0, -8), "arm_l": (0, 0, -8), "arm_r": (0, 0, -8)}
            fire = {"weapon": (0, 0, -2), "arm_l": (0, 0, -2), "arm_r": (0, 0, -2)}
        elif self.kind == "spear":
            ready = {"weapon": (0, 0, 8), "torso": (0, 0, -4)}
            fire = {"weapon": (0, 0, -20), "arm_r": (0, 0, -18)}
        else:
            ready = {"weapon": (0, 0, 35), "arm_r": (0, 0, 25)}
            fire = {"weapon": (0, 0, -32), "torso": (0, 0, 8), "arm_r": (0, 0, -20)}
        self.pose(1, ready)
        self.pose(9, fire, flash=self.kind == "gun")
        self.pose(17, ready)
        new_action(self.rig, "aside")
        self.pose(1, {"head": (0, 38, 0)})
        new_action(self.rig, "fallen")
        self.pose(1, {"root": (88, 0, 0) if self.mounted else (0, 0, -88),
                      "leg_l": (12, 0, 18), "leg_r": (-12, 0, -12),
                      "arm_l": (-20, 0, -15), "weapon": (0, 0, 15)}, bob=0.3)
        self.rig.animation_data.action = bpy.data.actions["idle"]
        bpy.context.scene.frame_set(1)


def main(side, kinds):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--kind", choices=kinds, default=kinds[0])
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args(sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else [])
    model = FigureBuilder(side, args.kind)
    if model.mounted:
        model.horse()
    model.soldier()
    model.weapons()
    model.animate()
    args.out.parent.mkdir(parents=True, exist_ok=True)
    bpy.ops.wm.save_as_mainfile(filepath=str(args.out.resolve()))
    print(f"Built {side}/{args.kind}: {len(model.rig.pose.bones)} bones, five actions")
