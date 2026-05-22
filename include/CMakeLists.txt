install(
    FILES
        asdf/gwcs/backend.h
        asdf/gwcs/core.h
        asdf/gwcs/eval.h
        asdf/gwcs/fitswcs_imaging.h
        asdf/gwcs/frame.h
        asdf/gwcs/frame2d.h
        asdf/gwcs/frame_celestial.h
        asdf/gwcs/gwcs.h
        asdf/gwcs/step.h
        asdf/gwcs/transform.h
        asdf/gwcs/wcs.h

    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/asdf/gwcs"
)

install(
    FILES
        asdf/gwcs/transform/affine.h
        asdf/gwcs/transform/compose.h
        asdf/gwcs/transform/concatenate.h
        asdf/gwcs/transform/constant.h
        asdf/gwcs/transform/divide.h
        asdf/gwcs/transform/identity.h
        asdf/gwcs/transform/polynomial.h
        asdf/gwcs/transform/remap_axes.h
        asdf/gwcs/transform/rotate_sequence_3d.h
        asdf/gwcs/transform/scale.h
        asdf/gwcs/transform/shift.h
        asdf/gwcs/transform/spherical_cartesian.h
        asdf/gwcs/transform/transform.h

    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/asdf/gwcs/transform"
)

install(
    FILES
        asdf/gwcs/transform/property/bounding_box.h

    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/asdf/gwcs/transform/property"
)
