/*
 * Minimal Vulkan smoke test for CI.
 *
 * Goals:
 * - Verify SDL2 Vulkan headers are available
 * - Verify Vulkan headers are available
 * - Verify we can create a Vulkan instance and an SDL2 VkSurfaceKHR
 * - Verify we can find a physical device + queue family that supports present
 *
 * This is intentionally not a full renderer and does not depend on EGL code.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <SDL.h>
#include <SDL_vulkan.h>

#include <vulkan/vulkan.h>

#ifndef ARRAY_LEN
#define ARRAY_LEN(x) ((int)(sizeof(x) / sizeof((x)[0])))
#endif

/*
 * Embedded SPIR-V shaders (generated locally via glslangValidator -V).
 * Vertex shader:
 *   layout(location=0) in vec2 inPos;
 *   gl_Position = vec4(inPos, 0.0, 1.0);
 * Fragment shader:
 *   outColor = vec4(1.0, 0.0, 1.0, 1.0);
 */
static const uint32_t kTriVertSpv[] = {
	0x07230203, 0x00010000, 0x0008000B, 0x0000001B, 0x00000000, 0x00020011, 0x00000001, 0x0006000B, 0x00000001, 0x4C534C47, 0x6474732E, 0x3035342E, 0x00000000, 0x0003000E, 0x00000000, 0x00000001, 0x0007000F, 0x00000000, 0x00000004, 0x6E69616D, 0x00000000, 0x0000000D, 0x00000012, 0x00030003, 0x00000002, 0x000001C2, 0x00040005, 0x00000004, 0x6E69616D, 0x00000000, 0x00060005, 0x0000000B, 0x505F6C67, 0x65567265, 0x78657472, 0x00000000, 0x00060006, 0x0000000B, 0x00000000, 0x505F6C67, 0x7469736F, 0x006E6F69, 0x00070006, 0x0000000B, 0x00000001, 0x505F6C67, 0x746E696F, 0x657A6953, 0x00000000, 0x00070006, 0x0000000B, 0x00000002, 0x435F6C67, 0x4470696C, 0x61747369, 0x0065636E, 0x00070006, 0x0000000B, 0x00000003, 0x435F6C67, 0x446C6C75, 0x61747369, 0x0065636E, 0x00030005, 0x0000000D, 0x00000000, 0x00040005, 0x00000012, 0x6F506E69, 0x00000073, 0x00030047, 0x0000000B, 0x00000002, 0x00050048, 0x0000000B, 0x00000000, 0x0000000B, 0x00000000, 0x00050048, 0x0000000B, 0x00000001, 0x0000000B, 0x00000001, 0x00050048, 0x0000000B, 0x00000002, 0x0000000B, 0x00000003, 0x00050048, 0x0000000B, 0x00000003, 0x0000000B, 0x00000004, 0x00040047, 0x00000012, 0x0000001E, 0x00000000, 0x00020013, 0x00000002, 0x00030021, 0x00000003, 0x00000002, 0x00030016, 0x00000006, 0x00000020, 0x00040017, 0x00000007, 0x00000006, 0x00000004, 0x00040015, 0x00000008, 0x00000020, 0x00000000, 0x0004002B, 0x00000008, 0x00000009, 0x00000001, 0x0004001C, 0x0000000A, 0x00000006, 0x00000009, 0x0006001E, 0x0000000B, 0x00000007, 0x00000006, 0x0000000A, 0x0000000A, 0x00040020, 0x0000000C, 0x00000003, 0x0000000B, 0x0004003B, 0x0000000C, 0x0000000D, 0x00000003, 0x00040015, 0x0000000E, 0x00000020, 0x00000001, 0x0004002B, 0x0000000E, 0x0000000F, 0x00000000, 0x00040017, 0x00000010, 0x00000006, 0x00000002, 0x00040020, 0x00000011, 0x00000001, 0x00000010, 0x0004003B, 0x00000011, 0x00000012, 0x00000001, 0x0004002B, 0x00000006, 0x00000014, 0x00000000, 0x0004002B, 0x00000006, 0x00000015, 0x3F800000, 0x00040020, 0x00000019, 0x00000003, 0x00000007, 0x00050036, 0x00000002, 0x00000004, 0x00000000, 0x00000003, 0x000200F8, 0x00000005, 0x0004003D, 0x00000010, 0x00000013, 0x00000012, 0x00050051, 0x00000006, 0x00000016, 0x00000013, 0x00000000, 0x00050051, 0x00000006, 0x00000017, 0x00000013, 0x00000001, 0x00070050, 0x00000007, 0x00000018, 0x00000016, 0x00000017, 0x00000014, 0x00000015, 0x00050041, 0x00000019, 0x0000001A, 0x0000000D, 0x0000000F, 0x0003003E, 0x0000001A, 0x00000018, 0x000100FD, 0x00010038
};

static const uint32_t kTriFragSpv[] = {
	0x07230203, 0x00010000, 0x0008000B, 0x0000000D, 0x00000000, 0x00020011, 0x00000001, 0x0006000B, 0x00000001, 0x4C534C47, 0x6474732E, 0x3035342E, 0x00000000, 0x0003000E, 0x00000000, 0x00000001, 0x0006000F, 0x00000004, 0x00000004, 0x6E69616D, 0x00000000, 0x00000009, 0x00030010, 0x00000004, 0x00000007, 0x00030003, 0x00000002, 0x000001C2, 0x00040005, 0x00000004, 0x6E69616D, 0x00000000, 0x00050005, 0x00000009, 0x4374756F, 0x726F6C6F, 0x00000000, 0x00040047, 0x00000009, 0x0000001E, 0x00000000, 0x00020013, 0x00000002, 0x00030021, 0x00000003, 0x00000002, 0x00030016, 0x00000006, 0x00000020, 0x00040017, 0x00000007, 0x00000006, 0x00000004, 0x00040020, 0x00000008, 0x00000003, 0x00000007, 0x0004003B, 0x00000008, 0x00000009, 0x00000003, 0x0004002B, 0x00000006, 0x0000000A, 0x3F800000, 0x0004002B, 0x00000006, 0x0000000B, 0x00000000, 0x0007002C, 0x00000007, 0x0000000C, 0x0000000A, 0x0000000B, 0x0000000A, 0x0000000A, 0x00050036, 0x00000002, 0x00000004, 0x00000000, 0x00000003, 0x000200F8, 0x00000005, 0x0003003E, 0x00000009, 0x0000000C, 0x000100FD, 0x00010038
};

static const char *vk_result_to_string(VkResult result)
{
	/* Keep this tiny; we only stringify common init failures. */
	switch (result) {
		case VK_SUCCESS: return "VK_SUCCESS";
		case VK_NOT_READY: return "VK_NOT_READY";
		case VK_TIMEOUT: return "VK_TIMEOUT";
		case VK_EVENT_SET: return "VK_EVENT_SET";
		case VK_EVENT_RESET: return "VK_EVENT_RESET";
		case VK_INCOMPLETE: return "VK_INCOMPLETE";
		case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
		case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
		case VK_ERROR_INITIALIZATION_FAILED: return "VK_ERROR_INITIALIZATION_FAILED";
		case VK_ERROR_DEVICE_LOST: return "VK_ERROR_DEVICE_LOST";
		case VK_ERROR_MEMORY_MAP_FAILED: return "VK_ERROR_MEMORY_MAP_FAILED";
		case VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
		case VK_ERROR_EXTENSION_NOT_PRESENT: return "VK_ERROR_EXTENSION_NOT_PRESENT";
		case VK_ERROR_FEATURE_NOT_PRESENT: return "VK_ERROR_FEATURE_NOT_PRESENT";
		case VK_ERROR_INCOMPATIBLE_DRIVER: return "VK_ERROR_INCOMPATIBLE_DRIVER";
		case VK_ERROR_TOO_MANY_OBJECTS: return "VK_ERROR_TOO_MANY_OBJECTS";
		case VK_ERROR_FORMAT_NOT_SUPPORTED: return "VK_ERROR_FORMAT_NOT_SUPPORTED";
		case VK_ERROR_SURFACE_LOST_KHR: return "VK_ERROR_SURFACE_LOST_KHR";
		case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
		default: return "VK_ERROR_<unknown>";
	}
}

static void die_sdl(const char *msg)
{
	fprintf(stderr, "ERROR: %s: %s\n", msg, SDL_GetError());
	exit(1);
}

static void die_vk(const char *msg, VkResult result)
{
	fprintf(stderr, "ERROR: %s: %s (%d)\n", msg, vk_result_to_string(result), (int)result);
	exit(1);
}

static uint32_t find_memory_type(VkPhysicalDevice phys, uint32_t type_bits, VkMemoryPropertyFlags props)
{
	VkPhysicalDeviceMemoryProperties mem_props;
	uint32_t i;

	vkGetPhysicalDeviceMemoryProperties(phys, &mem_props);
	for (i = 0; i < mem_props.memoryTypeCount; i++) {
		if ((type_bits & (1u << i)) == 0)
			continue;
		if ((mem_props.memoryTypes[i].propertyFlags & props) == props)
			return i;
	}

	fprintf(stderr, "ERROR: no suitable Vulkan memory type found\n");
	exit(1);
}

static VkSurfaceFormatKHR choose_surface_format(const VkSurfaceFormatKHR *formats, uint32_t count)
{
	uint32_t i;

	/* Prefer sRGB if offered, otherwise just take the first. */
	for (i = 0; i < count; i++) {
		if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB)
			return formats[i];
		if (formats[i].format == VK_FORMAT_R8G8B8A8_SRGB)
			return formats[i];
	}
	return formats[0];
}

static VkPresentModeKHR choose_present_mode(const VkPresentModeKHR *modes, uint32_t count)
{
	uint32_t i;

	for (i = 0; i < count; i++) {
		if (modes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
			return modes[i];
	}
	for (i = 0; i < count; i++) {
		if (modes[i] == VK_PRESENT_MODE_FIFO_KHR)
			return modes[i];
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}

static VkCompositeAlphaFlagBitsKHR choose_composite_alpha(VkCompositeAlphaFlagsKHR supported)
{
	if (supported & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
		return VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	if (supported & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR)
		return VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
	if (supported & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR)
		return VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;
	if (supported & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR)
		return VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
	return VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
}

int main(int argc, char **argv)
{
	SDL_Window *window;
	int win_w;
	int win_h;
	unsigned int sdl_ext_count;
	const char **sdl_exts;
	VkApplicationInfo app_info;
	VkInstanceCreateInfo instance_info;
	VkInstance instance;
	VkSurfaceKHR surface;
	VkResult vr;
	uint32_t phys_count;
	VkPhysicalDevice *phys_devs;
	VkPhysicalDevice chosen_phys;
	uint32_t chosen_queue_family;
	VkDevice device;
	VkQueue queue;
	VkDeviceQueueCreateInfo q_info;
	VkDeviceCreateInfo dev_info;
	const char *dev_exts[1];
	float queue_priority;
	VkBool32 present_supported;
	uint32_t qf_count;
	VkQueueFamilyProperties *qf_props;
	uint32_t i;
	uint32_t qf;

	VkSurfaceCapabilitiesKHR surf_caps;
	uint32_t format_count;
	VkSurfaceFormatKHR *formats;
	VkSurfaceFormatKHR chosen_format;
	uint32_t pm_count;
	VkPresentModeKHR *present_modes;
	VkPresentModeKHR chosen_present_mode;
	VkExtent2D extent;
	uint32_t desired_image_count;
	VkSwapchainKHR swapchain;
	VkSwapchainCreateInfoKHR sc_info;
	uint32_t swap_image_count;
	VkImage *swap_images;
	VkImageView *swap_image_views;
	VkRenderPass render_pass;
	VkPipelineLayout pipeline_layout;
	VkPipeline pipeline;
	VkShaderModule vert_module;
	VkShaderModule frag_module;
	VkFramebuffer *framebuffers;
	VkCommandPool cmd_pool;
	VkCommandBuffer *cmd_bufs;
	VkBuffer vbuf;
	VkDeviceMemory vmem;
	VkSemaphore sem_image_available;
	VkSemaphore sem_render_finished;
	VkFence fence_in_flight;
	uint32_t image_index;

	(void)argc;
	(void)argv;

	if (SDL_Init(SDL_INIT_VIDEO) != 0)
		die_sdl("SDL_Init(SDL_INIT_VIDEO) failed");

	window = SDL_CreateWindow(
		"egl vulkan smoke",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		320,
		240,
		(SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN)
	);
	if (!window)
		die_sdl("SDL_CreateWindow(SDL_WINDOW_VULKAN) failed");

	sdl_ext_count = 0;
	if (!SDL_Vulkan_GetInstanceExtensions(window, &sdl_ext_count, NULL))
		die_sdl("SDL_Vulkan_GetInstanceExtensions(count) failed");
	if (sdl_ext_count == 0) {
		fprintf(stderr, "ERROR: SDL_Vulkan_GetInstanceExtensions returned 0 extensions\n");
		exit(1);
	}

	sdl_exts = (const char **)calloc((size_t)sdl_ext_count, sizeof(*sdl_exts));
	if (!sdl_exts) {
		fprintf(stderr, "ERROR: out of memory allocating SDL Vulkan extension list\n");
		exit(1);
	}
	if (!SDL_Vulkan_GetInstanceExtensions(window, &sdl_ext_count, sdl_exts))
		die_sdl("SDL_Vulkan_GetInstanceExtensions(list) failed");

	memset(&app_info, 0, sizeof(app_info));
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName = "egl-vulkan-smoke";
	app_info.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
	app_info.pEngineName = "egl";
	app_info.engineVersion = VK_MAKE_VERSION(0, 0, 1);
	app_info.apiVersion = VK_API_VERSION_1_0;

	memset(&instance_info, 0, sizeof(instance_info));
	instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_info.pApplicationInfo = &app_info;
	instance_info.enabledExtensionCount = (uint32_t)sdl_ext_count;
	instance_info.ppEnabledExtensionNames = sdl_exts;
	instance_info.enabledLayerCount = 0;
	instance_info.ppEnabledLayerNames = NULL;

	instance = VK_NULL_HANDLE;
	vr = vkCreateInstance(&instance_info, NULL, &instance);
	if (vr != VK_SUCCESS)
		die_vk("vkCreateInstance failed", vr);

	surface = VK_NULL_HANDLE;
	if (!SDL_Vulkan_CreateSurface(window, instance, &surface))
		die_sdl("SDL_Vulkan_CreateSurface failed");

	SDL_GetWindowSize(window, &win_w, &win_h);

	phys_count = 0;
	vr = vkEnumeratePhysicalDevices(instance, &phys_count, NULL);
	if (vr != VK_SUCCESS)
		die_vk("vkEnumeratePhysicalDevices(count) failed", vr);
	if (phys_count == 0) {
		fprintf(stderr, "ERROR: no Vulkan physical devices found\n");
		exit(1);
	}

	phys_devs = (VkPhysicalDevice *)calloc((size_t)phys_count, sizeof(*phys_devs));
	if (!phys_devs) {
		fprintf(stderr, "ERROR: out of memory allocating physical device list\n");
		exit(1);
	}
	vr = vkEnumeratePhysicalDevices(instance, &phys_count, phys_devs);
	if (vr != VK_SUCCESS)
		die_vk("vkEnumeratePhysicalDevices(list) failed", vr);

	chosen_phys = VK_NULL_HANDLE;
	chosen_queue_family = UINT32_MAX;

	for (i = 0; i < phys_count && chosen_phys == VK_NULL_HANDLE; i++) {
		VkPhysicalDevice phys;

		phys = phys_devs[i];
		qf_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(phys, &qf_count, NULL);
		if (qf_count == 0)
			continue;

		qf_props = (VkQueueFamilyProperties *)calloc((size_t)qf_count, sizeof(*qf_props));
		if (!qf_props) {
			fprintf(stderr, "ERROR: out of memory allocating queue family props\n");
			exit(1);
		}
		vkGetPhysicalDeviceQueueFamilyProperties(phys, &qf_count, qf_props);

		for (qf = 0; qf < qf_count; qf++) {
			if ((qf_props[qf].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0)
				continue;

			present_supported = VK_FALSE;
			vr = vkGetPhysicalDeviceSurfaceSupportKHR(phys, qf, surface, &present_supported);
			if (vr == VK_SUCCESS && present_supported == VK_TRUE) {
				chosen_phys = phys;
				chosen_queue_family = qf;
				break;
			}
		}

		free(qf_props);
		qf_props = NULL;
	}

	free(phys_devs);
	phys_devs = NULL;

	if (chosen_phys == VK_NULL_HANDLE || chosen_queue_family == UINT32_MAX) {
		fprintf(stderr, "ERROR: no Vulkan device/queue family supports graphics+present\n");
		exit(1);
	}

	queue_priority = 1.0f;
	memset(&q_info, 0, sizeof(q_info));
	q_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	q_info.queueFamilyIndex = chosen_queue_family;
	q_info.queueCount = 1;
	q_info.pQueuePriorities = &queue_priority;

	dev_exts[0] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;

	memset(&dev_info, 0, sizeof(dev_info));
	dev_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	dev_info.queueCreateInfoCount = 1;
	dev_info.pQueueCreateInfos = &q_info;
	dev_info.enabledExtensionCount = 1;
	dev_info.ppEnabledExtensionNames = dev_exts;
	dev_info.enabledLayerCount = 0;
	dev_info.ppEnabledLayerNames = NULL;
	dev_info.pEnabledFeatures = NULL;

	device = VK_NULL_HANDLE;
	vr = vkCreateDevice(chosen_phys, &dev_info, NULL, &device);
	if (vr != VK_SUCCESS)
		die_vk("vkCreateDevice failed", vr);

	queue = VK_NULL_HANDLE;
	vkGetDeviceQueue(device, chosen_queue_family, 0, &queue);
	if (queue == VK_NULL_HANDLE) {
		fprintf(stderr, "ERROR: vkGetDeviceQueue returned NULL queue\n");
		exit(1);
	}

	printf("Vulkan init OK: instance+surface+device created (queue family %u)\n", (unsigned)chosen_queue_family);

	/* ---- Swapchain ---- */
	vr = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(chosen_phys, surface, &surf_caps);
	if (vr != VK_SUCCESS)
		die_vk("vkGetPhysicalDeviceSurfaceCapabilitiesKHR failed", vr);

	format_count = 0;
	vr = vkGetPhysicalDeviceSurfaceFormatsKHR(chosen_phys, surface, &format_count, NULL);
	if (vr != VK_SUCCESS)
		die_vk("vkGetPhysicalDeviceSurfaceFormatsKHR(count) failed", vr);
	if (format_count == 0) {
		fprintf(stderr, "ERROR: no Vulkan surface formats available\n");
		exit(1);
	}
	formats = (VkSurfaceFormatKHR *)calloc((size_t)format_count, sizeof(*formats));
	if (!formats) {
		fprintf(stderr, "ERROR: out of memory allocating surface formats\n");
		exit(1);
	}
	vr = vkGetPhysicalDeviceSurfaceFormatsKHR(chosen_phys, surface, &format_count, formats);
	if (vr != VK_SUCCESS)
		die_vk("vkGetPhysicalDeviceSurfaceFormatsKHR(list) failed", vr);
	chosen_format = choose_surface_format(formats, format_count);
	free(formats);
	formats = NULL;

	pm_count = 0;
	vr = vkGetPhysicalDeviceSurfacePresentModesKHR(chosen_phys, surface, &pm_count, NULL);
	if (vr != VK_SUCCESS)
		die_vk("vkGetPhysicalDeviceSurfacePresentModesKHR(count) failed", vr);
	if (pm_count == 0) {
		fprintf(stderr, "ERROR: no Vulkan present modes available\n");
		exit(1);
	}
	present_modes = (VkPresentModeKHR *)calloc((size_t)pm_count, sizeof(*present_modes));
	if (!present_modes) {
		fprintf(stderr, "ERROR: out of memory allocating present modes\n");
		exit(1);
	}
	vr = vkGetPhysicalDeviceSurfacePresentModesKHR(chosen_phys, surface, &pm_count, present_modes);
	if (vr != VK_SUCCESS)
		die_vk("vkGetPhysicalDeviceSurfacePresentModesKHR(list) failed", vr);
	chosen_present_mode = choose_present_mode(present_modes, pm_count);
	free(present_modes);
	present_modes = NULL;

	if (surf_caps.currentExtent.width != UINT32_MAX) {
		extent = surf_caps.currentExtent;
	} else {
		extent.width = (uint32_t)win_w;
		extent.height = (uint32_t)win_h;
		if (extent.width < surf_caps.minImageExtent.width)
			extent.width = surf_caps.minImageExtent.width;
		if (extent.width > surf_caps.maxImageExtent.width)
			extent.width = surf_caps.maxImageExtent.width;
		if (extent.height < surf_caps.minImageExtent.height)
			extent.height = surf_caps.minImageExtent.height;
		if (extent.height > surf_caps.maxImageExtent.height)
			extent.height = surf_caps.maxImageExtent.height;
	}

	desired_image_count = surf_caps.minImageCount + 1;
	if (surf_caps.maxImageCount > 0 && desired_image_count > surf_caps.maxImageCount)
		desired_image_count = surf_caps.maxImageCount;

	memset(&sc_info, 0, sizeof(sc_info));
	sc_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	sc_info.surface = surface;
	sc_info.minImageCount = desired_image_count;
	sc_info.imageFormat = chosen_format.format;
	sc_info.imageColorSpace = chosen_format.colorSpace;
	sc_info.imageExtent = extent;
	sc_info.imageArrayLayers = 1;
	sc_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	sc_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	sc_info.preTransform = surf_caps.currentTransform;
	sc_info.compositeAlpha = choose_composite_alpha(surf_caps.supportedCompositeAlpha);
	sc_info.presentMode = chosen_present_mode;
	sc_info.clipped = VK_TRUE;
	sc_info.oldSwapchain = VK_NULL_HANDLE;

	swapchain = VK_NULL_HANDLE;
	vr = vkCreateSwapchainKHR(device, &sc_info, NULL, &swapchain);
	if (vr != VK_SUCCESS)
		die_vk("vkCreateSwapchainKHR failed", vr);

	swap_image_count = 0;
	vr = vkGetSwapchainImagesKHR(device, swapchain, &swap_image_count, NULL);
	if (vr != VK_SUCCESS)
		die_vk("vkGetSwapchainImagesKHR(count) failed", vr);
	if (swap_image_count == 0) {
		fprintf(stderr, "ERROR: swapchain returned 0 images\n");
		exit(1);
	}
	swap_images = (VkImage *)calloc((size_t)swap_image_count, sizeof(*swap_images));
	swap_image_views = (VkImageView *)calloc((size_t)swap_image_count, sizeof(*swap_image_views));
	if (!swap_images || !swap_image_views) {
		fprintf(stderr, "ERROR: out of memory allocating swapchain image arrays\n");
		exit(1);
	}
	vr = vkGetSwapchainImagesKHR(device, swapchain, &swap_image_count, swap_images);
	if (vr != VK_SUCCESS)
		die_vk("vkGetSwapchainImagesKHR(list) failed", vr);

	for (i = 0; i < swap_image_count; i++) {
		VkImageViewCreateInfo iv_info;

		memset(&iv_info, 0, sizeof(iv_info));
		iv_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		iv_info.image = swap_images[i];
		iv_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		iv_info.format = chosen_format.format;
		iv_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		iv_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		iv_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		iv_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		iv_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		iv_info.subresourceRange.baseMipLevel = 0;
		iv_info.subresourceRange.levelCount = 1;
		iv_info.subresourceRange.baseArrayLayer = 0;
		iv_info.subresourceRange.layerCount = 1;

		swap_image_views[i] = VK_NULL_HANDLE;
		vr = vkCreateImageView(device, &iv_info, NULL, &swap_image_views[i]);
		if (vr != VK_SUCCESS)
			die_vk("vkCreateImageView failed", vr);
	}

	/* ---- Render pass ---- */
	{
		VkAttachmentDescription color_att;
		VkAttachmentReference color_ref;
		VkSubpassDescription subpass;
		VkSubpassDependency dep;
		VkRenderPassCreateInfo rp_info;

		memset(&color_att, 0, sizeof(color_att));
		color_att.format = chosen_format.format;
		color_att.samples = VK_SAMPLE_COUNT_1_BIT;
		color_att.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		color_att.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color_att.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		color_att.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		color_att.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		color_att.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		memset(&color_ref, 0, sizeof(color_ref));
		color_ref.attachment = 0;
		color_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		memset(&subpass, 0, sizeof(subpass));
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &color_ref;

		memset(&dep, 0, sizeof(dep));
		dep.srcSubpass = VK_SUBPASS_EXTERNAL;
		dep.dstSubpass = 0;
		dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		memset(&rp_info, 0, sizeof(rp_info));
		rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		rp_info.attachmentCount = 1;
		rp_info.pAttachments = &color_att;
		rp_info.subpassCount = 1;
		rp_info.pSubpasses = &subpass;
		rp_info.dependencyCount = 1;
		rp_info.pDependencies = &dep;

		render_pass = VK_NULL_HANDLE;
		vr = vkCreateRenderPass(device, &rp_info, NULL, &render_pass);
		if (vr != VK_SUCCESS)
			die_vk("vkCreateRenderPass failed", vr);
	}

	/* ---- Pipeline ---- */
	{
		VkShaderModuleCreateInfo sm_info;
		VkPipelineShaderStageCreateInfo stages[2];
		VkVertexInputBindingDescription bind_desc;
		VkVertexInputAttributeDescription attr_desc;
		VkPipelineVertexInputStateCreateInfo vi;
		VkPipelineInputAssemblyStateCreateInfo ia;
		VkViewport viewport;
		VkRect2D scissor;
		VkPipelineViewportStateCreateInfo vp;
		VkPipelineRasterizationStateCreateInfo rs;
		VkPipelineMultisampleStateCreateInfo ms;
		VkPipelineColorBlendAttachmentState cb_att;
		VkPipelineColorBlendStateCreateInfo cb;
		VkPipelineLayoutCreateInfo pl_info;
		VkGraphicsPipelineCreateInfo gp_info;

		memset(&sm_info, 0, sizeof(sm_info));
		sm_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		sm_info.codeSize = sizeof(kTriVertSpv);
		sm_info.pCode = kTriVertSpv;
		vert_module = VK_NULL_HANDLE;
		vr = vkCreateShaderModule(device, &sm_info, NULL, &vert_module);
		if (vr != VK_SUCCESS)
			die_vk("vkCreateShaderModule(vert) failed", vr);

		sm_info.codeSize = sizeof(kTriFragSpv);
		sm_info.pCode = kTriFragSpv;
		frag_module = VK_NULL_HANDLE;
		vr = vkCreateShaderModule(device, &sm_info, NULL, &frag_module);
		if (vr != VK_SUCCESS)
			die_vk("vkCreateShaderModule(frag) failed", vr);

		memset(stages, 0, sizeof(stages));
		stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		stages[0].module = vert_module;
		stages[0].pName = "main";
		stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		stages[1].module = frag_module;
		stages[1].pName = "main";

		memset(&bind_desc, 0, sizeof(bind_desc));
		bind_desc.binding = 0;
		bind_desc.stride = sizeof(float) * 2;
		bind_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		memset(&attr_desc, 0, sizeof(attr_desc));
		attr_desc.location = 0;
		attr_desc.binding = 0;
		attr_desc.format = VK_FORMAT_R32G32_SFLOAT;
		attr_desc.offset = 0;

		memset(&vi, 0, sizeof(vi));
		vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vi.vertexBindingDescriptionCount = 1;
		vi.pVertexBindingDescriptions = &bind_desc;
		vi.vertexAttributeDescriptionCount = 1;
		vi.pVertexAttributeDescriptions = &attr_desc;

		memset(&ia, 0, sizeof(ia));
		ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		ia.primitiveRestartEnable = VK_FALSE;

		memset(&viewport, 0, sizeof(viewport));
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)extent.width;
		viewport.height = (float)extent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		memset(&scissor, 0, sizeof(scissor));
		scissor.offset.x = 0;
		scissor.offset.y = 0;
		scissor.extent = extent;

		memset(&vp, 0, sizeof(vp));
		vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		vp.viewportCount = 1;
		vp.pViewports = &viewport;
		vp.scissorCount = 1;
		vp.pScissors = &scissor;

		memset(&rs, 0, sizeof(rs));
		rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rs.depthClampEnable = VK_FALSE;
		rs.rasterizerDiscardEnable = VK_FALSE;
		rs.polygonMode = VK_POLYGON_MODE_FILL;
		rs.cullMode = VK_CULL_MODE_NONE;
		rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rs.depthBiasEnable = VK_FALSE;
		rs.lineWidth = 1.0f;

		memset(&ms, 0, sizeof(ms));
		ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		memset(&cb_att, 0, sizeof(cb_att));
		cb_att.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		cb_att.blendEnable = VK_FALSE;

		memset(&cb, 0, sizeof(cb));
		cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		cb.attachmentCount = 1;
		cb.pAttachments = &cb_att;

		memset(&pl_info, 0, sizeof(pl_info));
		pl_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout = VK_NULL_HANDLE;
		vr = vkCreatePipelineLayout(device, &pl_info, NULL, &pipeline_layout);
		if (vr != VK_SUCCESS)
			die_vk("vkCreatePipelineLayout failed", vr);

		memset(&gp_info, 0, sizeof(gp_info));
		gp_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		gp_info.stageCount = 2;
		gp_info.pStages = stages;
		gp_info.pVertexInputState = &vi;
		gp_info.pInputAssemblyState = &ia;
		gp_info.pViewportState = &vp;
		gp_info.pRasterizationState = &rs;
		gp_info.pMultisampleState = &ms;
		gp_info.pColorBlendState = &cb;
		gp_info.layout = pipeline_layout;
		gp_info.renderPass = render_pass;
		gp_info.subpass = 0;
		pipeline = VK_NULL_HANDLE;
		vr = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &gp_info, NULL, &pipeline);
		if (vr != VK_SUCCESS)
			die_vk("vkCreateGraphicsPipelines failed", vr);

		vkDestroyShaderModule(device, vert_module, NULL);
		vkDestroyShaderModule(device, frag_module, NULL);
		vert_module = VK_NULL_HANDLE;
		frag_module = VK_NULL_HANDLE;
	}

	/* ---- Framebuffers ---- */
	framebuffers = (VkFramebuffer *)calloc((size_t)swap_image_count, sizeof(*framebuffers));
	if (!framebuffers) {
		fprintf(stderr, "ERROR: out of memory allocating framebuffers\n");
		exit(1);
	}
	for (i = 0; i < swap_image_count; i++) {
		VkImageView atts[1];
		VkFramebufferCreateInfo fb_info;

		atts[0] = swap_image_views[i];
		memset(&fb_info, 0, sizeof(fb_info));
		fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		fb_info.renderPass = render_pass;
		fb_info.attachmentCount = 1;
		fb_info.pAttachments = atts;
		fb_info.width = extent.width;
		fb_info.height = extent.height;
		fb_info.layers = 1;

		framebuffers[i] = VK_NULL_HANDLE;
		vr = vkCreateFramebuffer(device, &fb_info, NULL, &framebuffers[i]);
		if (vr != VK_SUCCESS)
			die_vk("vkCreateFramebuffer failed", vr);
	}

	/* ---- Vertex buffer ---- */
	{
		struct Vert2 { float x; float y; };
		struct Vert2 verts[3];
		VkBufferCreateInfo binfo;
		VkMemoryRequirements req;
		VkMemoryAllocateInfo ainfo;
		void *mapped;
		uint32_t mem_type;

		verts[0].x = 0.0f;  verts[0].y = -0.5f;
		verts[1].x = 0.5f;  verts[1].y = 0.5f;
		verts[2].x = -0.5f; verts[2].y = 0.5f;

		memset(&binfo, 0, sizeof(binfo));
		binfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		binfo.size = sizeof(verts);
		binfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		binfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		vbuf = VK_NULL_HANDLE;
		vr = vkCreateBuffer(device, &binfo, NULL, &vbuf);
		if (vr != VK_SUCCESS)
			die_vk("vkCreateBuffer failed", vr);

		vkGetBufferMemoryRequirements(device, vbuf, &req);
		mem_type = find_memory_type(chosen_phys, req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		memset(&ainfo, 0, sizeof(ainfo));
		ainfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		ainfo.allocationSize = req.size;
		ainfo.memoryTypeIndex = mem_type;

		vmem = VK_NULL_HANDLE;
		vr = vkAllocateMemory(device, &ainfo, NULL, &vmem);
		if (vr != VK_SUCCESS)
			die_vk("vkAllocateMemory failed", vr);

		vr = vkBindBufferMemory(device, vbuf, vmem, 0);
		if (vr != VK_SUCCESS)
			die_vk("vkBindBufferMemory failed", vr);

		mapped = NULL;
		vr = vkMapMemory(device, vmem, 0, sizeof(verts), 0, &mapped);
		if (vr != VK_SUCCESS)
			die_vk("vkMapMemory failed", vr);
		memcpy(mapped, verts, sizeof(verts));
		vkUnmapMemory(device, vmem);
	}

	/* ---- Commands ---- */
	{
		VkCommandPoolCreateInfo cp_info;
		VkCommandBufferAllocateInfo cba;
		VkCommandBufferBeginInfo cb_begin;
		VkClearValue clear;
		VkRenderPassBeginInfo rp_begin;
		VkDeviceSize vb_offset;

		memset(&cp_info, 0, sizeof(cp_info));
		cp_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cp_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		cp_info.queueFamilyIndex = chosen_queue_family;
		cmd_pool = VK_NULL_HANDLE;
		vr = vkCreateCommandPool(device, &cp_info, NULL, &cmd_pool);
		if (vr != VK_SUCCESS)
			die_vk("vkCreateCommandPool failed", vr);

		cmd_bufs = (VkCommandBuffer *)calloc((size_t)swap_image_count, sizeof(*cmd_bufs));
		if (!cmd_bufs) {
			fprintf(stderr, "ERROR: out of memory allocating command buffers\n");
			exit(1);
		}

		memset(&cba, 0, sizeof(cba));
		cba.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cba.commandPool = cmd_pool;
		cba.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cba.commandBufferCount = swap_image_count;
		vr = vkAllocateCommandBuffers(device, &cba, cmd_bufs);
		if (vr != VK_SUCCESS)
			die_vk("vkAllocateCommandBuffers failed", vr);

		memset(&cb_begin, 0, sizeof(cb_begin));
		cb_begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		memset(&clear, 0, sizeof(clear));
		clear.color.float32[0] = 0.1f;
		clear.color.float32[1] = 0.1f;
		clear.color.float32[2] = 0.1f;
		clear.color.float32[3] = 1.0f;

		memset(&rp_begin, 0, sizeof(rp_begin));
		rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		rp_begin.renderPass = render_pass;
		rp_begin.renderArea.offset.x = 0;
		rp_begin.renderArea.offset.y = 0;
		rp_begin.renderArea.extent = extent;
		rp_begin.clearValueCount = 1;
		rp_begin.pClearValues = &clear;

		vb_offset = 0;
		for (i = 0; i < swap_image_count; i++) {
			rp_begin.framebuffer = framebuffers[i];
			vr = vkBeginCommandBuffer(cmd_bufs[i], &cb_begin);
			if (vr != VK_SUCCESS)
				die_vk("vkBeginCommandBuffer failed", vr);

			vkCmdBeginRenderPass(cmd_bufs[i], &rp_begin, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdBindPipeline(cmd_bufs[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
			vkCmdBindVertexBuffers(cmd_bufs[i], 0, 1, &vbuf, &vb_offset);
			vkCmdDraw(cmd_bufs[i], 3, 1, 0, 0);
			vkCmdEndRenderPass(cmd_bufs[i]);

			vr = vkEndCommandBuffer(cmd_bufs[i]);
			if (vr != VK_SUCCESS)
				die_vk("vkEndCommandBuffer failed", vr);
		}
	}

	/* ---- Sync + one present ---- */
	{
		VkSemaphoreCreateInfo si;
		VkFenceCreateInfo fi;
		VkPipelineStageFlags wait_stage;
		VkSubmitInfo submit;
		VkPresentInfoKHR pres;

		memset(&si, 0, sizeof(si));
		si.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		sem_image_available = VK_NULL_HANDLE;
		sem_render_finished = VK_NULL_HANDLE;
		vr = vkCreateSemaphore(device, &si, NULL, &sem_image_available);
		if (vr != VK_SUCCESS)
			die_vk("vkCreateSemaphore(image_available) failed", vr);
		vr = vkCreateSemaphore(device, &si, NULL, &sem_render_finished);
		if (vr != VK_SUCCESS)
			die_vk("vkCreateSemaphore(render_finished) failed", vr);

		memset(&fi, 0, sizeof(fi));
		fi.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fi.flags = 0;
		fence_in_flight = VK_NULL_HANDLE;
		vr = vkCreateFence(device, &fi, NULL, &fence_in_flight);
		if (vr != VK_SUCCESS)
			die_vk("vkCreateFence failed", vr);

		image_index = 0;
		vr = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, sem_image_available, VK_NULL_HANDLE, &image_index);
		if (vr != VK_SUCCESS)
			die_vk("vkAcquireNextImageKHR failed", vr);

		wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		memset(&submit, 0, sizeof(submit));
		submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit.waitSemaphoreCount = 1;
		submit.pWaitSemaphores = &sem_image_available;
		submit.pWaitDstStageMask = &wait_stage;
		submit.commandBufferCount = 1;
		submit.pCommandBuffers = &cmd_bufs[image_index];
		submit.signalSemaphoreCount = 1;
		submit.pSignalSemaphores = &sem_render_finished;

		vr = vkQueueSubmit(queue, 1, &submit, fence_in_flight);
		if (vr != VK_SUCCESS)
			die_vk("vkQueueSubmit failed", vr);

		memset(&pres, 0, sizeof(pres));
		pres.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		pres.waitSemaphoreCount = 1;
		pres.pWaitSemaphores = &sem_render_finished;
		pres.swapchainCount = 1;
		pres.pSwapchains = &swapchain;
		pres.pImageIndices = &image_index;
		vr = vkQueuePresentKHR(queue, &pres);
		if (vr != VK_SUCCESS)
			die_vk("vkQueuePresentKHR failed", vr);

		vr = vkWaitForFences(device, 1, &fence_in_flight, VK_TRUE, UINT64_MAX);
		if (vr != VK_SUCCESS)
			die_vk("vkWaitForFences failed", vr);
	}

	printf("Vulkan smoke OK: rendered and presented one triangle\n");

	/* Cleanup (best-effort; keep it simple). */
	vkDeviceWaitIdle(device);

	vkDestroyFence(device, fence_in_flight, NULL);
	vkDestroySemaphore(device, sem_render_finished, NULL);
	vkDestroySemaphore(device, sem_image_available, NULL);

	if (cmd_bufs)
		free(cmd_bufs);
	if (cmd_pool)
		vkDestroyCommandPool(device, cmd_pool, NULL);

	if (vbuf)
		vkDestroyBuffer(device, vbuf, NULL);
	if (vmem)
		vkFreeMemory(device, vmem, NULL);

	if (framebuffers) {
		for (i = 0; i < swap_image_count; i++)
			vkDestroyFramebuffer(device, framebuffers[i], NULL);
		free(framebuffers);
	}
	if (pipeline)
		vkDestroyPipeline(device, pipeline, NULL);
	if (pipeline_layout)
		vkDestroyPipelineLayout(device, pipeline_layout, NULL);
	if (render_pass)
		vkDestroyRenderPass(device, render_pass, NULL);

	if (swap_image_views) {
		for (i = 0; i < swap_image_count; i++)
			vkDestroyImageView(device, swap_image_views[i], NULL);
		free(swap_image_views);
	}
	if (swap_images)
		free(swap_images);
	if (swapchain)
		vkDestroySwapchainKHR(device, swapchain, NULL);

	vkDestroyDevice(device, NULL);
	vkDestroySurfaceKHR(instance, surface, NULL);
	vkDestroyInstance(instance, NULL);

	free(sdl_exts);
	sdl_exts = NULL;

	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
