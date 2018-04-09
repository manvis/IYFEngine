--[[This file should contain the default values for all configuration values and comments describing their function]]

-- TODO FIXME a lot of these are simply ignored because the engine uses hardcoded values.

--################[[ Engine configuration ]]
-- This is passed to Vulkan when constructing a VkApplicationInfo and MUST NOT BE LOCALIZED 
Engine["engine_name"] = "IYFEngine"
-- This is passed to Vulkan when constructing a VkApplicationInfo and MUST NOT BE LOCALIZED 
Engine["application_name"] = "IYFEditor"

-- Should the engine enable Vulkan's debug and validation. This option requires the user to have the Vulkan SDK installed
-- and set up. Even if this is off, debug and validation can be forced externally (check the Vulkan SDK docs)
Engine["debug_and_validation"] = false

-- List of all Vulkan validation layers that the engine will try to load. Once again, can be overridden externally.
Engine["validation_layers"] = [[
VK_LAYER_GOOGLE_threading
VK_LAYER_LUNARG_parameter_validation
VK_LAYER_LUNARG_object_tracker
VK_LAYER_LUNARG_core_validation
VK_LAYER_GOOGLE_unique_objects
]]
--VK_LAYER_LUNARG_swapchain

-- When this is true, all screens and their supported resolutions will be written to the log file with verbose priority
Engine["dump_screen_resolutions"] = true

-- These next several settings enable or disable specific Vulkan debug callback flags
Engine["vulkan_debug_information_flag"] = false
Engine["vulkan_debug_warning_flag"] = true
Engine["vulkan_debug_performance_warning_flag"] = true
Engine["vulkan_debug_error_flag"] = true
Engine["vulkan_debug_debug_flag"] = false


--################[[ Graphics configuration ]]
Graphics["width"] = 1920
Graphics["height"] = 1080
-- 0 - window, 1 - borderless window, 2 - exclusive full screen
Graphics["window_mode"] = 2
Graphics["use_vulkan"] = true
-- MSAA only works with forward renderers and should otherwise be ignored
Graphics["MSAA"] = 4
-- TODO why did I expose this?
Graphics["present_mode"] = 0
Graphics["anisotropy_level"] = 16

--################[[ Sound configuration ]]
-- 128 is max, 0 is silent
Sound["music_volume"] = 128
-- 128 is max, 0 is silent
Sound["sfx_volume"] = 128

--################[[ Control configuration ]]
Controls["mouse_sensitivity"] = 0.0007

--################[[ Gameplay configuration ]]
Gameplay["fov"] = 45

--################[[ Localization configuration ]]
Localization["text_locale"] = "en_US"
Localization["voice_locale"] = "en_US"

--################[[ Editor configuration ]]
Editor["user_first_name"] = ""
Editor["user_middle_name"] = ""
Editor["user_last_name"] = ""
Editor["user_nickname"] = ""
Editor["user_job"] = ""
Editor["user_email"] = ""
Editor["auto_load_last"] = false
Editor["previously_opened_project_0"] = ""
Editor["previously_opened_project_time_0"] = ""
Editor["previously_opened_project_1"] = ""
Editor["previously_opened_project_time_1"] = ""
Editor["previously_opened_project_2"] = ""
Editor["previously_opened_project_time_2"] = ""
Editor["previously_opened_project_3"] = ""
Editor["previously_opened_project_time_3"] = ""
Editor["previously_opened_project_4"] = ""
Editor["previously_opened_project_time_4"] = ""
Editor["previously_opened_project_5"] = ""
Editor["previously_opened_project_time_5"] = ""
Editor["previously_opened_project_6"] = ""
Editor["previously_opened_project_time_6"] = ""
Editor["previously_opened_project_7"] = ""
Editor["previously_opened_project_time_7"] = ""
Editor["previously_opened_project_8"] = ""
Editor["previously_opened_project_time_8"] = ""
Editor["previously_opened_project_9"] = ""
Editor["previously_opened_project_time_9"] = ""
