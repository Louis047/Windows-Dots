local wezterm = require 'wezterm'
local config = wezterm.config_builder()

-- Font configuration
config.font = wezterm.font 'DepartureMono Nerd Font Propo'
config.font_size = 10

-- Use PowerShell Core (pwsh) as default shell
config.default_prog = { 'pwsh', '-NoLogo' }

-- Skip Exit Confirmation
config.window_close_confirmation = 'NeverPrompt'

-- Window padding
config.window_padding = {
  left = 14,
  right = 14,
  top = 14,
  bottom = 14,
}

-- Hide titlebar and tabs
config.window_decorations = 'RESIZE'
config.enable_tab_bar = false

-- Detect Windows OS theme and apply appropriate color scheme
local appearance = wezterm.gui.get_appearance()
if appearance:find 'Dark' then
  config.color_scheme = 'Dark+'
else
  config.color_scheme = 'Github Light (Gogh)'
end

-- Fix for tui-text editors visual artifacts
config.enable_kitty_keyboard = true

return config