rpicam_app_src += files([
    'encoder.cpp',
    'h264_encoder.cpp',
    'mjpeg_encoder.cpp',
    'null_encoder.cpp',
])

encoder_headers = files([
    'encoder.hpp',
    'h264_encoder.hpp',
    'mjpeg_encoder.hpp',
    'null_encoder.hpp',
])

# OpenCV based postprocessing stages.
enable_opencv = false
opencv_dep = dependency('opencv4', required : get_option('enable_opencv'))
if opencv_dep.found()
    rpicam_app_src += files([
      'chessboard_filter_encoder.cpp',
    ])

    encoder_headers += files([
        'chessboard_filter_encoder.hpp',
    ])
    rpicam_app_dep += opencv_dep
    enable_opencv = true
endif

install_headers(encoder_headers, subdir: meson.project_name() / 'encoder')
