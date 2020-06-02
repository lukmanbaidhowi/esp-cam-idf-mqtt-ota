idf_component_register(SRCS "app-main.c"
                            "uart_task.c"
                            "wifi_ota.c"
                            "mqtt_task.c"
                            "esp_cam.c"
                    INCLUDE_DIRS "."
                                 "${IDF_PATH}/components/wpa_supplicant/"
					EMBED_TXTFILES  ${project_dir}/main/server_root_cert.pem
                    )

