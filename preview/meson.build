rpicam_app_src += files([
    'null_preview.cpp',
    'preview.cpp',
])

preview_headers = files([
    'preview.hpp',
])

enable_egl = false
x11_deps = dependency('x11', required : get_option('enable_egl'))
epoxy_deps = dependency('epoxy', required : get_option('enable_egl'))

if x11_deps.found() and epoxy_deps.found()
    rpicam_app_dep += [x11_deps, epoxy_deps]
    rpicam_app_src += files('egl_preview.cpp')
    cpp_arguments += '-DLIBEGL_PRESENT=1'
    enable_egl = true
endif

install_headers(preview_headers, subdir: meson.project_name() / 'preview')
