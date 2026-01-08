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

int main(int argc, char **argv)
{
	SDL_Window *window;
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
	float queue_priority;
	VkBool32 present_supported;
	uint32_t qf_count;
	VkQueueFamilyProperties *qf_props;
	uint32_t i;
	uint32_t qf;

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

	memset(&dev_info, 0, sizeof(dev_info));
	dev_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	dev_info.queueCreateInfoCount = 1;
	dev_info.pQueueCreateInfos = &q_info;
	dev_info.enabledExtensionCount = 0;
	dev_info.ppEnabledExtensionNames = NULL;
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

	printf("Vulkan smoke OK: instance+surface+device created (queue family %u)\n", (unsigned)chosen_queue_family);

	vkDeviceWaitIdle(device);
	vkDestroyDevice(device, NULL);
	vkDestroySurfaceKHR(instance, surface, NULL);
	vkDestroyInstance(instance, NULL);

	free(sdl_exts);
	sdl_exts = NULL;

	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
