# VulkanTraining

<a href="https://vulkan.lunarg.com/" target="_blank"><img src="https://vulkan.lunarg.com/img/vulkan/vulkan-red.svg"></a>

## Technical Notions

### Queues

Queues are basically containers (std::queue-like) that receive orders and execute them on a FIFO order.<br/>
Each of these order differ based on the VkQueue type and working purpose.<br/>

<img src="https://static.packt-cdn.com/products/9781786469809/graphics/image_03_006.jpg">

### Devices

The two main types of devices we'll have to use with Vulkan are Physical and Logical devices.<br/>

#### Physical Device

The purpose of a Physical Device is to manage queues and orders to the displays.

<img src="https://vulkan.lunarg.com/doc/view/1.2.135.0/windows/tutorial/images/Device2QueueFamilies.png">

#### Logical Device

The main purpose of the logical device is to manage every components of Vulkan and to make them work together.<br/>

<img src ="https://static.wixstatic.com/media/9b0684_f73f0947829a4842ab0106eb7e1fe88f~mv2.png/v1/fill/w_560,h_238,al_c,q_85,usm_0.66_1.00_0.01/9b0684_f73f0947829a4842ab0106eb7e1fe88f~mv2.webp">

### Swapchain

A Swapchain is mainly used for its image organization capabilities. It is used to avoid display problems like 'tearing' and focuses on synchronization.<br/> It plays a big part on vertical synchronization.

<img src="https://vulkan.lunarg.com/doc/view/1.2.162.0/mac/tutorial/images/Swapchain.png">

### Image Views / Images

Images Views are basically an "interface" for Images to specify how they should be processed to be displayed then.
