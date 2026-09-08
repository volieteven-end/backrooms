"""Replace the four imported decorative keys with empty interactive sockets.

Run with UnrealEditor-Cmd -run=pythonscript -script=<this file> after building
backroomsEditor. The existing map is edited in place; re-running is idempotent.
"""
import unreal

LEVEL = '/Game/ReverseAsset/ParkingGarage/LightingStudy/L_MiddleFloor_Dark'
PREFIX = LEVEL + '.L_MiddleFloor_Dark:PersistentLevel.'
editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert editor.load_level(LEVEL)
all_actors = {a.get_path_name(): a for a in actors.get_all_level_actors()}
by_label = {a.get_actor_label(): a for a in all_actors.values() if a.get_path_name().startswith(PREFIX)}


def existing(name):
    return all_actors.get(PREFIX + name)


manager = existing('BRGarageKeyManager_0')
zone = existing('BRExtractionZone_0')
leaf = existing('StaticMeshActor_22')
assert manager and zone and leaf
assert leaf.static_mesh_component.static_mesh.get_name() == 'SM_Door_01a'
sockets = []
for index in range(4):
    label = 'BR_KeySocket_{}'.format(index + 1)
    socket = by_label.get(label)
    if not socket:
        source = [existing('StaticMeshActor_{}'.format(25 + index * 3 + part)) for part in range(3)]
        assert all(source), label
        assert 'Keyhole' in source[0].static_mesh_component.static_mesh.get_name()
        assert 'Plate' in source[1].static_mesh_component.static_mesh.get_name()
        assert source[2].static_mesh_component.static_mesh.get_name() == 'Key'
        socket = actors.spawn_actor_from_class(unreal.BRGarageKeySocket, source[1].get_actor_location(), source[1].get_actor_rotation())
        socket.set_actor_label(label)
        for name, original in zip(('keyhole', 'plate', 'inserted_key'), source):
            component = socket.get_editor_property(name)
            source_component = original.static_mesh_component
            component.set_static_mesh(source_component.static_mesh)
            component.set_world_transform(source_component.get_world_transform(), False, True)
            for material_index in range(source_component.get_num_materials()):
                component.set_material(material_index, source_component.get_material(material_index))
        for original in source:
            assert actors.destroy_actor(original)
        stub = existing('Actor_{}'.format(19 + index))
        if stub:
            assert not stub.get_components_by_class(unreal.StaticMeshComponent)
            assert actors.destroy_actor(stub)
    socket.set_editor_property('key_manager', manager)
    socket.set_editor_property('socket_number', index + 1)
    socket.get_editor_property('inserted_key').set_visibility(False)
    socket.get_editor_property('inserted_key').set_hidden_in_game(True)
    sockets.append(socket)

door = by_label.get('BR_CentralExitDoor')
if not door:
    door = actors.spawn_actor_from_class(unreal.BRGarageExitDoor, leaf.get_actor_location(), leaf.get_actor_rotation())
    door.set_actor_label('BR_CentralExitDoor')
    stub = existing('Actor_0')
    if stub:
        assert stub.get_actor_label() == 'ETB_BP_HeavyDoor_C_BP_HeavyDoor'
        assert actors.destroy_actor(stub)
door.set_editor_property('key_manager', manager)
door.set_editor_property('controlled_actor_tag', 'BR_CentralExitLeaf')
door.set_editor_property('open_angle', -100.0)
door.set_editor_property('open_duration', 1.3)
tags = list(leaf.tags)
if 'BR_CentralExitLeaf' not in [str(t) for t in tags]:
    tags.append('BR_CentralExitLeaf')
leaf.tags = tags
leaf.static_mesh_component.set_mobility(unreal.ComponentMobility.MOVABLE)
manager.set_editor_property('key_sockets', sockets)
manager.set_editor_property('exit_door', door)
zone.set_actor_location(unreal.Vector(1318, 7720, 3210), False, True)
zone.set_actor_scale3d(unreal.Vector(1, 1, 1))
zone.get_component_by_class(unreal.BoxComponent).set_box_extent(unreal.Vector(120, 90, 120))
zone.set_editor_property('required_exit_door', door)
zone.set_editor_property('require_all_players', False)
assert all(not s.get_editor_property('inserted_key').is_visible() for s in sockets)
assert editor.save_current_level()
unreal.log('BR_KEY_SETUP result=PASS empty_sockets=4 central_exit=1')
