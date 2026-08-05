del /a /f /s /q "*.log"
del /a /f /s /q .\chips\*.ini
del /a /f /s /q "iot.toml"
del /a /f /s /q "iot_linux.toml"
del /a /f /s /q "mcu.toml"
del /a /f /s /q "storage.toml"
del /a /f /s /q "partition.bin"
del /a /f /s /q "ro_params.dtb"
del /a /f /s /q "efusedata.bin"
del /a /f /s /q "efusedata_mask.bin"
rd /s /Q .\utils\flash
rd /s /Q .\chips\img
del /a /f /s /q .\chips\qcc743\img_create\*.bin
del /a /f /s /q .\chips\qcc743\img_create_iot\*.bin
del /a /f /s /q .\chips\qcc743\img_create_mcu\*.bin
del /a /f /s /q .\chips\qcc743\img_create_linux\*.bin
del /a /f /s /q .\chips\*.pack
rd /s /Q .\chips\qcc743\ota
rd /s /Q .\chips\qcc743\img_create
del /a /f /s /q ".DS_Store"
del /a /f /s /q "._.DS_Store"
exit 