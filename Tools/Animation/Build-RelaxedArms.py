"""Run in UnrealEditor-Cmd with -run=pythonscript -script=<this file>.

Transfers the existing body's relaxed idle and locomotion to the arm skeleton.
The arm skeleton omits the spine: bake that chain into the shoulders, keep the
root stationary and preserve arm/finger bone lengths. Held-item clips are separate.
"""
import math
import unreal

DEST = '/Game/Gameplay/Player/Animations'
SOURCE = '/Game/ReverseAsset/Player/Animations'
ARM_MESH = '/Game/ReverseAsset/Player/Characters/Arms/ArmsMesh/SkeletalMeshes/ArmsMesh'
BODY_MESH = '/Game/ReverseAsset/Player/Characters/Hazmat/Hazmat/SkeletalMeshes/Hazmat'
LOCAL, WORLD = unreal.AnimPoseSpaces.LOCAL, unreal.AnimPoseSpaces.WORLD
arms, body = unreal.load_asset(ARM_MESH), unreal.load_asset(BODY_MESH)
assert arms and body
arm_options = unreal.AnimPoseEvaluationOptions(optional_skeletal_mesh=arms, should_retarget=False)
body_options = unreal.AnimPoseEvaluationOptions(optional_skeletal_mesh=body, should_retarget=False)
arm_pose = unreal.load_asset(SOURCE + '/Arms/A_Player_Arms_Idle').get_anim_pose_at_frame(0, arm_options)
relaxed_pose = unreal.load_asset(SOURCE + '/Body/A_Player_Breathing_Idle').get_anim_pose_at_frame(0, body_options)
bones = list(arm_pose.get_bone_names())
root = arm_pose.get_bone_pose('Hips', LOCAL)
idle_hips = relaxed_pose.get_bone_pose('Hips', WORLD).translation
# A cropped first-person mesh needs its open shoulder ends behind the view.
# Anchor the shoulders there and angle the arms gently forward so the hands are
# visible when looking down. Preserve the source gait, at first-person amplitude.
half_tilt = math.radians(35) / 2
shoulder_tilt = unreal.Quat(math.sin(half_tilt), 0, 0, math.cos(half_tilt))
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

for suffix, source in [('Idle', 'A_Player_Breathing_Idle'), ('Walk', 'A_Player_Walk'), ('Run', 'A_Player_Fast_Run')]:
    body_anim = unreal.load_asset(SOURCE + '/Body/' + source)
    frames = round(body_anim.get_play_length() * 30)
    name = 'A_BR_Arms_Relaxed' + suffix
    path = DEST + '/' + name
    seq = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
    if not seq:
        factory = unreal.AnimSequenceFactory()
        factory.target_skeleton = arms.skeleton
        seq = asset_tools.create_asset(name, DEST, unreal.AnimSequence, factory)
    assert seq and seq.get_skeleton() == arms.skeleton
    tracks = {str(bone): ([], [], []) for bone in bones}
    for frame in range(frames + 1):
        # Explicitly close the loop; the source's final frame may contain drift.
        pose = body_anim.get_anim_pose_at_time(0 if frame == frames else frame / 30, body_options)
        for bone in bones:
            n = str(bone)
            ref = arm_pose.get_ref_bone_pose(bone, LOCAL)
            if n == 'Hips':
                value = root
            elif n in ['LeftShoulder', 'RightShoulder']:
                shoulder = relaxed_pose.get_bone_pose(bone, WORLD)
                position = shoulder.translation - idle_hips + unreal.Vector(0, -23, 0)
                value = unreal.Transform(
                    location=root.inverse_transform_location(position),
                    rotation=(root.rotation.inversed() * shoulder_tilt * shoulder.rotation).rotator(),
                    scale=unreal.Vector(1, 1, 1))
            else:
                # Walking and running must not inherit a can-shaped grip.
                sample = relaxed_pose if 'Hand' in n and n not in ['LeftHand', 'RightHand'] else pose
                q = sample.get_bone_pose(bone, LOCAL).rotation
                if suffix != 'Idle':
                    q = unreal.Quat.slerp_quat(relaxed_pose.get_bone_pose(bone, LOCAL).rotation, q, 0.55 if suffix == 'Walk' else 0.5)
                value = unreal.Transform(location=ref.translation,
                                         rotation=q.rotator(),
                                         scale=unreal.Vector(1, 1, 1))
            pos, rot, scale = tracks[n]
            pos.append(value.translation)
            rot.append(value.rotation)
            scale.append(value.scale3d)
    controller = seq.controller
    controller.open_bracket(unreal.Text('Relaxed empty-hand locomotion'), False)
    controller.set_frame_rate(unreal.FrameRate(30, 1), False)
    controller.set_number_of_frames(unreal.FrameNumber(frames), False)
    controller.remove_all_bone_tracks(False)
    for bone in bones:
        assert controller.add_bone_curve(bone, False)
        assert controller.set_bone_track_keys(bone, *tracks[str(bone)], False)
    controller.close_bracket(False)
    seq.set_preview_skeletal_mesh(arms)
    assert unreal.EditorAssetLibrary.save_loaded_asset(seq, False)
    unreal.log('BR_RELAXED_ARMS clip={} frames={} bones={}'.format(suffix, frames, len(bones)))

assert unreal.BRArmsAssetTools.build_locomotion_blend_space()
unreal.log('BR_RELAXED_ARMS result=PASS')
