// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "roguelike/app.h"

#include "workshop.core/filesystem/file.h"
#include "workshop.core/utils/frame_time.h"
#include "workshop.core/math/sphere.h"
#include "workshop.core/containers/sparse_vector.h"

#include "workshop.engine/engine/engine.h"
#include "workshop.engine/engine/world.h"
#include "workshop.engine/ecs/component_filter.h"

#include "workshop.renderer/renderer.h"
#include "workshop.renderer/render_object.h"
#include "workshop.renderer/objects/render_view.h"
#include "workshop.renderer/objects/render_static_mesh.h"
#include "workshop.renderer/assets/model/model.h"

#include "workshop.renderer/render_imgui_manager.h"

#include "workshop.game_framework/systems/default_systems.h"
#include "workshop.game_framework/systems/camera/fly_camera_movement_system.h"
#include "workshop.game_framework/systems/transform/transform_system.h"
#include "workshop.game_framework/systems/lighting/directional_light_system.h"
#include "workshop.game_framework/systems/lighting/light_probe_grid_system.h"
#include "workshop.game_framework/systems/lighting/point_light_system.h"
#include "workshop.game_framework/systems/lighting/reflection_probe_system.h"
#include "workshop.game_framework/systems/lighting/spot_light_system.h"
#include "workshop.game_framework/systems/geometry/static_mesh_system.h"

#include "workshop.game_framework/components/camera/camera_component.h"
#include "workshop.game_framework/components/camera/fly_camera_movement_component.h"
#include "workshop.game_framework/components/transform/transform_component.h"
#include "workshop.game_framework/components/transform/bounds_component.h"
#include "workshop.game_framework/components/lighting/directional_light_component.h"
#include "workshop.game_framework/components/lighting/point_light_component.h"
#include "workshop.game_framework/components/lighting/spot_light_component.h"
#include "workshop.game_framework/components/lighting/light_probe_grid_component.h"
#include "workshop.game_framework/components/lighting/reflection_probe_component.h"
#include "workshop.game_framework/components/geometry/static_mesh_component.h"

#include <chrono>

// TODO:
//      render_options to configure pipeline
//      Anti-Alising
//      Try and reduce light leakage for probes
//      Decals
//      Skinned Meshes

std::shared_ptr<ws::app> make_app()
{
    return std::make_shared<ws::rl_game_app>();
}

namespace ws {

std::string rl_game_app::get_name()
{
    return "roguelike";
}

void rl_game_app::configure_engine(engine& engine)
{
    // Register default interface configuration.
    engine.set_render_interface_type(ri_interface_type::dx12);
    engine.set_window_interface_type(window_interface_type::sdl);
    engine.set_input_interface_type(input_interface_type::sdl);
    engine.set_platform_interface_type(platform_interface_type::sdl);
    engine.set_window_mode(get_name(), 1920, 1080, ws::window_mode::windowed);
}

ws::result<void> rl_game_app::start()
{
    auto& ass_manager = get_engine().get_asset_manager();
    auto& obj_manager = get_engine().get_default_world().get_object_manager();
    register_default_systems(obj_manager);

    transform_system* transform_sys = obj_manager.get_system<transform_system>();
    directional_light_system* direction_light_sys = obj_manager.get_system<directional_light_system>();
    light_probe_grid_system* light_probe_grid_sys = obj_manager.get_system<light_probe_grid_system>();
    static_mesh_system* static_mesh_sys = obj_manager.get_system<static_mesh_system>();

    // Add the main camera!
    m_camera_object = obj_manager.create_object("main camera");
    obj_manager.add_component<transform_component>(m_camera_object);
    obj_manager.add_component<bounds_component>(m_camera_object);
    obj_manager.add_component<camera_component>(m_camera_object);
    obj_manager.add_component<fly_camera_movement_component>(m_camera_object);
    transform_sys->set_local_transform(m_camera_object, vector3(0.0f, 100.0f, -250.0f), quat::identity, vector3::one);

    // Add a directional light for the scene.
    object sun_object = obj_manager.create_object("sun light");
    obj_manager.add_component<transform_component>(sun_object);
    obj_manager.add_component<bounds_component>(sun_object);
    obj_manager.add_component<directional_light_component>(sun_object);
    direction_light_sys->set_light_shadow_casting(sun_object, true);
    direction_light_sys->set_light_shadow_map_size(sun_object, 2048);
    direction_light_sys->set_light_shadow_max_distance(sun_object, 10000.0f);
    direction_light_sys->set_light_shadow_cascade_exponent(sun_object, 0.6f);
    direction_light_sys->set_light_intensity(sun_object, 5.0f);
    transform_sys->set_local_transform(sun_object, vector3(0.0f, 300.0f, 0.0f), quat::angle_axis(-math::halfpi * 0.85f, vector3::right) * quat::angle_axis(0.5f, vector3::forward), vector3::one);

    // Add a skybox.
    object mesh_object = obj_manager.create_object("skybox");
    obj_manager.add_component<transform_component>(mesh_object);
    obj_manager.add_component<bounds_component>(mesh_object);
    obj_manager.add_component<static_mesh_component>(mesh_object);
    transform_sys->set_local_transform(mesh_object, vector3(0.0f, 0.0f, 0.0f), quat::identity, vector3(10000.0f, 10000.0f, 10000.0f));
    static_mesh_sys->set_model(mesh_object, ass_manager.request_asset<model>("data:models/skyboxs/skybox_3.yaml", 0));

#if 1
    // Create the sponza scene

    // Add light probe grid.
    object light_probe_object = obj_manager.create_object("light probe grid");
    obj_manager.add_component<transform_component>(light_probe_object);
    obj_manager.add_component<bounds_component>(light_probe_object);
    obj_manager.add_component<light_probe_grid_component>(light_probe_object);
    light_probe_grid_sys->set_grid_density(light_probe_object, 350.0f);
    transform_sys->set_local_transform(light_probe_object, vector3(200.0, 1050.0f, -100.0f), quat::identity, vector3(3900.f, 2200.0f, 2200.0f));

    // Add reflection probe.
    object reflection_probe_object = obj_manager.create_object("reflection probe");
    obj_manager.add_component<transform_component>(reflection_probe_object);
    obj_manager.add_component<bounds_component>(reflection_probe_object);
    obj_manager.add_component<reflection_probe_component>(reflection_probe_object);
    transform_sys->set_local_transform(reflection_probe_object, vector3(0.0, 200.0f, 0.0f), quat::identity, vector3(4000.f, 4000.0f, 4000.0f));

    // Add meshes
    mesh_object = obj_manager.create_object("sponza");
    obj_manager.add_component<transform_component>(mesh_object);
    obj_manager.add_component<bounds_component>(mesh_object);
    obj_manager.add_component<static_mesh_component>(mesh_object);
    static_mesh_sys->set_model(mesh_object, ass_manager.request_asset<model>("data:models/test_scenes/sponza/sponza.yaml", 0));

    mesh_object = obj_manager.create_object("sponza curtains");
    obj_manager.add_component<transform_component>(mesh_object);
    obj_manager.add_component<bounds_component>(mesh_object);
    obj_manager.add_component<static_mesh_component>(mesh_object);
    static_mesh_sys->set_model(mesh_object, ass_manager.request_asset<model>("data:models/test_scenes/sponza_curtains/sponza_curtains.yaml", 0));

    //mesh_object = obj_manager.create_object();
    //obj_manager.add_component<transform_component>(mesh_object);
    //obj_manager.add_component<bounds_component>(mesh_object);
    //obj_manager.add_component<static_mesh_component>(mesh_object);
    //static_mesh_sys->set_model(mesh_object, ass_manager.request_asset<model>("data:models/test_scenes/sponza_ivy/sponza_ivy.yaml", 0));

    //mesh_object = obj_manager.create_object();
    //obj_manager.add_component<transform_component>(mesh_object);
    //obj_manager.add_component<bounds_component>(mesh_object);
    //obj_manager.add_component<static_mesh_component>(mesh_object);
    //static_mesh_sys->set_model(mesh_object, ass_manager.request_asset<model>("data:models/test_scenes/sponza_trees/sponza_trees.yaml", 0));

    mesh_object = obj_manager.create_object("cerberus");
    obj_manager.add_component<transform_component>(mesh_object);
    obj_manager.add_component<bounds_component>(mesh_object);
    obj_manager.add_component<static_mesh_component>(mesh_object);
    static_mesh_sys->set_model(mesh_object, ass_manager.request_asset<model>("data:models/test_scenes/cerberus/cerberus.yaml", 0));

    // Testing for the editor.
    object test = obj_manager.create_object("root-test");


    object parent = obj_manager.create_object("parent");
    obj_manager.add_component<transform_component>(parent);

        object child1 = obj_manager.create_object("child 1");
        obj_manager.add_component<transform_component>(child1);
        obj_manager.add_component<bounds_component>(child1);
        transform_sys->set_parent(child1, parent);

        object child2 = obj_manager.create_object("child 2");
        obj_manager.add_component<transform_component>(child2);
        obj_manager.add_component<bounds_component>(child2);
        transform_sys->set_parent(child2, child1);
        

#else
    // Create the scene

    // Add light probe grid.
    object light_probe_object = obj_manager.create_object("light probe grid");
    obj_manager.add_component<transform_component>(light_probe_object);
    obj_manager.add_component<light_probe_grid_component>(light_probe_object);
    light_probe_grid_sys->set_grid_density(light_probe_object, 350.0f);
    transform_sys->set_local_transform(light_probe_object, vector3(3000.0f, 2000.0f, 0.0f), quat::identity, vector3(20000.f, 4000.0f, 20000.0f));

    // Add reflection probe.
    object reflection_probe_object = obj_manager.create_object("reflection probe");
    obj_manager.add_component<transform_component>(reflection_probe_object);
    obj_manager.add_component<reflection_probe_component>(reflection_probe_object);
    transform_sys->set_local_transform(reflection_probe_object, vector3(-900.0f, 400.0f, 0.0f), quat::identity, vector3(10000.f, 10000.0f, 10000.0f));

    // Add meshes
    mesh_object = obj_manager.create_object("bistro exterior");
    obj_manager.add_component<transform_component>(mesh_object);
    obj_manager.add_component<static_mesh_component>(mesh_object);
    static_mesh_sys->set_model(mesh_object, ass_manager.request_asset<model>("data:models/test_scenes/bistro/bistro_exterior.yaml", 0));

    mesh_object = obj_manager.create_object("bistro interior");
    obj_manager.add_component<transform_component>(mesh_object);
    obj_manager.add_component<static_mesh_component>(mesh_object);
    static_mesh_sys->set_model(mesh_object, ass_manager.request_asset<model>("data:models/test_scenes/bistro/bistro_interior.yaml", 0));

#endif

    m_on_step_delegate = get_engine().on_step.add_shared([this](const frame_time& time) {
        step(time);
    });

    return true;
}

ws::result<void> rl_game_app::stop()
{
    return true;
}

void rl_game_app::step(const frame_time& time)
{
    render_command_queue& cmd_queue = get_engine().get_renderer().get_command_queue();

    window& main_window = get_engine().get_main_window();
    input_interface& input = get_engine().get_input_interface();
    
}

}; // namespace hg