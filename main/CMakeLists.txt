idf_component_register(SRCS "recive.c"
                            "A7169/A7169.c"
                       PRIV_REQUIRES esp_driver_gpio driver
                       INCLUDE_DIRS "." "A7169")

set_source_files_properties("A7169/A7169.c" PROPERTIES COMPILE_FLAGS "-Wno-error=return-type -Wno-error=maybe-uninitialized")
