# VulkanTraining

<a href="https://vulkan.lunarg.com/" target="_blank"><img src="https://vulkan.lunarg.com/img/vulkan/vulkan-red.svg"></a>

# Technical Notions

## Queues

Queues are basically containers (std::queue-like) that receive orders and execute them on a FIFO pattern.<br/>
Each of these order differ based on the VkQueue type and working purpose.<br/>

<img src="https://static.packt-cdn.com/products/9781786469809/graphics/image_03_006.jpg">

## Devices

The two main types of devices we'll have to use with Vulkan are Physical and Logical devices.<br/>

### Physical Device

The purpose of a Physical Device is to manage queues and orders to the displays.

<img src="https://vulkan.lunarg.com/doc/view/1.2.135.0/windows/tutorial/images/Device2QueueFamilies.png">

#### Logical Device

The main purpose of the Logical device is to manage every components of Vulkan and to make them work together.<br/>

<img src ="https://static.wixstatic.com/media/9b0684_f73f0947829a4842ab0106eb7e1fe88f~mv2.png/v1/fill/w_560,h_238,al_c,q_85,usm_0.66_1.00_0.01/9b0684_f73f0947829a4842ab0106eb7e1fe88f~mv2.webp">

## Swapchain

A **Swapchain** is mainly used for its image organization capabilities. It is used to avoid display problems like **tearing** and focuses on synchronization.<br/> It plays a big part on vertical synchronization.

<img src="https://vulkan.lunarg.com/doc/view/1.2.162.0/mac/tutorial/images/Swapchain.png">

## Image Views / Images

Images Views are basically an "interface" for Images to specify how they should be processed to be displayed then.

## Graphics Pipeline

### 3D Theory

<img src="https://github.com/kevinpruvost/VulkanTraining/blob/main/Screenshots/Screenshot%20from%202021-01-17%2000-47-41.png">

* **Input Assembler** : Just assembles vertices inputs into Primitives (Triangles, Lines, ...).

* **Vertex Shader** : Tells how to handle vertices taken in inputs.

* **Tesselation** : It basically splits Triangles into smaller ones to enable some additional GFX features like water movement (waves, ocean, ...).

* **Geometry Shader** : Adds points to the focused geometry, used to create shadows.

* **Rasterization** : Locates where the soon to be drawn pixels should be based on the geometry.

* **Fragment Shader** : A part we code, that will take into account additional processes like lighting.

* **Color Blending** : Takes the alpha color channel primarily and deals with every other geometries that will be drawn to blend colors properly.

* **Framebuffer** : Contains the final image.

#### Needed components for the Graphics Pipeline

* **Vertex Input** : Defines layout and input of vertex data.

* **Input Assembler** : Just assembles vertices inputs into Primitives (Triangles, Lines, ...).

* **Viewport / Scissor** : How to fit output image and crop it.

* **Dynamic States** : Pipelines are static and settings can't be changed at runtime. You need to create new pipeline to get new settings.
However some settings can be given ability to change at runtime, and they can be set there.

* **Rasterizer** : How to handle computation of fragments from primitives.

* **Multisampling** : Multisampling information.

* **Blending** : How to blend fragments at the end of the pipeline.

* **Depth stencil** : How to determine depth + stencil culling and writing.

### Shaders

For shaders, Vulkan uses a specific format named **SPIR-V**, basically, **.vert** and **.frag** files will have to be compiled<br/>
to make Vulkan able to use the vertices and the fragments of a shader.<br/>
Vulkan gives a binary that has the ability to compile these shader files into **.spv** (binary format), it's called **glslangValidator**.

**SPIR-V** stands for ***Standard Portable Intermediate Representation - Vulkan***.

For this project, I'll use **GLSL** to write all my shaders.

### Render Pass

Handles the execution and output from each **pipeline** to the **framebuffer**. Can contain multiple **Subpasses**, each can have its own way of rendering the output.
**Subpasses** can be **switched** between them to tell which one should be used by the **render pass**.

A Render Pass can have multiple attachments referenced by subpasses, they are considered as dependencies for subpasses.

## Framebuffer

Contains **images** processed by the **Render Pass**, 1 by 1 like a queue container.
**Framebuffers** are attached to **pipelines**.

## Command Buffer

Pre-recorded group of commands meant to be sent to the GPU, usually in this order :

1. Start a Render Pass
2. Bind a Pipeline
3. Bind Vertex/Index data
4. Bind Descriptor Sets and Push Constants
5. Draw

We can also begin new subpasses, but binding pipeline will be needed.

Command buffers are also created from a **Command Pool**, unlike most of the Vulkan objects.

Command buffers then submits continously its commands to the appropriate queus for execution.

## Command Pool

It's just a structure that manages every commands allocated dynamically. It's very useful for Vulkan to manage memory in an easier way, so that allocated commands can be all freed from one place.

## Synchronization

As **Presentation** and **Rendering** are working in parallel with Vulkan. Some processes had to be created to manage access to the different resources used by both **Presentation** and **Rendering**.

**Semaphores** were created to handle that. This weird word is basically a flag that will specify if a resource is accessible or not. If a resource is "signalled" (set to true), it's accessible, if it's "unsignalled" (set to false), it is not.

It is the way Vulkan handles multithreading (or asynchronous ?) within its environment.

**But keep hard in mind that Semaphores are used for GPU-side operations.**

For the **CPU side**, there are **Fences** that do the same things but... for the **CPU**. **GPUs** can still "signal" **Fences** if they need to.

Some functions to illustrate this process :

* *vkWaitForFences* : Blocks the CPU code and waits for the GPU to signal the fence.

* *vkResetFences* : Unsignal a fence until the GPU signals it again.

**Fences** are very useful to be sure we do not flood the queues with too many draw/presentation commands.

To be clear : **Fences = CPU/GPU Synchronization**, **Semaphores = GPU/GPU Synchronization**
