project(
    'kbarinfo',
    'c',
    version: '0.1.0',
    license: 'GPL-3.0-or-later',
    default_options: ['c_std=c23', 'b_lto=true', 'b_asneeded=true', 'b_ndebug=if-release', 'warning_level=3', 'b_pie=true'],
)

cc = meson.get_compiler('c')

extra_args = cc.get_supported_arguments('-fstack-protector-strong', '-fstack-clash-protection', '-fcf-protection=full')
add_project_arguments(extra_args, language: 'c')
add_project_arguments('-D_FORTIFY_SOURCE=3', language: 'c')

extra_link_args = cc.get_supported_link_arguments(
    '-Wl,--as-needed',
    '-Wl,--sort-common',
    '-Wl,-O1',
    '-Wl,-z,now',
    '-Wl,-z,relro',
    '-Wl,-z,noexecstack',
)
add_project_link_arguments(extra_link_args, language: 'c')

deps = [
    dependency('glib-2.0'),
    dependency('gobject-2.0'),
    dependency('gio-unix-2.0'),
    dependency('libpulse-mainloop-glib'),
    dependency('libnm'),
    dependency('json-glib-1.0'),
]

subdir('src/')

executable('kbarinfo', kbarinfo_sources, dependencies: deps)
