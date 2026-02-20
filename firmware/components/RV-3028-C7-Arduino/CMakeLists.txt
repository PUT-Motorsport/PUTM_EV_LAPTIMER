    idf_component_register(
        SRCS
            "src/RV3028C7.cpp"
        INCLUDE_DIRS "src/."
        REQUIRES "arduino-esp32"
    )
    project(RV3028C7)
