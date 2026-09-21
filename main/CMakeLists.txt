idf_component_register(SRCS "recive.c"
                            "hardware/drivers/a7169/A7169.c"
                            "service/rf_service.c"
                       PRIV_REQUIRES esp_driver_gpio driver
                       INCLUDE_DIRS "."
                                    "hardware"
                                    "hardware/drivers"
                                    "hardware/drivers/a7169"
                                    "hardware/peripherals"
                                    "middleware"
                                    "service"
                                    "components")

# A7169 驱动放宽编译器警告检查
set_source_files_properties("hardware/drivers/a7169/A7169.c" PROPERTIES COMPILE_FLAGS "-Wno-error=return-type -Wno-error=maybe-uninitialized -Wno-error=unused-variable")
