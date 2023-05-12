project('xfce-py-plugin', 'c')

dependencies = [
	dependency('gtk+-3.0'),
	dependency('libxfce4panel-2.0'),
	meson.get_compiler('c').find_library('dl', required: true),
]

plugin_path = get_option('prefix') + '/' + get_option('libdir') + '/xfce4/panel-plugins'
plugin_id = 'python-sample-plugin'

config = configuration_data()
config.set('PLUGIN_ID', plugin_id)

configure_file(input: 'config.h.in',
	output: 'config.h',
	configuration: config)

library(plugin_id, 'src/plugin.c', dependencies: dependencies,
	install: true,
	install_dir: plugin_path)

install_data('python-sample-plugin.svg', install_dir: get_option('prefix') + '/' + get_option('datadir') + '/icons/hicolor/scalable/apps/')
install_data('plugin.desktop', install_dir: get_option('prefix') + '/' + get_option('datadir') + '/xfce4/panel/plugins/', rename: plugin_id + '.desktop')
