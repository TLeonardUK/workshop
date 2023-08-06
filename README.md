
![Workshop](./docs/banner.png?raw=true)
![GitHub license](https://img.shields.io/github/license/TLeonardUK/workshop)
![Build Status](https://github.com/TLeonardUK/workshop/actions/workflows/ci.yml/badge.svg)

# What is this project?
This is toy game engine that aims to have a modern, parallel-first design. It's main purpose is to be a sandbox to experiment and learn within.

The renderer uses a modern DX12/Vulkan deferred bindless architecture and will, over time,  support a lot of modern rendering features.

# Current Rendering Features
- Bindless architecture
- Full pbr pipeline
- Clustered deferred pipeline
- Compute-based light culling
- Diffuse light probe volumes
- Specular reflection probes
- Automatic exposure
- Blended weighted order independent transparency
- Standard shadow mapping features
- Standard light features

# Screenshots
![Screenshot 1](./docs/screenshots/1.png?raw=true)
![Screenshot 2](./docs/screenshots/2.png?raw=true)
![Screenshot 3](./docs/screenshots/3.png?raw=true)

# How do I build it?
Currently the project uses visual studio 2022 and C++20 for compilation.

We use cmake for generating project files. You can either use the cmake frontend to generate the project files, or you can use one of the generate_* shell scripts inside the engine/scripts folder.

Once generated the project files are stored in the intermediate folder, at this point you can just open them and build the project in visual studio.

# Whats in the repository?
```
/
├── docs/                     Contains general documentation, notes, etc.
├── engine/                   All engine assets and code.
│   ├── assets/               Contains raw format assets required by the engine, shaders/etc. 
│   ├── source/           
│   │   ├── thirdparty/       Contains all third party libraries the engine depends on.
│   │   ├── workshop.*/       Each individual folder is a seperate engine projects.
│   ├── tools/                Contains the tools used to build the engine, cmake scripts and various batch scripts.
├── game/                     All game assets and code.
│   ├── roguelike/            This is a simple example game using the engine.
```

# Source code structure
The source code is currently split into multiple projects, which are arranged into 4 tiers. The tiers are visible in the visual studio solution explorer or the cmake files (unfortunately the folder structure doesn't distinguish them well).

The purpose of tiers is to provide clear seperation of the different "layers" of the engine, and to control dependencies.

Projects in a tier can depend on other projects in the same tier or in tiers lower than them, but they can never depend on projects above them.

- tier 0 : Contains core / utility code. eg. Logging, IO, math, etc.
- tier 1 : Contains hardware interface code. eg. Render interface, audio interface, asset management, etc.
- tier 2 : Contains core engine code and gameplay frameworks. eg. The main engine class, the frame loop, scene management, etc.
- tier 3 : Contains game-specific code goes. The code for all this is held in the games directory rather than the engine directory.

Third party libraries do not have any of this control and can be used by any tier.
