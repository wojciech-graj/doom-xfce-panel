# xfce4-python-sample-plugin
A Sample/Template project to demonstrate the implementation of Xfce4 Panel plugin in Python.

# Requirements
- meson
- libgtk3-dev
- libxfce4panel-2.0-dev
- python-gi-dev

# How To Use
To use this repository for contructing a full fledged plugin you need to
- Change the `plugin_id` in `meson.build` with Your Unique Plugin Name
- Make sure the python plugin exists as `src/<plugin_id>.py`
- Code your plugin in `src/<plugin_id>.py`
- Make sure to instruct Meson to copy extra Python files you may wrote. Eg: \
  `install_data('src/utils.py', install_dir: py_plugin_path + '/src')` \
  if you happen to create a `utils.py` next to `src/<PLUGIN_ID>.py` and plan to import it

# Building
Make sure you have installed [Meson](https://mesonbuild.com/Quick-guide.html).
To setup the build environment, run the following in your repository root:

```shell
meson setup --prefix=/usr builddir
```

The name `builddir` is arbitrary. You can choose whatever name you'd like. Intermediate build files of Meson will end up there.

To compile and install your plugin, run:

```shell
sudo meson install -C builddir
```

Sudo privileges are most likely required, as we're asking Meson to install files to `/usr/lib/xfce4/`.

Make sure to restart your panel instance afterwards: `xfce4-panel -r`

The plugin should appear as an item in the panel's "Add items..." dialog.

# Debugging
If you encounter problems, for instance, your installed plugin does not show up in the "Add Items" dialog, the logs of `xfce4-panel` may shed some light. To see them, start the panel with debug output enabled:

```shell
PANEL_DEBUG=1 xfce4-panel
```

You may have to kill the running instance first (`xfce4-panel -q`).