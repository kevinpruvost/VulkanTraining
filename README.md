# VulkanTraining

<a href="https://vulkan.lunarg.com/" target="_blank"><img src="https://vulkan.lunarg.com/img/vulkan/vulkan-red.svg"></a>

## Technical Notions

### Queues

Queues are basically containers (std::queue-like) that receive orders and execute them on a FIFO pattern.<br/>
Each of these order differ based on the VkQueue type and working purpose.<br/>

<img src="https://static.packt-cdn.com/products/9781786469809/graphics/image_03_006.jpg">

### Devices

The two main types of devices we'll have to use with Vulkan are Physical and Logical devices.<br/>

#### Physical Device

The purpose of a Physical Device is to manage queues and orders to the displays.

<img src="https://vulkan.lunarg.com/doc/view/1.2.135.0/windows/tutorial/images/Device2QueueFamilies.png">

#### Logical Device

The main purpose of the Logical device is to manage every components of Vulkan and to make them work together.<br/>

<img src ="https://static.wixstatic.com/media/9b0684_f73f0947829a4842ab0106eb7e1fe88f~mv2.png/v1/fill/w_560,h_238,al_c,q_85,usm_0.66_1.00_0.01/9b0684_f73f0947829a4842ab0106eb7e1fe88f~mv2.webp">

### Swapchain

A **Swapchain** is mainly used for its image organization capabilities. It is used to avoid display problems like **tearing** and focuses on synchronization.<br/> It plays a big part on vertical synchronization.

<img src="https://vulkan.lunarg.com/doc/view/1.2.162.0/mac/tutorial/images/Swapchain.png">

### Image Views / Images

Images Views are basically an "interface" for Images to specify how they should be processed to be displayed then.

### Graphics Pipeline

#### 3D Theory

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

* **Render Pass** : Handles the execution and output from each **pipeline** to the **framebuffer**. Can contain multiple **Subpasses**, each can have its own way of rendering the output.
**Subpasses** can be **switched** between them to tell which one should be used by the **render pass**.

### Shaders

For shaders, Vulkan uses a specific format named **SPIR-V**, basically, **.vert** and **.frag** files will have to be compiled<br/>
to make Vulkan able to use the vertices and the fragments of a shader.<br/>
Vulkan gives a binary that has the ability to compile these shader files into **.spv** (binary format), it's called **glslangValidator**.

**SPIR-V** stands for ***Standard Portable Intermediate Representation - Vulkan***.

For this project, I'll use **GLSL** to write all my shaders.

